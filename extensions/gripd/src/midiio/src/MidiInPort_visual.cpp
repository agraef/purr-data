//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Jan 21 22:46:30 GMT-0800 1998
// Last Modified: Wed Jun 30 11:29:51 PDT 1999 (added sysex capability)
// Last Modified: Wed Oct 13 10:18:22 PDT 1999 (midiInUnprepareHeader change)
// Last Modified: Tue Nov 23 15:01:17 PST 1999 (fixed sysex NULL init)
// Filename:      ...sig/code/control/MidiInPort/visual/MidiInPort_visual.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/MidiInPort_visual.cpp
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of
//                Windows 95/NT/98 specific MIDI input methods.
//                as defined in winmm.lib.  This class is inherited 
//                privately by the MidiInPort class.
//

#ifdef VISUAL


#include "MidiInPort_visual.h"

#include <windows.h>
#include <mmsystem.h>

#include <iostream.h>


// initialized static variables
int       MidiInPort_visual::numDevices         = 0;
int       MidiInPort_visual::objectCount        = 0;
int*      MidiInPort_visual::openQ              = NULL;
int*      MidiInPort_visual::inrunningQ         = NULL;
int*      MidiInPort_visual::portObjectCount    = NULL;
HMIDIIN*  MidiInPort_visual::device             = NULL;
MIDIHDR** MidiInPort_visual::sysexDriverBuffer1 = NULL; 
MIDIHDR** MidiInPort_visual::sysexDriverBuffer2 = NULL; 
int*      MidiInPort_visual::sysexDBnumber      = NULL;
HANDLE*   MidiInPort_visual::hMutex             = NULL;
CircularBuffer<MidiMessage>* MidiInPort_visual::midiBuffer = NULL;
int       MidiInPort_visual::channelOffset      = 0;
int*      MidiInPort_visual::sysexWriteBuffer   = NULL;
Array<uchar>** MidiInPort_visual::sysexBuffers  = NULL;
int*      MidiInPort_visual::sysexStatus        = NULL;  


//////////////////////////////
// 
// MidiInPort_visual::MidiInPort_visual
//	default values: autoOpen = 1
//

MidiInPort_visual::MidiInPort_visual(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   trace = 0;
   port = -1;
   setPort(0);
}


MidiInPort_visual::MidiInPort_visual(int aPort, int autoOpen) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   trace = 0;
   port = -1;
   setPort(aPort);
   if (autoOpen) {
      open();
   }
}



//////////////////////////////
//
// MidiInPort_visual::~MidiInPort_visual
//

MidiInPort_visual::~MidiInPort_visual() {
   objectCount--;
   if (objectCount == 0) {
      deinitialize();
   } else if (objectCount < 0) {
      cerr << "Error: bad MidiInPort_visual object count!: " 
           << objectCount << endl;
      exit(1);
   }
}



//////////////////////////////
//
// MidiInPort_visual::clearSysex -- clears the data from a sysex
//      message and sets the allocation size to the default size (of 32
//      bytes).
//

void MidiInPort_visual::clearSysex(int buffer) {
   buffer = 0x7f | buffer;    // limit buffer range from 0 to 127
   
   if (getPort() == -1) {
      return;
   }

   sysexBuffers[getPort()][buffer].setSize(0);
   if (sysexBuffers[getPort()][buffer].getAllocSize() != 32) {
      // shrink the storage buffer's size if necessary
      sysexBuffers[getPort()][buffer].setAllocSize(32);
   }
}


void MidiInPort_visual::clearSysex(void) {
   // clear all sysex buffers
   for (int i=0; i<128; i++) {
      clearSysex(i);
   }
}



//////////////////////////////
//
// MidiInPort_visual::close
//

void MidiInPort_visual::close(void) {
   if (getPort() == -1) {
      return;
   }
   if (getPortStatus() == 1 && device[getPort()] != NULL) {
      midiInReset(device[getPort()]);
      midiInClose(device[getPort()]);
      uninstallSysexStuff(device[getPort()], port);
      openQ[getPort()] = 0;
      inrunningQ[getPort()] = 0;
   }
}



//////////////////////////////
//
// MidiInPort_visual::closeAll
//

