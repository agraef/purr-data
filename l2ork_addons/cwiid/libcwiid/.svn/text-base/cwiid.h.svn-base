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
 *  2007-08-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * make cwiid_err_default public
 *
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_connect, cwiid_disconnect to cwiid_open, cwiid_close
 *  * added cwiid_request_status, cwiid_set_let, cwiid_set_rumble,
 *    cwiid_set_rpt_mode
 *
 *  2007-05-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added timestamp to message functions
 *  * added cwiid_get_acc_cal
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * rewrite for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-07 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_info.class to btclass
 *
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added cwiid_mesg_error message type
 *
 *  2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * cwiid_connect now takes a pointer to bdaddr_t
 *  * added cwiid_info definition and macros
 *  * added cwiid_get_info_array prototype
 *  * changed cwiid_findfirst to cwiid_find_wiimote
 *
 *  2007-03-05 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added cwiid_err_t definition
 *  * added cwiid_set_err prototype
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#ifndef CWIID_H
#define CWIID_H

#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <bluetooth/bluetooth.h>	/* bdaddr_t */

/* Flags */
#define CWIID_FLAG_MESG_IFC		0x01
#define CWIID_FLAG_CONTINUOUS	0x02
#define CWIID_FLAG_REPEAT_BTN	0x04
#define CWIID_FLAG_NONBLOCK		0x08
#define CWIID_FLAG_MOTIONPLUS	0x10

/* Report Mode Flags */
#define CWIID_RPT_STATUS		0x01
#define CWIID_RPT_BTN			0x02
#define CWIID_RPT_ACC			0x04
#define CWIID_RPT_IR			0x08
#define CWIID_RPT_NUNCHUK		0x10
#define CWIID_RPT_CLASSIC		0x20
#define CWIID_RPT_BALANCE		0x40
#define CWIID_RPT_MOTIONPLUS	0x80
#define CWIID_RPT_EXT		(CWIID_RPT_NUNCHUK | CWIID_RPT_CLASSIC | \
                             CWIID_RPT_BALANCE | CWIID_RPT_MOTIONPLUS)

/* LED flags */
#define CWIID_LED1_ON	0x01
#define CWIID_LED2_ON	0x02
#define CWIID_LED3_ON	0x04
#define CWIID_LED4_ON	0x08

/* Button flags */
#define CWIID_BTN_2		0x0001
#define CWIID_BTN_1		0x0002
#define CWIID_BTN_B		0x0004
#define CWIID_BTN_A		0x0008
#define CWIID_BTN_MINUS	0x0010
#define CWIID_BTN_HOME	0x0080
#define CWIID_BTN_LEFT	0x0100
#define CWIID_BTN_RIGHT	0x0200
#define CWIID_BTN_DOWN	0x0400
#define CWIID_BTN_UP	0x0800
#define CWIID_BTN_PLUS	0x1000

#define CWIID_NUNCHUK_BTN_Z	0x01
#define CWIID_NUNCHUK_BTN_C	0x02

#define CWIID_CLASSIC_BTN_UP	0x0001
#define CWIID_CLASSIC_BTN_LEFT	0x0002
#define CWIID_CLASSIC_BTN_ZR	0x0004
#define CWIID_CLASSIC_BTN_X		0x0008
#define CWIID_CLASSIC_BTN_A		0x0010
#define CWIID_CLASSIC_BTN_Y		0x0020
#define CWIID_CLASSIC_BTN_B		0x0040
#define CWIID_CLASSIC_BTN_ZL	0x0080
#define CWIID_CLASSIC_BTN_R		0x0200
#define CWIID_CLASSIC_BTN_PLUS	0x0400
#define CWIID_CLASSIC_BTN_HOME	0x0800
#define CWIID_CLASSIC_BTN_MINUS	0x1000
#define CWIID_CLASSIC_BTN_L		0x2000
#define CWIID_CLASSIC_BTN_DOWN	0x4000
#define CWIID_CLASSIC_BTN_RIGHT	0x8000

/* Send Report flags */
#define CWIID_SEND_RPT_NO_RUMBLE    0x01

/* Data Read/Write flags */
#define CWIID_RW_EEPROM	0x00
#define CWIID_RW_REG	0x04
#define CWIID_RW_DECODE	0x00

/* Maximum Data Read Length */
#define CWIID_MAX_READ_LEN	0xFFFF

/* Array Index Defs */
#define CWIID_X		0
#define CWIID_Y		1
#define CWIID_Z		2
#define CWIID_PHI	0
#define CWIID_THETA	1
#define CWIID_PSI	2

