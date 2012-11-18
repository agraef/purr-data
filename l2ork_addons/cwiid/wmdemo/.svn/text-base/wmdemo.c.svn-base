#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <cwiid.h>

/* This is a sample program written to demonstrate basic CWiid libwiimote
 * usage, until _actual_ documentation can be written.  It's quick and dirty
 * has a horrible interface, but it's sparce enough to pick out the important
 * parts easily.  For examples of read and write code, see wmgui.  Speaker
 * support is "experimental" (read: bad) enough to be disabled.  The beginnings
 * of a speaker output function are in libwiimote source. */
/* Note: accelerometer (including nunchuk) and IR outputs produce a
 * lot of data - the purpose of this program is demonstration, not good
 * interface, and it shows. */

cwiid_mesg_callback_t cwiid_callback;

#define toggle_bit(bf,b)	\
	(bf) = ((bf) & b)		\
	       ? ((bf) & ~(b))	\
	       : ((bf) | (b))

#define MENU \
	"1: toggle LED 1\n" \
	"2: toggle LED 2\n" \
	"3: toggle LED 3\n" \
	"4: toggle LED 4\n" \
	"5: toggle rumble\n" \
	"a: toggle accelerometer reporting\n" \
	"b: toggle button reporting\n" \
	"c: enable motionplus, if connected\n" \
	"e: toggle extension reporting\n" \
	"i: toggle ir reporting\n" \
	"m: toggle messages\n" \
	"p: print this menu\n" \
	"r: request status message ((t) enables callback output)\n" \
	"s: print current state\n" \
	"t: toggle status reporting\n" \
	"x: exit\n"

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state);
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode);
void print_state(struct cwiid_state *state);

cwiid_err_t err;
void err(cwiid_wiimote_t *wiimote, const char *s, va_list ap)
{
	if (wiimote) printf("%d:", cwiid_get_id(wiimote));
	else printf("-1:");
	vprintf(s, ap);
	printf("\n");
}

int main(int argc, char *argv[])
{
	cwiid_wiimote_t *wiimote;	/* wiimote handle */
	struct cwiid_state state;	/* wiimote state */
	bdaddr_t bdaddr;	/* bluetooth device address */
	unsigned char mesg = 0;
	unsigned char led_state = 0;
	unsigned char rpt_mode = 0;
	unsigned char rumble = 0;
	int exit = 0;

	cwiid_set_err(err);

	/* Connect to address given on command-line, if present */
	if (argc > 1) {
		str2ba(argv[1], &bdaddr);
	}
	else {
		bdaddr = *BDADDR_ANY;
	}

	/* Connect to the wiimote */
	printf("Put Wiimote in discoverable mode now (press 1+2)...\n");
	if (!(wiimote = cwiid_open(&bdaddr, 0))) {
		fprintf(stderr, "Unable to connect to wiimote\n");
		return -1;
	}
	if (cwiid_set_mesg_callback(wiimote, cwiid_callback)) {
		fprintf(stderr, "Unable to set message callback\n");
	}

	printf("Note: To demonstrate the new API interfaces, wmdemo no longer "
	       "enables messages by default.\n"
	       "Output can be gathered through the new state-based interface (s), "
	       "or by enabling the messages interface (m).\n");

	/* Menu */
	printf("%s", MENU);

	while (!exit) {
		switch (getchar()) {
		case '1':
			toggle_bit(led_state, CWIID_LED1_ON);
			set_led_state(wiimote, led_state);
			break;
		case '2':
			toggle_bit(led_state, CWIID_LED2_ON);
			set_led_state(wiimote, led_state);
			break;
		case '3':
			toggle_bit(led_state, CWIID_LED3_ON);
			set_led_state(wiimote, led_state);
			break;
		case '4':
			toggle_bit(led_state, CWIID_LED4_ON);
			set_led_state(wiimote, led_state);
			break;
		case '5':
			toggle_bit(rumble, 1);
			if (cwiid_set_rumble(wiimote, rumble)) {
				fprintf(stderr, "Error setting rumble\n");
			}
			break;
		case 'a':
			toggle_bit(rpt_mode, CWIID_RPT_ACC);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'b':
			toggle_bit(rpt_mode, CWIID_RPT_BTN);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'c':
			cwiid_enable(wiimote, CWIID_FLAG_MOTIONPLUS);
			break;
		case 'e':
			/* CWIID_RPT_EXT is actually
			 * CWIID_RPT_NUNCHUK | CWIID_RPT_CLASSIC | CWIID_RPT_BALANCE */
			toggle_bit(rpt_mode, CWIID_RPT_EXT);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'i':
			/* libwiimote picks the highest quality IR mode available with the
			 * other options selected (not including as-yet-undeciphered
			 * interleaved mode */
			toggle_bit(rpt_mode, CWIID_RPT_IR);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'm':
			if (!mesg) {
				if (cwiid_enable(wiimote, CWIID_FLAG_MESG_IFC)) {
					fprintf(stderr, "Error enabling messages\n");
				}
				else {
					mesg = 1;
				}
			}
			else {
				if (cwiid_disable(wiimote, CWIID_FLAG_MESG_IFC)) {
					fprintf(stderr, "Error disabling message\n");
				}
				else {
					mesg = 0;
				}
			}
			break;
		case 'p':
			printf("%s", MENU);
			break;
		case 'r':
			if (cwiid_request_status(wiimote)) {
				fprintf(stderr, "Error requesting status message\n");
			}
			break;
		case 's':
			if (cwiid_get_state(wiimote, &state)) {
				fprintf(stderr, "Error getting state\n");
			}
			print_state(&state);
			break;
		case 't':
			toggle_bit(rpt_mode, CWIID_RPT_STATUS);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'x':
			exit = -1;
			break;
		case '\n':
			break;
		default:
			fprintf(stderr, "invalid option\n");
		}
	}

	if (cwiid_close(wiimote)) {
		fprintf(stderr, "Error on wiimote disconnect\n");
		return -1;
	}

	return 0;
}

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state)
{
	if (cwiid_set_led(wiimote, led_state)) {
		fprintf(stderr, "Error setting LEDs \n");
	}
}
	
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode)
{
	if (cwiid_set_rpt_mode(wiimote, rpt_mode)) {
		fprintf(stderr, "Error setting report mode\n");
	}
}

