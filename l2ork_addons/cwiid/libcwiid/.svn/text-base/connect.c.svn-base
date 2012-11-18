/* Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ChangeLog:
 *  2009-06-13 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * add cwiid_open_timeout function, call it with default from cwiid_open
 *
 *  2007-06-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added sleep after cwiid_find_wiimote call
 *
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * remove error_pipe init and destruct
 *  * renamed connect and disconnect to open and close
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * rewrite for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * cancel rw operations from cwiid_disconnect
 *
 *  2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * cwiid_connect now takes a pointer to bdaddr_t
 *  * changed cwiid_findfirst to cwiid_find_wiimote
 *
 *  2007-03-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed memcpy to bacmp
 *  * audited error checking (coda and error handler sections)
 *  * updated comments
 *
 *  2007-03-06 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote parameter to cwiid_err calls
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include "cwiid_internal.h"

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static int wiimote_id = 0;

/* TODO: Turn this onto a macro on next major so version */
cwiid_wiimote_t *cwiid_open(bdaddr_t *bdaddr, int flags)
{
	return cwiid_open_timeout(bdaddr, flags, 5);
}

cwiid_wiimote_t *cwiid_open_timeout(bdaddr_t *bdaddr, int flags, int timeout)
{
	struct wiimote *wiimote = NULL;
	struct sockaddr_l2 remote_addr;
	char mesg_pipe_init = 0, status_pipe_init = 0, rw_pipe_init = 0,
	     state_mutex_init = 0, rw_mutex_init = 0, rpt_mutex_init = 0,
	     router_thread_init = 0, status_thread_init = 0;
	void *pthread_ret;

	/* Allocate wiimote */
	if ((wiimote = malloc(sizeof *wiimote)) == NULL) {
		cwiid_err(NULL, "Memory allocation error (cwiid_wiimote_t)");
		goto ERR_HND;
	}

	/* set flags */
	wiimote->flags = flags;

	/* For error detection */
	wiimote->ctl_socket = wiimote->int_socket = -1;

	/* Global Lock, Store and Increment wiimote_id */
	if (pthread_mutex_lock(&global_mutex)) {
		cwiid_err(NULL, "Mutex lock error (global mutex)");
		goto ERR_HND;
	}
	wiimote->id = wiimote_id++;
	if (pthread_mutex_unlock(&global_mutex)) {
		cwiid_err(wiimote, "Mutex unlock error (global mutex) - "
		                   "deadlock warning");
		goto ERR_HND;
	}

	/* If BDADDR_ANY is given, find available wiimote */
	if (bacmp(bdaddr, BDADDR_ANY) == 0) {
		if (cwiid_find_wiimote(bdaddr, timeout)) {
			goto ERR_HND;
		}
		sleep(1);
	}

	/* Connect to Wiimote */
	/* Control Channel */
	memset(&remote_addr, 0, sizeof remote_addr);
	remote_addr.l2_family = AF_BLUETOOTH;
	remote_addr.l2_bdaddr = *bdaddr;
	remote_addr.l2_psm = htobs(CTL_PSM);
	if ((wiimote->ctl_socket =
	  socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)) == -1) {
		cwiid_err(wiimote, "Socket creation error (control socket)");
		goto ERR_HND;
	}
	if (connect(wiimote->ctl_socket, (struct sockaddr *)&remote_addr,
		        sizeof remote_addr)) {
		cwiid_err(wiimote, "Socket connect error (control channel)");
		goto ERR_HND;
	}

	/* Interrupt Channel */
	remote_addr.l2_psm = htobs(INT_PSM);
	if ((wiimote->int_socket =
	  socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)) == -1) {
		cwiid_err(wiimote, "Socket creation error (interrupt socket)");
		goto ERR_HND;
	}
	if (connect(wiimote->int_socket, (struct sockaddr *)&remote_addr,
		        sizeof remote_addr)) {
		cwiid_err(wiimote, "Socket connect error (interrupt channel)");
		goto ERR_HND;
	}

	/* Create pipes */
	if (pipe(wiimote->mesg_pipe)) {
		cwiid_err(wiimote, "Pipe creation error (mesg pipe)");
		goto ERR_HND;
	}
	mesg_pipe_init = 1;
	if (pipe(wiimote->status_pipe)) {
		cwiid_err(wiimote, "Pipe creation error (status pipe)");
		goto ERR_HND;
	}
	status_pipe_init = 1;
	if (pipe(wiimote->rw_pipe)) {
		cwiid_err(wiimote, "Pipe creation error (rw pipe)");
		goto ERR_HND;
	}
	rw_pipe_init = 1;

	/* Setup blocking */
	if (fcntl(wiimote->mesg_pipe[1], F_SETFL, O_NONBLOCK)) {
		cwiid_err(wiimote, "File control error (mesg write pipe)");
		goto ERR_HND;
	}
	if (wiimote->flags & CWIID_FLAG_NONBLOCK) {
		if (fcntl(wiimote->mesg_pipe[0], F_SETFL, O_NONBLOCK)) {
			cwiid_err(wiimote, "File control error (mesg read pipe)");
			goto ERR_HND;
		}
	}

	/* Init mutexes */
	if (pthread_mutex_init(&wiimote->state_mutex, NULL)) {
		cwiid_err(wiimote, "Mutex initialization error (state mutex)");
		goto ERR_HND;
	}
	state_mutex_init = 1;
	if (pthread_mutex_init(&wiimote->rw_mutex, NULL)) {
		cwiid_err(wiimote, "Mutex initialization error (rw mutex)");
		goto ERR_HND;
	}
	rw_mutex_init = 1;
	if (pthread_mutex_init(&wiimote->rpt_mutex, NULL)) {
		cwiid_err(wiimote, "Mutex initialization error (rpt mutex)");
		goto ERR_HND;
	}
	rpt_mutex_init = 1;

	/* Set rw_status before starting router thread */
	wiimote->rw_status = RW_IDLE;

	/* Launch interrupt channel listener and dispatch threads */
	if (pthread_create(&wiimote->router_thread, NULL,
	                   (void *(*)(void *))&router_thread, wiimote)) {
		cwiid_err(wiimote, "Thread creation error (router thread)");
		goto ERR_HND;
	}
	router_thread_init = 1;
	if (pthread_create(&wiimote->status_thread, NULL,
	                   (void *(*)(void *))&status_thread, wiimote)) {
		cwiid_err(wiimote, "Thread creation error (status thread)");
		goto ERR_HND;
	}
	status_thread_init = 1;

	/* Success!  Update state */
	memset(&wiimote->state, 0, sizeof wiimote->state);
	wiimote->mesg_callback = NULL;
	cwiid_set_led(wiimote, 0);
	cwiid_request_status(wiimote);

	return wiimote;

