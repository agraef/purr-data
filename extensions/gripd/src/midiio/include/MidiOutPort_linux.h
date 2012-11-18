//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed May 10 16:03:52 PDT 2000
// Last Modified: Thu May 18 23:37:17 PDT 2000
// Last Modified: Sat Nov  2 20:40:01 PST 2002 (added ALSA OSS def)
// Filename:      ...sig/code/control/MidiOutPort_linux/MidiOutPort_linux.h
// Web Address:   http://sig.sapp.org/include/sig/MidiOutPort_linux.h
// Syntax:        C++ 
//
// Description:   Linux MIDI output class which detects which
//                type of MIDI drivers are available: either
//                ALSA or OSS. 
//

#ifndef _MIDIOUTPORT_LINUX_H_INCLUDED
#define _MIDIOUTPORT_LINUX_H_INCLUDED

#ifdef LINUX
#if defined(ALSA) && defined(OSS)

#include "MidiOutPort_oss.h"
#include "MidiOutPort_alsa.h"
#include "MidiOutPort_unsupported.h"

#define UNKNOWN_MIDI_SELECT   0    /* use dummy MIDI output */
#define OSS_MIDI_SELECT       1    /* use OSS MIDI output */
#define ALSA_MIDI_SELECT      2    /* use ALSA MIDI output */

class MidiOutPort_linux {
   public:
                      MidiOutPort_linux     (void);
                      MidiOutPort_linux     (int aPort, int autoOpen = 1);
                     ~MidiOutPort_linux     ();

      void            close                 (void);
      void            closeAll              (void);
      int             getChannelOffset      (void) const;
      const char*     getName               (void);
      static const    char* getName         (int i);
      static int      getNumPorts           (void);
      int             getPort               (void);
      int             getPortStatus         (void);
      int             getTrace              (void);
      int             open                  (void);
      int             rawsend               (int command, int p1, int p2);
      int             rawsend               (int command, int p1);
      int             rawsend               (int command);
      int             rawsend               (uchar* array, int size);
      void            setAndOpenPort        (int aPort);
      void            setChannelOffset      (int aChannel);
      void            setPort               (int aPort);
      int             setTrace              (int aState);
      int             sysex                 (uchar* array, int size);
      void            toggleTrace           (void);

      static int      getSelect             (void);
      static int      selectOSS             (void);
      static int      selectALSA            (void);
      static int      selectUnknown         (void);

   private:
      static int      current;              // the type of MIDI out selected
      static int      alsaQ;                // boolean for if ALSA is present
      static int      ossQ;                 // boolean for if OSS is present

      static int                       objectCount;
      static MidiOutPort_oss          *oss_output;
      static MidiOutPort_alsa         *alsa_output;
      static MidiOutPort_unsupported  *unknown_output;

      void            determineDrivers      (void);
};

#endif  /* ALSA and OSS */
#endif  /* LINUX */

#endif  /* _MIDIOUTPORT_LINUX_H_INCLUDED */


// md5sum:	c80f9f47c45a6d4a20b6549743cae9fb  - MidiOutPort_linux.h =css= 20030102
