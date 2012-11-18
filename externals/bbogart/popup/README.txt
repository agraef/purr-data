This is the readme for "popup" a popup menu for PD. 

popup is Copyright Ben Bogart 2003

If you have any questions/comments you can reach the author at ben@ekran.org.

This program is distributed under the terms of the GNU General Public 
License 

popup is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or 
(at your option) any later version.

popup is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details. 

You should have received a copy of the GNU General Public License
along with popup; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

USAGE:

Put the binary in your extra folder.
Put the helpfile in your 5.reference folder.

Arguments: [pixel width] [pixel height] [background colour] [name] [opt1] [opt2] [...]

Methods:

  float			Select index value
  name [name]		Popup's name
  bgcolour [colour]	Background Colour (white, green, #5500ff)
  options [opt1] [...]	List of the popup options
  append [opt1] [...]   Add the arg list to the bottom of the current options

BUGS:
- Do not use a loadbang to set a patch-default value. Will cause a segfault.
  The left outlet will send a "bang" message when we're ready to set values,
  see example patch on how to use this to set a default value. Unfortunatly 
  this bang message is actually set on un-minimization and un-shading as 
  well as on load.

Have Fun.
