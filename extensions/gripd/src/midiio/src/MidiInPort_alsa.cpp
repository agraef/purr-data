//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun May 14 22:03:16 PDT 2000
// Last Modified: Sat Oct 13 16:11:10 PDT 2001 (updated for ALSA 0.9)
// Last Modified: Fri Oct 26 14:41:36 PDT 2001 (running status for 0xa0 and 0xd0 
//                                              fixed by Daniel Gardner)
// Last Modified: Mon Nov 19 17:52:15 PST 2001 (thread on exit improved)
// Filename:      ...sig/code/control/MidiInPort/linux/MidiInPort_alsa.cpp
// Web Address:   http://sig.sapp.org/src/sig/MidiInPort_alsa.cpp
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of
//                linux ALSA sound driver's specific MIDI input methods.
//                This class is inherited privately by the MidiInPort class.
//

#if defined(LINUX) && defined(ALSA)

#include "MidiInPort_alsa.h"
#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <unistd.h>

#define DEFAULT_INPUT_BUFFER_SIZE (1024)

// initialized static variables

int       MidiInPort_alsa::numDevices                     = 0;
int       MidiInPort_alsa::objectCount                    = 0;
int*      MidiInPort_alsa::portObjectCount                = NULL;
CircularBuffer<MidiMessage>** MidiInPort_alsa::midiBuffer = NULL;
int       MidiInPort_alsa::channelOffset                  = 0;
SigTimer  MidiInPort_alsa::midiTimer;
int*      MidiInPort_alsa::pauseQ                         = NULL;
int*      MidiInPort_alsa::trace                          = NULL;
std::ostream*  MidiInPort_alsa::tracedisplay                   = &std::cout;
Array<pthread_t> MidiInPort_alsa::midiInThread;    
int*      MidiInPort_alsa::sysexWriteBuffer               = NULL;
Array<uchar>** MidiInPort_alsa::sysexBuffers              = NULL;

Array<int> MidiInPort_alsa::threadinitport;



//////////////////////////////
// 
// MidiInPort_alsa::MidiInPort_alsa
//	default values: autoOpen = 1
//

MidiInPort_alsa::MidiInPort_alsa(void) {
   if (objectCount == 0) {
      initialize();
   }
   objectCount++;

   port = -1;
   setPort(0);
}


