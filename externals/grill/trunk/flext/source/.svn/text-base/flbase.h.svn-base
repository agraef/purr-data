/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file flbase.h
	\brief Internal flext base classes
    
	\remark This uses some ideas of GEM invented by Mark Danks
*/
 
#ifndef __FLEXT_BASE_H
#define __FLEXT_BASE_H

#include "flstdc.h"
#include "flsupport.h"

#include "flpushns.h"

class FLEXT_SHARE FLEXT_CLASSDEF(flext_obj);
typedef class FLEXT_CLASSDEF(flext_obj) flext_obj;

// ----------------------------------------------------------------------------
/*! \brief The obligatory PD or Max/MSP object header
	\internal

    This is in a separate struct to assure that obj is the very first thing.  
    If it were the first thing in flext_obj, then there could be problems with
    the virtual table of the C++ class.
*/
// ----------------------------------------------------------------------------

struct FLEXT_SHARE flext_hdr
{
	/*!	\defgroup FLEXT_OBJHEADER Actual PD or Max/MSP object
		\internal
		@{ 
	*/

    	/*! \brief The obligatory object header
			\note MUST reside at memory offset 0 (no virtual table possible)
		*/
    	t_sigobj    	    obj;  

#if FLEXT_SYS == FLEXT_SYS_PD
		//! PD only: float signal holder for pd
		float defsig;			
#endif

#if FLEXT_SYS == FLEXT_SYS_MAX
		//! Max/MSP only: current inlet used by proxy objects
		long curinlet;      
#endif

    	/*! \brief This points to the actual polymorphic C++ class
		*/
        FLEXT_CLASSDEF(flext_obj) *data;

	//!	@}  FLEXT_OBJHEADER
};


class flext_class;

// ----------------------------------------------------------------------------
/*! \brief The mother of base classes for all flext external objects

    Each extern which is written in C++ needs to use the #defines at the
    end of this header file.  
    
    The define
    
        FLEXT_HEADER(NEW_CLASS, PARENT_CLASS)
    
    should be somewhere in your header file.
    One of the defines like
    
    FLEXT_NEW(NEW_CLASS)
	or
    FLEXT_NEW_2(NEW_CLASS, float, float)
    
    should be the first thing in your implementation file.
    NEW_CLASS is the name of your class and PARENT_CLASS is the 
    parent of your class.
*/
// ----------------------------------------------------------------------------

