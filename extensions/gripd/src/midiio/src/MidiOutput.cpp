//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu> 
// Creation Date: 18 December 1997
// Last Modified: Mon Jan 26 23:54:36 GMT-0800 1998
// Last Modified: Tue Feb  2 08:30:28 PST 1999
// Last Modified: Sun Jul 18 18:52:29 PDT 1999 (added RPN functions)
// Last Modified: Sun Dec  9 15:01:33 PST 2001 (switched con/des code)
// Filename:      ...sig/code/control/MidiOutput/MidiOutput.cpp
// Web Address:   http://sig.sapp.org/src/sig/MidiOutput.cpp
// Syntax:        C++
//
// Description:   The MIDI output interface for MIDI synthesizers/equipment
//                which has many convienience functions defined for
//                various types of MIDI output.
//

#include "MidiOutput.h"
#include <iostream>
#include <iomanip>

#define RECORD_ASCII     (0)
#define RECORD_BINARY    (1)
#define RECORD_MIDI_FILE (2)


// declaration of static variables
SigTimer    MidiOutput::timer;
Array<int>* MidiOutput::rpn_lsb_status = NULL;
Array<int>* MidiOutput::rpn_msb_status = NULL;
int         MidiOutput::objectCount    = 0;


//////////////////////////////
//
// MidiOutput::MidiOutput --
//


MidiOutput::MidiOutput(void) : MidiOutPort() {
   outputRecordQ = 0;

   if (objectCount == 0) {
      initializeRPN();
   }
   objectCount++;
}



MidiOutput::MidiOutput(int aPort, int autoOpen) : MidiOutPort(aPort, autoOpen) {
   outputRecordQ = 0;

   if (objectCount == 0) {
      initializeRPN();
   }
   objectCount++;
}



//////////////////////////////
//
// MidiOutput::~MidiOutput
//

MidiOutput::~MidiOutput() {
   objectCount--;
   if (objectCount == 0) {
      deinitializeRPN();
   } else if (objectCount < 0) {
      std::cout << "Error in MidiOutput decontruction" << std::endl; 
   }

}



//////////////////////////////
//
// MidiOutput::cont -- send a controller command MIDI message.
//
//    channel = the Midi channel ofset from 0 [0..15]
//    controller = the continuous controller number [0..127]
//    data = the value of the specified controller [0..127]
//

int MidiOutput::cont(int channel, int controller, int data) {
   return send(0xb0 | (channel & 0x0f), controller, data);
}



//////////////////////////////
//
// MidiOutput::off -- sends a Note Off MIDI message (0x80).
//
//    channel = MIDI channel to send note on. range is [0..15]
//    keynum = MIDI key number to play (middle C = 60, C# = 61, etc.) [0..127]
//    velocity = release velocity of the note, 127 = quickest possible
//
// Note: The more common method of turning off a note is to use the
//	play() function (midi command 0x90) but send an attack velocity of 0.
//

int MidiOutput::off(int channel, int keynum, int releaseVelocity) {
   return send(0x80 | (channel & 0x0f), keynum, releaseVelocity);
}



//////////////////////////////
//
// MidiOutput::pc -- send a patch change MIDI message. changes the timbre
//    on the specified channel.  
//
//    channel = MIDI channel to which to send the patch change [0..15]
//    timbre = the voice to select on the specified channel [0..127]
//

int MidiOutput::pc(int channel, int timbre) {
   return send(0xc0 | (channel & 0x0f), timbre);
}



//////////////////////////////
//
// MidiOutput::play -- sends a Note On/Off MIDI message.
//
//    channel = MIDI channel to send note on. range is [0..15]
//    keynum = MIDI key number to play (middle C = 60, C# = 61, etc.) [0..127]
//    velocity = attack velocity of the note, 0 = 0ff, 127 = loudest possible
//

int MidiOutput::play(int channel, int keynum, int velocity) {
   return send(0x90 | (channel & 0x0f), keynum, velocity);
}



//////////////////////////////
//
// MidiOutput::pw -- Pitch Wheel: send a MIDI pitch bend.
//      Parameters are:
//         1. channel   -- MIDI channel offset from 0.
//         2. mostByte  -- most significant 7 bits (coarse tuning)
//         3. leastByte -- least significant 7 bits (fine tuning)
//

