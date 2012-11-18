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

#define LIMIT 1.e-4f

namespace clk {

class Delay
    : public ClientExt
{
    FLEXT_HEADER_S(Delay,ClientExt,Setup)

public:
    Delay(int argc,const t_atom *argv)
        : ClientExt(argc,argv)
        , scheduled(-1)
        , limit(LIMIT)
    {
		FLEXT_ADDTIMER(timer,CbTimer);
    }

	void m_delay(double intv,double offs = 0) 
	{ 
        if(LIKELY(clock)) {
            double factor = clock->Factor()*Factor();
            double dur = intv/factor;
            // factor might be 0 if initialization is still in progress or big jumps in the reference time base happened
            if(dur < 0) {
                t_atom at[2]; 
                ToOutAnything(GetOutAttr(),sym_limit,dblprec?2:1,SetDouble(at,dur));
                dur = 0;
            }
            else {
		        if(t3mode) {
			        double dticks = (dur+offs)/ticks2s;
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

    //            post("%lf: Scheduled for +%lf = %lf",cur,dur,scheduled);
            }

            // schedule
            if(dur)
                timer.Delay(dur);

            double cur = Current();
            scheduled = cur+intv;
        }
	}

	void m_delay2(float intv1,float intv2) { m_delay((double)intv1+(double)intv2); }
	
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
            m_stop();

            if(LIKELY(missedmsg)) {
                t_atom at[2]; 
                ToOutAnything(GetOutAttr(),sym_missed,dblprec?2:1,SetDouble(at,-still));
                // we missed the time already... output immediately!
                m_get();
            }
        }
        else {
//            post("Reschedule in %lf!",still);

            // schedule new delay
            m_delay(still);
        }
    }

	void CbTimer(void *) 
	{ 
		m_get(tickoffs);
        scheduled = -1;
	}

	FLEXT_CALLBACK_F(m_delay)
	FLEXT_CALLBACK_FF(m_delay2)
	FLEXT_CALLBACK(m_stop)

	FLEXT_CALLBACK_T(CbTimer)

	FLEXT_ATTRVAR_F(limit)

    static void Setup(t_classid c)
    {
        sym_missed = MakeSymbol("missed");
        sym_limit = MakeSymbol("limit");

		FLEXT_CADDMETHOD(c,0,m_delay);
		FLEXT_CADDMETHOD_FF(c,0,sym_list,m_delay2);
		FLEXT_CADDMETHOD_(c,0,"stop",m_stop);

		FLEXT_CADDATTR_VAR1(c,sym_limit,limit);
    }

	Timer timer;
	double scheduled,tickoffs;
    float limit;

    static const t_symbol *sym_missed;
    static const t_symbol *sym_limit;
};

const t_symbol *Delay::sym_missed;
const t_symbol *Delay::sym_limit;

FLEXT_LIB_V("clk.delay, clk",Delay)

} // namespace
