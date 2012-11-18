/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#include "pybase.h"

class pyobj
    : public pybase
    , public flext_base
{
	FLEXT_HEADER_S(pyobj,flext_base,Setup)

public:
	pyobj(int argc,const t_atom *argv);
	~pyobj();

protected:
    virtual void Exit();

	virtual bool CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv);
    virtual void CbClick();

    void m_help();    

    void m_reload() { Reload(); }
    void m_reload_(int argc,const t_atom *argv) { args(argc,argv); Reload(); }
	void m_set(int argc,const t_atom *argv);
    void m_dir_() { m__dir(function); }
    void m_doc_() { m__doc(function); }

	const t_symbol *funname;
	PyObject *function;
    bool withfunction;

	virtual void LoadModule();
	virtual void UnloadModule();

	virtual void Load();
	virtual void Unload();

	bool SetFunction(const t_symbol *func);
	bool ResetFunction();

    virtual void DumpOut(const t_symbol *sym,int argc,const t_atom *argv);

    PyObject **objects;

private:

    virtual void callpy(PyObject *fun,PyObject *args);

	static void Setup(t_classid c);

	FLEXT_CALLBACK(m_help)
	FLEXT_CALLBACK(m_reload)
	FLEXT_CALLBACK_V(m_reload_)
	FLEXT_CALLBACK_V(m_set)
	FLEXT_CALLBACK(m_dir_)
	FLEXT_CALLBACK(m_doc_)

	// callbacks
	FLEXT_ATTRVAR_I(detach)
	FLEXT_ATTRVAR_B(pymsg)
	FLEXT_ATTRVAR_B(respond)

	FLEXT_CALLBACK_V(m_stop)
	FLEXT_CALLBACK(m_dir)
	FLEXT_CALLGET_V(mg_dir)
	FLEXT_CALLBACK(m_doc)

	FLEXT_CALLBACK(CbClick)

#ifdef FLEXT_THREADS
    FLEXT_CALLBACK_T(tick)
#endif
};

FLEXT_LIB_V("py",pyobj)


void pyobj::Setup(t_classid c)
{
	FLEXT_CADDMETHOD_(c,0,"doc",m_doc);
	FLEXT_CADDMETHOD_(c,0,"dir",m_dir);
#ifdef FLEXT_THREADS
	FLEXT_CADDATTR_VAR1(c,"detach",detach);
	FLEXT_CADDMETHOD_(c,0,"stop",m_stop);
#endif

	FLEXT_CADDMETHOD_(c,0,"help",m_help);
	FLEXT_CADDMETHOD_(c,0,"reload",m_reload_);
    FLEXT_CADDMETHOD_(c,0,"reload.",m_reload);
	FLEXT_CADDMETHOD_(c,0,"doc+",m_doc_);
	FLEXT_CADDMETHOD_(c,0,"dir+",m_dir_);

	FLEXT_CADDMETHOD_(c,0,"set",m_set);

    FLEXT_CADDMETHOD_(c,0,"edit",CbClick);

  	FLEXT_CADDATTR_VAR1(c,"py",pymsg);
  	FLEXT_CADDATTR_VAR1(c,"respond",respond);
}

pyobj::pyobj(int argc,const t_atom *argv)
    : funname(NULL)
    , function(NULL)
    , withfunction(false)
    , objects(NULL)
{ 
#ifdef FLEXT_THREADS
    FLEXT_ADDTIMER(stoptmr,tick);
#endif

	ThrState state = PyLockSys();

    int inlets;
    if(argc && CanbeInt(*argv)) {
        inlets = GetAInt(*argv);
        if(inlets < 0) inlets = 1;
        argv++,argc--;
    }
    else
         // -1 signals non-explicit definition
        inlets = -1;

    if(inlets >= 1) {
        objects = new PyObject *[inlets];
        for(int i = 0; i < inlets; ++i) { objects[i] = Py_None; Py_INCREF(Py_None); }
    }

    AddInAnything(1+(inlets < 0?1:inlets));
	AddOutAnything();  

    const t_symbol *funnm = NULL;

	// init script module
	if(argc) {
        AddCurrentPath(this);

	    const char *sn = GetAString(*argv);
        argv++,argc--;

        if(sn) {
            char modnm[64];
            strcpy(modnm,sn);

            char *pt = strrchr(modnm,'.'); // search for last dot
            if(pt && *pt) {
                funnm = MakeSymbol(pt+1);
                *pt = 0;
            }

            if(*modnm)
                ImportModule(modnm);
            else
                ImportModule(NULL);          
        }
        else
            PyErr_SetString(PyExc_ValueError,"Invalid module name");
	}

	Register(GetRegistry(REGNAME));

    if(funnm || argc) {
        if(!funnm) {
	        funnm = GetASymbol(*argv);
            argv++,argc--;
        }

        if(funnm)
	        SetFunction(funnm);
        else
            PyErr_SetString(PyExc_ValueError,"Invalid function name");
    }

	if(argc) args(argc,argv);

    Report();

	PyUnlock(state);
}

pyobj::~pyobj() 
{
    ThrState state = PyLockSys();
    if(objects) {
        for(int i = 0; i < CntIn()-1; ++i) Py_DECREF(objects[i]);
        delete[] objects;
    }
    
	Unregister(GetRegistry(REGNAME));
    Report();
	PyUnlock(state);
}

void pyobj::Exit() 
{ 
    pybase::Exit(); 
    flext_base::Exit(); 
}

