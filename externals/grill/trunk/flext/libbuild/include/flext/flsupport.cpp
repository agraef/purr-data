/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flsupport.cpp
    \brief flext support functions and classes.
*/

#ifndef __FLEXT_SUPPORT_CPP
#define __FLEXT_SUPPORT_CPP

#include "flext.h"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <new>

#include "flpushns.h"

#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif

FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym__ = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_float = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_symbol = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_bang = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_list = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_pointer = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_int = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_signal = NULL;

FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_anything = NULL;

#if FLEXT_SYS == FLEXT_SYS_MAX
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_buffer = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_size = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_dirty = NULL;
#endif

FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_attributes = NULL;
FLEXT_TEMPIMPL(const t_symbol *FLEXT_CLASSDEF(flext))::sym_methods = NULL;

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::indsp = false;


FLEXT_TEMPIMPL(int FLEXT_CLASSDEF(flext))::Version() { return FLEXT_VERSION; }
FLEXT_TEMPIMPL(const char *FLEXT_CLASSDEF(flext))::VersionStr() { return FLEXT_VERSTR; }

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::Setup()
{
	if(sym__) return;

#if FLEXT_SYS == FLEXT_SYS_PD
	sym__ = &s_;
	sym_anything = &s_anything;
	sym_pointer = &s_pointer;
	sym_float = &s_float;
	sym_symbol = &s_symbol;
	sym_bang = &s_bang;
	sym_list = &s_list;
	sym_signal = &s_signal;
	sym_int = flext::MakeSymbol("int");
#elif FLEXT_SYS == FLEXT_SYS_MAX
	sym__ = flext::MakeSymbol("");
	sym_int = flext::MakeSymbol("int");
	sym_float = flext::MakeSymbol("float");
	sym_symbol = flext::MakeSymbol("symbol");
	sym_bang = flext::MakeSymbol("bang");
	sym_list = flext::MakeSymbol("list");
	sym_anything = flext::MakeSymbol("anything");
	sym_signal = flext::MakeSymbol("signal");

    sym_buffer = flext::MakeSymbol("buffer~");
    sym_size = flext::MakeSymbol("size");
    sym_dirty = flext::MakeSymbol("dirty");
#endif
    
    sym_attributes = flext::MakeSymbol("attributes");
    sym_methods = flext::MakeSymbol("methods");

#ifdef FLEXT_THREADS
	thrid = GetThreadId();
    StartHelper();
#endif
}


#if FLEXT_SYS == FLEXT_SYS_PD && defined(FLEXT_THREADED) && defined(FLEXT_PDLOCK)
#define SYSLOCK() sys_lock()
#define SYSUNLOCK() sys_unlock()
#else
#define SYSLOCK() (void)0
#define SYSUNLOCK() (void)0
#endif


/////////////////////////////////////////////////////////
// overloaded new/delete memory allocation methods
//
/////////////////////////////////////////////////////////

#define LARGEALLOC 32000

#ifndef FLEXT_USE_CMEM

#ifdef FLEXT_DEBUGMEM
static const size_t memtest = 0x12345678L;
#endif

FLEXT_TEMPIMPL(void *FLEXT_CLASSDEF(flext_root))::operator new(size_t bytes)
{
	bytes += sizeof(size_t);
#ifdef FLEXT_DEBUGMEM
    bytes += sizeof(memtest)*2;
#endif
    char *blk;
    if(UNLIKELY(bytes >= LARGEALLOC)) {
#if FLEXT_SYS == FLEXT_SYS_MAX && defined(_SYSMEM_H_)
        blk = (char *)sysmem_newptr(bytes);
#else
        // use C library function for large memory blocks
        blk = (char *)malloc(bytes);
#endif
    }
    else {
    	//! We need system locking here for secondary threads!
        SYSLOCK();
	    blk = (char *)getbytes(bytes);
        SYSUNLOCK();
    }

	FLEXT_ASSERT(blk);

	*(size_t *)blk = bytes;
#ifdef FLEXT_DEBUGMEM
    *(size_t *)(blk+sizeof(size_t)) = memtest;
    *(size_t *)(blk+bytes-sizeof(memtest)) = memtest;
	return blk+sizeof(size_t)+sizeof(memtest);
#else
	return blk+sizeof(size_t);
#endif
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_root))::operator delete(void *blk)
{
    if(!blk) return;

    FLEXT_ASSERT(MemCheck(blk));

#ifdef FLEXT_DEBUGMEM
	char *ori = (char *)blk-sizeof(size_t)-sizeof(memtest);
#else
    char *ori = (char *)blk-sizeof(size_t);
#endif
	size_t bytes = *(size_t *)ori;

    if(UNLIKELY(bytes >= LARGEALLOC)) {
#if FLEXT_SYS == FLEXT_SYS_MAX && defined(_SYSMEM_H_)
        sysmem_freeptr(ori);
#else
        // use C library function for large memory blocks
        free(ori);
#endif
    }
    else {
	    //! We need system locking here for secondary threads!
        SYSLOCK();
	    freebytes(ori,bytes);
        SYSUNLOCK();
    }
}

