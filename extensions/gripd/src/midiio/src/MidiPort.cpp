//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 21 December 1997
// Last Modified: Fri Jan 23 10:24:35 GMT-0800 1998
// Filename:      .../sig/code/control/MidiPort/MidiPort.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/MidiPort.cpp
// Syntax:        C++
// 
// Description:   A unified object that handles basic MIDI input and output.
//                Derived from the MidiInPort and MidiOutPort classes.
//

#include "MidiPort.h"


//////////////////////////////
//
// MidiPort::MidiPort
//

MidiPort::MidiPort(void) : MidiOutPort(), MidiInPort() {
   // nothing
}


MidiPort::MidiPort(int outputPort, int inputPort) :
      MidiOutPort(outputPort), MidiInPort(inputPort) {
   // nothing
}



//////////////////////////////
//
// MidiPort::~MidiPort
//

MidiPort::~MidiPort() {
   // nothing
}



//////////////////////////////
//
// MidiPort::getChannelInOffset -- return the MIDI channel offset of
//     the MIDI input.
//

int MidiPort::getChannelInOffset(void) const {
   return MidiInPort::getChannelOffset();
}



//////////////////////////////
//
// MidiPort::getChannelOutOffset -- return the MIDI channel offset of
//     the MIDI output.
//

int MidiPort::getChannelOutOffset (void) const {
   return MidiOutPort::getChannelOffset();
}



//////////////////////////////
//
// MidiPort::getInputPort
//

int MidiPort::getInputPort(void) {
   return MidiInPort::getPort();
}



//////////////////////////////
//
// MidiPort::getInputTrace
//

int MidiPort::getInputTrace(void) {
   return MidiInPort::getTrace();
}



//////////////////////////////
//
// MidiPort::getOutputPort
//
 
int MidiPort::getOutputPort(void) {
   return MidiOutPort::getPort();
}



//////////////////////////////
//
// MidiPort::getOutputTrace
//

int MidiPort::getOutputTrace(void) {
   return MidiOutPort::getTrace();
}



//////////////////////////////
//
// MidiPort::setChannelOffset -- sets the MIDI channel offset
//

void MidiPort::setChannelOffset(int anOffset) {
   MidiInPort::setChannelOffset(anOffset);
   MidiOutPort::setChannelOffset(anOffset);
}



//////////////////////////////
//
// MidiPort::setInputPort
//   
 
void MidiPort::setInputPort(int aPort) {
   MidiInPort::setPort(aPort);
}



//////////////////////////////
//
// MidiPort::setInputTrace
//

int MidiPort::setInputTrace(int aState) {
   return MidiInPort::setTrace(aState);
}



//////////////////////////////
//
// MidiPort::setOutputPort
//

void MidiPort::setOutputPort(int aPort) {
   MidiOutPort::setPort(aPort);
}



//////////////////////////////
//
// MidiPort::setOutputTrace
//

int MidiPort::setOutputTrace(int aState) {
   return MidiOutPort::setTrace(aState);
}



//////////////////////////////
//
// MidiPort::toggleInputTrace
//

void MidiPort::toggleInputTrace(void) {
   MidiInPort::toggleTrace();
}


//////////////////////////////
//
// MidiPort::toggleOutputTrace
//

void MidiPort::toggleOutputTrace(void) {
   MidiOutPort::toggleTrace();
}



// md5sum:	c2583f3ed21e238ba6b298915cb728aa  - MidiPort.cpp =css= 20030102
