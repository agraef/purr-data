//
// Copyright 1997-1998 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 19 December 1997
// Last Modified: Fri Jan 23 00:26:12 GMT-0800 1998
// Last Modified: Sun Sep 20 12:17:26 PDT 1998
// Last Modified: Mon Oct 15 14:29:12 PDT 2001 (added is_note functions)
// Filename:      ...sig/src/sigControl/MidiMessage.cpp
// Web Address:   http://sig.sapp.org/src/sigControl/MidiMessage.cpp
// Syntax:        C++ 
//
// Description:   A structure for handling MIDI input messages.
//                This class stores a time stamp plus up to 
//                four MIDI message bytes.  System Exclusive messages
//                are stored in a separate array in the MidiInPort
//                class, and their storage index is passed to the
//                user through a MIDI message for later extraction
//                of the full sysex message.
//

#include "MidiMessage.h"


//////////////////////////////
//
// MidiMessage::MidiMessage
//

MidiMessage::MidiMessage(void) {
   // no initialization.  Note that the default contents are undefined.
}


// default value aTime = 0
MidiMessage::MidiMessage(int aCommand, int aP1, int aP2, int aTime) {
   time = aTime;
   command() = (uchar)aCommand;
   p1() = (uchar)aP1;
   p2() = (uchar)aP2;
}


MidiMessage::MidiMessage(const MidiMessage& aMessage) {
   time = aMessage.time;
   data = aMessage.data;
}



//////////////////////////////
//
// MidiMessage::~MidiMessage -- Destructor.
//

MidiMessage::~MidiMessage() {
   // do nothing
}



//////////////////////////////
//
// MidiMessage::command -- returns the MIDI command byte
//

uchar& MidiMessage::command(void) {
   return p0();
}



//////////////////////////////
//
// MidiMessage::operator= -- defines how objects are to be copied
//

MidiMessage& MidiMessage::operator=(const MidiMessage& aMessage) {
   time = aMessage.time;
   data = aMessage.data;
   return *this;
}



//////////////////////////////
//
// MidiMessage::operator[] -- access to byte data
//     can only access index 0 to 3, other number will be
//     chopped.
//

uchar& MidiMessage::operator[](int index) {
   return *(((uchar*)&data)+(index & 0x3));
}



//////////////////////////////
//
// MidiMessage::p0 -- returns the command byte of the
//    midi message.  Same as the command() function.
//

uchar& MidiMessage::p0(void) {
   return *(((uchar*)&data)+0);
}



//////////////////////////////
//
// MidiMessage::p1 -- returns the first parameter of the
//    midi message.  Valid if the command requires a parameter.
//

uchar& MidiMessage::p1(void) {
   return *(((uchar*)&data)+1);
}



//////////////////////////////
//
// MidiMessage::p2 -- returns the second parameter of the
//    midi message.  Valid if the command requires two parameters.
//

uchar& MidiMessage::p2(void) {
   return *(((uchar*)&data)+2);
}


//////////////////////////////
//
// MidiMessage::p3 -- returns the third parameter of the
//    midi message.  Valid if the command requires three parameters
//    (but none of the MIDI command do).
//

uchar& MidiMessage::p3(void) {
   return *(((uchar*)&data)+3);
}



//////////////////////////////
//
// MidiMessage:getArgCount -- 
//

int MidiMessage::getArgCount(void) const {
   return getParameterCount();
}



//////////////////////////////
//
// MidiMessage::getParameterCount -- returns the number
//	of valid parameters for the assiciated MIDI command
//	stored in p0.  Returns -1 if MIDI command is invalid,
//	or the number of valid parameters is unknown.
//

int MidiMessage::getParameterCount(void) const {
   int output = -1;
   switch (*(((uchar*)&data)+0) & 0xf0) {
      case 0x80:                // note-off
      case 0x90:                // note-on
      case 0xa0:                // aftertouch
      case 0xb0:                // continuous controller
      case 0xe0:                // pitch wheel
         output = 2;
         break;
      case 0xc0:                // patch change
      case 0xd0:                // channel pressure
         output = 1;
         break;
      case 0xf0:
         switch (*(((uchar*)&data)+0)) {
            // System Common Messages
            case 0xf0: return -1;     // variable for sysex
            case 0xf1: return  1;     // MIDI Time Code Quarter Frame
            case 0xf2: return  2;     // Song Position Pointer
            case 0xf3: return  1;     // Song Select
            case 0xf4: return  0;     // Undefined
            case 0xf5: return  0;     // Undefined
            case 0xf6: return  0;     // Tune Request
            case 0xf7: return  0;     // End of System exclusive
            // System Real-Time Messages
            case 0xf8: return  0;     // Timing Clock
            case 0xf9: return  0;     // Undefined
            case 0xfa: return  0;     // Start
            case 0xfb: return  0;     // Continue
            case 0xfc: return  0;     // Stop
            case 0xfd: return  0;     // Undefined
            case 0xfe: return  0;     // Active Sensing
            case 0xff: return  0;     // System Reset
         }
         return -1;
         break;
      default:                  // don't know or invalid command
         output = -1;
         break;
   }

   return output;
}



