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
  t_sample  *x_coef_beg;
  int       *x_index_beg;
  int       x_n_coef_resp_order;
  int       x_n_coef;
  int       x_n_coef_malloc;
  t_sample  *x_history_beg;
  int       x_n_order;
  int       x_n_order_malloc;
  int       x_rw_index;
  t_float   x_float_sig_in;
} t_sparse_FIR_tilde;

static t_class *sparse_FIR_tilde_class;

static t_int *sparse_FIR_tilde_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_sparse_FIR_tilde *x = (t_sparse_FIR_tilde *)(w[3]);
  int n = (t_int)(w[4]);
  int rw_index = x->x_rw_index;
  int i, j, ix;
  int order = x->x_n_order;
  int n_coef = x->x_n_coef_resp_order;
  int n_coef8;
  t_sample sum=0.0;
  t_sample *coef = x->x_coef_beg;
  int *index = x->x_index_beg;
  t_sample *write_hist1=x->x_history_beg;
  t_sample *write_hist2;
  t_sample *read_hist;
  t_sample *coef_vec;
  int *index_vec;
  t_sample *hist_vec;
  
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
    sum = 0.0;
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
    *out++ = 0.0;
  return(w+5);
}

static void sparse_FIR_tilde_sort_within(t_sparse_FIR_tilde *x)
{
  int cur_order = x->x_n_order;
  int n_coef = x->x_n_coef;
  int index, i;
  int n_coef_resp_order = 0;
  int *index_pointer_within = x->x_index_beg;
  t_float *coef_pointer_within = x->x_coef_beg;
  int *index_pointer = x->x_index_beg + x->x_n_coef_malloc;
  t_float *coef_pointer = x->x_coef_beg + x->x_n_coef_malloc;
  t_float coef;
  
  for(i=0; i<n_coef; i++)
  {
    index = index_pointer[i];
    coef = coef_pointer[i];
    if((index >= 0) && (index < cur_order))
    {
      index_pointer_within[i] = -index; /* negate index for FIR direction */
      coef_pointer_within[i] = coef;
      n_coef_resp_order++;
    }
  }
  x->x_n_coef_resp_order = n_coef_resp_order;
}

static void sparse_FIR_tilde_list(t_sparse_FIR_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int max_order = x->x_n_order_malloc;
  int n_pair_arg = argc/2, index, i;
  int n_coef = 0;
  int *index_pointer;
  t_float *coef_pointer;
  t_float coef;
  
  if(n_pair_arg > 0)
  {
    if(n_pair_arg > x->x_n_coef_malloc) /* resize */
    {
      x->x_index_beg =  (int *)resizebytes(x->x_index_beg, 2*x->x_n_coef_malloc*sizeof(int), 2*n_pair_arg*sizeof(int));
      x->x_coef_beg =  (t_float *)resizebytes(x->x_coef_beg, 2*x->x_n_coef_malloc*sizeof(t_float), 2*n_pair_arg*sizeof(t_float));
      x->x_n_coef_malloc = n_pair_arg;
    }
    
    index_pointer = x->x_index_beg + x->x_n_coef_malloc;
    coef_pointer = x->x_coef_beg + x->x_n_coef_malloc;
    
    for(i=0; i<n_pair_arg; i++)
    {
      index = (int)atom_getfloat(argv++);
      coef = (t_float)atom_getfloat(argv++);
      if((index >= 0) && (index < max_order))
      {
        *index_pointer++ = index;
        *coef_pointer++ = coef;
        n_coef++;
      }
    }
    x->x_n_coef = n_coef;
    
    sparse_FIR_tilde_sort_within(x);
  }
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
  int n_order = (int)fn;
  
  if(n_order > 0)
  {
    if(n_order > x->x_n_order_malloc) /* resize */
    {
      x->x_history_beg =  (t_float *)resizebytes(x->x_history_beg, 2*x->x_n_order_malloc*sizeof(t_float), 2*n_order*sizeof(t_float));
      x->x_n_order_malloc = n_order;
    }
    x->x_n_order = n_order;
    x->x_rw_index = 0;
    
    sparse_FIR_tilde_sort_within(x);
  }
}

static void sparse_FIR_tilde_dsp(t_sparse_FIR_tilde *x, t_signal **sp)
{
  dsp_add(sparse_FIR_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, (t_int)sp[0]->s_n);
}

