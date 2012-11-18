#!BPY

"""
Name: 'Raw Triangle Across The Net Via UDP...'
Blender: 232
Group: 'Export'
Tooltip: 'Export selected mesh via UDP packets in Raw Triangle Format'
"""

__author__ = "Chris McCormick"
__url__ = ("blender", "mccormick",
"Author's homepage, http://mccormick.cx/")
__version__ = "0.1"

__bpydoc__ = """\
This script exports meshes to Raw Triangle file format across the network via UDP.
It's used to send mesh data into puredata for GEM.

This is a modified version of the Raw Triangle exporter by Anthony D'Agostino and is mostly his work.
Any mistakes are my own.

The raw triangle format is very simple; it has no verts or faces lists.
It's just a simple ascii text file with the vertices of each triangle
listed on each line. There were some very old utilities (when the PovRay
forum was in existence on CompuServe) that preformed operations on these
files.

Usage:<br>
	Select meshes to be exported and run this script from "File->Export" menu.
"""

# $Id: net_triangle.py,v 1.1.1.1 2007-07-10 07:44:47 chr15m Exp $
#
# +--------------------------------------------------------------+
# | Copyright (c) 2002 Anthony D'Agostino, 2006 Chris McCormick  |
# | http://mccormick.cx/                                         |
# | chris@mccormick.cx                                           |
# | April 28, 2002                                               |
# | Send RAW Triangle File Format via UDP                        |
# +--------------------------------------------------------------+

# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# ***** END GPL LICENCE BLOCK *****

import Blender, meshtools
import sys, socket
#import time

# ==============================================================
# === Write RAW Triangle Format to a UDP socket at localhost ===
# ==============================================================
	
# open socket with the file descriptor "file"
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("localhost", 3142))

objects = Blender.Object.GetSelected()
objname = objects[0].name
meshname = objects[0].data.name
mesh = Blender.NMesh.GetRaw(meshname)
obj = Blender.Object.Get(objname)

# first send the clear command to get rid of existing mesh data for this object
s.send(objname + " clear;\n")

# this is our buffer for sending triangle data
data = ""

for face in mesh.faces:
	# send four faces at a time until we're done
	if not (mesh.faces.index(face) % 4):
		# finish off the last one, if this isn't the first one
		if mesh.faces.index(face):
			data += ";\n"
			s.send(data)
		# start a new buffer
		data = objname + " "
	
	if len(face.v) == 3:		# triangle
		v1, v2, v3 = face.v
		faceverts = tuple(v1.co) + tuple(v2.co) + tuple(v3.co)
		data += "%f %f %f %f %f %f %f %f %f " % faceverts
	else:						# quadrilateral
		v1, v2, v3, v4 = face.v
		faceverts1 = tuple(v1.co) + tuple(v2.co) + tuple(v3.co)
		faceverts2 = tuple(v3.co) + tuple(v4.co) + tuple(v1.co)
		data += "%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f " % faceverts1
		data += "%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f " % faceverts2

# finish off the very last one
data += ";\n"
s.send(data)

Blender.Window.DrawProgressBar(1.0, '')  # clear progressbar

s.close()

message = "Successfully sent " + objname
meshtools.print_boxed(message)

