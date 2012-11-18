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
from gripdFunctions import *
import os
import math

MBUTTONTYPE       = 0
MTOGGLETYPE       = 1
MVSLIDERTYPE      = 2
MHSLIDERTYPE      = 3
MRECTTYPE         = 4
MTEXTTYPE         = 5
MVGAUGETYPE       = 6
MHGAUGETYPE       = 7
MCHECKBOXTYPE     = 8
MTEXTBOXTYPE      = 9
MSPINBUTTONTYPE   = 10
MMOUSEAREATYPE    = 11
MRADIOBUTTONTYPE  = 12
MRADIONEWTYPE     = 13
MIMAGETYPE        = 14
MGRAPHTYPE        = 15
PAIRSEPCHAR       = chr(29)
SYMMSGSEP         = chr(31)
DEFCONTROLID      = -1
DEFEDITPOS        = (5,7)
DEFSLIDERVALUE    = 0
DEFSLIDERMIN      = 0
DEFSLIDERMAX      = 100
DEFVSLIDERSIZE    = (30, 80)
DEFHSLIDERSIZE    = (80, 30)
DEFRECTSIZE       = (200, 200)
DEFVGAUGESIZE     = (30, 80)
DEFHGAUGESIZE     = (80, 30)
DEFGAUGEMIN       = 0
DEFGAUGEMAX       = 100
DEFTAGSIZE        = 9
RESIZETAGCOLOR    = "#555555"
DEFMOUSEAREASIZE  = 100
SETRCVRCOMMAND    = "!setRcvr"
SETARRAYCOMMAND   = "!setArray"
TEXTEDITINPUT     = 0
FONTEDITINPUT     = 1
COLOREDITINPUT    = 2
FILEEDITINPUT     = 3
DROPDOWNEDITINPUT = 4
MAXARRAYSENDSIZE  = 2000

lastEditWindowPosition = (-1, -1)