int MidiOutput::pw(int channel, int mostByte, int leastByte) {
   return send(0xe0 | (channel & 0x0f), leastByte, mostByte);
}



//////////////////////////////
//
// MidiOutput::pw -- Pitch Wheel: 14 bit number given as input
//      range for 14 bit number is 0 to 16383.
//

int MidiOutput::pw(int channel, int tuningData) {
   uchar greaterBits = (uchar)((tuningData >> 7) & 0x7f);
   uchar lesserBits = (uchar)(tuningData & 0x7f);
   return pw(channel, greaterBits, lesserBits);
}



//////////////////////////////
//
// MidiOutput::pw -- Pitch Wheel: range between -1 to 1 given as input.
//      Range is then converted to a 14 bit number.
//      +1 = highest value of pitch wheel
//       0 = rest position of pitch wheel
//      -1 = lowest value of pitch wheel
//

int MidiOutput::pw(int channel, double tuningData) {
   if (tuningData < -1.0 || tuningData > 1.0) {
      std::cerr << "Error: pitch wheel data is out of range: " << tuningData << std::endl;
      exit(1);
   }

   int output = (int)((tuningData+1.0)/2.0*16383 + 0.5);
   return pw(channel, output);
}



//////////////////////////////
//
// MidiOutput::recordStart
//

void MidiOutput::recordStart(char *filename, int format) {
   if (outputRecordQ) {  // already recording, so close old file
      recordStop();
   }

   outputRecordFile.open(filename, std::ios::out);
   if (!outputRecordFile) {   // open file failed
      std::cerr << "Error: cannot open file " << filename << std::endl;
      outputRecordQ = 0;
   } else {
      outputRecordQ = 1;
   }

   if (outputRecordQ) {
      switch (format) {
         case RECORD_ASCII:   // ascii
            outputRecordType = RECORD_ASCII;
            outputRecordFile <<"; delta time/MIDI output at delta time" << std::endl;
            break;
         case RECORD_BINARY:   // binary
            outputRecordType = RECORD_BINARY;
            // record the magic number for binary format
            outputRecordFile << (uchar)0xf8 << (uchar)0xf8
                       << (uchar)0xf8 << (uchar)0xf8;
            break;
         case RECORD_MIDI_FILE: // standard MIDI file, type 0
         default:
            outputRecordType = RECORD_MIDI_FILE;
            // header stuff to be written here
            break;
      }
   }

   lastFlushTime = timer.getTime();
}



//////////////////////////////
//
// MidiOutput::recordStop
//

void MidiOutput::recordStop(void) {
   if (outputRecordQ) {
      outputRecordQ = 0;
      outputRecordFile.close();
   }
}



//////////////////////////////
//
// MidiOutput::reset -- sends the MIDI command 0xFF which
//      should force the MIDI devices on the other side of the
//      MIDI cable into their power-on reset condition, clear running
//      status, turn off any sounding notes, set Local Control on, and
//      otherwise clean up the state of things.
//

void MidiOutput::reset(void) {
   send(0xff, 0, 0);
}



//////////////////////////////
//
// MidiOutput::send -- send a byte to the MIDI port but record it
//	first.
//

int MidiOutput::send(int command, int p1, int p2) {
   if (outputRecordQ) {
      switch (outputRecordType) {
         case 0:   // ascii
            writeOutputAscii(command, p1, p2);
            break;
         case 1:   // binary
            writeOutputBinary(command, p1, p2);
            break;
         case 2:   // standard MIDI file type 0
            writeOutputMidifile(command, p1, p2);
            break;
      }
      lastFlushTime = timer.getTime();  // only keep track if recording
   }
   return rawsend(command, p1, p2);
}


int MidiOutput::send(int command, int p1) {
   if (outputRecordQ) {
      switch (outputRecordType) {
         case 0:   // ascii
            writeOutputAscii(command, p1, -1);
            break;
         case 1:   // binary
            writeOutputBinary(command, p1, -1);
            break;
         case 2:   // standard MIDI file type 0
            writeOutputMidifile(command, p1, -1);
            break;
      }
      lastFlushTime = timer.getTime();  // only keep track if recording
   }
   return rawsend(command, p1);
}


