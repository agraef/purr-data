//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Jan  3 21:02:02 PST 1999
// Last Modified: Fri Jan  8 04:50:05 PST 1999
// Last Modified: Wed May 10 17:00:11 PDT 2000 (name change from _linux to _oss)
// Filename:      ...sig/maint/code/control/MidiOutPort/Sequencer_oss.cpp
// Web Address:   http://sig.sapp.org/src/sig/Sequencer_oss.cpp
// Syntax:        C++ 
//
// Description:   Basic MIDI input/output functionality for the 
//                Linux OSS midi device /dev/sequencer.  This class
//                is inherited by the classes MidiInPort_oss and
//                MidiOutPort_oss.
//

#ifdef LINUX

#include <stdlib.h>


#include <linux/soundcard.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include "Sequencer_oss.h"


// define static variables:
const char* Sequencer_oss::sequencer       = "/dev/sequencer";
int    Sequencer_oss::sequencer_fd         = -1;
int    Sequencer_oss::class_count          =  0;
int    Sequencer_oss::initialized          =  0;
uchar  Sequencer_oss::midi_write_packet[4] = {SEQ_MIDIPUTC, 0, 0, 0};
uchar  Sequencer_oss::midi_read_packet[4];


// static variables for MIDI I/O information database
int    Sequencer_oss::indevcount      = 0;
int    Sequencer_oss::outdevcount     = 0;

int*   Sequencer_oss::indevnum        = NULL;
int*   Sequencer_oss::outdevnum       = NULL;

int*   Sequencer_oss::indevtype       = NULL;
int*   Sequencer_oss::outdevtype      = NULL;

char** Sequencer_oss::indevnames      = NULL;
char** Sequencer_oss::outdevnames     = NULL;



///////////////////////////////
//
// Sequencer_oss::Sequencer_oss --
//	default value: autoOpen = 1;
//

Sequencer_oss::Sequencer_oss(int autoOpen) {
   if (autoOpen) {
      open();
   }

   if (class_count < 0) {
      std::cerr << "Unusual class instatiation count: " << class_count << std::endl;
      exit(1);
   } else if (class_count == 0) {
      buildInfoDatabase();
   }

   class_count++;
}



//////////////////////////////
//
// Sequencer_oss::~Sequencer_oss --
//

Sequencer_oss::~Sequencer_oss() {
   class_count--;

   if (class_count == 0) {
      close();
      removeInfoDatabase();
   } else if (class_count < 0) {
      std::cerr << "Unusual class instatiation count: " << class_count << std::endl;
      exit(1);
   }
}



//////////////////////////////
//
// Sequencer_oss::close -- close the sequencer device.  The device
//   automatically closes once the program ends, but you can close it
//   so that other programs can use it.
//

void Sequencer_oss::close(void) {
   ::close(getFd());
}



//////////////////////////////
//
// Sequencer_oss::displayInputs -- display a list of the
//     available MIDI input devices.
//	default values: out = cout, initial = "\t"
//
 
void Sequencer_oss::displayInputs(std::ostream& out, char* initial) {
   for (int i=0; i<getNumInputs(); i++) {
      out << initial << i << ": " << getInputName(i) << '\n';
   }
}



//////////////////////////////
//
// Sequencer_oss::displayOutputs -- display a list of the
//     available MIDI output devices.
//	default values: out = cout, initial = "\t"
//
 
void Sequencer_oss::displayOutputs(std::ostream& out, char* initial) {
   for (int i=0; i<getNumOutputs(); i++) {
      out << initial << i << ": " << getOutputName(i) << '\n';
   }
}



//////////////////////////////
//
// Sequencer_oss::getInputName -- returns a string to the name of
//    the specified input device.  The string will remain valid as
//    long as there are any sequencer devices in existence.
//

const char* Sequencer_oss::getInputName(int aDevice) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   if (aDevice >= getNumInputs()) {
      std::cerr << "Error: " << aDevice << " is greater than max in (" 
           << getNumInputs() << ")" << std::endl;
      exit(1);
   }

   return indevnames[aDevice];
}



//////////////////////////////
//
// Sequencer_oss::getNumInputs -- returns the total number of
//     MIDI inputs that can be used.
//

int Sequencer_oss::getNumInputs(void) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return indevcount;
}



//////////////////////////////
//
// Sequencer_oss::getNumOutputs -- returns the total number of
//     MIDI inputs that can be used.
//

