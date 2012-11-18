/* 
flext examples - henon

Copyright (c) 2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is a simple external featuring the chaotic Henon attractor.

See also http://improv.sapp.org/doc/examples/synthImprov/henontune/henontune.html
Thanks to David Casal for the pointer!

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


class henon:
	public flext_base
{
	FLEXT_HEADER_S(henon,flext_base,Setup)
 
public:
	// constructor 
	henon(int argc,const t_atom *argv);

protected:
	void m_bang();
	void m_reset() { x = y = 0; }

	void m_alpha(float a) { alpha = a; }
	void m_beta(float b) { beta = b; }
	
	float alpha,beta; 
	float x,y;

private:
	static void Setup(t_classid c);

	// method callbacks
	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK_F(m_alpha)
	FLEXT_CALLBACK_F(m_beta)

	// define attribute callbacks for variables alpha and beta (with GET and SET properties)
	FLEXT_ATTRVAR_F(alpha)
	FLEXT_ATTRVAR_F(beta)
};

// instantiate the class 
FLEXT_NEW_V("henon",henon)


henon::henon(int argc,const t_atom *argv):
	alpha(0),beta(0),x(0),y(0)  // initializations
{ 
	// define inlets
	AddInAnything("reset,bang,...");  
	AddInFloat("alpha");
	AddInFloat("beta");
	
	// define outlets
	AddOutFloat();  

	// processing command line
	if(argc == 2 && CanbeFloat(argv[0]) && CanbeFloat(argv[1])) {
		// two float args
		alpha = GetAFloat(argv[0]);
		beta = GetAFloat(argv[1]);
	}
} 

void henon::Setup(t_classid c)
{
	// register methods
	FLEXT_CADDBANG(c,0,m_bang);  
	FLEXT_CADDMETHOD_(c,0,"reset",m_reset);  

    // methods for non-left inlets
	FLEXT_CADDMETHOD(c,1,m_alpha);
	FLEXT_CADDMETHOD(c,2,m_beta);  

	// register attributes
	FLEXT_CADDATTR_VAR1(c,"alpha",alpha);  // register attribute "alpha" 
	FLEXT_CADDATTR_VAR1(c,"beta",beta);  // register attribute "beta" 
}

// Trigger output
void henon::m_bang()
{
	float _alpha_ = alpha*1.5f-2.5f;
	float _beta_ = beta-0.5f;

	float newx = 1 + _alpha_ * x * x + _beta_ * y;
	float newy = x;
	x = newx;
	y = newy;

	float output = (x + 1.0f)/2.0f;
	if(output < 0)
		output = 0;
	else if(output > 1)
		output = 1;
	
	// output value to outlet
	ToOutFloat(0,output); // (0 stands for the outlet index 0)
}
