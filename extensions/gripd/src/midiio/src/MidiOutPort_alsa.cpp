//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed May 10 16:16:21 PDT 2000
// Last Modified: Sun May 14 20:44:12 PDT 2000
// Filename:      ...sig/code/control/MidiOutPort/alsa/MidiOutPort_alsa.cpp
// Web Address:   http://sig.sapp.org/src/sig/MidiOutPort_alsa.cpp
// Syntax:        C++ 
//
// Description:   Operating-System specific interface for
//                basic MIDI output capabilities in Linux using
//                ALSA sound drivers.  Privately inherited by the
//                MidiOutPort class via the MidiOutPort_alsa class.
// 

#if defined(LINUX) && defined(ALSA)

#include "MidiOutPort_alsa.h"
#include <iostream>
#include <stdlib.h>

// initialized static variables
int       MidiOutPort_alsa::numDevices      = 0;
int       MidiOutPort_alsa::objectCount     = 0;
int*      MidiOutPort_alsa::portObjectCount = NULL;
int       MidiOutPort_alsa::channelOffset   = 0;
int*      MidiOutPort_alsa::trace           = NULL;
std::ostream*  MidiOutPort_alsa::tracedisplay    = &std::cout;



//////////////////////////////
// 
// MidiOutPort_alsa::MidiOutPort_alsa
//	default values: autoOpen = 1
//

#include <unistd.h>

MidiOutPort_alsa::MidiOutPort_alsa(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(0);
}


MidiOutPort_alsa::MidiOutPort_alsa(int aPort, int autoOpen) {
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
// MidiOutPort_alsa::~MidiOutPort_alsa --
//

MidiOutPort_alsa::~MidiOutPort_alsa() {
   objectCount--;
   if (objectCount == 0) {
      deinitialize();
   } else if (objectCount < 0) {
      std::cout << "Error: bad MidiOutPort object count!: " << objectCount << std::endl;
      exit(1);
   }
}



//////////////////////////////
//
// MidiOutPort_alsa::close --
//

void MidiOutPort_alsa::close(void) {
   Sequencer_alsa::closeOutput(getPort());
}



//////////////////////////////
//
// MidiOutPort_alsa::closeAll --
//

void MidiOutPort_alsa::closeAll(void) {
   int i;
   for (i=0; i<getNumPorts(); i++) {
      Sequencer_alsa::closeOutput(i);
   }
}



//////////////////////////////
//
// MidiOutPort_alsa::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiOutPort_alsa::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiOutPort_alsa::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until getName is called again.
//

const char* MidiOutPort_alsa::getName(void) {
   if (getPort() == -1) { 
      return "Null ALSA Midi Output";
   }
   return getOutputName(getPort());
}


const char* MidiOutPort_alsa::getName(int i) {
   return Sequencer_alsa::getOutputName(i);
}



//////////////////////////////
//
// MidiOutPort_alsa::getNumPorts -- returns the number of available
// 	ports for MIDI output
//

int MidiOutPort_alsa::getNumPorts(void) {
   return Sequencer_alsa::getNumOutputs();
}



//////////////////////////////
//
// MidiOutPort_alsa::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiOutPort_alsa::getPort(void) {
   return port;
}



//////////////////////////////
//
// MidiOutPort_alsa::getPortStatus -- 0 if closed, 1 if open
//

int MidiOutPort_alsa::getPortStatus(void) {
   return is_open_out(getPort());
}



//////////////////////////////
//
// MidiOutPort_alsa::getTrace -- returns true if trace is on or
//	false if off.  If trace is on, then prints to standard output
//	the Midi message being sent.
//

int MidiOutPort_alsa::getTrace(void) {
   if (getPort() == -1) return -1;

   return trace[getPort()];
}



//////////////////////////////
//
// MidiOutPort_alsa::rawsend -- send the Midi command and its parameters
//

int MidiOutPort_alsa::rawsend(int command, int p1, int p2) {
   if (getPort() == -1) return 0;

   int status;
   uchar mdata[3] = {(uchar)command, (uchar)p1, (uchar)p2};
   status = write(getPort(), mdata, 3);   

   if (getTrace()) {
      if (status == 1) {
         std::cout << "(" << std::hex << (int)mdata[0] << std::dec << ":"
              << (int)mdata[1] << "," << (int)mdata[2] << ")";
         std::cout.flush();
      } else {
         std::cout << "(" << std::hex << (int)mdata[0] << std::dec << "X"
              << (int)mdata[1] << "," << (int)mdata[2] << ")";
         std::cout.flush();
      }
   }      

   return status;
}


