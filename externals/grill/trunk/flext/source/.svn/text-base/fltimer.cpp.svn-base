/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file fltimer.cpp
    \brief flext timer functions and classes   
*/

#include "flext.h"

#if FLEXT_OS == FLEXT_OS_WIN
#include <windows.h>
#elif FLEXT_OS == FLEXT_OS_LINUX || FLEXT_OS == FLEXT_OS_IRIX || FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH
#include <unistd.h>
#include <sys/time.h>
#elif FLEXT_OS == FLEXT_OS_MAC
#include <Timer.h>
#include <Threads.h>
#endif

#include "flpushns.h"

#if FLEXT_OS == FLEXT_OS_WIN
static double perffrq = 0;
#endif

static double getstarttime();
static double starttime = getstarttime();

static double getstarttime()
{
#if FLEXT_OS == FLEXT_OS_WIN
    LARGE_INTEGER frq;
    if(QueryPerformanceFrequency(&frq)) perffrq = (double)frq.QuadPart;
#endif

    starttime = 0;
    return flext::GetOSTime();
}

double flext::GetOSTime()
{
    double tm;

#if FLEXT_OS == FLEXT_OS_WIN
    LARGE_INTEGER cnt;
    if(perffrq && QueryPerformanceCounter(&cnt))
        tm = cnt.QuadPart/perffrq;
    else {
        SYSTEMTIME systm;
        FILETIME fltm;
        GetSystemTime(&systm);
        SystemTimeToFileTime(&systm,&fltm);
        tm = ((LARGE_INTEGER *)&fltm)->QuadPart*1.e-7;
    }
#elif FLEXT_OS == FLEXT_OS_LINUX || FLEXT_OS == FLEXT_OS_IRIX || FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH // POSIX
    timeval tmv;
    gettimeofday(&tmv,NULL);
    tm = tmv.tv_sec+tmv.tv_usec*1.e-6;
#elif FLEXT_OS == FLEXT_OS_MAC // that's just for OS9 & Carbon!
    UnsignedWide tick;
    Microseconds(&tick);
    tm = (tick.hi*((double)(1L<<((sizeof tick.lo)*4))*(double)(1L<<((sizeof tick.lo)*4)))+tick.lo)*1.e-6; 
#else
    #error Not implemented
#endif
    return tm-starttime;
}

void flext::Sleep(double s)
{
    if(s <= 0) return;
#if FLEXT_OS == FLEXT_OS_WIN
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x400
#if 0
    LARGE_INTEGER liDueTime;
    liDueTime.QuadPart = (LONGLONG)(-1.e7*s);

    // Create a waitable timer.
    HANDLE hTimer = CreateWaitableTimer(NULL,TRUE,NULL);
    if(hTimer) {
        if(SetWaitableTimer(hTimer,&liDueTime,0,NULL,NULL,0))
            // Wait for the timer.
            WaitForSingleObject(hTimer,INFINITE); // != WAIT_OBJECT_0)
        else
            ::Sleep((long)(s*1000.));
        CloseHandle(hTimer);
    }
    else
#else
    LARGE_INTEGER cnt;
    if(perffrq && QueryPerformanceCounter(&cnt)) {
        LONGLONG dst = (LONGLONG)(cnt.QuadPart+perffrq*s);
        for(;;) {
            SwitchToThread(); // while waiting switch to another thread
            QueryPerformanceCounter(&cnt);
            if(cnt.QuadPart > dst) break;
        }
    }
    else
#endif
#endif
        // last resort....
        ::Sleep((long)(s*1000.));
#elif FLEXT_OS == FLEXT_OS_LINUX || FLEXT_OS == FLEXT_OS_IRIX || FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH // POSIX
    usleep((long)(s*1000000.));
#elif FLEXT_OS == FLEXT_OS_MAC // that's just for OS9 & Carbon!
    UnsignedWide tick;
    Microseconds(&tick);
    double target = tick.hi*((double)(1L<<((sizeof tick.lo)*4))*(double)(1L<<((sizeof tick.lo)*4)))+tick.lo+s*1.e6; 
    for(;;) {
        // this is just a loop running until the time has passed - stone age (but we yield at least)
        Microseconds(&tick);
        if(target <= tick.hi*((double)(1L<<((sizeof tick.lo)*4))*(double)(1L<<((sizeof tick.lo)*4)))+tick.lo) break;
        YieldToAnyThread(); // yielding surely reduces the timing precision (but we're civilized)
    }
#else
    #error Not implemented
#endif
}