ERR_HND:
	if (wiimote) {
		/* Close threads */
		if (router_thread_init) {
			pthread_cancel(wiimote->router_thread);
			if (pthread_join(wiimote->router_thread, &pthread_ret)) {
				cwiid_err(wiimote, "Thread join error (router thread)");
			}
			else if (!((pthread_ret == PTHREAD_CANCELED) &&
			         (pthread_ret == NULL))) {
				cwiid_err(wiimote, "Bad return value from router thread");
			}
		}

		if (status_thread_init) {
			pthread_cancel(wiimote->status_thread);
			if (pthread_join(wiimote->status_thread, &pthread_ret)) {
				cwiid_err(wiimote, "Thread join error (status thread)");
			}
			else if (!((pthread_ret == PTHREAD_CANCELED) && (pthread_ret == NULL))) {
				cwiid_err(wiimote, "Bad return value from status thread");
			}
		}

		/* Close Sockets */
		if (wiimote->int_socket != -1) {
			if (close(wiimote->int_socket)) {
				cwiid_err(wiimote, "Socket close error (interrupt channel)");
			}
		}
		if (wiimote->ctl_socket != -1) {
			if (close(wiimote->ctl_socket)) {
				cwiid_err(wiimote, "Socket close error (control channel)");
			}
		}
		/* Close Pipes */
		if (mesg_pipe_init) {
			if (close(wiimote->mesg_pipe[0]) || close(wiimote->mesg_pipe[1])) {
				cwiid_err(wiimote, "Pipe close error (mesg pipe)");
			}
		}
		if (status_pipe_init) {
			if (close(wiimote->status_pipe[0]) ||
			  close(wiimote->status_pipe[1])) {
				cwiid_err(wiimote, "Pipe close error (status pipe)");
			}
		}
		if (rw_pipe_init) {
			if (close(wiimote->rw_pipe[0]) || close(wiimote->rw_pipe[1])) {
				cwiid_err(wiimote, "Pipe close error (rw pipe)");
			}
		}
		/* Destroy Mutexes */
		if (state_mutex_init) {
			if (pthread_mutex_destroy(&wiimote->state_mutex)) {
				cwiid_err(wiimote, "Mutex destroy error (state mutex)");
			}
		}
		if (rw_mutex_init) {
			if (pthread_mutex_destroy(&wiimote->rw_mutex)) {
				cwiid_err(wiimote, "Mutex destroy error (rw mutex)");
			}
		}
		if (rpt_mutex_init) {
			if (pthread_mutex_destroy(&wiimote->rpt_mutex)) {
				cwiid_err(wiimote, "Mutex destroy error (rpt mutex)");
			}
		}
		free(wiimote);
	}
	return NULL;
}

