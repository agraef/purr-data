//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Dec 28 15:18:46 GMT-0800 1997
// Last Modified: Mon Jan 12 20:05:27 GMT-0800 1998
// Filename:      ...sig/code/control/MidiOutPort/visual/MidiOutPort_visual.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiOutPort_visual.h
// Syntax:        C++ 
//
// Description:   Operating-System specific interface for
//                basic MIDI output capabilities in Windows 95/NT/98
//                using winmm.lib.  Privately inherited by the
//                MidiOutPort class.
// 

#ifndef _MIDIOUTPUT_VISUAL_H_INCLUDED
#define _MIDIOUTPUT_VISUAL_H_INCLUDED

#ifdef VISUAL

typedef unsigned char uchar;

#include <windows.h>
#include <mmsystem.h>

class MidiOutPort_visual {
   public:
                         MidiOutPort_visual  (void);
                         MidiOutPort_visual  (int aPort, int autoOpen = 1);
                        ~MidiOutPort_visual  ();

      void               close               (void);
      void               closeAll            (void);
      int                getChannelOffset    (void) const;
      const char*        getName             (void);
      static const char* getName             (int i);
      int                getPort             (void);
      static int         getNumPorts         (void);
      int                getPortStatus       (void);
      int                getTrace            (void);
      int                rawsend             (int command, int p1, int p2);
      int                rawsend             (int command, int p1);
      int                rawsend             (int command);
      int                rawsend             (uchar* array, int size);
      int                open                (void);
      void               setChannelOffset    (int aChannel);
      void               setPort             (int aPort);
      int                setTrace            (int aState);
      int                sysex               (uchar* array, int size);
      void               toggleTrace         (void);

   protected:
      int    port;     // the port to which this object belongs
      int    trace;    // for printing out Midi messages to standard output
 
      static int        objectCount;     // num of similar objects in existence
      static int*       portObjectCount; // objects connected to particular port
      static int*       openQ;           // for open/close status of port
      static int        numDevices;      // number of output ports
      static HMIDIOUT*  device;          // Windoze midi out device structure

   private:
      void            deinitialize               (void); 
      void            initialize                 (void); 
      void            setPortStatus              (int aStatus);

      static int      channelOffset;     // channel offset, either 0 or 1.
                                         // not being used right now.
};


#endif  /* VISUAL */
#endif  /* _MIDIOUTPUT_VISUAL_H_INCLUDED */


// md5sum:	47799e340effa57676be8a3943cabb70  - MidiOutPort_visual.h =css= 20030102
