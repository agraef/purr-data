/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file fldsp.cpp
    \brief Implementation of the flext dsp base class.
*/
 
#include "flext.h"
#include "flinternal.h"
#include <cstring>

#include "flpushns.h"

// === flext_dsp ==============================================

void flext_dsp::Setup(t_classid id)
{
#if FLEXT_SYS == FLEXT_SYS_PD
//    add_method1(c,cb_enable,"enable",A_FLOAT);
    AddMethod(id,0,MakeSymbol("enable"),&cb_enable);
#endif
}

flext_dsp::FLEXT_CLASSDEF(flext_dsp)()
    : srate(sys_getsr()),blksz(sys_getblksize())
    , vecs(NULL)
#if FLEXT_SYS != FLEXT_SYS_MAX
    , dspon(true)
#endif
{}

void flext_dsp::Exit()
{
    flext_base::Exit();
    
    if(vecs) delete[] vecs;
}


t_int *flext_dsp::dspmeth(t_int *w) 
{ 
    flext_dsp *obj = (flext_dsp *)(size_t)w[1];

#if FLEXT_SYS == FLEXT_SYS_MAX
    if(!obj->thisHdr()->z_disabled)
#else
    if(LIKELY(obj->dspon))
#endif
    {
        flext_base::indsp = true;
        obj->CbSignal(); 
        flext_base::indsp = false;
    }
    return w+2;
}

void flext_dsp::SetupDsp(t_signal **sp) 
{ 
    int i;
    int in = CntInSig();
    int out = CntOutSig();
#if FLEXT_SYS == FLEXT_SYS_PD
    // min. 1 input channel! (CLASS_MAININLET in pd...)
    if(!in) in = 1;
#endif

    // store current dsp parameters
    srate = sys_getsr();   // \TODO need not be stored in each object....
    // overlap = sp[0]->s_sr/srate;  // currently not used/exposed
    blksz = sp[0]->s_n;  // is this guaranteed to be the same as sys_getblksize() ?

    // store in and out signal vectors

    if((in+out) && !vecs)
        vecs = new t_signalvec[in+out];

    for(i = 0; i < in; ++i) 
        vecs[i] = sp[i]->s_vec;
    for(i = 0; i < out; ++i) 
        vecs[in+i] = sp[in+i]->s_vec;

    // with the following call derived classes can do their eventual DSP setup
    if(CbDsp()) {
        // set the DSP function
        dsp_add((t_dspmethod)dspmeth,1,this);  
    }
}

void flext_dsp::m_dsp(int /*n*/,t_signalvec const * /*insigs*/,t_signalvec const * /*outsigs*/) {}

bool flext_dsp::CbDsp() 
{ 
	// invoke legacy method
    m_dsp(Blocksize(),InSig(),OutSig()); 
    return true;
}

// this function will be overridden anyway - the probably useless default is clearing all outputs
void flext_dsp::m_signal(int n,t_sample *const * /*insigs*/,t_sample *const *outs) 
{
    for(int i = 0; i < CntOutSig(); ++i) ZeroSamples(outs[i],n);
}

void flext_dsp::CbSignal() 
{ 
	// invoke legacy method
	m_signal(Blocksize(),InSig(),OutSig()); 
}


#if FLEXT_SYS == FLEXT_SYS_PD
//void flext_dsp::cb_enable(flext_hdr *c,t_float on) { thisObject(c)->dspon = on != 0; }
bool flext_dsp::cb_enable(flext_base *b,float &on) { static_cast<flext_dsp *>(b)->dspon = on != 0; return true; }
#endif

#include "flpopns.h"

