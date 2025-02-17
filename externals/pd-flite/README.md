README for Pd external distribution 'pd-flite'

Last updated for version 0.3.2

# DESCRIPTION

The 'pd-flite' distribution contains a single Pd external ("flite"),
which provides a high-level text-to-speech interface for English based on
the 'libflite' library by Alan W Black and Kevin A. Lenzo.

'libflite' lives at https://github.com/festvox/flite

# WINDOWS BUILD

With MSYS2 install the ntldd package:

	pacman -S mingw32/mingw-w64-i686-ntldd-git

	pacman -S mingw64/mingw-w64-x86_64-ntldd-git

Then cd MinGW to this repo and do:

	make

or you can also specify more options with:

	make PDDIR=<path/to/pd-directory>

then do this command that installs and fills the `pthread` dependencies on the output dir:

	make localdep_windows

or with more options:

	make PDLIBDIR=<path/you/want-the/output> extension=<m_i386 or m_amd64> localdep_windows

# ACKNOWLEDGEMENTS

Pd by Miller Puckette and others.

Flite run-time speech synthesis library by Alan W Black
and Kevin A. Lenzo.

Ideas, black magic, and other nuggets of information drawn
from code by Guenter Geiger, Larry Troxler, and iohannes m zmoelnig.

# KNOWN BUGS

It gobbles memory, and also processor time on synthesis operations.


# AUTHOR

Bryan Jurish <moocow@ling.uni-potsdam.de>

# MAINTENANCE 

Since v0.3.0 Lucas Cordiviola https://github.com/Lucarda/pd-flite

- Thanks to Christof Ressi for code reviews.