int MidiOutput::send(int command) {
   if (outputRecordQ) {
      switch (outputRecordType) {
         case 0:   // ascii
            writeOutputAscii(command, -1, -1);
            break;
         case 1:   // binary
            writeOutputBinary(command, -1, -1);
            break;
         case 2:   // standard MIDI file type 0
            writeOutputMidifile(command, -1, -1);
            break;
      }
      lastFlushTime = timer.getTime();  // only keep track if recording
   }
   return rawsend(command);
}



//////////////////////////////
//
// MidiOutput::silence -- send a note off to all notes on all channels.
//    default value: aChannel = -1
//

void MidiOutput::silence(int aChannel) {
   int keyno;
   if (aChannel == -1) {
      for (int channel=0; channel<16; channel++) {
         for (keyno=0; keyno<128; keyno++) {
            play(channel, keyno, 0);
         }
      }
   } else {
      for (keyno=0; keyno<128; keyno++) {
         play(aChannel, keyno, 0);
      }
   }
}



//////////////////////////////
//
// MidiOutput::sustain -- set the MIDI sustain continuous controller on or off.
//	Equivalent to the command cont(channel, 0x40, status).
//

void MidiOutput::sustain(int channel, int status) {
   if (status) {  // turn on sustain
      cont(channel, 0x40, 127);
   } else {       // turn off sustain
      cont(channel, 0x40, 0);
   }
}



///////////////////////////////
//
// MidiOutput::sysex -- sends a system exclusive MIDI message.
//	you must supply the 0xf0 at the start of the array
//	and the 0xf7 at the end of the array.
//

int MidiOutput::sysex(char* data, int length) {
   return rawsend((uchar*)data, length);
}


int MidiOutput::sysex(uchar* data, int length) {
   return rawsend(data, length);
}


///////////////////////////////////////////////////////////////////////////
//
// RPN functions
//


//////////////////////////////
//
// NRPN -- sends a Non-registered parameter number where:
//   parameter #1: channel (0-15)
//   parameter #2: NRPN MSB indicator (controller #99 data)
//   parameter #3: NRPN LSB indicator (controller #98 data)
//   parameter #4: NRPN MSB data      (controller #6  data)
//  [parameter #5: NRPN LSB data      (controller #38 data)]
// or:
//   parameter #1: channel (0-15)
//   parameter #2: NRPN MSB indicator (controller #99 data)
//   parameter #3: NRPN LSB indicator (controller #98 data)
//   parameter #4: NRPN Floating point in range (-1..1) (ccont#6 and #38 data)
//
//
// NRPN (Non-registered parameter number) -- General MIDI and 
//    Extended MIDI mess.  It becomes the receiving synthesizer's
//    responsibility to determine the meaning of continuous
//    controller (ccont) #6 from data sent with cconts #98,99 (for
//    NRPNS) and cconts #100,101 (for RPNS).  NRPN parameters
//    are not reset when the ccont#121 is sent to reset controllers.
//
// NRPN's are "non-standard" meaning that any synthesizer could
//    do whatever they want with a given NRPN;  However, the 
//    GS and XG specifications are given in the table further below.
//
// The data for NRPNs are transfered to synthesizer with 
//    data slider ccont #6(MSB) and ccont #38(LSB).  Also data increment
//    ccont#96 (data increment) and ccont#97 (data decrement) are in
//    relation to the RPN or NRPN in effect.  Increment and Decrement
//    are not recommend to use with RPN's because of confusion in the
//    MIDI industry over which 7-bit data (#6 or #38) to increment.
//  
// Once you have selected an NRPN on a given channel, the
//    channel will apply subsequent Data Entry to the 
//    selected parameter.  After making the necessary settings
//    you should set NRPN to NULL to reduce the risk of 
//    operational errors.  a NUL RPN will disable the previous
//    values of either RPN or NRPN data.
// 
// The following NRPN values are supported in Yamaha's XG specification:
//    CCont #98 = LSB of NRPN parameter ID
//    CCont #99 = MSB of NRPN parameter ID
// 
// NRPN
// MSB LSB                           Data Range
// #99 #98    Parameter              (ccont#6=MSB, ccont#38=LSB)
// === ===== ====================== ======================
//   1   8  Vibrato Rate            -64.. 0..+63 logical range or (-50..+50)
//                                    0..64..127 MIDI data range  ( 14..114)
//   1   9  Vibrato Depth           same ranges as above      
//   1  10  Vibrato Delay           same ranges as above      
//   1  32  Filter Cutoff Freq.     same ranges as above      
//   1  33  Filter Resonance        same ranges as above      
//   1  99  EG Attack Time          same ranges as above      
//   1 100  EG Decay Time           same ranges as above      
//   1 102  EG Release Time         same ranges as above
//  20  xx  Drum Filter Cutoff Freq same ranges as above          
//          xx = drum MIDI key number
//  21  xx  Drum Filter Resonance   same ranges as above          
//          xx = drum MIDI key number
//  22  xx  Drum EG Attack Rage     same ranges as above          
//          xx = drum MIDI key number
//  23  xx  Drum EG Decay Rate      same ranges as above          
//          xx = drum MIDI key number
//  24  xx  Drum Pitch Coarse       same ranges as above          
//          xx = drum MIDI key number
//  25  xx  Drum Pitch Fine         same ranges as above          
//          xx = drum MIDI key number
//  26  xx  Drum Level                0..64..127 MIDI data range
//          xx = drum MIDI key number
//  28  xx  Drum Pan                  Random, Left..Center..Right
//                                    0.......1.....64......127 MIDI data range
//          xx = drum MIDI key number
//  29  xx  Drum Reverb Send Level    0..64..127 MIDI data range
//          xx = drum MIDI key number
//  30  xx  Drum Chorus Send Level    0..64..127 MIDI data range
//          xx = drum MIDI key number
//  31  xx  Drum Variation Send Level 0..64..127 MIDI data range
//          xx = drum MIDI key number
// 127 127  Null RPN (disables RPN/NRPN parameters from being altered).
//
//

