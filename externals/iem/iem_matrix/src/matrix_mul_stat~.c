/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil (c) IEM KUG Graz Austria 2002 - 2007 */

#include "m_pd.h"
#include "iemlib.h"


/* ---------- matrix_mul_stat~ - signal matrix multiplication object with message matrix-coeff. ----------- */

typedef struct matrix_mul_stat_tilde
{
  t_object  x_obj;
  t_float   *x_matbuf;
  t_float   **x_io;
  t_float   *x_outsumbuf;
  int       x_outsumbufsize;
  int       x_n_in; /* columns */
  int       x_n_out;   /* rows  */
  t_float   x_msi;
} t_matrix_mul_stat_tilde;

t_class *matrix_mul_stat_tilde_class;

static void matrix_mul_stat_tilde_matrix(t_matrix_mul_stat_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int col, row, i;
  t_float *matrix = x->x_matbuf;
  
  if(argc<2)
  {
    post("matrix_mul_stat~ : bad matrix: <int> out_rows <int> in_cols !");
    return;
  }
  
  row = atom_getint(argv);
  argv++;
  col = atom_getint(argv);
  argv++;
  argc-=2;
  
  if((col!=x->x_n_in)||(row!=x->x_n_out))
  {
    post("matrix_mul_stat~ : matrix dimensions do not match !!");
    return;
  }
  if(argc<row*col)
  {
    post("matrix_mul_stat~ : reduced matrices not yet supported");
    return;
  }
  
  col *= row;
  for(i=0; i<col; i++)
  {
    *matrix++ = atom_getfloat(argv);
    argv++;
  }
}

static void matrix_mul_stat_tilde_element(t_matrix_mul_stat_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int col, row, n_in_cols=x->x_n_in;
  t_float element; 
  t_float *matrix = x->x_matbuf;
  
  if(argc != 3)
  {
    post("matrix_mul_stat~ : bad element: 3 floats: <int> out_row <int> in_col <float> element !");
    return;
  }
  
  row = atom_getint(argv) - 1;
  col = atom_getint(argv+1) - 1;
  element = atom_getfloat(argv+2);
  
  if((row >= x->x_n_out) || (row < 0))
  {
    post("matrix_mul_stat~ : row dimensions do not match !!");
    return;
  }
  if((col >= n_in_cols) || (col < 0))
  {
    post("matrix_mul_stat~ : col dimensions do not match !!");
    return;
  }
  
  matrix += row * n_in_cols + col;
  
  *matrix = element;
}

static void matrix_mul_stat_tilde_row(t_matrix_mul_stat_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int col, nth_row, i;
  t_float *matrix = x->x_matbuf;
  
  if(argc<1)
  {
    post("matrix_mul_stat~ : bad row: <int> in_rows !");
    return;
  }
  
  nth_row = atom_getint(argv) - 1;
  argv++;
  argc--;
  
  if((nth_row < 0)||(nth_row >= x->x_n_out))
  {
    post("matrix_mul_stat~ : row dimensions do not match !!");
    return;
  }
  col = x->x_n_in;
  if(argc < col)
  {
    post("matrix_mul_stat~ : col dimensions do not match !!");
    return;
  }
  
  matrix += nth_row * col;
  for(i=0; i<col; i++)
  {
    *matrix++ = atom_getfloat(argv);
    argv++;
  }
}

static void matrix_mul_stat_tilde_col(t_matrix_mul_stat_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
  int row, col, nth_col, i;
  t_float *matrix = x->x_matbuf;
  
  if(argc<1)
  {
    post("matrix_mul_stat~ : bad col: <int> in_cols !");
    return;
  }
  
  nth_col = atom_getint(argv) - 1;
  argv++;
  argc--;
  
  col = x->x_n_in;
  if((nth_col < 0)||(nth_col >= col))
  {
    post("matrix_mul_stat~ : col dimensions do not match !!");
    return;
  }
  row = x->x_n_out;
  if(argc < row)
  {
    post("matrix_mul_stat~ : row dimensions do not match !!");
    return;
  }
  
  matrix += nth_col;
  for(i=0; i<row; i++)
  {
    *matrix = atom_getfloat(argv);
    argv++;
    matrix += col;
  }
}

/* the dsp thing */

static t_int *matrix_mul_stat_tilde_perform(t_int *w)
{
  t_matrix_mul_stat_tilde *x = (t_matrix_mul_stat_tilde *)(w[1]);
  int n = (int)(w[2]);
  t_float **io = x->x_io;
  t_float *outsum, *houtsum;
  t_float *mat  = x->x_matbuf;
  int n_in = x->x_n_in;   /* columns */
  int n_out = x->x_n_out; /* rows */
  t_float *in, *out, mul;
  int r, c, i;
  
  /* 1. output-vector-row */
  in = io[0];
  houtsum = x->x_outsumbuf;
  outsum = houtsum;
  mul = *mat++;
  for(i=0; i<n; i++)/* 1. element of 1. row */
  {
    *outsum++ = *in++ * mul;
  }
  for(c=1; c<n_in; c++)/* c+1. element of 1. row */
  {
    in = io[c];
    outsum = x->x_outsumbuf;
    mul = *mat++;
    for(i=0; i<n; i++)
    {
      *outsum++ += *in++ * mul;
    }
  }
  for(r=1; r<n_out; r++)/* 2. .. n_out. output-vector-row */
  {
    in = io[0];
    houtsum += n;
    outsum = houtsum;
    mul = *mat++;
    for(i=0; i<n; i++)/* 1. element of r+1. row */
    {
      *outsum++ = *in++ * mul;
    }
    for(c=1; c<n_in; c++)/* c+1. element of r+1. row */
    {
      in = io[c];
      outsum = houtsum;
      mul = *mat++;
      for(i=0; i<n; i++)
      {
        *outsum++ += *in++ * mul;
      }
    }
  }
  outsum = x->x_outsumbuf;
  for(r=0; r<n_out; r++)/* output-vector-row */
  {
    out = io[n_in+r];
    for(i=0; i<n; i++)
    {
      *out++ = *outsum++;
    }
  }
  return (w+3);
}

