//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu May 11 21:10:02 PDT 2000
// Last Modified: Sat Oct 13 14:51:43 PDT 2001 (updated for ALSA 0.9 interface)
// Filename:      ...sig/maint/code/control/Sequencer_alsa.cpp
// Web Address:   http://sig.sapp.org/src/sig/Sequencer_alsa.cpp
// Syntax:        C++ 
//
// Description:   Basic MIDI input/output functionality for the 
//                Linux ALSA raw midi devices.  This class
//                is inherited by the classes MidiInPort_alsa and
//                MidiOutPort_alsa.
//

#if defined(LINUX) && defined(ALSA)
 
#include "Collection.h"
#include <alsa/asoundlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <dirent.h>           /* for reading filename for MIDI info */
#include "Sequencer_alsa.h"

typedef unsigned char uchar;

// define static variables:
int    Sequencer_alsa::class_count          =  0;
int    Sequencer_alsa::initialized          =  0;

// static variables for MIDI I/O information database
int    Sequencer_alsa::indevcount      = 0;
int    Sequencer_alsa::outdevcount     = 0;

Collection<snd_rawmidi_t*> Sequencer_alsa::rawmidi_in;
Collection<snd_rawmidi_t*> Sequencer_alsa::rawmidi_out;
Collection<int>            Sequencer_alsa::midiincard;
Collection<int>            Sequencer_alsa::midioutcard;
Collection<int>            Sequencer_alsa::midiindevice;
Collection<int>            Sequencer_alsa::midioutdevice;
Collection<char*>          Sequencer_alsa::midiinname;
Collection<char*>          Sequencer_alsa::midioutname;



///////////////////////////////
//
// Sequencer_alsa::Sequencer_alsa --
//	default value: autoOpen = 1;
//

Sequencer_alsa::Sequencer_alsa(int autoOpen) {
   if (class_count < 0) {
      std::cerr << "Unusual class instantiation count: " << class_count << std::endl;
      exit(1);
   } else if (class_count == 0) {
      buildInfoDatabase();
   }

   // will not autoOpen

   class_count++;
}



//////////////////////////////
//
// Sequencer_alsa::~Sequencer_alsa --
//

Sequencer_alsa::~Sequencer_alsa() {

   if (class_count == 1) {
      close();
      removeInfoDatabase();
   } else if (class_count <= 0) {
      std::cerr << "Unusual class instantiation count: " << class_count << std::endl;
      exit(1);
   }

   class_count--;
}



//////////////////////////////
//
// Sequencer_alsa::close -- close the sequencer device.  The device
//   automatically closes once the program ends.
//

void Sequencer_alsa::close(void) {
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


void Sequencer_alsa::closeInput(int index) {
   if (index < 0 || index >= rawmidi_in.getSize()) {
      return;
   }

   if (rawmidi_in[index] != NULL) {
      snd_rawmidi_close(rawmidi_in[index]); 
      rawmidi_in[index] = NULL;
   }
}


void Sequencer_alsa::closeOutput(int index) {
   if (index < 0 || index >= rawmidi_out.getSize()) {
      return;
   }

   if (rawmidi_out[index] != NULL) {
      snd_rawmidi_close(rawmidi_out[index]); 
      rawmidi_out[index] = NULL;
   }
}



//////////////////////////////
//
// Sequencer_alsa::displayInputs -- display a list of the
//     available MIDI input devices.
//	default values: out = std::cout, initial = "\t"
//
 
void Sequencer_alsa::displayInputs(std::ostream& out, char* initial) {
   for (int i=0; i<getNumInputs(); i++) {
      out << initial << i << ": " << getInputName(i) << '\n';
   }
}



//////////////////////////////
//
// Sequencer_alsa::displayOutputs -- display a list of the
//     available MIDI output devices.
//	default values: out = std::cout, initial = "\t"
//
 
void Sequencer_alsa::displayOutputs(std::ostream& out, char* initial) {
   for (int i=0; i<getNumOutputs(); i++) {
      out << initial << i << ": " << getOutputName(i) << '\n';
   }
}



//////////////////////////////
//
// Sequencer_alsa::getInputName -- returns a string to the name of
//    the specified input device.  The string will remain valid as
//    long as there are any sequencer devices in existence.
//

const char* Sequencer_alsa::getInputName(int aDevice) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return midiinname[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa::getNumInputs -- returns the total number of
//     MIDI inputs that can be used.
//

int Sequencer_alsa::getNumInputs(void) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return indevcount;
}



//////////////////////////////
//
// Sequencer_alsa::getNumOutputs -- returns the total number of
//     MIDI inputs that can be used.
//

int Sequencer_alsa::getNumOutputs(void) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return outdevcount;
}



