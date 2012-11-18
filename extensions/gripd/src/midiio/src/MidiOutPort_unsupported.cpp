//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Jan 12 21:40:35 GMT-0800 1998
// Last Modified: Mon Jan 12 21:40:39 GMT-0800 1998
// Filename:      ...sig/code/control/MidiOutPort/unsupported/MidiOutPort_unsupported.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/MidiOutPort_unsupported.cpp
// Syntax:        C++ 
//
// Description:   Operating-System specific interface for basic MIDI output
//                capabilities in an unknown operating system.  Privately 
//                inherited by the MidiOutPort class. Used for compiling
//                and running MIDI programs on a computer with no
//                MIDI output.
//

#include "MidiOutPort_unsupported.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sstream>

// initialized static variables
int       MidiOutPort_unsupported::numDevices      = 0;
int       MidiOutPort_unsupported::objectCount     = 0;
int*      MidiOutPort_unsupported::openQ           = NULL;
int*      MidiOutPort_unsupported::portObjectCount = NULL;
int       MidiOutPort_unsupported::channelOffset   = 0;


//////////////////////////////
// 
// MidiOutPort_unsupported::MidiOutPort_unsupported
//	default values: autoOpen = 1
//


MidiOutPort_unsupported::MidiOutPort_unsupported(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(0);
}


MidiOutPort_unsupported::MidiOutPort_unsupported(int aPort, int autoOpen) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(aPort);
   if (autoOpen) {
      open();
   }
}



//////////////////////////////
//
// MidiOutPort_unsupported::~MidiOutPort_unsupported
//

MidiOutPort_unsupported::~MidiOutPort_unsupported() {
   objectCount--;
   if (objectCount == 0) {
      deinitialize();
   } else if (objectCount < 0) {
      std::cerr << "Error: bad MidiOutPort object count!: " << objectCount << std::endl;
      exit(1);
   }
}



//////////////////////////////
//
// MidiOutPort_unsupported::close
//

void MidiOutPort_unsupported::close(void) {
   if (getPortStatus() == 1) {
      setPortStatus(0);
   }
}



//////////////////////////////
//
// MidiOutPort_unsupported::closeAll
//

void MidiOutPort_unsupported::closeAll(void) {
   for (int i=0; i<getNumPorts(); i++) {
      if (openQ[i] == 1) {
         openQ[i] = 0;
      }
   }
}



//////////////////////////////
//
// MidiOutPort_unsupported::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiOutPort_unsupported::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiOutPort_unsupported::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until getName is called again.
//

const char* MidiOutPort_unsupported::getName(void) const {
   static char* name = NULL;
   std::stringstream temp;
   temp << "Inactive MIDI output test port #";
   temp << getPort();
   if (name != NULL) delete [] name;
   name = new char[temp.str().length()+1];
   strcpy(name, temp.str().c_str());
   return name;
}

const char* MidiOutPort_unsupported::getName(int i) const {
   static char* name = NULL;
   std::stringstream temp;
   temp << "Inactive MIDI output test port #";
   temp << i;
   if (name != NULL) delete [] name;
   name = new char[temp.str().length()+1];
   strcpy(name, temp.str().c_str());
   return name;
}



//////////////////////////////
//
// MidiOutPort_unsupported::getNumPorts -- returns the number of available
// 	ports for MIDI output
//

int MidiOutPort_unsupported::getNumPorts(void) const {
   return numDevices;
}



//////////////////////////////
//
// MidiOutPort_unsupported::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiOutPort_unsupported::getPort(void) const {
   return port;
}



//////////////////////////////
//
// MidiOutPort_unsupported::getPortStatus -- 0 if closed, 1 if open
//

