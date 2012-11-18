GrIPD: Graphical Interface for Pure Data 
---------------------------------------- 

v0.1.1

-What is GrIPD-
GrIPD is a cross-platform extension to Miller Puckette's Pure Data
software that allows the one to design custom graphical user
interfaces for Pd patches.  GrIPD is not a replacement for the Pd
Tcl/Tk GUI, but instead is intended to allow one to create a front end
to a Pd patch.  The concept is to create your PD patch normally and
then your GUI using GrIPD.  You can then lauch Pd using the -nogui
command line argument (although this is certainly not necessary) so
only your custom front end will be displayed.  GrIPD, itself, consists
of two parts: the "gripd" Pd object and an external GUI
window/editor. The Pd object was written in C and the external GUI was
written in Python using the wxWindows.  GrIPD is released under the
GNU General Public License.


-How GrIPD works-
The two parts of GrIPD communicate via TCP/IP sockets so they can run
on one machine or on separate machines over a network; so, for
example, the GUI could be on a laptop on stage controlling or
displaying info from the Pd audio engine in the house next to the
mixer. The communication works through Pd's implimentation of
"send" and "receive" objects. Basically, each GrIPD control object has
a send and receive symbol associated with it.


-Supported platforms-
GrIPD is currently available for Linux/GTK+ and MS Windows
platforms. It may work on other Unix's, but as of yet it has not been
tested. Since wxPython and C are both highly portable, most of GrIPD
is as well. The only issues should be the C socket and multi-process
code.


-Requirements-
    For Win32:
    * Pd
    For Linux:
    * Pd
    * GTK+

    To compile under Win32:
    * Python (v2.2 or later)
    * wxPython- wxWindows for Python (v2.4.0 or later)
    * a C/C++ compiler
    * to make a stand-alone .exe file you'll also need
      py2exe v0.3.1 (http://starship.python.net/crew/theller/py2exe/)

    To compile under Linux:
    * Python (v2.2 or later)
    * wxPython- wxWindows for Python (v2.4.0 or later)
    * GTK+ and wxGTK
    * C/C++ compiler
    * to make a stand-alone binary executable you'll also need
      Installer v5b4 (http://www.mcmillan-inc.com/install5_ann.html)

All of the above are, of course, free. 
For Windows: the package includes compiled binaries of the gripd.dll Pd object 
             and the gripd.exe GUI executeable. 
For Linux: stand-alone binary packages are available for x86 the architecture
           with either OSS or ALSA MIDI support.


-Installation-

Windows:
   1) Unzip contents of gripd-*.zip to .\pd\gripd\
   2) Put gripd.dll where Pd can find it
 To compile from source
   1) In .\pd\gripd\src:
      a) edit makefile
      b) run: nmake gripd_nt
   2) gripd.exe is included, but to recompile run:
      python gripdSetup.py py2exe -w -O2 --icon icon.pic

Linux:
   1) Ungzip/untar contents of gripd-*.tar.gz to ./pd/gripd/
   2) Put gripd.pd_linux where Pd can find it
 To compile from source:
   2) In ./pd/gripd/src
      a) edit makefile
      b) run: make gripd_linux
   3) to build a stand-alone binary executable of the GrIPD GUI run:
      python -OO Build.py gripd.spec (Build.py is part of Installer v5b4)

Be sure to have gripd.dll or gripd.pd_linux in your Pd path

-Using GrIPD- 
To use GrIPD, launch Pd with the -lib gripd command line
argument, and put the gripd Pd object on your patch; it's scope will be
global throughout all canvases.  Then send it an "open <optional
filename>" message to launch the GUI (gripd.exe or gripd.py).  
You can also send a "open_locked <filename>" message which will open the 
GUI in locked mode. This will prevent any changes from being made to
the GUI itself. 

You may need to set the path to gripd.py or gripd.exe by sending a 
"set_path <path>" message to the gripd Pd object.  For Windows users not using
gripd.exe, you may also have to set the path to python.exe by sending a
"set_python_path <path>" message. 

You may also send a "connect" message to set the gripd Pd object to wait for 
an incomming connection and launch gripd.exe or gripd.py separately.

If the path supplied to either an "open" message or a "set_path" message
is relative (i.e. begins with ./ or ../) they will be considered relative
to the directory containing the Pd executable file (pd.exe for Windows and 
pd for Linux). This keeps behavior consistent no matter where Pd is launched
from.

If the GUI is launched from PD, When the GUI window is closed you can re-open
it by sending the gripd Pd object a "show" message. You can also hide it by
sending the gripd Pd object a "hide" message.

The GrIPD GUI itself has two modes: "Performance Mode" and "Edit
Mode".  In "Edit Mode" you can add and remove controls using the
"Edit" menu, move them around by selecting them and dragging them by
their tag with the mouse or use the arrow keys (note: ctrl+<arrow key>
will move controls a greater distance).  You can edit a controls
properties by either selecting "Edit" from the "Edit" menu or
right-clicking the control's tab.  In "Performance Mode" the controls
properties are locked and when activated (e.g. clicked, slid, checked,
etc.) they will send a message via their send symbol.  Conversely,
they will receive messages sent within the Pd patch using their
receive symbol.  Look at gripd.pd and gripdExamples.pd.

GrIPD can forward MIDI input to Pd from up to two devices. To enable MIDI 
function, select "Enable MIDI" from the "Configure" menu. GrIPD will send 
note information via the "midi<n>note" symbol where <n> is either 0 or 1.
It will also send controller information via "midi<n>ctl" and program change
information via "midi<n>pgm".

GrIPD also allows for the use of up to two joysticks. To enable joystick 
function, select "Enable Joystick" from the "Configure" menu. Joystick 
axis and button information are sent to Pd with the send symbols
"joy<n>axis<m>" and "joy<n>button<m>" where <n> is 0 or 1 and ,<m> is 
0,1,... for the number of axes and buttons your joystick supports. For
example, to read from joystick 0 axis 0, put a "r joy0axis0" object in your
Pd patch. Axes will send integers in a range that will depend on your 
joystick, and buttons will send 1 when depressed and 0 when released.

GrIPD will also catch keystrokes and send the ASCII value to Pd while in 
performance mode via a "keystroke" send symbol. Simply put a "r keystroke" 
object in your Pd patch.

Note about duplicating radio buttons:
When creating radio buttons, the first button created in a group is the 
group marker.  Duplicating any of the buttons in a group other than the 
group marker button will add a button of the last group created. 
Duplicating the group marker button will start a new group.

Note about image paths:
When a path to an image is relative (i.e. begins with ./ or ../), it is 
considered relative to the .gpd file containing the image. If no file 
has been opened or saved, the path is considered relative to the directory
containing the gripd executable file (gripd.exe for Windows and gripd.py 
for Linux). It is therefore recommended that all images used in a GUI be
placed in a directory directly lower than the directory containing the .gpd
file. For example if your .gpd file is in c:\pd-guis put all images in 
c:\pd-guis\images. This will make distributing GUIs much simpler.

Note about MIDI and joystick input:
If problems occur due to MIDI or joystick input, you can disable them by
editing gripd.opt

-New in 0.1.1
Added graph control
Added openpanel and savepanel
added MIDI and joystick activity blinking
Fixed zombie bug
Fixed multiple opens bug
Fixed checkbox and radio buttons bug
Fixed rectangle redrawing problem
Fixed selecting inside rectangle problem

-Contact-
Drop me a line at jsarlo@ucsd.edu
