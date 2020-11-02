==============================================================================
the zexy external
==============================================================================

outline of this file::
==============================================================================
 +  general
 +  using
 +  bugs
 +  compilation/installation
 +  license


general::
==============================================================================
the zexy external is a collection of externals for miller.s.puckette's
realtime-computermusic-environment called "pure data" (or abbreviated "pd")
this zexy external will be of no use, if you don't have a running version of
pd on your system.
check out for http://pd.iem.at to learn more about pd and how to get it

note: the zexy external is published under the GNU General Public License
that is included (LICENSE.txt). some parts of the code are taken directly
from the pd source-code, they, of course, fall under the license pd is
published under.

making pd run with the zexy external::
==============================================================================
make sure, that pd will be looking at the location you installed zexy to.
ideally install zexy to some place, where pd already searches for libraries,
e.g. "~/.local/lib/pd/extra/" (linux), "~/Library/Pd/extra/" (macOS) or
"%AppData%\Pd\extra\" (Windows).
if this is not an option, either add the path to your "Path..." settings in pd,
or start pd with the cmdline option "-path /path/where/zexy/lives" (you can omit
the trailing "/zexy" component of the path)

make sure, that you somehow load the zexy external, either by adding "zexy" to
the "Startup..." libraries (or by starting pd with "-lib zexy"), or (and this is
the preferred method) by adding something like the following to your patches:

    [declare -path zexy -lib zexy]


bugs::
==============================================================================
if you happen to find any bugs, please report them at
   https://git.iem.at/pd/zexy


installation::
==============================================================================

linux, irix, osx, mingw,... :
------------------------------------------------------------------------------

you will need to have Pd installed, and a C-compiler (preferably gcc)
as well as a GNUmake compatible implementation of 'make'.

on windows, you also need MSYS2/MinGW installed.

#1> make
#2> make install


license::
==============================================================================
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program.  If not, see <http://www.gnu.org/licenses/>.
