//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu May 11 21:10:02 PDT 2000
// Last Modified: Wed Oct  3 22:18:27 PDT 2001 (frozen to ALSA 0.5)
// Filename:      ...sig/maint/code/control/Sequencer_alsa05.cpp
// Web Address:   http://sig.sapp.org/src/sig/Sequencer_alsa05.cpp
// Syntax:        C++ 
//
// Description:   Basic MIDI input/output functionality for the 
//                Linux ALSA raw midi devices.  This class
//                is inherited by the classes MidiInPort_alsa05 and
//                MidiOutPort_alsa05.
//

#if defined(LINUX) && defined(ALSA05)
 
#include "Collection.h"
#include <sys/asoundlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <iostream.h>
#include <stdio.h>
#include "Sequencer_alsa05.h"

typedef unsigned char uchar;

// define static variables:
int    Sequencer_alsa05::class_count          =  0;
int    Sequencer_alsa05::initialized          =  0;

// static variables for MIDI I/O information database
int    Sequencer_alsa05::indevcount      = 0;
int    Sequencer_alsa05::outdevcount     = 0;

Collection<snd_rawmidi_t*> Sequencer_alsa05::rawmidi_in;
Collection<snd_rawmidi_t*> Sequencer_alsa05::rawmidi_out;
Collection<int> Sequencer_alsa05::midiincard;
Collection<int> Sequencer_alsa05::midioutcard;
Collection<int> Sequencer_alsa05::midiindevice;
Collection<int> Sequencer_alsa05::midioutdevice;
Collection<char*> Sequencer_alsa05::midiinname;
Collection<char*> Sequencer_alsa05::midioutname;



///////////////////////////////
//
// Sequencer_alsa05::Sequencer_alsa05 --
//	default value: autoOpen = 1;
//

Sequencer_alsa05::Sequencer_alsa05(int autoOpen) {
   if (class_count < 0) {
      cerr << "Unusual class instatiation count: " << class_count << endl;
      exit(1);
   } else if (class_count == 0) {
      buildInfoDatabase();
   }

   if (autoOpen) {
      open();
   }

   class_count++;
}



//////////////////////////////
//
// Sequencer_alsa05::~Sequencer_alsa05 --
//

Sequencer_alsa05::~Sequencer_alsa05() {

   if (class_count == 1) {
      close();
      removeInfoDatabase();
   } else if (class_count <= 0) {
      cerr << "Unusual class instatiation count: " << class_count << endl;
      exit(1);
   }

   class_count--;
}



//////////////////////////////
//
// Sequencer_alsa05::close -- close the sequencer device.  The device
//   automatically closes once the program ends.
//

void Sequencer_alsa05::close(void) {
   int i;

   for (i=0; i<getNumInputs(); i++) {
      if (rawmidi_in[i] != NULL) {
         snd_rawmidi_close(rawmidi_in[i]);
         rawmidi_in[i] = NULL;
      }
   }

   for (i=0; i<getNumOutputs(); i++) {
      if (rawmidi_out[i] != NULL) {
         snd_rawmidi_close(rawmidi_out[i]);
         rawmidi_out[i] = NULL;
      }
   }

}



//////////////////////////////
//
// Sequencer_alsa05::displayInputs -- display a list of the
//     available MIDI input devices.
//	default values: out = cout, initial = "\t"
//
 
void Sequencer_alsa05::displayInputs(ostream& out, char* initial) {
   for (int i=0; i<getNumInputs(); i++) {
      out << initial << i << ": " << getInputName(i) << '\n';
   }
}



//////////////////////////////
//
// Sequencer_alsa05::displayOutputs -- display a list of the
//     available MIDI output devices.
//	default values: out = cout, initial = "\t"
//
 
void Sequencer_alsa05::displayOutputs(ostream& out, char* initial) {
   for (int i=0; i<getNumOutputs(); i++) {
      out << initial << i << ": " << getOutputName(i) << '\n';
   }
}



//////////////////////////////
//
// Sequencer_alsa05::getInputName -- returns a string to the name of
//    the specified input device.  The string will remain valid as
//    long as there are any sequencer devices in existence.
//

