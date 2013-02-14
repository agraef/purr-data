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
 *  2012-04-11 Deba Pratim Saha <dpsaha@vt.edu> and Ivica Ico Bukvic <ico@vt.edu>
 * * implemented passthrough toggle
 * * implemented Mplus + Nunchuk passthrough support
 * * removed balance board from the CWIID_RPT_EXT define as that limited incoming streams to 2 (bug)
 * * various clean-ups and improvements making auto-detection seamless
 *
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * remove error_pipe
 *  * add struct mesg_array to process_error
 *
 *  2007-05-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added timestamp to mesg_array
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * rewrite for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * moved RW error state to separate member
 *
 *  2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * removed CWIID_CMP_LEN macro and cwiid_findfirst prototype
 *
 *  2007-03-05 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote parameter to cwiid_err prototype
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#ifndef CWIID_INTERNAL_H
#define CWIID_INTERNAL_H

#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>	/* ssize_t */
#include "cwiid.h"

/* Bluetooth magic numbers */
#define BT_TRANS_MASK		0xF0
#define BT_TRANS_HANDSHAKE	0x00
#define BT_TRANS_SET_REPORT	0x50
#define BT_TRANS_DATA		0xA0
#define BT_TRANS_DATAC		0xB0

#define BT_PARAM_MASK		0x0F
/* HANDSHAKE params */
#define BT_PARAM_SUCCESSFUL					0x00
#define BT_PARAM_NOT_READY					0x01
#define BT_PARAM_ERR_INVALID_REPORT_ID		0x02
#define BT_PARAM_ERR_UNSUPPORTED_REQUEST	0x03
#define BT_PARAM_ERR_INVALID_PARAMETER		0x04
#define BT_PARAM_ERR_UNKNOWN				0x0E
#define BT_PARAM_ERR_FATAL					0x0F
/* SET_REPORT, DATA, DATAC params */
#define BT_PARAM_INPUT		0x01
#define BT_PARAM_OUTPUT		0x02
#define BT_PARAM_FEATURE	0x03

/* Wiimote specific magic numbers */
#define WIIMOTE_NAME "Nintendo RVL-CNT-01"
#define WIIMOTE_PLUS_NAME "Nintendo RVL-CNT-01-TR"
#define WIIBALANCE_NAME "Nintendo RVL-WBC-01"
#define WIIMOTE_CLASS_0 0x04
#define WIIMOTE_CLASS_1 0x25
#define WIIMOTE_CLASS_2 0x00
#define WIIMOTE_PLUS_CLASS_0 0x08
#define WIIMOTE_PLUS_CLASS_1 0x05

/* Wiimote port/channel/PSMs */
#define CTL_PSM	17
#define INT_PSM	19

/* Report numbers */
#define RPT_LED_RUMBLE			0x11
#define RPT_RPT_MODE			0x12
#define RPT_IR_ENABLE1			0x13
#define RPT_SPEAKER_ENABLE		0x14
#define RPT_STATUS_REQ			0x15
#define RPT_WRITE				0x16
#define RPT_READ_REQ			0x17
#define RPT_SPEAKER_DATA		0x18
#define RPT_SPEAKER_MUTE		0x19
#define RPT_IR_ENABLE2			0x1A
#define RPT_STATUS				0x20
#define RPT_READ_DATA			0x21
#define RPT_WRITE_ACK			0x22
#define RPT_BTN					0x30
#define RPT_BTN_ACC				0x31
#define RPT_BTN_EXT8			0x32
#define RPT_BTN_ACC_IR12		0x33
#define RPT_BTN_EXT19			0x34
#define RPT_BTN_ACC_EXT16		0x35
#define RPT_BTN_IR10_EXT9		0x36
#define RPT_BTN_ACC_IR10_EXT6	0x37
#define RPT_EXT21				0x3D
#define RPT_BTN_ACC_IR36_1		0x3E
#define RPT_BTN_ACC_IR36_2		0x3F

/* Button Mask (masks unknown bits in button bytes) */
#define BTN_MASK_0			0x1F
#define BTN_MASK_1			0x9F
#define NUNCHUK_BTN_MASK	0x03
#define PT_NUNCHUK_BTN_MASK 0x0C

/* Extension Values */
#define EXT_NONE				0x2E2E
#define EXT_PARTIAL				0xFFFF
#define EXT_NUNCHUK				0x0000
#define EXT_CLASSIC				0x0101
#define EXT_BALANCE				0x0402
#define EXT_MOTIONPLUS			0x0405
#define EXT_NUNCHUK_MOTIONPLUS	0x0505
#define EXT_CLASSIC_MOTIONPLUS	0x0705

