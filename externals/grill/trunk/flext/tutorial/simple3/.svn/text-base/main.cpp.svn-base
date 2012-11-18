/* 
flext tutorial - simple 3

Copyright (c) 2002-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of an object digesting several "tagged" messages

*/

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

class simple3:
	public flext_base
{
	FLEXT_HEADER(simple3,flext_base)
 
public:
	// constructor with no arguments
	simple3();

protected:
	void m_tag();   
	void m_tag_and_int(int i);   
	void m_sym(t_symbol *s);   

	// override default flext help function	
	void m_help();
	
private:

	FLEXT_CALLBACK(m_tag)  // callback for method "m_tag" (no arguments)
	FLEXT_CALLBACK_I(m_tag_and_int)  // callback for method "m_tag_and_int" (int arguments)
	FLEXT_CALLBACK_S(m_sym)  // callback for method "m_sym" (with one symbol argument)

	FLEXT_CALLBACK(m_help)  // callback for method "m_help" (no arguments)
};

// instantiate the class (constructor takes no arguments)
FLEXT_NEW("simple3",simple3)


simple3::simple3()
{ 
	// define inlets
	AddInAnything();  // add inlet of type anything (index 0)

	// register methods
	FLEXT_ADDMETHOD_(0,"born",m_tag);  // register method for tag "born"
	FLEXT_ADDMETHOD_(0,"to",m_tag);  // register method for tag "to"
	FLEXT_ADDMETHOD_(0,"hula",m_tag);  // register method for tag "hula"
	FLEXT_ADDMETHOD_I(0,"hula",m_tag_and_int);  // register method for tag "hula" and int argument

	FLEXT_ADDMETHOD(0,m_sym);  // register method for all other symbols

	FLEXT_ADDMETHOD_(0,"help",m_help);  // register method for "help" message
} 

void simple3::m_tag()
{
	post("tag recognized");
}

void simple3::m_tag_and_int(int i)
{
	post("tag recognized (has int arg: %i)",i);
}

void simple3::m_sym(t_symbol *s)
{
	post("symbol: %s",GetString(s));
}


void simple3::m_help()
{
	// post a help message
	// thisName() returns a char * for the object name
	post("%s - example for tagged messages",thisName());
}
	

