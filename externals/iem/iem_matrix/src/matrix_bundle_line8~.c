/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2007 */

#include "m_pd.h"
#include "iemlib.h"


/* ---------- matrix_bundle_line8~ - signal matrix multiplication object with message matrix-coeff. ----------- */

typedef struct matrix_bundle_line8_tilde
{
  t_object  x_obj;
  int       *x_in2out_new;
  int       *x_in2out_old;
  int       *x_remaining_ticks;
  int       *x_retarget;
  t_float   **x_io;
  t_float   *x_outsumbuf;
  int       x_outsumbufsize;
  int       x_n_in; /* columns */
  int       x_n_out;   /* rows  */
  t_float   x_inc8;
  t_float   x_biginc;
  t_float   x_raise_cur;
  t_float   x_raise_end;
  t_float   x_fall_cur;
  t_float   x_fall_end;
  t_float   x_msi;
  int       x_remaining_ticks_start;
  t_float   x_time_ms;
  t_float   x_ms2tick;
  t_float   x_8overn;
} t_matrix_bundle_line8_tilde;

t_class *matrix_bundle_line8_tilde_class;

static void matrix_bundle_line8_tilde_time(t_matrix_bundle_line8_tilde *x, t_floatarg time_ms)
{
  if(time_ms <= 0.0f)
    time_ms = 0.0f;
  x->x_time_ms = time_ms;
  
  x->x_remaining_ticks_start = (int)(x->x_time_ms * x->x_ms2tick);
  if(!x->x_remaining_ticks_start)
    x->x_remaining_ticks_start = 1;
}

static void matrix_bundle_line8_tilde_element(t_matrix_bundle_line8_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int inindex, outindex;
  
  if(argc < 2)
  {
    post("matrix_bundle_line8~ : bad list: <int> output_row_index <int> input_col_index !");
    return;
  }
  
  if(x->x_time_ms <= 0.0f)
  {
    outindex = (int)atom_getint(argv);
    argv++;
    inindex = (int)atom_getint(argv) - 1;
    
    if(inindex >= x->x_n_in)
      inindex = x->x_n_in - 1;
    if(inindex < 0)
      inindex = 0;
    if(outindex >= x->x_n_out)
      outindex = x->x_n_out;
    if(outindex < 0)
      outindex = 0;
    
    x->x_in2out_old[inindex] = x->x_in2out_new[inindex] = outindex;/*retarget = 0*/
    x->x_remaining_ticks[inindex] = x->x_retarget[inindex] = 0;
    x->x_fall_end = x->x_fall_cur = 0.0f;
    x->x_raise_end = x->x_raise_cur = 1.0f;
  }
  else
  {
    x->x_inc8 = x->x_8overn / (float)x->x_remaining_ticks_start;
    x->x_biginc = 1.0f / (float)x->x_remaining_ticks_start;
    
    x->x_fall_end = 0.0f;
    x->x_fall_cur = 1.0f;
    x->x_raise_end = 1.0f;
    x->x_raise_cur = 0.0f;
    
    outindex = (int)atom_getint(argv);
    argv++;
    inindex = (int)atom_getint(argv) - 1;
    
    if(inindex >= x->x_n_in)
      inindex = x->x_n_in - 1;
    if(inindex < 0)
      inindex = 0;
    if(outindex >= x->x_n_out)
      outindex = x->x_n_out;
    if(outindex < 0)
      outindex = 0;
    
    x->x_in2out_new[inindex] = outindex;
    if(x->x_in2out_new[inindex] == x->x_in2out_old[inindex])
      x->x_retarget[inindex] = 0;
    else
      x->x_retarget[inindex] = 1;
  }
}

