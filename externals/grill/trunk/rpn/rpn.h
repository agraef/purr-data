/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2410 $
$LastChangedDate: 2007-12-11 09:16:34 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#ifndef __RPN_H
#define __RPN_H

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#define FLEXT_ATTRIBUTES 1

#include <flext.h>
#include <map>
#include <vector>
#include <valarray>

typedef double Real;
typedef float VEl;
typedef std::valarray<VEl> Vector;

typedef const t_symbol *Symbol;


namespace Error {

class OpNotDef {};

class OpTypeErr {};
class OpNotImp {};
class OpUnderflow {};
class OpBounds {};

class MemNotSet {};

}

#include "rpn_ops.h"

#endif
