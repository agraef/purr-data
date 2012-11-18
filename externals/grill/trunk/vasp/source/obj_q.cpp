/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_list
	\remark \b vasp.list
	\brief Get samples of a single vasp vector.
	\since 0.0.1
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet vector - vasp samples

	\note Outputs 0 if vasp is undefined or invalid
	\note Only works for a vasp with one vector. No output otherwise.
*/
class vasp_list:
	public vasp_op
{
	FLEXT_HEADER(vasp_list,vasp_op)

public:

	vasp_list()
	{
		AddInAnything();
		AddOutList();
	}

	virtual V m_bang() 
	{ 
		if(!ref.Ok())
			post("%s - Invalid vasp!",thisName());
		else if(ref.Vectors() > 1) 
			post("%s - More than one vector in vasp!",thisName());
		else {
			VBuffer *buf = ref.Buffer(0);
			I cnt = buf->Length();			
			BS const *p = buf->Pointer();
			AtomList lst(cnt);
			for(I i = 0; i < cnt; ++i) SetFloat(lst[i],p[i]);
			ToOutList(0,lst);

            delete buf;
		}
	}

	virtual V m_help() { post("%s - Get list of samples of a vasp vector",thisName()); }
};

VASP_LIB("vasp.list vasp.?",vasp_list)



/*! \class vasp_nonzero
	\remark \b vasp.nonzero
	\brief Get samples of a single vasp vector.
	\since 0.0.2
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet.0 list - non-zero samples positions
	\retval outlet.1 list - non-zero sample values

	\note Outputs 0 if vasp is undefined or invalid
	\note Only works for a vasp with one vector. No output otherwise.
	\todo units for position list
*/
class vasp_nonzero:
	public vasp_op
{
	FLEXT_HEADER(vasp_nonzero,vasp_op)

public:

	vasp_nonzero()
	{
		AddInAnything();
		AddOutList(2);
	}

	virtual V m_bang() 
	{ 
		if(!ref.Ok())
			post("%s - Invalid vasp!",thisName());
		else if(ref.Vectors() > 1) 
			post("%s - More than one vector in vasp!",thisName());
		else {
			VBuffer *buf = ref.Buffer(0);
			I i,cnt = buf->Length(),cp,ci;
			BS const *p = buf->Pointer();
			for(cp = i = 0; i < cnt; ++i) if(p[i]) ++cp;
			
			AtomList pos(cp),lst(cp);
			for(ci = i = 0; ci < cp; ++i)
				if(p[i]) {
					SetFloat(pos[ci],i);
					SetFloat(lst[ci],p[i]);
					++ci;			
				}
			ToOutList(0,pos);
			ToOutList(1,lst);

            delete buf;
		}
	}

	virtual V m_help() { post("%s - Get list of non-zero samples of a vasp vector",thisName()); }
};

VASP_LIB("vasp.nonzero vasp.??",vasp_nonzero)

