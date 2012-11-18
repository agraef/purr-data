//
// Copyright 1997 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Feb  5 19:42:53 PST 1997
// Last Modified: Tue Apr 22 20:28:16 GMT-0800 1997
// Last Modified: Fri Sep 14 15:50:52 PDT 2001 (added last() function)
// Filename:      ...sig/maint/code/base/Collection/Collection.h
// Web Address:   http://sig.sapp.org/include/sigBase/Collection.h
// Documentation: http://sig.sapp.org/doc/classes/Collection
// Syntax:        C++ 
//
// Description:   A dynamic array which can grow as necessary.
//                This class can hold any type of item, but the
//                derived Array class is specifically for collections
//                of numbers.
//

#ifndef _COLLECTION_H_INCLUDED
#define _COLLECTION_H_INCLUDED


template<class type>
class Collection {
   public:
                Collection        (void);
                Collection        (int arraySize);
                Collection        (int arraySize, type *aCollection);
                Collection        (Collection<type>& aCollection);
               ~Collection        ();

      void      allowGrowth       (int status = 1);
      void      append            (type& element);
      void      appendcopy        (type element);
      void      append            (type* element);
      type     *getBase           (void);
      long      getAllocSize      (void) const;
      long      getSize           (void) const;
      type     *pointer           (void);
      void      setAllocSize      (long aSize);
      void      setGrowth         (long growth);
      void      setSize           (long newSize);
      type&     operator[]        (int arrayIndex);
      type      operator[]        (int arrayIndex) const;
      void      grow              (long growamt = -1);
      type&     last              (void);


   protected:
      long      size;             // actual array size
      long      allocSize;        // maximum allowable array size
      type     *array;            // where the array data is stored
      char      allowGrowthQ;     // allow/disallow growth
      long      growthAmount;     // number of elements to grow by if index
				  //    element one beyond max size is accessed
      long maxSize;               // the largest size the array is allowed 
                                  //    to grow to, if 0, then ignore max
  
      void      shrinkTo          (long aSize);
};


#include "Collection.cpp"



#endif  /* _COLLECTION_H_INCLUDED */



// md5sum:	01bec04835c0bd117f40c2bfe51c4abd  - Collection.h =css= 20030102
