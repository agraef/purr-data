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
from gripdControls import *
from gripdFunctions import *
from socket import *
from string import *

midi               = 0
joystick           = 0
DEFMAINFRAMESIZE   = (600, 400)
DEFCONTROLPOS      = (20,20)
ID_ABOUT           = 100
ID_EXIT            = 101
ID_ADD             = 102
ID_EDITMODE        = 103
ID_EDIT            = 104
ID_DELETE          = 105
ID_CONNECT         = 106
ID_OPEN            = 107
ID_SAVE            = 108
ID_OPTIONS         = 109
ID_BUTTONMENU      = 110
ID_SLIDERMENU      = 111
ID_GAUGEMENU       = 112
ID_SELECTALL       = 113
ID_NEW             = 114
ID_DUPLICATE       = 115
ID_ALIGNVERT       = 116
ID_ALIGNHORZ       = 117
ID_REFRESH         = 118
ID_BUTTON          = 200
ID_TOGGLE          = 201
ID_VSLIDER         = 202
ID_HSLIDER         = 203
ID_RECT            = 204
ID_TEXT            = 205
ID_SETOPTS         = 206
ID_DISCONNECT      = 207
ID_VGAUGE          = 208
ID_HGAUGE          = 209
ID_CHECKBOX        = 210
ID_TEXTBOX         = 211
ID_SPINBUTTON      = 212
ID_MOUSEAREA       = 213
ID_ENABLEJOY       = 214
ID_ENABLEKEY       = 215
ID_LOCKONPERF      = 216
ID_RADIOBUTTON     = 217
ID_IMAGE           = 218
ID_ALWAYSONTOP     = 219
ID_SHOWGRID        = 220
ID_SNAPTOGRID      = 221
ID_ENABLEMIDI      = 222
ID_GRAPH           = 223
COMMANDCHAR        = '!'
DEFHOST            = 'localhost'
DEFPORT            = 3490
CONNECTINTERVAL    = 1000
TIMEOUT            = 20
HIDECOMMAND        = "!hide"
SHOWCOMMAND        = "!show"
EXITCOMMAND        = "!exit"
LOCKCOMMAND        = "!lock"
UNLOCKCOMMAND      = "!unlock"
SETTITLECOMMAND    = "!settitle"
CLOSECOMMAND       = "!disconnect"
PINGCOMMAND        = "!ping"
OPENPANELCOMMAND   = "!openpanel"
SAVEPANELCOMMAND   = "!savepanel"
BEGINCONTROLMARKER = "!BEGIN-CONTROL"
ENDCONTROLMARKER   = "!END-CONTROL"
DEFOPTIONSPOS      = (5, 5)
DUPPOSOFFSET       = (10, 10)
SMALLCHARMOVE      = 1
LARGECHARMOVE      = 20
BGCOLORDIF         = 30
DEFSOCKETTIMERTIME = 5
DEFJOYPOLLTIME     = 10
DEFMIDIPOLLTIME    = 5
DEFMIDIDEVICE1     = 0
DEFMIDIDEVICE2     = 1
PAIRSEPCHAR        = chr(29)
SYMMSGSEP          = chr(31)
FILETOKENSEP       = "|"
MAXPINGCOUNT       = 10
PINGPOLLTIME       = 1000
MIDINOTEMESSAGE    = "9"
MIDICTLMESSAGE     = "b"
MIDIPGMMESSAGE     = "c"
SETARRAYSTRING     = "!setArray"

if (os.name == "posix"):
    FUGEFACTOR = 1
    DEFJOYDEVICE1 = "/dev/js0"
    DEFJOYDEVICE2 = "/dev/js1"    
else:
    FUGEFACTOR = 2
    DEFJOYDEVICE1 = "1"
    DEFJOYDEVICE2 = "2"    
    
