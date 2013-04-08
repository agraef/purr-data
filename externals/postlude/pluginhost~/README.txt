pluginhost~ - a plugin host for Pure Data
==========================================

This directory (pluginhost) contains sourcecode and possibly binaries for a plugin host for Pure Data. It currently supports the following plugin types:

    LADSPA:     http://www.ladspa.org/
    DSSI:       http://dssi.sourceforge.net/

The functionality of the external is demonstrated in in the help patch (pluginhost/pluginhost~-help.pd).

Dependencies
------------

To compile pluginhost~ the following are required;

pd header   (m_pd.h)    >= 0.37
LADSPA SDK  (ladspa.h)  >= 1.1
DSSI SDK    (dssi.h)    >= 0.9
ALSA header (seq_event.h)

The help patch requires:

PD >= 0.39

Installing Dependencies


Installating Dependencies
-------------------------

On Debian-based Linux:

    $ sudo apt-get install dssi-dev libasound2-dev ladspa-sdk puredata-dev

On Fedora-based Linux (e.g. Planet CCRMA):

    $ sudo yum install dssi-devel alsa-lib-devel ladspa-devel

On Mac OS X (using fink):

    $ fink install dssi-dev libdssialsacompat ladspa-dev

On Mac OS X (from source):

    Download and install:

        LADSPA SDK: http://www.ladspa.org
        libdssialsacompat: http://smbolton.com/linux/
        DSSI SDK: https://sourceforge.net/projects/dssi/

If needed, the Pd header (m_pd.h) can be obtained for all platforms using the official sources:

    http://puredata.info/downloads/pure-data

Compiling
---------

From the same directory as the makefile type:

make
make install (as root)

If compiling on recent versions of OS X, but you are using a 32-bit Pd, you may need to type:

make FAT_FLAGS="-m32"

Once compiled the binary file and help file should be placed in directories that are included in Pure Data's search path.

License
-------

All files included in the pluginhost~ directory, and all binary files (if included) are licensed under the GNU GPL Version 2 (see LICENSE.txt for details).


