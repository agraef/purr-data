/**
 * [x11mouse] Generates X11 mouse events on Linux
 * @author Alexandre Quessy <alex@sourcelibre.com>
 * @license GNU Public License    )c( 2006
 */

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "m_pd.h"

#define PUREX11_DEFAULT_MOUSEBUTTON 1
#define PUREX11_DEFAULT_MOUSESTATE 0
#define PUREX11_DEFAULT_DISPLAY 0
#define PUREX11_DEFAULT_XVALUE 0
#define PUREX11_DEFAULT_YVALUE 0


/** variables of the pd object */
typedef struct x11mouse {
  t_object x_ob; /* The instance. Contains inlets and outlets */
  int *displayName;
} t_x11mouse;



/** 
 * Mouse click
 * @param int Button number
 */
void x11mouse_click(t_x11mouse *x, t_symbol *s, int argc, t_atom *argv) {
  Display *display;
  int result;
  
  int button = PUREX11_DEFAULT_MOUSEBUTTON;
  
  display = XOpenDisplay(0); /* defaults to $DISPLAY if 0 */
  if (display == NULL) {
    post("[x11mouse] error : could not open display.");
  }
  if (argc >= 1) {
    if (argv[0].a_type == A_FLOAT) { 
      button = (int) atom_getfloatarg(0, argc, argv);
    }
  }
  
  result = XTestFakeButtonEvent(display, button, True, CurrentTime);
  result = XTestFakeButtonEvent(display, button, False, CurrentTime);
  XCloseDisplay(display);
}

/** 
 * Mouse button press or release
 * @param int State. Defaults to 0.
 * @param int Button number
 */
void x11mouse_press(t_x11mouse *x, t_symbol *s, int argc, t_atom *argv) {
  Display *display;
  int result;
  
  int button = PUREX11_DEFAULT_MOUSEBUTTON;
  int state = PUREX11_DEFAULT_MOUSESTATE;
  bool boolState;
  
  display = XOpenDisplay(0); /* defaults to $DISPLAY if 0 */
  if (display == NULL) {
    post("[x11mouse] error : could not open display.");
  }
  
  if (argc >= 2) {
    if (argv[1].a_type == A_FLOAT) { 
      button = (int) atom_getfloatarg(1, argc, argv);
    }
    if (argv[0].a_type == A_FLOAT) { 
      state = (int) atom_getfloatarg(0, argc, argv);
    }
  } else if (argc >= 1) {
    if (argv[0].a_type == A_FLOAT) { 
      state = (int) atom_getfloatarg(0, argc, argv);
    }
  }
  
  boolState = (state == 0) ? False : True;
  result = XTestFakeButtonEvent(display, button, boolState, CurrentTime);
  if (!result) {
    post("[x11mouse] error : could not fake button event.");
  }
  XCloseDisplay(display);
}

/**
 * Moves the mouse to the coordonates you give it.
 */
void x11mouse_move(t_x11mouse *x, t_symbol *s, int argc, t_atom *argv) {
  Display *display;
  int result;
  int xCoord = PUREX11_DEFAULT_XVALUE;
  int yCoord = PUREX11_DEFAULT_YVALUE;
  
  display = XOpenDisplay(0); /* defaults to $DISPLAY if 0 */
  if (display == NULL) {
    post("[x11mouse] error : could not open display.");
  }
  
  if (argc >= 2) {
    if (argv[1].a_type == A_FLOAT) { 
      yCoord = (int) atom_getfloatarg(0, argc, argv);
    }
    if (argv[0].a_type == A_FLOAT) { 
      xCoord = (int) atom_getfloatarg(0, argc, argv);
    }
  } else if (argc >= 1) {
    if (argv[0].a_type == A_FLOAT) { 
      xCoord = (int) atom_getfloatarg(0, argc, argv);
    }
  }
  
  result = XTestFakeMotionEvent(display, 0, xCoord, yCoord, CurrentTime);
  if (!result) {
    post("[x11mouse] error : could not fake motion event.");
  }
  XCloseDisplay(display);
}

/**
 * Sets the X11 display name.
 * Defaults to $DISPLAY, usually :0.0
 * 
 * Not currently implemented. You cannot change the display. (for now)
 * @todo Allow the user to change the display
 */
void x11mouse_display(t_x11mouse *x, t_symbol *s, int argc, t_atom *argv) {
  Display *display;
  display = XOpenDisplay(0); /* defaults to $DISPLAY if 0 */
  if (display == NULL) {
    post("[x11mouse] error : could not open display.");
  }
  XCloseDisplay(display);
}

void x11mouse_help(t_x11mouse *x, t_symbol *s, int argc, t_atom *argv) {
  post("press [state 0/1] [button number 1-3]");
  post("move [x] [y]");
  post("click [button number 1-3]");
  post("help");
  post("display not yet implemented");
  
  
  
}

/** The class */
t_class *x11mouse_class;

/** constructor */
void *x11mouse_new(t_symbol *selector, int argc, t_atom *argv) {
  t_x11mouse *x = (t_x11mouse *) pd_new(x11mouse_class);
  //x->displayName = PUREX11_DEFAULT_DISPLAY;
  return (void *)x;
}

/** setup */
void x11mouse_setup(void) {
  x11mouse_class = class_new(gensym("x11mouse"), (t_newmethod) x11mouse_new, 0, sizeof(t_x11mouse), 0, A_GIMME, 0);
  class_addmethod(x11mouse_class, (t_method)x11mouse_press, gensym("press"), A_GIMME, 0);
  class_addmethod(x11mouse_class, (t_method)x11mouse_move, gensym("move"), A_GIMME, 0);
  class_addmethod(x11mouse_class, (t_method)x11mouse_display, gensym("display"), A_GIMME, 0);
  class_addmethod(x11mouse_class, (t_method)x11mouse_click, gensym("click"), A_GIMME, 0);
  class_addmethod(x11mouse_class, (t_method)x11mouse_help, gensym("help"), A_GIMME, 0);
  
  post("==============================================");
  post("                Pure X11");
  post("Copyleft 2006 Alexandre Quessy");
  post("GNU Public License");
  post("[x11mouse] simulates X11 mouse events.");
  post("==============================================");
  
}
