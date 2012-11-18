/*
flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3669 $
$LastChangedDate: 2009-03-05 18:34:39 -0500 (Thu, 05 Mar 2009) $
$LastChangedBy: thomas $
*/

/*! \file flthr.cpp
    \brief Implementation of the flext thread functionality.
*/
 
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
flext::thrid_t flext::thrid;

//! Thread id of helper thread - will be initialized in flext::Setup
flext::thrid_t flext::thrhelpid;


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

	FLEXT_CLASSDEF(flext_base) *This() const { return th; }
	thrid_t Id() const { return thrid; }

	FLEXT_CLASSDEF(flext_base) *th;
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

static ThrFinder< PooledLifo<thr_entry,1,10> > thrpending;
static ThrFinder< TypedLifo<thr_entry> > thractive,thrstopped;

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

#if 0
class ThrIdCell
	: public LifoCell
	, public ThrId
{
public:
	ThrIdCell(const thrid_t &_id): ThrId(_id) {}
};

class RegQueue
    : public TypedLifo<ThrIdCell>
{
public:
    ~RegQueue() { ThrIdCell *pid; while((pid = Pop()) != NULL) delete pid; }
};

static RegQueue regqueue,unregqueue;
#endif

// this should _definitely_ be a hashmap....
// \TODO above all it should be populated immediately, otherwise it could easily happen 
// that the passing on to the set happens too late! We need that lockfree set!
static std::set<ThrId> regthreads;

//! Registry lock
static flext::ThrMutex *thrregmtx = NULL;

//! Helper thread conditional
static flext::ThrCond *thrhelpcond = NULL;

static void LaunchHelper(thr_entry *e)
{
    e->thrid = flext::GetThreadId();
    flext::RegisterThread(e->thrid);
    e->meth(e->params);
    flext::UnregisterThread(e->thrid);
}

bool initialized = false;

//! Start helper thread
bool flext::StartHelper()
{
	bool ok = false;
    initialized = false;

    thrregmtx = new ThrMutex;

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
        while(!initialized) Sleep(0.001);

        // we are ready for threading now!
    }
    
#if FLEXT_THREADS == FLEXT_THR_POSIX
	pthread_attr_destroy(&attr);
#endif    
	return ok;
}

