//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Jan 21 22:46:30 GMT-0800 1998
// Last Modified: Sat Jan  9 06:05:24 PST 1999
// Last Modified: Tue Jun 29 16:18:45 PDT 1999 (added sysex capability)
// Last Modified: Mon Dec  6 16:28:45 PST 1999 
// Last Modified: Wed Jan 12 10:59:33 PST 2000 (orphan 0xf0 behavior change)
// Last Modified: Wed May 10 17:10:05 PDT 2000 (name change from _linux to _oss)
// Last Modified: Fri Oct 26 14:41:36 PDT 2001 (running status for 0xa0 and 0xd0 
//                                              fixed by Daniel Gardner)
// Filename:      ...sig/code/control/MidiInPort/linux/MidiInPort_oss.cpp
// Web Address:   http://sig.sapp.org/src/sig/MidiInPort_oss.cpp
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of
//                linux OSS sound driver's specific MIDI input methods.
//                This class is inherited privately by the MidiInPort class.
//

#ifdef LINUX

#include "MidiInPort_oss.h"
#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <linux/soundcard.h>


#define DEFAULT_INPUT_BUFFER_SIZE (1024)

// initialized static variables

int       MidiInPort_oss::numDevices                     = 0;
int       MidiInPort_oss::objectCount                    = 0;
int*      MidiInPort_oss::portObjectCount                = NULL;
CircularBuffer<MidiMessage>** MidiInPort_oss::midiBuffer = NULL;
int       MidiInPort_oss::channelOffset                  = 0;
SigTimer  MidiInPort_oss::midiTimer;
int*      MidiInPort_oss::pauseQ                         = NULL;
int*      MidiInPort_oss::trace                          = NULL;
std::ostream*  MidiInPort_oss::tracedisplay                   = &std::cout;
pthread_t MidiInPort_oss::midiInThread;    
int*      MidiInPort_oss::sysexWriteBuffer               = NULL;
Array<uchar>** MidiInPort_oss::sysexBuffers              = NULL;



//////////////////////////////
// 
// MidiInPort_oss::MidiInPort_oss
//	default values: autoOpen = 1
//

MidiInPort_oss::MidiInPort_oss(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(0);
}


