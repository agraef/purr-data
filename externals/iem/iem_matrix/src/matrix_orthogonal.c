/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil (c) IEM KUG Graz Austria 2002 - 2006 */

#include "m_pd.h"

/* -------------------------- matrix_orthogonal ------------------------------ */

/*
versucht, eine eingehende matrix (quadratisch) zu orthogonalisiern (Transponierte = Inverse)

  und zwar wie folgt:
  
    1. zeile orthonarmalisieren (summe aller zeilen-elemente-quadrate = 1)
    K = summe aller a_1i (1 <= i <= N)
    fuer alle an_1i = a_1i / sqrt(K) (1 <= i <= N)
    
      2. zeile: 
      K = summe aller a_2i (2 <= i <= N) - an_12
      an_21 = an_12
      fuer alle an_2i = a_2i / sqrt(K) (2 <= i <= N)
      
        3. zeile: 
        K = summe aller a_3i (3 <= i <= N) - an_13 - an_23
        an_31 = an_13
        an_32 = an_23
        fuer alle an_3i = a_3i / sqrt(K) (3 <= i <= N)
        
          usw.
          
*/

typedef struct _matrix_orthogonal
{
  t_object  x_obj;
  int       x_dim;
  double    *x_work;
  t_atom    *x_at;
} t_matrix_orthogonal;

static t_class *matrix_orthogonal_class;

static void matrix_orthogonal_matrix(t_matrix_orthogonal *x, t_symbol *s, int argc, t_atom *argv)
{
  int oldsize = x->x_dim;
  int dim;
  int i, j;
  int r, c;
  double sum1, sum2, el, *v, *u;
  t_atom *at=x->x_at;
  
  if(argc > 2)
  {
    r = (int)(atom_getint(argv++));
    c = (int)(atom_getint(argv++));
    if(r == c)
    {
      dim = r;
      if(dim < 1)
        dim = 1;
      if(dim > oldsize)
      {
        if(oldsize)
        {
          x->x_work = (double *)resizebytes(x->x_work, oldsize * oldsize * sizeof(double), dim * dim * sizeof(double));
          x->x_at = (t_atom *)resizebytes(x->x_at, (oldsize * oldsize + 2) * sizeof(t_atom), (dim * dim + 2) * sizeof(t_atom));
          x->x_dim = dim;
        }
        else
        {
          x->x_work = (double *)getbytes(dim * dim * sizeof(double));
          x->x_at = (t_atom *)getbytes((dim * dim + 2) * sizeof(t_atom));
          x->x_dim = dim;
        }
      }
      v = x->x_work;
      for(i=0; i<dim; i++) /* init */
      {
        for(j=0; j<dim; j++)
        {
          *v++ = (double)(atom_getfloat(argv++));
        }
      }
      for(i=0; i<dim; i++) /* jede zeile */
      {
        sum1 = 0.0;
        v = x->x_work + i*dim + i;
        for(j=i; j<dim; j++) /* rest der zeile ab hauptdiagonale quadratisch summieren */
        {
          el = *v++;
          sum1 += el * el;
        }
        v = x->x_work + i;
        u = x->x_work + i*dim;
        sum2 = 1.0;
        for(j=0; j<i; j++)
        {
          el = *v;
          v += dim;
          *u++ = el;
          sum2 -= el * el;
        }
        if(sum1 == 0.0)
          sum1 = 1.0;
        if(sum2 <= 0.0)
          el = 0.0;
        else
          el = sqrt(sum2 / sum1);
        for(j=i; j<dim; j++)
        {
          *u *= el;
          u++;
        }
      }
      at = x->x_at;
      SETFLOAT(at, (t_float)dim);
      at++;
      SETFLOAT(at, (t_float)dim);
      at++;
      v = x->x_work;
      for(i=0; i<dim; i++)
      {
        for(j=0; j<dim; j++)
        {
          SETFLOAT(at, (t_float)(*v));
          at++;
          v++;
        }
      }
      outlet_anything(x->x_obj.ob_outlet, gensym("matrix"), dim*dim+2, x->x_at);
    }
  }
}

static void matrix_orthogonal_free(t_matrix_orthogonal *x)
{
  if(x->x_dim)
  {
    freebytes(x->x_work, x->x_dim * x->x_dim * sizeof(double));
    freebytes(x->x_at, (x->x_dim * x->x_dim + 2) * sizeof(t_atom));
  }
}

static void *matrix_orthogonal_new(void)
{
  t_matrix_orthogonal *x = (t_matrix_orthogonal *)pd_new(matrix_orthogonal_class);
  
  x->x_dim = 0;
  x->x_work = (double *)0;
  x->x_at = (t_atom *)0;
  outlet_new(&x->x_obj, &s_list);
  return (x);
}

static void matrix_orthogonal_setup(void)
{
  matrix_orthogonal_class = class_new(gensym("matrix_orthogonal"), (t_newmethod)matrix_orthogonal_new, (t_method)matrix_orthogonal_free,
    sizeof(t_matrix_orthogonal), 0, 0);
  class_addmethod(matrix_orthogonal_class, (t_method)matrix_orthogonal_matrix, gensym("matrix"), A_GIMME, 0);
//  class_sethelpsymbol(matrix_orthogonal_class, gensym("iemhelp/matrix_orthogonal-help"));
}
