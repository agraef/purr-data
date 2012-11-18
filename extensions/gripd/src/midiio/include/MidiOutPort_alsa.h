//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed May 10 16:22:00 PDT 2000
// Last Modified: Sun May 14 20:43:44 PDT 2000
// Last Modified: Sat Nov  2 20:39:01 PST 2002 (added ALSA def)
// Filename:      ...sig/maint/code/control/MidiOutPort/linux/MidiOutPort_alsa.h
// Web Address:   http://sig.sapp.org/include/sig/MidiOutPort_alsa.h
// Syntax:        C++
//
// Description:   Operating-System specific interface for
//                basic MIDI output capabilities in Linux using
//                OSS sound drivers.  Privately inherited by the
//                MidiOutPort class.
// 

#ifndef _MIDIOUTPORT_ALSA_H_INCLUDED
#define _MIDIOUTPORT_ALSA_H_INCLUDED

#ifdef LINUX
#ifdef ALSA

#include "Sequencer_alsa.h"
#include <iostream>

typedef unsigned char uchar;


class MidiOutPort_alsa : public Sequencer_alsa {
   public:
                      MidiOutPort_alsa          (void);
                      MidiOutPort_alsa          (int aPort, int autoOpen = 1);
                     ~MidiOutPort_alsa          ();

      void            close                      (void);
      void            closeAll                   (void);
      int             getChannelOffset           (void) const;
      const char*     getName                    (void);
      static const char* getName                 (int i);
      int             getPort                    (void);
      static int      getNumPorts                (void);
      int             getPortStatus              (void);
      int             getTrace                   (void);
      int             rawsend                    (int command, int p1, int p2);
      int             rawsend                    (int command, int p1);
      int             rawsend                    (int command);
      int             rawsend                    (uchar* array, int size);
      int             open                       (void);
      void            setChannelOffset           (int aChannel);
      void            setPort                    (int aPort);
      int             setTrace                   (int aState);
      int             sysex                      (uchar* array, int size);
      void            toggleTrace                (void);

   protected:
      int    port;     // the port to which this object belongs
 
      static int        objectCount;     // num of similar objects in existence
      static int*       portObjectCount; // objects connected to particular port
      static int        numDevices;      // number of output ports
      static int*       trace;           // for printing messages to output
      static std::ostream*   tracedisplay;    // for printing trace messages

   private:
      void            deinitialize               (void); 
      void            initialize                 (void); 
      void            setPortStatus              (int aStatus);

      static int      channelOffset;     // channel offset, either 0 or 1.
                                         // not being used right now.
};



#endif  /* ALSA */
#endif  /* LINUX */
#endif  /* _MIDIOUTPUT_ALSA_H_INCLUDED */


// md5sum:	5b7648c7b493df7cb0d1fae3bbb8be24  - MidiOutPort_alsa.h =css= 20030102
