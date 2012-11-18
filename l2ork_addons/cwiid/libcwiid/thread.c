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
 *
 *  2012-11-01 Ivica Ico Bukvic <ico@vt.edu>
 * * fixed segfault when trying to toggle passthrough mode with wiimote disconnected
 *
 *  2012-04-11 Deba Pratim Saha <dpsaha@vt.edu> and Ivica Ico Bukvic <ico@vt.edu>
 * * implemented passthrough toggle
 * * implemented Mplus + Nunchuk passthrough support
 * * removed balance board from the CWIID_RPT_EXT define as that limited incoming streams to 2 (bug)
 * * various clean-ups and improvements making auto-detection seamless
 *
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * send error_mesg from process_error
 *
 *  2007-05-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added timestamp to message array
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * rewrite for API overhaul (renamed from event.c)
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-08 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * fixed incompatible pointer warning in process_error
 *
 *  2007-04-08 Petter Reinholdtsen <pere@hungry.com>
 *  * fixed signed/unsigned comparison error in int_listen
 *
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * implemented process_error to handle socket read errors
 *  * added rw_status triggers to read and write handlers
 *
 *  2007-03-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * audit error checking
 *  * reorganized file
 *  * moved int_listen read/write code to process_read and process_write
 *  * updated (some/a few) comments
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
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "cwiid_internal.h"

int cwiid_toggle_passthrough_mode(cwiid_wiimote_t *wiimote, int var)						//Deba
{
	if (wiimote) {
		if (var != wiimote->passthrough_activate_flag) {
			if (var) {
				wiimote->passthrough_activate_flag = 1;
				pthread_cond_signal(&wiimote->condition_var);	
			} else {
				wiimote->passthrough_activate_flag = 0;
				pthread_cond_signal(&wiimote->condition_var);
			}
		}
	}

	return 0;
}

#define POLL_THREAD_DELAY 1000000

void *passthrough_polling_thread(struct wiimote *wiimote)
{
	unsigned char buf, data;	

	while(1)
	{
		if ((wiimote->passthrough_activate_flag && wiimote->ext == NUNCHUK_MOTIONPLUS_PRESENT) || 
				(wiimote->passthrough_activate_flag && wiimote->ext == CLASSIC_MOTIONPLUS_PRESENT) || 
				(!wiimote->passthrough_activate_flag && wiimote->ext == MOTIONPLUS_PRESENT) || 
				(!wiimote->passthrough_activate_flag && wiimote->ext == EXT_PRESENT_NOT_MOTIONPLUS))
		{
			pthread_mutex_lock(&wiimote->poll_mutex);
			pthread_cond_wait(&wiimote->condition_var,&wiimote->poll_mutex);
			pthread_mutex_unlock(&wiimote->poll_mutex);
		}

		//fprintf(stderr,"passthrough_thread: resume\n");

		if (wiimote->passthrough_activate_flag) {

			//fprintf(stderr,"passthrough_thread: try activating passthrough\n");

			if (wiimote->ext != NUNCHUK_MOTIONPLUS_PRESENT && wiimote->ext != CLASSIC_MOTIONPLUS_PRESENT && wiimote->ext != MOTIONPLUS_PRESENT) {
				/* re-detecting motionplus */
				buf=0xAB;																	//any junk value initialisation
				cwiid_try_read(wiimote, CWIID_RW_REG, 0xA600FE, 1, &buf);					//Checking for the MotionPlus jackin
				if(buf==0x00 || buf==0x04 || buf==0x05 )	
				{														
					cwiid_enable(wiimote, CWIID_FLAG_MOTIONPLUS);
				}
			}

			if (wiimote->ext == MOTIONPLUS_PRESENT) {
				/* motionplus_present trying passthrough */
				data = 0x55;
				cwiid_write(wiimote, CWIID_RW_REG, 0xA400F0, 1, &data);
				data = 0x00;
				cwiid_write(wiimote, CWIID_RW_REG, 0xA400FB, 1, &data);
			}

			usleep(POLL_THREAD_DELAY);
			cwiid_request_status(wiimote);

		} else {

			/* disabling passthrough and/or re-detecting motionplus if necessary */

			/* only do this call if some other expansion is not already connected
			   this works because in this implementation motionplus takes precedence
			   over other kinds of extensions, like nunchuk and classic */

			//fprintf(stderr,"passthrough_thread: reverting to motionplus\n");

			if (wiimote->ext != EXT_PRESENT_NOT_MOTIONPLUS && wiimote->ext != EXT_UNINITIALIZED) {

				//fprintf(stderr,"passthrough_thread: !EXT_PRESENT_NOT_MOTIONPLUS\n");
				data = 0x55;
				cwiid_write(wiimote, CWIID_RW_REG, 0xA400F0, 1, &data);
				data = 0x00;
				cwiid_write(wiimote, CWIID_RW_REG, 0xA400FB, 1, &data);

				buf=0xAB;																	//any junk value initialisation
				cwiid_try_read(wiimote, CWIID_RW_REG, 0xA600FE, 1, &buf);					//Checking for the MotionPlus jackin
				if(buf==0x00 || buf==0x04 || buf==0x05 || buf==0x07)	
				{												
					cwiid_enable(wiimote, CWIID_FLAG_MOTIONPLUS);

					usleep(POLL_THREAD_DELAY);
					cwiid_request_status(wiimote);
				}
			}
		}				
	}

	return NULL;
}


