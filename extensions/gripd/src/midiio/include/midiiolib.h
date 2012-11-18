//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sat Nov  2 20:20:24 PST 2002
// Last Modified: Thu Jan  2 18:51:20 PST 2003 (changed name from midio.h)
// Filename:      ...sig/code/misc/midiiolib.h
// Web Address:   http://sig.sapp.org/include/sig/midiiolib.h
// Syntax:        C++ 
//
// Description:   Includes all of the header files for using the midiio
//                Library.
//

#ifndef _MIDIIOLIB_H_INCLUDED
#define _MIDIIOLIB_H_INCLUDED

#include "Array.h"
#include "CircularBuffer.h"
#include "Collection.h"
#include "FileIO.h"
#include "gminstruments.h"
#include "midichannels.h"
#include "mididefines.h"
#include "MidiFile.h"
#include "MidiFileWrite.h"
#include "MidiInPort_alsa05.h"
#include "MidiInPort_alsa.h"
#include "MidiInPort.h"
#include "MidiInPort_linux.h"
#include "MidiInPort_oss.h"
#include "MidiInPort_unsupported.h"
#include "MidiInPort_visual.h"
#include "MidiInput.h"
#include "MidiIO.h"
#include "MidiMessage.h"
#include "MidiOutPort_alsa.h"
#include "MidiOutPort.h"
#include "MidiOutPort_linux.h"
#include "MidiOutPort_oss.h"
#include "MidiOutPort_unsupported.h"
#include "MidiOutPort_visual.h"
#include "MidiOutput.h"
#include "MidiPort.h"
#include "notenames.h"
#include "Options.h"
#include "Options_private.h"
#include "Sequencer_alsa.h"
#include "Sequencer_oss.h"
#include "sigConfiguration.h"
#include "SigTimer.h"
#include "Voice.h"


#endif  /* _MIDIIO_H_INCLUDED */



// md5sum:	b389c20c620865344d827a88a0fb048d  - midiiolib.h =css= 20030102
