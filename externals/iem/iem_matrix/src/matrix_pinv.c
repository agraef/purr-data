/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil (c) IEM KUG Graz Austria 2002 - 2006 */

#include "m_pd.h"

/* -------------------------- matrix_pinv ------------------------------ */

typedef struct _matrix_pinv
{
  t_object  x_obj;
  int       x_dim;
  int       x_size;
  double    *x_work2;
  double    *x_buf2;
  t_atom    *x_at;
} t_matrix_pinv;

static t_class *matrix_pinv_class;




static void matrix_pinv_copy_row2buf(t_matrix_pinv *x, int row)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  double *db=x->x_buf2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
    *db++ = *dw++;
}

static void matrix_pinv_copy_buf2row(t_matrix_pinv *x, int row)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  double *db=x->x_buf2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
    *dw++ = *db++;
}

static void matrix_pinv_copy_row2row(t_matrix_pinv *x, int src_row, int dst_row)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw_src=x->x_work2;
  double *dw_dst=x->x_work2;
  
  dw_src += src_row*dim2;
  dw_dst += dst_row*dim2;
  for(i=0; i<dim2; i++)
  {
    *dw_dst++ = *dw_src++;
  }
}

static void matrix_pinv_xch_rows(t_matrix_pinv *x, int row1, int row2)
{
  matrix_pinv_copy_row2buf(x, row1);
  matrix_pinv_copy_row2row(x, row2, row1);
  matrix_pinv_copy_buf2row(x, row2);
}

static void matrix_pinv_mul_row(t_matrix_pinv *x, int row, double mul)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
    (*dw++) *= mul;
}

static void matrix_pinv_mul_col(t_matrix_pinv *x, int col, double mul)
{
  int dim = x->x_dim;
  int dim2 = 2*dim;
  int i;
  double *dw=x->x_work2;
  
  dw += col;
  for(i=0; i<dim; i++)
  {
    (*dw) *= mul;
    dw += dim2;
  }
}

static void matrix_pinv_mul_buf_and_add2row(t_matrix_pinv *x, int row, double mul)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  double *db=x->x_buf2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
    *dw++ += (*db++)*mul;
}

static int matrix_pinv_eval_which_element_of_col_not_zero(t_matrix_pinv *x, int col, int start_row)
{
  int dim = x->x_dim;
  int dim2 = 2*dim;
  int i, j;
  double *dw=x->x_work2;
  int ret=-1;
  
  dw += start_row*dim2 + col;
  j = 0;
  for(i=start_row; i<dim; i++)
  {
    if((*dw > 1.0e-10) || (*dw < -1.0e-10))
    {
      ret = i;
      i = dim+1;
    }
    dw += dim2;
  }
  return(ret);
}


static void matrix_pinv_inverse(t_matrix_pinv *x)
{
  int n_ambi = x->x_n_ambi;
  int n_ambi2 = 2*n_ambi;
  int i, j, nz;
  int r,c;
  double *src=x->x_inv_work1;
  double *db=x->x_inv_work2;
  double *acw_vec=x->x_ambi_channel_weight;
  double rcp, *dv;
  
  dv = db;
  for(i=0; i<n_ambi; i++) /* init */
  {
    for(j=0; j<n_ambi; j++)
    {
      *dv++ = *src++;
    }
    for(j=0; j<n_ambi; j++)
    {
      if(j == i)
        *dv++ = 1.0;
      else
        *dv++ = 0.0;
    }
  }
  /* make 1 in main-diagonale, and 0 below */
  for(i=0; i<n_ambi; i++)
  {
    nz = matrix_pinv_eval_which_element_of_col_not_zero(x, i, i);
    if(nz < 0)
    {
      post("matrix_pinv ERROR: matrix not regular !!!!");
      return;
    }
    else
    {
      if(nz != i)
        matrix_pinv_xch_rows(x, i, nz);
      dv = db + i*n_ambi2 + i;
      rcp = 1.0 /(*dv);
      matrix_pinv_mul_row(x, i, rcp);
      matrix_pinv_copy_row2buf(x, i);
      for(j=i+1; j<n_ambi; j++)
      {
        dv += n_ambi2;
        rcp = -(*dv);
        matrix_pinv_mul_buf_and_add2row(x, j, rcp);
      }
    }
  }
  /* make 0 above the main diagonale */
  for(i=n_ambi-1; i>=0; i--)
  {
    dv = db + i*n_ambi2 + i;
    matrix_pinv_copy_row2buf(x, i);
    for(j=i-1; j>=0; j--)
    {
      dv -= n_ambi2;
      rcp = -(*dv);
      matrix_pinv_mul_buf_and_add2row(x, j, rcp);
    }
  }
  
  for(i=0; i<n_ambi; i++)
  {
    matrix_pinv_mul_col(x, i+n_ambi, acw_vec[i]);
  }
  
  post("matrix_inverse regular");
}































