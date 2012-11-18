//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 21 December 1997
// Last Modified: Sun Jan 25 15:44:35 GMT-0800 1998
// Filename:      ...sig/code/control/MidiIO/MidiIO.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiIO.h
// Syntax:        C++
//
// Description:   A unified class for MidiInput and MidiOutput that handles 
//                MIDI input and output connections.  The Synthesizer
//                and RadioBaton classes are derived from this class.
//

#ifndef _MIDIIO_H_INCLUDED
#define _MIDIIO_H_INCLUDED


#include "MidiInput.h"
#include "MidiOutput.h"


class MidiIO : public MidiOutput, public MidiInput {
   public:
                 MidiIO              (void);
                 MidiIO              (int outPort, int inPort);
                ~MidiIO              ();

      void       close               (void);
      void       closeInput          (void);
      void       closeOutput         (void);
      int        getChannelInOffset  (void) const;
      int        getChannelOutOffset (void) const;
      int        getInputPort        (void);
      int        getInputTrace       (void);
      int        getNumInputPorts    (void);
      int        getNumOutputPorts   (void);
      int        getOutputPort       (void);
      int        getOutputTrace      (void);
      int        open                (void);
      int        openInput           (void);
      int        openOutput          (void);
      void       setChannelOffset    (int anOffset);
      void       setInputPort        (int aPort);
      void       setInputTrace       (int aState);
      void       setOutputPort       (int aPort);
      void       setOutputTrace      (int aState);
      void       toggleInputTrace    (void);
      void       toggleOutputTrace   (void);

};



#endif  /* _MIDIIO_H_INCLUDED */



// md5sum:	9f6122405c4d9e83994457210217ff22  - MidiIO.h =css= 20030102
