/* 
flext tutorial - threads 2 

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This shows an example of multiple threads and syncing with a thread conditional
*/

/* define FLEXT_THREADS for thread usage. Flext must also have been compiled with that defined!
	it's even better to define that as a compiler flag (-D FLEXT_THREADS) for all files of the
	flext external
*/
#ifndef FLEXT_THREADS
#define FLEXT_THREADS
#endif

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


class thread2:
	public flext_base
{
	FLEXT_HEADER(thread2,flext_base)
 
public:
	thread2(int del); 

protected:
	void m_start(int st);
	void m_stop();
	void m_text();

	void m_textout();

private:
	FLEXT_THREAD_I(m_start) // define threaded callback for method m_start
	FLEXT_CALLBACK(m_stop)  // normal callback for m_stop
	FLEXT_CALLBACK(m_text)  // turn on console output

	FLEXT_THREAD(m_textout) // text output

	float delay;
	volatile int count;

	// caution: CodeWarrior seems to ignore volatile modifier!!
	volatile bool stopit,running,blipping;  // flags for running and stopping

	// thread conditional for stop signal
	ThrCond cond;
};

FLEXT_NEW_1("thread2",thread2,int)



thread2::thread2(int del):
	delay(del/1000.f),
	stopit(false),
	running(false),blipping(false)
{ 
	AddInAnything();  
	AddOutInt(2); 

	FLEXT_ADDMETHOD(0,m_start); // register start for integer numbers (floats in PD)
	FLEXT_ADDMETHOD_(0,"text",m_text); // register m_text method for "text" tag
	FLEXT_ADDMETHOD_(0,"stop",m_stop); // register m_text method for "stop" tag
} 

void thread2::m_start(int st)
{
	// if already running, just set back the counter
	if(running) { count = st; return; }

	running = true;

	// loop until either the system exit flag or the "stopit" flag is set 
	for(count = st; !ShouldExit() && !stopit; ++count) 
	{
		Sleep(delay);
		ToOutInt(0,count); // output loop count
	}

	running = false; // change state flag
//	cond.Lock(); // lock conditional
	cond.Signal(); // signal changed flag to waiting "stop" method
//	cond.Unlock(); // unlock conditional
}

void thread2::m_stop()
{
//	cond.Lock(); // lock conditional
	stopit = true; // set termination flag

	while(*(&running) || *(&blipping)) // workaround for CodeWarrior (doesn't honor volatile modifier!)
	{
		cond.Wait(); // wait for signal by running threads
	}

	// --- Here, the threads should have stopped ---

	stopit = false; // reset flag
//	cond.Unlock(); // unlock conditional
}


void thread2::m_text()
{
	FLEXT_CALLMETHOD(m_textout);
}

void thread2::m_textout()
{
	if(blipping) return;
	blipping = true;

	while(!ShouldExit() && !stopit) {
		post("%i",count);
		Sleep(1.f);
	}

	blipping = false; // change state flag
//	cond.Lock(); // lock conditional
	cond.Signal(); // signal changed flag to waiting "stop" method
//	cond.Unlock(); // unlock conditional
}

