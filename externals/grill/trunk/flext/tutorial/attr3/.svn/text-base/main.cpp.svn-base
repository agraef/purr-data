/* 
flext tutorial - attributes 3

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is tutorial example "advanced 3" with the usage of attributes.

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


class attr3:
	public flext_base
{
	FLEXT_HEADER_S(attr3,flext_base,setup)
 
public:
	// constructor with no arguments
	// initial values are set by attributes at creation time (see help file)
	attr3():
		// initialize data members
		i_down(0),i_up(0),i_count(0),i_step(1)
	{
		// --- define inlets and outlets ---
		AddInAnything(); // default inlet
		AddInList();	// inlet for bounds
		AddInInt();		// inlet for step size

		AddOutInt();	// outlet for integer count
		AddOutBang();	// outlet for bang
	}

protected:

	void m_reset() { i_count = i_down; }

	void m_set(int argc,t_atom *argv) 
	{ 
		i_count = argc?GetAInt(argv[0]):0; 
	}

	void m_bang()
	{
		int f = i_count;
		i_count += i_step;
		if(i_down != i_up) {
			if((i_step > 0) && (i_count > i_up)) {
				i_count = i_down;
				ToOutBang(1);
			} 
			else if(i_count < i_down) {
				i_count = i_up;
				ToOutBang(1);
			}
		}
		ToOutInt(0,f);
	}

	void m_bounds(int f1,int f2)
	{
		i_down = f1 < f2?f1:f2;
		i_up   = f1 > f2?f1:f2;
	}

	void m_step(int s) { i_step = s; }

	
	// setter method of bounds variables
	void ms_bounds(const AtomList &l)
	{
		if(l.Count() == 2 && CanbeInt(l[0]) && CanbeInt(l[1]))
			// if it is a two element integer list use m_bounds method
			m_bounds(GetAInt(l[0]),GetAInt(l[1]));
		else
			// else post a warning
			post("%s - 'bounds' needs two integer parameters",thisName());
	}

	// getter method of bounds variables
	void mg_bounds(AtomList &l) const
	{
		l(2); // initialize two element list
		SetInt(l[0],i_down); // set first element
		SetInt(l[1],i_up);	// set second element
	}


	int i_count,i_down,i_up,i_step;

private:

	static void setup(t_classid c)
	{
		// --- set up methods (class scope) ---

		// register a bang method to the default inlet (0)
		FLEXT_CADDBANG(c,0,m_bang);

		// set up tagged methods for the default inlet (0)
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
		FLEXT_CADDMETHOD_(c,0,"set",m_set);

		// set up methods for inlets 1 and 2
		// no message tag used
		FLEXT_CADDMETHOD(c,1,m_bounds);  // variable arg type recognized automatically
		FLEXT_CADDMETHOD(c,2,m_step);	// single int arg also recognized automatically

		// --- set up attributes (class scope) ---

		// these have equally named getters and setters 
		// see the wrappers below
		FLEXT_CADDATTR_VAR1(c,"count",i_count);  
		FLEXT_CADDATTR_VAR1(c,"step",i_step);  

		// bounds has differently named getter and setter functions
		// see the wrappers below
		FLEXT_CADDATTR_VAR(c,"bounds",mg_bounds,ms_bounds);  
	}

	// normal method callbacks for bang and reset
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK(m_reset)

	FLEXT_CALLBACK_V(m_set)  // normal method wrapper for m_set
	FLEXT_ATTRVAR_I(i_count) // wrapper functions (get and set) for integer variable i_count

	FLEXT_CALLBACK_II(m_bounds) // normal method wrapper for m_bounds
	FLEXT_CALLVAR_V(mg_bounds,ms_bounds) // getter and setter method of bounds

	FLEXT_CALLBACK_I(m_step)  // normal method wrapper for m_step
	FLEXT_ATTRVAR_I(i_step) // wrapper functions (get and set) for integer variable i_step
};


// instantiate the class (constructor takes no arguments)
FLEXT_NEW("attr3",attr3)