/* \param qu determines whether timed messages should be queued (low priority - only when supported by the system).
*/
flext::Timer::Timer(bool qu):
    queued(qu),
    clss(NULL),userdata(NULL),
    period(0)
{
#if FLEXT_SYS == FLEXT_SYS_PD
    clk = (t_clock *)clock_new(this,(t_method)callback);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    clk = (t_clock *)clock_new(this,(t_method)callback);
    if(queued) qelem = (t_qelem *)qelem_new(this,(method)queuefun);
#else
    #error Not implemented
#endif
}

flext::Timer::~Timer()
{
#if FLEXT_SYS == FLEXT_SYS_PD
    clock_free(clk);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    clock_free(clk);
    if(queued) ::qelem_free(qelem);
#else
    #error Not implemented
#endif
}

bool flext::Timer::Reset()
{
#if FLEXT_SYS == FLEXT_SYS_PD
    clock_unset(clk);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    clock_unset(clk);
    if(queued) ::qelem_unset(qelem);
#else
    #error Not implemented
#endif
    return true;
}

/*! \param tm absolute time (in seconds)
    \param data user data
    \param dopast if set events with times lying in the past will be triggered immediately, if not set they are ignored
    \return true on success
*/
bool flext::Timer::At(double tm,void *data,bool dopast)
{
    userdata = data;
    period = 0;
#if FLEXT_SYS == FLEXT_SYS_PD 
    const double systm = clock_gettimesince(0);
    double df = tm*1000.-systm;
    if(dopast && df < 0) df = 0;
    if(df >= 0)
        clock_delay(clk,df);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    const double ms = tm*1000.;
    double cur;
    clock_getftime(&cur);
    if(cur <= ms)
        clock_fdelay(clk,ms-cur);
    else if(dopast) // trigger timer is past
        clock_fdelay(clk,0);
#else
    #error Not implemented
#endif
    return true;
}

/*! \param tm relative time (in seconds)
    \param data user data
    \return true on success
*/
bool flext::Timer::Delay(double tm,void *data)
{
    userdata = data;
    period = 0;
#if FLEXT_SYS == FLEXT_SYS_PD 
    clock_delay(clk,tm*1000);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    clock_fdelay(clk,tm*1000.);
#else
    #error Not implemented
#endif
    return true;
}

/*! \param tm relative time between periodic events (in seconds)
    \param data user data
    \return true on success
    \note the first event will be delayed by tm
*/
bool flext::Timer::Periodic(double tm,void *data) 
{
    userdata = data;
	period = tm;
#if FLEXT_SYS == FLEXT_SYS_PD 
    clock_delay(clk,tm*1000.);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    clock_fdelay(clk,tm*1000.);
#else
    #error Not implemented
#endif
    return true;
}

//! \brief Callback function for system clock.
void flext::Timer::callback(Timer *tmr)
{
#if FLEXT_SYS == FLEXT_SYS_MAX
    if(tmr->queued) 
        qelem_set(tmr->qelem);
    else
#endif
        tmr->Work();

    if(tmr->period) {
		// reschedule
#if FLEXT_SYS == FLEXT_SYS_PD 
        clock_delay(tmr->clk,tmr->period*1000.);
#elif FLEXT_SYS == FLEXT_SYS_MAX
        clock_fdelay(tmr->clk,tmr->period*1000.);
#else
    #error Not implemented
#endif
    }
}

#if FLEXT_SYS == FLEXT_SYS_MAX
/*! \brief Callback function for low priority clock (for queued messages).
*/
void flext::Timer::queuefun(Timer *tmr) { tmr->Work(); }
#endif

/*! \brief Virtual worker function - by default it calls the user callback function.
    \remark The respective callback parameter format is chosen depending on whether clss is defined or not.
*/
void flext::Timer::Work()
{
    if(cback) {
        if(clss) 
            ((bool (*)(flext_base *,void *))cback)(clss,userdata);
        else
            cback(userdata);
    }
}

#include "flpopns.h"
