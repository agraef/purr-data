/* 
clk - syncable clocking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __CLK_CLIENT_H
#define __CLK_CLIENT_H

#include "clk.h"

namespace clk {

class Client
	: public Parent
{
    friend class Clock;

protected:
	Client(int argc,const t_atom *argv);
    virtual ~Client();

    virtual void Update(double told,double tnew,bool missedmsg = true) = 0;

    void Update() 
    { 
        if(clock) { 
            double t = clock->Current(); 
            Update(t,t,false); 
        }
    }

    void ms_name(int argc = 0,const t_atom *argv = NULL);

    void ms_name(const AtomList &l) 
    { 
        ms_name(l.Count(),l.Atoms()); 
    }

	void mg_name(AtomList &l) 
    { 
        if(LIKELY(clock)) 
            SetSymbol(l(1)[0],clock->name); 
    }

	void m_reset(int argc,const t_atom *argv) 
	{
		offset = LIKELY(clock)?-clock->Current():0;
		if(argc)
			offset += GetDouble(argc,argv);
	} 

	double Convert(double time) const 
    { 
        return (time+offset)*factor; 
    }

	double Current(double offs = 0) const 
    { 
        return Convert(clock->Get(clock->Time()+offs)); 
    }

    void Factor(double f) 
    {
        if(LIKELY(clock)) {
            double current = clock->Current();
            offset = (current+offset)*factor/f-current;
        }
		factor = f; 
    }

    double Factor() const { return factor; }

    void Offset(double o) { offset = o; }

    double Offset() const { return offset; }

	Timer timer;

private:
	double offset,factor;
};

class ClientExt
    : public flext_dsp
    , public Client
{
    FLEXT_HEADER_S(ClientExt,flext_dsp,Setup)

    friend class MasterExt;

public:
    ClientExt(int argc,const t_atom *argv);

	void m_get(double offs = 0);

    void ms_factor(const AtomList &l) 
    { 
		double f = GetDouble(l);	
        if(UNLIKELY(f <= 0))
		    post("%s - factor must be > 0",thisName()); 
        else if(LIKELY(Factor() != f)) {
            Factor(f);
            Update();
        }
    } 

    void mg_factor(AtomList &l) 
	{ 
		double d = Factor();
		if(dblprec) 
			SetDouble(l,d);
		else
			SetFloat(l(1)[0],static_cast<float>(d));
	}

    void ms_offset(const AtomList &l) 
    { 
		double o = GetDouble(l);
        if(LIKELY(Offset() != o)) {
            Offset(o);
            Update();
        }
    }

    void mg_offset(AtomList &l) 
	{ 
		double d = Offset();
		if(dblprec) 
			SetDouble(l,d);
		else
			SetFloat(l(1)[0],static_cast<float>(d));
	}

    void m_message(int argc,const t_atom *argv) { Forward(sym_message,argc,argv); }

protected:

    static const t_symbol *sym_message;

    void Forward(const t_symbol *sym,int argc,const t_atom *argv);

    void Message(const t_symbol *sym,int argc,const t_atom *argv) 
    { 
        ToOutAnything(GetOutAttr(),sym,argc,argv); 
    }

	virtual bool CbDsp();

	void setcnv() { ticks2s = (double)Blocksize()/(double)Samplerate(); }


	FLEXT_CALLSET_V(ms_name)
	FLEXT_CALLGET_V(mg_name)

	FLEXT_CALLBACK_V(m_reset)
	FLEXT_CALLBACK_V(m_message)

    FLEXT_CALLVAR_V(mg_offset,ms_offset)
	FLEXT_CALLVAR_V(mg_factor,ms_factor)

	FLEXT_CALLGET_F(mg_timebase)

	FLEXT_ATTRVAR_B(dblprec)
	FLEXT_ATTRVAR_B(t3mode)

    static void Setup(t_classid c);

	bool dblprec;
    bool t3mode;
    double ticks2s;
};

} // namespace

#endif
