/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3686 $
$LastChangedDate: 2009-06-10 12:44:55 -0400 (Wed, 10 Jun 2009) $
$LastChangedBy: thomas $
*/

/*! \file flatom_pr.cpp
    \brief Definitions for printing and scanning the t_atom type.
*/
 
#include "flext.h"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "flpushns.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

bool flext::PrintAtom(const t_atom &a,char *buf,size_t bufsz)
{
    bool ok = true;
    if(IsFloat(a)) {
        ok = STD::snprintf(buf,bufsz,"%g",GetFloat(a)) > 0;
    }
    else if(IsInt(a)) {
        ok = STD::snprintf(buf,bufsz,"%i",GetInt(a)) > 0;
    }
    else if(IsSymbol(a)) {
		const char *c = GetString(a);
		size_t len = strlen(c);
		if(len < bufsz) {
			memcpy(buf,c,len); buf[len] = 0;
			ok = true;
		}
		else 
			ok = false;
    }
#if FLEXT_SYS == FLEXT_SYS_PD
#ifndef FLEXT_COMPATIBLE
    else if(IsPointer(a)) {
        ok = STD::snprintf(buf,bufsz,"%p",GetPointer(a)) > 0;
    }
#endif
    else if(a.a_type == A_DOLLAR) {
        ok = STD::snprintf(buf,bufsz,"$%d",a.a_w.w_index) > 0;
    }
    else if(a.a_type == A_DOLLSYM) {
        ok = STD::snprintf(buf,bufsz,"$%s",GetString(a)) > 0;
    }
#elif FLEXT_SYS == FLEXT_SYS_MAX
    else if(a.a_type == A_DOLLAR) {
        ok = STD::snprintf(buf,bufsz,"$%ld",a.a_w.w_long) > 0;
    }
#else
//#pragma message("Not implemented")
#endif
    else {
        error("flext: atom type unknown");
        ok = false;
    }
    return ok;
}

bool flext::PrintList(int argc,const t_atom *argv,char *buf,size_t bufsz)
{
    bool ok = true;
    for(int i = 0; ok && i < argc && bufsz > 0; ++i) {
        if(i) { *(buf++) = ' '; --bufsz; } // prepend space

        if(PrintAtom(argv[i],buf,bufsz)) {
            size_t len = strlen(buf);
            buf += len,bufsz -= len;
        }
        else
            ok = false;
    }
    *buf = 0;
    return ok;
}


const char *flext::ScanAtom(t_atom &a,const char *c)
{
	// skip leading whitespace
	while(*c && isspace(*c)) ++c;
	if(!*c) return NULL;

    // go to next space and save character
    char *end = const_cast<char *>(c);
    while(*end && !isspace(*end)) ++end;
    char sv = *end;

    float fres;
    // first try float
    char *endp;
    // see if it's a float - thanks to Frank Barknecht
    fres = (float)strtod(c,&endp);   
    if(*c && endp != c) { 
        int ires = (int)fres; // try a cast
        if(fres == ires)
            SetInt(a,ires);
        else
            SetFloat(a,fres);
    }
    // no, it's a symbol
    else
        SetString(a,c);

    *end = sv;

	return end;
}

int flext::ScanList(int argc,t_atom *argv,const char *buf)
{
    int read;    
    for(read = 0; read < argc; ++read)
    {
        buf = ScanAtom(argv[read],buf);
        if(!buf) break;
    }
    return read;
}

#include "flpopns.h"
