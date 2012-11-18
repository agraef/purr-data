/* 
flext tutorial - bind 1 

Copyright (c) 2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of a simple object demonstrating method to symbol binding and message forwarding
*/


// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


// define the class that stands for a pd/Max object

class bind1:
	// inherit from basic flext class
	public flext_base
{
	// obligatory flext header (class name,base class name) featuring a setup function
	FLEXT_HEADER_S(bind1,flext_base,setup)
 
public:
	// constructor with no arguments
	bind1()
	{ 
		// define inlets:
		// first inlet must always be of type anything (or signal for dsp objects)
		AddInAnything("message inlet");  // add one inlet for any message
		AddInAnything("forwarding inlet");  // add one inlet for any message	
		
		AddOutAnything("bound message");  // output received bound message
	}	

    /*
        no destructor necessary here:
        flext frees all eventually remaining bound symbols when the object is destroyed
        (but NOT the data that can be passed via the FLEXT_BINDMETHOD call!)
    */

protected:
	const t_symbol *bufname;
	buffer *buf;

	// bind object
	void m_bind(const t_symbol *s)
	{
		if(!Bind(s)) {
			post("%s (%s) - Binding failed",thisName(),GetString(thisTag()));
		}
	}  

	// unbind object
	void m_unbind(const t_symbol *s)
	{
		if(!Unbind(s)) {
			post("%s (%s) - Binding failed",thisName(),GetString(thisTag()));
		}
	}

	// bind method
	void m_bindmethod(const t_symbol *s)
	{
		if(!FLEXT_BINDMETHOD(s,m_bound,NULL)) {
			post("%s (%s) - Binding failed",thisName(),GetString(thisTag()));
		}
	}

	// unbind method
	void m_unbindmethod(const t_symbol *s)
	{
		if(!FLEXT_UNBINDMETHOD(s)) {
			post("%s (%s) - Binding failed",thisName(),GetString(thisTag()));
		}
	}

	// forward message
	void m_forward(const t_symbol *s,int argc,const t_atom *argv)
	{
		Forward(s,argc,argv);
	}

	// method for symbol-bound messages
	void m_bound(const t_symbol *sym,int argc,const t_atom *argv,void *data)
	{
		ToOutAnything(0,sym,argc,argv);
	}

	// method for binding test
	void m_test(float value)
	{
		post("%s - TEST METHOD: value %f",thisName(),value);
	}

private:
	static void setup(t_classid c)
	{
		// register methods
		
		FLEXT_CADDMETHOD_(c,0,"bind",m_bind);  // register method "bind" for inlet 0
		FLEXT_CADDMETHOD_(c,0,"unbind",m_unbind);  // register method "unbind" for inlet 0
		FLEXT_CADDMETHOD_(c,0,"bindmethod",m_bindmethod);  // register method "bindmethod" for inlet 0
		FLEXT_CADDMETHOD_(c,0,"unbindmethod",m_unbindmethod);  // register method "unbindmethod" for inlet 0

		FLEXT_CADDMETHOD_(c,0,"test",m_test);  // register method m_test for inlet 0

		FLEXT_CADDMETHOD(c,1,m_forward);  // register method m_forward for inlet 1
	}

	FLEXT_CALLBACK_S(m_bind)  // wrapper for method m_bind (with symbol argument)
	FLEXT_CALLBACK_S(m_unbind)  // wrapper for method m_unbind (with symbol argument)
	FLEXT_CALLBACK_S(m_bindmethod)  // wrapper for method m_bindmethod (with symbol argument)
	FLEXT_CALLBACK_S(m_unbindmethod)  // wrapper for method m_unbindmethod (with symbol argument)

	FLEXT_CALLBACK_A(m_forward) // wrapper for method m_forward (with anything argument)
	
	FLEXT_CALLBACK_AX(m_bound)  // wrapper for method m_bound (anything+data arguments)
	FLEXT_CALLBACK_F(m_test)    // wrapper for method m_test (one float argument)
};

// instantiate the class
FLEXT_NEW("bind1",bind1)



