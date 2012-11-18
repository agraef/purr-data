/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file flext.cpp
    \brief Implementation of the flext base class.
*/
 
#include "flext.h"
#include "flinternal.h"
#include "fldsp.h"
#include <cstring>

#include "flpushns.h"

// === flext_base ============================================

const t_symbol *flext_base::curtag = NULL;

flext_base::FLEXT_CLASSDEF(flext_base)()
    : incnt(0),outcnt(0)
    , insigs(0),outsigs(0)
#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
    ,outlets(NULL),inlets(NULL)
#endif
#if FLEXT_SYS == FLEXT_SYS_MAX
    ,indesc(NULL),outdesc(NULL)
#endif
{
    FLEXT_LOG1("%s - flext logging is on",thisName());

    methhead = NULL;
    bindhead = NULL;

    if(HasAttributes()) {
        // initialize when attribute processing is enabled
        attrhead = new ItemCont;
        attrdata = new AttrDataCont;
    }
    else {
        attrhead = NULL;
        attrdata = NULL;
    }
}

/*! This virtual function is called after the object has been created, that is, 
    after the constructor has been processed. 
    It creates the inlets and outlets and the message and attribute lists.
    \note You can override it in your own class, but be sure to call it, 
    \note otherwise no inlets/outlets will be created
    \note All inlet, outlets, method and attribute declarations must be made before a call to Init!
    \remark Creation of inlets/outlets can't be done upon declaration, as Max/MSP needs creation
    \remark in reverse.
*/
bool flext_base::Init()
{
    bool ok = flext_obj::Init();

    if(ok) ok = InitInlets() && InitOutlets();

    if(ok) {
#if FLEXT_SYS == FLEXT_SYS_MAX
		// according to the Max/MSP SDK this should be prior to any inlet creation, BUT
		// that doesn't seem to be true... multiple signal ins and additional inlets don't seem to work then      
		if(NeedDSP()) dsp_setup(thisHdr(),CntInSig()); // signal inlets   
#endif

        if(HasAttributes() && m_holdaargc && m_holdaargv) {
            // initialize creation attributes
            ok = InitAttrib(m_holdaargc,m_holdaargv);
        }
    }

    return ok;
}


/*! This virtual function is called before the destructor.
    We do this because here we can still call virtual methods.
*/
void flext_base::Exit()
{
#if FLEXT_SYS == FLEXT_SYS_MAX
    // according to David Z. one should do that first...
	if(NeedDSP()) dsp_free(thisHdr());
#endif

#if FLEXT_SYS == FLEXT_SYS_PD && !defined(FLEXT_NOATTREDIT)
    // attribute editor window may still be open -> close it
    gfxstub_deleteforkey(thisHdr());
#endif

#ifdef FLEXT_THREADS
    StopThreads();
#endif

    // send remaining pending messages for this object
    QFlush(this);

    // delete message lists
    if(bindhead) delete bindhead;  // ATTENTION: the object must free all memory associated to bindings itself
    if(methhead) delete methhead;
    if(attrhead) delete attrhead;
    if(attrdata) delete attrdata;
    
#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
    if(outlets) delete[] outlets;

    if(inlets) {
        FLEXT_ASSERT(incnt > 1);
        for(int ix = 1; ix < incnt; ++ix)
            if(inlets[ix-1]) {
                // release proxy object
#if FLEXT_SYS == FLEXT_SYS_PD
                pd_free(&inlets[ix-1]->obj.ob_pd);
#elif FLEXT_SYS == FLEXT_SYS_MAX
                freeobject((object *)inlets[ix-1]);
#endif
            }
        delete[] inlets;
    }
#endif

#if FLEXT_SYS == FLEXT_SYS_MAX
    if(indesc) {
        for(int i = 0; i < incnt; ++i) if(indesc[i]) delete[] indesc[i];
        delete[] indesc;
    }
    if(outdesc) {
        for(int i = 0; i < outcnt; ++i) if(outdesc[i]) delete[] outdesc[i];
        delete[] outdesc;
    }
#endif

    flext_obj::Exit();
}