# Main application class
class mainApp(wxPySimpleApp):
    def __init__(self, args):
        self.args = args
        wxApp.__init__(self, False)
        try:
            wxLog_SetLogLevel(0)
        except:
            pass
    def OnInit(self):
        global midi
        global joystick
        openAuto = False
        newPort = 0
        filename = ""
        locked = False
        self.path = scrubPath(getDirectory(self.args[0]))
        setDebugFile(self.path + "/log.txt")
        self.filepath = ""
        if (len(self.args) > 1):
            filename = self.args[1]
            self.filepath = getDirectory(filename)
            if (self.filepath == ""):
                self.filepath = self.path
            self.filepath = makeAbsolutePath(self.path, self.filepath)
            if (len(self.args) > 2):
                newPort = atoi(self.args[2])
                if (len(self.args) > 3):
                    openAuto = atoi(self.args[3])
                    if (len(self.args) > 4):
                        locked = atoi(self.args[4])
        self.midiAvailable = True
        self.joystickAvailable = True
        try:
            optionsFile = open(self.path + "/gripd.opt", 'r', -1)
            options = optionsFile.readlines()
            for option in options:
                [key, value] = split(option, '=')
                key = strip(key)
                value = strip(value)
                if (lower(key) == 'midi'):
                    if (lower(value) == 'false'):
                        self.midiAvailable = False
                if (lower(key) == 'joystick'):
                    if (lower(value) == 'false'):
                        self.joystickAvailable = False
        except:
            pass
        if (self.midiAvailable):
            import midi
        if (self.joystickAvailable):
            import joystick
        wxInitAllImageHandlers()
        if (not os.name == "posix"):
            self.icon = wxIcon(self.path + "icon.pic", wxBITMAP_TYPE_ICO)
        self.jsImage = wxImage(self.path + "joystick.xpm",
                               wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.jsImageX = wxImage(self.path + "joystickX.xpm",
                                wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.jsImageA = wxImage(self.path + "joystickA.xpm",
                                wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.midiImage = wxImage(self.path + "midi.xpm",
                                 wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.midiImageX = wxImage(self.path + "midiX.xpm",
                                  wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.midiImageA = wxImage(self.path + "midiA.xpm",
                                  wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.connectImage = wxImage(self.path + "connect.xpm",
                                    wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.connectImageX = wxImage(self.path + "connectX.xpm",
                                     wxBITMAP_TYPE_XPM).ConvertToBitmap()
        self.editMode = False
        # socket stuff
        self.host = DEFHOST
        self.port = DEFPORT
        self.connection = nullConnection()
        self.connected = False
        # All controls (on mainPanel)
        self.controlList = []
        # Selected controls, Should speed up editing
        # DO NOT use to loop through selected controls to deselect
        # <control>.deselect() modifies selectedControlList
        self.selectedControlList = []
        # If currently using selecting with selectRect
        self.selecting = False
        # If currently dragging a control
        self.dragging = False
        # Keys currently depressed
        self.keysDown = []
        # If left mouse was clicked on panel
        # not on control or tag, for selectRect
        self.leftDownOnPanel = False
        # If current GUI has been changed since last save
        self.edited = False
        # control counters (for send/receive syms)
        self.resetControlIndecies()
        # timer to check connection status
        self.pTimer = pingTimer(self)
        # set ping count
        self.pingCount = 0
        # timer to poll for connections
        self.cTimer = connectionTimer(self)
        # socket polling time
        self.pollTime = DEFSOCKETTIMERTIME
        # timer to poll socket for receive messages
        self.sTimer = socketTimer(self)
        # timer to poll joystick
        self.joyTimer = joystickTimer(self)     
        # joystick poll time
        self.joyPollTime = DEFJOYPOLLTIME
        # joystick device string
        self.joyDevice1 = DEFJOYDEVICE1
        self.joyDevice2 = DEFJOYDEVICE2
        # joystick device id
        self.joyID1 = -1
        self.joyID2 = -1
        # joystick on
        self.joystickEnabled = False
        self.jsOn1 = False
        self.jsOn2 = False
        # timer to poll midi
        self.midiTimer = midiTimer(self)     
        # midi poll time
        self.midiPollTime = DEFMIDIPOLLTIME
        # midi device numbers
        self.midiDevice1 = DEFMIDIDEVICE1
        self.midiDevice2 = DEFMIDIDEVICE2
        # midi device id
        self.midiID1 = -1
        self.midiID2 = -1
        # midi on
        self.midiEnabled = False
        self.midiOn1 = False
        self.midiOn2 = False
        # send keystrokes
        self.sendKeys = True
        # grid sized
        self.gridSpace = 10
        # display grid
        self.showGrid = False
        # snap moved controls to grid
        self.snapToGrid = False
        # locked or unlocked GUI
        self.locked = locked
        self.openAuto = openAuto
        self.frameTitle = "GrIPD"
        self.alwaysOnTop = False
        self.resizable = False
        self.mainFrame = gripdFrame(self, -1, self.frameTitle,
                                    (-1, -1),
                                    self.resizable, self.locked,
                                    self.alwaysOnTop)
        self.SetTopWindow(self.mainFrame)
        # for catching keystokes
        EVT_CHAR(self.mainFrame.mainPanel, self.eChar)
        EVT_KEY_UP(self.mainFrame.mainPanel, self.eKeyUp)
        EVT_KEY_DOWN(self.mainFrame.mainPanel, self.eKeyDown)
        if (not os.name == "posix"):
            EVT_CHAR_HOOK(self.mainFrame, self.eChar)
        EVT_CLOSE(self.mainFrame, self.eClose)
        EVT_SIZE(self.mainFrame, self.eSize)
        self.mainFrame.positionIcons()
        if ((filename != "0") and (filename != "")):
            self.openFromDisk(filename)
        if (newPort):
            self.port = newPort
        if (self.openAuto):
            self.connect()
        else:
            self.mainFrame.Show(True)
        return True

## Functions ##

    def createControl(self, type, tIndex, tString, filepath = ""):
        if (self.editMode and not self.dragging):
            indexStr = tString + repr(tIndex)
            clientMousePos = self.mainFrame.mainPanel.ScreenToClient( \
                                                        wxGetMousePosition())
            coords = (clientMousePos[0] + 4, clientMousePos[1] + 4)
            control = self.addControl(type, indexStr, coords, 
                                      "s" + indexStr, "r" + indexStr, filepath)
            control.select()
            # I'm not sure why this is necessary
            try:
                wxYield()
            except:
                pass
            control.grab()
    def addControl(self, type, label, pos, sSym, rSym, filepath = ""):
        tControl = NULL
        if (type == MBUTTONTYPE):
            self.buttonIndex = self.buttonIndex + 1
            tControl = mButton(self, type, -1,
                               label, pos, sSym, rSym, self.connection)
        if (type == MTOGGLETYPE):
            self.toggleIndex = self.toggleIndex + 1
            tControl = mToggle(self, type, -1,
                               label, pos, sSym, rSym, self.connection)
        if (type == MVSLIDERTYPE):
            self.sliderIndex = self.sliderIndex + 1
            tControl = mSlider(self, type, -1, label, pos,
                               wxSL_VERTICAL, sSym, rSym, self.connection)
        if (type == MHSLIDERTYPE):
            self.sliderIndex = self.sliderIndex + 1
            tControl = mSlider(self, type, -1, label, pos,
                               wxSL_HORIZONTAL, sSym, rSym, self.connection)
        if (type == MRECTTYPE):
            self.rectIndex = self.rectIndex + 1
            tControl = mRectangle(self, type, -1,
                                  label, pos, sSym, rSym, self.connection)
        if (type == MTEXTTYPE):
            self.textIndex = self.textIndex + 1
            tControl = mText(self, type, -1,
                             label, pos, sSym, rSym, self.connection)
        if (type == MVGAUGETYPE):
            self.gaugeIndex = self.gaugeIndex + 1
            tControl = mGauge(self, type, -1, label, pos,
                              wxGA_VERTICAL, sSym, rSym, self.connection)
        if (type == MHGAUGETYPE):
            self.gaugeIndex = self.gaugeIndex + 1
            tControl = mGauge(self, type, -1, label, pos,
                              wxGA_HORIZONTAL, sSym, rSym, self.connection)
        if (type == MCHECKBOXTYPE):
            self.checkBoxIndex = self.checkBoxIndex + 1
            tControl = mCheckBox(self, type, -1,
                                 label, pos, sSym, rSym, self.connection)
        if (type == MRADIOBUTTONTYPE):
            self.radioButtonIndex = self.radioButtonIndex + 1
            tControl = mRadioButton(self, type, -1,
                                    label, pos, sSym, rSym, 0, self.connection)
        if (type == MRADIONEWTYPE):
            self.radioButtonIndex = self.radioButtonIndex + 1
            tControl = mRadioButton(self, type, -1,
                                    label, pos, sSym, rSym, wxRB_GROUP,
                                    self.connection)
        if (type == MTEXTBOXTYPE):
            self.textBoxIndex = self.textBoxIndex + 1
            tControl = mTextBox(self, type, -1, label,
                                pos, sSym, rSym, self.connection)
        if (type == MSPINBUTTONTYPE):
            self.spinButtonIndex = self.spinButtonIndex + 1
            tControl = mSpinButton(self, type, -1, label,
                                   pos, sSym, rSym, self.connection)
        if (type == MMOUSEAREATYPE):
            self.mouseAreaIndex = self.mouseAreaIndex + 1
            tControl = mMouseArea(self, type, -1,
                                  pos, sSym, rSym, self.connection)
        if (type == MIMAGETYPE):
            if (filepath == ""):
                filepath = label
            filepath = makeAbsolutePath(self.filepath, filepath)
            self.imageIndex = self.imageIndex + 1
            try:
                tControl = mImage(self, type, -1,
                                  filepath, pos, sSym, rSym, self.connection)
            except:
                tControl = NULL
        if (type == MGRAPHTYPE):
            self.graphIndex = self.graphIndex + 1
            tControl = mGraph(self, type, -1, label, pos, sSym, 
                              rSym, self.connection) 
        if (tControl != NULL):
            self.controlList.append(tControl)
        return (tControl)
    def findControlByID(self, id):
        for control in self.controlList:
            if (control.GetId() == id):
                return(control)
    def startMoveControls(self):
        for control in self.selectedControlList:
            control.startMove()
    def moveSelectedControls(self, deltaPos):
        self.edited = True
        for control in self.selectedControlList:
            control.move(deltaPos)
    def endMoveControls(self):
        for control in self.selectedControlList:
            control.endMove()
    def endDragMoveControls(self):
        for control in self.selectedControlList:
            control.endDragMove()
    def resizeSelectedControls(self, deltaPos):
        self.edited = True
        for control in self.selectedControlList:
            control.resize(deltaPos)
    def moveKey(self, deltaPos):
        self.moveSelectedControls(deltaPos)
        for control in self.selectedControlList:
            control.endMove()
    def alignControls(self, dir):
        otherDir = 1 - dir
        alignVal = 1
        alignPos = [0, 0]
        for control in self.selectedControlList:
            alignVal = max(alignVal, control.GetPosition()[dir])
            alignPos[dir] = alignVal
        for control in self.selectedControlList:
            alignPos[otherDir] = control.GetPosition()[otherDir]            
            control.setPosition(alignPos)
    def deselectOthers(self, calledByID):
        # Not using selectedControlList since we are removing from it
        # as we cycle through the list
        for control in self.controlList:
            if ((control.GetId() != calledByID) and control.isSelected()):
                control.deselect()
    def raiseControlTags(self):
        if (self.editMode):
            for control in self.selectedControlList:
                control.relocateTags()
                control.refreshTags()
    def traverseFocus(self, multi):
        if ((len(self.controlList) > 0) and (self.editMode)):
            i = -1
            newIndex = 0
            for control in self.controlList:
                i = i + 1
                if (control.isSelected()):
                    newIndex = i + 1
            newIndex = newIndex % len(self.controlList)
            self.controlList[newIndex].select()
            if (not multi):
                self.deselectOthers(self.controlList[newIndex].GetId())
    def parseReceiveBuffer(self, receiveBuffer):
        # remove trailing PAIRSEPCHAR
        receiveBuffer = receiveBuffer[0:-1]
        messagePairs = split(receiveBuffer, PAIRSEPCHAR)
        for pair in messagePairs:
            (symName, value) = split(pair, SYMMSGSEP)
            if (symName[0] == COMMANDCHAR):
                if (symName == CLOSECOMMAND):
                    self.disconnect()
                    dlg = wxMessageDialog(self.mainFrame,
                                          "Connection closed by PD",
                                          "Connection", wxOK)
                    dlg.ShowModal()
                    dlg.Destroy()
                if (symName == EXITCOMMAND):
                    self.close()
                elif (symName == LOCKCOMMAND):
                    self.lock()
                elif (symName == UNLOCKCOMMAND):
                    self.unlock()
                elif (symName == SETTITLECOMMAND):
                    self.frameTitle = value
                    self.setFrameTitle(value)
                elif (symName == HIDECOMMAND):
                    self.mainFrame.Show(False)
                    self.connection.send(HIDECOMMAND + \
                                         SYMMSGSEP + "0" + \
                                         PAIRSEPCHAR)
                elif (symName == SHOWCOMMAND):
                    self.mainFrame.Show(True)
                    self.connection.send(SHOWCOMMAND + \
                                         SYMMSGSEP + "0" + \
                                         PAIRSEPCHAR)
                elif (symName == PINGCOMMAND):
                    self.pingCount = 0
                elif (symName == OPENPANELCOMMAND):
                    self.mainFrame.openpanel()
                elif (symName == SAVEPANELCOMMAND):
                    self.mainFrame.savepanel()
            else:
                for control in self.controlList:
                    if (symName == control.getReceiveSymbol()):
                        control.PDAction(value)
    def writeControls(self, list, file):
        for control in list:
            file.write(repr(control.getType()) + "\n")
            if (control.type == MIMAGETYPE):
                file.write(makeRelativePath(self.filepath,
                                            control.GetLabel()) + "\n")
            else:
                file.write(control.GetLabel() + "\n")
            file.write(repr(control.GetPosition()[0]) + "\n")
            file.write(repr(control.GetPosition()[1]) + "\n")
            file.write(repr(control.GetSize()[0]) + "\n")
            file.write(repr(control.GetSize()[1]) + "\n")
            file.write(control.getSendSymbol() + "\n")
            file.write(control.getReceiveSymbol() + "\n")
            if (control.GetBackgroundColour().Ok()):
                file.write("#" + colorTuple2HexString((control.\
                                                GetBackgroundColour().Red(),
                                                control.\
                                                GetBackgroundColour().Green(),
                                                control.\
                                                GetBackgroundColour().Blue()))\
                                                + "\n")
            else:
                file.write("#00000\n")
            if (control.GetForegroundColour().Ok()):
                file.write("#" + colorTuple2HexString((control.\
                                                 GetForegroundColour().Red(),
                                                 control.\
                                                 GetForegroundColour().Green(),
                                                 control.\
                                                 GetForegroundColour().Blue()))\
                                                 + "\n")
            else:
                file.write("#000000\n")
            if (not control.GetFont().Ok()):
                control.SetFont(wxNORMAL_FONT)
            file.write(repr(control.GetFont().GetPointSize()) + "\n")
            file.write(repr(control.GetFont().GetFamily()) + "\n")
            file.write(repr(control.GetFont().GetStyle()) + "\n")
            file.write(repr(control.GetFont().GetWeight()) + "\n")
            file.write(repr(control.GetFont().GetUnderlined()) + "\n")
            file.write(control.GetFont().GetFaceName() + "\n")
            if (isinstance(control, mSlider)):
                file.write(repr(control.GetMin()) + "\n")
                file.write(repr(control.GetMax()) + "\n")
                file.write(repr(control.getDirection()) + "\n")
            file.write(ENDCONTROLMARKER + "\n")
    def readControls(self, list):
        newList = []
        for i in range(0, list.count(ENDCONTROLMARKER)):
            params = list[0:list.index(ENDCONTROLMARKER)]
            type = atoi(params[0])
            label = params[1]
            pos = (atoi(params[2]), atoi(params[3]))
            size = (atoi(params[4]), atoi(params[5]))
            sSym = params[6]
            rSym = params[7]
            bColor = params[8]
            fColor = params[9]
            try:
                fontSize = atoi(params[10])
                fontFam = atoi(params[11])
                fontStyle = atoi(params[12])
                fontWeight = atoi(params[13])
                fontULine = atoi(params[14])
                fontFace = params[15]
            except:
                print "Font Error!!!"
            if (type == MVSLIDERTYPE or \
                type == MHSLIDERTYPE or \
                type == MVGAUGETYPE or \
                type == MHGAUGETYPE):
                min = atoi(params[16])
                max = atoi(params[17])
                dir = atoi(params[18])
                label = [label, min, max, dir]
            if (type == MGRAPHTYPE):
                min = atof(params[16])
                max = atof(params[17])
                dir = eval(params[18])
                label = [label, min, max, dir]
            control = self.addControl(type, label, pos, sSym, rSym)
            control.setSize(size)
            control.SetBackgroundColour(bColor)
            control.SetForegroundColour(fColor)
            font = wxFont(fontSize, fontFam, fontStyle, fontWeight,
                          fontULine, fontFace, wxFONTENCODING_SYSTEM)
            control.SetFont(font)
            control.setEditMode(self.editMode)
            # +1 to include ENDCONTROLMARKER
            del list[0:(list.index(ENDCONTROLMARKER) + 1)]
            newList.append(control)
        return newList
    def connect(self):
        if (not self.locked):
            self.mainFrame.fileMenu.Enable(ID_DISCONNECT, True)        
        if (not self.cTimer.IsRunning()):
            self.cTimer.timeCount = 0  
            self.cTimer.Start(CONNECTINTERVAL)
    def disconnect(self):
        if (self.sTimer.IsRunning()):
            self.sTimer.Stop() 
            try:
                self.connection.send(CLOSECOMMAND + SYMMSGSEP + \
                                     "0" + PAIRSEPCHAR)
            except:
                pass
        self.connection.close()
        self.connection = nullConnection()
        for control in self.controlList:
            control.connection = self.connection
        self.cTimer.Stop()
        self.pTimer.Stop()
        self.pingCount = 0
        if (not self.locked):
            self.mainFrame.fileMenu.Enable(ID_CONNECT, True)
            self.mainFrame.fileMenu.Enable(ID_DISCONNECT, False)
            self.mainFrame.connectIcon.SetBitmap(self.connectImageX) 
        self.connected = False
    def saveToDisk(self, filename):
        try:
            if (lower(filename[len(filename) - 4:len(filename)]) != ".gpd"):
                filename = filename + ".gpd"
            file = open(filename, 'w', -1)
            self.filepath = getDirectory(filename)
            self.filepath = makeAbsolutePath(self.path, self.filepath)
            if (self.filepath == ""):
                self.filepath = self.path
            file.write(repr(self.mainFrame.GetSize()[0]) + "\n")
            file.write(repr(self.mainFrame.GetSize()[1]) + "\n")
            file.write("#" + colorTuple2HexString((self.mainFrame.mainPanel.\
                                        GetBackgroundColour().Red(),
                                        self.mainFrame.mainPanel.\
                                        GetBackgroundColour().Green(),
                                        self.mainFrame.mainPanel.\
                                        GetBackgroundColour().Blue())) + "\n")
            file.write(self.host + "\n")
            file.write(repr(self.port) + FILETOKENSEP +
                       self.joyDevice1 + FILETOKENSEP +
                       self.joyDevice2 + FILETOKENSEP +
                       repr(self.pollTime) + FILETOKENSEP +
                       repr(self.joyPollTime) + FILETOKENSEP + 
                       repr(self.joystickEnabled) + FILETOKENSEP +
                       repr(self.sendKeys) + FILETOKENSEP +
                       self.frameTitle + FILETOKENSEP +
                       repr(self.alwaysOnTop) + FILETOKENSEP + 
                       repr(self.showGrid) + FILETOKENSEP + 
                       repr(self.snapToGrid) + FILETOKENSEP + 
                       repr(self.midiDevice1) + FILETOKENSEP +
                       repr(self.midiDevice2) + FILETOKENSEP +
                       repr(self.midiPollTime) + FILETOKENSEP +
                       repr(self.midiEnabled) + "\n")
            self.writeControls(self.controlList, file)
            file.close()
            self.edited = False
        except Exception:
            strerror = str(sys.exc_info()[1])
            dlg = wxMessageDialog(self.mainFrame, "Unable to save to " + \
                                  filename + ":\n\n" + strerror,
                                  "File Error", wxOK)
            dlg.ShowModal()
            dlg.Destroy()
    def openFromDisk(self, filename):
        try:
            file = open(filename, 'r', -1)
            try:
                self.filepath = getDirectory(filename)
                self.filepath = makeAbsolutePath(self.path, self.filepath)
                fullFileList = file.readlines()
                for control in self.controlList:
                    control.mDestroy()
                self.controlList = []
                self.selectedControlList = []
                self.resetControlIndecies()
                # remove line endings (need Perl's chomp())
                for i in range(0, len(fullFileList)):
                    fullFileList[i] = replace(fullFileList[i], "\n", "")
                frameSize = (atoi(fullFileList[0]), atoi(fullFileList[1]))
                panelColor = (fullFileList[2])
                panelHost = (fullFileList[3])
                otherArgs = split(fullFileList[4], FILETOKENSEP)
                if (len(otherArgs) > 0):
                    panelPort = otherArgs[0]
                if (len(otherArgs) > 1):
                    jDev1 = otherArgs[1]
                else:
                    jDev1 = DEFJOYDEVICE1
                if (len(otherArgs) > 2):
                    jDev2 = otherArgs[2]
                else:
                    jDev2 = DEFJOYDEVICE2
                if (len(otherArgs) > 3):
                    sockPollTime = atoi(otherArgs[3])
                else:
                    sockPollTime = DEFSOCKETTIMERTIME
                if (len(otherArgs) > 4):
                    joyPollTime = atoi(otherArgs[4])
                else:
                    joyPollTime = DEFJOYPOLLTIME
                if (len(otherArgs) > 5 and self.joystickAvailable):
                    self.joystickEnabled = atoi(otherArgs[5])
                    if (self.joystickEnabled):
                        self.startJoystick()
                    else:
                        self.stopJoystick()
                    if (not self.locked):
                        self.mainFrame.optionsMenu.Check(ID_ENABLEJOY,
                                                         self.joystickEnabled)
                if (len(otherArgs) > 6):
                    self.sendKeys = atoi(otherArgs[6])
                    if (not self.locked):
                        self.mainFrame.optionsMenu.Check(ID_ENABLEKEY,
                                                         self.sendKeys)
                if (len(otherArgs) > 7):
                    self.setFrameTitle(otherArgs[7])
                if (len(otherArgs) > 8):
                    self.alwaysOnTop = atoi(otherArgs[8])
                if (len(otherArgs) > 9):
                    self.showGrid = atoi(otherArgs[9])
                if (len(otherArgs) > 10):
                    self.snapToGrid = atoi(otherArgs[10])
                if (len(otherArgs) > 11):
                    self.midiDevice1 = atoi(otherArgs[11])
                if (len(otherArgs) > 12):
                    self.midiDevice2 = atoi(otherArgs[12])
                if (len(otherArgs) > 13):
                    self.midiPollTime = atoi(otherArgs[13])
                if (len(otherArgs) > 14 and self.midiAvailable):
                    self.midiEnabled = atoi(otherArgs[14])
                    if (self.midiEnabled):
                        self.startMidi()
                    else:
                        self.stopMidi()
                    if (not self.locked):
                        self.mainFrame.optionsMenu.Check(ID_ENABLEMIDI,
                                                         self.midiEnabled)

                self.mainFrame.SetSize(frameSize)
                self.mainFrame.mainPanel.SetBackgroundColour(panelColor)
                self.host = panelHost
                self.port = atoi(panelPort)
                self.joyDevice1 = jDev1
                self.joyDevice2 = jDev2
                self.pollTime = sockPollTime
                self.joyPollTime = joyPollTime
                del fullFileList[0:5]
                self.readControls(fullFileList)
                self.edited = False
                self.recreateFrame()
                self.mainFrame.mainPanel.Refresh()
            except Exception:
                strerror = "File is invalid"
                #FIXME
                strerror =  str(sys.exc_info()[1])
                dlg = wxMessageDialog(self.mainFrame, "Unable to open file " + \
                                      filename + ":\n\n" + strerror,
                                      "File Error", wxOK)
                dlg.ShowModal()
                dlg.Destroy()
        except Exception:
            strerror = str(sys.exc_info()[1])
            dlg = wxMessageDialog(self.mainFrame, "Unable to open file " + \
                                  filename + ":\n\n" + strerror,
                                  "File Error", wxOK)
            dlg.ShowModal()
            dlg.Destroy()
        self.mainFrame.positionIcons()
        self.setEditMode(False)
    def resetControlIndecies(self):
        self.buttonIndex = 0
        self.toggleIndex = 0
        self.sliderIndex = 0
        self.textIndex = 0
        self.rectIndex = 0
        self.gaugeIndex = 0
        self.checkBoxIndex = 0
        self.textBoxIndex = 0
        self.spinButtonIndex = 0
        self.mouseAreaIndex = 0
        self.radioButtonIndex = 0
        self.imageIndex = 0
        self.graphIndex = 0
    def startJoystick(self):
        self.joystickEnabled = True
        self.joyID1 = joystick.openDevice(self.joyDevice1)
        self.joyID2 = joystick.openDevice(self.joyDevice2)        
        if ((self.joyID1 > -1) or (self.joyID2 > -1)):
            self.joyTimer.Start(self.joyPollTime)
        if (self.joyID1 == -1):
            self.jsOn1 = False
            if (not self.locked):
                self.mainFrame.jsIcon1.SetBitmap(self.jsImageX)
        else:
            self.jsOn1 = True
            if (not self.locked):
                self.mainFrame.jsIcon1.SetBitmap(self.jsImage)
        if (self.joyID2 == -1):
            self.jsOn2 = False
            if (not self.locked):
                self.mainFrame.jsIcon2.SetBitmap(self.jsImageX)
        else:
            self.jsOn2 = True
            if (not self.locked):
                self.mainFrame.jsIcon2.SetBitmap(self.jsImage)
        if (not self.locked):
            self.mainFrame.positionIcons()
            self.mainFrame.jsIcon1.Show(not self.locked)
            self.mainFrame.jsIcon1.Refresh()
            self.mainFrame.jsIcon2.Show(not self.locked)
            self.mainFrame.jsIcon2.Refresh()
    def stopJoystick(self):
        self.joyTimer.Stop()
        joystick.closeDevice(self.joyID1)
        joystick.closeDevice(self.joyID2)
        self.joystickEnabled = False
        self.jsOn1 = False
        self.jsOn2 = False
        if (not self.locked):
            self.mainFrame.jsIconFlasher1.Stop()
            self.mainFrame.jsIconFlasher2.Stop()
            self.mainFrame.positionIcons()
            self.mainFrame.jsIcon1.Show(False)
            self.mainFrame.jsIcon2.Show(False)
    def showJoystickActive(self, jsNum):
        if (self.joystickEnabled and not self.locked):
            if (jsNum == 1):
                self.mainFrame.jsIconFlasher1.Start(250)
            else:
                self.mainFrame.jsIconFlasher2.Start(250)
    def startMidi(self):
        self.midiEnabled = True
        self.midiID1 = midi.openDevice(self.midiDevice1)
        self.midiID2 = midi.openDevice(self.midiDevice2)
        if ((self.midiID1 > -1) or (self.midiID2 > -1)):
            self.midiTimer.Start(self.midiPollTime)
        if (self.midiID1 == -1):
            self.midiOn1 = False
            if (not self.locked):
                self.mainFrame.midiIcon1.SetBitmap(self.midiImageX)
        else:
            self.midiOn1 = True
            if (not self.locked):
                self.mainFrame.midiIcon1.SetBitmap(self.midiImage)
        if (self.midiID2 == -1):
            self.midiOn2 = False
            if (not self.locked):
                self.mainFrame.midiIcon2.SetBitmap(self.midiImageX)
        else:
            self.midiOn2 = True
            if (not self.locked):
                self.mainFrame.midiIcon2.SetBitmap(self.midiImage)
        if (not self.locked):
            self.mainFrame.positionIcons()
            self.mainFrame.midiIcon1.Show(not self.locked)
            self.mainFrame.midiIcon1.Refresh()
            self.mainFrame.midiIcon2.Show(not self.locked)
            self.mainFrame.midiIcon2.Refresh()
    def stopMidi(self):
        self.midiTimer.Stop()
        midi.closeDevice(self.midiID1)
        midi.closeDevice(self.midiID2)
        self.midiEnabled = False
        self.midiOn1 = False
        self.midiOn2 = False
        if (not self.locked):
            self.mainFrame.midiIconFlasher1.Stop()
            self.mainFrame.midiIconFlasher2.Stop()
            self.mainFrame.positionIcons()
            self.mainFrame.midiIcon1.Show(False)
            self.mainFrame.midiIcon2.Show(False)
    def showMidiActive(self, midiNum):
        if (self.midiEnabled and not self.locked):
            if (midiNum == 1):
                self.mainFrame.midiIconFlasher1.Start(250)
            else:
                self.mainFrame.midiIconFlasher2.Start(250)
    def sendCharDown(self, charCode):
        self.sendChar(charCode, 1)
    def sendCharUp(self, charCode):
        self.sendChar(charCode, 0)
    def sendChar(self, charCode, downOrUp):
        if (self.sendKeys):
            self.connection.send("keystroke" +
                                 SYMMSGSEP +
                                 repr(charCode) +
                                 " " +
                                 repr(downOrUp) +
                                 PAIRSEPCHAR)
    def lock(self):
        if (not self.locked):
            if (self.editMode):
                self.setEditMode(False)
            self.locked = True
            self.recreateFrame()
    def unlock(self):
        if (self.locked):
            self.locked = False
            self.recreateFrame()
    def setEditMode(self, value):
        self.editMode = value
        self.resizable = value
        self.mainFrame.SetCursor(wxHOURGLASS_CURSOR)
        self.mainFrame.mainPanel.SetCursor(wxHOURGLASS_CURSOR)
        try:
            wxYield()
        except:
            pass
        self.recreateFrame()
        if (value):
            EVT_PAINT(self.mainFrame.mainPanel, self.ePaint)
            EVT_LEFT_DOWN(self.mainFrame.mainPanel, self.eLeftDown)
            EVT_LEFT_UP(self.mainFrame.mainPanel, self.eLeftUp)
            EVT_RIGHT_DOWN(self.mainFrame.mainPanel, self.eRightPopUp)
            for control in self.controlList:
                control.setEditMode(True)
            self.editMode = True
            if (not self.locked):
                self.mainFrame.editMenu.Enable(ID_ADD, True)
                self.mainFrame.editMenu.Enable(ID_EDIT, True)
                self.mainFrame.editMenu.Enable(ID_DELETE, True)
                self.mainFrame.editMenu.Enable(ID_SELECTALL, True)
                self.mainFrame.editMenu.Enable(ID_DUPLICATE, True)
                self.mainFrame.editMenu.Enable(ID_ALIGNVERT, True)
                self.mainFrame.editMenu.Enable(ID_ALIGNHORZ, True)
            self.mainFrame.setEditModeText("Edit Mode")
            self.mainFrame.positionIcons()
            self.selectedControlList = []
            self.edited = True
            if (self.showGrid):
                self.mainFrame.mainPanel.Refresh() 
        else:
            self.mainFrame.mainPanel.Disconnect(-1, wxEVT_PAINT)
            self.mainFrame.mainPanel.Disconnect(-1, wxEVT_LEFT_DOWN)
            self.mainFrame.mainPanel.Disconnect(-1, wxEVT_LEFT_UP)
            self.mainFrame.mainPanel.Disconnect(-1, wxEVT_RIGHT_DOWN)
            for control in self.controlList:
                control.setEditMode(False)
            self.editMode = False
            if (not self.locked):
                self.mainFrame.editMenu.Enable(ID_ADD, False)
                self.mainFrame.editMenu.Enable(ID_EDIT, False)
                self.mainFrame.editMenu.Enable(ID_DELETE, False)
                self.mainFrame.editMenu.Enable(ID_SELECTALL, False)
                self.mainFrame.editMenu.Enable(ID_DUPLICATE, False)
                self.mainFrame.editMenu.Enable(ID_ALIGNVERT, False)
                self.mainFrame.editMenu.Enable(ID_ALIGNHORZ, False)
            self.mainFrame.setEditModeText("Performance Mode")
            self.mainFrame.positionIcons()
        for control in self.controlList:
            control.Refresh()
        self.mainFrame.SetCursor(wxSTANDARD_CURSOR)
        self.mainFrame.mainPanel.SetCursor(wxSTANDARD_CURSOR)
        self.mainFrame.mainPanel.SetFocus()
    def recreateFrame(self):
        position = self.mainFrame.GetPosition()
        newWindow = gripdFrame(self,
                               -1,
                               self.frameTitle,
                               position,
                               self.resizable,
                               self.locked,
                               self.alwaysOnTop)
        shown = self.mainFrame.IsShown()
        self.copyFrame(newWindow)
        if (shown):
            self.mainFrame.Show(True)
        try:
            wxYield()
        except:
            pass
        self.setFrameTitle(self.frameTitle)
    def copyFrame(self, newWindow):
        newWindow.SetSize(self.mainFrame.GetSize())
        if (not self.locked):
            newWindow.editMenu.Check(ID_EDITMODE,
                                     self.editMode)
            newWindow.optionsMenu.Check(ID_ENABLEJOY,
                                        self.joystickEnabled)
            newWindow.optionsMenu.Check(ID_ENABLEMIDI,
                                        self.midiEnabled)
            newWindow.optionsMenu.Check(ID_ENABLEKEY,
                                        self.sendKeys)
            newWindow.positionIcons()
            newWindow.jsIcon1.Show(self.joystickEnabled)
            newWindow.jsIcon2.Show(self.joystickEnabled)
            newWindow.midiIcon1.Show(self.midiEnabled)
            newWindow.midiIcon2.Show(self.midiEnabled)
        if (self.jsOn1 and not self.locked):
            newWindow.jsIcon1.SetBitmap(self.jsImage)
        elif (not self.locked):
            newWindow.jsIcon1.SetBitmap(self.jsImageX)
        if (self.jsOn2 and not self.locked):
            newWindow.jsIcon2.SetBitmap(self.jsImage)
        elif (not self.locked):
            newWindow.jsIcon2.SetBitmap(self.jsImageX)
        if (self.midiOn1 and not self.locked):
            newWindow.midiIcon1.SetBitmap(self.midiImage)
        elif (not self.locked):
            newWindow.midiIcon1.SetBitmap(self.midiImageX)
        if (self.midiOn2 and not self.locked):
            newWindow.midiIcon2.SetBitmap(self.midiImage)
        elif (not self.locked):
            newWindow.midiIcon2.SetBitmap(self.midiImageX)
        newWindow.mainPanel.SetBackgroundColour(\
            self.mainFrame.mainPanel.GetBackgroundColour())
        self.mainFrame.Destroy()
        if (self.connected and not self.locked):
            newWindow.fileMenu.Enable(ID_CONNECT, False)
            newWindow.fileMenu.Enable(ID_DISCONNECT, True)
            newWindow.connectIcon.SetBitmap(self.connectImage)
        elif (not self.locked):
            newWindow.fileMenu.Enable(ID_CONNECT, True)
            newWindow.fileMenu.Enable(ID_DISCONNECT, False)
            newWindow.connectIcon.SetBitmap(self.connectImageX)
        self.SetTopWindow(newWindow)
        self.mainFrame = newWindow
        EVT_KEY_UP(self.mainFrame.mainPanel, self.eKeyUp)
        EVT_KEY_DOWN(self.mainFrame.mainPanel, self.eKeyDown)
        EVT_CHAR(self.mainFrame.mainPanel, self.eChar)
        if (not os.name == "posix"):
            EVT_CHAR_HOOK(self.mainFrame, self.eChar)
        EVT_CLOSE(self.mainFrame, self.eClose)
        EVT_SIZE(self.mainFrame, self.eSize)
        list = []
        container = duplicationContainer()
        for control in self.controlList:
            list.append(control)
        self.controlList = []
        self.writeControls(list, container)
        container.chomp()
        self.readControls(container.getList())
    def setFrameTitle(self, value):
        self.frameTitle = value
        if (self.editMode):
            self.mainFrame.SetTitle(value + "  [ Edit ]")
        else:
            self.mainFrame.SetTitle(value)
    def pingTimeout(self):
        if (self.openAuto):
            debugLog("timeout")
            self.disconnect()
            self.close()
        else:
            self.disconnect()
    def getNearestGridPoint(self, point):
        x = self.gridSpace * round((float(point[0]) / self.gridSpace))
        y = self.gridSpace * round((float(point[1]) / self.gridSpace))
        gridPoint = (int(x), int(y))
        return gridPoint
    def close(self):
        self.openAuto = False
        if ((self.edited) and (len(self.controlList) > 0)):
            self.mainFrame.Show(True)
            dlg = wxMessageDialog(self.mainFrame,
                                  "Current GUI not saved. Close anyway?",
                                  "GUI Not Saved", wxOK | wxCANCEL)
            if (dlg.ShowModal() == wxID_OK):
                dlg.Destroy()
                self.mainFrame.Destroy()
                try:
                    self.disconnect()
                except:
                    pass
            else:
                return
        else:
            self.mainFrame.Show(True)
            self.mainFrame.Destroy()
            try:
                self.disconnect()
            except:
                pass
        if (self.joystickEnabled):
            self.stopJoystick()
        if (self.midiEnabled):
            self.stopMidi()
        self.ExitMainLoop()        

## Events ##

    def eOpenConnection(self, event):
        cString = "Connect to " + self.host + ":" + repr(self.port) + "\n "
        dlg = wxMessageDialog(self.mainFrame.mainPanel, cString, "Connect",
                              wxOK | wxCANCEL)
        if (dlg.ShowModal() == wxID_OK):
            self.connect()
        dlg.Destroy()
    def eCloseConnection(self, event):
        self.disconnect()
    def eToggleJoystick(self, event):
        if (not self.locked):
            self.joystickEnabled = self.mainFrame.optionsMenu.\
                                   IsChecked(ID_ENABLEJOY)
        if (self.joystickEnabled):
            self.startJoystick()
        else:
            self.stopJoystick()
    def eToggleMidi(self, event):
        if (not self.locked):
            self.midiEnabled = self.mainFrame.optionsMenu.\
                                   IsChecked(ID_ENABLEMIDI)
        if (self.midiEnabled):
            self.startMidi()
        else:
            self.stopMidi()
    def eToggleKeySend(self, event):
        if (not self.locked):
            self.sendKeys = self.mainFrame.optionsMenu.IsChecked(ID_ENABLEKEY)
    def eToggleAlwaysOnTop(self, event):
        if (not self.locked and os.name != "posix"):
           self.alwaysOnTop = self.mainFrame.optionsMenu.\
                                    IsChecked(ID_ALWAYSONTOP)
        self.recreateFrame()
    def eToggleShowGrid(self, event):
        self.showGrid = self.mainFrame.optionsMenu.IsChecked(ID_SHOWGRID)
        if (not self.showGrid):
            self.snapToGrid = False
            if (not self.locked):
                self.mainFrame.optionsMenu.Check(ID_SNAPTOGRID, False)
        else:
            if (self.editMode):
                self.mainFrame.mainPanel.Refresh()
        self.mainFrame.mainPanel.Refresh()
    def eToggleSnapToGrid(self, event):
        self.snapToGrid = self.mainFrame.optionsMenu.IsChecked(ID_SNAPTOGRID)
        if (self.snapToGrid):
            self.showGrid = True
            if (not self.locked):
                self.mainFrame.optionsMenu.Check(ID_SHOWGRID, True)
            if (self.editMode):
                self.mainFrame.mainPanel.Refresh()
    def eRightPopUp(self, event):
        if ((not self.locked) and (not self.selecting)):
            self.mainFrame.PopupMenuXY(self.mainFrame.editMenu,
                                       event.GetX(),
                                       event.GetY())
    def eLeftDown(self, event):
        # Editing stuff (mostly for selectRect)
        if (self.editMode):
            EVT_MOTION(self.mainFrame.mainPanel, self.eMotion)
            self.mouseDownPos = event.GetPosition()
            self.mainFrame.mainPanel.CaptureMouse()
            self.leftDownOnPanel = True
            if (not event.m_controlDown):
                self.deselectOthers(-1)
    def eLeftUp(self, event):
        if (self.editMode):
            if (self.leftDownOnPanel):
                self.mainFrame.mainPanel.ReleaseMouse()
            self.mainFrame.mainPanel.Disconnect(-1, wxEVT_MOTION)
            if (self.selecting):
                self.selecting = False
                for control in self.controlList:
                    if (selectIntersect(control.GetRect(),
                                        self.mainFrame.selectRect)):
                        control.select()
                    elif ((not event.m_controlDown) and control.isSelected()):
                        control.deselect()
                self.mainFrame.mainPanel.Refresh()
            self.leftDownOnPanel = False
    def eMotion(self, event):
        if (event.LeftIsDown() and self.editMode and self.leftDownOnPanel):
            self.selecting = True
            width = abs(event.GetPosition()[0] - self.mouseDownPos[0])
            height = abs(event.GetPosition()[1] - self.mouseDownPos[1])
            leftBound = min(self.mouseDownPos[0], event.GetPosition()[0])
            topBound = min(self.mouseDownPos[1], event.GetPosition()[1])
            clearRegion = wxRegion(self.mainFrame.selectRect[0],
                                   self.mainFrame.selectRect[1],
                                   self.mainFrame.selectRect[2],
                                   self.mainFrame.selectRect[3])
            self.mainFrame.selectRect = [leftBound, topBound,
                                         width, height]
            clearRegion.Union(leftBound, topBound, width, height)
            self.mainFrame.mainPanel.Refresh(False, clearRegion.GetBox())
    def ePaint(self, event):
        event.Skip()
        myDC = wxPaintDC(self.mainFrame.mainPanel)
        myDC.BeginDrawing()
        if (self.editMode):
            if (self.showGrid):
                self.mainFrame.drawGrid(myDC)
            else:
                self.mainFrame.drawCleanBackground(myDC)
        if (self.selecting):
            self.mainFrame.drawSelectRect(myDC)
        myDC.EndDrawing()
    def eAddButton(self, event):
        self.createControl(MBUTTONTYPE, self.buttonIndex, "button")
    def eAddToggle(self, event):
        self.createControl(MTOGGLETYPE, self.toggleIndex, "toggle")
    def eAddVSlider(self, event):
        self.createControl(MVSLIDERTYPE, self.sliderIndex, "slider")
    def eAddHSlider(self, event):
        self.createControl(MHSLIDERTYPE, self.sliderIndex, "slider")
    def eAddRect(self, event):
        self.createControl(MRECTTYPE, self.rectIndex, "rectangle")
    def eAddText(self, event):
        self.createControl(MTEXTTYPE, self.textIndex, "text")
    def eAddVGauge(self, event):
        self.createControl(MVGAUGETYPE, self.gaugeIndex, "gauge")
    def eAddHGauge(self, event):
        self.createControl(MHGAUGETYPE, self.gaugeIndex, "gauge")
    def eAddCheckBox(self, event):
        self.createControl(MCHECKBOXTYPE, self.checkBoxIndex, "checkbox")
    def eAddRadioButtons(self, event):
        if (self.editMode and not self.dragging):
            txtDialog = wxTextEntryDialog(self.mainFrame.mainPanel,
                                          "Number of radio buttons to add:",
                                          "Create Radio Buttons", "2")
            if (txtDialog.ShowModal() == wxID_OK):
                numRadButs = atoi(txtDialog.GetValue())
                txtDialog.Destroy()
                if (numRadButs > 1):
                    indexStr = "radiobutton" + repr(self.radioButtonIndex)
                    pos = self.mainFrame.mainPanel.ScreenToClient( \
                                                       wxGetMousePosition())
                    controlN = self.addControl(MRADIONEWTYPE,
                                               indexStr,
                                               pos,
                                               "s" + indexStr,
                                               "r" + indexStr)
                    coords = pos
                    for i in range(1, numRadButs):
                        indexStr = "radiobutton" + repr(self.radioButtonIndex)
                        coords = [coords[0], coords[1] + 25]
                        maxPos = self.mainFrame.mainPanel.GetSize()[1] - 10
                        minPos = 0
                        if (coords[1] > maxPos):
                            coords[0] = pos[0] + 40
                            coords[1] = pos[1]
                        control = self.addControl(MRADIOBUTTONTYPE,
                                                  indexStr,
                                                  coords,
                                                  "s" + indexStr,
                                                  "r" + indexStr)
                        control.select()
                    controlN.select()
                    try:
                        wxYield()
                    except:
                        pass
                    controlN.grab()
    def eAddTextBox(self, event):
        self.createControl(MTEXTBOXTYPE, self.textBoxIndex, "textbox")
    def eAddSpinButton(self, event):
        self.createControl(MSPINBUTTONTYPE, self.spinButtonIndex, "spinbutton")
    def eAddMouseArea(self, event):
        self.createControl(MMOUSEAREATYPE, self.mouseAreaIndex, "mousearea")
    def eAddImage(self, event):
            if (self.editMode and not self.dragging):
                dlg = wxFileDialog(self.mainFrame, "Open file", self.filepath,
                                   "", "*.*", wxOPEN)
            if (dlg.ShowModal() == wxID_OK):
                filepath = dlg.GetPath()
                self.createControl(MIMAGETYPE,
                                   self.imageIndex,
                                   "image",
                                   filepath)
            dlg.Destroy()
    def eAddGraph(self, event):
        self.createControl(MGRAPHTYPE, self.graphIndex, "graph")
    def eEditMode(self, event):
        if (not self.locked):
            newEditMode = self.mainFrame.editMenu.IsChecked(ID_EDITMODE)
            self.setEditMode(newEditMode)
    def eEditControl(self, event):
        self.edited = True
        if (len(self.selectedControlList) > 0):
            self.selectedControlList[0].eEdit(event)
    # extra array needed because we're removing items from the array
    # we are cycling through (selectedControlList)
    def eDeleteControl(self, event):
        deleteList = []
        for control in self.selectedControlList:
            deleteList.append(control)
        for control in deleteList:
            self.controlList.remove(control)
            self.selectedControlList.remove(control)
            control.mDestroy()
    def eSelectAll(self, event):
        for control in self.controlList:
            control.select()
    def eDuplicate(self, event):
        list = []
        container = duplicationContainer()
        # Not using selectedControlList since we are removing from it
        # as we cycle through the list
        for control in self.controlList:
            if (control.isSelected()):
                list.append(control)
                control.deselect()
        self.writeControls(list, container)
        container.chomp()
        for control in self.readControls(container.getList()):
            control.select()
            control.move(DUPPOSOFFSET)
    def eAlignVertical(self, event):
        self.alignControls(0)
    def eAlignHorizontal(self, event):
        self.alignControls(1)
    def eRefresh(self, event):
        for control in self.controlList:
            control.Refresh()
            if (control.isSelected()):
                control.relocateTags()
        self.mainFrame.mainPanel.Refresh(True)
    def eNew(self, event):
        flag = 0;
        if ((self.edited) and (len(self.controlList) > 0)):
            flag = 1
            dlg = wxMessageDialog(self.mainFrame,
                                  "Current GUI not saved. Clear anyway?",
                                  "GUI Not Saved", wxOK | wxCANCEL)
            if (dlg.ShowModal() == wxID_OK):
                flag = 0
        if (not flag):
            for control in self.controlList:
                control.mDestroy()
            self.controlList = []
            self.selectedControlList = []
            self.resetControlIndecies()
            self.mainFrame.mainPanel.SetBackgroundColour(\
                self.mainFrame.defBgColor)
            self.mainFrame.mainPanel.Refresh()
            self.filepath = ""
    def eOpen(self, event):
        if ((self.edited) and (len(self.controlList) > 0)):
            dlg2 = wxMessageDialog(self.mainFrame,
                                   "Current GUI not saved. Open anyway?",
                                   "GUI Not Saved", wxOK | wxCANCEL)
            if (dlg2.ShowModal() == wxID_OK):
                dlg = wxFileDialog(self.mainFrame,
                                   "Open file", self.filepath, "", "*.gpd",
                                   wxOPEN)
                if (dlg.ShowModal() == wxID_OK):
                    self.openFromDisk(dlg.GetPath())
                # FIXME
                # dlg.Destroy()
            # FIXME
            # dlg2.Destroy()
        else:
            dlg = wxFileDialog(self.mainFrame,
                               "Open file", self.filepath, "", "*.gpd",
                               wxOPEN)
            if (dlg.ShowModal() == wxID_OK):
                self.openFromDisk(dlg.GetPath())
            # FIXME
            # dlg.Destroy()
        self.mainFrame.mainPanel.Refresh()
    def eSave(self, event):
        dlg = wxFileDialog(self.mainFrame, "Save file", self.filepath,
                           "", "*.gpd", wxSAVE)
        if (dlg.ShowModal() == wxID_OK):
            filename = dlg.GetPath()
            try:
                file = open(filename, 'r', -1)
                file.close()
                dlg2 = wxMessageDialog(self.mainFrame, "File " + filename \
                                       + " already exists. Overwrite?",
                                       "File Exists", wxOK | wxCANCEL)
                if (dlg2.ShowModal() == wxID_OK):
                    self.saveToDisk(filename)
                    dlg2.Destroy()
            except:
                self.saveToDisk(filename)
        dlg.Destroy()
    def eSetOptions(self, event):
        dlg = optionsDialog(self)
        dlg.ShowModal()
        self.host = dlg.addrBox.GetValue()
        try:
            self.port = atoi(dlg.portBox.GetValue())
        except:
            errDlg = wxMessageDialog(self.mainFrame,
                                  "Invalid Port",
                                  "Options Error",
                                  wxOK)
            errDlg.ShowModal()
            errDlg.Destroy()
        try:
            self.pollTime = atoi(dlg.pollBox.GetValue())
        except:
            errDlg = wxMessageDialog(self.mainFrame,
                                  "Invalid Socket Poll Time",
                                  "Options Error",
                                  wxOK)
            errDlg.ShowModal()
            errDlg.Destroy()
        self.joyDevice1 = dlg.joyDevBox1.GetValue()
        self.joyDevice2 = dlg.joyDevBox2.GetValue()
        try:
            self.joyPollTime = atoi(dlg.joyPollBox.GetValue())
        except:
            errDlg = wxMessageDialog(self.mainFrame,
                                  "Invalid Joystick Poll Time",
                                  "Options Error",
                                  wxOK)
            errDlg.ShowModal()
            errDlg.Destroy()
        try:
            self.midiDevice1 = atoi(dlg.midiDevBox1.GetValue())
        except:
            errDlg = wxMessageDialog(self.mainFrame,
                            "Invalid MIDI Device 0\nShould be an integer.",
                            "Options Error",
                            wxOK)
            errDlg.ShowModal()
            errDlg.Destroy()
        try:
            self.midiDevice2 = atoi(dlg.midiDevBox2.GetValue())
        except:
            errDlg = wxMessageDialog(self.mainFrame,
                            "Invalid MIDI Device 1\nShould be an integer.",
                            "Options Error",
                            wxOK)
            errDlg.ShowModal()
            errDlg.Destroy()
        try:
            self.midiPollTime = atoi(dlg.midiPollBox.GetValue())
        except:
            errDlg = wxMessageDialog(self.mainFrame,
                            "Invalid MIDI Poll Time",
                            "Options Error",
                            wxOK)
            errDlg.ShowModal()
            errDlg.Destroy()
        self.setFrameTitle(dlg.titleBox.GetValue())
        if (self.sTimer.IsRunning):
            self.sTimer.Stop()
            self.sTimer.Start(self.pollTime)
        if (not self.locked):
            if (self.mainFrame.optionsMenu.IsChecked(ID_ENABLEJOY)):
                self.stopJoystick()
                self.startJoystick()
            if (self.mainFrame.optionsMenu.IsChecked(ID_ENABLEMIDI)):
                self.stopMidi()
                self.startMidi()
        dlg.Destroy()
        self.mainFrame.Refresh(True)
        self.mainFrame.mainPanel.Refresh(True)
        for control in self.controlList:
            control.Refresh()
    def eAbout(self, event):
        aboutString = "GrIPD v" + VERSION + ": "
        aboutString = aboutString \
                      + "Graphical Interface for Pure Data\n\n"
        aboutString = aboutString \
                      + "(C) Copyright 2003 Joseph A. Sarlo\nGNU "
        aboutString = aboutString \
                      + "General Public License\njsarlo@ucsd.edu"
        dlg = wxMessageDialog(self.mainFrame, aboutString , "About GrIPD",
                              wxOK | wxICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()
    def eQuit(self, event):
        self.mainFrame.Close(True)
    def eKeyDown(self, event):
        # already depressed
        try:
            self.keysDown.index(event.GetKeyCode())
        # first time
        except:
            self.keysDown.append(event.GetKeyCode())
            if (not self.editMode):
                self.sendCharDown(event.GetKeyCode())
        event.Skip()
    def eKeyUp(self, event):
        try:
            self.keysDown.remove(event.GetKeyCode())
            self.sendCharUp(event.GetKeyCode())
        except:
            pass
        event.Skip()
    def eChar(self, event):
        if (self.editMode and not self.dragging):
            if (event.m_controlDown):
                deltaPos = LARGECHARMOVE
            else:
                deltaPos = SMALLCHARMOVE
            if (event.GetKeyCode() == WXK_LEFT):
                self.moveKey((-deltaPos, 0))
                return
            if (event.GetKeyCode() == WXK_RIGHT):
                self.moveKey((deltaPos, 0))
                return
            if (event.GetKeyCode() == WXK_UP):
                self.moveKey((0, -deltaPos))
                return
            if (event.GetKeyCode() == WXK_DOWN):
                self.moveKey((0, deltaPos))
                return
            if (event.GetKeyCode() == WXK_TAB):
                self.traverseFocus(event.m_controlDown)
                return
        if (not self.editMode or (self.editMode and not self.dragging)):
            # Allow accelerators to be called
            event.Skip()
    def eRepaintControlTags(self, event):
        event.getControl().refreshTags()
    def eClose(self, event):
        if (self.openAuto and self.connected):
            self.mainFrame.Show(False)
            try:
                self.connection.send(HIDECOMMAND + SYMMSGSEP + \
                                     "0" + PAIRSEPCHAR)
            except:
                self.close()
        else:
            self.close()
    def eSize(self, event):
        self.mainFrame.positionIcons()
        event.Skip()
 
# Frame class
class gripdFrame(wxFrame):
# Setup
    def __init__(self, parent, ID, title, position,
                 resizable, locked, alwaysOnTop):
        self.mParent = parent
        self.resizable = resizable
        self.alwaysOnTop = alwaysOnTop
        self.locked = locked
        if (self.resizable):
            style = wxDEFAULT_FRAME_STYLE
        else:
            style = wxDEFAULT_FRAME_STYLE & (~wxRESIZE_BORDER) & \
                    (~wxMAXIMIZE_BOX)
        if (self.alwaysOnTop):
            style = style | wxSTAY_ON_TOP
        if (position == (-1, -1)):
            position = wxDefaultPosition
        wxFrame.__init__(self, NULL, ID, title, position,
                         DEFMAINFRAMESIZE,
                         style)
        if (not os.name == "posix"):
            self.SetIcon(self.mParent.icon)
        if (not self.locked):
            # Menu accelerator stuff
            aclList = []
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL,
                                              WXK_DELETE,
                                              ID_DELETE))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F1,
                                              ID_BUTTON))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F2,
                                              ID_TOGGLE))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F3,
                                              ID_SPINBUTTON))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F4,
                                              ID_RADIOBUTTON))        
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F5,
                                              ID_VSLIDER))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F6,
                                              ID_HSLIDER))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F7,
                                              ID_VGAUGE))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F8,
                                              ID_HGAUGE))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F9,
                                              ID_CHECKBOX))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F10,
                                              ID_TEXTBOX))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F11,
                                              ID_MOUSEAREA))
            aclList.append(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F12,
                                              ID_RECT))
            aclList.append(wxAcceleratorEntry(wxACCEL_ALT, ord("1"),
                                              ID_TEXT))
            aclList.append(wxAcceleratorEntry(wxACCEL_ALT, ord("2"),
                                              ID_IMAGE))
            self.SetAcceleratorTable(wxAcceleratorTable(aclList))           
            self.menuBar = wxMenuBar()
            self.SetMenuBar(self.menuBar)
            self.createMenu()
            self.CreateStatusBar(3)
            self.jsIcon1 = wxStaticBitmap(self.GetStatusBar(), -1,
                                          self.mParent.jsImageX, (0, 0),
                                          wxDefaultSize)
            self.jsIcon2 = wxStaticBitmap(self.GetStatusBar(), -1,
                                          self.mParent.jsImageX, (0, 0),
                                          wxDefaultSize)
            self.midiIcon1 = wxStaticBitmap(self.GetStatusBar(), -1,
                                          self.mParent.midiImageX, (0, 0),
                                          wxDefaultSize)
            self.midiIcon2 = wxStaticBitmap(self.GetStatusBar(), -1,
                                          self.mParent.midiImageX, (0, 0),
                                          wxDefaultSize)
            self.connectIcon = wxStaticBitmap(self.GetStatusBar(), 
                                              -1,
                                              self.mParent.connectImageX, 
                                              (0, 0),
                                              wxDefaultSize)
            # timers to flash icons
            self.jsIconFlasher1 = iconFlasher(self.jsIcon1, 
                                              self.mParent.jsImage,
                                              self.mParent.jsImageA,
                                              1)
            self.jsIconFlasher2 = iconFlasher(self.jsIcon2, 
                                              self.mParent.jsImage,
                                              self.mParent.jsImageA,
                                              1)
            self.midiIconFlasher1 = iconFlasher(self.midiIcon1, 
                                                self.mParent.midiImage,
                                                self.mParent.midiImageA,
                                                1)
            self.midiIconFlasher2 = iconFlasher(self.midiIcon2, 
                                                self.mParent.midiImage,
                                                self.mParent.midiImageA,
                                                1)
            self.editModeText = wxStaticText(self.GetStatusBar(), 
                                             -1,
                                             "Performance Mode",
                                             (0, 0),
                                             wxDefaultSize,
                                             wxALIGN_CENTRE | \
                                             wxST_NO_AUTORESIZE)
            self.editModeText.Show(True)
            self.jsIcon1.Show(False)
            self.jsIcon2.Show(False)
            self.midiIcon1.Show(False)
            self.midiIcon2.Show(False)
            self.connectIcon.Show(True)
        # left, top, width, height of selection rect (for drawing)
        self.selectRect = [0, 0, 0, 0]
        # mainPanel stuff
        self.mainPanel = wxPanel(self, -1, wxDefaultPosition,
                                 wxDefaultSize, 
                                 wxSTATIC_BORDER)
        self.defBgColor = wxColour(self.mainPanel.GetBackgroundColour().Red() \
                                   - BGCOLORDIF,
                                   self.mainPanel.GetBackgroundColour().Green() \
                                   - BGCOLORDIF,
                                   self.mainPanel.GetBackgroundColour().Blue() \
                                   - BGCOLORDIF)
        self.mainPanel.SetBackgroundColour(self.defBgColor)
        self.mainPanel.Show(True)
        if (self.locked):
            self.lock()
        else:
            self.unlock()
    def createMenu(self):
        self.fileMenu = wxMenu()
        self.fileMenu.Append(ID_NEW, "&New\tCtrl-N", "Clear current GUI")
        self.fileMenu.Append(ID_OPEN, "&Open\tCtrl-O", "Open a GUI")
        self.fileMenu.Append(ID_SAVE, "&Save\tCtrl-S", "Save current GUI")
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(ID_CONNECT, "&Connect\tAlt-C",
                             "Connect to PD")
        self.fileMenu.Append(ID_DISCONNECT, "&Disconnect\tAlt-D",
                             "Disconnect from PD")
        self.fileMenu.AppendSeparator()
        self.fileMenu.Append(ID_EXIT, "E&xit\tCtrl-Q", "Quit GrIPD")
        self.editMenu = wxMenu()
        self.editMenu.Append(ID_EDITMODE, "Edit &mode\tCtrl-E",
                             "Toggle Edit/Performance Modes", True)
        self.editMenu.AppendSeparator()
        self.addMenu = wxMenu()
        self.buttonMenu = wxMenu()
        self.buttonMenu.Append(ID_BUTTON, "&Push Button\tF1",
                               "Add a push button")
        self.buttonMenu.Append(ID_TOGGLE, "&Toggle Button\tF2",
                               "Add a toggle button")
        self.buttonMenu.Append(ID_SPINBUTTON, "&Spin Button\tF3",
                               "Add a spin button")
        self.buttonMenu.Append(ID_RADIOBUTTON, "&Radio Buttons\tF4",
                               "Add radio buttons")
        self.addMenu.AppendMenu(ID_BUTTONMENU, "&Button", self.buttonMenu);
        self.sliderMenu = wxMenu()
        self.sliderMenu.Append(ID_VSLIDER, "&Vertical Slider\tF5",
                               "Add a vertical slider")
        self.sliderMenu.Append(ID_HSLIDER, "&Horizontal Slider\tF6",
                               "Add a horizontal slider")
        self.addMenu.AppendMenu(ID_SLIDERMENU, "&Slider", self.sliderMenu);
        self.gaugeMenu = wxMenu()
        self.gaugeMenu.Append(ID_VGAUGE, "&Vertical Gauge\tF7",
                              "Add a vertical gauge")
        self.gaugeMenu.Append(ID_HGAUGE, "&Horizontal Gauge\tF8",
                              "Add a horizontal gauge")
        self.addMenu.AppendMenu(ID_GAUGEMENU, "&Gauge", self.gaugeMenu);
        self.addMenu.Append(ID_CHECKBOX, "&Checkbox\tF9",
                            "Add a labeld checkbox")
        self.addMenu.Append(ID_TEXTBOX, "Te&xtbox\tF10",
                            "Add a box for entering text")
        self.addMenu.Append(ID_MOUSEAREA, "&Mouse Area\tF11",
                            "Add a mouse capture area")
        self.addMenu.Append(ID_RECT, "&Rectangle\tF12",
                            "Add a boundary rectangle")
        self.addMenu.Append(ID_TEXT, "&Text\tAlt-1", "Add text")
        self.addMenu.Append(ID_IMAGE, "&Image\tAlt-2", "Add an image")
        self.addMenu.Append(ID_GRAPH, "&Graph\tAlt-3", "Add a graph")
        self.editMenu.AppendMenu(ID_ADD, "&Add", self.addMenu)
        self.editMenu.Append(ID_EDIT, "&Edit", "Edit selected controls")
        self.editMenu.Append(ID_DELETE, "&Delete", "Delete selected controls")
        self.editMenu.AppendSeparator()
        self.editMenu.Append(ID_SELECTALL, "&Select All\tCtrl-A",
                             "Select all controls")
        self.editMenu.Append(ID_DUPLICATE, "D&uplicate\tCtrl-D",
                             "Duplicate selected controls")
        self.editMenu.AppendSeparator()
        self.editMenu.Append(ID_ALIGNVERT, "Align &Vertical\tCtrl-V",
                            "Vertically align selected controls")
        self.editMenu.Append(ID_ALIGNHORZ, "Align &Horizontal\tCtrl-H",
                             "Horizontally align selected controls")
        self.editMenu.AppendSeparator()
        self.editMenu.Append(ID_REFRESH, "&Refresh\tCtrl-R", "Refresh screen")
        self.optionsMenu = wxMenu()
        if (os.name != "posix"):
            self.optionsMenu.Append(ID_ALWAYSONTOP, "Always On Top",
                                    "Window will stay above all other windows",
                                    True)
            self.optionsMenu.AppendSeparator()
        self.optionsMenu.Append(ID_ENABLEMIDI, "Enable &MIDI",
                                "Enables the use of MIDI input", 
                                True)
        self.optionsMenu.Append(ID_ENABLEJOY, "Enable &Joystick(s)",
                                "Enables the use of a joystick",
                                True)
        self.optionsMenu.Append(ID_ENABLEKEY, "Send &Keystrokes",
                                "Send all keystrokes to PD 'keystroke' symbol",
                                True)
        self.optionsMenu.AppendSeparator()
        self.optionsMenu.Append(ID_SHOWGRID, "Show Grid", 
                                "Show Edit Grid", True)
        self.optionsMenu.Append(ID_SNAPTOGRID, "Snap To Grid",
                                "Snap controls to edit grid", True)
        self.optionsMenu.AppendSeparator()
        self.optionsMenu.Append(ID_SETOPTS, "&Configure\tCtrl-C",
                                "Set GUI options")
        self.helpMenu = wxMenu()
        self.helpMenu.Append(ID_ABOUT, "&About",
                             "GrIPD: Graphical Interface for Pure Data")
        self.menuBar.Append(self.fileMenu, "&File")
        self.menuBar.Append(self.editMenu, "&Edit")
        self.menuBar.Append(self.optionsMenu, "&Options")
        self.menuBar.Append(self.helpMenu, "&Help")
        self.fileMenu.Enable(ID_DISCONNECT, False)
        self.editMenu.Enable(ID_ADD, False)
        self.editMenu.Enable(ID_EDIT, False)
        self.editMenu.Enable(ID_DELETE, False)
        self.editMenu.Enable(ID_SELECTALL, False)
        self.editMenu.Enable(ID_DUPLICATE, False)
        self.editMenu.Enable(ID_ALIGNVERT, False)
        self.editMenu.Enable(ID_ALIGNHORZ, False)
        if (self.mParent.joystickAvailable):
            self.optionsMenu.Check(ID_ENABLEJOY, self.mParent.joystickEnabled)
        if (self.mParent.midiAvailable):
            self.optionsMenu.Check(ID_ENABLEMIDI, self.mParent.midiEnabled)
        self.optionsMenu.Check(ID_ENABLEKEY, self.mParent.sendKeys)
        if (os.name != "posix"):
            self.optionsMenu.Check(ID_ALWAYSONTOP, self.alwaysOnTop)
        self.optionsMenu.Check(ID_SHOWGRID, self.mParent.showGrid)
        self.optionsMenu.Check(ID_SNAPTOGRID, self.mParent.snapToGrid)
        self.optionsMenu.Enable(ID_ENABLEJOY, 
                                self.mParent.joystickAvailable)
        self.optionsMenu.Enable(ID_ENABLEMIDI,
                                self.mParent.midiAvailable)
        EVT_MENU(self, ID_NEW, self.mParent.eNew)
        EVT_MENU(self, ID_ABOUT, self.mParent.eAbout)
        EVT_MENU(self, ID_EXIT,  self.mParent.eQuit)
        EVT_MENU(self, ID_CONNECT, self.mParent.eOpenConnection)
        EVT_MENU(self, ID_DISCONNECT, self.mParent.eCloseConnection)
        EVT_MENU(self, ID_OPEN, self.mParent.eOpen)
        EVT_MENU(self, ID_SAVE, self.mParent.eSave)
        EVT_MENU(self, ID_BUTTON, self.mParent.eAddButton)
        EVT_MENU(self, ID_TOGGLE, self.mParent.eAddToggle)
        EVT_MENU(self, ID_SPINBUTTON, self.mParent.eAddSpinButton)
        EVT_MENU(self, ID_VSLIDER, self.mParent.eAddVSlider)
        EVT_MENU(self, ID_HSLIDER, self.mParent.eAddHSlider)
        EVT_MENU(self, ID_RECT, self.mParent.eAddRect)
        EVT_MENU(self, ID_VGAUGE, self.mParent.eAddVGauge)
        EVT_MENU(self, ID_HGAUGE, self.mParent.eAddHGauge)
        EVT_MENU(self, ID_CHECKBOX, self.mParent.eAddCheckBox)
        EVT_MENU(self, ID_RADIOBUTTON, self.mParent.eAddRadioButtons)        
        EVT_MENU(self, ID_TEXTBOX, self.mParent.eAddTextBox)
        EVT_MENU(self, ID_TEXT, self.mParent.eAddText)
        EVT_MENU(self, ID_IMAGE, self.mParent.eAddImage)        
        EVT_MENU(self, ID_MOUSEAREA, self.mParent.eAddMouseArea)
        EVT_MENU(self, ID_GRAPH, self.mParent.eAddGraph)
        EVT_MENU(self, ID_EDITMODE, self.mParent.eEditMode)
        EVT_MENU(self, ID_EDIT, self.mParent.eEditControl)
        EVT_MENU(self, ID_DELETE, self.mParent.eDeleteControl)
        EVT_MENU(self, ID_SELECTALL, self.mParent.eSelectAll)
        EVT_MENU(self, ID_DUPLICATE, self.mParent.eDuplicate)
        EVT_MENU(self, ID_ALIGNVERT, self.mParent.eAlignVertical)
        EVT_MENU(self, ID_ALIGNHORZ, self.mParent.eAlignHorizontal)
        EVT_MENU(self, ID_REFRESH, self.mParent.eRefresh)
        EVT_MENU(self, ID_SETOPTS, self.mParent.eSetOptions)
        EVT_MENU(self, ID_ENABLEJOY, self.mParent.eToggleJoystick)
        EVT_MENU(self, ID_ENABLEMIDI, self.mParent.eToggleMidi)
        EVT_MENU(self, ID_ENABLEKEY, self.mParent.eToggleKeySend)
        if (os.name != "posix"):
            EVT_MENU(self, ID_ALWAYSONTOP, self.mParent.eToggleAlwaysOnTop)
        EVT_MENU(self, ID_SHOWGRID, self.mParent.eToggleShowGrid)
        EVT_MENU(self, ID_SNAPTOGRID, self.mParent.eToggleSnapToGrid)
    def createLockedMenu(self):
        self.fileMenu = wxMenu()
        self.fileMenu.Append(ID_EXIT, "E&xit\tCtrl-Q", "Quit GrIPD")
        self.helpMenu = wxMenu()
        self.helpMenu.Append(ID_ABOUT, "&About",
                             "GrIPD: Graphical Interface for Pure Data")
        self.menuBar.Append(self.fileMenu, "&File")
        self.menuBar.Append(self.helpMenu, "&Help")
        EVT_MENU(self, ID_ABOUT, self.mParent.eAbout)
        EVT_MENU(self, ID_EXIT,  self.mParent.eQuit)
    def removeMenu(self):
        for i in range(0, self.GetMenuBar().GetMenuCount()):
            self.GetMenuBar().Remove(0)
    def drawCleanBackground(self, myDC, rect = ()):
        box = self.mainPanel.GetUpdateRegion().GetBox()
        if (rect == ()):
            rect = tuple((box[0],
                          box[1],
                          box[0] + box[2],
                          box[1] + box[3]))
        myDC.SetPen(wxPen(self.mainPanel.GetBackgroundColour(), 
                          1, 
                          wxTRANSPARENT))
        myDC.SetBrush(wxBrush(self.mainPanel.GetBackgroundColour(), wxSOLID))
        myDC.DrawRectangle(rect[0],
                           rect[1],
                           rect[2],
                           rect[3])
    def drawGrid(self, myDC, rect = ()):
        box = self.mainPanel.GetUpdateRegion().GetBox()
        if (rect == ()):
            rect = tuple((box[0],
                          box[1],
                          box[0] + box[2],
                          box[1] + box[3]))
        color = self.mainPanel.GetBackgroundColour()
        newColorTuple = [color.Red() - 50,
                         color.Green() - 50,
                         color.Blue() - 50]
        if (newColorTuple[0] < 0 or \
            newColorTuple[1] < 0 or \
            newColorTuple[2] < 0):
            newColorTuple = [color.Red() + 50,
                             color.Green() + 50,
                             color.Blue() + 50]
        for i in range(0, 3):
            if (newColorTuple[i] > 255 or newColorTuple[i] < 0):
                newColorTuple[i] = 0
        myDC.SetPen(wxPen(self.mainPanel.GetBackgroundColour(), 
                          1, 
                          wxTRANSPARENT))
        myDC.SetBrush(wxBrush(self.mainPanel.GetBackgroundColour(), wxSOLID))
        myDC.DrawRectangle(rect[0],
                           rect[1],
                           rect[2],
                           rect[3])
        myDC.SetPen(wxPen(wxColour(newColorTuple[0],
                                   newColorTuple[1],
                                   newColorTuple[2]),
                           1,
                           wxSOLID))
        myDC.SetBrush(wxBrush("#000000", wxTRANSPARENT))
        for i in range(rect[0], rect[2]):
            if (i% self.mParent.gridSpace == 0):
                myDC.DrawLine(i, rect[1], i, rect[3])
        for i in range(rect[1], rect[3]):
            if (i % self.mParent.gridSpace == 0):
                myDC.DrawLine(rect[0], i, rect[2], i)
    def drawSelectRect(self, myDC):
        myDC.SetPen(wxPen("#000000", 1, wxDOT))
        myDC.SetBrush(wxBrush(self.mainPanel.GetBackgroundColour(), 
                                   wxTRANSPARENT))
        myDC.DrawRectangle(self.selectRect[0], self.selectRect[1],
                           self.selectRect[2], self.selectRect[3])
    def setEditModeText(self, text):
        if (not self.locked):
            self.editModeText.SetLabel(text)
            self.editModeText.SetSize(( \
                                self.GetStatusBar().GetFieldRect(1)[2] - 10,
                                self.editModeText.GetSize()[1]))
            self.editModeText.Show(True)
            pos = (((self.GetStatusBar().GetFieldRect(1)[2] - \
                     self.editModeText.GetSize()[0]) / 2) + \
                   self.GetStatusBar().GetFieldRect(1)[0] + 1,
                   ((self.GetStatusBar().GetFieldRect(1)[3] - \
                     self.editModeText.GetSize()[1]) / 2) + \
                   self.GetStatusBar().GetFieldRect(1)[1] + \
                   -1 * (FUGEFACTOR - 2))
            self.editModeText.Move(pos)
    def lock(self):
        self.locked = True
    def unlock(self):
        self.locked = False
    def positionIcons(self):
        if (not self.locked):
            self.setEditModeText(self.editModeText.GetLabel())
            pos = (self.GetStatusBar().GetFieldRect(2)[0] + 3,
                   self.GetStatusBar().GetFieldRect(2)[3] - \
                   self.connectIcon.GetSize()[1] + 1)
            self.connectIcon.Move(pos)
            self.midiIcon1.Move((self.connectIcon.GetPosition()[0] + \
                                 self.connectIcon.GetSize()[0] + 5,
                                pos[1]))
            self.midiIcon2.Move((self.midiIcon1.GetPosition()[0] + \
                                 self.midiIcon1.GetSize()[0] + 3, pos[1]))
            if (self.mParent.midiEnabled):
                self.jsIcon1.Move((self.midiIcon2.GetPosition()[0] + \
                                   self.midiIcon2.GetSize()[0] + 5,
                                 pos[1]))
            else:
                self.jsIcon1.Move((self.connectIcon.GetPosition()[0] + \
                                   self.connectIcon.GetSize()[0] + 5,
                                  pos[1]))
            self.jsIcon2.Move((self.jsIcon1.GetPosition()[0] + \
                               self.jsIcon1.GetSize()[0] + 3, pos[1]))
               
            self.Refresh()
    def openpanel(self):
        dlg = wxFileDialog(self, "Open file", self.mParent.filepath,
                           "", "*.*", wxOPEN)
        if (dlg.ShowModal() == wxID_OK):
            filename = dlg.GetPath()
            self.mParent.connection.send('openpanel' \
                                         + SYMMSGSEP \
                                         + filename \
                                         + PAIRSEPCHAR)


    def savepanel(self):
        dlg = wxFileDialog(self, "Save file", self.mParent.filepath,
                           "", "*.*", wxSAVE)
        if (dlg.ShowModal() == wxID_OK):
            filename = dlg.GetPath()
            self.mParent.connection.send('savepanel' \
                                         + SYMMSGSEP \
                                         + filename \
                                         + PAIRSEPCHAR)
    def stopIconFlashers(self):
        self.jsIconFlasher1.Stop()
        self.jsIconFlasher2.Stop()
        self.midiIconFlasher1.Stop()
        self.midiIconFlasher2.Stop()
    def Destroy(self):
        if (not self.locked):
            self.stopIconFlashers()
        wxFrame.Destroy(self)

           