void pyobj::m_set(int argc,const t_atom *argv)
{
	ThrState state = PyLockSys();

    // function name has precedence
	if(argc >= 2) {
	    const char *sn = GetAString(*argv);
	    ++argv,--argc;

        if(sn) {
//		    if(!module || !strcmp(sn,PyModule_GetName(module))) 
            {
			    ImportModule(sn);
			    Register(GetRegistry(REGNAME));
		    }
        }
        else
            PyErr_SetString(PyExc_ValueError,"Invalid module name");
	}

    if(argc) {
	    const t_symbol *fn = GetASymbol(*argv);
        if(fn)
	        SetFunction(fn);
        else
            PyErr_SetString(PyExc_ValueError,"Invalid function name");
    }

    Report();

	PyUnlock(state);
}

void pyobj::m_help()
{
	post("");
	post("%s %s - python script object, (C)2002-2011 Thomas Grill",thisName(),PY__VERSION);
#ifdef FLEXT_DEBUG
	post("DEBUG VERSION, compiled on " __DATE__ " " __TIME__);
#endif

	post("Arguments: %s [script name] [function name] {args...}",thisName());

	post("Inlet 1:messages to control the py object");
	post("      2:call python function with message as argument(s)");
	post("Outlet: 1:return values from python function");	
	post("Methods:");
	post("\thelp: shows this help");
	post("\tbang: call script without arguments");
	post("\tset [script name] [function name]: set (script and) function name");
	post("\treload {args...}: reload python script");
	post("\treload. : reload with former arguments");
	post("\tdoc: display module doc string");
	post("\tdoc+: display function doc string");
	post("\tdir: dump module dictionary");
	post("\tdir+: dump function dictionary");
#ifdef FLEXT_THREADS
	post("\tdetach 0/1/2: detach threads");
	post("\tstop {wait time (ms)}: stop threads");
#endif
	post("");
}

bool pyobj::ResetFunction()
{
    // function was borrowed from dict!
    function = NULL;
    
    if(!dict)
		post("%s - No namespace available",thisName());
    else {
        if(funname) {
	        function = PyDict_GetItemString(dict,(char *)GetString(funname)); // borrowed!!!

            if(!function && dict == module_dict)
                // search also in __builtins__
    	        function = PyDict_GetItemString(builtins_dict,(char *)GetString(funname)); // borrowed!!!

            if(!function) 
                PyErr_SetString(PyExc_AttributeError,"Function not found");
            else if(!PyCallable_Check(function)) {
    		    function = NULL;
                PyErr_SetString(PyExc_TypeError,"Attribute is not callable");
            }
	    }
    }

    // exception could be set here
    return function != NULL;
}

bool pyobj::SetFunction(const t_symbol *func)
{
	if(func) {
		funname = func;
        withfunction = ResetFunction();
	}
    else {
		function = NULL,funname = NULL;
        withfunction = false;
    }

    // exception could be set here
    return withfunction;
}


void pyobj::LoadModule() 
{
    SetFunction(funname);
}

void pyobj::UnloadModule() 
{
}

void pyobj::Load()
{
	ResetFunction();
}

void pyobj::Unload()
{
//    SetFunction(NULL);
    function = NULL; // just clear the PyObject, not the function name
}

void pyobj::callpy(PyObject *fun,PyObject *args)
{
    PyObject *ret = PyObject_CallObject(fun,args); 
    if(ret) {
        OutObject(this,0,ret); // exception might be raised here
        Py_DECREF(ret);
    }
} 

bool pyobj::CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv)
{
    if(n == 0 && s != sym_bang) 
        return flext_base::CbMethodResort(n,s,argc,argv);

    ThrState state = PyLockSys();

    bool ret = false;
 
    if(objects && n >= 1) {
        // store args
        PyObject *&obj = objects[n-1];
        Py_DECREF(obj);
        obj = MakePyArg(s,argc,argv); // steal reference

        if(n > 1) ret = true; // just store, don't trigger
    }

    if(!ret) {
        if(withfunction) {
            if(function) {
                Py_INCREF(function);

		        PyObject *pargs;
            
                if(objects || CntIn() == 1) {
                    int inlets = CntIn()-1;
            	    pargs = PyTuple_New(inlets);
                    for(int i = 0; i < inlets; ++i) {
                        Py_INCREF(objects[i]);
    		            PyTuple_SET_ITEM(pargs,i,objects[i]);
                    }
                }
                else
                    // construct tuple from args
                    // if n == 0, it's a pure bang
                    pargs = MakePyArgs(n?s:NULL,argc,argv);

                gencall(function,pargs); // references are stolen
                ret = true;
            }
	        else
		        PyErr_SetString(PyExc_RuntimeError,"No function set");
        }
        else if(module) {
            // no function defined as creation argument -> use message tag
            if(s) {
                PyObject *func = PyObject_GetAttrString(module,const_cast<char *>(GetString(s)));
                if(func) {
		            PyObject *pargs = MakePyArgs(sym_list,argc,argv);
                    gencall(func,pargs);
                    ret = true;
                }
            }
            else
		        PyErr_SetString(PyExc_RuntimeError,"No function set");
        }

        Report();
    }

    PyUnlock(state);

    Respond(ret);

    return ret;
}

void pyobj::CbClick() { pybase::OpenEditor(); }

void pyobj::DumpOut(const t_symbol *sym,int argc,const t_atom *argv)
{
    ToOutAnything(GetOutAttr(),sym?sym:thisTag(),argc,argv);
}
