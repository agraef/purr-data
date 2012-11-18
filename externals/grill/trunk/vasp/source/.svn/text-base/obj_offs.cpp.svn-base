/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_offset
	\remark \b vasp.offset
	\brief Sets offset of vasp vectors.
	\since 0.0.1
	\param cmdln.1 [_time=0] - offset into buffer(s)
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _time - offset into buffer(s)
	\retval outlet vasp - modified vasp

	\attention Normally vasp vectors have individual offsets - this operations sets all the offsets to equal values.
	\todo Implement unit processing.
*/
class vasp_offset:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_offset,vasp_tx,Setup)

public:
	vasp_offset(I argc,const t_atom *argv):
		offs(0),seto(false)
	{
		if(argc >= 1 && CanbeFloat(argv[0]))
			m_offs(GetAFloat(argv[0]));
		else if(argc)
			post("%s - Offset argument invalid -> ignored",thisName());

		AddInAnything();
		AddInFloat();
		AddOutAnything();
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD(c,1,m_offs);
		FLEXT_CADDATTR_VAR(c,"frames",offs,m_offs);
	}

	V m_offs(F o) 
	{ 
		offs = (I)o; //! \todo unit processing 
		seto = true; 
	}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(seto) ret->Offset(offs);
		return ret;
	}

	virtual V m_help() { post("%s - Set a vasp's offset(s) into the vector buffers",thisName()); }
protected:
	I offs;
	BL seto;

private:
	FLEXT_CALLBACK_F(m_offs);
	FLEXT_CALLSET_I(m_offs);
	FLEXT_ATTRGET_I(offs);
};

VASP_LIB_V("vasp.offset vasp.o",vasp_offset)




/*! \class vasp_doffset
	\remark \b vasp.offset+
	\brief Sets offset of vasp vectors differentially.
	\since 0.0.1
	\param cmdln.1 [_time=0] - increase offset of into buffer(s)
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _time - increase of offset into buffer(s)
	\retval outlet vasp - modified vasp

	\todo Implement unit processing
*/
class vasp_doffset:
	public vasp_offset
{
	FLEXT_HEADER(vasp_doffset,vasp_offset)

public:
	vasp_doffset(I argc,const t_atom *argv): vasp_offset(argc,argv) {}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(seto) ret->OffsetD(offs);
		return ret;
	}

	virtual V m_help() { post("%s - Shift a vasp's offset(s) into the vector buffers",thisName()); }
};

VASP_LIB_V("vasp.offset+ vasp.o+",vasp_doffset)



/*! \class vasp_qoffset
	\remark \b vasp.offset?
	\brief Get offset of singled vector vasp.
	\since 0.0.1
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet _time - offset into vector buffer

	\note Outputs 0 if vasp is undefined or invalid
	\note Only works for a vasp with one vector. No output otherwise.

	\todo Implement unit processing
	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
*/
class vasp_qoffset:
	public vasp_op
{
	FLEXT_HEADER(vasp_qoffset,vasp_op)

public:

	vasp_qoffset()
	{
		AddInAnything();
//		AddOutAnything();
		AddOutFloat();
	}

	virtual V m_bang() 
	{ 
		if(!ref.Ok())
			post("%s - Invalid vasp!",thisName());
		else if(ref.Vectors() > 1) 
			post("%s - More than one vector in vasp!",thisName());
		else {
			I o = 0;
			if(ref.Vectors() == 1) {
				o = ref.Vector(0).Offset();
				if(o < 0) o = 0;
				else {
					VBuffer *buf = ref.Buffer(0);
					if(buf) {
						I f = buf->Frames();
						if(o > f) o = f;
						delete buf;
					}
				}
			}
			//! \todo unit processing
//			ToOutVasp(0,ref);
			ToOutFloat(0,o);
		}
	}

	virtual V m_help() { post("%s - Get a single vectored vasp's offset into the buffer",thisName()); }
};

VASP_LIB("vasp.offset? vasp.o?",vasp_qoffset)