void flext_base::AddMessageMethods(t_class *c,bool dsp,bool dspin)
{
    add_loadbang(c,cb_loadbang);

#if FLEXT_SYS == FLEXT_SYS_PD
    class_addmethod(c,(t_method)cb_click,gensym(const_cast<char *>("click")),A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,A_NULL);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    add_assist(c,cb_assist);
    add_dblclick(c,cb_click);
#endif

    SetProxies(c,dsp);
    StartQueue();
    
    if(dsp) {
#if FLEXT_SYS == FLEXT_SYS_MAX
        add_dsp(c,cb_dsp);
        dsp_initclass();
#elif FLEXT_SYS == FLEXT_SYS_PD
        if(dspin)
            CLASS_MAINSIGNALIN(c,flext_hdr,defsig); // float messages going into the left inlet are converted to signal
        add_dsp(c,cb_dsp);
#else
#error Platform not supported!
#endif
    }
}


/*! Set up proxy classes and basic methods at class creation time
    This ensures that they are processed before the registered flext messages
*/
void flext_base::Setup(t_classid id)
{
    t_class *c = getClass(id);

#if FLEXT_SYS == FLEXT_SYS_MAX
	if(!IsLib(id))
#endif
        AddMessageMethods(c,IsDSP(id),HasDSPIn(id));

    if(HasAttributes(id)) {
        AddMethod(id,0,"getattributes",cb_ListAttrib);
        AddMethod(id,0,"getmethods",cb_ListMethods);

#if FLEXT_SYS == FLEXT_SYS_PD && !defined(FLEXT_NOATTREDIT)
        AddMethod(id,0,"attributedialog",cb_AttrDialog);
#endif
    }

#if FLEXT_SYS == FLEXT_SYS_PD
    SetGfx(id);
#endif
}

void flext_base::cb_loadbang(flext_hdr *c) 
{ 
    Locker lock(c);
    thisObject(c)->CbLoadbang(); 
}   

void flext_base::m_loadbang() {}
void flext_base::CbLoadbang() { m_loadbang(); }

void flext_base::CbClick() {}

#if FLEXT_SYS == FLEXT_SYS_PD
void flext_base::cb_click(flext_hdr *c,t_floatarg xpos,t_floatarg ypos,t_floatarg shift,t_floatarg ctrl,t_floatarg alt)
{
    if(shift) {
        Locker lock(c);
        thisObject(c)->CbClick();
    }
}
#endif

#if FLEXT_SYS == FLEXT_SYS_MAX
void flext_base::cb_click(flext_hdr *c, Point pt, short mods)
{
    Locker lock(c);
    thisObject(c)->CbClick();
}

void flext_base::cb_assist(flext_hdr *c,void * /*b*/,long msg,long arg,char *s) 
{ 
    Locker lock(c);
    flext_base *th = thisObject(c); 

    switch(msg) {
    case 1: //ASSIST_INLET:
        if(arg < th->incnt && th->indesc[arg]) strcpy(s,th->indesc[arg]);
        break;
    case 2: //ASSIST_OUTLET:
        if(arg < th->outcnt) {
            if(th->outdesc[arg]) strcpy(s,th->outdesc[arg]);
        }
        else
            if(arg == th->outcnt && th->HasAttributes()) strcpy(s,"Attributes");
        break;
    }
}
#endif

#if FLEXT_SYS == FLEXT_SYS_MAX
void flext_base::cb_dsp(flext_hdr *c,t_signal **sp,short *count) 
#else
void flext_base::cb_dsp(flext_hdr *c,t_signal **sp) 
#endif
{ 
    Locker lock(c);
    flext_base *bobj = thisObject(c); 
	
#if FLEXT_SYS == FLEXT_SYS_MAX
	// we must extra-check here if it is really a DSP object
	// obviously, for objects that are part of a library, one dsp_initclass enables DSP for all
	if(!bobj->IsDSP()) return;
#endif

	flext_dsp *obj;
#ifdef FLEXT_DEBUG
    obj = dynamic_cast<flext_dsp *>(bobj);
#else
    obj = static_cast<flext_dsp *>(bobj); 
#endif

    FLEXT_ASSERT(obj);
	obj->SetupDsp(sp);
}

bool flext_base::CbIdle() { return 0; }

#include "flpopns.h"
