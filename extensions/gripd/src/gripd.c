/* GrIPD v0.1. - Graphical Interface for Pure Data
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

#include "gripd.h"

void gripd_setup(void)
{
    post (" \n");
    post ("   .______________________________________.");
    post ("   |                                      |");
    post ("   |             GrIPD: v%s            |", VERSION);
    post ("   |  Graphical Interface for Pure Data   |");
    post ("   |  (C) Copyright 2003 Joseph A. Sarlo  |");
    post ("   |      GNU General Public License      |");
    post ("   |______________________________________|");
    post (" \n");
#ifndef NT
    signal(SIGCHLD, gripd_sigChild);
#endif
    gripd_class = class_new(gensym("gripd"), (t_newmethod)gripd_new,
                            (t_method)gripd_free, sizeof(t_gripd), 0,
                            A_DEFFLOAT, 0);
    gripdRcvr_class = class_new(gensym("gripdRcvr"),0,0,sizeof(t_gripd),
                                0, 0);
    class_addmethod(gripd_class, (t_method)gripd_connect,
                    gensym("connect"), 0);
    class_addmethod(gripd_class, (t_method)gripd_disconnect,
                    gensym("disconnect"), 0);
    class_addmethod(gripd_class, (t_method)gripd_open, 
                    gensym("open"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_openLocked,
                    gensym("open_locked"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_setPythonPath,
                    gensym("set_python_path"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_setPath,
                    gensym("set_path"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_setSTime,
                    gensym("poll_send"), A_DEFFLOAT, 0);
    class_addmethod(gripd_class, (t_method)gripd_setRTime,
                    gensym("poll_receive"), A_DEFFLOAT, 0);
    class_addmethod(gripd_class, (t_method)gripd_lock,
                    gensym("lock"), 0);
    class_addmethod(gripd_class, (t_method)gripd_unlock,
                    gensym("unlock"), 0);
    class_addmethod(gripd_class, (t_method)gripd_setTitle,
                    gensym("set_title"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_hide,
                    gensym("hide"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_show,
                    gensym("show"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_openpanel,
                    gensym("openpanel"), A_GIMME, 0);
    class_addmethod(gripd_class, (t_method)gripd_savepanel,
                    gensym("savepanel"), A_GIMME, 0);
    class_addbang(gripdRcvr_class, (t_method)gripdR_bang);
    class_addfloat(gripdRcvr_class, (t_method)gripdR_float);
    class_addsymbol(gripdRcvr_class, (t_method)gripdR_symbol);
    class_addanything(gripdRcvr_class, (t_method)gripdR_anything);
    class_addlist(gripdRcvr_class, (t_method)gripdR_list);
}

void *gripd_new(t_floatarg port)
{
    t_gripd *x = (t_gripd *)pd_new(gripd_class);

    if (port == 0)
        x->x_port = DEFPORT;
    else
        x->x_port = (int)port;
    x->x_bound = 0;
    x->x_connected = 0;
#ifdef NT
    x->x_localOpened = (int *)malloc(sizeof(int));
#else
    x->x_childPID = 0;
    x->x_localOpened = (int *)shmat(shmget(IPC_PRIVATE,sizeof(int),
                                           IPC_CREAT | SHM_R | SHM_W), 0,
                                    0);
#endif
    *(x->x_localOpened) = 0;
    x->x_rdeltime = DEFRDELTIME;
    x->x_sdeltime = DEFSDELTIME;
    x->x_rclock = clock_new(x, (t_method)gripd_recv);
    x->x_sclock = clock_new(x, (t_method)gripd_send);
    x->x_connectionClock = clock_new(x, (t_method)gripd_trySocket);
    x->x_bindClock = clock_new(x, (t_method)gripd_connect);
#ifdef NT
    strcpy(x->x_pythExec, "c:\\program files\\python\\");
    strcpy(x->x_pythFile, "..\\gripd\\");
    x->x_wsockInitByMe = 0;
#else
    strcpy(x->x_pythExec, "");
    strcpy(x->x_pythFile, "../gripd/");
#endif
    x->x_rcvrListMaxSize = DEFLISTSIZE;
    x->x_rcvrs = (t_gripdRcvr **)calloc(x->x_rcvrListMaxSize,
                                        sizeof(t_gripdRcvr *));
    x->x_rcvrListSize = 0;
    x->x_sendBuffer[0] = '\0';
    outlet_new(&x->t_ob, &s_float);
    x->x_outlet2 = outlet_new(&x->t_ob, &s_float);
    gripd_getApplicationPath(x);
    return (void *)x;
}

void gripd_connect(t_gripd *x)
{
    gripd_openSocket(x);
    clock_delay(x->x_rclock, x->x_rdeltime);
    clock_delay(x->x_sclock, x->x_sdeltime);
}

void gripd_openSocket(t_gripd *x)
{
#ifdef NT
    char my_name[DEFSTRSIZE];
    struct sockaddr_in my_addr;
    struct sockaddr_in new_addr; 
    struct hostent *hp;
    WSADATA wsaData;
    int nSize, temp = 1;
    unsigned long on = 1;

    if (!x->x_bound)
    {
        memset(&my_addr, 0, sizeof(struct sockaddr_in));
        memset(&new_addr, 0, sizeof(struct sockaddr_in));
        gethostname(my_name, sizeof(my_name));
        hp = gethostbyname(my_name);
        if (hp == NULL)
        {
            if (WSAGetLastError() == WSANOTINITIALISED)
            {
                if (WSAStartup (MAKEWORD(1,1), &wsaData) != 0)
                    post("GrIPD: Failed to initialize winsock");
                else
                {
                    x->x_wsockInitByMe = 1;
                    gripd_openSocket(x);
                    return;
                }
            }
            else
            {
                post("GrIPD: Gethostname Error %d", WSAGetLastError());
                gripd_disconnect(x);
            }
        }
        my_addr.sin_family = hp->h_addrtype;
        my_addr.sin_port = htons(x->x_port);
        x->x_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (x->x_sock == INVALID_SOCKET)
        {
            post("GrIPD: Socket Error %d", WSAGetLastError());
            gripd_disconnect(x);
        }
        else
        {
            ioctlsocket(x->x_sock, FIONBIO, &on);
            setsockopt(x->x_sock, SOL_SOCKET, SO_REUSEADDR,
                       (char *)&temp, sizeof(temp));
        }
        if (bind(x->x_sock, (struct sockaddr *)&my_addr,
            sizeof(struct sockaddr)) == SOCKET_ERROR)
        {
            post("GrIPD: Bind Error %d", WSAGetLastError());
            post("GrIPD: Attempting to re-bind");
            clock_delay(x->x_bindClock, REBINDTIME);
        }
        else
        {
            nSize = sizeof(struct sockaddr_in);
            getsockname(x->x_sock, (struct sockaddr *)&new_addr, &nSize);
            x->x_port = (int)ntohs(new_addr.sin_port);
            post("GrIPD: Using port %d", x->x_port);
            x->x_bound = 1;
            listen(x->x_sock, BACKLOG);
            post("GrIPD: Waiting for a connection...");
            clock_delay(x->x_connectionClock, CONNECTIONPOLLTIME);
        }
    }
    else
        post("GrIPD: Already waiting for a connection");
#else
    char portstr[MAXPORTSTRLEN];
    struct sockaddr_in my_addr, new_addr;
    int nSize, temp = 1;

    if (!x->x_bound)
    {
        x->x_sock = -1;
        if ((x->x_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("GrIPD: Socket Error");
            gripd_disconnect(x);
        }
        setsockopt(x->x_sock, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&temp, sizeof(temp));
        fcntl(x->x_sock, F_SETFL, O_NONBLOCK);
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(x->x_port);
        my_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(my_addr.sin_zero), 8);
        if (bind(x->x_sock, (struct sockaddr *)&my_addr,
            sizeof(struct sockaddr)) == -1)
        {
            perror("GrIPD: Bind Error");
            clock_delay(x->x_bindClock, REBINDTIME);
        }
        else
        {
            nSize = sizeof(struct sockaddr_in);
            getsockname(x->x_sock, (struct sockaddr *)&new_addr, &nSize);
            x->x_port = ntohs(new_addr.sin_port);
            post("GrIPD: Using port %d", x->x_port);
            x->x_bound = 1;
            if (listen(x->x_sock, BACKLOG) == -1)
            {
                perror("GrIPD: Listen Error");
                gripd_disconnect(x);
            }
            post("GrIPD: Waiting for a connection...");
            clock_delay(x->x_connectionClock, CONNECTIONPOLLTIME);
        }
    }
    else
        post("GrIPD: Already waiting for a connection");
#endif
}

void gripd_disconnect(t_gripd *x)
{
    int numbytes;
    char buf[MAXDATASIZE];

    clock_unset(x->x_sclock);
    clock_unset(x->x_rclock);
    clock_unset(x->x_bindClock);
    clock_unset(x->x_connectionClock);
    if (*(x->x_localOpened)) 
    {
        gripd_closePyth(x);
    }
    else
    {
        sprintf(buf, "%s%c0%c", CLOSECOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
        numbytes = recv(x->x_newSock, buf, MAXDATASIZE, MSG_NOSIGNAL);
    }
    if (x->x_bound) 
    {
        gripd_closeSocket(x);
    }
}
 
void gripd_trySocket(t_gripd *x)
{
#ifdef NT
    fd_set rfds;
    struct timeval tv;
    unsigned long on;

    on = 1;
    tv.tv_sec = 0;
    tv.tv_usec = WAITTIME;
    FD_ZERO(&rfds);
    FD_SET(x->x_sock, &rfds);
    if (select(x->x_sock + 1, &rfds, NULL, NULL, &tv) > 0)
    {
        x->x_newSock = accept(x->x_sock, NULL, NULL);
        if (x->x_newSock == INVALID_SOCKET)
        {
            post("GrIPD: Accept Error %d", WSAGetLastError());
            gripd_disconnect(x);
        }
        else
        {
            ioctlsocket(x->x_newSock, FIONBIO, &on);
            x->x_connected = 1;
	    outlet_float(x->t_ob.ob_outlet, 1);
	    post("GrIPD: Connected");
        }
    }
    else
        clock_delay(x->x_connectionClock, CONNECTIONPOLLTIME);
#else
    fd_set rfds;
    struct timeval tv;
    struct sockaddr_in their_addr;
    int new_fd, sin_size;
 
    sin_size = sizeof(struct sockaddr_in);
    tv.tv_sec = 0;
    tv.tv_usec = WAITTIME;
    FD_ZERO(&rfds);
    FD_SET(x->x_sock, &rfds);
    if (select(x->x_sock + 1, &rfds, NULL, NULL, &tv))
    {
        if ((x->x_newSock = accept(x->x_sock,
                                   (struct sockaddr *)&their_addr,
                                    &sin_size)) == -1)
        {
            perror("GrIPD: Accept Error");
            gripd_disconnect(x);
        }
        else
        {
            x->x_connected = 1;
	    outlet_float(x->t_ob.ob_outlet, 1);
	    post("GrIPD: Connected");
        }
    }
    else
        clock_delay(x->x_connectionClock, CONNECTIONPOLLTIME);
#endif
}

void gripd_closeSocket(t_gripd *x)
{
#ifdef NT
    closesocket(x->x_newSock);
    closesocket(x->x_sock);
#else
    shutdown(x->x_newSock, 2);
    close(x->x_newSock);
    shutdown(x->x_sock, 2);
    close(x->x_sock);
#endif
    x->x_bound = 0;
    x->x_connected = 0;
    *(x->x_localOpened) = 0;
    outlet_float(x->t_ob.ob_outlet, 0);
    outlet_float(x->x_outlet2, 0);
}

void gripd_recv(t_gripd *x)
{
    int numbytes, count, idx, i, j, argc;
    char buf[MAXDATASIZE], symName[MAXSYMNAMELEN],
         symValue[MAXSYMVALUELEN],
         tempString[MAXDATASIZE];
    t_symbol *tempSym;
    t_atom atomList[MAXALISTLEN];
    numbytes = count = idx = i = j = argc = 0;
    tempSym = gensym("");

    if (x->x_connected)
    {
        numbytes = recv(x->x_newSock, buf, MAXDATASIZE,
                        MSG_DONTWAIT | MSG_NOSIGNAL);
        if (numbytes > 0)
        {
            buf[numbytes] = '\0';
            while (count <= numbytes)
            {
                while ((buf[count] != SYMMSGSEP) && (count <= numbytes))
                {
                    symName[i] = buf[count];
                    count++;
                    i++;
                }
                symName[i] = '\0';
                i = 0;
                count++;
                while ((buf[count] != PAIRSEPCHAR) && (count <= numbytes))
                {
                    symValue[i] =  buf[count];
                    count ++;
                    i++;
                }
                symValue[i] = '\0';
                count++;
                i = 0;
                if (symName[0] == COMMANDCHAR)
                {
                    if ((strcmp(SETRCVRSTRING, symName) == 0) \
                    && (gripd_checkExistance(x, symValue) == 0))
                    {
                        if (x->x_rcvrListSize \
                            == (x->x_rcvrListMaxSize - 1))
                            gripd_expandRcvrList(x);
                        gripd_makeGripdRcvr(x, gensym(symValue));
                        x->x_rcvrListSize++;
                    }
                    if (strcmp(CLOSECOMMAND, symName) == 0)
                    {
                        post("GrIPD: Connection closed remotely");
                        gripd_disconnect(x);
                    }
                    if (strcmp(HIDECOMMAND, symName) == 0)
                    {
                        outlet_float(x->x_outlet2, 0);
                    }
                    if (strcmp(SHOWCOMMAND, symName) == 0)
                    {
                        outlet_float(x->x_outlet2, 1);
                    }
                    if (strcmp(PINGCOMMAND, symName) == 0)
                    {
                        char pingCommandStr[64];
                    
                        sprintf(pingCommandStr, 
                                "%s%c0%c", 
                                PINGCOMMAND, 
                                SYMMSGSEP, 
                                PAIRSEPCHAR);
                        gripd_appendSendBuffer(x, pingCommandStr);
                        gripd_send(x);
                    }
                }
                else
                {
                    tempSym = gensym(symName);
                    if (tempSym->s_thing)
                    {
                        if (strcmp(BANGSTRING, symValue) == 0)
                            pd_bang(tempSym->s_thing);
                        else
                        {
			    if (strchr(symValue, ' ') == NULL)
			    {
			        if (gripd_isNumeric(symValue))
                                    pd_float(tempSym->s_thing, 
				             (float)atof(symValue));
			        else
				    typedmess(tempSym->s_thing,
					      gensym(symValue),
					      0,
					      NULL);
			    }
                            else
                            {
                                idx = 0;
                                argc = 0;
                                for (j = 0; j < (int)strlen(symValue); j++)
                                {
                                    if (symValue[j] != ' ')
                                    {
                                        tempString[idx] = symValue[j];
                                        idx++;
                                    }
                                    if ((symValue[j] == ' ')
                                    || (j == (int)strlen(symValue) -1))
                                    {
                                        tempString[idx] = '\0';
                                        if (gripd_isNumeric(tempString))
                                        {
                                            atomList[argc].a_type = A_FLOAT;
                                            atomList[argc].a_w.w_float =
                                                (float)atof(tempString);
                                        }
                                        else
                                        {
                                            atomList[argc].a_type = A_SYMBOL;
                                            atomList[argc].a_w.w_symbol
                                                = gensym(tempString);
                                        }
                                        argc++;
                                        idx = 0;
                                    }
                                }
                                if (atomList[0].a_type == A_FLOAT)
                                    pd_list(tempSym->s_thing,
                                            atomList[0].a_w.w_symbol,
                                            argc, atomList);
                                else
                                    typedmess(tempSym->s_thing,
                                              atomList[0].a_w.w_symbol,
					      argc - 1, &(atomList[1]));
                            }
                        }
                    }
                }
            }
        }
    }
    clock_delay(x->x_rclock, x->x_rdeltime);
}

void gripd_send(t_gripd *x)
{
    int charsSent, endPos;
    if (x->x_connected && (strlen(x->x_sendBuffer) > 0))
    {
        charsSent = send(x->x_newSock, x->x_sendBuffer,
			 strlen(x->x_sendBuffer),
			 MSG_DONTWAIT | MSG_NOSIGNAL);
	if (charsSent == SOCKET_ERROR)
	{
	    post("GrIPD: Client is not responding");
	    gripd_disconnect(x);
	    return;
        }
	else if ((charsSent <= (signed int)strlen(x->x_sendBuffer)) && 
                 (charsSent > -1))
        {
	    endPos = strlen(x->x_sendBuffer) - charsSent;
	    strcpy(x->x_sendBuffer, &(x->x_sendBuffer[charsSent]));
            x->x_sendBuffer[endPos] = '\0';
        }
    }
    clock_delay(x->x_sclock, x->x_sdeltime);
}

/* this does NOT take care of separator strings, sending function must
    do that */
