/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2007 */

#include "m_pd.h"
#include "iemlib.h"


/* ---------- matrix_mul_line~ - signal matrix multiplication object with message matrix-coeff. ----------- */

typedef struct matrix_mul_line_tilde
{
  t_object  x_obj;
  t_float   *x_matcur;
  t_float   *x_matend;
  t_float   *x_inc;
  t_float   *x_biginc;
  t_float   **x_io;
  t_float   *x_outsumbuf;
  int       x_outsumbufsize;
  int       x_n_in; /* columns */
  int       x_n_out; /* rows  */
  t_float   x_msi;
  int       x_retarget;
  t_float   x_time_ms;
  int       x_remaining_ticks;
  t_float   x_ms2tick;
  t_float   x_1overn;
} t_matrix_mul_line_tilde;

t_class *matrix_mul_line_tilde_class;

static void matrix_mul_line_tilde_time(t_matrix_mul_line_tilde *x, t_floatarg time_ms)
{
  if(time_ms <= 0.0f)
    time_ms = 0.0f;
  x->x_time_ms = time_ms;
}

static void matrix_mul_line_tilde_matrix(t_matrix_mul_line_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int col, row, i;
  t_float *matcur = x->x_matcur;
  t_float *matend = x->x_matend;
  
  if(argc<2)
  {
    post("matrix_mul_line~ : bad matrix: <int> out_rows <int> in_cols !");
    return;
  }
  
  row = atom_getint(argv);
  argv++;
  col = atom_getint(argv);
  argv++;
  argc-=2;
  
  if((col!=x->x_n_in)||(row!=x->x_n_out))
  {
    post("matrix_mul_line~ : matrix dimensions do not match !!");
    return;
  }
  if(argc<row*col)
  {
    post("matrix_mul_line~ : reduced matrices not yet supported");
    return;
  }
  
  col *= row;
  if(x->x_time_ms <= 0.0f)
  {
    for(i=0; i<col; i++)
    {
      *matend++ = *matcur++ = atom_getfloat(argv);
      argv++;
    }
    x->x_remaining_ticks = x->x_retarget = 0;
  }
  else
  {
    for(i=0; i<col; i++)
    {
      *matend++ = atom_getfloat(argv);
      argv++;
    }
    x->x_retarget = 1;
  }
}

static void matrix_mul_line_tilde_element(t_matrix_mul_line_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int col, row, n_in_cols=x->x_n_in;
  t_float element; 
  t_float *matcur = x->x_matcur;
  t_float *matend = x->x_matend;
  
  if(argc != 3)
  {
    post("matrix_mul_line~ : bad element: 3 floats: <int> out_row <int> in_col <float> element !");
    return;
  }
  
  row = atom_getint(argv) - 1;
  col = atom_getint(argv+1) - 1;
  element = atom_getfloat(argv+2);
  
  if((row >= x->x_n_out) || (row < 0))
  {
    post("matrix_mul_line~ : row dimensions do not match !!");
    return;
  }
  if((col >= n_in_cols) || (col < 0))
  {
    post("matrix_mul_line~ : col dimensions do not match !!");
    return;
  }
  
  matend += row * n_in_cols + col;
  matcur += row * n_in_cols + col;
  
  if(x->x_time_ms <= 0.0f)
  {
    *matend = *matcur = element;
    x->x_remaining_ticks = x->x_retarget = 0;
  }
  else
  {
    *matend = element;
    x->x_retarget = 1;
  }
}

