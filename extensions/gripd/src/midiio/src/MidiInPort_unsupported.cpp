//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Jan 21 22:46:30 GMT-0800 1998
// Last Modified: Thu Jan 22 22:53:53 GMT-0800 1998
// Last Modified: Wed Jun 30 11:42:59 PDT 1999 (added sysex capability)
// Filename:      ...sig/code/control/MidiInPort/unsupported/MidiInPort_unsupported.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/MidiInPort_unsupported.cpp
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of
//                an unknown sound driver's specific MIDI input methods.
//                This class is inherited privately by the MidiInPort class.
//                This class is used when there is no MIDI input, so
//                that MIDI programs can otherwise be compiled and run.
//                This file can also serve as a template for creating
//                an OS specific class for MIDI input.
//

#include "MidiInPort_unsupported.h"
#include <iostream>
#include <stdlib.h>


#define DEFAULT_INPUT_BUFFER_SIZE (1024)

// initialized static variables
int       MidiInPort_unsupported::numDevices        = 0;
int       MidiInPort_unsupported::objectCount       = 0;
int*      MidiInPort_unsupported::openQ             = NULL;
int*      MidiInPort_unsupported::portObjectCount   = NULL;
CircularBuffer<MidiMessage>* MidiInPort_unsupported::midiBuffer = NULL;
int       MidiInPort_unsupported::channelOffset     = 0;
int*      MidiInPort_unsupported::sysexWriteBuffer  = NULL;
Array<uchar>** MidiInPort_unsupported::sysexBuffers = NULL; 


//////////////////////////////
// 
// MidiInPort_unsupported::MidiInPort_unsupported
//	default values: autoOpen = 1
//

MidiInPort_unsupported::MidiInPort_unsupported(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(0);
}


MidiInPort_unsupported::MidiInPort_unsupported(int aPort, int autoOpen) {
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
// MidiInPort_unsupported::~MidiInPort_unsupported
//

MidiInPort_unsupported::~MidiInPort_unsupported() {
   objectCount--;
   if (objectCount == 0) {
      deinitialize();
   } else if (objectCount < 0) {
      std::cerr << "Error: bad MidiInPort_unsupported object count!: " 
           << objectCount << std::endl;
      exit(1);
   }
}



//////////////////////////////
//
// MidiInPort_unsupported::close
//

void MidiInPort_unsupported::close(void) {
   if (getPortStatus() == 1) {
      openQ[getPort()] = 0;
   }
}



//////////////////////////////
//
// MidiInPort_unsupported::closeAll
//

void MidiInPort_unsupported::closeAll(void) {
   for (int i=0; i<getNumPorts(); i++) {
      if (openQ[i] == 1) {
         openQ[i] = 0;
      }
   }
}



//////////////////////////////
//
// MidiInPort_unsupported::extract -- returns the next MIDI message
//	received since that last extracted message.
//

MidiMessage MidiInPort_unsupported::extract(void) {
   return midiBuffer[getPort()].extract();
}



//////////////////////////////
//
// MidiInPort_unsupported::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiInPort_unsupported::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiInPort_unsupported::getCount -- returns the number of unexamined
//	MIDI messages waiting in the input buffer.
//

int MidiInPort_unsupported::getCount(void) {
   return midiBuffer[getPort()].getCount();
}



//////////////////////////////
//
// MidiInPort_unsupported::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until getName is called again.
//

const char* MidiInPort_unsupported::getName(void) {
   return "none";
}

const char* MidiInPort_unsupported::getName(int i) {
   return "none";
}



//////////////////////////////
//
// MidiInPort_unsupported::getNumPorts -- returns the number of available
// 	ports for MIDI input
//

int MidiInPort_unsupported::getNumPorts(void) {
   return numDevices;
}



//////////////////////////////
//
// MidiInPort_unsupported::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiInPort_unsupported::getPort(void) {
   return port;
}



//////////////////////////////
//
// MidiInPort_unsupported::getPortStatus -- 0 if closed, 1 if open
//

