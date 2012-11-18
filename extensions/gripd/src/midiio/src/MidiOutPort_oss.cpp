//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Dec 18 19:22:20 PST 1998
// Last Modified: Fri Jan  8 04:26:16 PST 1999
// Last Modified: Wed May 10 17:00:11 PDT 2000 (name change from _linux to _oss)
// Filename:      ...sig/code/control/MidiOutPort/linux/MidiOutPort_oss.cpp
// Web Address:   http://sig.sapp.org/src/sig/MidiOutPort_oss.cpp
// Syntax:        C++ 
//
// Description:   Operating-System specific interface for
//                basic MIDI output capabilities in Linux using
//                OSS sound drivers.  Privately inherited by the
//                MidiOutPort class.
// 

#ifdef LINUX

#include "MidiOutPort_oss.h"
#include <iostream>
#include <stdlib.h>

// initialized static variables
int       MidiOutPort_oss::numDevices      = 0;
int       MidiOutPort_oss::objectCount     = 0;
int*      MidiOutPort_oss::portObjectCount = NULL;
int       MidiOutPort_oss::channelOffset   = 0;
int*      MidiOutPort_oss::trace           = NULL;
std::ostream*  MidiOutPort_oss::tracedisplay    = &std::cout;



//////////////////////////////
// 
// MidiOutPort_oss::MidiOutPort_oss
//	default values: autoOpen = 1
//


MidiOutPort_oss::MidiOutPort_oss(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(0);
}


MidiOutPort_oss::MidiOutPort_oss(int aPort, int autoOpen) {
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
// MidiOutPort_oss::~MidiOutPort_oss
//

MidiOutPort_oss::~MidiOutPort_oss() {
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
// MidiOutPort_oss::close
//

void MidiOutPort_oss::close(void) {
   // don't close anything, because the 
   // Linux driver keeps all of the ports open while the
   // midi driver (/dev/sequencer) is running.
}



//////////////////////////////
//
// MidiOutPort_oss::closeAll
//

void MidiOutPort_oss::closeAll(void) {
   // the Linux MIDI driver will close the /dev/sequencer device
   // which will close all MIDI output ports at the same time.
   Sequencer_oss::close();
}



//////////////////////////////
//
// MidiOutPort_oss::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiOutPort_oss::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiOutPort_oss::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until getName is called again.
//

const char* MidiOutPort_oss::getName(void) {
   if (getPort() == -1) { 
      return "Null OSS MIDI Output";
   }
   return getOutputName(getPort());
}


const char* MidiOutPort_oss::getName(int i) {
   return Sequencer_oss::getOutputName(i);
}



//////////////////////////////
//
// MidiOutPort_oss::getNumPorts -- returns the number of available
// 	ports for MIDI output
//

int MidiOutPort_oss::getNumPorts(void) {
   return Sequencer_oss::getNumOutputs();
}



//////////////////////////////
//
// MidiOutPort_oss::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiOutPort_oss::getPort(void) {
   return port;
}



//////////////////////////////
//
// MidiOutPort_oss::getPortStatus -- 0 if closed, 1 if open
//

int MidiOutPort_oss::getPortStatus(void) {
   // Linux MIDI devices are all open at the same time,
   // so if one is open, then they all are.
   return is_open();
}



//////////////////////////////
//
// MidiOutPort_oss::getTrace -- returns true if trace is on or
//	false if off.  If trace is on, then prints to standard output
//	the Midi message being sent.
//

int MidiOutPort_oss::getTrace(void) {
   if (getPort() == -1) return -1;

   return trace[getPort()];
}



//////////////////////////////
//
// MidiOutPort_oss::rawsend -- send the Midi command and its parameters
//

int MidiOutPort_oss::rawsend(int command, int p1, int p2) {
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


int MidiOutPort_oss::rawsend(int command, int p1) {
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


int MidiOutPort_oss::rawsend(int command) {
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


int MidiOutPort_oss::rawsend(uchar* array, int size) {
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
// MidiOutPort_oss::open -- returns true if MIDI output port was
//	opened.
//

int MidiOutPort_oss::open(void) {
   return Sequencer_oss::open();
}



//////////////////////////////
//
// MidiOutPort_oss::setChannelOffset -- sets the MIDI channel offset, 
//     either 0 or 1.
//

void MidiOutPort_oss::setChannelOffset(int anOffset) {
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
// MidiOutPort_oss::setPort --
//

void MidiOutPort_oss::setPort(int aPort) {
   if (aPort < -1 || aPort >= getNumPorts()) {
      std::cerr << "Error: maximum port number is: " << getNumPorts()-1
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
// MidiOutPort_oss::setTrace -- if false, then won't print
//      Midi messages to standard output.
//

int MidiOutPort_oss::setTrace(int aState) {
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
// MidiOutPort_oss::sysex -- send a system exclusive message.
//     The message must start with a 0xf0 byte and end with
//     a 0xf7 byte.
//

int MidiOutPort_oss::sysex(uchar* array, int size) {
   if (size == 0 || array[0] != 0xf0) {
      std::cout << "Error: invalid sysex message" << std::endl;
      exit(1);
   }

   return rawsend(array,size);
}



//////////////////////////////
//
// MidiOutPort_oss::toggleTrace
//

void MidiOutPort_oss::toggleTrace(void) {
   if (getPort() == -1) return;

   trace[getPort()] = !trace[getPort()];
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiOutPort_oss::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_oss::deinitialize(void) {
   closeAll();
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = NULL;
   if (trace != NULL) delete [] trace;
   trace = NULL;
}



//////////////////////////////
//
// MidiOutPort_oss::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_oss::initialize(void) {
   // get the number of ports
   numDevices = getNumOutputs();
   if  (getNumPorts() <= 0) {
      std::cerr << "Warning: no MIDI output devices" << std::endl;
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
// MidiOutPort_oss::setPortStatus
//

void MidiOutPort_oss::setPortStatus(int aStatus) {
   // not used in Linux implementation
}


#endif  // LINUX



// md5sum:	c09dbe18ce8a0ff6ff11030d43a98c4a  - MidiOutPort_oss.cpp =css= 20030102