# Classes used by mainFrame
# Class to poll for connection to server
class connectionTimer(wxTimer):
    def __init__(self, parent):
        wxTimer.__init__(self)
        self.mParent = parent
        self.timeCount = 0
    def connect(self):
        try:
            if (not self.mParent.locked):
                self.mParent.mainFrame.fileMenu.Enable(ID_CONNECT, False)
            tempConn = socket(AF_INET, SOCK_STREAM)
            tempConn.connect((self.mParent.host, self.mParent.port))
            tempConn.setblocking(0)
            rcvrList = self.mParent.connection.rcvrList
            self.mParent.connection = tempConn
            self.Stop()
            self.mParent.sTimer.Start(self.mParent.pollTime)
            self.mParent.connection.send(rcvrList)
            for control in self.mParent.controlList:
                control.connection = self.mParent.connection
            self.mParent.connected = True
            self.mParent.pingCount = 0
            self.mParent.pTimer.Start(PINGPOLLTIME)
            if (not self.mParent.locked):
                self.mParent.mainFrame.connectIcon.SetBitmap(\
                    self.mParent.connectImage)
                self.mParent.mainFrame.connectIcon.Refresh()
            self.mParent.connection.send(SHOWCOMMAND + \
                                         SYMMSGSEP + "0" + \
                                         PAIRSEPCHAR)
            self.mParent.mainFrame.Show(True)
        except:
            self.timeCount = self.timeCount + 1
    def Notify(self):
        self.connect()
        if (self.timeCount >= TIMEOUT):
            self.Stop()
            dlg = wxMessageDialog(self.mParent.mainFrame, "Connection Timeout",
                                  "Connection", wxOK)
            dlg.ShowModal()
            dlg.Destroy()
            if (not self.mParent.locked):
                self.mParent.mainFrame.fileMenu.Enable(ID_CONNECT, True)

