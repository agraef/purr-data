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
 *  * process_err adds error_mesg to mesg_array
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * created for API overhaul (moved from old event.c)
 */

#include <unistd.h>
#include "cwiid_internal.h"

int process_error(struct wiimote *wiimote, ssize_t len, struct mesg_array *ma)
{
	struct cwiid_error_mesg *error_mesg;

	error_mesg = &ma->array[ma->count++].error_mesg;
	error_mesg->type = CWIID_MESG_ERROR;
	if (len == 0) {
		error_mesg->error = CWIID_ERROR_DISCONNECT;
	}
	else {
		error_mesg->error = CWIID_ERROR_COMM;
	}

	if (cancel_rw(wiimote)) {
		cwiid_err(wiimote, "RW cancel error");
	}

	return 0;
}

int process_status(struct wiimote *wiimote, const unsigned char *data,
                   struct mesg_array *ma)
{
	struct cwiid_status_mesg status_mesg;

	status_mesg.type = CWIID_MESG_STATUS;
	status_mesg.battery = data[5];
	if (data[2] & 0x02) {
		/* status_thread will figure out what it is */
		status_mesg.ext_type = CWIID_EXT_UNKNOWN;
	}
	else {
		status_mesg.ext_type = CWIID_EXT_NONE;
	}

	if (write(wiimote->status_pipe[1], &status_mesg, sizeof status_mesg)
	  != sizeof status_mesg) {
		cwiid_err(wiimote, "Status pipe write error");
		return -1;
	}

	return 0;
}

int process_btn(struct wiimote *wiimote, const unsigned char *data,
                struct mesg_array *ma)
{
	struct cwiid_btn_mesg *btn_mesg;
	uint16_t buttons;

	buttons = (data[0] & BTN_MASK_0)<<8 |
	          (data[1] & BTN_MASK_1);
	if (wiimote->state.rpt_mode & CWIID_RPT_BTN) {
		if ((wiimote->state.buttons != buttons) ||
		  (wiimote->flags & CWIID_FLAG_REPEAT_BTN)) {
			btn_mesg = &ma->array[ma->count++].btn_mesg;
			btn_mesg->type = CWIID_MESG_BTN;
			btn_mesg->buttons = buttons;
		}
	}

	return 0;
}

int process_acc(struct wiimote *wiimote, const unsigned char *data,
                struct mesg_array *ma)
{
	struct cwiid_acc_mesg *acc_mesg;

	if (wiimote->state.rpt_mode & CWIID_RPT_ACC) {
		acc_mesg = &ma->array[ma->count++].acc_mesg;
		acc_mesg->type = CWIID_MESG_ACC;
		acc_mesg->acc[CWIID_X] = data[0];
		acc_mesg->acc[CWIID_Y] = data[1];
		acc_mesg->acc[CWIID_Z] = data[2];
	}

	return 0;
}

int process_ir10(struct wiimote *wiimote, const unsigned char *data,
                 struct mesg_array *ma)
{
	struct cwiid_ir_mesg *ir_mesg;
	int i;
	const unsigned char *block;

	if (wiimote->state.rpt_mode & CWIID_RPT_IR) {
		ir_mesg = &ma->array[ma->count++].ir_mesg;
		ir_mesg->type = CWIID_MESG_IR;

		for (i=0, block=data; i < CWIID_IR_SRC_COUNT; i+=2, block+=5) {
			if (block[0] == 0xFF) {
				ir_mesg->src[i].valid = 0;
			}
			else {
				ir_mesg->src[i].valid = 1;
				ir_mesg->src[i].pos[CWIID_X] = ((uint16_t)block[2] & 0x30)<<4 |
				                                (uint16_t)block[0];
				ir_mesg->src[i].pos[CWIID_Y] = ((uint16_t)block[2] & 0xC0)<<2 |
				                                (uint16_t)block[1];
				ir_mesg->src[i].size = -1;
			}

			if (block[3] == 0xFF) {
				ir_mesg->src[i+1].valid = 0;
			}
			else {
				ir_mesg->src[i+1].valid = 1;
				ir_mesg->src[i+1].pos[CWIID_X] =
				                               ((uint16_t)block[2] & 0x03)<<8 |
				                                (uint16_t)block[3];
				ir_mesg->src[i+1].pos[CWIID_Y] =
				                               ((uint16_t)block[2] & 0x0C)<<6 |
				                                (uint16_t)block[4];
				ir_mesg->src[i+1].size = -1;
			}
		}
	}

	return 0;
}

int process_ir12(struct wiimote *wiimote, const unsigned char *data,
                 struct mesg_array *ma)
{
	struct cwiid_ir_mesg *ir_mesg;
	int i;
	const unsigned char *block;

	if (wiimote->state.rpt_mode & CWIID_RPT_IR) {
		ir_mesg = &ma->array[ma->count++].ir_mesg;
		ir_mesg->type = CWIID_MESG_IR;

		for (i=0, block=data; i < CWIID_IR_SRC_COUNT; i++, block+=3) {
			if (block[0] == 0xFF) {
				ir_mesg->src[i].valid = 0;
			}
			else {
				ir_mesg->src[i].valid = 1;
				ir_mesg->src[i].pos[CWIID_X] = ((uint16_t)block[2] & 0x30)<<4 |
				                                (uint16_t)block[0];
				ir_mesg->src[i].pos[CWIID_Y] = ((uint16_t)block[2] & 0xC0)<<2 |
				                                (uint16_t)block[1];
				ir_mesg->src[i].size = block[2] & 0x0F;
			}
		}
	}