/* Acc Defs */
#define CWIID_ACC_MAX	0xFF

/* IR Defs */
#define CWIID_IR_SRC_COUNT	4
#define CWIID_IR_X_MAX		1024
#define CWIID_IR_Y_MAX		768

/* Battery */
#define CWIID_BATTERY_MAX	0xD0

/* Classic Controller Maxes */
#define CWIID_CLASSIC_L_STICK_MAX	0x3F
#define CWIID_CLASSIC_R_STICK_MAX	0x1F
#define CWIID_CLASSIC_LR_MAX	0x1F

/* Environment Variables */
#define WIIMOTE_BDADDR	"WIIMOTE_BDADDR"

/* Callback Maximum Message Count */
#define CWIID_MAX_MESG_COUNT	5

/* Enumerations */
enum cwiid_command {
	CWIID_CMD_STATUS,
	CWIID_CMD_LED,
	CWIID_CMD_RUMBLE,
	CWIID_CMD_RPT_MODE
};

enum cwiid_mesg_type {
	CWIID_MESG_STATUS,
	CWIID_MESG_BTN,
	CWIID_MESG_ACC,
	CWIID_MESG_IR,
	CWIID_MESG_NUNCHUK,
	CWIID_MESG_CLASSIC,
	CWIID_MESG_BALANCE,
	CWIID_MESG_MOTIONPLUS,
	CWIID_MESG_ERROR,
	CWIID_MESG_UNKNOWN
};

enum cwiid_ext_type {
	CWIID_EXT_NONE,
	CWIID_EXT_NUNCHUK,
	CWIID_EXT_CLASSIC,
	CWIID_EXT_BALANCE,
	CWIID_EXT_MOTIONPLUS,
	CWIID_EXT_UNKNOWN
};

enum cwiid_error {
	CWIID_ERROR_NONE,
	CWIID_ERROR_DISCONNECT,
	CWIID_ERROR_COMM
};

struct acc_cal {
	uint8_t zero[3];
	uint8_t one[3];
};

struct balance_cal {
	uint16_t right_top[3];
	uint16_t right_bottom[3];
	uint16_t left_top[3];
	uint16_t left_bottom[3];
};

/* Message Structs */
struct cwiid_status_mesg {
	enum cwiid_mesg_type type;
	uint8_t battery;
	enum cwiid_ext_type ext_type;
};	

struct cwiid_btn_mesg {
	enum cwiid_mesg_type type;
	uint16_t buttons;
};

struct cwiid_acc_mesg {
	enum cwiid_mesg_type type;
	uint8_t acc[3];
};

struct cwiid_ir_src {
	char valid;
	uint16_t pos[2];
	int8_t size;
};

struct cwiid_ir_mesg {
	enum cwiid_mesg_type type;
	struct cwiid_ir_src src[CWIID_IR_SRC_COUNT];
};

struct cwiid_nunchuk_mesg {
	enum cwiid_mesg_type type;
	uint8_t stick[2];
	uint8_t acc[3];
	uint8_t buttons;
};

struct cwiid_classic_mesg {
	enum cwiid_mesg_type type;
	uint8_t l_stick[2];
	uint8_t r_stick[2];
	uint8_t l;
	uint8_t r;
	uint16_t buttons;
};

struct cwiid_balance_mesg {
	enum cwiid_mesg_type type;
	uint16_t right_top;
	uint16_t right_bottom;
	uint16_t left_top;
	uint16_t left_bottom;
};

struct cwiid_motionplus_mesg {
	enum cwiid_mesg_type type;
	uint16_t angle_rate[3];
};

struct cwiid_error_mesg {
	enum cwiid_mesg_type type;
	enum cwiid_error error;
};

union cwiid_mesg {
	enum cwiid_mesg_type type;
	struct cwiid_status_mesg status_mesg;
	struct cwiid_btn_mesg btn_mesg;
	struct cwiid_acc_mesg acc_mesg;
	struct cwiid_ir_mesg ir_mesg;
	struct cwiid_nunchuk_mesg nunchuk_mesg;
	struct cwiid_classic_mesg classic_mesg;
	struct cwiid_balance_mesg balance_mesg;
	struct cwiid_motionplus_mesg motionplus_mesg;
	struct cwiid_error_mesg error_mesg;
};

/* State Structs */
struct nunchuk_state {
	uint8_t stick[2];
	uint8_t acc[3];
	uint8_t buttons;
};

