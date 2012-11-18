/* Rhythm estimation in real time
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: rhythm_ioi_histogram.c,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "rhythm_ioi_histogram.h"


Rhythm_Ioi_Histogram*
rhythm_ioi_histogram_new (void)
{
    Rhythm_Ioi_Histogram* ge = NULL;

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_ioi_histogram_new\n", stderr);
#endif

    /* Allocate memory for our data structure */
    ge = calloc (1, sizeof (Rhythm_Ioi_Histogram));
    if (ge == NULL)
    {
	printf ("Not enough memory.");
	assert (0);
    }

    /* Initialize data members */
    ge->ioi_resolution = RHYTHM_ESTIMATOR_DEFAULT_IOI_RESOLUTION;
    ge->min_quantum = RHYTHM_ESTIMATOR_DEFAULT_MIN_QUANTUM;
    ge->max_quantum = RHYTHM_ESTIMATOR_DEFAULT_MAX_QUANTUM;
    ge->hist_cycles = RHYTHM_IOI_HISTOGRAM_DEFAULT_HIST_CYCLES;
    ge->hist_half_life = RHYTHM_IOI_HISTOGRAM_DEFAULT_HIST_HALF_LIFE;

    /* Initialize onset FIFO */
    ge->fifo_length = RHYTHM_IOI_HISTOGRAM_IOI_FIFO_LENGTH;
    ge->fifo_buffer = calloc (ge->fifo_length, sizeof (float));
    if (ge->fifo_buffer == NULL)
    {
	printf("Not enough memory.");
	assert (0);
    }
    
    /* Compute internal "helper" variables */
    assert (ge->ioi_resolution != 0);
    ge->hist_length = (unsigned)(ge->hist_cycles * ge->max_quantum
				 / ge->ioi_resolution);

    /* Initialize filling IOI histogram */
    ge->fill_ioi_histogram = calloc (ge->hist_length, sizeof (float));
    if (ge->fill_ioi_histogram == NULL)
    {
	printf("Not enough memory.");
	assert (0);
    }

    /* Initialize main IOI histogram */
    ge->main_ioi_histogram = calloc (ge->hist_length, sizeof (float));
    if (ge->main_ioi_histogram == NULL)
    {
	printf("Not enough memory.");
	assert (0);
    }
    
    return ge;
}

void
rhythm_ioi_histogram_delete (Rhythm_Ioi_Histogram* rhythm_ioi_histogram)
{
#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_ioi_histogram_delete\n", stderr);
#endif

    if (rhythm_ioi_histogram->main_ioi_histogram != NULL)
    {
	free (rhythm_ioi_histogram->main_ioi_histogram);
	rhythm_ioi_histogram->main_ioi_histogram = NULL;
    }

    if (rhythm_ioi_histogram->fill_ioi_histogram != NULL)
    {
	free (rhythm_ioi_histogram->fill_ioi_histogram);
	rhythm_ioi_histogram->fill_ioi_histogram = NULL;
    }

    if (rhythm_ioi_histogram->fifo_buffer != NULL)
    {
	free (rhythm_ioi_histogram->fifo_buffer);
	rhythm_ioi_histogram->fifo_buffer = NULL;
    }
    rhythm_ioi_histogram->fifo_length = 0;
    rhythm_ioi_histogram->fifo_num_onsets = 0;

    if (rhythm_ioi_histogram != NULL)
    {
	free (rhythm_ioi_histogram);
	rhythm_ioi_histogram = NULL;
    }
}

void
rhythm_ioi_histogram_initialize (Rhythm_Ioi_Histogram* ge)
{
    unsigned i = 0;

    /* precondition(s) */
    assert (ge != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_ioi_histogram_initialize\n", stderr);
#endif

    /* Clear FIFO */
    ge->fifo_num_onsets = 0;
    ge->fifo_current_index = 0;
   
    /* Clear IOI histograms */
    for (i = 0; i < ge->hist_length; i++)
    {
	ge->fill_ioi_histogram[i] = 0.0;
	ge->main_ioi_histogram[i] = 0.0;
    }

    /* Initialize histogram update time variable */
    ge->main_histogram_update_time = 0;
}

void
rhythm_ioi_histogram_finish (Rhythm_Ioi_Histogram* rhythm_ioi_histogram)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_ioi_histogram_finish\n", stderr);
#endif
}

