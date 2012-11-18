/******************************************************
 *
 * dmxout - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   0603:forum::für::umläute:2008
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

#include "dmx4pd.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <pthread.h>


static t_class *dmxout_class;
static t_class *dmxout_class2;

#define NUM_DMXVALUES 512

static pthread_t g_thread_id;
static pthread_mutex_t *g_mutex;
static dmx_t g_values[NUM_DMXVALUES];
static int g_device;
static int g_thread_running, g_thread_continue;

typedef struct _dmxout
{
  t_object x_obj;

  t_inlet *x_portinlet;
  t_float  x_port;
  int  x_portrange;


} t_dmxout;


static void *dmxout_thread(void*you)
{
  pthread_mutex_t *mutex=g_mutex;
  struct timeval timout;

  g_thread_running=1;

  while(g_thread_continue) {
    timout.tv_sec = 0;
    timout.tv_usec=100;
    select(0,0,0,0,&timout);

    pthread_mutex_lock(g_mutex);
    if(g_device>0) {
      lseek (g_device, 0, SEEK_SET);  /* set to the current channel */
      write (g_device, g_values, NUM_DMXVALUES); /* write the channel */
    }
    pthread_mutex_unlock(g_mutex);
  }
  g_thread_running=0;

  return NULL;
}

static void dmxout_close()
{
  if(g_device>=0) {
    close(g_device);
  }
  g_device=-1;

  if(g_thread_running) {
    /* terminate the current thread! */
    void*dummy=0;
    int counter=0;
    g_thread_continue=0;
    pthread_join(g_thread_id, &dummy);
    while(g_thread_running) {
      counter++;
    }
  }
  g_thread_id=0;
  if(g_mutex) {
    pthread_mutex_destroy(g_mutex);
    freebytes(g_mutex, sizeof(pthread_mutex_t));
    g_mutex=NULL;
  }
}


