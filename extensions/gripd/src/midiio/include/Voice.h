//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Oct 11 18:21:46 PDT 1998
// Last Modified: Sun Oct 11 18:33:04 PDT 1998
// Filename:      ...sig/maint/code/control/Voice/Voice.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/Voice.h
// Syntax:        C++
//
// Description:   The Voice class is a MIDI output class which keeps
//                track of the last note played in to it.  If the last
//                note was not turned off when a new note is played,
//                then the old note will be turned off before the 
//                new note is played.
//

#ifndef _VOICE_H_INCLUDED
#define _VOICE_H_INCLUDED

#include "MidiOutput.h"

class Voice : public MidiOutput {
   public:
                   Voice          (int aChannel);
                   Voice          (const Voice& aVoice);
                   Voice          (void);
                  ~Voice          ();

      void         cont           (int controler, int data);
      int          getChan        (void) const;
      int          getChannel     (void) const;
      int          getKey         (void) const;
      int          getKeynum      (void) const;
      int          getOffTime     (void) const;
      int          getOnTime      (void) const;
      int          getVel         (void) const;
      int          getVelocity    (void) const;
      void         off            (void);
      void         pc             (int aTimbre);
      void         play           (int aChannel, int aKeyno, int aVelocity);
      void         play           (int aKeyno, int aVelocity);
      void         play           (void);
      void         setChan        (int aChannel);
      void         setChannel     (int aChannel);
      void         setKey         (int aKeyno);
      void         setKeynum      (int aKeyno);
      void         setVel         (int aVelocity);
      void         setVelocity    (int aVelocity);
      int          status         (void) const;
      void         sustain        (int aStatus);

   protected:
      int          chan;         // the current channel number
      int          key;          // the current key number
      int          vel;          // the current velocity value

      int          onTime;       // last note on message sent
      int          offTime;      // last note off message sent

      int          oldChan;      // last channel played on
      int          oldKey;       // last key to be played
      int          oldVel;       // last velocity of last key

      static SigTimer timer;     // for recording on/off times
};


#endif /* _VOICE_H_INCLUDED */


// md5sum:	8a5495ecc10d42be6b1832492e107723  - Voice.h =css= 20030102
