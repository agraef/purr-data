//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Dec 28 15:18:46 GMT-0800 1997
// Last Modified: Mon Jan 12 20:05:27 GMT-0800 1998
// Last Modified: Wed Jun 30 11:29:51 PDT 1999 (added sysex capability)
// Filename:      ...sig/code/control/MidiInPort/visual/MidiInPort_visual.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiInPort_visual.h
// Syntax:        C++ 
//
// Description:   An interface for MIDI input capabilities of
//                Windows 95/NT/98 specific MIDI input methods.
//                as defined in winmm.lib.  This class is inherited 
//                privately by the MidiInPort class.
//

#ifndef _MIDIINPUT_VISUAL_H_INCLUDED
#define _MIDIINPUT_VISUAL_H_INCLUDED


#ifdef VISUAL

#define DEFAULT_INPUT_BUFFER_SIZE (1024)

#include <windows.h>
#include <mmsystem.h>

#include "MidiMessage.h"
#include "CircularBuffer.h"
#include "Array.h"

class MidiInPort_visual {
   public:
                      MidiInPort_visual          (void);
                      MidiInPort_visual          (int aPort, int autoOpen = 1);
                     ~MidiInPort_visual          ();

      void            clearSysex                 (void);
      void            clearSysex                 (int buffer);
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

   protected:
      int    port;     // the port to which this object belongs
      int    trace;

      int             installSysexPrivate (int port, uchar* anArray, int aSize);
      void            installSysexStuff   (HMIDIIN dev, int port); 
      void            uninstallSysexStuff (HMIDIIN dev, int port); 

      static int      objectCount;     // num of similar objects in existence
      static int*     portObjectCount; // objects connected to particular port
      static int*     openQ;           // for open/close status of port
      static int*     inrunningQ;      // for running open input port 
      static int      numDevices;      // number of input ports
      static HMIDIIN* device;          // Windoze MIDI-in device structure
      static MIDIHDR** sysexDriverBuffer1; // for Windoze driver sysex buffers
      static MIDIHDR** sysexDriverBuffer2; // for Windoze driver sysex buffers
      static int*      sysexDBnumber;  // for Windoze driver sysex buffers
      static HANDLE*  hMutex;          // mutual exclusive
      static CircularBuffer<MidiMessage>* midiBuffer; // MIDI storage from ports
      static int      channelOffset;     // channel offset from either 0 or 1.
                                         // not being used right now.
      static int*     sysexWriteBuffer;  // for MIDI sysex write location
      static Array<uchar>** sysexBuffers;// for MIDI sysex storage
      static int*     sysexStatus;       // tracing multiple MIM_LONGDATA messgs

   private:
      void            deinitialize               (void); 
      void            initialize                 (void); 
      void            releaseMutex               (void);
      void            setPortStatus              (int aStatus);
      void            waitForMutex               (void);


   friend void CALLBACK midiInputCallback(HMIDIIN hMidiIn, UINT inputStatus, 
                         DWORD instancePtr, DWORD midiMessage, DWORD timestamp);
};


// This is the command which is called by the driver when there is
// MIDI data being received from the MIDI input port:
 void CALLBACK midiInputCallback(HMIDIIN hMidiIn, UINT inputStatus, 
   DWORD instancePtr, DWORD midiMessage, DWORD timestamp);


#endif  /* VISUAL */
#endif  /* _MIDIINPUT_VISUAL_H_INCLUDED */



// md5sum:	d5aee7a88c4a054b3e2d4d40622fdc42  - MidiInPort_visual.h =css= 20030102