void gripd_appendSendBuffer(t_gripd *x, char *aString)
{
    if (x->x_connected)
    {
        /* +1 below since strlen does not include '\0' */
        if ((strlen(x->x_sendBuffer) + strlen(aString)) + 1 > MAXDATASIZE)
            post("GrIPD: Send buffer overflow");
        else
            strcat(x->x_sendBuffer, aString);
    }
}

void gripd_open(t_gripd *x, t_symbol *sym, int argc, t_atom *argv)
{
    gripd_openPyth(x, sym, argc, argv, 0);
}
void gripd_openLocked(t_gripd *x, t_symbol *sym, int argc, t_atom *argv)
{
    gripd_openPyth(x, sym, argc, argv, 1);
}

void gripd_openPyth(t_gripd *x, t_symbol *sym, int argc,
                    t_atom *argv, int locked)
{
#ifdef NT
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char pythExec[DEFSTRSIZE], filename[DEFSTRSIZE], tempStr[DEFSTRSIZE];
    int i;
 
    if (!x->x_connected && !(*(x->x_localOpened)))
    {
        gripd_connect(x);
        *(x->x_localOpened) = 1;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (argc > 0)
        {
            if (argv[0].a_type == A_SYMBOL)
            {
                strcpy(filename, argv[0].a_w.w_symbol->s_name);
                for (i = 1; i < argc; i++)
                    if (argv[i].a_type == A_SYMBOL)
                       sprintf(filename, "%s %s", filename,
                       argv[i].a_w.w_symbol->s_name);
            }
            else
                strcpy(filename, "0");
        }
        else
            strcpy(filename, "0");
        for (i = 0; i < (int)strlen(x->x_pythExec); i++)
            if (x->x_pythExec[i] == '/')
                x->x_pythExec[i] = '\\';
        for (i = 0; i < (int)strlen(x->x_pythFile); i++)
            if (x->x_pythFile[i] == '/')
                x->x_pythFile[i] = '\\';
        for (i = 0; i < (int)strlen(filename); i++)
            if (filename[i] == '/')
                filename[i] = '\\';
        if (filename[0] == '.' && 
            (filename[1] == '\\' || 
             (filename[1] == '.' && filename[2] == '\\')))
        {
            sprintf(tempStr, "%s%s", x->x_appPath, filename);
            strcpy(filename, tempStr);
        } 
        if (x->x_pythFile[0] == '.' && 
            (x->x_pythFile[1] == '\\' || 
             (x->x_pythFile[1] == '.' && x->x_pythFile[2] == '\\')))
        {
            sprintf(tempStr, "%s%s", x->x_appPath, x->x_pythFile);
            strcpy(x->x_pythFile, tempStr);
        } 
        sprintf(pythExec, "\"%sgripd.exe\" \"%s\" %d 1 %d", x->x_pythFile,
                filename, x->x_port, locked);
        if (!(CreateProcess(NULL, pythExec, NULL, NULL, FALSE, 0, NULL, NULL,
                            &si, &(x->x_childProcessInfo)) != 0))
        {
            post("GrIPD: Failed to execute %sgripd.exe", x->x_pythFile);
            sprintf(pythExec, "\"%spython.exe\" \"%sgripd.py\" \"%s\" %d 1 %d",
                    x->x_pythExec, x->x_pythFile, filename, x->x_port, locked);
            if (!(CreateProcess(NULL, pythExec, NULL, NULL, FALSE, 0, NULL,
                                NULL, &si, &(x->x_childProcessInfo)) != 0))
            {
                post("GrIPD: Failed to execute %spython.exe", x->x_pythExec);
                gripd_disconnect(x);
                *(x->x_localOpened) = 0;
            }
        }
    }
#else
    char pythExec[DEFSTRSIZE], filename[DEFSTRSIZE], portString[DEFSTRSIZE],
         lockedString[DEFSTRSIZE], tempStr[DEFSTRSIZE];
    int i, pid;
 
    if (!x->x_connected && !(*(x->x_localOpened)))
    {
        gripd_connect(x);
        pid = fork();
        if (pid == 0)
        {
            struct sched_param par;
            int p1;
 
            /* Lose setuid priveliges */
            seteuid(getuid());
            *(x->x_localOpened) = 1;
            /* set lowest priority, SCHED_OTHER policy, unlock mem*/
#ifdef _POSIX_PRIORITY_SCHEDULING
            p1 = sched_get_priority_min(SCHED_OTHER);
            par.sched_priority = p1;
            if (sched_setscheduler(0, SCHED_OTHER, &par) == -1)
                post("GrIPD: unable to set priority %d scheduling.", p1);
#endif
#ifdef _POSIX_MEMLOCK
            if ((munlockall() == -1) && (!getuid()))
    	        post("GrIPD: unable to unlock memory.");
#endif
            clock_free(x->x_rclock);
            clock_free(x->x_sclock);
            clock_free(x->x_connectionClock);
            clock_free(x->x_bindClock);
            for (i = 0; i < x->x_rcvrListSize; i++)
                pd_unbind(&(x->x_rcvrs[i])->r_obj.ob_pd,
                            x->x_rcvrs[i]->r_sym);
            free(x->x_rcvrs);
            sprintf(pythExec, "%sgripd", x->x_pythFile);
            if (argc > 0)
            {
                strcpy(filename, argv[0].a_w.w_symbol->s_name);
                for (i = 1; i < argc; i++)
                    if (argv[i].a_type == A_SYMBOL)
                       sprintf(filename, "%s %s", filename,
                               argv[i].a_w.w_symbol->s_name);
            }
            else
                strcpy(filename, "0");
            if (filename[0] == '.' && 
                (filename[1] == '/' || 
                 (filename[1] == '.' && filename[2] == '/')))
            {
	          sprintf(tempStr, "%s%s", x->x_appPath, filename);
		    strcpy(filename, tempStr);
            } 
            if (pythExec[0] == '.' && 
                (pythExec[1] == '/' || 
                 (pythExec[1] == '.' && pythExec[2] == '/')))
            {
                sprintf(tempStr, "%s%s", x->x_appPath, pythExec);
                strcpy(pythExec, tempStr);
            } 
            /* set x_localOpened before opened since execlp will exit
               process on success */
            sprintf(portString, "%d", x->x_port);
            sprintf(lockedString, "%d", locked);
            if ((execlp(pythExec, pythExec, filename, portString,
                 "1", lockedString, (char *)0)) == -1)
            {
                post("GrIPD: Error launching %s", pythExec);
                perror("     ");
                *(x->x_localOpened) = 0;
                exit(1);
            }
            exit(1);
        }
        else
        {
            x->x_childPID = pid;
        }
    }
#endif
    else
    {
        gripd_show(x);
    }
}