#define READ_BUF_LEN 23
void *router_thread(struct wiimote *wiimote)
{
	unsigned char buf[READ_BUF_LEN];
	ssize_t len;
	struct mesg_array ma;
	char err, print_clock_err = 1;

	while (1) {

		/* Read packet */
		len = read(wiimote->int_socket, buf, READ_BUF_LEN);
		ma.count = 0;
		if (clock_gettime(CLOCK_REALTIME, &ma.timestamp)) {
			if (print_clock_err) {
				cwiid_err(wiimote, "clock_gettime error");
				print_clock_err = 0;
			}
		}
		err = 0;
		if ((len == -1) || (len == 0)) {
			process_error(wiimote, len, &ma);
			write_mesg_array(wiimote, &ma);
			/* Quit! */
			break;
		}
		else {
			/* Verify first byte (DATA/INPUT) */
			if (buf[0] != (BT_TRANS_DATA | BT_PARAM_INPUT)) {
				cwiid_err(wiimote, "Invalid packet type");
			}

			/* Main switch */
			//printf("%.2X %.2X %.2X %.2X  %.2X %.2X %.2X %.2X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
			//printf("%.2X %.2X %.2X %.2X  %.2X %.2X %.2X %.2X\n", buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
			//printf("%.2X %.2X %.2X %.2X  %.2X %.2X %.2X %.2X\n", buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);
			//printf("\n");

			switch (buf[1]) {
			case RPT_STATUS:
				err = process_status(wiimote, &buf[2], &ma);
				break;
			case RPT_BTN:
				err = process_btn(wiimote, &buf[2], &ma);
				break;
			case RPT_BTN_ACC:
				err = process_btn(wiimote, &buf[2], &ma) ||
				      process_acc(wiimote, &buf[4], &ma);
				break;
			case RPT_BTN_EXT8:
				err = process_btn(wiimote, &buf[2], &ma) ||
				      process_ext(wiimote, &buf[4], 8, &ma);
				break;
			case RPT_BTN_ACC_IR12:
				err = process_btn(wiimote, &buf[2], &ma) ||
				      process_acc(wiimote, &buf[4], &ma) ||
				      process_ir12(wiimote, &buf[7], &ma);
				break;
			case RPT_BTN_EXT19:
				err = process_btn(wiimote, &buf[2], &ma) ||
				      process_ext(wiimote, &buf[4], 19, &ma);
				break;
			case RPT_BTN_ACC_EXT16:
				err = process_btn(wiimote, &buf[2], &ma) ||
				      process_acc(wiimote, &buf[4], &ma) ||
				      process_ext(wiimote, &buf[7], 16, &ma);
				break;
			case RPT_BTN_IR10_EXT9:
				err = process_btn(wiimote, &buf[2], &ma)  ||
				      process_ir10(wiimote, &buf[4], &ma) ||
				      process_ext(wiimote, &buf[14], 9, &ma);
				break;
			case RPT_BTN_ACC_IR10_EXT6:
				err = process_btn(wiimote, &buf[2], &ma)  ||
				      process_acc(wiimote, &buf[4], &ma)  ||
				      process_ir10(wiimote, &buf[7], &ma) ||
				      process_ext(wiimote, &buf[17], 6, &ma);
				break;
			case RPT_EXT21:
				err = process_ext(wiimote, &buf[2], 21, &ma);
				break;
			case RPT_BTN_ACC_IR36_1:
			case RPT_BTN_ACC_IR36_2:
				cwiid_err(wiimote, "Unsupported report type received "
				                   "(interleaved data)");
				err = 1;
				break;
			case RPT_READ_DATA:
				err = process_read(wiimote, &buf[4]) ||
				      process_btn(wiimote, &buf[2], &ma);
				break;
			case RPT_WRITE_ACK:
				err = process_write(wiimote, &buf[2]);
				break;
			default:
				cwiid_err(wiimote, "Unknown message type");
				err = 1;
				break;
			}

			if (!err && (ma.count > 0)) {
				if (update_state(wiimote, &ma)) {
					cwiid_err(wiimote, "State update error");
				}
				if (wiimote->flags & CWIID_FLAG_MESG_IFC) {
					/* prints its own errors */
					write_mesg_array(wiimote, &ma);
				}
			}
		}
	}

	return NULL;
}


