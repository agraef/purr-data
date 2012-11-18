/* 
flext tutorial - advanced 1

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of a simplified prepend object
*/

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1
#endif


class adv1:
	public flext_base
{
	FLEXT_HEADER(adv1,flext_base)
 
public:
	// constructor with variable argument list
	adv1(int argc,t_atom *argv);

protected:
	void m_any(const t_symbol *s,int argc,t_atom *argv);  // method which digests anything

	AtomList lst;
private:
	FLEXT_CALLBACK_A(m_any);  // callback for method "m_any" (with anything argument)	
};

// instantiate the class (constructor has a variable argument list)
// note the two words in the string: prepend acts as an alias for adv1!
FLEXT_NEW_V("adv1 prepend",adv1)



// constructor

adv1::adv1(int argc,t_atom *argv)
{ 
	AddInAnything();  // one inlet that can receive anything 
	AddOutAnything();  // one outlet for anything 
	
	// register method
	FLEXT_ADDMETHOD(0,m_any);  // register method "m_any" for inlet 0
	
	if(argc != 0) { // check for arg count
		// store arg list
		lst(argc,argv);
	}
	else { 
		// no args given
		post("%s - no arguments given",thisName());
		
		// tell flext that the initialization was not successful... object will not live
		InitProblem();
	}
} 



// method

void adv1::m_any(const t_symbol *s,int argc,t_atom *argv)
{
	// reserve space for as many atoms as possibly necessary
	AtomList result(lst.Count()+argc+2);

	// ix is our counter of atoms to output
	int ix = 0; 

	int i = 0;
	if(!IsSymbol(lst[0])) {
		// if first element to prepend is no symbol then make it a "list"
		SetSymbol(result[ix++],sym_list);
	}
	// copy atoms to prepend to result list
	for(; i < lst.Count(); ++i) CopyAtom(&result[ix++],&lst[i]);

	// if anything is no "list" or "float" then append it to result list
	if(s != sym_list && s != sym_float
#if FLEXT_SYS == FLEXT_SYS_MAX
		 && s != sym_int  // in Max integers are system data types
#endif
	)
		SetSymbol(result[ix++],s); 

	// append pending arguments to result list
	for(i = 0; i < argc; ++i) CopyAtom(&result[ix++],argv+i);

	// output result list as an anything
	ToOutAnything(0,GetSymbol(result[0]),ix-1,result.Atoms()+1);
}


