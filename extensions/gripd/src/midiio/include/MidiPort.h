//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 21 December 1997
// Last Modified: Fri Jan 23 10:21:25 GMT-0800 1998
// Filename:      .../sig/code/control/MidiPort/MidiPort.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiPort.h
// Syntax:        C++
//
// Description:   A unified object that handles basic MIDI input and output.
//                Derived from the MidiInPort and MidiOutPort classes.
//

#ifndef _MIDIPORT_H_INCLUDED
#define _MIDIPORT_H_INCLUDED


#include "MidiInPort.h"
#include "MidiOutPort.h"


class MidiPort : public MidiOutPort, public MidiInPort {
   public:
                 MidiPort            (void);
                 MidiPort            (int outputPort, int inputPort);
                ~MidiPort            ();

      int        getChannelInOffset  (void) const;
      int        getChannelOutOffset (void) const;
      int        getInputPort        (void);
      int        getInputTrace       (void);
      int        getOutputPort       (void);
      int        getOutputTrace      (void);
      void       setChannelOffset    (int anOffset);
      void       setInputPort        (int aPort);
      int        setInputTrace       (int aState);
      void       setOutputPort       (int aPort);
      int        setOutputTrace      (int aState);
      void       toggleInputTrace    (void);
      void       toggleOutputTrace   (void);

};



#endif  /* _MIDIPORT_H_INCLUDED */



// md5sum:	84d8155528b06c9aa902e8f06649385f  - MidiPort.h =css= 20030102
