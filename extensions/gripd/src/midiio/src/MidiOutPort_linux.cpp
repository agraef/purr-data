//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun May 14 20:58:32 PDT 2000
// Last Modified: Thu May 18 23:37:11 PDT 2000
// Last Modified: Sat Nov  2 20:40:01 PST 2002 (added ALSA OSS def)
// Filename:      ...sig/code/control/MidiOutPort_linux/MidiOutPort_linux.cpp
// Web Address:   http://sig.sapp.org/include/sig/MidiOutPort_linux.cpp
// Syntax:        C++ 
//
// Description:   Linux MIDI output class which detects which
//                type of MIDI drivers are available: either
//                ALSA or OSS. 
//

#ifdef LINUX
#if defined(ALSA) && defined(OSS)

#include "MidiOutPort_linux.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>


// initialize static members
int MidiOutPort_linux::objectCount = 0;
int MidiOutPort_linux::current   = UNKNOWN_MIDI_SELECT; // MIDI out selected
int MidiOutPort_linux::alsaQ     = 0;    // boolean for if ALSA is present
int MidiOutPort_linux::ossQ      = 0;    // boolean for if OSS is present

MidiOutPort_oss         *MidiOutPort_linux::oss_output = NULL;
MidiOutPort_alsa        *MidiOutPort_linux::alsa_output = NULL;
MidiOutPort_unsupported *MidiOutPort_linux::unknown_output = NULL;


MidiOutPort_linux::MidiOutPort_linux(void) {
   if (objectCount == 0) {
      determineDrivers();
   } else if (objectCount < 0) {
      cout << "Error: unusual MidiOutPort_linux object count" << endl;
      exit(1);
   } 

   objectCount++;
} 

MidiOutPort_linux::MidiOutPort_linux(int aPort, int autoOpen = 1) {
   determineDrivers();
   setAndOpenPort(aPort);
} 

MidiOutPort_linux::~MidiOutPort_linux() {
   objectCount--;
   if (objectCount == 0) {
      if (oss_output != NULL) {
         delete oss_output;
         oss_output = NULL;
      }
   
      if (alsa_output != NULL) {
         delete alsa_output;
         alsa_output = NULL;
      }
   
      if (unknown_output != NULL) {
         delete unknown_output;
         unknown_output = NULL;
      }
   }

   if (objectCount < 0) {
      cout << "Error: unusual MidiOutPort_linux count when destructing" << endl;
      exit(1);
   }
} 


void MidiOutPort_linux::close(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:       oss_output->close();       break;
      case ALSA_MIDI_SELECT:      alsa_output->close();      break;
      default:                    unknown_output->close();   break;
   }
}

void MidiOutPort_linux::closeAll(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:       oss_output->closeAll();       break;
      case ALSA_MIDI_SELECT:      alsa_output->closeAll();      break;
      default:                    unknown_output->closeAll();   break;
   }
} 

int MidiOutPort_linux::getChannelOffset(void) const {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->getChannelOffset();     break;
      case ALSA_MIDI_SELECT: return alsa_output->getChannelOffset();    break;
      default:               return unknown_output->getChannelOffset(); break;
   }
} 

const char* MidiOutPort_linux::getName(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->getName();     break;
      case ALSA_MIDI_SELECT: return alsa_output->getName();    break;
      default:               return unknown_output->getName(); break;
   }
} 

const char* MidiOutPort_linux::getName(int i) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->getName(i);     break;
      case ALSA_MIDI_SELECT: return alsa_output->getName(i);    break;
      default:               return unknown_output->getName(i); break;
   }
} 

int MidiOutPort_linux::getNumPorts(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->getNumPorts();     break;
      case ALSA_MIDI_SELECT: return alsa_output->getNumPorts();    break;
      default:               return unknown_output->getNumPorts(); break;
   }
} 

int MidiOutPort_linux::getPort(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->getPort();     break;
      case ALSA_MIDI_SELECT: return alsa_output->getPort();    break;
      default:               return unknown_output->getPort(); break;
   }
} 

int MidiOutPort_linux::getPortStatus(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->getPortStatus();     break;
      case ALSA_MIDI_SELECT: return alsa_output->getPortStatus();    break;
      default:               return unknown_output->getPortStatus(); break;
   }
} 

int MidiOutPort_linux::getTrace(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->getTrace();     break;
      case ALSA_MIDI_SELECT: return alsa_output->getTrace();    break;
      default:               return unknown_output->getTrace(); break;
   }
} 

int MidiOutPort_linux::open(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->open();     break;
      case ALSA_MIDI_SELECT: return alsa_output->open();    break;
      default:               return unknown_output->open(); break;
   }
} 

int MidiOutPort_linux::rawsend(int command, int p1, int p2) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->rawsend(command, p1, p2);     break;
      case ALSA_MIDI_SELECT: return alsa_output->rawsend(command, p1, p2);    break;
      default:               return unknown_output->rawsend(command, p1, p2); break;
   }
} 

int MidiOutPort_linux::rawsend(int command, int p1) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->rawsend(command, p1);     break;
      case ALSA_MIDI_SELECT: return alsa_output->rawsend(command, p1);    break;
      default:               return unknown_output->rawsend(command, p1); break;
   }
} 

