--------------------------
msd - Mass Spring Damper modeling for Pd - Max/MSP
--------------------------
--------------------------
07.06.2006

Copyright (C) 2006  Nicolas Montgermont
Written by Nicolas Montgermont for a Master's train in Acoustic,
Signal processing and Computing Applied to Music (ATIAM, Paris 6) 
at La Kitchen supervised by Cyrille Henry.
 
Optimized by Thomas Grill for Flext
Based on Pure Data by Miller Puckette and others
Based on pmpd by Cyrille Henry 

Contact : Nicolas Montgermont, nicolas_montgermont@yahoo.fr
	  Cyrille Henry, Cyrille.Henry@la-kitchen.fr

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

--------------------------
# Version 0.07 -- 17.05.2005
initial final vertion

Version 0.08 -- 20070517
add somes get messages
--------------------------
BUILD -- msd, msd2D, msd3D

To build any of the predefined msd objects you need flext from Thomas Grill. You must compile and install flext following the instructions, and then compile and install msd.

For example to build the msd2D object in pd with gcc on Linux, you must compile and install flext first, and then do

cd EXTERNALSDIR/nusmuk/msd2D
sudo bash ../../grill/flext/build.sh pd gcc 
sudo bash ../../grill/flext/build.sh pd gcc install

It will compile and install the msd external in the directory you previously defined in flext.

------------------
BUILD -- msdND

msd can virtually be build for any number of dimension. To do that you must edit the main.cpp and package.txt files in the msdND subfolder. Follow the instructions of the build.txt file also in the msdND folder.

--------------------------
USE -- msd, msd2D, msd3D

To start open the 01_msdtest.pd file under pd or look at the help-msd.pd patch.

