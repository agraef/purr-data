/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_quantum.c,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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
#include "pd_rhythm_quantum.h"
#include "rhythm_quantum.h"

static t_class* pd_rhythm_quantum_class = NULL;

void
rhythm_quantum_setup (void)
{
    pd_rhythm_quantum_class = class_new (gensym ("rhythm_quantum"),
					(t_newmethod)pd_rhythm_quantum_new,
					(t_method)pd_rhythm_quantum_free,
					sizeof (pd_t_rhythm_quantum),
					0,
					A_DEFSYM, 0);
    assert (pd_rhythm_quantum_class != NULL);

    /* Set the callback for bang messages */
    class_addbang (pd_rhythm_quantum_class, pd_rhythm_quantum_bang);

    /* SET-Message */
    class_addmethod(pd_rhythm_quantum_class,
		    (t_method)pd_rhythm_quantum_set,
		    gensym("set"),
		    A_SYMBOL, A_FLOAT, 0);
    /* PRINT-Message */
    class_addmethod(pd_rhythm_quantum_class,
		    (t_method)pd_rhythm_quantum_print,
		    gensym("print"),
		    0);
}

static void*
pd_rhythm_quantum_new (t_symbol* s)
{
    pd_t_rhythm_quantum* x = NULL;

    /* Allocate object struct */
    x = (pd_t_rhythm_quantum*)pd_new (pd_rhythm_quantum_class);
    assert (x != NULL);

    /* Construct rhythm_quantum */
    x->rhythm_quantum = rhythm_quantum_new ();
    assert (x->rhythm_quantum != NULL);
    rhythm_quantum_initialize (x->rhythm_quantum);

    /* Initialize debug array */
    pd_rhythm_quantum_array_initialize (x, s);

    /* Create outlet(s) */

    /* Bang message outlet */
    outlet_new (&x->x_obj, gensym ("float"));

    return x;
}

static void
pd_rhythm_quantum_free (pd_t_rhythm_quantum* x)
{
    /* precondition(s) */
    assert (x != NULL);

    pd_rhythm_quantum_array_finish (x);

    /* Destruct rhythm_quantum */
    rhythm_quantum_finish (x->rhythm_quantum);
    rhythm_quantum_delete (x->rhythm_quantum);
    x->rhythm_quantum = NULL;
}

static void
pd_rhythm_quantum_bang (pd_t_rhythm_quantum* x)
{
    float *vector;
    unsigned len;
    unsigned i;
#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
    double time;
#endif

    /* precondition(s) */
    assert (x != NULL);

    /* Invoke rhythm_quantum onset event */

    pd_rhythm_quantum_array_read(x, &vector, &len);
#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
    time = sys_getrealtime();
#endif
    outlet_float(x->x_obj.ob_outlet,
		 rhythm_quantum_compute_quantum (x->rhythm_quantum, vector, len));

#if RHYTHM_ESTIMATOR_DEBUG_PROFILE
    printf("quantum: Used time: %f\n",  (sys_getrealtime()- time)*1000.);
#endif

    /* FIXME - Only for debugging: copy error function to histogram array */
#if RHYTHM_ESTIMATOR_DEBUG_QUANTUM
    for(i=0; i<x->rhythm_quantum->gcd_re_length; i++)
	vector[i]=x->rhythm_quantum->gcd_remainder_error[i];
    garray_redraw ((t_garray*)pd_findbyclass (x->array_symbol, garray_class));
#endif
}

