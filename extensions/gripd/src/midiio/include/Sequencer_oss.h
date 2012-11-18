//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Jan  3 21:02:02 PST 1999
// Last Modified: Sat Jan 30 14:11:18 PST 1999
// Last Modified: Wed May 10 17:00:11 PDT 2000 (name change from _oss to _oss)
// Filename:      ...sig/maint/code/control/MidiOutPort/Sequencer_oss.h
// Web Address:   http://sig.sapp.org/include/sig/Sequencer_oss.h
// Syntax:        C++ 
//
// Description:   Basic MIDI input/output functionality for the 
//                Linux OSS midi device /dev/sequencer.  This class
//                is inherited by the classes MidiInPort_oss and
//                MidiOutPort_oss.
//

#ifndef _SEQUENCER_OSS_H_INCLUDED
#define _SEQUENCER_OSS_H_INCLUDED

#include <iostream>

#define MIDI_EXTERNAL  (1)
#define MIDI_INTERNAL  (2)

typedef unsigned char uchar;


class Sequencer_oss {
   public:
                    Sequencer_oss        (int autoOpen = 1);
                   ~Sequencer_oss        ();

      void          close                (void);
      void          displayInputs        (std::ostream& out = std::cout, 
                                            char* initial = "\t");
      void          displayOutputs       (std::ostream& out = std::cout, 
                                            char* initial = "\t");
      static int    getNumInputs         (void);
      static int    getNumOutputs        (void);
      static const char*   getInputName  (int aDevice);
      static const char*   getOutputName (int aDevice);
      int           is_open              (void);
      int           open                 (void);
      void          read                 (uchar* buf, uchar* dev, int count);
      void          rawread              (uchar* buf, int packetCount);
      void          rebuildInfoDatabase  (void);
      int           write                (int aDevice, int aByte);
      int           write                (int aDevice, uchar* bytes, int count);
      int           write                (int aDevice, char* bytes, int count);
      int           write                (int aDevice, int* bytes, int count);
      
   protected:
      static const char* sequencer;         // name of sequencer device
      static int    sequencer_fd;           // sequencer file descriptor
      static int    class_count;            // number of existing classes using
      static uchar  midi_write_packet[4];   // for writing MIDI bytes out
      static uchar  midi_read_packet[4];    // for reading MIDI bytes out
      static uchar  synth_write_message[8]; // for writing to internal seq
      static int    indevcount;             // number of MIDI input devices
      static int    outdevcount;            // number of MIDI output devices
      static char** indevnames;             // MIDI input device names
      static char** outdevnames;            // MIDI output device names
      static int*   indevnum;               // total number of MIDI inputs
      static int*   outdevnum;              // total number of MIDI outputs
      static int*   indevtype;              // 1 = External, 2 = Internal
      static int*   outdevtype;             // 1 = External, 2 = Internal
      static uchar  synth_message_buffer[1024];   // hold bytes for synth dev
      static int    synth_message_buffer_count;   // count of synth buffer
      static int    synth_message_bytes_expected; // expected count of synth
      static int    synth_message_curr_device;    // for keeping track of dev
      static int    initialized;            // for starting buileinfodatabase

   private:
      static void   buildInfoDatabase     (void);
      static int    getFd                 (void);   
      int           getInDeviceValue      (int aDevice) const;
      int           getInputType          (int aDevice) const;
      int           getOutDeviceValue     (int aDevice) const;
      int           getOutputType         (int aDevice) const;
      void          removeInfoDatabase    (void);
      void          setFd                 (int anFd);   

      int           writeInternal(int aDevice, int aByte);
      int           transmitMessageToInternalSynth(void);
      int           transmitVoiceMessage(void);
      int           transmitCommonMessage(void);
};


#endif  /* _SEQUENCER_OSS_H_INCLUDED */


// md5sum:	1df08cd946c609b9b42aadbc96b7a296  - Sequencer_oss.h =css= 20030102
