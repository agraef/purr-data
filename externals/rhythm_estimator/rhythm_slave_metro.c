/* Rhythm estimation in real time
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: rhythm_slave_metro.c,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#include "rhythm_estimator.h"
#include "rhythm_slave_metro.h"


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

Rhythm_Slave_Metro*
rhythm_slave_metro_new (void)
{
    Rhythm_Slave_Metro* ge = NULL;

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_slave_metro_new\n", stderr);
#endif

    /* Allocate memory for our data structure */
    ge = calloc (1, sizeof (Rhythm_Slave_Metro));
    if (ge == NULL)
    {
	printf ("Not enough memory.");
	assert (0);
    }

    /* Initialize data members */
    ge->phase_adap_speed = RHYTHM_SLAVE_METRO_DEFAULT_PHASE_ADAP_SPEED;
    ge->ioi_resolution = RHYTHM_ESTIMATOR_DEFAULT_IOI_RESOLUTION;

    return ge;
}

void
rhythm_slave_metro_delete (Rhythm_Slave_Metro* rhythm_slave_metro)
{
#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_slave_metro_delete\n", stderr);
#endif

    if (rhythm_slave_metro != NULL)
    {
	free (rhythm_slave_metro);
	rhythm_slave_metro = NULL;
    }
}

void
rhythm_slave_metro_initialize (Rhythm_Slave_Metro* ge)
{
    unsigned i = 0;

    /* precondition(s) */
    assert (ge != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_slave_metro_initialize\n", stderr);
#endif

    /* Initialize */
    ge->last_bang_time = 0;
    ge->period = 500;
    ge->dev_mean_sum_re = 0;
    ge->dev_mean_sum_im = 0;
    ge->dev_mean_count = 0;
    ge->dev_syst = 0;
    ge->dev_syst_limit = (double)ge->ioi_resolution / 2;
}

void
rhythm_slave_metro_finish (Rhythm_Slave_Metro* rhythm_slave_metro)
{
    /* precondition(s) */
    assert (rhythm_slave_metro != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_slave_metro_finish\n", stderr);
#endif
}

void
rhythm_slave_metro_set_period (Rhythm_Slave_Metro* ge,
			       double new_period)
{
    /* precondition(s) */
    assert (ge != NULL);

    if (new_period < 1)
    {
	new_period = 1;
    }

    if (new_period != ge->period)
    {
	/* systematic deviation isn't true anymore */
	ge->dev_syst = 0;

#if RHYTHM_ESTIMATOR_DEBUG_TEXT_SP
	printf ("DEBUG: dev_syst cleared\n");
#endif
    }

    ge->period = new_period;
}

double
rhythm_slave_metro_get_period (Rhythm_Slave_Metro* ge)
{
    /* precondition(s) */
    assert (ge != NULL);

    return ge->period;
}

double
rhythm_slave_metro_onset_event (Rhythm_Slave_Metro* ge, double time)
{
    double dev;
    double dev_arg;

    /* precondition(s) */
    assert (ge != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fprintf (stderr, "rhythm_slave_metro_onset_event: time %f ms\n", time);
#endif

    dev = time - ge->last_bang_time;

    /* Circular mean: accumulate exp (j * dev * 2 * pi / period) */
    dev_arg = 2 * M_PI * dev / (ge->period + ge->dev_syst);
    ge->dev_mean_sum_re += cos (dev_arg);
    ge->dev_mean_sum_im += sin (dev_arg);
    ge->dev_mean_count++;

#if RHYTHM_ESTIMATOR_DEBUG_TEXT_SP
    printf ("onset: dev=%+04.2f ms\n", dev);
#endif

    return 0;
}

double
rhythm_slave_metro_periodic_event (Rhythm_Slave_Metro* ge, double time)
{
    double new_delay = 0;
    double dev_mean = 0;
    double dev_rand = 0;
    double beta = 0;
    double dev_mean_goodness = 0;

    /* precondition(s) */
    assert (ge != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fprintf (stderr, "rhythm_slave_metro_periodic_event: time %f ms\n", time);
#endif

    if (ge->dev_mean_count)
    {
	double dev_mean_re = 0;
	double dev_mean_im = 0;
#if 0
	double elapsed_since_bang = 0;
#endif

	/* compute circular mean of deviations */
	dev_mean_re = ge->dev_mean_sum_re / ge->dev_mean_count;
	dev_mean_im = ge->dev_mean_sum_im / ge->dev_mean_count;
	/* angle */
	dev_mean = (atan2 (dev_mean_im, dev_mean_re)
		    * (ge->period + ge->dev_syst) / (2 * M_PI));
	/* magnitude */
	dev_mean_goodness = sqrt (dev_mean_re * dev_mean_re
				     + dev_mean_im * dev_mean_im);

	/* mean wrapped implicitly with atan2() */
#if 0
	elapsed_since_bang = time - ge->last_bang_time;
	if (dev_mean > elapsed_since_bang / 2)
	{
	    dev_mean -= elapsed_since_bang;
	}
#endif

#if RHYTHM_ESTIMATOR_DEBUG_TEXT_SP
	printf (" dev=%+04.2f ms (%.0f; %2.0f%%)",
		dev_mean, ge->dev_mean_count, 100 * dev_mean_goodness);
#endif

	/* start accumulating a fresh new mean of deviations */
	ge->dev_mean_sum_re = 0;
	ge->dev_mean_sum_im = 0;
	ge->dev_mean_count = 0;
    }
    else
    {
	dev_mean = 0;
	dev_mean_goodness = 0;

#if RHYTHM_ESTIMATOR_DEBUG_TEXT_SP
	printf (" dev=%+04.2f ms (%.0f)",
		(double)0, (double)0);
#endif
    }

    /* systematic deviation adaptation speed coefficient */
    /*beta = (double)ge->phase_adap_speed * (double)ge->phase_adap_speed;*/
    beta = 0.3 * (double)ge->phase_adap_speed * dev_mean_goodness;

    /* compute systematic and random deviations */
    ge->dev_syst = (beta * MAX (-ge->dev_syst_limit,
				MIN (ge->dev_syst_limit, dev_mean))
		    + (1 - beta) * ge->dev_syst);
    dev_rand = dev_mean - ge->dev_syst;

    /* compute delay until next bang, i.e. new (temporary) period */
    new_delay = (ge->period
		 + ge->dev_syst
		 + dev_mean_goodness * ge->phase_adap_speed * dev_rand);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT_SP
    printf (", rdev=%+04.2f ms, sdev=%+04.2f ms, delta=%+04.1f ms, quant=%+04.1f ms\n",
	    dev_rand, ge->dev_syst, ge->dev_syst + dev_rand * ge->phase_adap_speed, new_delay);
#endif

    /* store this bang time */
    ge->last_bang_time = time;

    return new_delay;
}

float
rhythm_slave_metro_get_phase_adap_speed (Rhythm_Slave_Metro* rhythm_slave_metro)
{
    /* precondition(s) */
    assert (rhythm_slave_metro != NULL);

    /* get the parameter */
    return rhythm_slave_metro->phase_adap_speed;
}

void
rhythm_slave_metro_set_phase_adap_speed (Rhythm_Slave_Metro* rhythm_slave_metro, float v)
{
    /* precondition(s) */
    assert (rhythm_slave_metro != NULL);

    /* sanity check(s) */
    if (v <= 0)
    {
	fprintf (stderr, "ERROR rhythm_slave_metro: phase_adap_speed must be positive\n");
	v = 1;
    }

    /* set the parameter */
    rhythm_slave_metro->phase_adap_speed = v;
}

/* EOF */
