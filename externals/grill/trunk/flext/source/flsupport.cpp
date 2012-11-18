/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3692 $
$LastChangedDate: 2009-06-17 09:46:01 -0400 (Wed, 17 Jun 2009) $
$LastChangedBy: thomas $
*/

/*! \file flsupport.cpp
    \brief flext support functions and classes.
*/
 
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

const t_symbol *flext::sym__ = NULL;
const t_symbol *flext::sym_float = NULL;
const t_symbol *flext::sym_symbol = NULL;
const t_symbol *flext::sym_bang = NULL;
const t_symbol *flext::sym_list = NULL;
const t_symbol *flext::sym_pointer = NULL;
const t_symbol *flext::sym_int = NULL;
const t_symbol *flext::sym_signal = NULL;

const t_symbol *flext::sym_anything = NULL;

#if FLEXT_SYS == FLEXT_SYS_MAX
const t_symbol *flext::sym_buffer = NULL;
const t_symbol *flext::sym_size = NULL;
const t_symbol *flext::sym_dirty = NULL;
#endif

const t_symbol *flext::sym_attributes = NULL;
const t_symbol *flext::sym_methods = NULL;

bool flext::indsp = false;


int flext::Version() { return FLEXT_VERSION; }
const char *flext::VersionStr() { return FLEXT_VERSTR; }

void flext::Setup()
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

void *flext_root::operator new(size_t bytes)
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

void flext_root::operator delete(void *blk)
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
bool flext_root::MemCheck(void *blk)
{
	char *ori = (char *)blk-sizeof(size_t)-sizeof(memtest);
	size_t bytes = *(size_t *)ori;

    return 
        *(size_t *)((char *)ori+sizeof(size_t)) == memtest && 
        *(size_t *)((char *)ori+bytes-sizeof(memtest)) == memtest;
}
#endif

#endif

void *flext_root::NewAligned(size_t bytes,int bitalign)
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

void flext_root::FreeAligned(void *blk)
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
void flext::GetAString(const t_atom &a,char *buf,size_t szbuf)
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

unsigned long flext::AtomHash(const t_atom &a)
{
#if FLEXT_SYS == FLEXT_SYS_MAX || FLEXT_SYS == FLEXT_SYS_PD
	return ((unsigned long)a.a_type<<28)^*(unsigned long *)&a.a_w;
#else
#error Not implemented
#endif
}

void flext_root::post(const char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);

	char buf[1024];
    vsnprintf(buf,sizeof buf,fmt, ap);
	buf[sizeof buf-1] = 0; // in case of full buffer
	::post(buf);

    va_end(ap);
}

void flext_root::error(const char *fmt,...)
{
	va_list ap;
    va_start(ap, fmt);

	char buf[1024];
    STD::strcpy(buf,"error: ");
    vsnprintf(buf+7,sizeof buf-7,fmt, ap);
	buf[sizeof buf-1] = 0; // in case of full buffer
	::post(buf);

    va_end(ap);
}

#include "flpopns.h"

