/* GrIPD v0.1.1 - Graphical Interface for Pure Data
** Copyright (C) 2003 Joseph A. Sarlo
**
** This program is free software; you can redistribute it and/orsig
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

#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#ifdef NT
#include <windows.h>
#include <winsock.h>
#else
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sched.h>
#include <sys/mman.h>
#endif

#define VERSION            "0.1.1"
#define DEFPORT            0
/* maxiumum connections */
#define BACKLOG            1
/* max size of send buffer */
#define MAXDATASIZE        16384
#define MAXPORTSTRLEN      16
#define MAXSYMNAMELEN      128
#define MAXSYMVALUELEN     128
/* separates send/recv object names from value they send/recv */
#define SYMMSGSEP          31
/* separates send/recv value pairs */
#define PAIRSEPCHAR        29
#define BANGSTRING         "bang"
/* if 1st char, indicates send/recv name is actually a command */
#define COMMANDCHAR '!'
/* command to add value to recv list */
#define SETRCVRSTRING      "!setRcvr"
/* command to close connection */
#define CLOSECOMMAND       "!disconnect"
/* command to close python app */
#define EXITCOMMAND        "!exit"
/* command to lock GUI */
#define LOCKCOMMAND        "!lock"
/* command to unlock GUI */
#define UNLOCKCOMMAND      "!unlock"
/* command to set title */
#define SETTITLECOMMAND    "!settitle"
/* command to hide GUI window */
#define HIDECOMMAND        "!hide"
/* command to show GUI window */
#define SHOWCOMMAND        "!show"
/* command for connection status */
#define PINGCOMMAND        "!ping"
// command to have GUI open a open file dialog
#define OPENPANELCOMMAND   "!openpanel"
// command to have GUI open a save file dialog
#define SAVEPANELCOMMAND   "!savepanel"
/* initial size of rcvr object list */
#define DEFLISTSIZE        16
/* poll time in ms to re-try accept() */
#define CONNECTIONPOLLTIME 100
/* blocking time us for accept() */
#define WAITTIME           1
/* poll time in ms to re-try bind after "address already in use" */
#define REBINDTIME         1000
/* default receive poll time in ms */
#define DEFRDELTIME        5
#define DEFSDELTIME        1
/* default send poll time in ms */
#define DEFSTRSIZE         256
#define MAXALISTLEN        64

#ifdef NT
#define MSG_DONTWAIT       0
#define MSG_NOSIGNAL       0
#define MSG_WAITALL        0
#else
#define SOCKET_ERROR       -1
#endif

/* Individual "receive" class */
typedef struct _gripdRcvr
{
    t_object r_obj;
    t_symbol *r_sym;
    /* gripd object, needed so recevied symbol functions can access
       gripd data (socket)*/
    void *r_x;
}t_gripdRcvr;

/* Main object class */
typedef struct _gripd
{
    t_object t_ob;
    t_outlet *x_outlet2;
    unsigned short int x_port;
#ifdef NT
    SOCKET x_sock;
    SOCKET x_newSock;
    /* for Windows, 1 if WSAStartup is called */
    int x_wsockInitByMe;
    PROCESS_INFORMATION x_childProcessInfo;
#else
    int x_sock;
    int x_newSock;
    int x_childPID;
#endif
    /* 1 if socket has been bound, 0 otherwise */
    int x_bound;
    /* 1 if currently connected, 0 otherwise */
    int x_connected;
    /* 1 if PD opened python app, otherwise (shared mem for linux)*/
    int *x_localOpened;
    /* timer for receiveing/sending */
    t_clock *x_rclock;
    t_clock *x_sclock;
    /* timer for polling accept() */
    t_clock *x_connectionClock;
    /* timer for re-binding after "address already in use" error */
    t_clock *x_bindClock;
    /* path to python.exe for Windows */
    char x_pythExec[DEFSTRSIZE];
    /* path to gripd.py (or gripd.exe and gripd.py in Windows) */
    char x_pythFile[DEFSTRSIZE];
    /* send/receive poll times */
    double x_sdeltime;
    double x_rdeltime;
    /* current number of receive "objects" being used */
    int x_rcvrListSize;
    /* size currently allocated for receiver objects */
    int x_rcvrListMaxSize;
    /* pointer to list of receiver objects */
    t_gripdRcvr **x_rcvrs;
    /* buffer to be sent */
    char x_sendBuffer[MAXDATASIZE];
    /* path to application */
    char x_appPath[MAXDATASIZE];
}t_gripd;

t_class *gripd_class;
t_class *gripdRcvr_class;

void gripd_setup(void);
void *gripd_new(t_floatarg port);
void gripd_connect(t_gripd *x);
/* initialize "server" */
void gripd_openSocket(t_gripd *x);
/* shutdown server */
void gripd_disconnect(t_gripd *x);
/* poll accept() */
void gripd_trySocket(t_gripd *x);
void gripd_closeSocket(t_gripd *x);
void gripd_recv(t_gripd *x);
void gripd_send(t_gripd *x);
/* add name:value pair to send string */
void gripd_appendSendBuffer(t_gripd *x, char *aString);
/* open gripd.py (or gripd.exe inWindows) unlocked mode*/
void gripd_open(t_gripd *x, t_symbol *sym, int argc, t_atom *argv);
/* open gripd.py (or gripd.exe inWindows) locked mode*/
void gripd_openLocked(t_gripd *x, t_symbol *sym, int argc, t_atom *argv);
/* actually open gripd.py (or gripd.exe inWindows)*/
void gripd_openPyth(t_gripd *x, t_symbol *sym, int argc,
		    t_atom *argv, int locked);
/* tell python app to close itself */
void gripd_closePyth(t_gripd *x);
/* lock GUI */
void gripd_lock(t_gripd *x);
/* unlock GUI */
void gripd_unlock(t_gripd *x);
/* hide GUI */
void gripd_hide(t_gripd *x);
/* show GUI */
void gripd_show(t_gripd *x);
/* set GUI window title */
void gripd_setTitle(t_gripd *x, t_symbol *sym, int argc, t_atom *argv);
/* set path to gripd.py (or gripd.exe in Windows) */
void gripd_setPath(t_gripd *x, t_symbol *sym, int argc, t_atom *argv);
/* set path to python.exe for Windows */
void gripd_setPythonPath(t_gripd *x, t_symbol *sym, int argc,
                         t_atom *argv);
void gripd_setSTime(t_gripd *x, t_floatarg val);
void gripd_setRTime(t_gripd *x, t_floatarg val);
void gripd_openpanel(t_gripd *x);
void gripd_savepanel(t_gripd *x);
void gripdR_bang(t_gripdRcvr *r);
void gripdR_float(t_gripdRcvr *r, t_float floatValue);
void gripdR_symbol(t_gripdRcvr *r, t_symbol *sym);
void gripdR_anything(t_gripdRcvr *r, t_symbol *sym, int argc,
                     t_atom *argv);
void gripdR_list(t_gripdRcvr *r, t_symbol *sym, int argc, t_atom *argv);
/* instantiate new rcv object */
void gripd_makeGripdRcvr(t_gripd *x, t_symbol *s);
/* check is rcv object has already been created */
int gripd_checkExistance(t_gripd *x, char *name);
/* allocate more mem for recv objects */
void gripd_expandRcvrList(t_gripd *x);
void gripd_free(t_gripd *x);
int gripd_isNumeric(char *string);
#ifndef NT
void gripd_sigChild(int signum);
#endif
void gripd_getApplicationPath(t_gripd *x);
