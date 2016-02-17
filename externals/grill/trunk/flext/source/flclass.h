/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flclass.h
	\brief User accessible flext base classes
    
*/

#ifndef __FLCLASS_H
#define __FLCLASS_H

// include the header file declaring the base classes
#include "flbase.h"
#include "flsupport.h"
#include "flmap.h"
#include "flinternal.h"

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#ifdef __BORLANDC__
#pragma warn -8008 // Condition is always false
#pragma warn -8057 // Parameter is never used
#pragma warn -8066 // Unreachable code
#endif


#include "flpushns.h"

// === flext_base ==================================================

/*! \brief Flext message only base object

    This is the base class from which typical external objects derive.
    DSP objects should use the flext_dsp class which inherits from flext_base and
    provides the necessary functionality.

    For a valid external object class you would also need FLEXT_HEADER, also if it's only
    a base class without instantiated objects again.
    To make an instance of an object class you would typically use FLEXT_NEW or 
    its companions.

    See the flext_obj class for additional information.
*/


FLEXT_TEMPLATE class FLEXT_SHARE FLEXT_CLASSDEF(flext_base);

typedef class FLEXT_SHARE FLEXT_TEMPINST(FLEXT_CLASSDEF(flext_base)) flext_base;

FLEXT_TEMPLATE
class FLEXT_SHARE FLEXT_CLASSDEF(flext_base): 
	public flext_obj
{
	FLEXT_HEADER_S(FLEXT_CLASSDEF(flext_base),flext_obj,Setup)
	
	friend class FLEXT_SHARE FLEXT_CLASSDEF(flext_obj);

public:

// --- inheritable virtual methods --------------------------------

	/*!	\defgroup FLEXT_C_VIRTUAL Virtual base class functions

		@{ 
	*/

	//! called on patcher load (not on mere object creation!)
	virtual void CbLoadbang();
	virtual void m_loadbang();

	//! called on (double-)click into object box
	virtual void CbClick();

	/*!	\brief Called for every incoming message.
		All method handling is done in there
		\return True if a handler was found and called
	*/
	virtual bool CbMethodHandler(int inlet,const t_symbol *s,int argc,const t_atom *argv);

	/*! \brief Called for every unhandled message (by CbMethodHandler)
	*/
	virtual bool CbMethodResort(int inlet,const t_symbol *s,int argc,const t_atom *argv);
	virtual bool m_method_(int inlet,const t_symbol *s,int argc,const t_atom *argv);

	virtual bool CbIdle();

//!		@} FLEXT_C_VIRTUAL


// --- inlet/outlet stuff -----------------------------------------	

	/*!	\defgroup FLEXT_C_INOUT Flext in-/outlet functions
		\note These must be called in the class' constructor
		\note All (also default) inlets must be defined
		@{ 
	*/

	/*!	\defgroup FLEXT_C_IO_ADD Announce in-/outlet functions
		@{ 
	*/

	// argument m specifies multiple inlet/outlet count
	
//	void AddInDef() { AddInlet(xlet_def,1); }

	/*! \brief Add inlet(s) for anythings
		\remark That's the one to choose for the left-most (first) inlet unless it's a signal inlet.
	*/
	void AddInAnything(int m = 1) { AddInlet(xlet_any,m); } 
	/*! \brief Add inlet(s) for anythings (with description)
		\remark That's the one to choose for the left-most (first) inlet unless it's a signal inlet.
	*/
	void AddInAnything(const char *desc,int m = 1) { AddInlet(xlet_any,m,desc); } 
	//! Add inlet(s) for floats
	void AddInFloat(int m = 1) { AddInlet(xlet_float,m); }
	//! Add inlet(s) for floats (with description)
	void AddInFloat(const char *desc,int m = 1) { AddInlet(xlet_float,m,desc); }
	//! Add inlet(s) for integers
	void AddInInt(int m = 1) { AddInlet(xlet_int,m); }
	//! Add inlet(s) for integers (with description)
	void AddInInt(const char *desc,int m = 1) { AddInlet(xlet_int,m,desc); }
	//! Add inlet(s) for symbols
	void AddInSymbol(int m = 1) { AddInlet(xlet_sym,m); }
	//! Add inlet(s) for symbol (with description)
	void AddInSymbol(const char *desc,int m = 1) { AddInlet(xlet_sym,m,desc); }
	//! Add inlet(s) for bang
	void AddInBang(int m = 1) { AddInlet(xlet_sym,m); }
	//! Add inlet(s) for bangs (with description)
	void AddInBang(const char *desc,int m = 1) { AddInlet(xlet_sym,m,desc); }
	//! Add inlet(s) for lists
	void AddInList(int m = 1) { AddInlet(xlet_list,m); }  
	//! Add inlet(s) for lists (with description)
	void AddInList(const char *desc,int m = 1) { AddInlet(xlet_list,m,desc); }  
	
	//! Add outlet(s) for anythings
	void AddOutAnything(int m = 1) { AddOutlet(xlet_any,m); }
	//! Add outlet(s) for anythings (with description)
	void AddOutAnything(const char *desc,int m = 1) { AddOutlet(xlet_any,m,desc); }
	//! Add outlet(s) for floats
	void AddOutFloat(int m = 1) { AddOutlet(xlet_float,m); }
	//! Add outlet(s) for floats (with description)
	void AddOutFloat(const char *desc,int m = 1) { AddOutlet(xlet_float,m,desc); }
	//! Add outlet(s) for integers
	void AddOutInt(int m = 1) { AddOutlet(xlet_int,m); }
	//! Add outlet(s) for integers (with description)
	void AddOutInt(const char *desc,int m = 1) { AddOutlet(xlet_int,m,desc); }
	//! Add outlet(s) for symbols
	void AddOutSymbol(int m = 1) { AddOutlet(xlet_sym,m); }
	//! Add outlet(s) for symbols (with description)
	void AddOutSymbol(const char *desc,int m = 1) { AddOutlet(xlet_sym,m,desc); }
	//! Add outlet(s) for bangs
	void AddOutBang(int m = 1) { AddOutlet(xlet_sym,m); }
	//! Add outlet(s) for bangs (with description)
	void AddOutBang(const char *desc,int m = 1) { AddOutlet(xlet_sym,m,desc); }
	//! Add outlet(s) for lists
	void AddOutList(int m = 1) { AddOutlet(xlet_list,m); }
	//! Add outlet(s) for lists (with description)
	void AddOutList(const char *desc,int m = 1) { AddOutlet(xlet_list,m,desc); }

	//! \deprecated inlets and outlets are now set up automatically
	bool SetupInOut() { return true; }

	//!	@} FLEXT_C_IO_ADD 

	/*!	\defgroup FLEXT_C_IO_MISC Miscellanous in-/outlet functionality
		@{ 
	*/

	//! Get number of inlets
	int CntIn() const { return incnt; }
	//! Get number of outlets
	int CntOut() const { return outcnt; }
	//! Get number of signal inlets
	int CntInSig() const { return insigs; }
	//! Get number of signal outlets
	int CntOutSig() const { return outsigs; }


	//! Retrieve currently processed message tag (NULL if no message processing)
	static const t_symbol *thisTag() { return curtag; }

	class outlet;

	//! Get pointer to outlet (not in the constructor!)
	outlet *GetOut(int ix) const { return outlets[ix]; }

	int GetOutAttr() const { return HasAttributes()?CntOut():0; }

	//! @} FLEXT_C_IO_MISC

	/*!	\defgroup FLEXT_C_IO_OUT Output data to inlets/outlets
		@{ 
	*/

	// output messages 

	//! Output bang (index n starts with 0)
	void ToOutBang(int n) const;

	//! Output float (index n starts with 0)
	void ToOutFloat(int n,float f) const;

	//! Output integer (index n starts with 0)
	void ToOutInt(int n,int f) const;
	
	//! Output boolean (index n starts with 0)
    void ToOutBool(int n,bool f) const { ToOutInt(n,f?1:0); }

	//! Output double (index n starts with 0)
	void ToOutDouble(int n,double d) const { t_atom dbl[2]; ToOutList(n,2,SetDouble(dbl,d)); }

	//! Output symbol (index n starts with 0)
	void ToOutSymbol(int n,const t_symbol *s) const;
	//! Output string aka symbol (index n starts with 0)
	void ToOutString(int n,const char *s) const { ToOutSymbol(n,MakeSymbol(s)); }

	//! Output atom (index n starts with 0)
	void ToOutAtom(int n,const t_atom &at) const; 

	//! Output list (index n starts with 0)
	void ToOutList(int n,int argc,const t_atom *argv) const;
	//! Output list (index n starts with 0)
	void ToOutList(int n,const AtomList &list) const { ToOutList(n,list.Count(),list.Atoms()); }
	
	//! Output anything (index n starts with 0)
	void ToOutAnything(int n,const t_symbol *s,int argc,const t_atom *argv) const;
	//! Output anything (index n starts with 0)
	void ToOutAnything(int n,const AtomAnything &any) const { ToOutAnything(n,any.Header(),any.Count(),any.Atoms()); }
	//! Output anything (index n starts with 0)
	void ToOutAnything(int n,const t_symbol *s,const AtomList &list) const { ToOutAnything(n,s,list.Count(),list.Atoms()); }
	
	//! @} FLEXT_C_IO_OUT

	/*!	\defgroup FLEXT_C_IO_QUEUE Low-priority output of data to inlets/outlets
		@{ 
	*/

	//! Output low priority bang (index n starts with 0)
	void ToQueueBang(int n) const;

	//! Output low priority float (index n starts with 0)
	void ToQueueFloat(int n,float f) const;

	//! Output low priority integer (index n starts with 0)
	void ToQueueInt(int n,int f) const;

	//! Output low priority boolean (index n starts with 0)
	void ToQueueBool(int n,bool f) const { ToQueueInt(n,f?1:0); }

	//! Output double (index n starts with 0)
	void ToQueueDouble(int n,double d) const { t_atom dbl[2]; ToQueueList(n,2,SetDouble(dbl,d)); }

	//! Output low priority symbol (index n starts with 0)
	void ToQueueSymbol(int n,const t_symbol *s) const;
	//! Output low priority string aka symbol (to appointed outlet)
	void ToQueueString(int n,const char *s) const { ToQueueSymbol(n,MakeSymbol(s)); }

	//! Output low priority atom (index n starts with 0)
	void ToQueueAtom(int n,const t_atom &at) const; 

	//! Output low priority list (index n starts with 0)
	void ToQueueList(int n,int argc,const t_atom *argv) const; 
	//! Output low priority list (index n starts with 0)
	void ToQueueList(int n,const AtomList &list) const  { ToQueueList(n,list.Count(),list.Atoms()); }

	//! Output low priority anything (index n starts with 0)
	void ToQueueAnything(int n,const t_symbol *s,int argc,const t_atom *argv)  const;
	//! Output low priority anything (index n starts with 0)
	void ToQueueAnything(int n,const AtomAnything &any) const  { ToQueueAnything(n,any.Header(),any.Count(),any.Atoms()); }

    //!	@} FLEXT_C_IO_QUEUE


	/*!	\defgroup FLEXT_C_IO_SELF Output of data to inlets/outlets of this object
		@{ 
	*/

    //! Send bang to self (inlet n)
    void ToSelfBang(int n) const { ToQueueBang(-1-n); }

	//! Send float to self (inlet n)
    void ToSelfFloat(int n,float f) const { ToQueueFloat(-1-n,f); }

	//! Send integer to self (inlet n)
    void ToSelfInt(int n,int f) const { ToQueueInt(-1-n,f); }

	//! Send boolean to self (inlet n)
	void ToSelfBool(int n,bool f) const { ToSelfInt(n,f?1:0); }

	//! Send double to self (index n starts with 0)
	void ToSelfDouble(int n,double d) const { t_atom dbl[2]; ToSelfList(n,2,SetDouble(dbl,d)); }

	//! Send symbol to self (inlet n)
    void ToSelfSymbol(int n,const t_symbol *s) const { ToQueueSymbol(-1-n,s); }
	//! Send string aka symbol to self (inlet 0)
	void ToSelfString(int n,const char *s) const { ToSelfSymbol(n,MakeSymbol(s)); }

	//! Output atom (index n starts with 0)
    void ToSelfAtom(int n,const t_atom &at) const { ToQueueAtom(-1-n,at); }

	//! Send list to self (inlet n)
    void ToSelfList(int n,int argc,const t_atom *argv) const { ToQueueList(-1-n,argc,argv); }
	//! Send list to self (inlet n)
	void ToSelfList(int n,const AtomList &list) const  { ToSelfList(n,list.Count(),list.Atoms()); }

	//! Send anything to self (inlet n)
    void ToSelfAnything(int n,const t_symbol *s,int argc,const t_atom *argv) const { ToQueueAnything(-1-n,s,argc,argv); }
	//! Send anything to self (inlet n)
	void ToSelfAnything(int n,const AtomAnything &any) const { ToSelfAnything(n,any.Header(),any.Count(),any.Atoms()); }

    //!	@} FLEXT_C_IO_SELF


	/*!	\defgroup FLEXT_C_IO_MESSAGEBUNDLE Output of data via message bundles

        These are used to assure the sending of several messages from a second thread to the same logical time

		@{ 
	*/

	//! Output bang (index n starts with 0)
	void MsgAddBang(MsgBundle *mb,int n) const;

	//! Output float (index n starts with 0)
	void MsgAddFloat(MsgBundle *mb,int n,float f) const;

	//! Output integer (index n starts with 0)
	void MsgAddInt(MsgBundle *mb,int n,int f) const;

	//! Output boolean (index n starts with 0)
	void MsgAddBool(MsgBundle *mb,int n,bool f) const { MsgAddInt(mb,n,f?1:0); }

	//! Output double (index n starts with 0)
	void MsgAddDouble(MsgBundle *mb,int n,double d) const { t_atom dbl[2]; MsgAddList(mb,n,2,SetDouble(dbl,d)); }

	//! Output symbol (index n starts with 0)
	void MsgAddSymbol(MsgBundle *mb,int n,const t_symbol *s) const;
	//! Output string aka symbol (to appointed outlet)
	void MsgAddString(MsgBundle *mb,int n,const char *s) const { MsgAddSymbol(mb,n,MakeSymbol(s)); }

	//! Output atom (index n starts with 0)
	void MsgAddAtom(MsgBundle *mb,int n,const t_atom &at) const;

	//! Output list (index n starts with 0)
	void MsgAddList(MsgBundle *mb,int n,int argc,const t_atom *argv) const;

	//! Output list (index n starts with 0)
	void MsgAddList(MsgBundle *mb,int n,const AtomList &list) const { MsgAddList(mb,n,list.Count(),list.Atoms()); }

	//! Output anything (index n starts with 0)
	void MsgAddAnything(MsgBundle *mb,int n,const t_symbol *s,int argc,const t_atom *argv) const;
	//! Output anything (index n starts with 0)
	void MsgAddAnything(MsgBundle *mb,int n,const AtomAnything &any) const { MsgAddAnything(mb,n,any.Header(),any.Count(),any.Atoms()); }

    void MsgSelfBang(MsgBundle *mb,int n) const { MsgAddBang(mb,-1-n); }

	//! Send float to self (inlet n)
    void MsgSelfFloat(MsgBundle *mb,int n,float f) const { MsgAddFloat(mb,-1-n,f); }

	//! Send integer to self (inlet n)
    void MsgSelfInt(MsgBundle *mb,int n,int f) const { MsgAddInt(mb,-1-n,f); }

	//! Send boolean to self (inlet n)
	void MsgSelfBool(MsgBundle *mb,int n,bool f) const { MsgSelfInt(mb,n,f?1:0); }

	//! Output double (index n starts with 0)
	void MsgSelfDouble(MsgBundle *mb,int n,double d) const { t_atom dbl[2]; MsgSelfList(mb,n,2,SetDouble(dbl,d)); }

	//! Send symbol to self (inlet n)
    void MsgSelfSymbol(MsgBundle *mb,int n,const t_symbol *s) const { MsgAddSymbol(mb,-1-n,s); }
	//! Send string aka symbol to self (inlet 0)
	void MsgSelfString(MsgBundle *mb,int n,const char *s) const { MsgSelfSymbol(mb,n,MakeSymbol(s)); }

	//! Output atom (index n starts with 0)
    void MsgSelfAtom(MsgBundle *mb,int n,const t_atom &at) const { MsgAddAtom(mb,-1-n,at); }

	//! Send list to self (inlet n)
    void MsgSelfList(MsgBundle *mb,int n,int argc,const t_atom *argv) const { MsgAddList(mb,-1-n,argc,argv); }
	//! Send list to self (inlet n)
	void MsgSelfList(MsgBundle *mb,int n,const AtomList &list) const { MsgSelfList(mb,n,list.Count(),list.Atoms()); }

	//! Send anything to self (inlet n)
    void MsgSelfAnything(MsgBundle *mb,int n,const t_symbol *s,int argc,const t_atom *argv) const { MsgAddAnything(mb,-1-n,s,argc,argv); }
	//! Send anything to self (inlet n)
	void MsgSelfAnything(MsgBundle *mb,int n,const AtomAnything &any) const { MsgSelfAnything(mb,n,any.Header(),any.Count(),any.Atoms()); }

    //! @} FLEXT_C_IO_MESSAGEBUNDLE

//!	@} FLEXT_C_INOUT


// --- message handling -------------------------------------------

	enum metharg {
		a_null = 0,
		a_float,a_int,a_bool,
		a_symbol,a_pointer,
		a_list,a_any, // (t_symbol *) / int / t_atom *
		a_LIST,a_ANY // AtomList, AtomAnything
	};

	typedef bool (*methfun)(flext_base *c);

	/*!	\defgroup FLEXT_C_ADDMETHOD Method handling (object scope)
		\internal
		@{ 
	*/

    void AddMethodDef(int inlet,const t_symbol *tag = NULL) { ThMeths()->Add(new MethItem,tag,inlet); }
    void AddMethodDef(int inlet,const char *tag = NULL) { AddMethodDef(inlet,MakeSymbol(tag)); }

	void AddMethod(int inlet,bool (*m)(flext_base *)) { AddMethod(ThMeths(),inlet,sym_bang,(methfun)m,a_null); }
	void AddMethod(int inlet,bool (*m)(flext_base *,int,t_atom *)) { AddMethod(ThMeths(),inlet,sym_list,(methfun)m,a_list,a_null); }
	void AddMethod(int inlet,bool (*m)(flext_base *,int,const t_atom *)) { AddMethod(ThMeths(),inlet,sym_list,(methfun)m,a_list,a_null); }
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_null); }  // pure method
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,bool (*m)(flext_base *,t_symbol *,int,t_atom *)) { AddMethod(ThMeths(),inlet,sym_anything,(methfun)m,a_any,a_null); } // anything
	void AddMethod(int inlet,bool (*m)(flext_base *,const t_symbol *,int,const t_atom *)) { AddMethod(ThMeths(),inlet,sym_anything,(methfun)m,a_any,a_null); } // anything
	void AddMethod(int inlet,bool (*m)(flext_base *,t_symbol *&)) { AddMethod(ThMeths(),inlet,sym_symbol,(methfun)m,a_symbol,a_null); } // single symbol
	void AddMethod(int inlet,bool (*m)(flext_base *,const t_symbol *&)) { AddMethod(ThMeths(),inlet,sym_symbol,(methfun)m,a_symbol,a_null); } // single symbol
	void AddMethod(int inlet,bool (*m)(flext_base *,float &)) { AddMethod(ThMeths(),inlet,sym_float,(methfun)m,a_float,a_null); }  // single float
	void AddMethod(int inlet,bool (*m)(flext_base *,float &,float &)) { AddMethod(ThMeths(),inlet,sym_list,(methfun)m,a_float,a_float,a_null); } // list of 2 floats
	void AddMethod(int inlet,bool (*m)(flext_base *,float &,float &,float &)) { AddMethod(ThMeths(),inlet,sym_list,(methfun)m,a_float,a_float,a_float,a_null); } // list of 3 floats