//////////////////////////////
//
// Sequencer_alsa::getOutputName -- returns a string to the name of
//    the specified output device.  The string will remain valid as
//    long as there are any sequencer devices in existence.
//

const char* Sequencer_alsa::getOutputName(int aDevice) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return midioutname[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa::is_open -- returns true if the
//     sequencer device is open, false otherwise.
//

int Sequencer_alsa::is_open(int mode, int index) {
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


int Sequencer_alsa::is_open_in(int index) {
   return is_open(1, index);
}


int Sequencer_alsa::is_open_out(int index) {
   return is_open(0, index);
}



/////////////////////////////
//
// Sequencer_alsa::open -- returns true if the device
//	was successfully opended (or already opened)
//

int Sequencer_alsa::open(int direction, int index) {
   if (direction == 0) {
      return openOutput(index);
   } else {
      return openInput(index);
   }
}


int Sequencer_alsa::openInput(int index) {
   if (rawmidi_in[index] != NULL) {
      return 1;
   }
   int status;
   char devname[128] = {0};
   sprintf(devname, "hw:%d,%d", midiincard[index], midiindevice[index]);
   status = snd_rawmidi_open(&rawmidi_in[index], NULL, devname, 0);
   if (status == 0) {
      return 1;
   } else { 
      return 0;
   }
}


int Sequencer_alsa::openOutput(int index) {
   if (rawmidi_out[index] != NULL) {
      return 1;
   }
   int status;
   char devname[128] = {0};
   sprintf(devname, "hw:%d,%d", midioutcard[index], midioutdevice[index]);
   status = snd_rawmidi_open(NULL, &rawmidi_out[index], devname, 0);
   if (status == 0) {
      return 1;
   } else { 
      return 0;
   }
}



//////////////////////////////
//
// Sequencer_alsa::read -- reads MIDI bytes and also stores the 
//     device from which the byte was read from.  Timing is not
//     saved from the device.  If needed, then it would have to 
//     be saved in this function, or just return the raw packet
//     data (use rawread function).
//
 
void Sequencer_alsa::read(int dev, uchar* buf, int count) {
   if (is_open_in(dev)) {
      snd_rawmidi_read(rawmidi_in[dev], buf, count);
   } else {
      std::cout << "Warning: MIDI input " << dev << " is not open for reading" 
           << std::endl;
   }
}



//////////////////////////////
//
// Sequencer_alsa::rebuildInfoDatabase -- rebuild the internal
//   database that keeps track of the MIDI input and output devices.
//

void Sequencer_alsa::rebuildInfoDatabase(void) {
   removeInfoDatabase();
   buildInfoDatabase();
}



///////////////////////////////
//
// Sequencer_alsa::write -- Send a byte out the specified MIDI
//    port which can be either an internal or an external synthesizer.
//

int Sequencer_alsa::write(int aDevice, int aByte) {
   uchar byte[1];
   byte[0] = (uchar)aByte;
   return write(aDevice, byte, 1);   
}


int Sequencer_alsa::write(int aDevice, uchar* bytes, int count) {
   if (is_open_out(aDevice)) {
      int status = snd_rawmidi_write(rawmidi_out[aDevice], bytes, count);
      return status == count ? 1 : 0;
   } else {
      std::cout << "Warning: MIDI output " << aDevice << " is not open for writing" 
           << std::endl;
      return 0;
   }

   return 0;
}


int Sequencer_alsa::write(int aDevice, char* bytes, int count) {
   return write(aDevice, (uchar*)bytes, count);
}


int Sequencer_alsa::write(int aDevice, int* bytes, int count) {
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
// Sequencer_alsa::buildInfoDatabase -- determines the number
//     of MIDI input and output devices available from
//     /dev/snd/midiC%dD%d, and determines their names.
//

void Sequencer_alsa::buildInfoDatabase(void) {
   if (initialized) {
      return;
   }

   initialized = 1;
  
   if (indevcount != 0 || outdevcount != 0) {
      std::cout << "Error: Sequencer_alsa is already running" << std::endl;
      std::cout << "Indevcout = " << indevcount << " and " 
           << " outdevcount = " << outdevcount << std::endl;
      exit(1);
   }

   indevcount  = 0;
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
   Collection<int> cards;
   Collection<int> devices;
   getPossibleMidiStreams(cards, devices);
   char devname[128] = {0};

   // check for MIDI input streams
   int i;
   for (i=0; i<cards.getSize(); i++) {
      sprintf(devname, "hw:%d,%d", cards[i], devices[i]);
      if (snd_rawmidi_open(&rawmidi_in[indevcount], NULL, devname, 0) == 0){
         midiincard.append(cards[i]);
         midiindevice.append(devices[i]);
         snd_rawmidi_close(rawmidi_in[indevcount]);
         rawmidi_in[indevcount] = NULL;
         indevcount++;
      }
   }
   for (i=0; i<rawmidi_in.getSize(); i++) {
      rawmidi_in[i] = NULL;
   }

   // check for MIDI output streams
   for (i=0; i<cards.getSize(); i++) {
      sprintf(devname, "hw:%d,%d", cards[i], devices[i]);
      if (snd_rawmidi_open(NULL, &rawmidi_out[outdevcount], devname, 0) == 0) {
         midioutcard.append(cards[i]);
         midioutdevice.append(devices[i]);
         snd_rawmidi_close(rawmidi_out[outdevcount]);
         rawmidi_out[indevcount] = NULL;
         outdevcount++;
      }
   }
   for (i=0; i<rawmidi_out.getSize(); i++) {
      rawmidi_out[i] = NULL;
   }

   char buffer[256] = {0};
   char* temp;
   for (i=0; i<indevcount; i++) {
      sprintf(buffer, "MIDI input %d: card %d, device %d", i,
            midiincard[i], midiindevice[i]);
      temp = new char[strlen(buffer) + 1];
      strcpy(temp, buffer);
      midiinname.append(temp);      
   }

   for (i=0; i<outdevcount; i++) {
      sprintf(buffer, "MIDI output %d: card %d, device %d", i,
            midioutcard[i], midioutdevice[i]);
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
// Sequencer_alsa::getInDeviceValue --
//

int Sequencer_alsa::getInDeviceValue(int aDevice) const {
   return midiindevice[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa::getInCardValue --
//

int Sequencer_alsa::getInCardValue(int aDevice) const {
   return midiincard[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa::getOutDeviceValue --
//

int Sequencer_alsa::getOutDeviceValue(int aDevice) const {
   return midioutdevice[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa::getOutCardValue --
//

int Sequencer_alsa::getOutCardValue(int aDevice) const {
   return midioutcard[aDevice];
}



//////////////////////////////
//
// Sequencer_alsa::removeInfoDatabase --
//

void Sequencer_alsa::removeInfoDatabase(void) {
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



//////////////////////////////
//
// getPossibleMidiStreams --  read the directory /dev/snd for files
//     that match the pattern midiC%dD%d, and extract the card/device
//     numbers from these filenames.
//

void Sequencer_alsa::getPossibleMidiStreams(Collection<int>& cards,
      Collection<int>& devices) {

   cards.setSize(0);
   devices.setSize(0);
   cards.allowGrowth(1);
   devices.allowGrowth(1);

   DIR* dir = opendir("/dev/snd");
   if (dir == NULL) {
//      std::cout << "Error determining ALSA MIDI info: no directory called /dev/snd"
//           << std::endl;
//      exit(1);
   }

   // read each file in the directory and store information if it is a MIDI dev
   else {
       int card;
       int device;
       struct dirent *dinfo;
       dinfo = readdir(dir);
       int count;
       while (dinfo != NULL) {
          if (strncmp(dinfo->d_name, "midi", 4) == 0) {
             count = sscanf(dinfo->d_name, "midiC%dD%d", &card, &device);
             if (count == 2) {
                cards.append(card);
                devices.append(device);
             }
          }
          dinfo = readdir(dir);
       }

       closedir(dir);
       cards.allowGrowth(0);
       devices.allowGrowth(0);
    }
}


#endif   /* LINUX and ALSA */

// md5sum:	8ccf0e750be06aeea90cdc8a7cc4499c  - Sequencer_alsa.cpp =css= 20030102
