//
// Copyright 1997 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri May  9 22:30:32 PDT 1997
// Last Modified: Sun Dec 14 03:29:39 GMT-0800 1997
// Filename:      ...sig/maint/code/sigBase/FileIO.cpp
// Web Address:   http://sig.sapp.org/src/sigBase/FileIO.cpp
// Documentation: http://sig.sapp.org/doc/classes/FileIO
// Syntax:        C++ 
//
// Description:   Derived from the fstream class, this class has
//                functions which allow writing binary files in
//                both little and big endian formats.  Useful for
//                writing files such as soundfiles and MIDI files
//                which require numbers to be stored in a particular
//                endian format.
//

#include "FileIO.h"
#include "sigConfiguration.h"

//////////////////////////////
//
// FileIO::FileIO --
//

FileIO::FileIO(void) {
   // do nothing
};

FileIO::FileIO(const char* filename, std::ios::openmode state) :
#ifdef VISUAL                 /* for stupid LF-CR prevention in DOS */
   std::fstream(filename, state | ios::binary) { 
#else
   std::fstream(filename, state) { 
#endif
   // do nothing
};



//////////////////////////////
//
// FileIO::~FileIO --
//

FileIO::~FileIO() {
   // do nothing
}



//////////////////////////////
//
// FileIO::readBigEndian --
//	Read numbers from a file as big endian
//

void FileIO::readBigEndian(char& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(uchar& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(short& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(ushort& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(long& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(ulong& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(int& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(uint& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(float& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}

void FileIO::readBigEndian(double& aNumber) {
   #ifdef OTHEREND
      readNotMachineEndian(aNumber);
   #else
      readMachineEndian(aNumber);
   #endif
}



//////////////////////////////
//
// FileIO::readLittleEndian --
//	Read numbers from a file as little endian
//

void FileIO::readLittleEndian(char& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(uchar& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(short& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(ushort& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(long& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(ulong& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(int& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(uint& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(float& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}

void FileIO::readLittleEndian(double& aNumber) {
   #ifdef OTHEREND
      readMachineEndian(aNumber);
   #else
      readNotMachineEndian(aNumber);
   #endif
}



//////////////////////////////
//
// FileIO::readMachineEndian --
//	Read numbers from a file in the same endian as the computer.
//

void FileIO::readMachineEndian(char& aNumber) {
   this->read(&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(uchar& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(short& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(ushort& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(long& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(ulong& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(int& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(uint& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(float& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}

void FileIO::readMachineEndian(double& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
}



//////////////////////////////
//
// FileIO::readNotMachineEndian --
//	Read numbers from a file with different endian from the computer.
//

void FileIO::readNotMachineEndian(char& aNumber) {
   this->read(&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(uchar& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(short& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(ushort& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(long& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(ulong& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(int& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(uint& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(float& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}

void FileIO::readNotMachineEndian(double& aNumber) {
   this->read((char*)&aNumber, sizeof(aNumber));
   aNumber = flipBytes(aNumber);
}



//////////////////////////////
//
// FileIO::writeBigEndian --
//

void FileIO::writeBigEndian(char aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(uchar aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(short aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(ushort aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(long aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(ulong aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(int aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(uint aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(float aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}

void FileIO::writeBigEndian(double aNumber) {
   #ifdef OTHEREND
      writeNotMachineEndian(aNumber);
   #else
      writeMachineEndian(aNumber);
   #endif
}



//////////////////////////////
//
// FileIO::writeLittleEndian --
//

void FileIO::writeLittleEndian(char aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(uchar aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(short aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(ushort aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(long aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(ulong aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(int aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(uint aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(float aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}

void FileIO::writeLittleEndian(double aNumber) {
   #ifdef OTHEREND
      writeMachineEndian(aNumber);
   #else
      writeNotMachineEndian(aNumber);
   #endif
}



//////////////////////////////
//
// FileIO::writeMachineEndian --
//

void FileIO::writeMachineEndian(char aNumber) {
   this->write(&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(uchar aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(short aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(ushort aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(long aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(ulong aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(int aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(uint aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(float aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeMachineEndian(double aNumber) {
   this->write((char*)&aNumber, sizeof(aNumber));
}



//////////////////////////////
//
// FileIO::writeNotMachineEndian --
//

void FileIO::writeNotMachineEndian(char aNumber) {
   // aNumber = flipBytes(aNumber);
   this->write(&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(uchar aNumber) {
   // aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(short aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(ushort aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(long aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(ulong aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(int aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(uint aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(float aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}

void FileIO::writeNotMachineEndian(double aNumber) {
   aNumber = flipBytes(aNumber);
   this->write((char*)&aNumber, sizeof(aNumber));
}


///////////////////////////////////////////////////////////////////////////
// 
// private functions
//


//////////////////////////////
//
// flipBytes -- flip the bytes in a number
//

char FileIO::flipBytes(char aNumber) {
   return aNumber;
}


uchar FileIO::flipBytes(uchar aNumber) {
   return aNumber;
}


short FileIO::flipBytes(short aNumber) {
   static uchar output[2];
   static uchar* input;
   input = (uchar*)(&aNumber);

   output[0] = input[1];
   output[1] = input[0];

   return *((short*)(&output));
}


ushort FileIO::flipBytes(ushort aNumber) {
   static uchar output[2];
   static uchar* input;
   input = (uchar*)(&aNumber);

   output[0] = input[1];
   output[1] = input[0];
 
   return *((ushort*)(&output));
}


long FileIO::flipBytes(long aNumber) {
   static uchar output[4];
   static uchar* input;
   input = (uchar*)(&aNumber);

   output[0] = input[3];
   output[1] = input[2];
   output[2] = input[1];
   output[3] = input[0];

   return *((long*)(&output));
}


ulong FileIO::flipBytes(ulong aNumber) {
   static uchar output[4];
   static uchar* input;
   input = (uchar*)(&aNumber);

   output[0] = input[3];
   output[1] = input[2];
   output[2] = input[1];
   output[3] = input[0];

   return *((ulong*)(&output));
}


int FileIO::flipBytes(int aNumber) {
   static uchar output[sizeof(uint)];
   static uchar* input;
   input = (uchar*)(&aNumber);

   for(uint i=0; i<sizeof(int); i++) {
      output[i] = input[sizeof(int)-1-i];
   }

   return *((int*)(&output));
}


uint FileIO::flipBytes(uint aNumber) {
   static uchar output[sizeof(uint)];
   static uchar* input;
   input = (uchar*)(&aNumber);

   for(uint i=0; i<sizeof(uint); i++) {
      output[i] = input[sizeof(uint)-1-i];
   }

   return *((uint*)(&output));
}

 
   
float FileIO::flipBytes(float aNumber) {
   static uchar output[4];
   static uchar* input;
   input = (uchar*)(&aNumber);

   output[0] = input[3];
   output[1] = input[2];
   output[2] = input[1];
   output[3] = input[0];

   return *((float*)(&output));
}


double FileIO::flipBytes(double aNumber) {
   static uchar output[8];
   static uchar* input;
   input = (uchar*)(&aNumber);

   output[0] = input[7];
   output[1] = input[6];
   output[2] = input[5];
   output[3] = input[4];
   output[4] = input[3];
   output[5] = input[2];
   output[6] = input[1];
   output[7] = input[0];

   return *((double*)(&output));
}



/*   This is what I want to use
template<class type>
type FileIO::flipBytes(type aThing) {
   uchar* input = (uchar*)(&aNumber);
   uchar output[sizeof(aThing)];

   for(int i=0; i<sizeof(aThing); i++) {
      output[i] = input[sizeof(aThing) - 1 - i];
   }

   return *((type*)(&output));
}
*/
 
   

// md5sum:	a82bcb961043a48d2cae34d5eaac0101  - FileIO.cpp =css= 20030102