#if FLEXT_SYS == FLEXT_SYS_PD
	void AddMethod(int inlet,bool (*m)(flext_base *,int &)) { AddMethod(ThMeths(),inlet,sym_float,(methfun)m,a_int,a_null); }  // single float
#else
	void AddMethod(int inlet,bool (*m)(flext_base *,int &)) { AddMethod(ThMeths(),inlet,sym_int,(methfun)m,a_int,a_null); }  // single float
#endif
	void AddMethod(int inlet,bool (*m)(flext_base *,int &,int &)) { AddMethod(ThMeths(),inlet,sym_list,(methfun)m,a_int,a_int,a_null); } // list of 2 floats
	void AddMethod(int inlet,bool (*m)(flext_base *,int &,int &,int &)) { AddMethod(ThMeths(),inlet,sym_list,(methfun)m,a_int,a_int,a_int,a_null); } // list of 3 floats
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,int,t_atom *)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_list,a_null); } // method+gimme
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,int,const t_atom *)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_list,a_null); } // method+gimme
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,t_symbol *,int,t_atom *)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_any,a_null); } // method+gimme 
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,const t_symbol *,int,const t_atom *)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_any,a_null); } // method+gimme 
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,t_symbol *&)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_symbol,a_null); } // method+symbol
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,const t_symbol *&)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_symbol,a_null); } // method+symbol
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,float &)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_float,a_null); }  // method+float
	void AddMethod(int inlet,const t_symbol *tag,bool (*m)(flext_base *,int &)) { AddMethod(ThMeths(),inlet,tag,(methfun)m,a_int,a_null); } // method+int
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,int,t_atom *)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,int,const t_atom *)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,t_symbol *,int,t_atom *)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,const t_symbol *,int,const t_atom *)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,t_symbol *&)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,const t_symbol *&)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,float &)) { AddMethod(inlet,MakeSymbol(tag),m); }
	void AddMethod(int inlet,const char *tag,bool (*m)(flext_base *,int &)) { AddMethod(inlet,MakeSymbol(tag),m); }

	// ¥schedule call of the CbIdle method during the next idle cycle
	void AddIdle();

	//! Set Max/MSP style of distributing list elements over (message) inlets
	static void SetDist(t_classid c,bool d = true);
    //! Query whether lists are distributed
	bool DoDist() const;