static void dmxout_open(t_symbol*s_devname)
{
  int argc=2;
  const char *args[2] = {"--dmx", s_devname->s_name};
  const char**argv=args;
  const char*devname="";
  int fd;

  dmxout_close();

  if(s_devname && s_devname->s_name)
    devname=s_devname->s_name;

  //  strncpy(args[0], "--dmx", MAXPDSTRING);
  //  strncpy(args[1], devname, MAXPDSTRING);
  verbose(2, "[dmxout]: trying to open '%s'", args[1]);
  devname=DMXdev(&argc, argv);
  if(!devname){
  	error("couldn't find DMX device");
	return;
  }
  verbose(1, "[dmxout] opening %s", devname);

  fd = open (devname, O_WRONLY);

  if(fd!=-1) {
    g_device=fd;

    g_thread_running=0;
    g_thread_continue=0;
    g_mutex=(pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if ( pthread_mutex_init(g_mutex, NULL) < 0 ) {
      error("couldn't create mutex");
    } else {
      g_thread_continue = 1;
      pthread_create(&g_thread_id, 0, dmxout_thread, NULL);
    }
  } else {
    error("failed to open DMX-device '%s'",devname);
  }
}

static void dmxout_doout(t_dmxout*x) {
  if(g_device<=0) {
    pd_error(x, "no DMX universe found");
    return;
  }
}


static void dmxout_doout1(t_dmxout*x, short port, unsigned char value)
{
  g_values[port]=value;
  dmxout_doout(x);
}


static void dmxout_float(t_dmxout*x, t_float f)
{
  unsigned char val=(unsigned char)f;
  short port = (short)x->x_port;
  if(f<0. || f>255.) {
    pd_error(x, "value %f out of bounds [0..255]", f);
    return;
  }
  if(x->x_port<0. || x->x_port>NUM_DMXVALUES) {
    pd_error(x, "port %f out of bounds [0..%d]", x->x_port, NUM_DMXVALUES);
    return;
  }

  dmxout_doout1(x, port, val);
}

static void dmxout_list(t_dmxout*x, t_symbol*s, int argc, t_atom*argv)
{
  int count=(argc<x->x_portrange)?argc:x->x_portrange;
  int i=0;
  int errors=0;

  int port=x->x_port;
  if((port+count)>=NUM_DMXVALUES) {
    if(count>NUM_DMXVALUES)count=NUM_DMXVALUES;
    port=NUM_DMXVALUES-count;
  }

  for(i=0; i<count; i++) {
    t_float f=atom_getfloat(argv+i);
    if(f<0. || f>255.) {
      errors++;
      if(f<0.)f=0.;
      if(f>255)f=255;
    }
    g_values[port+i]=(unsigned char)f;
  }
  if(errors) {
    pd_error(x, "%d valu%s out of bound [0..255]", errors, (1==errors)?"e":"es");
  }

  dmxout_doout(x);
}

static void dmxout_port(t_dmxout*x, t_float f_baseport, t_floatarg f_portrange)
{
  short baseport =(short)f_baseport;
  short portrange=(short)f_portrange;


  if(baseport<0 || baseport>=NUM_DMXVALUES) {
    pd_error(x, "port %f out of bounds [0..%d]", f_baseport, NUM_DMXVALUES);
    baseport =0;
  }
  x->x_port = baseport;

  if(portrange<0) {
    pd_error(x, "portrange %f<0! setting to 1", portrange);
    portrange=1;
  } else if (portrange==0) {
    portrange=x->x_portrange;
  }

  if (baseport+portrange>NUM_DMXVALUES) {
    pd_error(x, "upper port exceeds %d! clamping", NUM_DMXVALUES);
    portrange=NUM_DMXVALUES-baseport;
  }
  x->x_portrange=portrange;
}

static void *dmxout_new(t_symbol*s, int argc, t_atom*argv)
{
  t_floatarg baseport=0.f, portrange=0.f;
  t_dmxout *x = 0;

  switch(argc) {
  case 2:
    x=(t_dmxout *)pd_new(dmxout_class2);
    x->x_portinlet=inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("port"));
    baseport=atom_getfloat(argv);
    portrange=atom_getfloat(argv+1);
    dmxout_port(x, baseport, portrange);
    break;
  case 1:
    baseport=atom_getfloat(argv);
  case 0:
    x=(t_dmxout *)pd_new(dmxout_class);
    x->x_portinlet=floatinlet_new(&x->x_obj, &x->x_port);
    x->x_port  = baseport;
    x->x_portrange = -1;
    break;
  default:
    return 0;
  }
  return (x);
}

static void *dmxout_free(t_dmxout*x)
{
  //  dmxout_close();
}

static void dmxout_init(void) {
  int i=0;
  g_thread_id=0;
  g_mutex=NULL;
  for(i=0; i<NUM_DMXVALUES; i++) g_values[i]=0;

  g_device=-1;
  g_thread_running=0;
  g_thread_continue=0;

  dmxout_open(gensym(""));
}


void dmxout_setup(void)
{

#ifdef DMX4PD_POSTBANNER
  DMX4PD_POSTBANNER;
#endif

  dmxout_class = class_new(gensym("dmxout"), (t_newmethod)dmxout_new, (t_method)dmxout_free,
                           sizeof(t_dmxout), 
                           0,
                           A_GIMME, A_NULL);

  class_addfloat(dmxout_class, dmxout_float);

  dmxout_class2 = class_new(gensym("dmxout"), (t_newmethod)dmxout_new, (t_method)dmxout_free,
			    sizeof(t_dmxout), 
			    0,
			    A_GIMME, A_NULL);

  class_addlist(dmxout_class2, dmxout_list);


  class_addmethod(dmxout_class2, (t_method)dmxout_port, gensym("port"), 
		  A_FLOAT, A_DEFFLOAT, A_NULL);


  dmxout_init();
}
