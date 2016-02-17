/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

#ifdef FLEXT_USE_NAMESPACE

#ifndef _FLEXT_IN_NAMESPACE
    #define _FLEXT_IN_NAMESPACE 0
#endif

#if _FLEXT_IN_NAMESPACE == 0

    #if 1 //defined(FLEXT_SHARED)
    namespace flext_ns {
    #elif defined(__GNUC__)
    namespace {  // anonymous namespace (don't export symbols)
    #endif

#endif

#define __FLEXT_IN_NAMESPACE (_FLEXT_IN_NAMESPACE+1)
#undef _FLEXT_IN_NAMESPACE
#define _FLEXT_IN_NAMESPACE __FLEXT_IN_NAMESPACE
#undef __FLEXT_IN_NAMESPACE

#endif
