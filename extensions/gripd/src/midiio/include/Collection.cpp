//
// Copyright 1997 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Feb  5 19:42:53 PST 1997
// Last Modified: Wed Apr 23 22:08:34 GMT-0800 1997
// Last Modified: Fri Sep 14 15:50:52 PDT 2001 (added last() function)
// Filename:      ...sig/maint/code/base/Collection/Collection.cpp
// Web Address:   http://sig.sapp.org/src/sigBase/Collection.cpp
// Syntax:        C++ 
//
// Description:   A dynamic array which can grow as necessary.
//                This class can hold any type of item, but the
//                derived Array class is specifically for collections
//                of numbers.
//

#ifndef _COLLECTION_CPP_INCLUDED
#define _COLLECTION_CPP_INCLUDED

#include "Collection.h"
#include <iostream>
#include <stdlib.h>


//////////////////////////////
//
// Collection::Collection 
//

template<class type>
Collection<type>::Collection(void) {
   allocSize = 0;
   size = 0;
   array = NULL;
   allowGrowthQ = 0;
   growthAmount = 8;
   maxSize = 0;
}

template<class type>
Collection<type>::Collection(int arraySize) {
   array = new type[arraySize];
   size = arraySize;
   allocSize = arraySize;
   allowGrowthQ = 0;
   growthAmount = arraySize;
   maxSize = 0;
}


template<class type>
Collection<type>::Collection(int arraySize, type *aCollection) {
   size = arraySize;
   allocSize = arraySize;
   array = new type[size];
   for (int i=0; i<size; i++) {
      array[i] = aCollection[i];
   }
   growthAmount = arraySize;
   allowGrowthQ = 0;
   maxSize = 0;
}


template<class type>
Collection<type>::Collection(Collection<type>& aCollection) {
   size = aCollection.size;
   allocSize = size;
   array = new type[size];
   for (int i=0; i<size; i++) {
      array[i] = aCollection.array[i];
   }
   allowGrowthQ = aCollection.allowGrowthQ;
   growthAmount = aCollection.growthAmount;
   maxSize = aCollection.maxSize;
}



//////////////////////////////
//
// Collection::~Collection
//

template<class type>
Collection<type>::~Collection() {
   if (getAllocSize() != 0) {
      delete [] array;
   }
}



//////////////////////////////
//
// Collection::allowGrowth
//	default value: status = 1 
//

template<class type>
void Collection<type>::allowGrowth(int status) {
   if (status == 0) {
      allowGrowthQ = 0;
   } else {
      allowGrowthQ = 1;
   }
}



//////////////////////////////
//
// Collection::append 
//

template<class type>
void Collection<type>::append(type& element) {
   if (size == getAllocSize()) {
      grow();
   }
   array[size] = element;
   size++;
}

template<class type>
void Collection<type>::appendcopy(type element) {
   if (size == getAllocSize()) {
      grow();
   }
   array[size] = element;
   size++;
}

template<class type>
void Collection<type>::append(type *element) {
   if (size == getAllocSize()) {
      grow();
   }
   array[size] = *element;
   size++;
}



//////////////////////////////
//
// Collection::grow 
// 	default parameter: growamt = -1
//

template<class type>
void Collection<type>::grow(long growamt) {
   allocSize += growamt > 0 ? growamt : growthAmount;
   if (maxSize != 0 && getAllocSize() > maxSize) {
      std::cerr << "Error: Maximum size allowed for array exceeded." << std::endl;
      exit(1);
   }
 
   type *temp = new type[getAllocSize()];
   for (int i=0; i<size; i++) {
      temp[i] = array[i];
   }
   array = temp;
}



//////////////////////////////
//
// Collection::pointer
//

template<class type>
type* Collection<type>::pointer(void) {
   return array;
}



//////////////////////////////
//
// Collection::getBase
//

template<class type>
type* Collection<type>::getBase(void) {
   return array;
}



//////////////////////////////
//
// Collection::getAllocSize
//

template<class type>
long Collection<type>::getAllocSize(void) const {
   return allocSize;
}



//////////////////////////////
//
// Collection::getSize --
//

template<class type>
long Collection<type>::getSize(void) const {
   return size;
}



//////////////////////////////
//
// Collection::last --
//

template<class type>
type& Collection<type>::last(void) {
   return array[getSize()-1];
}



//////////////////////////////
//
// Collection::setAllocSize
//

template<class type>
void Collection<type>::setAllocSize(long aSize) {
   if (aSize < getSize()) {
      std::cerr << "Error: cannot set allocated size smaller than actual size." 
           << std::endl;
      exit(1);
   }

   if (aSize <= getAllocSize()) {
      shrinkTo(aSize);
   } else {
      grow(aSize-getAllocSize());
      size = aSize;
   }
}



//////////////////////////////
//
// Collection::setGrowth
//	default parameter: growth = -1
//

template<class type>
void Collection<type>::setGrowth(long growth) {
   if (growth > 0) {
      growthAmount = growth;
   }
}



//////////////////////////////
//
// Collection::setSize
//

template<class type>
void Collection<type>::setSize(long newSize) {
   if (newSize <= getAllocSize()) { 
      size = newSize;
   } else {
      grow(newSize-getAllocSize());
      size = newSize;
   }
}



////////////////////////////////////////////////////////////////////////////////
//
// Collection operators
//

//////////////////////////////
//
// Collection::operator[]
//

template<class type>
type& Collection<type>::operator[](int elementIndex) {
   if (allowGrowthQ && elementIndex == size) {
      if (size == getAllocSize()) {
         grow();
      }
      size++;
   } else if (elementIndex >= size) {
      std::cerr << "Error: accessing invalid array location " 
           << elementIndex 
           << " Maximum is " << size-1 << std::endl;
      exit(1);
   }
   return array[elementIndex];
}


//////////////////////////////
//
// Collection::operator[] const
//

template<class type>
type Collection<type>::operator[](int elementIndex) const {
   if (elementIndex >= size) {
      std::cerr << "Error: accessing invalid array location " 
           << elementIndex 
           << " Maximum is " << size-1 << std::endl;
      exit(1);
   }
   return array[elementIndex];
}

//////////////////////////////
//
// shrinkTo
//

template<class type>
void Collection<type>::shrinkTo(long aSize) {
   if (aSize < getSize()) {
      exit(1);
   }

   type *temp = new type[aSize];
   for (int i=0; i<size; i++) {
      temp[i] = array[i];
   }
   delete [] array;
   array = temp;

   allocSize = aSize;
   if (size > allocSize) {
      size = allocSize;
   }
}


#endif  /* _COLLECTION_CPP_INCLUDED */



// md5sum:	9929fee30b1bede4305e1fb46303ddc1  - Collection.cpp =css= 20030102
