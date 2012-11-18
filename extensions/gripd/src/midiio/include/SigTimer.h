//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Thanks to:     Erik Neuenschwander <erikn@leland.stanford.edu>
//                   for Windows 95 assembly code for Pentium clock cycles.
//                Ozgur Izmirli <ozgur@ccrma.stanford.edu> 
//                   for concept of periodic timer.
// Creation Date: Mon Oct 13 11:34:57 GMT-0800 1997
// Last Modified: Tue Feb 10 21:05:19 GMT-0800 1998
// Last Modified: Sat Sep 19 15:56:48 PDT 1998
// Last Modified: Mon Feb 22 04:44:25 PST 1999
// Last Modified: Sun Nov 28 12:39:39 PST 1999   (added adjustPeriod())
// Filename:      .../sig/code/control/SigTimer/SigTimer.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/SigTimer.h
// Syntax:        C++ 
//
// Description:   This class can only be used on Motorola Pentinum 75 Mhz
//                chips or better because the timing information is
//                extracted from the clock cycle count from a register
//                on the CPU itself.  This class will estimate the 
//                speed of the computer, but it would be better if there
//                was a way of finding out the speed from some function.
//                This class is used primarily for timing of MIDI input 
//                and output at a millisecond resolution.
//
// Interesting:   http://www.datasilicon.nl/I786/timer_1.htm
//

#ifndef _SIGTIMER_H_INCLUDED
#define _SIGTIMER_H_INCLUDED


#ifdef VISUAL
   #include <wtypes.h>
   typedef LONGLONG int64bits;
#else
   typedef long long int int64bits;
   #include <unistd.h>                 /* for millisleep function */
#endif


class SigTimer {
   public:
                       SigTimer           (void);
                       SigTimer           (int aSpeed);
                       SigTimer           (SigTimer& aTimer);
                      ~SigTimer           ();

      void             adjustPeriod       (double periodDelta);
      int              expired            (void) const;
      double           getPeriod          (void) const;
      double           getPeriodCount     (void) const;
      double           getTempo           (void) const;
      int              getTicksPerSecond  (void) const;
      int              getTime            (void) const;
      double           getTimeInSeconds   (void) const;
      int              getTimeInTicks     (void) const;
      void             reset              (void);
      void             setPeriod          (double aPeriod);
      void             setTempo           (double beatsPerMinute);
      void             setPeriodCount     (double aCount);
      void             setTicksPerSecond  (int aTickRate);
      void             start              (void);
      void             sync               (SigTimer& aTimer);
      void             update             (void);
      void             update             (int periodCount);

      // The following functions are semi-private.  They do not have
      // anything to do with timing themselves, but are a by-product
      // of the timer implementation.  They are useful, so they have
      // been left public; however, they should be used judiciously.
      static int       getCpuSpeed        (void);
      static int       measureCpuSpeed    (int quantize = 0);
      static void      setCpuSpeed        (int aSpeed);
      
      // the following function is hardware specific to Intel Pentium
      // computers with a processor speed of at least 75 MHz.
      // This function is the only non-portable function in this
      // class, but everything else is based on it.
      static int64bits clockCycles        (void);

   protected:
      static int64bits globalOffset;
      static int       cpuSpeed;         

      int64bits        offset;          
      int              ticksPerSecond;    
      double           period;

   // protected functions
      double           getFactor          (void) const;

};


// The following function is mostly for Linux:
void millisleep(int milliseconds);
void millisleep(float milliseconds);

   
#endif  /* _SIGTIMER_H_INCLUDED */



// md5sum:	601fa3caae4e3bacc4e6fb87f545c86b  - SigTimer.h =css= 20030102