//!		@} FLEXT_C_ADDMETHOD

	/*!	\defgroup FLEXT_C_CADDMETHOD Method handling (class scope)
		\internal
		@{ 
	*/

	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *)) { AddMethod(ClMeths(c),inlet,sym_bang,(methfun)m,a_null); }
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,int,t_atom *)) { AddMethod(ClMeths(c),inlet,sym_list,(methfun)m,a_list,a_null); }
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,int,const t_atom *)) { AddMethod(ClMeths(c),inlet,sym_list,(methfun)m,a_list,a_null); }
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_null); }  // pure method
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,t_symbol *,int,t_atom *)) { AddMethod(ClMeths(c),inlet,sym_anything,(methfun)m,a_any,a_null); } // anything
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,const t_symbol *,int,const t_atom *)) { AddMethod(ClMeths(c),inlet,sym_anything,(methfun)m,a_any,a_null); } // anything
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,t_symbol *&)) { AddMethod(ClMeths(c),inlet,sym_symbol,(methfun)m,a_symbol,a_null); } // single symbol
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,const t_symbol *&)) { AddMethod(ClMeths(c),inlet,sym_symbol,(methfun)m,a_symbol,a_null); } // single symbol
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,float &)) { AddMethod(ClMeths(c),inlet,sym_float,(methfun)m,a_float,a_null); }  // single float
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,float &,float &)) { AddMethod(ClMeths(c),inlet,sym_list,(methfun)m,a_float,a_float,a_null); } // list of 2 floats
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,float &,float &,float &)) { AddMethod(ClMeths(c),inlet,sym_list,(methfun)m,a_float,a_float,a_float,a_null); } // list of 3 floats
#if FLEXT_SYS == FLEXT_SYS_PD
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,int &)) { AddMethod(ClMeths(c),inlet,sym_float,(methfun)m,a_int,a_null); }  // single integer
#else
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,int &)) { AddMethod(ClMeths(c),inlet,sym_int,(methfun)m,a_int,a_null); }  // single integer
#endif
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,int &,int &)) { AddMethod(ClMeths(c),inlet,sym_list,(methfun)m,a_int,a_int,a_null); } // list of 2 floats
	static void AddMethod(t_classid c,int inlet,bool (*m)(flext_base *,int &,int &,int &)) { AddMethod(ClMeths(c),inlet,sym_list,(methfun)m,a_int,a_int,a_int,a_null); } // list of 3 floats
    static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,int,t_atom *)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_list,a_null); } // method+gimme
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,int,const t_atom *)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_list,a_null); } // method+gimme
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,t_symbol *,int,t_atom *)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_any,a_null); } // method+gimme 
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,const t_symbol *,int,const t_atom *)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_any,a_null); } // method+gimme 
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,t_symbol *&)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_symbol,a_null); } // method+symbol
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,const t_symbol *&)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_symbol,a_null); } // method+symbol
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,float &)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_float,a_null); }  // method+float
	static void AddMethod(t_classid c,int inlet,const t_symbol *tag,bool (*m)(flext_base *,int &)) { AddMethod(ClMeths(c),inlet,tag,(methfun)m,a_int,a_null); } // method+int
    static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,int,t_atom *)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,int,const t_atom *)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,t_symbol *,int,t_atom *)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,const t_symbol *,int,const t_atom *)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,t_symbol *&)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,const t_symbol *&)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,float &)) { AddMethod(c,inlet,MakeSymbol(tag),m); }
	static void AddMethod(t_classid c,int inlet,const char *tag,bool (*m)(flext_base *,int &)) { AddMethod(c,inlet,MakeSymbol(tag),m); }

	// ¥schedule call of the given idlefun during the next idle cycle
	static void AddIdle(bool (*idlefun)(int argc,const t_atom *argv),int argc,const t_atom *argv);

