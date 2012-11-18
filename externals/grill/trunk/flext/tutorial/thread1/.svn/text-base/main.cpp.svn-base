/* 
flext tutorial - threads 1

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This shows an example of a method running as a thread
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


class thread1:
	public flext_base
{
	FLEXT_HEADER(thread1,flext_base)
 
public:
	thread1(); 

protected:
	void m_start(); // method function

private:
	// define threaded callback for method m_start
	// the same syntax as with FLEXT_CALLBACK is used here
	FLEXT_THREAD(m_start)
};

FLEXT_NEW("thread1",thread1)



thread1::thread1()
{ 
	AddInAnything();
	AddOutInt(); 

	FLEXT_ADDBANG(0,m_start); // register method
} 

void thread1::m_start()
{
	// Please note that this functions needs about 10 seconds to complete
	// Without threads it would block the real-time system

	// Okay, that functionality would be far more elegant with timers
	// ... but hey, it's a demo!

	for(int i = 0; i < 20 && !ShouldExit(); ++i) {
		ToOutInt(0,i); // output loop count
//		post("%i",i);

		// wait for half a second
		for(int j = 0; j < 5 && !ShouldExit(); ++j) Sleep(0.1f); 
		// note: we shall not block a thread for a longer time.
		// The system might want to destroy the object in the meantime and
		// expects thread termination. In such a case flext waits
		// for 1 second by default, then it aborts the thread brutally
	}

	// output a final zero
	ToOutInt(0,0);
//	post("end");
}



