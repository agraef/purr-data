/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_quantum.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

   This file is part of rhythm_estimator.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

#ifndef __PD_RHYTHM_QUANTUM_H__
#define __PD_RHYTHM_QUANTUM_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MAXPDSTRING /* how primitive */
#include "m_pd.h"
#endif /* MAXPDSTRING */

#include "rhythm_quantum.h"

    /* Parameter string for setting using set-method */
#define RHYTHM_QUANTUM_GCD_PERC_STR "gcd_perc"


typedef struct
{
    t_object	x_obj;

    Rhythm_Quantum*	rhythm_quantum;

    /* Array-related variables */
    t_symbol*	array_symbol;
    int		array_nsampsintab;
    float*	array_vec;

} pd_t_rhythm_quantum;

/* Object construction and destruction */
void		rhythm_quantum_setup (void);
static void*	pd_rhythm_quantum_new (t_symbol* s);
static void	pd_rhythm_quantum_free (pd_t_rhythm_quantum* x);

/* "Bang" inlet message callback */
static void	pd_rhythm_quantum_bang (pd_t_rhythm_quantum* x);
static void	pd_rhythm_quantum_set (pd_t_rhythm_quantum* x,
				       t_symbol* s,
				       t_float value);
static void	pd_rhythm_quantum_print (pd_t_rhythm_quantum* x);

    /* helper functions */
    /* access to an array for reading the histogram */
static void	pd_rhythm_quantum_array_initialize (pd_t_rhythm_quantum* x,
						   t_symbol* s);
static void	pd_rhythm_quantum_array_read (pd_t_rhythm_quantum* x,
					      float**  vector,
					      unsigned* length);
static void	pd_rhythm_quantum_array_finish (pd_t_rhythm_quantum* x);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PD_RHYTHM_QUANTUM_H__ */
/* EOF */

