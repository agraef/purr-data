/* Rhythm estimation in real time
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: rhythm_quantum.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef __RHYTHM_QUANTUM_H__
#define __RHYTHM_QUANTUM_H__

#include "rhythm_estimator.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Parameter default values */
#define RHYTHM_QUANTUM_DEFAULT_GCD_PERC		0.4 /* internal */
#define RHYTHM_QUANTUM_GCD_PERC_MIN 0
#define RHYTHM_QUANTUM_GCD_PERC_MAX 1

typedef struct
{
    /* --- Public data --- */

    /* User-changeable parameters: */

    /* --- Private data --- */

    /* Internal parameters: */
    float		gcd_perc;
    float   	max_quantum;
    float   	min_quantum;
    float   	ioi_resolution;

    /* Internal "helper" variables */
	float*		gcd_remainder_error; /* Length is below */
	unsigned	gcd_re_length; /* ceil(max_quantum/ioi_resolution) */

} Rhythm_Quantum;


Rhythm_Quantum*	rhythm_quantum_new (void);
void	rhythm_quantum_delete (Rhythm_Quantum* rhythm_quantum);

void	rhythm_quantum_initialize (Rhythm_Quantum* rhythm_quantum);
void	rhythm_quantum_finish (Rhythm_Quantum* rhythm_quantum);


float   rhythm_quantum_compute_quantum (Rhythm_Quantum* rhythm_quantum, float *vector, unsigned len);

    /* Subroutines */
static float	rhythm_quantum_determine_quantum (Rhythm_Quantum* ge);
static void	rhythm_quantum_compute_gcd_re (Rhythm_Quantum* ge, float *histogram, unsigned hist_len);

static float	compute_median (float* vector, unsigned length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RHYTHM_QUANTUM_H__ */
/* EOF */
