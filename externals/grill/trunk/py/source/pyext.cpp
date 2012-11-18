/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2011-03-12 15:13:33 -0500 (Sat, 12 Mar 2011) $
$LastChangedBy: thomas $
*/

#include "pyext.h"
#include <flinternal.h>


FLEXT_LIB_V("pyext pyext. pyx pyx.",pyext)


static const t_symbol *sym_get;

void pyext::Setup(t_classid c)
{
    sym_get = flext::MakeSymbol("get");
    
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
	FLEXT_CADDATTR_GET(c,"dir+",mg_dir_);

    FLEXT_CADDATTR_VAR(c,"args",initargs,ms_initargs);

	FLEXT_CADDMETHOD_(c,0,"get",m_get);
	FLEXT_CADDMETHOD_(c,0,"set",m_set);

	FLEXT_CADDMETHOD_(c,0,"edit",CbClick);

  	FLEXT_CADDATTR_VAR1(c,"py",pymsg);
  	FLEXT_CADDATTR_VAR1(c,"respond",respond);

	// ----------------------------------------------------

	// register/initialize pyext base class along with module
	class_dict = PyDict_New();
	PyObject *className = PyString_FromString(PYEXT_CLASS);
	PyMethodDef *def;

	// add setattr/getattr to class 
	for(def = attr_tbl; def->ml_name; def++) {
			PyObject *func = PyCFunction_New(def, NULL);
			PyDict_SetItemString(class_dict, def->ml_name, func);
			Py_DECREF(func);
	}

	class_obj = PyClass_New(NULL, class_dict, className);
	Py_DECREF(className);

	// add methods to class 
	for (def = meth_tbl; def->ml_name != NULL; def++) {
		PyObject *func = PyCFunction_New(def, NULL);
		PyObject *method = PyMethod_New(func, NULL, class_obj); // increases class_obj ref count by 1
		PyDict_SetItemString(class_dict, def->ml_name, method);
		Py_DECREF(func);
		Py_DECREF(method);
	}

#if PY_VERSION_HEX >= 0x02020000
	// not absolutely necessary, existent in python 2.2 upwards
	// make pyext functions available in class scope
	PyDict_Merge(class_dict,module_dict,0);
#endif
	// after merge so that it's not in class_dict as well...
	PyDict_SetItemString(module_dict, PYEXT_CLASS,class_obj); // increases class_obj ref count by 1

	PyDict_SetItemString(class_dict,"__doc__",PyString_FromString(pyext_doc));
}

pyext *pyext::GetThis(PyObject *self)
{
	PyObject *th = PyObject_GetAttrString(self,"_this");
    if(th) {
	    pyext *ret = static_cast<pyext *>(PyLong_AsVoidPtr(th));
	    Py_DECREF(th);
        return ret;
    }
    else {
    	PyErr_Clear();
        return NULL;
    }
}

void pyext::SetThis()
{
	// remember the this pointer
	PyObject *th = PyLong_FromVoidPtr(this); 
	PyObject_SetAttrString(pyobj,"_this",th); // ref is taken
}

void pyext::ClearThis()
{
	int ret = PyObject_DelAttrString(pyobj,"_this");
    FLEXT_ASSERT(ret != -1);
}

PyObject *pyext::class_obj = NULL;
PyObject *pyext::class_dict = NULL;

pyext::pyext(int argc,const t_atom *argv,bool sig):
	methname(NULL),
	pyobj(NULL),
	inlets(-1),outlets(-1),
    siginlets(0),sigoutlets(0)
#ifndef PY_USE_GIL
	,pythr(NULL)