class FLEXT_SHARE FLEXT_CLASSDEF(flext_obj):
	public flext
{
    public:

// --- creation -------------------------------------------------------	

	/*!	\defgroup FLEXT_OBJ_CREATION Object creation/destruction functionality
		@{ 
	*/

        //! Constructor
    	FLEXT_CLASSDEF(flext_obj)();

    	//! Destructor
    	virtual ~FLEXT_CLASSDEF(flext_obj)();

        /*! \brief Signal a construction problem
			\note This should only be used in the constructor. Object creation will be aborted.
		*/
		static void InitProblem() { init_ok = false; }

		/*! \brief Enable/disable attribute procession (default = false)
			\note Use that in the static class setup function (also library setup function)
		*/
//		static void ProcessAttributes(bool attr); //{ process_attributes = attr; }

		//! Virtual function called at creation time (but after the constructor)
		// this also guarantees that there are no instances of flext_obj
		virtual bool Init(); 

		//! Virtual function called after Init() has succeeded
		virtual bool Finalize();
	
		//! Virtual function called at destruction (before the destructor)
		virtual void Exit();

	//!	@}  FLEXT_OBJ_CREATION

// --- info -------------------------------------------------------	

	/*!	\defgroup FLEXT_OBJ_INFO Get various information
		@{ 
	*/

        //! Get the object's canvas
        t_canvas *thisCanvas() const { return m_canvas; }

        //! Get the PD or Max/MSP object
		t_sigobj *thisHdr() { FLEXT_ASSERT(x_obj); return &x_obj->obj; }
		const t_sigobj *thisHdr() const { FLEXT_ASSERT(x_obj); return &x_obj->obj; }
        //! Get the class name (as a string)
		const char *thisName() const { return GetString(m_name); } 
        //! Get the class name (as a symbol)
		const t_symbol *thisNameSym() const { return m_name; } 
        //! Get the class pointer
		t_class *thisClass() const;

		//! Typedef for unique class identifier
		typedef flext_class *t_classid;

		//! Get unique id for object class
		t_classid thisClassId() const { return clss; }

		//! Get class pointer from class id
		static t_class *getClass(t_classid id);
		
        static bool HasAttributes(t_classid id);
        static bool IsDSP(t_classid id);
        static bool HasDSPIn(t_classid id);
        static bool IsLib(t_classid id);

        bool HasAttributes() const;
        bool IsLib() const;
        bool IsDSP() const;
        bool HasDSPIn() const;

#if FLEXT_SYS == FLEXT_SYS_MAX
		// under Max/MSP it could be necessary to activate DSP also for message objects
		// namely for those coexisting with DSP objects in a library
		bool NeedDSP() const;
#endif

	//!	@}  FLEXT_OBJ_INFO

// --- help -------------------------------------------------------	

	/*!	\defgroup FLEXT_OBJ_HELP Help/assistance functionality
		\remark This is still PD only
		@{ 
	*/

		/*! Define the help reference symbol for a class
			\internal
		*/
		static void DefineHelp(t_classid c,const char *ref,const char *dir = NULL,bool addtilde = false);

		//! Define the help reference symbol for a class
		void DefineHelp(const char *ref,const char *dir = NULL,bool addtilde = false) { DefineHelp(thisClassId(),ref,dir,addtilde); }

	//!	@} FLEXT_OBJ_HELP


// --- internal stuff -------------------------------------------------------	

	/*!	\defgroup FLEXT_OBJ_INTERNAL Internal stuff
		\internal
		@{ 
	*/

    protected:    	

        //! backpointer to object header
        mutable flext_hdr *x_obj;        	

        //! pointer to flext class definition
        flext_class *clss;

//        static bool	process_attributes;

#if FLEXT_SYS == FLEXT_SYS_MAX
        t_critical lock;
        void Lock() { critical_enter(lock); }
        void Unlock() { critical_exit(lock); }
        static void SysLock() { critical_enter(0); }
        static void SysUnlock() { critical_exit(0); }
#elif FLEXT_SYS == FLEXT_SYS_PD
        void Lock() {}
        void Unlock() {}
        static void SysLock() {}
        static void SysUnlock() {}
#else
    #error
#endif

        class Locker
        {
        public:
            Locker(flext_obj *o = NULL): obj(o)  { if(obj) obj->Lock(); else SysLock(); }
            Locker(flext_hdr *h): obj(h->data)  { FLEXT_ASSERT(obj); obj->Lock(); }
            ~Locker() { if(obj) obj->Unlock(); else SysUnlock();  }
        protected:
            flext_obj *obj;
        };

    private:

        //! The canvas (patcher) that the object is in
        mutable t_canvas            *m_canvas;
        
        //! Flag for successful object construction
        static bool	init_ok;

        // flags for init and exit procedure;
        static bool initing;
        static bool exiting;

	public:

    	//! Creation callback
		static void __setup__(t_classid);	

		/*! \brief This is a temporary holder
			\warning don't touch it!
		*/
        static flext_hdr     *m_holder;
		//! Hold object's class during construction
		static flext_class *m_holdclass;
		//! Hold object's name during construction
        static const t_symbol *m_holdname;  

		//! Holders for attribute procession flag
		static int m_holdaargc;
		static const t_atom *m_holdaargv;

        /*! The object's name in the patcher 
            \note objects of the same class can have various alias names!
        */
		const t_symbol *m_name;

        /*! Return true if in object initialization phase
            true when in constructor or Init, false when in Finalize
        */
        static bool Initing() { return initing; }

        //! Return true if in object destruction phase (Exit or destructor)
        static bool Exiting() { return exiting; }

		// Definitions for library objects
		static void lib_init(const char *name,void setupfun());
		static void obj_add(bool lib,bool dsp,bool noi,bool attr,const char *idname,const char *names,void setupfun(t_classid),FLEXT_CLASSDEF(flext_obj) *(*newfun)(int,t_atom *),void (*freefun)(flext_hdr *),int argtp1,...);
#if FLEXT_SYS == FLEXT_SYS_MAX
		static flext_hdr *obj_new(const t_symbol *s,short argc,t_atom *argv);
#else
		static flext_hdr *obj_new(const t_symbol *s,int argc,t_atom *argv);
#endif
		static void obj_free(flext_hdr *o);

		//! Convert $0 or #0 symbol into appropriate value
		static bool GetParamSym(t_atom &dst,const t_symbol *s,t_canvas *c);

		//! Get the canvas arguments
		void GetCanvasArgs(AtomList &args) const;

		//! Get the canvas/patcher directory
        void GetCanvasDir(char *buf,size_t bufsz) const;

	//!	@} FLEXT_OBJ_INTERNAL
};


