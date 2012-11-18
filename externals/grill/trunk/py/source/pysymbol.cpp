/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2008-01-03 12:15:53 -0500 (Thu, 03 Jan 2008) $
$LastChangedBy: thomas $
*/

#include "pyprefix.h"
#include "pysymbol.h"

inline pySymbol *symbol_newsym(const t_symbol *sym)
{
    pySymbol *self = (pySymbol *)pySymbol_Type.tp_alloc(&pySymbol_Type, 0);
    if(self) self->sym = sym;
    return self;
}

static PyObject *symbol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return (PyObject *)symbol_newsym(flext::sym__);
}

static int symbol_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    FLEXT_ASSERT(pySymbol_Check(self));

    PyObject *arg = PySequence_GetItem(args,0); // new reference
    if(!arg) return -1;

    int ret = 0;

    if(pySymbol_Check(arg))
        ((pySymbol *)self)->sym = pySymbol_AS_SYMBOL(arg);
    else if(PyString_Check(arg))
        ((pySymbol *)self)->sym = flext::MakeSymbol(PyString_AS_STRING(arg));
    else {
        PyErr_SetString(PyExc_TypeError,"string or symbol argument expected");
        ret = -1;
    }
    Py_DECREF(arg);

    return ret;
}

static PyObject *symbol_str(PyObject *self)
{
    FLEXT_ASSERT(pySymbol_Check(self));
    return (PyObject *)PyString_FromString(pySymbol_AS_STRING(self));
}

static PyObject *symbol_repr(PyObject *self)
{
    FLEXT_ASSERT(pySymbol_Check(self));
    return (PyObject *)PyString_FromFormat("<Symbol %s>",pySymbol_AS_STRING(self));
}