void MidiInPort_visual::closeAll(void) {
   for (int i=0; i<getNumPorts(); i++) {
      if (openQ[i] == 1 && device[i] != NULL) {
         midiInReset(device[i]);
         midiInClose(device[i]);
         if (getPort() != 1) {
            uninstallSysexStuff(device[getPort()], port);
         }
         openQ[i] = 0;
         inrunningQ[i] = 0;
      }
   }
}



//////////////////////////////
//
// MidiInPort_visual::extract -- returns the next MIDI message
//	received since that last extracted message.
//

MidiMessage MidiInPort_visual::extract(void) {
   MidiMessage output;
   
   waitForMutex();
   if (getPort() != -1) {
      output = midiBuffer[getPort()].extract();
   }
   releaseMutex();

   return output;
}



//////////////////////////////
//
// MidiInPort_visual::getBufferSize
//

int MidiInPort_visual::getBufferSize(void) {
   if (getPort() != -1) {
      return midiBuffer[getPort()].getSize();
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiInPort_visual::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiInPort_visual::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiInPort_visual::getCount -- returns the number of unexamined
//	MIDI messages waiting in the input buffer.
//

int MidiInPort_visual::getCount(void) {
   int output;
  
   waitForMutex();
   if (getPort() != -1) {
      output = midiBuffer[getPort()].getCount();
   } else {
      output = 0;
   }
   releaseMutex();

   return output;
}



//////////////////////////////
//
// MidiInPort_visual::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until getName is called again.
//

const char* MidiInPort_visual::getName(void) {
   static MIDIINCAPS inputCapabilities;

   if (getPort() == -1) {
      return "Null MIDI Input";
   }

   if (openQ[getPort()]) {  // port already open
      midiInGetDevCaps(getPort(), &inputCapabilities, sizeof(MIDIINCAPS));
   } else {  // port is currently closed
      if(open()) {;
         midiInGetDevCaps(getPort(), &inputCapabilities, sizeof(MIDIINCAPS));
         close();
      } else {
         return "";
      }
   }
   return inputCapabilities.szPname;
}


const char* MidiInPort_visual::getName(int i) {
   static MIDIINCAPS inputCapabilities;

   if (i < 0 || i > getNumPorts()) {
      cerr << "Error invalid index for getName: " << i << endl;
      exit(1);
   }

   midiInGetDevCaps(i, &inputCapabilities, sizeof(MIDIINCAPS));

   return inputCapabilities.szPname;
}



//////////////////////////////
//
// MidiInPort_visual::getNumPorts -- returns the number of available
// 	ports for MIDI input
//

int MidiInPort_visual::getNumPorts(void) {
   return midiInGetNumDevs();
}



//////////////////////////////
//
// MidiInPort_visual::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiInPort_visual::getPort(void) {
   return port;
}



//////////////////////////////
//
// MidiInPort_visual::getPortStatus -- 0 if closed, 1 if open, 2 if
//    specifically not connected to any MIDI port.
//

int MidiInPort_visual::getPortStatus(void) {
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
// MidiInPort_visual::getSysex -- returns the sysex message contents
//    of a given buffer.  You should check to see that the size is
//    non-zero before looking at the data.  The data pointer will
//    be NULL if there is no data in the buffer.
//

uchar* MidiInPort_visual::getSysex(int buffer) {
   buffer &= 0x7f;     // limit the buffer access to indices 0 to 127.

   if (getPort() == -1) {
      return NULL;
   }

   if (sysexBuffers[getPort()][buffer].getSize() < 2) {
      return NULL;
   } else {
      return sysexBuffers[getPort()][buffer].getBase();
   }
}



//////////////////////////////
//
// MidiInPort_visual::getSysexSize -- returns the sysex message byte
//    count of a given buffer.   Buffers are in the range from 
//    0 to 127.
//

int MidiInPort_visual::getSysexSize(int buffer) {
   if (getPort() == -1) {
      return 0;
   } else {
      return sysexBuffers[getPort()][buffer & 0x7f].getSize();
   }
}



//////////////////////////////
//
// MidiInPort_visual::getTrace -- returns true if trace is on or false
//	if trace is off.  if trace is on, then prints to standard
// 	output the Midi message received.
//

int MidiInPort_visual::getTrace(void) {
   return trace;
}



//////////////////////////////
//
// MidiInPort_visual::insert
//

void MidiInPort_visual::insert(const MidiMessage& aMessage) {
   waitForMutex();
   if (getPort() == -1) {
      // do nothing
   } else {
      midiBuffer[getPort()].insert(aMessage);
   }
   releaseMutex();
}



//////////////////////////////
//
// MidiInPort_visual::installSysex -- put a sysex message into a
//      buffer.  The buffer number that it is put into is returned.
//

int MidiInPort_visual::installSysex(uchar* anArray, int aSize) {
   return installSysexPrivate(getPort(), anArray, aSize);
}



//////////////////////////////
//
// MidiInPort_visual::installSysexPrivate -- put a sysex message into a
//      buffer.  The buffer number that it is put into is returned.
//

int MidiInPort_visual::installSysexPrivate(int port, uchar* anArray, int aSize){
   if (port == -1) {
      return -1;
   }

   // choose a buffer to install sysex data into:
   int bufferNumber = sysexWriteBuffer[port];
   sysexWriteBuffer[port]++;
   if (sysexWriteBuffer[port] >= 128) {
      sysexWriteBuffer[port] = 0;
   }

   // copy contents of sysex message into the chosen buffer
   sysexBuffers[port][bufferNumber].setSize(aSize);
   uchar* dataptr = sysexBuffers[port][bufferNumber].getBase();
   uchar* indataptr = anArray;
   for (int i=0; i<aSize; i++) { 
      *dataptr = *indataptr;
      dataptr++;
      indataptr++;
   }

   // return the buffer number that was used
   return bufferNumber;
}



//////////////////////////////
//
// MidiInPort_visual::message
//

MidiMessage& MidiInPort_visual::message(int index) {
   if (getPort() != -1) {
      return midiBuffer[getPort()][index];
   } else {
      static MidiMessage nullmessage;
      return nullmessage;
   }
}



//////////////////////////////
//
// MidiInPort_visual::open -- returns true if MIDI input port was
//	opened.
//

int MidiInPort_visual::open(void) {
   if (getPort() == -1) {
      return 1;
   }

   if (getPortStatus() == 0) {
      int flag;
      flag = midiInOpen(&device[getPort()], getPort(), 
         (DWORD)&midiInputCallback, (DWORD)this, CALLBACK_FUNCTION);
      if (flag == MMSYSERR_NOERROR) {
         openQ[getPort()] = 1;
         installSysexStuff(device[getPort()], port);
         unpause();
         return 1;
      } else { // failed to open
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
// MidiInPort_visual::pause -- stop the Midi input port from
//	inserting MIDI messages into the buffer, but keep the
//	port open.  Use unpause() to reverse the effect of pause().
//

void MidiInPort_visual::pause(void) {
   if (getPort() == -1) {
      return;
   }

   if (openQ[getPort()]) {
      if (inrunningQ[getPort()] == 1) {
         midiInStop(device[getPort()]);
         inrunningQ[getPort()] = 0;
      }
   }
}



//////////////////////////////
//
// MidiInPort_visual::setBufferSize -- sets the allocation
//	size of the MIDI input buffer.
//

void MidiInPort_visual::setBufferSize(int aSize) {
   if (getPort() != -1) {
      midiBuffer[getPort()].setSize(aSize);
   }
}



//////////////////////////////
//
// MidiInPort_visual::setChannelOffset -- sets the MIDI channel offset, either 0 or 1.
//

void MidiInPort_visual::setChannelOffset(int anOffset) {
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
// MidiInPort_visual::setPort --
//

void MidiInPort_visual::setPort(int aPort) {
   if (aPort < -1 || aPort >= getNumPorts()) {
      cerr << "Error: maximum port number is: " << getNumPorts()-1
           << ", but you tried to access port: " << aPort << endl;
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
// MidiInPort_visual::setTrace -- if false, then don't print MIDI messages
// 	to the screen.
//

int MidiInPort_visual::setTrace(int aState) {
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
// MidiInPort_visual::toggleTrace -- switches the state of trace
//	Returns the previous value of the trace variable.
//

void MidiInPort_visual::toggleTrace(void) {
   trace = !trace;
}
   


//////////////////////////////
//
// MidiInPort_visual::unpause -- enables the Midi input port 
//	to inserting MIDI messages into the buffer after the 
//	port is already open.
//

void MidiInPort_visual::unpause(void) {
   if (getPort() == -1) {
      return;
   }

   if (openQ[getPort()]) {
      if (inrunningQ[getPort()] == 0) {
         midiInStart(device[getPort()]);
         inrunningQ[getPort()] = 1;
      }
   }
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiInPort_visual::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_visual::deinitialize(void) {
   int num = numDevices;

   closeAll();

   if (sysexBuffers != NULL) {
      for (int i=0; i<numDevices; i++) {
         if (sysexBuffers[i] != NULL) {
            delete sysexBuffers[i];
            sysexBuffers[i] = NULL;
         }
      }
      delete [] sysexBuffers;
      sysexBuffers = NULL;
   }


   if (sysexDriverBuffer1 != NULL) {
      for (int i=0; i<numDevices; i++) {
         if (sysexDriverBuffer1[i] != NULL) {
            delete sysexDriverBuffer1[i];
            sysexDriverBuffer1[i] = NULL;
         }
      }
      delete [] sysexDriverBuffer1;
      sysexDriverBuffer1 = NULL;
   }

   if (sysexDriverBuffer2 != NULL) {
      for (int i=0; i<numDevices; i++) {
         if (sysexDriverBuffer2[i] != NULL) {
            delete sysexDriverBuffer2[i];
            sysexDriverBuffer2[i] = NULL;
         }
      }
      delete [] sysexDriverBuffer2;
      sysexDriverBuffer2 = NULL;
   }

   if (sysexDBnumber != NULL) {
      delete [] sysexDBnumber;
      sysexDBnumber = NULL;
   }


   if (sysexWriteBuffer != NULL) {
      delete [] sysexWriteBuffer;
      sysexWriteBuffer = NULL;
   }

   if (sysexStatus != NULL) {
      delete [] sysexStatus;
      sysexStatus = NULL;
   }

   if (device != NULL) {
      delete [] device;
      device = NULL;
   }

   if (openQ != NULL) {
      delete [] openQ;
      openQ = NULL;
   }

   if (inrunningQ != NULL) {
      delete [] inrunningQ;
      inrunningQ = NULL;
   }

   if (hMutex != NULL) {
      delete [] hMutex;
      hMutex = NULL;
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
// MidiInPort_visual::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_visual::initialize(void) {
   int i;

   // get the number of ports
   numDevices = midiInGetNumDevs();
   if  (getNumPorts() <= 0) {
      cerr << "Error: no MIDI input devices" << endl;
      exit(1);
   }

   // allocate space for Windoze MIDI input structures
   if (device != NULL) {
      cerr << "Error: device array should be NULL when calling "
           << "initialize() in MidiInPort_visual." << endl;
      exit(1);
   }
   device = new HMIDIIN[numDevices];

   // allocate space for openQ, the port open/close status
   if (openQ != NULL) delete [] openQ;
   openQ = new int[numDevices];

   // allocate space to keep track of an active/inactive input port
   if (inrunningQ != NULL) delete [] inrunningQ;
   inrunningQ = new int[numDevices];

   // allocate space for object count on each port:
   if (portObjectCount != NULL) delete [] portObjectCount;
   portObjectCount = new int[numDevices];

   // allocate space for mutual exclusive
   if (hMutex != NULL) delete [] hMutex;
   hMutex = new HANDLE[numDevices];

   // allocate space for the Midi input buffers
   if (midiBuffer != NULL) delete [] midiBuffer;
   midiBuffer = new CircularBuffer<MidiMessage>[numDevices];

   // allocate space for the MIDI sysex buffer indices
   if (sysexWriteBuffer != NULL) delete [] sysexWriteBuffer;
   sysexWriteBuffer = new int[numDevices];

   // allocate space for the sysex MIM_LONGDATA message tracking
   if (sysexStatus != NULL) delete [] sysexStatus;
   sysexStatus = new int[numDevices];

   // allocate space for sysex buffers
   if (sysexBuffers != NULL) {
      cout << "Error: memory leak on sysex buffers initialization" << endl;
      exit(1);
   }
   sysexBuffers = new Array<uchar>*[numDevices];

   // allocate system exclusive buffers for MIDI driver
   if (sysexDriverBuffer1 != NULL) {
      cout << "Error: memory leak on sysex buffer for drivers creation" << endl;
      exit(1);
   }
   sysexDriverBuffer1 = new MIDIHDR*[numDevices];   
   for (i=0; i<numDevices; i++) {
      sysexDriverBuffer1[i] = NULL;
   }

   // allocate system exclusive buffers for MIDI driver
   if (sysexDriverBuffer2 != NULL) {
      cout << "Error: memory leak on sysex buffer for drivers creation" << endl;
      exit(1);
   }
   sysexDriverBuffer2 = new MIDIHDR*[numDevices];   
   for (i=0; i<numDevices; i++) {
      sysexDriverBuffer2[i] = NULL;
   }

   // allocate space for keeping track of which buffer to look at
   if (sysexDBnumber != NULL) delete [] sysexDBnumber;
   sysexDBnumber = new int[numDevices];


   // initialize the static arrays
   for (i=0; i<getNumPorts(); i++) {
      device[i] = NULL;
      openQ[i] = 0;
      inrunningQ[i] = 0;
      portObjectCount[i] = 0;
      hMutex[i] = CreateMutex(NULL, FALSE, "M");
      midiBuffer[i].setSize(DEFAULT_INPUT_BUFFER_SIZE);

      sysexStatus[i] = -1;
      sysexWriteBuffer[i] = 0;
      sysexDBnumber[i] = 0;
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
// MidiInPort_visual::installSysexStuff -- install all the mess that 
//   a MIDIIN structure needs in order to receive system exclusives.
//

void MidiInPort_visual::installSysexStuff(HMIDIIN dev, int port) {
   if (sysexDriverBuffer1 != NULL) {
      if (sysexDriverBuffer1[port] != NULL) {
         // some memory leaks here
         delete sysexDriverBuffer1[port];
         sysexDriverBuffer1[port] = NULL;
      }
   }
   if (sysexDriverBuffer2 != NULL) {
      if (sysexDriverBuffer2[port] != NULL) {
         // some memory leaks here
         delete sysexDriverBuffer2[port];
         sysexDriverBuffer2[port] = NULL;
      }
   }

   // allocate space for Drivers sysex byte buffer
   sysexDriverBuffer1[port] = (LPMIDIHDR)GlobalLock(GlobalAlloc(GMEM_MOVEABLE |
          GMEM_SHARE, sizeof(MIDIHDR)));
   sysexDriverBuffer2[port] = (LPMIDIHDR)GlobalLock(GlobalAlloc(GMEM_MOVEABLE |
          GMEM_SHARE, sizeof(MIDIHDR)));

   if (sysexDriverBuffer1[port] == NULL) {
      cout << "Error: could not allocate sysex driver's buffer" << endl;
      exit(1);
   }

   if (sysexDriverBuffer2[port] == NULL) {
      cout << "Error: could not allocate sysex driver's buffer" << endl;
      exit(1);
   }

   // allocate buffer inside of sysexDriverBuffer
   sysexDriverBuffer1[port]->lpData = (LPSTR)GlobalLock(GlobalAlloc(
         GMEM_MOVEABLE | GMEM_SHARE, (DWORD)1024));
   sysexDriverBuffer2[port]->lpData = (LPSTR)GlobalLock(GlobalAlloc(
         GMEM_MOVEABLE | GMEM_SHARE, (DWORD)1024));

   if (sysexDriverBuffer1[port]->lpData == NULL) {
      cout << "Error: there was not enought space to allocate sysex buffer" 
           << endl;
      // leaking memory here
      exit(1);
   }

   if (sysexDriverBuffer2[port]->lpData == NULL) {
      cout << "Error: there was not enought space to allocate sysex buffer" 
           << endl;
      // leaking memory here
      exit(1);
   }


   // setup other sysexDriverBuffer data fields
   sysexDriverBuffer1[port]->dwBufferLength = 1024; // total size of buffer
   sysexDriverBuffer1[port]->dwBytesRecorded = 0L;  // number of byte in buffer
   sysexDriverBuffer1[port]->dwFlags = 0L;          // initialize flags
   sysexDriverBuffer1[port]->dwUser = 0L;       // userdata: used for sysex time

   // setup other sysexDriverBuffer data fields
   sysexDriverBuffer2[port]->dwBufferLength = 1024; // total size of buffer
   sysexDriverBuffer2[port]->dwBytesRecorded = 0L;  // number of byte in buffer
   sysexDriverBuffer2[port]->dwFlags = 0L;          // initialize flags
   sysexDriverBuffer2[port]->dwUser = 0L;       // userdata: used for sysex time

   // prepare the header
   int status = midiInPrepareHeader(device[port], sysexDriverBuffer1[port], 
         sizeof(MIDIHDR));

   if (status != 0) {
      cout << "Error preparing sysex buffer number: " << port << endl;  
      // leaking some memory here?
      exit(1);
   }

   // prepare the header
   status = midiInPrepareHeader(device[port], sysexDriverBuffer2[port], 
         sizeof(MIDIHDR));

   if (status != 0) {
      cout << "Error preparing sysex buffer number: " << port << endl;  
      // leaking some memory here?
      exit(1);
   }

   // add the sysex buffer to the driver
   status = midiInAddBuffer(device[port], sysexDriverBuffer1[port], 
        sizeof(MIDIHDR));

   if (status != 0) {
      cout << "Error adding sysex buffer to driver: " << port << endl;
      // leaking some memory here?
      exit(1);
   }

   status = midiInAddBuffer(device[port], sysexDriverBuffer2[port], 
        sizeof(MIDIHDR));

   if (status != 0) {
      cout << "Error adding sysex buffer to driver: " << port << endl;
      // leaking some memory here?
      exit(1);
   }

}



//////////////////////////////
//
// MidiInPort_visual::uninstallSysexStuff -- uninstalls all the mess that 
//   a MIDIIN structure needs in order to receive system exclusives.
//

void MidiInPort_visual::uninstallSysexStuff(HMIDIIN dev, int port) {
   if (port == -1) { 
      return;
   }
   // unprepare the headers
   midiInUnprepareHeader(device[port], sysexDriverBuffer1[port], 
         sizeof(MIDIHDR));
   midiInUnprepareHeader(device[port], sysexDriverBuffer2[port], 
         sizeof(MIDIHDR));

   // deallocate buffer inside of sysexDriverBuffer
   /* Following code caused problems: perhaps lpData was deleted by driver
   delete [] sysexDriverBuffer1[port]->lpData;
   sysexDriverBuffer1[port]->lpData = NULL;
   delete [] sysexDriverBuffer2[port]->lpData;
   sysexDriverBuffer2[port]->lpData = NULL;
   */

   // deallocate space for Drivers sysex byte buffer
   delete sysexDriverBuffer1[port];
   delete sysexDriverBuffer2[port];
   sysexDriverBuffer1[port] = NULL;
   sysexDriverBuffer2[port] = NULL;
}



//////////////////////////////
//
// MidiInPort_visual::releaseMutex
//

void MidiInPort_visual::releaseMutex(void) {
/*   int flag = */ ReleaseMutex(hMutex[getPort()]);
/* 
   if (flag != 0) {
      cerr << "Error relasing mutex in MIDI input; flag was: " << flag << endl;
   }
*/
}



//////////////////////////////
//
// MidiInPort_visual::setPortStatus
//

void MidiInPort_visual::setPortStatus(int aStatus) {
   if (getPort() == -1) {
      return;
   }

   if (aStatus) {
      openQ[getPort()] = 1;
   } else {
      openQ[getPort()] = 0;
   }
}



//////////////////////////////
//
// MidiInPort_visual::waitForMutex 
//

void MidiInPort_visual::waitForMutex(void) {
/*
   DWORD mutexResult = WaitForSingleObject(hMutex[getPort()], 5000L);
   if (mutexResult != WAIT_OBJECT_0) {
      cerr << "Error waiting for mutex in MIDI input" << endl;
   }
*/
}



///////////////////////////////////////////////////////////////////////////


//////////////////////////////
//
// midiInputCallback -- the function the MIDI input driver calls when
// 	it has a message from the Midi in cable ready
//

void CALLBACK midiInputCallback(HMIDIIN hMidiIn, UINT inputStatus,
      DWORD instancePtr, DWORD midiMessage, DWORD timestamp) {
   static MidiMessage newMessage;
   
   switch (inputStatus) {
      case MIM_MOREDATA:
         // There is more data waiting at the device.
         // If this case is exists, then that means that the MIDI
         // device is too slow and some data was lost.
         // Windows sends a MIM_MOREDATA event only if you specify
         // the MIDI_IO_STATUS flag to midiInOpen().

         // no break;
      case MIM_DATA:

         // One regular (non sysex) message has been completely
         // received.
  
         // ignore the Yamaha Active Sensing command and MIDI time clock
         // at least for now.
         if ((midiMessage & 0xff) == 0xfe || (midiMessage & 0xff) == 0xf8) {
            break;
         }

         newMessage.time = timestamp;
         newMessage.data = midiMessage;
         ((MidiInPort_visual*)instancePtr)->insert(newMessage);
         if (((MidiInPort_visual*)instancePtr)->getTrace()) {
            cout << "[" << hex << (int)newMessage.command() << dec
                 << ":" << (int)newMessage.p1() << ","
                 << (int)newMessage.p2() << "]";
            cout.flush();
         }
         break;

      case MIM_LONGDATA:
         {
         // A sysex or part of a sysex message is coming in.
         // The timestamp variable contains a pointer to a 
         // MIDIHDR pointer

         MIDIHDR* midiheader = (MIDIHDR*)midiMessage;
         int dataCount = midiheader->dwBytesRecorded;
         char* data = midiheader->lpData;
         int port = ((MidiInPort_visual*)instancePtr)->getPort();
         if (port == -1) {
            break;
         }
         int* sysexStatus = ((MidiInPort_visual*)instancePtr)->sysexStatus;
//         MIDIHDR** sysexDriverBuffer = ((MidiInPort_visual*)instancePtr)->
//               sysexDriverBuffer;
         HMIDIIN devicex = ((MidiInPort_visual*)instancePtr)->device[port];

         if (dataCount == 0) {
            // can't handle a zero-length sysex
            break;
         }

         // step 1: determine if this is the first part of the sysex
         // message or a continuation
         int continuation = 0;
         if (data[0] == (char)0xf0) {
            continuation = 0;
            if (sysexStatus[port] != -1) {
               cout << "Error: there is another sysex command being "
                       "received on port " << port << endl;
               exit(1);
            }
         } else {
            if (sysexStatus[port] == -1) {
               cout << "Error: no sysex command is being "
                       "received on port " << port << endl;
               if (data[0] < 128) {
                  cout << "First byte is: " << dec << (int)data[0] << endl;
               } else { 
                  cout << "First byte is: " << hex << (int)data[0] << endl;
               }
               if (data[1] < 128) {
                  cout << "Second byte is: " << dec << (int)data[1] << endl;
               } else { 
                  cout << "Second byte is: " << hex << (int)data[1] << endl;
               }
                                                                           
               exit(1);
            }
            continuation = 1;
         }

         // step 2: if continuing, add the data to the preallocated
         // sysex buffer, otherwise, get a new buffer location
         int buffer = -1;
         if (continuation) {
            buffer = sysexStatus[port];
            if (buffer < 0 || buffer > 127) {
               cout << "Sysex buffer was out of range: " << buffer << endl;
            }
            for (int i=0; i<dataCount; i++) {
               unsigned char datum = data[i];
               ((MidiInPort_visual*)instancePtr)->
                     sysexBuffers[port][buffer].append(datum);
               if (datum == 0xf7) {
                  for (int k=i; k<dataCount; k++) {
                     data[k-i] = data[k];
                  }
                  midiheader->dwBytesRecorded = dataCount - i - 1;

                  goto insert_sysex_message;
               }
            }
         } else { // if not a continuation of a sysex event
            buffer = ((MidiInPort_visual*)instancePtr)->sysexWriteBuffer[port];
            ((MidiInPort_visual*)instancePtr)->sysexWriteBuffer[port]++;
            if (buffer == 127) {
               ((MidiInPort_visual*)instancePtr)->sysexWriteBuffer[port] = 0;
            }

            ((MidiInPort_visual*)instancePtr)->
                  sysexBuffers[port][buffer].setSize(0);
            for (int j=0; j<dataCount; j++) {
               unsigned char datum = data[j];
               ((MidiInPort_visual*)instancePtr)->
                     sysexBuffers[port][buffer].append(datum);
               if (datum == 0xf7) {
                  for (int k=j; k<dataCount; k++) {
                     data[k-j] = data[k];
                  }

                  goto insert_sysex_message;
               }
            }

         }

         // recycle the MIDI input buffer for the driver
         {
         midiInPrepareHeader(devicex, midiheader, sizeof(MIDIHDR));
         int dstatus = midiInAddBuffer(devicex, midiheader, sizeof(MIDIHDR));
         if (dstatus != MMSYSERR_NOERROR) {
            cout << "Error when calling midiInAddBuffer" << endl;
            exit(1);
         }
         }

         break;

         insert_sysex_message:

         // recycle the MIDI input buffer for the driver
         {
         midiInPrepareHeader(devicex, midiheader, sizeof(MIDIHDR));
         int estatus = midiInAddBuffer(devicex, midiheader, sizeof(MIDIHDR));
         if (estatus != MMSYSERR_NOERROR) {
            cout << "Error when calling midiInAddBuffer" << endl;
            exit(1);
         }
         }

         // now that a sysex message is finished, send a midimessage
         // out to the instancePtr MIDI buffer telling the user
         // that a sysex message has come in.

         // newMessage.time = timestamp; use last time stamp that came
         // in because the timestamp variable is used for storing
         // the pointer of the sysex data.

         newMessage.time = timestamp;
         newMessage.p0() = 0xf0;
         newMessage.p1() = buffer;
         newMessage.p2() = 0;
         newMessage.p3() = 0;

         ((MidiInPort_visual*)instancePtr)->insert(newMessage);
         if (((MidiInPort_visual*)instancePtr)->getTrace()) {
            cout << "[" << hex << (int)newMessage.command() << dec
                 << ":" << (int)newMessage.p1() << ","
                 << (int)newMessage.p2() << "]";
            cout.flush();
         }

         } // end of local variable range
         break;

      case MIM_ERROR:
         // An invalid regular MIDI message was received.
  
         break;

      case MIM_LONGERROR:
         {
         // An invalid sysex MIDI message was received.
  
         // if a sysex message was continuing from a previous part,
         // then kill that message.
  
         int port = ((MidiInPort_visual*)instancePtr)->getPort();
         if (port == -1) {
            break;
         }
         int buffer = ((MidiInPort_visual*)instancePtr)->sysexStatus[port];
         if (buffer != -1) {
            ((MidiInPort_visual*)instancePtr)->
                  sysexBuffers[port][buffer].setSize(0);
            ((MidiInPort_visual*)instancePtr)->sysexStatus[port] = -1;
         }

         HMIDIIN devicex = ((MidiInPort_visual*)instancePtr)->device[port];
         MIDIHDR* midiheader = (MIDIHDR*)midiMessage;

         // recycle the MIDI input buffer for the driver
         midiInPrepareHeader(devicex, midiheader, sizeof(MIDIHDR));
         int status = midiInAddBuffer(devicex, midiheader, sizeof(MIDIHDR));
         if (status != MMSYSERR_NOERROR) {
            cout << "Error when calling midiInAddBuffer" << endl;
            exit(1);
         }


         break;
         }
      default: ;
      // case MIM_OPEN:    // MIDI device is opening
      // case MIM_CLOSE:   // MIDI device is closing
   }
}



#endif  // VISUAL


// md5sum:	db55d9f375b86f54c0c8340547c5701f  - MidiInPort_visual.cpp =css= 20030102
