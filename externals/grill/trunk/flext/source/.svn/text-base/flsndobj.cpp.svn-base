/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#include "flext.h"
#include "flsndobj.h"

#include "flpushns.h"

flext_sndobj::flext_sndobj():
    inobjs(0),outobjs(0),
    inobj(NULL),tmpobj(NULL),outobj(NULL),
    smprt(0),blsz(0)
{}

bool flext_sndobj::Init()
{
    bool ret = flext_dsp::Init();
    inobjs = CntInSig();
    outobjs = CntOutSig();
    return ret;
}

void flext_sndobj::Exit()
{
    ClearObjs();
    flext_dsp::Exit();
}

void flext_sndobj::ClearObjs()
{
    FreeObjs();

    if(inobj) {
        for(int i = 0; i < inobjs; ++i) delete inobj[i]; 
        delete[] inobj; inobj = NULL; 
    }
    if(tmpobj) {
        for(int i = 0; i < inobjs; ++i) delete tmpobj[i];
        delete[] tmpobj; tmpobj = NULL;
    }
    if(outobj) {
        for(int i = 0; i < outobjs; ++i) delete outobj[i];
        delete[] outobj; outobj = NULL; 
    }
}

bool flext_sndobj::CbDsp()
{
    // called on every rebuild of the dsp chain
    
    int i;
    if(Blocksize() != blsz || Samplerate() != smprt) {
        // block size or sample rate has changed... rebuild all objects

        ClearObjs();

        blsz = Blocksize();
        smprt = Samplerate();

        // set up sndobjs for inlets and outlets
        if(inobjs) {
            inobj = new Inlet *[inobjs];
            tmpobj = new SndObj *[inobjs];
            for(i = 0; i < inobjs; ++i) {
                inobj[i] = new Inlet(InSig(i),blsz,smprt);
                tmpobj[i] = new SndObj(NULL,blsz,smprt);
            }
        }
        if(outobjs) {
            outobj = new Outlet *[outobjs];
            for(i = 0; i < outobjs; ++i) outobj[i] = new Outlet(OutSig(i),blsz,smprt);
        }

        if(!NewObjs()) ClearObjs();
    }
    else {
        // assign changed input/output vectors

        for(i = 0; i < inobjs; ++i) inobj[i]->SetBuf(InSig(i));
        for(i = 0; i < outobjs; ++i) outobj[i]->SetBuf(OutSig(i));
    }
    return true;
}

void flext_sndobj::CbSignal()
{
    for(int i = 0; i < inobjs; ++i) *tmpobj[i] << *inobj[i];
    ProcessObjs();
}


flext_sndobj::Inlet::Inlet(const t_sample *b,int vecsz,float sr): 
    SndIO(1,sizeof(t_sample)*8,NULL,vecsz,sr),buf(b) 
{}

short flext_sndobj::Inlet::Read() 
{ 
    if(!m_error) { 
        for(m_vecpos = 0; m_vecpos < m_samples; m_vecpos++)
            m_output[m_vecpos] = buf[m_vecpos];
        return 1; 
    } 
    else return 0; 
}

short flext_sndobj::Inlet::Write() { return 0; }


flext_sndobj::Outlet::Outlet(t_sample *b,int vecsz,float sr): 
    SndIO(1,sizeof(t_sample)*8,NULL,vecsz,sr),buf(b) 
{}

short flext_sndobj::Outlet::Read() { return 0; }

short flext_sndobj::Outlet::Write() 
{ 
    if(!m_error) { 
        if(m_IOobjs[0])
            for(m_vecpos = 0; m_vecpos < m_samples; m_vecpos++)
                buf[m_vecpos] = m_IOobjs[0]->Output(m_vecpos);
        return 1; 
    } 
    else return 0; 
}

#include "flpopns.h"