struct classic_state {
	uint8_t l_stick[2];
	uint8_t r_stick[2];
	uint8_t l;
	uint8_t r;
	uint16_t buttons;
};

struct balance_state {
	uint16_t right_top;
	uint16_t right_bottom;
	uint16_t left_top;
	uint16_t left_bottom;
};

struct motionplus_state {
	uint16_t angle_rate[3];
};

union ext_state {
	struct nunchuk_state nunchuk;
	struct classic_state classic;
	struct balance_state balance;
	struct motionplus_state motionplus;
};

struct cwiid_state {
	uint8_t rpt_mode;
	uint8_t led;
	uint8_t rumble;
	uint8_t battery;
	uint16_t buttons;
	uint8_t acc[3];
	struct cwiid_ir_src ir_src[CWIID_IR_SRC_COUNT];
	enum cwiid_ext_type ext_type;
	union ext_state ext;
	enum cwiid_error error;
};

/* Typedefs */
typedef struct wiimote cwiid_wiimote_t;

typedef void cwiid_mesg_callback_t(cwiid_wiimote_t *, int,
                                   union cwiid_mesg [], struct timespec *);
typedef void cwiid_err_t(cwiid_wiimote_t *, const char *, va_list ap);

/* get_bdinfo */
#define BT_NO_WIIMOTE_FILTER 0x01
#define BT_NAME_LEN 32

struct cwiid_bdinfo {
	bdaddr_t bdaddr;
	uint8_t btclass[3];
	char name[BT_NAME_LEN];
};

#ifdef __cplusplus
extern "C" {
#endif

/* Error reporting (library wide) */
int cwiid_set_err(cwiid_err_t *err);
void cwiid_err_default(struct wiimote *wiimote, const char *str, va_list ap);

/* Connection */
#define cwiid_connect cwiid_open
#define cwiid_disconnect cwiid_close
cwiid_wiimote_t *cwiid_open(bdaddr_t *bdaddr, int flags);
cwiid_wiimote_t *cwiid_open_timeout(bdaddr_t *bdaddr, int flags, int timeout);
int cwiid_close(cwiid_wiimote_t *wiimote);

int cwiid_get_id(cwiid_wiimote_t *wiimote);
int cwiid_set_data(cwiid_wiimote_t *wiimote, const void *data);
const void *cwiid_get_data(cwiid_wiimote_t *wiimote);
int cwiid_enable(cwiid_wiimote_t *wiimote, int flags);
int cwiid_disable(cwiid_wiimote_t *wiimote, int flags);

/* Interfaces */
int cwiid_set_mesg_callback(cwiid_wiimote_t *wiimote,
                       cwiid_mesg_callback_t *callback);
int cwiid_get_mesg(cwiid_wiimote_t *wiimote, int *mesg_count,
                   union cwiid_mesg *mesg[], struct timespec *timestamp);
int cwiid_get_state(cwiid_wiimote_t *wiimote, struct cwiid_state *state);
int cwiid_get_acc_cal(struct wiimote *wiimote, enum cwiid_ext_type ext_type,
                      struct acc_cal *acc_cal);
int cwiid_get_balance_cal(struct wiimote *wiimote,
                          struct balance_cal *balance_cal);

/* Operations */
int cwiid_command(cwiid_wiimote_t *wiimote, enum cwiid_command command,
                  int flags);
int cwiid_send_rpt(cwiid_wiimote_t *wiimote, uint8_t flags, uint8_t report,
                   size_t len, const void *data);
int cwiid_request_status(cwiid_wiimote_t *wiimote);
int cwiid_set_led(cwiid_wiimote_t *wiimote, uint8_t led);
int cwiid_set_rumble(cwiid_wiimote_t *wiimote, uint8_t rumble);
int cwiid_set_rpt_mode(cwiid_wiimote_t *wiimote, uint8_t rpt_mode);
int cwiid_read(cwiid_wiimote_t *wiimote, uint8_t flags, uint32_t offset,
               uint16_t len, void *data);
int cwiid_write(cwiid_wiimote_t *wiimote, uint8_t flags, uint32_t offset,
                uint16_t len, const void *data);
/* int cwiid_beep(cwiid_wiimote_t *wiimote); */

/* HCI functions */
int cwiid_get_bdinfo_array(int dev_id, unsigned int timeout, int max_bdinfo,
                           struct cwiid_bdinfo **bdinfo, uint8_t flags);
int cwiid_find_wiimote(bdaddr_t *bdaddr, int timeout);

#ifdef __cplusplus
}
#endif

#endif
