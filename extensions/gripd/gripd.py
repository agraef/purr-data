#!/usr/bin/python -OO

## GrIPD v0.1.1 - Graphical Interface for Pure Data
## Copyright (C) 2003 Joseph A. Sarlo
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
##
## jsarlo@ucsd.edu

from wxPython.wx import *
from gripdMain import *
import sys
import signal
import os

def signalHandler(sigNum, frame):
    print 'Caught signal', sigNum
    try:
        app.frame.eClose(wxEvent())
    except:
        app.ExitMainLoop()
if (os.name == "posix"):
    signal.signal(signal.SIGQUIT, signalHandler)
    signal.signal(signal.SIGINT, signalHandler)
    signal.signal(signal.SIGTERM, signalHandler)
app = mainApp(sys.argv)
app.MainLoop()



    
