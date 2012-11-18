/* Rhythm estimation in real time -- PD wrapper
   Copyright (C) 2000 Jarno Seppänen and Piotr Majdak
   $Id: pd_rhythm_estimator.c,v 1.1 2002-11-19 11:39:55 ggeiger Exp $

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


float ParameterCheck (char* name, float value, float min, float max )
{

    if (value > max)
    {
	printf("Parameter: %s out of range [%.1f...%.1f]. Value limited.\n", name,
	       min, max );
	return(max);
    }

    if(value < min)
    {
	printf("Parameter: %s out of range [%.1f...%.1f]. Value limited.\n", name,
	       min, max );
	return(min);	
    }
    return(value);
}