# Class to poll socket for receive string
class socketTimer(wxTimer):
    def __init__(self, parent):
        wxTimer.__init__(self)
        self.mParent = parent
    def Notify(self):
        try:
            receiveBuffer = self.mParent.connection.recv(16384)
            numBytes = len(strip(receiveBuffer))
            if (numBytes > 0):
                self.mParent.parseReceiveBuffer(receiveBuffer)
        except:
            pass

# Class to check connection status
class pingTimer(wxTimer):
    def __init__(self, parent):
        wxTimer.__init__(self)
        self.mParent = parent
    def Notify(self):
        try:
            self.mParent.connection.send(PINGCOMMAND + SYMMSGSEP + \
                                         "0" + PAIRSEPCHAR)

        except:
            self.Stop()
            self.mParent.pingTimeout()

# Class to poll joystick
class joystickTimer(wxTimer):
    def __init__(self, parent):
        wxTimer.__init__(self)
        self.mParent = parent
    def Notify(self):
        if (self.mParent.joyID1 > -1):
            numEvents1 = joystick.readEvents(self.mParent.joyID1)
            if (numEvents1 > 0):
                self.mParent.showJoystickActive(1)
            for i in range(0, numEvents1):
                if (joystick.getEventType(self.mParent.joyID1, i) == 0):
                    try:
                        self.mParent.connection.send('joy0axis'
                                                 + repr(joystick.getEventNumber(
                                                          self.mParent.joyID1,
                                                          i)) \
                                                 + SYMMSGSEP \
                                                 + repr(joystick.getEventValue(
                                                          self.mParent.joyID1,
                                                          i)) \
                                                 + PAIRSEPCHAR)
                    except:
                        self.mParent.close()
                if (joystick.getEventType(self.mParent.joyID1,i) == 1):
                    try:
                        self.mParent.connection.send('joy0button'
                                                 + repr(joystick.getEventNumber(
                                                         self.mParent.joyID1,
                                                         i)) \
                                                 + SYMMSGSEP \
                                                 + repr(joystick.getEventValue(
                                                          self.mParent.joyID1,
                                                          i)) \
                                                 + PAIRSEPCHAR)
                    except:
                        self.mParent.close()
        if (self.mParent.joyID2 > -1):
            numEvents2 = joystick.readEvents(self.mParent.joyID2)
            if (numEvents2 > 0):
                self.mParent.showJoystickActive(2)
            for i in range(0, numEvents2):
                if (joystick.getEventType(self.mParent.joyID2, i) == 0):
                    try:
                        self.mParent.connection.send('joy1axis'
                                                 + repr(joystick.getEventNumber(
                                                          self.mParent.joyID2,
                                                          i)) \
                                                 + SYMMSGSEP \
                                                 + repr(joystick.getEventValue(
                                                          self.mParent.joyID2,
                                                          i)) \
                                                 + PAIRSEPCHAR)
                    except:
                        self.mParent.close()
                if (joystick.getEventType(self.mParent.joyID2,i) == 1):
                    try:
                        self.mParent.connection.send('joy1button'
                                                 + repr(joystick.getEventNumber(
                                                          self.mParent.joyID2,
                                                          i)) \
                                                 + SYMMSGSEP \
                                                 + repr(joystick.getEventValue(
                                                          self.mParent.joyID2,
                                                          i)) \
                                                 + PAIRSEPCHAR)
                    except:
                        self.mParent.close()