static t_int *matrix_mul_stat_tilde_perf8(t_int *w)
{
  t_matrix_mul_stat_tilde *x = (t_matrix_mul_stat_tilde *)(w[1]);
  int n = (int)(w[2]);
  
  t_float **io = x->x_io;
  t_float *outsum, *houtsum;
  t_float *mat  = x->x_matbuf;
  int n_in = x->x_n_in;   /* columns */
  int n_out = x->x_n_out; /* rows */
  t_float *in, *out, mul;
  int r, c, i;
  
  /* 1. output-vector-row */
  houtsum = x->x_outsumbuf;
  outsum = houtsum;
  mul = *mat++;
  if(mul == 0.0f)
  {
    for(i=n; i; i -= 8, outsum += 8)
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
    in = io[0];
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
    mul = *mat++;
    if(mul != 0.0f)
    {
      in = io[c];
      outsum = houtsum;
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
    houtsum += n;
    outsum = houtsum;
    mul = *mat++;
    if(mul == 0.0f)
    {
      for(i=n; i; i -= 8, outsum += 8)
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
      in = io[0];
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
      mul = *mat++;
      if(mul != 0.0f)
      {
        in = io[c];
        outsum = houtsum;
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

static void matrix_mul_stat_tilde_dsp(t_matrix_mul_stat_tilde *x, t_signal **sp)
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
  {
    x->x_io[i] = sp[i]->s_vec;
    /*post("iovec_addr = %d", (unsigned int)x->x_io[i]);*/
  }
  
  n = sp[0]->s_n;
  if(n&7)
    dsp_add(matrix_mul_stat_tilde_perform, 2, x, n);
  else
    dsp_add(matrix_mul_stat_tilde_perf8, 2, x, n);
}


/* setup/setdown things */

static void matrix_mul_stat_tilde_free(t_matrix_mul_stat_tilde *x)
{
  freebytes(x->x_matbuf, x->x_n_in * x->x_n_out * sizeof(t_float));
  freebytes(x->x_io, (x->x_n_in + x->x_n_out) * sizeof(t_float *));
  if(x->x_outsumbuf)
    freebytes(x->x_outsumbuf, x->x_outsumbufsize * sizeof(t_float));
}

static void *matrix_mul_stat_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_matrix_mul_stat_tilde *x = (t_matrix_mul_stat_tilde *)pd_new(matrix_mul_stat_tilde_class);
  int i;
  
  switch (argc)
  {
  case 0:
    x->x_n_in = x->x_n_out = 1;
    break;
  case 1:
    x->x_n_in = x->x_n_out = (int)atom_getint(argv);
    break;
  default:
    x->x_n_in = (int)atom_getint(argv);
    x->x_n_out = (int)atom_getint(argv+1);
    break;
  }
  
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
  x->x_matbuf = (t_float *)getbytes(x->x_n_in * x->x_n_out * sizeof(t_float));
  x->x_io = (t_float **)getbytes((x->x_n_in + x->x_n_out) * sizeof(t_float *));
  return (x);
}

void matrix_mul_stat_tilde_setup(void)
{
  matrix_mul_stat_tilde_class = class_new(gensym("matrix_mul_stat~"), (t_newmethod)matrix_mul_stat_tilde_new, (t_method)matrix_mul_stat_tilde_free,
    sizeof(t_matrix_mul_stat_tilde), 0, A_GIMME, 0);
  CLASS_MAINSIGNALIN(matrix_mul_stat_tilde_class, t_matrix_mul_stat_tilde, x_msi);
  class_addmethod(matrix_mul_stat_tilde_class, (t_method)matrix_mul_stat_tilde_dsp, gensym("dsp"), 0);
  class_addmethod(matrix_mul_stat_tilde_class, (t_method)matrix_mul_stat_tilde_matrix, gensym("matrix"), A_GIMME, 0);
  class_addmethod(matrix_mul_stat_tilde_class, (t_method)matrix_mul_stat_tilde_element, gensym("element"), A_GIMME, 0);
  class_addmethod(matrix_mul_stat_tilde_class, (t_method)matrix_mul_stat_tilde_row, gensym("row"), A_GIMME, 0);
  class_addmethod(matrix_mul_stat_tilde_class, (t_method)matrix_mul_stat_tilde_col, gensym("col"), A_GIMME, 0);
//  class_sethelpsymbol(matrix_mul_stat_tilde_class, gensym("iemhelp2/matrix_mul_stat~-help"));
}
