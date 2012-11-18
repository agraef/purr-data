/* 
py/pyext - python script object for PD and MaxMSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2008-10-31 18:47:47 -0400 (Fri, 31 Oct 2008) $
$LastChangedBy: thomas $
*/

#ifndef __PYBASE_H
#define __PYBASE_H

#include "main.h"
#include "pysymbol.h"
#include "pybuffer.h"
#include "pybundle.h"

#ifdef FLEXT_THREADS
#   ifdef PY_USE_GIL
        typedef PyGILState_STATE ThrState;
#   else
        typedef PyThreadState *ThrState;
#   endif
#else
    typedef int ThrState; // dummy
#endif

class pybase
    : public flext
{
public:
    pybase();
    virtual ~pybase();

    void Exit();

    static PyObject *MakePyArgs(const t_symbol *s,int argc,const t_atom *argv,int inlet = -1);
    static PyObject *MakePyArg(const t_symbol *s,int argc,const t_atom *argv);
    static const t_symbol *GetPyArgs(AtomList &lst,PyObject *pValue,int offs = 0);
    static const t_symbol *GetPyAtom(AtomList &lst,PyObject *pValue);

    static void lib_setup();

protected:

    virtual void DumpOut(const t_symbol *sym,int argc,const t_atom *argv) = 0;

    void m__dir(PyObject *obj);
    void m__doc(PyObject *obj);

    void m_dir() { m__dir(module); }
    void mg_dir(AtomList &lst) { m__dir(module); }
    void m_doc() { m__doc(dict); }

    std::string modname; // module name
    PyObject *module,*dict; // object module and associated dictionary

    static const char *py_doc;

    void GetDir(PyObject *obj,AtomList &lst);

    AtomList args;

    void AddCurrentPath(flext_base *o);
    void SetArgs();

    bool OutObject(flext_base *ext,int o,PyObject *obj);

    // reload module and all connected objects
    void Reload();

    bool ImportModule(const char *name);
    void UnimportModule();
    bool ReloadModule();

    // Get module registry
    PyObject *GetRegistry(const char *regname);
    // Set module registry
    void SetRegistry(const char *regname,PyObject *reg);

    // Register object
    void Register(PyObject *reg);
    // Unregister object
    void Unregister(PyObject *reg);

    virtual void LoadModule() = 0;
    virtual void UnloadModule() = 0;

    virtual void Load() = 0;
    virtual void Unload() = 0;

    void OpenEditor();

    void Respond(bool b)
    { 
        if(respond) { 
            t_atom a; 
            SetBool(a,b); 
            DumpOut(sym_response,1,&a); 
        } 
    }

    void Report() { while(PyErr_Occurred()) PyErr_Print(); }

    static bool IsAnything(const t_symbol *s) { return s && s != sym_float && s != sym_int && s != sym_symbol && s != sym_list && s != sym_pointer; }
    static bool IsAtom(const t_symbol *s) { return s == sym_float || s == sym_int || s == sym_symbol || s == sym_pointer; }

//  enum retval { nothing,atom,sequ };

    // --- module stuff -----

    static PyObject *module_obj,*module_dict;
    static PyObject *builtins_obj,*builtins_dict;
    static PyMethodDef func_tbl[],attr_tbl[];

    static PyObject *py__doc__(PyObject *,PyObject *args);
    static PyObject *py_send(PyObject *,PyObject *args);
#ifdef FLEXT_THREADS
    static PyObject *py_priority(PyObject *,PyObject *args);
#endif

    static PyObject *py_arraysupport(PyObject *,PyObject *args);
    static PyObject *py_samplerate(PyObject *,PyObject *args);
    static PyObject *py_blocksize(PyObject *,PyObject *args);

    static PyObject *py_searchpaths(PyObject *,PyObject *args);
    static PyObject *py_helppaths(PyObject *,PyObject *args);

#if FLEXT_SYS == FLEXT_SYS_PD
    static PyObject *py_getvalue(PyObject *,PyObject *args);
    static PyObject *py_setvalue(PyObject *,PyObject *args);
#endif

    static PyObject *py_list(PyObject *,PyObject *args);
    static PyObject *py_tuple(PyObject *,PyObject *args);

    // ----thread stuff ------------

    virtual void m_stop(int argc,const t_atom *argv);

    bool respond;
#ifdef FLEXT_THREADS
    int thrcount;
    bool shouldexit;
    int stoptick;
    Timer stoptmr;

    void tick(void *);
#endif

    int detach;
    bool pymsg;

    bool gencall(PyObject *fun,PyObject *args);

    bool docall(PyObject *fun,PyObject *args)
    {
        callpy(fun,args);
        if(PyErr_Occurred()) { 
            exchandle(); 
            return false; 
        }
        else 
            return true;
    }

    virtual void callpy(PyObject *fun,PyObject *args) = 0;

    void exchandle();

    static bool collect();

protected:

#ifdef FLEXT_THREADS
    static void thrworker(thr_params *data); 

    bool qucall(PyObject *fun,PyObject *args)
    {
        FifoEl *el = qufifo.New();
        el->Set(this,fun,args);
        qufifo.Put(el);
        qucond.Signal();
        return true;
    }

    static void quworker(thr_params *);
    static void pyworker(thr_params *);
    void erasethreads();

    static PyFifo qufifo;
    static ThrCond qucond;
    
#ifndef PY_USE_GIL
    static ThrState pythrsys;
#endif
#endif

    static const t_symbol *sym_fint; // float or int symbol, depending on native number message type
    static const t_symbol *sym_response;

    static const t_symbol *getone(t_atom &at,PyObject *arg);
    static const t_symbol *getlist(t_atom *lst,PyObject *seq,int cnt,int offs = 0);

public:

    static void AddToPath(const char *dir);

#ifdef FLEXT_THREADS
    // this is especially needed when one py/pyext object calls another one
    // we don't want the message to be queued, but otoh we have to avoid deadlock
    // (recursive calls can only happen in the system thread)
    static int lockcount;

#ifdef PY_USE_GIL
    static inline ThrState FindThreadState() { return ThrState(); }

    static inline ThrState PyLock(ThrState = ThrState()) { return PyGILState_Ensure(); }
    static inline ThrState PyLockSys() { return PyLock(); }
    static inline void PyUnlock(ThrState st) { PyGILState_Release(st); }
#else // PY_USE_GIL
    static ThrState FindThreadState();
    static void FreeThreadState();

    static ThrState PyLock(ThrState st = FindThreadState()) 
    { 
        if(st != pythrsys || !lockcount++) PyEval_AcquireLock();
        return PyThreadState_Swap(st);
    }

#if 1
    static inline ThrState PyLockSys() { return PyLock(); }
#else
    static ThrState PyLockSys() 
    { 
        if(!lockcount++) PyEval_AcquireLock();
        return PyThreadState_Swap(pythrsys);
    }
#endif

    static void PyUnlock(ThrState st) 
    {
        ThrState old = PyThreadState_Swap(st);
        if(old != pythrsys || !--lockcount) PyEval_ReleaseLock();
    }
#endif // PY_USE_GIL
    
#else // FLEXT_THREADS
    static inline ThrState PyLock(ThrState = NULL) { return NULL; }
    static inline ThrState PyLockSys() { return NULL; }
    static inline void PyUnlock(ThrState st) {}
#endif

    class ThrLock
    {
    public:
        ThrLock(): state(PyLock()) {}
        ThrLock(const ThrState &st): state(PyLock(st)) {}
        ThrLock(const ThrLock &t): state(PyLock(t.state)) {}
        ~ThrLock() { PyUnlock(state); }
        ThrState state;
    };

    class ThrLockSys
    {
    public:
        ThrLockSys(): state(PyLockSys()) {}
        ~ThrLockSys() { PyUnlock(state); }
        ThrState state;
    };

    static PyObject* StdOut_Write(PyObject* Self, PyObject* Args);
};

#endif
