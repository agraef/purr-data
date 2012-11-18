//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu> 
// Creation Date: 18 December 1997
// Last Modified: Sun Jan 25 15:31:49 GMT-0800 1998
// Last Modified: Thu Apr 27 17:56:03 PDT 2000 (added scale function)
// Filename:      ...sig/code/control/MidiInput/MidiInput.cpp
// Web Address:   http://sig.sapp.org/src/sig/MidiInput.cpp
// Syntax:        C++
//
// Description:   A higher-level MIDI input interface than the 
//                MidiInPort class.  Can be used to allow multiple
//                objects to share a single MIDI input stream, or
//                to fake a MIDI input connection.
//

#include "MidiInput.h"
#include <stdlib.h>
#include <iostream>


//////////////////////////////
//
// MidiInput::MidiInput -- opens the specified MIDI input device and
//    sets the size of the input buffer.
//

MidiInput::MidiInput(void) : MidiInPort() {
   orphanBuffer = NULL;
}


MidiInput::MidiInput(int aPort, int autoOpen) : MidiInPort(aPort, autoOpen) {
   orphanBuffer = NULL;
}



//////////////////////////////
//
// MidiInput::~MidiInput
//

MidiInput::~MidiInput() {
   if (orphanBuffer != NULL) {
      delete orphanBuffer;
      orphanBuffer = NULL;
   }
}



//////////////////////////////
//
// MidiInput::getBufferSize 
//

int MidiInput::getBufferSize(void) {
   if (isOrphan()) {
      return orphanBuffer->getSize();
   } else {
      return MidiInPort::getBufferSize();
   }
}



//////////////////////////////
//
// MidiInput::getCount
//

int MidiInput::getCount(void) {
   if (isOrphan()) {
      return orphanBuffer->getCount();
   } else {
      return MidiInPort::getCount();
   }
}



//////////////////////////////
//
// MidiInput::extract
//

MidiMessage MidiInput::extract(void) {
   if (isOrphan()) {
      return orphanBuffer->extract();
   } else {
      return MidiInPort::extract();
   }
}



//////////////////////////////
//
// fscale -- converts a value in the range from 0 to 127
//      into a number in a new range.  For example the value
//      64 scaled to the range from 0 to 2 would be 1.
//

double MidiInput::fscale(int value, double min, double max) {
   return value >= 127 ? max : (value/127.0*(max-min)+min);
}



//////////////////////////////
//
// fscale14 -- converts a value in the range from 0 to 2^15-1
//      into a number in a new range.
//

double MidiInput::fscale14(int value, double min, double max) {
   return value >= 16383 ? max : (value/16383.0*(max-min)+min);
}



//////////////////////////////
//
// MidiInput::insert
//

void MidiInput::insert(const MidiMessage& aMessage) {
   if (isOrphan()) {
      orphanBuffer->insert(aMessage);
   } else {
      MidiInPort::insert(aMessage);
   }
}



//////////////////////////////
//
// MidiInput::isOrphan
//

int MidiInput::isOrphan(void) const {
   if (orphanBuffer == NULL) {
      return 0;
   } else {
      return 1;
   }
}



//////////////////////////////
//
// MidiInput::makeOrphanBuffer
//     default value: aSize = 1024
//

void MidiInput::makeOrphanBuffer(int aSize) {
   if (!isOrphan()) {
      if (orphanBuffer == NULL) {
         delete orphanBuffer;
         orphanBuffer = NULL;
      }
      orphanBuffer = new CircularBuffer<MidiMessage>(aSize);
   }
}



//////////////////////////////
//
// MidiInput::removeOrphanBuffer
//

void MidiInput::removeOrphanBuffer(void) {
   if (isOrphan()) {
      delete orphanBuffer;
      orphanBuffer = NULL;
   }
}



//////////////////////////////
//
// scale -- converts a value in the range from 0 to 127
//      into a number in a new range.  For example the value
//      64 scaled to the range from 0 to 2 would be 1.
//

int MidiInput::scale(int value, int min, int max) {
   return value >= 127 ? max : (int)(value/127.0*(max-min+1)+min);
}



//////////////////////////////
//
// scale14 -- converts a value in the range from 0 to 2^15-1
//      into a number in a new range.  
//

int MidiInput::scale14(int value, int min, int max) {
   return value >= 16383 ? max : (int)(value/16383.0*(max-min+1)+min);
}



//////////////////////////////
//
// MidiInput::setBufferSize
//

void MidiInput::setBufferSize(int aSize) {
   if (isOrphan()) {
      orphanBuffer->setSize(aSize);
   } else {
      MidiInPort::setBufferSize(aSize);
   }
}   



// md5sum:	826d403708263eaf0089b4742179c58c  - MidiInput.cpp =css= 20030102