int MidiOutPort_unsupported::getPortStatus(void) const {
   if (openQ[getPort()] == 1) {
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiOutPort_unsupported::getTrace -- returns true if trace is on or
//      false if off.  If trace is on, then prints to standard output
//      the Midi message being sent.
//

int MidiOutPort_unsupported::getTrace(void) const {
   return trace;
}



//////////////////////////////
//
// MidiOutPort_unsupported::rawsend -- send the Midi command and its parameters
//

int MidiOutPort_unsupported::rawsend(int command, int p1, int p2) {
   if (getTrace()) {
      std::cout << "{" << std::hex << command << std::dec << ":" << (p1 & 0xff) 
           << "," << (p2 & 0xff) << "}";
      std::cout.flush();
   }

   return 1;
}


int MidiOutPort_unsupported::rawsend(int command, int p1) {
   return 1;
}


int MidiOutPort_unsupported::rawsend(int command) {
   return 1;
}


int MidiOutPort_unsupported::rawsend(uchar* array, int size) {
   return 1;
}



//////////////////////////////
//
// MidiOutPort_unsupported::open -- returns true if MIDI output port was
//	opened.
//

int MidiOutPort_unsupported::open(void) {
   if (getPortStatus() == 0) {
      openQ[getPort()] = 1;
   }
   return 1;
}



//////////////////////////////
//
// MidiOutPort_unsupported::setChannelOffset -- sets the MIDI channel offset, either 0 or 1.
//

void MidiOutPort_unsupported::setChannelOffset(int anOffset) {
   switch (anOffset) {
      case 0:   channelOffset = 0;   break;
      case 1:   channelOffset = 1;   break;
      default:
         std::cout << "Error:  Channel offset can be only 0 or 1." << std::endl;
         exit(1);
   }
}



//////////////////////////////
//
// MidiOutPort_unsupported::setPort --
//

void MidiOutPort_unsupported::setPort(int aPort) {
   if (aPort < 0 || aPort >= getNumPorts()) {
      std::cerr << "Error: maximum port number is: " << getNumPorts()-1
           << ", but you tried to access port: " << aPort << std::endl;
      exit(1);
   }

   if (port != -1) {
      portObjectCount[port]--;
   }
   port = aPort;
   portObjectCount[port]++;
}



//////////////////////////////
//
// MidiOutPort_unsupported::setTrace -- if false, then won't print
//	Midi messages to standard output.
//

int MidiOutPort_unsupported::setTrace(int aState) {
   int oldtrace = trace;
   if (aState == 0) {
      trace = 0;
   } else {
      trace = 1;
   }
   return oldtrace;
}



//////////////////////////////
//
// MidiOutPort_unsupported::sysex -- 
//

int MidiOutPort_unsupported::sysex(uchar* array, int size) {
   return 1;
}



//////////////////////////////
//
// MidiOutPort_unsupported::toggleTrace --
//

void MidiOutPort_unsupported::toggleTrace(void) {
   trace = !trace;
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiOutPort_unsupported::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_unsupported::deinitialize(void) {
   closeAll();
   if (openQ != NULL) delete [] openQ;
   openQ = NULL;
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = NULL;
}



//////////////////////////////
//
// MidiOutPort_unsupported::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_unsupported::initialize(void) {
   // get the number of ports
   numDevices = 16;
   if  (getNumPorts() <= 0) {
      std::cerr << "Error: no MIDI output devices" << std::endl;
      exit(1);
   }


   // allocate space for openQ, the port open/close status
   if (openQ != NULL) delete [] openQ;
   openQ = new int[numDevices];

   // allocate space for object count on each port:
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = new int[numDevices];


   // initialize the static arrays
   for (int i=0; i<getNumPorts(); i++) {
      openQ[i] = 0;
      portObjectCount[i] = 0;
   }
}



//////////////////////////////
//
// MidiOutPort_unsupported::setPortStatus(int aStatus) {
//

void MidiOutPort_unsupported::setPortStatus(int aStatus) {
   if (aStatus) {
      openQ[getPort()] = 1;
   } else {
      openQ[getPort()] = 0;
   }
}




// md5sum:	eff3d6cd2cab4c2def6ca60ef0ca197f  - MidiOutPort_unsupported.cpp =css= 20030102