int MidiOutPort_linux::rawsend(int command) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->rawsend(command);     break;
      case ALSA_MIDI_SELECT: return alsa_output->rawsend(command);    break;
      default:               return unknown_output->rawsend(command); break;
   }
} 

int MidiOutPort_linux::rawsend(uchar* array, int size) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->rawsend(array, size);     break;
      case ALSA_MIDI_SELECT: return alsa_output->rawsend(array, size);    break;
      default:               return unknown_output->rawsend(array, size); break;
   }
} 

void MidiOutPort_linux::setAndOpenPort(int aPort) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  
         oss_output->setPort(aPort);
         oss_output->open();
         break;
      case ALSA_MIDI_SELECT: 
         alsa_output->setPort(aPort);
         alsa_output->open();
         break;
      default:
         unknown_output->setPort(aPort); 
         unknown_output->open(); 
         break;
   }
} 

void MidiOutPort_linux::setChannelOffset(int aChannel) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  oss_output->setChannelOffset(aChannel);     break;
      case ALSA_MIDI_SELECT: alsa_output->setChannelOffset(aChannel);    break;
      default:               unknown_output->setChannelOffset(aChannel); break;
   }
} 

void MidiOutPort_linux::setPort(int aPort) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  oss_output->setPort(aPort);     break;
      case ALSA_MIDI_SELECT: 
      alsa_output->setPort(aPort);    break;
      default:               unknown_output->setPort(aPort); break;
   }
} 

int MidiOutPort_linux::setTrace(int aState) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->setTrace(aState);      break;
      case ALSA_MIDI_SELECT: return alsa_output->setTrace(aState);     break;
      default:               return unknown_output->setTrace(aState);  break;
   }
} 

int MidiOutPort_linux::sysex(uchar* array, int size) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  return oss_output->sysex(array, size);     break;
      case ALSA_MIDI_SELECT: return alsa_output->sysex(array, size);    break;
      default:               return unknown_output->sysex(array, size); break;
   }
} 

void MidiOutPort_linux::toggleTrace(void) {
   switch (getSelect()) {
      case OSS_MIDI_SELECT:  oss_output->toggleTrace();       break;
      case ALSA_MIDI_SELECT: alsa_output->toggleTrace();      break;
      default:               unknown_output->toggleTrace();   break;
   }
} 



//////////////////////////////
//
// MidiOutPort_linux::getSelect -- return the type of MIDI which
//      is being used to send MIDI output.
//

int MidiOutPort_linux::getSelect(void) {
   return current;
}



//////////////////////////////
//
// MidiOutPort_linux::selectOSS -- select the OSS MIDI output
//   returns 1 if OSS is available, otherwise returns 0.
//

int MidiOutPort_linux::selectOSS(void) {
   if (ossQ) {
      current = OSS_MIDI_SELECT;
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiOutPort_linux::selectALSA -- select the ALSA MIDI output
//   returns 1 if ALSA is available, otherwise returns 0.
//

int MidiOutPort_linux::selectALSA(void) {
   if (alsaQ) {
      current = ALSA_MIDI_SELECT;
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiOutPort_linux::selectUnknown -- select the Unknown MIDI output
//   returns 1 always.
//

int MidiOutPort_linux::selectUnknown(void) {
   current = UNKNOWN_MIDI_SELECT;
   return 1;
}



///////////////////////////////////////////////////////////////////////////
//
// Private Functions
//

#include <unistd.h>

//////////////////////////////
//
// MidiOutPort_linux::determineDrivers -- see if OSS/ALSA are
//      available.  If /dev/sequencer is present, assume that OSS is
//      available.  If /dev/snd/sdq is present, assume that ALSA is
//      available.  
//

void MidiOutPort_linux::determineDrivers(void) {
   struct stat filestats;
   int status;
   status = stat("/dev/sequencer", &filestats);

   if (status != 0) {
      ossQ = 0;
   } else {
      ossQ = 1;
   }

   status = stat("/dev/snd/seq", &filestats);

   if (status != 0) {
      alsaQ = 0;
   } else {
      alsaQ = 1;
   }


   current = UNKNOWN_MIDI_SELECT;

   if (ossQ) {
      current = OSS_MIDI_SELECT;
   }

   if (alsaQ) {
      current = ALSA_MIDI_SELECT;
   }

   // create MIDI output types which are available:

   if (oss_output != NULL) {
      delete oss_output;
      oss_output = NULL;
   }
   if (alsa_output != NULL) {
      delete alsa_output;
      alsa_output = NULL;
   }
   if (unknown_output != NULL) {
      delete unknown_output;
      unknown_output = NULL;
   }

   if (ossQ) {
      oss_output = new MidiOutPort_oss;
   }
   if (alsaQ) {
      alsa_output = new MidiOutPort_alsa;
   }
   unknown_output = new MidiOutPort_unsupported;
}


#endif /* ALSA and OSS */
#endif /* LINUX */

// md5sum:	be1ccf667122f1c9cf56a95b2ffb8e79  - MidiOutPort_linux.cpp =css= 20030102
