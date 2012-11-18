/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file vasp_imm.cpp
	\brief Definitions for immediate vasps
*/

#include "main.h"
#include "classes.h"
#include "util.h"
#include "buflib.h"
#include "oploop.h"


/*! \class vasp_imm
	\remark \b vasp.imm
	\brief Get vasp immediate.
	\since 0.0.6
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
	\param inlet.1 frames - minimum frame length
	\param inlet.2 int - minimum frame length
	\retval outlet vasp! - vasp immediate

*/
class vasp_imm:
	public vasp_op
{
	FLEXT_HEADER_S(vasp_imm,vasp_op,Setup)

public:
	vasp_imm(I argc,const t_atom *argv):
		frms(0),zero(true)
	{
		if(argc >= 1 && CanbeInt(argv[0]))
			m_frames(GetAInt(argv[0]));
		else if(argc)
			post("%s - Frame count argument invalid -> ignored",thisName());

		AddInAnything();
		AddInInt();
		AddOutAnything();
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD(c,1,m_frames);
		FLEXT_CADDATTR_VAR(c,"frames",frms,m_frames);
		FLEXT_CADDATTR_VAR1(c,"zero",zero);
	}

	V m_frames(I n) { frms = n; }

	virtual V m_bang() 
	{ 
		if(!ref.Ok() || !ref.Check()) {
/*
			if(!frms) 
				post("%s - No length defined!",thisName());
			else 
*/
			{
				ImmBuf ibuf(frms,zero);
				Vasp ret(frms,Vasp::Ref(ibuf));
                ToOutVasp(0,ret);
			}
		}
		else if(ref.Vectors() > 1) 
			post("%s - More than one vector in vasp!",thisName());
		else {
			VBuffer *buf = ref.Buffer(0);
			const I len = buf->Length(),chns = buf->Channels();

			// size of memory reservation (at least frms samples)
			const I rlen = frms > len?frms:len; 
			
			ImmBuf imm(rlen,false);

			BS *dst = imm.Pointer();
			const BS *src = buf->Pointer();

//			post("!copy: src: %p,%i,%i -> dst: %p,%i",src,len,chns,dst,rlen);

			register int i;
			_DE_LOOP(i,len, ( dst[i] = *src,src += chns ) )
			if(zero && rlen > len) ZeroSamples(dst+len,rlen-len);

			Vasp::Ref vr(imm);

//			post("!vr: %s,%i",vr.Ok()?vr.Symbol().Name():"***",vr.Offset());

			Vasp ret(len,vr);
			ToOutVasp(0,ret);

            delete buf;
		}
	}

	virtual V m_help() { post("%s - Get immediate vasp vectors",thisName()); }

protected:

	I frms;
	BL zero;
	
private:
	FLEXT_CALLBACK_I(m_frames)
	FLEXT_CALLSET_I(m_frames);
	FLEXT_ATTRGET_I(frms);
	FLEXT_ATTRVAR_B(zero);
};

VASP_LIB_V("vasp.imm vasp.!",vasp_imm)