void print_state(struct cwiid_state *state)
{
	int i;
	int valid_source = 0;

	printf("Report Mode:");
	if (state->rpt_mode & CWIID_RPT_STATUS) printf(" STATUS");
	if (state->rpt_mode & CWIID_RPT_BTN) printf(" BTN");
	if (state->rpt_mode & CWIID_RPT_ACC) printf(" ACC");
	if (state->rpt_mode & CWIID_RPT_IR) printf(" IR");
	if (state->rpt_mode & CWIID_RPT_NUNCHUK) printf(" NUNCHUK");
	if (state->rpt_mode & CWIID_RPT_CLASSIC) printf(" CLASSIC");
	if (state->rpt_mode & CWIID_RPT_BALANCE) printf(" BALANCE");
	if (state->rpt_mode & CWIID_RPT_MOTIONPLUS) printf(" MOTIONPLUS");
	printf("\n");
	
	printf("Active LEDs:");
	if (state->led & CWIID_LED1_ON) printf(" 1");
	if (state->led & CWIID_LED2_ON) printf(" 2");
	if (state->led & CWIID_LED3_ON) printf(" 3");
	if (state->led & CWIID_LED4_ON) printf(" 4");
	printf("\n");

	printf("Rumble: %s\n", state->rumble ? "On" : "Off");

	printf("Battery: %d%%\n",
	       (int)(100.0 * state->battery / CWIID_BATTERY_MAX));

	printf("Buttons: %X\n", state->buttons);

	printf("Acc: x=%d y=%d z=%d\n", state->acc[CWIID_X], state->acc[CWIID_Y],
	       state->acc[CWIID_Z]);

	printf("IR: ");
	for (i = 0; i < CWIID_IR_SRC_COUNT; i++) {
		if (state->ir_src[i].valid) {
			valid_source = 1;
			printf("(%d,%d) ", state->ir_src[i].pos[CWIID_X],
			                   state->ir_src[i].pos[CWIID_Y]);
		}
	}
	if (!valid_source) {
		printf("no sources detected");
	}
	printf("\n");

	switch (state->ext_type) {
	case CWIID_EXT_NONE:
		printf("No extension\n");
		break;
	case CWIID_EXT_UNKNOWN:
		printf("Unknown extension attached\n");
		break;
	case CWIID_EXT_NUNCHUK:
		printf("Nunchuk: btns=%.2X stick=(%d,%d) acc.x=%d acc.y=%d "
		       "acc.z=%d\n", state->ext.nunchuk.buttons,
		       state->ext.nunchuk.stick[CWIID_X],
		       state->ext.nunchuk.stick[CWIID_Y],
		       state->ext.nunchuk.acc[CWIID_X],
		       state->ext.nunchuk.acc[CWIID_Y],
		       state->ext.nunchuk.acc[CWIID_Z]);
		break;
	case CWIID_EXT_CLASSIC:
		printf("Classic: btns=%.4X l_stick=(%d,%d) r_stick=(%d,%d) "
		       "l=%d r=%d\n", state->ext.classic.buttons,
		       state->ext.classic.l_stick[CWIID_X],
		       state->ext.classic.l_stick[CWIID_Y],
		       state->ext.classic.r_stick[CWIID_X],
		       state->ext.classic.r_stick[CWIID_Y],
		       state->ext.classic.l, state->ext.classic.r);
		break;
	case CWIID_EXT_BALANCE:
		printf("Balance: right_top=%d right_bottom=%d "
		       "left_top=%d left_bottom=%d\n",
		       state->ext.balance.right_top,
		       state->ext.balance.right_bottom,
		       state->ext.balance.left_top,
		       state->ext.balance.left_bottom);
		break;
	case CWIID_EXT_MOTIONPLUS:
		printf("MotionPlus: angle_rate=(%d,%d,%d)\n",
		       state->ext.motionplus.angle_rate[0],
		       state->ext.motionplus.angle_rate[1],
		       state->ext.motionplus.angle_rate[2]);
		break;
	}
}