#ifdef FLEXT_DEBUGMEM
FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_root))::MemCheck(void *blk)
{
	char *ori = (char *)blk-sizeof(size_t)-sizeof(memtest);
	size_t bytes = *(size_t *)ori;

    return 
        *(size_t *)((char *)ori+sizeof(size_t)) == memtest && 
        *(size_t *)((char *)ori+bytes-sizeof(memtest)) == memtest;
}
#endif

#endif

FLEXT_TEMPIMPL(void *FLEXT_CLASSDEF(flext_root))::NewAligned(size_t bytes,int bitalign)
{
	const size_t ovh = sizeof(size_t)+sizeof(char *);
	const size_t alignovh = bitalign/8-1;
	bytes += ovh+alignovh;

    char *blk;
    if(UNLIKELY(bytes >= LARGEALLOC)) {
#if FLEXT_SYS == FLEXT_SYS_MAX && defined(_SYSMEM_H_)
        blk = (char *)sysmem_newptr(bytes);
#else
        // use C library function for large memory blocks
        blk = (char *)malloc(bytes);
#endif
    }
    else {
	//! We need system locking here for secondary threads!
        SYSLOCK();

#if defined(FLEXT_USE_CMEM)
	    blk = (char *)malloc(bytes);
#else
	    blk = (char *)getbytes(bytes);
#endif
        SYSUNLOCK();
    }
	FLEXT_ASSERT(blk);

	char *ablk = reinterpret_cast<char *>((reinterpret_cast<size_t>(blk)+ovh+alignovh) & ~alignovh);
	*(char **)(ablk-sizeof(size_t)-sizeof(char *)) = blk;
	*(size_t *)(ablk-sizeof(size_t)) = bytes;
	return ablk;
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_root))::FreeAligned(void *blk)
{
	FLEXT_ASSERT(blk);

	char *ori = *(char **)((char *)blk-sizeof(size_t)-sizeof(char *));
	size_t bytes = *(size_t *)((char *)blk-sizeof(size_t));

    if(UNLIKELY(bytes >= LARGEALLOC)) {
#if FLEXT_SYS == FLEXT_SYS_MAX && defined(_SYSMEM_H_)
        sysmem_freeptr(ori);
#else
        // use C library function for large memory blocks
        free(ori);
#endif
    }
    else {
	//! We need system locking here for secondary threads!
        SYSLOCK();

#if defined(FLEXT_USE_CMEM)
	    free(ori);
#else
	    freebytes(ori,bytes);
#endif
        SYSUNLOCK();
    }
}

// ------------------------------------------

/*! \todo there is probably also a shortcut for Max and jMax
    \todo size checking
*/
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::GetAString(const t_atom &a,char *buf,size_t szbuf)
{ 
#if FLEXT_SYS == FLEXT_SYS_PD
	atom_string(const_cast<t_atom *>(&a),buf,(int)szbuf);
#else
    if(IsSymbol(a)) STD::strncpy(buf,GetString(a),szbuf);
	else if(IsFloat(a)) STD::snprintf(buf,szbuf,"%f",GetFloat(a));
	else if(IsInt(a)) STD::snprintf(buf,szbuf,"%i",GetInt(a));
    else *buf = 0;
#endif
}  

FLEXT_TEMPIMPL(unsigned long FLEXT_CLASSDEF(flext))::AtomHash(const t_atom &a)
{
#if FLEXT_SYS == FLEXT_SYS_MAX || FLEXT_SYS == FLEXT_SYS_PD
	return ((unsigned long)a.a_type<<28)^*(unsigned long *)&a.a_w;
#else
#error Not implemented
#endif
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_root))::post(const char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);

	char buf[1024];
    vsnprintf(buf,sizeof buf,fmt, ap);
	buf[sizeof buf-1] = 0; // in case of full buffer
	
#if FLEXT_SYS == FLEXT_SYS_MAX && C74_MAX_SDK_VERSION >= 0x0500
    ::object_post(NULL,buf);
#else
	::post(buf);
#endif

    va_end(ap);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_root))::error(const char *fmt,...)
{
	va_list ap;
    va_start(ap, fmt);

	char buf[1024];
    STD::strcpy(buf,"error: ");
    vsnprintf(buf+7,sizeof buf-7,fmt, ap);
	buf[sizeof buf-1] = 0; // in case of full buffer

#if FLEXT_SYS == FLEXT_SYS_MAX
    #if C74_MAX_SDK_VERSION >= 0x0500
        ::object_error(NULL,buf);
    #else
    	::error(buf);
    #endif
#else
	::post(buf);
#endif

    va_end(ap);
}

#include "flpopns.h"

#endif // __FLEXT_SUPPORT_CPP


