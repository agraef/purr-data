/**
 * [xmms] is a Pure Data object to control the X Multimedia System, an audio player.
 *
 * @url http://alexandre.quessy.net/
 * @author Alexandre Quessy <alexandre@quessy.net>
 * @license GNU General Public License 2006
 * thanks to Andy Gimblett for code examples
 */
/*
To know which flags to use for compilation: 
`xmms-config --cflags --libs`

On Debian, you need libxmms and xmms-festalon if you want to read nsf files.
*/

//#define _GNU_SOURCE

/* PD includes */
#include "m_pd.h"
#include <math.h>
// #include <string.h>
#include <stdlib.h>
// #include <stdio.h>

/* XMMS includes */
#include <xmms/xmmsctrl.h>
// #include <glib.h>
/* local constants */
#define PDXMMS_DEFAULTSKIP 5000


//need following declarations so internal XMMS functions can be called
//(ripped from XMMS source)
//from xmms.c
//gboolean playlist_load(gchar * inpipefile);


/** variables of the pd object */
typedef struct xmms {
  t_object x_ob; /* The instance. Contains inlets and outlets */
  /* xmms session. 0 is the first one when we start it */
  int session;
  //t_outlet *x_outlet;
} t_xmms;



void xmms_prev(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  xmms_remote_playlist_prev(x->session);
}

void xmms_next(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  xmms_remote_playlist_next(x->session);
}

void xmms_play(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  xmms_remote_play(x->session);
}

void xmms_pause(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  xmms_remote_pause(x->session);
}

void xmms_stop(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  xmms_remote_stop(x->session);
}

void xmms_volume(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  float f;
  int l, r;
  if (argc >= 1) {
    if (argv[0].a_type == A_FLOAT) { 
      f = (float) atom_getfloatarg(0, argc, argv);
      l = (int) f;
      //if (mute_flag) mute_flag = 0;
      if (l > 100) l = 100;
      else if (l < 0) l = 0;
      //if (r > 100) r = 100;
      //else if (r < 0) r = 0;
      r = f;
      xmms_remote_set_volume(x->session, l, r);
    }
  }
}


/* skipf and skipb */
void xmms_forward(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  int time = xmms_remote_get_output_time(x->session);
  double skip = PDXMMS_DEFAULTSKIP;
  
  if (argc >= 1) {
    if (argv[0].a_type == A_FLOAT) { 
      skip = (double) atom_getfloatarg(0, argc, argv);
    }
  }
  xmms_remote_jump_to_time(x->session, (int) rint(time + skip));
}

void xmms_backward(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  int time = xmms_remote_get_output_time(x->session);
  double skip = PDXMMS_DEFAULTSKIP;
  
  if (argc >= 1) {
    if (argv[0].a_type == A_FLOAT) { 
      skip = (double) atom_getfloatarg(0, argc, argv);
    }
  }
  xmms_remote_jump_to_time(x->session, (int) rint(time - skip));
}

/*
void xmms_load0(t_xmms *x, t_symbol *s, int argc, t_atom *argv) {
  if (argc >= 1) { 
    if (argv[0].a_type == A_SYMBOL) {
      t_symbol *tmp = atom_getsymbol(&argv[0]);
      playlist_load(tmp->s_name);
    }
  }
}
  */
/** The class */
t_class *xmms_class;

/** constructor */
void *xmms_new(t_symbol *selector, int argc, t_atom *argv) {
  t_xmms *x = (t_xmms *) pd_new(xmms_class);
  //x->x_outlet = outlet_new(&x->x_ob, gensym("symbol"));
  x->session = 0;
  // if (!xmms_remote_is_running(x->session)) return 0;
  return (void *)x;
}

/** setup */
void xmms_setup(void) {
  xmms_class = class_new(gensym("xmms"), (t_newmethod) xmms_new, 0, sizeof(t_xmms), 0, A_GIMME, 0);
  
  class_addmethod(xmms_class, (t_method)xmms_stop, gensym("stop"), A_GIMME, 0);
  class_addmethod(xmms_class, (t_method)xmms_play, gensym("play"), A_GIMME, 0);
  class_addmethod(xmms_class, (t_method)xmms_pause, gensym("pause"), A_GIMME, 0);
  class_addmethod(xmms_class, (t_method)xmms_next, gensym("next"), A_GIMME, 0);
  class_addmethod(xmms_class, (t_method)xmms_prev, gensym("prev"), A_GIMME, 0);
  class_addmethod(xmms_class, (t_method)xmms_forward, gensym("forward"), A_GIMME, 0);
  class_addmethod(xmms_class, (t_method)xmms_backward, gensym("backward"), A_GIMME, 0);
  class_addmethod(xmms_class, (t_method)xmms_volume, gensym("volume"), A_GIMME, 0);
  //class_addmethod(xmms_class, (t_method)xmms_load0, gensym("load"), A_GIMME, 0);
}