static void matrix_pinv_matrix(t_matrix_pinv *x, t_symbol *s, int argc, t_atom *argv)
{
  int dim = x->x_dim;
  int dim2 = 2*dim;
  int i, j, nz;
  int r,c;
  double *db=x->x_work2;
  double rcp, *dv=db;
  t_atom *at=x->x_at;
  
  if(argc != (dim*dim + 2))
  {
    post("matrix_pinv ERROR: wrong dimension of input-list");
    return;
    
    
    if(ac > x->x_size)
    {
      x->x_at = (t_atom *)resizebytes(x->x_at, x->x_size*sizeof(t_atom), ac*sizeof(t_atom));
      x->x_size = ac;
    }
    x->x_ac = ac;
    x->x_sym = &s_list;
    
  }
  r = (int)(atom_getint(argv++));
  c = (int)(atom_getint(argv++));
  if(r != dim)
  {
    post("matrix_pinv ERROR: wrong number of rows of input-list");
    return;
  }
  if(c != dim)
  {
    post("matrix_pinv ERROR: wrong number of cols of input-list");
    return;
  }
  for(i=0; i<dim; i++) /* init */
  {
    for(j=0; j<dim; j++)
    {
      *dv++ = (double)(atom_getfloat(argv++));
    }
    for(j=0; j<dim; j++)
    {
      if(j == i)
        *dv++ = 1.0;
      else
        *dv++ = 0.0;
    }
  }
  
  /* make 1 in main-diagonale, and 0 below */
  for(i=0; i<dim; i++)
  {
    nz = matrix_pinv_eval_which_element_of_col_not_zero(x, i, i);
    if(nz < 0)
    {
      post("matrix_pinv ERROR: matrix not regular");
      return;
    }
    else
    {
      if(nz != i)
      {
        matrix_pinv_xch_rows(x, i, nz);
      }
      dv = db + i*dim2 + i;
      rcp = 1.0 /(*dv);
      matrix_pinv_mul_row(x, i, rcp);
      matrix_pinv_copy_row2buf(x, i);
      for(j=i+1; j<dim; j++)
      {
        dv += dim2;
        rcp = -(*dv);
        matrix_pinv_mul_buf_and_add2row(x, j, rcp);
      }
    }
  }
  
  /* make 0 above the main diagonale */
  for(i=dim-1; i>=0; i--)
  {
    dv = db + i*dim2 + i;
    matrix_pinv_copy_row2buf(x, i);
    for(j=i-1; j>=0; j--)
    {
      dv -= dim2;
      rcp = -(*dv);
      matrix_pinv_mul_buf_and_add2row(x, j, rcp);
    }
  }
  
  
  at = x->x_at;
  SETFLOAT(at, (t_float)dim);
  at++;
  SETFLOAT(at, (t_float)dim);
  at++;
  dv = db;
  dv += dim;
  for(i=0; i<dim; i++)    /* output left half of double-matrix */
  {
    for(j=0; j<dim; j++)
    {
      SETFLOAT(at, (t_float)(*dv++));
      at++;
    }
    dv += dim;
  }
  
  outlet_anything(x->x_obj.ob_outlet, gensym("matrix"), argc, x->x_at);
}

























