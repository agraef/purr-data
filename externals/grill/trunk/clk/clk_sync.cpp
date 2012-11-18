/* 
clk - syncable clocking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1313 $
$LastChangedDate: 2008-01-30 10:15:53 -0500 (Wed, 30 Jan 2008) $
$LastChangedBy: thomas $
*/

#include "clk_master.h"

namespace clk {

class Sync
    : public MasterExt
{
    FLEXT_HEADER_S(Sync,MasterExt,Setup)

public:
    Sync(int argc,const t_atom *argv)
        : MasterExt(argc,argv)
    {
    }

	void m_set(double y) 
    { 
        setcurrent(y); 
    }

    void m_double(float a,float b)  
    { 
        setcurrent((double)a+(double)b); 
    }

protected:
    FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK_F(m_set)
	FLEXT_CALLBACK_FF(m_double)

    static void Setup(t_classid c)
    {
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);

		FLEXT_CADDMETHOD(c,0,m_set);

        FLEXT_CADDMETHOD_FF(c,0,sym_list,m_double);
		FLEXT_CADDMETHOD_FF(c,0,"double",m_double);
    }
};

FLEXT_LIB_V("clk.sync, clk",Sync)

} // namespace
