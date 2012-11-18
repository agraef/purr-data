/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#include "pybase.h"
#include <map>

struct xlt { const t_symbol *from,*to; };

static const xlt xtdefs[] = { 
    { flext::MakeSymbol("+"),flext::MakeSymbol("__add__") },
    { flext::MakeSymbol("+="),flext::MakeSymbol("__iadd__") },
    { flext::MakeSymbol("!+"),flext::MakeSymbol("__radd__") },
    { flext::MakeSymbol("-"),flext::MakeSymbol("__sub__") },
    { flext::MakeSymbol("-="),flext::MakeSymbol("__isub__") },
    { flext::MakeSymbol("!-"),flext::MakeSymbol("__rsub__") },
    { flext::MakeSymbol("*"),flext::MakeSymbol("__mul__") },
    { flext::MakeSymbol("*="),flext::MakeSymbol("__imul__") },
    { flext::MakeSymbol("!*"),flext::MakeSymbol("__rmul__") },
    { flext::MakeSymbol("/"),flext::MakeSymbol("__div__") },
    { flext::MakeSymbol("/="),flext::MakeSymbol("__idiv__") },
    { flext::MakeSymbol("!/"),flext::MakeSymbol("__rdiv__") },
    { flext::MakeSymbol("//"),flext::MakeSymbol("__floordiv__") },
    { flext::MakeSymbol("//="),flext::MakeSymbol("__ifloordiv__") },
    { flext::MakeSymbol("!//"),flext::MakeSymbol("__rfloordiv__") },
    { flext::MakeSymbol("%"),flext::MakeSymbol("__mod__") },
    { flext::MakeSymbol("%="),flext::MakeSymbol("__imod__") },
    { flext::MakeSymbol("!%"),flext::MakeSymbol("__rmod__") },
    { flext::MakeSymbol("**"),flext::MakeSymbol("__pow__") },
    { flext::MakeSymbol("**="),flext::MakeSymbol("__ipow__") },
    { flext::MakeSymbol("!**"),flext::MakeSymbol("__rpow__") },
    { flext::MakeSymbol("&"),flext::MakeSymbol("__and__") },
    { flext::MakeSymbol("&="),flext::MakeSymbol("__iand__") },
    { flext::MakeSymbol("!&"),flext::MakeSymbol("__rand__") },
    { flext::MakeSymbol("|"),flext::MakeSymbol("__or__") },
    { flext::MakeSymbol("|="),flext::MakeSymbol("__ior__") },
    { flext::MakeSymbol("!|"),flext::MakeSymbol("__ror__") },
    { flext::MakeSymbol("^"),flext::MakeSymbol("__xor__") },
    { flext::MakeSymbol("^="),flext::MakeSymbol("__ixor__") },
    { flext::MakeSymbol("!^"),flext::MakeSymbol("__rxor__") },
    { flext::MakeSymbol("<<"),flext::MakeSymbol("__lshift__") },
    { flext::MakeSymbol("<<="),flext::MakeSymbol("__ilshift__") },
    { flext::MakeSymbol("!<<"),flext::MakeSymbol("__rlshift__") },
    { flext::MakeSymbol(">>"),flext::MakeSymbol("__rshift__") },
    { flext::MakeSymbol(">>="),flext::MakeSymbol("__irshift__") },
    { flext::MakeSymbol("!>>"),flext::MakeSymbol("__rrshift__") },
    { flext::MakeSymbol("=="),flext::MakeSymbol("__eq__") },
    { flext::MakeSymbol("!="),flext::MakeSymbol("__ne__") },
    { flext::MakeSymbol("<"),flext::MakeSymbol("__lt__") },
    { flext::MakeSymbol(">"),flext::MakeSymbol("__gt__") },
    { flext::MakeSymbol("<="),flext::MakeSymbol("__le__") },
    { flext::MakeSymbol(">="),flext::MakeSymbol("__ge__") },
    { flext::MakeSymbol("!"),flext::MakeSymbol("__nonzero__") },
    { flext::MakeSymbol("~"),flext::MakeSymbol("__invert__") },
    { flext::MakeSymbol("[]"),flext::MakeSymbol("__getitem__") },
    { flext::MakeSymbol("[]="),flext::MakeSymbol("__setitem__") },
    { flext::MakeSymbol("[:]"),flext::MakeSymbol("__getslice__") },
    { flext::MakeSymbol("[:]="),flext::MakeSymbol("__setslice__") },

    { flext::MakeSymbol(".abs"),flext::MakeSymbol("__abs__") },
    { flext::MakeSymbol(".neg"),flext::MakeSymbol("__neg__") },
    { flext::MakeSymbol(".pos"),flext::MakeSymbol("__pos__") },
    { flext::MakeSymbol(".divmod"),flext::MakeSymbol("__divmod__") },

    { flext::MakeSymbol(".int"),flext::MakeSymbol("__int__") },
    { flext::MakeSymbol(".long"),flext::MakeSymbol("__long__") },
    { flext::MakeSymbol(".float"),flext::MakeSymbol("__float__") },
    { flext::MakeSymbol(".complex"),flext::MakeSymbol("__complex__") },
    { flext::MakeSymbol(".str"),flext::MakeSymbol("__str__") },
    { flext::MakeSymbol(".coerce"),flext::MakeSymbol("__coerce__") },

    { flext::MakeSymbol(".doc"),flext::MakeSymbol("__doc__") },
    { flext::MakeSymbol(".repr"),flext::MakeSymbol("__repr__") },

    { flext::MakeSymbol(".len"),flext::MakeSymbol("__len__") },
    { flext::MakeSymbol(".in"),flext::MakeSymbol("__contains") },

    { NULL,NULL } // sentinel
};