static void matrix_bundle_line8_tilde_list(t_matrix_bundle_line8_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int outindex, i, n=x->x_n_in;
  
  if(argc < n)
  {
    post("matrix_bundle_line8~ : bad list: (number_of_input_cols = %d) * <int> output_row_index !", n);
    return;
  }
  
  if(x->x_time_ms <= 0.0f)
  {
    for(i=0; i<n; i++)
    {
      outindex = (int)atom_getint(argv);
      argv++;
      if(outindex >= x->x_n_out)
        outindex = x->x_n_out;
      if(outindex < 0)
        outindex = 0;
      x->x_in2out_old[i] = x->x_in2out_new[i] = outindex;/*retarget = 0*/
      x->x_remaining_ticks[i] = x->x_retarget[i] = 0;
    }
    x->x_fall_end = x->x_fall_cur = 0.0f;
    x->x_raise_end = x->x_raise_cur = 1.0f;
  }
  else
  {
    x->x_inc8 = x->x_8overn / (float)x->x_remaining_ticks_start;
    x->x_biginc = 1.0f / (float)x->x_remaining_ticks_start;
    
    x->x_fall_end = 0.0f;
    x->x_fall_cur = 1.0f;
    x->x_raise_end = 1.0f;
    x->x_raise_cur = 0.0f;
    
    for(i=0; i<argc; i++)
    {
      x->x_in2out_old[i] = x->x_in2out_new[i];
      
      outindex = (int)atom_getint(argv);
      argv++;
      if(outindex >= x->x_n_out)
        outindex = x->x_n_out;
      if(outindex < 0)
        outindex = 0;
      x->x_in2out_new[i] = outindex;
      if(x->x_in2out_new[i] == x->x_in2out_old[i])
        x->x_retarget[i] = 0;
      else
        x->x_retarget[i] = 1;
    }
  }
}

static void matrix_bundle_line8_tilde_bundle(t_matrix_bundle_line8_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  matrix_bundle_line8_tilde_list(x, &s_list, argc, argv);
}

static void matrix_bundle_line8_tilde_stop(t_matrix_bundle_line8_tilde *x)
{
  int i, n=x->x_n_in;
  
  x->x_fall_end = x->x_fall_cur;
  x->x_raise_end = x->x_raise_cur;
  for(i=0; i<n; i++)
  {
    x->x_in2out_new[i] = x->x_in2out_old[i];
    x->x_remaining_ticks[i] = x->x_retarget[i] = 0;
  }
}

/* the dsp thing */

static t_int *matrix_bundle_line8_tilde_perform_zero(t_int *w)
{
  t_matrix_bundle_line8_tilde *x = (t_matrix_bundle_line8_tilde *)(w[1]);
  int n = (int)(w[2]);
  t_float **io = x->x_io;
  int n_in = x->x_n_in;
  int n_out = x->x_n_out;
  t_float *out;
  int j, i;
  
  for(j=0; j<n_out; j++)/* output-vector-row */
  {
    out = io[n_in+j];
    for(i=0; i<n; i++)
      *out++ = 0.0f;
  }
  return (w+3);
}

