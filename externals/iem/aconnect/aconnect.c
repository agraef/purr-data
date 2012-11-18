/******************************************************
 *
 * aconnect - ALSA sequencer connection manager
 * Copyright (C) 1999-2000 Takashi Iwai
 *
 *
 *
 * ported from alsa-1.0.9a to pd by:
 * copyleft (c) 2005 IOhannes m zmölnig
 *
 *   forum::für::umläute
 *
 *   institute of electronic music and acoustics (iem)
 *
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/
#include "m_pd.h"

/* aconnect :: connect/disconnect 2 ALSA sequencer subscriber ports */

static t_class *aconnect_class;

typedef struct _aconnect
{
  t_object x_obj;
  t_outlet*x_error;
} t_aconnect;


#ifdef HAVE_ALSA

#include <alsa/asoundlib.h>

#define LIST_INPUT	1
#define LIST_OUTPUT	2
#define ACONNECT_SEQ_NAME "default"

#define perm_ok(pinfo,bits) ((snd_seq_port_info_get_capability(pinfo) & (bits)) == (bits))

static int ac_count=0;
static snd_seq_t* ac_seq=0;

static int check_permission(snd_seq_port_info_t *pinfo, int perm)
{
  if (perm) {
    if (perm & LIST_INPUT) {
      if (perm_ok(pinfo, SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ))
	goto __ok;
    }
    if (perm & LIST_OUTPUT) {
      if (perm_ok(pinfo, SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE))
	goto __ok;
    }
    return 0;
  }
 __ok:
  if (snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_NO_EXPORT)
    return 0;
  return 1;
}

/*
 * search all ports
 */
typedef void (*action_func_t)(t_aconnect *x, 
			      snd_seq_t *seq, snd_seq_client_info_t *cinfo, snd_seq_port_info_t *pinfo);

static void do_search_port(t_aconnect *x, snd_seq_t *seq, int perm, action_func_t do_action)
{
  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;
  int count;
  snd_seq_client_info_alloca(&cinfo);
  snd_seq_port_info_alloca(&pinfo);
  snd_seq_client_info_set_client(cinfo, -1);
  while (snd_seq_query_next_client(seq, cinfo) >= 0) {
    /* reset query info */
    int senderport=snd_seq_client_info_get_client(cinfo);
    if(SND_SEQ_CLIENT_SYSTEM != senderport){ /* skipping port 0 */
      snd_seq_port_info_set_client(pinfo, senderport);
      snd_seq_port_info_set_port(pinfo, -1);
      count = 0;
      while (snd_seq_query_next_port(seq, pinfo) >= 0) {
        if (check_permission(pinfo, perm)) {
          do_action(x, seq, cinfo, pinfo);
          count++;
        }
      }
    }
  }
}

static void print_port(t_aconnect *x, t_symbol*s, snd_seq_t *seq, snd_seq_client_info_t *cinfo,
		       snd_seq_port_info_t *pinfo)
{
  t_atom ap[4];
  int client_id    =snd_seq_client_info_get_client(cinfo);
  int port_id      =snd_seq_port_info_get_port(pinfo);
  char *client_name=(char*)snd_seq_client_info_get_name(cinfo);
  char *port_name  =(char*)snd_seq_port_info_get_name(pinfo);

  SETFLOAT (ap+0, client_id);
  SETSYMBOL(ap+1, gensym(client_name));
  SETFLOAT (ap+2, port_id);
  SETSYMBOL(ap+3, gensym(port_name));

  outlet_anything(x->x_obj.ob_outlet, s, 4, ap);
}
static void print_output(t_aconnect *x, snd_seq_t *seq, snd_seq_client_info_t *cinfo,
		       snd_seq_port_info_t *pinfo)
{
  print_port(x, gensym("dst"), seq, cinfo, pinfo);
}

static void print_input(t_aconnect *x, snd_seq_t *seq, snd_seq_client_info_t *cinfo,
		       snd_seq_port_info_t *pinfo)
{
  print_port(x, gensym("src"), seq, cinfo, pinfo);
}

static void print_connections(t_aconnect *x, snd_seq_t *seq, snd_seq_client_info_t *cinfo,
				snd_seq_port_info_t *pinfo)
{
  int count = 0;
  int sender_id     =snd_seq_client_info_get_client(cinfo);
  int sender_port   =snd_seq_port_info_get_port(pinfo);
  int receiver_id   =-1;
  int receiver_port =-1;
  snd_seq_query_subscribe_t *subs;

  t_atom ap[4];

  SETFLOAT (ap+0, sender_id);
  SETFLOAT (ap+1, sender_port);

  snd_seq_query_subscribe_alloca(&subs);
  snd_seq_query_subscribe_set_root(subs, snd_seq_port_info_get_addr(pinfo));

  snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_READ);
  snd_seq_query_subscribe_set_index(subs, 0);
  while (snd_seq_query_port_subscribers(seq, subs) >= 0) {
    const snd_seq_addr_t *addr = snd_seq_query_subscribe_get_addr(subs);
    receiver_id   =addr->client;
    receiver_port =addr->port;

    SETFLOAT (ap+2, receiver_id);
    SETFLOAT (ap+3, receiver_port);

    outlet_list(x->x_obj.ob_outlet, 0, 4, ap);

    snd_seq_query_subscribe_set_index(subs, snd_seq_query_subscribe_get_index(subs) + 1);
  }
}


