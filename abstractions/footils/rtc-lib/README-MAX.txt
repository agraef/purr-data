=======
RTC-lib
=======

Real Time Composition Library

Software library for algorithmic composition in Max/MSP/Jitter
copyleft 1992-2006 by Karlheinz Essl and others

http://www.essl.at/works/rtc.html


Abstract 
========

This software library - a collection of patches and externals for Max/MSP  -
offers the possibility to experiment with a number of compositional techniques,
such as serial procedures, permutations and controlled randomness. Most of
these objects are geared towards straightforward processing of data. By using
these specialized objects together in a patch, programming becomes much more
clear and easy. Many functions that are often useful in algorithmic composition
are provided with this library - therefore the composer could concentrate
rather on the composition than the programming aspects.

Originally, the Real Time Composition Library (RTC-lib) was developed during my
extensive work on Lexikon-Sonate (1992 ff.), an interactive realtime
composition for computer-controlled piano which I started at IRCAM, Paris in
1992.

Regardless the fact that it was conceived for a specific project it became more
and more obvious that its functionalities are open and generic enough to be
used by other composers in different compositional contexts. Based on paradigms
which have been extracted from serial thinking (cf. Gottfried Michael Koenig
and Karlheinz Stockhausen) and its further development until nowadays it does
not force towards a certain aesthetic, but provides a programming environment
for testing and developing compositional strategies.

The Real Time Composition Library comes with a Hypertext-like on-line help
which allows to have a perfect overview on the library objects and their
multiple relationships.  Thanks to Richard Dudas (Cycling74), Peter Elsea
(University of California, Santa Cruz), Serge Lemouton (IRCAM, Paris) and jasch
who have ported the RTC-lib's externals to OSX.


Content 
=======

The library consists of "abstractions" (Max-patches that appear as objects) and
external objects. Most of them were written by myself, but there are also
contributions from other authors like Gerhard Eckel (GMD, St. Augustin) and
Serge Lemouton (IRCAM, Paris), James McCartney (namely his generic list
objects), and selected list objects from Peter Elsea's (UCSC) Lobject library.
Other contributions came from Claus Philipp, Les Stuck, Timothy Place, Orm
Finnendahl and Trond Lossius. Because the library objects are highly dependent
on each other, it is recommended not to take them apart. 


History 
=======

The devlopment of the RTC-lib started in 1992 when I was working at IRCAM on a
commission - Entsagung for ensemble and live-electronics. Having worked with
computer-aided composition algorithms before on an Atari using an experimental
LOGO implementation I came across Max. I immediately felt in love with it for
it offered the possibility of realtime processing and interactivity. (In LOGO,
it took many hours to calculate a score list which I had to transcribe into of
musical notation in order to analyze it - a very time-consuming procedure).

At this time, Max appeared as programming environment mainly optimized for MIDI
processing. List operations (which are crucial for my own compositional
thinking which stems from serialism) have not been implemented yet.
Higher-level compositional tools were not available, only customized solutions
for specific problems which were not geneneric enough to be used in different
contexts.

In order to create an evironment which enables one to concentrate rather on
high-level compositional questions than on low-level technical problems,
Gerhard Eckel and I started to develop a set of tools which became the corner
stones of the RTC-lib. At the this time, Serge Lemouton (my musical assistant
at IRCAM) wrote "nth" according to my indications, James McCartney released his
"list objects", and a few years later Peter Elsea developed his "LObjects"
(inspired by RTC-lib, as he told me). Some externals from these packages were
included into the RTC-lib, and Peter Elsea was nice enough to port some of my
abstractions (like trans-log) into C-externals.

On this basis, I developed a large number of higher-level compositional
algorithms written as "abstractions" (objects, coded in Max): rhythm
generators, harmony generators, chance operations, ramp generators, MSP
functions etc. which are often based on those primordial externals that have
been implemented by other authors.

Those externals (the low-level side of the RTC-lib) need to be re-compiled for
each operation system. We had to make the shift from 68k processors to PPC, and
now to OSX. In the next turn a port to Windows XP is envisaged. This task would
no have been possible without the help of people from the Max community like
Richard Dudas and Peter Elsea - many thanks!


Literature 
==========

Karlheinz Essl: Lexikon-Sonate. An Interactive Realtime Composition for
Computer-Controlled Piano. Proceedings of the "Second Brazilian Symposium on
Computer Music" (Canela 1995)

http://www.essl.at/bibliogr/lexson-sbcm.html  

Karlheinz Essl: Strukturgeneratoren. Algorithmische Komposition in Echtzeit.
Beiträge zur Elektronischen Musik, ed. by R. Höldrich und A. Weixler, Vol. 5
(Graz 1996)

http://www.essl.at/bibliogr/struktgen.html  

Karlheinz Essl & Bernhard Günther: Realtime Composition. Musik diesseits der
Schrift. Positionen, ed. by G. Nauck, Vol. 36 (Berlin 1998)  

http://www.essl.at/bibliogr/realtime-comp.html

 
Feedback
========

Please report bugs and feature requests to:

Dr. Karlheinz Essl
http://www.essl.at
mailto:essl@eunet.at