int MidiOutput::NRPN(int channel, int nrpn_msb, int nrpn_lsb, 
      int data_msb, int data_lsb) {
   channel  = channel  & 0x0f;
   nrpn_msb = nrpn_msb & 0x7f;
   nrpn_lsb = nrpn_msb & 0x7f;
   data_msb = nrpn_msb & 0x7f;
   data_lsb = nrpn_msb & 0x7f;
 
   int status = 1;

   // check to see if the nrpn_msb and nrpn_lsb are the same
   // as the last call to this function, if not, then send
   // the appropriate MIDI controller values.
   if (rpn_msb_status[getPort()][channel] != nrpn_msb) {
      status &= cont(channel, 99, nrpn_msb);
      rpn_msb_status[getPort()][channel] = nrpn_msb;
   }
   if (rpn_lsb_status[getPort()][channel] != nrpn_lsb) {
      status &= cont(channel, 98, nrpn_lsb);
      rpn_lsb_status[getPort()][channel] = nrpn_lsb;
   }

   // now that the NRPN state is set, send the NRPN data values
   // but do not bother sending any data if the Null RPN is in effect.
   if (nrpn_msb != 127 && nrpn_lsb != 127) {
      status &= cont(channel, 6, data_msb);
      status &= cont(channel, 38, data_msb);
   }

   return status;
}


int MidiOutput::NRPN(int channel, int nrpn_msb, int nrpn_lsb, int data_msb) {
   channel  = channel  & 0x0f;
   nrpn_msb = nrpn_msb & 0x7f;
   nrpn_lsb = nrpn_msb & 0x7f;
   data_msb = nrpn_msb & 0x7f;
 
   int status = 1;

   // check to see if the nrpn_msb and nrpn_lsb are the same
   // as the last call to this function, if not, then send
   // the appropriate MIDI controller values.
   if (rpn_msb_status[getPort()][channel] != nrpn_msb) {
      status &= cont(channel, 99, nrpn_msb);
      rpn_msb_status[getPort()][channel] = nrpn_msb;
   }
   if (rpn_lsb_status[getPort()][channel] != nrpn_lsb) {
      status &= cont(channel, 98, nrpn_lsb);
      rpn_lsb_status[getPort()][channel] = nrpn_lsb;
   }

   // now that the NRPN state is set, send the NRPN data value,
   // but do not bother sending any data if the Null RPN is in effect.
   if (nrpn_msb != 127 && nrpn_lsb != 127) {
      status &= cont(channel, 6, data_msb);
   }

   return status;
}
 