#endif
{ 
#ifdef FLEXT_THREADS
    FLEXT_ADDTIMER(stoptmr,tick);
#endif

    if(argc >= 2 && CanbeInt(argv[0]) && CanbeInt(argv[1])) {
        inlets = GetAInt(argv[0]);
        outlets = GetAInt(argv[1]);
        argv += 2,argc -= 2;
    }

    if(sig && argc >= 2 && CanbeInt(argv[0]) && CanbeInt(argv[1])) {
        siginlets = GetAInt(argv[0]);
        sigoutlets = GetAInt(argv[1]);
        argv += 2,argc -= 2;
    }

    const t_symbol *clname = NULL;

    // check if the object name is pyext. , pyx. or similar
    bool dotted = strrchr(thisName(),'.') != NULL;

    ThrLockSys lock;

	// init script module
	if(argc) {
        AddCurrentPath(this);

        const t_symbol *scr = GetASymbol(*argv);
        argv++,argc--;

        if(scr) {
            char modnm[64];
            strcpy(modnm,GetString(scr));

            if(!dotted) {
                char *pt = strrchr(modnm,'.'); // search for last dot
                if(pt && *pt) {
                    clname = MakeSymbol(pt+1);
                    *pt = 0;
                }
            }

			ImportModule(modnm);
		}
        else
            PyErr_SetString(PyExc_ValueError,"Invalid module name");

        // check for alias creation names
        if(dotted) clname = scr;
	}

    Register(GetRegistry(REGNAME));

	if(argc || clname) {
        if(!clname) {
            clname = GetASymbol(*argv);
            argv++,argc--;
        }
    
		if(clname) 
			methname = clname;
		else
            PyErr_SetString(PyExc_ValueError,"Invalid class name");
	}

	if(argc) initargs(argc,argv);

    Report();
}

bool pyext::Init()
{
	ThrLockSys lock;

	if(methname) {
		MakeInstance();
        if(pyobj) InitInOut(inlets,outlets);
	}
    else
        inlets = outlets = 0;

    if(inlets < 0) inlets = 0;
    if(outlets < 0) outlets = 0;

    AddInSignal(siginlets);  
    AddInAnything((siginlets?0:1)+inlets);  
    AddOutSignal(sigoutlets);
	AddOutAnything(outlets);  

    Report();

    return pyobj && flext_dsp::Init();
}

bool pyext::Finalize()
{
	bool ok = true;
	ThrLockSys lock;

	PyObject *init = PyObject_GetAttrString(pyobj,"_init"); // get ref
	if(init) {
		if(PyMethod_Check(init)) {
			PyObject *res = PyObject_CallObject(init,NULL);
			if(!res) {
				// exception is set
				ok = false;
				// we want to know why __init__ failed...
				PyErr_Print();
			}
			else
				Py_DECREF(res);
		}
		Py_DECREF(init);
	}
	else
		// __init__ has not been found - don't care
		PyErr_Clear();

    return ok && flext_dsp::Finalize(); 
}

void pyext::Exit() 
{ 
    pybase::Exit(); // exit threads

	ThrLockSys lock;
    DoExit();

    Unregister(GetRegistry(REGNAME));
	UnimportModule();

    Report();

    flext_dsp::Exit(); 
}

bool pyext::DoInit()
{
    // call init now, after _this has been set, which is
	// important for eventual callbacks from __init__ to c
	PyObject *pargs = MakePyArgs(NULL,initargs.Count(),initargs.Atoms());
    if(pargs) {
        bool ok = true;

        SetThis();

	    PyObject *init = PyObject_GetAttrString(pyobj,"__init__"); // get ref
        if(init) {
            if(PyMethod_Check(init)) {
			    PyObject *res = PyObject_CallObject(init,pargs);
                if(!res) {
                    // exception is set
				    ok = false;
                    // we want to know why __init__ failed...
                    PyErr_Print();
                }
			    else
				    Py_DECREF(res);
            }
            Py_DECREF(init);
	    }
        else
            // __init__ has not been found - don't care
            PyErr_Clear();
        
	    Py_DECREF(pargs);
        return ok;
    }
    else
        return false;
}

void pyext::DoExit()
{
	ClearBinding();

    bool gcrun = false;
    if(pyobj) {
        // try to run del to clean up the class instance
        PyObject *objdel = PyObject_GetAttrString(pyobj,"_del");
        if(objdel) {
            PyObject *ret = PyObject_CallObject(objdel,NULL);
            if(ret)
                Py_DECREF(ret);
            else
                PyErr_Print();
            Py_DECREF(objdel);
        }
        else
            // _del has not been found - don't care
            PyErr_Clear();

        ClearThis();

        gcrun = pyobj->ob_refcnt > 1;
    	Py_DECREF(pyobj);  // opposite of SetClssMeth
    }

    if(gcrun && !collect()) {
        post("%s - Unloading: Object is still referenced",thisName());
    }
}

