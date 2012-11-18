//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Mar 15 10:55:56 GMT-0800 1998
// Last Modified: Sun Mar 15 10:55:56 GMT-0800 1998
// Filename:      ...sig/code/control/MidiFileWrite/MidiFileWrite.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiFileWrite.h
// Syntax:        C++ 
//
// Description:   The MidiFileWrite class will write out a Type 0 MidiFile.
//                Used for recording MIDI data streams into Standard
//                MIDI files.
//

#ifndef _MIDIFILEWRITE_INCLUDED
#define _MIDIFILEWRITE_INCLUDED


#include "FileIO.h"


class MidiFileWrite {
   public:
                MidiFileWrite     (void);
                MidiFileWrite     (const char* aFilename, int startTime = -1);
               ~MidiFileWrite     ();

      void      close             (void); 
      void      setup             (const char* aFilename, int startTime = -1); 
      void      start             (int startTime = -1); 
      void      writeAbsolute     (int aTime, int command, int p1, int p2);
      void      writeAbsolute     (int aTime, int command, int p1);
      void      writeAbsolute     (int aTime, int command);
      void      writeRaw          (uchar aByte);
      void      writeRaw          (uchar aByte, uchar Byte);
      void      writeRaw          (uchar aByte, uchar Byte, uchar cByte);
      void      writeRaw          (uchar aByte, uchar Byte, uchar cByte, 
                                     uchar dByte);
      void      writeRaw          (uchar aByte, uchar Byte, uchar cByte, 
                                     uchar dByte, uchar eByte);
      void      writeRaw          (uchar* anArray, int arraySize);
      void      writeRelative     (int aTime, int command, int p1, int p2);
      void      writeRelative     (int aTime, int command, int p1);
      void      writeRelative     (int aTime, int command);
      void      writeVLValue      (long aValue);

    
   protected:
      FileIO   *midifile;         // file stream for MIDI file
      long      trackSize;        // size count for MIDI track
      int       lastPlayTime;     // for calculating delta times
      int       openQ;            // for checking file status

};



#endif  /* _MIDIFILEWRITE_INCLUDED */



// md5sum:	44ac572078bff648d096c7e7867d1b3c  - MidiFileWrite.h =css= 20030102
