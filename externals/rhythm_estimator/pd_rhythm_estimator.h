/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_estimator.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef __PD_RHYTHM_ESTIMATOR_H__
#define __PD_RHYTHM_ESTIMATOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* compilation "flags" for debugging */
#define RHYTHM_ESTIMATOR_DEBUG_PROFILE 0
#define RHYTHM_ESTIMATOR_DEBUG_TEXT 0
#define RHYTHM_ESTIMATOR_DEBUG_QUANTUM 0
#define RHYTHM_ESTIMATOR_DEBUG_TEXT_SP 0


/* Parameter strings for setting using set-method */
#define RHYTHM_ESTIMATOR_IOI_RESOLUTION_STR "ioi_resolution"
#define RHYTHM_ESTIMATOR_MIN_QUANTUM_STR "min_quantum"
#define RHYTHM_ESTIMATOR_MAX_QUANTUM_STR "max_quantum"

/* Checks/limits parameter for bounds and prints warnings */
float ParameterCheck (char* name, float value, float min, float max );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PD_RHYTHM_ESTIMATOR_H__ */
/* EOF */
