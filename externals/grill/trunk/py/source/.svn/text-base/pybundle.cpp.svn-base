/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#include "pyprefix.h"
#include "pybundle.h"
#include "pyext.h"

static PyObject *bundle_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    pyBundle *self = (pyBundle *)pyBundle_Type.tp_alloc(&pyBundle_Type, 0);
    if(self) self->bundle = flext::MsgNew();
    return (PyObject *)self;
}

static int bundle_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    FLEXT_ASSERT(pyBundle_Check(self));

    int len = PySequence_Length(args);
    if(len) {
        PyErr_SetString(PyExc_TypeError,"no arguments expected");
        return -1;
    }

    return 0;
}

static void bundle_dealloc(PyObject *obj)
{
    pyBundle *self = (pyBundle *)obj;
    if(self->bundle) flext::MsgFree(self->bundle);
    obj->ob_type->tp_free(obj);
}

static PyObject *bundle_send(PyObject *obj)
{
    pyBundle *self = (pyBundle *)obj;
    if(self->bundle) {
        flext::ToOutMsg(self->bundle);
        self->bundle = NULL;

        Py_INCREF(obj);
        return obj;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError,"already sent");
        return NULL;
    }
}

static PyObject *bundle_repr(PyObject *self)
{
    FLEXT_ASSERT(pyBundle_Check(self));
    return (PyObject *)PyString_FromFormat("<Bundle %p>",pyBundle_AS_BUNDLE(self));
}

static PyObject *bundle_str(PyObject *self)
{
    return bundle_repr(self);
}

static PyObject *bundle_richcompare(PyObject *a,PyObject *b,int cmp)
{
    if(pyBundle_Check(a) && pyBundle_Check(b)) {
        const flext::MsgBundle *abnd = pyBundle_AS_BUNDLE(a);
        const flext::MsgBundle *bbnd = pyBundle_AS_BUNDLE(b);
        bool ret;
        switch(cmp) {
            case Py_LT: ret = abnd < bbnd; break;
            case Py_LE: ret = abnd <= bbnd; break;
            case Py_EQ: ret = abnd == bbnd; break;
            case Py_NE: ret = abnd != bbnd; break;
            case Py_GT: ret = abnd > bbnd; break;
            case Py_GE: ret = abnd >= bbnd; break;
        }
        return PyBool_FromLong(ret);
    }
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static long bundle_hash(PyObject *self)
{
    FLEXT_ASSERT(pyBundle_Check(self));
    return (long)pyBundle_AS_BUNDLE(self);
}


static PyObject *bundle_append(PyObject *self,PyObject *args)
{
    flext::MsgBundle *b = pyBundle_AS_BUNDLE(self);
    if(b) {
        int sz = PyTuple_GET_SIZE(args),offs = 0;
        PyObject *tg,*outl;
        pyext *ext = NULL;
        const t_symbol *recv;
        int o;

        if(sz > 2 &&
		    (tg = PyTuple_GET_ITEM(args,0)) != NULL && PyInstance_Check(tg) && 
		    (outl = PyTuple_GET_ITEM(args,1)) != NULL && PyInt_Check(outl)
        ) {
            // Sending to outlet
            ext = pyext::GetThis(tg);
            o = PyInt_AS_LONG(outl);

    		if(o < 1 || o > ext->Outlets()) {
                PyErr_SetString(PyExc_ValueError,"Outlet index out of range");
                return NULL;
            }

            offs += 2;
        }
        else if(sz > 1 &&
		    (tg = PyTuple_GET_ITEM(args,0)) != NULL && (recv = pyObject_AsSymbol(tg)) != NULL
        ) {
            // Sending to receiver
            offs++;
        }
        else {
            // not recognized
    		PyErr_SetString(PyExc_SyntaxError,"Unrecognized arguments");
            return NULL;
        }

        PyObject *val;
        if(sz-offs == 1) {
            val = PyTuple_GET_ITEM(args,offs); // borrow reference
            Py_INCREF(val);
        }
        else
            val = PyTuple_GetSlice(args,offs,sz);  // new ref

        flext::AtomListStatic<16> lst;
        const t_symbol *sym = pybase::GetPyArgs(lst,val);
		Py_DECREF(val);
        
		if(sym) {
            if(ext) {
                FLEXT_ASSERT(outl);
                ext->MsgAddAnything(b,o-1,sym,lst.Count(),lst.Atoms());
            }
            else {
                FLEXT_ASSERT(sym);
                if(!flext::MsgForward(b,recv,sym,lst.Count(),lst.Atoms())) {
    		        PyErr_SetString(PyExc_ValueError,"Receiver not found");
                    return NULL;
                }
            }

            Py_INCREF(Py_None);
            return Py_None;
		}
        else {
            FLEXT_ASSERT(PyErr_Occurred());
            return NULL;
        }

        Py_INCREF(self);
        return self;
    }
    else {
		PyErr_SetString(PyExc_RuntimeError,"Invalid bundle");
        return NULL;
    }
}

static PyMethodDef bundle_methods[] = {
    {"append", (PyCFunction)bundle_append,METH_VARARGS,"Append message to bundle"},
    {"send", (PyCFunction)bundle_send,METH_NOARGS,"Send bundle"},
    {NULL}  /* Sentinel */
};



PyTypeObject pyBundle_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "Bundle",              /*tp_name*/
    sizeof(pyBundle),          /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    bundle_dealloc,             /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,            /*tp_compare*/
    bundle_repr,               /*tp_repr*/
    0,                         /*tp_as_number*/
    0,            /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    bundle_hash,               /*tp_hash */
    0,                         /*tp_call*/
    bundle_str,                /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT /*| Py_TPFLAGS_BASETYPE*/,   /*tp_flags*/
    "Bundle objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    bundle_richcompare,	       /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		    /* tp_iter */
    0,		               /* tp_iternext */
    bundle_methods,            /* tp_methods */
    0,                          /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    bundle_init,            /* tp_init */
    0,                         /* tp_alloc */
    bundle_new,                 /* tp_new */
};


void initbundle()
{
    if(PyType_Ready(&pyBundle_Type) < 0)
        FLEXT_ASSERT(false);
    else
        Py_INCREF(&pyBundle_Type);
}
