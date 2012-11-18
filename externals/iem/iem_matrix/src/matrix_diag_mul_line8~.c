/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2007 */

#include "m_pd.h"
#include "iemlib.h"


/* ---------- matrix_diag_mul_line8~ - signal matrix multiplication object with message matrix-coeff. ----------- */

typedef struct matrix_diag_mul_line8_tilde
{
  t_object  x_obj;
  t_float   *x_matcur;
  t_float   *x_matend;
  t_float   *x_inc8;
  t_float   *x_biginc;
  t_float   **x_io;
  t_float   *x_buf;
  int       x_bufsize;
  int       x_n_io;
  t_float   x_msi;
  int       x_retarget;
  t_float   x_time_ms;
  int       x_remaining_ticks;
  t_float   x_ms2tick;
  t_float   x_8overn;
} t_matrix_diag_mul_line8_tilde;

t_class *matrix_diag_mul_line8_tilde_class;

static void matrix_diag_mul_line8_tilde_time(t_matrix_diag_mul_line8_tilde *x, t_floatarg time_ms)
{
  if(time_ms <= 0.0f)
    time_ms = 0.0f;
  x->x_time_ms = time_ms;
}

static void matrix_diag_mul_line8_tilde_element(t_matrix_diag_mul_line8_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, j, n=x->x_n_io;
  t_float *matcur = x->x_matcur;
  t_float *matend = x->x_matend;
  
  if(x->x_time_ms <= 0.0f)
  {
    if(argc == 2)
    {
      i = (int)(atom_getint(argv));
      argv++;
      if((i >= 1) && (i <= n))
        matend[i-1] = matcur[i-1] = atom_getfloat(argv);
    }
    else if(argc == 3)
    {
      i = (int)(atom_getint(argv));
      argv++;
      j = (int)(atom_getint(argv));
      argv++;
      if((i >= 1) && (i <= n) && (i == j))
        matend[i-1] = matcur[i-1] = atom_getfloat(argv);
    }
    x->x_remaining_ticks = x->x_retarget = 0;
  }
  else
  {
    if(argc == 2)
    {
      i = (int)(atom_getint(argv));
      argv++;
      if((i >= 1) && (i <= n))
        matend[i-1] = atom_getfloat(argv);
    }
    else if(argc == 3)
    {
      i = (int)(atom_getint(argv));
      argv++;
      j = (int)(atom_getint(argv));
      argv++;
      if((i >= 1) && (i <= n) && (i == j))
        matend[i-1] = atom_getfloat(argv);
    }
    x->x_retarget = 1;
  }
}

static void matrix_diag_mul_line8_tilde_list(t_matrix_diag_mul_line8_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int i, n=x->x_n_io;
  t_float *matcur = x->x_matcur;
  t_float *matend = x->x_matend;
  
  if(argc < n)
  {
    post("matrix_diag_mul_line8~ : dimensions do not match !!");
    return;
  }
  
  if(x->x_time_ms <= 0.0f)
  {
    for(i=0; i<n; i++)
    {
      *matend++ = *matcur++ = atom_getfloat(argv);
      argv++;
    }
    x->x_remaining_ticks = x->x_retarget = 0;
  }
  else
  {
    for(i=0; i<n; i++)
    {
      *matend++ = atom_getfloat(argv);
      argv++;
    }
    x->x_retarget = 1;
  }
}

static void matrix_diag_mul_line8_tilde_diag(t_matrix_diag_mul_line8_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  matrix_diag_mul_line8_tilde_list(x, &s_list, argc, argv);
}

static void matrix_diag_mul_line8_tilde_stop(t_matrix_diag_mul_line8_tilde *x)
{
  int i = x->x_n_io;
  t_float *matend=x->x_matend;
  t_float *matcur=x->x_matcur;
  
  while(i--)
  {
    *matend++ = *matcur++;
  }
  x->x_remaining_ticks = x->x_retarget = 0;
}

/* the dsp thing */

static t_int *matrix_diag_mul_line8_tilde_perform_zero(t_int *w)
{
  t_matrix_diag_mul_line8_tilde *x = (t_matrix_diag_mul_line8_tilde *)(w[1]);
  int n = (int)(w[2]);
  t_float **io = x->x_io;
  int n_io = x->x_n_io;
  t_float *out;
  int i, j;
  
  for(j=0; j<n_io; j++)/* output-vector-row */
  {
    out = io[n_io+j];
    for(i=0; i<n; i++)
      *out++ = 0.0f;
  }
  return (w+3);
}

