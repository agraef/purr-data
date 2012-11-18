//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Dec 28 15:04:24 GMT-0800 1997
// Last Modified: Fri Jan 23 10:56:18 GMT-0800 1998
// Last Modified: Sat Nov  7 16:15:54 PST 1998
// Last Modified: Tue May 23 23:08:44 PDT 2000 (oss/alsa selection added)
// Last Modified: Mon Jun 19 10:32:11 PDT 2000 (oss/alsa define fix)
// Filename:      ...sig/code/control/MidiOutPort/MidiOutPort.h
// Web Address:   http://sig.sapp.org/include/sig/MidiOutPort.h
// Syntax:        C++ 
//
// Description:   Operating-System independent interface for
//                basic MIDI output capabilities.  Privately
//                inherits the operating-system specific class
//                for MIDI output.
//

#ifndef _MIDIOUTPORT_H_INCLUDED
#define _MIDIOUTPORT_H_INCLUDED


#ifdef VISUAL
   #define MIDIOUTPORT  MidiOutPort_visual
   #include "MidiOutPort_visual.h"
#elif defined(LINUX) && defined(ALSA) && defined(OSS)
   #define MIDIOUTPORT  MidiOutPort_linux
   #include "MidiOutPort_linux.h"
#elif defined(LINUX) && defined(ALSA) && !defined(OSS)
   #define MIDIOUTPORT  MidiOutPort_alsa
   #include "MidiOutPort_alsa.h"
#elif defined (LINUX) && defined(OSS) && !defined(ALSA)
   #define MIDIOUTPORT  MidiOutPort_oss
   #include "MidiOutPort_oss.h"
#elif defined(LINUX)
   #define MIDIOUTPORT  MidiOutPort_oss
   #include "MidiOutPort_oss.h"
#else
   #define MIDIOUTPORT  MidiOutPort_unsupported
   #include "MidiOutPort_unsupported.h"
#endif


class MidiOutPort : protected MIDIOUTPORT {
   public:
                  MidiOutPort        (void) : MIDIOUTPORT() {}
                  MidiOutPort        (int aPort, int autoOpen = 1) : 
                                         MIDIOUTPORT(aPort, autoOpen) {}
                 ~MidiOutPort()      { }

      void        close(void)         { MIDIOUTPORT::close(); }
      void        closeAll(void)      { MIDIOUTPORT::closeAll(); }
      int         getChannelOffset(void) const { 
                     return MIDIOUTPORT::getChannelOffset(); }
      const char* getName(void)       { return MIDIOUTPORT::getName(); }
      static const char* getName(int i) { return MIDIOUTPORT::getName(i); }
      static int  getNumPorts(void)   { return MIDIOUTPORT::getNumPorts(); }
      int         getPort(void)       { return MIDIOUTPORT::getPort(); }
      int         getPortStatus(void) { 
                     return MIDIOUTPORT::getPortStatus(); }
      int         getTrace(void) { 
                     return MIDIOUTPORT::getTrace(); }
      int         open(void)         { return MIDIOUTPORT::open(); }
      int         rawsend(int command, int p1, int p2) {
                     return MIDIOUTPORT::rawsend(command, p1, p2); }
      int         rawsend(int command, int p1) {
                     return MIDIOUTPORT::rawsend(command, p1); }
      int         rawsend(int command) {
                     return MIDIOUTPORT::rawsend(command); }
      int         rawsend(uchar* array, int size) {
                     return MIDIOUTPORT::rawsend(array, size); }
      void        setAndOpenPort(int aPort) { setPort(aPort); open(); }
//    void        setChannelOffset(int aChannel) { 
//                   MIDIOUTPORT::setChannelOffset(aChannel); }
      void        setPort(int aPort) { MIDIOUTPORT::setPort(aPort); }
      int         setTrace(int aState) {
                     return MIDIOUTPORT::setTrace(aState); }
      int         sysex(uchar* array, int size) {
                     return MIDIOUTPORT::sysex(array, size); }
      void        toggleTrace(void) { MIDIOUTPORT::toggleTrace(); }
      MidiOutPort& operator=(MidiOutPort& other) {
                     setPort(other.getPort());
                     if (other.getPortStatus()) { open(); }
                     return *this; }
};




#endif  /* _MIDIOUTPORT_H_INCLUDED */


// md5sum:	2f7b8aa8ef705eab57179b626ce1d62d  - MidiOutPort.h =css= 20030102
