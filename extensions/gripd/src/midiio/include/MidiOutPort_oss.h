//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Dec 18 19:15:00 PST 1998
// Last Modified: Wed May 10 17:00:11 PDT 2000 (name change from _linux to _oss)
// Filename:      ...sig/maint/code/control/MidiOutPort/linux/MidiOutPort_oss.h
// Web Address:   http://sig.sapp.org/include/sig/MidiOutPort_oss.h
// Syntax:        C++
//
// Description:   Operating-System specific interface for
//                basic MIDI output capabilities in Linux using
//                OSS sound drivers.  Privately inherited by the
//                MidiOutPort class.
// 

#ifndef _MIDIOUTPORT_OSS_H_INCLUDED
#define _MIDIOUTPORT_OSS_H_INCLUDED

#ifdef LINUX

#include "Sequencer_oss.h"

typedef unsigned char uchar;


class MidiOutPort_oss : public Sequencer_oss {
   public:
                      MidiOutPort_oss            (void);
                      MidiOutPort_oss            (int aPort, int autoOpen = 1);
                     ~MidiOutPort_oss            ();

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



#endif  /* LINUX */
#endif  /* _MIDIOUTPUT_OSS_H_INCLUDED */


// md5sum:	f60183e23c49741e93d9b965bbe9a6d8  - MidiOutPort_oss.h =css= 20030102
