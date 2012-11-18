DMX512
======

controlling DMX from within Pd

this readme assumes that you have a running dmx4linux setup.
if not, read the README.dmx4linux.txt file for hints on how to
get it going...

A. compiling the Pd-objects
===========================
for this, change into the "./src" directory of the iem/dmx512/ folder
(this might well be the folder that holds this README.txt you are currently
reading)

if you have obtained the source-code via subversion, you will first have to run
% autoconf

(this should not be needed if you downloaded the sources as a release tarball;
that is: if the person who created the tarball has not forgotten to do it for you)

then run
% configure
% make

you should now have 2 binary files in the src/ folder called [dmxin] and [dmxout]


B. Installation
===============
you should install the binaries (+helpfiles) somewhere Pd can find them.
i would suggest to put them into
	</path/to/pd>/extra/dmx512/
and add this path to the startup-flags of Pd.


C. Usage
========
there should be help-files in the ./help directory
if not, the useage should be very similar to that of [ctlin] and [ctlout] 
(it's just using DMX512 instead of MIDI)


D. Help!
========
read the FAQ


