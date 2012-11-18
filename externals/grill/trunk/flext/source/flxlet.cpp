/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3669 $
$LastChangedDate: 2009-03-05 18:34:39 -0500 (Thu, 05 Mar 2009) $
$LastChangedBy: thomas $
*/

/*! \file flxlet.cpp
    \brief Implementation of the variable inlet/outlet functionality.
*/
 
#include "flext.h"
#include "flinternal.h"
#include <cstring>
#include <cstdarg>

#include "flpushns.h"

#define MAXLETS 256

flext_base::xlet flext_base::inlist[MAXLETS],flext_base::outlist[MAXLETS];

flext_base::xlet::xlet(): tp(xlet_none),desc(NULL) {}
flext_base::xlet::~xlet() { if(desc) delete[] desc; }

void flext_base::xlet::Desc(const char *c)
{
    if(desc) delete[] desc;
    if(c) {
        size_t l = strlen(c);
        desc = new char[l+1];
        memcpy(desc,c,l+1);
    }
    else
        desc = NULL;
}

void flext_base::AddInlet(xlettype tp,int mult,const char *desc)
{
    if(UNLIKELY(incnt+mult >= MAXLETS))
        post("%s - too many inlets",thisName());
    else
        for(int i = 0; i < mult; ++i) {
            xlet &x = inlist[incnt++];
            x.tp = tp;
            x.Desc(desc);
        }
}

void flext_base::AddOutlet(xlettype tp,int mult,const char *desc)
{
    if(UNLIKELY(outcnt+mult >= MAXLETS))
        post("%s - too many outlets",thisName());
    else
        for(int i = 0; i < mult; ++i) {
            xlet &x = outlist[outcnt++];
            x.tp = tp;
            x.Desc(desc);
        }
}

void flext_base::DescInlet(int ix,const char *d)
{
    if(UNLIKELY(ix >= incnt))
        post("%s - inlet %i not found",thisName(),ix);
    else
        inlist[ix].Desc(d);
}

void flext_base::DescOutlet(int ix,const char *d)
{
    if(UNLIKELY(ix >= incnt))
        post("%s - outlet %i not found",thisName(),ix);
    else
        outlist[ix].Desc(d);
}

unsigned long flext_base::XletCode(xlettype tp,...)
{
    unsigned long code = 0;

    va_list marker;
    va_start(marker,tp);
    int cnt = 0;
    xlettype arg = tp;
    for(; arg; ++cnt) {
#ifdef FLEXT_DEBUG
        if(cnt > 9) {
            error("%s - Too many in/outlets defined - truncated to 9",thisName());
            break;          
        }
#endif          

        code = code*10+(int)arg;
        arg = (xlettype)va_arg(marker,int);
    }
    va_end(marker);

    return code;
}

void flext_base::AddInlets(unsigned long code) 
{ 
    for(; code; code /= 10) AddInlet((xlettype)(code%10));
}

void flext_base::AddOutlets(unsigned long code) 
{ 
    for(; code; code /= 10) AddOutlet((xlettype)(code%10));
}

#include "flpopns.h"
