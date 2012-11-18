//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Mar 15 10:55:56 GMT-0800 1998
// Last Modified: Sun Mar 15 10:55:56 GMT-0800 1998
// Filename:      ...sig/code/control/MidiFileWrite/MidiFileWrite.cpp
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/src/MidiFileWrite.cpp
// Syntax:        C++ 
//
// Description:   The MidiFileWrite class will write out a Type 0 MidiFile.
//                Used for recording MIDI data streams into Standard
//                MIDI files.
//

#include "MidiFileWrite.h"
#include "SigTimer.h"
#include <assert.h>


//////////////////////////////
//
// MidiFileWrite::MidiFileWrite
//    default value: startTime = -1
//

MidiFileWrite::MidiFileWrite(void) {
   trackSize = 0;
   lastPlayTime = 0;
   midifile = NULL;
   openQ = 0;
}


MidiFileWrite::MidiFileWrite(const char* aFilename, int startTime) {
   trackSize = 0;
   lastPlayTime = 0;
   midifile = NULL;
   openQ = 0;
   setup(aFilename, startTime);
}



//////////////////////////////
//
// MidiFileWrite::~MidiFileWrite
//

MidiFileWrite::~MidiFileWrite() {
   close();
}



//////////////////////////////
//
// MidiFileWrite::close
//

void MidiFileWrite::close(void) {
   writeRaw(0, 0xff, 0x2f, 0);       // end of track meta event

   midifile->seekg(18);
   midifile->writeBigEndian(trackSize);

   midifile->close();

   midifile = NULL;
   openQ = 0;
}



//////////////////////////////
//
// MidiFileWrite::setup -- writes the Midi file header and
//	prepares the midifile for writing of data
//   default value: startTime = -1
//

void MidiFileWrite::setup(const char* aFilename, int startTime) {
   if (openQ) {
      close();
   }

   if (midifile != NULL)  delete midifile;
   midifile = new FileIO;
   midifile->open(aFilename, std::ios::out);
   
   // write the header chunk
   *midifile << "MThd";                    // file identification: MIDI file
   midifile->writeBigEndian(6L);          // size of header (always 6)
   midifile->writeBigEndian((short)0);    // format: type 0;
   midifile->writeBigEndian((short)0);    // num of tracks (always 0 for type 0)
   midifile->writeBigEndian((short)1000); // divisions per quarter note
   

   // write the track header
   *midifile << "MTrk"; 
   midifile->writeBigEndian(0xffffL);     // the track size which will
                                          // be corrected with close()


   // the midifile stream is now setup for writing
   // track events

   openQ = 1;
 
   start();  // start can be called later and will behave well
             // as long as no track events have been written
}



//////////////////////////////
//
// MidiFileWrite::start
//	default value: startTime = -1;
//

void MidiFileWrite::start(int startTime) {
   if (startTime < 0) {
      SigTimer localTime;
      lastPlayTime = localTime.getTime();
   } else {
      lastPlayTime = startTime;
   }
}



//////////////////////////////
//
// MidiFileWrite::writeAbsolute -- considers the time data
//	to be the current time.  It will generate a difference
//	time with the previously stored last playing time.
//

void MidiFileWrite::writeAbsolute(int aTime, int command, int p1, int p2) {
   writeVLValue(aTime - lastPlayTime);
   writeRaw((uchar)command, (uchar)p1, (uchar)p2);
   lastPlayTime = aTime;
}

void MidiFileWrite::writeAbsolute(int aTime, int command, int p1) {
   writeVLValue(aTime - lastPlayTime);
   writeRaw((uchar)command, (uchar)p1);
   lastPlayTime = aTime;
}

void MidiFileWrite::writeAbsolute(int aTime, int command) {
   writeVLValue(aTime - lastPlayTime);
   writeRaw((uchar)command);
   lastPlayTime = aTime;
}



//////////////////////////////
//
// MidiFileWrite::writeRaw -- write an event byte to the midifile
//

void MidiFileWrite::writeRaw(uchar aByte) {
   assert(midifile != NULL);
   *midifile << aByte;
   trackSize++;
}


void MidiFileWrite::writeRaw(uchar aByte, uchar bByte) {
   writeRaw(aByte);
   writeRaw(bByte);
}


void MidiFileWrite::writeRaw(uchar aByte, uchar bByte, uchar cByte) {
   writeRaw(aByte);
   writeRaw(bByte);
   writeRaw(cByte);
}


void MidiFileWrite::writeRaw(uchar aByte, uchar bByte, uchar cByte, 
      uchar dByte) {
   writeRaw(aByte);
   writeRaw(bByte);
   writeRaw(cByte);
   writeRaw(dByte);
}


void MidiFileWrite::writeRaw(uchar aByte, uchar bByte, uchar cByte, 
      uchar dByte, uchar eByte) {
   writeRaw(aByte);
   writeRaw(bByte);
   writeRaw(cByte);
   writeRaw(dByte);
   writeRaw(eByte);
}


void MidiFileWrite::writeRaw(uchar* anArray, int arraySize) {
   for (int i=0; i<arraySize; i++) {
      writeRaw(anArray[i]);
   }
}



//////////////////////////////
//
// MidiFileWrite::writeRelative -- cosiders the time data
//	to be a delta time from the last input message.
//

void MidiFileWrite::writeRelative(int aTime, int command, int p1, int p2) {
   writeVLValue(aTime);
   writeRaw((uchar)command, (uchar)p1, (uchar)p2);
   lastPlayTime += aTime;
}

void MidiFileWrite::writeRelative(int aTime, int command, int p1) {
   writeVLValue(aTime);
   writeRaw((uchar)command, (uchar)p1);
   lastPlayTime += aTime;
}

void MidiFileWrite::writeRelative(int aTime, int command) {
   writeVLValue(aTime);
   writeRaw((uchar)command);
   lastPlayTime += aTime;
}


//////////////////////////////
//
// MidiFileWrite::writeVLValue -- write a number to the midifile
//    as a variable length value which segments a file into 7-bit
//    values.  Maximum size of aValue is 0x7fffffff
//

void MidiFileWrite::writeVLValue(long aValue) {
   uchar bytes[5];
    bytes[0] = (uchar)((aValue >> 28) & 0x7f);    // most significant 5 bits
    bytes[1] = (uchar)((aValue >> 21) & 0x7f);    // next largest 7 bits
    bytes[2] = (uchar)((aValue >> 14) & 0x7f);
    bytes[3] = (uchar)((aValue >> 7)  & 0x7f);
    bytes[4] = (uchar)((aValue)       & 0x7f);    // least significant 7 bits

   int start = 0;
   while (start<5 && bytes[start] == 0)  start++;

   for (int i=start; i<4; i++) {
      writeRaw((uchar)(bytes[i] | 0x80));
   }
   writeRaw(bytes[4]);
}

// md5sum:	251468fa23862745f0cf36f359bccc17  - MidiFileWrite.cpp =css= 20030102
