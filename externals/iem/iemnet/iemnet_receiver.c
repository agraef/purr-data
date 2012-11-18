/* iemnet
 *
 * receiver
 *   receives data "chunks" from a socket
 *   possibly threaded
 *
 *  copyright (c) 2010 IOhannes m zmölnig, IEM
 */

/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc.,                                                            */
/* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.                  */
/*                                                                              */

#define DEBUGLEVEL 4

#include "iemnet.h"
#include "iemnet_data.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <pthread.h>

#define INBUFSIZE 65536L /* was 4096: size of receiving data buffer */


struct _iemnet_receiver {
  pthread_t thread;
  int sockfd; /* owned outside; you must call iemnet__receiver_destroy() before freeing socket yourself */
  void*userdata;
  t_iemnet_chunk*data;
  t_iemnet_receivecallback callback;
  t_iemnet_queue*queue;
  t_clock *clock;


  int newdataflag;
  int running;
  int keepreceiving;

  pthread_mutex_t newdata_mtx, running_mtx, keeprec_mtx;
};

/* notifies Pd that there is new data to fetch */
static void iemnet_signalNewData(t_iemnet_receiver*x) {
  int already=0;
  int trylock=0;
  pthread_mutex_lock(&x->newdata_mtx);
   already=x->newdataflag;
   x->newdataflag=1;

   /* don't schedule ticks at the end of life */
   if(x->sockfd<0)already=1;

  pthread_mutex_unlock(&x->newdata_mtx);

  if(already) {
    return;
  }

  /*
   * try to lock Pd's main mutex
   *  this is bound to deadlock if this function is called from within Pd's mainthread
   *  (which happens when we destroy the receiver and signalNewData is called on cleanup)
   *
   * - shan't we check whether sys_trylock() returns EBUSY ?
   */
  trylock=sys_trylock();
  switch(trylock) {
  case 0:
  case EBUSY:
    if(x->clock)clock_delay(x->clock, 0);
    if(0==trylock)sys_unlock();
  default:
    break;
  }
}


/* the workhorse of the family */
static void*iemnet__receiver_readthread(void*arg) {
  unsigned int i=0;
  int result = 0;
  t_iemnet_receiver*receiver=(t_iemnet_receiver*)arg;

  int sockfd=receiver->sockfd;
  t_iemnet_queue*q=receiver->queue;

  unsigned char data[INBUFSIZE];
  unsigned int size=INBUFSIZE;

  struct sockaddr_in  from;
  socklen_t           fromlen = sizeof(from);

  int recv_flags=0;

  struct timeval timout;
  fd_set readset;
  FD_ZERO(&readset);
  FD_SET(sockfd, &readset);

  for(i=0; i<size; i++)data[i]=0;
  pthread_mutex_lock(&receiver->running_mtx);
   receiver->running=1;
  pthread_mutex_unlock(&receiver->running_mtx);

  while(1) {
    t_iemnet_chunk*c=NULL;

    pthread_mutex_lock(&receiver->keeprec_mtx);
     if(!receiver->keepreceiving) {
      pthread_mutex_unlock(&receiver->keeprec_mtx);
      break;
     }
    pthread_mutex_unlock(&receiver->keeprec_mtx);

    fromlen = sizeof(from);
    fd_set rs=readset;
    timout.tv_sec=0;
    timout.tv_usec=1000;

    recv_flags|=MSG_DONTWAIT;
    select(sockfd+1, &rs, NULL, NULL,
           &timout);
    if (!FD_ISSET(sockfd, &rs))continue;

    DEBUG("select can read");

    //fprintf(stderr, "reading %d bytes...\n", size);
    //result = recv(sockfd, data, size, 0);

    result = recvfrom(sockfd, data, size, recv_flags, (struct sockaddr *)&from, &fromlen);
    //fprintf(stderr, "read %d bytes...\n", result);
    DEBUG("recfrom %d bytes", result);
    if(result<=0)break;
    c= iemnet__chunk_create_dataaddr(result, data, &from);
    DEBUG("pushing");  
    queue_push(q, c);
    DEBUG("signalling");  
    iemnet_signalNewData(receiver);
    DEBUG("rereceive");  
  }
  // oha
  DEBUG("readthread loop termination: %d", result);
  //if(result>=0)iemnet_signalNewData(receiver);

  pthread_mutex_lock(&receiver->running_mtx);
   receiver->running=0;
  pthread_mutex_unlock(&receiver->running_mtx);

  DEBUG("read thread terminated");
  return NULL;
}