void gripd_closePyth(t_gripd *x)
{
    char buf[MAXDATASIZE];

    if (*(x->x_localOpened))
    {
        sprintf(buf, "%s%c0%c", CLOSECOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
        *(x->x_localOpened) = 0;
#ifdef NT
        TerminateProcess(x->x_childProcessInfo.hProcess, 0);
#else
        kill(x->x_childPID, SIGKILL);
        x->x_childPID = 0;
#endif
    }
}

void gripd_lock(t_gripd *x)
{
    char buf[MAXDATASIZE];

    if (x->x_connected)
    {
        sprintf(buf, "%s%c0%c", LOCKCOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
    }
}

void gripd_unlock(t_gripd *x)
{
    char buf[MAXDATASIZE];

    if (x->x_connected)
    {
        sprintf(buf, "%s%c0%c", UNLOCKCOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
    }
}

void gripd_hide(t_gripd *x)
{
    char buf[MAXDATASIZE];

    if (x->x_connected)
    {
        sprintf(buf, "%s%c0%c", HIDECOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
    }
}

void gripd_show(t_gripd *x)
{
    char buf[MAXDATASIZE];

    if (x->x_connected)
    {
        sprintf(buf, "%s%c0%c", SHOWCOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
    }
}

void gripd_openpanel(t_gripd *x)
{
    char buf[MAXDATASIZE];

    if (x->x_connected)
    {
        sprintf(buf, "%s%c0%c", OPENPANELCOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
    }
}

void gripd_savepanel(t_gripd *x)
{
    char buf[MAXDATASIZE];

    if (x->x_connected)
    {
        sprintf(buf, "%s%c0%c", SAVEPANELCOMMAND, SYMMSGSEP, PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);
    }
}

void gripd_setTitle(t_gripd *x, t_symbol *sym, int argc, t_atom *argv)
{
    int i;
    char title[DEFSTRSIZE];
    char buf[MAXDATASIZE];

    if (x->x_connected) {
        if (argc > 0)
            strcpy(title, argv[0].a_w.w_symbol->s_name);
        for (i = 1; i < argc; i++)
	    if (argv[i].a_type == A_SYMBOL)
	        sprintf(title, "%s %s", title,
			argv[i].a_w.w_symbol->s_name);
        sprintf(buf,
		"%s%c%s%c",
		SETTITLECOMMAND,
		SYMMSGSEP,
		title,
		PAIRSEPCHAR);
        send(x->x_newSock, buf, strlen(buf), MSG_DONTWAIT | MSG_NOSIGNAL);	
    }
}

void gripd_setPath(t_gripd *x, t_symbol *sym, int argc, t_atom *argv)
{
    int i;
    if (argc > 0)
        strcpy(x->x_pythFile, argv[0].a_w.w_symbol->s_name);
    for (i = 1; i < argc; i++)
        if (argv[i].a_type == A_SYMBOL)
           sprintf(x->x_pythFile, "%s %s", x->x_pythFile,
                   argv[i].a_w.w_symbol->s_name);
    sprintf(x->x_pythFile, "%s/", x->x_pythFile);
}

void gripd_setPythonPath(t_gripd *x, t_symbol *sym, int argc, t_atom *argv)
{
    int i;
    if (argc >0)
        strcpy(x->x_pythExec, argv[0].a_w.w_symbol->s_name);
    for (i = 1; i < argc; i++)
        if (argv[i].a_type == A_SYMBOL)
            sprintf(x->x_pythExec, "%s %s", x->x_pythExec,
                    argv[i].a_w.w_symbol->s_name);
    sprintf(x->x_pythExec, "%s/", x->x_pythExec);
}

void gripd_setSTime(t_gripd *x, t_floatarg val)
{
    if (val > 0)
        x->x_sdeltime = val;
    else
        post("GrIPD: Illegal update time");
}

void gripd_setRTime(t_gripd *x, t_floatarg val)
{
    if (val > 0)
        x->x_rdeltime = val;
    else
        post("GrIPD: Illegal update time");
}

void gripdR_bang(t_gripdRcvr *r)
{
    char aString[MAXDATASIZE];
    char valueString[MAXDATASIZE];
 
    strcpy(aString, r->r_sym->s_name);
    sprintf(valueString,"%cbang%c", SYMMSGSEP, PAIRSEPCHAR);
    strcat(aString, valueString);
 
    gripd_appendSendBuffer((t_gripd *)(r->r_x), aString);
}

void gripdR_float(t_gripdRcvr *r, t_float floatValue)
{
    char aString[MAXDATASIZE];
    char valueString[MAXDATASIZE];
 
    strcpy(aString, r->r_sym->s_name);
    sprintf(valueString,"%c%g%c", SYMMSGSEP, floatValue, PAIRSEPCHAR);
    strcat(aString, valueString);
 
    gripd_appendSendBuffer((t_gripd *)(r->r_x), aString);
}
 
void gripdR_symbol(t_gripdRcvr *r, t_symbol *sym)
{
    char aString[MAXDATASIZE];
    char valueString[MAXDATASIZE];
 
    strcpy(aString, r->r_sym->s_name);
    sprintf(valueString,"%c%s%c", SYMMSGSEP, sym->s_name, PAIRSEPCHAR);
    strcat(aString, valueString);
 
    gripd_appendSendBuffer((t_gripd *)(r->r_x), aString);
}

void gripdR_anything(t_gripdRcvr *r, t_symbol *sym, int argc, t_atom *argv)
{
    char aString[MAXDATASIZE];
    char valueString[MAXDATASIZE];
    int i;

    strcpy(aString, r->r_sym->s_name);
    sprintf(valueString, "%c%s", SYMMSGSEP, sym->s_name);
    strcat(aString, valueString);
    for (i = 0; i < argc; i++)
    {
        if (argv[i].a_type == A_SYMBOL)
        {
            sprintf(valueString, " %s", argv[i].a_w.w_symbol->s_name);
            strcat(aString, valueString);
        }
        else if (argv[i].a_type == A_FLOAT)
        {
            sprintf(valueString, " %g", argv[i].a_w.w_float);
            strcat(aString, valueString);
        }
    }
    sprintf(aString, "%s%c", aString, PAIRSEPCHAR);
    gripd_appendSendBuffer((t_gripd *)(r->r_x), aString);
}

void gripdR_list(t_gripdRcvr *r, t_symbol *sym, int argc, t_atom *argv)
{
    char aString[MAXDATASIZE];
    char valueString[MAXDATASIZE];
    int i;

    strcpy(aString, r->r_sym->s_name);
    sprintf(valueString, "%c", SYMMSGSEP);
    strcat(aString, valueString);    
    for (i = 0; i < argc; i++)
    {
        if (argv[i].a_type == A_SYMBOL)
        {
            sprintf(valueString, " %s", argv[i].a_w.w_symbol->s_name);
            strcat(aString, valueString);
        }
        else if (argv[i].a_type == A_FLOAT)
        {
            sprintf(valueString, " %g", argv[i].a_w.w_float);
            strcat(aString, valueString);
        }
    }
    sprintf(aString, "%s%c", aString, PAIRSEPCHAR);    
    gripd_appendSendBuffer((t_gripd *)(r->r_x), aString);
}

void gripd_makeGripdRcvr(t_gripd *x, t_symbol *s)
{
    t_gripdRcvr *r = (t_gripdRcvr *)pd_new(gripdRcvr_class);
    r->r_sym = s;
    pd_bind(&r->r_obj.ob_pd, s);
    r->r_x = (t_gripd *)x;
    x->x_rcvrs[x->x_rcvrListSize] = r;
}

int gripd_checkExistance(t_gripd *x, char *name)
{
    int i, flag;
 
    flag = 0;
    for (i = 0; i < x->x_rcvrListSize; i++)
    {
        if (strcmp(name, x->x_rcvrs[i]->r_sym->s_name) == 0)
            flag = 1;
    }
    return flag;
}

void gripd_expandRcvrList(t_gripd *x)
{
    x->x_rcvrListMaxSize *= 2;
    x->x_rcvrs = (t_gripdRcvr **)realloc(x->x_rcvrs,
                                         x->x_rcvrListMaxSize \
                                         * sizeof(t_gripdRcvr *));
}

void gripd_free(t_gripd *x)
{
    int i;
    
    if (*(x->x_localOpened))
        gripd_closePyth(x);
    if (x->x_connected)
        gripd_disconnect(x);
    else if (x->x_bound)
        gripd_closeSocket(x);
    clock_free(x->x_rclock);
    clock_free(x->x_sclock);
    clock_free(x->x_connectionClock);
    clock_free(x->x_bindClock);
#ifdef NT
    free(x->x_localOpened);
    if (x->x_wsockInitByMe != 0)
        WSACleanup();
#endif
    for (i = 0; i < x->x_rcvrListSize; i++)
        pd_unbind(&(x->x_rcvrs[i])->r_obj.ob_pd, x->x_rcvrs[i]->r_sym);
    free(x->x_rcvrs);
}

int gripd_isNumeric(char *string)
{
    if ((strspn(string, "0123456789.+-") == strlen(string)) &&
	(strchr(string, '+') == strrchr(string, '+')) &&
	(strchr(string, '-') == strrchr(string, '-')) &&
	(strchr(string, '.') == strrchr(string, '.')) &&
	(!((strchr(string, '+') != NULL) && 
         (strchr(string, '-') != NULL))))
        return 1;
    else
        return 0;
}

#ifndef NT
void gripd_sigChild(int sig)
{
    wait(NULL);
}
#endif

void gripd_getApplicationPath(t_gripd *x)
{
    char rawStr[MAXDATASIZE];
#ifdef NT
    GetModuleFileName(NULL, rawStr, sizeof(rawStr));
    rawStr[strrchr(rawStr, '\\') - rawStr + 1] = '\0';
    strcpy(x->x_appPath, rawStr);   
#else
    char *pathStr;
    FILE *fp;  
    fp = fopen("/proc/self/maps", "r");
    fgets(rawStr, MAXDATASIZE, fp);
    fclose(fp);
    pathStr = index(rawStr, '/');
    pathStr[index(pathStr, '\n') - pathStr] = '\0';
    pathStr[rindex(pathStr, '/') - pathStr + 1] = '\0';
    strcpy(x->x_appPath, pathStr);
#endif
}

