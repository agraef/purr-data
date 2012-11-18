/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_part
	\remark \b vasp.part
	\brief Gets parts of vasp vectors.
	\since 0.0.1
	\param cmdln.1 list - list of part lengts
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 list - list of part lengts
 	\retval outlet.1 vasp - consecutive vasp parts
 	\retval outlet.2 vasp - remainder

	\todo Implement unit processing.
	\remarks Output zero length vasps?
*/
class vasp_part:
	public vasp_op
{
	FLEXT_HEADER_S(vasp_part,vasp_op,Setup)

public:
	vasp_part(I argc,const t_atom *argv):
		parts(0),part(NULL)
	{
		m_part(argc,argv);

		AddInAnything(2);
		AddOutAnything(2);
	}

	~vasp_part() { if(part) delete[] part; }

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD_(c,1,"list",m_part);
		FLEXT_CADDATTR_VAR(c,"parts",m_getpart,m_setpart);
	}

	V m_part(I argc,const t_atom *argv) 
	{ 
		if(part) delete[] part; parts = 0;
		part = new I[argc]; 
		for(I i = 0; i < argc; ++i) {
			BL warn = false;
			I p = (I)GetAFloat(argv[i]); // \todo unit processing
			if(p < 0 && !warn) {
				post("%s - invalid part length(s) -> set to 0",thisName());
				p = 0; warn = true;
			}
			part[i] = p; ++parts;
		}
	}

	V m_getpart(AtomList &ret) 
	{
		ret(parts);
		for(I i = 0; i < parts; ++i) SetInt(ret[i],part[i]);
	}

	V m_setpart(const AtomList &ret) { m_part(ret.Count(),ret.Atoms()); }

	virtual V m_bang() 
	{ 
		if(!ref.Ok()) {
			post("%s - Invalid vasp!",thisName());
			return;
		}

		I fr = ref.ChkFrames(),o = 0;
		for(I i = 0; i < parts && (fr < 0 || fr); ++i) {
			I p = part[i];
			if(fr >= 0) { p = min(p,fr); fr -= p; }

			Vasp ret(ref); 
			ret.Frames(p);
			ret.OffsetD(o);
			ToOutVasp(0,ret);

			o += p;
		}

		if(fr) {
			Vasp ret(ref); 
			ret.Frames(fr);
			ret.OffsetD(o);
			ToOutVasp(1,ret);
		}
	}

	virtual V m_help() { post("%s - Return consecutive vasps with lengths given by argument list",thisName()); }
protected:
	I parts,*part;

	FLEXT_CALLBACK_V(m_part)
	FLEXT_CALLVAR_V(m_getpart,m_setpart);
};

VASP_LIB_V("vasp.part",vasp_part)


