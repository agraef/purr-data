/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file flsupport.h
    \brief flext support functions and classes   
*/

#ifndef __FLSUPPORT_H
#define __FLSUPPORT_H

#include "flstdc.h"
#include <new>
#include <cstring>


#include "flpushns.h"

/*! \defgroup FLEXT_SUPPORT Flext support classes
    @{
*/

class FLEXT_SHARE FLEXT_CLASSDEF(flext_root);
typedef class FLEXT_CLASSDEF(flext_root) flext_root;

/*! \brief Flext root support class

    Moved memory functions and console output here so that all the classes
    contained in flext can use them
*/
class FLEXT_SHARE FLEXT_CLASSDEF(flext_root) {
public:
// --- console output -----------------------------------------------   

    //! post message to console, with line feed (limited to 1k chars!)
    static void post(const char *fmt,...);
    //! post error message to console (limited to 1k chars!)
    static void error(const char *fmt,...);

// --- memory -------------------------------------------------------   

    /*! \defgroup FLEXT_S_MEMORY Memory allocation functions
        @{ 
    */

#ifdef FLEXT_NOGLOBALNEW
#error FLEXT_NOGLOBALNEW is deprecated, define FLEXT_USE_CMEM instead
#define FLEXT_USE_CMEM
#endif

#ifdef FLEXT_USE_CMEM
    inline void *operator new(size_t bytes) { return ::operator new(bytes); }
    inline void operator delete(void *blk) { ::operator delete(blk); }

    inline void *operator new[](size_t bytes) { return ::operator new[](bytes); }
    inline void operator delete[](void *blk) { ::operator delete[](blk); }

    static bool MemCheck(void *) { return true; }
#else
    /*! Overloaded new memory allocation method
        \note this uses a fast allocation method of the real-time system
        \warning Max/MSP (or MacOS) allows only 32K in overdrive mode!
    */
    void *operator new(size_t bytes);
    //! Overloaded delete method
    void operator delete(void *blk);

#ifndef __MRC__ // doesn't allow new[] overloading?!
    inline void *operator new[](size_t bytes) { return operator new(bytes); }
    inline void operator delete[](void *blk) { operator delete(blk); }
#endif

#ifdef FLEXT_DEBUGMEM
    static bool MemCheck(void *blk);
#else
    static bool MemCheck(void *) { return true; }
#endif

#endif // USECMEM

#ifndef __BORLANDC__
    inline void *operator new(size_t,void *p) { return p; }
    inline void operator delete(void *,void *) {}
#ifndef __MRC__
    inline void *operator new[](size_t,void *p) { return p; }
    inline void operator delete[](void *,void *) {}
#endif
#endif

    //! Get an aligned memory block
    static void *NewAligned(size_t bytes,int bitalign = 128);
    // same with templated type
    template<typename T>
    static T *NewAligned(size_t times,int bitalign = 128) { return static_cast<T *>(NewAligned(times*sizeof(T),bitalign)); }
    //! Free an aligned memory block
    static void FreeAligned(void *blk);
    //! Test for alignment
    static bool IsAligned(void *ptr,int bitalign = 128) { 
        return (reinterpret_cast<size_t>(ptr)&(bitalign-1)) == 0; 
    }
    //! @}  FLEXT_S_MEMORY      
};

#ifndef FLEXT_USE_CMEM
/************************************************************************/
// MFC doesn't like global overloading of allocators
// anyway, who likes MFC

#if !defined(_MSC_VER) && !defined(__BORLANDC__)
#define NEWTHROW throw(std::bad_alloc)
#define DELTHROW throw()
#else
#define NEWTHROW
#define DELTHROW
#endif

// define global new/delete operators
inline void *operator new(size_t bytes) NEWTHROW { return flext_root::operator new(bytes); }
inline void operator delete(void *blk) DELTHROW { flext_root::operator delete(blk); }
#ifndef __MRC__ // doesn't allow new[] overloading?!
inline void *operator new[](size_t bytes) NEWTHROW { return flext_root::operator new[](bytes); }
inline void operator delete[](void *blk) DELTHROW { flext_root::operator delete[](blk); }
#endif

#endif // FLEXT_USE_CMEM

/************************************************************************/

class FLEXT_SHARE FLEXT_CLASSDEF(flext);
typedef class FLEXT_CLASSDEF(flext) flext;

class FLEXT_SHARE FLEXT_CLASSDEF(flext_base);

/*! \brief Flext support class

    A number of methods (most are static functions) are defined here for convenience.
    This class doesn't define any data members, hence it can be inherited to all
    classes (not only PD objects) to profit from the cross-platform functionality.
    Examples are the overloaded memory allocation, atom and atom list functions,
    thread functions and classes, the sample buffer class and others.

    This class can also be used for a non-object class (not representing an external object)
    and won't give any extra burden to it.
*/

