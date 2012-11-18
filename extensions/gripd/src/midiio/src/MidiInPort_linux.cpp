//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun May 14 22:35:27 PDT 2000
// Last Modified: Thu May 18 23:36:00 PDT 2000
// Last Modified: Sat Nov  2 20:37:49 PST 2002 (added ifdef ALSA OSS)
// Filename:      ...sig/maint/code/control/MidiInPort/MidiInPort_linux.cpp
// Web Address:   http://sig.sapp.org/include/sig/MidiInPort_linux.cpp
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of an
//                operating-system specific MIDI input method.
//                Provides control of all low-level MIDI input
//                functionality such that it will work on all
//                computers in the same manner.
//

#ifdef LINUX
#if defined(ALSA) && defined(OSS)

#include <sys/types.h>
#include <sys/stat.h>
#include "MidiInPort_linux.h"


// initialize static variables oss_input->close(); break;
int MidiInPort_linux::objectCount = 0;
int MidiInPort_linux::current = MIDI_IN_OSS_SELECT; 
int MidiInPort_linux::alsaQ   = 0;         
int MidiInPort_linux::ossQ    = 0;          
MidiInPort_oss          *MidiInPort_linux::oss_input     = NULL;
MidiInPort_alsa         *MidiInPort_linux::alsa_input    = NULL;
MidiInPort_unsupported  *MidiInPort_linux::unknown_input = NULL;


MidiInPort_linux::MidiInPort_linux(void) {
   if (objectCount == 0) {
      determineDrivers();
   } else if (objectCount < 0) {
      cout << "Error: unusual MidiInPort_linux object count" << endl;
      exit(1);
   } 

   objectCount++;
} 


MidiInPort_linux::MidiInPort_linux(int aPort, int autoOpen = 1) {
   if (objectCount == 0) {
      determineDrivers();
      setAndOpenPort(aPort);
   } else if (objectCount < 0) {
      cout << "Error: unusual MidiInPort_linux object count" << endl;
      exit(1);
   } 

   objectCount++;
} 


MidiInPort_linux::~MidiInPort_linux() {
   objectCount--;
   if (objectCount == 0) {
      if (oss_input != NULL) {
         delete oss_input;
         oss_input = NULL;
      }
      if (alsa_input != NULL) {
         delete alsa_input;
         alsa_input = NULL;
      }
      if (unknown_input != NULL) {
         delete unknown_input;
         unknown_input = NULL;
      }
   }

   if (objectCount < 0) {
      cout << "Error: unusual MidiOutPort_linux count when destructing" << endl;
      exit(1);
   }
} 


void MidiInPort_linux::clearSysex(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->clearSysex(); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->clearSysex(); break;
      default:                  unknown_input->clearSysex(); break;
   }
}


void MidiInPort_linux::clearSysex(int buffer) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->clearSysex(); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->clearSysex(); break;
      default:                  unknown_input->clearSysex(); break;
   }
}


void MidiInPort_linux::close(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->close(); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->close(); break;
      default:                  unknown_input->close(); break;
   }
}


void MidiInPort_linux::closeAll(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->closeAll(); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->closeAll(); break;
      default:                  unknown_input->closeAll(); break;
   }
}


MidiMessage MidiInPort_linux::extract(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->extract(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->extract(); break;
      default:                  return unknown_input->extract(); break;
   }
}


int MidiInPort_linux::getBufferSize(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getBufferSize(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getBufferSize(); break;
      default:                  return unknown_input->getBufferSize(); break;
   }
}


int MidiInPort_linux::getChannelOffset(void) const {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getChannelOffset(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getChannelOffset(); break;
      default:                  return unknown_input->getChannelOffset(); break;
   }
}


int MidiInPort_linux::getCount(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getCount(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getCount(); break;
      default:                  return unknown_input->getCount(); break;
   }
}


const char* MidiInPort_linux::getName(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getName(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getName(); break;
      default:                  return unknown_input->getName(); break;
   }
}


const char* MidiInPort_linux::getName(int i) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getName(i); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getName(i); break;
      default:                  return unknown_input->getName(i); break;
   }
}


int MidiInPort_linux::getNumPorts(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getNumPorts(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getNumPorts(); break;
      default:                  return unknown_input->getNumPorts(); break;
   }
}


int MidiInPort_linux::getPort(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getPort(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getPort(); break;
      default:                  return unknown_input->getPort(); break;
   }
}


int MidiInPort_linux::getPortStatus(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getPortStatus(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getPortStatus(); break;
      default:                  return unknown_input->getPortStatus(); break;
   }
}


uchar* MidiInPort_linux::getSysex(int buffer) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getSysex(buffer); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getSysex(buffer); break;
      default:                  return unknown_input->getSysex(buffer); break;
   }
}


int MidiInPort_linux::getSysexSize(int buffer) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getSysexSize(buffer); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getSysexSize(buffer); break;
      default: return unknown_input->getSysexSize(buffer); break;
   }
}


int MidiInPort_linux::getTrace(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->getTrace(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->getTrace(); break;
      default:                  return unknown_input->getTrace(); break;
   }
}


void MidiInPort_linux::insert(const MidiMessage& aMessage) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->insert(aMessage); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->insert(aMessage); break;
      default:                  unknown_input->insert(aMessage); break;
   }
}


