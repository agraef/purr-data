/******************************************************

   Very simple Mixer for RME DSP-MADI 
                     and maybe other hammerfall dsp 
   (C) 2003 IEM, Winfried Ritsch  (ritsch at iem.at)
   Institute of Electronic Music and Acoustics

   PD-External (see hdspmmixer-help.pd)

   institute of electronic music and acoustics (iem)


****************************************************

 license: GNU General Public License v.2

****************************************************/
#include "m_pd.h"
#include "hdspm_mixer.h"

/* hdspmmixer :: allows control of the mixer (controls) for the ALSA soundcard driver */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <getopt.h>
# include <stdarg.h>
# include <ctype.h>
# include <math.h>
# include <errno.h>
# include <assert.h>
#include <sys/ioctl.h>

#ifdef HAVE_ALSA
# include <alsa/asoundlib.h>
#endif /* ALSA */

static t_class *hdspmmixer_class;

typedef struct _hdspmmixer
{
  t_object x_obj;
  t_outlet*x_error;
} t_hdspmmixer;

static void hdspmmixer_bang(t_hdspmmixer *x)
{
	outlet_float(x->x_error,(float) find_cards());
}

static void hdspmmixer_get(t_hdspmmixer *x, t_symbol *s, int argc, t_atom *argv)
{
  int idx, src, dst,val;


  if (argc < 3 || A_FLOAT != argv->a_type || A_FLOAT != (argv+1)->a_type || A_FLOAT != (argv+2)->a_type ) {	
    error("hdspmmixer: set <float cardnr> <float source> <float destination>\n");
    /*   return -EINVAL;*/
  }

  idx = atom_getint(argv);
  src = atom_getint(argv+1);
  dst = atom_getint(argv+2);

  val = get_gain(idx,src,dst);

  if(val < 0)
	outlet_float(x->x_error,(float) val);
  else		
   	outlet_float(x->x_obj.ob_outlet,(float) val);

  /*  post("gain: %i",get_gain(idx,src,dst));*/
}

static void hdspmmixer_set(t_hdspmmixer *x, t_symbol *s, int argc, t_atom *argv)
{
  int idx, src, dst,val;

  if (argc < 4 || A_FLOAT != argv->a_type || A_FLOAT != (argv+1)->a_type || A_FLOAT != (argv+2)->a_type ) {	
    error("hdspmmixer: set <float cardnr> <float source> <float destination> <float value>\n");
    /* return -EINVAL; */
  }

  idx = atom_getint(argv);
  src = atom_getint(argv+1);
  dst = atom_getint(argv+2);
  val = atom_getint(argv+3);

  val = set_gain(idx,src,dst,val);

  if(val < 0)
	outlet_float(x->x_error,val);
  else		
   	outlet_float(x->x_obj.ob_outlet,(float) val);

/*  post("gain: %i",set_gain(idx,src,dst,val)); */
}

static void hdspmmixer_free(t_hdspmmixer *x){

	return;

}

static void *hdspmmixer_new(void)
{
  t_hdspmmixer *x = (t_hdspmmixer *)pd_new(hdspmmixer_class);
  outlet_new(&x->x_obj, 0);
  x->x_error=outlet_new(&x->x_obj, 0);

  return (x);
}

void hdspmmixer_setup(void)
{
  post("hdspmmixer: ALSA HDSP Mixer control");
  post("          Copyright (C) Winfried Ritsch");
  post("          institute of electronic music and acoustics (iem)");
  post("          published under the GNU General Public License version 2");
#ifdef VERSION
  startpost("          version:"VERSION);
#endif
  post("\tcompiled: "__DATE__"");

  hdspmmixer_class = class_new(gensym("hdspmmixer"), (t_newmethod)hdspmmixer_new, (t_method)hdspmmixer_free,
                           sizeof(t_hdspmmixer), 0, 0);

  class_addbang(hdspmmixer_class, (t_method)hdspmmixer_bang);
  class_addmethod(hdspmmixer_class, (t_method)hdspmmixer_bang,gensym("find"), 0);
  class_addmethod(hdspmmixer_class, (t_method)hdspmmixer_get,gensym("get"), A_GIMME, 0);
  class_addmethod(hdspmmixer_class, (t_method)hdspmmixer_set,gensym("set"), A_GIMME, 0);

  //  class_addmethod(hdspmmixer_class, (t_method)hdspmmixer_listdevices,gensym(""), A_DEFSYM, 0);
}
