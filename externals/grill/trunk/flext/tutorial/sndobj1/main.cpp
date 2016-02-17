/* 
flext tutorial - sndobj 1

Copyright (c) 2002-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of an external using the SndObj library.
See http://www.may.ie/academic/music/musictec/SndObj/

The SndObj library should be compiled multithreaded.

This external features simple stereo pitch shifting.

*/

#define FLEXT_ATTRIBUTES 1

#include <flsndobj.h>
 
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1 with sndobj support
#endif


class sndobj1:
	public flext_sndobj
{
	FLEXT_HEADER_S(sndobj1,flext_sndobj,Setup)

public:
	sndobj1();

	// these are obligatory!
	virtual bool NewObjs();
	virtual void FreeObjs();
	virtual void ProcessObjs();

	// space for a few sndobjs
	Pitch *obj1,*obj2;

	float sh1,sh2;

private:
	static void Setup(t_classid c);

	FLEXT_ATTRVAR_F(sh1)
	FLEXT_ATTRVAR_F(sh2)
};

FLEXT_NEW_DSP("sndobj1~",sndobj1)


sndobj1::sndobj1():
	sh1(1),sh2(1),
	obj1(NULL),obj2(NULL)
{ 
	AddInSignal(2);  // audio ins
	AddOutSignal(2);  // audio outs
}

void sndobj1::Setup(t_classid c)
{
	FLEXT_CADDATTR_VAR1(c,"shL",sh1);
	FLEXT_CADDATTR_VAR1(c,"shR",sh2);
}

// construct needed SndObjs
bool sndobj1::NewObjs()
{
	// set up objects
	obj1 = new Pitch(.1f,&InObj(0),sh1,Blocksize(),Samplerate());
	obj2 = new Pitch(.1f,&InObj(1),sh2,Blocksize(),Samplerate());
	return true;
}

// destroy the SndObjs
void sndobj1::FreeObjs()
{
	if(obj1) delete obj1;
	if(obj2) delete obj2;
}

// this is called on every DSP block
void sndobj1::ProcessObjs()
{
	// set current pitch shift
	obj1->SetPitch(sh1);
	obj2->SetPitch(sh2);

	// do processing here!!
	obj1->DoProcess();
	obj2->DoProcess();

	// output
	*obj1 >> OutObj(0);
	*obj2 >> OutObj(1);
}