int MidiInPort_linux::installSysex(uchar* anArray, int aSize) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  
         return oss_input->installSysex(anArray, aSize);
         break;
      case MIDI_IN_ALSA_SELECT: 
         return alsa_input->installSysex(anArray, aSize); 
         break;
      default:                  
         return unknown_input->installSysex(anArray, aSize); 
         break;
   }
}


MidiMessage& MidiInPort_linux::message(int index) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->message(index); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->message(index); break;
      default:                  return unknown_input->message(index); break;
   }
}

int MidiInPort_linux::open(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->open(); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->open(); break;
      default:                  return unknown_input->open(); break;
   }
}


MidiMessage& MidiInPort_linux::operator[](int index) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->message(index); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->message(index); break;
      default:                  return unknown_input->message(index); break;
   }
}


void MidiInPort_linux::pause(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->pause(); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->pause(); break;
      default:                  unknown_input->pause(); break;
   }
}


void MidiInPort_linux::setBufferSize(int aSize) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->setBufferSize(aSize); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->setBufferSize(aSize); break;
      default:                  unknown_input->setBufferSize(aSize); break;
   }
}


void MidiInPort_linux::setChannelOffset(int anOffset) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->setChannelOffset(anOffset); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->setChannelOffset(anOffset); break;
      default:                  unknown_input->setChannelOffset(anOffset); break;
   }
}


void MidiInPort_linux::setAndOpenPort(int aPort) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  
         oss_input->setPort(aPort); 
         oss_input->open();
         break;
      case MIDI_IN_ALSA_SELECT:
         alsa_input->setPort(aPort); 
         alsa_input->open();
         break;
      default:
         unknown_input->setPort(aPort); 
         unknown_input->open();
         break;
   }
}


void MidiInPort_linux::setPort(int aPort) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->setPort(aPort); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->setPort(aPort); break;
      default:                  unknown_input->setPort(aPort); break;
   }
}


int MidiInPort_linux::setTrace(int aState) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  return oss_input->setTrace(aState); break;
      case MIDI_IN_ALSA_SELECT: return alsa_input->setTrace(aState); break;
      default:                  return unknown_input->setTrace(aState); break;
   }
}


void MidiInPort_linux::toggleTrace(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->toggleTrace(); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->toggleTrace(); break;
      default:                  unknown_input->toggleTrace(); break;
   }
}


void MidiInPort_linux::unpause(void) {
   switch (getSelect()) {
      case MIDI_IN_OSS_SELECT:  oss_input->unpause(); break;
      case MIDI_IN_ALSA_SELECT: alsa_input->unpause(); break;
      default:                  unknown_input->unpause(); break;
   }
}



//////////////////////////////
//
// MidiInPort_linux::getSelect -- return the type of MIDI which
//      is being used to send MIDI output.
//

int MidiInPort_linux::getSelect(void) {
   return current;
}


//////////////////////////////
//
// MidiInPort_linux::selectOSS -- select the OSS MIDI output
//   returns 1 if OSS is available, otherwise returns 0.
//

int MidiInPort_linux::selectOSS(void) {
   if (ossQ) {
      current = MIDI_IN_OSS_SELECT;
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiInPort_linux::selectALSA -- select the ALSA MIDI output
//   returns 1 if ALSA is available, otherwise returns 0.
//

int MidiInPort_linux::selectALSA(void) {
   if (alsaQ) {
      current = MIDI_IN_ALSA_SELECT;
      return 1;
   } else {
      return 0;
   }
}



//////////////////////////////
//
// MidiInPort_linux::selectUnknown -- select the Unknown MIDI output
//   returns 1 always.
//

int MidiInPort_linux::selectUnknown(void) {
   current = MIDI_IN_UNKNOWN_SELECT;
   return 1;
}


//////////////////////////////////////////////////////////////////////////
//
// Private Functions
//

//////////////////////////////
//
// MidiInPort_linux::determineDrivers -- see if OSS/ALSA are
//      available.  If /dev/sequencer is present, assume that OSS is
//      available.  If /dev/snd/sdq is present, assume that ALSA is
//      available.  
//

void MidiInPort_linux::determineDrivers(void) {
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


   current = MIDI_IN_UNKNOWN_SELECT;

   if (ossQ) {
      current = MIDI_IN_OSS_SELECT;
   }

   if (alsaQ) {
      current = MIDI_IN_ALSA_SELECT;
   }

   // create MIDI output types which are available:

   if (oss_input != NULL) {
      delete oss_input;
      oss_input = NULL;
   }
   if (alsa_input != NULL) {
      delete alsa_input;
      alsa_input = NULL;
   }
   if (unknown_input != NULL) {
      delete unknown_input;
      unknown_input = NULL;
   }

   if (ossQ) {
      oss_input = new MidiInPort_oss;
   }
   if (alsaQ) {
      alsa_input = new MidiInPort_alsa;
   }

   unknown_input = new MidiInPort_unsupported;

}


#endif /* ALSA and OSS */
#endif /* LINUX */


// md5sum:	d634dd5c3b7e8c4d75b99d7459c3f073  - MidiInPort_linux.cpp =css= 20030102
