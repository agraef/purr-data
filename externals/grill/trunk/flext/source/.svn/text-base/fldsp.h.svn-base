/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file fldsp.h
    \brief Declares the flext dsp class
    
*/

#ifndef __FLDSP_H
#define __FLDSP_H

// include the header file declaring the base classes
#include "flext.h"

#include "flpushns.h"

// === flext_dsp ==================================================

class FLEXT_SHARE FLEXT_CLASSDEF(flext_dsp);
typedef class FLEXT_SHARE FLEXT_CLASSDEF(flext_dsp) flext_dsp;


/*! \brief Flext dsp enabled base object
*/
class FLEXT_SHARE FLEXT_CLASSDEF(flext_dsp):
	public flext_base
{
	FLEXT_HEADER_S(FLEXT_CLASSDEF(flext_dsp),flext_base,Setup)
	
	friend class FLEXT_SHARE FLEXT_CLASSDEF(flext_base);

public:

/*!	\defgroup FLEXT_DSP Flext dsp class

	@{ 
*/

/*!	\defgroup FLEXT_C_DSP Basic dsp functionality

	@{ 
*/

	//! returns current sample rate
	float Samplerate() const { return srate; }
	
	//! returns current block (aka vector) size
	int Blocksize() const { return blksz; }
    
	//! returns array of input vectors (CntInSig() vectors)
    t_sample *const *InSig() const { return vecs; }

	//! returns input vector
    t_sample *InSig(int i) const { return InSig()[i]; }

	//! returns array of output vectors (CntOutSig() vectors)
    // \todo cache that returned pointer
    t_sample *const *OutSig() const 
    { 
        int i = CntInSig(); 
        // in PD we have at least one actual dsp in vector
#if FLEXT_SYS == FLEXT_SYS_PD
        return vecs+(i?i:1); 
#elif FLEXT_SYS == FLEXT_SYS_MAX
        return vecs+i; 
#else
#error
#endif
    }

	//! returns output vector
    t_sample *OutSig(int i) const { return OutSig()[i]; }

	//! typedef describing a signal vector
	typedef t_sample *t_signalvec;

//!	@} 

// --- inheritable virtual methods --------------------------------

/*!	\defgroup FLEXT_C_DSP_VIRTUAL Flext virtual dsp functions

	@{ 
*/
	/*! \brief Called on every dsp init.
		\note Don't expect any valid data in the signal vectors!
        flext_dsp::CbDsp should not be called by the derived class

        \return true (default)... use DSP, false, don't use DSP
    */
	virtual bool CbDsp();

	/*! \brief Called with every signal vector - here you do the dsp calculation
        flext_dsp::CbSignal fills all output vectors with silence
    */
	virtual void CbSignal();


    /*! \brief Deprecated method for CbSignal
        \deprecated
		\param n: frames (aka samples) in one signal vector
		\param insigs: array of input vectors  (get number with function CntInSig())
		\param outsigs: array of output vectors  (get number with function CntOutSig())
	*/
	virtual void m_dsp(int n,t_signalvec const *insigs,t_signalvec const *outsigs);

    /*! \brief Deprecated method for CbSignal
        \deprecated
		\param n: frames (aka samples) in one signal vector
		\param insigs: array of input vectors  (get number with function CntInSig())
		\param outsigs: array of output vectors  (get number with function CntOutSig())
	*/
	virtual void m_signal(int n,t_sample *const *insigs,t_sample *const *outsigs);

//!	@} 


/*!	\defgroup FLEXT_C_DSP_INOUT Flext dsp in-/outlet functions
	\note These must be called in the class' constructor

	@{ 
*/
// --- inlet/outlet stuff -----------------------------------------	

	/*! \brief Add signal inlet(s)
		\param m Number of inlets to add
	*/
	void AddInSignal(int m = 1) { AddInlet(xlet_sig,m); }

	/*! \brief Add signal inlet (with description)
		\param desc Description of inlet
	*/
	void AddInSignal(const char *desc) { AddInlet(xlet_sig,1,desc); }

	/*! \brief Add signal outlet(s)
		\param m Number of inlets to add
	*/
	void AddOutSignal(int m = 1) { AddOutlet(xlet_sig,m); }

	/*! \brief Add signal outlet (with description)
		\param desc Description of outlet
	*/
	void AddOutSignal(const char *desc) { AddOutlet(xlet_sig,1,desc); }

//!	@} 

//!	@} 

protected:
	
	FLEXT_CLASSDEF(flext_dsp)();

    virtual void Exit();

private:

	// not static, could be different in different patchers..
	float srate; 
	int blksz;
	t_signalvec *vecs;

	// setup function
	static void Setup(t_classid c);

#if FLEXT_SYS == FLEXT_SYS_PD
	static bool cb_enable(flext_base *c,float &on);
	bool dspon;
#endif

	static inline flext_dsp *thisObject(flext_hdr *c) { return FLEXT_CAST<flext_dsp *>(c->data); } 

	void SetupDsp(t_signal **sp);

	// dsp stuff
	static t_int *dspmeth(t_int *w); 
};

#include "flpopns.h"

#endif