//!		@} FLEXT_C_CADDMETHOD

// --- bind/unbind ---------------------------------------

	/*!	\defgroup FLEXT_C_BIND Methods for binding a flext class to a symbol

		@{ 
	*/

	//! Bind object to a symbol
	bool Bind(const t_symbol *sym);
	//! Unbind object from a symbol
	bool Unbind(const t_symbol *sym);

	//! Bind object to a symbol (as string)
	bool Bind(const char *sym) { return Bind(MakeSymbol(sym)); }  
	//! Unbind object from a symbol (as string)
	bool Unbind(const char *sym) { return Unbind(MakeSymbol(sym)); }  

    /*! \brief Bind a method to a symbol
        \param sym Symbol to bind to
        \param meth Function to bind
        \param data User data that is passed to the function
        \return true on success
    */
	bool BindMethod(const t_symbol *sym,bool (*meth)(flext_base *obj,t_symbol *sym,int argc,t_atom *argv,void *data),void *data = NULL);
    /*! \brief Unbind a method from a symbol
        \param sym Symbol to unbind from (if NULL... unbind all functions)
        \param meth Method to unbind (if NULL ... unbind all functions bound to symbol)
        \param data returns data pointer specified with BindMethod
        \return true on success
    */
	bool UnbindMethod(const t_symbol *sym,bool (*meth)(flext_base *obj,t_symbol *sym,int argc,t_atom *argv,void *data) = NULL,void **data = NULL);
    /*! \brief Get data of bound method of a symbol
        \param sym Symbol to bind to
        \param meth Function to bind
        \param data Reference to returned user data
        \return true on success (symbol/method combination was found)
    */
	bool GetBoundMethod(const t_symbol *sym,bool (*meth)(flext_base *obj,t_symbol *sym,int argc,t_atom *argv,void *data),void *&data);

    //! \brief Bind a method to a symbol (as string)
    bool BindMethod(const char *sym,bool (*meth)(flext_base *obj,t_symbol *sym,int argc,t_atom *argv,void *data),void *data = NULL) { return BindMethod(MakeSymbol(sym),meth,data); }
    //! \brief Unbind a method from a symbol (as string)
    bool UnbindMethod(const char *sym,bool (*meth)(flext_base *obj,t_symbol *sym,int argc,t_atom *argv,void *data) = NULL,void **data = NULL) { return UnbindMethod(MakeSymbol(sym),meth,data); }
    //! \brief Get data of bound method of a symbol (as string)
    bool GetBoundMethod(const char *sym,bool (*meth)(flext_base *obj,t_symbol *sym,int argc,t_atom *argv,void *data),void *&data) { return GetBoundMethod(MakeSymbol(sym),meth,data); }

	/*! Unbind all symbol bindings
		\note Memory associated to data pointers passed by BindMethod will not be freed!
	*/
	bool UnbindAll();

