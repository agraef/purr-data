/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3669 $
$LastChangedDate: 2009-03-05 18:34:39 -0500 (Thu, 05 Mar 2009) $
$LastChangedBy: thomas $
*/

/*! \file flbase.cpp
    \brief Implementation of the internal flext base classes.

    \remark This is all derived from GEM by Mark Danks
*/
 
#include "flext.h"
#include "flinternal.h"
#include <cstring>
#include <cctype>
#include <cstdlib>

#if FLEXT_SYS == FLEXT_SYS_PD
#ifdef _MSC_VER
    #pragma warning (push)
    #pragma warning (disable:4091)
#endif
// for canvas_realizedollar (should be non-critical)
#include <g_canvas.h>
#ifdef _MSC_VER
    #pragma warning (pop)
#endif
#endif

#include "flpushns.h"

/////////////////////////////////////////////////////////
//
// flext_obj
//
/////////////////////////////////////////////////////////

flext_hdr *flext_obj::m_holder = NULL;
const t_symbol *flext_obj::m_holdname = NULL;
flext_class *flext_obj::m_holdclass = NULL;
int flext_obj::m_holdaargc = 0;
const t_atom *flext_obj::m_holdaargv = NULL;
//bool flext_obj::process_attributes = false;

bool flext_obj::initing = false;
bool flext_obj::exiting = false;
bool flext_obj::init_ok;

//void flext_obj::ProcessAttributes(bool attr) { process_attributes = attr; }

#if FLEXT_SYS == FLEXT_SYS_MAX
static const t_symbol *sym__shP = NULL;
#endif

/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
flext_obj :: FLEXT_CLASSDEF(flext_obj)()
    : x_obj(m_holder)
    , clss(m_holdclass)
    , m_name(m_holdname)
{
#if FLEXT_SYS == FLEXT_SYS_PD
    m_canvas = canvas_getcurrent();
#elif FLEXT_SYS == FLEXT_SYS_MAX
    m_canvas = (t_patcher *)sym__shP->s_thing;
    x_obj->curinlet = 0;
#endif
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
flext_obj :: ~FLEXT_CLASSDEF(flext_obj)() 
{
    x_obj = NULL;
}

void flext_obj::__setup__(t_classid) 
{ 
#if FLEXT_SYS == FLEXT_SYS_MAX
    sym__shP = MakeSymbol("#P");
#endif
    flext::Setup(); 
}	

bool flext_obj::Init() { return true; }
bool flext_obj::Finalize() { return true; }
void flext_obj::Exit() {}

void flext_obj::DefineHelp(t_classid c,const char *ref,const char *dir,bool addtilde)
{
#if FLEXT_SYS == FLEXT_SYS_PD
    char tmp[256];
    if(dir && *dir) { 
        strcpy(tmp,dir);
		char *last = tmp+strlen(tmp)-1; 
        if(*last != '/') strcat(last,"/"); 
        strcat(last,ref); 
    }
    else 
        strcpy(tmp,ref);
    if(addtilde) strcat(tmp,"~"); 

    ::class_sethelpsymbol(getClass(c),gensym(const_cast<char *>(tmp)));
#else
    // no solution for Max/MSP yet
#endif
}

bool flext_obj::GetParamSym(t_atom &dst,const t_symbol *sym,t_canvas *c)
{
#if FLEXT_SYS == FLEXT_SYS_PD && defined(PD_MINOR_VERSION) && PD_MINOR_VERSION >= 37
    if(!c) c = canvas_getcurrent();

    const char *s = GetString(sym);
    if((s[0] == '$' || s[0] == '#') && isdigit(s[1])) {
        const t_symbol *res;
        // patcher parameter detected... get value!
        if(s[0] != '$') {
            char tmp[MAXPDSTRING];
            strcpy(tmp,s);
            tmp[0] = '$';
            res = canvas_realizedollar(c,const_cast<t_symbol *>(MakeSymbol(tmp)));
        }
        else
            res = canvas_realizedollar(c,const_cast<t_symbol *>(sym));

        // check for number
        const char *c = GetString(res);
        while(*c && (isdigit(*c) || *c == '.')) ++c;

        if(!*c) 
            SetFloat(dst,(float)atof(GetString(res)));
        else
            SetSymbol(dst,res);
        return true;
    }
    else
#else
    #pragma message("Not implemented")
#endif
    SetSymbol(dst,sym);
    return true;
}


#if FLEXT_SYS == FLEXT_SYS_PD
// this declaration is missing in m_pd.h (0.37-0 and -1)
// but it is there in 0.37-2 (but how to tell which micro-version?)
extern "C" 
#ifdef _MSC_VER
__declspec(dllimport)
#endif
void canvas_getargs(int *argcp, t_atom **argvp);
#endif


void flext_obj::GetCanvasArgs(AtomList &args) const
{
#if FLEXT_SYS == FLEXT_SYS_PD
    int argc;
    t_atom *argv;
    canvas_getargs(&argc,&argv);
    args(argc,argv);
#else
    #pragma message("Not implemented")
    args(0);
#endif
}


#if FLEXT_SYS == FLEXT_SYS_MAX 
static short patcher_myvol(t_patcher *x)
{
#ifndef	MM_UNIFIED // Max5 check... we don't know what to do yet
    t_box *w;
    if(x->p_vol)
        return x->p_vol;
    else if((w = (t_box *)x->p_vnewobj) != NULL)
        return patcher_myvol(w->b_patcher);
    else
#endif
        return 0;
}
#endif

void flext_obj::GetCanvasDir(char *buf,size_t bufsz) const
{
#if FLEXT_SYS == FLEXT_SYS_PD
	const char *c = GetString(canvas_getdir(thisCanvas()));
    strncpy(buf,c,bufsz);
#elif FLEXT_SYS == FLEXT_SYS_MAX 
	short path = patcher_myvol(thisCanvas());
    // \TODO dangerous!! no check for path length (got to be long enough... like 1024 chars)
	path_topathname(path,NULL,buf);
#else 
#error Not implemented
#endif
}

#include "flpopns.h"

