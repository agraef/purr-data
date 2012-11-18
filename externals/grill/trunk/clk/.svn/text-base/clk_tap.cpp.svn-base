/* 
clk - syncable clocking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#include "clk_master.h"

namespace clk {

class Tap
    : public MasterExt
{
    FLEXT_HEADER_S(Tap,MasterExt,Setup)

public:
    Tap(int argc,const t_atom *argv)
        : MasterExt(argc,argv)
        , taps(0)
    {
    }

    virtual void m_reset() { thisParent::m_reset(); taps = 0; }
	
	void m_tap() { setcurrent(taps++); }

protected:
    FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK(m_tap)

    FLEXT_ATTRGET_I(taps)

    static void Setup(t_classid c)
    {
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
		FLEXT_CADDMETHOD(c,0,m_tap); // bang

        FLEXT_CADDATTR_GET(c,"taps",taps);
    }

	int taps;
};

FLEXT_LIB_V("clk.tap, clk",Tap)

} // namespace