// max. 4 creation args (see the following macros)
#define FLEXT_MAXNEWARGS 4 

// max. 5 method args (see the following macros)
#define FLEXT_MAXMETHARGS 5 

// prefixes for the macro generated handler functions
#define FLEXT_CALL_PRE(F) flext_c_##F
#define FLEXT_THR_PRE(F) flext_t_##F
#define FLEXT_GET_PRE(F) flext_g_##F
#define FLEXT_SET_PRE(F) flext_s_##F


#ifndef FLEXT_ATTRIBUTES
/*! \brief Switch for global attribute processing
	\note Should be set to 1 or 0 (or not be defined)
	\ingroup FLEXT_DEFS
*/
#define FLEXT_ATTRIBUTES \
\
0


#elif FLEXT_ATTRIBUTES != 0 && FLEXT_ATTRIBUTES != 1
#error "FLEXT_ATTRIBUTES must be 0 or 1"
#endif

// ----------------------------------------
// These should be used in the header
// ----------------------------------------


#define FLEXT_REALHDR(NEW_CLASS, PARENT_CLASS)    	    	\
public:     	    	    \
typedef NEW_CLASS thisType;  \
typedef PARENT_CLASS thisParent;  \
static FLEXT_CLASSDEF(flext_obj) *__init__(int argc,t_atom *argv);  \
static void __free__(flext_hdr *hdr) { delete hdr->data; }   	    	\
static void __setup__(t_classid classid) { thisParent::__setup__(classid); }


#define FLEXT_REALHDR_S(NEW_CLASS, PARENT_CLASS,SETUPFUN)    	    	\
public:     	    	    \
typedef NEW_CLASS thisType;  \
typedef PARENT_CLASS thisParent;  \
static FLEXT_CLASSDEF(flext_obj) *__init__(int argc,t_atom *argv);  \
static void __free__(flext_hdr *hdr) { delete hdr->data; }   	    	\
static void __setup__(t_classid classid) { 	    	\
	thisParent::__setup__(classid);    	    	\
	thisType::SETUPFUN(classid); \
}

#define FLEXT_REALHDR_T(NEW_CLASS, PARENT_CLASS)    	    	\
public:    	    \
typedef NEW_CLASS thisType;  \
typedef PARENT_CLASS thisParent;  \
typedef typename thisParent::t_classid t_classid;  \
static FLEXT_CLASSDEF(flext_obj) *__init__(int argc,t_atom *argv);  \
static void __free__(flext_hdr *hdr) { delete hdr->data; }   	    	\
static void __setup__(t_classid classid) { thisParent::__setup__(classid); }


