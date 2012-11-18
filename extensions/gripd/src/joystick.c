/* GrIPD v0.1.1 - Graphical Interface for Pure Data
** Copyright (C) 2002 Joseph A. Sarlo
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

#include <stdio.h>
#define MAXEVENTS         4096
#define MAXDEVS           16
#ifdef NT
#include <STRING.H>
#include <WINDOWS.H>
#include <MMSYSTEM.H>
#include <stdlib.h>
#define MAX_AXIS_OUTS     10
#define MAX_BUTTON_OUTS   32
#else
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define JOYSTICK_DEVICE   "/dev/js0"
#define JS_EVENT_BUTTON   0x01
#define JS_EVENT_AXIS     0x02
#define JS_EVENT_INTI     0x80
#define DEFSCALE          1
#define DEFTRANSLATION    0
#define JSIOCGAXES        _IOR('j', 0x11, unsigned char)
#define JSIOCGBUTTONS     _IOR('j', 0x12, unsigned char)
#define MAXAXES           4
#endif

static int eventValue[MAXEVENTS][MAXDEVS];
static int eventNumber[MAXEVENTS][MAXDEVS];
static int eventType[MAXEVENTS][MAXDEVS];
static int eventCount[MAXDEVS];
static int devCount = 0;
static int devList[MAXDEVS];
#ifdef NT
static JOYINFOEX joyInfoEx[MAXDEVS];
static int joyDevNum[MAXDEVS];
static int joy_buttons[MAXDEVS];
static int joy_axes[MAXDEVS];
static int button_val[MAX_BUTTON_OUTS][MAXDEVS];
static int axis_val[MAX_AXIS_OUTS][MAXDEVS];
/* FIXME */
static DWORD *axes_ptr[MAX_AXIS_OUTS][MAXDEVS];
#else
static int joy_fd[MAXDEVS];
struct js_event
{
    unsigned int time;
    signed short value;
    unsigned char type;
    unsigned char number;
};
static struct js_event joy_e[MAXDEVS];
#endif

