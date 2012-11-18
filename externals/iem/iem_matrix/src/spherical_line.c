/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil (c) IEM KUG Graz Austria 2002 - 2006 */

#include "m_pd.h"
#include "math.h"

/* -------------------------- spherical_line ------------------------------ */

typedef struct _spherical_line
{
  t_object  x_obj;
  int       x_dim;
  double    *x_work2;
  double    *x_buf2;
  t_atom    *x_at;
} t_spherical_line;

static t_class *spherical_line_class;

static void spherical_line_rot_z(t_spherical_line *x, double *vec, double angle)
{
  int i;
  double s=sin(angle);
  double c=cos(angle);
  double sum=0.0;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
  {
    *db++ = *dw++;
  }
}

static void spherical_line_copy_row2buf(t_spherical_line *x, int row)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  double *db=x->x_buf2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
  {
    *db++ = *dw++;
  }
}

static void spherical_line_copy_buf2row(t_spherical_line *x, int row)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  double *db=x->x_buf2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
  {
    *dw++ = *db++;
  }
}

static void spherical_line_copy_row2row(t_spherical_line *x, int src_row, int dst_row)
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

static void spherical_line_xch_rows(t_spherical_line *x, int row1, int row2)
{
  spherical_line_copy_row2buf(x, row1);
  spherical_line_copy_row2row(x, row2, row1);
  spherical_line_copy_buf2row(x, row2);
}

static void spherical_line_mul_row(t_spherical_line *x, int row, double mul)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
  {
    (*dw++) *= mul;
  }
}

static void spherical_line_mul_buf_and_add2row(t_spherical_line *x, int row, double mul)
{
  int dim2 = 2*x->x_dim;
  int i;
  double *dw=x->x_work2;
  double *db=x->x_buf2;
  
  dw += row*dim2;
  for(i=0; i<dim2; i++)
  {
    *dw++ += (*db++)*mul;
  }
}

static int spherical_line_eval_which_element_of_col_not_zero(t_spherical_line *x, int col, int start_row)
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
    if( (*dw > 1.0e-10) || (*dw < -1.0e-10) )
    {
      ret = i;
      i = dim+1;
    }
    dw += dim2;
  }
  return(ret);
}

static void spherical_line_matrix(t_spherical_line *x, t_symbol *s, int argc, t_atom *argv)
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
    post("spherical_line ERROR: wrong dimension of input-list");
    return;
  }
  r = (int)(atom_getint(argv++));
  c = (int)(atom_getint(argv++));
  if(r != dim)
  {
    post("spherical_line ERROR: wrong number of rows of input-list");
    return;
  }
  if(c != dim)
  {
    post("spherical_line ERROR: wrong number of cols of input-list");
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
    nz = spherical_line_eval_which_element_of_col_not_zero(x, i, i);
    if(nz < 0)
    {
      post("spherical_line ERROR: matrix not regular");
      return;
    }
    else
    {
      if(nz != i)
      {
        spherical_line_xch_rows(x, i, nz);
      }
      dv = db + i*dim2 + i;
      rcp = 1.0 /(*dv);
      spherical_line_mul_row(x, i, rcp);
      spherical_line_copy_row2buf(x, i);
      for(j=i+1; j<dim; j++)
      {
        dv += dim2;
        rcp = -(*dv);
        spherical_line_mul_buf_and_add2row(x, j, rcp);
      }
    }
  }
  
  /* make 0 above the main diagonale */
  for(i=dim-1; i>=0; i--)
  {
    dv = db + i*dim2 + i;
    spherical_line_copy_row2buf(x, i);
    for(j=i-1; j>=0; j--)
    {
      dv -= dim2;
      rcp = -(*dv);
      spherical_line_mul_buf_and_add2row(x, j, rcp);
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

static void spherical_line_free(t_spherical_line *x)
{
  freebytes(x->x_work2, 2 * x->x_dim * x->x_dim * sizeof(double));
  freebytes(x->x_buf2, 2 * x->x_dim * sizeof(double));
  freebytes(x->x_at, (x->x_dim * x->x_dim + 2) * sizeof(t_atom));
}

static void *spherical_line_new(t_floatarg fdim)
{
  t_spherical_line *x = (t_spherical_line *)pd_new(spherical_line_class);
  int dim = (int)fdim;
  
  if(dim < 1)
    dim = 1;
  x->x_dim = dim;
  x->x_work2 = (double *)getbytes(2 * x->x_dim * x->x_dim * sizeof(double));
  x->x_buf2 = (double *)getbytes(2 * x->x_dim * sizeof(double));
  x->x_at = (t_atom *)getbytes((x->x_dim * x->x_dim + 2) * sizeof(t_atom));
  outlet_new(&x->x_obj, &s_list);
  return (x);
}

static void spherical_line_setup(void)
{
  spherical_line_class = class_new(gensym("spherical_line"), (t_newmethod)spherical_line_new, (t_method)spherical_line_free,
    sizeof(t_spherical_line), 0, A_FLOAT, 0);
  class_addmethod(spherical_line_class, (t_method)spherical_line_matrix, gensym("matrix"), A_GIMME, 0);
//  class_sethelpsymbol(spherical_line_class, gensym("iemhelp/spherical_line-help"));
}
