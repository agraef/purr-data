/* 
flext tutorial - timer 1 

Copyright (c) 2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of an object using timers
*/

// enable flext attributes
#define FLEXT_ATTRIBUTES 1

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 403)
#error You need at least flext version 0.4.3
#endif


// define the class that stands for a pd/Max object

class timer1:
	// inherit from basic flext class
	public flext_base
{
	// obligatory flext header (class name,base class name)
	FLEXT_HEADER_S(timer1,flext_base,Setup)
 
public:
	// constructor
	timer1();

protected:
	// timers
	Timer tmrA,tmrB;

	void m_getostime(float &f) { f = (float)GetOSTime(); }  // method for operating system time attribute
	void m_getrttime(float &f) { f = (float)GetTime(); }  // method for real-time system time attribute

	void m_timerA(void *) { ToOutString(0,"Timer A"); }  // timer A method
	void m_timerB(void *) { ToOutString(0,"Timer B"); }  // timer B method

	void m_resetA() { tmrA.Reset(); }  // timer A reset
	void m_resetB() { tmrB.Reset(); }  // timer B reset
	void m_oneshotA(int del) { tmrA.Delay(del*0.001); }  // timer A one shot
	void m_oneshotB(int del) { tmrB.Delay(del*0.001); }  // timer B one shot
	void m_periodicA(int del) { tmrA.Periodic(del*0.001); }  // timer A periodic
	void m_periodicB(int del) { tmrB.Periodic(del*0.001); }  // timer B periodic

private:
	static void Setup(t_classid c);

	// register timer callbacks
	FLEXT_CALLBACK_T(m_timerA)  
	FLEXT_CALLBACK_T(m_timerB)  
	
	// register method callbacks
	FLEXT_CALLGET_F(m_getostime)
	FLEXT_CALLGET_F(m_getrttime)
	FLEXT_CALLBACK(m_resetA)
	FLEXT_CALLBACK(m_resetB)
	FLEXT_CALLBACK_I(m_oneshotA)
	FLEXT_CALLBACK_I(m_oneshotB)
	FLEXT_CALLBACK_I(m_periodicA)
	FLEXT_CALLBACK_I(m_periodicB)
};

// instantiate the class
FLEXT_NEW("timer1",timer1)

// class setup function
void timer1::Setup(t_classid c)
{
	FLEXT_CADDATTR_GET(c,"ostime",m_getostime);  // register attribute for OS time
	FLEXT_CADDATTR_GET(c,"time",m_getrttime);  // register attribute for RT time
	
	FLEXT_CADDMETHOD_(c,0,"resetA",m_resetA);  // register reset method for timer A
	FLEXT_CADDMETHOD_(c,0,"resetB",m_resetB);  // register reset method for timer B
	FLEXT_CADDMETHOD_(c,0,"oneshotA",m_oneshotA);  // register one shot method for timer A
	FLEXT_CADDMETHOD_(c,0,"oneshotB",m_oneshotB);  // register one shot method for timer B
	FLEXT_CADDMETHOD_(c,0,"periodicA",m_periodicA);  // register periodic method for timer A
	FLEXT_CADDMETHOD_(c,0,"periodicB",m_periodicB);  // register periodic method for timer B
}

// class constructor
timer1::timer1():
	tmrA(false),tmrB(false)
{ 
	AddInAnything("Control timers");  // add inlet for control commands
	AddOutAnything("Timer output"); // add outlet for timer output
	
	// register methods
	FLEXT_ADDTIMER(tmrA,m_timerA);  // register method "m_timerA" for timer A
	FLEXT_ADDTIMER(tmrB,m_timerB);  // register method "m_timerB" for timer B	
}