//!		@} FLEXT_C_BIND

	// --- thread stuff -----------------------------------------------

#ifdef FLEXT_THREADS
	/*!	\defgroup FLEXT_C_THREAD Thread handling 

		@{ 
	*/

	//! Start a thread for this object
	bool StartThread(void (*meth)(thr_params *p),thr_params *p,const char * = NULL) { p->cl = this; return flext::LaunchThread(meth,p); }

	//! Terminate all threads of this object
	bool StopThreads();
#endif // FLEXT_THREADS

//!		@}  FLEXT_C_THREAD

// xxx internal stuff xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

public: // needed by VC++ 6

    enum xlettype {
	    xlet_none = 0,
	    xlet_float,xlet_int,xlet_sym,xlet_list,xlet_any,
	    xlet_LIST,xlet_ANY, // use AtomList and AtomAnything
	    xlet_sig
    };

protected:

	FLEXT_CLASSDEF(flext_base)();

	/*! \brief Set up inlets and outlets, method and attribute lists
	*/
	virtual bool Init();
	
	/*! \brief Deallocate all kinds of stuff
	*/
	virtual void Exit();
	

	/*!	\defgroup FLEXT_C_ATTR Attribute handling methods (object scope)
		@{ 
	*/

	void AddAttrib(const t_symbol *attr,bool (*get)(flext_base *,float &),bool (*set)(flext_base *,float &)) { AddAttrib(attr,a_float,(methfun)get,(methfun)set); }
	void AddAttrib(const t_symbol *attr,bool (*get)(flext_base *,int &),bool (*set)(flext_base *,int &)) { AddAttrib(attr,a_int,(methfun)get,(methfun)set); }
	void AddAttrib(const t_symbol *attr,bool (*get)(flext_base *,bool &),bool (*set)(flext_base *,bool &)) { AddAttrib(attr,a_bool,(methfun)get,(methfun)set); }
	void AddAttrib(const t_symbol *attr,bool (*get)(flext_base *,const t_symbol *&),bool (*set)(flext_base *,const t_symbol *&)) { AddAttrib(attr,a_symbol,(methfun)get,(methfun)set); }
	void AddAttrib(const t_symbol *attr,bool (*get)(flext_base *,t_symptr &),bool (*set)(flext_base *,t_symptr &)) { AddAttrib(attr,a_symbol,(methfun)get,(methfun)set); }
	void AddAttrib(const t_symbol *attr,bool (*get)(flext_base *,AtomList *&),bool (*set)(flext_base *,AtomList *&)) { AddAttrib(attr,a_LIST,(methfun)get,(methfun)set); }
	void AddAttrib(const t_symbol *attr,bool (*get)(flext_base *,AtomAnything *&),bool (*set)(flext_base *,AtomAnything *&)) { AddAttrib(attr,a_ANY,(methfun)get,(methfun)set); }
	void AddAttrib(const char *attr,bool (*get)(flext_base *,float &),bool (*set)(flext_base *,float &)) { AddAttrib(MakeSymbol(attr),get,set); }
	void AddAttrib(const char *attr,bool (*get)(flext_base *,int &),bool (*set)(flext_base *,int &)) { AddAttrib(MakeSymbol(attr),get,set); }
	void AddAttrib(const char *attr,bool (*get)(flext_base *,bool &),bool (*set)(flext_base *,bool &)) { AddAttrib(MakeSymbol(attr),get,set); }
	void AddAttrib(const char *attr,bool (*get)(flext_base *,const t_symbol *&),bool (*set)(flext_base *,const t_symbol *&)) { AddAttrib(MakeSymbol(attr),get,set); }
	void AddAttrib(const char *attr,bool (*get)(flext_base *,t_symptr &),bool (*set)(flext_base *,t_symptr &)) { AddAttrib(MakeSymbol(attr),get,set); }
	void AddAttrib(const char *attr,bool (*get)(flext_base *,AtomList *&),bool (*set)(flext_base *,AtomList *&)) { AddAttrib(MakeSymbol(attr),get,set); }
	void AddAttrib(const char *attr,bool (*get)(flext_base *,AtomAnything *&),bool (*set)(flext_base *,AtomAnything *&)) { AddAttrib(MakeSymbol(attr),get,set); }