static t_int *matrix_diag_mul_line8_tilde_perf8(t_int *w)
{
  t_matrix_diag_mul_line8_tilde *x = (t_matrix_diag_mul_line8_tilde *)(w[1]);
  int n = (int)(w[2]);
  t_float **io = x->x_io;
  t_float *buf = x->x_buf;
  t_float *matcur, *matend;
  t_float *inc8, *biginc, inc;
  int n_io = x->x_n_io;
  t_float *in, *out, mul;
  int i, j;
  
  if(x->x_retarget)
  {
    int nticks = (int)(x->x_time_ms * x->x_ms2tick);
    
    if(!nticks)
      nticks = 1;
    x->x_remaining_ticks = nticks;
    j = n_io;
    inc8 = x->x_inc8;
    matcur = x->x_matcur;
    matend = x->x_matend;
    mul = x->x_8overn / (t_float)nticks;
    while(j--)
    {
      *inc8++ = (*matend++ - *matcur++) * mul;
    }
    j = n_io;
    biginc = x->x_biginc;
    matcur = x->x_matcur;
    matend = x->x_matend;
    mul = 1.0f / (t_float)nticks;
    while(j--)
    {
      *biginc++ = (*matend++ - *matcur++) * mul;
    }
    x->x_retarget = 0;
  }
  
  if(x->x_remaining_ticks)
  {
    inc8 = x->x_inc8;
    biginc = x->x_biginc;
    matcur = x->x_matcur;
    for(j=0; j<n_io; j++)
    {
      inc = inc8[j];
      mul = matcur[j];
      in = io[j];
      for(i=n; i; i -= 8, in += 8, buf += 8)
      {
        buf[0] = in[0] * mul;
        buf[1] = in[1] * mul;
        buf[2] = in[2] * mul;
        buf[3] = in[3] * mul;
        buf[4] = in[4] * mul;
        buf[5] = in[5] * mul;
        buf[6] = in[6] * mul;
        buf[7] = in[7] * mul;
        mul += inc;
      }
      matcur[j] += biginc[j];
    }
    if(!--x->x_remaining_ticks)
    {
      matcur = x->x_matcur;
      matend = x->x_matend;
      i = n_io;
      while(i--)
        *matcur++ = *matend++;
    }
  }
  else
  {
    matend = x->x_matend;
    for(j=0; j<n_io; j++)
    {
      mul = matend[j];
      in = io[j];
      for(i=n; i; i -= 8, in += 8, buf += 8)
      {
        buf[0] = in[0] * mul;
        buf[1] = in[1] * mul;
        buf[2] = in[2] * mul;
        buf[3] = in[3] * mul;
        buf[4] = in[4] * mul;
        buf[5] = in[5] * mul;
        buf[6] = in[6] * mul;
        buf[7] = in[7] * mul;
      }
    }
  }
  
  buf = x->x_buf;
  for(j=0; j<n_io; j++)
  {
    out = io[j+n_io];
    for(i=n; i; i -= 8, buf += 8, out += 8)
    {
      out[0] = buf[0];
      out[1] = buf[1];
      out[2] = buf[2];
      out[3] = buf[3];
      out[4] = buf[4];
      out[5] = buf[5];
      out[6] = buf[6];
      out[7] = buf[7];
    }
  }
  return (w+3);
}

static void matrix_diag_mul_line8_tilde_dsp(t_matrix_diag_mul_line8_tilde *x, t_signal **sp)
{
  int i, n=sp[0]->s_n;
  int bufsize = sp[0]->s_n*x->x_n_io;
  
  if(!x->x_buf)
  {
    x->x_bufsize = bufsize;
    x->x_buf = (t_float *)getbytes(x->x_bufsize * sizeof(t_float));
  }
  else if(x->x_bufsize != bufsize)
  {
    x->x_buf = (t_float *)resizebytes(x->x_buf, x->x_bufsize*sizeof(t_float), bufsize*sizeof(t_float));
    x->x_bufsize = bufsize;
  }
  
  n = 2 * x->x_n_io;
  for(i=0; i<n; i++)
  {
    x->x_io[i] = sp[i]->s_vec;
    //    post("iovec_addr = %d", (unsigned int)x->x_io[i]);
  }
  
  n = sp[0]->s_n;
  x->x_ms2tick = 0.001f * (t_float)(sp[0]->s_sr) / (t_float)n;
  x->x_8overn = 8.0f / (t_float)n;
  
  if(n&7)
    dsp_add(matrix_diag_mul_line8_tilde_perform_zero, 2, x, n);
  else
    dsp_add(matrix_diag_mul_line8_tilde_perf8, 2, x, n);
}


