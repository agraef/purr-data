/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_ioi_histogram.c,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m_pd.h"

#include "pd_rhythm_estimator.h"
#include "pd_rhythm_ioi_histogram.h"
#include "rhythm_ioi_histogram.h"


static t_class* pd_rhythm_ioi_histogram_class = NULL;

/* We need a reference systime value */
static double pd_rhythm_ioi_histogram_setup_systime = 0;

void
rhythm_ioi_histogram_setup (void)
{
    pd_rhythm_ioi_histogram_class = class_new (gensym ("rhythm_ioi_histogram"),
					(t_newmethod)pd_rhythm_ioi_histogram_new,
					(t_method)pd_rhythm_ioi_histogram_free,
					sizeof (pd_t_rhythm_ioi_histogram),
					0,
					A_DEFSYM, 0);
    assert (pd_rhythm_ioi_histogram_class != NULL);

    /* Initialize setup time variable */
    pd_rhythm_ioi_histogram_setup_systime = clock_getsystime ();

    /* Set the callback for bang messages */
    class_addbang (pd_rhythm_ioi_histogram_class, pd_rhythm_ioi_histogram_bang);
    /* Set the callback for float messages */
    class_addfloat (pd_rhythm_ioi_histogram_class,
		    pd_rhythm_ioi_histogram_float);
    /* SET-Message */
    class_addmethod (pd_rhythm_ioi_histogram_class,
		     (t_method)pd_rhythm_ioi_histogram_set,
		     gensym("set"),
		     A_SYMBOL, A_FLOAT, 0);
    /* PRINT-Message */
    class_addmethod (pd_rhythm_ioi_histogram_class,
		     (t_method)pd_rhythm_ioi_histogram_print,
		     gensym("print"),
		     0);
}

static void*
pd_rhythm_ioi_histogram_new (t_symbol* s)
{
    pd_t_rhythm_ioi_histogram* x = NULL;

    /* Allocate object struct */
    x = (pd_t_rhythm_ioi_histogram*)pd_new (pd_rhythm_ioi_histogram_class);
    assert (x != NULL);

    /* Construct rhythm_ioi_histogram */
    x->rhythm_ioi_histogram = rhythm_ioi_histogram_new ();
    assert (x->rhythm_ioi_histogram != NULL);
    rhythm_ioi_histogram_initialize (x->rhythm_ioi_histogram);

    /* Initialize debug array */
    pd_rhythm_ioi_histogram_array_initialize (x, s);

    /* Inlet(s) already created? */

    /* Create outlet(s) */

    /* Bang message outlet */
    outlet_new (&x->x_obj, gensym ("bang"));

    return x;
}

static void
pd_rhythm_ioi_histogram_free (pd_t_rhythm_ioi_histogram* x)
{
    /* precondition(s) */
    assert (x != NULL);

    pd_rhythm_ioi_histogram_array_finish (x);

    /* Destruct rhythm_ioi_histogram */
    rhythm_ioi_histogram_finish (x->rhythm_ioi_histogram);
    rhythm_ioi_histogram_delete (x->rhythm_ioi_histogram);
    x->rhythm_ioi_histogram = NULL;
}

static void
pd_rhythm_ioi_histogram_bang (pd_t_rhythm_ioi_histogram* x)
{
    /* precondition(s) */
    assert (x != NULL);

    /* a "bang" message is equal to a float "0" message */
    pd_rhythm_ioi_histogram_float (x, 0);
}