int MidiOutPort_alsa::rawsend(int command, int p1) {
   if (getPort() == -1) return 0;   

   int status;
   uchar mdata[2] = {(uchar)command, (uchar)p1};

   status = write(getPort(), mdata, 2);   

   if (getTrace()) {
      if (status == 1) {
         std::cout << "(" << std::hex << (int)mdata[0] << std::dec << ":"
              << (int)mdata[1] << ")";
         std::cout.flush();
      } else {
         std::cout << "(" << std::hex << (int)mdata[0] << std::dec << "X"
              << (int)mdata[1] << "," << (int)mdata[2] << ")";
         std::cout.flush();
      }
   }
 
   return status;
}


int MidiOutPort_alsa::rawsend(int command) {
   if (getPort() == -1) return 0;   

   int status;
   uchar mdata[1] = {(uchar)command};

   status = write(getPort(), mdata, 1);

   if (getTrace()) {
      if (status == 1) {
         std::cout << "(" << std::hex << (int)mdata[0] << ")";
         std::cout.flush();
      } else {
         std::cout << "(" << std::hex << (int)mdata[0] << ")";
         std::cout.flush();
      }
   }

   return status;
}


int MidiOutPort_alsa::rawsend(uchar* array, int size) {
   if (getPort() == -1) return 0;   

   int status;
   status = write(getPort(), array, size);
   
   if (getTrace()) {
      if (status == 1) {
         std::cout << "(array)";
         std::cout.flush();
      } else {
         std::cout << "(XarrayX)";
         std::cout.flush();
      }
   }

   return status;
}



//////////////////////////////
//
// MidiOutPort_alsa::open -- returns true if MIDI output port was
//	opened.
//

int MidiOutPort_alsa::open(void) {
   if (getPort() == -1) {
      return 2;
   } else {
      return Sequencer_alsa::openOutput(getPort());
   }
}



//////////////////////////////
//
// MidiOutPort_alsa::setChannelOffset -- sets the MIDI channel offset, 
//     either 0 or 1.
//

void MidiOutPort_alsa::setChannelOffset(int anOffset) {
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
// MidiOutPort_alsa::setPort --
//

void MidiOutPort_alsa::setPort(int aPort) {
   if (aPort == -1) return;
 
   if (aPort < 0 || aPort >= getNumPorts()) {
      std::cout << "Error: maximum port number is: " << getNumPorts()-1
           << ", but you tried to access port: " << aPort << std::endl;
      exit(1);
   }

   if (port != -1) {
      portObjectCount[port]--;
   }
   port = aPort;
   if (port != -1) {
      portObjectCount[port]++;
   }
}



//////////////////////////////
//
// MidiOutPort_alsa::setTrace -- if false, then won't print
//      Midi messages to standard output.
//

int MidiOutPort_alsa::setTrace(int aState) {
   if (getPort() == -1) return -1;

   int oldtrace = trace[getPort()];
   if (aState == 0) {
      trace[getPort()] = 0;
   } else {
      trace[getPort()] = 1;
   }
   return oldtrace;
}



//////////////////////////////
//
// MidiOutPort_alsa::sysex -- send a system exclusive message.
//     The message must start with a 0xf0 byte and end with
//     a 0xf7 byte.
//

int MidiOutPort_alsa::sysex(uchar* array, int size) {
   if (getPort() == -1) {
      return 2;
   }

   if (size == 0 || array[0] != 0xf0 || array[size-1] != 0xf7) {
      std::cout << "Error: invalid sysex message" << std::endl;
      exit(1);
   }

   return rawsend(array,size);
}



//////////////////////////////
//
// MidiOutPort_alsa::toggleTrace --
//

void MidiOutPort_alsa::toggleTrace(void) {
   if (getPort() == -1) return;

   trace[getPort()] = !trace[getPort()];
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiOutPort_alsa::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_alsa::deinitialize(void) {
   closeAll();
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = NULL;
   if (trace != NULL) delete [] trace;
   trace = NULL;
}



//////////////////////////////
//
// MidiOutPort_alsa::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_alsa::initialize(void) {
   // get the number of ports
   numDevices = getNumOutputs();
   if  (getNumPorts() <= 0) {
      std::cout << "Warning: no MIDI output devices" << std::endl;
      portObjectCount = NULL;
      trace = NULL;
   } else {
      // allocate space for object count on each port:
      if (portObjectCount != NULL) delete [] portObjectCount;
      portObjectCount = new int[numDevices];
   
      // allocate space for trace variable for each port:
      if (trace != NULL) delete [] trace;
      trace = new int[numDevices];
   
      // initialize the static arrays
      for (int i=0; i<getNumPorts(); i++) {
         portObjectCount[i] = 0;
         trace[i] = 0;
      }
   }
}



//////////////////////////////
//
// MidiOutPort_alsa::setPortStatus --
//

void MidiOutPort_alsa::setPortStatus(int aStatus) {
   // not used in Linux implementation
}


#endif  /* LINUX and ALSA */



// md5sum:	6f5d0ce75ae6e086ad949178e7deefcb  - MidiOutPort_alsa.cpp =css= 20030102
