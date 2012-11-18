
 Tcl for Pd
 ==========

This library allows to to write externals for Pd using the Tcl language.

It is based on the standard API of PD (defined in m_pd.h, plus some other
private header files, like g_canvas.h, s_stuff.h, ...).

Also a library of Tcl helper functions is provided. It is not mandatory to use
it (moreover: it requires Tcl 8.5, while the tclpd external alone requires only
Tcl 8.4), but it is a syntactic sugar and can simplify a lot the code.
Using it is as simple as sourcing pdlib.tcl in your Tcl external.

Anyway, disregarding any approach chosen to develop Tcl externals, a general
knowledge of Pd internals (atoms, symbols, symbol table, inlets, objects) is
strongly required. (Pd-External-HOWTO is always a good reading)


 Compiling and installing
 ========================

To compile tclpd, simply type:

  make clean all

To compile it with debug enabled:

  make DEBUG=1 clean all

Requirements are pd >= 0.39, swig, c++ compiler.
To install tclpd, simply copy it to /usr/lib/pd/extra (or where you installed
pure-data).


 Writing GUI externals
 =====================

Pd is split into two processes: pd (the core) and pd-gui.
A simple pd external just runs in the core. A simple Tcl externals still runs
in the core, because tclpd creates a Tcl interpreter for that.

Instead, pd-gui has its own Tcl interpreter. In order to to GUI things (i.e.
draw on the canvas, or react to mouse events), the core process needs to
communicate with the pd-gui process (generally sending Tk commands, or calling
procedures defined in the pd-gui interp.
This is done with the sys_gui() function, if using the plain API.

Also pdlib.tcl provide means to simplify this task, with the guiproc function,
which defines procedures directly into the pd-gui interpreter.

As a counterexample, I'd like to cite tot/toxy/widget externals, which you may
be familiar with.
Such externals run in the pd-gui process. That was fine for writing simple gui
externals, that don't need to react to any message.
But, for instance, you cannot do a metronome or anything which is timing
accurate, or heavy IO, as that is not the purpose of the gui process.
Tclpd instead, by running in the core process, allows that.


 Data conversion between Tcl <=> Pd
 ==================================

In pd exists 'atoms'. An atom is a float, a symbol, a list item, and such.
Tcl does not have data types. In Tcl everything is a string, also numbers and
lists. Just when something needs to be read as number, then evaluation comes
in.
This leads to loss of information about atom types. Imagine a
symbol '456' comes into tclpd, you won't know anymore if "456"
is a symbol or a float.

Here a little convention comes in: in tclpd an atom gets converted into a
two-item list, where first item is atom type, and second item is its value.

Some examples of this conversion:

 Pd:  456
 Tcl: {float 456}

 Pd:  symbol foo
 Tcl: {symbol foo}

 Pd:  list cat dog 123 456 weee
 Tcl: {{symbol cat} {symbol dog} {float 123} {float 456} {symbol wee}}


 Examples
 ========

I provided small examples.
after loading pd with option '-lib tcl', just type the filename
(minus the .tcl extension) to load the Tcl externals examples.

actually there is one simple example: list_change (behaves like
[change] object, but work with lists only)

examples make use of pdlib.tcl. It's still possible to port the example to use
only the plain Pd api. Contributions are welcome.


 Authors
 =======

Please refer to AUTHORS file found in tclpd package.


 License
 =======

Please refer to COPYING file found in tclpd package.