class FLEXT_SHARE FLEXT_CLASSDEF(flext):
    public flext_root
{
    /*! \defgroup FLEXT_SUPPORT Flext support class
        @{ 
    */
public:

// --- version -----------------------------------------------  

    /*! \brief Flext version number 

        Return the version number of the flext library.
        For statically linked flext this is identical to the header definition FLEXT_VERSION,
        otherwise it reflects the version number of the shared flext library.
    */
    static int Version();    

    //! Flext version string
    static const char *VersionStr();

// --- special typedefs ---------------------------------------------   

// later!
#if 0
    typedef t_float Float;
    typedef t_int Int;
    typedef t_sample Sample;
    typedef const t_symbol *Symbol;
    typedef t_atom Atom;
#endif

// --- buffer/array stuff ----------------------------------------- 

    /*! \defgroup FLEXT_S_BUFFER Buffer handling
        @{ 
    */

    //! Class for platform independent buffer handling
    class FLEXT_SHARE buffer:
        public flext_root
    {
    public:
    
#if FLEXT_SYS == FLEXT_SYS_PD
        typedef bool lock_t;
#elif FLEXT_SYS == FLEXT_SYS_MAX
        typedef long lock_t;
#else
#error Not implemented
#endif


// PD 64-bit buffer handling macros
#if FLEXT_SYS == FLEXT_SYS_PD
#       if PD_MINOR_VERSION >= 41
                /* use new garray support that is 64-bit safe */
#               define FLEXT_PD_ARRAYGRAB garray_getfloatwords
#               define FLEXT_ARRAYTYPE t_word
#               define FLEXT_GETSAMPLE(x) ((x).w_float)

#       else
                /* use old garray support, not 64-bit safe */
#               define FLEXT_PD_ARRAYGRAB garray_getfloatarray
#               define FLEXT_ARRAYTYPE t_sample
#               define FLEXT_GETSAMPLE(x) (x)
#       endif

#elif FLEXT_SYS == FLEXT_SYS_MAX
#       define FLEXT_ARRAYTYPE t_sample
#       define FLEXT_GETSAMPLE(x) (x)
#endif

		class Element {
		public:
			Element() {}
			Element(t_sample s) { FLEXT_GETSAMPLE(el) = s; }
			operator t_sample &() { return FLEXT_GETSAMPLE(el); }
			operator t_sample () const { return FLEXT_GETSAMPLE(el); }
		protected:
			FLEXT_ARRAYTYPE el;
		};

        /*! \brief Construct buffer.
            \param s: symbol name, can be NULL
            \param delayed = true: only sets name, needs another Set(NULL) to really initialize the buffer 
            \remark As externals can be created prior to the buffer objects they are pointing to, initialization should be done at loadbang!
        */
        buffer(const t_symbol *s = NULL,bool delayed = false);
        
        //! Destroy buffer
        ~buffer();

        /*! \brief Check if the buffer is valid for use
            \note This must be true to use any of the other functions except set
        */
        bool Ok() const 
        { 
            return sym  
#if FLEXT_SYS == FLEXT_SYS_PD
                && arr
#endif
                && data; 
        }
        
        /*! \brief Check if buffer content is valid (not in state of content change)
            \note buffer must be Ok()
        */
        bool Valid() const
        {
            FLEXT_ASSERT(sym);
#if FLEXT_SYS == FLEXT_SYS_PD
            return true;
#elif FLEXT_SYS == FLEXT_SYS_MAX
            const t_buffer *p = (const t_buffer *)sym->s_thing;
            return p && p->b_valid;
#else
#error not implemented
#endif
        }
        
        /*! \brief Check and update if the buffer has been changed (e.g. resized)
            \note buffer must be Ok()
        */
        bool Update();
        
        /*! \brief Lock buffer
            \return previous state (needed for Unlock)
            \note buffer must be Ok()
        */
        lock_t Lock();
        
        /*! \brief Unlock buffer
            \param prv: Previous state is returned by Lock()
            \note buffer must be Ok()
        */
        void Unlock(lock_t prv);
        
        /*! \brief Set to specified buffer.
            \param nameonly: if true sets name only, but doesn't look at buffer actually
            \return -1 on failure, 0 on success, 1 if parameters (length, data ptr, channels) have changed
        */
        int Set(const t_symbol *s = NULL,bool nameonly = false);
        
        /*! \brief Declare buffer content as dirty.
            \param refr: if true forces immediate graphics refresh
        */
        void Dirty(bool refr = false);

        //! Clear the dirty flag.
        void ClearDirty();

        /*! Query whether the buffer content has been changed since the last ClearDirty()
            \note With mainstream versions of PD this will always return true, since the dirtiness can't be judged
        */
        bool IsDirty() const;

        //! Get symbol of buffer 
        const t_symbol *Symbol() const { return sym; }

        //! Get literal name of buffer 
        const char *Name() const { return sym?GetString(sym):""; }
        
        /*! \brief Get pointer to buffer, channel and frame count.
            \remark Channels are interleaved
        */
        Element *Data() { return data; }

    	const Element *Data() const { return data; }

        //! Get channel count
        int Channels() const { return chns; }
        //! Get frame count
        int Frames() const { return frames; }
        //! Set frame count
        void Frames(int fr,bool keep = false,bool zero = true);

        //! Get data value in a platform-independent way
        inline t_sample operator [](int index) const { return data[index]; }

        //! Reference data value in a platform-independent way
        inline t_sample &operator [](int index) { return data[index]; }
        
        //! Graphic auto refresh interval
        void SetRefrIntv(float intv);

        //! Buffer locking class
        class Locker
        {
        public:
            Locker(buffer &b): buf(b),lock(b.Lock()) {}
            ~Locker() { buf.Unlock(lock); }
        private:
            buffer &buf;
            lock_t lock;
        };

    protected:
        //! buffer name
        const t_symbol *sym;
        //! array holding audio data
        Element *data;
        //! number of audio channels
        int chns;
        //! number of frames (multiplied by chns for the number of samples)
        int frames;
#if FLEXT_SYS == FLEXT_SYS_PD
        //! pointer to the PD array structure
        t_garray *arr;
        //! update interval
        float interval;
        //! flag signaling that the data has been changed
        bool isdirty;
        //! flag showing that the update clock is active
        bool ticking;
        //! update clock
        t_clock *tick;
        //! last time the dirty flag was cleared (using the clock_getlogicaltime function)
        double cleantime;

    private:
        //! update clock callback
        static void cb_tick(buffer *b);
#elif FLEXT_SYS == FLEXT_SYS_MAX
        //! last time the dirty flag was cleared (using the gettime function)
        long cleantime;
#endif
    };


//!     @} FLEXT_S_BUFFER

// --- utilities --------------------------------------------------

    /*! \defgroup FLEXT_S_UTIL Utility functions
        @{ 
    */

    //! Copy an atom
    static void CopyAtom(t_atom *dst,const t_atom *src) { *dst = *src; }

    //! Copy atoms
    static void CopyAtoms(int cnt,t_atom *dst,const t_atom *src);

    //! Print an atom
    static bool PrintAtom(const t_atom &a,char *buf,size_t bufsz);

    /*! Scan an atom until whitespace
        \return next token position, or NULL on failure
    */
    static const char *ScanAtom(t_atom &a,const char *buf);

    //! Copy a list of atoms
    static t_atom *CopyList(int argc,const t_atom *argv);
    
    //! Print an atom list
    static bool PrintList(int argc,const t_atom *argv,char *buf,size_t bufsz);
    
    /*! Scan an atom list
        \param argc ... maximum amount of atoms scanned
        \param argv ... array of atoms
        \param buf ... char buffer
    */
    static int ScanList(int argc,t_atom *argv,const char *buf);

    //! Copy a memory region
    static void CopyMem(void *dst,const void *src,int bytes);
    //! Copy a sample array
    static void CopySamples(t_sample *dst,const t_sample *src,int cnt);
    static void CopySamples(buffer::Element *dst,const buffer::Element *src,int cnt) { CopyMem(dst,src,sizeof(*src)*cnt); }
    //! Set a memory region
    static void ZeroMem(void *dst,int bytes);
    //! Set a sample array to a fixed value
    static void SetSamples(t_sample *dst,int cnt,t_sample s);
    static void SetSamples(buffer::Element *dst,int cnt,t_sample s) { for(int i = 0; i < cnt; ++i) dst[i] = s; }
    //! Set a sample array to 0
    static void ZeroSamples(t_sample *dst,int cnt) { SetSamples(dst,cnt,0); }   
    static void ZeroSamples(buffer::Element *dst,int cnt) { ZeroMem(dst,sizeof(*dst)*cnt); }   


    //! Get a 32 bit hash value from an atom
    static unsigned long AtomHash(const t_atom &a);

//!     @} FLEXT_S_UTIL

// --- various symbols --------------------------------------------

    /*! \defgroup FLEXT_S_ATOM Atom/list handling
        @{ 
    */

    //! Symbol constant for ""
    static const t_symbol *sym__;
    //! Symbol constant for "float"
    static const t_symbol *sym_float;
    //! Symbol constant for "symbol"
    static const t_symbol *sym_symbol;
    //! Symbol constant for "bang"
    static const t_symbol *sym_bang;
    //! Symbol constant for "list"
    static const t_symbol *sym_list;
    //! Symbol constant for "anything"
    static const t_symbol *sym_anything;

    /*! \brief Symbol constant for "int"
        \note Only the Max/MSP system has this defined as an internal type
    */
    static const t_symbol *sym_int;

    /*! Symbol constant for "pointer" 
        \note Only PD has this defined as an internal type
    */
    static const t_symbol *sym_pointer;

    //! Symbol constant for "signal"
    static const t_symbol *sym_signal;

    //! \note This is used in macros where the type of the arg is not clear
    static const t_symbol *MakeSymbol(const t_symbol *s) { return s; }

    //! Make a symbol from a string
    static const t_symbol *MakeSymbol(const char *s) { return ::gensym(const_cast<char *>(s)); }
    //! Get symbol string
    static const char *GetString(const t_symbol *s) { return s->s_name; }  
    //! Check for symbol and get string
    static const char *GetAString(const t_symbol *s,const char *def = NULL) { return s?GetString(s):def; }

// --- atom stuff ----------------------------------------
        
    //! Set atom from another atom
    static void SetAtom(t_atom &a,const t_atom &b) { CopyAtom(&a,&b); }
    //! Compare two atoms
    static int CmpAtom(const t_atom &a,const t_atom &b);

    // there are some more comparison functions for t_atom types outside the class

    //! Set atom from another atom
    static int GetType(const t_atom &a) { return a.a_type; }

    //! Check whether the atom is nothing
    static bool IsNothing(const t_atom &a) { return a.a_type == A_NULL; }
    //! Set the atom to represent nothing
    static void SetNothing(t_atom &a) { a.a_type = A_NULL; }

    //! Check whether the atom is a float
    static bool IsFloat(const t_atom &a) { return a.a_type == A_FLOAT; }

    //! Check whether the atom can be represented as a float
    static bool CanbeFloat(const t_atom &a) { return IsFloat(a) || IsInt(a); }

    //! Access the float value (without type check)
    static float GetFloat(const t_atom &a) { return a.a_w.w_float; }
    //! Set the atom to represent a float 
    static void SetFloat(t_atom &a,float v) { a.a_type = A_FLOAT; a.a_w.w_float = v; }

    //! Check whether the atom is a symbol
    static bool IsSymbol(const t_atom &a) { return a.a_type == A_SYMBOL; }

#if FLEXT_SYS == FLEXT_SYS_PD
    //! Access the symbol value (without type check)
    static const t_symbol *GetSymbol(const t_atom &a) { return const_cast<const t_symbol *>(a.a_w.w_symbol); }
    //! Set the atom to represent a symbol
    static void SetSymbol(t_atom &a,const t_symbol *s) { a.a_type = A_SYMBOL; a.a_w.w_symbol = const_cast<t_symbol *>(s); }
#elif FLEXT_SYS == FLEXT_SYS_MAX
    //! Access the symbol value (without type check)
    static const t_symbol *GetSymbol(const t_atom &a) { return const_cast<const t_symbol *>(a.a_w.w_sym); }
    //! Set the atom to represent a symbol
    static void SetSymbol(t_atom &a,const t_symbol *s) { a.a_type = A_SYMBOL; a.a_w.w_sym = const_cast<t_symbol *>(s); }
#else
#error
#endif
    //! Check for a symbol and get its value 
    static const t_symbol *GetASymbol(const t_atom &a,const t_symbol *def = NULL) { return IsSymbol(a)?GetSymbol(a):def; }  // NULL or empty symbol?

    //! Check whether the atom is a string
    static bool IsString(const t_atom &a) { return IsSymbol(a); }
    //! Access the string value (without type check)
    static const char *GetString(const t_atom &a) { const t_symbol *s = GetSymbol(a); return s?GetString(s):NULL; }  
    //! Check for a string and get its value 
    static const char *GetAString(const t_atom &a,const char *def = NULL) { return IsSymbol(a)?GetAString(GetSymbol(a),def):def; }
    //! Check for a string and get its value 
    static void GetAString(const t_atom &a,char *buf,size_t szbuf);
    //! Set the atom to represent a string
    static void SetString(t_atom &a,const char *c) { SetSymbol(a,MakeSymbol(c)); }

    //! Check whether the atom can be represented as an integer
    static bool CanbeInt(const t_atom &a) { return IsFloat(a) || IsInt(a); }

#if FLEXT_SYS == FLEXT_SYS_PD
    //! Check for a float and get its value 
    static float GetAFloat(const t_atom &a,float def = 0) { return IsFloat(a)?GetFloat(a):def; }

    //! Check whether the atom is an integer
    static bool IsInt(const t_atom &) { return false; }
    //! Access the integer value (without type check)
    static int GetInt(const t_atom &a) { return (int)GetFloat(a); }
    //! Check for an integer and get its value 
    static int GetAInt(const t_atom &a,int def = 0) { return (int)GetAFloat(a,(float)def); }
    //! Set the atom to represent a integer (depending on the system)
    static void SetInt(t_atom &a,int v) { a.a_type = A_FLOAT; a.a_w.w_float = (float)v; }

#ifndef FLEXT_COMPATIBLE
    //! Check whether the atom strictly is a pointer
    static bool IsPointer(const t_atom &a) { return a.a_type == A_POINTER; }
    //! Check whether the atom can be a pointer
    static bool CanbePointer(const t_atom &a) { return IsPointer(a); }
    //! Access the pointer value (without type check)
    static t_gpointer *GetPointer(const t_atom &a) { return a.a_w.w_gpointer; }
    //! Check for a pointer and get its value 
    static t_gpointer *GetAPointer(const t_atom &a,t_gpointer *def = NULL) { return IsPointer(a)?GetPointer(a):def; }
    //! Set the atom to represent a pointer
    static void SetPointer(t_atom &a,t_gpointer *p) { a.a_type = A_POINTER; a.a_w.w_gpointer = (t_gpointer *)p; }
#endif

#elif FLEXT_SYS == FLEXT_SYS_MAX
    //! Check for a float and get its value 
    static float GetAFloat(const t_atom &a,float def = 0) { return IsFloat(a)?GetFloat(a):(IsInt(a)?GetInt(a):def); }

    //! Check whether the atom is an int
    static bool IsInt(const t_atom &a) { return a.a_type == A_INT; }
    //! Access the integer value (without type check)
    static int GetInt(const t_atom &a) { return a.a_w.w_long; }
    //! Check for an integer and get its value 
    static int GetAInt(const t_atom &a,int def = 0) { return IsInt(a)?GetInt(a):(IsFloat(a)?(int)GetFloat(a):def); }
    //! Set the atom to represent an integer
    static void SetInt(t_atom &a,int v) { a.a_type = A_INT; a.a_w.w_long = v; }
#else
#error "Platform not supported"
#endif

    // bool type - based on int

    //! Set the atom to represent a boolean
    static void SetBool(t_atom &a,bool v) { SetInt(a,v?1:0); }
    //! Check whether the atom can be represented as a boolean
    static bool CanbeBool(const t_atom &a) { return CanbeInt(a); }
    //! Check for an boolean and get its value 
    static bool GetABool(const t_atom &a) { return GetAInt(a) != 0; }
    //! Check for an boolean and get its value 
    static bool GetBool(const t_atom &a) { return GetInt(a) != 0; }

// --- atom list stuff -------------------------------------------

    //! Class representing a list of atoms
    class FLEXT_SHARE AtomList
        : public flext_root
    {
    public:
        //! Construct list
        AtomList(): cnt(0),lst(NULL) {}
        //! Construct list
        explicit AtomList(int argc,const t_atom *argv = NULL): cnt(0),lst(NULL) { operator()(argc,argv); }
        //! Construct list
        AtomList(const AtomList &a): cnt(0),lst(NULL) { operator =(a); }
        //! Destroy list
        virtual ~AtomList();

        //! Clear list
        AtomList &Clear() { return operator()(); }

        //! Set list
        AtomList &Set(int argc,const t_atom *argv,int offs = 0,bool resize = false);
        //! Get list
        int Get(t_atom *argv,int mxsz = -1) const;

        //! Set list
        AtomList &operator()(int argc = 0,const t_atom *argv = NULL) { return Set(argc,argv,0,true); }
        //! Set list by another AtomList
        AtomList &operator =(const AtomList &a) { return operator()(a.Count(),a.Atoms()); }

        //! Compare list to another AtomList ( -1..< , 0..==, 1...> )
        int Compare(const AtomList &a) const;

        bool operator <(const AtomList &a) const { return Compare(a) < 0; }
        bool operator <=(const AtomList &a) const { return Compare(a) <= 0; }
        bool operator >(const AtomList &a) const { return Compare(a) > 0; }
        bool operator >=(const AtomList &a) const { return Compare(a) >= 0; }
        bool operator ==(const AtomList &a) const { return Compare(a) == 0; }
        bool operator !=(const AtomList &a) const { return Compare(a) != 0; }

        //! Get number of atoms in the list
        int Count() const { return cnt; }
        //! Get a reference to an indexed atom
        t_atom &operator [](int ix) { return lst[ix]; }
        //! Get a reference to an indexed atom
        const t_atom &operator [](int ix) const { return lst[ix]; }

        //! Get a pointer to the list of atoms
        t_atom *Atoms() { return lst; }
        //! Get a pointer to the list of atoms
        const t_atom *Atoms() const { return lst; }

        //! Append an atom list to the list
        AtomList &Append(int argc,const t_atom *argv = NULL)
        {
            int c = Count();
            Alloc(c+argc,0,c);
            Set(argc,argv,c);
            return *this;
        }

        //! Prepend an atom list to the list
        AtomList &Prepend(int argc,const t_atom *argv = NULL)
        {
            int c = Count();
            Alloc(c+argc,0,c,argc);
            Set(argc,argv);
            return *this;
        }

        //! Append an atom to the list
        AtomList &Append(const t_atom &a) { return Append(1,&a); }
        //! Append an atom list to the list
        AtomList &Append(const AtomList &a) { return Append(a.Count(),a.Atoms()); }
        //! Prepend an atom to the list
        AtomList &Prepend(const t_atom &a) { return Prepend(1,&a); }
        //! Prepend an atom list to the list
        AtomList &Prepend(const AtomList &a) { return Prepend(a.Count(),a.Atoms()); }

        //! Get a part of the list
        void GetPart(int offs,int len,AtomList &ret) const;
        //! Set to a part of the list
        AtomList &Part(int offs,int len) { GetPart(offs,len,*this); return *this; }

        //! Represent as a string
        bool Print(char *buffer,int buflen) const { return flext::PrintList(Count(),Atoms(),buffer,buflen); }

        /*! Read from string
            \note: doesn't clear or reallocate the list
        */
        int Scan(const char *buffer) { return flext::ScanList(Count(),Atoms(),buffer); }

    protected:
        virtual void Alloc(int sz,int keepix = -1,int keeplen = -1,int keepto = 0);
        virtual void Free();

        int cnt;
        t_atom *lst;
    };

    class FLEXT_SHARE AtomListStaticBase
        : public AtomList
    {
    protected:
        explicit AtomListStaticBase(int pc,t_atom *dt): precnt(pc),predata(dt) {}
        virtual ~AtomListStaticBase();
        virtual void Alloc(int sz,int keepix = -1,int keeplen = -1,int keepto = 0);
        virtual void Free();

        AtomListStaticBase &operator =(const AtomList &a) { AtomList::operator =(a); return *this; }
        AtomListStaticBase &operator =(const AtomListStaticBase &a) { AtomList::operator =(a); return *this; }

        const int precnt;
        t_atom *const predata;
    };

    template<int PRE>
    class AtomListStatic
        : public AtomListStaticBase
    {
    public:
        //! Construct list
        explicit AtomListStatic(): AtomListStaticBase(PRE,pre) {}
        //! Construct list
        explicit AtomListStatic(int argc,const t_atom *argv = NULL): AtomListStaticBase(PRE,pre) { operator()(argc,argv); }
        //! Construct list
        explicit AtomListStatic(const AtomList &a): AtomListStaticBase(PRE,pre) { operator =(a); }

        //! Set list by another AtomList
        AtomListStatic &operator =(const AtomList &a) { AtomListStaticBase::operator =(a); return *this; }
        AtomListStatic &operator =(const AtomListStatic &a) { AtomListStaticBase::operator =(a); return *this; }
    protected:
        t_atom pre[PRE];
    };

    //! Class representing an "anything"
    class FLEXT_SHARE AtomAnything: 
        public AtomList
    {
    public:
        explicit AtomAnything(): hdr(NULL) {}

        //! Construct anything
        explicit AtomAnything(const t_symbol *h,int argc = 0,const t_atom *argv = NULL)
            : AtomList(argc,argv),hdr(h?h:sym__) 
        {}

        //! Construct anything
        explicit AtomAnything(const char *h,int argc = 0,const t_atom *argv = NULL)
            : AtomList(argc,argv),hdr(MakeSymbol(h)) 
        {}

        //! Construct anything
        AtomAnything(const AtomAnything &a)
            : AtomList(a),hdr(a.hdr) 
        {}

        //! Clear anything
        AtomAnything &Clear() { return operator()(); }

        //! Get header symbol of anything
        const t_symbol *Header() const { return hdr; }

        //! Set header symbol of anything
        void Header(const t_symbol *h) { hdr = h; }
        
        //! Set anything
        AtomAnything &operator()(const t_symbol *h = NULL,int argc = 0,const t_atom *argv = NULL)
        { 
            hdr = h; AtomList::operator()(argc,argv);   
            return *this; 
        }

        //! Set list by another AtomAnything
        AtomAnything &operator =(const AtomAnything &a) { return operator()(a.Header(),a.Count(),a.Atoms()); }

    protected:
        const t_symbol *hdr;
    };


    // double type - based on two floats

#ifdef _MSC_VER
#pragma optimize("p",off)  // improve floating point precision consistency
#endif
    static t_atom *SetDouble(t_atom *dbl,double d)
    {
        float f = static_cast<float>(d);
        float r = static_cast<float>(d-f);
        SetFloat(dbl[0],f);
        SetFloat(dbl[1],r);
        return dbl;
    }
#ifdef _MSC_VER
#pragma optimize("p",on)
#endif

    static double GetDouble(int argc,const t_atom *argv)
    {
        double d = argc >= 1?GetAFloat(argv[0]):0;
        return argc >= 2?d+GetAFloat(argv[1]):d;
    }

    static AtomList &SetDouble(AtomList &l,double d) { SetDouble(l(2).Atoms(),d); return l; }

    static double GetDouble(const AtomList &l) { return GetDouble(l.Count(),l.Atoms()); }

    //! @} FLEXT_S_ATOM


// --- messages ------------------------------------------------------- 

    /*! \defgroup FLEXT_S_MSGBUNDLE Flext message handling 
        @{ 
    */

    class MsgBundle;

    //! Make new message bundle
    static MsgBundle *MsgNew();

    //! Destroy message bundle
    static void MsgFree(MsgBundle *mb);

    //! Send (and destroy) message bundle
    static void ToSysMsg(MsgBundle *mb);

    //! Send (and destroy) message bundle
    static void ToOutMsg(MsgBundle *mb);

    //! Send low priority (and destroy) message bundle
    static void ToQueueMsg(MsgBundle *mb);

    //! @} FLEXT_S_MSGBUNDLE


    /*! \defgroup FLEXT_S_MSG Flext message handling 
        @{ 
    */

    static bool Forward(const t_symbol *sym,const t_symbol *s,int argc,const t_atom *argv);
    static bool Forward(const t_symbol *sym,const AtomAnything &args) { return Forward(sym,args.Header(),args.Count(),args.Atoms()); }
    static bool Forward(const char *sym,const AtomAnything &args) { return Forward(MakeSymbol(sym),args.Header(),args.Count(),args.Atoms()); }
    static bool Forward(const t_symbol *sym,int argc,const t_atom *argv) { return Forward(sym,sym_list,argc,argv); }
    static bool Forward(const t_symbol *sym,const AtomList &args) { return Forward(sym,args.Count(),args.Atoms()); }
    static bool Forward(const char *sym,const AtomList &args) { return Forward(MakeSymbol(sym),args.Count(),args.Atoms()); }

    static bool SysForward(const t_symbol *sym,const t_symbol *s,int argc,const t_atom *argv);
    static bool SysForward(const t_symbol *sym,const AtomAnything &args) { return SysForward(sym,args.Header(),args.Count(),args.Atoms()); }
    static bool SysForward(const char *sym,const AtomAnything &args) { return SysForward(MakeSymbol(sym),args.Header(),args.Count(),args.Atoms()); }
    static bool SysForward(const t_symbol *sym,int argc,const t_atom *argv) { return SysForward(sym,sym_list,argc,argv); }
    static bool SysForward(const t_symbol *sym,const AtomList &args) { return SysForward(sym,args.Count(),args.Atoms()); }
    static bool SysForward(const char *sym,const AtomList &args) { return SysForward(MakeSymbol(sym),args.Count(),args.Atoms()); }

    static bool QueueForward(const t_symbol *sym,const t_symbol *s,int argc,const t_atom *argv);
    static bool QueueForward(const t_symbol *sym,const AtomAnything &args) { return QueueForward(sym,args.Header(),args.Count(),args.Atoms()); }
    static bool QueueForward(const char *sym,const AtomAnything &args) { return QueueForward(MakeSymbol(sym),args.Header(),args.Count(),args.Atoms()); }
    static bool QueueForward(const t_symbol *sym,int argc,const t_atom *argv) { return QueueForward(sym,sym_list,argc,argv); }
    static bool QueueForward(const t_symbol *sym,const AtomList &args) { return QueueForward(sym,args.Count(),args.Atoms()); }
    static bool QueueForward(const char *sym,const AtomList &args) { return QueueForward(MakeSymbol(sym),args.Count(),args.Atoms()); }

    static bool MsgForward(MsgBundle *mb,const t_symbol *sym,const t_symbol *s,int argc,const t_atom *argv);
    static bool MsgForward(MsgBundle *mb,const t_symbol *sym,const AtomAnything &args) { return MsgForward(mb,sym,args.Header(),args.Count(),args.Atoms()); }
    static bool MsgForward(MsgBundle *mb,const char *sym,const AtomAnything &args) { return MsgForward(mb,MakeSymbol(sym),args.Header(),args.Count(),args.Atoms()); }
    static bool MsgForward(MsgBundle *mb,const t_symbol *sym,int argc,const t_atom *argv) { return MsgForward(mb,sym,sym_list,argc,argv); }
    static bool MsgForward(MsgBundle *mb,const t_symbol *sym,const AtomList &args) { return MsgForward(mb,sym,args.Count(),args.Atoms()); }
    static bool MsgForward(MsgBundle *mb,const char *sym,const AtomList &args) { return MsgForward(mb,MakeSymbol(sym),args.Count(),args.Atoms()); }

    //! @} FLEXT_S_MSG

    

// --- thread stuff -----------------------------------------------

    /*! \defgroup FLEXT_S_LOCK Global system locking
        @{ 
    */

#if FLEXT_SYS == FLEXT_SYS_PD
    #if PD_MINOR_VERSION >= 38 || (PD_MINOR_VERSION >= 37 && defined(PD_DEVEL_VERSION))
        static void Lock() { sys_lock(); }
        static void Unlock() { sys_unlock(); }
    #else
        // no system locking for old PD versions
        static void Lock() {}
        static void Unlock() {}
    #endif
#elif FLEXT_SYS == FLEXT_SYS_MAX
    // Max 4.2 upwards!
    static void Lock() { critical_enter(0); }
    static void Unlock() { critical_exit(0); }
#else
#error
#endif

//!     @} FLEXT_S_LOCK

    /*! \defgroup FLEXT_S_THREAD Flext thread handling 
        @{ 
    */

    //! Check if current thread is registered to be a secondary thread
#ifdef FLEXT_THREADS
    static bool IsThreadRegistered();
#else
    static bool IsThreadRegistered() { return false; }
#endif

#ifdef FLEXT_THREADS

    //! thread type
#   if FLEXT_THREADS == FLEXT_THR_MP
    typedef MPTaskID thrid_t;
#   elif FLEXT_THREADS == FLEXT_THR_POSIX
    typedef pthread_t thrid_t;
#   elif FLEXT_THREADS == FLEXT_THR_WIN32
    typedef DWORD thrid_t;
#   else
#       error Threading model not supported
#   endif

    /*! \brief Get current thread id
    */
    static thrid_t GetThreadId() { 
#if FLEXT_THREADS == FLEXT_THR_POSIX
        return pthread_self(); 
#elif FLEXT_THREADS == FLEXT_THR_MP
        return MPCurrentTaskID();
#elif FLEXT_THREADS == FLEXT_THR_WIN32
        return GetCurrentThreadId();
#else
#error
#endif
    }

    /*! \brief Get system thread id
    */
    static thrid_t GetSysThreadId() { return thrid; }

    //! Check if current thread should terminate
    static bool ShouldExit();

    //! Check if current thread is the realtime system's thread
    static bool IsThread(thrid_t t,thrid_t ref = GetThreadId()) { 
#if FLEXT_THREADS == FLEXT_THR_POSIX
        return pthread_equal(ref,t) != 0; 
#else
        return ref == t;
#endif
    }


    /*! \brief Thread parameters
        \internal
    */
    class FLEXT_SHARE thr_params:
        public flext_root
    {
    public:
        thr_params(int n = 1);
        ~thr_params();

        void set_any(const t_symbol *s,int argc,const t_atom *argv);
        void set_list(int argc,const t_atom *argv);

        FLEXT_CLASSDEF(flext_base) *cl;
        union _data {
            bool _bool;
            float _float;
            int _int;
            t_symptr _t_symptr;
            AtomAnything *_any;
            AtomList *_list;
            void *_ext;
        } *var;
    };

protected:

    static thrid_t thrhelpid;
    static thrid_t thrmsgid;
    static void ThrHelper(void *);

    //! the system's thread id
    static thrid_t thrid;  // the system thread

private:
    static bool StartHelper(); // used in flext::Setup()

public:

    /*! \brief Yield to other threads
        \remark A call to this is only needed for systems with cooperative multitasking like MacOS<=9
    */
    static void ThrYield() { 
#if FLEXT_THREADS == FLEXT_THR_POSIX
        // for a preemptive system this should do nothing
        sched_yield(); 
#elif FLEXT_THREADS == FLEXT_THR_MP
        MPYield();
#elif FLEXT_THREADS == FLEXT_THR_WIN32
        SwitchToThread();
#else
#error
#endif
    }

    /*! \brief Query whether task is preemptive
    */
    static bool IsThreadPreemptive(thrid_t t = GetThreadId()) {
#if FLEXT_THREADS == FLEXT_THR_POSIX || FLEXT_THREADS == FLEXT_THR_WIN32
        return true;
#elif FLEXT_THREADS == FLEXT_THR_MP
        return MPTaskIsPreemptive(t);
#else
#error
#endif
    }
    

    /*! \brief Increase/Decrease priority of a thread
    */
    static bool RelPriority(int dp,thrid_t ref = GetSysThreadId(),thrid_t thr = GetThreadId());

    /*! \brief Get priority of a thread
    */
    static int GetPriority(thrid_t thr = GetThreadId());

    /*! \brief Set priority of a thread
    */
    static bool SetPriority(int p,thrid_t thr = GetThreadId());

    /*! \brief Thread mutex
        \sa pthreads documentation
    */
    class FLEXT_SHARE ThrMutex:
        public flext_root
#if FLEXT_THREADS == FLEXT_THR_POSIX
    {
    public:
        //! Construct thread mutex
        ThrMutex() { pthread_mutex_init(&mutex,NULL); }
        //! Destroy thread mutex
        ~ThrMutex() { pthread_mutex_destroy(&mutex); }

        //! Lock thread mutex
        bool Lock() { return pthread_mutex_lock(&mutex) == 0; }
        /*! Wait to lock thread mutex.
            \todo Implement!
        */
//      bool WaitForLock(double tm) { return pthread_mutex_lock(&mutex) == 0; }
        //! Try to lock, but don't wait
        bool TryLock() { return pthread_mutex_trylock(&mutex) == 0; }
        //! Unlock thread mutex
        bool Unlock() { return pthread_mutex_unlock(&mutex) == 0; }

    protected:
        pthread_mutex_t mutex;
//      int cnt;
    };
#elif FLEXT_THREADS == FLEXT_THR_WIN32
    {
    public:
        //! Construct thread mutex
        ThrMutex() { ::InitializeCriticalSection(&mutex); }
        //! Destroy thread mutex
        ~ThrMutex() { ::DeleteCriticalSection(&mutex); }

        //! Lock thread mutex
        bool Lock() { ::EnterCriticalSection(&mutex); return true; }
        /*! Wait to lock thread mutex.
            \todo Implement!
        */
//      bool WaitForLock(double tm) { return pthread_mutex_lock(&mutex) == 0; }
        //! Try to lock, but don't wait
        bool TryLock() { return ::TryEnterCriticalSection(&mutex) != 0; }
        //! Unlock thread mutex
        bool Unlock() { ::LeaveCriticalSection(&mutex); return true; }

    protected:
        CRITICAL_SECTION mutex;
    };
#elif FLEXT_THREADS == FLEXT_THR_MP
    {
    public:
        //! Construct thread mutex
        ThrMutex() { MPCreateCriticalRegion(&crit); }
        //! Destroy thread mutex
        ~ThrMutex() { MPDeleteCriticalRegion(crit); }

        //! Lock thread mutex
        bool Lock() { return MPEnterCriticalRegion(crit,kDurationForever) == noErr; }
        //! Wait to lock thread mutex
//      bool WaitForLock(double tm) { return MPEnterCriticalRegion(crit,tm*kDurationMicrosecond*1.e6) == noErr; }
        //! Try to lock, but don't wait
        bool TryLock() { return MPEnterCriticalRegion(crit,kDurationImmediate) == noErr; }
        //! Unlock thread mutex
        bool Unlock() { return MPExitCriticalRegion(crit) == noErr; }
        
    protected:
        MPCriticalRegionID crit;
    };
#else
#error "Not implemented"
#endif

    /*! \brief Thread conditional
        \sa pthreads documentation
    */
    class FLEXT_SHARE ThrCond
#if FLEXT_THREADS == FLEXT_THR_POSIX
        :public ThrMutex
    {
    public:
        //! Construct thread conditional
        ThrCond() { pthread_cond_init(&cond,NULL); }
        //! Destroy thread conditional
        ~ThrCond() { pthread_cond_destroy(&cond); }

        //! Wait for condition 
        bool Wait();

        /*! Wait for condition (for a certain time).
            \param ftime Wait time in seconds
            \ret true = signalled, false = timed out 
            \remark If ftime = 0 this may suck away your cpu if used in a signalled loop.
            \remark The time resolution of the implementation is required to be at least ms.
        */
        bool TimedWait(double ftime);

        //! Signal condition
        bool Signal() { return pthread_cond_signal(&cond) == 0; }

    protected:
        pthread_cond_t cond;
    };
#elif FLEXT_THREADS == FLEXT_THR_WIN32
    {
    public:
        //! Construct thread conditional
        ThrCond() { cond = CreateEvent(NULL,FALSE,FALSE,NULL); }
        //! Destroy thread conditional
        ~ThrCond() { CloseHandle(cond); }

        //! Wait for condition 
        bool Wait() { return WaitForSingleObject(cond,INFINITE) == WAIT_OBJECT_0; }

        /*! Wait for condition (for a certain time).
            \param ftime Wait time in seconds
            \ret true = signalled, false = timed out 
            \remark If ftime = 0 this may suck away your cpu if used in a signalled loop.
            \remark The time resolution of the implementation is required to be at least ms.
        */
        bool TimedWait(double ftime) { return WaitForSingleObject(cond,(LONG)(ftime*1000)) == WAIT_OBJECT_0; }

        //! Signal condition
        bool Signal() { return SetEvent(cond) != 0; }

    protected:
        HANDLE cond;
    };
#elif FLEXT_THREADS == FLEXT_THR_MP
    {
    public:
        //! Construct thread conditional
        ThrCond() { MPCreateEvent(&ev); }
        //! Destroy thread conditional
        ~ThrCond() { MPDeleteEvent(ev); }

        //! Wait for condition 
        bool Wait() { return MPWaitForEvent(ev,NULL,kDurationForever) == noErr; }

        /*! \brief Wait for condition (for a certain time).
            \param time Wait time in seconds
        */
        bool TimedWait(double tm) { return MPWaitForEvent(ev,NULL,tm*kDurationMicrosecond*1.e6) == noErr; }

        //! Signal condition
        bool Signal() { return MPSetEvent(ev,1) == noErr; } // one bit needs to be set at least

    protected:
        MPEventID ev;
    };
#else
#error "Not implemented"
#endif

    protected:
    /*! \brief Add current thread to list of active threads.
        \note Calls RegisterThread automatically
        \return true on success
        \internal
    */
    static bool PushThread();

    /*! \brief Remove current thread from list of active threads.
        \note Calls UnregisterThread automatically
        \internal
    */
    static void PopThread();

    public:
    /*! \brief Launch a thread.
        \param meth Thread function
        \param params Parameters to pass to the thread, may be NULL if not needed.
        \return Thread id on success, NULL on failure
    */
    static bool LaunchThread(void (*meth)(thr_params *p),thr_params *params = NULL);

    /*! \brief Terminate a thread.
        \param meth Thread function
        \param params Parameters to pass to the thread, may be NULL if not needed.
        \return True if at least one matching thread has been found.
        \remark Terminates all running threads with matching meth and params.
        \note Function doesn NOT wait for termination
    */
    static bool StopThread(void (*meth)(thr_params *p),thr_params *params = NULL,bool wait = false);


    //! \brief Register current thread to be allowed to execute flext functions.
    static void RegisterThread(thrid_t id = GetThreadId());

    //! \brief Unregister current thread 
    static void UnregisterThread(thrid_t id = GetThreadId());

#endif // FLEXT_THREADS

//!     @} FLEXT_S_THREAD


    public:
// --- timer stuff -----------------------------------------------

/*! \defgroup FLEXT_S_TIMER Flext timer handling 
        @{ 
        
    \remark The clock of the real-time system is used for most of these functions. 
    \remark Since this clock can be synchronized to an external clock (or e.g. the audio card) 
    \remark it may differ from the clock of the operating system
*/

    /*! \brief Get time since real-time system startup.
        \note This is not the time of the operating system but of the real-time system.
        \note It may depend on the time source the system is synchronized to (e.g. audio sample rate).
    */
    static double GetTime()
    {
    #if FLEXT_SYS == FLEXT_SYS_PD
        return clock_gettimesince(0)*0.001;
    #elif FLEXT_SYS == FLEXT_SYS_MAX
        double tm;
        clock_getftime(&tm);
        return tm*0.001;
    #else
        #error Not implemented
    #endif
    }
    
    /*! \brief Get time granularity of the GetTime function.
        \note This can be zero if not determined.
    */
    static double GetTimeGrain()
    {
    #if FLEXT_SYS == FLEXT_SYS_PD
        return 0;
    #elif FLEXT_SYS == FLEXT_SYS_MAX
        return 0.001;
    #else
        #error Not implemented
    #endif
    }

    /*! \brief Get operating system time since flext startup.
    */
    static double GetOSTime();
    
    /*! \brief Sleep for an amount of time.
        \remark The OS clock is used for that.
        \note Clearly in a real-time system this should only be used in a detached thread.
    */
    static void Sleep(double s);

    /*! \brief Class encapsulating a timer with callback functionality.
        This class can either be used with FLEXT_ADDTIMER or used as a base class with an overloaded virtual Work function.
    */ 
    class FLEXT_SHARE Timer:
        public flext_root
    {
    public:
        Timer(bool queued = false);
        virtual ~Timer();

        //! Set timer callback function.
        void SetCallback(void (*cb)(void *data)) { clss = NULL,cback = cb; }
        //! Set timer callback function (with class pointer).
        void SetCallback(FLEXT_CLASSDEF(flext_base) &th,bool (*cb)(FLEXT_CLASSDEF(flext_base) *th,void *data)) { clss = &th,cback = (void (*)(void *))cb; }

        //! Clear timer.
        bool Reset();
        //! Trigger a one shot at an absolute time.
        bool At(double time,void *data = NULL,bool dopast = true);
        //! Trigger a one shot interval.
        bool Delay(double time,void *data = NULL);
        //! Trigger a periodic interval.
        bool Periodic(double time,void *data = NULL);
        //! Trigger immediately.
        bool Now(void *data = NULL) { return Delay(0,data); }

        //! Worker function, called on every timer event.
        virtual void Work();
        
    protected:
        static void callback(Timer *tmr);
    
#if FLEXT_SYS == FLEXT_SYS_PD
        t_clock *clk;
#elif FLEXT_SYS == FLEXT_SYS_MAX
        static void queuefun(Timer *tmr);
        t_clock *clk;
        t_qelem *qelem;
#else
#error Not implemented
#endif

        const bool queued;
        void (*cback)(void *data);
        FLEXT_CLASSDEF(flext_base) *clss;
        void *userdata;
        double period;
    };

//!     @} FLEXT_S_TIMER

    //! Check if we are in DSP time
    static bool InDSP() { return indsp; }

// --- SIMD functionality -----------------------------------------------

/*! \defgroup FLEXT_S_SIMD Cross platform SIMD support for modern CPUs 
        @{ 
*/      
        enum simd_type {
            simd_none = 0,
            simd_mmx = 0x01,
            simd_3dnow = 0x02,
            simd_sse = 0x04,
            simd_sse2 = 0x08,
            simd_altivec = 0x10
        };
        
        /*! Check for SIMD capabilities of the CPU */
        static unsigned long GetSIMDCapabilities();


        static void MulSamples(t_sample *dst,const t_sample *src,t_sample mul,int cnt);
        static void MulSamples(t_sample *dst,const t_sample *src,const t_sample *mul,int cnt);
        static void AddSamples(t_sample *dst,const t_sample *src,t_sample add,int cnt);
        static void AddSamples(t_sample *dst,const t_sample *src,const t_sample *add,int cnt);
        static void ScaleSamples(t_sample *dst,const t_sample *src,t_sample mul,t_sample add,int cnt);
        static void ScaleSamples(t_sample *dst,const t_sample *src,t_sample mul,const t_sample *add,int cnt);
        static void ScaleSamples(t_sample *dst,const t_sample *src,const t_sample *mul,const t_sample *add,int cnt);

//!     @} FLEXT_S_SIMD

        
//!     @} FLEXT_SUPPORT

protected:
#ifdef __MRC__
    friend class flext_obj;
#endif

    static void Setup();

    static bool chktilde(const char *objname);

    static unsigned long simdcaps;

    static const t_symbol *sym_attributes;
    static const t_symbol *sym_methods;

#if FLEXT_SYS == FLEXT_SYS_MAX
    static const t_symbol *sym_buffer;
    static const t_symbol *sym_size;
    static const t_symbol *sym_dirty;
#endif

    //! flag if we are within DSP
    static bool indsp;
};


// gcc doesn't like these to be included into the flext class (even if static)
inline bool operator ==(const t_atom &a,const t_atom &b) { return flext::CmpAtom(a,b) == 0; }
inline bool operator !=(const t_atom &a,const t_atom &b) { return flext::CmpAtom(a,b) != 0; }
inline bool operator <(const t_atom &a,const t_atom &b) { return flext::CmpAtom(a,b) < 0; }
inline bool operator <=(const t_atom &a,const t_atom &b) { return flext::CmpAtom(a,b) <= 0; }
inline bool operator >(const t_atom &a,const t_atom &b) { return flext::CmpAtom(a,b) > 0; }
inline bool operator >=(const t_atom &a,const t_atom &b) { return flext::CmpAtom(a,b) >= 0; }

//! @} // FLEXT_SUPPORT

#include "flpopns.h"

#endif