/*
 * remove all (exported) connections
 */
static void remove_connection(t_aconnect *x, snd_seq_t *seq, snd_seq_client_info_t *cinfo,
			      snd_seq_port_info_t *pinfo)
{
  snd_seq_query_subscribe_t *query;

  snd_seq_query_subscribe_alloca(&query);
  snd_seq_query_subscribe_set_root(query, snd_seq_port_info_get_addr(pinfo));

  snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_READ);
  snd_seq_query_subscribe_set_index(query, 0);
  for (; snd_seq_query_port_subscribers(seq, query) >= 0;
       snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1)) {
    snd_seq_port_info_t *port;
    snd_seq_port_subscribe_t *subs;
    const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_root(query);
    const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(query);
    snd_seq_port_info_alloca(&port);
    if (snd_seq_get_any_port_info(seq, dest->client, dest->port, port) < 0)
      continue;
    if (!(snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_SUBS_WRITE))
      continue;
    if (snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_NO_EXPORT)
      continue;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_queue(subs, snd_seq_query_subscribe_get_queue(query));
    snd_seq_port_subscribe_set_sender(subs, sender);
    snd_seq_port_subscribe_set_dest(subs, dest);
    snd_seq_unsubscribe_port(seq, subs);
  }

  snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_WRITE);
  snd_seq_query_subscribe_set_index(query, 0);
  for (; snd_seq_query_port_subscribers(seq, query) >= 0;
       snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1)) {
    snd_seq_port_info_t *port;
    snd_seq_port_subscribe_t *subs;
    const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_root(query);
    const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_addr(query);
    snd_seq_port_info_alloca(&port);
    if (snd_seq_get_any_port_info(seq, sender->client, sender->port, port) < 0)
      continue;
    if (!(snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_SUBS_READ))
      continue;
    if (snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_NO_EXPORT)
      continue;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_queue(subs, snd_seq_query_subscribe_get_queue(query));
    snd_seq_port_subscribe_set_sender(subs, sender);
    snd_seq_port_subscribe_set_dest(subs, dest);
    snd_seq_unsubscribe_port(seq, subs);
  }
}

static void remove_all_connections(t_aconnect *x, snd_seq_t *seq)
{
  do_search_port(x, seq, 0, remove_connection);
}


/*
 * list input and output clients in a format like
 * "output <client#> <clientsymbol> <#ports> <portsymbol>"
 * "input <client#> <clientsymbol> <#ports> <portsymbol>"
 */