static t_int *matrix_bundle_line8_tilde_perf8(t_int *w)
{
  t_matrix_bundle_line8_tilde *x = (t_matrix_bundle_line8_tilde *)(w[1]);
  int n = (int)(w[2]);
  t_float **io = x->x_io;
  t_float *outsum;
  int *in2out_new = x->x_in2out_new;
  int *in2out_old = x->x_in2out_old;
  int n_in = x->x_n_in;   /* columns */
  int n_out = x->x_n_out; /* rows */
  t_float *in, *out, mul;
  t_float inc8 = x->x_inc8;
  int i, j, endofticks=0;
  
  for(i=0; i<n_in; i++)
  {
    if(x->x_retarget[i])
    {
      x->x_remaining_ticks[i] = x->x_remaining_ticks_start;
      x->x_retarget[i] = 0;
    }
  }
  
  for(j=0; j<n_out; j++)
  {
    outsum = x->x_outsumbuf + j*n;
    for(i=n; i; i -= 8, outsum += 8)/* reset out-buffer */
    {
      outsum[0] = 0.0f;
      outsum[1] = 0.0f;
      outsum[2] = 0.0f;
      outsum[3] = 0.0f;
      outsum[4] = 0.0f;
      outsum[5] = 0.0f;
      outsum[6] = 0.0f;
      outsum[7] = 0.0f;
    }
  }
  
  for(j=0; j<n_in; j++)
  {
    if(x->x_remaining_ticks[j])
    {
      in = io[j];
      if(in2out_new[j])
      {
        outsum = x->x_outsumbuf + n*(in2out_new[j] - 1);
        mul = x->x_raise_cur;
        for(i=n; i; i -= 8, outsum += 8, in += 8)/* each new in */
        {
          outsum[0] += in[0]*mul;
          outsum[1] += in[1]*mul;
          outsum[2] += in[2]*mul;
          outsum[3] += in[3]*mul;
          outsum[4] += in[4]*mul;
          outsum[5] += in[5]*mul;
          outsum[6] += in[6]*mul;
          outsum[7] += in[7]*mul;
          mul += inc8;
        }
      }
      //      raise_cur = mul;
      
      in = io[j];
      if(in2out_old[j])
      {
        outsum = x->x_outsumbuf + n*(in2out_old[j] - 1);
        mul = x->x_fall_cur;
        for(i=n; i; i -= 8, outsum += 8, in += 8)/* each old in */
        {
          outsum[0] += in[0]*mul;
          outsum[1] += in[1]*mul;
          outsum[2] += in[2]*mul;
          outsum[3] += in[3]*mul;
          outsum[4] += in[4]*mul;
          outsum[5] += in[5]*mul;
          outsum[6] += in[6]*mul;
          outsum[7] += in[7]*mul;
          mul -= inc8;
        }
      }
      //      fall_cur = mul;
      
      if(!--x->x_remaining_ticks[j])
      {
        endofticks = 1;
      }
    }
    else
    {
      in = io[j];
      if(in2out_new[j])
      {
        outsum = x->x_outsumbuf + n*(in2out_new[j] - 1);
        for(i=n; i; i -= 8, outsum += 8, in += 8)/* each in */
        {
          outsum[0] += in[0];
          outsum[1] += in[1];
          outsum[2] += in[2];
          outsum[3] += in[3];
          outsum[4] += in[4];
          outsum[5] += in[5];
          outsum[6] += in[6];
          outsum[7] += in[7];
        }
      }
    }
  }
  
  if(x->x_remaining_ticks[j])
  {
    x->x_raise_cur += x->x_biginc;
    x->x_fall_cur -= x->x_biginc;
  }
  
  if(endofticks)
  {
    x->x_fall_cur = x->x_fall_end;
    x->x_raise_cur = x->x_raise_end;
  }
  
  outsum = x->x_outsumbuf;
  for(j=0; j<n_out; j++)/* copy out-buffer to out */
  {
    out = io[n_in+j];
    for (i=n; i; i -= 8, out += 8, outsum += 8)
    {
      out[0] = outsum[0];
      out[1] = outsum[1];
      out[2] = outsum[2];
      out[3] = outsum[3];
      out[4] = outsum[4];
      out[5] = outsum[5];
      out[6] = outsum[6];
      out[7] = outsum[7];
    }
  }
  return (w+3);
}

static void matrix_bundle_line8_tilde_dsp(t_matrix_bundle_line8_tilde *x, t_signal **sp)
{
  int i, n=sp[0]->s_n*x->x_n_out;
  
  if(!x->x_outsumbuf)
  {
    x->x_outsumbufsize = n;
    x->x_outsumbuf = (t_float *)getbytes(x->x_outsumbufsize * sizeof(t_float));
  }
  else if(x->x_outsumbufsize != n)
  {
    x->x_outsumbuf = (t_float *)resizebytes(x->x_outsumbuf, x->x_outsumbufsize*sizeof(t_float), n*sizeof(t_float));
    x->x_outsumbufsize = n;
  }
  
  n = x->x_n_in + x->x_n_out;
  for(i=0; i<n; i++)
    x->x_io[i] = sp[i]->s_vec;
  
  n = sp[0]->s_n;
  x->x_ms2tick = 0.001f * (float)(sp[0]->s_sr) / (float)n;
  x->x_8overn = 8.0f / (float)n;
  
  x->x_remaining_ticks_start = (int)(x->x_time_ms * x->x_ms2tick);
  if(!x->x_remaining_ticks_start)
    x->x_remaining_ticks_start = 1;
  
  if(n&7)
  {
    dsp_add(matrix_bundle_line8_tilde_perform_zero, 2, x, n);
    post("ERROR!!! matrix_bundle_line8_tilde~ : blocksize is %d and not a multiple of 8", n);
  }
  else
    dsp_add(matrix_bundle_line8_tilde_perf8, 2, x, n);
}


/* setup/setdown things */

static void matrix_bundle_line8_tilde_free(t_matrix_bundle_line8_tilde *x)
{
  freebytes(x->x_in2out_new, x->x_n_in * sizeof(int));
  freebytes(x->x_in2out_old, x->x_n_in * sizeof(int));
  freebytes(x->x_remaining_ticks, x->x_n_in * sizeof(int));
  freebytes(x->x_retarget, x->x_n_in * sizeof(int));
  freebytes(x->x_io, (x->x_n_in + x->x_n_out) * sizeof(t_float *));
  if(x->x_outsumbuf)
    freebytes(x->x_outsumbuf, x->x_outsumbufsize * sizeof(t_float));
}