MidiInPort_oss::MidiInPort_oss(int aPort, int autoOpen) {
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
// MidiInPort_oss::~MidiInPort_oss
//

MidiInPort_oss::~MidiInPort_oss() {
   objectCount--;
   if (objectCount == 0) {
      deinitialize();
   } else if (objectCount < 0) {
      std::cerr << "Error: bad MidiInPort_oss object count!: " 
           << objectCount << std::endl;
      exit(1);
   }
}



//////////////////////////////
//
// MidiInPort_oss::clearSysex -- clears the data from a sysex
//      message and sets the allocation size to the default size (of 32
//      bytes).
//

void MidiInPort_oss::clearSysex(int buffer) {
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


void MidiInPort_oss::clearSysex(void) {
   // clear all sysex buffers
   for (int i=0; i<128; i++) {
      clearSysex(i);
   }
}



//////////////////////////////
//
// MidiInPort_oss::close
//

void MidiInPort_oss::close(void) {
   if (getPort() == -1) return;

   pauseQ[getPort()] = 1;
}



//////////////////////////////
//
// MidiInPort_oss::closeAll --
//

void MidiInPort_oss::closeAll(void) {
   Sequencer_oss::close();
}



//////////////////////////////
//
// MidiInPort_oss::extract -- returns the next MIDI message
//	received since that last extracted message.
//

MidiMessage MidiInPort_oss::extract(void) {
   if (getPort() == -1) {
      MidiMessage temp;
      return temp;
   }

   return midiBuffer[getPort()]->extract();
}



//////////////////////////////
//
// MidiInPort_oss::getBufferSize -- returns the maximum possible number
//	of MIDI messages that can be stored in the buffer
//

int MidiInPort_oss::getBufferSize(void) {
   if (getPort() == -1)   return 0;

   return midiBuffer[getPort()]->getSize();
}



//////////////////////////////
//
// MidiInPort_oss::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiInPort_oss::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiInPort_oss::getCount -- returns the number of unexamined
//	MIDI messages waiting in the input buffer.
//

int MidiInPort_oss::getCount(void) {
   if (getPort() == -1)   return 0;

   return midiBuffer[getPort()]->getCount();
}



//////////////////////////////
//
// MidiInPort_oss::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until all instances
//      of MIDI classes are.
//

const char* MidiInPort_oss::getName(void) {
   if (getPort() == -1) {
      return "Null OSS MIDI Input";
   }
   return getInputName(getPort());
}


const char* MidiInPort_oss::getName(int i) {
   return getInputName(i);
}



//////////////////////////////
//
// MidiInPort_oss::getNumPorts -- returns the number of available
// 	ports for MIDI input
//

int MidiInPort_oss::getNumPorts(void) {
   return getNumInputs();
}



//////////////////////////////
//
// MidiInPort_oss::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiInPort_oss::getPort(void) {
   return port;
}



//////////////////////////////
//
// MidiInPort_oss::getPortStatus -- 0 if closed, 1 if open
//

int MidiInPort_oss::getPortStatus(void) {
   return is_open();
}



//////////////////////////////
//
// MidiInPort_oss::getSysex -- returns the sysex message contents
//    of a given buffer.  You should check to see that the size is
//    non-zero before looking at the data.  The data pointer will
//    be NULL if there is no data in the buffer.
//

uchar* MidiInPort_oss::getSysex(int buffer) {
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
// MidiInPort_oss::getSysexSize -- returns the sysex message byte
//    count of a given buffer.   Buffers are in the range from 
//    0 to 127.
//

int MidiInPort_oss::getSysexSize(int buffer) {
   if (getPort() == -1) {
      return 0;
   } else {
      return sysexBuffers[getPort()][buffer & 0x7f].getSize();
   }
}



//////////////////////////////
//
// MidiInPort_oss::getTrace -- returns true if trace is on or false
//	if trace is off.  if trace is on, then prints to standard
// 	output the Midi message received.
//

int MidiInPort_oss::getTrace(void) {
   if (getPort() == -1)   return -1;

   return trace[getPort()];
}



//////////////////////////////
//
// MidiInPort_oss::insert
//

void MidiInPort_oss::insert(const MidiMessage& aMessage) {
   if (getPort() == -1)   return;

   midiBuffer[getPort()]->insert(aMessage);
}



//////////////////////////////
//
// MidiInPort_oss::installSysex -- put a sysex message into a
//      buffer.  The buffer number that it is put into is returned.
//

int MidiInPort_oss::installSysex(uchar* anArray, int aSize) {
   if (getPort() == -1) {
      return -1;
   } else {
      return installSysexPrivate(getPort(), anArray, aSize);
   }
}

//////////////////////////////
//
// MidiInPort_oss::installSysexPrivate -- put a sysex message into a
//      buffer.  The buffer number that it is put into is returned.
//

int MidiInPort_oss::installSysexPrivate(int port, uchar* anArray, int aSize) {
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
// MidiInPort_oss::message
//

MidiMessage& MidiInPort_oss::message(int index) {
   if (getPort() == -1) {
      static MidiMessage x;
      return x;
   }

   CircularBuffer<MidiMessage>& temp = *midiBuffer[getPort()];
   return temp[index];
}



//////////////////////////////
//
// MidiInPort_oss::open -- returns true if MIDI input port was
//	opened.
//

int MidiInPort_oss::open(void) {
   if (getPort() == -1)   return 0;

   return Sequencer_oss::open();
   pauseQ[getPort()] = 0;
}



//////////////////////////////
//
// MidiInPort_oss::pause -- stop the Midi input port from
//	inserting MIDI messages into the buffer, but keeps the
//	port open.  Use unpause() to reverse the effect of pause().
//

void MidiInPort_oss::pause(void) {
   if (getPort() == -1)   return;

   pauseQ[getPort()] = 1;
}



//////////////////////////////
//
// MidiInPort_oss::setBufferSize -- sets the allocation
//	size of the MIDI input buffer.
//

void MidiInPort_oss::setBufferSize(int aSize) {
   if (getPort() == -1)  return;

   midiBuffer[getPort()]->setSize(aSize);
}



//////////////////////////////
//
// MidiInPort_oss::setChannelOffset -- sets the MIDI chan offset, 
//     either 0 or 1.
//

void MidiInPort_oss::setChannelOffset(int anOffset) {
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
// MidiInPort_oss::setPort
//

void MidiInPort_oss::setPort(int aPort) {
//   if (aPort == -1) return;
   if (aPort < -1 || aPort >= getNumPorts()) {
//      std::cerr << "Error: maximum port number is: " << getNumPorts()-1
//           << ", but you tried to access port: " << aPort << std::endl;
//     exit(1);
   }
   else {
      if (port != -1) {
         portObjectCount[port]--;
      }
      port = aPort;
      if (port != -1) {
         portObjectCount[port]++;
      }
   }
}



//////////////////////////////
//
// MidiInPort_oss::setTrace -- if false, then don't print MIDI messages
// 	to the screen.
//

int MidiInPort_oss::setTrace(int aState) {
   if (getPort() == -1)   return -1;


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
// MidiInPort_oss::toggleTrace -- switches the state of trace
//	Returns the previous value of the trace variable.
//

void MidiInPort_oss::toggleTrace(void) {
   if (getPort() == -1)   return;

   trace[getPort()] = !trace[getPort()];
}
   


//////////////////////////////
//
// MidiInPort_oss::unpause -- enables the Midi input port 
//	to inserting MIDI messages into the buffer after the 
//	port is already open.
//

void MidiInPort_oss::unpause(void) {
   if (getPort() == -1)   return;
  
   pauseQ[getPort()] = 0;
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiInPort_oss::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_oss::deinitialize(void) {
   closeAll();

   for (int i=0; i<getNumPorts(); i++) {
      if (sysexBuffers != NULL && sysexBuffers[i] != NULL) {
         delete [] sysexBuffers[i];
         sysexBuffers[i] = NULL;
      }
   }

   if (sysexBuffers != NULL) {
      delete [] sysexBuffers;
      sysexBuffers = NULL;
   }

   if (midiBuffer != NULL) {
      delete [] midiBuffer;
      midiBuffer = NULL;
   }

   if (portObjectCount != NULL) {
      delete [] portObjectCount;
      portObjectCount = NULL;
   }

   if (trace != NULL) {
      delete [] trace;
      trace = NULL;
   }

   if (pauseQ != NULL) {
      delete [] pauseQ;
      pauseQ = NULL;
   }
}



//////////////////////////////
//
// MidiInPort_oss::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_oss::initialize(void) {
   // set the number of ports
   numDevices = Sequencer_oss::indevcount;

   if  (getNumPorts() <= 0) {
//      std::cerr << "Warning: no MIDI input devices" << std::endl;
   } else {
   
      // allocate space for pauseQ, the port pause status
      if (pauseQ != NULL) {
         delete [] pauseQ;
      }
      pauseQ = new int[numDevices];
   
      // allocate space for object count on each port:
      if (portObjectCount != NULL) {
         delete [] portObjectCount;
      }
      portObjectCount = new int[numDevices];
   
      // allocate space for object count on each port:
      if (trace != NULL) {
         delete [] trace;
      }
      trace = new int[numDevices];
   
      // allocate space for the Midi input buffers
      if (midiBuffer != NULL) {
         delete [] midiBuffer;
      }
      midiBuffer = new CircularBuffer<MidiMessage>*[numDevices];

      // allocate space for Midi input sysex buffer write indices
      if (sysexWriteBuffer != NULL) {
         delete [] sysexWriteBuffer;
      }
      sysexWriteBuffer = new int[numDevices];

      // allocate space for Midi input sysex buffers
      if (sysexBuffers != NULL) {
         std::cout << "Error: memory leak on sysex buffers initialization" << std::endl;
         exit(1);
      }
      sysexBuffers = new Array<uchar>*[numDevices];
   
      // initialize the static arrays
      for (int i=0; i<getNumPorts(); i++) {
         portObjectCount[i] = 0;
         trace[i] = 0;
         pauseQ[i] = 0;
         midiBuffer[i] = new CircularBuffer<MidiMessage>;
         midiBuffer[i]->setSize(DEFAULT_INPUT_BUFFER_SIZE);

         sysexWriteBuffer[i] = 0;
         sysexBuffers[i] = new Array<uchar>[128];
         for (int n=0; n<128; n++) {
            sysexBuffers[i][n].allowGrowth(0);      // shouldn't need to grow
            sysexBuffers[i][n].setAllocSize(32);
            sysexBuffers[i][n].setSize(0);
            sysexBuffers[i][n].setGrowth(32);       // in case it will ever grow
         }
      }

      int flag = pthread_create(&midiInThread, NULL, 
         interpretMidiInputStreamPrivate, NULL);
      if (flag == -1) {
         std::cout << "Unable to create MIDI input thread." << std::endl;
         exit(1);
      }
   
   }
}



///////////////////////////////////////////////////////////////////////////
//
// friendly functions 
//


//////////////////////////////
//
// interpretMidiInputStreamPrivate -- handles the MIDI input stream
//     for the various input devices.
//
//  Note about system exclusive messages:
//     System Exclusive messages are stored in a separate buffer from
//     Other Midi messages since they can be variable in length.  If
//     The Midi Input returns a message with command byte 0xf0, then
//     the p1() byte indicates the system exclusive buffer number that is
//     holding the system exclusive data for that Midi message.  There
//     are 128 system exclusive buffers that are numbered between
//     0 and 127.  These buffers are filled in a cycle.
//     To extract a System exclusive message from MidiInPort_oss,
//     you first will receive a Message with a command byte of 0xf0.
//     you can then access the data for that sysex by the command:
//     MidiInPort_oss::getSysex(buffer_number), this will return
//     a pointer to the beginning of the sysex data.  The first byte
//     of the sysex data should be 0xf0, and the last byte of the data
//     is 0xf7.  All other bytes of data should be in the range from
//     0 to 127.  You can also get the size of the sysex buffer by the
//     following command:   MidiInPort_oss::getSysexSize(buffer_number).
//     This command will tell you the number of bytes in the system 
//     exclusive message including the starting 0xf0 and the ending 0xf7.
//
//     If you want to minimize memory useage of the system exclusive
//     buffers you can run the command:  
//     MidiInPort_oss::clearSysex(buffer_number);  Otherwise the sysex
//     buffer will be erased automatically the next time that the that
//     buffer number is cycled through when receiving more system exclusives.
//     Allocated the allocated size of the system exclusive storage will 
//     not be adjusted when the computer replaces the system exclusive
//     message unless more storage size is needed, clearSysex however,
//     will resize the sysex buffer to its default size (currently 32 bytes).
//     clearSysex() without arguments will resize all buffers so that
//     they are allocated to the default size and will erase data from
//     all buffers.  You can spoof a system exclusive message coming in
//     by installing a system exclusive message and then inserting
//     the system message command into the input buffer of the MidiInPort
//     class,  int sysex_buffer = MidiInPort_oss::installSysex(
//     uchar *data, int size); will put the data into a sysex buffer and
//     return the buffer number that it was placed into.
//
//     This function assumes that System Exclusive messages cannot be sent 
//     as a running status messages.
//
// Note about MidiMessage time stamps:
//     The MidiMessage::time field is a recording of the time that the 
//     first byte of the MidiMessage arrived.  If the message is from
//     running status mode, then the time that the first parameter byte
//     arrived is stored.   System exclusive message arrival times are
//     recoreded at the time of the last byte (0xf7) arriving.  This is
//     because other system messages can be coming in while the sysex
//     message is coming in.  Anyway, sysex messages are not really to
//     be used for real time MIDI messaging, so the exact moment that the
//     first byte of the sysex came in is not important to me.
//
//

void *interpretMidiInputStreamPrivate(void *) {

   int* argsExpected = NULL;     // MIDI parameter bytes expected to follow
   int* argsLeft     = NULL;     // MIDI parameter bytes left to wait for
   uchar packet[4];              // bytes for sequencer driver
   MidiMessage* message = NULL;  // holder for current MIDI message
   int newSigTime = 0;           // for millisecond timer
   int lastSigTime = -1;         // for millisecond timer
   int zeroSigTime = -1;         // for timing incoming events
   int device = -1;              // for sorting out the bytes by input device
   Array<uchar>* sysexIn;        // MIDI Input sysex temporary storage

   // Note on the use of argsExpected and argsLeft for sysexs:
   // If argsExpected is -1, then a sysex message is coming in.
   // If argsLeft < 0, then the sysex message has not finished comming
   // in.  If argsLeft == 0 and argsExpected == -1, then the sysex
   // has finished coming in and is to be sent to the correct
   // location.

   static int count = 0;
   if (count != 0) {
      std::cerr << "Cannot run this function more than once" << std::endl;
      exit(1);
   } else {
      // allocate space for MIDI messages, each device has a different message
      // holding spot in case the messages overlap in the input stream
      message      = new MidiMessage[MidiInPort_oss::numDevices];
      argsExpected = new int[MidiInPort_oss::numDevices];
      argsLeft     = new int[MidiInPort_oss::numDevices];

      sysexIn = new Array<uchar>[MidiInPort_oss::numDevices];
      for (int j=0; j<MidiInPort_oss::numDevices; j++) {
         sysexIn[j].allowGrowth();
         sysexIn[j].setSize(32);
         sysexIn[j].setSize(0);
         sysexIn[j].setGrowth(512);
      }

      count++;
   }
   
   // interpret MIDI bytes as they come into the computer
   // and repackage them as MIDI messages.
   int packetReadCount;
   while (1) {
top_of_loop:
      packetReadCount = ::read(MidiInPort_oss::sequencer_fd, 
         &packet, sizeof(packet));

      if (packetReadCount != 4) {
         // this if statement is used to prevent cases where the
         // read function above will time out and return 0 bytes 
         // read.  This if statment will also take care of -1 
         // error return values by ignoring them.
         continue;
      }
     

/*
      std::cout << "packet bytes "
           << " 0x" << std::hex << (int)packet[0]   // packet type
           << " 0x" << std::hex << (int)packet[1]   // possible MIDI byte
           << " 0x" << std::hex << (int)packet[2]   // device number
           << " 0x" << std::hex << (int)packet[3]   // unused for MIDI bytes
           << std::endl;
*/

      switch (packet[0]) {
         case SEQ_WAIT:
            // MIDI clock ticks ... the first MIDI message deltaTime is
            // calculated wrt the start of the MIDI clock.
            if (zeroSigTime < 0) {
               zeroSigTime = MidiInPort_oss::midiTimer.getTime();
            }
/* 
            int newTime;
            newTime = packet[3];
            newTime = (newTime << 8) | packet[2];
            newTime = (newTime << 8) | packet[1];
*/
            break;

         case SEQ_ECHO:
            // echo events
            break;

         case SEQ_MIDIPUTC:          // SEQ_MIDIPUTC = 5
            // determination of a full MIDI message from the input MIDI
            // stream is based here on the observation that MIDI status
            // bytes and subsequent data bytes are NOT returned in the same
            // read() call.  Rather, they are spread out over multiple read()
            // returns, with only a single value per return.  So if we
            // find a status byte, we then determine the number of expected
            // operands and process that number of subsequent read()s to
            // to determine the complete midi message.

/*
            std::cout << "MIDI byte: " << (int)packet[1] << std::endl;
*/

            // store the MIDI input device to which the incoming MIDI
            // byte belongs.
            device = packet[2];
           
            // ignore the active sensing 0xfe and MIDI clock 0xf8 commands:
            if (packet[1] == 0xfe || packet[1] == 0xf8) {
               break;
            }

            if (packet[1] & 0x80) {   // a command byte has arrived
               switch (packet[1] & 0xf0) {
                  case 0xf0:   
                     if (packet[1] == 0xf0) {
                        argsExpected[device] = -1;
                        argsLeft[device] = -1;
                        if (sysexIn[device].getSize() != 0) {
                           // ignore the command for now.  It is most
                           // likely an active sensing message.
                           goto top_of_loop;
                        } else {
                           uchar datum = 0xf0;
                           sysexIn[device].append(datum);
                        }
                     } if (packet[1] == 0xf7) {
                        argsLeft[device] = 0;         // indicates sysex is done
                        uchar datum = 0xf7;
                        sysexIn[device].append(datum);
                     } else if (argsExpected[device] != -1) {
                        // this is a system message that may or may
                        // not be coming while a sysex is coming in
                        argsExpected[device] = 0;
                     } else {
                        // this is a system message that is not coming
                        // while a system exclusive is coming in
                        //argsExpected[device] = 0;
                     }
                     break;
                  case 0xc0:   
                     if (argsExpected[device] < 0) {
                        std::cout << "Error: received program change during sysex" 
                             << std::endl;
                        exit(1);
                     } else {
                        argsExpected[device] = 1;    
                     }
                     break;
                  case 0xd0:   
                     if (argsExpected[device] < 0) {
                        std::cout << "Error: received aftertouch message during" 
                                " sysex" << std::endl;
                        exit(1);
                     } else {
                        argsExpected[device] = 1;    
                     }
                     break;
                  default:     
                     if (argsExpected[device] < 0) {
                        std::cout << "Error: received another message during sysex" 
                             << std::endl;
                        exit(1);
                     } else {
                        argsExpected[device] = 2;    
                     }
                     break;
               }
               if (argsExpected[device] >= 0) {
                  argsLeft[device] = argsExpected[device];
               }

               newSigTime = MidiInPort_oss::midiTimer.getTime();
               message[device].time = newSigTime - zeroSigTime;

               if (packet[1] != 0xf7) {
                  message[device].p0() = packet[1];
               } 
               message[device].p1() = 0;
               message[device].p2() = 0;
               message[device].p3() = 0;

               if (packet[1] == 0xf7) {
                  goto sysex_done;
               }
            } else if (argsLeft[device]) {   // not a command byte coming in
               if (message[device].time == 0) {
                  // store the receipt time of the first message byte
                  newSigTime = MidiInPort_oss::midiTimer.getTime();
                  message[device].time = newSigTime - zeroSigTime;
               }
                  
               if (argsExpected[device] < 0) {
                  // continue processing a sysex message
                  sysexIn[device].append(packet[1]);
               } else {
                  // handle a message other than a sysex message
                  if (argsLeft[device] == argsExpected[device]) {
                     message[device].p1() = packet[1];
                  } else {
                     message[device].p2() = packet[1];
                  }
                  argsLeft[device]--;
               }

               // if MIDI message is complete, setup for running status, and 
               // insert note into proper buffer.

               if (argsExpected[device] >= 0 && !argsLeft[device]) {

                  // store parameter data for running status
                  switch (message[device].p0() & 0xf0) {
                     case 0xc0:  argsLeft[device] = 1;    break;
                     case 0xd0:  argsLeft[device] = 1;    break;  // fix by dan
                     default:    argsLeft[device] = 2;    break;
                        // 0x80 expects two arguments
                        // 0x90 expects two arguments
                        // 0xa0 expects two arguments
                        // 0xb0 expects two arguments
                        // 0xe0 expects two arguments
                  }

                  lastSigTime = newSigTime;

                  sysex_done:      // come here when a sysex is completely done

                  // insert the MIDI message into the appropriate buffer
                  // do not insert into buffer if the MIDI input device
                  // is paused (which can mean closed).  Or if the
                  // pauseQ array is pointing to NULL (which probably means that
                  // things are about to shut down).
                  if (MidiInPort_oss::pauseQ != NULL &&
                        MidiInPort_oss::pauseQ[device] == 0) {
                     if (argsExpected[device] < 0) {
                        // store the sysex in the MidiInPort_oss
                        // buffer for sysexs and return the storage
                        // location:
                        int sysexlocation = 
                           MidiInPort_oss::installSysexPrivate(device,
                              sysexIn[device].getBase(),
                              sysexIn[device].getSize());

                        message[device].p0() = 0xf0;
                        message[device].p1() = sysexlocation;

                        sysexIn[device].setSize(0); // empty the sysex storage
                        argsExpected[device] = 0;   // no run status for sysex
                        argsLeft[device] = 0;       // turn off sysex input flag
                     }
                     MidiInPort_oss::midiBuffer[device]->insert(
                           message[device]);
//                   if (MidiInPort_oss::callbackFunction != NULL) {
//                      MidiInPort_oss::callbackFunction(device);
//                   }
                     if (MidiInPort_oss::trace[device]) {
                        std::cout << '[' << std::hex << (int)message[device].p0()
                             << ':' << std::dec << (int)message[device].p1()
                             << ',' << (int)message[device].p2() << ']'
                             << std::flush;
                     }
                     message[device].time = 0;
                  } else {
                     if (MidiInPort_oss::trace[device]) {
                        std::cout << '[' << std::hex << (int)message[device].p0()
                             << 'P' << std::dec << (int)message[device].p1()
                             << ',' << (int)message[device].p2() << ']'
                             << std::flush;
                     }
                  }
               }
            }
            break;

         default:
            break;
      } // end switch
   } // end while (1)

   // This code is not yet reached, but should be made to do so eventually

   if (message != NULL) {
      delete message;
      message = NULL;
   }

   if (argsExpected != NULL) {
      delete argsExpected;
      argsExpected = NULL;
   }

   if (argsLeft != NULL) {
      delete argsLeft;
      argsLeft = NULL;
   }

   if (sysexIn != NULL) { 
      delete sysexIn;
      sysexIn = NULL;
   }

}



#endif  // LINUX



// md5sum:	2008f4a298bef0cd85620a5507815866  - MidiInPort_oss.cpp =css= 20030102