#define FLEXT_REALHDR_TS(NEW_CLASS, PARENT_CLASS,SETUPFUN)    	    	\
public:     	    	    \
typedef NEW_CLASS thisType;  \
typedef PARENT_CLASS thisParent;  \
typedef typename thisParent::t_classid t_classid;  \
static FLEXT_CLASSDEF(flext_obj) *__init__(int argc,t_atom *argv);  \
static void __free__(flext_hdr *hdr) { delete hdr->data; }   	    	\
static void __setup__(t_classid classid) { 	    	\
	thisParent::__setup__(classid);    	    	\
	thisType::SETUPFUN(classid); \
}


// generate name of dsp/non-dsp setup function
#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
	#define FLEXT_STPF_0(NAME) NAME##_setup
	#define FLEXT_STPF_1(NAME) NAME##_tilde_setup
#else
#error Platform not supported
#endif

#define FLEXT_STPF_(DSP) FLEXT_STPF_##DSP
#define FLEXT_STPF(NAME,DSP) FLEXT_STPF_(DSP)(NAME)


// --------------------------------------------------------------------------------------


// used in library setup functions to register the individual objects in the library
#define REAL_SETUP(cl,DSP) extern void FLEXT_STPF(cl,DSP)(); FLEXT_STPF(cl,DSP)();

#ifdef FLEXT_USE_NAMESPACE
    #define _FLEXT_REAL_SETUP_NAME(NAME) ::##NAME##_setup
#else
    #define _FLEXT_REAL_SETUP_NAME(NAME) NAME##_setup
#endif

// specify that to define the library itself
#if FLEXT_SYS == FLEXT_SYS_PD
#   define REAL_LIB_SETUP(NAME,SETUPFUN) extern "C" FLEXT_EXT void _FLEXT_REAL_SETUP_NAME(NAME)() { flext_obj::lib_init(#NAME,SETUPFUN); }
#elif FLEXT_SYS == FLEXT_SYS_MAX
#   define REAL_LIB_SETUP(NAME,SETUPFUN) extern "C" FLEXT_EXT int main() { flext_obj::lib_init(#NAME,SETUPFUN); return 0; }
#else
#   error Platform not supported
#endif


// --------------------------------------------------


#define FLEXT_EXP_0 extern "C" FLEXT_EXT
#define FLEXT_EXP_1 
#define FLEXT_EXP(LIB) FLEXT_EXP_##LIB

#if FLEXT_SYS == FLEXT_SYS_PD
#define FLEXT_OBJ_SETUP_0(NEW_CLASS,DSP)
#elif FLEXT_SYS == FLEXT_SYS_MAX
#define FLEXT_OBJ_SETUP_0(NEW_CLASS,DSP) extern "C" FLEXT_EXT int main() { FLEXT_STPF(NEW_CLASS,DSP)(); return 0; }
#else
#error not implemented
#endif

#define FLEXT_OBJ_SETUP_1(NEW_CLASS,DSP)

#define FLEXT_OBJ_SETUP(NEW_CLASS,DSP,LIB) FLEXT_OBJ_SETUP_##LIB(NEW_CLASS,DSP)



// ----------------------------------------
// These definitions are used below
// ----------------------------------------

#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
	// maybe that's not necessary
	#define FLEXTTPN_NULL A_NULL
	#if FLEXT_SYS == FLEXT_SYS_PD 
		#define FLEXTTPN_PTR A_POINTER
	#else
		#define FLEXTTPN_INT A_INT
		#define FLEXTTPN_DEFINT A_DEFINT
	#endif
	#define FLEXTTPN_FLOAT A_FLOAT
	#define FLEXTTPN_DEFFLOAT A_DEFFLOAT
	#define FLEXTTPN_SYM A_SYMBOL
	#define FLEXTTPN_DEFSYM A_DEFSYMBOL
	#define FLEXTTPN_VAR A_GIMME
#else
	#define FLEXTTPN_NULL 0
	#define FLEXTTPN_PTR 1
	#define FLEXTTPN_INT 2
	#define FLEXTTPN_FLOAT 3
	#define FLEXTTPN_SYM 4
	#define FLEXTTPN_VAR 5
	#define FLEXTTPN_DEFINT 6
	#define FLEXTTPN_DEFFLOAT 7
	#define FLEXTTPN_DEFSYM 8