# Class to poll midi
class midiTimer(wxTimer):
    def __init__(self, parent):
        wxTimer.__init__(self)
        self.mParent = parent
    def Notify(self):
        if (self.mParent.midiID1 > -1):
            numEvents1 = midi.readEvents(self.mParent.midiID1)
            if (numEvents1 > 0):
                self.mParent.showMidiActive(1)
            for i in range(0, numEvents1):
                midiCommand = midi.getEventCommand(self.mParent.midiID1, i)
                midiCommand = hex(midiCommand)
                midiP1 = midi.getEventP1(self.mParent.midiID1, i)
                midiP2 = midi.getEventP2(self.mParent.midiID1, i)
                messageType = midiCommand[2]
                channel = repr(atoi(midiCommand[3]) + 1)
                try:
                    if (messageType == MIDINOTEMESSAGE):
                        self.mParent.connection.send('midi0note'
                                                 + SYMMSGSEP \
                                                 + repr(midiP1) \
                                                 + " " \
                                                 + repr(midiP2) \
                                                 + " " \
                                                 + channel \
                                                 + PAIRSEPCHAR)
                    elif (messageType == MIDICTLMESSAGE):
                        self.mParent.connection.send('midi0ctl'
                                                 + SYMMSGSEP \
                                                 + repr(midiP1) \
                                                 + " " \
                                                 + repr(midiP2) \
                                                 + " " \
                                                 + channel \
                                                 + PAIRSEPCHAR)
                    elif (messageType == MIDIPGMMESSAGE):
                        self.mParent.connection.send('midi0pgm'
                                                 + SYMMSGSEP \
                                                 + repr(midiP1) \
                                                 + " " \
                                                 + channel \
                                                 + PAIRSEPCHAR)
                except:
                    self.mParent.close()
        if (self.mParent.midiID2 > -1):
            numEvents2 = midi.readEvents(self.mParent.midiID2)
            if (numEvents2 > 0):
                self.mParent.showMidiActive(2)
            for i in range(0, numEvents2):
                midiCommand = midi.getEventCommand(self.mParent.midiID2, i)
                midiCommand = hex(midiCommand)
                midiP1 = midi.getEventP1(self.mParent.midiID2, i)
                midiP2 = midi.getEventP2(self.mParent.midiID2, i)
                messageType = midiCommand[2]
                channel = repr(atoi(midiCommand[3]) + 1)
                try:
                    if (messageType == MIDINOTEMESSAGE):
                        self.mParent.connection.send('midi1note'
                                                 + SYMMSGSEP \
                                                 + repr(midiP1) \
                                                 + " " \
                                                 + repr(midiP2) \
                                                 + " " \
                                                 + channel \
                                                 + PAIRSEPCHAR)
                    elif (messageType == MIDICTLMESSAGE):
                        self.mParent.connection.send('midi1ctl'
                                                 + SYMMSGSEP \
                                                 + repr(midiP1) \
                                                 + " " \
                                                 + repr(midiP2) \
                                                 + " " \
                                                 + channel \
                                                 + PAIRSEPCHAR)
                    elif (messageType == MIDIPGMMESSAGE):
                        self.mParent.connection.send('midi1pgm'
                                                 + SYMMSGSEP \
                                                 + repr(midiP1) \
                                                 + " " \
                                                 + channel \
                                                 + PAIRSEPCHAR)
                except:
                    self.mParent.close()

