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
 ******************************************************/


/* 1309:forum::für::umläute:2000 */

/*
  sort :  sort a package of floats
*/

#include "zexy.h"

/* ------------------------- sort ------------------------------- */

/*
  SHELL SORT: simple and easy
*/

static t_class *sort_class;

typedef struct _sort
{
  t_object x_obj;

  int bufsize;
  t_float *buffer;
  t_int   *indices;

  int ascending;

  t_outlet*indexOut, *sortedOut;
} t_sort;


static void sort_dir(t_sort *x, t_float f)
{
  x->ascending = (f < 0.f)?0:1;
}

static void sort_buffer(t_sort *x, int argc, t_atom *argv)
{
  int n = argc;
  t_float *buf;
  t_atom *atombuf = argv;

  if (argc != x->bufsize) {
    if (x->buffer) freebytes(x->buffer,  x->bufsize * sizeof(t_float));
    if (x->indices)freebytes(x->indices, x->bufsize * sizeof(t_int));

    x->bufsize = argc;
    x->buffer = getbytes(x->bufsize * sizeof(t_float));
    x->indices = getbytes(x->bufsize * sizeof(t_int));
  }

  buf = x->buffer;
  while (n--){
    *buf++ = atom_getfloat(atombuf++);
    x->indices[n] = n;
  }
}

static void sort_list(t_sort *x, t_symbol *s, int argc, t_atom *argv)
{
  int step = argc, n;
  t_atom *atombuf = (t_atom *)getbytes(sizeof(t_atom) * argc);
  t_float *buf;
  t_int   *idx;

  int i, loops = 1;

  sort_buffer(x, argc, argv);
  buf = x->buffer;
  idx = x->indices;

  while (step > 1) {
    step = (step % 2)?(step+1)/2:step/2;

    i = loops;
    loops += 2;

    while(i--) { /* there might be some optimization in here */
      for (n=0; n<(argc-step); n++) {
        if (buf[n] > buf[n+step]) {
          t_int   i_tmp = idx[n];
          t_float f_tmp = buf[n];
          buf[n]        = buf[n+step];
          buf[n+step]   = f_tmp;
          idx[n]        = idx[n+step];
          idx[n+step]   = i_tmp;
        }
      }
    }
  }

  if (x->ascending) 
    for (n = 0; n < argc; n++) SETFLOAT(&atombuf[n], idx[n]);
  else
    for (n = 0, i=argc-1; n < argc; n++, i--) SETFLOAT(&atombuf[n], idx[i]);

  outlet_list(x->indexOut , &s_list, n, atombuf);

  if (x->ascending) 
    for (n = 0; n < argc; n++) SETFLOAT(&atombuf[n], buf[n]);
  else
    for (n = 0, i=argc-1; n < argc; n++, i--) SETFLOAT(&atombuf[n], buf[i]);
  outlet_list(x->sortedOut, &s_list, n, atombuf);


  freebytes(atombuf, argc*sizeof(t_atom));
}

static void *sort_new(t_floatarg f)
{
  t_sort *x = (t_sort *)pd_new(sort_class);
  x->ascending = (f < 0.f)?0:1;

  x->sortedOut=outlet_new(&x->x_obj, &s_list);
  x->indexOut=outlet_new(&x->x_obj, &s_list);

  x->bufsize = 0;
  x->buffer = NULL;

  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("direction"));

  return (x);
}

static void sort_help(t_sort*x)
{
  post("\n%c sort\t\t:: sort a list of numbers", HEARTSYMBOL);
}
void sort_setup(void)
{
  sort_class = class_new(gensym("sort"), (t_newmethod)sort_new, 
                         0, sizeof(t_sort), 0, A_DEFFLOAT,  0);
  
  class_addlist    (sort_class, sort_list);
  class_addmethod   (sort_class, (t_method)sort_dir, gensym("direction"), A_DEFFLOAT, 0);
  class_addmethod(sort_class, (t_method)sort_help, gensym("help"), A_NULL);

  zexy_register("sort");
}