static void *sparse_FIR_tilde_new(t_floatarg fn)
{
  t_sparse_FIR_tilde *x = (t_sparse_FIR_tilde *)pd_new(sparse_FIR_tilde_class);
  int n_order=(int)fn;
  int i;
  
  outlet_new(&x->x_obj, &s_signal);
  
  x->x_n_coef = 1;
  x->x_n_coef_resp_order = 1;
  x->x_n_coef_malloc = 1;
  x->x_index_beg = (int *)getbytes(2*x->x_n_coef_malloc*sizeof(int));
  x->x_coef_beg = (t_float *)getbytes(2*x->x_n_coef_malloc*sizeof(t_float));
  x->x_index_beg[0] = 0;
  x->x_index_beg[1] = 0;
  x->x_coef_beg[0] = 0.0f;
  x->x_coef_beg[1] = 0.0f;
  if(n_order < 1)
    n_order = 1;
  x->x_n_order = n_order;
  x->x_n_order_malloc = n_order;
  x->x_history_beg = (t_float *)getbytes((2*x->x_n_order_malloc)*sizeof(t_float));
  x->x_rw_index = 0;
  n_order = 2*x->x_n_order_malloc;
  for(i=0; i<n_order; i++)
    x->x_history_beg[i] = 0.0f;
  
  x->x_float_sig_in = 0.0f;
  
  post("NEW: n_coef_resp_order = %d, n_coef = %d, n_coef_malloc = %d, n_order = %d, n_order_malloc = %d", x->x_n_coef_resp_order, x->x_n_coef, x->x_n_coef_malloc, x->x_n_order, x->x_n_order_malloc);
  
  return(x);
}

static void sparse_FIR_tilde_free(t_sparse_FIR_tilde *x)
{
  freebytes(x->x_history_beg, (2*x->x_n_order_malloc)*sizeof(t_float)); /* twice, because of my simple circle-buffer */
  freebytes(x->x_index_beg, 2*x->x_n_coef_malloc*sizeof(int)); /* twice, because of buffering both, all coefficients and only the relevant for current order */
  freebytes(x->x_coef_beg, 2*x->x_n_coef_malloc*sizeof(t_float)); /* twice, because of buffering both, all coefficients and only the relevant for current order */
}

/*static void sparse_FIR_tilde_dump(t_sparse_FIR_tilde *x)
{
  t_float *hist=x->x_history_beg;
  int *ix=x->x_index_beg;
  int n=x->x_n_order;
  
  post("n_coef_resp_order = %d, n_coef = %d, n_coef_malloc = %d, n_order = %d, n_order_malloc = %d", x->x_n_coef_resp_order, x->x_n_coef, x->x_n_coef_malloc, x->x_n_order, x->x_n_order_malloc);
  post("HIST:");
  
  while(n > 8)
  {
    post("hist = %g, %g, %g, %g, %g, %g, %g, %g", hist[n-1], hist[n-2], hist[n-3], hist[n-4], hist[n-5], hist[n-6], hist[n-7], hist[n-8]);
    n -= 8;
    hist -= 8;
  }
  while(n > 0)
  {
    post("hist = %g", hist[n-1]);
    n--;
    hist--;
  }
  post("COEF:");
  
  hist = x->x_coef_beg;
  n = x->x_n_coef_resp_order;
  while(n > 8)
  {
    post("coef = %d@%g, %d@%g, %d@%g, %d@%g, %d@%g, %d@%g, %d@%g, %d@%g", ix[n-1],hist[n-1], ix[n-2],hist[n-2], ix[n-3],hist[n-3], ix[n-4],hist[n-4], ix[n-5],hist[n-5], ix[n-6],hist[n-6], ix[n-7],hist[n-7], ix[n-8],hist[n-8]);
    n -= 8;
    hist -= 8;
  }
  while(n > 0)
  {
    post("coef = %d@%g", ix[n-1],hist[n-1]);
    n--;
    hist--;
  }
  post("***********************");
}*/

void sparse_FIR_tilde_setup(void)
{
  sparse_FIR_tilde_class = class_new(gensym("sparse_FIR~"), (t_newmethod)sparse_FIR_tilde_new,
    (t_method)sparse_FIR_tilde_free, sizeof(t_sparse_FIR_tilde), 0, A_DEFFLOAT, 0);
  CLASS_MAINSIGNALIN(sparse_FIR_tilde_class, t_sparse_FIR_tilde, x_float_sig_in);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_dsp, gensym("dsp"), A_CANT, 0);
  class_addlist(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_list);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_matrix, gensym("matrix"), A_GIMME, 0);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_order, gensym("order"), A_FLOAT, 0);
  class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_order, gensym("size"), A_FLOAT, 0);
  //class_addmethod(sparse_FIR_tilde_class, (t_method)sparse_FIR_tilde_dump, gensym("dump"), 0);
}
