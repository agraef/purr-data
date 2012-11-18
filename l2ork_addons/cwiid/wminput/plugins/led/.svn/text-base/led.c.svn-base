#include "wmplugin.h"
#include <math.h>


cwiid_wiimote_t *wiimote;

static unsigned char info_init = 0;
static struct wmplugin_info info;
static struct wmplugin_data data;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;

static int Led1 = 0;
static int Led2 = 0;
static int Led3 = 0;
static int Led4 = 0;
static int Battery = 0;
static uint8_t Button = CWIID_BTN_A + CWIID_BTN_B;

static void show_battery();

struct wmplugin_info *wmplugin_info()
{
	if (!info_init) {
		info.button_count = 0;
		info.axis_count = 0;
		info.param_count = 6;
		info.param_info[0].name = "Led1";
		info.param_info[0].type = WMPLUGIN_PARAM_INT;
		info.param_info[0].ptr = &Led1;
		info.param_info[1].name = "Led2";
		info.param_info[1].type = WMPLUGIN_PARAM_INT;
		info.param_info[1].ptr = &Led2;
		info.param_info[2].name = "Led3";
		info.param_info[2].type = WMPLUGIN_PARAM_INT;
		info.param_info[2].ptr = &Led3;
		info.param_info[3].name = "Led4";
		info.param_info[3].type = WMPLUGIN_PARAM_INT;
		info.param_info[3].ptr = &Led4;
		info.param_info[4].name = "Battery";
		info.param_info[4].type = WMPLUGIN_PARAM_INT;
		info.param_info[4].ptr = &Battery;
		info.param_info[5].name = "Button";
		info.param_info[5].type = WMPLUGIN_PARAM_INT;
		info.param_info[5].ptr = &Button;		
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, cwiid_wiimote_t *arg_wiimote)
{
	wiimote = arg_wiimote;

	uint8_t led_state = (Led1 ? CWIID_LED1_ON : 0)
	                  | (Led2 ? CWIID_LED2_ON : 0)
	                  | (Led3 ? CWIID_LED3_ON : 0)
	                  | (Led4 ? CWIID_LED4_ON : 0);

	cwiid_command(wiimote, CWIID_CMD_LED, led_state);
	
	if (wmplugin_set_rpt_mode(id, CWIID_RPT_BTN)) {
		return -1;
	}	

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg mesg[])
{
	int i;
	uint8_t button;
	struct cwiid_btn_message *btn_mesg;

	uint8_t led_state = (Led1 ? CWIID_LED1_ON : 0)
	                  | (Led2 ? CWIID_LED2_ON : 0)
	                  | (Led3 ? CWIID_LED3_ON : 0)
	                  | (Led4 ? CWIID_LED4_ON : 0);

	if(Battery != 0) {		
		btn_mesg = NULL;
		for (i=0; i < mesg_count; i++) {
			if (mesg[i].type == CWIID_MESG_BTN) {
				btn_mesg = &mesg[i].btn_mesg;
				button = mesg[i].btn_mesg.buttons;
			}
		}

		if (!btn_mesg) {
			return NULL;
		}

		if(button == Button) {
			show_battery();
		} 
		else {
			cwiid_command(wiimote, CWIID_CMD_LED, led_state);
		}
	}
	
	return &data;
}

static void show_battery()
{
	struct cwiid_state state;
	float battery_percent;
	int battery_led;

	cwiid_get_state(wiimote, &state);
	
	// calculate battery as a percent, then decide how many leds to light up	
	battery_percent = (100*state.battery / CWIID_BATTERY_MAX);
	battery_led = ceil(battery_percent / 25);

	switch(battery_led) {
	case 1:
		cwiid_set_led(wiimote, CWIID_LED1_ON);
		break;
	case 2:
		cwiid_set_led(wiimote, CWIID_LED1_ON | CWIID_LED2_ON );
		break;
	case 3:
		cwiid_set_led(wiimote, CWIID_LED1_ON | CWIID_LED2_ON | CWIID_LED3_ON );
		break;
	case 4:
		cwiid_set_led(wiimote, CWIID_LED1_ON | CWIID_LED2_ON | CWIID_LED3_ON | CWIID_LED4_ON );
		break;
	default:
		break;
	}
}
