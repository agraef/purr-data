fftw~ - FFT objects for pd

a set of alternative fft~-objects, using the fftw-library.
these objects should behave exactly like the original ones:

[fftw~]		<->	[fft~]
[ifftw~]	<->	[ifft~]
[rfftw~]	<->	[rfft~]
[rifftw~]	<->	[rifft~]


REASONING:
 the code in here has originally been written by tim blechmann and was
 meant to be part of pd itself.
 since miller puckette is somewhat reluctant to integrate these fftw-enabled
 objects into vanilla pd, this library provides another way to get
 high-performance FFTs.

TODO:
 search for a way to override the default FFT-objects in pd with these.
 (i think cyclone uses such a mechanism)


AUTHORS:
 tim blechmann
 IOhannes m zmölnig

LICENSING:
 since this code was originally meant to be part of pd, it is licensed under the
 same license as pd itself.
 see LICENSE.txt for details
