/* GrIPD v0.1.1 - Graphical Interface for Pure Data
** Copyright (C) 2003 Joseph A. Sarlo
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
** jsarlo@ucsd.edu
*/

#include "midiiolib.h"
#include <stdio.h>
#ifndef NT
#include <unistd.h>
#endif
#define MAXDEVS 16
#define MAXEVENTS 4096

#ifndef VISUAL
using namespace std;
#endif

static int eventCount[MAXDEVS];
static int eventCommand[MAXEVENTS][MAXDEVS];
static int eventP0[MAXEVENTS][MAXDEVS];
static int eventP1[MAXEVENTS][MAXDEVS];
static int eventP2[MAXEVENTS][MAXDEVS];
static int eventP3[MAXEVENTS][MAXDEVS];
static MidiInput mInput[MAXDEVS];
static int devCount = 0;
static int devList[MAXDEVS];

int openDevice(int dev) {
    int i, devId, numPorts;
   
    numPorts = MidiInPort::getNumPorts();
    if (dev >= numPorts || dev < 0) {
        return (-1);
    }
    else {
        if (devCount == 0) {
            for (i = 0; i < MAXDEVS; i++) {
                devList[i] = 0;
            }
        }
        for (i = 0; i < MAXDEVS; i++) {
            if (devList[i] == 0) {
                devId = i;
                i = MAXDEVS;
            }
        }
        devCount++;
        eventCount[devId] = 0;
        devList[devId] = 1;
        mInput[devId].setPort(dev);
        mInput[devId].open();
        return (devId);
    }
}

int closeDevice(int devno) {
    if (devList[devno]) {
        devCount--;
        devList[devno] = 0;
#ifndef LINUX
        mInput[devno].close();
#endif
        return (1);
    }
    else
        return (-1);
}
    
int readEvents(int devno) {
    int i;
    MidiMessage message;
    
    if (devno < 0 || devno >= MAXDEVS)
        return (-1);
    eventCount[devno] = mInput[devno].getCount();
    if (eventCount[devno] > MAXEVENTS) {
        eventCount[devno] = MAXEVENTS;
    }
    for (i = 0; i < eventCount[devno]; i++) {
        message = mInput[devno].extract();
        eventCommand[i][devno] = message.getCommand();
        eventP0[i][devno] = message.getP0();
        eventP1[i][devno] = message.getP1();
        eventP2[i][devno] = message.getP2();
        eventP3[i][devno] = message.getP3();
    }
    return (eventCount[devno]);
}

int getEventCount(int devno) {
    if (devno < 0 || devno >= MAXDEVS)
        return (-1);
    else
        return (eventCount[devno]);
}

int getEventCommand(int devno, int eNum) {
    if (eNum < 0 || eNum >= eventCount[devno] || devno < 0 || devno >= MAXDEVS)
        return (-1);
    else
        return eventCommand[eNum][devno];
}

int getEventP0(int devno, int eNum) {
    if (eNum < 0 || eNum >= eventCount[devno] || devno < 0 || devno >= MAXDEVS)
        return (-1);
    else
        return eventP0[eNum][devno];
}

int getEventP1(int devno, int eNum) {
    if (eNum < 0 || eNum >= eventCount[devno] || devno < 0 || devno >= MAXDEVS)
        return (-1);
    else
        return eventP1[eNum][devno];
}

int getEventP2(int devno, int eNum) {
    if (eNum < 0 || eNum >= eventCount[devno] || devno < 0 || devno >= MAXDEVS)
        return (-1);
    else
        return eventP2[eNum][devno];
}

int getEventP3(int devno, int eNum) {
    if (eNum < 0 || eNum >= eventCount[devno] || devno < 0 || devno >= MAXDEVS)
        return (-1);
    else
        return eventP3[eNum][devno];
}