typedef std::map<const t_symbol *,const t_symbol *> XTable;
static XTable xtable;


class pymeth
    : public pybase
    , public flext_base
{
	FLEXT_HEADER_S(pymeth,flext_base,Setup)

public:
	pymeth(int argc,const t_atom *argv);
	~pymeth();

protected:
    virtual void Exit();

	virtual bool CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv);

    void m_help();    

    void m_reload() { Reload(); }
    void m_reload_(int argc,const t_atom *argv) { args(argc,argv); Reload(); }
	void m_set(int argc,const t_atom *argv);
    void m_dir_() { m__dir(function); }
    void m_doc_() { m__doc(function); }

	const t_symbol *funname;
	PyObject *function;

	virtual void LoadModule();
	virtual void UnloadModule();

	virtual void Load();
	virtual void Unload();

	void SetFunction(const t_symbol *func);
	void ResetFunction();

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

#ifdef FLEXT_THREADS
    FLEXT_CALLBACK_T(tick)
#endif
};

FLEXT_LIB_V("pym",pymeth)


void pymeth::Setup(t_classid c)
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

  	FLEXT_CADDATTR_VAR1(c,"py",pymsg);
  	FLEXT_CADDATTR_VAR1(c,"respond",respond);

    // init translation map
    for(const xlt *xi = xtdefs; xi->from; ++xi) xtable[xi->from] = xi->to;
}

pymeth::pymeth(int argc,const t_atom *argv)
    : funname(NULL)
    , function(NULL)
    , objects(NULL)
{ 
#ifdef FLEXT_THREADS
    FLEXT_ADDTIMER(stoptmr,tick);
#endif

	ThrLockSys lock;

    int inlets;
    if(argc && CanbeInt(*argv)) {
        inlets = GetAInt(*argv);
        if(inlets < 1) inlets = 1;
        argv++,argc--;
    }
    else inlets = 1;

    objects = new PyObject *[inlets];
    for(int i = 0; i < inlets; ++i) { objects[i] = Py_None; Py_INCREF(Py_None); }

    if(inlets <= 0) InitProblem();

    AddInAnything(1+(inlets < 0?1:inlets));
	AddOutAnything();  

	Register(GetRegistry(REGNAME));

    if(argc) {
        const t_symbol *funnm = GetASymbol(*argv);
        argv++,argc--;

        if(funnm)
	        SetFunction(funnm);
        else
            PyErr_SetString(PyExc_ValueError,"Invalid function name");
    }

	if(argc) args(argc,argv);

    Report();
}

