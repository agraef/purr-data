/* Rhythm estimation in real time
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: rhythm_estimator.h,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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

#ifndef __RHYTHM_ESTIMATOR_H__
#define __RHYTHM_ESTIMATOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Parameter default values */
#define RHYTHM_ESTIMATOR_DEFAULT_IOI_RESOLUTION	5 /* ms */
#define RHYTHM_ESTIMATOR_DEFAULT_MIN_QUANTUM	80 /* ms */
#define RHYTHM_ESTIMATOR_DEFAULT_MAX_QUANTUM	400 /* ms */

#define RHYTHM_ESTIMATOR_IOI_RESOLUTION_MIN 1
#define RHYTHM_ESTIMATOR_MIN_QUANTUM_MIN 10	
#define RHYTHM_ESTIMATOR_MAX_QUANTUM_MIN 100	
			
#define RHYTHM_ESTIMATOR_IOI_RESOLUTION_MAX 40
#define RHYTHM_ESTIMATOR_MIN_QUANTUM_MAX 99
#define RHYTHM_ESTIMATOR_MAX_QUANTUM_MAX 1000	


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RHYTHM_ESTIMATOR_H__ */
/* EOF */
