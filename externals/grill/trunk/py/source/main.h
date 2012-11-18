/* 
py/pyext - python script object for PD and MaxMSP

Copyright (c)2002-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2011-03-12 15:13:33 -0500 (Sat, 12 Mar 2011) $
$LastChangedBy: thomas $
*/

#ifndef __MAIN_H
#define __MAIN_H

#include "pyprefix.h"

#define PY__VERSION "0.2.2"


#define PYEXT_MODULE "pyext" // name for module
#define PYEXT_CLASS "_class"  // name for base class

#define REGNAME "_registry"

#define PY_STOP_WAIT 100  // ms
#define PY_STOP_TICK 1  // ms


class pybase;

class FifoEl
    : public FifoCell
{
public:
    void Set(pybase *t,PyObject *f,PyObject *a) { th = t,fun = f,args = a; }
    pybase *th;
    PyObject *fun,*args;
};

typedef PooledFifo<FifoEl> PyFifo;

#endif
