/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flthr.cpp
    \brief Implementation of the flext thread functionality.
*/
 
#ifndef __FLEXT_THR_CPP
#define __FLEXT_THR_CPP

#include "flext.h"

#ifdef FLEXT_THREADS

// maximum wait time for threads to finish (in ms)
#define MAXIMUMWAIT 100


#include "flinternal.h"
#include "flcontainers.h"
#include <set>
#include <ctime>

#if FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH || FLEXT_OSAPI == FLEXT_OSAPI_UNIX_POSIX || FLEXT_OSAPI == FLEXT_OSAPI_WIN_POSIX
#include <sys/time.h>
#include <unistd.h>
#elif FLEXT_OS == FLEXT_OS_WIN
#include <sys/timeb.h>
#endif

#if FLEXT_THREADS == FLEXT_THR_WIN32 && WINVER < 0x0500
#error WIN32 threads need Windows SDK version >= 0x500
#endif

#include <cerrno>

#include "flpushns.h"

//! Thread id of system thread - will be initialized in flext::Setup
FLEXT_TEMPIMPL(FLEXT_TEMPINST(FLEXT_CLASSDEF(flext))::thrid_t FLEXT_CLASSDEF(flext))::thrid;

//! Thread id of helper thread - will be initialized in flext::Setup
FLEXT_TEMPIMPL(FLEXT_TEMPINST(FLEXT_CLASSDEF(flext))::thrid_t FLEXT_CLASSDEF(flext))::thrhelpid;


//! \brief This represents an entry to the list of active method threads
class thr_entry
    : public flext
    , public LifoCell
{
public:
    void Set(void (*m)(thr_params *),thr_params *p,thrid_t id = GetThreadId())
    {
        th = p?p->cl:NULL;
        meth = m,params = p,thrid = id;
        shouldexit = false;
#if FLEXT_THREADS == FLEXT_THR_MP
	    weight = 100; // MP default weight
#endif
    }

	//! \brief Check if this class represents the current thread
	bool Is(thrid_t id = GetThreadId()) const { return IsThread(thrid,id); }

	FLEXT_TEMPINST(FLEXT_CLASSDEF(flext_base)) *This() const { return th; }
	thrid_t Id() const { return thrid; }

	FLEXT_TEMPINST(FLEXT_CLASSDEF(flext_base)) *th;
	void (*meth)(thr_params *);
	thr_params *params;
	thrid_t thrid;
	bool shouldexit;
#if FLEXT_THREADS == FLEXT_THR_MP
	int weight;
#endif
};

template<class T>
class ThrFinder:
    public T
{
public:
    ~ThrFinder() { thr_entry *e; while((e = Pop()) != NULL) delete e; }

    void Push(thr_entry *e) { T::Push(e); }
    thr_entry *Pop() { return T::Pop(); }

    thr_entry *Find(flext::thrid_t id,bool pop = false) 
    {
        TypedLifo<thr_entry> qutmp;
        thr_entry *fnd;
        while((fnd = Pop()) && !fnd->Is(id)) qutmp.Push(fnd);
        // put back entries
        for(thr_entry *ti; (ti = qutmp.Pop()) != NULL; ) Push(ti);
        if(fnd && !pop) Push(fnd);
        return fnd;
    }
};

FLEXT_TEMPLATE
struct ThrRegistry
{
    typedef ThrFinder< PooledLifo<thr_entry,1,10> > RegPooledLifo;
    typedef ThrFinder< TypedLifo<thr_entry> > RegFinder;
    static RegPooledLifo pending;
    static RegFinder active,stopped;
};

FLEXT_TEMPIMPL(FLEXT_TEMPINST(ThrRegistry)::RegPooledLifo ThrRegistry)::pending;
FLEXT_TEMPIMPL(FLEXT_TEMPINST(ThrRegistry)::RegFinder ThrRegistry)::active;
FLEXT_TEMPIMPL(FLEXT_TEMPINST(ThrRegistry)::RegFinder ThrRegistry)::stopped;


class ThrId
	: public flext
{
public:
	ThrId(const thrid_t &_id): id(_id) {}
	thrid_t id;

	bool operator <(const ThrId &tid) const 
	{ 
		if(sizeof(id) == sizeof(unsigned))
			return (unsigned *)&id < (unsigned *)&tid;
		else
			return memcmp(&id,&tid,sizeof(id)) < 0;
	}
};

