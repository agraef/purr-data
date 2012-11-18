Version 0.01 
copyright (c) 2001 by Yves Degoyon

mp3write~.dll is a MPEG I Layer III (mp3) file writer.

To install mp3write~, follow the steps from INSTALL

This software is published under GPL terms.

This is software with ABSOLUTELY NO WARRANTY.
Use it at your OWN RISK. It's possible to damage e.g. hardware or your hearing
due to a bug or for other reasons. 
We do not warrant that the program is free of infringement of any third-party
patents.

*****************************************************************************

mp3write~ has been compiled for Linux using LAME 3.92.
The newest version of LAME can be found via freshmeat.net

COPYING: you may use this source under GPL terms!

PLEASE NOTE: This software may contain patented alogrithm (at least
  patented in some countries). It may be not allowed to sell/use products
  based on this source code in these countries. Check this out first!

COPYRIGHT of MP3 music:
  Please note, that the duplicating of copyrighted music without explicit
  permission violates the rights of the owner.

*****************************************************************************

	using mp3write~ external for Pure Data

Open the help-mp3write~.pd to understand how it works.
In this patch, you must send the messages to mp3write~ 
in the following order :

1/ append|truncate if you wish to change file creation options ( default is append )
2/ open /my/file 
3/ start
5/ pd dsp 1
4/ stop : the tag is written at this stage

Parameters sent to mp3write~ object :

  Sampling Rate (Hz): 
Possible values are 48000, 44100 and 32000. If Pd runs at a different sampling 
rate, LAME will resample the signal. Default value for mp3 sampling rate is Pd's 
sampling rate.

  Bitrate (kbit/s): 
Possible values are 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256 
and 320. Default is 224.

  Mode: 
Possible values are 0 (stereo), 1 (joint stereo, the default), 2 (dual channel) 
and 3 (mono).

  Outlet:
The outlet outputs an int, which the number of bytes written in this session.
this might be different from file size if you're using append mode.


  Known Problems :

All combinations of samplerate, bitrate, quality factor will not be accepted.

ALLOWED QUALITY FACTOR :

    -q <arg>        <arg> = 0...9.  Default  -q 5 
                    -q 0:  Highest quality, very slow 
                    -q 9:  Poor quality, but fast 
    -h              Same as -q 2.   Recommended.
    -f              Same as -q 7.   Fast, ok quality

ALLOWED SAMPLERATE/BITRATES

MPEG-1   layer III sample frequencies (kHz):  32  48  44.1
bitrates (kbps): 32 40 48 56 64 80 96 112 128 160 192 224 256 320

MPEG-2   layer III sample frequencies (kHz):  16  24  22.05
bitrates (kbps):  8 16 24 32 40 48 56 64 80 96 112 128 144 160

MPEG-2.5 layer III sample frequencies (kHz):   8  12  11.025
bitrates (kbps):  8 16 24 32 40 48 56 64 80 96 112 128 144 160

Furthermore, it seems that high quality factors will not work
with this release of lame ( 3.92 ).
The same errors can be obtained with the command line :
lame -q 1 file.wav
outputs errors and mp3write can't do better.


