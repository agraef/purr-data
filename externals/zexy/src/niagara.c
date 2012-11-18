/******************************************************
 *
 * zexy - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1999:forum::für::umläute:2004
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

/* 3108:forum::für::umläute:2000 */

#include "zexy.h"

/* ------------------------- niagara ------------------------------- */

/*
  divides a package into 2 sub-packages at a specified point
  like the niagara-falls, some water goes down to the left side, the rest to the right side, devided by the rock

  nowadays you can do this with [list split] (though this cannot handle negative indices)
*/

static t_class *niagara_class;

typedef struct _niagara
{
  t_object x_obj;
  t_float rock;
  t_outlet *left, *right;
} t_niagara;

static void niagara_list(t_niagara *x, t_symbol *s, int argc, t_atom *argv)
{
  int n_l, n_r;
  t_atom *ap_l, *ap_r;
  int dumrock = x->rock;
  int rock = ((dumrock < 0.f)?(argc+dumrock):dumrock);

  n_l  = (rock < argc)?rock:argc;
  ap_l = argv;

  n_r  = argc - n_l;
  ap_r = &argv[n_l];

  if (n_r) outlet_list(x->right, s, n_r, ap_r);
  if (n_l) outlet_list(x->left, s, n_l, ap_l);
}

static void niagara_any(t_niagara *x, t_symbol *s, int argc, t_atom *argv)
{
  int n_l, n_r;
  t_atom *ap_l, *ap_r;
  t_symbol *s_r, *s_l;
  int dumrock = x->rock;
  int rock = ((dumrock < 0.f)?(argc+dumrock):dumrock-1);

  n_l  = (rock < argc)?rock:argc;
  ap_l = argv;
  s_l  = s;

  n_r  = argc - n_l;
  ap_r = &argv[n_l];

  if (n_r) {
    s_r = 0;
    if (ap_r->a_type == A_FLOAT) s_r = gensym("list");
    else {
      s_r = atom_getsymbol(ap_r);
      ap_r++;
      n_r--;
    }
    outlet_anything(x->right, s_r, n_r, ap_r);
  }

  if (n_l+1 ) outlet_anything(x->left, s_l, n_l, ap_l);
}

static void *niagara_new(t_floatarg f)
{
  t_niagara *x = (t_niagara *)pd_new(niagara_class);

  x->rock = f;

  x->left =  outlet_new(&x->x_obj, &s_list);
  x->right = outlet_new(&x->x_obj, &s_list);

  floatinlet_new(&x->x_obj, &x->rock);

  return (x);
}

void niagara_setup(void)
{
  niagara_class = class_new(gensym("niagara"), (t_newmethod)niagara_new, 
			    0, sizeof(t_niagara), 0, A_DEFFLOAT,  0);
  
  class_addlist    (niagara_class, niagara_list);
  class_addanything(niagara_class, niagara_any);

  zexy_register("niagara");
}