int MidiOutput::NRPN(int channel, int nrpn_msb, int nrpn_lsb, double data) {
   channel  = channel  & 0x0f;
   nrpn_msb = nrpn_msb & 0x7f;
   nrpn_lsb = nrpn_msb & 0x7f;
   if (data < -1.0) {
      data = -1.0;
   } else if (data > 1.0) {
      data = 1.0;
   }

   int status = 1;

   // check to see if the nrpn_msb and nrpn_lsb are the same
   // as the last call to this function, if not, then send
   // the appropriate MIDI controller values.
   if (rpn_msb_status[getPort()][channel] != nrpn_msb) {
      status &= cont(channel, 99, nrpn_msb);
      rpn_msb_status[getPort()][channel] = nrpn_msb;
   }
   if (rpn_lsb_status[getPort()][channel] != nrpn_lsb) {
      status &= cont(channel, 98, nrpn_lsb);
      rpn_lsb_status[getPort()][channel] = nrpn_lsb;
   }

   // convert data into 14 bit number
   int data14 = (int)((data+1.0)/2.0*16383 + 0.5);

   // send the NRPN data values, two message of 7 bits each
   // but do not bother sending any data if the Null RPN is in effect.
   if (nrpn_msb != 127 && nrpn_lsb != 127) {
      status &= cont(channel,  6, data14 >> 7);
      status &= cont(channel, 38, data14 & 0x7f);
   }

   return status;
}


//////////
//
// Convenience functions for use of NRPN function.  Note that these
// are "Non-Registered" Parameter Numbers which means that each
// synthesizer manufacture can do whatever they want, so these
// functions might not behave the way you expect them to do so.
// Yamaha XG and Roland GS NRPN specifications are given below.
//

int MidiOutput::NRPN_null(int channel) {
   return NRPN(channel, 127, 127, 0);
}

int MidiOutput::NRPN_vibratoRate(int channel, int value) {
   // value in range -64..+63
   return NRPN(channel, 1, 8, value+64);
}

int MidiOutput::NRPN_vibratoRate(int channel, double value) {
   // value in range -1.0..+1.0
   return NRPN(channel, 1, 8, value);
}

int MidiOutput::NRPN_vibratoDepth(int channel, int value) {
   // value in range -64..+63
   return NRPN(channel, 1, 9, value+64);
}

int MidiOutput::NRPN_vibratoDepth(int channel, double value) {
   // value in range -1.0..+1.0
   return NRPN(channel, 1, 9, value);
}

int MidiOutput::NRPN_vibratoDelay(int channel, int value) {
   // value in range -64..+63
   return NRPN(channel, 1, 32, value+64);
}

int MidiOutput::NRPN_vibratoDelay(int channel, double value) {
   // value in range -1.0..+1.0
   return NRPN(channel, 1, 32, value);
}

int MidiOutput::NRPN_filterCutoff(int channel, int value) {
   // value in range -64..+63
   return NRPN(channel, 1, 33, value+64);
}

int MidiOutput::NRPN_filterCutoff(int channel, double value) {
   // value in range -1.0..+1.0
   return NRPN(channel, 1, 33, value);
}

int MidiOutput::NRPN_attack(int channel, int value) {
   // value in range -64..+63
   return NRPN(channel, 1, 99, value+64);
}

int MidiOutput::NRPN_attack(int channel, double value) {
   // value in range -1.0..+1.0
   return NRPN(channel, 1, 99, value);
}

int MidiOutput::NRPN_decay(int channel, int value) {
   // value in range -64..+63
   return NRPN(channel, 1, 100, value+64);
}

int MidiOutput::NRPN_decay(int channel, double value) {
   // value in range -1.0..+1.0
   return NRPN(channel, 1, 100, value);
}

int MidiOutput::NRPN_release(int channel, int value) {
   // value in range -64..+63
   return NRPN(channel, 1, 102, value+64);
}

int MidiOutput::NRPN_release(int channel, double value) {
   // value in range -1.0..+1.0
   return NRPN(channel, 1, 102, value);
}

int MidiOutput::NRPN_drumFilterCutoff(int drum, int value) {
   // value in range -64..+63
   return NRPN(9, 20, drum, value+64);
}

int MidiOutput::NRPN_drumFilterCutoff(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 20, drum, value);
}