//!		@} FLEXT_C_ATTR

	/*!	\defgroup FLEXT_C_CATTR Attribute handling methods (class scope)
		@{ 
	*/

	static void AddAttrib(t_classid c,const t_symbol *attr,bool (*get)(flext_base *,float &),bool (*set)(flext_base *,float &)) { AddAttrib(c,attr,a_float,(methfun)get,(methfun)set); }
	static void AddAttrib(t_classid c,const t_symbol *attr,bool (*get)(flext_base *,int &),bool (*set)(flext_base *,int &)) { AddAttrib(c,attr,a_int,(methfun)get,(methfun)set); }
	static void AddAttrib(t_classid c,const t_symbol *attr,bool (*get)(flext_base *,bool &),bool (*set)(flext_base *,bool &)) { AddAttrib(c,attr,a_bool,(methfun)get,(methfun)set); }
	static void AddAttrib(t_classid c,const t_symbol *attr,bool (*get)(flext_base *,const t_symbol *&),bool (*set)(flext_base *,const t_symbol *&)) { AddAttrib(c,attr,a_symbol,(methfun)get,(methfun)set); }
	static void AddAttrib(t_classid c,const t_symbol *attr,bool (*get)(flext_base *,t_symptr &),bool (*set)(flext_base *,t_symptr &)) { AddAttrib(c,attr,a_symbol,(methfun)get,(methfun)set); }
	static void AddAttrib(t_classid c,const t_symbol *attr,bool (*get)(flext_base *,AtomList *&),bool (*set)(flext_base *,AtomList *&)) { AddAttrib(c,attr,a_LIST,(methfun)get,(methfun)set); }
	static void AddAttrib(t_classid c,const t_symbol *attr,bool (*get)(flext_base *,AtomAnything *&),bool (*set)(flext_base *,AtomAnything *&)) { AddAttrib(c,attr,a_ANY,(methfun)get,(methfun)set); }
	static void AddAttrib(t_classid c,const char *attr,bool (*get)(flext_base *,float &),bool (*set)(flext_base *,float &)) { AddAttrib(c,MakeSymbol(attr),get,set); }
	static void AddAttrib(t_classid c,const char *attr,bool (*get)(flext_base *,int &),bool (*set)(flext_base *,int &)) { AddAttrib(c,MakeSymbol(attr),get,set); }
	static void AddAttrib(t_classid c,const char *attr,bool (*get)(flext_base *,bool &),bool (*set)(flext_base *,bool &)) { AddAttrib(c,MakeSymbol(attr),get,set); }
	static void AddAttrib(t_classid c,const char *attr,bool (*get)(flext_base *,const t_symbol *&),bool (*set)(flext_base *,const t_symbol *&)) { AddAttrib(c,MakeSymbol(attr),get,set); }
	static void AddAttrib(t_classid c,const char *attr,bool (*get)(flext_base *,t_symptr &),bool (*set)(flext_base *,t_symptr &)) { AddAttrib(c,MakeSymbol(attr),get,set); }
	static void AddAttrib(t_classid c,const char *attr,bool (*get)(flext_base *,AtomList *&),bool (*set)(flext_base *,AtomList *&)) { AddAttrib(c,MakeSymbol(attr),get,set); }
	static void AddAttrib(t_classid c,const char *attr,bool (*get)(flext_base *,AtomAnything *&),bool (*set)(flext_base *,AtomAnything *&)) { AddAttrib(c,MakeSymbol(attr),get,set); }

//!		@} FLEXT_C_CATTR

	//! Dump an attribute to the attribute outlet
	bool DumpAttrib(const t_symbol *attr) const;
    //! Dump an attribute to the attribute outlet
	bool DumpAttrib(const char *attr) const { return DumpAttrib(MakeSymbol(attr)); }

    // check for attribute symbol @
	static int CheckAttrib(int argc,const t_atom *argv);
    // check for attribute symbol @
    static int CheckAttrib(const AtomList &args,int offset = 0) { return CheckAttrib(args.Count()-offset,args.Atoms()+offset)+offset; }

	//! List attributes
	bool ListAttrib() const;
	//! List attributes
	void ListAttrib(AtomList &a) const;
	//! Get an attribute value
	bool GetAttrib(const t_symbol *s,AtomList &a) const;
	//! Set an attribute value
	bool SetAttrib(const t_symbol *s,int argc,const t_atom *argv);
	//! Set an attribute value
	bool SetAttrib(const t_symbol *s,const AtomList &a) { return SetAttrib(s,a.Count(),a.Atoms()); }

	// get and set the attribute
	bool BangAttrib(const t_symbol *a);
	// get and set the attribute
	bool BangAttrib(const char *a) { return BangAttrib(MakeSymbol(a)); }
	// get and set all (both get- and settables)
	bool BangAttribAll();
	// show/hide the attribute
	bool ShowAttrib(const t_symbol *a,bool show) const;
	// show/hide the attribute
	bool ShowAttrib(const char *a,bool show) { return ShowAttrib(MakeSymbol(a),show); }

	//! List methods
	void ListMethods(AtomList &a,int inlet = 0) const;

/*!		\addtogroup FLEXT_C_INOUT 
		@{ 
*/

	//! \brief get a code for a list of inlets or outlets
	unsigned long XletCode(xlettype tp = xlet_none,...); // end list with 0 (= tp_none) !!

	/*! \brief Add some inlets by a special code representing the types
		\remark use XletCode function to get code value
	*/
	void AddInlets(unsigned long code); 

	//! \brief Add one or more inlet(s)
	void AddInlet(xlettype tp,int mult = 1,const char *desc = NULL);

	/*! \brief Add some inlets by a special code representing the types
		\remark use XletCode function to get code value
	*/
	void AddOutlets(unsigned long code); 

	//! \brief Add one or more outlet(s)
	void AddOutlet(xlettype tp,int mult = 1,const char *desc = NULL);

	//! \brief Set the description of an indexed inlet
	void DescInlet(int ix,const char *desc);

	//! \brief Set the description of an indexed outlet
	void DescOutlet(int ix,const char *desc);

//!		@} FLEXT_C_INOUT