static void aconnect_listdevices(t_aconnect *x, t_symbol *s)
{
  snd_seq_t *seq;
  if (((seq=ac_seq)==0) && snd_seq_open(&seq, ACONNECT_SEQ_NAME, SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    error("aconnect: can't open sequencer");
    outlet_float(x->x_error, (float)(-2));
    return;
  }
  if(&s_==s || gensym("src")==s)
    do_search_port(x, seq, LIST_INPUT, print_input);

  if(&s_==s || gensym("dst")==s)
    do_search_port(x, seq, LIST_OUTPUT, print_output);

  if(!ac_seq)snd_seq_close(seq);
  outlet_float(x->x_error, 0.);
}


/*
 * list connections in the form
 * "list <sender_client> <sender_port> <receiver_client> <receiver_port>"
 */
static void aconnect_bang(t_aconnect *x)
{
  snd_seq_t *seq;
  if (((seq=ac_seq)==0) && snd_seq_open(&seq, ACONNECT_SEQ_NAME, SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    error("aconnect: can't open sequencer");
    outlet_float(x->x_error, (float)(-2));
    return;
  }

  do_search_port(x, seq, LIST_INPUT, print_connections);

  if(!ac_seq)snd_seq_close(seq);
  outlet_float(x->x_error, 0.);
}

static int aconnect_subscribe(snd_seq_t*seq, int subscribe, 
			       int sender_id, int sender_port, int dest_id, int dest_port){
  snd_seq_port_subscribe_t *subs;
  snd_seq_addr_t sender, dest;
  int queue = 0, convert_time = 0, convert_real = 0, exclusive = 0;
  int err=0;

  sender.client=sender_id;
  sender.port  =sender_port;
  dest.client  =dest_id;
  dest.port    =dest_port;  

  /* set the information into "subs" */
  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_sender(subs, &sender);
  snd_seq_port_subscribe_set_dest(subs, &dest);

  snd_seq_port_subscribe_set_queue(subs, queue);
  snd_seq_port_subscribe_set_exclusive(subs, exclusive);
  snd_seq_port_subscribe_set_time_update(subs, convert_time);
  snd_seq_port_subscribe_set_time_real(subs, convert_real);

  /* do the subscription/unsubscription */
  if(subscribe){
    if (snd_seq_get_port_subscription(seq, subs) == 0) {
      post("Connection is already subscribed");
      err=1;
    } else if (snd_seq_subscribe_port(seq, subs) < 0) {
      error("aconnect: Connection failed (%s)", snd_strerror(errno));
      err=-1;
    }
  } else {
    if (snd_seq_get_port_subscription(seq, subs) < 0) {
      post("aconnect: no subscription is found");
      err=1;
    } else if (snd_seq_unsubscribe_port(seq, subs) < 0) {
      error("aconnect: Disonnection failed (%s)", snd_strerror(errno));
      err=-1;
    }
  }
  return err;
}

/* a list like "connect <sender_client> <sender_port> <receiver_client> <receiver_port>"
 * clients can be both numeric or symbolic
 * port>=0; 
 * if port==-1; then all ports are connected (TODO)
 * if numeric client == -1 then all clients are connected (TODO)
 */
static void aconnect_connect(t_aconnect *x, t_symbol *s, int argc, t_atom *argv)
{
  snd_seq_t *seq;
  int sender_id, sender_port, dest_id, dest_port;
  int err=0;

  if(argc!=4){
    pd_error(x, "aconnect: invalid connection string!");
    return;
  }

  if (((seq=ac_seq)==0) && snd_seq_open(&seq, ACONNECT_SEQ_NAME, SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    error("aconnect: can't open sequencer");
    outlet_float(x->x_error, (float)(-2));
    return;
  }

  /* get sender and dest */
  sender_id  =atom_getint(argv);
  sender_port=atom_getint(argv+1);
    dest_id  =atom_getint(argv+2);
    dest_port=atom_getint(argv+3);

  err=aconnect_subscribe(seq, 1, sender_id, sender_port, dest_id, dest_port);
  
  if(!ac_seq)snd_seq_close(seq);
  outlet_float(x->x_error, (float)err);
}
/* a list like "disconnect <sender_client> <sender_port> <receiver_client> <receiver_port>"
 * see aconnect_connect()
 */
static void aconnect_disconnect(t_aconnect *x, t_symbol *s, int argc, t_atom *argv)
{
  snd_seq_t *seq;
  int err;
  int sender_id, sender_port, dest_id, dest_port;

  if(argc!=4){
    pd_error(x, "aconnect: invalid connection string!");
    return;
  }

  if (((seq=ac_seq)==0) && snd_seq_open(&seq, ACONNECT_SEQ_NAME, SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    error("aconnect: can't open sequencer");
    outlet_float(x->x_error, (float)(-2));
    return;
  }

  /* get sender and dest */
  sender_id  =atom_getint(argv);
  sender_port=atom_getint(argv+1);
    dest_id  =atom_getint(argv+2);
    dest_port=atom_getint(argv+3);

  err=aconnect_subscribe(seq, 0, sender_id, sender_port, dest_id, dest_port);
  outlet_float(x->x_error, (float)err);

  if(!ac_seq)snd_seq_close(seq);
}
#endif /* ALSA */

static void aconnect_free(t_aconnect *x){
#ifdef HAVE_ALSA
  ac_count--;
  if(ac_count<=0){
    if(ac_seq)snd_seq_close(ac_seq);
    ac_seq=0;
  }
#endif /* ALSA */
}


static void *aconnect_new(void)
{
  t_aconnect *x = (t_aconnect *)pd_new(aconnect_class);
  outlet_new(&x->x_obj, 0);
  x->x_error=outlet_new(&x->x_obj, 0);

#ifndef HAVE_ALSA
  error("aconnect: compiled without ALSA-suppor !!");
  error("aconnect: no functionality enabled!");
#else
  if(ac_count<=0){
    ac_count=0;
    if (snd_seq_open(&ac_seq, ACONNECT_SEQ_NAME, SND_SEQ_OPEN_DUPLEX, 0) < 0){
      error("aconnect: can't open sequencer");
      ac_seq=0;
    }
  }
  ac_count++;

#endif /* !ALSA */

  return (x);
}


void aconnect_setup(void)
{
  post("aconnect: ALSA sequencer connection manager");
  post("          Copyright (C) 1999-2000 Takashi Iwai");
  post("          ported to pure-data by IOhannes m zmölnig 2005");
  post("          institute of electronic music and acoustics (iem)");
  post("          published under the GNU General Public License version 2");
#ifdef ACONNECT_VERSION
  startpost("          version:"ACONNECT_VERSION);
#endif
  post("\tcompiled: "__DATE__"");

  aconnect_class = class_new(gensym("aconnect"), (t_newmethod)aconnect_new, (t_method)aconnect_free,
			     sizeof(t_aconnect), 0, 0);
#ifdef HAVE_ALSA
  class_addmethod(aconnect_class, (t_method)aconnect_connect,gensym("connect"), A_GIMME, 0);
  class_addmethod(aconnect_class, (t_method)aconnect_disconnect,gensym("disconnect"), A_GIMME, 0);
  class_addmethod(aconnect_class, (t_method)aconnect_listdevices,gensym("devices"), A_DEFSYM, 0);
  class_addbang(aconnect_class, (t_method)aconnect_bang);
#endif /* ALSA */
}