FLEXT_TEMPLATE
struct ThrVars {
    // this should _definitely_ be a hashmap....
    // \TODO above all it should be populated immediately, otherwise it could easily happen 
    // that the passing on to the set happens too late! We need that lockfree set!
    static std::set<ThrId> regthreads;

    //! Registry lock
    static flext::ThrMutex *thrregmtx;

    //! Helper thread conditional
    static flext::ThrCond *thrhelpcond;

    static bool initialized;
};

FLEXT_TEMPIMPL(std::set<ThrId> ThrVars)::regthreads;
FLEXT_TEMPIMPL(flext::ThrMutex *ThrVars)::thrregmtx = NULL;
FLEXT_TEMPIMPL(flext::ThrCond *ThrVars)::thrhelpcond = NULL;
FLEXT_TEMPIMPL(bool ThrVars)::initialized = false;

FLEXT_TEMPLATE void LaunchHelper(thr_entry *e)
{
    e->thrid = flext::GetThreadId();
    flext::RegisterThread(e->thrid);
    e->meth(e->params);
    flext::UnregisterThread(e->thrid);
}

//! Start helper thread
FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::StartHelper()
{
	bool ok = false;
    FLEXT_TEMPINST(ThrVars)::initialized = false;

    FLEXT_TEMPINST(ThrVars)::thrregmtx = new ThrMutex;

#if FLEXT_THREADS == FLEXT_THR_POSIX
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    pthread_t tmp;
	ok = pthread_create (&tmp,&attr,(void *(*)(void *))ThrHelper,NULL) == 0;
#elif FLEXT_THREADS == FLEXT_THR_MP
	if(!MPLibraryIsLoaded())
		error("Thread library is not loaded");
	else {
        MPTaskID tmp;
		OSStatus ret = MPCreateTask((TaskProc)ThrHelper,NULL,0,0,0,0,0,&tmp);
		ok = ret == noErr;
	}
#elif FLEXT_THREADS == FLEXT_THR_WIN32
    ok = _beginthread(ThrHelper,0,NULL) >= 0;
#else
#error
#endif
	if(!ok)
		error("flext - Could not launch helper thread!"); 
    else {
        // now we have to wait for thread helper to initialize
        while(!FLEXT_TEMPINST(ThrVars)::initialized) Sleep(0.001);

        // we are ready for threading now!
    }
    
#if FLEXT_THREADS == FLEXT_THR_POSIX
	pthread_attr_destroy(&attr);
#endif    
	return ok;
}

//! Static helper thread function
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::ThrHelper(void *)
{
    thrhelpid = GetThreadId();

#if FLEXT_THREADS == FLEXT_THR_POSIX
	// set prototype thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
#endif

	// set thread priority one point below normal
	// so thread construction won't disturb real-time audio
	RelPriority(-1);

	FLEXT_TEMPINST(ThrVars)::thrhelpcond = new ThrCond;

    FLEXT_TEMPINST(ThrVars)::initialized = true;

	// helper loop
	for(;;) {
		FLEXT_TEMPINST(ThrVars)::thrhelpcond->Wait();

   		// start all inactive threads
        thr_entry *ti;
        while((ti = FLEXT_TEMPINST(ThrRegistry)::pending.Pop()) != NULL) {
		    bool ok;
    		
    #if FLEXT_THREADS == FLEXT_THR_POSIX
            thrid_t dummy;
		    ok = pthread_create (&dummy,&attr,(void *(*)(void *))FLEXT_TEMPINST(LaunchHelper),ti) == 0;
    #elif FLEXT_THREADS == FLEXT_THR_MP
            thrid_t dummy;
		    ok = MPCreateTask((TaskProc)FLEXT_TEMPINST(LaunchHelper),ti,0,0,0,0,0,&dummy) == noErr;
    #elif FLEXT_THREADS == FLEXT_THR_WIN32
		    ok = _beginthread((void (*)(void *))FLEXT_TEMPINST(LaunchHelper),0,ti) >= 0;
    #else
    #error
    #endif
		    if(!ok) { 
			    error("flext - Could not launch thread!");
			    FLEXT_TEMPINST(ThrRegistry)::pending.Free(ti); ti = NULL;
		    }
		    else
			    // insert into queue of active threads
                FLEXT_TEMPINST(ThrRegistry)::active.Push(ti);
        }
	}

    FLEXT_ASSERT(false);
/*
    // Never reached!

	delete thrhelpcond;
	thrhelpcond = NULL;
	
#if FLEXT_THREADS == FLEXT_THR_POSIX
	pthread_attr_destroy(&attr);
#endif
*/
}


FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::LaunchThread(void (*meth)(thr_params *p),thr_params *p)
{
	FLEXT_ASSERT(FLEXT_TEMPINST(ThrVars)::thrhelpcond);

	// make an entry into thread list
    thr_entry *e = FLEXT_TEMPINST(ThrRegistry)::pending.New();
    e->Set(meth,p);
	FLEXT_TEMPINST(ThrRegistry)::pending.Push(e);
	// signal thread helper
	FLEXT_TEMPINST(ThrVars)::thrhelpcond->Signal();

	return true;
}

FLEXT_TEMPLATE bool waitforstopped(TypedLifo<thr_entry> &qufnd,float wait = 0)
{
    TypedLifo<thr_entry> qutmp;

    double until;
    if(wait) until = flext::GetOSTime()+wait;

    for(;;) {
        thr_entry *fnd = qufnd.Pop();
        if(!fnd) break; // no more entries -> done!

        thr_entry *ti;
        // search for entry
        while((ti = FLEXT_TEMPINST(ThrRegistry)::stopped.Pop()) != NULL && ti != fnd) qutmp.Push(ti);
        // put back entries
        while((ti = qutmp.Pop()) != NULL) FLEXT_TEMPINST(ThrRegistry)::stopped.Push(ti);

        if(ti) { 
            // still in ThrRegistry::stopped queue
            qufnd.Push(fnd);
            // yield to other threads
            flext::ThrYield();
            
            if(wait && flext::GetOSTime() > until) 
                // not successful -> remaining thread are still in qufnd queue
                return false;
        }
    }
    return true;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::StopThread(void (*meth)(thr_params *p),thr_params *p,bool wait)
{
	FLEXT_ASSERT(FLEXT_TEMPINST(ThrVars)::thrhelpcond);

    TypedLifo<thr_entry> qutmp;
    thr_entry *ti;

    // first search pending queue
    // --------------------------

    {
        bool found = false;
        while((ti = FLEXT_TEMPINST(ThrRegistry)::pending.Pop()) != NULL)
            if(ti->meth == meth && ti->params == p) {
                // found -> thread hasn't started -> just delete
                FLEXT_TEMPINST(ThrRegistry)::pending.Free(ti);
                found = true;
            }
            else
                qutmp.Push(ti);

        // put back into pending queue (order doesn't matter)
        while((ti = qutmp.Pop()) != NULL) FLEXT_TEMPINST(ThrRegistry)::pending.Push(ti);

        if(found) return true;
    }

    // now search active queue
    // -----------------------

    TypedLifo<thr_entry> qufnd;

    while((ti = FLEXT_TEMPINST(ThrRegistry)::active.Pop()) != NULL)
        if(ti->meth == meth && ti->params == p) {
            FLEXT_TEMPINST(ThrRegistry)::stopped.Push(ti);
            FLEXT_TEMPINST(ThrVars)::thrhelpcond->Signal();
            qufnd.Push(ti);
        }
        else
            qutmp.Push(ti);

    // put back into pending queue (order doesn't matter)
    while((ti = qutmp.Pop()) != NULL) FLEXT_TEMPINST(ThrRegistry)::active.Push(ti);

    // wakeup helper thread
    FLEXT_TEMPINST(ThrVars)::thrhelpcond->Signal();

    // now wait for entries in qufnd to have vanished from ThrRegistry::stopped
    if(wait) 
        return FLEXT_TEMPINST(waitforstopped)(qufnd);
    else
        return !qufnd.Avail();
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::ShouldExit()
{
    return FLEXT_TEMPINST(ThrRegistry)::stopped.Find(GetThreadId()) != NULL;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::PushThread()
{
	// set priority of newly created thread one point below the system thread's
	RelPriority(-1);
	RegisterThread();
	return true;
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::PopThread()
{
    thrid_t id = GetThreadId();
	UnregisterThread(id);
    thr_entry *fnd = FLEXT_TEMPINST(ThrRegistry)::stopped.Find(id,true);
    if(!fnd) fnd = FLEXT_TEMPINST(ThrRegistry)::active.Find(id,true);

    if(fnd) 
        FLEXT_TEMPINST(ThrRegistry)::pending.Free(fnd);
#ifdef FLEXT_DEBUG
	else
		post("flext - INTERNAL ERROR: Thread not found!");
#endif
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::RegisterThread(thrid_t id)
{
#if 1
    FLEXT_ASSERT(FLEXT_TEMPINST(ThrVars)::thrregmtx);
    FLEXT_TEMPINST(ThrVars)::thrregmtx->Lock();
    FLEXT_TEMPINST(ThrVars)::regthreads.insert(id);
    FLEXT_TEMPINST(ThrVars)::thrregmtx->Unlock();
#else
	regqueue.Push(new ThrIdCell(id));
#endif
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::UnregisterThread(thrid_t id)
{
#if 1
    FLEXT_ASSERT(FLEXT_TEMPINST(ThrVars)::thrregmtx);
    FLEXT_TEMPINST(ThrVars)::thrregmtx->Lock();
    FLEXT_TEMPINST(ThrVars)::regthreads.erase(id);
    FLEXT_TEMPINST(ThrVars)::thrregmtx->Unlock();
#else
	unregqueue.Push(new ThrIdCell(id));
#endif
}

#if 0
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::ThreadRegistryWorker()
{
	ThrIdCell *pid;
	while((pid = regqueue.Pop()) != NULL) { regthreads.insert(pid->id); delete pid; }
	while((pid = unregqueue.Pop()) != NULL) { regthreads.erase(pid->id); delete pid; }
}
#endif

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::IsThreadRegistered()
{
    FLEXT_ASSERT(FLEXT_TEMPINST(ThrVars)::thrregmtx);
    FLEXT_TEMPINST(ThrVars)::thrregmtx->Lock();
	bool fnd = FLEXT_TEMPINST(ThrVars)::regthreads.find(GetThreadId()) != FLEXT_TEMPINST(ThrVars)::regthreads.end();
    FLEXT_TEMPINST(ThrVars)::thrregmtx->Unlock();
    return fnd;
}

//! Terminate all object threads
FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::StopThreads()
{
	FLEXT_ASSERT(FLEXT_TEMPINST(ThrVars)::thrhelpcond);

    TypedLifo<thr_entry> qutmp;
    thr_entry *ti;

    // first search pending queue
    // --------------------------

    while((ti = FLEXT_TEMPINST(ThrRegistry)::pending.Pop()) != NULL)
        if(ti->This() == this)
            // found -> thread hasn't started -> just delete
            FLEXT_TEMPINST(ThrRegistry)::pending.Free(ti);
        else
            qutmp.Push(ti);

    // put back into pending queue (order doesn't matter)
    while((ti = qutmp.Pop()) != NULL) FLEXT_TEMPINST(ThrRegistry)::pending.Push(ti);

    // now search active queue
    // -----------------------

    TypedLifo<thr_entry> qufnd;

    while((ti = FLEXT_TEMPINST(ThrRegistry)::active.Pop()) != NULL)
        if(ti->This() == this) {
            FLEXT_TEMPINST(ThrRegistry)::stopped.Push(ti);
            FLEXT_TEMPINST(ThrVars)::thrhelpcond->Signal();
            qufnd.Push(ti);
        }
        else
            qutmp.Push(ti);

    // put back into pending queue (order doesn't matter)
    while((ti = qutmp.Pop()) != NULL) FLEXT_TEMPINST(ThrRegistry)::active.Push(ti);

    // wakeup helper thread
    FLEXT_TEMPINST(ThrVars)::thrhelpcond->Signal();

    // now wait for entries in qufnd to have vanished from ThrRegistry::stopped
    if(!FLEXT_TEMPINST(waitforstopped)(qufnd,MAXIMUMWAIT*0.001f)) {
#ifdef FLEXT_DEBUG
		post("flext - doing hard thread termination");
#endif

		// timeout -> hard termination
        while((ti = qufnd.Pop()) != NULL) {
#if FLEXT_THREADS == FLEXT_THR_POSIX
			if(pthread_cancel(ti->thrid)) 
                post("%s - Thread could not be terminated!",thisName());
#elif FLEXT_THREADS == FLEXT_THR_MP
			MPTerminateTask(ti->thrid,0);
			// here, we should use a task queue to check whether the task has really terminated!!
#elif FLEXT_THREADS == FLEXT_THR_WIN32
            // can't use the c library function _endthread.. memory leaks will occur
            HANDLE hnd = OpenThread(THREAD_ALL_ACCESS,TRUE,ti->thrid);
            TerminateThread(hnd,0);
#else
# error Not implemented
#endif
            FLEXT_TEMPINST(ThrRegistry)::pending.Free(ti);
        }
        return false;
	}
    else
	    return true;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::RelPriority(int dp,thrid_t ref,thrid_t id)
{
#if FLEXT_THREADS == FLEXT_THR_POSIX
	sched_param parm;
	int policy;
	if(pthread_getschedparam(ref,&policy,&parm) < 0) {
# ifdef FLEXT_DEBUG
		post("flext - failed to get thread priority");
# endif
		return false;
	}
	else {
		parm.sched_priority += dp;

		// MSVC++ 6 produces wrong code with the following lines!!!
//		int schmin = sched_get_priority_min(policy);
//		int schmax = sched_get_priority_max(policy);

		if(parm.sched_priority < sched_get_priority_min(policy)) {
# ifdef FLEXT_DEBUG
			post("flext - minimum thread priority reached");
# endif
			parm.sched_priority = sched_get_priority_min(policy);
		}
		else if(parm.sched_priority > sched_get_priority_max(policy)) {
# ifdef FLEXT_DEBUG
			post("flext - maximum thread priority reached");
# endif
			parm.sched_priority = sched_get_priority_max(policy);
		}
		
		if(pthread_setschedparam(id,policy,&parm) < 0) {
# ifdef FLEXT_DEBUG
			post("flext - failed to change thread priority");
# endif
			return false;
		}
	}
	return true;

#elif FLEXT_THREADS == FLEXT_THR_WIN32
    HANDLE href = OpenThread(THREAD_ALL_ACCESS,TRUE,ref);
    HANDLE hid = OpenThread(THREAD_ALL_ACCESS,TRUE,id);
    int pr = GetThreadPriority(href);

    if(pr == THREAD_PRIORITY_ERROR_RETURN) {
# ifdef FLEXT_DEBUG
		post("flext - failed to get thread priority");
# endif
		return false;
	}

    pr += dp;
	if(pr < THREAD_PRIORITY_IDLE) {
# ifdef FLEXT_DEBUG
		post("flext - minimum thread priority reached");
# endif
		pr = THREAD_PRIORITY_IDLE;
	}
	else if(pr > THREAD_PRIORITY_TIME_CRITICAL) {
# ifdef FLEXT_DEBUG
		post("flext - maximum thread priority reached");
# endif
		pr = THREAD_PRIORITY_TIME_CRITICAL;
	}
	
	if(SetThreadPriority(hid,pr) == 0) {
# ifdef FLEXT_DEBUG
		post("flext - failed to change thread priority");
# endif
		return false;
	}
    return true;

#elif FLEXT_THREADS == FLEXT_THR_MP
    thr_entry *ti = FLEXT_TEMPINST(ThrRegistry)::pending.Find(id);
    if(!ti) ti = FLEXT_TEMPINST(ThrRegistry)::active.Find(id);
	if(ti) {
		// thread found in list
		int w = GetPriority(id);
		if(dp < 0) w /= 1<<(-dp);
		else w *= 1<<dp;
		if(w < 1) {
# ifdef FLEXT_DEBUG
			post("flext - minimum thread priority reached");
# endif
			w = 1;
		}
		else if(w > 10000) {
# ifdef FLEXT_DEBUG
			post("flext - maximum thread priority reached");
# endif
			w = 10000;
		}
		ti->weight = w;
		return MPSetTaskWeight(id,w) == noErr;
	}
	else return false;
#else
# error
#endif
}


FLEXT_TEMPIMPL(int FLEXT_CLASSDEF(flext))::GetPriority(thrid_t id)
{
#if FLEXT_THREADS == FLEXT_THR_POSIX
	sched_param parm;
	int policy;
	if(pthread_getschedparam(id,&policy,&parm) < 0) {
# ifdef FLEXT_DEBUG
		post("flext - failed to get parms");
# endif
		return -1;
	}
	return parm.sched_priority;

#elif FLEXT_THREADS == FLEXT_THR_WIN32
    HANDLE hid = OpenThread(THREAD_ALL_ACCESS,TRUE,id);
    int pr = GetThreadPriority(hid);

    if(pr == THREAD_PRIORITY_ERROR_RETURN) {
# ifdef FLEXT_DEBUG
		post("flext - failed to get thread priority");
# endif
		return -1;
	}
    return pr;

#elif FLEXT_THREADS == FLEXT_THR_MP
    thr_entry *ti = FLEXT_TEMPINST(ThrRegistry)::pending.Find(id);
    if(!ti) ti = FLEXT_TEMPINST(ThrRegistry)::active.Find(id);
    return ti?ti->weight:-1;
#else
# error
#endif
}


FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::SetPriority(int p,thrid_t id)
{
#if FLEXT_THREADS == FLEXT_THR_POSIX
	sched_param parm;
	int policy;
	if(pthread_getschedparam(id,&policy,&parm) < 0) {
# ifdef FLEXT_DEBUG
		post("flext - failed to get parms");
# endif
		return false;
	}
	else {
		parm.sched_priority = p;
		if(pthread_setschedparam(id,policy,&parm) < 0) {
# ifdef FLEXT_DEBUG
			post("flext - failed to change priority");
# endif
			return false;
		}
	}
	return true;

#elif FLEXT_THREADS == FLEXT_THR_WIN32
    HANDLE hid = OpenThread(THREAD_ALL_ACCESS,TRUE,id);
	if(SetThreadPriority(hid,p) == 0) {
# ifdef FLEXT_DEBUG
		post("flext - failed to change thread priority");
# endif
		return false;
	}
    return true;

#elif FLEXT_THREADS == FLEXT_THR_MP
    thr_entry *ti = FLEXT_TEMPINST(ThrRegistry)::pending.Find(id);
    if(!ti) ti = FLEXT_TEMPINST(ThrRegistry)::active.Find(id);
    return ti && MPSetTaskWeight(id,ti->weight = p) == noErr;
#else
# error
#endif
}


#if FLEXT_THREADS == FLEXT_THR_POSIX
FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::ThrCond::Wait() {
	this->Lock(); // use this-> to avoid wrong function invocation (global Unlock)
    bool ret = pthread_cond_wait(&cond,&this->mutex) == 0;
	this->Unlock(); // use this-> to avoid wrong function invocation (global Unlock)
	return ret;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext))::ThrCond::TimedWait(double ftm)
{ 
	timespec tm; 
#if FLEXT_OS == FLEXT_OS_WIN && FLEXT_OSAPI == FLEXT_OSAPI_WIN_NATIVE
# ifdef _MSC_VER
	_timeb tmb;
	_ftime(&tmb);
# else
	timeb tmb;
	ftime(&tmb);
# endif
	tm.tv_nsec = tmb.millitm*1000000;
	tm.tv_sec = (long)tmb.time; 
#else // POSIX
# if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	clock_gettime(CLOCK_REALTIME,&tm);
# else
	struct timeval tp;
	gettimeofday(&tp, NULL);
	tm.tv_nsec = tp.tv_usec*1000;
	tm.tv_sec = tp.tv_sec;
# endif
#endif

	tm.tv_nsec += (long)((ftm-(long)ftm)*1.e9);
	long nns = tm.tv_nsec%1000000000;
	tm.tv_sec += (long)ftm+(tm.tv_nsec-nns)/1000000000; 
	tm.tv_nsec = nns;

	this->Lock(); // use this-> to avoid wrong function invocation (global Unlock)
    bool ret = pthread_cond_timedwait(&cond,&this->mutex,&tm) == 0;
	this->Unlock(); // use this-> to avoid wrong function invocation (global Unlock)
	return ret;
}
#endif

#include "flpopns.h"

#endif // FLEXT_THREADS

#endif // __FLEXT_THR_CPP


