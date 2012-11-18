//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Jan 21 22:35:31 GMT-0800 1998
// Last Modified: Thu Jan 22 23:13:54 GMT-0800 1998
// Last Modified: Sat Nov  7 16:09:18 PST 1998
// Last Modified: Tue Jun 29 16:14:50 PDT 1999 (added Sysex input)
// Last Modified: Tue May 23 23:08:44 PDT 2000 (oss/alsa selection added)
// Filename:      ...sig/maint/code/control/MidiInPort/MidiInPort.h
// Web Address:   http://sig.sapp.org/include/sig/MidiInPort.h
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of an
//                operating-system specific MIDI input method.
//                Provides control of all low-level MIDI input
//                functionality such that it will work on all
//                computers in the same manner.
//

#ifndef _MIDIINPORT_H_INCLUDED
#define _MIDIINPORT_H_INCLUDED


#include "MidiMessage.h"

#ifdef VISUAL
   #define MIDIINPORT  MidiInPort_visual
   #include "MidiInPort_visual.h"
#elif defined(LINUX) && defined(ALSA) && defined(OSS)
   #define MIDIINPORT  MidiInPort_linux
   #include "MidiInPort_linux.h"
#elif defined(LINUX) && defined(ALSA) && !defined(OSS)
   #define MIDIINPORT  MidiInPort_alsa
   #include "MidiInPort_alsa.h"
#elif defined (LINUX) && defined(OSS) && !defined(ALSA)
   #define MIDIINPORT  MidiInPort_oss
   #include "MidiInPort_oss.h"
#elif defined(LINUX)
   #define MIDIINPORT  MidiInPort_oss
   #include "MidiInPort_oss.h"
#else
   #define MIDIINPORT  MidiInPort_unsupported
   #include "MidiInPort_unsupported.h"
#endif


class MidiInPort : protected MIDIINPORT {
   public:
                  MidiInPort         (void) : MIDIINPORT() {}
                  MidiInPort         (int aPort, int autoOpen = 1) : 
                                        MIDIINPORT(aPort, autoOpen) {}
                 ~MidiInPort()       { }

      void        clearSysex(void) { MIDIINPORT::clearSysex(); }
      void        clearSysex(int buffer) { MIDIINPORT::clearSysex(buffer); }
      void        close(void)        { MIDIINPORT::close(); }
      void        closeAll(void)     { MIDIINPORT::closeAll(); }
      MidiMessage extract(void)      { return MIDIINPORT::extract(); }
      int         getBufferSize(void) { return MIDIINPORT::getBufferSize(); }
      int         getChannelOffset(void) const { 
                                        return MIDIINPORT::getChannelOffset(); }
      int         getCount(void)     { return MIDIINPORT::getCount(); }
      const char* getName(void)      { return MIDIINPORT::getName(); }
      static const char* getName(int i)  { return MIDIINPORT::getName(i); }
      static int  getNumPorts(void) { 
                     return MIDIINPORT::getNumPorts(); }
      int         getPort(void)      { return MIDIINPORT::getPort(); }
      int         getPortStatus(void){ 
                     return MIDIINPORT::getPortStatus(); }
      uchar*      getSysex(int buffer) { return MIDIINPORT::getSysex(buffer); }
      int getSysexSize(int buffer) { return MIDIINPORT::getSysexSize(buffer); }
      int         getTrace(void)     { return MIDIINPORT::getTrace(); }
      void        insert(const MidiMessage& aMessage) {
                     MIDIINPORT::insert(aMessage); }
      int         installSysex(uchar* anArray, int aSize) {
                     return MIDIINPORT::installSysex(anArray, aSize); }
      int         open(void)         { return MIDIINPORT::open(); }
      MidiMessage& operator[](int index) {
                     return MIDIINPORT::message(index); }
      void        pause(void)        { MIDIINPORT::pause(); }
      void        setBufferSize(int aSize) {
                     MIDIINPORT::setBufferSize(aSize); }
      void        setChannelOffset(int anOffset) { 
                     MIDIINPORT::setChannelOffset(anOffset); }
      void        setAndOpenPort(int aPort) { setPort(aPort); open(); }
      void        setPort(int aPort) { MIDIINPORT::setPort(aPort); }
      int         setTrace(int aState) { 
                     return MIDIINPORT::setTrace(aState); }
      void        toggleTrace(void) { MIDIINPORT::toggleTrace(); }
      void        unpause(void)      { MIDIINPORT::unpause(); }
};



#endif  /* _MIDIINPORT_H_INCLUDED */



// md5sum:	96f8a2b4411a356d1b73cd96421b8931  - MidiInPort.h =css= 20030102
