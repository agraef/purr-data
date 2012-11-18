/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2010 */


#include "m_pd.h"
#include "iemlib.h"


/* ---------- sparse_FIR~ - sparse_FIR-filter with coef-matrix-message ----------- */
/* the list or matrix message should have an index and its value, index , value, aso.... */

typedef struct _sparse_FIR_tilde
{
  t_object  x_obj;
  t_float   *x_coef_beg;
  int       *x_index_beg;
  int       x_n_coef;
  int       x_n_coef_malloc;
  t_float   *x_history_beg;
  int       x_rw_index;
  int       x_sparse_FIR_order;
  t_float   x_msi;
} t_sparse_FIR_tilde;

static t_class *sparse_FIR_tilde_class;

static t_int *sparse_FIR_tilde_perform(t_int *w)
{
  t_float *in = (t_float *)(w[1]);
  t_float *out = (t_float *)(w[2]);
  t_sparse_FIR_tilde *x = (t_sparse_FIR_tilde *)(w[3]);
  int n = (t_int)(w[4]);
  int rw_index = x->x_rw_index;
  int i, j, ix;
  int order = x->x_sparse_FIR_order;
  int n_coef = x->x_n_coef;
  int n_coef8;
  t_float sum=0.0f;
  t_float *coef = x->x_coef_beg;
  int *index = x->x_index_beg;
  t_float *write_hist1=x->x_history_beg;
  t_float *write_hist2;
  t_float *read_hist;
  t_float *coef_vec;
  int *index_vec;
  t_float *hist_vec;
  
  if((order < 1) || (n_coef < 1))
    goto sparse_FIR_tilde_perf_zero;
  
  n_coef8 = n_coef / 8;
  write_hist1 = x->x_history_beg;
  write_hist2 = write_hist1 + order;
  read_hist = write_hist2;
  
  for(i=0; i<n; i++)
  {
    write_hist1[rw_index] = in[i];
    write_hist2[rw_index] = in[i];
    sum = 0.0f;
    coef_vec = coef;
    index_vec = index;
    hist_vec = &read_hist[rw_index];
    for(j=0; j<n_coef8; j++)
    {
      ix = index_vec[0];
      sum += coef_vec[0] * hist_vec[ix];
      ix = index_vec[1];
      sum += coef_vec[1] * hist_vec[ix];
      ix = index_vec[2];
      sum += coef_vec[2] * hist_vec[ix];
      ix = index_vec[3];
      sum += coef_vec[3] * hist_vec[ix];
      ix = index_vec[4];
      sum += coef_vec[4] * hist_vec[ix];
      ix = index_vec[5];
      sum += coef_vec[5] * hist_vec[ix];
      ix = index_vec[6];
      sum += coef_vec[6] * hist_vec[ix];
      ix = index_vec[7];
      sum += coef_vec[7] * hist_vec[ix];
      coef_vec += 8;
      index_vec += 8;
    }
    for(j=n_coef8*8; j<n_coef; j++)
    {
      ix = index[j];
      sum += coef[j] * read_hist[rw_index+ix];
    }
    out[i] = sum;
    rw_index++;
    if(rw_index >= order)
      rw_index -= order;
  }
  
  x->x_rw_index = rw_index;
  return(w+5);
  
sparse_FIR_tilde_perf_zero:
  
  while(n--)
    *out++ = 0.0f;
  return(w+5);
}

static void sparse_FIR_tilde_list(t_sparse_FIR_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int order = x->x_sparse_FIR_order;
  int n_arg2 = argc/2, index, i;
  int n_coef = 0;
  int *index_pointer = x->x_index_beg;
  t_float *coef_pointer = x->x_coef_beg;
  t_float coef;
  
  for(i=0; i<n_arg2; i++)
  {
    index = (int)atom_getfloat(argv++);
    coef = (t_float)atom_getfloat(argv++);
    if((index >= 0) && (index < order))
    {
      *index_pointer++ = -index;
      *coef_pointer++ = coef;
      n_coef++;
    }
  }
  x->x_n_coef = n_coef;
}

