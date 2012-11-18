/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_slave_metro.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef __PD_RHYTHM_SLAVE_METRO_H__
#define __PD_RHYTHM_SLAVE_METRO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MAXPDSTRING /* how primitive */
#include "m_pd.h"
#endif /* MAXPDSTRING */

#include "rhythm_slave_metro.h"

#define RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_STR "adapt_speed"


typedef struct
{
    t_object	x_obj;
    t_clock*	x_clock;

    Rhythm_Slave_Metro*	rhythm_slave_metro;

} pd_t_rhythm_slave_metro;

/* Object construction and destruction */
void		rhythm_slave_metro_setup (void);
static void*	pd_rhythm_slave_metro_new (t_symbol* s);
static void	pd_rhythm_slave_metro_free (pd_t_rhythm_slave_metro* x);

/* First inlet message callback for "bang" messages */
static void	pd_rhythm_slave_metro_bang (pd_t_rhythm_slave_metro* x);
/* First inlet message callback for float messages */
static void	pd_rhythm_slave_metro_float (pd_t_rhythm_slave_metro* x, t_floatarg f);
/* Second inlet message callback for float messages */
static void	pd_rhythm_slave_metro_ft1 (pd_t_rhythm_slave_metro* x, t_floatarg f);
/* Periodic clock tick callback */
static void	pd_rhythm_slave_metro_tick (pd_t_rhythm_slave_metro* x);
static void	pd_rhythm_slave_metro_set (pd_t_rhythm_slave_metro* x,
					   t_symbol* s,
					   t_float value);
static void	pd_rhythm_slave_metro_print (pd_t_rhythm_slave_metro* x);

/* Helper function for time */
static double	pd_rhythm_slave_metro_get_time (void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PD_RHYTHM_SLAVE_METRO_H__ */
/* EOF */

