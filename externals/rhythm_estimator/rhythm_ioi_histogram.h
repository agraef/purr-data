/* Rhythm estimation in real time
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: rhythm_ioi_histogram.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef __RHYTHM_IOI_HISTOGRAM_H__
#define __RHYTHM_IOI_HISTOGRAM_H__

/* Global header for Rhythm Estimator */
#include "rhythm_estimator.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Local Parameter default values */
#define RHYTHM_IOI_HISTOGRAM_DEFAULT_HIST_HALF_LIFE	1300 /* ms */
#define RHYTHM_IOI_HISTOGRAM_DEFAULT_HIST_CYCLES        4

#define RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_MIN 10 /*ms*/
#define RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_MIN    1
			    
#define RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_MAX 10000 /* ms*/
#define RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_MAX    16




#define RHYTHM_IOI_HISTOGRAM_IOI_FIFO_LENGTH		500 /* onsets */

typedef struct
{
    /* User-changeable parameters: */
    float	ioi_resolution; /* ms */
    float	min_quantum; /* ms */
    float	max_quantum; /* ms */
    float	hist_cycles;
    float	hist_half_life; /* ms */


    /* --- Private data --- */

    /* Internal "helper" variables */
    unsigned	hist_length;
    float	hist_new_coef; /* Coefficient for histogram update */

    /* Onsets are stored in the following FIFO */
    float*	fifo_buffer; /* FIFO address */
    unsigned	fifo_num_onsets; /* number of onsets contained in FIFO */
    unsigned	fifo_length; /* FIFO length, i.e. maximum num_onsets */
    unsigned	fifo_current_index; /* Ringbuffer read/write index;
				       points to the oldest value */

    /* IOI histograms are stored in the following buffers */
    float*	fill_ioi_histogram; /* Buffer length is given with hist_length */
    float*	main_ioi_histogram; /* Buffer length is given with hist_length */
    int		fill_histogram_changed;
    double	main_histogram_update_time;

} Rhythm_Ioi_Histogram;


Rhythm_Ioi_Histogram*	rhythm_ioi_histogram_new (void);
void	rhythm_ioi_histogram_delete (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);

void	rhythm_ioi_histogram_initialize (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);
void	rhythm_ioi_histogram_finish (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);

int	rhythm_ioi_histogram_onset_event (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, double time);
void	rhythm_ioi_histogram_periodic_event (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, double time);

float	rhythm_ioi_histogram_get_ioi_resolution (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);
float	rhythm_ioi_histogram_get_min_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);
float	rhythm_ioi_histogram_get_max_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);
float	rhythm_ioi_histogram_get_hist_cycles (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);
float	rhythm_ioi_histogram_get_hist_half_life (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);
float	rhythm_ioi_histogram_get_phase_adap_speed (Rhythm_Ioi_Histogram* rhythm_ioi_histogram);
void	rhythm_ioi_histogram_set_ioi_resolution (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v);
void	rhythm_ioi_histogram_set_min_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v);
void	rhythm_ioi_histogram_set_max_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v);
void	rhythm_ioi_histogram_set_hist_cycles (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v);
void	rhythm_ioi_histogram_set_hist_half_life (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v);
void	rhythm_ioi_histogram_set_phase_adap_speed (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v);

    /* Subroutines */
static void	rhythm_ioi_histogram_update_ioi_histogram (Rhythm_Ioi_Histogram* ge, double time_passed);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RHYTHM_IOI_HISTOGRAM_H__ */
/* EOF */
