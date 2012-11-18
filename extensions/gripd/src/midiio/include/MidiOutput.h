//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 18 December 1997
// Last Modified: Mon Jan 26 23:53:05 GMT-0800 1998
// Last Modified: Sat Jan 30 14:00:29 PST 1999
// Last Modified: Sun Jul 18 18:52:42 PDT 1999 (added RPN functions)
// Filename:      ...sig/maint/code/control/MidiOutput/MidiOutput.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiOutput.h
// Syntax:        C++
//
// Description:   The MIDI output interface for MIDI synthesizers/equipment
//                which has many convienience functions defined for
//                various types of MIDI output.
//

#ifndef _MIDIOUTPUT_H_INCLUDED
#define _MIDIOUTPUT_H_INCLUDED

#include "MidiOutPort.h"
#include "FileIO.h"
#include "SigTimer.h"
#include "Array.h"


class MidiOutput : public MidiOutPort {
   public:
                MidiOutput     (void);
                MidiOutput     (int aPort, int autoOpen = 1);
               ~MidiOutput     ();

      // Basic user MIDI output commands:
      int       cont           (int channel, int controller, int data);
      int       off            (int channel, int keynum, int releaseVelocity);
      int       pc             (int channel, int timbre);
      int       play           (int channel, int keynum, int velocity);
      int       pw             (int channel, int mostByte, int leastByte);
      int       pw             (int channel, int tuningData);
      int       pw             (int channel, double tuningData);
      void      recordStart    (char *filename, int format);
      void      recordStop     (void);
      void      reset          (void);
      int       send           (int command, int p1, int p2);
      int       send           (int command, int p1);
      int       send           (int command);
      void      silence        (int aChannel = -1);
      void      sustain        (int channel, int status);
      int       sysex          (char* data, int length);
      int       sysex          (uchar* data, int length);

   protected:
      int       outputRecordQ;     // boolean for recording
      int       outputRecordType;  // what form to record MIDI data in
      int       lastFlushTime;     // for recording midi data
      FileIO    outputRecordFile;  // file for recording midi data
      static SigTimer timer;       // for recording midi data
      static Array<int>* rpn_lsb_status;  // for RPN messages
      static Array<int>* rpn_msb_status;  // for RPN messages
      static int objectCount;             // for RPN messages

      void      deinitializeRPN    (void);
      void      initializeRPN      (void);
      void      writeOutputAscii   (int channel, int p1, int p2);
      void      writeOutputBinary  (int channel, int p1, int p2); 
      void      writeOutputMidifile(int channel, int p1, int p2);

   public: // RPN controller functions
      int    NRPN                    (int channel, int nrpn_msb, int nrpn_lsb, 
                                           int data_msb, int data_lsb);
      int    NRPN                    (int channel, int nrpn_msb, int nrpn_lsb, 
                                           int data_msb);
      int    NRPN                    (int channel, int nrpn_msb, int nrpn_lsb, 
                                           double data);
      int    NRPN_null               (int channel);
      int    NRPN_attack             (int channel, double value);
      int    NRPN_attack             (int channel, int value);
      int    NRPN_decay              (int channel, double value);
      int    NRPN_decay              (int channel, int value);
      int    NRPN_drumAttack         (int drum, double value);
      int    NRPN_drumAttack         (int drum, int value);
      int    NRPN_drumChorus         (int drum, double value);
      int    NRPN_drumChorus         (int drum, int value);
      int    NRPN_drumDecay          (int drum, double value);
      int    NRPN_drumDecay          (int drum, int value);
      int    NRPN_drumLevel          (int drum, double value);
      int    NRPN_drumLevel          (int drum, int value);
      int    NRPN_drumPan            (int drum, double value);
      int    NRPN_drumPan            (int drum, int value);
      int    NRPN_drumPitch          (int drum, double value);
      int    NRPN_drumPitch          (int drum, int value);
      int    NRPN_drumFilterCutoff   (int drum, double value);
      int    NRPN_drumFilterCutoff   (int drum, int value);
      int    NRPN_drumFilterResonance(int drum, double value);
      int    NRPN_drumFilterResonance(int drum, int value);
      int    NRPN_drumReverb         (int drum, double value);
      int    NRPN_drumReverb         (int drum, int value);
      int    NRPN_drumVariation      (int drum, double value);
      int    NRPN_drumVariation      (int drum, int value);
      int    NRPN_filterCutoff       (int channel, double value);
      int    NRPN_filterCutoff       (int channel, int value);
      int    NRPN_release            (int channel, double value);
      int    NRPN_release            (int channel, int value);
      int    NRPN_vibratoDelay       (int channel, double value);
      int    NRPN_vibratoDelay       (int channel, int value);
      int    NRPN_vibratoDepth       (int channel, double value);
      int    NRPN_vibratoDepth       (int channel, int value);
      int    NRPN_vibratoRate        (int channel, double value);
      int    NRPN_vibratoRate        (int channel, int value);

      int    RPN                     (int channel, int rpn_msb, int rpn_lsb, 
                                           int data_msb, int data_lsb);
      int    RPN                     (int channel, int rpn_msb, int rpn_lsb, 
                                           int data_msb);
      int    RPN                     (int channel, int rpn_msb, int rpn_lsb, 
                                           double data);
      int    RPN_null                (void);
      int    RPN_null                (int channel);
      int    pbRange                 (int channel, int steps);
      int    tuneFine                (int channel, int cents);
      int    fineTune                (int channel, int cents);
      int    tuneCoarse              (int channel, int steps);
      int    coarseTune              (int channel, int steps);
      int    tuningProgram           (int channel, int program);
      int    tuningBank              (int channel, int bank);

};


#endif /* _MIDIOUTPUT_H_INCLUDED */

// Brief description of MidiOutput public member functions:
//
// send:    sends a MIDI command to the MIDI output with up to two parameters
// play:    sends a playnote command to the MIDI output
// pc:      Patch Change.  changes the timbre (instrument) on the given channel
// cont:    sends a CONTinuous CONTroller MIDI command
// sysex:   sends a system exclusive command to the MIDI output
//


// md5sum:	12ee02c32563ae219aaa8c7599de55db  - MidiOutput.h =css= 20030102
