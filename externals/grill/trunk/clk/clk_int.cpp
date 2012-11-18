/* 
clk - syncable clocking objects

Copyright (c)2006-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1313 $
$LastChangedDate: 2008-01-30 10:15:53 -0500 (Wed, 30 Jan 2008) $
$LastChangedBy: thomas $
*/

#include "clk_master.h"

namespace clk {

class Internal
    : public MasterExt
{
    FLEXT_HEADER_S(Internal,MasterExt,Setup)

public:
    Internal(int argc,const t_atom *argv)
        : MasterExt(argc,argv)
        , intv(-1),cnt(0)
    {
		FLEXT_ADDTIMER(check,CbCheck);
    }

    virtual bool Init()
    {
        ms_interval(0);
        return MasterExt::Init();
    }

    void ms_interval(float i)
    {
        FLEXT_ASSERT(clock);

        if(intv == i) return;
    
        // first time hint, or change between physical and logical?
        bool change = intv < 0 || (intv != 0) != (i != 0);

        if(change) {
            check.Reset(); // unset checking timer
            clock->Logical(i == 0);
//            clock->reset();
            settime(flext::GetTime()-1,clock->Time()-1);
        }
        intv = i;

        if(intv)
            // physical, start checking timer
            CbCheck();
        else
            // logical, no checking timer
    	    settime(flext::GetTime(),clock->Time());
    }

	void CbCheck(void * = NULL) 
	{ 
        FLEXT_ASSERT(clock);
		settime(flext::GetTime(),clock->Time());
        check.Delay(intv);
	}

    static void Setup(t_classid c)
    {
		FLEXT_CADDATTR_VAR(c,"interval",intv,ms_interval);
    }

    Timer check;
    double cnt,intv;

	FLEXT_CALLSET_F(ms_interval)
	FLEXT_ATTRGET_F(intv)
    FLEXT_CALLBACK_T(CbCheck)
};

FLEXT_LIB_V("clk.int, clk",Internal)

} // namespace