int MidiOutput::NRPN_drumFilterResonance(int drum, int value) {
   // value in range -64..+63
   return NRPN(9, 21, drum, value+64);
}

int MidiOutput::NRPN_drumFilterResonance(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 21, drum, value);
}

int MidiOutput::NRPN_drumAttack(int drum, int value) {
   // value in range -64..+63
   return NRPN(9, 22, drum, value+64);
}

int MidiOutput::NRPN_drumAttack(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 22, drum, value);
}

int MidiOutput::NRPN_drumDecay(int drum, int value) {
   // value in range -64..+63
   return NRPN(9, 23, drum, value+64);
}

int MidiOutput::NRPN_drumDecay(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 23, drum, value);
}

int MidiOutput::NRPN_drumPitch(int drum, int value) {
   // value in range -64..+63
   return NRPN(9, 24, drum, value+64);
}

int MidiOutput::NRPN_drumPitch(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 24, drum, value);
}

int MidiOutput::NRPN_drumLevel(int drum, int value) {
   // value in range -64..+63
   return NRPN(9, 26, drum, value+64);
}

int MidiOutput::NRPN_drumLevel(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 26, drum, value);
}

int MidiOutput::NRPN_drumPan(int drum, int value) {
   return NRPN(9, 28, drum, value+64);
}

int MidiOutput::NRPN_drumPan(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 28, drum, value);
}

int MidiOutput::NRPN_drumReverb(int drum, int value) {
   // note offset from 0 not -64
   return NRPN(9, 29, drum, value);
}

int MidiOutput::NRPN_drumReverb(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 29, drum, value);
}

int MidiOutput::NRPN_drumChorus(int drum, int value) {
   // note offset from 0 not -64
   return NRPN(9, 30, drum, value);
}

int MidiOutput::NRPN_drumChorus(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 30, drum, value);
}

int MidiOutput::NRPN_drumVariation(int drum, int value) {
   // note offset from 0 not -64
   return NRPN(9, 31, drum, value);
}

int MidiOutput::NRPN_drumVariation(int drum, double value) {
   // value in range -1.0..+1.0
   return NRPN(9, 31, drum, value);
}

//
// Convenience functions for use of NRPN function.
//
//////////



//////////////////////////////
//
// RPN -- sends a registered parameter number where:
//   parameter #1: channel (0-15)
//   parameter #2: RPN MSB indicator (controller #101 data)
//   parameter #3: RPN LSB indicator (controller #100 data)
//   parameter #4: RPN MSB data      (controller #6  data)
//  [parameter #5: RPN LSB data      (controller #38 data)]
// or:
//   parameter #1: channel (0-15)
//   parameter #2: NRPN MSB indicator (controller #99 data)
//   parameter #3: NRPN LSB indicator (controller #98 data)
//   parameter #4: NRPN Floating point in range (-1..1) (ccont#6 and #38 data)
//
//
// RPN (registered parameter number) -- General MIDI and 
//    Extended MIDI mess.  It becomes the receiving synthesizer's
//    responsibility to determine the meaning of continuous
//    controller (ccont) #6 from data sent with cconts #100,101 (for
//    RPNS) and cconts #98,99 (for NRPNS).  
//
// The data for RPNs are transfered to synthesizer with 
//    data slider ccont #6(MSB) and ccont #38(LSB).  Also data increment
//    ccont#96 (data increment) and ccont#97 (data decrement) are in
//    relation to the RPN or NRPN in effect.  Increment and Decrement
//    are not recommend to use with RPN's because of confusion in the
//    MIDI industry over which 7-bit data (#6 or #38) to increment.
//  
// Once you have selected an RPN on a given channel, the
//    channel will apply subsequent Data Entry to the 
//    selected parameter.  After making the necessary settings
//    you should set RPN's to NULL to reduce the risk of 
//    operational errors.  a NULL RPN will disable the previous
//    values of either RPN or NRPN data.
// 
// The following RPN values are registered:
//    CCont #100 = LSB of RPN parameter ID
//    CCont #101 = MSB of RPN parameter ID
// 
// RPN                               Data Range
// MSB LSB     Parameter              (ccont#6=MSB, ccont#38=LSB)
// === ===== ====================== ======================

