/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file flcwmax-thr.h
    \brief Prefix file for CodeWarrior projects - OS 9 threaded version.
*/
 
#ifndef _FLEXT_CW_MAX_THR_H
#define _FLEXT_CW_MAX_THR_H

#define FLEXT_THREADS

/*
    old CodeWarrior version (<= 6) don't have sigset_t defined which
    is needed for pthreads
*/
#if defined(__MWERKS__) && (__MWERKS__ == 1)  // read __MWERKS__ numbering starts with CW7
    typedef unsigned int sigset_t;
    #define _CW_NOPRECOMP // no precompiled headers
#endif

#include "flcwmax.h"

#endif
