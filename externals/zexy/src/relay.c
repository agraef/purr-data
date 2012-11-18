/******************************************************
 *
 * zexy - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1999:forum::für::umläute:2005
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

/* 
   (c) 2106:forum::für::umläute:2005

   "relay" is like "relay" but doesn't change the incoming list

   example:
     [foo bar( --> [relay foo] --> [bar(
     [foo bar( --> [relay foo] --> [foor bar(

   namings: 
     direct, channel, relay, steer, guide, ??

     in the meantime i choose [relay] (as in mail-relay)

*/


#include "zexy.h"

/* -------------------------- relay ------------------------------ */

static t_class *relay_class;

typedef struct _relayelement
{
    t_word e_w;
    t_outlet *e_outlet;
} t_relayelement;

typedef struct _relay
{
    t_object x_obj;
    t_atomtype x_type;
    t_int x_nelement;
    t_relayelement *x_vec;
    t_outlet *x_rejectout;
} t_relay;

static void relay_anything(t_relay *x, t_symbol *sel, int argc, t_atom *argv)
{
    t_relayelement *e;
    int nelement;
    if (x->x_type == A_SYMBOL) 
    {
      for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
        if (e->e_w.w_symbol == sel)
          {
            outlet_anything(e->e_outlet, sel, argc, argv);
            return;
          }
    }
    outlet_anything(x->x_rejectout, sel, argc, argv);
}

static void relay_list(t_relay *x, t_symbol *sel, int argc, t_atom *argv)
{
  t_relayelement *e;
  int nelement;
  if (x->x_type == A_FLOAT)
    {
      t_float f;
      if (!argc){
        outlet_bang(x->x_rejectout);
        return;
      }
      f = atom_getfloat(argv);
      for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
        if (e->e_w.w_float == f)
          {
            outlet_anything(e->e_outlet, sel, argc, argv);
            return;
          }
    }
  else    /* symbol arguments */
    {
      if (argc == 0)         /* no args: treat as "bang" */
        {
          for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
              if (e->e_w.w_symbol == &s_bang)
                {
                  outlet_bang(e->e_outlet);
                  return;
                }
            }
        }
      else if (argc>1)
        {
          for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            { 
              if (e->e_w.w_symbol == &s_list)
                {
                  outlet_anything(e->e_outlet, sel, argc, argv);
                  return;
                }
            }
        }
        else if (argv[0].a_type == A_FLOAT)     /* one float arg */
        {
            for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
                if (e->e_w.w_symbol == &s_float)
                {
                    outlet_float(e->e_outlet, argv[0].a_w.w_float);
                    return;
                }
            }
        }
        else
        {
            for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
                if (e->e_w.w_symbol == &s_symbol)
                {
                    outlet_symbol(e->e_outlet, argv[0].a_w.w_symbol);
                    return;
                }
            }
        }
    }
  outlet_list(x->x_rejectout, &s_list, argc, argv);
}


static void relay_free(t_relay *x)
{
    freebytes(x->x_vec, x->x_nelement * sizeof(*x->x_vec));
}

static void *relay_new(t_symbol *s, int argc, t_atom *argv)
{
    int n;
    t_relayelement *e;
    t_relay *x = (t_relay *)pd_new(relay_class);
    t_atom a;
    ZEXY_USEVAR(s);
    if (argc == 0)
    {
        argc = 1;
        SETFLOAT(&a, 0);
        argv = &a;
    }
    x->x_type = argv[0].a_type;
    x->x_nelement = argc;
    x->x_vec = (t_relayelement *)getbytes(argc * sizeof(*x->x_vec));
    for (n = 0, e = x->x_vec; n < argc; n++, e++)
    {
        e->e_outlet = outlet_new(&x->x_obj, &s_list);
        if (x->x_type == A_FLOAT)
            e->e_w.w_float = atom_getfloatarg(n, argc, argv);
        else e->e_w.w_symbol = atom_getsymbolarg(n, argc, argv);
    }
    x->x_rejectout = outlet_new(&x->x_obj, &s_list);
    return (x);
}

void relay_setup(void)
{
    relay_class = class_new(gensym("relay"), (t_newmethod)relay_new,
        (t_method)relay_free, sizeof(t_relay), 0, A_GIMME, 0);
    class_addlist(relay_class, relay_list);
    class_addanything(relay_class, relay_anything);
    zexy_register("relay");
}