int cwiid_close(cwiid_wiimote_t *wiimote)
{
	void *pthread_ret;

	/* Cancel and join router_thread and status_thread */
	if (pthread_cancel(wiimote->router_thread)) {
		/* if thread quit abnormally, would have printed it's own error */
	}
	if (pthread_join(wiimote->router_thread, &pthread_ret)) {
		cwiid_err(wiimote, "Thread join error (router thread)");
	}
	else if (!((pthread_ret == PTHREAD_CANCELED) || (pthread_ret == NULL))) {
		cwiid_err(wiimote, "Bad return value from router thread");
	}

	if (pthread_cancel(wiimote->status_thread)) {
		/* if thread quit abnormally, would have printed it's own error */
	}
	if (pthread_join(wiimote->status_thread, &pthread_ret)) {
		cwiid_err(wiimote, "Thread join error (status thread)");
	}
	else if (!((pthread_ret == PTHREAD_CANCELED) || (pthread_ret == NULL))) {
		cwiid_err(wiimote, "Bad return value from status thread");
	}

	if (wiimote->mesg_callback) {
		if (cancel_mesg_callback(wiimote)) {
			/* prints it's own errors */
		}
	}

	if (cancel_rw(wiimote)) {
		/* prints it's own errors */
	}

	/* Close sockets */
	if (close(wiimote->int_socket)) {
		cwiid_err(wiimote, "Socket close error (interrupt channel)");
	}
	if (close(wiimote->ctl_socket)) {
		cwiid_err(wiimote, "Socket close error (control channel)");
	}
	/* Close Pipes */
	if (close(wiimote->mesg_pipe[0]) || close(wiimote->mesg_pipe[1])) {
		cwiid_err(wiimote, "Pipe close error (mesg pipe)");
	}
	if (close(wiimote->status_pipe[0]) || close(wiimote->status_pipe[1])) {
		cwiid_err(wiimote, "Pipe close error (status pipe)");
	}
	if (close(wiimote->rw_pipe[0]) || close(wiimote->rw_pipe[1])) {
		cwiid_err(wiimote, "Pipe close error (rw pipe)");
	}
	/* Destroy mutexes */
	if (pthread_mutex_destroy(&wiimote->state_mutex)) {
		cwiid_err(wiimote, "Mutex destroy error (state)");
	}
	if (pthread_mutex_destroy(&wiimote->rw_mutex)) {
		cwiid_err(wiimote, "Mutex destroy error (rw)");
	}
	if (pthread_mutex_destroy(&wiimote->rpt_mutex)) {
		cwiid_err(wiimote, "Mutex destroy error (rpt)");
	}

	free(wiimote);

	return 0;
}
