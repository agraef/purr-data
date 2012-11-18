OVERVIEW
========

disis_munger~ (a.k.a. munger1~)
a realtime multichannel granulator
a.k.a. the swiss-army-knife of realtime granular synthesis

a flext (cross-platform PD & Max/MSP) port of
the munger~ object from the PeRColate library (0.9 beta6)
http://www.music.columbia.edu/PeRColate/

Original PeRColate library by:

Dan Trueman http://www.music.princeton.edu/~dan/
R. Luke DuBois's http://www.lukedubois.com/

Flext port and additions by:
Ivica Ico Bukvic http://ico.bukvic.net
Ji-Sun Kim hideaway@vt.edu
http://disis.music.vt.edu
http://www.cctad.vt.edu

Released under GPL license
(whichever is the latest version--as of this release, version 2)
For more info on the GPL license please visit:
http://www.gnu.org/copyleft/gpl.html

ACKNOWLEDGEMENTS
================

Many thanks to Dan Trueman for open-sourcing this great object!

SOURCE INSTALL
==============

If you simply intend to use prebuilt binaries, please skip to the INSTALL section. Otherwise take a big breath and read on...

1) You need stk library which can be downloaded from:

http://ccrma.stanford.edu/software/stk/

2) You need to also install latest flext library (this is a library that allows for creation of externals for both Max/MSP and PD using the same source). The best thing is to Google for the latest version.

3) To compile flext, read flext instructions (it boils down to running build.sh with appropriate parameters and then editing two simple config files, i.e. "build pd gcc build" or "build max gcc" or "build max msvc" etc.)

Your will need to edit buildsys/config-<platform-compiler-pdormax>.txt to adjust paths to various folders.

Then you will need to edit config.txt file. You do not need to include SndObj for this external but you do need stk option to be properly set. On Windows+MSVC, STK flag at the time of this release does not work, so you will have to use included testmunger1 MSVC project file and adjust path settings to compile disis_munger~.

NB: It is possible to build a universal binary but for this you will also need universal binary of STK. For more info on how to do this, please bug Thomas :-).

4) Once stk and flext are compiled, make sure that the libraries and their includes are installed in a place where compiler can find them (usually /usr/local/lib or /usr/local/include).

5) After you've made sure that the libs are properly installed, go into disis_munger~/source folder and type:

<path to flext folder>/build.sh <platform> <compiler> <build/clean/install>

NB: on Mac <build/clean/install> is not needed. On Windows, please use MSVC and open the testmunger1 project file in the root of the folder.

6) Once compiled, your binary will be created in a <maxorpd-platform> subfolder (i.e. pd-linux, or max-darwin), followed by another subfolder which reflects whether a threaded or singlethread flext was used. Inside you will find your external.

INSTALL
=======

You can either use the prebuilt externals (found in the bin/ folder) or ones built using the "SOURCE INSTALL" instructions above. Binaries are provided for Intel-based Macs, Win32, and Intel-based Linux OS. The included prebuilt binaries DO NOT REQUIRE you to install flext or stk as these are statically linked.

1) Copy the external in your externals folder (i.e. /usr/lib/pd/extra or C:\Program Files\Cycling '74\MaxMSP 4.6\Cycling '74\externals\, or "Applications/MaxMSP 4.6/Cycling '74/externals)

2) Copy appropriate help file (found in the help/ folder) into the help folder (i.e. /usr/lib/pd/doc/5.reference or C:\Program Files\Cycling '74\MaxMSP 4.6\max-help, or Applications/MaxMSP 4.6/max-help)

NB: Pd help file has a ".pd" extension, while Max/MSP help file has a ".help" extension.

3) Start your app (PD or Max) and create object called disis_munger~. Right-click (ctrl-click on Macs) and select "help" and this should open the help file with additional documentation.

Questions? See OVERVIEW for contact and Q&A info.

Enjoy!

FAQ
===

The following is Ico's FAQ, so it may or may not reflect other project participants' opinions, including original author(s) of munger~, flext, etc.