pymeth::~pymeth() 
{
    if(objects) {
        for(int i = 0; i < CntIn()-1; ++i) Py_DECREF(objects[i]);
        delete[] objects;
    }

    ThrLockSys lock;
	Unregister(GetRegistry(REGNAME));
    Report();
}

void pymeth::Exit() 
{ 
    pybase::Exit(); 
    flext_base::Exit(); 
}

void pymeth::m_set(int argc,const t_atom *argv)
{
	ThrLockSys lock;

    // function name has precedence
	if(argc >= 2) {
	    const char *sn = GetAString(*argv);
	    ++argv,--argc;

        if(sn) {
		    if(!module || !strcmp(sn,PyModule_GetName(module))) {
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
}

void pymeth::m_help()
{
	post("");
	post("%s %s - python method object, (C)2002-2008 Thomas Grill",thisName(),PY__VERSION);
#ifdef FLEXT_DEBUG
	post("DEBUG VERSION, compiled on " __DATE__ " " __TIME__);
#endif

	post("Arguments: %s [method name] {args...}",thisName());

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

void pymeth::ResetFunction()
{
    Py_XDECREF(function);
    function = NULL;
    
    if(funname && objects[0] != Py_None) {
        function = PyObject_GetAttrString(objects[0],(char *)GetString(funname)); // new reference
        if(!function) 
            PyErr_SetString(PyExc_AttributeError,"Method not found");
    }

    // exception could be set here
}

void pymeth::SetFunction(const t_symbol *func)
{
    // look for method name in translation table
    XTable::iterator it = xtable.find(func);
    funname = it == xtable.end()?func:it->second;

    ResetFunction();
}


void pymeth::LoadModule() 
{
    SetFunction(funname);
}

void pymeth::UnloadModule() 
{
}

void pymeth::Load()
{
	ResetFunction();
}

void pymeth::Unload()
{
    SetFunction(NULL);
}

void pymeth::callpy(PyObject *fun,PyObject *args)
{
    PyObject *ret = PyObject_CallObject(fun,args); 
    if(ret) {
        OutObject(this,0,ret); // exception might be raised here
        Py_DECREF(ret);
    }
} 

bool pymeth::CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv)
{
    if(n == 0 && s != sym_bang) 
        return flext_base::CbMethodResort(n,s,argc,argv);

    ThrState state = PyLockSys();

    bool ret = false;
 
    if(n >= 1) {
        // store args
        PyObject *&obj = objects[n-1];
        Py_DECREF(obj);
        obj = MakePyArg(s,argc,argv); // steal reference

        if(n > 1) ret = true; // just store, don't trigger
    }

    if(!ret) {
        if(function) {
            PyObject *self = PyMethod_Self(function);
            PyErr_Clear();
            if(!self || self->ob_type != objects[0]->ob_type)
                // type has changed, search for new method
                ResetFunction();
            else if(self != objects[0]) {
                // type hasn't changed, but object has
                PyObject *f = function;
                function = PyMethod_New(PyMethod_GET_FUNCTION(f),objects[0],PyMethod_GET_CLASS(f));
                Py_DECREF(f);
            }
        }
        else
            ResetFunction();

        if(function) {
            Py_INCREF(function);

            int inlets = CntIn()-1;
            PyObject *pargs = PyTuple_New(inlets-1);
            for(int i = 1; i < inlets; ++i) {
                Py_INCREF(objects[i]);
    		    PyTuple_SET_ITEM(pargs,i-1,objects[i]);
            }

            gencall(function,pargs); // references are stolen
            ret = true;
        }
	    else
		    PyErr_SetString(PyExc_RuntimeError,"No function set");

        Report();
    }

    PyUnlock(state);

    Respond(ret);

    return ret;
}

void pymeth::DumpOut(const t_symbol *sym,int argc,const t_atom *argv)
{
    ToOutAnything(GetOutAttr(),sym?sym:thisTag(),argc,argv);
}
