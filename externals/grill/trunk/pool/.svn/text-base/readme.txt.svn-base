pool - a hierarchical storage object for PD and Max/MSP

Copyright (c) 2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

Donations for further development of the package are highly appreciated.
Visit https://www.paypal.com/xclick/business=gr%40grrrr.org&item_name=pool&no_note=1&tax=0&currency_code=EUR

----------------------------------------------------------------------------

Goals/features of the package:

- pool can store and retrieve key/value pairs, where a key can be any atom and 
	the value can be any list of atoms
- pool can manage folders. A folder name can be any atom.
- pool objects can be named and then share their data space
- clipboard operations are possible in a pool or among several pools
- file operations can load/save data from disk

----------------------------------------------------------------------------

IMPORTANT INFORMATION for all PD users:

Put the pd-msvc/pool.dll, pd-linux/pool.pd_linux or pd-darwin/pool.pd_darwin file
into the extra folder of the PD installation, or use a -path or -lib option 
at PD startup to find the pool external.

Put the help-pool.pd file into the doc\5.reference subfolder of your PD installation.

----------------------------------------------------------------------------

IMPORTANT INFORMATION for all Max/MSP users:

For Mac OSX put the max-osx/pool.mxd file into the folder 
/Library/Application Support/Cycling '74/externals

For Mac OS9 put the max-os9/pool.mxe file into the externals subfolder of your Max/MSP installation

For Windows put the max-msvc\pool.mxe file into the folder
C:\program files\common files\Cycling '74\externals (english version)

Put the pool.help file into the max-help folder.

============================================================================


BUILDING from source
--------------------

You will need the flext C++ layer for PD and Max/MSP externals to compile this.
See http://grrrr.org/ext/flext
Download, install and compile the package.
Afterwards you can proceed with building this external.


POSIX configure style
---------------------
./bootstrap.sh
./configure --enable-system=pd_OR_max --with-sysdir=PATH_TO_PD_OR_MAX_HEADER_FILES --libdir=WHERE_TO_PUT_THE_EXTERNAL
make
sudo make install



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


============================================================================

Version history:

0.2.2:
- fixed serious bug with clearing values and dirs. e.g. "clrall" and "clrrec" messages.
- fixed double-free for clearing dirs and values
- re-introduced a help message
- fixed bug in nested-dir XML saving
- changed printrec/printroot to display empty folders
- new curdir attribute for getting/setting the current directory
- changed pool name searching with STL code (more efficient)
- added success/error reporting for file operations (through attribute outlet)
- fixed handling of non-ASCII-characters
- XML files are now encoded UTF-8
- implemented output sorting (ogetall, ogetrec, ogetsub)
- fixed some potential buffer overrun problems

0.2.1:
- fixed "cntsub"... directories in current directory have been forgotten
- store/create also empty dirs with file I/O
- more inlined functions and better symbol handling
- added "seti" message to set elements at index
- added "clri" message to erase elements at index
- fixed bad bug: pool::priv was not initialized
- enhanced and optimized atom parsing
- escaped symbols (with \) for whitespace support on store and load
- escape symbols also with "" to help the load routine
- improved reading of legacy data by Frank Barknecht
- use statically allocated lists where feasible
- bug fix: [reset( didn't reset the current dir
- file loading: fixed recognition of stringified directory names

0.2.0:
- attributes (pool,private,echodir,absdir)
- added "geti" message for retrieval of a value at an index
- fixed bug in "get" message if key not present
- adapted source to flext 0.4.1 - register methods at class creation
- extensive use of hashing for keys and directories
- database can be saved/loaded as XML data
- fixed bug with stored numbers starting with - or +
- relative file names will be based on the folder of the current patcher
- added printall, printrec, printroot messages for console printout
- added mkchdir, mkchsub to create and change to directories at once
- change storage object only when name has changed

0.1.0:
- first public release

---------------------------------------------------------------------------

general:
- what is output as value if it is key only? (Max->nothing!)
- XML format ok?
