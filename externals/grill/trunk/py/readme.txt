py/pyext - python script objects for PD and Max/MSP

Copyright ©2002-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

Donations for further development of the package are highly appreciated.
Visit https://www.paypal.com/xclick/business=gr%40grrrr.org&item_name=pyext&no_note=1&tax=0&currency_code=EUR

----------------------------------------------------------------------------

You need to have Python installed on your system for the py/pyext external to work.
For Windows pick an up-to-date package from http://www.python.org .
For linux use the package manager. 
For OS X keep things as the are - it has Python installed by default.


The py/pyext package should run with Python version >= 2.1.
It has been tested with versions 2.2 to 2.7

The default build setting using PY_USE_GIL requires Python version >= 2.3.

Check out the sample patches and scripts

----------------------------------------------------------------------------

INSTALLATION
============

PD version >= 0.38 - Add "py" to the Startup items ("binaries to load") and add the folder "scripts" to the pd search path.
PD version < 0.38 - Load it as a library with e.g. "pd -lib py -path scripts"

Max/MSP - Copy py-objectmappings.txt into the init folder and py.mxe (Windows) or py.mxo (OSX) into the externals folder.

----------------------------------------------------------------------------

DESCRIPTION
===========

With the py object you can load python modules and execute the functions therein.
With the pyext you can use python classes to represent full-featured pd/Max message objects.
Multithreading (detached methods) is supported for both objects.
You can send messages to named objects or receive (with pyext) with Python methods.


Known bugs:
- The TCL/TK help patch is not usable under OSX.
- With standard PD 0.37, threaded py scripts will cause "Stack overflows" under some circumstances
  -> use PD 0.38 or the devel_0_37 cvs branch instead
- It has been reported that pyext crashes on AMD64 with SSE enabled (for these CPUs, disable the respective compiler flags)

----------------------------------------------------------------------------

BUILDING from source
====================

You will need the flext C++ layer for PD and Max/MSP externals to compile this.
See http://grrrr.org/ext/flext
Download, install and compile the package.
Afterwards you can proceed with building this external.


pd/Max - Windows - Microsoft Visual C, Borland C++, MinGW:
----------------------------------------------------------
Start a command shell with your eventual build environment
(e.g. run vcvars32.bat for Microsoft Visual Studio)

then run
 ..\flext\build.bat
(you would have to substitute ..\flext with the respective path to the flext package)


pd/Max - OSX/Linux - GCC:
-------------------------
From a shell run
bash ../flext/build.sh
(you would have to substitute ../flext with the respective path to the flext package)


----------------------------------------------------------------------------

Python array support for py/pyext@Max/MSP:

In the Max/MSP SDK change the file 
4.5 headers\c74support\max-includes\ext_types.h, line 45
from 
    typedef unsigned long                    UInt32;
    typedef signed long                      SInt32;
to
    typedef unsigned int                    UInt32;
    typedef signed int                      SInt32;
to avoid a compile-time type definition clash.

----------------------------------------------------------------------------

Version history:

0.2.2:
- FIX: pyext._send(receiversym) sent an empty list to receiversym, now it sends a bang message (equivalent to pyext._send(receiversym,"bang",()) ). Thanks to Yvan Volochine for spotting that.

0.2.1:
- FIX: some simplifications in py and pyext
- ADD: Python objects can be sent/received through outlets/inlets
- ADD: py can have multiple inlets for multiple function arguments (right inlets are non-triggering)
- ADD: allow module.function syntax for py and pyext
- FIX: pyext: cleanup float vs. int ... first decision is made by tag, afterwards a conversion is tried
- ADD: pym: object-oriented object... Python methods for any object type
- ADD: py: allow all callables (also object constructors and builtins)
- ADD: py: enable Python built-in functions (like range, str etc.)
- ADD: sequence protocol for symbol type
- FIX: cleanup for outbound messages (e.g. symbol atoms instead of one-element general messages)
- FIX: better exception handling (no longer leaves reference to function object) and cleared misleading error message
- FIX: better definition of output values for atoms, lists and anythings
- FIX: much better detached method handling (one thread for all object instances!)
- ADD: open module file in editor on "edit" message (or shift-click (PD) or double click (Max))
- FIX: _inlets and _outlets default to 0 if not given
- ADD: enable optimization of Python code in reease build
- CHG: _isthreaded is now a data member instead of a method
- FIX: more safety for calls where association python-pd has already been removed
- ADD: __str__ method for pyext, to enable print self calls
- ADD: enable symbol binding for all callables (not only functions and methods)
- ADD: Buffer.resize(frames,keep=1,zero=1) method
- ADD: py.Bundle class to support flext message bundles
- ADD: enable usage of compiled-only modules (.py[co])
- ADD: enable usage of module packages (with module/__init__.py[co])
- ADD: make use of the PyGILState_*() functions
- ADD: always run the Python interpreter in the background (to keep alive Python threads)
- ADD: added PY_USE_INOFFICIAL to enable usage of s_stuff.h PD header, to have access to search and help paths
- ADD: pyext: _init method is now called after __init__ (after inlets/outlets have been created)
- FIX: buffer protocol adapted to Python 2.5
- FIX: subpath support for modules (tested for Pd)