#endif

// Shortcuts for PD/Max type arguments
#define FLEXTTYPE_void FLEXTTPN_NULL
#define CALLBTYPE_void void
#define FLEXTTYPE_float FLEXTTPN_FLOAT
#define FLEXTTYPE_float0 FLEXTTPN_DEFFLOAT
#define CALLBTYPE_float float
#define FLEXTTYPE_t_float FLEXTTPN_FLOAT
#define CALLBTYPE_t_float t_float

#if FLEXT_SYS == FLEXT_SYS_PD
#define FLEXTTYPE_int FLEXTTPN_FLOAT
#define FLEXTTYPE_int0 FLEXTTPN_DEFFLOAT
#define CALLBTYPE_int float
#define FLEXTTYPE_bool FLEXTTPN_FLOAT
#define FLEXTTYPE_bool0 FLEXTTPN_DEFFLOAT
#define CALLBTYPE_bool float
#elif FLEXT_SYS == FLEXT_SYS_MAX
#define FLEXTTYPE_int FLEXTTPN_INT
#define FLEXTTYPE_int0 FLEXTTPN_DEFINT
#define CALLBTYPE_int int
#define FLEXTTYPE_bool FLEXTTPN_INT
#define FLEXTTYPE_bool0 FLEXTTPN_DEFINT
#define CALLBTYPE_bool int
#else
#error Platform not supported
#endif

#define FLEXTTYPE_t_symptr FLEXTTPN_SYM
#define FLEXTTYPE_t_symptr0 FLEXTTPN_DEFSYM
#define CALLBTYPE_t_symptr t_symptr
#define FLEXTTYPE_t_symtype FLEXTTYPE_t_symptr
#define FLEXTTYPE_t_symtype0 FLEXTTYPE_t_symptr0
#define CALLBTYPE_t_symtype t_symptr
#define FLEXTTYPE_t_ptrtype FLEXTTPN_PTR
#define CALLBTYPE_t_ptrtype t_ptrtype

#define FLEXTTP(TP) FLEXTTYPE_ ## TP
#define CALLBTP(TP) CALLBTYPE_ ## TP


#define ARGMEMBER_bool(a) GetBool(a)
#define ARGMEMBER_bool0(a) ARGMEMBER_bool(a)
#define ARGMEMBER_int(a) GetInt(a)
#define ARGMEMBER_int0(a) ARGMEMBER_int(a)
#define ARGMEMBER_float(a) GetFloat(a)
#define ARGMEMBER_float0(a) ARGMEMBER_float(a)
#define ARGMEMBER_t_symptr(a) GetSymbol(a)
#define ARGMEMBER_t_symptr0(a) ARGMEMBER_t_symptr(a)
#define ARGMEMBER_t_symtype(a) ARGMEMBER_t_symptr(a)
#define ARGMEMBER_t_symtype0(a) ARGMEMBER_t_symptr0(a)
#define ARGCAST(a,tp) ARGMEMBER_##tp(a)

#define REAL_NEW(NAME,NEW_CLASS,DSP,NOI,LIB) \
flext_obj *NEW_CLASS::__init__(int ,t_atom *) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS;                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::obj_add(LIB,DSP,NOI,FLEXT_ATTRIBUTES,#NEW_CLASS,NAME,NEW_CLASS::__setup__,NEW_CLASS::__init__,&NEW_CLASS::__free__,FLEXTTPN_NULL); \
} \
FLEXT_OBJ_SETUP(NEW_CLASS,DSP,LIB)