# Class to flash icon between 2 bitmaps
class iconFlasher(wxTimer):
    def __init__(self, icon, bitmap1, bitmap2, nFlashes):
        wxTimer.__init__(self)
        self.icon = icon
        self.bitmap1 = bitmap1
        self.bitmap2 = bitmap2
        self.nFlashes = nFlashes
        self.running = False
    def Start(self, ms):
        if (not self.running):
            self.running = True
            self.flashCount = 0
            self.activeBitmap = 1
            wxTimer.Start(self, ms)
    def Stop(self):
        self.running = False
        self.icon.SetBitmap(self.bitmap1)
        wxTimer.Stop(self)
    def Notify(self):
        if (self.flashCount >= self.nFlashes):
            self.icon.SetBitmap(self.bitmap1)
            self.icon.Refresh()
            self.Stop()
        else:
            if (self.activeBitmap == 1):
                self.icon.SetBitmap(self.bitmap2)
                self.icon.Refresh()
                self.activeBitmap = 2
                self.flashCount = self.flashCount + 1
            else:
                self.icon.SetBitmap(self.bitmap1)
                self.activeBitmap = 1
                self.icon.Refresh()

# Class to hold commands (receive names) while not connected
class nullConnection:
    def __init__(self):
        self.rcvrList = ""
    def send(self, receiveBuffer):
            (symName, value) = split(receiveBuffer,SYMMSGSEP)
            if (symName[0] == COMMANDCHAR):
                if (symName == SETRCVRCOMMAND):
                    self.rcvrList = self.rcvrList + receiveBuffer
    def close(self):
        pass
 