bool pyext::InitInOut(int &inl,int &outl)
{
    if(inl >= 0) {
        // set number of inlets
        int ret = PyObject_SetAttrString(pyobj,"_inlets",PyInt_FromLong(inl));
        FLEXT_ASSERT(!ret);
    }
    if(outl >= 0) {
        // set number of outlets
		int ret = PyObject_SetAttrString(pyobj,"_outlets",PyInt_FromLong(outl));
        FLEXT_ASSERT(!ret);
    }

    // __init__ can override the number of inlets and outlets
    if(!DoInit()) // call __init__ constructor
        return false;

    if(inl < 0) {
		// get number of inlets
		inl = inlets;
		PyObject *res = PyObject_GetAttrString(pyobj,"_inlets"); // get ref
		if(res) {
			if(PyCallable_Check(res)) {
				PyObject *fres = PyEval_CallObject(res,NULL);
				Py_DECREF(res);
				res = fres;
			}
			if(PyInt_Check(res)) 
				inl = PyInt_AS_LONG(res);
			Py_DECREF(res);
		}
		else 
			PyErr_Clear();
    }
    if(outl < 0) {
        // get number of outlets
        outl = outlets;
		PyObject *res = PyObject_GetAttrString(pyobj,"_outlets"); // get ref
		if(res) {
			if(PyCallable_Check(res)) {
				PyObject *fres = PyEval_CallObject(res,NULL);
				Py_DECREF(res);
				res = fres;
			}
			if(PyInt_Check(res))
				outl = PyInt_AS_LONG(res);
			Py_DECREF(res);
		}
		else
			PyErr_Clear();
    }

    return true;
}

bool pyext::MakeInstance()
{
	// pyobj should already have been decref'd / cleared before getting here!!
	
	if(module && methname) {
		PyObject *pref = PyObject_GetAttrString(module,const_cast<char *>(GetString(methname)));  
		if(!pref) 
			PyErr_Print();
        else {
            if(PyClass_Check(pref)) {
			    // make instance, but don't call __init__ 
			    pyobj = PyInstance_NewRaw(pref,NULL);

			    if(!pyobj) PyErr_Print();
            }
            else
			    post("%s - Type of \"%s\" is unhandled!",thisName(),GetString(methname));

		    Py_DECREF(pref);
		}
		return true;
	}
	else
		return false;
}

void pyext::LoadModule() 
{
}

void pyext::UnloadModule() 
{
}

void pyext::Load()
{
    FLEXT_ASSERT(!pyobj);
    
    bool ok = MakeInstance();

    if(ok) {
        int inl = -1,outl = -1;
        ok = InitInOut(inl,outl);

        if((inl >= 0 && inl != inlets) || (outl >= 0 && outl != outlets))
            post("%s - Inlet and outlet count can't be changed by reload",thisName());
    }

//    return ok;
}

void pyext::Unload() 
{ 
    DoExit(); 
    pyobj = NULL;
}

void pyext::m_get(const t_symbol *s)
{
    ThrLockSys lock;

	PyObject *pvar  = PyObject_GetAttrString(pyobj,const_cast<char *>(GetString(s))); /* fetch bound method */
	if(pvar) {
        flext::AtomListStatic<16> lst;
        const t_symbol *sym = GetPyArgs(lst,pvar,1);
        if(sym) {
            FLEXT_ASSERT(!IsAnything(sym));
            // dump value to attribute outlet
            SetSymbol(lst[0],s);
            ToOutAnything(GetOutAttr(),sym_get,lst.Count(),lst.Atoms());
        }

        Py_DECREF(pvar);
    }

    Report();
}

void pyext::m_set(int argc,const t_atom *argv)
{
    ThrLockSys lock;

    if(argc < 2 || !IsString(argv[0]))
        post("%s - Syntax: set varname arguments...",thisName());
    else if(*GetString(argv[0]) == '_')
        post("%s - set: variables with leading _ are reserved and can't be set",thisName());
    else {
        char *ch = const_cast<char *>(GetString(argv[0]));
        if(PyObject_HasAttrString(pyobj,ch)) {
            PyObject *pval = MakePyArgs(NULL,argc-1,argv+1);
            if(pval) {
                if(PySequence_Size(pval) == 1) {
                    // reduce lists of one element to element itself

                    PyObject *val1 = PySequence_GetItem(pval,0); // new reference
                    Py_DECREF(pval);
                    pval = val1;
                }

                PyObject_SetAttrString(pyobj,ch,pval);
                Py_DECREF(pval);
            }
        }
    }

    Report();
}


bool pyext::CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv)
{
    if(!n) 
        return flext_dsp::CbMethodResort(n,s,argc,argv);

	return pyobj && work(n,s,argc,argv);
}


