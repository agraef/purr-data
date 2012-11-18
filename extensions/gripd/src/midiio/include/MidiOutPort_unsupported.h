//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Mon Jan 12 21:36:26 GMT-0800 1998
// Last Modified: Mon Jan 12 21:36:31 GMT-0800 1998
// Filename:      ...sig/code/control/MidiOutPort/unsupported/MidiOutPort_unsupported.h
// Web Address:   http://www-ccrma.stanford.edu/~craig/improv/include/MidiOutPort_unsupported.h
// Syntax:        C++ 
//
// Description:   Operating-System specific interface for basic MIDI output
//                capabilities in an unknown operating system.  Privately 
//                inherited by the MidiOutPort class. Used for compiling
//                and running MIDI programs on a computer with no
//                MIDI output.
//

#ifndef _MIDIOUTPUT_UNSUPPORTED_H_INCLUDED
#define _MIDIOUTPUT_UNSUPPORTED_H_INCLUDED

typedef unsigned char uchar;

class MidiOutPort_unsupported {
   public:
                        MidiOutPort_unsupported  (void);
                        MidiOutPort_unsupported  (int aPort, int autoOpen = 1);
                       ~MidiOutPort_unsupported  ();

      void              close                    (void);
      void              closeAll                 (void);
      int               getChannelOffset         (void) const;
      const char*       getName                  (void) const;
      const char*       getName                  (int i) const;
      int               getPort                  (void) const;
      int               getNumPorts              (void) const;
      int               getPortStatus            (void) const;
      int               getTrace                 (void) const;
      int               rawsend                  (int command, int p1, int p2);
      int               rawsend                  (int command, int p1);
      int               rawsend                  (int command);
      int               rawsend                  (uchar* array, int size);
      int               open                     (void);
      void              setChannelOffset         (int aChannel);
      void              setPort                  (int aPort);
      int               setTrace                 (int aState);
      int               sysex                    (uchar* array, int size);
      void              toggleTrace              (void);

   protected:
      int    port;     // the port to which this object belongs
      int    trace;    // for printing out midi messages to standard output
 
      static int        objectCount;     // num of similar objects in existence
      static int*       portObjectCount; // objects connected to particular port
      static int*       openQ;           // for open/close status of port
      static int        numDevices;      // number of output ports

   private:
      void              deinitialize             (void); 
      void              initialize               (void); 
      void              setPortStatus            (int aStatus);
      
      static int      channelOffset;     // channel offset, either 0 or 1
                                         // not being used right now.
};



#endif  /* _MIDIOUTPUT_UNSUPPORTED_H_INCLUDED */



// md5sum:	e244688a99d220addc7b1c6f6f6a8022  - MidiOutPort_unsupported.h =css= 20030102
