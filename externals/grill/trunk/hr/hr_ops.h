/* 
hr - high resolution (double precision) math library

Copyright (c)2006-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3743 $
$LastChangedDate: 2011-03-09 17:23:18 -0500 (Wed, 09 Mar 2011) $
$LastChangedBy: thomas $
*/

#ifndef __HR_OPS_H
#define __HR_OPS_H

#include <cmath>

namespace Operators {

template <typename ResT,typename OpT>
class Op
{
public:
	typedef ResT rtype;
	typedef OpT otype;
};

template <typename ResT,typename OpT> class UnOp: public Op<ResT,OpT> {};
template <typename ResT,typename OpT> class BinOp: public Op<ResT,OpT> {};
template <typename OpT> class LogicOp: public Op<double,OpT> {};

// unary ////////////////////////////////////////////////////

class sqrt: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::sqrt(a); }
};

class sqr: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return a*a; }
};

class abs: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::fabs(a); }
};

class exp: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::exp(a); }
};

class ln: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::log(a); }
};

class sin: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::sin(a); }
};

class cos: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::cos(a); }
};

class tan: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::tan(a); }
};

class asin: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::asin(a); }
};

class acos: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::acos(a); }
};

class atan: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::atan(a); }
};

class sinh: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::sinh(a); }
};

class cosh: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::cosh(a); }
};

class tanh: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::tanh(a); }
};

#ifndef _MSC_VER
class asinh: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::asinh(a); }
};

class acosh: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::acosh(a); }
};

class atanh: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::atanh(a); }
};

class floor: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::floor(a); }
};

class ceil: public UnOp<double,double>
{
public:
	static rtype operate(otype a) { return ::ceil(a); }
};
#endif

// binary ///////////////////////////////////////////////////

class plus: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return a+b; }
};

class minus: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return a-b; }
};

class rminus: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return b-a; }
};

class times: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return a*b; }
};

class over: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return a/b; }
};

class rover: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return b/a; }
};

class mod: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return ::fmod(a,b); }
};

class rmod: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return ::fmod(b,a); }
};

class power: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return ::pow(a,b); }
};

class rpower: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return ::pow(b,a); }
};

class atan2: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return ::atan2(a,b); }
};

class min: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return a < b?a:b; }
};

class max: public BinOp<double,double>
{
public:
	static rtype operate(otype a,otype b) { return a > b?a:b; }
};

// comparison //////////////////////////////////////////////////

class eq: public LogicOp<double>
{
public:
	static rtype operate(otype a,otype b) { return a == b?1:0; }
};

class ne: public LogicOp<double>
{
public:
	static rtype operate(otype a,otype b) { return a != b?1:0; }
};

class gt: public LogicOp<double>
{
public:
	static rtype operate(otype a,otype b) { return a > b?1:0; }
};

class ge: public LogicOp<double>
{
public:
	static rtype operate(otype a,otype b) { return a >= b?1:0; }
};

class lt: public LogicOp<double>
{
public:
	static rtype operate(otype a,otype b) { return a < b?1:0; }
};

class le: public LogicOp<double>
{
public:
	static rtype operate(otype a,otype b) { return a <= b?1:0; }
};

}

#endif