Q: Why porting to flext?
A: Flext library (by Thomas Grill) is a layer which allows creation of externals for both Max/MSP and PD without any alterations to the code (obviously once it is adapted to use flext). While there have been a number of Max/MSP <-> PD external ports in the past, many of them have become outdated because such attempts required either maintaining one code full of ugly #ifdefs, or worse--maintaining two sources. Either way, what usually turned out to be the case is that original authors did not have the time, interest, or simply the software/hardware to deal with the newly generated overhead and/or test the code, while volunteers who made the original porting efforts eventually moved on to other projects. The result was/is outdated and/or broken externals. Flext circumvents this problem by allowing one clean code to compile on both platforms while also supplying in many cases cleaner (more legible) API and (as a whipped cream on top) object-oriented environment (C++).

Q: Why bother with PD <-> Max/MSP cross-platform compatibility...
   ...when I use only <insert-your-favorite-application-here>?
   ...<insert-your-favorite-application-here> is better?
A: Choice is what makes us human (this is also what makes Arts so vibrant and exciting). And while everyone's welcome to express their own preferences, we also have to realize that in this case these same preferences are also the main cause of a virtual divide which manifests itself at everyone's detriment. Wouldn't it be nicer if we could share externals transparently, or even better, open PD patches in Max and vice-versa? This would help in both the cross-pollination of ideas as well as creative efforts. This project has also taught me that creating flext-ready externals is as easy if not easier (due to the aforesaid API's legibility) than native objects (whether that be PD or Max/MSP). Finally, if all else fails, such externals are bound to reach wider audience, and are much easier to maintain if cross-platform compatibility is to be pursued.

Q: If flext is so cool, why don't we see more porting efforts?
A: Good question. The fact is that flext is much more widely known among PD users than it is among the Max/MSP community, so this seemingly one-way road may have contributed to the current situation. One could only hope that projects like this may help reverse this unfortunate trend.

Q: So, is all really that peachy in the flext-land?
A: Well, our lives teach us that nothing is truly free in this world. Flext is no exception. Its "fees," however are not tied to our checkbooks. Rather, they manifest themselves in a slightly greater CPU overhead in signal flow due to message translation. Thus, one could consider flext a "middle-person" between the <app-of-your-choice> and the external. This, however, in today's world is so negligible that during the testing phase I was unable to measure any noticeable CPU-overhead difference.

Another consideration is that flext might not be complete (see KNOWN ISSUES for an example). That being said, in its current state it did the trick for a relatively complex external such as munger~ or even FFTEASE collection which had been ported several years ago. All this leads me to believe that it is more than ready for the day-to-day use.

Q: I already have Dan and Luke's awesome PeRColate lib. Why should I download this one?
A: This is a cross-platform port of the latest version with several new features. Thus, it allows for those platforms which have not had the beta6 available (Linux, Windows) to finally dig into all the goodies it brings. Plus you also get the cool stuff such as verbose modes, discrete panning, more documentation, virtually unlimited number of grains per sample (instead of 50), up to 24-channel output (instead of 2 or 16, depending which one you used), etc.

KNOWN ISSUES
============

disis_munger~ has been tested extensively on Linux+PD, OSX+Max/MSP and Win32+Max/MSP setups, suggesting that it should work on other setups as well. Your mileage may vary, though.

Currently there are no known issues.

FYI, even though disis_munger~ allows by default practically unlimited number of (as of 1.1.0 version this is adjustable via optional object parameter--see documentation for more info; by default it is 50) simultaneous grains per sample and has been compiled with all available optimizations (SSE, Altivec is supposedly available via flext but has not been tested), on MBP (Core Duo 1.83GHz) at 48KHz I was unable to get more than 150 simultaneous grains per sample with grain sizes of 100ms, grain variation of 50ms, and 8-channel output (or ~7,500 grains/second) without dropouts, even though according to the Activity Monitor the two CPU cores were not getting maxed out. Win32 machine (3-year old AMD64 3000+) could produce ~135 simultaneous grains per sample (or ~6,750 grains/second) using same settings before its CPU was maxed out. Linux on the same AMD64 3000+ hardware had a marginally better performance than that of Windows. Due to architectural design the sampling rate appears to be the upper limit (i.e. if you run PD or Max/MSP at lower sampling rates, your upper limit will be restricted to the sampling rate). This is inherited from the way original munger~ works. FWIW, using smallest grains, the aforesaid MBP even at 96KHz sampling rate was able to reach 96K grains/second without topping off the CPU (albeit the aural output was a boring almost white noise ;-).