#define REAL_NEW_V(NAME,NEW_CLASS,DSP,NOI,LIB) \
flext_obj *NEW_CLASS::__init__(int argc,t_atom *argv) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(argc,argv);                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::obj_add(LIB,DSP,NOI,FLEXT_ATTRIBUTES,#NEW_CLASS,NAME,NEW_CLASS::__setup__,NEW_CLASS::__init__,&NEW_CLASS::__free__,FLEXTTPN_VAR,FLEXTTPN_NULL); \
} \
FLEXT_OBJ_SETUP(NEW_CLASS,DSP,LIB)

#define REAL_NEW_1(NAME,NEW_CLASS,DSP,NOI,LIB, TYPE1) \
flext_obj *NEW_CLASS::__init__(int,t_atom *argv) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(argv[0],TYPE1));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::obj_add(LIB,DSP,NOI,FLEXT_ATTRIBUTES,#NEW_CLASS,NAME,NEW_CLASS::__setup__,NEW_CLASS::__init__,NEW_CLASS::__free__,FLEXTTP(TYPE1),FLEXTTPN_NULL); \
} \
FLEXT_OBJ_SETUP(NEW_CLASS,DSP,LIB)

#define REAL_NEW_2(NAME,NEW_CLASS,DSP,NOI,LIB, TYPE1,TYPE2) \
flext_obj *NEW_CLASS::__init__(int,t_atom *argv) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(argv[0],TYPE1),ARGCAST(argv[1],TYPE2));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::obj_add(LIB,DSP,NOI,FLEXT_ATTRIBUTES,#NEW_CLASS,NAME,NEW_CLASS::__setup__,NEW_CLASS::__init__,NEW_CLASS::__free__,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTPN_NULL); \
} \
FLEXT_OBJ_SETUP(NEW_CLASS,DSP,LIB)

#define REAL_NEW_3(NAME,NEW_CLASS,DSP,NOI,LIB, TYPE1, TYPE2, TYPE3) \
flext_obj *NEW_CLASS::__init__(int,t_atom *argv) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(argv[0],TYPE1),ARGCAST(argv[1],TYPE2),ARGCAST(argv[2],TYPE3));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::obj_add(LIB,DSP,NOI,FLEXT_ATTRIBUTES,#NEW_CLASS,NAME,NEW_CLASS::__setup__,NEW_CLASS::__init__,NEW_CLASS::__free__,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3),FLEXTTPN_NULL); \
} \
FLEXT_OBJ_SETUP(NEW_CLASS,DSP,LIB)

#define REAL_NEW_4(NAME,NEW_CLASS,DSP,NOI,LIB, TYPE1,TYPE2, TYPE3, TYPE4) \
flext_obj *NEW_CLASS::__init__(int,t_atom *argv) \
{     	    	    	    	    	    	    	    	\
    return new NEW_CLASS(ARGCAST(argv[0],TYPE1),ARGCAST(argv[1],TYPE2),ARGCAST(argv[2],TYPE3),ARGCAST(argv[3],TYPE4));                     \
}   	    	    	    	    	    	    	    	\
FLEXT_EXP(LIB) void FLEXT_STPF(NEW_CLASS,DSP)()   \
{   	    	    	    	    	    	    	    	\
    flext_obj::obj_add(LIB,DSP,NOI,FLEXT_ATTRIBUTES,#NEW_CLASS,NAME,NEW_CLASS::__setup__,NEW_CLASS::__init__,NEW_CLASS::__free__,FLEXTTP(TYPE1),FLEXTTP(TYPE2),FLEXTTP(TYPE3),FLEXTTP(TYPE4),FLEXTTPN_NULL); \
} \
FLEXT_OBJ_SETUP(NEW_CLASS,DSP,LIB)


// Shortcuts for method arguments:
#define FLEXTARG_float a_float
#define FLEXTARG_int a_int
#define FLEXTARG_bool a_int
#define FLEXTARG_t_float a_float
#define FLEXTARG_t_symtype a_symbol
#define FLEXTARG_t_symptr a_symbol
#define FLEXTARG_t_ptrtype a_pointer

#define FLEXTARG(TP) FLEXTARG_ ## TP

#include "flpopns.h"

#endif
