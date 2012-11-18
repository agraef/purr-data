/* Rhythm estimation in real time
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: rhythm_quantum.c,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#include "rhythm_quantum.h"


Rhythm_Quantum*
rhythm_quantum_new (void)
{
    Rhythm_Quantum* ge = NULL;

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_quantum_new\n", stderr);
#endif

    /* Allocate memory for our data structure */
    ge = calloc (1, sizeof (Rhythm_Quantum));
    if (ge == NULL)
    {
	printf ("Not enough memory.");
	assert (0);
    }

    /* Initialize data members */
    ge->gcd_perc = RHYTHM_QUANTUM_DEFAULT_GCD_PERC;
    ge->max_quantum = RHYTHM_ESTIMATOR_DEFAULT_MAX_QUANTUM;
    ge->min_quantum = RHYTHM_ESTIMATOR_DEFAULT_MIN_QUANTUM;
    ge->ioi_resolution = RHYTHM_ESTIMATOR_DEFAULT_IOI_RESOLUTION;

    /* Compute internal "helper" variables */
    assert (ge->ioi_resolution != 0);

    /* Initialize GCD error fn */
    ge->gcd_re_length = (unsigned)ceil (ge->max_quantum / ge->ioi_resolution);
#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    printf ("GCD-RE length: %d\n", (int)(ge->gcd_re_length));
#endif
    ge->gcd_remainder_error = calloc (ge->gcd_re_length, sizeof (float));
    if (ge->gcd_remainder_error == NULL)
    {
	printf("Not enough memory.");
	assert (0);
    }
    
    return ge;
}

void
rhythm_quantum_delete (Rhythm_Quantum* rhythm_quantum)
{
#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_quantum_delete\n", stderr);
#endif

    if (rhythm_quantum->gcd_remainder_error != NULL)
    {
	free (rhythm_quantum->gcd_remainder_error);
	rhythm_quantum->gcd_remainder_error = NULL;
    }

    if (rhythm_quantum != NULL)
    {
	free (rhythm_quantum);
	rhythm_quantum = NULL;
    }
}

void
rhythm_quantum_initialize (Rhythm_Quantum* ge)
{
    /* precondition(s) */
    assert (ge != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_quantum_initialize\n", stderr);
#endif
}

void
rhythm_quantum_finish (Rhythm_Quantum* rhythm_quantum)
{
    /* precondition(s) */
    assert (rhythm_quantum != NULL);

#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    fputs ("rhythm_quantum_finish\n", stderr);
#endif
}


float rhythm_quantum_compute_quantum(Rhythm_Quantum* ge, float *vector, unsigned len)
{
    rhythm_quantum_compute_gcd_re (ge, vector, len);
    return rhythm_quantum_determine_quantum (ge);
}

static float
rhythm_quantum_determine_quantum (Rhythm_Quantum* ge)
{
    unsigned i;
    float median=0;
    float min=0;
    float threshold;
    unsigned first_index;
    unsigned min_index;

    first_index = (unsigned)rint (ge->min_quantum/ge->ioi_resolution);
    /* Compute minimum value of error function (within min_quantum and */
    /* max_quantum ) */
    min = 1.0e10;
    for(i = first_index; i < ge->gcd_re_length; i++)
    {
	if (ge->gcd_remainder_error[i] < min)
	{
	    min = ge->gcd_remainder_error[i];
	}
    }

    /* Compute median value of error function */
    if(ge->gcd_re_length-first_index == 0)
     assert (0);
    median = compute_median (&ge->gcd_remainder_error[first_index],
			     ge->gcd_re_length-first_index);
#if RHYTHM_ESTIMATOR_DEBUG_TEXT
    printf ("DEBUG min=%f, median=%f\n", (double)min, (double)median);
#endif

    /* Compute error function threshold */
    threshold = ge->gcd_perc * min + (1-ge->gcd_perc) * median;
    /* Find first sample under threshold, begining from the end of the error-function  */
    i = ge->gcd_re_length-1;
    while (ge->gcd_remainder_error[i--] >= threshold)
        ;
    /* i: pointer to the first sample under the threshold */
    i++;
    min = threshold;
    /* Now let's find minimum of error-function within the area under threshold */
    min_index = 0;
    while (ge->gcd_remainder_error[i] <= threshold)
    {
	if(ge->gcd_remainder_error[i] < min)
	{
	    min = ge->gcd_remainder_error[i];
	    min_index = i;
	}
	if (i <= first_index)
	    break;
	i--;
    }
    if (i < first_index || min_index == 0)
    {
	printf ("warning: quantum not found\n");
	return 0;
    }

    return (float)(min_index) * ge->ioi_resolution;
}


static void
rhythm_quantum_compute_gcd_re (Rhythm_Quantum* ge, float *histogram, unsigned hist_len)
{
    unsigned j = 0;
    unsigned k = 0;
    double denominator;

    denominator = 0.0;
    for (k = 0; k < hist_len; k++)
    {
	denominator += histogram[k];
    }

    if (denominator < 1.0e-4)
    {
	printf ("warning: histogram mass went small\n");
	denominator = 1.0e-4;
    }

    /* Quantum cannot be zero <=> 0th remainder error tap isn't used */
    assert (ge->gcd_re_length > 0);
    ge->gcd_remainder_error[0] = 0;
    for (j = 1; j < ge->gcd_re_length; j++)
    {
	double q;
	double nominator;

	q = j * ge->ioi_resolution;
	nominator = 0.0;
	for (k = 0; k < hist_len; k++)
	{
	    double mr; /* modified residual */
	    double term;
	    mr = fmod (k * ge->ioi_resolution / q + 0.5, 1) - 0.5;

	    mr = (fmod (k * ge->ioi_resolution + q / 2, q) - q / 2) / q;


	    term = histogram[k] * mr * mr; /* mr^2 */
	    nominator += term;
	}

	/* Denominator shouldn't theoretically be zero... */
	ge->gcd_remainder_error[j] = (float)(nominator / denominator);
    }
}

/* Find median with Torben's method: see
 * http://www.eso.org/~ndevilla/median/
 * FIXME this is actually the very slowest method presented on the page...
 */
static float
compute_median (float* vector, unsigned length)
{
    unsigned i, less, greater, equal;
    float  min, max, guess, maxltguess, mingtguess;

    min = max = vector[0] ;
    for (i=1 ; i<length ; i++) {
        if (vector[i]<min) min=vector[i];
        if (vector[i]>max) max=vector[i];
    }

    while (1) {
        guess = (min+max)/2;
        less = 0; greater = 0; equal = 0;
        maxltguess = min ;
        mingtguess = max ;
        for (i=0; i<length; i++) {
            if (vector[i]<guess) {
                less++;
                if (vector[i]>maxltguess) maxltguess = vector[i] ;
            } else if (vector[i]>guess) {
                greater++;
                if (vector[i]<mingtguess) mingtguess = vector[i] ;
            } else equal++;
        }
        if (less <= (length+1)/2 && greater <= (length+1)/2) break ; 
        else if (less>greater) max = maxltguess ;
        else min = mingtguess;
    }
    if (less >= (length+1)/2) return maxltguess;
    else if (less+equal >= (length+1)/2) return guess;
    else return mingtguess;
}


/* EOF */