static void
pd_rhythm_ioi_histogram_float (pd_t_rhythm_ioi_histogram* x, t_floatarg f)
{
#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
    double time; /* for debugging */
#endif

    /* precondition(s) */
    assert (x != NULL);

    /* Invoke rhythm_ioi_histogram onset event */
#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
    time = sys_getrealtime ();
#endif
    if (rhythm_ioi_histogram_onset_event (x->rhythm_ioi_histogram,
					  pd_rhythm_ioi_histogram_get_time ())
	!= 0)
    {
      /* Update our histogram array */
#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
      printf("ioi_histo: B'4 write time: %f\n",  (sys_getrealtime()- time)*1000.);
#endif
	pd_rhythm_ioi_histogram_array_write (x,
					     x->rhythm_ioi_histogram->main_ioi_histogram,
					     x->rhythm_ioi_histogram->hist_length);	
	/* Send a "bang" message to the output */
#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
	printf("ioi_histo: after write time: %f\n",  (sys_getrealtime()- time)*1000.);
#endif
	outlet_bang (x->x_obj.ob_outlet);
    }
#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
    printf("ioi_histo: Used time: %f\n",  (sys_getrealtime()- time)*1000.);
#endif
}

static void
pd_rhythm_ioi_histogram_set (pd_t_rhythm_ioi_histogram* x,
			     t_symbol* s,
			     t_float value)
{

    /*  Format: 'parameter value' */

    if(strlen(s->s_name))
    {	
			/* Check for IOI_RESOLUTION */
	    if (!strncmp( s->s_name, RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR
		      , strlen(RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR)))
	    {   
		x->rhythm_ioi_histogram->ioi_resolution =
		    ParameterCheck(RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR, 
				   value, 
				   RHYTHM_ESTIMATOR_IOI_RESOLUTION_MIN,
				   RHYTHM_ESTIMATOR_IOI_RESOLUTION_MAX);
		return;
	    }
	    else	    /* Check for MIN_QUANTUM */
	    if (!strncmp(s->s_name, RHYTHM_ESTIMATOR_MIN_QUANTUM_STR
		, strlen(RHYTHM_ESTIMATOR_MIN_QUANTUM_STR)))
	    {   
		x->rhythm_ioi_histogram->min_quantum =
		    ParameterCheck(RHYTHM_ESTIMATOR_MIN_QUANTUM_STR, 
				   value, 
				   RHYTHM_ESTIMATOR_MIN_QUANTUM_MIN,
				   RHYTHM_ESTIMATOR_MIN_QUANTUM_MAX);
		return;
	    }
	    else	    /* Check for MAX_QUANTUM */
	    if (!strncmp(s->s_name, RHYTHM_ESTIMATOR_MAX_QUANTUM_STR
		, strlen(RHYTHM_ESTIMATOR_MAX_QUANTUM_STR)))
	    {	
		x->rhythm_ioi_histogram->max_quantum =
		    ParameterCheck(RHYTHM_ESTIMATOR_MAX_QUANTUM_STR, 
				   value, 
				   RHYTHM_ESTIMATOR_MAX_QUANTUM_MIN,
				   RHYTHM_ESTIMATOR_MAX_QUANTUM_MAX);
		return;
	    }
	    else	    /* Check for HIST_HALF_LIFE */
	    if (!strncmp(s->s_name,RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_STR
		, strlen(RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_STR)))
	    {	
		x->rhythm_ioi_histogram->hist_half_life =
		    ParameterCheck(RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_STR, 
				   value, 
				   RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_MIN,
				   RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_MAX);
		return;
	    }
	    else	    /* Check for _HIST_CYCLES */
	    if (!strncmp(s->s_name, RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_STR
		, strlen(RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_STR)))
	    {	
		x->rhythm_ioi_histogram->hist_cycles =
		    ParameterCheck(RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_STR, 
				   value, 
				   RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_MIN,
				   RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_MAX);
		return;
	    }
    }
    printf("Error, use format like: 'parameter value'\n");
    printf("Valid 'parameter' for rhythm_ioi_resolution:\n    ");
    printf(RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR);
    printf("\n    ");
    printf(RHYTHM_ESTIMATOR_MIN_QUANTUM_STR);
    printf("\n    ");
    printf(RHYTHM_ESTIMATOR_MAX_QUANTUM_STR);
    printf("\n    ");
    printf(RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_STR);
    printf("\n    ");
    printf(RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_STR);
    printf("\n\n");
}