/* Prototype cwiid_callback with cwiid_callback_t, define it with the actual
 * type - this will cause a compile error (rather than some undefined bizarre
 * behavior) if cwiid_callback_t changes */
/* cwiid_mesg_callback_t has undergone a few changes lately, hopefully this
 * will be the last.  Some programs need to know which messages were received
 * simultaneously (e.g. for correlating accelerometer and IR data), and the
 * sequence number mechanism used previously proved cumbersome, so we just
 * pass an array of messages, all of which were received at the same time.
 * The id is to distinguish between multiple wiimotes using the same callback.
 * */
void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
                    union cwiid_mesg mesg[], struct timespec *timestamp)
{
	int i, j;
	int valid_source;

	for (i=0; i < mesg_count; i++)
	{
		switch (mesg[i].type) {
		case CWIID_MESG_STATUS:
			printf("Status Report: battery=%d extension=",
			       mesg[i].status_mesg.battery);
			switch (mesg[i].status_mesg.ext_type) {
			case CWIID_EXT_NONE:
				printf("none");
				break;
			case CWIID_EXT_NUNCHUK:
				printf("Nunchuk");
				break;
			case CWIID_EXT_CLASSIC:
				printf("Classic Controller");
				break;
			case CWIID_EXT_BALANCE:
				printf("Balance Board");
				break;
			case CWIID_EXT_MOTIONPLUS:
				printf("MotionPlus");
				break;
			default:
				printf("Unknown Extension");
				break;
			}
			printf("\n");
			break;
		case CWIID_MESG_BTN:
			printf("Button Report: %.4X\n", mesg[i].btn_mesg.buttons);
			break;
		case CWIID_MESG_ACC:
			printf("Acc Report: x=%d, y=%d, z=%d\n",
                   mesg[i].acc_mesg.acc[CWIID_X],
			       mesg[i].acc_mesg.acc[CWIID_Y],
			       mesg[i].acc_mesg.acc[CWIID_Z]);
			break;
		case CWIID_MESG_IR:
			printf("IR Report: ");
			valid_source = 0;
			for (j = 0; j < CWIID_IR_SRC_COUNT; j++) {
				if (mesg[i].ir_mesg.src[j].valid) {
					valid_source = 1;
					printf("(%d,%d) ", mesg[i].ir_mesg.src[j].pos[CWIID_X],
					                   mesg[i].ir_mesg.src[j].pos[CWIID_Y]);
				}
			}
			if (!valid_source) {
				printf("no sources detected");
			}
			printf("\n");
			break;
		case CWIID_MESG_NUNCHUK:
			printf("Nunchuk Report: btns=%.2X stick=(%d,%d) acc.x=%d acc.y=%d "
			       "acc.z=%d\n", mesg[i].nunchuk_mesg.buttons,
			       mesg[i].nunchuk_mesg.stick[CWIID_X],
			       mesg[i].nunchuk_mesg.stick[CWIID_Y],
			       mesg[i].nunchuk_mesg.acc[CWIID_X],
			       mesg[i].nunchuk_mesg.acc[CWIID_Y],
			       mesg[i].nunchuk_mesg.acc[CWIID_Z]);
			break;
		case CWIID_MESG_CLASSIC:
			printf("Classic Report: btns=%.4X l_stick=(%d,%d) r_stick=(%d,%d) "
			       "l=%d r=%d\n", mesg[i].classic_mesg.buttons,
			       mesg[i].classic_mesg.l_stick[CWIID_X],
			       mesg[i].classic_mesg.l_stick[CWIID_Y],
			       mesg[i].classic_mesg.r_stick[CWIID_X],
			       mesg[i].classic_mesg.r_stick[CWIID_Y],
			       mesg[i].classic_mesg.l, mesg[i].classic_mesg.r);
			break;
		case CWIID_MESG_BALANCE:
			printf("Balance Report: right_top=%d right_bottom=%d "
			       "left_top=%d left_bottom=%d\n",
			       mesg[i].balance_mesg.right_top,
			       mesg[i].balance_mesg.right_bottom,
			       mesg[i].balance_mesg.left_top,
			       mesg[i].balance_mesg.left_bottom);
			break;
		case CWIID_MESG_MOTIONPLUS:
			printf("MotionPlus Report: angle_rate=(%d,%d,%d)\n",
			       mesg[i].motionplus_mesg.angle_rate[0],
			       mesg[i].motionplus_mesg.angle_rate[1],
			       mesg[i].motionplus_mesg.angle_rate[2]);
			break;
		case CWIID_MESG_ERROR:
			if (cwiid_close(wiimote)) {
				fprintf(stderr, "Error on wiimote disconnect\n");
				exit(-1);
			}
			exit(0);
			break;
		default:
			printf("Unknown Report");
			break;
		}
	}
}