# Abstract class for controls (buttons, sliders, etc)
class mControl(wxControl):
    def __init__(self, parentApp, type, sSym, rSym, cnxtn):
        self.editMode = False
        self.selected = False
        self.parentApp = parentApp
        self.type = type
        self.sendSymbol = sSym
        self.receiveSymbol = rSym
        self.connection = cnxtn
        self.resetBackground = False
        self.updateRSym()
        self.clickedOn = False
        self.grabbed = False
        self.setEditMode(True)
        EVT_CHAR(self, self.eChar)
        EVT_KEY_UP(self, self.parentApp.eKeyUp)
        EVT_KEY_DOWN(self, self.parentApp.eKeyDown)
    def mDestroy(self):
        if (self.editMode):
            self.moveTag.Destroy()
            self.BLResizeTag.Destroy()
            self.BRResizeTag.Destroy()
            self.TRResizeTag.Destroy()
        self.Destroy()
    def select(self):
        if (not self.selected):
            self.relocateTags()
            self.moveTag.Show(True)
            self.BLResizeTag.Show(True)
            self.BRResizeTag.Show(True)
            self.TRResizeTag.Show(True)
            self.parentApp.selectedControlList.append(self)
            # To keep tags above controls in Windows, repaints tags in idle time
            # (i.e. after everything else has bee repainted)
            self.selected = True
            self.Connect(-1, -1, EVT_RAISE_TAGS_ID, self.eRepaintControlTags)
        wxPostEvent(self, RaiseTagsEvent())
    def deselect(self):
        if (self.selected):
            self.moveTag.Show(False)
            self.BLResizeTag.Show(False)
            self.BRResizeTag.Show(False)
            self.TRResizeTag.Show(False)
            self.parentApp.selectedControlList.remove(self)
            self.Disconnect(-1, EVT_RAISE_TAGS_ID)
        self.selected = False
    def isSelected(self):
        return self.selected
    def grab(self):
        if (not self.grabbed):
            self.grabbed = True
            self.parentApp.dragging = True
            EVT_MOTION(self.moveTag, self.moveTag.eMove)
            self.moveTag.lastPos = wxGetMousePosition()
            self.moveTag.SetCursor(wxStockCursor(self.moveTag.pointerStyle))
            self.parentApp.startMoveControls()
            self.moveTag.CaptureMouse()
    def ungrab(self):
        if (self.grabbed):
            self.grabbed = False
            self.parentApp.dragging = False
            self.moveTag.ReleaseMouse()
            # this doesn't seem to actually disconnect
            self.moveTag.Disconnect(-1, wxEVT_MOTION)
            self.moveTag.SetCursor(wxSTANDARD_CURSOR)
            self.parentApp.endDragMoveControls()
            self.parentApp.mainFrame.mainPanel.Refresh()
            self.Refresh()
    def updateRSym(self):
        self.connection.send(SETRCVRCOMMAND + SYMMSGSEP + \
                             self.receiveSymbol + PAIRSEPCHAR)
    def sendMessage(self, message):
        try:
            self.connection.send(self.sendSymbol + SYMMSGSEP + \
                                 message + PAIRSEPCHAR)
        except:
            pass
    def PDAction(self, value):
        self.SetLabel(value)
    def relocateTags(self):
        self.moveTag.relocate()
        self.BLResizeTag.relocate()
        self.BRResizeTag.relocate()
        self.TRResizeTag.relocate()
    def refreshTags(self):
        self.moveTag.Show(True)
        self.BLResizeTag.Show(True)
        self.BRResizeTag.Show(True)
        self.TRResizeTag.Show(True)
        self.moveTag.Refresh()
        self.BLResizeTag.Refresh()
        self.BRResizeTag.Refresh()
        self.TRResizeTag.Refresh()
    def startMove(self):
        self.TRResizeTag.Show(False)
        self.BRResizeTag.Show(False)
        self.BLResizeTag.Show(False)
    def move(self, deltaPos):
        if (self.editMode and self.isSelected()):
            xPos = self.GetPosition()[0] + deltaPos[0]
            yPos = self.GetPosition()[1] + deltaPos[1]
            deltaPosMTX = xPos - self.GetPosition()[0]
            deltaPosMTY = yPos - self.GetPosition()[1]
            xMTPos = self.moveTag.GetPosition()[0] + deltaPosMTX
            yMTPos = self.moveTag.GetPosition()[1] + deltaPosMTY
            self.MoveXY(xPos, yPos)
            self.moveTag.MoveXY(xMTPos, yMTPos)
    def endMove(self):
        self.relocateTags()
        self.TRResizeTag.Show(True)
        self.BRResizeTag.Show(True)
        self.BLResizeTag.Show(True)
    def endDragMove(self):
        if (self.parentApp.snapToGrid):
            self.setPosition( \
                   self.parentApp.getNearestGridPoint(self.GetPosition()))
        self.endMove()
    def resize(self, deltaPos):
        if (self.editMode):
            xSize = self.GetSize()[0] + deltaPos[0]
            ySize = self.GetSize()[1] + deltaPos[1]
            if (xSize > 0 and ySize > 0):
                self.setSize((xSize, ySize))
    def setEditMode(self, mode):
        if (mode):
            if (not self.editMode):
                EVT_LEFT_DOWN(self, self.eLeftDown)
                EVT_RIGHT_DOWN(self, self.eRightDown)
                self.moveTag = controlEditTag(self,
                                              (0,0),
                                              wxBLACK,
                                              wxCURSOR_CROSS)
                self.BLResizeTag = controlResizeTag(self,
                                                    (1,0),
                                                    RESIZETAGCOLOR,
                                                    wxCURSOR_SIZEWE)
                self.TRResizeTag = controlResizeTag(self,
                                                    (0,1),
                                                    RESIZETAGCOLOR,
                                                    wxCURSOR_SIZENS)
                self.BRResizeTag = controlResizeTag(self,
                                                    (1,1),
                                                    RESIZETAGCOLOR,
                                                    wxCURSOR_SIZENWSE)
                self.editMode = True
        elif (self.editMode):
            self.deselect()            
            self.moveTag.Destroy()
            self.BLResizeTag.Destroy()
            self.TRResizeTag.Destroy()
            self.BRResizeTag.Destroy()
            self.Disconnect(-1, wxEVT_LEFT_DOWN)
            self.Disconnect(-1, wxEVT_RIGHT_DOWN)
            self.editMode = False
    def getEditMode(self):
        return self.editMode
    def setSendSymbol(self, sym):
        self.sendSymbol = sym
    def getSendSymbol(self):
        return self.sendSymbol
    def setReceiveSymbol(self, sym):
        self.receiveSymbol = sym
        self.updateRSym()
    def getReceiveSymbol(self):
        return self.receiveSymbol
    def setConnection(self, cnxtn):
        self.connection = cnxtn
    def getConnection(self):
        return self.connection
    def getType(self):
        return self.type
    def setPosition(self, pos):
        self.MoveXY(pos[0], pos[1])
        self.relocateTags()
    def setSize(self, size):
        self.SetSize(size)
        if (self.selected):
            self.relocateTags()
    # catch GTK bug
    def GetBackgroundColour(self):
        color = wxControl.GetBackgroundColour(self)
        if (not color.Ok()):
            color = wxBLACK
        return color
    def editCallback(self, editValues):
        try:
            self.setPosition((atoi(editValues[0]), atoi(editValues[1])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Position",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.setSize(wxSize(atoi(editValues[2]), atoi(editValues[3])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.setSendSymbol(editValues[4])
        self.setReceiveSymbol(editValues[5])
        self.SetLabel(editValues[6])
        self.SetForegroundColour(editValues[7])
        self.SetBackgroundColour(editValues[8])
        self.SetFont(editValues[9])
    def eEdit(self, event):
        editor = controlEditor(self)
        editor.addEditItem("X Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[0])
        editor.addEditItem("Y Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[1])
        editor.addEditItem("Width:",
                           TEXTEDITINPUT,
                           self.GetSize()[0])
        editor.addEditItem("Height:",
                           TEXTEDITINPUT,
                           self.GetSize()[1])
        editor.addEditItem("S Symbol:",
                           TEXTEDITINPUT,
                           self.getSendSymbol())
        editor.addEditItem("R Symbol:",
                           TEXTEDITINPUT,
                           self.getReceiveSymbol())
        editor.addEditItem("Label:",
                           TEXTEDITINPUT,
                           self.GetLabel())
        editor.addEditItem("F Color:",
                           COLOREDITINPUT,
                           self.GetForegroundColour())
        editor.addEditItem("B Color:",
                           COLOREDITINPUT,
                           self.GetBackgroundColour())
        editor.addEditItem("Font:",
                           FONTEDITINPUT,
                           self.GetFont())
        editor.edit()
    def eChar(self, event):
        self.parentApp.eChar(event)
    def eLeftDown(self, event):
        if (self.editMode):
            if (event.ControlDown()):
                if (self.isSelected()):
                    self.deselect()
                else:
                    self.select()
            else:
                self.parentApp.deselectOthers(self.GetId())
                self.select()
        else:
            event.Skip()
    def eRightDown(self, event):
        if (self.editMode):
            self.parentApp.deselectOthers(self.GetId())
            self.select()
            self.eEdit(event)
        else:
            event.Skip()
    def eRepaintControlTags(self, event):
        self.refreshTags()

# Class for little box user grabs to move/edit control
class controlEditTag(wxPanel):
    def __init__(self, parentControl, position, color, pointerStyle):
        wxPanel.__init__(self,
                         parentControl.parentApp.mainFrame.mainPanel,
                         -1, (0,0),
                         (DEFTAGSIZE, DEFTAGSIZE), 0)
        self.SetBackgroundColour(color)
        self.parentControl = parentControl
        self.pointerStyle = pointerStyle
        self.leftDownOnTag = False
        self.position = position
        EVT_CHAR(self, self.parentControl.parentApp.eChar)
        EVT_KEY_UP(self, self.parentControl.parentApp.eKeyUp)
        EVT_KEY_DOWN(self, self.parentControl.parentApp.eKeyDown)
        EVT_LEFT_DOWN(self, self.eLeftDown)
        EVT_LEFT_UP(self, self.eRelease)
        EVT_RIGHT_DOWN(self, self.eRightDown)
        EVT_ENTER_WINDOW(self, self.eEnter)
        EVT_LEAVE_WINDOW(self, self.eLeave)
        self.Show(False)
    def relocate(self):
        nuPos = [0,0]
        parentPos = self.parentControl.GetPosition()
        parentSize = self.parentControl.GetSize()
        if (self.position[0]):
            nuPos[0] = parentPos[0] + parentSize[0]
        else:
            nuPos[0] = parentPos[0] - DEFTAGSIZE
        if (self.position[1]):
            nuPos[1] = parentPos[1] + parentSize[1]
        else:
            nuPos[1] = parentPos[1] - DEFTAGSIZE
        self.MoveXY(nuPos[0], nuPos[1])
    def eLeftDown(self, event):
        self.leftDownOnTag = True
        if (self.parentControl.grabbed):
            self.parentControl.ungrab()
            self.parentControl.deselect()
            self.parentControl.parentApp.deselectOthers(-1)
        else:
            self.parentControl.grab()
    def eRightDown(self, event):
        if (not self.parentControl.grabbed):
            self.parentControl.eEdit(event)
    def eMove(self, event):
        if (self.parentControl.grabbed):
            self.newPos = wxGetMousePosition()
            deltaPos = (self.newPos[0] - self.lastPos[0],
                        self.newPos[1] - self.lastPos[1])
            self.lastPos = self.newPos
            self.parentControl.parentApp.moveSelectedControls(deltaPos)
    def eRelease(self, event):
        self.leftDownOnTag = False
        self.parentControl.ungrab()
    def eEnter(self, event):
        self.SetCursor(wxStockCursor(self.pointerStyle))
    def eLeave(self, event):
        self.SetCursor(wxSTANDARD_CURSOR)
# Class for little box user grabs to resize control
class controlResizeTag(controlEditTag):
    def eLeftDown(self, event):
        EVT_MOTION(self, self.eMove)
        self.leftDownOnTag = True
        self.lastPos = wxGetMousePosition()
        self.SetCursor(wxStockCursor(self.pointerStyle))
        self.CaptureMouse()
    def eMove(self, event):
        if (event.LeftIsDown() and self.leftDownOnTag):
            self.newPos = wxGetMousePosition()
            deltaPos = ((self.newPos[0] - self.lastPos[0]) * self.position[0],
                        (self.newPos[1] - self.lastPos[1]) * self.position[1])
            self.lastPos = self.newPos
            self.parentControl.parentApp.resizeSelectedControls(deltaPos)
    def eRelease(self, event):
        self.ReleaseMouse()
        self.Disconnect(-1, wxEVT_MOTION)
        self.SetCursor(wxSTANDARD_CURSOR)
        self.leftDownOnTag = False
        self.parentControl.parentApp.mainFrame.mainPanel.Refresh()

# Class for regular buttons
class mButton(mControl, wxButton):
    def __init__(self, parentApp, type, id, text, pos, sSym, rSym, conn):
        wxButton.__init__(self, parentApp.mainFrame.mainPanel,
                          id, text, pos)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        EVT_BUTTON(parentApp.mainFrame.mainPanel,
                   self.GetId(), self.eClicked)
    def eClicked(self, event):
        if (not self.editMode):
            self.sendMessage("bang")

# Class for toggle buttons
# Problem with grabbing clicks with EVT_*_DOWN mouse events
# in GTK on toggle button, seems to be a wxWindows thing
class mToggle(mButton, wxToggleButton, wxButton):
    def __init__(self, parentApp, type, id, text, pos, sSym, rSym, conn):
        wxToggleButton.__init__(self, parentApp.mainFrame.mainPanel,
                                id, text, pos)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        EVT_TOGGLEBUTTON(parentApp.mainFrame.mainPanel,
                         self.GetId(), self.eClicked)
    def toggle(self):
        self.SetValue(1 - self.GetValue())
    def PDAction(self, value):
        try:
            num = atof(value)
            if (num == 0):
                self.SetValue(0)
            else:
                self.SetValue(1)
        except:
            if (value == "bang"):
                self.toggle()
            else:
                self.SetLabel(value)
    def eClicked(self, event):
        # checking edit mode due to above-mentioned problem
        # checking for ctrl key (308)  
        if (self.editMode):
            try:
                self.parentApp.keysDown.index(308)
                if (self.selected):
                    self.deselect()
                else:
                    self.select()
            except:
                self.parentApp.deselectOthers(-1)
                self.select()
        else:
            self.sendMessage(repr(event.IsChecked()))

# Class for sliders
# Screwyness since wxSliders have a maximum at down position.
# No way to change as far as I can tell.
class mSlider(mControl, wxSlider):
    def __init__(self, parentApp, type, id, label, 
                 pos, style, sSym, rSym, conn):
        if (style & wxSL_HORIZONTAL):
            sSize = DEFHSLIDERSIZE
        if (style & wxSL_VERTICAL):
            sSize = DEFVSLIDERSIZE
        if (isinstance(label, list)):
            try:
                value = atoi(label[0])
            except:
                value = 0
            min = label[1]
            max = label[2]
            dir = label[3]
        else:
            try:
                value = atoi(label)
            except:
                value = 0
            min = DEFSLIDERMIN
            max = DEFSLIDERMAX
            dir = 1
        wxSlider.__init__(self, parentApp.mainFrame.mainPanel,
                          id, value, min, max, pos, sSize, style)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        EVT_COMMAND_SCROLL(parentApp, self.GetId(), self.eScrolled)
        self.lastValue = value
        self.setDirection(dir)
        self.resetBackground = True
        self.setSliderValue(value)
    def PDAction(self, value):
        try:
            num = atof(value)
            self.setSliderValue(num)
        except:
            pass
    def setDirection(self, val):
        if (val == 0):
            self.direction = 0
        else:
            self.direction = 1
    def getDirection(self):
        return self.direction
    def getSliderValue(self):
        direction = self.direction
        styleFlag = self.GetWindowStyleFlag()
        if (((direction) and (styleFlag & wxSL_VERTICAL)) \
        or ((not direction) and (styleFlag & wxSL_HORIZONTAL))):
            value = self.GetMax() - self.GetValue() + self.GetMin()
        else:
            value = self.GetValue()
        return value
    def setSliderValue(self, num):
        if (((self.direction == 1) and (self.GetWindowStyleFlag() \
                                        & wxSL_VERTICAL)) \
                                   or ((self.direction == 0) \
                                        and (self.GetWindowStyleFlag() \
                                             & wxSL_HORIZONTAL))):
            value = self.GetMax() - num + self.GetMin()
        else:
            value = num
        self.SetValue(value)
    def GetLabel(self):
        return repr(self.getSliderValue())
    def SetLabel(self, value):
        try:
            self.setSliderValue(atoi(value))
        except:
            pass
    def editCallback(self, editValues):
        try:
            self.setPosition((atoi(editValues[0]), atoi(editValues[1])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Position",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.setSize(wxSize(atoi(editValues[2]), atoi(editValues[3])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.setSendSymbol(editValues[4])
        self.setReceiveSymbol(editValues[5])
        self.SetForegroundColour(editValues[6])
        self.SetBackgroundColour(editValues[7])
        try:
            self.SetRange(atoi(editValues[8]),
                          atoi(editValues[9]))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Range",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        if (editValues[10] == "Up"):
            self.setDirection(1)
        elif (editValues[10] == "Down"):
            self.setDirection(0)
    def eEdit(self, event):
        editor = controlEditor(self)
        editor.addEditItem("X Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[0])
        editor.addEditItem("Y Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[1])
        editor.addEditItem("Width:",
                           TEXTEDITINPUT,
                           self.GetSize()[0])
        editor.addEditItem("Height:",
                           TEXTEDITINPUT,
                           self.GetSize()[1])
        editor.addEditItem("S Symbol:",
                           TEXTEDITINPUT,
                           self.getSendSymbol())
        editor.addEditItem("R Symbol:",
                           TEXTEDITINPUT,
                           self.getReceiveSymbol())
        editor.addEditItem("F Color:",
                           COLOREDITINPUT,
                           self.GetForegroundColour())
        editor.addEditItem("B Color:",
                           COLOREDITINPUT,
                           self.GetBackgroundColour())
        editor.addEditItem("Minimum:",
                           TEXTEDITINPUT,
                           self.GetMin())
        editor.addEditItem("Maximum:",
                           TEXTEDITINPUT,
                           self.GetMax())
        if (self.getDirection() == 1):
            ddValues = ["Up", "Down"]
        else:
            ddValues = ["Down", "Up"]
        editor.addEditItem("Direction:",
                           DROPDOWNEDITINPUT,
                           ddValues)
        editor.edit()
    def eScrolled(self, event):
        if (not self.editMode):
            value = self.getSliderValue()
            if (self.lastValue != value):
                self.lastValue = value
                self.sendMessage(repr(value))

#class for `decorative' rectangle
class mRectangle(mControl, wxStaticBox):
    def __init__(self, parentApp, type, id, text, pos, sSym, rSym, conn):
        wxStaticBox.__init__(self, parentApp.mainFrame.mainPanel,
                             id, text, pos, DEFRECTSIZE)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.resetBackground = True
    def SetValue(self, value):
        self.SetLabel(value)
    def eLeftDown(self, event):
        if (self.editMode):
            if (event.GetPosition()[0] < 20 or \
                event.GetPosition()[1] < 20 or \
                event.GetPosition()[0] > self.GetSize()[0] - 20 or\
                event.GetPosition()[1] > self.GetSize()[1] - 20):
                if (event.ControlDown()):
                    if (self.isSelected()):
                        self.deselect()
                    else:
                        self.select()
                else:
                    self.parentApp.deselectOthers(self.GetId())
                    self.select()
            else:
                self.parentApp.eLeftDown(event)
        else:
            event.Skip()


# Class for `decorative' text
class mText(mControl, wxStaticText):
    def __init__(self, parentApp, type, id, text, pos, sSym, rSym, conn):
        wxStaticText.__init__(self, parentApp.mainFrame.mainPanel,
                              id, text, pos, wxDefaultSize,
                              wxALIGN_CENTRE | wxST_NO_AUTORESIZE)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.resetBackground = True
    def SetValue(self, value):
        self.SetLabel(value)
        self.Refresh()

#Class for gauges
class mGauge(wxGauge, mSlider):
    def __init__(self, parentApp, type, id, label, pos,
                 style, sSym, rSym, conn):
        if (style & wxGA_HORIZONTAL):
            size = DEFHGAUGESIZE
        else:
            size = DEFVGAUGESIZE
        if (isinstance(label, list)):
            try:
                value = atoi(label[0])
            except:
                value = 0
            max = label[2]
        else:
            try:
                value = atoi(label)
            except:
                value = 0
            max = DEFSLIDERMAX
        wxGauge.__init__(self, parentApp.mainFrame.mainPanel,
                         id, max, pos, size, style | wxGA_SMOOTH)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        try:
            self.SetValue(value)
        except:
            pass
    def GetMin(self):
        return 0
    def GetMax(self):
        return self.GetRange()
    def SetRange(self, min, max):
        wxGauge.SetRange(self, max)
    def getDirection(self):
        return 0
    def setDirection(self, x):
        pass
    def GetLabel(self):
        return repr(self.GetValue())
    def SetLabel(self, value):
        try:
            self.SetValue(atoi(value))
        except:
            pass
    def PDAction(self, value):
        try:
            num = atof(value)
            if (num > self.GetMax()):
                num = self.GetMax()
            if (num < 0):
                num = 0
            self.SetValue(num)
        except:
            self.SetLabel(value)
    def editCallback(self, editValues):
        try:
            self.setPosition((atoi(editValues[0]), atoi(editValues[1])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Position",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.setSize(wxSize(atoi(editValues[2]), atoi(editValues[3])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.setSendSymbol(editValues[4])
        self.setReceiveSymbol(editValues[5])
        try:
            self.SetRange(0,
                          atoi(editValues[6]))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Maximum",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.SetForegroundColour(editValues[7])
        self.SetBackgroundColour(editValues[8])
    def eEdit(self, event):
        editor = controlEditor(self)
        editor.addEditItem("X Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[0])
        editor.addEditItem("Y Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[1])
        editor.addEditItem("Width:",
                           TEXTEDITINPUT,
                           self.GetSize()[0])
        editor.addEditItem("Height:",
                           TEXTEDITINPUT,
                           self.GetSize()[1])
        editor.addEditItem("S Symbol:",
                           TEXTEDITINPUT,
                           self.getSendSymbol())
        editor.addEditItem("R Symbol:",
                           TEXTEDITINPUT,
                           self.getReceiveSymbol())
        editor.addEditItem("Maximum:",
                           TEXTEDITINPUT,
                           self.GetMax())
        editor.addEditItem("F Color:",
                           COLOREDITINPUT,
                           self.GetForegroundColour())
        editor.addEditItem("B Color:",
                           COLOREDITINPUT,
                           self.GetBackgroundColour())
        editor.edit()

# Class for checkboxes
class mCheckBox(wxCheckBox, mToggle):
    def __init__(self, parentApp, type, id, text, pos, sSym, rSym, conn):
        wxCheckBox.__init__(self, parentApp.mainFrame.mainPanel,
                            id, text, pos)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.resetBackground = True
        EVT_CHECKBOX(parentApp.mainFrame.mainPanel,
                     self.GetId(), self.eClicked)

# Class for radio buttons
class mRadioButton(wxRadioButton, mToggle):
    def __init__(self, parentApp, type, id, text,
                 pos, sSym, rSym, style, conn):
        wxRadioButton.__init__(self, parentApp.mainFrame.mainPanel,
                               id, text, pos, wxDefaultSize, style)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.resetBackground = True
        EVT_RADIOBUTTON(parentApp.mainFrame.mainPanel,
                        self.GetId(), self.eClicked)
        self.SetValue(False)        
    def eClicked(self, event):
        if (not self.editMode and (self.GetValue() > 0)):        
            self.sendMessage("bang")

# Class for textboxes
class mTextBox(mControl, wxTextCtrl):
    def __init__(self, parentApp, type, id, label, pos, sSym, rSym, conn):
        wxTextCtrl.__init__(self, parentApp.mainFrame.mainPanel,
                            id, "", pos, wxDefaultSize,
                            wxTE_PROCESS_ENTER)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.SetValue(label)
        EVT_TEXT_ENTER(parentApp.mainFrame.mainPanel, self.GetId(), self.eEnter)
    def PDAction(self, value):
        if (value == "bang"):
            self.SetValue("")
        else: 
            self.SetValue(value)
    def GetLabel(self):
        return self.GetValue()
    def SetLabel(self, value):
        self.SetValue(value)
    def eEnter(self, event):
        value = self.GetValue()
        if (not self.editMode):
            self.sendMessage(value)

# Class for spin buttons
class mSpinButton(wxSpinButton, mSlider):
    def __init__(self, parentApp, type, id, label, pos, sSym, rSym, conn):
        wxSpinButton.__init__(self, parentApp.mainFrame.mainPanel,
                              id, pos, wxDefaultSize,
                              wxSP_VERTICAL)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        try:
            self.SetValue(atoi(label))
        except:
            pass
        EVT_SPIN(parentApp.mainFrame.mainPanel, self.GetId(), self.eSpin)
    def getDirection(self):
        return 0
    def setDirection(self, x):
        pass
    def PDAction(self, value):
        try:
            num = atof(value)
            if (num > self.GetMax()):
                num = self.GetMax()
            if (num < 0):
                num = 0
            self.SetValue(num)
        except:
            self.SetLabel(value)
    def GetLabel(self):
        return repr(self.GetValue())
    def SetLabel(self, value):
        try:
            self.SetValue(atoi(value))
        except:
            pass
    def eSpin(self, event):
        value = repr(self.GetValue())
        if (not self.editMode):
            self.sendMessage(value)
    def editCallback(self, editValues):
        try:
            self.setPosition((atoi(editValues[0]), atoi(editValues[1])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Position",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.setSize(wxSize(atoi(editValues[2]), atoi(editValues[3])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.setSendSymbol(editValues[4])
        self.setReceiveSymbol(editValues[5])
        try:
            self.SetRange(atoi(editValues[6]),
                          atoi(editValues[7]))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Range",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.SetForegroundColour(editValues[8])
        self.SetBackgroundColour(editValues[9])
    def eEdit(self, event):
        editor = controlEditor(self)
        editor.addEditItem("X Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[0])
        editor.addEditItem("Y Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[1])
        editor.addEditItem("Width:",
                           TEXTEDITINPUT,
                           self.GetSize()[0])
        editor.addEditItem("Height:",
                           TEXTEDITINPUT,
                           self.GetSize()[1])
        editor.addEditItem("S Symbol:",
                           TEXTEDITINPUT,
                           self.getSendSymbol())
        editor.addEditItem("R Symbol:",
                           TEXTEDITINPUT,
                           self.getReceiveSymbol())
        editor.addEditItem("Minimum:",
                           TEXTEDITINPUT,
                           self.GetMin())
        editor.addEditItem("Maximum:",
                           TEXTEDITINPUT,
                           self.GetMax())
        editor.addEditItem("F Color:",
                           COLOREDITINPUT,
                           self.GetForegroundColour())
        editor.addEditItem("B Color:",
                           COLOREDITINPUT,
                           self.GetBackgroundColour())
        editor.edit()

# Class for mouse capture area
class mMouseArea(wxPanel, mControl):
    def __init__(self, parentApp, type, id, pos, sSym, rSym, conn):
        wxPanel.__init__(self, parentApp.mainFrame.mainPanel, -1, pos,
                         (DEFMOUSEAREASIZE, DEFMOUSEAREASIZE), wxBORDER)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.mouseCaptured = False
        EVT_MIDDLE_DOWN(self, self.eMiddleDown)
        EVT_MIDDLE_UP(self, self.eMiddleUp)
        EVT_RIGHT_UP(self, self.eRightUp)
    def sendMouseInfo(self, xPos, yPos, buttonNum, buttonStatus):
        xPosStr = repr(xPos)
        yPosStr = repr(yPos)
        if (buttonNum > -1):
            buttonNumStr = repr(buttonNum)
            buttonStatusStr = repr(buttonStatus)
            self.sendMessage(xPosStr + " " + \
                             yPosStr + " " + \
                             buttonNumStr + " " + \
                             buttonStatusStr)
        else:
            self.sendMessage(xPosStr + " " + yPosStr)
    def editCallback(self, editValues):
        try:
            self.setPosition((atoi(editValues[0]), atoi(editValues[1])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Position",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.setSize(wxSize(atoi(editValues[2]), atoi(editValues[3])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.setSendSymbol(editValues[4])
        self.setReceiveSymbol(editValues[5])
        self.SetBackgroundColour(editValues[6])
    def eEdit(self, event):
        editor = controlEditor(self)
        editor.addEditItem("X Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[0])
        editor.addEditItem("Y Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[1])
        editor.addEditItem("Width:",
                           TEXTEDITINPUT,
                           self.GetSize()[0])
        editor.addEditItem("Height:",
                           TEXTEDITINPUT,
                           self.GetSize()[1])
        editor.addEditItem("S Symbol:",
                           TEXTEDITINPUT,
                           self.getSendSymbol())
        editor.addEditItem("R Symbol:",
                           TEXTEDITINPUT,
                           self.getReceiveSymbol())
        editor.addEditItem("Color:",
                           COLOREDITINPUT,
                           self.GetBackgroundColour())
        editor.edit()
    def eLeftDown(self, event):
        if (self.editMode):
            if (event.ControlDown()):
                if (self.isSelected()):
                    self.deselect()
                else:
                    self.select()
            else:
                self.parentApp.deselectOthers(self.GetId())
                self.select()
        else:
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               0,
                               1)
            event.Skip()
            self.eStartCapture(event)
    def eRightDown(self, event):
        if (self.editMode):
            self.parentApp.deselectOthers(self.GetId())
            self.select()
            self.eEdit(event)
        else:
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               2,
                               1)
    def eRightUp(self, event):
        if (not self.editMode):
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               2,
                               0)           
    def eMiddleDown(self, event):
        if (not self.editMode):
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               1,
                               1)
    def eMiddleUp(self, event):
        if (not self.editMode):
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               1,
                               0) 
    def eStartCapture(self, event):
        if (not self.mouseCaptured):
            self.CaptureMouse()
            self.mouseCaptured = True
            EVT_LEFT_UP(self, self.eEndCapture)
            EVT_MOTION(self, self.eCaptureMouse)
    def eEndCapture(self, event):
        if (self.mouseCaptured):
            self.ReleaseMouse()
            self.mouseCaptured = False
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               0,
                               0)
        self.Disconnect(-1, wxEVT_MOTION)
        self.Disconnect(-1, wxEVT_LEFT_UP)
    def eCaptureMouse(self, event):
        if (event.LeftIsDown() and self.mouseCaptured):
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               -1,
                               -1)

class mGraph(wxPanel, mSlider):
    def __init__(self, parentApp, type, id, label, pos, sSym, rSym, conn):
        wxPanel.__init__(self, parentApp.mainFrame.mainPanel, -1, pos,
                         (DEFMOUSEAREASIZE, DEFMOUSEAREASIZE), wxSIMPLE_BORDER)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.arrayLength = 0
        self.arrayRawData = []
        self.arrayDrawData = []
        self.min = -1
        self.max = 1
        self.mouseCaptured = False
        if (isinstance(label, list)):
            try:
                value = atoi(label[0])
            except:
                value = 0
            self.min = label[1]
            self.max = label[2]
            aData = label[3]
            self.arrayLength = aData[0]
            self.arrayRawData = aData[1:]
        if (self.arrayLength == 0):
            self.arrayLength = 100
            for i in range(0, 100):
                self.arrayRawData.append(0)
        for i in range(0, self.arrayLength):
            drawData = self.getDrawData(i, 0)
            self.arrayDrawData.append(drawData)
        EVT_PAINT(self, self.ePaint)
    def GetMin(self):
        return self.min
    def GetMax(self):
        return self.max
    def getDirection(self):
        return [self.arrayLength] + self.arrayRawData
    def PDAction(self, value):
        value = strip(value)
        if (lower(value) == "bang"):
            self.Refresh()
        else:
            isAnInt = False
            try:
                aLen = atoi(value)
                isAnInt = True
            except:
                pass
            if (isAnInt):
                if (aLen > self.arrayLength):
                    for i in range(self.arrayLength, aLen):
                        self.arrayRawData.append(0)
                        self.arrayDrawData.append(self.getDrawData(i, 0))
                elif (aLen < self.arrayLength):
                    del self.arrayRawData[aLen:]
                    del self.arrayDrawData[aLen:]
                self.arrayLength = aLen
                for i in range(0, self.arrayLength):
                    self.arrayDrawData[i] = self.getDrawData(i, 
                                                             self.arrayRawData[i])
            else:
                try:
                    [aIndex, aValue] = split(value, ' ')
                    aIndex = atoi(aIndex)
                    aValue = atof(aValue)
                    if (self.arrayRawData[aIndex] != aValue and \
                        aIndex >= 0 and \
                        aIndex < self.arrayLength):
                        self.arrayRawData[aIndex] = aValue
                        self.arrayDrawData[aIndex] = self.getDrawData(aIndex,
                                                                      aValue)
                except:
                    pass
            self.Refresh()
    def getDrawData(self, index, value):
        (xSize, ySize) = self.GetSize()
        div =  float(self.arrayLength - 1)
        if (div != 0):
            xScale = xSize / div
        else:
            xScale = 1
        div = float(self.max - self.min)
        if (div != 0): 
            yScale = ySize / div 
        else:
            yScale = 1
        yOffset = int(self.max * yScale)
        x = int(xScale * index)
        y = yOffset - int(yScale * value)
        pnt = (x,y)
        return pnt
    def setSize(self, size):
        self.SetSize(size)
        if (self.selected):
            self.relocateTags()
        for i in range(0, len(self.arrayRawData)):
            self.arrayDrawData[i] = self.getDrawData(i,
                                                     self.arrayRawData[i])
        self.Refresh()
    def editCallback(self, editValues):
        try:
            self.setPosition((atoi(editValues[0]), atoi(editValues[1])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Position",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.setSize(wxSize(atoi(editValues[2]), atoi(editValues[3])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.min = atof(editValues[6])
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Minimum",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.max = atof(editValues[7])
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Maximum",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.setSendSymbol(editValues[4])
        self.setReceiveSymbol(editValues[5])
        self.SetBackgroundColour(editValues[8])
        self.SetForegroundColour(editValues[9])
    def eEdit(self, event):
        editor = controlEditor(self)
        editor.addEditItem("X Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[0])
        editor.addEditItem("Y Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[1])
        editor.addEditItem("Width:",
                           TEXTEDITINPUT,
                           self.GetSize()[0])
        editor.addEditItem("Height:",
                           TEXTEDITINPUT,
                           self.GetSize()[1])
        editor.addEditItem("S Symbol:",
                           TEXTEDITINPUT,
                           self.getSendSymbol())
        editor.addEditItem("R Symbol:",
                           TEXTEDITINPUT,
                           self.getReceiveSymbol())
        editor.addEditItem("Minumum:",
                           TEXTEDITINPUT,
                           repr(self.min))
        editor.addEditItem("Maximum:",
                           TEXTEDITINPUT,
                           repr(self.max))      
        editor.addEditItem("B Color:",
                           COLOREDITINPUT,
                           self.GetBackgroundColour())
        editor.addEditItem("F Color:",
                           COLOREDITINPUT,
                           self.GetForegroundColour())
        editor.edit()
    def ePaint(self, event):
        event.Skip()
        myDC = wxPaintDC(self)
        myDC.BeginDrawing()
        myDC.SetPen(wxPen(self.GetForegroundColour(), 1, wxSOLID))
        myDC.SetBrush(wxBrush("#000000", wxTRANSPARENT))
        myDC.DrawLines(self.arrayDrawData)
        myDC.EndDrawing()
    def eDrawMotion(self, event):
        if (self.mouseCaptured):
            (mouseX, mouseY) = event.GetPosition()
            div = float(self.arrayLength - 1)
            if (div != 0):
                xScale = self.GetSize()[0] / div
            else:
                xScale = 1
            div = float(self.max - self.min)
            if (div != 0):
                yScale = self.GetSize()[1] / div
            else:
                yScale = 1
            yOffset = int(self.max * yScale)
            if (xScale != 0):
                x = int(mouseX / xScale)
            else:
                x = 0
            if (yScale != 0):
                y = self.max - (mouseY / yScale)
            else:
                y = 0
            if (x >= 0 and x < self.arrayLength and \
                y >= self.min and y <= self.max):
                self.arrayRawData[x] = y
                self.arrayDrawData[x] = self.getDrawData(x, y)
                self.Refresh()
                self.sendMessage(repr(x) + " " + repr(y))
    def eLeftDown(self, event):
        if (self.editMode):
            if (event.ControlDown()):
                if (self.isSelected()):
                    self.deselect()
                else:
                    self.select()
            else:
                self.parentApp.deselectOthers(self.GetId())
                self.select()
        else:
            event.Skip()
            self.eStartCapture(event)
    def eStartCapture(self, event):
        if (not self.mouseCaptured):
            self.CaptureMouse()
            self.mouseCaptured = True
            EVT_LEFT_UP(self, self.eEndCapture)
            EVT_MOTION(self, self.eDrawMotion)
    def eEndCapture(self, event):
        if (self.mouseCaptured):
            self.ReleaseMouse()
            self.mouseCaptured = False
        self.Disconnect(-1, wxEVT_MOTION)
        self.Disconnect(-1, wxEVT_LEFT_UP)
    def eCaptureMouse(self, event):
        pass

# Class for clickable image
# seems to have problems capturing the mouse?
class mImage(mMouseArea, wxStaticBitmap):
    def __init__(self, parentApp, type, id, filepath,
                 pos, sSym, rSym, conn):
        self.filename = filepath
        self.image = wxImage(self.filename,
                             wxBITMAP_TYPE_ANY)
        tempBitmap = self.image.ConvertToBitmap()
        if (not tempBitmap.Ok()):
            raise Exception
        wxStaticBitmap.__init__(self, parentApp.mainFrame.mainPanel, id,
                                tempBitmap, pos, wxDefaultSize)
        mControl.__init__(self, parentApp, type, sSym, rSym, conn)
        self.resetBackground = True
        self.mouseCaptured = False
        EVT_LEFT_DOWN(self, self.eLeftDown)
        EVT_MIDDLE_DOWN(self, self.eMiddleDown)
        EVT_MIDDLE_UP(self, self.eMiddleUp)
        EVT_RIGHT_UP(self, self.eRightUp)
    def sendMouseInfo(self, xPos, yPos, buttonNum, buttonStatus):
        xPosStr = repr(xPos)
        yPosStr = repr(yPos)
        if (buttonNum > -1):
            buttonNumStr = repr(buttonNum)
            buttonStatusStr = repr(buttonStatus)
            self.sendMessage(xPosStr + " " + \
                             yPosStr + " " + \
                             buttonNumStr + " " + \
                             buttonStatusStr)
        else:
            self.sendMessage(xPosStr + " " + yPosStr)
    def resize(self, deltaPos):
        if (self.editMode):
            xSize = self.GetSize()[0] + deltaPos[0]
            ySize = self.GetSize()[1] + deltaPos[1]
            if (xSize > 0 and ySize > 0):
                if (xSize <= self.image.GetWidth() and
                    ySize <= self.image.GetHeight()):
                    self.setSize((xSize, ySize))
                elif (xSize <= self.image.GetWidth()):
                    self.setSize((xSize, self.GetSize()[1]))
                elif (ySize <= self.image.GetHeight()):
                    self.setSize((self.GetSize()[0], ySize))
    def setSize(self, size):
        self.SetSize(size)
        if (self.selected):
            self.relocateTags()
        newImage = self.image.GetSubImage((0,0,size[0],size[1]))
        newBitmap = newImage.ConvertToBitmap()
        self.SetBitmap(newBitmap)
    def setEditMode(self, mode):
        if (mode):
            if (not self.editMode):
                EVT_RIGHT_DOWN(self, self.eRightDown)
                self.moveTag = controlEditTag(self,
                                              (0,0),
                                              wxBLACK,
                                              wxCURSOR_CROSS)
                self.BLResizeTag = controlResizeTag(self,
                                                    (1,0),
                                                    RESIZETAGCOLOR,
                                                    wxCURSOR_SIZEWE)
                self.TRResizeTag = controlResizeTag(self,
                                                    (0,1),
                                                    RESIZETAGCOLOR,
                                                    wxCURSOR_SIZENS)
                self.BRResizeTag = controlResizeTag(self,
                                                    (1,1),
                                                    RESIZETAGCOLOR,
                                                    wxCURSOR_SIZENWSE)
                self.editMode = True
        else:
            if (self.editMode):
                self.Disconnect(-1, wxEVT_RIGHT_DOWN)
                self.deselect()            
                self.moveTag.Destroy()
                self.BLResizeTag.Destroy()
                self.TRResizeTag.Destroy()
                self.BRResizeTag.Destroy()
            self.editMode = False
    def Refresh(self):
        if (self.filename != self.GetLabel()):
            self.SetLabel(self.filename)
        wxStaticBitmap.Refresh(self)
        wxYield()
        self.parentApp.mainFrame.mainPanel.Refresh()
    def SetLabel(self, label):
        self.filename = label
        self.image = wxImage(self.filename,
                             wxBITMAP_TYPE_ANY)
        tempBitmap = self.image.ConvertToBitmap()
        self.SetBitmap(tempBitmap)
        self.setSize((self.image.GetWidth(), self.image.GetHeight()))
        wxStaticBitmap.Refresh(self)
        self.parentApp.mainFrame.mainPanel.Refresh()
    def GetLabel(self):
        return self.filename
    def PDAction(self, value):
        if (value[0:4] == "hide"):
            self.Show(False)
            # self.parentApp.mainFrame.mainPanel.Refresh()
        elif (value[0:4] == "show"):
            self.Show(True)
            # self.parentApp.mainFrame.mainPanel.Refresh()
        elif (value[0:6] == "rotate"):
            theta = atof(value[6:len(value)]) * math.pi / 180
            tempImage = wxImage(self.filename,wxBITMAP_TYPE_ANY)
            bgColor = self.parentApp.mainFrame.mainPanel.GetBackgroundColour()
            tempImage.SetMaskColour(bgColor.Red(),
                                    bgColor.Green(),
                                    bgColor.Blue())
            rotateImage = tempImage.Rotate(theta, (0,0), True)
            bgColor = self.parentApp.mainFrame.mainPanel.GetBackgroundColour()
            self.SetBitmap(rotateImage.ConvertToBitmap())
            wxStaticBitmap.Refresh(self)
            self.parentApp.mainFrame.mainPanel.Refresh()
        else:
            if (value[0:2] == "./" or
                value[0:3] == "../"):
                value = scrubPath(self.parentApp.filepath + value)
            self.SetLabel(value)
    def editCallback(self, editValues):
        try: 
            self.setPosition((atoi(editValues[0]), atoi(editValues[1])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        try:
            self.setSize(wxSize(atoi(editValues[2]), atoi(editValues[3])))
        except:
            dlg = wxMessageDialog(self.parentApp.mainFrame,
                                  "Invalid Size",
                                  "Edit Error",
                                  wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.setSendSymbol(editValues[4])
        self.setReceiveSymbol(editValues[5])
        self.SetLabel(editValues[6])
    def eEdit(self, event):
        editor = controlEditor(self)
        editor.addEditItem("X Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[0])
        editor.addEditItem("Y Position:",
                           TEXTEDITINPUT,
                           self.GetPosition()[1])
        editor.addEditItem("width:",
                           TEXTEDITINPUT,
                           self.GetSize()[0])
        editor.addEditItem("Height:",
                           TEXTEDITINPUT,
                           self.GetSize()[1])
        editor.addEditItem("S Symbol:",
                           TEXTEDITINPUT,
                           self.getSendSymbol())
        editor.addEditItem("R Symbol:",
                           TEXTEDITINPUT,
                           self.getReceiveSymbol())
        editor.addEditItem("File:",
                           FILEEDITINPUT,
                           self.GetLabel())
        editor.edit()
    def eStartCapture(self, event):
        if (not self.mouseCaptured):
            self.mouseCaptured = True
            EVT_LEFT_UP(self, self.eEndCapture)
            EVT_MOTION(self, self.eCaptureMouse)
    def eEndCapture(self, event):
        if (self.mouseCaptured):
            self.mouseCaptured = False
            self.sendMouseInfo(event.GetPosition()[0],
                               event.GetPosition()[1],
                               0,
                               0)
        self.Disconnect(-1, wxEVT_MOTION)
        self.Disconnect(-1, wxEVT_LEFT_UP)

class editItem:
    def __init__(self, title, dataType, value):
        self.title = title
        self.dataType = dataType
        self.value = value
    def setValue(self, value):
        self.value = value

class editSetButton(wxButton):
    def __init__(self, parent, position, size, value):
        wxButton.__init__(self, parent, -1, "Set", position, size)
        self.parent = parent
        self.value = value
        EVT_BUTTON(parent, self.GetId(), self.eClick)
    def GetValue(self):
        return self.value

class editSetFontButton(editSetButton):
    def eClick(self, event):
        self.value = getFontFromDialog(self.parent, self.value)

class editSetColorButton(editSetButton):
    def eClick(self, event):
        self.value = getColorFromDialog(self.parent, self.value)

class editSetFileButton(editSetButton):
    def eClick(self, event):
        tempFile = getFileFromDialog(self.parent, self.value)
        if (tempFile):
            self.value = tempFile

class editDropdown(wxComboBox):
    def __init__(self, parent, position, size, values):
        wxComboBox.__init__(self, 
                            parent, 
                            -1, 
                            values[0],  
                            position,
                            size,
                            values,
                            wxCB_READONLY)
        self.parent = parent

class controlEditor:
    def __init__(self, control):
        self.control = control
        self.editItems = []
        self.valueControls = []
    def addEditItem(self, title, dataType, value):
        self.editItems.append(editItem(title, dataType, value))
    def edit(self):
        global lastEditWindowPosition
        if (lastEditWindowPosition == (-1, -1)):
            lastEditWindowPosition = ( \
               self.control.parentApp.mainFrame.GetPosition()[0] + 20,
               self.control.parentApp.mainFrame.GetPosition()[1] + 20)
        self.editFrame = wxMiniFrame(self.control, 
                                -1, 
                                "Edit Control", 
                                lastEditWindowPosition, 
                                (100, 200),
                                wxCAPTION | wxSTAY_ON_TOP)
        self.editFrame.editPanel = wxPanel(self.editFrame, -1, (0,0), (-1, -1))
        labels = []
        self.valueControls = []
        count = 0
        newLabel = wxStaticText(self.editFrame.editPanel, 
                                -1, 
                                self.editItems[0].title, 
                                DEFEDITPOS)
        labels.append(newLabel)
        if (type(self.editItems[0].value) == type(0)):
            value = repr(self.editItems[0].value)
        else:
            value = self.editItems[0].value
        if (self.editItems[0].dataType == TEXTEDITINPUT):
            newControl = wxTextCtrl(self.editFrame.editPanel,
                                    -1,
                                    value,
                                    (labels[0].GetPosition()[0] \
                                     + 65,
                                     labels[0].GetPosition()[1] - 2),
                                    (60,-1))
        if (self.editItems[0].dataType == FONTEDITINPUT):
            newControl = editSetFontButton(self.editFrame.editPanel,
                                           (labels[0].GetPosition()[0] + 65,
                                            labels[0].GetPosition()[1] - 2),
                                           (60, -1),
                                           self.editItems[0].value)           
        if (self.editItems[0].dataType == COLOREDITINPUT):
            newControl = editSetColorButton(self.editFrame.editPanel,
                                            (labels[0].GetPosition()[0] + 65,
                                             labels[0].GetPosition()[1] - 2),
                                            (60, -1),
                                            self.editItems[0].value)
        if (self.editItems[0].dataType == FILEEDITINPUT):
            newControl = editSetFileButton(self.editFrame.editPanel,
                                           (labels[0].GetPosition()[0] + 65,
                                            labels[0].GetPosition()[1] - 2),
                                           (60, -1),
                                           self.editItems[0].value)
        if (self.editItems[0].dataType == DROPDOWNEDITINPUT):
            newControl = editDropdown(self.editFrame.editPanel,
                                      labels[0].GetPosition()[0] + 65,
                                      labels[0].GetPosition()[1] - 2,
                                      (60, -1),
                                      self.editItems[0].value)
        self.valueControls.append(newControl)
        for i in self.editItems[1:]:
            count = count + 1
            if (type(i.value) == type(0)):
                value = repr(i.value)
            else:
                value = i.value
            newLabel = wxStaticText(self.editFrame.editPanel, 
                           -1, 
                           i.title,
                           (labels[count - 1].GetPosition()[0],
                            self.valueControls[count - 1].GetPosition()[1] \
                            + self.valueControls[count - 1].GetSize()[1] + 10))
            labels.append(newLabel)
            if (i.dataType == TEXTEDITINPUT):
                newControl = wxTextCtrl(self.editFrame.editPanel, 
                              -1,
                              value,
                              (self.valueControls[count - 1].GetPosition()[0],
                               labels[count].GetPosition()[1] - 2),
                               self.valueControls[count - 1].GetSize())
            if (self.editItems[count].dataType == FONTEDITINPUT):
                newControl = editSetFontButton(self.editFrame.editPanel,
                               (self.valueControls[count - 1].GetPosition()[0],
                                labels[count].GetPosition()[1] - 2),
                               self.valueControls[count - 1].GetSize(),
                               self.editItems[count].value)
            if (self.editItems[count].dataType == COLOREDITINPUT):
                newControl = editSetColorButton(self.editFrame.editPanel,
                               (self.valueControls[count - 1].GetPosition()[0],
                                labels[count].GetPosition()[1] - 2),
                               self.valueControls[count - 1].GetSize(),
                               self.editItems[count].value)
            if (self.editItems[count].dataType == FILEEDITINPUT):
                newControl = editSetFileButton(self.editFrame.editPanel,
                               (self.valueControls[count - 1].GetPosition()[0],
                                labels[count].GetPosition()[1] - 2),
                               self.valueControls[count - 1].GetSize(),
                               self.editItems[count].value)
            if (self.editItems[count].dataType == DROPDOWNEDITINPUT):
                newControl = editDropdown(self.editFrame.editPanel,
                               (self.valueControls[count - 1].GetPosition()[0],
                                labels[count].GetPosition()[1] - 2),
                               self.valueControls[count - 1].GetSize(),
                               self.editItems[count].value) 
            self.valueControls.append(newControl)
        line = wxStaticLine(self.editFrame.editPanel, -1,
                            (0, self.valueControls[-1].GetPosition()[1] \
                             + self.valueControls[-1].GetSize()[1] + 20),
                             (self.valueControls[0].GetPosition()[0] \
                             + self.valueControls[0].GetSize()[0] + 10, 1))
        okButton = wxButton(self.editFrame.editPanel, -1, "OK",
                            (labels[0].GetPosition()[0],
                             line.GetPosition()[1] + 15),
                            (self.valueControls[0].GetPosition()[0] \
                             + self.valueControls[0].GetSize()[0], -1))
        self.editFrame.SetSize((okButton.GetPosition()[0] + \
                       okButton.GetSize()[0] + 10,
                       okButton.GetPosition()[1] + okButton.GetSize()[1] \
                        + (okButton.GetPosition()[1] - line.GetPosition()[1]) \
                        + 20))
        self.editFrame.editPanel.SetSize(self.editFrame.GetSize())
        EVT_BUTTON(self.editFrame.editPanel, 
                   okButton.GetId(), 
                   self.eOk)
        self.editFrame.Show(True)
        self.editFrame.MakeModal(True)
    def eOk(self, event):
        global lastEditWindowPosition
        returnValues = []
        for i in range(0, len(self.valueControls)):
            returnValues.append(self.valueControls[i].GetValue())
        lastEditWindowPosition = self.editFrame.GetPosition()
        self.editFrame.MakeModal(False)
        self.editFrame.Destroy()
        self.control.editCallback(returnValues)