//   0   0   Pitchbend Sensitivity  0-127 (default 2) (LSB ignored)
//                                  (The number of +/- half steps in
//                                  pitch wheel range).
//   0   1   Fine Tune              -64.. 0..+63 logical range
//                                    0..64..127 MIDI data range
//   0   2   Coarse Tune            same range as above.
//   0   3   Change Tuning Program  0..127
//   0   4   Change Tuning Bank     0..127
//   

int MidiOutput::RPN(int channel, int rpn_msb, int rpn_lsb, 
      int data_msb, int data_lsb) {
   channel  = channel & 0x0f;
   rpn_msb  = rpn_msb & 0x7f;
   rpn_lsb  = rpn_msb & 0x7f;
   data_msb = rpn_msb & 0x7f;
   data_lsb = rpn_msb & 0x7f;
 
   int status = 1;

   // check to see if the rpn_msb and rpn_lsb are the same
   // as the last call to this function, if not, then send
   // the appropriate MIDI controller values.
   if (rpn_msb_status[getPort()][channel] != rpn_msb) {
      status &= cont(channel, 101, rpn_msb);
      rpn_msb_status[getPort()][channel] = rpn_msb;
   }
   if (rpn_lsb_status[getPort()][channel] != rpn_lsb) {
      status &= cont(channel, 100, rpn_lsb);
      rpn_lsb_status[getPort()][channel] = rpn_lsb;
   }

   // now that the RPN state is set, send the RPN data values
   // but do not bother sending any data if the Null RPN is in effect.
   if (rpn_msb != 127 && rpn_lsb != 127) {
      status &= cont(channel, 6, data_msb);
      status &= cont(channel, 38, data_msb);
   }

   return status;
}


int MidiOutput::RPN(int channel, int rpn_msb, int rpn_lsb, int data_msb) {
   channel  = channel & 0x0f;
   rpn_msb  = rpn_msb & 0x7f;
   rpn_lsb  = rpn_msb & 0x7f;
   data_msb = rpn_msb & 0x7f;
 
   int status = 1;

   // check to see if the rpn_msb and rpn_lsb are the same
   // as the last call to this function, if not, then send
   // the appropriate MIDI controller values.
   if (rpn_msb_status[getPort()][channel] != rpn_msb) {
      status &= cont(channel, 101, rpn_msb);
      rpn_msb_status[getPort()][channel] = rpn_msb;
   }
   if (rpn_lsb_status[getPort()][channel] != rpn_lsb) {
      status &= cont(channel, 100, rpn_lsb);
      rpn_lsb_status[getPort()][channel] = rpn_lsb;
   }

   // now that the RPN state is set, send the RPN data value,
   // but do not bother sending any data if the Null RPN is in effect.
   if (rpn_msb != 127 && rpn_lsb != 127) {
      status &= cont(channel, 6, data_msb);
   }

   return status;
}
 

int MidiOutput::RPN(int channel, int rpn_msb, int rpn_lsb, double data) {
   channel = channel & 0x0f;
   rpn_msb = rpn_msb & 0x7f;
   rpn_lsb = rpn_msb & 0x7f;
   if (data < -1.0) {
      data = -1.0;
   } else if (data > 1.0) {
      data = 1.0;
   }

   int status = 1;

   // check to see if the rpn_msb and rpn_lsb are the same
   // as the last call to this function, if not, then send
   // the appropriate MIDI controller values.
   if (rpn_msb_status[getPort()][channel] != rpn_msb) {
      status &= cont(channel, 101, rpn_msb);
      rpn_msb_status[getPort()][channel] = rpn_msb;
   }
   if (rpn_lsb_status[getPort()][channel] != rpn_lsb) {
      status &= cont(channel, 100, rpn_lsb);
      rpn_lsb_status[getPort()][channel] = rpn_lsb;
   }

   // convert data into 14 bit number
   int data14 = (int)((data+1.0)/2.0*16383 + 0.5);

   // send the RPN data values, two message of 7 bits each
   // but do not bother sending any data if the Null RPN is in effect.
   if (rpn_msb != 127 && rpn_lsb != 127) {
      status &= cont(channel,  6, data14 >> 7);
      status &= cont(channel, 38, data14 & 0x7f);
   }

   return status;
}


//////////
//
// Convenience functions for use of RPN function. 
//

int MidiOutput::RPN_null(void) {
   int status = 1;
   for (int i=0; i<16; i++) {
      status &= RPN_null(i);
   }
   return status;
}

