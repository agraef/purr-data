//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun May 14 22:05:27 PDT 2000
// Last Modified: Sat Oct 13 16:11:24 PDT 2001 (updated for ALSA 0.9)
// Last Modified: Sat Nov  2 20:35:50 PST 2002 (added #ifdef ALSA)
// Filename:      ...sig/maint/code/control/MidiInPort/linux/MidiInPort_alsa.h
// Web Address:   http://sig.sapp.org/include/sig/MidiInPort_alsa.h
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of
//                linux ALSA sound driver's specific MIDI input methods.
//                This class is inherited privately by the MidiInPort class.
//

#ifndef _MIDIINPORT_ALSA_H_INCLUDED
#define _MIDIINPORT_ALSA_H_INCLUDED

#ifdef LINUX
#ifdef ALSA

#include "MidiMessage.h"
#include "CircularBuffer.h"
#include "Array.h"
#include "Sequencer_alsa.h"
#include "SigTimer.h"
#include <pthread.h>

typedef unsigned char uchar;
typedef void (*MIDI_Callback_function)(int arrivalPort);

void *interpretMidiInputStreamPrivateALSA(void * x);

class MidiInPort_alsa : public Sequencer_alsa {
   public:
                      MidiInPort_alsa             (void);
                      MidiInPort_alsa             (int aPort, int autoOpen = 1);
                     ~MidiInPort_alsa             ();

      void            clearSysex                 (int buffer);
      void            clearSysex                 (void);
      void            close                      (void);
      void            closeAll                   (void);
      MidiMessage     extract                    (void);
      int             getBufferSize              (void);
      int             getChannelOffset           (void) const;
      int             getCount                   (void);
      const char*     getName                    (void);
      static const char* getName                 (int i);
      static int      getNumPorts                (void);
      int             getPort                    (void);
      int             getPortStatus              (void);
      uchar*          getSysex                   (int buffer);
      int             getSysexSize               (int buffer);
      int             getTrace                   (void);
      void            insert                     (const MidiMessage& aMessage);
      int             installSysex               (uchar* anArray, int aSize);
      MidiMessage&    message                    (int index);
      int             open                       (void);
      void            pause                      (void);
      void            setBufferSize              (int aSize);
      void            setChannelOffset           (int anOffset);
      void            setPort                    (int aPort);
      int             setTrace                   (int aState);
      void            toggleTrace                (void);
      void            unpause                    (void);

      static Array<int> threadinitport;

   protected:
      int    port;     // the port to which this object belongs

      static MIDI_Callback_function  callbackFunction;

      static int      installSysexPrivate        (int port, 
                                                    uchar* anArray, int aSize);
 
      static int        objectCount;        // num of similar objects in existence
      static int*       portObjectCount;    // objects connected to particular port
      static int*       trace;              // for verifying input
      static std::ostream*   tracedisplay;       // stream for displaying trace
      static int        numDevices;         // number of input ports
      static CircularBuffer<MidiMessage>** midiBuffer; // MIDI storage frm ports
      static int        channelOffset;      // channel offset, either 0 or 1
                                            // not being used right now.
      static int*       pauseQ;             // for adding items to Buffer or not
      static SigTimer   midiTimer;          // for timing MIDI input
      static Array<pthread_t> midiInThread; // for MIDI input thread function
      static int*       sysexWriteBuffer;   // for MIDI sysex write location
      static Array<uchar>** sysexBuffers;   // for MIDI sysex storage

   private:
      void            deinitialize               (void); 
      void            initialize                 (void); 

 
   friend void *interpretMidiInputStreamPrivateALSA(void * x);
    
};


#endif  /* ALSA */
#endif  /* LINUX */

#endif  /* _MIDIINPORT_ALSA_H_INCLUDED */



// md5sum:	260a0accd6b08b638a00904c382293bc  - MidiInPort_alsa.h =css= 20030102