//////////////////////////////
//
// MidiMessage::getCommand -- same as command().
//

uchar MidiMessage::getCommand(void) const {
   return getP0();
}



//////////////////////////////
//
// MidiMessage::getP0 -- same as p0().
//

uchar MidiMessage::getP0(void) const {
   return *(((uchar*)&data)+0);
}



//////////////////////////////
//
// MidiMessage::getP1 -- same as p1().
//

uchar MidiMessage::getP1(void) const {
   return *(((uchar*)&data)+1);
}



//////////////////////////////
//
// MidiMessage::getP2 -- same as p2().
//

uchar MidiMessage::getP2(void) const {
   return *(((uchar*)&data)+2);
}



//////////////////////////////
//
// MidiMessage::getP3 -- same as p3().
//

uchar MidiMessage::getP3(void) const {
   return *(((uchar*)&data)+3);
}



//////////////////////////////
//
// MidiMessage::setCommand -- same as command().
//

void MidiMessage::setCommand(uchar aCommand) {
   command() = aCommand;
}



//////////////////////////////
//
// MidiMessage::setData -- sets the message bytes
//	default values: aP1 = 0, aP2 = 0, aP3 = 0.
//

void MidiMessage::setData(uchar aCommand, uchar aP1, uchar aP2, uchar aP3) {
   setCommand(aCommand);
   setP1(aP1);
   setP2(aP2);
   setP3(aP3);
}



//////////////////////////////
//
// MidiMessage::setP0 -- same as p0().
//

void MidiMessage::setP0(uchar aP0) {
   p0() = aP0;
}



//////////////////////////////
//
// MidiMessage::setP1 -- same as p1().
//

void MidiMessage::setP1(uchar aP1) {
   p1() = aP1;
}



//////////////////////////////
//
// MidiMessage::setP2 -- same as p2().
//

void MidiMessage::setP2(uchar aP2) {
   p2() = aP2;
}



//////////////////////////////
//
// MidiMessage::setP3 -- same as p3().
//

void MidiMessage::setP3(uchar aP3) {
   p3() = aP3;
}



//////////////////////////////
//
// MidiMessage::is_note -- Returns true if the MIDI command is 0x80 or 0x90.
//

int MidiMessage::is_note(void) {
   if ((p0() & 0xf0) == 0x90) {         // note-on or note-off
      return 1;
   } else if ((p0() & 0xf0) == 0x80) {   // note-off
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiMessage::is_note_on -- Returns true if the MIDI command is a note
//     on message (0x90 series with p2() > 0).
//

int MidiMessage::is_note_on(void) {
   if (((p0() & 0xf0) == 0x90) &&  p2() > 0) {
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiMessage::is_note_off -- Returns true if the MIDI command is a note
//     off message (0x80 series or 0x90 series with p2() == 0).
//

int MidiMessage::is_note_off(void) {
   if ((p0() & 0xf0) == 0x80) { 
      return 1;
   } else if (((p0() & 0xf0) == 0x90) && (p2() == 0)) {
      return 1;
   } else {
      return 0;
   }
}


///////////////////////////////////////////////////////////////////////////

//////////////////////////////
//
// operator<< MidiMessage
//

std::ostream& operator<<(std::ostream& out, MidiMessage& aMessage) {
   out << "(" << aMessage.time << ") " 
       << std::hex << (int)aMessage.getP0() << ": ";
   for (int i=1; i<=aMessage.getArgCount(); i++) {
      out << std::dec << (int)aMessage[i] << ' ';
   }

   return out;
}



// md5sum:	487f0fddeb8db20d81f9c039e2a460c9  - MidiMessage.cpp =css= 20030102
