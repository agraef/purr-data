/* 
py/pyext - python script object for PD and MaxMSP

Copyright (c)2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __PYPREFIX_H
#define __PYPREFIX_H

#define FLEXT_ATTRIBUTES 1
#include <flext.h>

// hack: must include math.h before Python.h (at least on OSX)
// otherwise some functions don't get defined
#include <cmath>

#if FLEXT_OS == FLEXT_OS_MAC
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 501)
#error You need at least flext version 0.5.1
#endif

#if FLEXT_OS == FLEXT_LINUX || FLEXT_OS == FLEXT_IRIX
#include <unistd.h>
#endif

#if FLEXT_SYS == FLEXT_SYS_PD && (!defined (PD_MINOR_VERSION) || PD_MINOR_VERSION < 37)
#error PD version >= 0.37 required, please upgrade! 
#endif

#include <flcontainers.h>
#include <string>

#if FLEXT_SYS == FLEXT_SYS_PD && defined(PY_USE_INOFFICIAL)
extern "C" {
#include <s_stuff.h>
}
#endif

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#endif

#endif
