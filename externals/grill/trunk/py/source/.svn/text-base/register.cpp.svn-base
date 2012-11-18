/* 
py/pyext - python external object for PD and MaxMSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#include "pybase.h"

#if 1

PyObject *pybase::GetRegistry(const char *regnm)
{
	if(module) {
        FLEXT_ASSERT(dict); // module must have a valid dict

		// add this to module registry
		PyObject *reg = PyDict_GetItemString(dict,(char *)regnm); // borrowed!!!
		if(reg) 
            FLEXT_ASSERT(PyDict_Check(reg));
        else {
            // make a new empty registry
    		reg = PyDict_New();
			PyDict_SetItemString(dict,(char *)regnm,reg);
        }
        return reg;
	}
    else
        return NULL;
}

void pybase::SetRegistry(const char *regnm,PyObject *reg)
{
    if(module) {
        FLEXT_ASSERT(dict); // module must have a valid dict
        FLEXT_ASSERT(reg && PyDict_Check(reg));
        PyDict_SetItemString(dict,(char *)regnm,reg);
    }
}

void pybase::Register(PyObject *reg)
{
    if(!module) return;
    FLEXT_ASSERT(reg && PyDict_Check(reg));
	
    // add this to module registry
    Py_INCREF(Py_None);
    PyObject *key = PyLong_FromUnsignedLong((size_t)this);
    PyDict_SetItem(reg,key,Py_None);
}

void pybase::Unregister(PyObject *reg)
{
    if(!module) return;
    FLEXT_ASSERT(reg && PyDict_Check(reg));
	
    // remove this from module registry
    PyObject *key = PyLong_FromUnsignedLong((size_t)this);
    PyObject *item = PyDict_GetItem(reg,key);		
    if(!item)
        post("py/pyext - Internal error: object not found in registry");
    else
		PyDict_DelItem(reg,key);
}

/*
void pybase::RegLoad(PyObject *reg)
{

}

void pybase::RegUnload(PyObject *reg)
{
}
*/

#else

void pybase::Register(const char *regnm)
{
	if(module) {
		// add this to module registry

		PyObject *reg = PyDict_GetItemString(dict,(char *)regnm); // borrowed!!!
		PyObject *add = Py_BuildValue("[i]",(long)this);
		if(!reg || !PyList_Check(reg)) {
			if(PyDict_SetItemString(dict,(char *)regnm,add)) {
				post("py/pyext - Could not set registry");
			}
		}
		else {
			PySequence_InPlaceConcat(reg,add);
		}
	}
}

void pybase::Unregister(const char *regnm)
{
	if(module) {
		// remove this from module registry
		
		PyObject *reg = PyDict_GetItemString(dict,(char *)regnm); // borrowed!!!
		PyObject *add = Py_BuildValue("i",(int)this);
		if(!reg || !PySequence_Check(reg)) 
			post("py/pyext - Internal error: Registry not found!?");
		else {
			int ix = PySequence_Index(reg,add);
			if(ix < 0) {
                post("py/pyext - Internal error: object not found in registry?!");
			}
			else {	
				PySequence_DelItem(reg,ix);
			}
		}
		Py_DECREF(add);
	}
}

void pybase::Reregister(const char *regnm)
{
	if(module) {
		// remove this from module registry
		
		PyObject *reg = PyDict_GetItemString(dict,(char *)regnm); // borrowed!!!

		if(!reg || !PySequence_Check(reg)) 
			post("py/pyext - Internal error: Registry not found!?");
		else {
			int cnt = PySequence_Size(reg);
			for(int i = 0; i < cnt; ++i) {
				PyObject *it = PySequence_GetItem(reg,i); // new reference
				if(!it || !PyInt_Check(it)) {
                    post("py/pyext - Internal error: Corrupt registry?!");
				}
				else {
					pybase *th = (pybase *)PyInt_AsLong(it);
					th->module = module;
					th->dict = dict;
					th->Reload();
				}

                Py_XDECREF(it);
			}
		}
	}
}

#endif
