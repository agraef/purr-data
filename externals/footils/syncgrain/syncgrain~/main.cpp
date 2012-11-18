//#=====================================================================================
//#
//#       Filename:  main.cpp
//#
//#    Description:  SyncGrain~ PD/Max external main code.
//#
//#        Version:  1.0
//#        Created:  01/09/03
//#       Revision:  none
//#
//#         Author:  Frank Barknecht  (fbar)
//#          Email:  fbar@footils.org
//#      Copyright:  Frank Barknecht , 2003
//#
//#        This program is free software; you can redistribute it and/or modify
//#    it under the terms of the GNU General Public License as published by
//#    the Free Software Foundation; either version 2 of the License, or
//#    (at your option) any later version.
//#
//#    This program is distributed in the hope that it will be useful,
//#    but WITHOUT ANY WARRANTY; without even the implied warranty of
//#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#    GNU General Public License for more details.
//#
//#    You should have received a copy of the GNU General Public License
//#    along with this program; if not, write to the Free Software
//#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//#
//#
//#=====================================================================================



// include flext header
#include <flext.h>
#include <flsndobj.h>
#include "buftable.h"
#include "BufSyncGrain.h"
// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1
#endif


class syncgrain:
	public flext_sndobj
{
	FLEXT_HEADER(syncgrain,flext_sndobj)
 
public:
	// constructor 
	syncgrain(int argc, t_atom *argv);
	// destroy
	// ~syncgrain();
	virtual bool NewObjs();
	virtual void FreeObjs();
	virtual void ProcessObjs();
	virtual void m_help();
protected:
	int mbuf_set(int argc,const t_atom *argv);
	void m_freq(float f);   

private:
	BufSyncGrain * grain;
	HammingTable *envtable;
	bool Ok();
	
	float fr;
	float amp; 
	float pitch;
	float grsize; 
	float prate; 
	int olaps; 
	
	int sr;
	int blocksize;
	int tablesize;
	
	t_symbol * bufname;
	buftable *buf; 
	
	FLEXT_CALLBACK_F(setFreq)
	void setFreq(float f);
	FLEXT_CALLBACK_F(setAmp)
	void setAmp(float f);
	FLEXT_CALLBACK_F(setPitch)
	void setPitch(float f);
	FLEXT_CALLBACK_F(setGrainSize)
	void setGrainSize(float f);
	FLEXT_CALLBACK_F(setPointerRate)
	void setPointerRate(float f);
	FLEXT_CALLBACK_V(mbuf_set)
};

// instantiate the class 
FLEXT_NEW_DSP_V("syncgrain~",syncgrain)


syncgrain::syncgrain(int argc, t_atom *argv)
	:  // initialize arguments
	fr(0.f), amp(1.f), pitch(1.f), grsize (0.f), prate(1.f), olaps(100), bufname(NULL)
	
{ 
	// define inlets
	AddInAnything();
	AddInFloat(5);
	AddOutSignal();         // 1 audio out [ == AddOutSignal(1) ]

	FLEXT_ADDMETHOD_(0,"set",mbuf_set);
	FLEXT_ADDMETHOD(1,setFreq);
	FLEXT_ADDMETHOD(2,setAmp);
	FLEXT_ADDMETHOD(3,setPitch);
	FLEXT_ADDMETHOD(4,setGrainSize);
	FLEXT_ADDMETHOD(5,setPointerRate);
} 


bool syncgrain::Ok()
{
	bool ret = false;
	if 
	(
		buf 		&&
		grain 		&&
		buf->Ok()	&&
		buf->GetTable()	&&
		buf->GetLen() > 0
	)
		ret = true;
	return ret;
}


bool syncgrain::NewObjs()
{
	// set up objects
	buf = new buftable(bufname);
	tablesize = buf->GetLen();
	//post("Created buf");
	envtable  = new HammingTable();
	sr = static_cast<int>(Samplerate());	
	blocksize = static_cast<int>(Blocksize());	
	grain = new BufSyncGrain(
		buf, envtable, fr, amp, pitch, grsize, prate, 0, 0, 0, 0, olaps, 
		blocksize, sr );
	// post("Created grain");
	return true;
}

// destroy the SndObjs
void syncgrain::FreeObjs()
{
	if (grain)	delete grain;
	if (buf) 	delete buf;
	if (envtable)	delete envtable;
	tablesize = 0;
}

// this is called on every DSP block
void syncgrain::ProcessObjs()
{
	if ( Ok() )
	{
		grain->DoProcess();

		// output
		*grain >> OutObj(0);
	}
}

int syncgrain::mbuf_set(int argc,const t_atom *argv)
{
	bufname  = argc >= 1 ? GetASymbol(argv[0]) : NULL;
	FreeObjs();
	NewObjs();
	tablesize = buf->GetLen();
	return tablesize;
	
}

void syncgrain::setFreq(float f)
{
	fr = f;
	if ( Ok() )
	{
		// set current pitch shift
		grain->SetFreq(fr);
	}
}

void syncgrain::setAmp(float f)
{
	amp = f;
	if ( Ok() )
	{
		grain->SetAmp(f);
	}
}

void syncgrain::setPitch(float f)
{
	if ( Ok() )
	{
		pitch = f * (float)(tablesize/sr);
		grain->SetPitch(f);
	}
}

void syncgrain::setGrainSize(float f)
{
	grsize = f * 0.001;
	if ( Ok() )
	{
		//post("Grainsize now: %f", grsize);
		grain->SetGrainSize(grsize);
	}
}

void syncgrain::setPointerRate(float f)
{
	prate = f;
	if ( Ok() )
	{
		grain->SetPointerRate(prate);
	}
}

void syncgrain::m_help()
{
	post("");
	post("_ _____syncgrain~ help___ _");
	post("_ _____SyncGrain implements synchronous granular synthesis.\n\nThe source sound for the grains is obtained by reading\na function table containing the samples of the source\nwaveform. The grain generator has full control of frequency (grains/sec),\noverall amplitude, grain pitch (a sampling increment)\nand grain size (in millisecs). An extra parameter is the grain\npointer speed (or rate), which controls which position\nthe generator will start reading samples in the table\nfor each successive grain. It is measured in fractions\nof grain size, so a value of 1 will make\neach successive grain read from where the previous\ngrain should finish. A value of 0.5 will make the next\ngrain start at the midway position from the previous\ngrain start and finish, etc. A value of 0 will make\nthe generator read always from the start of the table.\nThis control gives extra flexibility for creating\ntimescale modifications in the resynthesis.\n");
	post("_ ___help __ _");
	post("1. inlet: \"set name\" message to choose grain source.");
	post("2. inlet: Grain Frequency in grains per second");
	post("3. inlet: Amplitude, default starting value is 1");
	post("4. inlet: Pitch of the grains");
	post("5. inlet: GrainSize in milli-seconds! Good values are between 10 and 50 msec.");
	post("6. inlet: PointerRate. Normally 0-1, but have fun with greater or smaller values!");
}
	