int
rhythm_ioi_histogram_onset_event (Rhythm_Ioi_Histogram* ge, double time)
{
    unsigned i = 0;

    /* precondition(s) */
    assert (ge != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fprintf (stderr, "rhythm_ioi_histogram_onset_event: time %f ms\n", time);
#endif

    /* Compute inter-onset interval (IOI) histogram fill function */
    for (i = 0; i < ge->fifo_num_onsets; i++)
    {
	double ioi;
	unsigned ioi_index;

	/* First, compute the exact IOI */

	/* (a) When the FIFO is not full, the onset values are
	       positioned in FIFO begin from 0 to num_onsets - 1
	   (b) When the FIFO is full, it doesn't matter in
	       which order the onsets are inspected, therefore
	       we loop from 0 to num_onsets - 1 (end of FIFO) */
	ioi = time - ge->fifo_buffer[i];

	/* Then, the exact "continuous" IOI is discretized (quantized) */
	ioi_index = (unsigned)rint (ioi / ge->ioi_resolution);

	/* Next, increment the proper "fill" histogram bin by one */
	if (ioi_index < ge->hist_length)
	{
	    ge->fill_ioi_histogram[ioi_index]++;
	    /* Set a flag for the periodic_event function */
	    ge->fill_histogram_changed = 1;
	}
    }

    /* Shift onset FIFO and store new onset */
    ge->fifo_buffer[ge->fifo_current_index] = time;
    ge->fifo_current_index = (ge->fifo_current_index + 1) % ge->fifo_length;
    if (ge->fifo_num_onsets < ge->fifo_length)
    {
	ge->fifo_num_onsets++;
    }
    if(ge->fill_histogram_changed)
    {
	rhythm_ioi_histogram_periodic_event(ge, time);
	ge->fill_histogram_changed = 0;
	/* Update PD-array */
	return 1;
    }
    else
    {
	return 0;
    }
}

void
rhythm_ioi_histogram_periodic_event (Rhythm_Ioi_Histogram* ge, double time)
{
    unsigned i = 0;
    double time_passed = 0.0;

    /* precondition(s) */
    assert (ge != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fprintf (stderr, "rhythm_ioi_histogram_periodic_event: time %f ms\n", time);
#endif

    /* Get the amount of time since last histogram update */
    if (ge->main_histogram_update_time == 0)
    {
	/* Have the new IOI's get through to the histogram */
	time_passed = 1.0e10;
    }
    else
    {
	/* Compute time since last update */
	time_passed = time - ge->main_histogram_update_time;
    }

    /* Save time for next event */
    ge->main_histogram_update_time = time;

    /* Accumulate the main IOI histogram */
    rhythm_ioi_histogram_update_ioi_histogram (ge, time_passed);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    /* Dump main IOI histogram for debugging */
    printf ("HIST: ");
    for (i = 0; i < ge->hist_length; i++)
    {
	printf ("%f ", (double)(ge->main_ioi_histogram[i]));
    }
    printf ("\n.\n");
#endif
	
    /* Clear fill IOI histogram */
    for (i = 0; i < ge->hist_length; i++)
    {
	ge->fill_ioi_histogram[i] = 0;
    }
}

static void
rhythm_ioi_histogram_update_ioi_histogram (Rhythm_Ioi_Histogram* ge, double time_passed)
{
    unsigned i = 0;
    double coeff_old = 0.0;
    double coeff_new = 0.0;

    /* Compute the coefficients needed for update */
    coeff_old = pow (0.5, time_passed / ge->hist_half_life);
    /*coeff_new = (1 - coeff_old) / coeff_old;*/
    coeff_new = 1 - coeff_old;

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    printf ("time_passed=%.2f, coeff_old=%.2f, coeff_new=%.2f\n",
	    time_passed, coeff_old, coeff_new);
#endif

    /* Update main_ioi_histogram now */
    for (i = 0; i < ge->hist_length; i++)
    {
	ge->main_ioi_histogram[i] =
	    coeff_old * ge->main_ioi_histogram[i]
	    + coeff_new * ge->fill_ioi_histogram[i];
    }
}


float
rhythm_ioi_histogram_get_ioi_resolution (Rhythm_Ioi_Histogram* rhythm_ioi_histogram)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* get the parameter */
    return rhythm_ioi_histogram->ioi_resolution;
}

float
rhythm_ioi_histogram_get_min_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* get the parameter */
    return rhythm_ioi_histogram->min_quantum;
}

float
rhythm_ioi_histogram_get_max_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* get the parameter */
    return rhythm_ioi_histogram->max_quantum;
}

float
rhythm_ioi_histogram_get_hist_cycles (Rhythm_Ioi_Histogram* rhythm_ioi_histogram)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* get the parameter */
    return rhythm_ioi_histogram->hist_cycles;
}

float
rhythm_ioi_histogram_get_hist_half_life (Rhythm_Ioi_Histogram* rhythm_ioi_histogram)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* get the parameter */
    return rhythm_ioi_histogram->hist_half_life;
}

void
rhythm_ioi_histogram_set_ioi_resolution (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* sanity check(s) */
    if (v < 1)
    {
	fprintf (stderr, "ERROR rhythm_ioi_histogram: ioi_resolution must not be less than 1 ms\n");
	v = 1;
    }

    /* set the parameter */
    rhythm_ioi_histogram->ioi_resolution = v;
}

void
rhythm_ioi_histogram_set_min_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* sanity check(s) */
    if (v < 1)
    {
	fprintf (stderr, "ERROR rhythm_ioi_histogram: min_quantum must not be less than 1 ms\n");
	v = 1;
    }

    /* set the parameter */
    rhythm_ioi_histogram->min_quantum = v;
}

void
rhythm_ioi_histogram_set_max_quantum (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* sanity check(s) */
    if (v < 1)
    {
	fprintf (stderr, "ERROR rhythm_ioi_histogram: max_quantum must not be less than 1 ms\n");
	v = 1;
    }

    /* set the parameter */
    rhythm_ioi_histogram->max_quantum = v;
}

void
rhythm_ioi_histogram_set_hist_cycles (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* sanity check(s) */
    if (v <= 0)
    {
	fprintf (stderr, "ERROR rhythm_ioi_histogram: hist_cycles must be positive\n");
	v = 1;
    }

    /* set the parameter */
    rhythm_ioi_histogram->hist_cycles = v;
}

void
rhythm_ioi_histogram_set_hist_half_life (Rhythm_Ioi_Histogram* rhythm_ioi_histogram, float v)
{
    /* precondition(s) */
    assert (rhythm_ioi_histogram != NULL);

    /* sanity check(s) */
    if (v <= 0)
    {
	fprintf (stderr, "ERROR rhythm_ioi_histogram: hist_half_life must be positive\n");
	v = 1;
    }

    /* set the parameter */
    rhythm_ioi_histogram->hist_half_life = v;
}


/* EOF */
