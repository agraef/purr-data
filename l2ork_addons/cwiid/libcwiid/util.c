/* Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
 * Copyright (C) 2011-2015 Ivica Ico Bukvic <ico@vt.edu> and Deba Pratim Saha <dpsaha@vt.edu>
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
 *	2015-09-17 Ivica Ico Bukvic <ico@vt.edu>
 * * Added Wii MotionPlus Inside support, thereby completing support for all known Wii devices
 * * Version bump to 0.7.00
 * * Updated build and contact info
 *
 *  2008-08-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * make cwiid_err_default public
 *  * clean up cwiid_err
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * rewrite for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-08 Petter Reinholdtsen <pere@hungry.com>
 *  * fixed signed/unsigned comparison warning in send_report and
 *    exec_write_seq
 *
 *  2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * removed cwiid_findfirst (moved to bluetooth.c)
 *
 *  2007-03-27 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * moved cwiid_findfirst to bluetooth.c
 *
 *  2007-03-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * audited error checking (coda and error handler sections)
 *
 *  2007-03-05 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * created cwiid_err_func variable
 *  * created cwiid_err_default
 *  * added wiimote parameter to cwiid_err definition and calls
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "cwiid_internal.h"

cwiid_err_t cwiid_err_default;

static cwiid_err_t *cwiid_err_func = &cwiid_err_default;

int cwiid_set_err(cwiid_err_t *err)
{
	/* TODO: assuming pointer assignment is atomic operation */
	/* if it is, and the user doesn't care about race conditions, we don't
	 * either */
	cwiid_err_func = err;
	return 0;
}

void cwiid_err_default(struct wiimote *wiimote, const char *str, va_list ap)
{
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");
}

void cwiid_err(struct wiimote *wiimote, const char *str, ...)
{
	va_list ap;

	if (cwiid_err_func) {
		va_start(ap, str);
		(*cwiid_err_func)(wiimote, str, ap);
		va_end(ap);
	}
}

/*
int verify_handshake(struct wiimote *wiimote)
{
	// disabled because we don't use ctl_socket any more
	unsigned char handshake;
	if (read(wiimote->ctl_socket, &handshake, 1) != 1) {
		cwiid_err(wiimote, "Socket read error (handshake)");
		return -1;
	}
	else if ((handshake & BT_TRANS_MASK) != BT_TRANS_HANDSHAKE) {
		cwiid_err(wiimote, "Handshake expected, non-handshake received");
		return -1;
	}
	else if ((handshake & BT_PARAM_MASK) != BT_PARAM_SUCCESSFUL) {
		cwiid_err(wiimote, "Non-successful handshake");
		return -1;
	}

	return 0;
}
*/

int exec_write_seq(struct wiimote *wiimote, unsigned int len,
                   struct write_seq *seq)
{
	unsigned int i;

	for (i=0; i < len; i++) {
		switch (seq[i].type) {
		case WRITE_SEQ_RPT:
			if (cwiid_send_rpt(wiimote, seq[i].flags, seq[i].report_offset,
			                   seq[i].len, seq[i].data)) {
				return -1;
			}
			break;
		case WRITE_SEQ_MEM:
			if (cwiid_write(wiimote, seq[i].flags, seq[i].report_offset,
			                seq[i].len, seq[i].data)) {
				return -1;
			}
			break;
		}
	}

	return 0;
}

int full_read(int fd, void *buf, size_t len)
{
	ssize_t last_len = 0;

	do {
		if ((last_len = read(fd, buf, len)) == -1) {
			return -1;
		}
		len -= last_len;
		buf += last_len;
	} while (len > 0);

	return 0;
}

int write_mesg_array(struct wiimote *wiimote, struct mesg_array *ma)
{
	ssize_t len = (void *)&ma->array[ma->count] - (void *)ma;
	int ret = 0;

	/* This must remain a single write operation to ensure atomicity,
	 * which is required to avoid mutexes and cancellation issues */
	if (write(wiimote->mesg_pipe[1], ma, len) != len) {
		if (errno == EAGAIN) {
			cwiid_err(wiimote, "Mesg pipe overflow");
			if (fcntl(wiimote->mesg_pipe[1], F_SETFL, 0)) {
				cwiid_err(wiimote, "File control error (mesg pipe)");
				ret = -1;
			}
			else {
				if (write(wiimote->mesg_pipe[1], ma, len) != len) {
					cwiid_err(wiimote, "Pipe write error (mesg pipe)");
					ret = -1;
				}
				if (fcntl(wiimote->mesg_pipe[1], F_SETFL, O_NONBLOCK)) {
					cwiid_err(wiimote, "File control error (mesg pipe");
				}
			}
		}
		else {
			cwiid_err(wiimote, "Pipe write error (mesg pipe)");
			ret = -1;
		}
	}

	return ret;
}

int read_mesg_array(int fd, struct mesg_array *ma)
{
	ssize_t len;

	len = (void *)&ma->array[0] - (void *)ma;
	if (full_read(fd, ma, len)) {
		return -1;
	}

	len = ma->count * sizeof ma->array[0];
	if (full_read(fd, &ma->array[0], len)) {
		return -1;
	}

	return 0;
}

int cancel_rw(struct wiimote *wiimote)
{
	struct rw_mesg rw_mesg;

	rw_mesg.type = RW_CANCEL;

	if (write(wiimote->rw_pipe[1], &rw_mesg, sizeof rw_mesg) !=
	  sizeof rw_mesg) {
		cwiid_err(wiimote, "Pipe write error (rw)");
		return -1;
	}

	return 0;
}

int cancel_mesg_callback(struct wiimote *wiimote)
{
	int ret = 0;

	if (pthread_cancel(wiimote->mesg_callback_thread)) {
		cwiid_err(wiimote, "Thread cancel error (callback thread)");
		ret = -1;
	}

	if (pthread_detach(wiimote->mesg_callback_thread)) {
		cwiid_err(wiimote, "Thread detach error (callback thread)");
		ret = -1;
	}

	return ret;
}