static PyObject *symbol_richcompare(PyObject *a,PyObject *b,int cmp)
{
    if(pySymbol_Check(a) && pySymbol_Check(b)) {
        const t_symbol *asym = pySymbol_AS_SYMBOL(a);
        const t_symbol *bsym = pySymbol_AS_SYMBOL(b);

		int res = asym == bsym?0:strcmp(flext::GetString(asym),flext::GetString(bsym));
		
        bool ret;
        switch(cmp) {
            case Py_LT: ret = res < 0; break;
            case Py_LE: ret = res <= 0; break;
            case Py_EQ: ret = res == 0; break;
            case Py_NE: ret = res != 0; break;
            case Py_GE: ret = res >= 0; break;
            case Py_GT: ret = res > 0; break;
			default:
				FLEXT_ASSERT(false);
        }
        return PyBool_FromLong(ret);
    }
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static long symbol_hash(PyObject *self)
{
    FLEXT_ASSERT(pySymbol_Check(self));
    return (long)pySymbol_AS_SYMBOL(self);
}


static Py_ssize_t symbol_length(PyObject *s)
{
    pySymbol *self = reinterpret_cast<pySymbol *>(s);
    return strlen(flext::GetString(self->sym));
}

static PyObject *symbol_item(PyObject *s,Py_ssize_t i)
{
    pySymbol *self = reinterpret_cast<pySymbol *>(s);
    const char *str = flext::GetString(self->sym);
    int len = strlen(str);
    if(i < 0) i += len;

    if(i >= 0 && i < len)
        return PyString_FromStringAndSize(str+i,1);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static PyObject *symbol_slice(PyObject *s,Py_ssize_t ilow = 0,Py_ssize_t ihigh = 1<<(sizeof(int)*8-2))
{
    pySymbol *self = reinterpret_cast<pySymbol *>(s);
    const char *str = flext::GetString(self->sym);
    int len = strlen(str);
    if(ilow < 0) {
        ilow += len;
        if(ilow < 0) ilow = 0;
    }
    if(ihigh < 0) ihigh += len;
    if(ihigh >= len) ihigh = len-1;

    return PyString_FromStringAndSize(str+ilow,ilow <= ihigh?ihigh-ilow+1:0);
}

static PyObject *symbol_concat(PyObject *s,PyObject *op)
{
    pySymbol *self = reinterpret_cast<pySymbol *>(s);
    PyObject *nobj = symbol_slice(s); // take all
    if(nobj) {
        PyObject *ret = PySequence_Concat(nobj,op);
        Py_DECREF(nobj);
        return ret;
    }
    else
        return NULL;
}

static PyObject *symbol_repeat(PyObject *s,Py_ssize_t rep)
{
    pySymbol *self = reinterpret_cast<pySymbol *>(s);
    PyObject *nobj = symbol_slice(s); // take all
    if(nobj) {
        PyObject *ret = PySequence_Repeat(nobj,rep);
        Py_DECREF(nobj);
        return ret;
    }
    else
        return NULL;
}

static PySequenceMethods symbol_as_seq = {
	symbol_length,			/* inquiry sq_length;             __len__ */
	symbol_concat,          /* __add__ */
	symbol_repeat,          /* __mul__ */
	symbol_item,			/* intargfunc sq_item;            __getitem__ */
	symbol_slice,		 /* intintargfunc sq_slice;        __getslice__ */
	NULL,		/* intobjargproc sq_ass_item;     __setitem__ */
	NULL,	/* intintobjargproc sq_ass_slice; __setslice__ */
};

static PyObject *symbol_iter(PyObject *s)
{
    pySymbol *self = reinterpret_cast<pySymbol *>(s);
    PyObject *nobj = symbol_slice(s);
    if(nobj) {
        PyObject *it = PyObject_GetIter(nobj);
        Py_DECREF(nobj);
        return it;
    }
    else
        return NULL;
}



PyTypeObject pySymbol_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "Symbol",              /*tp_name*/
    sizeof(pySymbol),          /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,            /*tp_compare*/
    symbol_repr,               /*tp_repr*/
    0,                         /*tp_as_number*/
    &symbol_as_seq,            /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    symbol_hash,               /*tp_hash */
    0,                         /*tp_call*/
    symbol_str,                /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT /*| Py_TPFLAGS_BASETYPE*/,   /*tp_flags*/
    "Symbol objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    symbol_richcompare,	       /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    symbol_iter,		    /* tp_iter */
    0,		               /* tp_iternext */
    0,                          /* tp_methods */
    0,                          /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    symbol_init,            /* tp_init */
    0,                         /* tp_alloc */
    symbol_new,                 /* tp_new */
};

pySymbol *pySymbol__;
pySymbol *pySymbol_bang;
pySymbol *pySymbol_list;
pySymbol *pySymbol_symbol;
pySymbol *pySymbol_float;
pySymbol *pySymbol_int;


void initsymbol()
{
    if(PyType_Ready(&pySymbol_Type) < 0)
        return;

	Py_INCREF(&pySymbol_Type);

    // initialize predefined objects
    pySymbol__ = symbol_newsym(flext::sym__);
    pySymbol_bang = symbol_newsym(flext::sym_bang);
    pySymbol_list = symbol_newsym(flext::sym_list);
    pySymbol_symbol = symbol_newsym(flext::sym_symbol);
    pySymbol_float = symbol_newsym(flext::sym_float);
    pySymbol_int = symbol_newsym(flext::sym_int);
}


PyObject *pySymbol_FromSymbol(const t_symbol *sym)
{
    pySymbol *op;
    if(sym == flext::sym__)
        Py_INCREF(op = pySymbol__);
    else if(sym == flext::sym_bang)
        Py_INCREF(op = pySymbol_bang);
    else if(sym == flext::sym_list)
        Py_INCREF(op = pySymbol_list);
    else if(sym == flext::sym_symbol)
        Py_INCREF(op = pySymbol_symbol);
    else if(sym == flext::sym_float)
        Py_INCREF(op = pySymbol_float);
    else if(sym == flext::sym_int)
        Py_INCREF(op = pySymbol_int);
    else
        op = symbol_newsym(sym);
    return (PyObject *)op;
}
