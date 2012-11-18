/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_size
	\remark \b vasp.size
	\brief Resize buffer.
	\since 0.0.6
	\param cmdln.1 [_time=0] - size of buffer
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _time - offset into buffer(s)
	\retval outlet vasp - modified vasp

	\attention Normally vasp vectors have individual offsets - this operations sets all the buffer sizes to equal values.
	\todo Implement unit processing.
*/
class vasp_size:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_size,vasp_tx,Setup)

public:
	vasp_size(I argc,const t_atom *argv,BL abs = true):
		size(0),sets(false),keep(true),zero(true)
	{
		if(argc >= 1 && CanbeFloat(argv[0]))
			m_arg(GetAFloat(argv[0]));
		else if(argc)
			post("%s - Offset argument invalid -> ignored",thisName());

		AddInAnything();
		AddInFloat();
		AddOutAnything();

		if(abs) FLEXT_ADDATTR_VAR("frames",size,m_arg);
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD(c,1,m_arg);
		FLEXT_CADDATTR_VAR1(c,"keep",keep);
		FLEXT_CADDATTR_VAR1(c,"zero",zero);
	}

	virtual V m_arg(F s) 
	{ 
		size = (I)s;  // \todo unit processing 
		sets = true;
	}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(sets) ret->Size(size,keep,zero);
		return ret;
	}

	virtual V m_help() { post("%s - Set the size of the vector buffers",thisName()); }
protected:
	I size;
	BL sets,keep,zero;

private:
	FLEXT_CALLBACK_F(m_arg);
	FLEXT_CALLSET_I(m_arg);
	FLEXT_ATTRGET_I(size);
	FLEXT_ATTRVAR_B(keep);
	FLEXT_ATTRVAR_B(zero);
};

VASP_LIB_V("vasp.size vasp.s",vasp_size)




/*! \class vasp_dsize
	\remark \b vasp.size+
	\brief Sets vector buffer sizes differentially.
	\since 0.0.6
	\param cmdln.1 [_time=0] - increase offset of into buffer(s)
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _time - increase of offset into buffer(s)
	\retval outlet vasp - modified vasp

	\todo Implement unit processing
*/
class vasp_dsize:
	public vasp_size
{
	FLEXT_HEADER(vasp_dsize,vasp_size)

public:
	vasp_dsize(I argc,const t_atom *argv): vasp_size(argc,argv) {}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(sets) ret->SizeD(size);
		return ret;
	}

	virtual V m_help() { post("%s - Increase the size of the vector buffers",thisName()); }
};

VASP_LIB_V("vasp.size+ vasp.s+",vasp_dsize)



/*! \class vasp_msize
	\remark \b vasp.size*
	\brief Sets vector buffer sizes by a factor
	\since 0.0.6
	\param cmdln.1 [_number=1] - factor for size
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _number - factor for size
	\retval outlet vasp - modified vasp
*/
class vasp_msize:
	public vasp_size
{
	FLEXT_HEADER_S(vasp_msize,vasp_size,Setup)

public:
	vasp_msize(I argc,const t_atom *argv): 
		vasp_size(argc,argv,false) 
	{
		if(argc && CanbeFloat(argv[0])) m_arg(GetAFloat(argv[0]));
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDATTR_VAR(c,"factor",factor,m_arg);
	}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(sets) ret->SizeM(factor);
		return ret;
	}

	virtual V m_help() { post("%s - Multiply the size of the vector buffers",thisName()); }

	virtual V m_arg(F f) 
	{ 
		factor = f; 
		sets = true;
	}

protected:
	R factor;
	FLEXT_CALLSET_F(m_arg);
	FLEXT_ATTRGET_F(factor);
};

VASP_LIB_V("vasp.size* vasp.s*",vasp_msize)



/*! \class vasp_rsize
	\remark \b vasp.size/
	\brief Sets vector buffer sizes by a factor
	\since 0.0.6
	\param cmdln.1 [_number=1] - divisor for size
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _number - divisor for size
	\retval outlet vasp - modified vasp
*/
class vasp_rsize:
	public vasp_msize
{
	FLEXT_HEADER(vasp_rsize,vasp_msize)

public:
	vasp_rsize(I argc,const t_atom *argv): vasp_msize(argc,argv) {}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(sets) ret->SizeR(factor);
		return ret;
	}

	virtual V m_help() { post("%s - Divide the size of the vector buffers",thisName()); }
};

VASP_LIB_V("vasp.size/ vasp.s/",vasp_rsize)



/*! \class vasp_qsize
	\remark \b vasp.size?
	\brief Get size of a vector buffer.
	\since 0.0.6
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
class vasp_qsize:
	public vasp_op
{
	FLEXT_HEADER(vasp_qsize,vasp_op)

public:

	vasp_qsize()
	{
		AddInAnything();
//		AddOutAnything();
		AddOutFloat();
	}

	virtual V m_bang() 
	{ 
		if(!ref.Check())
			post("%s - Invalid vasp!",thisName());
		else if(ref.Vectors() > 1) 
			post("%s - More than one vector in vasp!",thisName());
		else {
			I s = 0;
			if(ref.Vectors() == 1) {
				VBuffer *buf = ref.Buffer(0);
				if(buf) {
					s = buf->Frames();
					delete buf;
				}
			}
			//! \todo unit processing
//			ToOutVasp(0,ref);
			ToOutFloat(0,s);
		}
	}

	virtual V m_help() { post("%s - Get the buffer size of a vector",thisName()); }
};

VASP_LIB("vasp.size? vasp.s?",vasp_qsize)

