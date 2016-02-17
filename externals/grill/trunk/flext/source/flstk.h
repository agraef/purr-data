/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

#ifndef __FLEXT_STK_H
#define __FLEXT_STK_H

#include "flext.h"

// PI is defined in the Max/MSP SDK, but clashes with Stk.h
#ifdef PI
#undef PI
#endif

#include <Stk.h>

#include "flpushns.h"

using stk::Stk;
using stk::StkFloat;
using stk::StkFrames;
    
class FLEXT_SHARE flext_stk:
    public flext_dsp
{ 
    FLEXT_HEADER(flext_stk,flext_dsp)
 
public:
    flext_stk();

    // these have to be overridden in child classes
    virtual bool NewObjs() { return true; }
    virtual void FreeObjs() {}
    virtual void ProcessObjs(int blocksize) {}

protected:
    virtual bool Init();
    virtual void Exit();

    //! STK object for reading from inlet buffer
    class Input:
        public Stk
    {
    public:
		Input(const t_sample *b,int v): 
		    buf(b),vecsz(v),
		    index(v-1)
		{}

        inline StkFloat lastOut() const { return (StkFloat)buf[index]; }

        inline StkFloat tick() 
        { 
            if(++index >= vecsz) index = 0; 
            return lastOut(); 
        }

        StkFloat *tick(StkFloat *vector,unsigned int vectorSize);
        
        inline StkFrames &tick(StkFrames &vector)
        {
            FLEXT_ASSERT(vector.channels() == 1);
            tick(&vector[0],vector.frames());
            return vector;
        }

        inline void SetBuf(const t_sample *b) { buf = b; }

    private:
        const t_sample *buf;
        int vecsz,index;
    };

    //! STK object for writing to outlet buffer
    class Output:
        public Stk
    {
    public:
		Output(t_sample *b,int v): 
		    buf(b),vecsz(v),
		    index(0)
		{}

        inline void tick(StkFloat s) 
        { 
            buf[index] = (t_sample)s; 
            if(++index >= vecsz) index = 0; 
        }

        void tick(const StkFloat *vector,unsigned int vectorSize);
        
        inline void tick(const StkFrames &vector)
        {
            FLEXT_ASSERT(vector.channels() == 1);
            // dirty casting due to bug in STK api... operator[] _should_ return const StkFloat &
            tick(&const_cast<StkFrames &>(vector)[0],vector.frames());
        }

        inline void SetBuf(t_sample *b) { buf = b; }

    private:
        t_sample *buf;
        int vecsz,index;
    };

    Input &Inlet(int ix) { return *inobj[ix]; }
    Output &Outlet(int ix) { return *outobj[ix]; }

private:
    virtual bool CbDsp(); 
    virtual void CbSignal(); 

    void ClearObjs();

    int inobjs,outobjs;
    Input **inobj;
    Output **outobj;

    float smprt;
    int blsz;
};

#include "flpopns.h"

#ifdef FLEXT_INLINE
#   include "flstk.cpp"
#endif

#endif
