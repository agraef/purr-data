//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Jan 23 00:04:51 GMT-0800 1998
// Last Modified: Fri Jan 23 00:04:58 GMT-0800 1998
// Last Modified: Wed Jun 30 11:42:59 PDT 1999 (added sysex capability)
// Filename:      ...sig/code/control/MidiInPort/unsupported/MidiInPort_unsupported.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiInPort_unsupported.h
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of
//                an unknown sound driver's specific MIDI input methods.
//                This class is inherited privately by the MidiInPort class.
//                This class is used when there is no MIDI input, so
//                that MIDI programs can otherwise be compiled and run.
//                This file can also serve as a template for creating
//                an OS specific class for MIDI input.
//

#ifndef _MIDIINPUT_UNSUPPORTED_H_INCLUDED
#define _MIDIINPUT_UNSUPPORTED_H_INCLUDED

#include "MidiMessage.h"
#include "CircularBuffer.h"
#include "Array.h"

class MidiInPort_unsupported {
   public:
                      MidiInPort_unsupported     (void);
                      MidiInPort_unsupported     (int aPort, int autoOpen = 1);
                     ~MidiInPort_unsupported     ();

      void            clearSysex                 (int index) { }
      void            clearSysex                 (void) { }
      int             getSysexSize               (int index) { return 0; }
      uchar*          getSysex                   (int buffer) { return NULL; }
      int             installSysex               (unsigned char *&, int &) { return 0; }
      int             getBufferSize              (void) { return 0; }
      void            close                      (void);
      void            close                      (int i) { close(); }
      void            closeAll                   (void);
      MidiMessage     extract                    (void);
      int             getChannelOffset           (void) const;
      int             getCount                   (void);
      const char*     getName                    (void);
      static const char* getName                 (int i);
      int             getNumPorts                (void);
      int             getPort                    (void);
      int             getPortStatus              (void);
      int             getTrace                   (void);
      void            insert                     (const MidiMessage& aMessage);
      MidiMessage&    message                    (int index);
      int             open                       (void);
      void            pause                      (void);
      void            setBufferSize              (int aSize);
      void            setChannelOffset           (int anOffset);
      void            setPort                    (int aPort);
      int             setTrace                   (int aState);
      void            toggleTrace                (void);
      void            unpause                    (void);

   protected:
      int    port;     // the port to which this object belongs
      int    trace;
 
      static int        objectCount;     // num of similar objects in existence
      static int*       portObjectCount; // objects connected to particular port
      static int*       openQ;           // for open/close status of port
      static int        numDevices;      // number of input ports
      static CircularBuffer<MidiMessage>* midiBuffer; // MIDI storage from ports
      static int        channelOffset;     // channel offset, either 0 or 1
                                           // not being used right now.
      static int*       sysexWriteBuffer;  // for MIDI sysex write location
      static Array<uchar>** sysexBuffers;  // for MIDI sysex storage

   private:
      void            deinitialize               (void); 
      void            initialize                 (void); 
      void            setPortStatus              (int aStatus);

    
};



#endif  /* _MIDIINPUT_UNSUPPORTED_H_INCLUDED */



// md5sum:	ff5492fbd59a47e48e2c0ce06705add1  - MidiInPort_unsupported.h =css= 20030102