	return 0;
}

int process_ext(struct wiimote *wiimote, unsigned char *data,
                unsigned char len, struct mesg_array *ma)
{
	struct cwiid_nunchuk_mesg *nunchuk_mesg;
	struct cwiid_classic_mesg *classic_mesg;
	struct cwiid_balance_mesg *balance_mesg;
	struct cwiid_motionplus_mesg *motionplus_mesg;
	int i;

	switch (wiimote->state.ext_type) {
	case CWIID_EXT_NONE:
		cwiid_err(wiimote, "Received unexpected extension report");
		break;
	case CWIID_EXT_UNKNOWN:
		break;
	case CWIID_EXT_NUNCHUK:
		if (wiimote->state.rpt_mode & CWIID_RPT_NUNCHUK) {
			nunchuk_mesg = &ma->array[ma->count++].nunchuk_mesg;
			nunchuk_mesg->type = CWIID_MESG_NUNCHUK;
			nunchuk_mesg->stick[CWIID_X] = data[0];
			nunchuk_mesg->stick[CWIID_Y] = data[1];
			nunchuk_mesg->acc[CWIID_X] = data[2];
			nunchuk_mesg->acc[CWIID_Y] = data[3];
			nunchuk_mesg->acc[CWIID_Z] = data[4];
			nunchuk_mesg->buttons = ~data[5] & NUNCHUK_BTN_MASK;
		}
		break;
	case CWIID_EXT_CLASSIC:
		if (wiimote->state.rpt_mode & CWIID_RPT_CLASSIC) {
			classic_mesg = &ma->array[ma->count++].classic_mesg;
			classic_mesg->type = CWIID_MESG_CLASSIC;

			for (i=0; i < 6; i++) {
				data[i] = data[i];
			}

			classic_mesg->l_stick[CWIID_X] = data[0] & 0x3F;
			classic_mesg->l_stick[CWIID_Y] = data[1] & 0x3F;
			classic_mesg->r_stick[CWIID_X] = (data[0] & 0xC0)>>3 |
			                                 (data[1] & 0xC0)>>5 |
			                                 (data[2] & 0x80)>>7;
			classic_mesg->r_stick[CWIID_Y] = data[2] & 0x1F;
			classic_mesg->l = (data[2] & 0x60)>>2 |
			                  (data[3] & 0xE0)>>5;
			classic_mesg->r = data[3] & 0x1F;
			classic_mesg->buttons = ~((uint16_t)data[4]<<8 |
			                          (uint16_t)data[5]);
		}
		break;
	case CWIID_EXT_BALANCE:
		if (wiimote->state.rpt_mode & CWIID_RPT_BALANCE) {
			balance_mesg = &ma->array[ma->count++].balance_mesg;
			balance_mesg->type = CWIID_MESG_BALANCE;
			balance_mesg->right_top = ((uint16_t)data[0]<<8 |
			                           (uint16_t)data[1]);
			balance_mesg->right_bottom = ((uint16_t)data[2]<<8 |
			                              (uint16_t)data[3]);
			balance_mesg->left_top = ((uint16_t)data[4]<<8 |
			                          (uint16_t)data[5]);
			balance_mesg->left_bottom = ((uint16_t)data[6]<<8 |
			                             (uint16_t)data[7]);
		}
	case CWIID_EXT_MOTIONPLUS:
		if (wiimote->state.rpt_mode & CWIID_RPT_MOTIONPLUS) {
			motionplus_mesg = &ma->array[ma->count++].motionplus_mesg;
			motionplus_mesg->type = CWIID_MESG_MOTIONPLUS;
			motionplus_mesg->angle_rate[CWIID_PHI]   = ((uint16_t)data[5] & 0xFC)<<6 |
			                                            (uint16_t)data[2];
			motionplus_mesg->angle_rate[CWIID_THETA] = ((uint16_t)data[4] & 0xFC)<<6 |
			                                            (uint16_t)data[1];
			motionplus_mesg->angle_rate[CWIID_PSI]   = ((uint16_t)data[3] & 0xFC)<<6 |
			                                            (uint16_t)data[0];
		}
		break;
	}

	return 0;
}

int process_read(struct wiimote *wiimote, unsigned char *data)
{
	struct rw_mesg rw_mesg;

	if (wiimote->rw_status != RW_READ) {
		cwiid_err(wiimote, "Received unexpected read report");
		return -1;
	}

	rw_mesg.type = RW_READ;
	rw_mesg.len = (data[0]>>4)+1;
	rw_mesg.error = data[0] & 0x0F;
	memcpy(&rw_mesg.data, data+3, rw_mesg.len);

	if (write(wiimote->rw_pipe[1], &rw_mesg, sizeof rw_mesg) !=
	  sizeof rw_mesg) {
		cwiid_err(wiimote, "RW pipe write error");
		return -1;
	}

	return 0;
}

int process_write(struct wiimote *wiimote, unsigned char *data)
{
	struct rw_mesg rw_mesg;

	if (wiimote->rw_status != RW_WRITE) {
		cwiid_err(wiimote, "Received unexpected write report");
		return -1;
	}

	rw_mesg.type = RW_WRITE;
	rw_mesg.error = data[0];

	if (write(wiimote->rw_pipe[1], &rw_mesg, sizeof rw_mesg) !=
	  sizeof rw_mesg) {
		cwiid_err(wiimote, "RW pipe write error");
		return -1;
	}

	return 0;
}
