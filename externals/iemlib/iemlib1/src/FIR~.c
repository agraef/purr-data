/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iemlib1 written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2012 */


#include "m_pd.h"
#include "iemlib.h"


/* ---------- FIR~ - FIR-filter with table-coef ----------- */

typedef struct _FIR_tilde
{
  t_object    x_obj;
  iemarray_t  *x_array_coef_beg;
  t_sample    *x_history_beg;
  int         x_rw_index;
  int         x_fir_order;
  int         x_malloc_history;
  t_symbol    *x_table_name;
  t_float     x_float_sig_in;
} t_FIR_tilde;

static t_class *FIR_tilde_class;

static t_int *FIR_tilde_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  t_FIR_tilde *x = (t_FIR_tilde *)(w[3]);
  int n = (t_int)(w[4]);
  int rw_index = x->x_rw_index;
  int i, j;
  int order = x->x_fir_order;
  int ord16 = order / 16;
  t_sample sum=0.0;
  iemarray_t *coef = x->x_array_coef_beg;
  t_sample *write_hist1=x->x_history_beg;
  t_sample *write_hist2;
  t_sample *read_hist;
  iemarray_t *coef_vec;
  t_sample *hist_vec;
  
  if(!coef)
    goto FIR_tildeperfzero;
  
  write_hist1 = x->x_history_beg;
  write_hist2 = write_hist1 + order;
  read_hist = write_hist2;
  
  for(i=0; i<n; i++)
  {
    write_hist1[rw_index] = in[i];
    write_hist2[rw_index] = in[i];
    
    sum = 0.0;
    coef_vec = coef;
    hist_vec = &read_hist[rw_index];
    for(j=0; j<ord16; j++)
    {
      sum += (t_sample)(iemarray_getfloat(coef_vec, 0)) * hist_vec[0];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 1)) * hist_vec[-1];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 2)) * hist_vec[-2];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 3)) * hist_vec[-3];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 4)) * hist_vec[-4];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 5)) * hist_vec[-5];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 6)) * hist_vec[-6];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 7)) * hist_vec[-7];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 8)) * hist_vec[-8];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 9)) * hist_vec[-9];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 10)) * hist_vec[-10];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 11)) * hist_vec[-11];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 12)) * hist_vec[-12];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 13)) * hist_vec[-13];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 14)) * hist_vec[-14];
      sum += (t_sample)(iemarray_getfloat(coef_vec, 15)) * hist_vec[-15];
      coef_vec += 16;
      hist_vec -= 16;
    }
    for(j=ord16*16; j<order; j++)
    {
      sum += (t_sample)(iemarray_getfloat(coef, j)) * read_hist[rw_index-j];
    }
    out[i] = sum;
    
    rw_index++;
    if(rw_index >= order)
      rw_index -= order;
  }
  
  x->x_rw_index = rw_index;
  return(w+5);
  
FIR_tildeperfzero:
  
  while(n--)
    *out++ = 0.0;
  return(w+5);
}

void FIR_tilde_set(t_FIR_tilde *x, t_symbol *table_name, t_floatarg forder)
{
  t_garray *ga;
  int table_size;
  int order = (int)forder;
  int i;
  
  x->x_table_name = table_name;
  if(order < 1)
    order = 1;
  x->x_fir_order = order;
  if(!(ga = (t_garray *)pd_findbyclass(x->x_table_name, garray_class)))
  {
    if(*table_name->s_name)
      error("FIR~: %s: no such table~", x->x_table_name->s_name);
    x->x_array_coef_beg = (iemarray_t *)0;
  }
  else if(!iemarray_getarray(ga, &table_size, &x->x_array_coef_beg))
  {
    error("%s: bad template for FIR~", x->x_table_name->s_name);
    x->x_array_coef_beg = (iemarray_t *)0;
  }
  else if(table_size < order)
  {
    error("FIR~: tablesize %d < order %d !!!!", table_size, order);
    x->x_array_coef_beg = (iemarray_t *)0;
  }
  else
  {
    garray_usedindsp(ga);/* good */
  }
  x->x_rw_index = 0;
  if(x->x_fir_order > x->x_malloc_history)/* resize */
  {
    x->x_history_beg =  (t_sample *)resizebytes(x->x_history_beg, 2*x->x_malloc_history*sizeof(t_sample), 2*x->x_fir_order*sizeof(t_sample));
    x->x_malloc_history = x->x_fir_order;
  }
}

static void FIR_tilde_dsp(t_FIR_tilde *x, t_signal **sp)
{
  FIR_tilde_set(x, x->x_table_name, x->x_fir_order);
  dsp_add(FIR_tilde_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void *FIR_tilde_new(t_symbol *array_name, t_floatarg forder)
{
  t_FIR_tilde *x = (t_FIR_tilde *)pd_new(FIR_tilde_class);
  int order = (int)forder;
  
  outlet_new(&x->x_obj, &s_signal);
  x->x_float_sig_in = 0;
  x->x_table_name = array_name;
  x->x_array_coef_beg = 0;
  if(order < 1)
    order = 1;
  x->x_fir_order = order;
  x->x_malloc_history = order;
  x->x_history_beg = (t_sample *)getbytes((2*x->x_malloc_history)*sizeof(t_sample));
  x->x_rw_index = 0;
  return(x);
}

static void FIR_tilde_free(t_FIR_tilde *x)
{
  if(x->x_history_beg)
    freebytes(x->x_history_beg, (2*x->x_malloc_history)*sizeof(t_sample));
}

void FIR_tilde_setup(void)
{
  FIR_tilde_class = class_new(gensym("FIR~"), (t_newmethod)FIR_tilde_new,
    (t_method)FIR_tilde_free, sizeof(t_FIR_tilde), 0, A_DEFSYM, A_DEFFLOAT, 0);
  CLASS_MAINSIGNALIN(FIR_tilde_class, t_FIR_tilde, x_float_sig_in);
  class_addmethod(FIR_tilde_class, (t_method)FIR_tilde_dsp, gensym("dsp"), 0);
  class_addmethod(FIR_tilde_class, (t_method)FIR_tilde_set, gensym("set"), A_SYMBOL, A_FLOAT, 0);
}
