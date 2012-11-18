/* 
clk - syncable clocking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#include "clk_client.h"
#include <cmath>

#define LIMIT 1.e-4f

namespace clk {

class Metro
    : public ClientExt
{
    FLEXT_HEADER_S(Metro,ClientExt,Setup)

public:
    Metro(int argc,const t_atom *argv)
        : ClientExt(argc,argv)
        , scheduled(-1)
        , limit(LIMIT)
    {
		FLEXT_ADDTIMER(timer,CbTimer);
    }

	void m_metro(double intv,double offs = 0) 
	{ 
		if(LIKELY(clock)) {
            tickoffs = 0;
            SchedDelay(perintv = intv);
        }
	}

	void m_metro2(float intv1,float intv2) { m_metro((double)intv1+(double)intv2); }
	
	void m_stop() 
    { 
        timer.Reset(); 
        scheduled = -1;
    }

protected:

    virtual void Update(double told,double tnew,bool missedmsg)
    {
        FLEXT_ASSERT(clock);

        if(scheduled < 0) return; // clock not set

        double time = Convert(tnew);
        double still = scheduled-time;

//        post("%lf: time=%lf",Current(),time);

        if(UNLIKELY(still < 0)) {
            if(LIKELY(missedmsg)) {
                t_atom at[2]; 
                ToOutAnything(GetOutAttr(),sym_missed,dblprec?2:1,SetDouble(at,-still));
                m_get();
            }

            // advance to the next future tick
            still = perintv+fmod(still,perintv);
        }

        // schedule new delay
        SchedDelay(still,false);
    }

	void CbTimer(void *) 
	{ 
        SchedDelay(perintv);
	}

    void SchedDelay(double intv,bool bang = true)
    {
        // obviously the metro can have been unregistered before being re-scheduled
        if(!clock) return; 

		reentered = false;
#if 1 // tentative fix
        double outoffs = tickoffs;
#else
		if(bang) m_get(tickoffs);  // bang out
        // Through the outlet in m_get we might re-enter this function.
        // "reentered" is false then and the following block is executed and a new delay scheduled.
        // After this block has been executed "reentered" is true, so we won't execute it again upon return from recursion

        // although we might not have been reentered, the metro might have been stopped in the meantime

		if(LIKELY(!reentered) && LIKELY(scheduled >= 0)) 
#endif
        {
            double factor = clock->Factor()*Factor();
		    double dur = intv/factor;
            // factor might be 0 if initialization is still in progress or big jumps in the reference time base happened
            if(dur < 0) {
                t_atom at[2]; 
                ToOutAnything(GetOutAttr(),sym_limit,dblprec?2:1,SetDouble(at,dur));
                dur = limit;
            }
            else {
                // reschedule
			    if(t3mode) {
				    double dticks = (dur+tickoffs)/ticks2s;
				    int iticks = (int)dticks;
				    tickoffs = (dticks-iticks)*ticks2s;
                    // recalculate dur
				    dur = iticks*ticks2s;
			    }
			    else
				    tickoffs = 0;

                // check
                if(dur < limit) {
                    t_atom at[2]; 
                    ToOutAnything(GetOutAttr(),sym_limit,dblprec?2:1,SetDouble(at,dur));
			        dur = limit;
                }
            }

            // schedule
            timer.Delay(dur);

            double cur = Current();
            scheduled = cur+intv;

//            fprintf(stderr,"%lf: Scheduled for +%lf = %lf\n",cur,dur,scheduled);
        }

#if 1 // tentative fix
		if(bang) m_get(outoffs);  // bang out
#endif
    }

	FLEXT_CALLBACK_F(m_metro)
	FLEXT_CALLBACK_FF(m_metro2)
	FLEXT_CALLBACK(m_stop)

	FLEXT_CALLBACK_T(CbTimer)

	FLEXT_ATTRVAR_F(limit)

    static void Setup(t_classid c)
    {
        sym_missed = MakeSymbol("missed");
        sym_limit = MakeSymbol("limit");

		FLEXT_CADDMETHOD(c,0,m_metro);
		FLEXT_CADDMETHOD_FF(c,0,sym_list,m_metro2);
		FLEXT_CADDMETHOD_(c,0,"stop",m_stop);

        FLEXT_CADDATTR_VAR1(c,sym_limit,limit);
    }

	Timer timer;
	double scheduled,perintv,tickoffs;
    bool reentered;
    float limit;

    static const t_symbol *sym_missed;
    static const t_symbol *sym_limit;
};

const t_symbol *Metro::sym_missed;
const t_symbol *Metro::sym_limit;

FLEXT_LIB_V("clk.metro, clk",Metro)

} // namespace