#ifdef NT
extern "C" {
#endif

int closeDevice(int devno)
{
    if (devList[devno]) 
    {
        devCount--;
        devList[devno] = 0;    
#ifdef NT
        return (1);
#else
        if (close(joy_fd[devno]) < 0)
            return (0);
        else
        {
            joy_fd[devno] = -1;
            return(1);
        }
#endif
    }
}

int openDevice(char *dev)
{
#ifdef NT
    int i, num_axes, num_buttons, devId;
    JOYCAPS jscaps;
    MMRESULT errCode;
    if (devCount == 0) 
    {
        for (i = 0; i < MAXDEVS; i++) 
        {
	    devList[i] = 0;
        }
    }
    for (i = 0; i < MAXDEVS; i++)
    {
        if (devList[i] == 0)
	{
	    devId = i;
            i = MAXDEVS;
        }
    }
    joyInfoEx[devId].dwSize = sizeof(joyInfoEx);
    joyInfoEx[devId].dwFlags = JOY_RETURNALL;
    joyDevNum[devId] = (JOYSTICKID1 - 1) + atoi(dev);
    errCode = joyGetPosEx(joyDevNum[devId], &joyInfoEx[devId]);
    if ((errCode == MMSYSERR_NOERROR) && (devCount < MAXDEVS))
    {
        joyGetDevCaps(joyDevNum[devId], &jscaps, sizeof(jscaps));
        if (jscaps.wNumAxes > MAX_AXIS_OUTS)
            joy_axes[devId] = MAX_AXIS_OUTS;
        else
            joy_axes[devId] = jscaps.wNumAxes;
        if (jscaps.wNumButtons > MAX_BUTTON_OUTS)
            joy_buttons[devId] = MAX_BUTTON_OUTS;
        else
            joy_buttons[devId] = jscaps.wNumButtons;
        for (i = 0; i < joy_axes[devId]; i++)
            axis_val[i][devId] = 0;
        for (i = 0; i < joy_buttons[devId]; i++)
            button_val[i][devId] = 0;
        axes_ptr[0][devId] = &(joyInfoEx[devId].dwXpos);
        axes_ptr[1][devId] = &(joyInfoEx[devId].dwYpos);
        axes_ptr[2][devId] = &(joyInfoEx[devId].dwZpos);
        axes_ptr[3][devId] = &(joyInfoEx[devId].dwRpos);
        axes_ptr[4][devId] = &(joyInfoEx[devId].dwUpos);
        axes_ptr[5][devId] = &(joyInfoEx[devId].dwVpos);
	eventCount[devId] = 0;
	devCount++;
        devList[devId] = 1;
        return (devId);
    }
    else
        return (-1);
#else
    int i, devId;
    char joy_dev[256];

    if (devCount == 0) 
    {
        for (i = 0; i < MAXDEVS; i++) 
        {
	    devList[i] = 0;
        }
    }
    for (i = 0; i < MAXDEVS; i++)
    {
        if (devList[i] == 0)
	{
	    devId = i;
            i = MAXDEVS;
        }
    }
    if (strcmp(dev, "") == 0)
        strcpy(joy_dev, JOYSTICK_DEVICE);
    else
        strcpy(joy_dev, dev);
    joy_fd[devId] = open (joy_dev, O_RDONLY | O_NONBLOCK);
    if ((joy_fd[devId] == -1) || (devCount >= MAXDEVS))
    {
        return (-1);
    }
    else
    {
        devCount++;
        eventCount[devId] = 0;
        devList[devId] = 1;
        return (devId);
    }
#endif
}
 
int readEvents(int devno)
{
#ifdef NT
    int i;
 
    eventCount[devno] = 0;
    joyGetPosEx(joyDevNum[devno], &(joyInfoEx[devno]));
    for (i = 0; i < joy_axes[devno]; i++)
        if (((int)(*(axes_ptr[i][devno])) != axis_val[i][devno]) &&
	    (eventCount[devno] < MAXEVENTS))
        {
            eventType[eventCount[devno]][devno] = 0;
            eventNumber[eventCount[devno]][devno] = i;
            eventValue[eventCount[devno]][devno] = (int)(*(axes_ptr[i][devno]));
            eventCount[devno]++;
            axis_val[i][devno] = (int)(*(axes_ptr[i][devno]));
        }
    for (i = 0; i < joy_buttons[devno]; i ++)
    {
        if (joyInfoEx[devno].dwButtons & (1 << i))
        {
            if ((button_val[i][devno] == 0) && (eventCount[devno] < MAXEVENTS))
            {
                eventType[eventCount[devno]][devno] = 1;
                eventNumber[eventCount[devno]][devno] = i;
                eventValue[eventCount[devno]][devno] = 1;
                eventCount[devno]++;
                button_val[i][devno] = 1;
            }
        }
        else
            if ((button_val[i][devno] == 1) && (eventCount[devno] < MAXEVENTS))
            {
                eventType[eventCount[devno]][devno] = 1;
                eventNumber[eventCount[devno]][devno] = i;
                eventValue[eventCount[devno]][devno] = 0;
                eventCount[devno]++;
                button_val[i][devno] = 0;
            }
    }
    return (eventCount[devno]);
#else
    int i;

    eventCount[devno] = 0;
    if (joy_fd[devno] > -1)
    {
        while (read (joy_fd[devno], &(joy_e[devno]), sizeof(struct js_event)) > -1)
        {
            if (eventCount[devno] < MAXEVENTS)
            {
                if (joy_e[devno].type == JS_EVENT_AXIS)
                    eventType[eventCount[devno]][devno] = 0;
                if (joy_e[devno].type == JS_EVENT_BUTTON)
                    eventType[eventCount[devno]][devno] = 1;
                eventNumber[eventCount[devno]][devno] = joy_e[devno].number;
                eventValue[eventCount[devno]][devno] = joy_e[devno].value;
                eventCount[devno]++;
            }
        }
    }
    return eventCount[devno];
#endif
}

int getEventCount(int devno)
{
    return (eventCount[devno]);
}

int getEventType(int devno, int eNum)
{
    int returnVal = 0;

    if (eNum >= 0 && eNum < eventCount[devno])
    {
        returnVal = eventType[eNum][devno];
    }
    return (returnVal);
}

int getEventNumber(int devno, int eNum)
{
    int returnVal = 0;

    if (eNum >= 0 && eNum < eventCount[devno])
    {
        returnVal = eventNumber[eNum][devno];
    }
    return (returnVal);
}

int getEventValue(int devno, int eNum)
{
    int returnVal = 0;

    if (eNum >= 0 && eNum < eventCount[devno])
    {
        returnVal = eventValue[eNum][devno];
    }
    return (returnVal);
}
#ifdef NT
}
#endif