int MidiOutput::RPN_null(int channel) {
   return RPN(channel, 127, 127, 0);
}

int MidiOutput::pbRange(int channel, int steps) {
   // default value for pitch bend sensitivity is 2 semitones.
   return RPN(channel, 0, 0, steps);
}

int MidiOutput::tuneFine(int channel, int cents) {
   // data from -64 to + 63
   return RPN(channel, 0, 1, cents+64);
}

int MidiOutput::fineTune(int channel, int cents) {
   return tuneFine(channel, cents);
}

int MidiOutput::tuneCoarse(int channel, int steps) {
   // data from -64 to + 63
   return RPN(channel, 0, 1, steps+64);
}

int MidiOutput::coarseTune(int channel, int steps) {
   return tuneCoarse(channel, steps);
}

int MidiOutput::tuningProgram(int channel, int program) {
   return RPN(channel, 0, 3, program);
}

int MidiOutput::tuningBank(int channel, int bank) {
   return RPN(channel, 0, 4, bank);
}


///////////////////////////////////////////////////////////////////////////
//
// private functions
//


//////////////////////////////
//
// MidiOutput::initializeRPN -- set up the RPN status arrays
//   ignores initiaization request if already initialized.
//

void MidiOutput::initializeRPN(void) {
   int i, channel;

   if (rpn_lsb_status == NULL) {
      rpn_lsb_status = new Array<int>[getNumPorts()];
      for (i=0; i<getNumPorts(); i++) {
         rpn_lsb_status[i].setSize(16);      
         rpn_lsb_status[i].allowGrowth(0);      
         for (channel=0; channel<16; channel++) {
            rpn_lsb_status[i][channel] = 127;
         }
      }
   }

   if (rpn_msb_status == NULL) {
      rpn_msb_status = new Array<int>[getNumPorts()];
      for (i=0; i<getNumPorts(); i++) {
         rpn_msb_status[i].setSize(16);      
         rpn_msb_status[i].allowGrowth(0);      
         for (channel=0; channel<16; channel++) {
            rpn_msb_status[i][channel] = 127;
         }
      }
   }
}



//////////////////////////////
//
// MidiOutput::deinitializeRPN -- destroy the RPN status arrays
//    do nothing if the arrays are not initialized
//

void MidiOutput::deinitializeRPN(void) {
   if (rpn_msb_status != NULL) {
      delete [] rpn_msb_status;
      rpn_msb_status = NULL;
   }

   if (rpn_msb_status != NULL) {
      delete [] rpn_msb_status;
      rpn_msb_status = NULL;
   }
}



//////////////////////////////
//
// MidiOutput::writeOutputAscii
//

void MidiOutput::writeOutputAscii(int command, int p1, int p2) {
   outputRecordFile << std::dec;
   outputRecordFile.width(6);
   outputRecordFile << (timer.getTime()-lastFlushTime) <<'\t';
   outputRecordFile << "0x" << std::hex;
   outputRecordFile.width(2);
   outputRecordFile << command << ' ';
   outputRecordFile << std::dec; 
   outputRecordFile.width(3);
   outputRecordFile << p1 << ' ';
   outputRecordFile << std::dec; 
   outputRecordFile.width(3);
   outputRecordFile<< p2;
   outputRecordFile << std::endl;
}



//////////////////////////////
//
// MidiOutput::writeOutputBinary
//

void MidiOutput::writeOutputBinary(int command, int p1, int p2) {
   // don't store 0xf8 command since it will be used to mark the end of the 
   if (command == 0xf8) return;

   // write the delta time (four bytes)
   outputRecordFile.writeBigEndian((ulong)(timer.getTime() - lastFlushTime));

   // write midi data 
   // don't store 0xf8 command since it will be used to mark the end of the 
   // delta time data.
   outputRecordFile << (uchar)p1;
   outputRecordFile << (uchar)p2;
   outputRecordFile << (uchar)0xf8;
}



//////////////////////////////
//
// MidiOutput::writeOutputMidifile
//

void MidiOutput::writeOutputMidifile(int command, int p1, int p2) {
   // not yet implemented
}



// md5sum:	1c518e5130ac9ba0d79c4e9ce7fa41cf  - MidiOutput.cpp =css= 20030102