//! Static helper thread function
void flext::ThrHelper(void *)
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

	thrhelpcond = new ThrCond;

    initialized = true;

	// helper loop
	for(;;) {
		thrhelpcond->Wait();

   		// start all inactive threads
        thr_entry *ti;
        while((ti = thrpending.Pop()) != NULL) {
		    bool ok;
    		
    #if FLEXT_THREADS == FLEXT_THR_POSIX
            thrid_t dummy;
		    ok = pthread_create (&dummy,&attr,(void *(*)(void *))LaunchHelper,ti) == 0;
    #elif FLEXT_THREADS == FLEXT_THR_MP
            thrid_t dummy;
		    ok = MPCreateTask((TaskProc)LaunchHelper,ti,0,0,0,0,0,&dummy) == noErr;
    #elif FLEXT_THREADS == FLEXT_THR_WIN32
		    ok = _beginthread((void (*)(void *))LaunchHelper,0,ti) >= 0;
    #else
    #error
    #endif
		    if(!ok) { 
			    error("flext - Could not launch thread!");
			    thrpending.Free(ti); ti = NULL;
		    }
		    else
			    // insert into queue of active threads
                thractive.Push(ti);
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


bool flext::LaunchThread(void (*meth)(thr_params *p),thr_params *p)
{
	FLEXT_ASSERT(thrhelpcond);

	// make an entry into thread list
    thr_entry *e = thrpending.New();
    e->Set(meth,p);
	thrpending.Push(e);
	// signal thread helper
	thrhelpcond->Signal();

	return true;
}

static bool waitforstopped(TypedLifo<thr_entry> &qufnd,float wait = 0)
{
    TypedLifo<thr_entry> qutmp;

    double until;
    if(wait) until = flext::GetOSTime()+wait;

    for(;;) {
        thr_entry *fnd = qufnd.Pop();
        if(!fnd) break; // no more entries -> done!

        thr_entry *ti;
        // search for entry
        while((ti = thrstopped.Pop()) != NULL && ti != fnd) qutmp.Push(ti);
        // put back entries
        while((ti = qutmp.Pop()) != NULL) thrstopped.Push(ti);

        if(ti) { 
            // still in thrstopped queue
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

bool flext::StopThread(void (*meth)(thr_params *p),thr_params *p,bool wait)
{
	FLEXT_ASSERT(thrhelpcond);

    TypedLifo<thr_entry> qutmp;
    thr_entry *ti;

    // first search pending queue
    // --------------------------

    {
        bool found = false;
        while((ti = thrpending.Pop()) != NULL)
            if(ti->meth == meth && ti->params == p) {
                // found -> thread hasn't started -> just delete
                thrpending.Free(ti);
                found = true;
            }
            else
                qutmp.Push(ti);

        // put back into pending queue (order doesn't matter)
        while((ti = qutmp.Pop()) != NULL) thrpending.Push(ti);

        if(found) return true;
    }

    // now search active queue
    // -----------------------

    TypedLifo<thr_entry> qufnd;

    while((ti = thractive.Pop()) != NULL)
        if(ti->meth == meth && ti->params == p) {
            thrstopped.Push(ti);
            thrhelpcond->Signal();
            qufnd.Push(ti);
        }
        else
            qutmp.Push(ti);

    // put back into pending queue (order doesn't matter)
    while((ti = qutmp.Pop()) != NULL) thractive.Push(ti);

    // wakeup helper thread
    thrhelpcond->Signal();

    // now wait for entries in qufnd to have vanished from thrstopped
    if(wait) 
        return waitforstopped(qufnd);
    else
        return !qufnd.Avail();
}

bool flext::ShouldExit() 
{
    return thrstopped.Find(GetThreadId()) != NULL;
}

bool flext::PushThread()
{
	// set priority of newly created thread one point below the system thread's
	RelPriority(-1);
	RegisterThread();
	return true;
}

void flext::PopThread()
{
    thrid_t id = GetThreadId();
	UnregisterThread(id);
    thr_entry *fnd = thrstopped.Find(id,true);
    if(!fnd) fnd = thractive.Find(id,true);

    if(fnd) 
        thrpending.Free(fnd);
#ifdef FLEXT_DEBUG
	else
		post("flext - INTERNAL ERROR: Thread not found!");
#endif
}

void flext::RegisterThread(thrid_t id)
{
#if 1
    FLEXT_ASSERT(thrregmtx);
    thrregmtx->Lock();
    regthreads.insert(id);
    thrregmtx->Unlock();
#else
	regqueue.Push(new ThrIdCell(id));
#endif
}

void flext::UnregisterThread(thrid_t id)
{
#if 1
    FLEXT_ASSERT(thrregmtx);
    thrregmtx->Lock();
    regthreads.erase(id);
    thrregmtx->Unlock();
#else
	unregqueue.Push(new ThrIdCell(id));
#endif
}

#if 0
void flext::ThreadRegistryWorker()
{
	ThrIdCell *pid;
	while((pid = regqueue.Pop()) != NULL) { regthreads.insert(pid->id); delete pid; }
	while((pid = unregqueue.Pop()) != NULL) { regthreads.erase(pid->id); delete pid; }
}
#endif

bool flext::IsThreadRegistered()
{
    FLEXT_ASSERT(thrregmtx);
    thrregmtx->Lock();
	bool fnd = regthreads.find(GetThreadId()) != regthreads.end();
    thrregmtx->Unlock();
    return fnd;
}

//! Terminate all object threads
bool flext_base::StopThreads()
{
	FLEXT_ASSERT(thrhelpcond);

    TypedLifo<thr_entry> qutmp;
    thr_entry *ti;

    // first search pending queue
    // --------------------------

    while((ti = thrpending.Pop()) != NULL)
        if(ti->This() == this)
            // found -> thread hasn't started -> just delete
            thrpending.Free(ti);
        else
            qutmp.Push(ti);

    // put back into pending queue (order doesn't matter)
    while((ti = qutmp.Pop()) != NULL) thrpending.Push(ti);

    // now search active queue
    // -----------------------

    TypedLifo<thr_entry> qufnd;

    while((ti = thractive.Pop()) != NULL)
        if(ti->This() == this) {
            thrstopped.Push(ti);
            thrhelpcond->Signal();
            qufnd.Push(ti);
        }
        else
            qutmp.Push(ti);

    // put back into pending queue (order doesn't matter)
    while((ti = qutmp.Pop()) != NULL) thractive.Push(ti);

    // wakeup helper thread
    thrhelpcond->Signal();

    // now wait for entries in qufnd to have vanished from thrstopped
    if(!waitforstopped(qufnd,MAXIMUMWAIT*0.001f)) {
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
#error Not implemented
#endif
            thrpending.Free(ti);
        }
        return false;
	}
    else
	    return true;
}

bool flext::RelPriority(int dp,thrid_t ref,thrid_t id)
{
#if FLEXT_THREADS == FLEXT_THR_POSIX
	sched_param parm;
	int policy;
	if(pthread_getschedparam(ref,&policy,&parm) < 0) {
#ifdef FLEXT_DEBUG
		post("flext - failed to get thread priority");
#endif
		return false;
	}
	else {
		parm.sched_priority += dp;

		// MSVC++ 6 produces wrong code with the following lines!!!
//		int schmin = sched_get_priority_min(policy);
//		int schmax = sched_get_priority_max(policy);

		if(parm.sched_priority < sched_get_priority_min(policy)) {
#ifdef FLEXT_DEBUG		
			post("flext - minimum thread priority reached");
#endif
			parm.sched_priority = sched_get_priority_min(policy);
		}
		else if(parm.sched_priority > sched_get_priority_max(policy)) {
#ifdef FLEXT_DEBUG		
			post("flext - maximum thread priority reached");
#endif
			parm.sched_priority = sched_get_priority_max(policy);
		}
		
		if(pthread_setschedparam(id,policy,&parm) < 0) {
#ifdef FLEXT_DEBUG		
			post("flext - failed to change thread priority");
#endif
			return false;
		}
	}
	return true;

#elif FLEXT_THREADS == FLEXT_THR_WIN32
    HANDLE href = OpenThread(THREAD_ALL_ACCESS,TRUE,ref);
    HANDLE hid = OpenThread(THREAD_ALL_ACCESS,TRUE,id);
    int pr = GetThreadPriority(href);

    if(pr == THREAD_PRIORITY_ERROR_RETURN) {
#ifdef FLEXT_DEBUG
		post("flext - failed to get thread priority");
#endif
		return false;
	}

    pr += dp;
	if(pr < THREAD_PRIORITY_IDLE) {
#ifdef FLEXT_DEBUG		
		post("flext - minimum thread priority reached");
#endif
		pr = THREAD_PRIORITY_IDLE;
	}
	else if(pr > THREAD_PRIORITY_TIME_CRITICAL) {
#ifdef FLEXT_DEBUG		
		post("flext - maximum thread priority reached");
#endif
		pr = THREAD_PRIORITY_TIME_CRITICAL;
	}
	
	if(SetThreadPriority(hid,pr) == 0) {
#ifdef FLEXT_DEBUG		
		post("flext - failed to change thread priority");
#endif
		return false;
	}
    return true;

#elif FLEXT_THREADS == FLEXT_THR_MP
    thr_entry *ti = thrpending.Find(id);
    if(!ti) ti = thractive.Find(id);
	if(ti) {
		// thread found in list
		int w = GetPriority(id);
		if(dp < 0) w /= 1<<(-dp);
		else w *= 1<<dp;
		if(w < 1) {
	#ifdef FLEXT_DEBUG		
			post("flext - minimum thread priority reached");
	#endif
			w = 1;
		}
		else if(w > 10000) {
	#ifdef FLEXT_DEBUG		
			post("flext - maximum thread priority reached");
	#endif
			w = 10000;
		}
		ti->weight = w;
		return MPSetTaskWeight(id,w) == noErr;
	}
	else return false;
#else
#error
#endif
}


int flext::GetPriority(thrid_t id)
{
#if FLEXT_THREADS == FLEXT_THR_POSIX
	sched_param parm;
	int policy;
	if(pthread_getschedparam(id,&policy,&parm) < 0) {
#ifdef FLEXT_DEBUG
		post("flext - failed to get parms");
#endif
		return -1;
	}
	return parm.sched_priority;

#elif FLEXT_THREADS == FLEXT_THR_WIN32
    HANDLE hid = OpenThread(THREAD_ALL_ACCESS,TRUE,id);
    int pr = GetThreadPriority(hid);

    if(pr == THREAD_PRIORITY_ERROR_RETURN) {
#ifdef FLEXT_DEBUG
		post("flext - failed to get thread priority");
#endif
		return -1;
	}
    return pr;

#elif FLEXT_THREADS == FLEXT_THR_MP
    thr_entry *ti = thrpending.Find(id);
    if(!ti) ti = thractive.Find(id);
    return ti?ti->weight:-1;
#else
#error
#endif
}


bool flext::SetPriority(int p,thrid_t id)
{
#if FLEXT_THREADS == FLEXT_THR_POSIX
	sched_param parm;
	int policy;
	if(pthread_getschedparam(id,&policy,&parm) < 0) {
#ifdef FLEXT_DEBUG
		post("flext - failed to get parms");
#endif
		return false;
	}
	else {
		parm.sched_priority = p;
		if(pthread_setschedparam(id,policy,&parm) < 0) {
#ifdef FLEXT_DEBUG		
			post("flext - failed to change priority");
#endif
			return false;
		}
	}
	return true;

#elif FLEXT_THREADS == FLEXT_THR_WIN32
    HANDLE hid = OpenThread(THREAD_ALL_ACCESS,TRUE,id);
	if(SetThreadPriority(hid,p) == 0) {
#ifdef FLEXT_DEBUG		
		post("flext - failed to change thread priority");
#endif
		return false;
	}
    return true;

#elif FLEXT_THREADS == FLEXT_THR_MP
    thr_entry *ti = thrpending.Find(id);
    if(!ti) ti = thractive.Find(id);
    return ti && MPSetTaskWeight(id,ti->weight = p) == noErr;
#else
#error
#endif
}


flext_base::thr_params::thr_params(int n): cl(NULL),var(new _data[n]) {}
flext_base::thr_params::~thr_params() { if(var) delete[] var; }

void flext_base::thr_params::set_any(const t_symbol *s,int argc,const t_atom *argv) { var[0]._any = new AtomAnything(s,argc,argv); }
void flext_base::thr_params::set_list(int argc,const t_atom *argv) { var[0]._list = new AtomList(argc,argv); }


#if FLEXT_THREADS == FLEXT_THR_POSIX
bool flext::ThrCond::Wait() { 
	Lock();
	bool ret = pthread_cond_wait(&cond,&mutex) == 0; 
	Unlock();
	return ret;
}

bool flext::ThrCond::TimedWait(double ftm) 
{ 
	timespec tm; 
#if FLEXT_OS == FLEXT_OS_WIN && FLEXT_OSAPI == FLEXT_OSAPI_WIN_NATIVE
#ifdef _MSC_VER
	_timeb tmb;
	_ftime(&tmb);
#else
	timeb tmb;
	ftime(&tmb);
#endif
	tm.tv_nsec = tmb.millitm*1000000;
	tm.tv_sec = (long)tmb.time; 
#else // POSIX
#if 0 // find out when the following is defined
	clock_gettime(CLOCK_REALTIME,tm);
#else
	struct timeval tp;
	gettimeofday(&tp, NULL);
	tm.tv_nsec = tp.tv_usec*1000;
	tm.tv_sec = tp.tv_sec;
#endif
#endif

	tm.tv_nsec += (long)((ftm-(long)ftm)*1.e9);
	long nns = tm.tv_nsec%1000000000;
	tm.tv_sec += (long)ftm+(tm.tv_nsec-nns)/1000000000; 
	tm.tv_nsec = nns;

	Lock();
	bool ret = pthread_cond_timedwait(&cond,&mutex,&tm) == 0; 
	Unlock();
	return ret;
}
#endif

#include "flpopns.h"

#endif // FLEXT_THREADS

