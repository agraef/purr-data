//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Dec 28 15:18:46 GMT-0800 1997
// Last Modified: Mon Jan 12 15:42:44 GMT-0800 1998
// Last Modified: Tue Jun 29 13:10:30 PDT 1999 (verified sysex sending)
// Last Modified: Tue Jun  4 22:10:16 PDT 2002 (getNumPorts fix for static use)
// Filename:      ...sig/code/control/MidiOutPort/visual/MidiOutPort_visual.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/MidiOutPort_visual.cpp
// Syntax:        C++ 
//
// Description:   Operating-System specific interface for
//                basic MIDI output capabilities in Windows 95/NT/98
//                using winmm.lib.  Privately inherited by the
//                MidiOutPort class.
// 


#ifdef VISUAL

#include <iostream.h>
#include "MidiOutPort_visual.h"

typedef unsigned long ulong;
typedef unsigned char uchar;


// initialized static variables
int       MidiOutPort_visual::numDevices      = 0;
int       MidiOutPort_visual::objectCount     = 0;
int*      MidiOutPort_visual::openQ           = NULL;
int*      MidiOutPort_visual::portObjectCount = NULL;
HMIDIOUT* MidiOutPort_visual::device          = NULL;
int       MidiOutPort_visual::channelOffset   = 0;



//////////////////////////////
// 
// MidiOutPort_visual::MidiOutPort_visual
//	default values: autoOpen = 1
//


MidiOutPort_visual::MidiOutPort_visual(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(0);
}


MidiOutPort_visual::MidiOutPort_visual(int aPort, int autoOpen) {
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
// MidiOutPort_visual::~MidiOutPort_visual
//

MidiOutPort_visual::~MidiOutPort_visual() {
   objectCount--;
   if (objectCount == 0) {
      deinitialize();
   } else if (objectCount < 0) {
      cerr << "Error: bad MidiOutPort object count!: " << objectCount << endl;
      exit(1);
   }
}



//////////////////////////////
//
// MidiOutPort_visual::close
//

void MidiOutPort_visual::close(void) {
   if (getPort() == -1) {
      return;
   }

   if (getPortStatus() == 1 && device[getPort()] != NULL) {

      // The following function, midiOutClose, is not what I like.
      // It will send note offs to any note which it thinks is
      // on when the port is closed.  Uncomment the line if
      // you want this feature.
      // midiOutReset(device[getPort()]);

      midiOutClose(device[getPort()]);
      setPortStatus(0);
   }
}



//////////////////////////////
//
// MidiOutPort_visual::closeAll
//

void MidiOutPort_visual::closeAll(void) {
   for (int i=0; i<getNumPorts(); i++) {
      if (openQ[i] == 1 && device[i] != NULL) {
         midiOutReset(device[i]);
         midiOutClose(device[i]);
         openQ[i] = 0;
      }
   }
}



//////////////////////////////
//
// MidiOutPort_visual::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiOutPort_visual::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiOutPort_visual::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until getName is called again.
//

const char* MidiOutPort_visual::getName(void) {
   static MIDIOUTCAPS outputCapabilities;

   if (getPort() == -1) {
      return "Null MIDI Output";
   }

   if (openQ[getPort()]) {  // port already open
      midiOutGetDevCaps(getPort(), &outputCapabilities, sizeof(MIDIOUTCAPS));
   } else {  // port is currently closed
      if(open()) {;
         midiOutGetDevCaps(getPort(), &outputCapabilities, sizeof(MIDIOUTCAPS));
         close();
      } else {
         return "";
      }
   }
   return outputCapabilities.szPname;
}


const char* MidiOutPort_visual::getName(int i) {
   static MIDIOUTCAPS outputCapabilities;

   midiOutGetDevCaps(i, &outputCapabilities, sizeof(MIDIOUTCAPS));
  
   return outputCapabilities.szPname;
}



//////////////////////////////
//
// MidiOutPort_visual::getNumPorts -- returns the number of available
// 	ports for MIDI output
//

int MidiOutPort_visual::getNumPorts(void) {
   if (numDevices <= 0) {
      return midiOutGetNumDevs();
   }
   return numDevices;
}



//////////////////////////////
//
// MidiOutPort_visual::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiOutPort_visual::getPort(void) {
   return port;
}



//////////////////////////////
//
// MidiOutPort_visual::getPortStatus -- 0 if closed, 1 if open
//   2 if null connection
//

