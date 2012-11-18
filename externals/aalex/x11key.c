/**
 * [x11kry] Generates X11 keyboard events on Linux
 * @author Alexandre Quessy <alex@sourcelibre.com>
 * @license GNU Public License    )c( 2006
 */

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
// #include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "m_pd.h"

#define PUREX11_DEFAULT_KEYSTATE 0
#define PUREX11_DEFAULT_DISPLAY 0

/** variables of the pd object */
typedef struct x11key {
  t_object x_ob; /* The instance. Contains inlets and outlets */
  int *displayName;
} t_x11key;

/**
 * letter : types the letter you give it.
 */
void x11key_letter(t_x11key *x, t_symbol *s, int argc, t_atom *argv) {
  Display *display;
  KeySym keysym;
  KeyCode keycode;
  int result;
  
  if (argc >= 1) {
    if (argv[0].a_type == A_SYMBOL) {
      t_symbol *sym = atom_getsymbolarg(0, argc, argv);
      char *theChars = sym->s_name;
      //keysym = XK_q;
      keysym = XStringToKeysym(theChars);
      /* should be hostname:number.screen_number */
      display = XOpenDisplay(NULL); /* defaults to $DISPLAY */
      if (display == NULL) {
        post("Error : could not open display.\n");
      }else {
        // printf("Display opened successfully.\n");
        keycode = XKeysymToKeycode(display, keysym);
        //post("letter = %s", theChars);
        //post("keycode = %d", (int) keycode);
        /** Pushes on and off the letter */
        /*
        if (argc >= 2) {
          if (argv[1].a_type == A_FLOAT) {
            int isPressed = (atom_getfloatarg(0, argc, argv) == 0) ? true: false;
            result = XTestFakeKeyEvent(display, keycode, isPressed, 0);
            //post("result = %d", result);
            if (result == 0) {
              post("Error : could not simulate typing the letter.");
            }
          }
        } else { */
          // defaults to press and release
          result = XTestFakeKeyEvent(display, keycode, True, 0);
          //post("result = %d", result);
          if (result == 0) {
            post("Error : could not simulate typing the letter.");
          }
          result = XTestFakeKeyEvent(display, keycode, False, 0); 
          if (result == 0) {
            post("Error : could not simulate typing the letter.");
          }
        /* } //endif argc >= 2 */
        XCloseDisplay(display);
      }
    } else {
      post("Error : Bad argument type. Must be a symbol. Try [symbol 7< if you want to type a number.");
    }
  } else {
    post("Error : Missing argument. The letter to type.");
  }
}



/**
 * key : types the key symbol you give it. See /usr/include/X11/keysymdef.h
 */
void x11key_keysym(t_x11key *x, t_symbol *s, int argc, t_atom *argv) {
  Display *display;
  KeySym keysym;
  KeyCode keycode;
  int result;
  int state = PUREX11_DEFAULT_KEYSTATE;
  bool boolState;
  
  if (argc >= 2) {
    if (argv[1].a_type == A_FLOAT) { 
      state = (int) atom_getfloatarg(1, argc, argv);
    }
  }
  
  if (argc >= 1) {
    if (argv[0].a_type == A_SYMBOL) {
      t_symbol *sym = atom_getsymbolarg(0, argc, argv);
      char *theChars = sym->s_name;
      //keysym = XK_q;
      keysym = XStringToKeysym(theChars);
      /* should be hostname:number.screen_number */
      display = XOpenDisplay(NULL); /* defaults to $DISPLAY */
      if (display == NULL) {
        post("Error : could not open display.\n");
      }else {
        // printf("Display opened successfully.\n");
        keycode = XKeysymToKeycode(display, keysym);
        
        /** Pushes on and off the letter */
        boolState = (state == 0) ? False : True;
        result = XTestFakeKeyEvent(display, keycode, boolState, 0);
        if (result == 0) {
          post("Error : could not simulate typing the letter.");
        }
        XCloseDisplay(display);
      }
    } else {
      post("Error : Bad argument type. Must be a symbol. ");
    }
  } else {
    post("Error : Missing argument. The letter to type.");
  }
}



/** display */
void x11key_display(t_x11key *x, t_symbol *s, int argc, t_atom *argv) {
  post("Not yet implemented.");
}

/** help */
void x11key_help(t_x11key *x, t_symbol *s, int argc, t_atom *argv) {
  post("keysym [see /usr/include/X11/keysymdef.h for the key symbols]");
  post("letter [letter a-zA-Z0-9...]");
  post("help");
  post("display : not yet implemented");
}





/** The class */
t_class *x11key_class;

/** constructor */
void *x11key_new(t_symbol *selector, int argc, t_atom *argv) {
  t_x11key *x = (t_x11key *) pd_new(x11key_class);
  //x->displayName = PUREX11_DEFAULT_DISPLAY;
  return (void *)x;
}

/** setup */
void x11key_setup(void) {
  x11key_class = class_new(gensym("x11key"), (t_newmethod) x11key_new, 0, sizeof(t_x11key), 0, A_GIMME, 0);
  class_addmethod(x11key_class, (t_method)x11key_letter, gensym("letter"), A_GIMME, 0);
  class_addmethod(x11key_class, (t_method)x11key_keysym, gensym("keysym"), A_GIMME, 0);
  class_addmethod(x11key_class, (t_method)x11key_display, gensym("display"), A_GIMME, 0);
  class_addmethod(x11key_class, (t_method)x11key_help, gensym("help"), A_GIMME, 0);
  
  post("==============================================");
  post("                Pure X11");
  post("Copyleft 2006 Alexandre Quessy");
  post("GNU Public License");
  post("[x11key] simulates X11 keyboard events.");
  post("==============================================");
  
}