/* callback from Pd's main thread to fetch queued data */
static void iemnet__receiver_tick(t_iemnet_receiver *x)
{
  int running=0, keepreceiving=0;
  // received data
  t_iemnet_chunk*c=queue_pop_noblock(x->queue);
	DEBUG("tick got chunk %p", c);

  while(NULL!=c) {
    (x->callback)(x->userdata, c);
    iemnet__chunk_destroy(c);
    c=queue_pop_noblock(x->queue);
  }
	DEBUG("tick cleanup");
  pthread_mutex_lock(&x->newdata_mtx);
   x->newdataflag=0;
  pthread_mutex_unlock(&x->newdata_mtx);

  pthread_mutex_lock(&x->running_mtx);
   running = x->running;
  pthread_mutex_unlock(&x->running_mtx);

	DEBUG("tick running %d", running);
  if(!running) {
    // read terminated
    pthread_mutex_lock(&x->keeprec_mtx);
     keepreceiving=x->keepreceiving;
    pthread_mutex_unlock(&x->keeprec_mtx);

    /* keepreceiving is set, if receiver is not yet in shutdown mode */
    if(keepreceiving) 
      x->callback(x->userdata, NULL);
  }
	DEBUG("tick DONE");
}

int iemnet__receiver_getsize(t_iemnet_receiver*x) {
  int size=-1;
  if(x && x->queue)
    size=queue_getsize(x->queue);

  return size;
}


t_iemnet_receiver*iemnet__receiver_create(int sock, void*userdata, t_iemnet_receivecallback callback) {
  static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
  t_iemnet_receiver*rec=(t_iemnet_receiver*)malloc(sizeof(t_iemnet_receiver));
  DEBUG("create new receiver for 0x%X:%d", userdata, sock);
  //fprintf(stderr, "new receiver for %d\t%x\t%x\n", sock, userdata, callback);
  if(rec) {
    t_iemnet_chunk*data=NULL;
    int res=0;

    data=iemnet__chunk_create_empty(INBUFSIZE);
    if(NULL==data) {
      iemnet__receiver_destroy(rec);
      DEBUG("create receiver failed");
      return NULL;
    }

    rec->keepreceiving=1;
    rec->sockfd=sock;
    rec->userdata=userdata;
    rec->data=data;
    rec->callback=callback;

    memcpy(&rec->newdata_mtx , &mtx, sizeof(pthread_mutex_t));
    memcpy(&rec->running_mtx , &mtx, sizeof(pthread_mutex_t));
    memcpy(&rec->keeprec_mtx , &mtx, sizeof(pthread_mutex_t));
    rec->newdataflag=0;
    rec->running=1;

    rec->queue = queue_create();
    rec->clock = clock_new(rec, (t_method)iemnet__receiver_tick);

    res=pthread_create(&rec->thread, 0, iemnet__receiver_readthread, rec);
  }
  //fprintf(stderr, "new receiver created\n");

  return rec;
}

void iemnet__receiver_destroy(t_iemnet_receiver*rec) {
  static int instance=0;
  int inst=instance++;

  int sockfd;
  DEBUG("[%d] destroy receiver %x", inst, rec);
  if(NULL==rec)return;
  pthread_mutex_lock(&rec->keeprec_mtx);
   if(!rec->keepreceiving) {
    pthread_mutex_unlock(&rec->keeprec_mtx);
    return;
   }
   rec->keepreceiving=0;
  pthread_mutex_unlock(&rec->keeprec_mtx);

  sockfd=rec->sockfd;

  DEBUG("joining thread");
  pthread_join(rec->thread, NULL);

  DEBUG("[%d] really destroying receiver %x -> %d", inst, rec, sockfd);

  if(sockfd>=0) {
    /* this doesn't alway make recvfrom() return!
     * - try polling
     * - try sending a signal with pthread_kill() ?
     */

    shutdown(sockfd, 2); /* needed on linux, since the recv won't shutdown on sys_closesocket() alone */
    sys_closesocket(sockfd); 
  }
  DEBUG("[%d] closed socket %d", inst, sockfd);

  rec->sockfd=-1;

  // empty the queue
  DEBUG("[%d] tick %d", inst, rec->running);
  iemnet__receiver_tick(rec);
  queue_destroy(rec->queue);  
  DEBUG("[%d] tack", inst);

  if(rec->data)iemnet__chunk_destroy(rec->data);

  pthread_mutex_destroy(&rec->newdata_mtx);
  pthread_mutex_destroy(&rec->running_mtx);
  pthread_mutex_destroy(&rec->keeprec_mtx);

  clock_free(rec->clock);
  rec->clock=NULL;

  rec->userdata=NULL;
  rec->data=NULL;
  rec->callback=NULL;
	rec->queue=NULL;

  free(rec);
  rec=NULL;
  DEBUG("[%d] destroyed receiver", inst);
}
