*********************************************************************** 
  File: pan_gogins~.c (a quick hack of pan~.c)
  Auth: 	Marco Scoffier [marco@metm.org] modified code by
  			Iain Mott [iain.mott@bigpond.com] 
  Maintainer (of pan~.c) : Iain Mott [iain.mott@bigpond.com] 
  Date: March 2003
  
  Description: Pd signal external. Stereo panning implementing an
  algorithm concieved by Michael Gogins and described at
  http://www.csounds.com/ezine/autumn1999/beginners/
  Angle input specified in degrees. -45 left, 0 centre, 45 right. 
  See supporting Pd patch: pan_gogins~.pd
  
  Copyright (C) 2001 by Iain Mott [iain.mott@bigpond.com] 
  
  This program is free software; you can redistribute it and/or modify 
  it under the terms of the GNU General Public License as published by 
  the Free Software Foundation; either version 2, or (at your option) 
  any later version. 
  
  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
  GNU General Public License, which should be included with this 
  program, for more details. 
  
*********************************************************************** 
I only tested this patch under linux.

INSTALLATION:
	
	type make pd_linux (or your platform's make target see Makefile)
	
	cp pan_gogins~.pd_linux to your pd/externs directory, 
	or somewhere in your pd -path
	
	cp pan_gogins~.pd into your pd/docs/5.reference directory 

Enjoy