// method handling

	public:

	class AttrItem;

    class Item
    {
	public:
        Item(AttrItem *a): attr(a),nxt(NULL) {}
        virtual ~Item();
		
		bool IsAttr() const { return attr != NULL; }

		AttrItem *attr;
		Item *nxt;
	};

    typedef TablePtrMap<const t_symbol *,Item *,8> TablePtrMapDef;
    
	class ItemSet
        :public TablePtrMapDef
    {
    public:
        virtual ~ItemSet();
        virtual void clear();
    };

    /*! This class holds hashed item entries
		\note not thread-safe!
	*/
    class FLEXT_SHARE ItemCont
    {
	public:
        ItemCont();
		~ItemCont();

		int Min() const { return -1; }
		int Max() const { return size-2; }

        bool Contained(int i) const { return i+1 < size; }

        //! Add an entry
		void Add(Item *it,const t_symbol *tag,int inlet = 0);
        //! Remove an entry
		bool Remove(Item *it,const t_symbol *tag,int inlet,bool free);
        //! Find an entry list in the Item array
		Item *FindList(const t_symbol *tag,int inlet = 0);
		
        //! Get list for an inlet
        ItemSet &GetInlet(int inlet = 0)
		{ 
			FLEXT_ASSERT(inlet >= Min() && inlet <= Max()); 
			return *cont[inlet+1]; 
		}

        //! Get counter for total members (for index of new item)
        int Members() const { return members; }

    protected:

		void Resize(int nsz);

        int members;
		int memsize,size;
		ItemSet **cont;
	};

    //! \brief This represents an item of the method list
	class MethItem:
		public Item 
    { 
	public:
		MethItem(AttrItem *conn = NULL);
		virtual ~MethItem();
		
		void SetArgs(methfun fun,int argc,metharg *args);

		int index;
		int argc;
		metharg *args;
		methfun fun;
	};
	
	//! \brief This represents an item of the attribute list
	class AttrItem:
		public Item 
    { 
	public:
		AttrItem(const t_symbol *tag,metharg tp,methfun fun,int flags);

		enum { 
			afl_get = 0x01, afl_set = 0x02, 
			afl_getset = afl_get|afl_set,
			afl_shown = 0x08
		};

		bool IsGet() const { return (flags&afl_getset) == afl_get; }
		bool IsSet() const { return (flags&afl_getset) == afl_set; }
		bool IsShown() const { return (flags&afl_shown) != 0; }
		bool BothExist() const { return counter != NULL; }
		AttrItem *Counterpart() { return counter; }

		int index;
		int flags;
		metharg argtp;
		methfun fun;
		AttrItem *counter;
		const t_symbol *tag;
	};

	//! Represent a data value of an attribute
    class AttrData:
        public flext_root
	{
	public:
		AttrData(): flags(0) {}

		enum { afl_save = 0x01,afl_init = 0x02,afl_inited = 0x04 };

		void SetSave(bool s) { if(s) flags  |= afl_save; else flags &= ~afl_save; }
		bool IsSaved() const { return (flags&afl_save) != 0; }
		void SetInit(bool s) { if(s) flags  |= afl_init; else flags &= ~afl_init; }
		bool IsInit() const { return (flags&afl_init) != 0; }
		void SetInitValue(int argc,const t_atom *argv) { init(argc,argv); flags |= afl_inited; }
		void SetInitValue(const AtomList &l) { SetInitValue(l.Count(),l.Atoms()); }
		bool IsInitValue() const { return (flags&afl_inited) != 0; }
		const AtomList &GetInitValue() const { return init; }

		AtomList init;
		int flags;
	};

	class AttrDataCont
        :public TablePtrMap<const t_symbol *,AttrData *,8>
    {
    public:
        virtual ~AttrDataCont();
        virtual void clear();
    };

	// these outlet functions don't check for thread but send directly to the real-time system
    void ToSysBang(int n) const { outlet *o = GetOut(n); if(o) { CRITON(); outlet_bang((t_outlet *)o); CRITOFF(); } }
    void ToSysFloat(int n,float f) const { outlet *o = GetOut(n); if(o) { CRITON(); outlet_float((t_outlet *)o,f); CRITOFF(); } }
    void ToSysInt(int n,int f) const { outlet *o = GetOut(n); if(o) { CRITON(); outlet_flint((t_outlet *)o,f); CRITOFF(); } }
    void ToSysSymbol(int n,const t_symbol *s) const { outlet *o = GetOut(n); if(o) { CRITON(); outlet_symbol((t_outlet *)o,const_cast<t_symbol *>(s)); CRITOFF(); } }
	void ToSysString(int n,const char *s) const { ToSysSymbol(n,MakeSymbol(s)); }
    void ToSysList(int n,int argc,const t_atom *argv) const { outlet *o = GetOut(n); if(o) { CRITON(); outlet_list((t_outlet *)o,const_cast<t_symbol *>(sym_list),argc,(t_atom *)argv); CRITOFF(); } }
	void ToSysList(int n,const AtomList &list) const { ToSysList(n,list.Count(),list.Atoms()); }
    void ToSysAnything(int n,const t_symbol *s,int argc,const t_atom *argv) const { outlet *o = GetOut(n); if(o) { CRITON(); outlet_anything((t_outlet *)o,const_cast<t_symbol *>(s),argc,(t_atom *)argv); CRITOFF(); } }
	void ToSysAnything(int n,const AtomAnything &any) const { ToSysAnything(n,any.Header(),any.Count(),any.Atoms()); }
	void ToSysAnything(int n,const t_symbol *s,const AtomList &list) const { ToSysAnything(n,s,list.Count(),list.Atoms()); }

	void ToSysBool(int n,bool f) const { ToSysInt(n,f?1:0); }
	void ToSysAtom(int n,const t_atom &at) const;
	void ToSysDouble(int n,double d) const { t_atom dbl[2]; ToSysList(n,2,SetDouble(dbl,d)); }

	static void ToSysMsg(MsgBundle *mb);

	// add class method handlers
	static void AddMessageMethods(t_class *c,bool dsp,bool dspin);

private:
	class pxbnd_object;
public:

	//! \brief This represents an item of the symbol-bound method list
    class BindItem:
		public Item 
	{ 
	public:
		BindItem(bool (*f)(flext_base *,t_symbol *s,int,t_atom *,void *),pxbnd_object *px);
		virtual ~BindItem();

		void Unbind(const t_symbol *s);

		bool (*fun)(flext_base *,t_symbol *s,int,t_atom *,void *);
        pxbnd_object *px;
	};
	
	ItemCont *ThMeths() { if(!methhead) methhead = new ItemCont; return methhead; }
	static ItemCont *ClMeths(t_classid c);

	//! \brief This is the central function to add message handlers. It is used by all other AddMethod incarnations.
	static void AddMethod(ItemCont *ma,int inlet,const t_symbol *tag,methfun fun,metharg tp,...); 

	ItemCont *ThAttrs() { return attrhead; }
	static ItemCont *ClAttrs(t_classid c);

    static void AddAttrib(ItemCont *aa,ItemCont *ma,const t_symbol *attr,metharg tp,methfun gfun,methfun sfun);
    void AddAttrib(const t_symbol *attr,metharg tp,methfun gfun,methfun sfun);
	static void AddAttrib(t_classid c,const t_symbol *attr,metharg tp,methfun gfun,methfun sfun);

private:

	static inline flext_base *thisObject(flext_hdr *c) { return FLEXT_CAST<flext_base *>(c->data); } 

	static void Setup(t_classid c);

    //! \brief This represents either an inlet or outlet during construction
	class FLEXT_SHARE xlet {	
    public:
        xlet();
        ~xlet();

        xlettype tp;
		char *desc;

        void Desc(const char *c);
	};

	static xlet inlist[];
    static xlet outlist[];

    //! current message tag
	static const t_symbol *curtag;
    //! number of message and signal inlets/outlets
	unsigned char incnt,outcnt,insigs,outsigs;

	outlet **outlets;

	union t_any {
		float ft;
		int it;
		bool bt;
		const t_symbol *st;
#if FLEXT_SYS == FLEXT_SYS_PD
		t_gpointer *pt;
#endif
		void *vt;
	};

	typedef bool (*methfun_V)(flext_base *c,int argc,t_atom *argv);
	typedef bool (*methfun_A)(flext_base *c,const t_symbol *s,int argc,t_atom *argv);
	typedef bool (*methfun_0)(flext_base *c);
	typedef bool (*methfun_1)(flext_base *c,t_any &);
	typedef bool (*methfun_2)(flext_base *c,t_any &,t_any &);
	typedef bool (*methfun_3)(flext_base *c,t_any &,t_any &,t_any &);
	typedef bool (*methfun_4)(flext_base *c,t_any &,t_any &,t_any &,t_any &);
	typedef bool (*methfun_5)(flext_base *c,t_any &,t_any &,t_any &,t_any &,t_any &);

	mutable ItemCont *methhead;
	mutable ItemCont *bindhead;
	
	bool FindMeth(int inlet,const t_symbol *s,int argc,const t_atom *argv);
	bool FindMethAny(int inlet,const t_symbol *s,int argc,const t_atom *argv);
	bool TryMethTag(Item *lst,const t_symbol *tag,int argc,const t_atom *argv);
	bool TryMethSym(Item *lst,const t_symbol *s);
	bool TryMethAny(Item *lst,const t_symbol *s,int argc,const t_atom *argv);

	mutable ItemCont *attrhead;
	mutable AttrDataCont *attrdata;

	AttrItem *FindAttrib(const t_symbol *tag,bool get,bool msg = false) const;

	bool InitAttrib(int argc,const t_atom *argv);

	bool DumpAttrib(const t_symbol *tag,AttrItem *a) const;
	bool GetAttrib(const t_symbol *tag,AttrItem *a,AtomList &l) const;
	bool SetAttrib(const t_symbol *tag,AttrItem *a,int argc,const t_atom *argv);
	bool SetAttrib(const t_symbol *tag,AttrItem *a,const AtomList &l) { return SetAttrib(tag,a,l.Count(),l.Atoms()); }
	// get and set the attribute
	bool BangAttrib(const t_symbol *tag,AttrItem *a);
	// show/hide the attribute
	bool ShowAttrib(AttrItem *a,bool show) const;

	static bool cb_ListMethods(flext_base *c,int argc,const t_atom *argv);
	static bool cb_ListAttrib(flext_base *c) { Locker lock(c); return c->ListAttrib(); }

	// queue stuff

	//! Start message queue
	static void StartQueue();
