//
// Copyright 1997-1999 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Wed Feb  5 19:42:53 PST 1997
// Last Modified: Sun May 11 20:33:13 GMT-0800 1997
// Last Modified: Wed Jul  7 11:44:50 PDT 1999 (added setAll() function)
// Last Modified: Mon Jul 29 22:08:32 PDT 2002 (added operator==) 
// Filename:      ...sig/maint/code/base/Array/Array.h
// Web Address:   http://sig.sapp.org/include/sigBase/Array.h
// Documentation: http://sig.sapp.org/doc/classes/Array
// Syntax:        C++ 
//
// Description:   An array which can grow dynamically.  Array is derived from 
//                the Collection class and adds various mathematical operators
//                to the Collection class.  The Array template class is used for
//                storing numbers of any type which can be added, multiplied
//                and divided into one another.
//

#ifndef _ARRAY_H_INCLUDED
#define _ARRAY_H_INCLUDED

#include "Collection.h"


template<class type>
class Array : public Collection<type> {
   public:
                     Array             (void);
                     Array             (int arraySize);
                     Array             (Array<type>& aArray);
                     Array             (int arraySize, type *anArray);
                    ~Array             ();

      void           setAll            (type aValue);
      type           sum               (void);
      type           sum               (int lowIndex, int hiIndex);
      void           zero              (int minIndex = -1, int maxIndex = -1);

      int            operator==        (const Array<type>& aArray);
      Array<type>&   operator=         (const Array<type>& aArray);
      Array<type>&   operator+=        (const Array<type>& aArray);
      Array<type>&   operator-=        (const Array<type>& aArray);
      Array<type>&   operator*=        (const Array<type>& aArray);
      Array<type>&   operator/=        (const Array<type>& aArray);

      Array<type>    operator+         (const Array<type>& aArray) const;
      Array<type>    operator+         (type aNumber) const;
      Array<type>    operator-         (const Array<type>& aArray) const;
      Array<type>    operator-         (void) const;

      Array<type>    operator-         (type aNumber) const;
      Array<type>    operator*         (const Array<type>& aArray) const;
      Array<type>    operator*         (type aNumber) const;
      Array<type>    operator/         (const Array<type>& aArray) const;
};


#include "Array.cpp"   /* necessary for templates */



#endif  /* _ARRAY_H_INCLUDED */



// md5sum:	09d1b1f8e70ecde53f484548e48f33c3  - Array.h =css= 20030102