int Sequencer_oss::getNumOutputs(void) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   return outdevcount;
}



//////////////////////////////
//
// Sequencer_oss::getOutputName -- returns a string to the name of
//    the specified output device.  The string will remain valid as
//    long as there are any sequencer devices in existence.
//

const char* Sequencer_oss::getOutputName(int aDevice) {
   if (initialized == 0) {
      buildInfoDatabase();
   }
   if (aDevice >= getNumOutputs()) {
      std::cerr << "Error: " << aDevice << " is greater than max out (" 
           << getNumOutputs() << ")" << std::endl;
      exit(1);
   }

   return outdevnames[aDevice];
}



//////////////////////////////
//
// Sequencer_oss::is_open -- returns true if the
//     sequencer device is open, false otherwise.
//

int Sequencer_oss::is_open(void) {
   if (getFd() > 0) {
      return 1;
   } else {
      return 0;
   }
}



/////////////////////////////
//
// Sequencer_oss::open -- returns true if the device
//	was successfully opended (or already opened)
//

int Sequencer_oss::open(void) {
   if (getFd() <= 0) {
      setFd(::open(sequencer, O_RDWR, 0));
   }
   
   return is_open();
}
   


//////////////////////////////
//
// Sequencer_oss::read -- reads MIDI bytes and also stores the 
//     device from which the byte was read from.  Timing is not
//     saved from the device.  If needed, then it would have to 
//     be saved in this function, or just return the raw packet
//     data (use rawread function).
//
 
void Sequencer_oss::read(uchar* buf, uchar* dev, int count) {
   int i = 0;
   while (i < count) {
      ::read(getFd(), midi_read_packet, sizeof(midi_read_packet));
      if (midi_read_packet[1] == SEQ_MIDIPUTC) {
         buf[i] = midi_read_packet[1];
         dev[i] = midi_read_packet[2];
         i++;
      }
   }
}



//////////////////////////////
//
// Sequencer_oss::rawread -- read Input MIDI packets.
//    each packet contains 4 bytes.
//

void Sequencer_oss::rawread(uchar* buf, int packetCount) {
   ::read(getFd(), buf, packetCount * 4);
}



//////////////////////////////
//
// Sequencer_oss::rebuildInfoDatabase -- rebuild the internal
//   database that keeps track of the MIDI input and output devices.
//

void Sequencer_oss::rebuildInfoDatabase(void) {
   removeInfoDatabase();
   buildInfoDatabase();
}



///////////////////////////////
//
// Sequencer_oss::write -- Send a byte out the specified MIDI
//    port which can be either an internal or an external synthesizer.
//

int Sequencer_oss::write(int device, int aByte) {
   int status = 0;

   switch (getOutputType(device)) {
      case MIDI_EXTERNAL:
         midi_write_packet[1] = (uchar) (0xff & aByte);
         midi_write_packet[2] = getOutDeviceValue(device);
         status = ::write(getFd(), midi_write_packet,sizeof(midi_write_packet));
         break;
      case MIDI_INTERNAL:
         status = writeInternal(getOutDeviceValue(device), aByte);
         break;
   }

   if (status > 0) {
      return 1;
   } else {
      return 0;
   }

}


int Sequencer_oss::write(int device, uchar* bytes, int count) {
   int status = 1;
   for (int i=0; i<count; i++) {
      status &= write(device, bytes[i]);
   }
   return status;
}


int Sequencer_oss::write(int device, char* bytes, int count) {
   return write(device, (uchar*)bytes, count);
}


int Sequencer_oss::write(int device, int* bytes, int count) {
   int status = 1;
   for (int i=0; i<count; i++) {
      status &= write(device, bytes[i]);
   }
   return status;
}



///////////////////////////////////////////////////////////////////////////
//
// private functions
//

//////////////////////////////
//
// Sequencer_oss::buildInfoDatabase -- determines the number
//     of MIDI input and output devices available from
//     /dev/sequencer, and determines their names.
//

