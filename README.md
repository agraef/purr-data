## Pure Data L2ork

maintainer: Ivica Bukvic <ico@vt.edu>

maintainer: Jonathan Wilkes <jancsika@yahoo.com>

*[One Paragraph Overview](#one-paragraph-overview)
*[Flavors of Pure Data](#flavors-of-pure-data)
*[Three Paragraph Overview](#three-paragraph-overview)
*[Goals](#goals)
*[Installation Guide](#installation-guide)
*[Contributor Guide](#contributor-guide)
*[Core Pd Notes](#core-pd-notes)

### One Paragraph Overview

Pure Data (aka Pd) is a visual programming.  That means you can use it to
create software graphically by drawing diagrams instead of writing lines of
code.  These diagram shows how data flows through the software, displaying on
the screen what text-based languages require you to piece together in your mind.

### Flavors of Pure Data

There are currently three main distributions of Pure Data:

1. Pd-l2ork.  Version used by Ivica Bukvic for his laptop orchestra.  This
   guide is for Pd-l2ork.
2. Pure Data "Vanilla".  Miller Puckette's personal version which he hosts on
   his website and maintains.  It doesn't include external libraries like
   objects for doing graphics, video, etc.
2. Pure Data Extended.  A monolithic distribution which ships with lots of
   external libraries.  At the moment it doesn't look to be maintained.

### Three Paragraph Overview

Pd has been designed with an emphasis on generating sound, video,
2D/3D graphics, and connecting through sensors, input devices, and MIDI as well
as OSC devices.

Pd has a special emphasis on generating audio and/or video in real time, with
low latency.  Much of its design focuses on receiving, manipulating, and
delivering high-quality audio signals.  Specifically, the software addresses
the problem of how to do this efficiently and reliably on general purpose
operating systems like OSX, Windows, Debian, etc.-- i.e., systems designed
mainly for multi-tasking.

Pd can easily work over local and remote networks.  It can be used to integrate
wearable technology, motor systems, lighting rigs, and other equipment. Pd is
also suitable for learning basic multimedia processing and visual programming
methods, as well as for realizing complex systems for large-scale projects.

### Goals

Pd-l2ork has the following goals:

1. Documentation.  We like documentation.  It's like code, except friendly.
2. Be reliable.  Binary releases must be usable for performances and
   installations.  The git repo must always be in a workable state that can be
   compiled.  Regressions must be fixed quickly.
3. Be discoverable.  Undocumented features are buggy.  Missing help files are
   bugs.  Patches for new functionality that lack documentation are spam.
4. Be consistent.  Consistent interfaces are themselves a kind of
   documentation.  We like documentation, so it follows that we like consistent
   interviews.

### Installation Guide
To install using a pre-compiled binary, follow these instructions:
http://l2ork.music.vt.edu/main/?page_id=56

To set up a development environment, first make sure you have the following
package dependencies listed here:
http://l2ork.music.vt.edu/main/?page_id=56

Then follow the steps outlined here:
http://l2ork.music.vt.edu/main/?page_id=56#install-dev

### Contributor Guide

Contributing is easy:

1. Join the development list:
   http://disis.music.vt.edu/cgi-bin/mailman/listinfo/l2ork-dev
2. Tell us what you'd like to work on.  Unfortunately there are _lots_ of
   externals and even core features that are poorly documented.  We can help 
   make sure you aren't duplicating functionality (or that you at least know
   what's already been implemented).
3. Send us your patch and we'll try it out.  If it's well-documented and
   there aren't any bugs we'll add it to the software.
4. If you want to do regular development and have commit access, just request
   it, then follow the Pd-l2ork goals above.

Here are some of the current tasks:

* coming up with a better name than Pd-l2ork. :)
  * skills needed: creativity, basic knowledge about programming in Pd
  * status: no work done on this yet
* writing small audio/visual Pd games or demos to include in the next release
  * skills needed: ability to write Pd programs
  * status: I wrote a little sprite-based game that will ship with the next
    version of Pd-l2ork.  In it, the character walks around in an actual
    Pd diagram shoots at the objects to progress, and to make realtime
    changes to the music.
    What I'd like is to include a new, smallish game with each release
    that has a link in the Pd console.  It can be a little demo or game,
    just something fun that shows off what can be done using Pure Data.
* porting Pd-l2ork's graphical user interface from Tcl/Tk to Qt.
  * skills needed: knowledge about Qt5/QML, threading, and Pd's core design
    and deterministic message-dispatching and scheduling
  * status: under active development
* designing/implementing regression test template
  * skills needed: knowledge about... regression tests. :)  But also some
    expertise in using Pd so that the tests themselves can
    be written in Pure Data.  At the same time, they should
    be able to be run as part of the automated packaging
    process (i.e., in -nogui mode).
  * status: some externals have their own testing environments, but they are
    limited as they require manual intervention to run and read the
    results inside a graphical window.
    Here's an email thread with Katja Vetter's design, which looks to
    be automatable:
    http://markmail.org/message/t7yitfc55anus76i#query:+page:1+mid:chb56ve7kea2qumn+state:results
    And Mathieu Bouchard's "pure unity" (not sure if this is the most
    recent link...):
    http://sourceforge.net/p/pure-data/svn/HEAD/tree/tags/externals/pureunity/pureunity-0.0/

### Core Pd Notes

The following is adapted from Pd Vanilla's original source notes.  (Found
in pd/src/CHANGELOG.txt for some reason...)

Sections 2-3 below are quite old.  Someone needs to check whether they even
hold true for Pd Vanilla any more.

#### Structure definition roadmap.

First, the containment tree of things
that can be sent messages ("pure data").  (note that t_object and t_text,
and t_graph and t_canvas, should be unified...)

BEFORE 0.35:

    m_pd.h	    t_pd    	    	    anything with a class
                    t_gobj	    	    "graphic object"
                        t_text  	    text object
    g_canvas.h  
                        t_glist 	    list of graphic objects
    g_canvas.c  	    	t_canvas    Pd "document"

AFTER 0.35:

    m_pd.h	    t_pd    	    	    anything with a class
                    t_gobj	    	    "graphic object"
                        t_text  	    patchable object, AKA t_object
    g_canvas.h     	    	t_glist     list of graphic objects, AKA t_canvas

Other structures:

    g_canvas.h  t_selection -- linked list of gobjs
                t_editor -- editor state, allocated for visible glists
    m_imp.h     t_methodentry -- method handler
                t_widgetbehavior -- class-dependent editing behavior for gobjs
                t_parentwidgetbehavior -- objects' behavior on parent window
                t_class -- method definitions, instance size, flags, etc.

#### 1. Coding Style

1.0  C coding style.  The source should pass most "warnings" of C compilers
(-Wall on linux, for instance; see the makefile.)  Some informalities
are intentional, for instance the loose use of function prototypes (see
below) and uncast conversions from longer to shorter numerical formats.
The code doesn't respect "const" yet.

1.1.  Prefixes in structure elements.  The names of structure elements always
have a K&R-style prefix, as in ((t_atom)x)->a_type, where the "a_" prefix
indicates "atom."  This is intended to enhance readability (although the
convention arose from a limitation of early C compilers.)  Common prefixes are
"w_" (word), "a_" (atom), "s_" (symbol), "ob_" (object), "te_" (text object),
"g_" (graphical object), and "gl_" (glist, a list of graphical objects).  Also,
global symbols sometimes get prefixes, as in "s_float" (the symbol whose string
is "float).  Typedefs are prefixed by "t_".  Most _private_ structures, i.e.,
structures whose definitions appear in a ".c" file, are prefixed by "x_".

1.2.   Function arguments.  Many functions take as their first
argument a pointer named "x", which is a pointer to a structure suggested
by the function prefix; e.g., canvas_dirty(x, n) where "x" points to a canvas
(t_canvas *x).

1.3.  Function Prototypes.  Functions which are used in at least two different
files (besides where they originate) are prototyped in the appropriate include
file. Functions which are provided in one file and used in one other are
prototyped right where they are used.  This is just to keep the size of the
".h" files down for readability's sake.

1.4.  Whacko private terminology.  Some terms are lifted from other historically
relevant programs, notably "ugen" (which is just a tilde object; see d_ugen.c.)

1.5.  Spacing.  Tabs are 8 spaces; indentation is 4 spaces.  Indenting
curly brackets are by themselves on their own lines, as in:

    if (x)
    {
	x = 0;
    }

Lines should fit within 80 spaces.

#### 2. Compatibility with Max

2.0.  Max patch-level compatibility.  "Import" and "Export" functions are
provided which aspire to strict compatibility with 0.26 patches (ISPW version),
but which don't get anywhere close to that yet.  Where possible, features
appearing on the Mac will someday also be provided; for instance, the connect
message on the Mac offers segmented patch cords; these will devolve into
straight lines in Pd.  Many, many UI objects in Opcode Max will not appear in
Pd, at least at first.

#### 3. Source-level Compatibility with Max

3.0.  Compatibility with Max 0.26 "externs"-- source-level compatibility. Pd
objects follow the style of 0.26 objects as closely as possible, making
exceptions in cases where the 0.26 model is clearly deficient.  These are:

3.1.  Anything involving the MacIntosh "Handle" data type is changed to use
char * or void * instead.

3.2.  Pd passes true single-precision floating-point arguments to methods;
Max uses double.
Typedefs are provided:
    t_floatarg, t_intarg for arguments passed by the message system
    t_float, t_int for the "word" union (in atoms, for example.)

3.3.  Badly-named entities got name changes:

    w_long --> w_int (in the "union word" structure)

3.4.  Many library functions are renamed and have different arguments;
I hope to provide an include file to alias them when compiling Max externs.

#### 4. Function name prefixes

4.0.  Function name prefixes.
Many function names have prefixes which indicate what "package" they belong
to.  The exceptions are:

    typedmess, vmess, getfn, gensym (m_class.c)
    getbytes, freebytes, resizebytes (m_memory.c)
    post, error, bug (s_print.c)
which are all frequently called and which don't fit into simple categories.
Important packages are:
    (pd-gui:)   pdgui -- everything
    (pd:)	    pd -- functions common to all "pd" objects
                obj -- fuctions common to all "patchable" objects ala Max
                sys -- "system" level functions
                binbuf -- functions manipulating binbufs
                class -- functions manipulating classes
                (other) -- functions common to the named Pd class

#### 5. Source file prefixes

5.0. Source file prefixes. 
PD:
s    system interface
m    message system
g    graphics stuff
d    DSP objects
x    control objects
z    other

PD-GUI:
t    TK front end

### GUI Messaging Specification

#### GUI interface

Purpose: a set of functions to communicate with the gui without putting
language-specific strings (like tcl) into the C code.  The new interface is a
step toward separating some (but not all) of the GUI logic out from the C code.
Of course the GUI can still be designed to parse and evaluate incoming messages
as commands.  But the idiosyncracies of the GUI toolkit can be limited to
either the GUI code itself or to a small set of modular wrappers around
sys_vgui.

The public interface consists of the following:

```c
gui_vmess(const char *msg, const char *format, ...);
```

where const char *format consists of zero or more of the following:

* f - floating point value (t_float)
* i - integer (int)
* s - c string (char*)
* x - hexadecimal integer value, with a precision of at least six digits.
      (hex value is preceded by an 'x', like "x123456"

For some of Pd's internals like array visualization, the message length may
vary. For these _special_ cases, the following functions allow the developer
to iteratively build up a message to send to the GUI.

```
gui_start_vmess(const char *msg, const char *format, ...); // send a message to the gui without terminating it
gui_start_array(); // start an array
gui_f(t_float float); // floating point array element (t_float)
gui_i(int int); // integer array element (int)
gui_s(const char *cstring); // c string array element
gui_end_array(); // end an array
gui_end_vmess(); // terminate the message
```

The above will send a well-formed message to the GUI, where the number of array
elements are limited by the amount of memory available to the GUI. Because of
the complexity of this approach, it may _only_ be used when it is necessary to
send a variable length message to the GUI. (Some of the current code may
violate this rule, but that can be viewed as a bug which needs to get fixed.)

The array element functions gui_f, gui_i, and gui_s may only be used inside an
array.  Arrays may be nested, but this adds complexity and should be avoided if
possible.

#### Current Wrappers (private)
--------------------------

The public functions above should fit any sensible message format.
Unfortunately, Pd's message format (FUDI) is too simplistic to handle arbitrary
c-strings and arrays, so it cannot be used here. (But if it happens to improve
in the future it should be trivial to make a wrapper for the public interface
above.)

The current wrapper was made with the assumption that there is a Javascript
Engine at the other end of the message. Messages consist of a selector,
followed by whitespace, followed by a comman-delimited list of valid Javascript
primitives (numbers, strings, and arrays). For the arrays, Javascript's array
notation is used. This is a highly idiosyncratic, quick-and-dirty approach.
But the point is that the idiosyncracy exists in a single file of the source
code, and can be easily made more modular (or replaced entirely by something
else) without affecting _any_ of the rest of the C code.