# Class for holding control info for duplicate (sort of emulates file object)
class duplicationContainer:
    def __init__(self):
        self.list = []
    def write(self, string):
        self.list.append(string)
    def chomp(self):
        for i in range(0, len(self.list)):
            self.list[i] = replace(self.list[i], "\n", "")
    def getList(self):
        return self.list

# Class for setting options
class optionsDialog(wxDialog):
    def __init__(self, parent):
        self.parent = parent
        wxDialog.__init__(self, parent.mainFrame, -1, "Configure",
                          wxDefaultPosition, wxDefaultSize,
                          wxDIALOG_MODAL | wxCAPTION)

        titleText = wxStaticText(self, -1, "Window Title:",
                                 (DEFOPTIONSPOS[0] + 10,
                                  DEFOPTIONSPOS[1] + 20))
        self.titleBox = wxTextCtrl(self, -1, parent.frameTitle,
                                   (titleText.GetPosition()[0] \
                                    + titleText.GetSize()[0] + 80,
                                    titleText.GetPosition()[1] - 4))
        
        addrText = wxStaticText(self, -1, "Connection Address:",
                                (titleText.GetPosition()[0],
                                 titleText.GetPosition()[1] \
                                 + titleText.GetSize()[1] + 15))
        self.addrBox = wxTextCtrl(self, -1, parent.host,
                                  (self.titleBox.GetPosition()[0],
                                   addrText.GetPosition()[1] - 4))
        
        portText = wxStaticText(self, -1, "Connection Port:",
                                (addrText.GetPosition()[0],
                                 addrText.GetPosition()[1] \
                                 + addrText.GetSize()[1] + 15))
        self.portBox = wxTextCtrl(self, -1, repr(parent.port),
                                  (self.titleBox.GetPosition()[0],
                                   portText.GetPosition()[1] - 4))
        
        pollText = wxStaticText(self, -1, "Socket Poll Time (ms): ",
                                (addrText.GetPosition()[0],
                                 portText.GetPosition()[1] \
                                 + portText.GetSize()[1] + 15))
        self.pollBox = wxTextCtrl(self, -1, repr(parent.pollTime),
                                  (self.titleBox.GetPosition()[0],
                                   pollText.GetPosition()[1] - 4))
                                   
        joyDevText1 = wxStaticText(self, -1, "Joystick Device 0: ",
                                  (addrText.GetPosition()[0],
                                   pollText.GetPosition()[1] \
                                   + pollText.GetSize()[1] + 15))
        self.joyDevBox1 = wxTextCtrl(self, -1, parent.joyDevice1,
                                     (self.titleBox.GetPosition()[0],
                                      joyDevText1.GetPosition()[1] - 4))
        
        joyDevText2 = wxStaticText(self, -1, "Joystick Device 1: ",
                                  (addrText.GetPosition()[0],
                                   joyDevText1.GetPosition()[1] \
                                   + joyDevText1.GetSize()[1] + 15))
        self.joyDevBox2 = wxTextCtrl(self, -1, parent.joyDevice2,
                                     (self.titleBox.GetPosition()[0],
                                      joyDevText2.GetPosition()[1] - 4))
        
        joyPollText = wxStaticText(self, -1, "Joystick Poll Time (ms): ",
                                   (addrText.GetPosition()[0],
                                    joyDevText2.GetPosition()[1] \
                                    + joyDevText2.GetSize()[1] + 15))
        self.joyPollBox = wxTextCtrl(self, -1, repr(parent.joyPollTime),
                                     (self.titleBox.GetPosition()[0],
                                      joyPollText.GetPosition()[1] - 4))

        midiDevText1 = wxStaticText(self, -1, "MIDI Device 0: ",
                                  (addrText.GetPosition()[0],
                                   joyPollText.GetPosition()[1] \
                                   + joyPollText.GetSize()[1] + 15))
        self.midiDevBox1 = wxTextCtrl(self, -1, repr(parent.midiDevice1),
                                     (self.titleBox.GetPosition()[0],
                                      midiDevText1.GetPosition()[1] - 4))
        
        midiDevText2 = wxStaticText(self, -1, "MIDI Device 1: ",
                                  (addrText.GetPosition()[0],
                                   midiDevText1.GetPosition()[1] \
                                   + midiDevText1.GetSize()[1] + 15))
        self.midiDevBox2 = wxTextCtrl(self, -1, repr(parent.midiDevice2),
                                     (self.titleBox.GetPosition()[0],
                                      midiDevText2.GetPosition()[1] - 4))
        
        midiPollText = wxStaticText(self, -1, "MIDI Poll Time (ms): ",
                                   (addrText.GetPosition()[0],
                                    midiDevText2.GetPosition()[1] \
                                    + midiDevText2.GetSize()[1] + 15))
        self.midiPollBox = wxTextCtrl(self, -1, repr(parent.midiPollTime),
                                     (self.titleBox.GetPosition()[0],
                                      midiPollText.GetPosition()[1] - 4))

        bColorText = wxStaticText(self, -1, "Background Color:",
                                  (addrText.GetPosition()[0],
                                   midiPollText.GetPosition()[1] \
                                   + midiPollText.GetSize()[1] + 15))
        bColorButton = wxButton(self, -1, "Set",
                                (self.titleBox.GetPosition()[0],
                                 bColorText.GetPosition()[1] - 4),
                                self.midiPollBox.GetSize())
                                    
        rect = wxStaticBox(self, -1, "Options", DEFOPTIONSPOS,
                           (self.addrBox.GetPosition()[0] \
                            + self.addrBox.GetSize()[0] + 10,
                            bColorButton.GetPosition()[1] + \
                            bColorButton.GetSize()[1] + 20))
        line = wxStaticLine(self, -1, (DEFOPTIONSPOS[0],
                            bColorButton.GetPosition()[1] \
                            + bColorButton.GetSize()[1] + 30),
                            (rect.GetSize()[0], 1))
        okButton = wxButton(self, -1, "OK", (0, line.GetPosition()[1] + 10))
        self.SetSize((rect.GetSize()[0] + 15,
                      okButton.GetPosition()[1] + \
                      (FUGEFACTOR * okButton.GetSize()[1]) + \
                      (okButton.GetPosition()[1] - line.GetPosition()[1]) + 4))
        okButton.Center(wxHORIZONTAL)
        okButton.Move((okButton.GetPosition()[0], line.GetPosition()[1] + 10))
        EVT_BUTTON(self, okButton.GetId(), self.eOK)
        EVT_BUTTON(self, bColorButton.GetId(), self.eBColor)
    def eOK(self, event):
        self.EndModal(wxID_OK)
    def eBColor(self, event):
        color = getColorFromDialog(self,
                         self.parent.mainFrame.mainPanel.GetBackgroundColour())
        self.parent.mainFrame.mainPanel.SetBackgroundColour(color)
        for control in self.parent.controlList:
            if (control.resetBackground):
                control.SetBackgroundColour(color)       