void Sequencer_oss::buildInfoDatabase(void) {
   int status;
   initialized = 1;
  
   int startup = sequencer_fd == -1 ? 0 : 1;
   if (startup == 0) {
      // setup the file descriptor for /dev/sequencer
      sequencer_fd = ::open(sequencer, O_RDWR);
      if (sequencer_fd < 0) {
//         std::cout << "Error: cannot open " << sequencer << std::endl;
//         exit(1);
      }
      else {
         // read number of inputs available (external MIDI devices only)
         status = ioctl(getFd(), SNDCTL_SEQ_NRMIDIS, &indevcount);
         if (status!= 0) {
            std::cerr << "Error determining the number of MIDI inputs" << std::endl;
            exit(1);
         }

         // read number of output available
         int extmidi = indevcount;
         int intmidi;
         status = ioctl(getFd(), SNDCTL_SEQ_NRSYNTHS, &intmidi);
         if (status!= 0) {
            std::cerr << "Error determining the number of MIDI inputs" << std::endl;
            exit(1);
         }
         outdevcount = extmidi + intmidi;
   
         // allocate space for names and device number arrays
         if (indevnum != NULL || outdevnum != NULL || indevnames != NULL ||
               outdevnames != NULL || indevtype != NULL || outdevtype != NULL) {
            std::cerr << "Error: buildInfoDatabase called twice." << std::endl;
            exit(1);
         } 

         indevnum = new int[indevcount];
         outdevnum = new int[outdevcount];

         indevtype = new int[indevcount];
         outdevtype = new int[outdevcount];
   
         indevnames = new char*[indevcount];
         outdevnames = new char*[outdevcount];


         // fill in the device translation table and fill in the device names
         int i;
         struct midi_info midiinfo;
         for (i=0; i<indevcount; i++) {
            midiinfo.device = i;
            status = ioctl(getFd(), SNDCTL_MIDI_INFO, &midiinfo);
            if (status != 0) {
               std::cerr << "Error while reading MIDI device " << i << std::endl;
               exit(1);
            }

            indevnum[i]    = midiinfo.device;
            outdevnum[i]   = midiinfo.device;
            indevtype[i]   = MIDI_EXTERNAL;
            outdevtype[i]  = MIDI_EXTERNAL;
            indevnames[i]  = new char[strlen(midiinfo.name) + 1 + 10];
            outdevnames[i] = new char[strlen(midiinfo.name) + 1 + 11];
            strcpy(indevnames[i], midiinfo.name);
            strcpy(outdevnames[i], midiinfo.name);
            strcat(indevnames[i], " (MIDI In)");
            strcat(outdevnames[i], " (MIDI Out)");
         }

         char tempstring[1024] = {0};
         struct synth_info synthinfo;
         for (i=0; i<intmidi; i++) {
            synthinfo.device = i;
            status = ioctl(getFd(), SNDCTL_SYNTH_INFO, &synthinfo);
            if (status != 0) {
               std::cerr << "Error while reading MIDI device " << i << std::endl;
               exit(1);
            }
            outdevnum[extmidi+i] = i;
            outdevtype[extmidi + i] = MIDI_INTERNAL;
   
            strcpy(tempstring, synthinfo.name);
            switch (synthinfo.synth_type) {
               case SYNTH_TYPE_FM:           // 0
                  strcat(tempstring, " (FM");
                  switch (synthinfo.synth_subtype) {
                     case FM_TYPE_ADLIB:     // 0
                        strcat(tempstring, " Adlib");
                        break;
                     case FM_TYPE_OPL3:      // 1
                        strcat(tempstring, " OPL3");
                        break;
                  }
                  strcat(tempstring, ")");
                  break;
               case SYNTH_TYPE_SAMPLE:       // 1
                  strcat(tempstring, " (Wavetable)");
                  break;
               case SYNTH_TYPE_MIDI:         // 2
                  strcat(tempstring, " (MIDI Interface");
                  switch (synthinfo.synth_subtype) {
                     case SYNTH_TYPE_MIDI:   // 0x401
                        strcat(tempstring, " MPU401");
                        break;
                  }
                  strcat(tempstring, ")");
                  break;
            }         
            outdevnames[i+extmidi] = new char[strlen(tempstring) + 1];
            strcpy(outdevnames[i+extmidi], tempstring);
         }


         if (startup == 0) {
            ::close(sequencer_fd);
         }
      }
   }
}


//////////////////////////////
//
// Sequencer_oss::getFd -- returns the file descriptor of the
//     sequencer device.
//

int Sequencer_oss::getFd(void) {
   return sequencer_fd;
}



//////////////////////////////
//
// Sequencer_oss::getInDeviceValue --
//

int Sequencer_oss::getInDeviceValue(int aDevice) const {
   if (aDevice >= getNumInputs()) {
      std::cerr << "Error: " << aDevice << " is greater than max in (" 
           << getNumInputs() << ")" << std::endl;
      exit(1);
   }

   return indevnum[aDevice];
}



