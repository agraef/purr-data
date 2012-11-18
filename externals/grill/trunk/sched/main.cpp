/* 
sched - message scheduler

Copyright (c)2005-2008 Thomas Grill
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#include <list>


#define SCHED_VERSION "0.1.0"

class sched:
	public flext_base
{
	FLEXT_HEADER_S(sched,flext_base,setup)

public:
	sched(float d)
        : delay(d)
    {
	    AddInAnything("Control messages/Input messages");
	    AddInAnything("delay time (ms)");
	    AddOutAnything("Scheduled messages");

	    FLEXT_ADDTIMER(timer,m_timer);
    }

protected:
    Timer timer;

    class Data:
        public AtomAnything
    {
    public:        
        Data() {}

        Data(const Data &d): time(d.time),AtomAnything(d) {}

        Data(double t,const t_symbol *sym,int argc,const t_atom *argv)
            : time(t)
            , AtomAnything(sym,argc,argv) 
        {}

        bool operator ==(const Data &d) const { return time == d.time; }
        bool operator <(const Data &d) const { return time < d.time; }

        double time;
    };

    typedef std::list<Data> Queue;
    Queue queue;

    void m_delay(float d) { delay = d; }

    void m_pending(int &n) { n = (int)queue.size(); }

    void m_timeout(float &tm) 
    { 
        Queue::iterator it = queue.begin();
        tm = it == queue.end()?0:(float)(it->time-flext::GetTime());
    }

    void m_timer(void *)
    {
        Queue::iterator it = queue.begin();
        if(it != queue.end()) {
            ToOutAnything(0,*it);
            queue.erase(it);
            reschedule();
        }
        else
            post("%s - Internal error, line %i",thisName(),__LINE__);
    }

    void m_reset()
    {
        timer.Reset();
        queue.clear();
    }

    void m_sched(int argc,const t_atom *argv)
    {
        double t = GetTime()+delay*0.001;
        queue.push_back(Data(t,sym_list,argc,argv));
        reschedule();
    }

private:
    float delay;

    void reschedule()
    {
        queue.sort();
        Queue::iterator it = queue.begin();
        if(it != queue.end()) timer.At(it->time);
    }

    static void setup(t_classid c)
    {
	    FLEXT_CADDMETHOD(c,0,m_sched);
	    FLEXT_CADDMETHOD(c,1,m_delay);
        FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
        FLEXT_CADDATTR_GET(c,"pending",m_pending);
        FLEXT_CADDATTR_GET(c,"timeout",m_timeout);
        FLEXT_CADDATTR_VAR1(c,"delay",delay);
    }

	FLEXT_CALLBACK_T(m_timer)  
	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK_F(m_delay)
	FLEXT_CALLBACK_V(m_sched)
	FLEXT_CALLGET_I(m_pending)
	FLEXT_CALLGET_F(m_timeout)
	FLEXT_ATTRVAR_F(delay)
};

FLEXT_NEW_1("sched",sched,float0)
