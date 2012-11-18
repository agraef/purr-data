//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun May 14 22:30:04 PDT 2000
// Last Modified: Thu May 18 23:36:07 PDT 2000 (added ifdef LINUX lines)
// Last Modified: Sat Nov  2 20:37:49 PST 2002 (added ifdef ALSA OSS)
// Filename:      ...sig/maint/code/control/MidiInPort/MidiInPort_linux.h
// Web Address:   http://sig.sapp.org/include/sig/MidiInPort_linux.h
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of an
//                operating-system specific MIDI input method.
//                Provides control of all low-level MIDI input
//                functionality such that it will work on all
//                computers in the same manner.
//

#ifndef _MIDIINPORT_LINUX_H_INCLUDED
#define _MIDIINPORT_LINUX_H_INCLUDED

#ifdef LINUX
#if defined(ALSA) && defined(OSS)

#define MIDI_IN_UNKNOWN_SELECT 0
#define MIDI_IN_OSS_SELECT     1
#define MIDI_IN_ALSA_SELECT    2

#include "MidiInPort_oss.h"
#include "MidiInPort_alsa.h"
#include "MidiInPort_unsupported.h"
#include "MidiMessage.h"


class MidiInPort_linux {
   public:
                  MidiInPort_linux(void);
                  MidiInPort_linux(int aPort, int autoOpen = 1);
                 ~MidiInPort_linux();

      void         clearSysex               (void);
      void         clearSysex               (int buffer);
      void         close                    (void);
      void         closeAll                 (void);
      MidiMessage  extract                  (void);
      int          getBufferSize            (void);
      int          getChannelOffset         (void) const;
      int          getCount                 (void);
      const char*  getName                  (void);
      static const char* getName            (int i);
      static int   getNumPorts              (void);
      int          getPort                  (void);
      int          getPortStatus            (void);
      uchar*       getSysex                 (int buffer);
      int          getSysexSize             (int buffer);
      int          getTrace                 (void);
      void         insert                   (const MidiMessage& aMessage);
      int          installSysex             (uchar* anArray, int aSize);
      MidiMessage& message                  (int index);
      int          open                     (void);
      MidiMessage& operator[]               (int index);
      void         pause                    (void);
      void         setBufferSize            (int aSize);
      void         setChannelOffset         (int anOffset);
      void         setAndOpenPort           (int aPort);
      void         setPort                  (int aPort);
      int          setTrace                 (int aState);
      void         toggleTrace              (void);
      void         unpause                  (void);

      static int   getSelect                (void);
      static int   selectOSS                (void);
      static int   selectALSA               (void);
      static int   selectUnknown            (void);

   private:

      static int      current;              // the type of MIDI out selected
      static int      alsaQ;                // boolean for if ALSA is present
      static int      ossQ;                 // boolean for if OSS is present
      static int      objectCount;          // keeps track of static variables

      static MidiInPort_oss          *oss_input;
      static MidiInPort_alsa         *alsa_input;
      static MidiInPort_unsupported  *unknown_input;

      void            determineDrivers      (void);
};

#endif  /* ALSA and OSS def */
#endif  /* LINUX def */

#endif  /* _MIDIINPORT_LINUX_H_INCLUDED */


// md5sum:	cc3608fb63ccf222f018efc89a4275f0  - MidiInPort_linux.h =css= 20030102
