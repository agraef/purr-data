//
// Copyright 1997-1998 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: 19 December 1997
// Last Modified: Wed Jan 21 23:08:13 GMT-0800 1998
// Filename:      ...sig/maint/code/base/CircularBuffer/CircularBuffer.h
// Web Address:   http://sig.sapp.org/include/sigBase/CircularBuffer.cpp
// Documentation: http://sig.sapp.org/doc/classes/CircularBuffer
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

#ifndef _CIRCULARBUFFER_H_INCLUDED
#define _CIRCULARBUFFER_H_INCLUDED


template<class type>
class CircularBuffer {
   public:
                    CircularBuffer     (void);
                    CircularBuffer     (int maxElements);
                    CircularBuffer     (const CircularBuffer<type>& 
                                           anotherBuffer);
                   ~CircularBuffer     ();

      int           capacity           (void) const;
      type          extract            (void);
      int           getCount           (void) const;
      int           getSize            (void) const;
      void          insert             (const type& aMessage);
      type&         operator[]         (int index);
      type          read               (void);
      void          reset              (void);
      void          setSize            (int aSize);
      void          write              (const type& aMessage);

   protected:
      type*         buffer;
      int           size;
      int           writeIndex;
      int           readIndex;
      int           itemCount;

      void          increment          (int& index);
};


#include "CircularBuffer.cpp"



#endif  /* _CIRCULARBUFFER_H_INCLUDED */



// md5sum:	2857693ec37fdcb6df09db479faf110b  - CircularBuffer.h =css= 20030102