//////////////////////////////
//
// Sequencer_oss::getInputType -- returns 1 = external MIDI,
//     2 = internal MIDI
//

int Sequencer_oss::getInputType(int aDevice) const {
   if (aDevice >= getNumInputs()) {
      std::cerr << "Error: " << aDevice << " is greater than max in (" 
           << getNumInputs() << ")" << std::endl;
      exit(1);
   }

   return indevtype[aDevice];
}



//////////////////////////////
//
// Sequencer_oss::getOutDeviceValue --
//

int Sequencer_oss::getOutDeviceValue(int aDevice) const {
   if (aDevice >= getNumOutputs()) {
      std::cerr << "Error: " << aDevice << " is greater than max out (" 
           << getNumOutputs() << ")" << std::endl;
      exit(1);
   }

   return outdevnum[aDevice];
}



//////////////////////////////
//
// Sequencer_oss::getOutputType -- returns 1 = external MIDI,
//     2 = internal MIDI
//

int Sequencer_oss::getOutputType(int aDevice) const {
   if (aDevice >= getNumOutputs()) {
      std::cerr << "Error: " << aDevice << " is greater than max out (" 
           << getNumOutputs() << ")" << std::endl;
      exit(1);
   }

   return outdevtype[aDevice];
}



//////////////////////////////
//
// Sequencer_oss::removeInfoDatabase --
//

void Sequencer_oss::removeInfoDatabase(void) {
   initialized = 0;

   if (indevnum   != NULL)   delete [] indevnum;
   if (outdevnum  != NULL)   delete [] outdevnum;
   if (indevtype  != NULL)   delete [] indevtype;
   if (outdevtype != NULL)   delete [] outdevtype;
  
   int i;
   if (indevnames != NULL) {
      for (i=0; i<indevcount; i++) {
         if (indevnames[i] != NULL)    delete [] indevnames[i];
      }
      delete [] indevnames;
   }

   if (outdevnames != NULL) {
      for (i=0; i<outdevcount; i++) {
         if (outdevnames[i] != NULL)   delete [] outdevnames[i];
      }
      delete [] outdevnames;
   }
 
   indevnum    = NULL;
   outdevnum   = NULL;
   indevtype   = NULL;
   outdevtype  = NULL;
   indevnames  = NULL;
   outdevnames = NULL;

   indevcount = 0;
   outdevcount = 0;
}



//////////////////////////////
//
// Sequencer_oss::setFd --
//

void Sequencer_oss::setFd(int anFd) {
   sequencer_fd = anFd;
}




///////////////////////////////////////////////////////////////////////////
//
// private functions dealing with the stupid internal sythesizer messages
//   which have to be processed as complete messages as opposed to 
//   external MIDI devices which are processed on the driver level as
//   discrete bytes.
// 

// static variables related to the processing of message for internal MIDI:
uchar  Sequencer_oss::synth_write_message[8];
uchar  Sequencer_oss::synth_message_buffer[1024]   = {0};
int    Sequencer_oss::synth_message_buffer_count   =  0;
int    Sequencer_oss::synth_message_bytes_expected =  0;
int    Sequencer_oss::synth_message_curr_device    =  -1;


//////////////////////////////
//
// Sequencer_oss::writeInternal -- the device number is the 
//     driver's device number *NOT* this class's device numbering
//     system.  MIDI bytes are stored in a buffer until a complete
//     message is received, then a synth message is generated.
//     While a complete message is being received, the device number
//     cannot change.  The first byte of a message must be a MIDI
//     command (i.e., no running status). 
//

