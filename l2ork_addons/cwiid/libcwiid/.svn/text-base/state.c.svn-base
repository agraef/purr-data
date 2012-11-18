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
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * created for API overhaul (update_rpt_mode moved from command.c)
 */

#include <string.h>
#include <pthread.h>
#include "cwiid_internal.h"

int update_state(struct wiimote *wiimote, struct mesg_array *ma)
{
	int i;
	union cwiid_mesg *mesg;

	if (pthread_mutex_lock(&wiimote->state_mutex)) {
		cwiid_err(wiimote, "Mutex lock error (state mutex)");
		return -1;
	}

	for (i=0; i < ma->count; i++) {
		mesg = &ma->array[i];

		switch (mesg->type) {
		case CWIID_MESG_STATUS:
			wiimote->state.battery = mesg->status_mesg.battery;
			if (wiimote->state.ext_type != mesg->status_mesg.ext_type) {
				memset(&wiimote->state.ext, 0, sizeof wiimote->state.ext);
				wiimote->state.ext_type = mesg->status_mesg.ext_type;
			}
			break;
		case CWIID_MESG_BTN:
			wiimote->state.buttons = mesg->btn_mesg.buttons;
			break;
		case CWIID_MESG_ACC:
			memcpy(wiimote->state.acc, mesg->acc_mesg.acc,
			       sizeof wiimote->state.acc);
			break;
		case CWIID_MESG_IR:
			memcpy(wiimote->state.ir_src, mesg->ir_mesg.src,
			       sizeof wiimote->state.ir_src);
			break;
		case CWIID_MESG_NUNCHUK:
			memcpy(wiimote->state.ext.nunchuk.stick,
			       mesg->nunchuk_mesg.stick,
			       sizeof wiimote->state.ext.nunchuk.stick);
			memcpy(wiimote->state.ext.nunchuk.acc,
			       mesg->nunchuk_mesg.acc,
			       sizeof wiimote->state.ext.nunchuk.acc);
			wiimote->state.ext.nunchuk.buttons = mesg->nunchuk_mesg.buttons;
			break;
		case CWIID_MESG_CLASSIC:
			memcpy(wiimote->state.ext.classic.l_stick,
			       mesg->classic_mesg.l_stick,
			       sizeof wiimote->state.ext.classic.l_stick);
			memcpy(wiimote->state.ext.classic.r_stick,
			       mesg->classic_mesg.r_stick,
			       sizeof wiimote->state.ext.classic.r_stick);
			wiimote->state.ext.classic.l = mesg->classic_mesg.l;
			wiimote->state.ext.classic.r = mesg->classic_mesg.r;
			wiimote->state.ext.classic.buttons = mesg->classic_mesg.buttons;
			break;
		case CWIID_MESG_BALANCE:
			wiimote->state.ext.balance.right_top = mesg->balance_mesg.right_top;
			wiimote->state.ext.balance.right_bottom = mesg->balance_mesg.right_bottom;
			wiimote->state.ext.balance.left_top = mesg->balance_mesg.left_top;
			wiimote->state.ext.balance.left_bottom = mesg->balance_mesg.left_bottom;
			break;
		case CWIID_MESG_MOTIONPLUS:
			memcpy(wiimote->state.ext.motionplus.angle_rate,
			       mesg->motionplus_mesg.angle_rate,
			       sizeof wiimote->state.ext.motionplus.angle_rate);
			break;
		case CWIID_MESG_ERROR:
			wiimote->state.error = mesg->error_mesg.error;
			break;
		case CWIID_MESG_UNKNOWN:
			/* do nothing, error has already been printed */
			break;
		}
	}

	if (pthread_mutex_unlock(&wiimote->state_mutex)) {
		cwiid_err(wiimote, "Mutex unlock error (state mutex) - "
		                   "deadlock warning");
		return -1;
	}

	return 0;
}

/* IR Sensitivity Block */
unsigned char ir_block1[] = CLIFF_IR_BLOCK_1;
unsigned char ir_block2[] = CLIFF_IR_BLOCK_2;

struct write_seq ir_enable10_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xB00030, (const void *)"\x08", 1,     CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00000, ir_block1, sizeof(ir_block1)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB0001A, ir_block2, sizeof(ir_block2)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00033, (const void *)"\x01", 1,     CWIID_RW_REG}
};

struct write_seq ir_enable12_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xB00030, (const void *)"\x08", 1,     CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00000, ir_block1, sizeof(ir_block1)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB0001A, ir_block2, sizeof(ir_block2)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00033, (const void *)"\x03", 1,     CWIID_RW_REG}
};

struct write_seq ir_disable_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x00", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x00", 1, 0}
};

