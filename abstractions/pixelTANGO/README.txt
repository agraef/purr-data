This is the readme for the pre-release of pixelTANGO.
pixelTANGO CVS Release $Revision: 1.1 $ $Date: 2005-11-26 22:21:06 $

PixelTANGO is a set of abstractions and patches that make use 
of pd/Gem for creating visuals in a live performance setting. 
Of course it can be used for many other things and provides 
an interface to use the power of pd/Gem with a less-steep 
learning curve.

PixelTANGO was written by Ben Bogart and Franz Hildgen @ 
The Société des arts technologiques (SAT) as part of the 
Territoires Ouverts / Open Territories (TOT) project funded 
by Heritage Canada.

For more information: http://www.tot.sat.qc.ca

pixelTANGO is Copyright Ben Bogart, Franz Hildgen and The 
Société des arts technologiques.

This is a pre-release, and not an official release supported 
by the SAT. There are many bugs and the software is incomplete.
Nevertheless it is quite usable and will be the basis of future
releases.

This program is distributed under the terms of the GNU General Public 
License 

PixelTANGO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or 
(at your option) any later version.

PixelTANGO is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details. 

You should have received a copy of the GNU General Public License
along with PixelTANGO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

USAGE:

Note this release is only supported by MacOSX (tested on 10.3) the 
required externals are available under linux, and the patches should 
be fully linux compatible. Feel free to take a crack at running it 
under linux. 

You must have PD/Gem installed to use pixelTANGO.
Choose a place for PixelTANGO /usr/local/lib/pd seems natural.
Add the "externals" folder to your PD path.
Add the "abstractions" folder to your PD path.
Add the "abstractions-memento" folder to your PD path. 
Add the "scripts" folder to your PD path.

Run PD will flags such as: 

pd -rt -helppath /usr/local/lib/pd/PixelTANGO-release-0.1/help -path /usr/local/lib/pd/PixelTANGO-release-0.1/abstractions/ -path /usr/local/lib/pd/PixelTANGO-release-0.1/abstractions-memento -path /usr/local/lib/pd/PixelTANGO-release-0.1/externals/ -path /usr/local/lib/pd/PixelTANGO-release-0.1/scripts -nosound -nomidi -lib Gem:zexy:OSC:py

Linux uses note that pixelTANGO has hard-coded directories for OSX in 
pt.entry and pt.layerfx. Open these files in an editor and change the
"/Applications/..." paths to reflect where your fonts (for pt.entry)
and fx/ abstractions (for pt.layerfx) are located. 

Try the included Example-Patches and give it a go. To get a run of
existing PixelTANGO modules open the PixelTANGO-help.pd file. Help
For each abstraction is available through the usual Right-Click or 
Bouble-Click menu.

Have fun!