static void sparse_FIR_tilde_matrix(t_sparse_FIR_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int row, col;
  
  if(argc < 2)
  {
    post("sparse_FIR~ : corrupt matrix passed");
    return;
  }
  row = (int)atom_getfloat(argv++);
  col = (int)atom_getfloat(argv++);
  if((row < 1)||(col < 1))
  {
    post("sparse_FIR~ : corrupt matrix passed");
    return;
  }
  if((row*col) < (argc - 2))
  {
    post("sparse_FIR~ WARNING: row column product less than message content!");
    sparse_FIR_tilde_list(x, &s_list, row*col, argv);
  }
  else if((row*col) > (argc-2))
  {
    post("sparse_FIR~ WARNING: row column product greater than message content!");
    sparse_FIR_tilde_list(x, &s_list, argc-2, argv);
  }
  else
    sparse_FIR_tilde_list(x, &s_list, argc-2, argv);
}

static void sparse_FIR_tilde_order(t_sparse_FIR_tilde *x, t_floatarg fn)
{
  int n = (int)fn;
  
  if(n > 0)
  {
    if(n > x->x_sparse_FIR_order)/* resize */
    {
      x->x_history_beg =  (t_float *)resizebytes(x->x_history_beg, 2*x->x_n_coef_malloc*sizeof(t_float), 2*n*sizeof(t_float));
      x->x_index_beg =  (int *)resizebytes(x->x_index_beg, x->x_n_coef_malloc*sizeof(int), n*sizeof(int));
      x->x_coef_beg =  (t_float *)resizebytes(x->x_coef_beg, x->x_n_coef_malloc*sizeof(t_float), n*sizeof(t_float));
      x->x_n_coef_malloc = n;
    }
    x->x_sparse_FIR_order = n;
    x->x_rw_index = 0;
  }
}

static void sparse_FIR_tilde_dsp(t_sparse_FIR_tilde *x, t_signal **sp)
{
  dsp_add(sparse_FIR_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void *sparse_FIR_tilde_new(t_floatarg fn)
{
  t_sparse_FIR_tilde *x = (t_sparse_FIR_tilde *)pd_new(sparse_FIR_tilde_class);
  int n=(int)fn;
  
  outlet_new(&x->x_obj, &s_signal);
  x->x_msi = 0;
  x->x_n_coef = 0;
  if(n < 1)
    n = 1;
  x->x_sparse_FIR_order = n;
  x->x_n_coef_malloc = n;
  x->x_history_beg = (t_float *)getbytes((2*x->x_n_coef_malloc)*sizeof(t_float));
  x->x_index_beg = (int *)getbytes(x->x_n_coef_malloc*sizeof(int));
  x->x_coef_beg = (t_float *)getbytes(x->x_n_coef_malloc*sizeof(t_float));
  x->x_rw_index = 0;
  return(x);
}

static void sparse_FIR_tilde_free(t_sparse_FIR_tilde *x)
{
  if(x->x_history_beg)
    freebytes(x->x_history_beg, (2*x->x_n_coef_malloc)*sizeof(t_float));
  if(x->x_index_beg)
    freebytes(x->x_index_beg, x->x_n_coef_malloc*sizeof(int));
  if(x->x_coef_beg)
    freebytes(x->x_coef_beg, x->x_n_coef_malloc*sizeof(t_float));
}

void sparse_FIR_tilde_setup(void)
{
  sparse_FIR_tilde_class = class_new(gensym("sparse_FIR~"), (t_newmethod)sparse_FIR_tilde_new,
    (t_method)sparse_FIR_tilde_free, sizeof(t_sparse_FIR_tilde), 0, A_DEFFLOAT, 0);
  CLASS_MAINSIGNALIN(sparse_FIR_tilde_class, t_sparse_FIR_tilde, x_msi);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_dsp, gensym("dsp"), 0);
  class_addlist(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_list);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_matrix, gensym("matrix"), A_GIMME, 0);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_order, gensym("order"), A_FLOAT, 0);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_order, gensym("size"), A_FLOAT, 0);
}
