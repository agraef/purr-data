/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_sync
	\remark \b vasp.sync
	\brief Waits for all inlets to be hit (by vasps/anything) to trigger output.
	\since 0.0.1
	\param cmdln.1 int - number of sync inlets
	\param inlet.1 vasp - is stored 
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
	\param inlet.1 reset - clear all hit flags
	\param inlet.+n vasp/anything - sets hit flag
	\retval outlet.* vasp - stored vasps

	\todo Message for selection if only vasp input triggers (or any one).
	\todo Message for selection of manual or auto reset upon trigger
*/
class vasp_sync:
	public vasp_op
{
	FLEXT_HEADER_S(vasp_sync,vasp_op,Setup)

public:
	vasp_sync(I argc,const t_atom *argv):
		autoreset(true),vasponly(false)
	{
		I cnt = -1;
		if(argc) {
			if(CanbeInt(argv[0])) cnt = GetAInt(argv[0]);
			if(cnt <= 1) {
				post("%s - integer argument invalid: set to 2",thisName());
				cnt = 2;
			}
		}
		else cnt = 2;

		flags = new BL[cnt];
		stored = new Vasp[cnt-1];

		AddInAnything(cnt);
		AddOutAnything(cnt);
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
	}

	virtual BL Init() 
	{ 
		BL ret = vasp_op::Init(); 
		m_reset(); 
		return ret; 
	}

	~vasp_sync()	
	{ 
		if(flags) delete[] flags; 
		if(stored) delete[] stored; 
	}

	V chkbang(I n,Vasp *a = NULL) 
	{
		if(a && n > 0) { 
			stored[n-1] = *a;
			delete a;
		}

		BL f = flags[n]; 
		flags[n] = true; 
		if(!f) { // flags have changed
			
			BL all = true;
			for(I i = 0; i < CntIn(); ++i) all = all && flags[i];

			if(all) {
				if(ref.Ok()) {
					for(I i = CntIn()-1; i > 0; --i) ToOutVasp(i,stored[i-1]);
					ToOutVasp(0,ref);
				}
				else ToOutBang(0);

				if(autoreset) m_reset();
			}
		}
	}

	virtual V m_bang() { chkbang(0); }

	V m_reset() 
	{ 
		for(I i = 0; i < CntIn(); ++i) flags[i] = false;
	}

	virtual bool m_method_(I inlet,const t_symbol *s,I argc,const t_atom *argv)
	{
		if(inlet > 0 && (!vasponly || s == sym_vasp)) {
			Vasp *a = new Vasp(argc,argv);
			chkbang(inlet,a);
			return true;
		}
		else
			return vasp_op::m_method_(inlet,s,argc,argv);
	}

	virtual V m_help() { post("%s - Synchronize a number of vasps (default 2)",thisName()); }
private:
	BL autoreset,vasponly;
	BL *flags;
	Vasp *stored;

	FLEXT_CALLBACK(m_reset)
};

VASP_LIB_V("vasp.sync",vasp_sync)


