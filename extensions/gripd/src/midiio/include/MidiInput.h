//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu> 
// Creation Date: 18 December 1997
// Last Modified: Sun Jan 25 15:27:02 GMT-0800 1998
// Last Modified: Thu Apr 20 16:23:24 PDT 2000 (added scale function)
// Filename:      ...sig/code/control/MidiInput/MidiInput.h
// Web Address:   http://sig.sapp.org/include/sig/MidiInput.h
// Syntax:        C++
//
// Description:   A higher-level MIDI input interface than the 
//                MidiInPort class.  Can be used to allow multiple
//                objects to share a single MIDI input stream, or
//                to fake a MIDI input connection.
//

#ifndef _MIDIINPUT_H_INCLUDED
#define _MIDIINPUT_H_INCLUDED


#include "MidiInPort.h"


class MidiInput : public MidiInPort {
   public:
                    MidiInput         (void);
                    MidiInput         (int aPort, int autoOpen = 1);
                   ~MidiInput         ();

      int           getBufferSize     (void);
      int           getCount          (void);
      MidiMessage   extract           (void);
      void          insert            (const MidiMessage& aMessage);
      int           isOrphan          (void) const;
      void          makeOrphanBuffer  (int aSize = 1024);
      void          removeOrphanBuffer(void);
      void          setBufferSize     (int aSize);

      int           scale             (int value, int min, int max);
      double        fscale            (int value, double min, double max);
      int           scale14           (int value, int min, int max);
      double        fscale14          (int value, double min, double max);

   protected:
      CircularBuffer<MidiMessage>* orphanBuffer;

};


#endif  /* _MIDIINPUT_H_INCLUDED */



// md5sum:	73972cc29d7bcf0fba136b098c0419a0  - MidiInput.h =css= 20030102