0.2.0:
- ADD: handling of Python threads
- FIX: output of single atoms instead of 1-element lists
- ADD: new detach mechanism (call queue)
- ADD: support for Max/MSP @ OSX and Windows
- DEL: eliminated meaningless inchannels and outchannels methods
- ADD: enabled "int"-tags for pyext class functions
- ADD: py: when no function is given on the command line, let it be selected by message tag
- FIX: __init__ wasn't called on reload
- FIX: bound instance methods weren't correctly decref'd
- ADD: Python symbol type
- ADD: _del method in pyext-derived class can be used to clean up things on exit
- FIX: solved py->py messaging problem with lock count instead of message queuing
- ADD: buffer handling with optional numarray support (if present)
- ADD: new objects for dsp processing: pyext~,pyx~,pyext.~,pyx.~
- FIX: correctly report Python errors while contructing the object

0.1.4:
- ADD: better (and independent) handling of inlet and outlet count (as class variables or dynamically initialized in __init__)
- FIX: many memory leaks associated to ***GetItem stuff (big thanks to sven!)
- FIX: set "this" memory in object after reloading script
- ADD: _getvalue,_setvalue to access PD values
- FIX: don't shout when Python script returns PyNone
- ADD: alias creation names pyext. and pyx. take the script name also for the class name

0.1.3:
- FIX: class variables are now set atomic if parameter list has only 1 element
- ADD: introduced shortcut "pyx" for pyext.
- ADD: arguments to the pyext class are now exposed as attributes "args"
- FIX: parameters to Python functions are treated as integers when they can be.
- ADD: inlet and outlet count can be given for pyext, python _inlet and _outlet members are ignored then
- FIX: crash if script or class names are non-strings
- FIX: long multi-line doc strings are now printed correctly
- FIX: message "doc+" for class/instance __doc__ now working
- FIX: improved/debugged handling of reference counts
- FIX: _pyext._send will now send anythings if feasible
- CHANGE: no more finalization - it's really not necessary...
- FIX: calling from unregistered threads (like flext helper thread) now works

0.1.2:
- CHANGE: updates for flext 0.4.1 - method registering within class scope
- FIX: bugs in bound.cpp (object_free calls)
- FIX: bug with threaded methods along with flext bug fix.
- ADD: map Python threads to system threads
- ADD: shut down the Python interpreter appropriately
- CHANGE: use flext timer and bind functionality
- ADD: attribute functionality
- ADD: dir and dir+ methods for Python dictionary listing
- ADD: get and set methods for Python class attributes

0.1.1:
- CHANGE: updates for flext 0.4.0
- FIX: crash when module couldn't be loaded
- FIX: GetBound method (modmeth.cpp, line 138) doesn't exist in flext any more
- FIX: deadlock occured when connecting two py/pyext boxes in non-detached mode
- ADD: current path and path of the canvas is added to the python path
- FIX: path is not added to python path if already included

0.1.0:
- completely reworked all code
- added class functionality for full-featured objects and renamed the merge to pyext
- enabled threads and made everything thread-safe ... phew!
- using flext 0.3.2
- pyext now gets full python path
- python's argv[0] is now "py" or "pyext"
- etc.etc.

0.0.2:
- fixed bug when calling script with no function defined (thanks to Ben Saylor)
- cleaner gcc makefile

0.0.1:
- using flext 0.2.1

---------------------------------------------------------------------------

TODO list:

bugs:
- crashes with long Python printouts

general:
- Documentation and better example patches
- better error reporting for runtime errors
- we should pre-scan and cache class methods

features:
- enable multiple interpreters? ( -> not possible within one thread)
- options to fully detach a script (included initialization and finalization)
- stop individual threads
- support named (keyword) arguments (like attributes for messages)

tests:
- compile-time check for python threading support
