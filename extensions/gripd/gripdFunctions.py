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
from string import *
import os
import sys

VERSION           = "0.1.1"
MAXDEC            = 9
DECTOHEXOFFSET    = 87
EVT_RAISE_TAGS_ID = wxNewId()

debugFile = sys.stderr

def setDebugFile(filename):
    global debugFile
    if (filename != ""):
        debugFile = open(filename, "w", -1)

def debugLog(logString):
    debugFile.write(logString + "\n")
    debugFile.flush()

def getColorFromDialog(parent, initColor):
    colorData = wxColourData()
    colorData.SetColour(initColor)
    colorData.SetCustomColour(0, initColor)
    colorDialog = wxColourDialog(parent, colorData)
    colorDialog.ShowModal()
    colorData = colorDialog.GetColourData()
    return colorData.GetColour()

def getFontFromDialog(parent, initFont):
    fontData = wxFontData()
    fontData.SetInitialFont(initFont)
    fontDialog = wxFontDialog(parent, fontData)
    fontDialog.ShowModal()
    fontData = fontDialog.GetFontData()
    return fontData.GetChosenFont()
 
def getFileFromDialog(parent, initFile):
    if (initFile == ""):
        initFile = parent.parentApp.filepath
    fileDialog = wxFileDialog(parent, 
                              "Open file",
                              getDirectory(initFile),
                              initFile, 
                              "*.*",
                              wxOPEN)
    if (fileDialog.ShowModal() == wxID_OK):
        return fileDialog.GetPath()
    else:
        return ""

def colorTuple2HexString(tuple):
    def dec2HexString(value):
        if (value > MAXDEC):
            return chr(value + DECTOHEXOFFSET)
        else:
            return repr(value)
    hexString = ""
    for i in range(0, 3):
        hexString = hexString + dec2HexString(int(round(tuple[i] / 16)))
        hexString = hexString + dec2HexString(int(round(tuple[i] \
                                              - (int(tuple[i] / 16) * 16))))
    return hexString

def selectIntersect(rect1, rect2):
    left1 = rect1[0]
    top1 = rect1[1]
    right1 = rect1[0] + rect1[2]
    bottom1 = rect1[1] + rect1[3]
    left2 = rect2[0]
    top2 = rect2[1]
    right2 = rect2[0] + rect2[2]
    bottom2 = rect2[1] + rect2[3]
    if ((left1 < left2) and (right1 > right2)
    and (top1 < top2) and (bottom1 > bottom2)):
        return False
    if (((left2 > left1) and (left2 < right1))
    or ((right2 > left1) and (right2 < right1))
    or ((left2 < left1) and (right2 > right1))):
        if ((top2 > top1) and (top2 < bottom1)):
            return True
        elif ((bottom2 >top1) and (bottom2 < bottom1)):
            return True
        elif ((top2 < top1) and (bottom2 > bottom1)):
            return True
    return False

def getDirectory(path):
    if (os.name == "posix"):
        index = rfind(path, "/")
        if (index > -1):
            tempPath = path[0:index]
            while (tempPath[len(tempPath) - 1] == "/"):
                tempPath = tempPath[0:len(tempPath) - 1]
            tempPath = tempPath + "/"
        else:
            tempPath = ""
    else:
        index = max(rfind(path, "\\"), rfind(path, "/"))
        if (index > -1):
            tempPath = path[0:index]
            while (tempPath[len(tempPath) - 1] == "/" or
                   tempPath[len(tempPath) - 1] == "\\"):
                tempPath = tempPath[0:len(tempPath) - 1]
            tempPath = tempPath + "\\"
        else:
            tempPath = ""
    return tempPath

def replaceSlashes(path):
    path = replace(path, "/", "\\")
    return path

def scrubPath(path):
    if (os.name == "posix"):
        while (find(path, "/../") > 0):
            index = rfind(path[0:find(path, "/../")], "/")
            path = path[0:index] + path[find(path, "/../") + 3:len(path)]
        while (find(path, "/./") > 0):
            index = find(path, "/./")
            path = path[0:index] + path[index + 2:len(path)]
        path = replace(path, "//", "/")
    else:
        path = replaceSlashes(path)
        while (find(path, "\\..\\") > 0):
            index = rfind(path[0:find(path, "\\..\\")], "\\")
            path = path[0:index] + path[find(path, "\\..\\") + 3:len(path)]
        while (find(path, "\\.\\") > 0):
            index = find(path, "\\.\\")
            path = path[0:index] + path[index + 2:len(path)]
        path = replace(path, "\\\\", "\\")
    return path     

def makeAbsolutePath(superPath, subPath):
    if (os.name == "posix"):
        if (find(subPath, "./") == 0 or
            find(subPath, "../") == 0):
            tempPath = superPath + subPath
        elif (subPath[0] != "/"):
            tempPath = superPath + "/" + subPath
        else:
            tempPath = subPath
    else:
        if (find(subPath, "/") >= 0):
            subPath = replaceSlashes(subPath)
        if (find(subPath, ".\\") == 0 or
            find(subPath, "..\\") == 0):
            tempPath = superPath + subPath
        elif (subPath[1] != ":"):
            tempPath = superPath + "\\" + subPath
        else:
            tempPath = subPath
    return scrubPath(tempPath)

def makeRelativePath(superPath, subPath):
    if (os.name != "posix"):
        superPath = lower(superPath)
        subPath = lower(subPath)
    if (find(subPath, superPath) == 0 and superPath != ""):
        tempPath = "./" + subPath[len(superPath):len(subPath)]
    else:
        tempPath = subPath
    return scrubPath(tempPath)


class RaiseTagsEvent(wxPyEvent):
    def __init__(self):
        wxPyEvent.__init__(self)
        self.SetEventType(EVT_RAISE_TAGS_ID)

        
