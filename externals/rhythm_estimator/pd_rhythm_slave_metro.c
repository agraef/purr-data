/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_slave_metro.c,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef MAXPDSTRING /* how primitive */
#include "m_pd.h"
#endif /* MAXPDSTRING */

#include "pd_rhythm_estimator.h"
#include "rhythm_estimator.h"
#include "pd_rhythm_slave_metro.h"
#include "rhythm_slave_metro.h"

static t_class* pd_rhythm_slave_metro_class = NULL;

/* We need a reference systime value */
static double pd_rhythm_slave_metro_setup_systime = 0;


void
rhythm_slave_metro_setup (void)
{
    pd_rhythm_slave_metro_class = class_new (gensym ("rhythm_slave_metro"),
					     (t_newmethod)pd_rhythm_slave_metro_new,
					     (t_method)pd_rhythm_slave_metro_free,
					     sizeof (pd_t_rhythm_slave_metro),
					     0,
					     0);
    assert (pd_rhythm_slave_metro_class != NULL);

    /* Initialize setup time variable */
    pd_rhythm_slave_metro_setup_systime = clock_getsystime ();

    /* Set the callback for bang messages in the first inlet */
    class_addbang (pd_rhythm_slave_metro_class, pd_rhythm_slave_metro_bang);
    /* Set the callback for float messages in the first inlet */
    class_addfloat (pd_rhythm_slave_metro_class, pd_rhythm_slave_metro_float);
    /* Set the callback for float messages in the second inlet */
    class_addmethod (pd_rhythm_slave_metro_class,
		     (t_method)pd_rhythm_slave_metro_ft1,
		     gensym ("ft1"),
		     A_FLOAT, 0);
    /* SET-Message */
    class_addmethod (pd_rhythm_slave_metro_class,
		     (t_method)pd_rhythm_slave_metro_set,
		     gensym("set"),
		     A_SYMBOL, A_FLOAT, 0);
    /* PRINT-Message */
    class_addmethod (pd_rhythm_slave_metro_class,
		     (t_method)pd_rhythm_slave_metro_print,
		     gensym("print"),
		     0);
}

static void*
pd_rhythm_slave_metro_new (t_symbol* s)
{
    pd_t_rhythm_slave_metro* x = NULL;

    /* Allocate object struct */
    x = (pd_t_rhythm_slave_metro*)pd_new (pd_rhythm_slave_metro_class);
    assert (x != NULL);

    /* Construct clock */
    x->x_clock = clock_new (x, (t_method)pd_rhythm_slave_metro_tick);
    assert (x->x_clock != NULL);

    /* Construct rhythm_slave_metro */
    x->rhythm_slave_metro = rhythm_slave_metro_new ();
    assert (x->rhythm_slave_metro != NULL);
    rhythm_slave_metro_initialize (x->rhythm_slave_metro);

     /* Create in- and outlet(s) */

    /* Second inlet for period time */
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("float"), gensym ("ft1"));
    /* Bang message outlet */
    outlet_new (&x->x_obj, gensym ("bang"));

    /* Start the periodic timer clock */
    pd_rhythm_slave_metro_ft1 (x, 500); /* initialize clock period */
    pd_rhythm_slave_metro_tick (x); /* schedule first clock tick */

    return x;
}

static void
pd_rhythm_slave_metro_free (pd_t_rhythm_slave_metro* x)
{
    /* precondition(s) */
    assert (x != NULL);

    /* Stop the periodic timer clock */
    clock_unset (x->x_clock);

    /* Destruct rhythm_slave_metro */
    rhythm_slave_metro_finish (x->rhythm_slave_metro);
    rhythm_slave_metro_delete (x->rhythm_slave_metro);
    x->rhythm_slave_metro = NULL;

    /* Destruct clock */
    clock_free (x->x_clock);
    x->x_clock = NULL;
}

static void
pd_rhythm_slave_metro_bang (pd_t_rhythm_slave_metro* x)
{
    /* precondition(s) */
    assert (x != NULL);

    /* a "bang" message is equal to a float "0" message */
    pd_rhythm_slave_metro_float (x, 0);
}

static void
pd_rhythm_slave_metro_float (pd_t_rhythm_slave_metro* x, t_floatarg f)
{
    double new_delay;

    /* precondition(s) */
    assert (x != NULL);

    /* Invoke rhythm_slave_metro onset event */
    new_delay = rhythm_slave_metro_onset_event (x->rhythm_slave_metro,
						pd_rhythm_slave_metro_get_time ());
    if (new_delay)
    {
	/* Reschedule, i.e. cancel the old and schedule a new tick */
	clock_delay (x->x_clock, new_delay);
    }
}

static void
pd_rhythm_slave_metro_ft1 (pd_t_rhythm_slave_metro* x, t_floatarg f)
{
    /* precondition(s) */
    assert (x != NULL);

    /* update clock period */
    rhythm_slave_metro_set_period (x->rhythm_slave_metro, f);
}

static void
pd_rhythm_slave_metro_tick (pd_t_rhythm_slave_metro* x)
{
    double new_delay;

    /* precondition(s) */
    assert (x != NULL);

    /* Invoke rhythm_slave_metro periodic event */
    new_delay = rhythm_slave_metro_periodic_event (x->rhythm_slave_metro,
						   pd_rhythm_slave_metro_get_time ());
    if (new_delay)
    {
	/* Schedule a new tick */
	clock_delay (x->x_clock, new_delay);
    }

    /* Send a "bang" message to the output */
    outlet_bang (x->x_obj.ob_outlet);
}

static void
pd_rhythm_slave_metro_set (pd_t_rhythm_slave_metro* x,
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
		x->rhythm_slave_metro->ioi_resolution =
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
		return;
	    }
	    else	    /* Check for MAX_QUANTUM */
	    if (!strncmp(s->s_name, RHYTHM_ESTIMATOR_MAX_QUANTUM_STR
		, strlen(RHYTHM_ESTIMATOR_MAX_QUANTUM_STR)))
	    {	
		return;
	    }
	    else	    /* Check for PHASE_ADAP_SPEED */
	    if (!strncmp(s->s_name,RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_STR
		, strlen(RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_STR)))
	    {	
		x->rhythm_slave_metro->phase_adap_speed =
		    ParameterCheck(RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_STR, 
				   value, 
				   RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_MIN,
				   RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_MAX);
		return;
	    }
    }
    printf("Error, use format like: 'parameter value'\n");
    printf("Valid 'parameter' for slave_metro:\n    ");
    printf(RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR);
    printf("\n    ");
    printf(RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_STR);
    printf("\n\n");
}

static void
pd_rhythm_slave_metro_print (pd_t_rhythm_slave_metro* x)
{
    
    printf("SLAVE_METRO-Paramater:\n    %s: %0.1f",
	   RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR, x->rhythm_slave_metro->ioi_resolution);
    printf("\n    ");
    printf("%s: %0.1f", RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_STR, 
	   x->rhythm_slave_metro->phase_adap_speed);
    printf("\n\n");					 
}

static double
pd_rhythm_slave_metro_get_time (void)
{
    return clock_gettimesince (pd_rhythm_slave_metro_setup_systime);
}

/* EOF */