const char* Sequencer_alsa05::getInputName(int aDevice) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return midiinname[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa05::getNumInputs -- returns the total number of
//     MIDI inputs that can be used.
//

int Sequencer_alsa05::getNumInputs(void) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return indevcount;
}



//////////////////////////////
//
// Sequencer_alsa05::getNumOutputs -- returns the total number of
//     MIDI inputs that can be used.
//

int Sequencer_alsa05::getNumOutputs(void) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return outdevcount;
}



//////////////////////////////
//
// Sequencer_alsa05::getOutputName -- returns a string to the name of
//    the specified output device.  The string will remain valid as
//    long as there are any sequencer devices in existence.
//

const char* Sequencer_alsa05::getOutputName(int aDevice) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return midioutname[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa05::is_open -- returns true if the
//     sequencer device is open, false otherwise.
//

int Sequencer_alsa05::is_open(int mode, int index) {
   if (mode == 0) {
      // midi output
      if (rawmidi_out[index] != NULL) {
         return 1;
      } else {
         return 0;
      }
   } else {
      if (rawmidi_in[index] != NULL) {
         return 1;
      } else {
         return 0;
      }
   }
}


int Sequencer_alsa05::is_open_in(int index) {
   return is_open(1, index);
}


int Sequencer_alsa05::is_open_out(int index) {
   return is_open(0, index);
}



/////////////////////////////
//
// Sequencer_alsa05::open -- returns true if the device
//	was successfully opended (or already opened)
//

int Sequencer_alsa05::open(void) {
   if (rawmidi_out.getSize() != 0 || rawmidi_in.getSize() != 0) {
      return 1;
   } else {
      return 0;
   }
}
   


//////////////////////////////
//
// Sequencer_alsa05::read -- reads MIDI bytes and also stores the 
//     device from which the byte was read from.  Timing is not
//     saved from the device.  If needed, then it would have to 
//     be saved in this function, or just return the raw packet
//     data (use rawread function).
//
 
void Sequencer_alsa05::read(int dev, uchar* buf, int count) {
   snd_rawmidi_read(rawmidi_in[dev], buf, count);
}



//////////////////////////////
//
// Sequencer_alsa05::rebuildInfoDatabase -- rebuild the internal
//   database that keeps track of the MIDI input and output devices.
//

void Sequencer_alsa05::rebuildInfoDatabase(void) {
   removeInfoDatabase();
   buildInfoDatabase();
}



///////////////////////////////
//
// Sequencer_alsa05::write -- Send a byte out the specified MIDI
//    port which can be either an internal or an external synthesizer.
//

int Sequencer_alsa05::write(int aDevice, int aByte) {
   uchar byte[1];
   byte[0] = (uchar)aByte;
   return write(aDevice, byte, 1);   
}


int Sequencer_alsa05::write(int aDevice, uchar* bytes, int count) {
   int status = snd_rawmidi_write(rawmidi_out[aDevice], bytes, count);
   return status == count ? 1 : 0;
}


int Sequencer_alsa05::write(int aDevice, char* bytes, int count) {
   return write(aDevice, (uchar*)bytes, count);
}


int Sequencer_alsa05::write(int aDevice, int* bytes, int count) {
   uchar *newBytes;
   newBytes = new uchar[count];
   for (int i=0; i<count; i++) {
      newBytes[i] = (uchar)bytes[i];
   }
   int status = write(aDevice, newBytes, count);
   delete [] newBytes;
   return status;
}



///////////////////////////////////////////////////////////////////////////
//
// private functions
//

//////////////////////////////
//
// Sequencer_alsa05::buildInfoDatabase -- determines the number
//     of MIDI input and output devices available from
//     /dev/snd/midiXX, and determines their names.
//

void Sequencer_alsa05::buildInfoDatabase(void) {
   if (initialized) {
      return;
   }

   initialized = 1;
  
   if (indevcount != 0 || outdevcount != 0) {
      cout << "Error: Sequencer_alsa05 is already running" << endl;
      cout << "Indevcout = " << indevcount << " and " 
           << " outdevcount = " << outdevcount << endl;
      exit(1);
   }

   indevcount = 0;
   outdevcount = 0;

   midiincard.setSize(0);
   midiincard.allowGrowth();
   midioutcard.setSize(0);
   midioutcard.allowGrowth();

   midiindevice.setSize(0);
   midiindevice.allowGrowth();
   midioutdevice.setSize(0);
   midioutdevice.allowGrowth();

   midiinname.setSize(0);
   midiinname.allowGrowth();
   midioutname.setSize(0);
   midioutname.allowGrowth();

   rawmidi_in.setSize(256);
   rawmidi_out.setSize(256);

   // read number of MIDI inputs/output available 
   int card; 
   int device;

   for (card=0; card<16; card++) {
      for (device=0; device<16; device++) {
         if (snd_rawmidi_open(&rawmidi_in[indevcount], card, device, 
               O_RDONLY | O_APPEND | O_NONBLOCK) == 0){
            midiincard.append(card);
            midiindevice.append(device);
            indevcount++;
         }
      }
   }
   rawmidi_in.setSize(indevcount);

   for (card=0; card<16; card++) {
      for (device=0; device<16; device++) {
         if (snd_rawmidi_open(&rawmidi_out[outdevcount], card,
               device, O_WRONLY | O_APPEND | O_NONBLOCK) == 0) {
            midioutcard.append(card);
            midioutdevice.append(device);
            outdevcount++;
         }
      }
   }
   rawmidi_out.setSize(indevcount);

   int i;
   char buffer[256] = {0};
   char* temp;
   for (i=0; i<indevcount; i++) {
      sprintf(buffer, "MIDI input %d", i);
      temp = new char[strlen(buffer) + 1];
      strcpy(temp, buffer);
      midiinname.append(temp);      
   }

   for (i=0; i<outdevcount; i++) {
      sprintf(buffer, "MIDI output %d", i);
      temp = new char[strlen(buffer) + 1];
      strcpy(temp, buffer);
      midioutname.append(temp);      
   }

   midiincard.allowGrowth(0);
   midioutcard.allowGrowth(0);
   midiindevice.allowGrowth(0);
   midioutdevice.allowGrowth(0);
   midiinname.allowGrowth(0);
   midioutname.allowGrowth(0);
   rawmidi_in.allowGrowth(0);
   rawmidi_out.allowGrowth(0);

}



//////////////////////////////
//
// Sequencer_alsa05::getInDeviceValue --
//

int Sequencer_alsa05::getInDeviceValue(int aDevice) const {
   return midiindevice[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa05::getInCardValue --
//

int Sequencer_alsa05::getInCardValue(int aDevice) const {
   return midiincard[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa05::getOutDeviceValue --
//

int Sequencer_alsa05::getOutDeviceValue(int aDevice) const {
   return midioutdevice[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa05::getOutCardValue --
//

int Sequencer_alsa05::getOutCardValue(int aDevice) const {
   return midioutcard[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa05::removeInfoDatabase --
//

void Sequencer_alsa05::removeInfoDatabase(void) {
   if (rawmidi_in.getSize() != 0) {
      close();
   }

   if (rawmidi_out.getSize() != 0) {
      close();
   }

   rawmidi_in.setSize(0);
   rawmidi_out.setSize(0);
   midiincard.setSize(0);
   midioutcard.setSize(0);
   midiindevice.setSize(0);
   midioutdevice.setSize(0);

   int i;
   for (i=0; i<midiinname.getSize(); i++) {
      if (midiinname[i] != NULL) {
         delete [] midiinname[i];
      }
   }

   for (i=0; i<midioutname.getSize(); i++) {
      if (midioutname[i] != NULL) {
         delete [] midioutname[i];
      }
   }

   indevcount = 0;
   outdevcount = 0;
   initialized = 0;
}


#endif   /* LINUX and ALSA05 */


// md5sum:	b47021f0b71b2bc1555755584777c9a8  - Sequencer_alsa05.cpp =css= 20030102
