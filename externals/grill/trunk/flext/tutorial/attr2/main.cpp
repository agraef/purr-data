/* 
flext tutorial - attributes 2

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of an object doing various float operations.

Methods and attributes are registered at class level (opposed to object level in example "attr1").
For details, see also example "adv2"

*/


// IMPORTANT: enable attribute processing (specify before inclusion of flext headers!)
// For clarity, this is done here, but you'd better specify it as a compiler definition
// FLEXT_ATTRIBUTES must be 0 or 1, 
#define FLEXT_ATTRIBUTES 1


// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1
#endif

#include <math.h>


class attr2:
	public flext_base
{
	// compulsory flext header with a class setup function
	FLEXT_HEADER_S(attr2,flext_base,setup)
 
public:
	// constructor 
	attr2();

protected:
	void m_trigger(float f);   
	
	// stored argument of operation
	float arg; 

	// stored result
	float res;

	enum operation {
		op_set,op_add,op_sub,op_mul,op_div,op_pow
	} op;

	static const t_symbol *sym_set,*sym_add,*sym_sub,*sym_div,*sym_mul,*sym_pow;

private:

	static void setup(t_classid);

	// callback for method "m_trigger" (with one float argument)
	FLEXT_CALLBACK_F(m_trigger)

	// define attribute callbacks for variable "arg" ("ATTRVAR" means GET and SET)
	FLEXT_ATTRVAR_F(arg) 

	// define attribute callbacks for variable "res" (GET only)
	FLEXT_ATTRGET_F(res)

	// methods for getting/setting the operation mode
	void opget(const t_symbol *&s) const; 
	void opset(const t_symbol *&s);

	// define attribute callbacks for variable "res" (GET only)
	FLEXT_CALLGET_S(opget)
	FLEXT_CALLSET_S(opset)
};

// instantiate the class
FLEXT_NEW("attr2",attr2)


// instantiate static variables
const t_symbol 
	*attr2::sym_set,
	*attr2::sym_add,*attr2::sym_sub,
	*attr2::sym_div,*attr2::sym_mul,
	*attr2::sym_pow;

void attr2::setup(t_classid c)
{
	// Upon class creation setup some symbols
	// This is done only upon creation of of the first "attr2" object
	sym_set = MakeSymbol("=");
	sym_add = MakeSymbol("+");
	sym_sub = MakeSymbol("-");
	sym_mul = MakeSymbol("*");
	sym_div = MakeSymbol("/");
	sym_pow = MakeSymbol("**");


	// setup methods and attributes at class scope

	// register method (for floats) "m_trigger" for inlet 0
	FLEXT_CADDMETHOD(c,0,m_trigger);  

	// register attribute "arg" with the variable "arg"
	FLEXT_CADDATTR_VAR1(c,"arg",arg);  

	// register attribute "result" with variable "res"
	FLEXT_CADDATTR_GET(c,"result",res);  

	// register attribute "op" with methods "opget" and "opset"
	FLEXT_CADDATTR_VAR(c,"op",opget,opset);  
}


attr2::attr2():
	arg(0),res(0),  // initialize argument and result
	op(op_set)  // initialize operation
{ 
	// define inlets
	AddInAnything();  // first inlet of type anything (index 0)
	
	// define outlets
	AddOutFloat();  // one float outlet (has index 0)
} 

// receive an operand, do the math operation and trigger the output
void attr2::m_trigger(float f)
{
	switch(op) {
	case op_set: res = f; break;
	case op_add: res = f+arg; break;
	case op_sub: res = f-arg; break;
	case op_mul: res = f*arg; break;
	case op_div: 
		if(arg) res = f/arg; 
		else {
			post("%s - argument to division is 0: result set to 0",thisName());
			res = 0;
		}
		break;
	case op_pow: res = (float)pow(f,arg); break;
#ifdef FLEXT_DEBUG
	default: ERRINTERNAL();  // operation not defined
#endif
	}
		
	// output value to outlet
	ToOutFloat(0,res); // (0 stands for the outlet index 0)
}


// report the operation mode
void attr2::opget(const t_symbol *&s) const
{

	switch(op) {
	case op_set: s = sym_set; break;
	case op_add: s = sym_add; break;
	case op_sub: s = sym_sub; break;
	case op_mul: s = sym_mul; break;
	case op_div: s = sym_div; break;
	case op_pow: s = sym_pow; break;
#ifdef FLEXT_DEBUG
	default: ERRINTERNAL();  // operation not defined
#endif
	}
}

// set the operation mode
void attr2::opset(const t_symbol *&s)
{
	if(s == sym_set)
		op = op_set;
	else if(s == sym_add)
		op = op_add;
	else if(s == sym_sub)
		op = op_sub;
	else if(s == sym_mul)
		op = op_mul;
	else if(s == sym_div)
		op = op_div;
	else if(s == sym_pow)
		op = op_pow;
	else {
		post("%s - operation is not defined, set to =",thisName());
		op = op_set;
	}
}