int MidiOutPort_visual::getPortStatus(void) {
   if (getPort() == -1) {
      return 2;
   }

   if (openQ[getPort()] == 1) {
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiOutPort_visual::getTrace -- returns true if trace is on or
//	false if off.  If trace is on, then prints to standard output
//	the Midi message being sent.
//

int MidiOutPort_visual::getTrace(void) {
   return trace;
}



//////////////////////////////
//
// MidiOutPort_visual::rawsend -- send the Midi command and its parameters
//

int MidiOutPort_visual::rawsend(int command, int p1, int p2) {
   union { ulong word; uchar data[4]; } u;
   u.data[0] = (uchar)command;
   u.data[1] = (uchar)(p1 & 0x7f);  // parameter limited to range 0-127;
   u.data[2] = (uchar)(p2 & 0x7f);  // parameter limited to range 0-127;
   u.data[3] = 0;
  
   if (getPort() == -1) {
      return 2;
   }

   int flag = midiOutShortMsg(device[getPort()], u.word);
   
   if (getTrace()) {
      if (flag == MMSYSERR_NOERROR) {
         cout << "(" << hex << (int)u.data[0] << dec << ":"
              << (int)u.data[1] << "," << (int)u.data[2] << ")";
         cout.flush();
      } else {
         cout << "(" << hex << (int)u.data[0] << dec << "X"
              << (int)u.data[1] << "," << (int)u.data[2] << ")";
         cout.flush();
      }
   }

   return flag;
}


int MidiOutPort_visual::rawsend(int command, int p1) {
   return rawsend(command, p1, 0);
}


int MidiOutPort_visual::rawsend(int command) {
   return rawsend(command, 0, 0);
}


int MidiOutPort_visual::rawsend(uchar* array, int size) {
   // Note: this function will work in Windows 95 and Windows NT.
   // This function will not work in Windows 3.x because a 
   // different memory model is necessary.

   if (size > 64000 || size < 1) {
      cerr << "Warning: cannot write a MIDI stream larger than 64kB" << endl;
      return 0;
   }

   MIDIHDR midiheader;   // structure for sending an array of MIDI bytes

   midiheader.lpData = (char *)array;
   midiheader.dwBufferLength = size;
   // midiheader.dwBytesRecorded = size;  // example program doesn't set
   midiheader.dwFlags = 0;                // flags must be set to 0

   if (getPort() == -1) {
      return -1;
   }

   int status = midiOutPrepareHeader(device[getPort()], &midiheader,
      sizeof(MIDIHDR));

   if (status != MMSYSERR_NOERROR) {
      return 0;
   }

   status = midiOutLongMsg(device[getPort()], &midiheader, sizeof(MIDIHDR));

   if (status != MMSYSERR_NOERROR) {
      return 0;
   }

   while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader(device[getPort()], 
         &midiheader, sizeof(MIDIHDR))) {
      Sleep(1);                           // sleep for 1 millisecond
   }

   return 1;
}



//////////////////////////////
//
// MidiOutPort_visual::open -- returns true if MIDI output port was
//	opened.
//

int MidiOutPort_visual::open(void) {
   if (getPort() == -1) {
      return 2;
   }

   if (getPortStatus() == 0) {
      int flag;
      flag = midiOutOpen(&device[getPort()], getPort(), 0, 0, CALLBACK_NULL);
      if (flag == MMSYSERR_NOERROR) {
         openQ[getPort()] = 1;
         return 1;
      } else { // faied to open
         openQ[getPort()] = 0;
         device[getPort()] = NULL;
         return 0;
      }
   } else { // already open
      return 1;
   }
}



//////////////////////////////
//
// MidiOutPort_visual::setChannelOffset -- sets the MIDI channel offset, 
//     either 0 or 1.
//

void MidiOutPort_visual::setChannelOffset(int anOffset) {
   switch (anOffset) {
      case 0:   channelOffset = 0;   break;
      case 1:   channelOffset = 1;   break;
      default:
         cout << "Error:  Channel offset can be only 0 or 1." << endl;
         exit(1);
   }
}



//////////////////////////////
//
// MidiOutPort_visual::setPort
//

void MidiOutPort_visual::setPort(int aPort) {
   if (aPort < 0 || aPort >= getNumPorts()) {
      cerr << "Error: maximum port number is: " << getNumPorts()-1
           << ", but you tried to access port: " << aPort << endl;
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
// MidiOutPort_visual::setTrace -- if false, then won't print
//      Midi messages to standard output.
//

int MidiOutPort_visual::setTrace(int aState) {
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
// MidiOutPort_visual::sysex -- send a system exclusive message.
//    The first byte of the message must be a 0xf0 byte.
//

int MidiOutPort_visual::sysex(uchar* array, int size) {
   if (size == 0 || array[0] != 0xf0) {
      cout << "Error: invalid system exclusive message,"
              " first byte must be 0xf0" << endl;
      exit(1);
   }

   return rawsend(array, size);
}



//////////////////////////////
//
// MidiOutPort_visual::toggleTrace
//

void MidiOutPort_visual::toggleTrace(void) {
   trace = !trace;
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiOutPort_visual::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_visual::deinitialize(void) {
   closeAll();
   if (device != NULL) delete [] device;
   device = NULL;
   if (openQ != NULL) delete [] openQ;
   openQ = NULL;
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = NULL;
}



//////////////////////////////
//
// MidiOutPort_visual::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiOutPort_visual::initialize(void) {
   // get the number of ports
   numDevices = midiOutGetNumDevs();
   if  (getNumPorts() <= 0) {
      cerr << "Error: no MIDI output devices" << endl;
      exit(1);
   }

   // allocate space for Windoze MIDI output structures
   if (device != NULL) {
      cerr << "Error: device array should be NULL when calling "
           << "initialize() in MidiOutPort." << endl;
      exit(1);
   }
   device = new HMIDIOUT[numDevices];

   // allocate space for openQ, the port open/close status
   if (openQ != NULL) delete [] openQ;
   openQ = new int[numDevices];

   // allocate space for object count on each port:
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = new int[numDevices];


   // initialize the static arrays
   for (int i=0; i<getNumPorts(); i++) {
      device[i] = NULL;
      openQ[i] = 0;
      portObjectCount[i] = 0;
   }
}



//////////////////////////////
//
// MidiOutPort_visual::setPortStatus
//

void MidiOutPort_visual::setPortStatus(int aStatus) {
   if (getPort() == -1) {
      return;
   }

   if (aStatus) {
      openQ[getPort()] = 1;
   } else {
      openQ[getPort()] = 0;
   }
}


#endif  // VISUAL




// md5sum:	8cb60bfb5dc9ea42808ffa4540e0fc52  - MidiOutPort_visual.cpp =css= 20030102
