/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_OPLOOP_H
#define __VASP_OPLOOP_H

#define _D_BLOCK 4096

#define _D_MIN(a,b) ((a) < (b)?(a):(b))

#ifdef FLEXT_THREADS

    #define _D_LOOP(VAR,LEN) { \
	    for(register int __i__ = 0; __i__ < LEN; flext_base::ThrYield()) { \
	    register const I __m__ = _D_MIN(LEN,__i__+_D_BLOCK); \
        for(; __i__ < __m__; ++__i__) { VAR = __i__; 

    #define _E_LOOP	} if(__i__ < __m__) break; }}

    #define _D_WHILE(COND) { \
	    for(; (COND) ; flext_base::ThrYield()) { \
	    register I __i__ = 0;  \
	    for(; __i__ < _D_BLOCK && (COND); ++__i__) {

    #define _E_WHILE } if(__i__ < _D_BLOCK) break; }}

#else

    #define _D_LOOP(VAR,LEN) { \
	    for(VAR = 0; VAR < LEN; ++VAR) {

    #define _E_LOOP }}

    #define _D_WHILE(COND) { \
	    while(COND) {

    #define _E_WHILE }}

#endif


#define _DE_WHILE(COND,BODY) { _D_WHILE(COND) BODY; _E_WHILE }
#define _DF_LOOP(VAR,LEN,BODY) { _D_LOOP(VAR,LEN) BODY; _E_LOOP }

#ifdef VASP_COMPACT

    #define _DE_LOOP(VAR,LEN,BODY) _DF_LOOP(VAR,LEN,BODY)

#else

    #ifdef FLEXT_THREADS

        #define _DE_LOOP(VAR,LEN,BODY) { \
	        for(VAR = 0; VAR < LEN; flext_base::ThrYield()) { \
                register const I __m__ = _D_MIN(LEN,VAR+_D_BLOCK); \
                for(; VAR <= __m__-4; ) { \
                    BODY; ++VAR; \
                    BODY; ++VAR; \
                    BODY; ++VAR; \
                    BODY; ++VAR; \
                } \
                for(; VAR < __m__; ++VAR) { \
                    BODY; \
                } \
            } \
        }

    #else

        #define _DE_LOOP(VAR,LEN,BODY) { \
            for(VAR = 0; VAR <= LEN-4; ) { \
                BODY; ++VAR; \
                BODY; ++VAR; \
                BODY; ++VAR; \
                BODY; ++VAR; \
            } \
            for(; VAR < LEN; ++VAR) { \
                BODY; \
            } \
        }

    #endif

#endif

#define _DQ_LOOP(UNROLL,VAR,LEN,BODY) { if(UNROLL) _DE_LOOP(VAR,LEN,BODY) else _DF_LOOP(VAR,LEN,BODY) }

#endif