static void matrix_mul_line_tilde_row(t_matrix_mul_line_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int col, nth_row, i;
  t_float *matcur = x->x_matcur;
  t_float *matend = x->x_matend;
  
  if(argc<1)
  {
    post("matrix_mul_line~ : bad row: <int> in_row !");
    return;
  }
  
  nth_row = atom_getint(argv) - 1;
  argv++;
  argc--;
  
  if((nth_row >= x->x_n_out) || (nth_row < 0))
  {
    post("matrix_mul_line~ : row dimensions do not match !!");
    return;
  }
  col = x->x_n_in;
  if(argc < col)
  {
    post("matrix_mul_line~ : col dimensions do not match !!");
    return;
  }
  
  matend += nth_row * col;
  matcur += nth_row * col;
  if(x->x_time_ms <= 0.0f)
  {
    for(i=0; i<col; i++)
    {
      *matend++ = *matcur++ = atom_getfloat(argv);
      argv++;
    }
    x->x_remaining_ticks = x->x_retarget = 0;
  }
  else
  {
    for(i=0; i<col; i++)
    {
      *matend++ = atom_getfloat(argv);
      argv++;
    }
    x->x_retarget = 1;
  }
}

static void matrix_mul_line_tilde_col(t_matrix_mul_line_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int row, col, nth_col, i;
  t_float *matcur = x->x_matcur;
  t_float *matend = x->x_matend;
  
  if(argc<1)
  {
    post("matrix_mul_line~ : bad col: <int> in_cols !");
    return;
  }
  
  nth_col = atom_getint(argv) - 1;
  argv++;
  argc--;
  
  col = x->x_n_in;
  if((nth_col < 0)||(nth_col >= col))
  {
    post("matrix_mul_line~ : col dimensions do not match !!");
    return;
  }
  row = x->x_n_out;
  if(argc < row)
  {
    post("matrix_mul_line~ : row dimensions do not match !!");
    return;
  }
  
  matend += nth_col;
  matcur += nth_col;
  if(x->x_time_ms <= 0.0f)
  {
    for(i=0; i<row; i++)
    {
      *matend = *matcur = atom_getfloat(argv);
      argv++;
      matend += col;
      matcur += col;
    }
    x->x_remaining_ticks = x->x_retarget = 0;
  }
  else
  {
    for(i=0; i<row; i++)
    {
      *matend = atom_getfloat(argv);
      argv++;
      matend += col;
      matcur += col;
    }
    x->x_retarget = 1;
  }
}

static void matrix_mul_line_tilde_stop(t_matrix_mul_line_tilde *x)
{
  int i = x->x_n_out*x->x_n_in;
  t_float *matend=x->x_matend;
  t_float *matcur=x->x_matcur;
  
  while(i--)
  {
    *matend++ = *matcur++;
  }
  x->x_remaining_ticks = x->x_retarget = 0;
}

/* the dsp thing */

static t_int *matrix_mul_line_tilde_perform_zero(t_int *w)
{
  t_matrix_mul_line_tilde *x = (t_matrix_mul_line_tilde *)(w[1]);
  int n = (int)(w[2]);
  t_float **io = x->x_io;
  int n_in = x->x_n_in;   /* columns */
  int n_out = x->x_n_out; /* rows */
  t_float *out;
  int r, i;
  
  for(r=0; r<n_out; r++)/* output-vector-row */
  {
    out = io[n_in+r];
    for(i=0; i<n; i++)
      *out++ = 0.0f;
  }
  return (w+3);
}

