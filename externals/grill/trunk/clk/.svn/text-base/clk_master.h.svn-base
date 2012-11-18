/* 
clk - syncable clocking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __CLK_MASTER_H
#define __CLK_MASTER_H

#include "clk.h"

namespace clk {

class Master
	: public Parent
{
    friend class Clock;

protected:
    Master(int argc,const t_atom *argv);
    virtual ~Master(); // must be virtual because we need a polymorphic class here

    void reset() 
    { 
        pre = true;
    }

    float getweight() const
    {
        return LIKELY(clock)?clock->Weight():0;
    }

    void setweight(float w) 
    {
        if(LIKELY(clock))
            clock->Weight(w);
    }

    bool getlogical() const { return LIKELY(clock) && clock->Logical(); }

    void setlogical(bool l) { if(LIKELY(clock)) clock->Logical(l); }

	void settime(double x,double y) 
    { 
//        fprintf(stderr,"TIME %lf %lf\n",x,y);
        if(LIKELY(clock))
            clock->Set(x,y,pre); 
        pre = false;
    }

    void setcurrent(double y) 
    { 
        if(LIKELY(clock))
            clock->Set(clock->Time(),y,pre); 
        pre = false;
    }

    bool pre;
};


class MasterExt
    : public flext_base
    , public Master
{
    FLEXT_HEADER_S(MasterExt,flext_base,Setup)

    friend class ClientExt;

public:
    MasterExt(int argc,const t_atom *argv);

	virtual void m_reset();

    void m_message(int argc,const t_atom *argv) { Forward(sym_message,argc,argv); }
	
    void mg_weight(float &w) const { w = getweight(); }
    void ms_weight(float w) { setweight(w); }

    void mg_logical(bool &l) const { l = getlogical(); }
    void ms_logical(bool l) { setlogical(l); }

protected:

    static const t_symbol *sym_message,*sym_reset;

    void Forward(const t_symbol *sym,int argc,const t_atom *argv);

    void Message(const t_symbol *sym,int argc,const t_atom *argv) 
    { 
        ToOutAnything(GetOutAttr(),sym,argc,argv); 
    }

	double current() const { return clock->Current(); }

    FLEXT_CALLBACK_V(m_message)
	FLEXT_CALLVAR_F(mg_timebase,ms_timebase)
	FLEXT_CALLVAR_F(mg_precision,ms_precision)
    FLEXT_CALLVAR_F(mg_weight,ms_weight)
    FLEXT_CALLVAR_B(mg_logical,ms_logical)

    static void Setup(t_classid c);
};

} // namespace

#endif
