## Purr-Data

**Note to Github users**: Please use our [**GitLab Repository**](https://git.purrdata.net/jwilkes/purr-data).


Maintainers:

* Ivica Ico Bukvic <ico@vt.edu>
* Albert Graef <aggraef@gmail.com>
* Jonathan Wilkes <jancsika@yahoo.com>

Contact: [DISIS mailing list](http://disis.music.vt.edu/cgi-bin/mailman/listinfo/l2ork-dev)

Contents:

* [Downloads](#downloads)
* [One Paragraph Overview](#one-paragraph-overview)
* [Three Paragraph Overview](#three-paragraph-overview)
* [Goals](#goals)
* [User Guide](#user-guide)
* [Relationship of Purr Data to Pure Data](#relationship-of-purr-data-to-pure-data)
* [Build Guide](#build-guide)
  * [Gnu/Linux](#linux)
  * [OSX](#osx-64-bit-using-homebrew)
  * [Windows](#windows-64-bit-using-msys2)
* [Code of Conduct](#code-of-conduct)
* [Project Governance](#project-governance)
* [Contributor Guide](#contributor-guide)
* [Human Interface Guidelines](#human-interface-guidelines)
* [Core Pd Notes](#core-pd-notes)
* [GUI Message Spec](#gui-messaging-specification)

### One Paragraph Overview

Pure Data (aka Pd) is a visual programming language.  That means you can use it
to create software graphically by drawing diagrams instead of writing lines of
code.  These diagrams show how data flows through the software, displaying on
the screen what text-based languages require you to piece together in your mind.

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

Purr-Data has the following goals:

1. Documentation.  We like documentation.  It's like code, except friendly.
2. Be reliable.  Binary releases must be usable for performances and
   installations.  The git repo must always be in a workable state that can be
   compiled.  Regressions must be fixed quickly.
3. Be discoverable.  Undocumented features are buggy.  Missing help files are
   bugs.  Patches for new functionality that lack documentation are spam.
4. Be consistent.  Consistent interfaces are themselves a kind of
   documentation.  We like documentation, so it follows that we like consistent
   interfaces.

### User Guide and Weblinks

For a more in-depth look at Purr Data for new users and developers, see:

<https://agraef.github.io/purr-data-intro/Purr-Data-Intro.html>

For more resources see:

<https://agraef.github.io/purr-data/>

For Ico Bukvic's original Pd-l2ork website see:

<http://l2ork.music.vt.edu/main/make-your-own-l2ork/software/>

### Relationship of Purr Data to Pure Data

There are three maintained distributions of Pure Data:

1. Purr Data. This started out as the 2.0 version of Pd-l2ork. It ships with
   lots of external libraries and uses a modern GUI written using HTML5.
2. Pd-l2ork is the version used by Ivica Bukvic for his laptop orchestra.
   Pd-l2ork 1.0 used tcl/tk (and tkpath) for the GUI. Pd-l2ork 2.x is a fork
   of an earlier Purr Data version which is developed separately. You can find
   these [here](http://l2ork.music.vt.edu/main/make-your-own-l2ork/software/).
3. Pure Data "Vanilla".  Miller Puckette's personal version which he hosts on
   his website and maintains.  It doesn't come with external libraries
   pre-installed, but it does include an interface you can use to search
   and install external libraries maintained and packaged by other developers.

### Downloads

**Windows, Ubuntu, and Mac OSX:**

Releases are done on Albert Gräf's GitHub mirror, which also provides a
website, wiki, additional documentation, and an up-to-date mirror of the
source code repository.

<https://github.com/agraef/purr-data/releases>

**More Linux packages:**

Packages for various Linux distributions (including Arch, Debian, Ubuntu, and
Fedora) are available through the JGU package repositories maintained by
Albert Gräf on the OBS (Open Build System). Detailed instructions can be found
[here](https://github.com/agraef/purr-data/wiki/Installation#linux).

You can also just go to the [OBS Download](https://software.opensuse.org/download/package?package=purr-data&project=home%3Aaggraef%3Apurr-data-jgu), pick your Linux system, and follow
the instructions.

### Build Guide

Purr Data is usually built by just running `make` in the toplevel source
directory after checking out the sources from its git repository. This works
across all supported platforms (Linux, Mac and Windows at this time).
The Makefile also offers the customary targets to clean (`make clean`, or
`make realclean` to put the sources in pristine state again) and to roll a
self-contained distribution tarball (`make dist`), as well as some other
convenience targets (please check the comments at the beginning of the Makefile
for more information).

However, to make this work, you will most likely have to install some
prerequisites first: *build tools* such as a C/C++ compiler and the make
program itself, as well as *dependencies*, the libraries that Purr Data needs.
Detailed instructions for each of the supported platforms are given below.

#### Linux

Time to build: *10 minutes light install, 45 minutes to 1.5 hours full install*
Hard drive space required: *roughly 2.5 GB*

0. Remember to update your packages:

        sudo apt-get update && sudo apt-get upgrade

1. Install the dependencies (please note that the packages may be named
   slightly differently for different Linux distributions; the given names are
   for Debian/Ubuntu)

        sudo apt-get install bison flex automake libasound2-dev \
             libjack-jackd2-dev libtool libbluetooth-dev libgl1-mesa-dev \
             libglu1-mesa-dev libglew-dev libmagick++-dev libftgl-dev \
             libgmerlin-dev libgmerlin-avdec-dev libavifile-0.7-dev \
             libmpeg3-dev libquicktime-dev libv4l-dev libraw1394-dev \
             libdc1394-22-dev libfftw3-dev libvorbis-dev ladspa-sdk \
             dssi-dev tap-plugins invada-studio-plugins-ladspa blepvco \
             swh-plugins mcp-plugins cmt blop slv2-jack omins rev-plugins \
             libslv2-dev dssi-utils vco-plugins wah-plugins fil-plugins \
             mda-lv2 libmp3lame-dev libspeex-dev libgsl0-dev \
             portaudio19-dev liblua5.3-dev python-dev libsmpeg0 libjpeg62-turbo \
             flite1-dev libgsm1-dev libgtk2.0-dev git libstk0-dev \
             libfluidsynth-dev fluid-soundfont-gm byacc \
             python3-markdown

2. The gui toolkit may require installing the following extra dependencies

        sudo apt-get install gconf2 libnss3

3. Clone the Purr-Data repository *(2 to 10 minutes)*

        git clone https://git.purrdata.net/jwilkes/purr-data.git

4. Compile the code *(5 minutes [light] to 1.5 hours [full])*

   * to build only the core: `make light` *(5 minutes)*
   * to build the core and all externals: `make all` *(20 minutes to 1.5 hours)*
   * to build everything *except* Gem: `make incremental` *(10 to 20 minutes)*

5. If you're using an apt-based Linux distribution and you have the necessary
   Debian packaging tools installed, there should now be an installer file in
   the main source directory, which can be installed as usual. Otherwise, run
   `make install` to install the software, and `make uninstall` to remove it
   again.

#### OSX 64-bit using Homebrew

Time to build: *50 minutes to 1.5 hours*  
Hard drive space required: *roughly 2 GB*

1. Install [Homebrew](https://brew.sh) *(15 minutes)*
   (asks for password twice-- once for command line tools, once for homebrew)

2. Install the dependencies *(10 minutes)*:

        brew install wget
        brew install autoconf
        brew install automake
        brew install libtool
        brew install fftw
        brew install python
        brew install lua
        brew install fluidsynth
        brew install faac
        brew install jpeg
        brew install lame
        brew install libvorbis
        brew install speex
        brew install gsl
        brew install libquicktime
        brew install sdl2
        brew install pkg-config

   You'll also need to install the python markdown module to generate the
   platform-specific release notes (ReadMe.html, Welcome.html):

        pip3 install markdown

   **Note:** Depending on your macOS and Xcode version, the 10 minutes
   estimate for this step may be a overly optimistic. Some build dependencies
   may require recompilation which can take a long time (up to several hours,
   if it includes a complete build of, e.g., gcc and cmake).

3. Clone the Purr-Data repository *(10 minutes)*

        git clone https://git.purrdata.net/jwilkes/purr-data.git

4. Change to the source directory

        cd purr-data

5. Build the OSX app and the installer disk image (.dmg file) *(15 minutes)*

        make

6. There should now be a .dmg file in your current directory, which lets you
install the app in the usual way

#### Windows 64-bit Using msys2

Time to build: *roughly 1.5 hours-- 30 minutes of this is for Gem alone*  
Hard drive space required to build: *rougly 2.5 GB*

**Important note:** We recommend doing the build under your msys2 home
directory (usually /home/username in the msys2 shell). This directory should
not have any spaces in it, which would otherwise cause trouble during the
build. Never try using your Windows home directory for this purpose instead,
since it will usually contain spaces, making the build fail.

1. In a browser, navigate to: `https://git.purrdata.net/jwilkes/ci-runner-setup/-/raw/master/win64_install_build_deps.ps1`
2. Select all with `<control-a>`
3. Right-click and choose "Copy"
4. In the Start menu type `PowerShell ISE` and click the "Windows Powershell ISE" app that pops up.
5. In the Powershell ISE window menu, choose File -> New
6. In the area with the white background, right-click and choose "Paste" 
7. Click the `Run Script` arrow in the toolbar *(20 minutes)*
8. If there were no errors in the script, msys2 and Inno Setup are now installed.
9. Open the directory "C:\msys64" and click `mingw64.exe`
10. Download the source code *(3-6 minutes)*  
In the msys terminal window, issue the following command to create a new directory "purr-data" and clone the repository to it:

        git clone https://git.purrdata.net/jwilkes/purr-data.git

6. Enter the source directory *(less than a minute)*

        cd purr-data

7. Finally, build Purr-Data *(45-80 minutes)*

        make

8. Look in the top level source directory and double-click the setup file to
   start installing Purr Data on your system or run `./"setup file name"` in MSYS2 shell.

### Code of Conduct

1. No sarcasm, please.
2. Don't appear to lack empathy.
3. You can't live here. If you're spending hours a day writing Purr Data
   code or-- worse-- spending hours a day *writing emails about* code that 
   has yet to be written, you're doing it wrong.
4. If working on something for the first time, ask to be mentored.
5. If no one asked you to mentor them, don't teach.
6. It is better to let small things go than to risk taking time away from
   solving bigger problems.

It is a bad idea to break this Code of Conduct *even if* no one complains
about your behaviour.

### Project Governance

* The three maintainers listed at the top of this document are the ones in
  charge of this project.
* Unanimous decisions are preferred.
* 2 out of 3 can break a disagreement.
* There will only ever be three maintainers of this project at any given time.
  If you'd like to temporarily step in as one of the three,
  send an inquiry to the list and we can discuss it.

### Contributor Guide

Contributing is easy:

1. Join the development list:
   http://disis.music.vt.edu/cgi-bin/mailman/listinfo/l2ork-dev
2. Fork Purr Data using the gitlab UI and then try to build it from source
   for your own platform using the [Build Guide](#build-guide) above. 
   If you run into problems ask on the development list for help.
3. Once you have successfully built Purr Data, install it and make sure it
   runs correctly.
4. Start making changes to the code with brief, clear commit messages. If you
   want some practice you can try fixing one of the bugs on the issue tracker
   labeled
   ["good-first-bug"](https://git.purrdata.net/jwilkes/purr-data/issues?label_name%5B%5D=good-first-bug)
5. One you are done fixing the bug or adding your feature, make a merge request
   in the Gitlab UI so we can merge the fix for the next release.

A few guidelines:
* There should be a short and clear commit message for each merge request.
* Short and clear title and description are required for each merge request.
* There should be a short branch name related to the issue, like "update-readme".
* _No prototypes, please_. Purr Data's biggest strength is that users can
  turn an idea into working code very quickly. But a prototyping language that 
  is itself a prototype isn't very useful. That means Purr Data's core code
  and libraries must be stable, consistent, well-documented, and easy to use.
* Develop incrementally. Small, solid improvements to the software are
  preferable to large, disruptive ones.
* Try not to duplicate existing functionality.
  For backwards compatibility Purr Data ships many legacy
  libraries which unfortunately duplicate the same functionality. This makes
  it harder to learn how to use Pd, and makes it burdensome to read patches
  and keep track of all the disparate implementations.
* Keep dependencies to a minimum. Cross-platform dependency handling is
  unfortunately still an open research problem. In the even that you need
  an external library dependency, please mirror it at git.purrdata.net
  so that the build system doesn't depend on the availability of external
  infrastructure.

Here are some of the current tasks:

* Writing small audio/visual Pd games or demos to include in the next release
  * Skills needed: Ability to write Pd programs
  * Status: I wrote a little sprite-based game that will ship with the next
    version of Pd-L2Ork.  In it, the character walks around in an actual
    Pd diagram shoots at the objects to progress, and to make realtime
    changes to the music.
    What I'd like is to include a new, smallish game with each release
    that has a link in the Pd console.  It can be a little demo or game,
    just something fun that shows off what can be done using Pure Data.
* Designing/Implementing regression test template
  * Skills needed: Knowledge about... regression tests. :)  But also some
    expertise in using Pd so that the tests themselves can
    be written in Pure Data.  At the same time, they should
    be able to be run as part of the automated packaging
    process (i.e., in -nogui mode).
  * Status: Some externals have their own testing environments, but they are
    limited as they require manual intervention to run and read the
    results inside a graphical window.
    We currently have a crude test system that at least ensures that each
    external library instantiates without crashing.
    Here's an email thread with Katja Vetter's design, which looks to
    be automatable:
    http://markmail.org/message/t7yitfc55anus76i#query:+page:1+mid:chb56ve7kea2qumn+state:results
    And Mathieu Bouchard's "pure unity" (not sure if this is the most
    recent link...):
    http://sourceforge.net/p/pure-data/svn/HEAD/tree/tags/externals/pureunity/pureunity-0.0/
* Adding support for double precision to the external libraries that ship with purr-data
  * Skills needed: Knowledge about data types in C language(specially float and double)
  * Status: The core classes of purr data and the freeverb~ external library
    have been changed to support both float and double but still the remaining
    external libraries only have support for single precision.
    The task ahead is to add double precision support to these external libraries.
    As per the current resources we have the merge requests that have been used to add double
    precision support to the core libraries:
    https://git.purrdata.net/jwilkes/purr-data/merge_requests?scope=all&utf8=%E2%9C%93&state=merged&author_username=pranay_36
    And Katja Vetter's double precision patches to the pd-double project which were
    actually used for adding double precision support to the core libraries of purr-data.
    https://github.com/pd-projects/pd-double/commit/982ad1aa1a82b9bcd29c5b6a6e6b597675d5f300

### Human Interface Guidelines

#### General Look and Feel

Pd is a multi-window application that consists of three parts:

1. A main window, called the "Pd Window" or "Console Window". This window
   displays informational and error messages for Pd programs.
2. One or more "canvas" windows-- aka "patch" windows, used to display the
   diagrams that make up a Pd program.
3. One or more dialog windows used to configure the various parts of Pd.

All should look simple and uncluttered. Although "canvas" windows cannot
(yet) be traversed and edited using only the keyboard, all three parts of Pd
should be designed so that they can be manipulated using only the keyboard.

### Hooks for new users
It should also be possible to produce sound and interact when a new user runs
program for the very first time. In every release, there should be a link at
the bottom of the Console Window to a short game written in Pd that demonstrates
one or more of the capabilities of the Pd environment. The game should be
designed to be fun outside of its efficacy as a demonstration of Pd.

#### Fonts
Pd ships with "DejaVu Sans Mono", which is used for the text in canvas windows.
Fonts are sized to fit the hard-coded constraints in Pd Vanilla. This way box
sizes will match as closely as possible across distributions and OSes.

These hard-coded sizes are maximum character widths and heights. No font
fits these maximums exactly, so it's currently impossible to tell when looking
at a Pd canvas whether the objects will collide on a system using a different
font (or even a different font-rendering engine).

Dialogs and console button labels may use variable-width fonts. There is not
yet a suggested default to use for these.

The console printout area currently uses "DejaVu Sans Mono". Errors are printed
as a link so that the user can click them to highlight the corresponded canvas
or object that triggered the error.

#### Colors

Nothing set in stone yet.

### Core Pd Notes

The following is adapted from Pd Vanilla's original source notes.  (Found
in pd/src/CHANGELOG.txt for some reason...)

Sections 2-3 below are quite old.  Someone needs to check whether they even
hold true for Pd Vanilla anymore.

#### Structure definition roadmap.

First, the containment tree of things
that can be sent messages ("pure data").  (note that t_object and t_text,
and t_graph and t_canvas, should be unified...)

BEFORE 0.35:

    m_pd.h      t_pd                        anything with a class
                    t_gobj                  "graphic object"
                        t_text              text object
    g_canvas.h
                        t_glist             list of graphic objects
    g_canvas.c              t_canvas        Pd "document"

AFTER 0.35:

    m_pd.h      t_pd                        anything with a class
                    t_gobj                  "graphic object"
                        t_text              patchable object, AKA t_object
    g_canvas.h              t_glist         list of graphic objects, AKA t_canvas

Other structures:

    g_canvas.h  t_selection -- linked list of gobjs
                t_editor -- editor state, allocated for visible glists
    m_imp.h     t_methodentry -- method handler
                t_widgetbehavior -- class-dependent editing behavior for gobjs
                t_parentwidgetbehavior -- objects' behavior on parent window
                t_class -- method definitions, instance size, flags, etc.

#### 1. Coding Style

1.0  C coding style.  The source should pass most "warnings" of C compilers
(-Wall on Linux, for instance-- see the makefile.)  Some informalities
are intentional, for instance the loose use of function prototypes (see
below) and uncast conversions from longer to shorter numerical formats.
The code doesn't respect "const" yet.

1.1.  Prefixes in structure elements.  The names of structure elements always
have a K&R-style prefix, as in `((t_atom)x)->a_type`, where the `a_` prefix
indicates "atom."  This is intended to enhance readability (although the
convention arose from a limitation of early C compilers.)  Common prefixes are:
  * `w_` (word)
  * `a_` (atom)
  * `s_` (symbol)
  * `ob_` (object)
  * `te_` (text object)
  * `g_` (graphical object)
  * `gl_` (glist, a list of graphical objects).

Also, global symbols sometimes get prefixes, as in `s_float` (the symbol whose
string is "float").  Typedefs are prefixed by `t_`.  Most _private_ structures,
i.e., structures whose definitions appear in a ".c" file, are prefixed by `x_`.

1.2.   Function arguments.  Many functions take as their first
argument a pointer named `x`, which is a pointer to a structure suggested
by the function prefix; e.g., `canvas_dirty(x, n)` where `x` points to a canvas
`(t_canvas *x)`.

1.3.  Function Prototypes.  Functions which are used in at least two different
files (besides where they originate) are prototyped in the appropriate include
file. Functions which are provided in one file and used in one other are
prototyped right where they are used.  This is just to keep the size of the
".h" files down for readability's sake.

1.4.  Whacko private terminology.  Some terms are lifted from other historically
relevant programs, notably "ugen" (which is just a tilde object; see d_ugen.c.)

1.5.  Spacing.  Tabs are 8 spaces; indentation is 4 spaces.  Indenting
curly brackets are by themselves on their own lines, as in:

```c
if (x)
{
    x = 0;
}
```

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
    (pd:)       pd -- functions common to all "pd" objects
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

    gui    GUI front end

#### 6. Javascript style
1. Brackets on the same line as declaration or expression: `if (a) {`.
2. Single line comments only: `//`.
3. Use double-quotes for strings.
4. Use underscores to separate words of function names and variables.

### GUI Messaging Specification
#### Public GUI interface

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

where `const char *format` consists of zero or more of the following:

* f - floating point value (`t_float`)
* i - integer (`int`)
* s - c string (`char*	)
* x - hexadecimal integer value, with a precision of at least six digits.
      (hex value is preceded by an 'x', like `x123456`)

For some of Pd's internals like array visualization, the message length may
vary. For these _special_ cases, the following functions allow the developer
to iteratively build up a message to send to the GUI.

```c
gui_start_vmess(const char *msg, const char *format, ...);
gui_start_array();      // start an array
gui_f(t_float float);   // floating point array element (t_float)
gui_i(int int);         // integer array element (int)
gui_s(const char *str); // c string array element
gui_end_array();        // end an array
gui_end_vmess();        // terminate the message
```

The above will send a well-formed message to the GUI, where the number of array
elements are limited by the amount of memory available to the GUI. Because of
the complexity of this approach, it may _only_ be used when it is necessary to
send a variable length message to the GUI. (Some of the current code may
violate this rule, but that can be viewed as a bug which needs to get fixed.)

The array element functions gui_f, gui_i, and gui_s may only be used inside an
array.  Arrays may be nested, but this adds complexity and should be avoided if
possible.

#### Private Wrapper for Nw.js Port

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
