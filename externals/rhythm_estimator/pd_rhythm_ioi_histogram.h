/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_ioi_histogram.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef __PD_RHYTHM_IOI_HISTOGRAM_H__
#define __PD_RHYTHM_IOI_HISTOGRAM_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MAXPDSTRING /* how primitive */
#include "m_pd.h"
#endif /* MAXPDSTRING */

#include "rhythm_ioi_histogram.h"

    /* Parameter strings for setting using set-method */
#define RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_STR "half_life"
#define RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_STR "cycles"

typedef struct
{
    t_object	x_obj;

    Rhythm_Ioi_Histogram*	rhythm_ioi_histogram;

    /* Array-related variables */
    t_symbol*	array_symbol;
    unsigned array_nsampsintab;
    float*	array_vec;

} pd_t_rhythm_ioi_histogram;

/* Object construction and destruction */
void		rhythm_ioi_histogram_setup (void);
static void*	pd_rhythm_ioi_histogram_new (t_symbol* s);
static void	pd_rhythm_ioi_histogram_free (pd_t_rhythm_ioi_histogram* x);

/* "Bang" inlet message callback */
static void	pd_rhythm_ioi_histogram_bang (pd_t_rhythm_ioi_histogram* x);
/* Float inlet message callback */
static void	pd_rhythm_ioi_histogram_float (pd_t_rhythm_ioi_histogram* x, t_floatarg f);
/* Set-Message interpreter */
static void	pd_rhythm_ioi_histogram_set (pd_t_rhythm_ioi_histogram* x,
					     t_symbol* s,
					     t_float value);
    /* print method */
static void	pd_rhythm_ioi_histogram_print (pd_t_rhythm_ioi_histogram* x);

    /* helper functions */
    /* access to time */
static	double pd_rhythm_ioi_histogram_get_time (void);

    /* access to an array for displaying the histogram */
static void	pd_rhythm_ioi_histogram_array_initialize (pd_t_rhythm_ioi_histogram* x,
						   t_symbol* s);
static void	pd_rhythm_ioi_histogram_array_write (pd_t_rhythm_ioi_histogram* x,
					      float* vector,
					      unsigned length);
static void	pd_rhythm_ioi_histogram_array_finish (pd_t_rhythm_ioi_histogram* x);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PD_RHYTHM_IOI_HISTOGRAM_H__ */
/* EOF */

