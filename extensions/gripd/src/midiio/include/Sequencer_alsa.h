//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Thu May 11 21:06:21 PDT 2000
// Last Modified: Sat Oct 13 14:50:04 PDT 2001 (updated for ALSA 0.9 interface)
// Filename:      ...sig/maint/code/control/MidiOutPort/Sequencer_alsa.h
// Web Address:   http://sig.sapp.org/include/sig/Sequencer_alsa.h
// Syntax:        C++ 
//
// Description:   Basic MIDI input/output functionality for the 
//                Linux ALSA midi device /dev/snd/midiXX.  This class
//                is inherited by the classes MidiInPort_alsa and
//                MidiOutPort_alsa.
//
// to get information of status a alsa hardware & software
//    cat /proc/asound/version
//    cat /proc/asound/devices
//    cat /proc/asound/card0/midi0
//

#ifndef _SEQUENCER_ALSA_H_INCLUDED
#define _SEQUENCER_ALSA_H_INCLUDED

#include <iostream>

#ifdef ALSA

#include <alsa/asoundlib.h>
#include "Collection.h"

#define MIDI_EXTERNAL  (1)
#define MIDI_INTERNAL  (2)

typedef unsigned char uchar;


class Sequencer_alsa {
   public:
                    Sequencer_alsa       (int autoOpen = 1);
                   ~Sequencer_alsa       ();

      void          close                (void);
      void          closeInput           (int index);
      void          closeOutput          (int index);
      void          displayInputs        (std::ostream& out = std::cout, 
                                            char* initial = "\t");
      void          displayOutputs       (std::ostream& out = std::cout, 
                                            char* initial = "\t");
      static const char*   getInputName  (int aDevice);
      static const char*   getOutputName (int aDevice);
      static int    getNumInputs         (void);
      static int    getNumOutputs        (void);
      int           is_open              (int mode, int index);
      int           is_open_in           (int index);
      int           is_open_out          (int index);
      int           open                 (int direction, int index);
      int           openInput            (int index);
      int           openOutput           (int index);
      void          read                 (int dev, uchar* buf, int count);
      void          rebuildInfoDatabase  (void);
      int           write                (int aDevice, int aByte);
      int           write                (int aDevice, uchar* bytes, int count);
      int           write                (int aDevice, char* bytes, int count);
      int           write                (int aDevice, int* bytes, int count);
      
      int           getInCardValue       (int aDevice) const;
      int           getOutCardValue      (int aDevice) const;
   protected:
      static int    class_count;            // number of existing classes using
      static int    indevcount;             // number of MIDI input devices
      static int    outdevcount;            // number of MIDI output devices
      static int    initialized;            // for starting buileinfodatabase

      static Collection<snd_rawmidi_t*>  rawmidi_in;
      static Collection<snd_rawmidi_t*>  rawmidi_out;
      static Collection<int>             midiincard;
      static Collection<int>             midioutcard;
      static Collection<int>             midiindevice;
      static Collection<int>             midioutdevice;
      static Collection<char*>           midiinname;
      static Collection<char*>           midioutname;

   private:
      static void   buildInfoDatabase     (void);
      static void   getPossibleMidiStreams(Collection<int>& cards,
                                           Collection<int>& devices);
      int           getInDeviceValue      (int aDevice) const;
      int           getInputType          (int aDevice) const;
      int           getOutDeviceValue     (int aDevice) const;
      int           getOutputType         (int aDevice) const;
      void          removeInfoDatabase    (void);


   friend void *interpretMidiInputStreamPrivateALSA(void * x);

};

#else  /* ALSA is not defined */

typedef unsigned char uchar;

class Sequencer_alsa {
   public:
                    Sequencer_alsa       (int autoOpen = 1) { }
                   ~Sequencer_alsa       () { }

      void          close                (void) { };
      void          displayInputs        (std::ostream& out = std::cout, 
                                            char* initial = "\t") 
                                         { out << initial << "NONE\n"; }
      void          displayOutputs       (std::ostream& out = std::cout, 
                                            char* initial = "\t") 
                                         { out << initial << "NONE\n"; }
      static const char*   getInputName  (int aDevice) { return ""; }
      static const char*   getOutputName (int aDevice) { return ""; }
      static int    getNumInputs         (void) { return 0; }
      static int    getNumOutputs        (void) { return 0; }
      int           is_open              (int mode, int index) { return 0; }
      int           is_open_in           (int index) { return 0; }
      int           is_open_out          (int index) { return 0; }
      int           open                 (void) { return 0; }
      void          read                 (int dev, uchar* buf, int count) { }
      void          rebuildInfoDatabase  (void) { }
      int           write                (int aDevice, int aByte) { return 0; }
      int           write                (int aDevice, uchar* bytes, int count) { return 0; }
      int           write                (int aDevice, char* bytes, int count) { return 0; }
      int           write                (int aDevice, int* bytes, int count) { return 0; }
      int           getInCardValue       (int aDevice) const { return 0; }
      int           getOutCardValue      (int aDevice) const { return 0; }

};


#endif /* ALSA */


#endif  /* _SEQUENCER_ALSA_H_INCLUDED */


// md5sum:	6147020b0646fca8245f653505308949  - Sequencer_alsa.h =css= 20030102
