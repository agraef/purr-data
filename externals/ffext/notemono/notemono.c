/* Copyright (c) 2005 Federico Ferri.
 * Release under the terms of GPL license.
 * Based on PureData by Miller Puckette and others. */

#include <stdio.h>
#include <stdlib.h>
#include "m_pd.h"

// change this if you have more than MAX_N fingers...
#define MAX_N 16

#define method(method_name) notemono ## _ ## method_name
#define t_self t_notemono
#define _self _notemono
#define self_class notemono_class
#define sz_classname "notemono"

#define MODE_MIN    0
#define MODE_MIN_S  "low"
#define MODE_MAX    1
#define MODE_MAX_S  "high"

typedef struct _self
{
    t_object       x_obj;
    t_outlet      *x_out2;
    int            x_note_status[MAX_N]; //array
    t_float        x_note_pitch;
    t_float        x_note_vel;
    t_float        x_out_midi;
    int            x_out_gate;
    int mode; // max or min ?
} t_self;

static t_class *self_class;

static void method(out)(t_self *x) {
    int j;
    t_float m = -1;
    int g = 0;
    for(j=0; j<MAX_N; j++) {
        if(x->x_note_status[j] >= 0) {
            switch(x->mode) {
                case MODE_MIN:
                    if(x->x_note_status[j] < m || m == -1) {
                        m = x->x_note_status[j];
                        g = 1;
                    }
                    break;
                case MODE_MAX:
                    if(x->x_note_status[j] > m || m == -1) {
                        m = x->x_note_status[j];
                        g = 1;
                    }
                    break;
            }
        }
    }
    if(m != x->x_out_midi || g != x->x_out_gate) {
        x->x_out_midi = m;
        x->x_out_gate = g;
        outlet_float(x->x_out2, g);
	    outlet_float(x->x_obj.ob_outlet, m);
    }
}

static void method(float)(t_self *x, t_float f) {
    int j;
    x->x_note_pitch = f;
    for(j=0; j<MAX_N; j++) {
	    if(x->x_note_status[j] == x->x_note_pitch) {
		    if(x->x_note_vel > 0) {
			    x->x_note_status[j] = x->x_note_pitch;
			    method(out)(x);
			    return;
		    } else {
			    x->x_note_status[j] = -1;
			    method(out)(x);
			    return;
		    }
	    }
    }
    if(x->x_note_vel > 0) {
        for(j=0; j<MAX_N; j++) {
            if(x->x_note_status[j] == -1 && x->x_note_vel > 0) {
                x->x_note_status[j] = x->x_note_pitch;
                method(out)(x);
                return;
            }
        }
    }
}

static void * method(new)(t_symbol* s, int ac, t_atom* av) {
    t_self *x;
    int err = 0;
    if(ac == 1) {
        x = (t_self *)pd_new(self_class);
        x->x_out_midi = -1;
        x->x_out_gate = 0;
        t_symbol *arg = atom_getsymbolarg(0, ac, av);
        if(gensym(MODE_MIN_S) == arg) {
            x->mode = MODE_MIN;
        } else if (gensym(MODE_MAX_S) == arg) {
            x->mode = MODE_MAX;
        } else {
            err = 1;
        }
    } else {
        err = 1;
    }
    if(err) {
        error(sz_classname ": wrong arguments."
            " possible values: " MODE_MIN_S " " MODE_MAX_S);
        return NULL;
    }
    int j;
    for(j=0; j<MAX_N; j++) x->x_note_status[j] = -1;
    outlet_new((t_object *)x, &s_float);
    floatinlet_new(&x->x_obj, &x->x_note_vel);
    x->x_out2 = outlet_new(&x->x_obj, gensym("float"));
    return (x);
}

void method(setup)(void) {
    self_class = class_new(gensym(sz_classname), 
			      (t_newmethod)method(new), 0,
			      sizeof(t_self), 0,
			      A_GIMME, 0);
    class_addfloat(self_class, method(float));
    class_sethelpsymbol(self_class, gensym(sz_classname));
}