static void
pd_rhythm_ioi_histogram_print (pd_t_rhythm_ioi_histogram* x)
{
    
    printf("HISTOGRAM:\n    %s: %0.1f",
	   RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR, x->rhythm_ioi_histogram->ioi_resolution);
    printf("\n    ");
    printf("%s: %0.1f",RHYTHM_ESTIMATOR_MIN_QUANTUM_STR, x->rhythm_ioi_histogram->min_quantum);
    printf("\n    ");
    printf("%s: %0.1f",RHYTHM_ESTIMATOR_MAX_QUANTUM_STR, x->rhythm_ioi_histogram->max_quantum);
    printf("\n    ");
    printf("%s: %0.1f",RHYTHM_IOI_HISTOGRAM_HIST_HALF_LIFE_STR, 
	   x->rhythm_ioi_histogram->hist_half_life);
    printf("\n    ");
    printf("%s: %0.1f",RHYTHM_IOI_HISTOGRAM_HIST_CYCLES_STR, 
	   x->rhythm_ioi_histogram->hist_cycles);
    printf("\n\n");					 
}

static double
pd_rhythm_ioi_histogram_get_time (void)
{
    return clock_gettimesince (pd_rhythm_ioi_histogram_setup_systime);
}

static void
pd_rhythm_ioi_histogram_array_initialize (pd_t_rhythm_ioi_histogram* x,
				   t_symbol* s)
{
    t_garray *a;
    x->array_symbol = s;

    /* Find the array by name */

    a = (t_garray*)pd_findbyclass (x->array_symbol, garray_class);
    if (a == 0)
    {
    	if (s->s_name != NULL)
	{
	    pd_error(x, "rhythm_ioi_histogram: %s: no such array",
		     x->array_symbol->s_name);
	}
    	x->array_vec = 0;
    }

    /* Initialize array buffer length and pointer */

    else if (!garray_getfloatarray (a, &x->array_nsampsintab, &x->array_vec))
    {
    	error("rhythm_ioi_histogram: %s: bad array", x->array_symbol->s_name);
    	x->array_vec = 0;
    }
    else garray_usedindsp (a);

    if (x->array_vec == NULL)
    {
	printf ("rhythm_ioi_histogram: array %s not given\n", x->array_symbol->s_name);
    }
}

void
pd_rhythm_ioi_histogram_array_write (pd_t_rhythm_ioi_histogram* x, float* vector,
			      unsigned length)
{
    t_garray *a;

    /* Find the array by name and test if we still have an array */

    a = (t_garray*)pd_findbyclass (x->array_symbol, garray_class);
    if (a == 0)
    {
    	if (x->array_symbol->s_name != NULL)
	{
	    pd_error(x, "rhythm_ioi_histogram: %s: no such array",
		     x->array_symbol->s_name);
	}
    	x->array_vec = 0;
    }

    /* Initialize array buffer length and pointer */

    else if (!garray_getfloatarray (a, &x->array_nsampsintab, &x->array_vec))
    {
    	error("rhythm_ioi_histogram: %s: bad array", x->array_symbol->s_name);
    	x->array_vec = 0;
    }
    else
    {
	unsigned i = 0;

	garray_usedindsp (a);

	/* Copy histogram content to array */

	if (length > x->array_nsampsintab)
	{
	    printf ("warning: array (%d) is shorter than histogram (%d)\n",
		    x->array_nsampsintab, length);
	    length = x->array_nsampsintab;
	}
	for (i = 0; i < length; i++)
	{
	    x->array_vec[i] = vector[i];
	}

	/* Redraw array on screen, if needed -- FIXME don't */
	/*garray_redraw (a);*/
    }
}

static void
pd_rhythm_ioi_histogram_array_finish (pd_t_rhythm_ioi_histogram* x)
{
}

/* EOF */