void pyext::m_help()
{
	post("");
	post("%s %s - python class object, (C)2002-2011 Thomas Grill",thisName(),PY__VERSION);
#ifdef FLEXT_DEBUG
	post("DEBUG VERSION, compiled on " __DATE__ " " __TIME__);
#endif

    post("Arguments: %s {inlets outlets} [script name] [class name] {args...}",thisName());

	post("Inlet 1: messages to control the pyext object");
	post("      2...: python inlets");
	post("Outlets: python outlets");	
	post("Methods:");
	post("\thelp: shows this help");
	post("\treload {args...}: reload python script");
	post("\treload. : reload with former arguments");
	post("\tdoc: display module doc string");
	post("\tdoc+: display class doc string");
	post("\tdir: dump module dictionary");
	post("\tdir+: dump class dictionary");
#ifdef FLEXT_THREADS
	post("\tdetach 0/1: detach threads");
	post("\tstop {wait time (ms)}: stop threads");
#endif
	post("");
}

void pyext::callpy(PyObject *fun,PyObject *args)
{
    PyObject *ret = PyObject_CallObject(fun,args);
    if(ret) {
        // function worked fine
		if(!PyObject_Not(ret)) post("pyext - returned value is ignored");
		Py_DECREF(ret);
    }
} 


bool pyext::call(const char *meth,int inlet,const t_symbol *s,int argc,const t_atom *argv) 
{
	bool ret = false;
    ThrLock lock;

	PyObject *pmeth = PyObject_GetAttrString(pyobj,const_cast<char *>(meth)); /* fetch bound method */
	if(pmeth == NULL) {
		PyErr_Clear(); // no method found
	}
	else {
		PyObject *pargs = MakePyArgs(s,argc,argv,inlet?inlet:-1); //,true);
        if(!pargs) {
			PyErr_Print();
    		Py_DECREF(pmeth);
        }
        else {
            gencall(pmeth,pargs);
            ret = true;
        }
	}
	return ret;
}

bool pyext::work(int n,const t_symbol *s,int argc,const t_atom *argv)
{
	bool ret = false;

    // should be enough...
	char str[256];

    // offset inlet index by signal inlets
    // \note first one is shared with messages!
    if(siginlets) n += siginlets-1;

	// try tag/inlet
	if(!ret) {
		sprintf(str,"%s_%i",GetString(s),n);
		ret = call(str,0,NULL,argc,argv);
	}

    if(!ret && argc == 1) {
        if(s == sym_float) {
    	    // try truncated float
            t_atom at; SetInt(at,GetAInt(argv[0]));
		    sprintf(str,"int_%i",n);
		    ret = call(str,0,NULL,1,&at);
        }
	    else if(s == sym_int) {
    	    // try floating int
            t_atom at; SetFloat(at,GetAFloat(argv[0]));
		    sprintf(str,"float_%i",n);
		    ret = call(str,0,NULL,1,&at);
	    }
	}

	// try anything/inlet
    if(!ret) {
		sprintf(str,"_anything_%i",n);
		ret = call(str,0,s,argc,argv);
	}

	// try tag at any inlet
    if(!ret) {
		sprintf(str,"%s_",GetString(s));
		ret = call(str,n,NULL,argc,argv);
	}

    if(!ret && argc == 1) {
        if(s == sym_float) {
            // try truncated float at any inlet
            t_atom at; SetInt(at,GetAInt(argv[0]));
		    ret = call("int_",0,NULL,1,&at);
        }
        else if(s == sym_int) {
            // try floating int at any inlet
            t_atom at; SetFloat(at,GetAFloat(argv[0]));
		    ret = call("float_",0,NULL,1,&at);
	    }
	}

    if(!ret) {
		// try anything at any inlet
		const char *str1 = "_anything_";
		if(s == sym_bang && !argc) {
			t_atom argv;
			SetSymbol(argv,sym__);
			ret = call(str1,n,s,1,&argv);
		}
		else
			ret = call(str1,n,s,argc,argv);
	}

	if(!ret) 
		// no matching python method found
		post("%s - no matching method found for '%s' into inlet %i",thisName(),GetString(s),n);

    Respond(ret);
	return ret;
}

PyObject *pyext::GetSig(int ix,bool in) { return NULL; }

void pyext::CbClick() { pybase::OpenEditor(); }
bool pyext::CbDsp() { return false; }

void pyext::DumpOut(const t_symbol *sym,int argc,const t_atom *argv)
{
    ToOutAnything(GetOutAttr(),sym?sym:thisTag(),argc,argv);
}
