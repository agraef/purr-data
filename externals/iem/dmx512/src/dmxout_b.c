/******************************************************
 *
 * dmxout_b - implementation file
 *
 * this is the "blocking" version
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

static t_class *dmxout_b_class;
static t_class *dmxout_b_class2;

#define NUM_DMXVALUES 512

typedef struct _dmxout_b
{
  t_object x_obj;

  t_inlet *x_portinlet;

  int      x_device;
  t_float  x_port;
  int  x_portrange;

  dmx_t x_values[NUM_DMXVALUES];
} t_dmxout_b;

static void dmxout_b_clearbuf(t_dmxout_b*x)
{
  int i=0;
  for(i=0; i<NUM_DMXVALUES; i++) x->x_values[i]=0;
}

static void dmxout_b_close(t_dmxout_b*x)
{
  if(x->x_device>=0) {
    close(x->x_device);
  }
  x->x_device=-1;
}


static void dmxout_b_open(t_dmxout_b*x, t_symbol*s_devname)
{
  int argc=2;
  const char *args[2] = {"--dmx", s_devname->s_name};
  const char**argv=args;
  const char*devname="";
  int fd;

  dmxout_b_close(x);

  if(s_devname && s_devname->s_name)
    devname=s_devname->s_name;

  verbose(2, "[dmxout_b]: trying to open '%s'", args[1]);
  devname=DMXdev(&argc, argv);
  if(!devname){
  	pd_error(x, "couldn't find DMX device");
	return;
  }
  verbose(1, "[dmxout_b] opening %s", devname);

  fd = open (devname, O_WRONLY | O_NONBLOCK);

  if(fd!=-1) {
    x->x_device=fd;
    dmxout_b_clearbuf(x);
  } else {
    pd_error(x, "failed to open DMX-device '%s'",devname);
  }
}

static void dmxout_b_doout(t_dmxout_b*x) {
  int device = x->x_device;
  if(device<=0) {
    pd_error(x, "no DMX universe found");
    return;
  }

  lseek (device, 0, SEEK_SET);  /* set to the current channel */
  write (device, x->x_values, NUM_DMXVALUES); /* write the channel */
}


static void dmxout_b_doout1(t_dmxout_b*x, short port, unsigned char value)
{
  x->x_values[port]=value;
  dmxout_b_doout(x);
}


static void dmxout_b_float(t_dmxout_b*x, t_float f)
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

  dmxout_b_doout1(x, port, val);
}

static void dmxout_b_list(t_dmxout_b*x, t_symbol*s, int argc, t_atom*argv)
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
    x->x_values[port+i]=(unsigned char)f;
  }
  if(errors) {
    pd_error(x, "%d valu%s out of bound [0..255]", errors, (1==errors)?"e":"es");
  }

  dmxout_b_doout(x);
}

static void dmxout_b_port(t_dmxout_b*x, t_float f_baseport, t_floatarg f_portrange)
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

static void *dmxout_b_new(t_symbol*s, int argc, t_atom*argv)
{
  t_floatarg baseport=0.f, portrange=0.f;
  t_dmxout_b *x = 0;

  switch(argc) {
  case 2:
    x=(t_dmxout_b *)pd_new(dmxout_b_class2);
    x->x_portinlet=inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("port"));
    baseport=atom_getfloat(argv);
    portrange=atom_getfloat(argv+1);
    dmxout_b_port(x, baseport, portrange);
    break;
  case 1:
    baseport=atom_getfloat(argv);
  case 0:
    x=(t_dmxout_b *)pd_new(dmxout_b_class);
    x->x_portinlet=floatinlet_new(&x->x_obj, &x->x_port);
    x->x_port  = baseport;
    x->x_portrange = -1;
    break;
  default:
    return 0;
  }
  x->x_device=-1;

  dmxout_b_open(x, gensym(""));
  return (x);
}

static void *dmxout_b_free(t_dmxout_b*x)
{
  dmxout_b_close(x);
}


void dmxout_b_setup(void)
{
#ifdef DMX4PD_POSTBANNER
  DMX4PD_POSTBANNER;
#endif

  dmxout_b_class = class_new(gensym("dmxout_b"), (t_newmethod)dmxout_b_new, (t_method)dmxout_b_free,
                           sizeof(t_dmxout_b), 
                           0,
                           A_GIMME, A_NULL);

  class_addfloat(dmxout_b_class, dmxout_b_float);
  class_addmethod(dmxout_b_class, (t_method)dmxout_b_open, gensym("open"), A_SYMBOL, A_NULL);

  dmxout_b_class2 = class_new(gensym("dmxout_b"), (t_newmethod)dmxout_b_new, (t_method)dmxout_b_free,
			    sizeof(t_dmxout_b), 
			    0,
			    A_GIMME, A_NULL);

  class_addlist(dmxout_b_class2, dmxout_b_list);


  class_addmethod(dmxout_b_class2, (t_method)dmxout_b_port, gensym("port"), 
		  A_FLOAT, A_DEFFLOAT, A_NULL);

  class_addmethod(dmxout_b_class2, (t_method)dmxout_b_open, gensym("open"), A_SYMBOL, A_NULL);
}
