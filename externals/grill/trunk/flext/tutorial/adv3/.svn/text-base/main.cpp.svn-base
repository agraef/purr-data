/* 
flext tutorial - advanced 3

Copyright (c) 2002-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is a port of Iohannes Zmölnigs "counter" example to the flext paradigm.
Find the original at http://iem.kug.ac.at/pd/externals-HOWTO/node5.html

The functionality is exactly the same, with one exception:
flext doesn't support default arguments, hence a message "bound 1" will translate into
"bound 1 0" in the original example, but won't be recognized with flext.
This can be easily circumvented by using a method digesting a variable argument list, but
was omitted for the sake of clearness.

Apart from that you'll notice several differences to the original C object:
- with flext, callbacks have to be declared for all registered methods
- Flext allows the full usage of integer types 
- there are no real "passive" methods with flext. 
	These can be emulated by methods, or more flexibly, attributes (see example "attr3")
- Help symbols can't be defined that freely. This is because in Max/MSP help files always
	have the name of the object with a suffix .help appended.
	However with flext, a path to the respective help file may be specified
*/

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 501)
#error You need at least flext version 0.5.1
#endif

class adv3:
	public flext_base
{
	FLEXT_HEADER_S(adv3,flext_base,setup)
 
public:
	// constructor with no arguments
	adv3(int argc,t_atom *argv):
		i_step(1)
	{
		// --- initialize bounds and step size ---
		int f1 = 0,f2 = 0;
		switch(argc) {
			default:
			case 3:
				i_step = GetInt(argv[2]);
			case 2:
				f2 = GetInt(argv[1]);
			case 1:
				f1 = GetInt(argv[0]);
			case 0:
				;
		}
		if(argc < 2) f2 = f1;

		m_bound(f1,f2);

		i_count = i_down;

		// --- define inlets and outlets ---
		AddInAnything("bang, reset, etc."); // default inlet
		AddInList("bounds (2 element list)");	// inlet for bounds
		AddInInt("step size");		// inlet for step size

		AddOutInt("counter");	// outlet for integer count
		AddOutBang("overflow bang");	// outlet for bang
	}

protected:

	void m_reset() 
	{ 
		i_count = i_down; 
	}

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

	void m_bound(int f1,int f2)
	{
		i_down = f1 < f2?f1:f2;
		i_up   = f1 > f2?f1:f2;
	}

	void m_step(int s)
	{
		i_step = s;
	}

	int i_count,i_down,i_up,i_step;
	
private:

	static void setup(t_classid c)
	{
		// --- set up methods (class scope) ---

		// register a bang method to the default inlet (0)
		FLEXT_CADDBANG(c,0,m_bang);

		// set up tagged methods for the default inlet (0)
		// the underscore _ after CADDMETHOD indicates that a message tag is used
		// no, variable list or anything and all single arguments are recognized automatically, ...
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
		FLEXT_CADDMETHOD_(c,0,"set",m_set);
		// ..., more complex types (combinations of types) have to be specified explicitly
		FLEXT_CADDMETHOD_II(c,0,"bound",m_bound);  // two int arguments

		// set up methods for inlets 1 and 2
		// no message tag used
		FLEXT_CADDMETHOD(c,1,m_bound);  // variable arg type recognized automatically
		FLEXT_CADDMETHOD(c,2,m_step);	// single int arg also recognized automatically
	}

	// for every registered method a callback has to be declared
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK_V(m_set)
	FLEXT_CALLBACK_II(m_bound)
	FLEXT_CALLBACK_I(m_step)
};

// instantiate the class (constructor has a variable argument list)
// let "counter" be an alternative name
// after the colon define the path/name of the help file (a path has a trailing /, a file has not)
FLEXT_NEW_V("adv3 counter,help/",adv3)


