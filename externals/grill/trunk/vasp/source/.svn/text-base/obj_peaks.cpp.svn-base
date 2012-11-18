/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_qpeaks
	\remark \b vasp.peaks?
	\brief Get most pronounced peaks of a single vasp vector.
	\since 0.0.6
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet.0 list - peak positions
	\retval outlet.1 list - peak values

	\note Outputs nothing if vasp is undefined or invalid
	\note Only works for a vasp with one vector. No output otherwise.
	\todo Units for peak position list
*/
class vasp_qpeaks:
	public vasp_op
{
	FLEXT_HEADER_S(vasp_qpeaks,vasp_op,Setup)

public:
	vasp_qpeaks(I argc,const t_atom *argv):
		peaks(1)
	{
		if(argc >= 1 && CanbeInt(argv[0]))
			m_peaks(GetAInt(argv[0]));
		else if(argc)
			post("%s - Number argument invalid -> ignored",thisName());

		AddInAnything();
		AddInInt();
		AddOutAnything(2);
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD(c,1,m_peaks);
		FLEXT_CADDATTR_VAR(c,"peaks",peaks,m_peaks);
	}

	V m_peaks(I n) { peaks = n;	}

	virtual V m_bang() 
	{ 
		if(!ref.Ok())
			post("%s - Invalid vasp!",thisName());
		else if(ref.Vectors() > 1) 
			post("%s - More than one vector in vasp!",thisName());
		else {
			VBuffer *buf = ref.Buffer(0);
			I i,cnt = buf->Length(),pkfnd = 0;
			BS *p = buf->Pointer();

			I mxpk = min(cnt,peaks);
			t_atom *pos = new t_atom[mxpk],*lst = new t_atom[mxpk];
			for(i = 0; i < mxpk; ++i) SetFloat(lst[i],0);

			for(i = 0; i < cnt; ++i) {
				const F v = fabs(p[i]);

				if(v && v > GetFloat(lst[mxpk-1])) {
					I ix;

					for(ix = min(pkfnd-1,mxpk-1); ix >= 0; --ix) {
						if(v > GetFloat(lst[ix])) {
							if(ix < mxpk-1) {
								pos[ix+1] = pos[ix];
								lst[ix+1] = lst[ix];
							}
						}
						else break;
					}
					++ix;

					SetFloat(pos[ix],i);
					SetFloat(lst[ix],v);

					if(++pkfnd > mxpk) pkfnd = mxpk;
				}
			}

			ToOutAnything(0,sym_list,pkfnd,pos);
			ToOutAnything(1,sym_list,pkfnd,lst);
			delete[] pos;
			delete[] lst;

            delete buf;
		}
	}

	virtual V m_help() { post("%s - Get list of most pronounced peaks of a vasp vector",thisName()); }

protected:
	I peaks;

private:
	FLEXT_CALLBACK_I(m_peaks);
	FLEXT_CALLSET_I(m_peaks);
	FLEXT_ATTRGET_I(peaks);
};

VASP_LIB_V("vasp.peaks?",vasp_qpeaks)