/* setup/setdown things */

static void matrix_diag_mul_line8_tilde_free(t_matrix_diag_mul_line8_tilde *x)
{
  freebytes(x->x_matcur, x->x_n_io * sizeof(t_float));
  freebytes(x->x_matend, x->x_n_io * sizeof(t_float));
  freebytes(x->x_inc8, x->x_n_io * sizeof(t_float));
  freebytes(x->x_biginc, x->x_n_io * sizeof(t_float));
  freebytes(x->x_io, 2 * x->x_n_io * sizeof(t_float *));
  if(x->x_buf)
    freebytes(x->x_buf, x->x_bufsize * sizeof(t_float));
}

static void *matrix_diag_mul_line8_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_matrix_diag_mul_line8_tilde *x = (t_matrix_diag_mul_line8_tilde *)pd_new(matrix_diag_mul_line8_tilde_class);
  int i, n;
  
  switch (argc)
  {
  case 0:
    x->x_n_io = 1;
    x->x_time_ms = 50.0f;
    break;
  case 1:
    x->x_n_io = (int)atom_getint(argv);
    x->x_time_ms = 50.0f;
    break;
  default:
    x->x_n_io = (int)atom_getint(argv);
    x->x_time_ms = atom_getfloat(argv+1);
    break;
  }
  
  if(x->x_time_ms < 0.0f)
    x->x_time_ms = 50.0f;
  if(x->x_n_io < 1)
    x->x_n_io = 1;
  i = x->x_n_io - 1;
  while(i--)
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  i = x->x_n_io;
  while(i--)
    outlet_new(&x->x_obj, &s_signal);
  x->x_msi = 0;
  x->x_buf = (t_float *)0;
  x->x_bufsize = 0;
  x->x_matcur = (t_float *)getbytes(x->x_n_io * sizeof(t_float));
  x->x_matend = (t_float *)getbytes(x->x_n_io * sizeof(t_float));
  x->x_inc8 = (t_float *)getbytes(x->x_n_io * sizeof(t_float));
  x->x_biginc = (t_float *)getbytes(x->x_n_io * sizeof(t_float));
  x->x_io = (t_float **)getbytes(2 * x->x_n_io * sizeof(t_float *));
  x->x_ms2tick = 0.001f * 44100.0f / 64.0f;
  x->x_8overn = 8.0f / 64.0f;
  x->x_remaining_ticks = 0;
  x->x_retarget = 0;
  
  n = x->x_n_io;
  for(i=0; i<n; i++)
  {
    x->x_matcur[i] = 0.0f;
    x->x_matend[i] = 0.0f;
    x->x_inc8[i] = 0.0f;
    x->x_biginc[i] = 0.0f;
  }
  return(x);
}

void matrix_diag_mul_line8_tilde_setup(void)
{
  matrix_diag_mul_line8_tilde_class = class_new(gensym("matrix_diag_mul_line8~"), (t_newmethod)matrix_diag_mul_line8_tilde_new, (t_method)matrix_diag_mul_line8_tilde_free,
    sizeof(t_matrix_diag_mul_line8_tilde), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(matrix_diag_mul_line8_tilde_class, t_matrix_diag_mul_line8_tilde, x_msi);
  class_addmethod(matrix_diag_mul_line8_tilde_class, (t_method)matrix_diag_mul_line8_tilde_dsp, gensym("dsp"), 0);
  class_addlist(matrix_diag_mul_line8_tilde_class, (t_method)matrix_diag_mul_line8_tilde_list);
  class_addmethod(matrix_diag_mul_line8_tilde_class, (t_method)matrix_diag_mul_line8_tilde_diag, gensym("diag"), A_GIMME, 0);
  class_addmethod(matrix_diag_mul_line8_tilde_class, (t_method)matrix_diag_mul_line8_tilde_element, gensym("element"), A_GIMME, 0);
  class_addmethod(matrix_diag_mul_line8_tilde_class, (t_method)matrix_diag_mul_line8_tilde_stop, gensym("stop"), 0);
  class_addmethod(matrix_diag_mul_line8_tilde_class, (t_method)matrix_diag_mul_line8_tilde_time, gensym("time"), A_FLOAT, 0);
//  class_sethelpsymbol(matrix_diag_mul_line8_tilde_class, gensym("iemhelp2/matrix_diag_mul_line8~-help"));
}
