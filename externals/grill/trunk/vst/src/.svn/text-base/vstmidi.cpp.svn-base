/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "vsthost.h"

bool VSTPlugin::AddMIDI(unsigned char data0,unsigned char data1,unsigned char data2)
{
	if(Is()) {
		VstMidiEvent *pevent = &midievent[eventqusz];

		pevent->type = kVstMidiType;
		pevent->byteSize = 24;
		pevent->deltaFrames = 0;
		pevent->flags = 0;
		pevent->detune = 0;
		pevent->noteLength = 0;
		pevent->noteOffset = 0;
		pevent->reserved1 = 0;
		pevent->reserved2 = 0;
		pevent->noteOffVelocity = 0;
		pevent->midiData[0] = data0;
		pevent->midiData[1] = data1;
		pevent->midiData[2] = data2;
		pevent->midiData[3] = 0;

		if(eventqusz < MIDI_MAX_EVENTS) ++eventqusz;
		SendMidi();
		return true;
	}
	else return false;
}


void VSTPlugin::SendMidi()
{
	if(Is() && eventqusz > 0) {
		// Prepare MIDI events and free queue dispatching all events
		events.numEvents = eventqusz;
		events.reserved  = 0;
		for(int q = 0; q < eventqusz; q++) 
            events.events[q] = (VstEvent*)&midievent[q];
		
		Dispatch(effProcessEvents, 0, 0, &events, 0.0f);
		eventqusz = 0;
	}
}