#if FLEXT_QMODE == 2
    //! Queue worker function
    static void QWorker(thr_params *);
#endif
	//! Flush messages in the queue
	static void QFlush(flext_base *th = NULL);

    static bool qustarted;
    
#if FLEXT_SYS == FLEXT_SYS_PD

	static void SetGfx(t_classid c);

#ifndef FLEXT_NOATTREDIT
	// attribute editor
	static bool cb_AttrDialog(flext_base *c,int argc,const t_atom *argv);
	static void cb_GfxProperties(flext_hdr *c, t_glist *);
#endif

#ifdef FLEXT_ATTRHIDE
	static void cb_GfxVis(flext_hdr *c, t_glist *gl, int vis);
	static void cb_GfxSave(flext_hdr *c, t_binbuf *b);
	static void cb_GfxSelect(flext_hdr *x, struct _glist *glist, int state);

    void BinbufArgs(t_binbuf *b,t_binbuf *args,bool withname,bool transdoll);
    void BinbufAttr(t_binbuf *b,bool transdoll);
#endif

	static void cb_bang(flext_hdr *c);
	static void cb_float(flext_hdr *c,t_float f);
	static void cb_symbol(flext_hdr *c,const t_symbol *s);
//    static void cb_pointer(fltext_hdr *c,const t_gpointer *p);
	static void cb_anything(flext_hdr *c,const t_symbol *s,int argc,t_atom *argv);

	// proxy object (for additional inlets)
	static t_class *px_class;

	struct px_object  // no virtual table!
	{ 
		t_object obj;			// MUST reside at memory offset 0
		flext_base *base;
		int index;

		void init(flext_base *b,int ix) { base = b; index = ix; }
		static void px_bang(px_object *c);
		static void px_float(px_object *c,t_float f);
		static void px_symbol(px_object *c,const t_symbol *s);
//		static void px_pointer(px_object *c,const t_gpointer *p);
		static void px_anything(px_object *c,const t_symbol *s,int argc,t_atom *argv);
	};

	static void cb_px_ft1(flext_hdr *c,t_float f);
	static void cb_px_ft2(flext_hdr *c,t_float f);
	static void cb_px_ft3(flext_hdr *c,t_float f);
	static void cb_px_ft4(flext_hdr *c,t_float f);
	static void cb_px_ft5(flext_hdr *c,t_float f);
	static void cb_px_ft6(flext_hdr *c,t_float f);
	static void cb_px_ft7(flext_hdr *c,t_float f);
	static void cb_px_ft8(flext_hdr *c,t_float f);
	static void cb_px_ft9(flext_hdr *c,t_float f);
	
#elif FLEXT_SYS == FLEXT_SYS_MAX
	typedef object px_object;
	static void cb_bang(flext_hdr *c);
	static void cb_float(flext_hdr *c,double f);
	static void cb_int(flext_hdr *c,long v);
	static void cb_anything(flext_hdr *c,const t_symbol *s,short argc,t_atom *argv);

	static void cb_px_in1(flext_hdr *c,long v);
	static void cb_px_in2(flext_hdr *c,long v);
	static void cb_px_in3(flext_hdr *c,long v);
	static void cb_px_in4(flext_hdr *c,long v);
	static void cb_px_in5(flext_hdr *c,long v);
	static void cb_px_in6(flext_hdr *c,long v);
	static void cb_px_in7(flext_hdr *c,long v);
	static void cb_px_in8(flext_hdr *c,long v);
	static void cb_px_in9(flext_hdr *c,long v);

	static void cb_px_ft1(flext_hdr *c,double f);
	static void cb_px_ft2(flext_hdr *c,double f);
	static void cb_px_ft3(flext_hdr *c,double f);
	static void cb_px_ft4(flext_hdr *c,double f);
	static void cb_px_ft5(flext_hdr *c,double f);
	static void cb_px_ft6(flext_hdr *c,double f);
	static void cb_px_ft7(flext_hdr *c,double f);
	static void cb_px_ft8(flext_hdr *c,double f);
	static void cb_px_ft9(flext_hdr *c,double f);
#endif

	px_object **inlets;

	// --------- symbol-bound proxy

	static t_class *pxbnd_class;

    class pxbnd_object:
        public flext_root
        // no virtual table!
	{ 
    public:
		t_object obj;			// MUST reside at memory offset 0
		flext_base *base;
		BindItem *item;
        void *data;

		void init(flext_base *b,BindItem *it,void *d) { base = b; item = it; data = d; }
		static void px_method(pxbnd_object *c,const t_symbol *s,int argc,t_atom *argv);
	};
	
    //! create proxy class for symbol binding
	static void SetupBindProxy();

	// ---------

    //! set up inlet proxies
	static void SetProxies(t_class *c,bool dsp);

    //! initialize inlets (according to class or object constructor definitions)
	bool InitInlets();

    //! initialize outlets (according to class or object constructor definitions)
	bool InitOutlets();

	// callback functions

	static void cb_loadbang(flext_hdr *c);

#if FLEXT_SYS == FLEXT_SYS_MAX
	char **indesc,**outdesc;

	static void cb_assist(flext_hdr *c,void *b,long msg,long arg,char *s);
    static void cb_click (flext_hdr *c, Point pt, short mods);

	static void cb_dsp(flext_hdr *c,t_signal **s,short *count);
#elif FLEXT_SYS == FLEXT_SYS_PD
	static void cb_click(flext_hdr *z,t_floatarg xpos,t_floatarg ypos,t_floatarg shift,t_floatarg ctrl,t_floatarg alt);

	static void cb_dsp(flext_hdr *c,t_signal **s);
#endif
};

#include "flpopns.h"

#endif
