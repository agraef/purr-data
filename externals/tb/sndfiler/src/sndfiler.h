/*
 *
 * threaded soundfiler for pd
 * Copyright (C) 2005, Tim Blechmann
 *           (C) 2005, Georg Holzmann <grh@mur.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _SND_FILER__
#define _SND_FILER__


/* to be compatible with main pd */
#ifdef USE_PD_MAIN

#define  getalignedbytes(a)     getbytes(a)
#define  freealignedbytes(a,b)  freebytes(a,b)
#include "threadlib.h"

#else /* now for pd_devel */

#include "m_pd.h"
#include "m_fifo.h"

#include "pthread.h"

#endif /* USE_PD_MAIN */


#include "g_canvas.h"
#include "sndfile.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "stdlib.h"
#include <stdio.h>
#include "sched.h" /* for thread priority */
#include <string.h>
#include "semaphore.h"

#ifdef MSW
#include <io.h>
#include <fcntl.h>
#endif

/* for alloca */
#ifdef MSW
#include <malloc.h>
#else
#include "alloca.h"
#endif

#if (_POSIX_MEMLOCK - 0) >=  200112L
#include <sys/mman.h>
#endif /* _POSIX_MEMLOCK */

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/semaphore.h>
#define SEM_T semaphore_t
#define SEM_INIT(s) (semaphore_create(mach_task_self(),&s,SYNC_POLICY_FIFO,0) == 0)
#define SEM_SIGNAL(s) semaphore_signal(s)
#define SEM_WAIT(s) semaphore_wait(s)
#else
#define SEM_T sem_t
#define SEM_INIT(s) (sem_init(&s,0,0) == 0)
#define SEM_SIGNAL(s) sem_post(&s)
#define SEM_WAIT(s) sem_wait(&s)
#endif


#endif // _SND_FILER__
