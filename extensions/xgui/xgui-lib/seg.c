/* Copyright (c) 2002 Damien HENRY.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* code for seg pd class NOT WORKING YET*/

#include "m_pd.h"
#include "g_canvas.h"


typedef struct s_pd_obj_seg
{
  t_object t_ob;
  t_symbol *seg_name;  
  t_float *posx;
  t_float *posy;
  t_float *x1;
  t_float *y1;
  t_float *x2;
  t_float *y2;
  t_float *width;
  t_symbol *color;
} t_seg;

typedef struct s_seg
{
  t_object t_ob;
  t_symbol* seg_name;  
  t_float posx;
  t_float posy;
  t_float x1;
  t_float y1;
  t_float x2;
  t_float y2;
  t_float width;
  t_symbol color;
} t_seg;

void seg_help(t_seg *x)
{
   post(" ");
   post("seg v001");
   post("+ symbol list :");
   post("++ help : this help !!!");
   post(" ");
}

void seg_width(t_seg *x, t_floatarg f)
{
  post("seg: width %f",f);
  t_atom my_atom ;
  t_atom *my_pointer = &my_atom;
  SETFLOAT(my_pointer, *f);
  outlet_anything(x->x_obj.ob_outlet, gensym("!width"), 1,my_pointer);
}


void seg_pos(t_seg *x, t_floatarg f1, t_floatarg f2)
{
   outlet_float(x->x_outlet1, (t_float)f1+(t_float)f2);
}

void seg_free(void)
{
    post("seg_free");
}

t_class *seg_class;

void *seg_new(void)
{
    t_seg *x = (t_seg *)pd_new(seg_class);
    post("seg created");
    
    sys_vgui(".x%x.c create oval 10 10 20 20\n",glist_getcanvas(glist),
			x->x_obj.te_xpos+1,x->x_obj.te_ypos+1,
			x->x_obj.te_xpos + x->x_width -1,
			x->x_obj.te_ypos + x->x_height -1,x->x_color->s_name,x);
    x->posx = 0; x->posy = 0;
    x->x1 = 10; x->y1 = 10;
    x->x1 = 20; x->y1 = 20;
    x->x_outlet1 = outlet_new(&x->t_ob, &s_float);
    
    return (void *)x;
}

void seg_setup(void)
{
    post("seg_setup");
    seg_class = class_new(gensym("seg"), (t_newmethod)seg_new,(t_method)seg_free, sizeof(t_seg), 0, A_GIMME, 0);
    class_addmethod(seg_class, (t_method)seg_width, gensym("width"),A_FLOAT, 0);
    class_addmethod(seg_class, (t_method)seg_pos, gensym("pos"),A_FLOAT,A_FLOAT, 0);
    class_addmethod(seg_class, (t_method)seg_help, gensym("help"), 0);
    class_sethelpsymbol(seg_class, gensym("xgui/help_seg"));
}