void *status_thread(struct wiimote *wiimote)
{
	struct mesg_array ma;
	struct cwiid_status_mesg *status_mesg;
	unsigned char buf[2];
	unsigned char data[2];

	ma.count = 1;
	status_mesg = &ma.array[0].status_mesg;

	while (1) {

		if (full_read(wiimote->status_pipe[0], status_mesg,
		              sizeof *status_mesg)) {
			cwiid_err(wiimote, "Pipe read error (status)");
			/* Quit! */
			break;
		}

		if (status_mesg->type != CWIID_MESG_STATUS) {
			cwiid_err(wiimote, "Bad message on status pipe");
			continue;
		}

		if (status_mesg->ext_type == CWIID_EXT_UNKNOWN) {
			/* Read extension ID */
			if (cwiid_read(wiimote, CWIID_RW_REG, 0xA400FE, 2, &buf)) {
				cwiid_err(wiimote, "Read error (extension error)");
				status_mesg->ext_type = CWIID_EXT_UNKNOWN;
			}

			/* If we have just disabled passthrough override passthrough with just plain motionplus */
			/*if ((!wiimote->passthrough_activate_flag && ((buf[0] << 8) | buf[1]) == EXT_NUNCHUK_MOTIONPLUS) || 
				(!wiimote->passthrough_activate_flag && ((buf[0] << 8) | buf[1]) == EXT_CLASSIC_MOTIONPLUS)) {
				fprintf(stderr, "status_thread: reverting to motionplus\n");
				buf[0] = EXT_MOTIONPLUS;
			}*/

			/* If the extension didn't change, or if the extension is a
			 * MotionPlus, no init necessary */
			switch ((buf[0] << 8) | buf[1]) {
			case EXT_NONE:
				status_mesg->ext_type = CWIID_EXT_NONE;
				break;
			case EXT_NUNCHUK:
				if (wiimote->ext == MOTIONPLUS_PRESENT && wiimote->passthrough_activate_flag){		
					data[0] = 0x05;
					cwiid_write(wiimote, CWIID_RW_REG, 0xA600FE, 1, &data[0]);
					cwiid_read(wiimote, CWIID_RW_REG, 0xA400FE, 1, &data[1]);
					if (data[1] == 0x05) {
						status_mesg->ext_type = CWIID_EXT_NUNCHUK_MPLUS_PASSTHROUGH;
						wiimote->ext = NUNCHUK_MOTIONPLUS_PRESENT;
						break;
					}
				}else{
					status_mesg->ext_type = CWIID_EXT_NUNCHUK;
					wiimote->ext = EXT_PRESENT_NOT_MOTIONPLUS;
					break;
				}
			case EXT_NUNCHUK_MOTIONPLUS:
				status_mesg->ext_type = CWIID_EXT_NUNCHUK_MPLUS_PASSTHROUGH;
				wiimote->ext = NUNCHUK_MOTIONPLUS_PRESENT;
				break;
			case EXT_CLASSIC:
				if (wiimote->ext == MOTIONPLUS_PRESENT && wiimote->passthrough_activate_flag){		
					data[0] = 0x07;
					cwiid_write(wiimote, CWIID_RW_REG, 0xA600FE, 1, &data[0]);
					cwiid_read(wiimote, CWIID_RW_REG, 0xA400FE, 1, &data[1]);
					if (data[1] == 0x07) {
						status_mesg->ext_type = CWIID_EXT_CLASSIC_MPLUS_PASSTHROUGH;
						wiimote->ext = CLASSIC_MOTIONPLUS_PRESENT;
						break;
					}
				}else{
					status_mesg->ext_type = CWIID_EXT_CLASSIC;
					wiimote->ext = EXT_PRESENT_NOT_MOTIONPLUS;
					break;
				}
			case EXT_CLASSIC_MOTIONPLUS:
				status_mesg->ext_type = CWIID_EXT_CLASSIC_MPLUS_PASSTHROUGH;
				wiimote->ext = CLASSIC_MOTIONPLUS_PRESENT;
				break;
			case EXT_BALANCE:
				status_mesg->ext_type = CWIID_EXT_BALANCE;
				wiimote->ext = EXT_PRESENT_NOT_MOTIONPLUS;
				break;
			case EXT_MOTIONPLUS:
				status_mesg->ext_type = CWIID_EXT_MOTIONPLUS;
				wiimote->ext = MOTIONPLUS_PRESENT;
				break;
			case EXT_PARTIAL:
				/* Everything (but MotionPlus) shows up as partial until initialized */
				buf[0] = 0x55;
				buf[1] = 0x00;
				/* Initialize extension register space */
				if (cwiid_write(wiimote, CWIID_RW_REG, 0xA400F0, 1, &buf[0])) {
					cwiid_err(wiimote, "Extension initialization error");
					status_mesg->ext_type = CWIID_EXT_UNKNOWN;
				}
				else if (cwiid_write(wiimote, CWIID_RW_REG, 0xA400FB, 1, &buf[1])) {
						cwiid_err(wiimote, "Extension initialization error");
						status_mesg->ext_type = CWIID_EXT_UNKNOWN;
				}
				/* Read extension ID */
				else if (cwiid_read(wiimote, CWIID_RW_REG, 0xA400FE, 2, &buf)) {
					cwiid_err(wiimote, "Read error (extension error)");
					status_mesg->ext_type = CWIID_EXT_UNKNOWN;
				}
				else {
					switch ((buf[0] << 8) | buf[1]) {
					case EXT_NONE:
					case EXT_PARTIAL:
						status_mesg->ext_type = CWIID_EXT_NONE;
						break;
					case EXT_NUNCHUK:
						status_mesg->ext_type = CWIID_EXT_NUNCHUK;
						break;
					case EXT_CLASSIC:
						status_mesg->ext_type = CWIID_EXT_CLASSIC;
						break;
					case EXT_BALANCE:
						status_mesg->ext_type = CWIID_EXT_BALANCE;
						break;
					case EXT_NUNCHUK_MOTIONPLUS:
						status_mesg->ext_type = CWIID_EXT_NUNCHUK_MPLUS_PASSTHROUGH;
						break;
					case EXT_CLASSIC_MOTIONPLUS:
						status_mesg->ext_type = CWIID_EXT_CLASSIC_MPLUS_PASSTHROUGH;
						break;
					default:
						status_mesg->ext_type = CWIID_EXT_UNKNOWN;
						break;
					}
				}
				break;
			}
		}
		else {
			//fprintf(stderr,"ext_type=%d ext=%d\n", status_mesg->ext_type, wiimote->ext);
			if (!(wiimote->passthrough_activate_flag && wiimote->ext == MOTIONPLUS_PRESENT) ||
					!wiimote->passthrough_activate_flag) {
				//fprintf(stderr,"NO EXTENSION\n");
				wiimote->ext = NO_EXTENSION;
			}

			pthread_cond_signal(&wiimote->condition_var);
		}				

		if (update_state(wiimote, &ma)) {
			cwiid_err(wiimote, "State update error");
		}
		if (update_rpt_mode(wiimote, -1)) {
			cwiid_err(wiimote, "Error reseting report mode");
		}
		if ((wiimote->state.rpt_mode & CWIID_RPT_STATUS) &&
		  (wiimote->flags & CWIID_FLAG_MESG_IFC)) {
			if (write_mesg_array(wiimote, &ma)) {
				/* prints its own errors */
			}
		}
	}

	return NULL;
}



void *mesg_callback_thread(struct wiimote *wiimote)
{
	int mesg_pipe = wiimote->mesg_pipe[0];
	cwiid_mesg_callback_t *callback = wiimote->mesg_callback;
	struct mesg_array ma;
	int cancelstate;

	while (1) {
		if (read_mesg_array(mesg_pipe, &ma)) {
			cwiid_err(wiimote, "Mesg pipe read error");
			continue;
		}

		/* TODO: The callback can still be called once after disconnect,
		 * although it's very unlikely.  User must keep track and avoid
		 * accessing the wiimote struct after disconnect. */
		if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancelstate)) {
			cwiid_err(wiimote, "Cancel state disable error (callback thread)");
		}
		callback(wiimote, ma.count, ma.array, &ma.timestamp);
		if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &cancelstate)) {
			cwiid_err(wiimote, "Cancel state restore error (callback thread)");
		}
	}

	return NULL;
}