static void matrix_pinv_mul1(t_matrix_pinv *x)
{
  double *vec1, *beg1=x->x_ls_encode;
  double *vec2, *beg2=x->x_ls_encode;
  double *inv=x->x_inv_work1;
  double sum;
  int n_ls=x->x_n_ls+x->x_n_phls;
  int n_ambi=x->x_n_ambi;
  int i, j, k;
  
  for(k=0; k<n_ambi; k++)
  {
    beg2=x->x_ls_encode;
    for(j=0; j<n_ambi; j++)
    {
      sum = 0.0;
      vec1 = beg1;
      vec2 = beg2;
      for(i=0; i<n_ls; i++)
      {
        sum += *vec1++ * *vec2++;
      }
      beg2 += n_ls;
      *inv++ = sum;
    }
    beg1 += n_ls;
  }
}

static void matrix_pinv_mul2(t_matrix_pinv *x)
{
  int n_ls=x->x_n_ls+x->x_n_phls;
  int n_ambi=x->x_n_ambi;
  int n_ambi2=2*x->x_n_ambi;
  int i, j, k;
  double *vec1, *beg1=x->x_transp;
  double *vec2, *beg2=x->x_inv_work2+n_ambi;
  double *vec3=x->x_prod;
  double *acw_vec=x->x_ambi_channel_weight;
  double sum;
  
  for(k=0; k<n_ls; k++)
  {
    beg2=x->x_inv_work2+n_ambi;
    for(j=0; j<n_ambi; j++)
    {
      sum = 0.0;
      vec1 = beg1;
      vec2 = beg2;
      for(i=0; i<n_ambi; i++)
      {
        sum += *vec1++ * *vec2;
        vec2 += n_ambi2;
      }
      beg2++;
      *vec3++ = sum * acw_vec[j];
    }
    beg1 += n_ambi;
  }
}

static void matrix_pinv_transp_back(t_matrix_pinv *x)
{
  double *vec, *transp=x->x_transp;
  double *straight=x->x_ls_encode;
  int n_ls=x->x_n_ls+x->x_n_phls;
  int n_ambi=x->x_n_ambi;
  int i, j;
  
  for(j=0; j<n_ambi; j++)
  {
    vec = transp;
    for(i=0; i<n_ls; i++)
    {
      *straight++ = *vec;
      vec += n_ambi;
    }
    transp++;
  }
}

static void matrix_pinv_inverse(t_matrix_pinv *x)
{
  int n_ambi = x->x_n_ambi;
  int n_ambi2 = 2*n_ambi;
  int i, j, nz;
  int r,c;
  double *src=x->x_inv_work1;
  double *db=x->x_inv_work2;
  double rcp, *dv;
  
  dv = db;
  for(i=0; i<n_ambi; i++) /* init */
  {
    for(j=0; j<n_ambi; j++)
    {
      *dv++ = *src++;
    }
    for(j=0; j<n_ambi; j++)
    {
      if(j == i)
        *dv++ = 1.0;
      else
        *dv++ = 0.0;
    }
  }
  
  /* make 1 in main-diagonale, and 0 below */
  for(i=0; i<n_ambi; i++)
  {
    nz = matrix_pinv_eval_which_element_of_col_not_zero(x, i, i);
    if(nz < 0)
    {
      post("matrix_pinv ERROR: matrix not regular !!!!");
      return;
    }
    else
    {
      if(nz != i)
        matrix_pinv_xch_rows(x, i, nz);
      dv = db + i*n_ambi2 + i;
      rcp = 1.0 /(*dv);
      matrix_pinv_mul_row(x, i, rcp);
      matrix_pinv_copy_row2buf(x, i);
      for(j=i+1; j<n_ambi; j++)
      {
        dv += n_ambi2;
        rcp = -(*dv);
        matrix_pinv_mul_buf_and_add2row(x, j, rcp);
      }
    }
  }
  
  /* make 0 above the main diagonale */
  for(i=n_ambi-1; i>=0; i--)
  {
    dv = db + i*n_ambi2 + i;
    matrix_pinv_copy_row2buf(x, i);
    for(j=i-1; j>=0; j--)
    {
      dv -= n_ambi2;
      rcp = -(*dv);
      matrix_pinv_mul_buf_and_add2row(x, j, rcp);
    }
  }
  
  post("matrix_inverse regular");
}

