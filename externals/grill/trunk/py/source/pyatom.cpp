/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2008-01-03 11:20:03 -0500 (Thu, 03 Jan 2008) $
$LastChangedBy: thomas $
*/

#include "pyatom.h"
#include <map>

#define INTV 0.01

typedef std::map<size_t,PyObject *> ObjMap;

static ObjMap objmap;
static size_t collix = 0,curix = 0;
static double last = 0;

size_t PyAtom::Register(PyObject *obj)
{
    Collect();

    Py_INCREF(obj);
    objmap[++curix] = obj;

#ifdef _DEBUG
//    post("REG %p (%i)\n",obj,objmap.size());
#endif
    return curix;
}

PyObject *PyAtom::Retrieve(size_t id)
{
    ObjMap::iterator it = objmap.find(id);
    PyObject *ret = it == objmap.end()?NULL:it->second;
    Collect();
    return ret;
}

void PyAtom::Collect()
{
    for(;;) {
        ObjMap::iterator it = objmap.begin();
        if(it == objmap.end() || it->first > collix) break;
        
        PyObject *obj = it->second;
        Py_DECREF(obj);
        objmap.erase(it);

#ifdef _DEBUG
//        post("DEL %p\n",obj);
#endif
    }

    // schedule next collect time
    double tm = flext::GetTime();
    if(tm > last+INTV) last = tm,collix = curix;
}
