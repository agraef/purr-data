//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Oct 11 18:39:14 PDT 1998
// Last Modified: Sun Oct 11 18:39:18 PDT 1998
// Filename:      ...sig/maint/code/control/Voice/Voice.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/Voice.cpp
// Syntax:        C++
//
// Description:   The Voice class is a MIDI output class which keeps
//                track of the last note played in to it.  If the last
//                note was not turned off when a new note is played,
//                then the old note will be turned off before the 
//                new note is played.
//

#include "Voice.h"


// declare static variables:
SigTimer Voice::timer;

//////////////////////////////
//
// Voice::Voice
//

Voice::Voice(int aChannel) {
   chan = oldChan = aChannel;
   vel = oldVel = key = oldKey = 0;
}


Voice::Voice(const Voice& aVoice) {
   chan = aVoice.chan;
   vel = aVoice.vel;
   key = aVoice.key;
   oldChan = 0;
   oldVel = 0;
   oldKey = 0;
}


Voice::Voice(void) {
   chan = oldChan = key = oldKey = vel = oldVel = 0;
}


//////////////////////////////
//
// Voice::~Voice
//

Voice::~Voice() {
   off();
}



//////////////////////////////
//
// Voice::cont -- use default channel if none specified for
//   the continuous controller message.
//

void Voice::cont(int controller, int data) {
   MidiOutput::cont(getChannel(), controller, data);
}



//////////////////////////////
//
// Voice::getChan -- returns the channel of the voice.  
//     Synonym for getChannel.
//

int Voice::getChan(void) const {
   return chan;
}



//////////////////////////////
//
// Voice::getChannel -- returs the channel of the voice.
//

int Voice::getChannel(void) const {
   return chan;
}



//////////////////////////////
//
// Voice::getKey -- returns the current MIDI key number of the voice.
//     Synonym for getKeynum.
//

int Voice::getKey(void) const {
   return key;
}



//////////////////////////////
//
// Voice::getKeynum -- returns the current MIDI key number of the voice.
//

int Voice::getKeynum(void) const {
   return key;
}


//////////////////////////////
//
// Voice::getOffTime -- returns the last note off message sent
//     out of the voice object
//

int Voice::getOffTime(void) const {
   return offTime;
}



//////////////////////////////
//
// Voice::getOnTime -- returns the last note on message sent
//     out of the voice object.
//

int Voice::getOnTime(void) const {
   return onTime;
}



//////////////////////////////
//
// Voice::getVel -- returns the current velocity of the MIDI key.
//     Synonym for getVelocity.
//

int Voice::getVel(void) const {
   return vel;
}



//////////////////////////////
//
// Voice::getVelocity -- returns the current velocity of the MIDI key.
//

int Voice::getVelocity(void) const {
   return vel;
}



//////////////////////////////
//
// Voice::off
//

void Voice::off(void) {
   if (status() != 0) {
      offTime = timer.getTime();
      MidiOutput::play(oldChan, oldKey, 0);
      oldVel = 0;
   }
}



//////////////////////////////
//
// Voice::pc -- use default channel if none is specified
//

void Voice::pc(int aTimbre) {
   MidiOutput::pc(getChannel(), aTimbre);
}



//////////////////////////////
//
// Voice::play
//

void Voice::play(int aChannel, int aKeyno, int aVelocity) {
   off();
   MidiOutput::play(aChannel, aKeyno, aVelocity);
   oldChan = aChannel;
   oldKey = aKeyno;
   oldVel = aVelocity;
   setChannel(aChannel);
   setKeynum(aKeyno);
   setVelocity(aVelocity);

   if (aVelocity != 0) {
      onTime = timer.getTime();
   } else {
      offTime = timer.getTime();
   }
}


void Voice::play(int aKeyno, int aVelocity) {
   off();
   MidiOutput::play(getChannel(), aKeyno, aVelocity);
   oldChan = getChannel();
   oldKey = aKeyno;
   oldVel = aVelocity;
   setKeynum(aKeyno);
   setVelocity(aVelocity);

   if (aVelocity != 0) {
      onTime = timer.getTime();
   } else {
      offTime = timer.getTime();
   }
}


void Voice::play(void) {
   off();
   MidiOutput::play(getChannel(), getKey(), getVel());
   oldChan = getChannel();
   oldKey = getKey();
   oldVel = getVel();

   if (getVel() != 0) {
      onTime = timer.getTime();
   } else {
      offTime = timer.getTime();
   }
}



//////////////////////////////
//
// Voice::setChan -- set the MIDI channel.  Synonym for setChannel.
//

void Voice::setChan(int aChannel) {
   chan = aChannel;
}



//////////////////////////////
//
// Voice::setChannel -- set the MIDI channel of the voice.
//

void Voice::setChannel(int aChannel) {
   chan = aChannel;
}



//////////////////////////////
//
// Voice::setKey -- set the keynumber of the voice
//     Synonym for setKeyno.
//

void Voice::setKey(int aKeynum) {
   key = aKeynum;
}



//////////////////////////////
//
// Voice::setKeynum -- set the keynumber of the voice
//

void Voice::setKeynum(int aKeynum) {
   key = aKeynum;
}



//////////////////////////////
//
// Voice::setVel -- set the MIDI velocity of the voice.
//     Synonym for setVelocity.
//

void Voice::setVel(int aVelocity) {
   vel = aVelocity;
}



//////////////////////////////
//
// Voice::setVelocity
//

void Voice::setVelocity(int aVelocity) {
   vel = aVelocity;
}



//////////////////////////////
//
// Voice::status -- returns zero if off or positive if on.
//

int Voice::status(void) const {
   return oldVel;
}



//////////////////////////////
//
//  Voice::sustain -- uses default channel if none specified.
//

void Voice::sustain(int aStatus) {
   MidiOutput::sustain(getChannel(), aStatus);
}


// md5sum:	d02ca41ae3b4e07efe7fedc720e52573  - Voice.cpp =css= 20030102
