/* 
py/pyext - python external object for PD and MaxMSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#include "pyext.h"
#include "flinternal.h"

#include <set>

class MethodCompare:
    public std::less<PyObject *>
{
public:
    bool operator()(PyObject *a,PyObject *b) const
    {
        if(PyMethod_Check(a))
            if(PyMethod_Check(b)) {
                // both are methods
                PyObject *sa = PyMethod_GET_SELF(a);
                PyObject *sb = PyMethod_GET_SELF(b);
                if(sa)
                    if(sb) {
                        // both have self
                        if(sa == sb)
                            return PyMethod_GET_FUNCTION(a) < PyMethod_GET_FUNCTION(b);
                        else
                            return sa < sb;
                    }
                    else 
                        return false;
                else
                    if(sb)
                        return true;
                    else 
                        return PyMethod_GET_FUNCTION(a) < PyMethod_GET_FUNCTION(b);
            }
            else
                return false;
        else
            if(PyMethod_Check(b))
                return true;
            else
                // both are non-method callables
                return a < b;
    }
};

typedef std::set<PyObject *,MethodCompare> FuncSet;

struct bounddata 
{ 
    PyObject *self;
    FuncSet funcs;
};

bool pyext::boundmeth(flext_base *th,t_symbol *sym,int argc,t_atom *argv,void *data)
{
    bounddata *obj = (bounddata *)data;
    pyext *pyth = static_cast<pyext *>(th);

	ThrState state = pyth->PyLock();

	PyObject *args = MakePyArgs(sym,argc,argv);

    // call all functions bound by this symbol
    for(FuncSet::iterator it = obj->funcs.begin(); it != obj->funcs.end(); ++it) {
	    PyObject *ret = PyObject_CallObject(*it,args);
	    if(!ret)
		    PyErr_Print();
        else
    	    Py_DECREF(ret);
    }

    Py_XDECREF(args);

	pyth->PyUnlock(state);
    return true;
}

PyObject *pyext::pyext_bind(PyObject *,PyObject *args)
{
    PyObject *self,*meth,*name;
    if(!PyArg_ParseTuple(args, "OOO:pyext_bind", &self,&name,&meth)) // borrowed references
		post("py/pyext - Wrong arguments!");
	else if(!PyInstance_Check(self) || !PyCallable_Check(meth)) {
		post("py/pyext - Wrong argument types!");
    }
	else {
        pyext *th = GetThis(self);
        if(!th) {    
            PyErr_SetString(PyExc_RuntimeError,"pyext - _bind: instance not associated with pd object");
            return NULL;
        }

		const t_symbol *recv = pyObject_AsSymbol(name);

        void *data = NULL;
        if(recv && th->GetBoundMethod(recv,boundmeth,data)) {
            // already bound to that symbol and function
            bounddata *bdt = (bounddata *)data;
            FLEXT_ASSERT(bdt != NULL && bdt->self == self);

            FuncSet::iterator it = bdt->funcs.find(meth);
            if(it == bdt->funcs.end()) {
                bdt->funcs.insert(meth);
                Py_INCREF(meth);
            }
        }
        else {
    		Py_INCREF(self); // self is borrowed reference
            Py_INCREF(meth);

            bounddata *data = new bounddata;
            data->self = self;
            data->funcs.insert(meth);

            th->BindMethod(recv,boundmeth,data);
        }
	}

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *pyext::pyext_unbind(PyObject *,PyObject *args)
{
    PyObject *self,*meth,*name;
    if(!PyArg_ParseTuple(args, "OOO:pyext_bind", &self,&name,&meth))  // borrowed references
		post("py/pyext - Wrong arguments!");
	else if(!PyInstance_Check(self) || !PyCallable_Check(meth)) {
		post("py/pyext - Wrong argument types!");
    }
	else {
        pyext *th = GetThis(self);
        if(!th) {    
            PyErr_SetString(PyExc_RuntimeError,"pyext - _unbind: instance not associated with pd object");
            return NULL;
        }

		const t_symbol *recv = pyObject_AsSymbol(name);

        void *data = NULL;
        if(recv && th->GetBoundMethod(recv,boundmeth,data)) {
            bounddata *bdt = (bounddata *)data;
            FLEXT_ASSERT(bdt != NULL);

            // erase from map
            // ATTENTION: meth is different from the element found in the map
            // it just points to the same instance method
            FuncSet::iterator it = bdt->funcs.find(meth);
            if(it != bdt->funcs.end()) {
    	        Py_DECREF(*it);
                bdt->funcs.erase(it);               
            }
            else
                post("py/pyext - Function to unbind couldn't be found");

            if(bdt->funcs.empty()) {
    		    Py_DECREF(bdt->self);
                delete bdt; 

                th->UnbindMethod(recv,boundmeth,NULL);
            }
        }
	}

    Py_INCREF(Py_None);
    return Py_None;
}


void pyext::ClearBinding()
{
    // in case the object couldn't be constructed...
    if(!pyobj) return;

    pyext *th = GetThis(pyobj);
    if(!th) return;

    void *data = NULL;
    const t_symbol *sym = NULL;

    // unbind all 
    while(th->UnbindMethod(sym,NULL,&data)) {
        bounddata *bdt = (bounddata *)data; 
        if(bdt) {
            for(FuncSet::iterator it = bdt->funcs.begin(); it != bdt->funcs.end(); ++it) 
                Py_DECREF(*it);

		    Py_DECREF(bdt->self);
            delete bdt; 
        }
    }
}