/* IR Enable blocks */
#define MARCAN_IR_BLOCK_1			"\x00\x00\x00\x00\x00\x00\x90\x00\xC0"
#define MARCAN_IR_BLOCK_2			"\x40\x00"
#define CLIFF_IR_BLOCK_1			"\x02\x00\x00\x71\x01\x00\xAA\x00\x64"
#define CLIFF_IR_BLOCK_2			"\x63\x03"
#define MAX_SENSITIVITY_IR_BLOCK_1	"\x00\x00\x00\x00\x00\x00\x90\x00\x41"
#define MAX_SENSITIVITY_IR_BLOCK_2	"\x40\x00"
#define WII_L1_IR_BLOCK_1			"\x02\x00\x00\x71\x01\x00\x64\x00\xFE"
#define WII_L1_IR_BLOCK_2			"\xFD\x05"
#define WII_L2_IR_BLOCK_1			"\x02\x00\x00\x71\x01\x00\x96\x00\xB4"
#define WII_L2_IR_BLOCK_2			"\xB3\x04"
#define WII_L3_IR_BLOCK_1			"\x02\x00\x00\x71\x01\x00\xAA\x00\x64"
#define WII_L3_IR_BLOCK_2			"\x63\x03"
#define WII_L4_IR_BLOCK_1			"\x02\x00\x00\x71\x01\x00\xC8\x00\x36"
#define WII_L4_IR_BLOCK_2			"\x35\x03"
#define WII_L5_IR_BLOCK_1			"\x02\x00\x00\x71\x01\x00\x72\x00\x20"
#define WII_L5_IR_BLOCK_2			"\x1F\x03"

/* Write Sequences */
enum write_seq_type {
	WRITE_SEQ_RPT,
	WRITE_SEQ_MEM
};

struct write_seq {
	enum write_seq_type type;
	uint32_t report_offset;
	const void *data;
	uint16_t len;
	uint8_t flags;
};

#define SEQ_LEN(seq) (sizeof(seq)/sizeof(struct write_seq))

/* Message arrays */
struct mesg_array {
	uint8_t count;
	struct timespec timestamp;
	union cwiid_mesg array[CWIID_MAX_MESG_COUNT];
};

/* RW State/Mesg */
enum rw_status {
	RW_IDLE,
	RW_READ,
	RW_WRITE,
	RW_CANCEL
};

struct rw_mesg {
	enum rw_status type;
	uint8_t error;
	uint32_t offset;
	uint8_t len;
	char data[16];
};


/* Wiimote struct */
struct wiimote {
	int flags;
	int ctl_socket;
	int int_socket;
	pthread_t router_thread;
	pthread_t status_thread;
	pthread_t mesg_callback_thread;
	int mesg_pipe[2];
	int status_pipe[2];
	int rw_pipe[2];
	struct cwiid_state state;
	enum rw_status rw_status;
	cwiid_mesg_callback_t *mesg_callback;
	pthread_mutex_t state_mutex;
	pthread_mutex_t rw_mutex;
	pthread_mutex_t rpt_mutex;
	int id;
	const void *data;
	int ext;	//extension identifier already added to wiimote
	pthread_t passthrough_polling_thread;
	int passthrough_activate_flag;
	pthread_mutex_t poll_mutex;
	pthread_cond_t condition_var;
};

/* prototypes */
/* thread.c */
void *router_thread(struct wiimote *wiimote);
void *status_thread(struct wiimote *wiimote);
void *passthrough_polling_thread(struct wiimote *wiimote);
void *mesg_callback_thread(struct wiimote *wiimote);

/* util.c */
void cwiid_err(struct wiimote *wiimote, const char *str, ...);
int verify_handshake(struct wiimote *wiimote);
int exec_write_seq(struct wiimote *wiimote, unsigned int len,
                   struct write_seq *seq);
int full_read(int fd, void *buf, size_t len);
int write_mesg_array(struct wiimote *wiimote, struct mesg_array *ma);
int read_mesg_array(int fd, struct mesg_array *ma);
int cancel_rw(struct wiimote *wiimote);
int cancel_mesg_callback(struct wiimote *wiimote);

/* process.c */
int process_error(struct wiimote *, ssize_t, struct mesg_array *);
int process_status(struct wiimote *, const unsigned char *,
                   struct mesg_array *);
int process_btn(struct wiimote *, const unsigned char *, struct mesg_array *);
int process_acc(struct wiimote *, const unsigned char *, struct mesg_array *);
int process_ir10(struct wiimote *, const unsigned char *, struct mesg_array *);
int process_ir12(struct wiimote *, const unsigned char *, struct mesg_array *);
int process_ext(struct wiimote *, unsigned char *, unsigned char,
                struct mesg_array *);
int process_read(struct wiimote *, unsigned char *);
int process_write(struct wiimote *, unsigned char *);

/* state.c */
int update_state(struct wiimote *wiimote, struct mesg_array *ma);
int update_rpt_mode(struct wiimote *wiimote, int8_t rpt_mode);

#endif
