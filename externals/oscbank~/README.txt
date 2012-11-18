-----------------------------------------------------------------------------
oscbank~: for additive synthesis
-----------------------------------------------------------------------------

The oscbank external is an object for Miller Pucketter's PureData
("pd"). It was written in order to synthesize sinusoidal models that
consist of hundreds of partial sinewave components.  The external
takes three parameters for each partial: index(unique), frequency, and
amplitude. It then synthesizes the partial's information in a bank and
synthesizes it.  If the partial's frequency or amplitude are
modified (by providing another set with the same index), the
parameters are ramped to the new values.  

note: the oscbank~ external is published under the Gnu General Public License 
that is included (GnuGPL.txt). some parts of the code are taken directly 
from the pd source-code, they, of course, fall under the license pd is 
published under.

installation (from this directory):
-----------------------------------------------------------------------------
linux: make pd_linux
mac: make pd_darwin
windows: make pd_nt

author:
-----------------------------------------------------------------------------
this software is copyleft 2007-2015 by
Richie Eakin < reakinator [at] gmail [dot] com
with the kind help of Tom Erbe.

history:
-----------------------------------------------------------------------------
10-15-2007 - v0.1
- The code has been sitting around for a while with a bunch of bugs,
  finally got around to fixing them all.  So here is the first version.
