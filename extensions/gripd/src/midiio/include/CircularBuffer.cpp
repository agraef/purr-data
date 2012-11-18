//
// Copyright 1997-1998 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 19 December 1997
// Last Modified: Wed Jan 21 23:16:54 GMT-0800 1998
// Filename:      ...sig/maint/code/base/CircularBuffer/CircularBuffer.cpp
// Web Address:   http://sig.sapp.org/src/sigBase/CircularBuffer.cpp
// Syntax:        C++
//
// Description:   A Circular buffer designed to handle MIDI input,
//                but able to store any type of object.  Elements
//                can be read out of the buffer in two ways. 
//                (1) from a read pointer which extracts the
//                elements in order by following the write pointer,
//                and (2) from an index operator related to the
//                write pointer's location, for example,
//                object[0] is the last value written into the
//                buffer and object[-1] (or object[1]) is the
//                item written just before that.
//              
//

#ifndef _CIRCULARBUFFER_CPP_INCLUDED
#define _CIRCULARBUFFER_CPP_INCLUDED

#include "CircularBuffer.h"
#include <stdlib.h>
#include <iostream>


//////////////////////////////
//
// CircularBuffer::CircularBuffer -- Constructor.
//

template<class type>
CircularBuffer<type>::CircularBuffer(void) {
   size = 0;
   buffer = NULL;
   reset();
}


template<class type>
CircularBuffer<type>::CircularBuffer(int maxElements) {
   if (maxElements < 0) {
      std::cerr << "Error: cannot have a negative number of elements: " 
           << maxElements << std::endl;
      exit(1);
   }
   if (maxElements == 0) {
      size = 0;
      buffer = NULL;
      reset();
   } else {
      size = maxElements;
      buffer = new type[maxElements];
      reset();
   }
}


template<class type>
CircularBuffer<type>::CircularBuffer(const CircularBuffer<type>& anotherBuffer) {
   size = anotherBuffer.size;
   if (getSize() == 0) {
      buffer = NULL;
      reset();
   } else {
      buffer = new type[getSize()];
      writeIndex = anotherBuffer.writeIndex;
      readIndex = anotherBuffer.readIndex;
      itemCount = anotherBuffer.itemCount;
      for (int i=0; i<getSize(); i++) {
         buffer[i] = anotherBuffer.buffer[i];
      }
   }
}



//////////////////////////////
//
// CircularBuffer::~CircularBuffer -- Destructor.
//    deallocates buffer memory.
//

template<class type>
CircularBuffer<type>::~CircularBuffer() {
   if (buffer != NULL) {
      delete [] buffer;
   }
}



//////////////////////////////
//
// CircularBuffer::capacity -- returns the number of items which
//    can be added to the buffer.  Returns a positive number
//    if the buffer has empty locations available.  Returns 0 if the
//    buffer is 100% full.  Returns a negative number if the
//    buffer has overflowed.

template<class type>
int CircularBuffer<type>::capacity(void) const {
   return getSize() - getCount();
}



//////////////////////////////
//
// CircularBuffer::extract -- reads the next value from the buffer.
//

template<class type>
type CircularBuffer<type>::extract(void) {
   itemCount--;
   if (itemCount < 0) {
      std::cerr << "Error: no elements in buffer to extract." << std::endl;
      exit(1);
   }
   increment(readIndex);
   return buffer[readIndex];
}



//////////////////////////////
//
// CircularBuffer::getCount -- returns the number of elements
//    between the write index and the read index.
//

template<class type>
int CircularBuffer<type>::getCount(void) const {
   return itemCount;
}



//////////////////////////////
//
// CircularBuffer::getSize -- returns the allocated size of the buffer.
//

template<class type>  
int CircularBuffer<type>::getSize(void) const {
   return size;
}



//////////////////////////////
//
// CircularBuffer::insert -- add an element to the circular buffer
//

template<class type>
void CircularBuffer<type>::insert(const type& anItem) {
   itemCount++;
   increment(writeIndex);
   buffer[writeIndex] = anItem;
}



//////////////////////////////
//
// CircularBuffer::operator[] -- access an element relative to the
//    currently written element
//

template<class type>
type& CircularBuffer<type>::operator[](int index) {
   if (buffer == NULL) {
      std::cerr << "Error: buffer has no allocated space" << std::endl;
      exit(1);
   }
   int realIndex = (index < 0) ? -index : index;
   if (realIndex >= getSize()) {
      std::cerr << "Error:   Invalid access: " << realIndex << ", maximum is "
           << getSize()-1 << std::endl;
      exit(1);
   }
   realIndex = writeIndex - realIndex;

   // should need to go through this loop a max of one time:
   while (realIndex < 0) {
      realIndex += getSize();
   }
   
   return buffer[realIndex];
}



//////////////////////////////
//
// CircularBuffer::read -- an alias for the extract function.
//

template<class type>
type CircularBuffer<type>::read(void) {
   return extract();
}



//////////////////////////////
//
// CircularBuffer::reset -- throws out all previous data and
//    sets the read/write/count to initial values.  The size
//    data variable must be valid before this function is
//    called.
//

template<class type>
void CircularBuffer<type>::reset(void) {
   readIndex = writeIndex = getSize() - 1;
   itemCount = 0;
}
 
  

//////////////////////////////
//
// CircularBuffer::setSize -- warning: will throw out all previous data 
//    stored in buffer.
//

template<class type>
void CircularBuffer<type>::setSize(int aSize) {
   if (aSize < 0) {
      std::cerr << "Error: cannot have a negative buffer size: " << aSize << std::endl;
      exit(1);
   }
   if (buffer != NULL) {
      delete [] buffer;
   }

   if (aSize == 0) {
      size = aSize;
      buffer = NULL;
      reset();
   } else {
      size = aSize;
      buffer = new type[aSize];
      reset();
   }
}   



//////////////////////////////
//
// CircularBuffer::write --  an alias for the insert function.
//

template<class type>
void CircularBuffer<type>::write(const type& anElement) {
   write(anElement);
}


///////////////////////////////////////////////////////////////////////////
//
// private functions
//

//////////////////////////////
//
// CircularBuffer::increment -- adds one to specified index and
//    will automatically wrap the index when it gets too large.
//

template<class type>
void CircularBuffer<type>::increment(int& index) {
   index++;
   if (index >= getSize()) {
      index = 0;
   }
}


#endif  /* _CIRCULARBUFFER_CPP_INCLUDED */



// md5sum:	31b2e8d6efe7398a12ddb0a1b5680ca2  - CircularBuffer.cpp =css= 20030102
