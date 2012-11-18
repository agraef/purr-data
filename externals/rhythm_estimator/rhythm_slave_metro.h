/* Rhythm estimation in real time
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: rhythm_slave_metro.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef __RHYTHM_SLAVE_METRO_H__
#define __RHYTHM_SLAVE_METRO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Parameter default values */
#define RHYTHM_SLAVE_METRO_DEFAULT_PHASE_ADAP_SPEED	0.1
#define RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_MIN 0
#define RHYTHM_SLAVE_METRO_PHASE_ADAP_SPEED_MAX 1

typedef struct
{
    /* --- Public data --- */

    /* User-changeable parameters: */
    float	phase_adap_speed;
    float	ioi_resolution;

    /* --- Private data --- */

    double	last_bang_time;	/* [ms] */

    double	period;		/* quantum [ms] */

    double	dev_mean_sum_re;	/* circular mean - real part [ms] */
    double	dev_mean_sum_im;	/* circular mean - imaginary part [ms] */
    double	dev_mean_count;
    double	dev_syst;	/* systematic deviation (lpf) */
    double	dev_syst_limit;	/* syst. deviation limit */

} Rhythm_Slave_Metro;


Rhythm_Slave_Metro*	rhythm_slave_metro_new (void);
void	rhythm_slave_metro_delete (Rhythm_Slave_Metro* rhythm_slave_metro);

void	rhythm_slave_metro_initialize (Rhythm_Slave_Metro* rhythm_slave_metro);
void	rhythm_slave_metro_finish (Rhythm_Slave_Metro* rhythm_slave_metro);

void	rhythm_slave_metro_set_period (Rhythm_Slave_Metro* rhythm_slave_metro,
				       double period);
double	rhythm_slave_metro_get_period (Rhythm_Slave_Metro* rhythm_slave_metro);

double	rhythm_slave_metro_onset_event (Rhythm_Slave_Metro* rhythm_slave_metro,
					double time);
double	rhythm_slave_metro_periodic_event (Rhythm_Slave_Metro* rhythm_slave_metro,
					   double time);

float	rhythm_slave_metro_get_phase_adap_speed (Rhythm_Slave_Metro* rhythm_slave_metro);
void	rhythm_slave_metro_set_phase_adap_speed (Rhythm_Slave_Metro* rhythm_slave_metro, float v);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RHYTHM_SLAVE_METRO_H__ */
/* EOF */