static t_int *matrix_mul_line_tilde_perf8(t_int *w)
{
  t_matrix_mul_line_tilde *x = (t_matrix_mul_line_tilde *)(w[1]);
  int n = (int)(w[2]);
  t_float **io = x->x_io;
  t_float *outsum, *houtsum;
  t_float *matcur, *matend;
  t_float *inc1 ,*biginc, inc;
  int n_in = x->x_n_in;   /* columns */
  int n_out = x->x_n_out; /* rows */
  t_float *in, *out, mul, bigmul;
  int r, c, i;
  
  if(x->x_retarget)
  {
    int nticks = (int)(x->x_time_ms * x->x_ms2tick);
    
    if(!nticks)
      nticks = 1;
    x->x_remaining_ticks = nticks;
    inc1 = x->x_inc;
    biginc = x->x_biginc;
    matcur = x->x_matcur;
    matend = x->x_matend;
    mul = x->x_1overn / (t_float)nticks;
    bigmul = 1.0f / (t_float)nticks;
    i = n_out*n_in;
    while(i--)
    {
      inc = *matend++ - *matcur++;
      *inc1++ = inc * mul;
      *biginc++ = inc * bigmul;
    }
    x->x_retarget = 0;
    //post("time = %f ms, ticks = %d, inc = %f", x->x_time_ms, nticks, *inc);
  }
  
  if(x->x_remaining_ticks)
  {
    inc1 = x->x_inc;
    biginc = x->x_biginc;
    matcur = x->x_matcur;
    /* 1. output-vector-row */
    in = io[0];
    houtsum = x->x_outsumbuf;
    outsum = houtsum;
    inc = *inc1++;
    mul = *matcur;
    for(i=n; i; i -= 8, outsum += 8, in += 8)
    {
      outsum[0] = in[0] * mul;
      mul += inc;
      outsum[1] = in[1] * mul;
      mul += inc;
      outsum[2] = in[2] * mul;
      mul += inc;
      outsum[3] = in[3] * mul;
      mul += inc;
      outsum[4] = in[4] * mul;
      mul += inc;
      outsum[5] = in[5] * mul;
      mul += inc;
      outsum[6] = in[6] * mul;
      mul += inc;
      outsum[7] = in[7] * mul;
      mul += inc;
    }
    *matcur++ += *biginc++;
    for(c=1; c<n_in; c++)/* c+1. element of 1. row */
    {
      in = io[c];
      outsum = houtsum;
      inc = *inc1++;
      mul = *matcur;
      for(i=n; i; i -= 8, outsum += 8, in += 8)
      {
        outsum[0] += in[0] * mul;
        mul += inc;
        outsum[1] += in[1] * mul;
        mul += inc;
        outsum[2] += in[2] * mul;
        mul += inc;
        outsum[3] += in[3] * mul;
        mul += inc;
        outsum[4] += in[4] * mul;
        mul += inc;
        outsum[5] += in[5] * mul;
        mul += inc;
        outsum[6] += in[6] * mul;
        mul += inc;
        outsum[7] += in[7] * mul;
        mul += inc;
      }
      *matcur++ += *biginc++;
    }
    for(r=1; r<n_out; r++)/* 2. .. n_out. output-vector-row */
    {
      in = io[0];
      houtsum += n;
      outsum = houtsum;
      inc = *inc1++;
      mul = *matcur;
      for(i=n; i; i -= 8, outsum += 8, in += 8)
      {
        outsum[0] = in[0] * mul;
        mul += inc;
        outsum[1] = in[1] * mul;
        mul += inc;
        outsum[2] = in[2] * mul;
        mul += inc;
        outsum[3] = in[3] * mul;
        mul += inc;
        outsum[4] = in[4] * mul;
        mul += inc;
        outsum[5] = in[5] * mul;
        mul += inc;
        outsum[6] = in[6] * mul;
        mul += inc;
        outsum[7] = in[7] * mul;
        mul += inc;
      }
      *matcur++ += *biginc++;
      for(c=1; c<n_in; c++)/* c+1. element of r+1. row */
      {
        in = io[c];
        outsum = houtsum;
        inc = *inc1++;
        mul = *matcur;
        for(i=n; i; i -= 8, outsum += 8, in += 8)
        {
          outsum[0] += in[0] * mul;
          mul += inc;
          outsum[1] += in[1] * mul;
          mul += inc;
          outsum[2] += in[2] * mul;
          mul += inc;
          outsum[3] += in[3] * mul;
          mul += inc;
          outsum[4] += in[4] * mul;
          mul += inc;
          outsum[5] += in[5] * mul;
          mul += inc;
          outsum[6] += in[6] * mul;
          mul += inc;
          outsum[7] += in[7] * mul;
          mul += inc;
        }
        *matcur++ += *biginc++;
      }
    }
    
    if(!--x->x_remaining_ticks)
    {
      matcur = x->x_matcur;
      matend = x->x_matend;
      i = n_in * n_out;
      while(i--)
        *matcur++ = *matend++;
    }
  }
  else
  {
    matend = x->x_matend;
    /* 1. output-vector-row */
    in = io[0];
    houtsum = x->x_outsumbuf;
    outsum = houtsum;
    mul = *matend++;
    if(mul == 0.0f)
    {
      for(i=n; i; i -= 8, outsum += 8, in += 8)
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
    else
    {
      for(i=n; i; i -= 8, outsum += 8, in += 8)
      {
        outsum[0] = in[0] * mul;
        outsum[1] = in[1] * mul;
        outsum[2] = in[2] * mul;
        outsum[3] = in[3] * mul;
        outsum[4] = in[4] * mul;
        outsum[5] = in[5] * mul;
        outsum[6] = in[6] * mul;
        outsum[7] = in[7] * mul;
      }
    }
    for(c=1; c<n_in; c++)/* c+1. element of 1. row */
    {
      in = io[c];
      outsum = houtsum;
      mul = *matend++;
      if(mul != 0.0f)
      {
        for(i=n; i; i -= 8, outsum += 8, in += 8)
        {
          outsum[0] += in[0] * mul;
          outsum[1] += in[1] * mul;
          outsum[2] += in[2] * mul;
          outsum[3] += in[3] * mul;
          outsum[4] += in[4] * mul;
          outsum[5] += in[5] * mul;
          outsum[6] += in[6] * mul;
          outsum[7] += in[7] * mul;
        }
      }
    }
    for(r=1; r<n_out; r++)/* 2. .. n_out. output-vector-row */
    {
      in = io[0];
      houtsum += n;
      outsum = houtsum;
      mul = *matend++;
      if(mul == 0.0f)
      {
        for(i=n; i; i -= 8, outsum += 8, in += 8)
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
      else
      {
        for(i=n; i; i -= 8, outsum += 8, in += 8)
        {
          outsum[0] = in[0] * mul;
          outsum[1] = in[1] * mul;
          outsum[2] = in[2] * mul;
          outsum[3] = in[3] * mul;
          outsum[4] = in[4] * mul;
          outsum[5] = in[5] * mul;
          outsum[6] = in[6] * mul;
          outsum[7] = in[7] * mul;
        }
      }
      for(c=1; c<n_in; c++)/* c+1. element of r+1. row */
      {
        in = io[c];
        outsum = houtsum;
        mul = *matend++;
        if(mul != 0.0f)
        {
          for(i=n; i; i -= 8, outsum += 8, in += 8)
          {
            outsum[0] += in[0] * mul;
            outsum[1] += in[1] * mul;
            outsum[2] += in[2] * mul;
            outsum[3] += in[3] * mul;
            outsum[4] += in[4] * mul;
            outsum[5] += in[5] * mul;
            outsum[6] += in[6] * mul;
            outsum[7] += in[7] * mul;
          }
        }
      }
    }
  }
  outsum = x->x_outsumbuf;
  for(r=0; r<n_out; r++)/* output-vector-row */
  {
    out = io[n_in+r];
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

static void matrix_mul_line_tilde_dsp(t_matrix_mul_line_tilde *x, t_signal **sp)
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
  x->x_ms2tick = 0.001f * (t_float)(sp[0]->s_sr) / (t_float)n;
  x->x_1overn = 1.0f / (t_float)n;
  
  if(n&7)
  {
    dsp_add(matrix_mul_line_tilde_perform_zero, 2, x, n);
    post("ERROR!!! matrix_mul_line~ : blocksize is %d and not a multiple of 8", n);
  }
  else
    dsp_add(matrix_mul_line_tilde_perf8, 2, x, n);
}


/* setup/setdown things */

static void matrix_mul_line_tilde_free(t_matrix_mul_line_tilde *x)
{
  freebytes(x->x_matcur, x->x_n_in * x->x_n_out * sizeof(t_float));
  freebytes(x->x_matend, x->x_n_in * x->x_n_out * sizeof(t_float));
  freebytes(x->x_inc, x->x_n_in * x->x_n_out * sizeof(t_float));
  freebytes(x->x_biginc, x->x_n_in * x->x_n_out * sizeof(t_float));
  freebytes(x->x_io, (x->x_n_in + x->x_n_out) * sizeof(t_float *));
  if(x->x_outsumbuf)
    freebytes(x->x_outsumbuf, x->x_outsumbufsize * sizeof(t_float));
}

static void *matrix_mul_line_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_matrix_mul_line_tilde *x = (t_matrix_mul_line_tilde *)pd_new(matrix_mul_line_tilde_class);
  int i, n;
  
  switch(argc)
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
  
  if(x->x_time_ms < 0.0f)
    x->x_time_ms = 50.0f;
  if(x->x_n_in < 1)
    x->x_n_in = 1;
  if(x->x_n_out < 1)
    x->x_n_out = 1;
  i = x->x_n_in - 1;
  while(i--)
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  i = x->x_n_out;
  while(i--)
    outlet_new(&x->x_obj, &s_signal);
  x->x_msi = 0;
  x->x_outsumbuf = (t_float *)0;
  x->x_outsumbufsize = 0;
  x->x_matcur = (t_float *)getbytes(x->x_n_in * x->x_n_out * sizeof(t_float));
  x->x_matend = (t_float *)getbytes(x->x_n_in * x->x_n_out * sizeof(t_float));
  x->x_inc = (t_float *)getbytes(x->x_n_in * x->x_n_out * sizeof(t_float));
  x->x_biginc = (t_float *)getbytes(x->x_n_in * x->x_n_out * sizeof(t_float));
  x->x_io = (t_float **)getbytes((x->x_n_in + x->x_n_out) * sizeof(t_float *));
  x->x_ms2tick = 0.001f * 44100.0f / 64.0f;
  x->x_1overn = 1.0f / 64.0f;
  x->x_remaining_ticks = 0;
  x->x_retarget = 0;
  
  n = x->x_n_in * x->x_n_out;
  for(i=0; i<n; i++)
  {
    x->x_matcur[i] = 0.0f;
    x->x_matend[i] = 0.0f;
    x->x_inc[i] = 0.0f;
    x->x_biginc[i] = 0.0f;
  }
  return (x);
}

void matrix_mul_line_tilde_setup(void)
{
  matrix_mul_line_tilde_class = class_new(gensym("matrix_mul_line~"), (t_newmethod)matrix_mul_line_tilde_new, (t_method)matrix_mul_line_tilde_free,
    sizeof(t_matrix_mul_line_tilde), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(matrix_mul_line_tilde_class, t_matrix_mul_line_tilde, x_msi);
  class_addmethod(matrix_mul_line_tilde_class, (t_method)matrix_mul_line_tilde_dsp, gensym("dsp"), 0);
  class_addmethod(matrix_mul_line_tilde_class, (t_method)matrix_mul_line_tilde_matrix, gensym("matrix"), A_GIMME, 0);
  class_addmethod(matrix_mul_line_tilde_class, (t_method)matrix_mul_line_tilde_element, gensym("element"), A_GIMME, 0);
  class_addmethod(matrix_mul_line_tilde_class, (t_method)matrix_mul_line_tilde_row, gensym("row"), A_GIMME, 0);
  class_addmethod(matrix_mul_line_tilde_class, (t_method)matrix_mul_line_tilde_col, gensym("col"), A_GIMME, 0);
  class_addmethod(matrix_mul_line_tilde_class, (t_method)matrix_mul_line_tilde_stop, gensym("stop"), 0);
  class_addmethod(matrix_mul_line_tilde_class, (t_method)matrix_mul_line_tilde_time, gensym("time"), A_FLOAT, 0);
//  class_sethelpsymbol(matrix_mul_line_tilde_class, gensym("iemhelp2/matrix_mul_line~-help"));
}