static void *matrix_bundle_line8_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_matrix_bundle_line8_tilde *x = (t_matrix_bundle_line8_tilde *)pd_new(matrix_bundle_line8_tilde_class);
  int i, n;
  
  switch (argc)
  {
  case 0:
    x->x_n_in = x->x_n_out = 1;
    x->x_time_ms = 50.0f;
    break;
  case 1:
    x->x_n_in = x->x_n_out = (int)atom_getint(argv);
    x->x_time_ms = 50.0f;
    break;
  case 2:
    x->x_n_in = (int)atom_getint(argv);
    x->x_n_out = (int)atom_getint(argv+1);
    x->x_time_ms = 50.0f;
    break;
  default:
    x->x_n_in = (int)atom_getint(argv);
    x->x_n_out = (int)atom_getint(argv+1);
    x->x_time_ms = atom_getfloat(argv+2);
    break;
  }
  
  if(x->x_n_in < 1)
    x->x_n_in = 1;
  if(x->x_n_out < 1)
    x->x_n_out = 1;
  if(x->x_time_ms < 0.0f)
    x->x_time_ms = 50.0f;
  i = x->x_n_in - 1;
  while(i--)
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  i = x->x_n_out;
  while(i--)
    outlet_new(&x->x_obj, &s_signal);
  
  x->x_in2out_new = (int *)getbytes(x->x_n_in * sizeof(int));
  x->x_in2out_old = (int *)getbytes(x->x_n_in * sizeof(int));
  x->x_remaining_ticks = (int *)getbytes(x->x_n_in * sizeof(int));
  x->x_retarget = (int *)getbytes(x->x_n_in * sizeof(int));
  x->x_io = (t_float **)getbytes((x->x_n_in + x->x_n_out) * sizeof(t_float *));
  x->x_outsumbuf = (t_float *)0;
  x->x_outsumbufsize = 0;
  
  x->x_raise_cur = 1.0f;
  x->x_raise_end = 1.0f;
  x->x_fall_cur = 0.0f;
  x->x_fall_end = 0.0f;
  x->x_inc8 = 0.0f;
  x->x_biginc = 0.0f;
  x->x_msi = 0;
  x->x_ms2tick = 0.001f * 44100.0f / 64.0f;
  x->x_8overn = 8.0f / 64.0f;
  x->x_remaining_ticks_start = (int)(x->x_time_ms * x->x_ms2tick);
  if(!x->x_remaining_ticks_start)
    x->x_remaining_ticks_start = 1;
  
  n = x->x_n_in;
  for(i=0; i<n; i++)
  {
    x->x_in2out_new[i] = 0;
    x->x_in2out_old[i] = 0;
    x->x_remaining_ticks[i] = 0;
    x->x_retarget[i] = 0;
  }
  return(x);
}

void matrix_bundle_line8_tilde_setup(void)
{
  matrix_bundle_line8_tilde_class = class_new(gensym("matrix_bundle_line8~"), (t_newmethod)matrix_bundle_line8_tilde_new, (t_method)matrix_bundle_line8_tilde_free,
    sizeof(t_matrix_bundle_line8_tilde), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(matrix_bundle_line8_tilde_class, t_matrix_bundle_line8_tilde, x_msi);
  class_addmethod(matrix_bundle_line8_tilde_class, (t_method)matrix_bundle_line8_tilde_dsp, gensym("dsp"), 0);
  class_addlist(matrix_bundle_line8_tilde_class, (t_method)matrix_bundle_line8_tilde_list);
  class_addmethod(matrix_bundle_line8_tilde_class, (t_method)matrix_bundle_line8_tilde_element, gensym("element"), A_GIMME, 0);
  class_addmethod(matrix_bundle_line8_tilde_class, (t_method)matrix_bundle_line8_tilde_bundle, gensym("bundle"), A_GIMME, 0);
  class_addmethod(matrix_bundle_line8_tilde_class, (t_method)matrix_bundle_line8_tilde_stop, gensym("stop"), 0);
  class_addmethod(matrix_bundle_line8_tilde_class, (t_method)matrix_bundle_line8_tilde_time, gensym("time"), A_FLOAT, 0);
//  class_sethelpsymbol(matrix_bundle_line8_tilde_class, gensym("iemhelp/matrix_bundle_line8~-help"));
}
