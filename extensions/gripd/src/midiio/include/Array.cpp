//
// Copyright 1997-1999 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Feb  5 19:42:53 PST 1997
// Last Modified: Sun May 11 20:41:28 GMT-0800 1997
// Last Modified: Wed Jul  7 11:44:50 PDT 1999 (added setAll() function)
// Filename:      ...sig/maint/code/base/Array/Array.cpp
// Web Address:   http://sig.sapp.org/src/sigBase/Array.cpp
// Syntax:        C++ 
//
// Description:   An array which can grow dynamically.  Array is derived from 
//                the Collection class and adds various mathematical operators
//                to the Collection class.  The Array template class is used for
//                storing numbers of any type which can be added, multiplied
//                and divided into one another.
//

#ifndef _ARRAY_CPP_INCLUDED
#define _ARRAY_CPP_INCLUDED

#include "Array.h"
#include <iostream>
#include <stdlib.h>

using namespace std;

//////////////////////////////
//
// Array::Array 
//

template<class type>
Array<type>::Array(void) : Collection<type>(4) { }

template<class type>
Array<type>::Array(int arraySize) : Collection<type>(arraySize) { }

template<class type>
Array<type>::Array(Array<type>& anArray) : Collection<type>(anArray) { }

template<class type>
Array<type>::Array(int arraySize, type *anArray) : 
   Collection<type>(arraySize, anArray) { }




//////////////////////////////
//
// Array::~Array
//

template<class type>
Array<type>::~Array() { }



//////////////////////////////
//
// Array::setAll -- sets the contents of each element to the 
//   specified value
//

template<class type>
void Array<type>::setAll(type aValue) {
  for (int i=0; i<this->getSize(); i++) {
    this->array[i] = aValue;
   }
}



//////////////////////////////
//
// Array::sum
//

template<class type>
type Array<type>::sum(void) {
   type theSum = 0;
   for (int i=0; i<this->getSize(); i++) {
     theSum += this->array[i];
   }
   return theSum;
}

template<class type>
type Array<type>::sum(int loIndex, int hiIndex) {
   type theSum = 0;
   for (int i=loIndex; i<=hiIndex; i++) {
      theSum += this->array[i];
   }
   return theSum;
}



//////////////////////////////
//
// Array::zero(-1, -1)
//

template<class type>
void Array<type>::zero(int minIndex, int maxIndex) {
   if (this->size == 0) return;
   if (minIndex == -1) minIndex = 0;
   if (maxIndex == -1) maxIndex = this->size - 1;

   if (minIndex < 0 || maxIndex < 0 || minIndex > maxIndex ||
       maxIndex >= this->size) {
     cerr << "Error in zero function: min = " << minIndex
           << " max = " << maxIndex << " size = " << this->size << this->endl;
      exit(1);
   }

   for (int i=minIndex; i<=maxIndex; i++) {
      this->array[i] = 0;
   }
}


////////////////////////////////////////////////////////////////////////////
//
// operators
//


template<class type>
int Array<type>::operator==(const Array<type>& aArray) {
   if (this->getSize() != aArray.getSize()) {
      return 0;
   }
   Array<type>& t = *this;
   int i;
   for (i=0; i<this->getSize(); i++) {
      if (t[i] != aArray[i]) {
         return 0;
      }
   }
   return 1;
}



//////////////////////////////
//
// Array::operator=
//

template<class type>
Array<type>& Array<type>::operator=(const Array<type>& anArray) {
   if (this->allocSize < anArray.size) {
      if (this->allocSize != 0) {
         delete [] this->array;
      }
      this->allocSize = anArray.size;
      this->size = anArray.size;
      this->array = new type[this->size];
      this->allowGrowthQ = anArray.allowGrowthQ;
      this->growthAmount = anArray.growthAmount;
      this->maxSize = anArray.maxSize;
   }
   this->size = anArray.size;
   for (int i=0; i<this->size; i++) {
      this->array[i] = anArray.array[i];
   }

   return *this;
}



//////////////////////////////
//
// Array::operator+=
//

template<class type>
Array<type>& Array<type>::operator+=(const Array<type>& anArray) {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   for (int i=0; i<this->size; i++) {
      this->array[i] += anArray.array[i];
   }

   return *this;
}



//////////////////////////////
//
// Array::operator+
//

template<class type>
Array<type> Array<type>::operator+(const Array<type>& anArray) const {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   Array<type> bArray(*this);
   bArray += anArray;
   return bArray;
}


template<class type>
Array<type> Array<type>::operator+(type aNumber) const {
   Array<type> anArray(*this);
   for (int i=0; i<this->size; i++) {
      anArray[i] += aNumber;
   }
   return anArray;
}



//////////////////////////////
//
// Array::operator-=
//

template<class type>
Array<type>& Array<type>::operator-=(const Array<type>& anArray) {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   for (int i=0; i<this->size; i++) {
      this->array[i] -= anArray.array[i];
   }

   return *this;
}



//////////////////////////////
//
// Array::operator-
//

template<class type>
Array<type> Array<type>::operator-(const Array<type>& anArray) const {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   Array<type> bArray(*this);
   bArray -= anArray;
   return bArray;
}


template<class type>
Array<type> Array<type>::operator-(void) const {
   Array<type> anArray(*this);
   for (int i=0; i<this->size; i++) {
      anArray[i] = -anArray[i];
   }
   return anArray;
}

template<class type>
Array<type> Array<type>::operator-(type aNumber) const {
   Array<type> anArray(*this);
   for (int i=0; i<this->size; i++) {
      anArray[i] -= aNumber;
   }
   return anArray;
}



//////////////////////////////
//
// Array::operator*=
//

template<class type>
Array<type>& Array<type>::operator*=(const Array<type>& anArray) {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   for (int i=0; i<this->size; i++) {
      this->array[i] *= anArray.array[i];
   }

   return *this;
}



//////////////////////////////
//
// Array::operator*
//

template<class type>
Array<type> Array<type>::operator*(const Array<type>& anArray) const {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   Array<type> bArray(*this);
   bArray *= anArray;
   return bArray;
}


template<class type>
Array<type> Array<type>::operator*(type aNumber) const {
   Array<type> anArray(*this);
   for (int i=0; i<this->size; i++) {
      anArray[i] *= aNumber;
   }
   return anArray;
}

//////////////////////////////
//
// Array::operator/=
//

template<class type>
Array<type>& Array<type>::operator/=(const Array<type>& anArray) {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   for (int i=0; i<this->size; i++) {
      this->array[i] /= anArray.array[i];
   }

   return *this;
}

//////////////////////////////
//
// Array::operator/
//

template<class type>
Array<type> Array<type>::operator/(const Array<type>& anArray) const {
   if (this->size != anArray.size) {
      cerr << "Error: different size arrays " << this->size << " and " 
           << anArray.size << endl;
      exit(1);
   }

   Array<type> bArray(*this);
   bArray /= anArray;
   return bArray;
}


#endif  /* _ARRAY_CPP_INCLUDED */



// md5sum:	8f52a167c93f51702ce316204fd6e722  - Array.cpp =css= 20030102
