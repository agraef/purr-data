/* ------------------------ netsend~ ------------------------------------------ */
/*                                                                              */
/* Tilde object to send uncompressed audio data to netreceive~.                 */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>.                               */
/* Based on streamout~ by Guenter Geiger.                                       */
/* Get source at http://www.akustische-kunst.org/                               */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* See file LICENSE for further informations on licensing terms.                */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/* This project was commissioned by the Society for Arts and Technology [SAT],  */
/* Montreal, Quebec, Canada, http://www.sat.qc.ca/.                             */
/*                                                                              */
/* ---------------------------------------------------------------------------- */


/* This file is based on and inspired by stream.h (C) Guenter Geiger 1999.   */
/* Some enhancements have been made with the goal of keeping compatibility   */
/* between the stream formats of streamout~/in~ and netsend~/receive~.       */

#define VERSION "1.0alpha11"

#define DEFAULT_AUDIO_CHANNELS 32	    /* nax. number of audio channels we support */
#define DEFAULT_AUDIO_BUFFER_SIZE 1024	/* number of samples in one audio block */
#define DEFAULT_UDP_PACKT_SIZE 8192		/* number of bytes we send in one UDP datagram (OS X only) */
#define DEFAULT_PORT 8000               /* default network port number */

#ifdef _WINDOWS
#ifndef HAVE_INT32_T
typedef int int32_t;
#define HAVE_INT32_T
#endif
#ifndef HAVE_INT16_T
typedef short int16_t;
#define HAVE_INT16_T
#endif
#ifndef HAVE_U_INT32_T
typedef unsigned int u_int32_t;
#define HAVE_U_INT32_T
#endif
#ifndef HAVE_U_INT16_T
typedef unsigned short u_int16_t;
#define HAVE_U_INT16_T
#endif
#endif

#ifndef CLIP
#define CLIP(a, lo, hi) ( (a)>(lo)?( (a)<(hi)?(a):(hi) ):(lo) )
#endif


/* swap 32bit t_float. Is there a better way to do that???? */
#ifdef _WINDOWS
__inline static float netsend_float(float f)
#else
inline static float netsend_float(float f)
#endif
{
    union
    {
        float f;
        unsigned char b[4];
    } dat1, dat2;
    
    dat1.f = f;
    dat2.b[0] = dat1.b[3];
    dat2.b[1] = dat1.b[2];
    dat2.b[2] = dat1.b[1];
    dat2.b[3] = dat1.b[0];
    return dat2.f;
}

/* swap 32bit long int */
#ifdef _WINDOWS
__inline static long netsend_long(long n)
#else
inline static long netsend_long(long n)
#endif
{
    return (((n & 0xff) << 24) | ((n & 0xff00) << 8) |
    	((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24));
}

/* swap 16bit short int */
#ifdef _WINDOWS
__inline static long netsend_short(long n)
#else
inline static short netsend_short(short n)
#endif
{
    return (((n & 0xff) << 8) | ((n & 0xff00) >> 8));
}


/* format specific stuff */

#define SF_FLOAT  1
#define SF_DOUBLE 2		/* not implemented */
#define SF_8BIT   10
#define SF_16BIT  11
#define SF_32BIT  12	/* not implemented */
#define SF_ALAW   20	/* not implemented */
#define SF_MP3    30    /* not implemented */
#define SF_AAC    31    /* AAC encoding using FAAC */
#define SF_VORBIS 40	/* not implemented */
#define SF_FLAC   50	/* not implemented */

#define SF_SIZEOF(a) (a == SF_FLOAT ? sizeof(t_float) : \
                     a == SF_16BIT ? sizeof(short) : 1)


/* version / byte-endian specific stuff */

#define SF_BYTE_LE 1		/* little endian */
#define SF_BYTE_BE 2		/* big endian */

#if defined(_WINDOWS) || defined(__linux__) || defined(IRIX)
#define SF_BYTE_NATIVE SF_BYTE_LE
#else /* must be  __APPLE__ */
#define SF_BYTE_NATIVE SF_BYTE_BE
#endif


typedef struct _tag {      /* size (bytes) */
     char version;         /*    1         */
     char format;          /*    1         */
     long count;           /*    4         */
     char channels;        /*    1         */
     long framesize;       /*    4         */
     char  extension[5];   /*    5         */
} t_tag;                   /*--------------*/
                           /*   16         */


typedef struct _frame {
     t_tag  tag;
     char  *data;
} t_frame;

