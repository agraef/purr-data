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
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added cwiid_request_status, cwiid_set_let, cwiid_set_rumble,
 *    cwiid_set_rpt_mode
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * rewrite for API overhaul
 *  * added rw and beep functions from rw.c
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated cwiid_read and cwiid_write to trigger and detect rw_error
 *
 *  2007-03-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * audited error checking (coda and error handler sections)
 *  * updated comments
 *  * cwiid_read - changed to obey decode flag only for register read
 *
 *  2007-03-06 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote parameter to cwiid_err calls
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "cwiid_internal.h"

int cwiid_command(cwiid_wiimote_t *wiimote, enum cwiid_command command,
                  int flags) {
	int ret;

	switch (command) {
	case CWIID_CMD_STATUS:
		ret = cwiid_request_status(wiimote);
		break;
	case CWIID_CMD_LED:
		ret = cwiid_set_led(wiimote, flags);
		break;
	case CWIID_CMD_RUMBLE:
		ret = cwiid_set_rumble(wiimote, flags);
		break;
	case CWIID_CMD_RPT_MODE:
		ret = cwiid_set_rpt_mode(wiimote, flags);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

/* TODO: fix error reporting - this is public now and
 * should report its own errors */
int cwiid_send_rpt(cwiid_wiimote_t *wiimote, uint8_t flags, uint8_t report,
                   size_t len, const void *data)
{
	unsigned char *buf;

	if ((buf = malloc((len*2) * sizeof *buf)) == NULL) {
		cwiid_err(wiimote, "Memory allocation error (mesg array)");
		return -1;
	}

	buf[0] = BT_TRANS_SET_REPORT | BT_PARAM_OUTPUT;
	buf[1] = report;
	memcpy(buf+2, data, len);
	if (!(flags & CWIID_SEND_RPT_NO_RUMBLE)) {
		buf[2] |= wiimote->state.rumble;
	}

	if (write(wiimote->ctl_socket, buf, len+2) != (ssize_t)(len+2)) {
		free(buf);
		return -1;
	}
	else if (verify_handshake(wiimote)) {
		free(buf);
		return -1;
	}

	return 0;
}

int cwiid_request_status(cwiid_wiimote_t *wiimote)
{
	unsigned char data;

	data = 0;
	if (cwiid_send_rpt(wiimote, 0, RPT_STATUS_REQ, 1, &data)) {
		cwiid_err(wiimote, "Status request error");
		return -1;
	}

	return 0;
}

int cwiid_set_led(cwiid_wiimote_t *wiimote, uint8_t led)
{
	unsigned char data;

	/* TODO: assumption: char assignments are atomic, no mutex lock needed */
	wiimote->state.led = led & 0x0F;
	data = wiimote->state.led << 4;
	if (cwiid_send_rpt(wiimote, 0, RPT_LED_RUMBLE, 1, &data)) {
		cwiid_err(wiimote, "Report send error (led)");
		return -1;
	}

	return 0;
}

int cwiid_set_rumble(cwiid_wiimote_t *wiimote, uint8_t rumble)
{
	unsigned char data;

	/* TODO: assumption: char assignments are atomic, no mutex lock needed */
	wiimote->state.rumble = rumble ? 1 : 0;
	data = wiimote->state.led << 4;
	if (cwiid_send_rpt(wiimote, 0, RPT_LED_RUMBLE, 1, &data)) {
		cwiid_err(wiimote, "Report send error (led)");
		return -1;
	}

	return 0;
}

int cwiid_set_rpt_mode(cwiid_wiimote_t *wiimote, uint8_t rpt_mode)
{
	return update_rpt_mode(wiimote, rpt_mode);
}

#define RPT_READ_REQ_LEN 6
int cwiid_read(cwiid_wiimote_t *wiimote, uint8_t flags, uint32_t offset,
               uint16_t len, void *data)
{
	unsigned char buf[RPT_READ_REQ_LEN];
	struct rw_mesg mesg;
	unsigned char *cursor;
	int ret = 0;

	/* Compose read request packet */
	buf[0]=flags & (CWIID_RW_EEPROM | CWIID_RW_REG);
	buf[1]=(unsigned char)((offset>>16) & 0xFF);
	buf[2]=(unsigned char)((offset>>8) & 0xFF);
	buf[3]=(unsigned char)(offset & 0xFF);
	buf[4]=(unsigned char)((len>>8) & 0xFF);
	buf[5]=(unsigned char)(len & 0xFF);

	/* Lock wiimote rw access */
	if (pthread_mutex_lock(&wiimote->rw_mutex)) {
		cwiid_err(wiimote, "Mutex lock error (rw_mutex)");
		return -1;
	}

	/* Setup read info */
	wiimote->rw_status = RW_READ;

	/* TODO: Document: user is responsible for ensuring that read/write
	 * operations are not in flight while disconnecting.  Nothing serious,
	 * just accesses to freed memory */
	/* Send read request packet */
	if (cwiid_send_rpt(wiimote, 0, RPT_READ_REQ, RPT_READ_REQ_LEN, buf)) {
		cwiid_err(wiimote, "Report send error (read)");
		ret = -1;
		goto CODA;
	}

	/* TODO:Better sanity checks (offset) */
	/* Read packets */
	for (cursor = data; cursor - (unsigned char *)data < len;
	     cursor += mesg.len) {
		if (full_read(wiimote->rw_pipe[0], &mesg, sizeof mesg)) {
			cwiid_err(wiimote, "Pipe read error (rw pipe)");
			ret = -1;
			goto CODA;
		}

		if (mesg.type == RW_CANCEL) {
			ret = -1;
			goto CODA;
		}
		else if (mesg.type != RW_READ) {
			cwiid_err(wiimote, "Unexpected write message");
			ret = -1;
			goto CODA;
		}

		if (mesg.error) {
			cwiid_err(wiimote, "Wiimote read error");
			ret = -1;
			goto CODA;
		}

		memcpy(cursor, &mesg.data, mesg.len);
	}

CODA:
	/* Clear rw_status */
	wiimote->rw_status = RW_IDLE;

	/* Unlock rw_mutex */
	if (pthread_mutex_unlock(&wiimote->rw_mutex)) {
		cwiid_err(wiimote, "Mutex unlock error (rw_mutex) - deadlock warning");
	}

	return ret;
}

#define RPT_WRITE_LEN 21
int cwiid_write(cwiid_wiimote_t *wiimote, uint8_t flags, uint32_t offset,
                  uint16_t len, const void *data)
{
	unsigned char buf[RPT_WRITE_LEN];
	uint16_t sent=0;
	struct rw_mesg mesg;
	int ret = 0;

	/* Compose write packet header */
	buf[0]=flags;

	/* Lock wiimote rw access */
	if (pthread_mutex_lock(&wiimote->rw_mutex)) {
		cwiid_err(wiimote, "Mutex lock error (rw mutex)");
		return -1;
	}

	/* Send packets */
	wiimote->rw_status = RW_WRITE;
	while (sent<len) {
		/* Compose write packet */
		buf[1]=(unsigned char)(((offset+sent)>>16) & 0xFF);
		buf[2]=(unsigned char)(((offset+sent)>>8) & 0xFF);
		buf[3]=(unsigned char)((offset+sent) & 0xFF);
		if (len-sent >= 0x10) {
			buf[4]=(unsigned char)0x10;
		}
		else {
			buf[4]=(unsigned char)(len-sent);
		}
		memcpy(buf+5, data+sent, buf[4]);

		if (cwiid_send_rpt(wiimote, 0, RPT_WRITE, RPT_WRITE_LEN, buf)) {
			cwiid_err(wiimote, "Report send error (write)");
			ret = -1;
			goto CODA;
		}

		/* Read packets from pipe */
		if (read(wiimote->rw_pipe[0], &mesg, sizeof mesg) != sizeof mesg) {
			cwiid_err(wiimote, "Pipe read error (rw pipe)");
			ret = -1;
			goto CODA;
		}

		if (mesg.type == RW_CANCEL) {
			ret = -1;
			goto CODA;
		}
		else if (mesg.type != RW_WRITE) {
			cwiid_err(wiimote, "Unexpected read message");
			ret = -1;
			goto CODA;
		}

		if (mesg.error) {
			cwiid_err(wiimote, "Wiimote write error");
			ret = -1;
			goto CODA;
		};

		sent+=buf[4];
	}

CODA:
	/* Clear rw_status */
	wiimote->rw_status = RW_IDLE;

	/* Unlock rw_mutex */
	if (pthread_mutex_unlock(&wiimote->rw_mutex)) {
		cwiid_err(wiimote, "Mutex unlock error (rw_mutex) - deadlock warning");
	}

	return ret;
}


struct write_seq speaker_enable_seq[] = {
	{WRITE_SEQ_RPT, RPT_SPEAKER_ENABLE, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT,   RPT_SPEAKER_MUTE, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xA20009, (const void *)"\x01", 1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xA20001, (const void *)"\x08", 1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xA20001, (const void *)"\x00\x00\x00\x0C\x40\x00\x00",
	                          7, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xA20008, (const void *)"\x01", 1, CWIID_RW_REG},
	{WRITE_SEQ_RPT,   RPT_SPEAKER_MUTE, (const void *)"\x00", 1, 0}
};

struct write_seq speaker_disable_seq[] = {
	{WRITE_SEQ_RPT,   RPT_SPEAKER_MUTE, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_SPEAKER_ENABLE, (const void *)"\x00", 1, 0}
};

#define SOUND_BUF_LEN	21
int cwiid_beep(cwiid_wiimote_t *wiimote)
{
	/* unsigned char buf[SOUND_BUF_LEN] = { 0xA0, 0xCC, 0x33, 0xCC, 0x33,
	    0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33,
	    0xCC, 0x33, 0xCC, 0x33}; */
	unsigned char buf[SOUND_BUF_LEN] = { 0xA0, 0xC3, 0xC3, 0xC3, 0xC3,
	    0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3,
	    0xC3, 0xC3, 0xC3, 0xC3};
	int i;
	int ret = 0;
	pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;
	struct timespec t;

	if (exec_write_seq(wiimote, SEQ_LEN(speaker_enable_seq),
	                   speaker_enable_seq)) {
		cwiid_err(wiimote, "Speaker enable error");
		ret = -1;
	}

	pthread_mutex_lock(&timer_mutex);

	for (i=0; i<100; i++) {
		clock_gettime(CLOCK_REALTIME, &t);
		t.tv_nsec += 10204081;
		/* t.tv_nsec += 7000000; */
		if (cwiid_send_rpt(wiimote, 0, RPT_SPEAKER_DATA, SOUND_BUF_LEN, buf)) {
		 	printf("%d\n", i);
			cwiid_err(wiimote, "Report send error (speaker data)");
			ret = -1;
			break;
		}
		/* TODO: I should be shot for this, but hey, it works.
		 * longterm - find a better wait */
		pthread_cond_timedwait(&timer_cond, &timer_mutex, &t);
	}

	pthread_mutex_unlock(&timer_mutex);

	if (exec_write_seq(wiimote, SEQ_LEN(speaker_disable_seq),
	                   speaker_disable_seq)) {
		cwiid_err(wiimote, "Speaker disable error");
		ret = -1;
	}

	return ret;
}