static void
pd_rhythm_quantum_set (pd_t_rhythm_quantum* x,
		       t_symbol* s,
		       t_float value)
{

    /*  Format: s value */

    if(strlen(s->s_name))
    {	
			/* Check for IOI_RESOLUTION */
	    if (!strncmp( s->s_name, RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR
		      , strlen(RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR)))
	    {   
		/*		rhythm_ioi_quantum_set_ioi_resolution(x, value);*/
		x->rhythm_quantum->ioi_resolution =
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
		x->rhythm_quantum->min_quantum =
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
		x->rhythm_quantum->max_quantum =
		    ParameterCheck(RHYTHM_ESTIMATOR_MAX_QUANTUM_STR, 
				   value, 
				   RHYTHM_ESTIMATOR_MAX_QUANTUM_MIN,
				   RHYTHM_ESTIMATOR_MAX_QUANTUM_MAX);
		return;
	    }
	    else	    /* Check for GCD_PERC */
	    if (!strncmp(s->s_name,RHYTHM_QUANTUM_GCD_PERC_STR
		, strlen(RHYTHM_QUANTUM_GCD_PERC_STR)))
	    {	
		x->rhythm_quantum->gcd_perc =
		    ParameterCheck(RHYTHM_QUANTUM_GCD_PERC_STR, 
				   value, 
				   RHYTHM_QUANTUM_GCD_PERC_MIN,
				   RHYTHM_QUANTUM_GCD_PERC_MAX);
		return;
	    }
    }
    printf("Error, use format like: 'parameter value'\n");
    printf("Valid 'parameter' for rhythm_quantum:\n    ");
    printf(RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR);
    printf("\n    ");
    printf(RHYTHM_ESTIMATOR_MIN_QUANTUM_STR);
    printf("\n    ");
    printf(RHYTHM_ESTIMATOR_MAX_QUANTUM_STR);
    printf("\n    ");
    printf(RHYTHM_QUANTUM_GCD_PERC_STR);
    printf("\n\n");
}

static void
pd_rhythm_quantum_print (pd_t_rhythm_quantum* x)
{
    
    printf("QUANTUM-Paramater:\n    %s: %0.1f",
	   RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR, x->rhythm_quantum->ioi_resolution);
    printf("\n    ");
    printf("%s: %0.1f",RHYTHM_ESTIMATOR_MIN_QUANTUM_STR, x->rhythm_quantum->min_quantum);
    printf("\n    ");
    printf("%s: %0.1f",RHYTHM_ESTIMATOR_MAX_QUANTUM_STR, x->rhythm_quantum->max_quantum);
    printf("\n    ");
    printf("%s: %0.1f",RHYTHM_QUANTUM_GCD_PERC_STR, 
	   x->rhythm_quantum->gcd_perc);
    printf("\n\n");					 
}

static void
pd_rhythm_quantum_array_initialize (pd_t_rhythm_quantum* x,
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
	    pd_error(x, "rhythm_quantum: %s: no such array",
		     x->array_symbol->s_name);
	}
    	x->array_vec = 0;
    }

    /* Get array buffer length and pointer */

    else if (!garray_getfloatarray (a, &x->array_nsampsintab, &x->array_vec))
    {
    	error("rhythm_quantum: %s: bad array", x->array_symbol->s_name);
    	x->array_vec = 0;
    }
    else garray_usedindsp (a);

    if (x->array_vec == NULL)
    {
	printf ("rhythm_quantum: array %s not given\n", x->array_symbol->s_name);
    }
}

static void
pd_rhythm_quantum_array_read (pd_t_rhythm_quantum* x, float** vector,
			      unsigned* length)
{
    t_garray *a;

    /* Find the array by name and test if we still have an array */

    a = (t_garray*)pd_findbyclass (x->array_symbol, garray_class);
    if (a == 0)
    {
    	if (x->array_symbol->s_name != NULL)
	{
	    pd_error(x, "rhythm_quantum: %s: no such array",
		     x->array_symbol->s_name);
	}
    	x->array_vec = 0;
    }

    /* Initialize array buffer length and pointer */

    else if (!garray_getfloatarray (a, &x->array_nsampsintab, &x->array_vec))
    {
    	error("rhythm_quantum: %s: bad array", x->array_symbol->s_name);
    	x->array_vec = 0;
    }
    else
    {
	garray_usedindsp (a);
	*vector = x->array_vec;
	*length = x->array_nsampsintab;
    }
}

static void
pd_rhythm_quantum_array_finish (pd_t_rhythm_quantum* x)
{
}

/* EOF */
