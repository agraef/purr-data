/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __PYBUNDLE_H
#define __PYBUNDLE_H

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#if FLEXT_OS == FLEXT_OS_MAC
#include <Python/Python.h>
#else
#include <Python.h>
#endif


#ifdef _MSC_VER
    #ifdef PY_EXPORTS
        #define PY_EXPORT __declspec(dllexport)
    #else
        #define PY_EXPORT __declspec(dllimport)
    #endif
#else
    #define PY_EXPORT
#endif

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    flext::MsgBundle *bundle;
} pyBundle;

PY_EXPORT extern PyTypeObject pyBundle_Type;

#define pyBundle_Check(op) PyObject_TypeCheck(op, &pyBundle_Type)
#define pyBundle_CheckExact(op) ((op)->ob_type == &pyBundle_Type)


inline flext::MsgBundle *pyBundle_AS_BUNDLE(PyObject *op) 
{
    return ((pyBundle *)op)->bundle;
}

inline flext::MsgBundle *pyBundle_AsBundle(PyObject *op) 
{
    return pyBundle_Check(op)?pyBundle_AS_BUNDLE(op):NULL;
}


#endif