#define RPT_MODE_BUF_LEN 2
int update_rpt_mode(struct wiimote *wiimote, int8_t rpt_mode)
{
	unsigned char buf[RPT_MODE_BUF_LEN];
	uint8_t rpt_type;
	struct write_seq *ir_enable_seq;
	int seq_len;

	/* rpt_mode = bitmask of requested report types */
	/* rpt_type = report id sent to the wiimote */
	if (pthread_mutex_lock(&wiimote->rpt_mutex)) {
		cwiid_err(wiimote, "Mutex lock error (rpt mutex)");
		return -1;
	}

	/* -1 updates the reporting mode using old rpt_mode
	 * (reporting type may change if extensions are
	 * plugged in/unplugged */
	if (rpt_mode == -1) {
		rpt_mode = wiimote->state.rpt_mode;
	}

	/* Pick a report mode based on report flags */
	if ((rpt_mode & CWIID_RPT_EXT) &&
	  ((wiimote->state.ext_type == CWIID_EXT_NUNCHUK) ||
	   (wiimote->state.ext_type == CWIID_EXT_CLASSIC) ||
	   (wiimote->state.ext_type == CWIID_EXT_MOTIONPLUS))) {
		if ((rpt_mode & CWIID_RPT_IR) && (rpt_mode & CWIID_RPT_ACC)) {
			rpt_type = RPT_BTN_ACC_IR10_EXT6;
			ir_enable_seq = ir_enable10_seq;
			seq_len = SEQ_LEN(ir_enable10_seq);
		}
		else if (rpt_mode & CWIID_RPT_IR) {
			rpt_type = RPT_BTN_IR10_EXT9;
			ir_enable_seq = ir_enable10_seq;
			seq_len = SEQ_LEN(ir_enable10_seq);
		}
		else if (rpt_mode & CWIID_RPT_ACC) {
			rpt_type = RPT_BTN_ACC_EXT16;
		}
		else if (rpt_mode & CWIID_RPT_BTN) {
			rpt_type = RPT_BTN_EXT8;
		}
		else {
			rpt_type = RPT_EXT21;
		}	
	}
	else if ((rpt_mode & CWIID_RPT_EXT) &&
	  wiimote->state.ext_type == CWIID_EXT_BALANCE) {
		rpt_type = RPT_BTN_EXT8;
	}
	else {
		if (rpt_mode & CWIID_RPT_IR) {
			rpt_type = RPT_BTN_ACC_IR12;
			ir_enable_seq = ir_enable12_seq;
			seq_len = SEQ_LEN(ir_enable12_seq);
		}
		else if (rpt_mode & CWIID_RPT_ACC) {
			rpt_type = RPT_BTN_ACC;
		}
		else {
			rpt_type = RPT_BTN;
		}
	}

	/* Enable IR */
	/* TODO: only do this when necessary (record old IR mode) */
	if ((rpt_mode & CWIID_RPT_IR)) {
		if (exec_write_seq(wiimote, seq_len, ir_enable_seq)) {
			cwiid_err(wiimote, "IR enable error");
			return -1;
		}
	}
	/* Disable IR */
	else if ((wiimote->state.rpt_mode & CWIID_RPT_IR) &&
	         !(rpt_mode & CWIID_RPT_IR)) {
		if (exec_write_seq(wiimote, SEQ_LEN(ir_disable_seq), ir_disable_seq)) {
			cwiid_err(wiimote, "IR disable error");
			return -1;
		}
	}

	/* Send SET_REPORT */
	buf[0] = (wiimote->flags & CWIID_FLAG_CONTINUOUS) ? 0x04 : 0;
	buf[1] = rpt_type;
	if (cwiid_send_rpt(wiimote, 0, RPT_RPT_MODE, RPT_MODE_BUF_LEN, buf)) {
		cwiid_err(wiimote, "Send report error (report mode)");
		return -1;
	}

	/* clear state for unreported data */
	if (CWIID_RPT_BTN & ~rpt_mode & wiimote->state.rpt_mode) {
		wiimote->state.buttons = 0;
	}
	if (CWIID_RPT_ACC & ~rpt_mode & wiimote->state.rpt_mode) {
		memset(wiimote->state.acc, 0, sizeof wiimote->state.acc);
	}
	if (CWIID_RPT_IR & ~rpt_mode & wiimote->state.rpt_mode) {
		memset(wiimote->state.ir_src, 0, sizeof wiimote->state.ir_src);
	}
	if ((wiimote->state.ext_type == CWIID_EXT_NUNCHUK) &&
	  (CWIID_RPT_NUNCHUK & ~rpt_mode & wiimote->state.rpt_mode)) {
		memset(&wiimote->state.ext, 0, sizeof wiimote->state.ext);
	}
	else if ((wiimote->state.ext_type == CWIID_EXT_CLASSIC) &&
	  (CWIID_RPT_CLASSIC & ~rpt_mode & wiimote->state.rpt_mode)) {
		memset(&wiimote->state.ext, 0, sizeof wiimote->state.ext);
	}
	else if ((wiimote->state.ext_type == CWIID_EXT_BALANCE) &&
	  (CWIID_RPT_BALANCE & ~rpt_mode & wiimote->state.rpt_mode)) {
		memset(&wiimote->state.ext, 0, sizeof wiimote->state.ext);
	}
	else if ((wiimote->state.ext_type == CWIID_EXT_MOTIONPLUS) &&
	  (CWIID_RPT_MOTIONPLUS & ~rpt_mode & wiimote->state.rpt_mode)) {
		memset(&wiimote->state.ext, 0, sizeof wiimote->state.ext);
	}

	wiimote->state.rpt_mode = rpt_mode;

	if (pthread_mutex_unlock(&wiimote->rpt_mutex)) {
		cwiid_err(wiimote, "Mutex unlock error (rpt mutex) - "
		          "deadlock warning");
		return -1;
	}

	return 0;
}
