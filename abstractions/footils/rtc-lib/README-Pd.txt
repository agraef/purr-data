=======
RTC-lib
=======

Real Time Composition Library for Pure Data
===========================================

This file contains some notes specific to the Pd-version of the RTC-lib. For a
general overview of the scope of the RTC-library please refer to the file
README-MAX.txt. 

Contents
========

RTC-lib was ported to Pd mainly by Frank Barknecht in 2006. The port was made
without access to Max/MSP directly, but as RTC-lib contains excellent
documentation this wasn't really necessary most of the time. To import the
Max-patches, the Cyclone importer by Krzysztof Czaja was used, which turned out
to be an invaluable help.

RTC-lib for Pd contains almost all objects of the RTC-lib for Max/MSP. A
current list of what's still missing is contained in the file rtc-progress.csv.
Max-specific objects to control aspects of the Max-software deliberately have
been left out.

Where nameclashes with existing Pd objects and externals occured, the
respective RTC-objects also have been omitted. So far this only affected the
[minus] abstraction.

RTC-lib for Pd no longer mimicks the directory layout of the Max-version with
its seperate directories for each section like Harmony, Rhythm etc. Instead all
patches and their help files have been put into a single directory called
"rtc".  To install the patches, either add this directory to your path and use
the objects with their standard names, or copy the whole "rtc"-directory
somewhere into your path, like into /usr/lib/pd/extra/rtc and use them with a
"rtc"-namespace prefix: Then for example the [super-rhythm] object could be
called as [rtc/super-rhythm]. Note that then you may also need to add the
rtc-directory to your help path with "-helppath /usr/lib/pd/extra/rtc" or
similar, if you want to call the help-files.

Help-files have been placed next to their respective abstractions for
simplicity. They follow the naming convention "NAME-help.pd" to give help for
an abstraction "NAME.pd". Additionally you will find overview patches for each
topic section in the subdirectory "rtc/rtc-help". The main file here is
rtc/rtc-help/RTC-Overview.pd (Later this will be moved to "rtc/RTC-Overview.pd",
but a bug in the [declare] object currently makes this non-functional.

There is a special directory, "pdutils" which contains some small tools that
aided in the porting as well as some abstractions not included in the original
RTC-lib for Max, but that are used in the Pd-version.

Requirements
============

As a lot of the RTC-objects deal with list manipulations there is quite some
overlap with the abstraction collection "[list]-abs" by Frank Barknecht,
available in the Pd repository at http://pure-data.sf.net/. Having this
installed (without any kind of namespace-prefix) is necessary to be able to use
most RTC-lib abstractions.

Apart from that, also objects from the following external libraries have been
used:

* Cyclone (prob, zl, ...)
* zexy (sort,...)
* maxlib (???)

Generally if you install the pd-extended distribution you should be fine and
you will only need to load the respective libraries and adjust the search
paths.

The [play2] abstraction used in several help files optionally can play midi
notes with the [fluid~] soundfont synthesizer. If you don't have [fluid~], you
can also choose to send the midi output to an external player accessed through
[noteout]. See the [play2] help file for details. 

Feedback
========

Please report bugs and feature requests regarding the Pd version to:

Frank Barknecht
http://footils.org
fbar (a) footils.org 
