/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

iem_matrix written by Thomas Musil, Copyright (c) IEM KUG Graz Austria 2000 - 2007 */

#include "m_pd.h"
#include "iemlib.h"

static t_class *iem_matrix_class;

static void *iem_matrix_new(void)
{
  t_object *x = (t_object *)pd_new(iem_matrix_class);
  
  return (x);
}

void matrix_mul_line_tilde_setup(void);
void matrix_mul_line8_tilde_setup(void);
void matrix_mul_stat_tilde_setup(void);
void matrix_diag_mul_line_tilde_setup(void);
void matrix_diag_mul_line8_tilde_setup(void);
void matrix_diag_mul_stat_tilde_setup(void);
void matrix_bundle_line_tilde_setup(void);
void matrix_bundle_line8_tilde_setup(void);
void matrix_bundle_stat_tilde_setup(void);

/* ------------------------ setup routine ------------------------- */

void iem_matrix_setup(void)
{
  matrix_mul_line_tilde_setup();
  matrix_mul_line8_tilde_setup();
  matrix_mul_stat_tilde_setup();
  matrix_diag_mul_line_tilde_setup();
  matrix_diag_mul_line8_tilde_setup();
  matrix_diag_mul_stat_tilde_setup();
  matrix_bundle_line_tilde_setup();
  matrix_bundle_line8_tilde_setup();
  matrix_bundle_stat_tilde_setup();
  
  post("iem_matrix (R-1.17) library loaded!   (c) Thomas Musil 06.2007");
  post("   musil%ciem.at iem KUG Graz Austria", '@');
}