int MidiInPort_unsupported::getPortStatus(void) {
   if (openQ[getPort()] == 1) {
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiInPort_unsupported::getTrace -- returns true if trace is on or false
//	if trace is off.  if trace is on, then prints to standard
// 	output the Midi message received.
//

int MidiInPort_unsupported::getTrace(void) {
   return trace;
}



//////////////////////////////
//
// MidiInPort_unsupported::insert
//

void MidiInPort_unsupported::insert(const MidiMessage& aMessage) {
   midiBuffer[getPort()].insert(aMessage);
}



//////////////////////////////
//
// MidiInPort_unsupported::message
//

MidiMessage& MidiInPort_unsupported::message(int index) {
   return midiBuffer[getPort()][index];
}



//////////////////////////////
//
// MidiInPort_unsupported::open -- returns true if MIDI input port was
//	opened.
//

int MidiInPort_unsupported::open(void) {
   if (getPortStatus() == 0) {
      openQ[getPort()] = 1;
   }
   return openQ[getPort()];
}



//////////////////////////////
//
// MidiInPort_unsupported::pause -- stop the Midi input port from
//	inserting MIDI messages into the buffer, but keep the
//	port open.  Use unpause() to reverse the effect of pause().
//

void MidiInPort_unsupported::pause(void) {
   // nothing
}



//////////////////////////////
//
// MidiInPort_unsupported::setBufferSize -- sets the allocation
//	size of the MIDI input buffer.
//

void MidiInPort_unsupported::setBufferSize(int aSize) {
   midiBuffer[getPort()].setSize(aSize);
}



//////////////////////////////
//
// MidiInPort_unsupported::setChannelOffset -- sets the MIDI channel offset, either 0 or 1.
//

void MidiInPort_unsupported::setChannelOffset(int anOffset) {
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
// MidiInPort_unsupported::setPort
//

void MidiInPort_unsupported::setPort(int aPort) {
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
// MidiInPort_unsupported::setTrace -- if false, then don't print MIDI messages
// 	to the screen.
//

int MidiInPort_unsupported::setTrace(int aState) {
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
// MidiInPort_unsupported::toggleTrace -- switches the state of trace
//	Returns the previous value of the trace variable.
//

void MidiInPort_unsupported::toggleTrace(void) {
   trace = !trace;
}
   


//////////////////////////////
//
// MidiInPort_unsupported::unpause -- enables the Midi input port 
//	to inserting MIDI messages into the buffer after the 
//	port is already open.
//

void MidiInPort_unsupported::unpause(void) {
   // nothing
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiInPort_unsupported::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_unsupported::deinitialize(void) {
   int deviceCount = numDevices;
   closeAll();

   if (sysexBuffers != NULL) {
      for (int i=0; i<deviceCount; i++) {
         if (sysexBuffers[i] != NULL) {
            delete [] sysexBuffers[i];
            sysexBuffers[i] = NULL;
         }
      }
      delete [] sysexBuffers;
      sysexBuffers = NULL;
   }

   if (sysexWriteBuffer != NULL) {
      delete [] sysexWriteBuffer;
      sysexWriteBuffer = NULL;
   }

   if (openQ != NULL) {
      delete [] openQ;
      openQ = NULL;
   }

   if (midiBuffer != NULL) {
      delete [] midiBuffer;
      midiBuffer = NULL;
   }

   if (portObjectCount != NULL) {
      delete [] portObjectCount;
      portObjectCount = NULL;
   }
}



//////////////////////////////
//
// MidiInPort_unsupported::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_unsupported::initialize(void) {
   // get the number of ports
   numDevices = 9;
   if  (getNumPorts() <= 0) {
      std::cerr << "Error: no MIDI input devices" << std::endl;
      exit(1);
   }


   // allocate space for sysexBuffers, the port open/close status
   if (sysexBuffers != NULL) {
      std::cout << "Error: sysexBuffers are not empty, don't know size" << std::endl;
      exit(1);
   }
   sysexBuffers = new Array<uchar>*[numDevices];

   // allocate space for sysexWriteBuffer, the port open/close status
   if (sysexWriteBuffer != NULL) delete [] sysexWriteBuffer;
   sysexWriteBuffer = new int[numDevices];

   // allocate space for openQ, the port open/close status
   if (openQ != NULL) delete [] openQ;
   openQ = new int[numDevices];

   // allocate space for object count on each port:
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = new int[numDevices];

   // allocate space for the Midi input buffers
   if (midiBuffer != NULL) delete [] midiBuffer;
   midiBuffer = new CircularBuffer<MidiMessage>[numDevices];

   // initialize the static arrays
   for (int i=0; i<getNumPorts(); i++) {
      openQ[i] = 0;
      portObjectCount[i] = 0;
      midiBuffer[i].setSize(DEFAULT_INPUT_BUFFER_SIZE);

      sysexWriteBuffer[i] = 0;
      sysexBuffers[i] = new Array<uchar>[128];
      for (int n=0; n<128; n++) {
         sysexBuffers[i][n].allowGrowth(0);      // shouldn't need to grow
         sysexBuffers[i][n].setAllocSize(32);
         sysexBuffers[i][n].setSize(0);
         sysexBuffers[i][n].setGrowth(32);       // in case it will ever grow
      }

   }
}



//////////////////////////////
//
// MidiInPort_unsupported::setPortStatus
//

void MidiInPort_unsupported::setPortStatus(int aStatus) {
   if (aStatus) {
      openQ[getPort()] = 1;
   } else {
      openQ[getPort()] = 0;
   }
}


// md5sum:	d8b8f65af70a9b3c33e62794c2a4a91e  - MidiInPort_unsupported.cpp =css= 20030102
