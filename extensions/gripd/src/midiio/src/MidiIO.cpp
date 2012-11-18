//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 21 December 1997
// Last Modified: Sun Jan 25 15:45:18 GMT-0800 1998
// Filename:      ...sig/code/control/MidiIO/MidiIO.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/MidiIO.cpp
// Syntax:        C++
// 
// Description:   A unified class for MidiInput and MidiOutput that handles 
//                MIDI input and output connections.  The Synthesizer
//                and RadioBaton classes are derived from this class.
//

#include "MidiIO.h"


//////////////////////////////
//
// MidiIO::MidiIO
//

MidiIO::MidiIO(void) : MidiOutput(), MidiInput() {
   // does nothing
}


MidiIO::MidiIO(int outPort, int inPort) :
      MidiOutput(outPort), MidiInput(inPort) {
   // does nothing
}



//////////////////////////////
//
// MidiIO::~MidiIO
//

MidiIO::~MidiIO() {
   // does nothing
}



//////////////////////////////
//
// MidiIO::close
//

void MidiIO::close(void) {
   MidiInput::close();
   MidiOutput::close();
}



//////////////////////////////
//
// MidiIO::closeInput
//

void MidiIO::closeInput(void) {
   MidiInput::close();
}



//////////////////////////////
//
// MidiIO::closeOutput
//

void MidiIO::closeOutput(void) {
   MidiOutput::close();
}



//////////////////////////////
//
// MidiIO::getChannelInOffset -- return the MIDI channel offset of
//     the MIDI input.
//

int MidiIO::getChannelInOffset(void) const {
   return MidiInPort::getChannelOffset();
}



//////////////////////////////
//
// MidiIO::getChannelOutOffset -- return the MIDI channel offset of
//     the MIDI output.
//

int MidiIO::getChannelOutOffset (void) const {
   return MidiOutPort::getChannelOffset();
}



//////////////////////////////
//
// MidiIO::getInputPort
//

int MidiIO::getInputPort(void) {
   return MidiInput::getPort();
}



//////////////////////////////
//
// MidiIO::getInputTrace
//

int MidiIO::getInputTrace(void) {
   return MidiInput::getTrace();
}



//////////////////////////////
//
// MidiIO::getNumInputPorts
//

int MidiIO::getNumInputPorts(void) {
   return MidiInput::getNumPorts();
}



//////////////////////////////
//
// MidiIO::getNumOutputPorts
//

int MidiIO::getNumOutputPorts(void) {
   return MidiOutput::getNumPorts();
}



//////////////////////////////
//
// MidiIO::getOutputPort
//
 
int MidiIO::getOutputPort(void) {
   return MidiOutput::getPort();
}



//////////////////////////////
//
// MidiIO::getOutputTrace
//

int MidiIO::getOutputTrace(void) {
   return MidiOutput::getTrace();
}



//////////////////////////////
//
// MidiIO::open
//

int MidiIO::open(void) {
   if (MidiInput::open()) {
      return MidiOutput::open();
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiIO::openInput
//

int MidiIO::openInput(void) {
   return MidiInput::open();
}



//////////////////////////////
//
// MidiIO::openOutput
//

int MidiIO::openOutput(void) {
   return MidiOutput::open();
}



//////////////////////////////
//
// MidiIO::setChannelOffset -- sets the MIDI channel offset
//

void MidiIO::setChannelOffset(int anOffset) {
   MidiInPort::setChannelOffset(anOffset);
   MidiOutPort::setChannelOffset(anOffset);
}



//////////////////////////////
//
// MidiIO::setInputPort
//   
 
void MidiIO::setInputPort(int aPort) {
   MidiInput::setPort(aPort);
}



//////////////////////////////
//
// MidiIO::setInputTrace
//

void MidiIO::setInputTrace(int aState) {
   MidiInput::setTrace(aState);
}



//////////////////////////////
//
// MidiIO::setOutputPort
//

void MidiIO::setOutputPort(int aPort) {
   MidiOutput::setPort(aPort);
}



//////////////////////////////
//
// MidiIO::setOutputTrace
//

void MidiIO::setOutputTrace(int aState) {
   MidiOutput::setTrace(aState);
}



//////////////////////////////
//
// MidiIO::toggleInputTrace
//

void MidiIO::toggleInputTrace(void) {
   MidiInput::toggleTrace();
}


//////////////////////////////
//
// MidiIO::toggleOutputTrace
//

void MidiIO::toggleOutputTrace(void) {
   MidiOutput::toggleTrace();
}



// md5sum:	860227c67236eb6f8897ae67f1338cb0  - MidiIO.cpp =css= 20030102
