the MIDIvice library
====================

VERSION:
0.1

--------------------

GENERAL::
the MIDIvice library is a collection of externals for miller.s.puckette's realtime-computermusic-environment called "puredata" (or abbreviated "pd")
this MIDIvice-library will be of no use, if you don't have a running version of pd on your system.
check out for http://puredata.info to learn more about pd and how to get it 

LICENSE::
the MIDIvice library is published under the Gnu General Public License that is included (LICENSE.txt). 
some parts of the code are taken directly from the pd source-code, they, of course, fall under the license pd is published under.

AUTHORS::
this software is copyleft 2002-2008 by IOhannes m zmoelnig <zmoelnig@iem.at>, Institute of Electronic Music and Acoustics, University of Music and Dramatic Arts, Graz, Austria

--------------------

PURPOSE::
MIDIvice attempts to make handling of complex(!!!) MIDI-devices easier under Pd. It's like hiding all the SysEx-crap. 
Such devices include 
 - mixing-consoles
 - controllable patch-bays
 - Synthesizers (for doing SampleDumps etc.)

Such devices do NOT include: 
- Synthesizers (for playing purposes; there is enough support under pd, i think)
- hardware, that gives your PC the possibility of doing MIDI (so really, i am not going to write another device-driver for your USBMIDI thing)

SUPPORTED DEVICES::
- MotorMix(tm) by cm-labs(r) -  http://www.cmlabs.net
       8-channel motorfader-box with lots of buttons, LCDisplay and pan-pots.
       the MotorMix-specification were supplied by cm-labs (Many thanks !!).
       You can now download it from ftp://ftp.iem.at/pub/pd/Externals/MIDIvice/motormix.pdf

  objects:
  MotorMix - ping and reset the MotorMix
  motormix_rotary   - get movements of the rotaries
  motormix_encoder  - get movement and push-state of the special "encoder"-rotary
  motormix_faderIn  - get movements of the faders
  motormix_faderOut - move the motorfaders
  motormix_button   - get button press/releases
  motormix_LED      - switch on/off the button-LEDs (ot let them blink)
  motormix_LCDtext  - display some text on the MotorMix-LCDisplay
  motormix_LCDgraph - display some simple graphics on the MotorMix-LCDisplay
  motormix_7seg     - display something on MotorMix's 7segment dispay

--------------------

TODO::
support for FriendChip digital patchbays
support for TCelectronics M-5000
...
feel free to send me your wish-list (probably with MIDI-specifications)

BUGS:
none known (right now)
motormix_button/LED could be more intuitive...

--------------------

INSTALLING::
linux :
change to directory source
adapt the makefile to match your system (where is pd installed ?)
"make clean"
"make"
"make install"
this will install the MIDIvice library into <mypdpath>/pd/extra
documentation will be installed to <mypdpath>/pd/doc/5.reference/MIDIvice
alternatively you can try "make everything"

win32 :
i haven't had time to compile and test the MIDIvice-library under Windos yet.
Good luck !

darwin :
TODO

irix :
though i have physical access to both SGI's O2s and indys,  i haven't tried to compile the MIDIvice library there.
Good luck !

--------------------

RUNNING::
add the "MIDIvice" library to your startup-path
see doc/ for more information