MidiInPort_alsa::MidiInPort_alsa(int aPort, int autoOpen) {
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
// MidiInPort_alsa::~MidiInPort_alsa
//

MidiInPort_alsa::~MidiInPort_alsa() {
   objectCount--;
   if (objectCount == 0) {
      deinitialize();
   } else if (objectCount < 0) {
      std::cerr << "Error: bad MidiInPort_alsa object count!: " 
           << objectCount << std::endl;
      exit(1);
   }
}



//////////////////////////////
//
// MidiInPort_alsa::clearSysex -- clears the data from a sysex
//      message and sets the allocation size to the default size (of 32
//      bytes).
//

void MidiInPort_alsa::clearSysex(int buffer) {
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


void MidiInPort_alsa::clearSysex(void) {
   // clear all sysex buffers
   for (int i=0; i<128; i++) {
      clearSysex(i);
   }
}



//////////////////////////////
//
// MidiInPort_alsa::close
//

void MidiInPort_alsa::close(void) {
   if (getPort() == -1) return;

   pauseQ[getPort()] = 1;
   Sequencer_alsa::closeInput(getPort());
}



//////////////////////////////
//
// MidiInPort_alsa::closeAll --
//

void MidiInPort_alsa::closeAll(void) {
   for (int i=0; i<getNumPorts(); i++) {
      pauseQ[i] = 1;
      Sequencer_alsa::closeInput(i);
   }
}



//////////////////////////////
//
// MidiInPort_alsa::extract -- returns the next MIDI message
//	received since that last extracted message.
//

MidiMessage MidiInPort_alsa::extract(void) {
   if (getPort() == -1) {
      MidiMessage temp;
      return temp;
   }

   return midiBuffer[getPort()]->extract();
}



//////////////////////////////
//
// MidiInPort_alsa::getBufferSize -- returns the maximum possible number
//	of MIDI messages that can be stored in the buffer
//

int MidiInPort_alsa::getBufferSize(void) {
   if (getPort() == -1)   return 0;

   return midiBuffer[getPort()]->getSize();
}



//////////////////////////////
//
// MidiInPort_alsa::getChannelOffset -- returns zero if MIDI channel 
//     offset is 0, or 1 if offset is 1.
//

int MidiInPort_alsa::getChannelOffset(void) const {
   return channelOffset;
}



//////////////////////////////
//
// MidiInPort_alsa::getCount -- returns the number of unexamined
//	MIDI messages waiting in the input buffer.
//

int MidiInPort_alsa::getCount(void) {
   if (getPort() == -1)   return 0;
   return midiBuffer[getPort()]->getCount();
}



//////////////////////////////
//
// MidiInPort_alsa::getName -- returns the name of the port.
//	returns "" if no name. Name is valid until all instances
//      of MIDI classes are.
//

const char* MidiInPort_alsa::getName(void) {
   if (getPort() == -1) {
      return "Null ALSA MIDI Input";
   }
   return getInputName(getPort());
}


const char* MidiInPort_alsa::getName(int i) {
   return getInputName(i);
}



//////////////////////////////
//
// MidiInPort_alsa::getNumPorts -- returns the number of available
// 	ports for MIDI input
//

int MidiInPort_alsa::getNumPorts(void) {
   return getNumInputs();
}



//////////////////////////////
//
// MidiInPort_alsa::getPort -- returns the port to which this
//	object belongs (as set with the setPort function).
//

int MidiInPort_alsa::getPort(void) {
   return port;
}



//////////////////////////////
//
// MidiInPort_alsa::getPortStatus -- 0 if closed, 1 if open
//

int MidiInPort_alsa::getPortStatus(void) {
   return is_open_in(getPort());
}



//////////////////////////////
//
// MidiInPort_alsa::getSysex -- returns the sysex message contents
//    of a given buffer.  You should check to see that the size is
//    non-zero before looking at the data.  The data pointer will
//    be NULL if there is no data in the buffer.
//

uchar* MidiInPort_alsa::getSysex(int buffer) {
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
// MidiInPort_alsa::getSysexSize -- returns the sysex message byte
//    count of a given buffer.   Buffers are in the range from 
//    0 to 127.
//

int MidiInPort_alsa::getSysexSize(int buffer) {
   if (getPort() == -1) {
      return 0;
   } else {
      return sysexBuffers[getPort()][buffer & 0x7f].getSize();
   }
}



//////////////////////////////
//
// MidiInPort_alsa::getTrace -- returns true if trace is on or false
//	if trace is off.  if trace is on, then prints to standard
// 	output the Midi message received.
//

int MidiInPort_alsa::getTrace(void) {
   if (getPort() == -1)   return -1;

   return trace[getPort()];
}



//////////////////////////////
//
// MidiInPort_alsa::insert
//

void MidiInPort_alsa::insert(const MidiMessage& aMessage) {
   if (getPort() == -1)   return;

   midiBuffer[getPort()]->insert(aMessage);
}



//////////////////////////////
//
// MidiInPort_alsa::installSysex -- put a sysex message into a
//      buffer.  The buffer number that it is put into is returned.
//

int MidiInPort_alsa::installSysex(uchar* anArray, int aSize) {
   if (getPort() == -1) {
      return -1;
   } else {
      return installSysexPrivate(getPort(), anArray, aSize);
   }
}



//////////////////////////////
//
// MidiInPort_alsa::installSysexPrivate -- put a sysex message into a
//      buffer.  The buffer number that it is put into is returned.
//

int MidiInPort_alsa::installSysexPrivate(int port, uchar* anArray, int aSize) {
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
// MidiInPort_alsa::message --  look at an incoming MIDI message
//     without extracting it from the input buffer.
//

MidiMessage& MidiInPort_alsa::message(int index) {
   if (getPort() == -1) {
      static MidiMessage x;
      return x;
   }

   CircularBuffer<MidiMessage>& temp = *midiBuffer[getPort()];
   return temp[index];
}



//////////////////////////////
//
// MidiInPort_alsa::open -- returns true if MIDI input port was
//	opened.
//

int MidiInPort_alsa::open(void) {
   if (getPort() == -1)   return 0;

   int status = Sequencer_alsa::openInput(getPort());
   if (status) {
      pauseQ[getPort()] = 0;
      return 1;
   } else {
      pauseQ[getPort()] = 1;
      return 0;
   }

   return 0;
}



//////////////////////////////
//
// MidiInPort_alsa::pause -- stop the Midi input port from
//	inserting MIDI messages into the buffer, but keeps the
//	port open.  Use unpause() to reverse the effect of pause().
//

void MidiInPort_alsa::pause(void) {
   if (getPort() == -1)   return;

   pauseQ[getPort()] = 1;
}



//////////////////////////////
//
// MidiInPort_alsa::setBufferSize -- sets the allocation
//	size of the MIDI input buffer.
//

void MidiInPort_alsa::setBufferSize(int aSize) {
   if (getPort() == -1)  return;

   midiBuffer[getPort()]->setSize(aSize);
}



//////////////////////////////
//
// MidiInPort_alsa::setChannelOffset -- sets the MIDI chan offset, 
//     either 0 or 1.
//

void MidiInPort_alsa::setChannelOffset(int anOffset) {
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
// MidiInPort_alsa::setPort --
//

void MidiInPort_alsa::setPort(int aPort) {
//   if (aPort == -1) return;
   if (aPort < -1 || aPort >= getNumPorts()) {
//      std::cerr << "Error: maximum port number is: " << getNumPorts()-1
//           << ", but you tried to access port: " << aPort << std::endl;
//      exit(1);
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
// MidiInPort_alsa::setTrace -- if false, then don't print MIDI messages
// 	to the screen.
//

int MidiInPort_alsa::setTrace(int aState) {
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
// MidiInPort_alsa::toggleTrace -- switches the state of trace
//	Returns the previous value of the trace variable.
//

void MidiInPort_alsa::toggleTrace(void) {
   if (getPort() == -1)   return;

   trace[getPort()] = !trace[getPort()];
}
   


//////////////////////////////
//
// MidiInPort_alsa::unpause -- enables the Midi input port 
//	to inserting MIDI messages into the buffer after the 
//	port is already open.
//

void MidiInPort_alsa::unpause(void) {
   if (getPort() == -1)   return;
  
   pauseQ[getPort()] = 0;
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//



//////////////////////////////
//
// MidiInPort_alsa::deinitialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_alsa::deinitialize(void) {
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
// MidiInPort_alsa::initialize -- sets up storage if necessary
//	This function should be called if the current object is
//	the first object to be created.
//

void MidiInPort_alsa::initialize(void) {
   // set the number of ports
   numDevices = Sequencer_alsa::indevcount;

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
   
      int flag;
      midiInThread.setSize(getNumPorts());
      threadinitport.setSize(getNumPorts());
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
    
         threadinitport[i] = i;
         flag = pthread_create(&midiInThread[i], NULL, 
            interpretMidiInputStreamPrivateALSA, &threadinitport[i]);
         if (flag == -1) {
            std::cout << "Unable to create MIDI input thread." << std::endl;
            exit(1);
         }
      }
   
   }
}



///////////////////////////////////////////////////////////////////////////
//
// friendly functions 
//


//////////////////////////////
//
// interpretMidiInputStreamPrivateALSA -- handles the MIDI input stream
//     for the various input devices from the ALSA MIDI driver.
//
//  Note about system exclusive messages:
//     System Exclusive messages are stored in a separate buffer from
//     Other Midi messages since they can be variable in length.  If
//     The Midi Input returns a message with command byte 0xf0, then
//     the p1() byte indicates the system exclusive buffer number that is
//     holding the system exclusive data for that Midi message.  There
//     are 128 system exclusive buffers that are numbered between
//     0 and 127.  These buffers are filled in a cycle.
//     To extract a System exclusive message from MidiInPort_alsa,
//     you first will receive a Message with a command byte of 0xf0.
//     you can then access the data for that sysex by the command:
//     MidiInPort_alsa::getSysex(buffer_number), this will return
//     a pointer to the beginning of the sysex data.  The first byte
//     of the sysex data should be 0xf0, and the last byte of the data
//     is 0xf7.  All other bytes of data should be in the range from
//     0 to 127.  You can also get the size of the sysex buffer by the
//     following command:   MidiInPort_alsa::getSysexSize(buffer_number).
//     This command will tell you the number of bytes in the system 
//     exclusive message including the starting 0xf0 and the ending 0xf7.
//
//     If you want to minimize memory useage of the system exclusive
//     buffers you can run the command:  
//     MidiInPort_alsa::clearSysex(buffer_number);  Otherwise the sysex
//     buffer will be erased automatically the next time that the 
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
//     class,  int sysex_buffer = MidiInPort_alsa::installSysex(
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

void *interpretMidiInputStreamPrivateALSA(void * arg) {
   int portToWatch = *(int*)arg;
   if (portToWatch < 0 || portToWatch > 1000) {
      // the port to watch is invalid -- because the program has died 
      // before the thread function could start.  Cause of invalid port 
      // data should be examined more carefully.
      return NULL;
   }

   int* argsExpected = NULL;     // MIDI parameter bytes expected to follow
   int* argsLeft     = NULL;     // MIDI parameter bytes left to wait for
   uchar packet[1];              // bytes for sequencer driver
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

   // allocate space for MIDI messages, each device has a different message
   // holding spot in case the messages overlap in the input stream
   message      = new MidiMessage[MidiInPort_alsa::numDevices];
   argsExpected = new int[MidiInPort_alsa::numDevices];
   argsLeft     = new int[MidiInPort_alsa::numDevices];

   sysexIn = new Array<uchar>[MidiInPort_alsa::numDevices];
   for (int j=0; j<MidiInPort_alsa::numDevices; j++) {
      sysexIn[j].allowGrowth();
      sysexIn[j].setSize(32);
      sysexIn[j].setSize(0);
      sysexIn[j].setGrowth(512);
   }

   // interpret MIDI bytes as they come into the computer
   // and repackage them as MIDI messages.
   int packetReadCount;
   while (1) {
top_of_loop:
      packetReadCount = 0;

      // If the all Sequencer_alsa classes have been deleted,
      // then Sequencer_alsa::rawmidi_in will have zero size.
      // If the size is zero, then that means the thread will be
      // killed soon, and we do not want any processing to happen
      // in this thread.  If the port to watch is NULL, then that
      // means that the MIDI input is not open, and we should not
      // add any MIDI data to the input buffers.  These cases are
      // handled by the following if-else statement:
      if (Sequencer_alsa::rawmidi_in.getSize() > 0 && 
            Sequencer_alsa::rawmidi_in[portToWatch] != NULL) {
         packetReadCount = snd_rawmidi_read(
               Sequencer_alsa::rawmidi_in[portToWatch], packet, 1);
      } else {
         usleep(100000);  // sleep for 1/10th of a second if the Input 
                          // port is not open.         
         continue;
      }


      if (packetReadCount != 1) {
         // this if statement is used to prevent cases where the
         // read function above will time out and return 0 bytes 
         // read.  This if statment will also take care of -1 
         // error return values by ignoring them.
         continue;
      }

      if (Sequencer_alsa::initialized == 0) {
         continue;
      }

      // determination of a full MIDI message from the input MIDI
      // stream is based here on the observation that MIDI status
      // bytes and subsequent data bytes are NOT returned in the same
      // read() call.  Rather, they are spread out over multiple read()
      // returns, with only a single value per return.  So if we
      // find a status byte, we then determine the number of expected
      // operands and process that number of subsequent read()s to
      // to determine the complete midi message.

      // store the MIDI input device to which the incoming MIDI
      // byte belongs.
      device = portToWatch;
     
      // ignore the active sensing 0xfe and MIDI clock 0xf8 commands:
      if (packet[0] == 0xfe || packet[0] == 0xf8) {
         continue;
      }

      if (packet[0] & 0x80) {   // a command byte has arrived
         switch (packet[0] & 0xf0) {
            case 0xf0:   
               if (packet[0] == 0xf0) {
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
               } if (packet[0] == 0xf7) {
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

         newSigTime = MidiInPort_alsa::midiTimer.getTime();
         message[device].time = newSigTime - zeroSigTime;

         if (packet[0] != 0xf7) {
            message[device].p0() = packet[0];
         } 
         message[device].p1() = 0;
         message[device].p2() = 0;
         message[device].p3() = 0;

         if (packet[0] == 0xf7) {
            goto sysex_done;
         }
      } else if (argsLeft[device]) {   // not a command byte coming in
         if (message[device].time == 0) {
            // store the receipt time of the first message byte
            newSigTime = MidiInPort_alsa::midiTimer.getTime();
            message[device].time = newSigTime - zeroSigTime;
         }
            
         if (argsExpected[device] < 0) {
            // continue processing a sysex message
            sysexIn[device].append(packet[0]);
         } else {
            // handle a message other than a sysex message
            if (argsLeft[device] == argsExpected[device]) {
               message[device].p1() = packet[0];
            } else {
               message[device].p2() = packet[0];
            }
            argsLeft[device]--;
         }

         // if MIDI message is complete, setup for running status, and 
         // insert note into proper buffer.

         if (argsExpected[device] >= 0 && !argsLeft[device]) {

            // store parameter data for running status
            switch (message[device].p0() & 0xf0) {
               case 0xc0:      argsLeft[device] = 1;      break;
               case 0xd0:      argsLeft[device] = 1;      break;  // fix by dan
               default:        argsLeft[device] = 2;      break;
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
            if (MidiInPort_alsa::pauseQ != NULL &&
                  MidiInPort_alsa::pauseQ[device] == 0) {
               if (argsExpected[device] < 0) {
                  // store the sysex in the MidiInPort_alsa
                  // buffer for sysexs and return the storage
                  // location:
                  int sysexlocation = 
                     MidiInPort_alsa::installSysexPrivate(device,
                        sysexIn[device].getBase(),
                        sysexIn[device].getSize());

                  message[device].p0() = 0xf0;
                  message[device].p1() = sysexlocation;

                  sysexIn[device].setSize(0); // empty the sysex storage
                  argsExpected[device] = 0;   // no run status for sysex
                  argsLeft[device] = 0;       // turn off sysex input flag
               }
               MidiInPort_alsa::midiBuffer[device]->insert(
                     message[device]);
//                   if (MidiInPort_alsa::callbackFunction != NULL) {
//                      MidiInPort_alsa::callbackFunction(device);
//                   }
               if (MidiInPort_alsa::trace[device]) {
                  std::cout << '[' << std::hex << (int)message[device].p0()
                       << ':' << std::dec << (int)message[device].p1()
                       << ',' << (int)message[device].p2() << ']'
                       << std::flush;
               }
               message[device].time = 0;
            } else {
               if (MidiInPort_alsa::trace[device]) {
                  std::cout << '[' << std::hex << (int)message[device].p0()
                       << 'P' << std::dec << (int)message[device].p1()
                       << ',' << (int)message[device].p2() << ']'
                       << std::flush;
               }
            }
         }
      }

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


   return NULL;
}



#endif  // LINUX && ALSA



// md5sum:	14663a91d1f5283e05ca36399d5ab767  - MidiInPort_alsa.cpp =css= 20030102