static void matrix_pinv_pinv(t_matrix_pinv *x)
{
  t_atom *at=x->x_at;
  
  matrix_pinv_transp_back(x);
  matrix_pinv_mul1(x);
  matrix_pinv_inverse(x);
  matrix_pinv_mul2(x);
  if((x->x_mirrorsum_end > x->x_mirrorsum_beg)&&
    (x->x_realsum_end > x->x_realsum_beg)&&
    ((x->x_mirrorsum_end - x->x_mirrorsum_beg) == (x->x_realsum_end - x->x_realsum_beg)))
  {
    double *mir=x->x_prod+x->x_mirrorsum_beg*x->x_n_ambi;
    double *real=x->x_prod+x->x_realsum_beg*x->x_n_ambi;
    double mwght=x->x_mir_wght;
    int i, n=(x->x_mirrorsum_end - x->x_mirrorsum_beg)*x->x_n_ambi;
    
    //    post("mirror");
    for(i=0; i<n; i++)
      real[i] += mir[i]*mwght;
    
    n = x->x_mirrorsum_beg*x->x_n_ambi;
    real=x->x_prod;
    SETFLOAT(at, (t_float)x->x_n_ambi);
    at++;
    SETFLOAT(at, (t_float)x->x_mirrorsum_beg);
    at++;
    for(i=0; i<n; i++)
    {
      SETFLOAT(at, (t_float)(*real));
      real++;
      at++;
    }
    outlet_anything(x->x_obj.ob_outlet, x->x_s_matrix, n+2, x->x_at);
  }
  else
  {
    int i, n=x->x_n_ls*x->x_n_ambi;
    double *dv=x->x_prod;
    
    //    post("real");
    SETFLOAT(at, (t_float)x->x_n_ambi);
    at++;
    SETFLOAT(at, (t_float)x->x_n_ls);
    at++;
    for(i=0; i<n; i++)
    {
      SETFLOAT(at, (t_float)(*dv));
      dv++;
      at++;
    }
    outlet_anything(x->x_obj.ob_outlet, x->x_s_matrix, n+2, x->x_at);
  }
}

static void matrix_pinv_free(t_matrix_pinv *x)
{
  freebytes(x->x_work2, 2 * x->x_dim * x->x_dim * sizeof(double));
  freebytes(x->x_buf2, 2 * x->x_dim * sizeof(double));
  freebytes(x->x_at, x->x_size * sizeof(t_atom));
}

static void *matrix_pinv_new(void)
{
  t_matrix_pinv *x = (t_matrix_pinv *)pd_new(matrix_pinv_class);
  
  x->x_dim = 10;
  x->x_size = 102;
  x->x_work2 = (double *)getbytes(2 * x->x_dim * x->x_dim * sizeof(double));
  x->x_buf2 = (double *)getbytes(2 * x->x_dim * sizeof(double));
  x->x_at = (t_atom *)getbytes(x->x_size * sizeof(t_atom));
  outlet_new(&x->x_obj, &s_list);
  return (x);
}

void matrix_pinv_setup(void)
{
  matrix_pinv_class = class_new(gensym("matrix_pinv"), (t_newmethod)matrix_pinv_new, (t_method)matrix_pinv_free,
    sizeof(t_matrix_pinv), 0, 0);
  class_addmethod(matrix_pinv_class, (t_method)matrix_pinv_matrix, gensym("matrix"), A_GIMME, 0);
//  class_sethelpsymbol(matrix_pinv_class, gensym("iemhelp/matrix_pinv-help"));
}