int Sequencer_oss::writeInternal(int aDevice, int aByte) {
   int status = 0;

   if (synth_message_bytes_expected == 0) {
      // a new message is coming in.
      synth_message_curr_device = aDevice;
      if (aByte < 128) {
         std::cerr << "Error: MIDI output byte: " << aByte 
              << " is not a command byte." << std::endl;
         exit(1);
      } else {
         synth_message_buffer[0] = aByte;
         synth_message_buffer_count = 1;
      }

      switch (aByte & 0xf0) {
         case 0x80:   synth_message_bytes_expected = 3;   break;
         case 0x90:   synth_message_bytes_expected = 3;   break;
         case 0xA0:   synth_message_bytes_expected = 3;   break;
         case 0xB0:   synth_message_bytes_expected = 3;   break;
         case 0xC0:   synth_message_bytes_expected = 2;   break;
         case 0xD0:   synth_message_bytes_expected = 2;   break;
         case 0xE0:   synth_message_bytes_expected = 3;   break;
         case 0xF0:   std::cerr << "Can't handle 0xE0 yet" << std::endl;   exit(1);
         default:     std::cerr << "Unknown error" << std::endl;   exit(1);
      }
   }

   // otherwise expecting at least one more byte for the MIDI message
   else {
      if (synth_message_curr_device != aDevice) {
         std::cerr << "Error: device number changed during message" << std::endl;
         exit(1);
      }
      if (aByte > 127) {
         std::cerr << "Error: expecting MIDI data but got MIDI command: "
              << aByte << std::endl;
         exit(1);
      }

      synth_message_buffer[synth_message_buffer_count++] = aByte;
   }

   // check to see if the message is complete:
   if (synth_message_bytes_expected == synth_message_buffer_count) {
      status = transmitMessageToInternalSynth();
      synth_message_bytes_expected = 0;
      synth_message_buffer_count = 0;
   }

   return status;
}

     

//////////////////////////////
//
// Sequencer_oss::transmitMessageToInternalSynth -- send the stored
//    MIDI message to the internal synthesizer.
//

int Sequencer_oss::transmitMessageToInternalSynth(void) {
   int status;
   switch (synth_message_buffer[0] & 0xf0) {
      case 0x80:                      // Note-off
      case 0x90:                      // Note-on
      case 0xA0:                      // Aftertouch
         status = transmitVoiceMessage();
         break;
      case 0xB0:                      // Control change
      case 0xC0:                      // Patch change
      case 0xD0:                      // Channel pressure
      case 0xE0:                      // Pitch wheel
         status = transmitCommonMessage();
         break;
      case 0xF0:
         std::cerr << "Cannot handle 0xf0 commands yet" << std::endl;
         exit(1);
         break;
      default:
         std::cerr << "Error: unknown MIDI command" << std::endl;
         exit(1);
   }

   return status;
}



//////////////////////////////
//
// Sequencer_oss::transmitVoiceMessage -- send a voice-type MIDI
//     message to an internal synthesizer.
//

int Sequencer_oss::transmitVoiceMessage(void) {
   synth_write_message[0] = EV_CHN_VOICE;
   synth_write_message[1] = synth_message_curr_device;
   synth_write_message[2] = synth_message_buffer[0] & 0xf0;
   synth_write_message[3] = synth_message_buffer[0] & 0x0f;
   synth_write_message[4] = synth_message_buffer[1];
   synth_write_message[5] = synth_message_buffer[2];
   synth_write_message[6] = 0;
   synth_write_message[7] = 0;

   int status;
   status = ::write(getFd(), synth_write_message, sizeof(synth_write_message));
 
   if (status > 0) {
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// Sequencer_oss::transmitCommonMessage -- send a common-type MIDI
//     message to an internal synthesizer.
//

int Sequencer_oss::transmitCommonMessage(void) {
   synth_write_message[0] = EV_CHN_COMMON;
   synth_write_message[1] = synth_message_curr_device;
   synth_write_message[2] = synth_message_buffer[0] & 0xf0;
   synth_write_message[3] = synth_message_buffer[0] & 0x0f;

   switch (synth_write_message[2]) {
      case 0xB0:                           // Control change
         synth_write_message[4] = synth_message_buffer[1];
         synth_write_message[5] = 0;
         synth_write_message[6] = synth_message_buffer[2];
         synth_write_message[7] = 0;
         break;
      case 0xC0:                           // Patch change
      case 0xD0:                           // Channel pressure
         synth_write_message[4] = synth_message_buffer[1];
         synth_write_message[5] = 0;
         synth_write_message[6] = 0;
         synth_write_message[7] = 0;
         break;
      case 0xE0:                           // Pitch wheel
         synth_write_message[4] = 0;
         synth_write_message[5] = 0;
         synth_write_message[6] = synth_message_buffer[1];
         synth_write_message[7] = synth_message_buffer[2];
         break;
      default:
         std::cerr << "Unknown Common MIDI message" << std::endl;
         exit(1);
   }

   int status;
   status = ::write(getFd(), synth_write_message, sizeof(synth_write_message));

   if (status > 0) {
      return 1;
   } else {
      return 0;
   }
}



#endif  // LINUX
// md5sum:	bc7b96041137b22f3d3c35376b5912c6  - Sequencer_oss.cpp =css= 20030102
