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
 *  2007-07-28 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added config.h include
 *  * use PACKAGE_VERSION from config.h instead of CWIID_VERSION
 *
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_{connect,disconnect,command} to
 *    cwiid_{open,close,request_status|set_led|set_rumble|set_rpt_mode}
 *
 *  2007-05-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added timestamp to message callback
 *  * use cwiid_get_acc_cal to get acc calibration values
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for libcwiid rename
 *
 *  2007-04-08 L. Donnie Smith <cwiid@anstrakraft.org>
 *  * fixed signed/unsigned comparison warning in btnRead_clicked
 *
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * disconnect on cwiid_mesg_error
 *
 *  2007-04-03 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * commented custom cwiid_err (causing Xlib errors)
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial Changelog
 *  * type audit (stdint, const, char booleans)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define APP_NAME		"CWiid wmgui"
#define APP_VERSION		PACKAGE_VERSION
#define APP_COPYRIGHT	"Copyright (C) 2007 L. Donnie Smith " \
                        "<cwiid@abstrakraft.org>"
#define APP_COMMENTS	"Wiimote GUI"

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"

#include <bluetooth/bluetooth.h>
#include "cwiid.h"

#define PI	3.14159265358979323

struct stick {
	char valid;
	uint8_t x;
	uint8_t y;
	uint8_t max;
};

/* Globals */
cwiid_wiimote_t *wiimote = NULL;
bdaddr_t bdaddr;
struct acc_cal wm_cal, nc_cal;
struct cwiid_ir_mesg ir_data;
struct stick nc_stick;
struct stick cc_l_stick, cc_r_stick;

/* Widgets */
GtkWidget *winMain;
GtkWidget *winRW;
GtkWidget *winDialog;
GtkWidget *menuConnect, *menuDisconnect, *menuQuit, *menuRW, *menuAbout;
GtkWidget *chkAcc, *chkIR, *chkExt;
GtkWidget *chkLED1, *chkLED2, *chkLED3, *chkLED4;
GtkWidget *chkRumble;
GtkWidget *evUp, *evDown, *evLeft, *evRight, *evA, *evB,
          *evMinus, *evPlus, *evHome, *ev1, *ev2;
GtkWidget *lblUp, *lblDown, *lblLeft, *lblRight, *lblA, *lblB,
          *lblMinus, *lblPlus, *lblHome, *lbl1, *lbl2;
GtkWidget *lblAccX, *lblAccY, *lblAccZ;
GtkWidget *lblAccXVal, *lblAccYVal, *lblAccZVal;
GtkWidget *progAccX, *progAccY, *progAccZ;
GtkWidget *lblAcc, *lblRoll, *lblPitch;
GtkWidget *lblAccVal, *lblRollVal, *lblPitchVal;
GtkWidget *lblIR;
GtkWidget *drawIR;
GtkWidget *lblNC;
GtkWidget *drawNCStick;
GtkWidget *evNCC, *evNCZ;
GtkWidget *lblNCC, *lblNCZ;
GtkWidget *lblNCAccX, *lblNCAccY, *lblNCAccZ;
GtkWidget *lblNCAccXVal, *lblNCAccYVal, *lblNCAccZVal;
GtkWidget *progNCAccX, *progNCAccY, *progNCAccZ;
GtkWidget *lblNCAcc, *lblNCRoll, *lblNCPitch;
GtkWidget *lblNCAccVal, *lblNCRollVal, *lblNCPitchVal;
GtkWidget *evCCUp, *evCCDown, *evCCLeft, *evCCRight, *evCCMinus, *evCCPlus,
          *evCCHome, *evCCA, *evCCB, *evCCX, *evCCY, *evCCZL, *evCCZR;
GtkWidget *lblCCUp, *lblCCDown, *lblCCLeft, *lblCCRight, *lblCCMinus,
          *lblCCPlus, *lblCCHome, *lblCCA, *lblCCB, *lblCCX, *lblCCY, *lblCCZL,
          *lblCCZR;
GtkWidget *drawCCLStick, *drawCCRStick;
GtkWidget *evCCL, *evCCR;
GtkWidget *lblCCL, *lblCCR;
GtkWidget *lblCCLVal, *lblCCRVal;
GtkWidget *progCCL, *progCCR;
GtkWidget *statConnection, *statBattery, *statExtension;
GtkWidget *txtReadOffset, *txtReadLen;
GtkWidget *radReadEEPROM, *radReadReg;
GtkWidget *btnRead;
GtkWidget *txtWriteOffset, *txtWriteData;
GtkWidget *radWriteEEPROM, *radWriteReg;
GtkWidget *btnWrite;
GtkWidget *tvRW;
GtkWidget *btnRWClose;
GtkWidget *btnBeep;

GtkTextBuffer *tbRW;

GdkColor btn_on, btn_off;

/* Utility functions */
void set_gui_state();
void clear_widgets();
void clear_acc_widgets();
void clear_ir_data();
void clear_nunchuk_widgets();
void clear_classic_widgets();
void message(GtkMessageType type, const gchar *message, GtkWindow *parent);
void status(const gchar *status);

/* GTK Callbacks */
gboolean winMain_delete_event(void);
gboolean winRW_delete_event(void);
void menuConnect_activate(void);
void menuDisconnect_activate(void);
void menuQuit_activate(void);
void menuRW_activate(void);
void menuAbout_activate(void);
void chkAcc_toggled(void);
void chkIR_toggled(void);
void chkExt_toggled(void);
void chkLED_toggled(void);
void chkRumble_toggled(void);
void drawIR_expose_event(void);
void drawStick_expose_event(GtkWidget *, GdkEventExpose *, struct stick *);
void btnRead_clicked(void);
void btnWrite_clicked(void);
void btnRWClose_clicked(void);
void btnBeep_clicked(void);

void set_report_mode(void);

/* Wiimote Callback */
cwiid_mesg_callback_t cwiid_callback;

/* Wiimote Handler Functions */
void cwiid_btn(struct cwiid_btn_mesg *);
void cwiid_acc(struct cwiid_acc_mesg *);
void cwiid_ir(struct cwiid_ir_mesg *);
void cwiid_nunchuk(struct cwiid_nunchuk_mesg *);
void cwiid_classic(struct cwiid_classic_mesg *);

/* GetOpt */
#define OPTSTRING	"h"
extern char *optarg;
extern int optind, opterr, optopt;

#define USAGE "usage:%s [-h] [BDADDR]\n"

/*
cwiid_err_t err;

void err(int id, const char *s, ...)
{
	message(GTK_MESSAGE_ERROR, s, GTK_WINDOW(winMain));
}
*/

int main (int argc, char *argv[])
{
	int c;
	char *str_addr;

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
	gdk_threads_init();
	gdk_threads_enter();

	/* cwiid_set_err(err); */

	/* Parse Options */
	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		switch (c) {
		case 'h':
			printf(USAGE, argv[0]);
			return 0;
			break;
		case '?':
			return -1;
			break;
		default:
			printf("unknown command-line option: -%c\n", c);
			break;
		}
	}

	/* BDADDR */
	if (optind < argc) {
		if (str2ba(argv[optind], &bdaddr)) {
			printf("invalid bdaddr\n");
			bdaddr = *BDADDR_ANY;
		}
		optind++;
		if (optind < argc) {
			printf("invalid command-line\n");
			printf(USAGE, argv[0]);
			return -1;
		}
	}
	else if ((str_addr = getenv(WIIMOTE_BDADDR)) != NULL) {
		if (str2ba(str_addr, &bdaddr)) {
			printf("invalid address in %s\n", WIIMOTE_BDADDR);
			bdaddr = *BDADDR_ANY;
		}
	}
	else {
		bdaddr = *BDADDR_ANY;
	}		

	/* Create the window */
	winMain = create_winMain();
	winRW = create_winRW();

	/* Lookup Widgets */
	menuConnect = lookup_widget(winMain, "menuConnect");
	menuDisconnect = lookup_widget(winMain, "menuDisconnect");
	menuQuit = lookup_widget(winMain, "menuQuit");
	menuRW = lookup_widget(winMain, "menuRW");
	menuAbout = lookup_widget(winMain, "menuAbout");
	chkAcc = lookup_widget(winMain, "chkAcc");
	chkIR = lookup_widget(winMain, "chkIR");
	chkExt = lookup_widget(winMain, "chkExt");
	chkLED1 = lookup_widget(winMain, "chkLED1");
	chkLED2 = lookup_widget(winMain, "chkLED2");
	chkLED3 = lookup_widget(winMain, "chkLED3");
	chkLED4 = lookup_widget(winMain, "chkLED4");
	chkRumble = lookup_widget(winMain, "chkRumble");
	evUp = lookup_widget(winMain, "evUp");
	evDown = lookup_widget(winMain, "evDown");
	evLeft = lookup_widget(winMain, "evLeft");
	evRight = lookup_widget(winMain, "evRight");
	evA = lookup_widget(winMain, "evA");
	evB = lookup_widget(winMain, "evB");
	evMinus = lookup_widget(winMain, "evMinus");
	evPlus = lookup_widget(winMain, "evPlus");
	evHome = lookup_widget(winMain, "evHome");
	ev1 = lookup_widget(winMain, "ev1");
	ev2 = lookup_widget(winMain, "ev2");
	lblUp = lookup_widget(winMain, "lblUp");
	lblDown = lookup_widget(winMain, "lblDown");
	lblLeft = lookup_widget(winMain, "lblLeft");
	lblRight = lookup_widget(winMain, "lblRight");
	lblA = lookup_widget(winMain, "lblA");
	lblB = lookup_widget(winMain, "lblB");
	lblMinus = lookup_widget(winMain, "lblMinus");
	lblPlus = lookup_widget(winMain, "lblPlus");
	lblHome = lookup_widget(winMain, "lblHome");
	lbl1 = lookup_widget(winMain, "lbl1");
	lbl2 = lookup_widget(winMain, "lbl2");
	lblAccX = lookup_widget(winMain, "lblAccX");
	lblAccY = lookup_widget(winMain, "lblAccY");
	lblAccZ = lookup_widget(winMain, "lblAccZ");
	lblAccXVal = lookup_widget(winMain, "lblAccXVal");
	lblAccYVal = lookup_widget(winMain, "lblAccYVal");
	lblAccZVal = lookup_widget(winMain, "lblAccZVal");
	progAccX = lookup_widget(winMain, "progAccX");
	progAccY = lookup_widget(winMain, "progAccY");
	progAccZ = lookup_widget(winMain, "progAccZ");
	lblAcc = lookup_widget(winMain, "lblAcc");
	lblRoll = lookup_widget(winMain, "lblRoll");
	lblPitch = lookup_widget(winMain, "lblPitch");
	lblAccVal = lookup_widget(winMain, "lblAccVal");
	lblRollVal = lookup_widget(winMain, "lblRollVal");
	lblPitchVal = lookup_widget(winMain, "lblPitchVal");
	lblIR = lookup_widget(winMain, "lblIR");
	drawIR = lookup_widget(winMain, "drawIR");
	lblNC = lookup_widget(winMain, "lblNC");
	drawNCStick = lookup_widget(winMain, "drawNCStick");
	evNCC = lookup_widget(winMain, "evNCC");
	evNCZ = lookup_widget(winMain, "evNCZ");
	lblNCC = lookup_widget(winMain, "lblNCC");
	lblNCZ = lookup_widget(winMain, "lblNCZ");
	lblNCAccX = lookup_widget(winMain, "lblNCAccX");
	lblNCAccY = lookup_widget(winMain, "lblNCAccY");
	lblNCAccZ = lookup_widget(winMain, "lblNCAccZ");
	lblNCAccXVal = lookup_widget(winMain, "lblNCAccXVal");
	lblNCAccYVal = lookup_widget(winMain, "lblNCAccYVal");
	lblNCAccZVal = lookup_widget(winMain, "lblNCAccZVal");
	progNCAccX = lookup_widget(winMain, "progNCAccX");
	progNCAccY = lookup_widget(winMain, "progNCAccY");
	progNCAccZ = lookup_widget(winMain, "progNCAccZ");
	lblNCAcc = lookup_widget(winMain, "lblNCAcc");
	lblNCRoll = lookup_widget(winMain, "lblNCRoll");
	lblNCPitch = lookup_widget(winMain, "lblNCPitch");
	lblNCAccVal = lookup_widget(winMain, "lblNCAccVal");
	lblNCRollVal = lookup_widget(winMain, "lblNCRollVal");
	lblNCPitchVal = lookup_widget(winMain, "lblNCPitchVal");
	evCCUp = lookup_widget(winMain, "evCCUp");
	evCCDown = lookup_widget(winMain, "evCCDown");
	evCCLeft = lookup_widget(winMain, "evCCLeft");
	evCCRight = lookup_widget(winMain, "evCCRight");
	evCCMinus = lookup_widget(winMain, "evCCMinus");
	evCCPlus = lookup_widget(winMain, "evCCPlus");
	evCCHome = lookup_widget(winMain, "evCCHome");
	evCCA = lookup_widget(winMain, "evCCA");
	evCCB = lookup_widget(winMain, "evCCB");
	evCCX = lookup_widget(winMain, "evCCX");
	evCCY = lookup_widget(winMain, "evCCY");
	evCCZL = lookup_widget(winMain, "evCCZL");
	evCCZR = lookup_widget(winMain, "evCCZR");
	lblCCUp = lookup_widget(winMain, "lblCCUp");
	lblCCDown = lookup_widget(winMain, "lblCCDown");
	lblCCLeft = lookup_widget(winMain, "lblCCLeft");
	lblCCRight = lookup_widget(winMain, "lblCCRight");
	lblCCMinus = lookup_widget(winMain, "lblCCMinus");
	lblCCPlus = lookup_widget(winMain, "lblCCPlus");
	lblCCHome = lookup_widget(winMain, "lblCCHome");
	lblCCA = lookup_widget(winMain, "lblCCA");
	lblCCB = lookup_widget(winMain, "lblCCB");
	lblCCX = lookup_widget(winMain, "lblCCX");
	lblCCY = lookup_widget(winMain, "lblCCY");
	lblCCZL = lookup_widget(winMain, "lblCCZL");
	lblCCZR = lookup_widget(winMain, "lblCCZR");
	drawCCLStick = lookup_widget(winMain, "drawCCLStick");
	drawCCRStick = lookup_widget(winMain, "drawCCRStick");
	evCCL = lookup_widget(winMain, "evCCL");
	evCCR = lookup_widget(winMain, "evCCR");
	lblCCL = lookup_widget(winMain, "lblCCL");
	lblCCR = lookup_widget(winMain, "lblCCR");
	lblCCLVal = lookup_widget(winMain, "lblCCLVal");
	lblCCRVal = lookup_widget(winMain, "lblCCRVal");
	progCCL = lookup_widget(winMain, "progCCL");
	progCCR = lookup_widget(winMain, "progCCR");
	statConnection = lookup_widget(winMain, "statConnection");
	statBattery = lookup_widget(winMain, "statBattery");
	statExtension = lookup_widget(winMain, "statExtension");
	txtReadOffset = lookup_widget(winRW, "txtReadOffset");
	txtReadLen = lookup_widget(winRW, "txtReadLen");
	radReadEEPROM = lookup_widget(winRW, "radReadEEPROM");
	radReadReg = lookup_widget(winRW, "radReadReg");
	btnRead = lookup_widget(winRW, "btnRead");
	txtWriteOffset = lookup_widget(winRW, "txtWriteOffset");
	txtWriteData = lookup_widget(winRW, "txtWriteData");
	radWriteEEPROM = lookup_widget(winRW, "radWriteEEPROM");
	radWriteReg = lookup_widget(winRW, "radWriteReg");
	btnWrite = lookup_widget(winRW, "btnWrite");
	tvRW = lookup_widget(winRW, "tvRW");
	btnRWClose = lookup_widget(winRW, "btnRWClose");
	btnBeep = lookup_widget(winMain, "btnBeep");
	
	tbRW = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tvRW));

	/* Connect Callbacks */
	g_signal_connect(winMain, "delete_event", G_CALLBACK(winMain_delete_event),
	                 NULL);
	g_signal_connect(winRW, "delete_event", G_CALLBACK(winRW_delete_event),
	                 NULL);
	g_signal_connect(menuConnect, "activate", G_CALLBACK(menuConnect_activate),
	                 NULL);
	g_signal_connect(menuDisconnect, "activate",
	                 G_CALLBACK(menuDisconnect_activate), NULL);
	g_signal_connect(menuQuit, "activate", G_CALLBACK(menuQuit_activate),
	                 NULL);
	g_signal_connect(menuRW, "activate", G_CALLBACK(menuRW_activate), NULL);
	g_signal_connect(menuAbout, "activate", G_CALLBACK(menuAbout_activate),
	                 NULL);
	g_signal_connect(chkAcc, "toggled", G_CALLBACK(chkAcc_toggled), NULL);
	g_signal_connect(chkIR, "toggled", G_CALLBACK(chkIR_toggled), NULL);
	g_signal_connect(chkExt, "toggled", G_CALLBACK(chkExt_toggled), NULL);
	g_signal_connect(chkLED1, "toggled", G_CALLBACK(chkLED_toggled), NULL);
	g_signal_connect(chkLED2, "toggled", G_CALLBACK(chkLED_toggled), NULL);
	g_signal_connect(chkLED3, "toggled", G_CALLBACK(chkLED_toggled), NULL);
	g_signal_connect(chkLED4, "toggled", G_CALLBACK(chkLED_toggled), NULL);
	g_signal_connect(chkRumble, "toggled", G_CALLBACK(chkRumble_toggled),
	                 NULL);
	g_signal_connect(drawIR, "expose_event",
	                 G_CALLBACK(drawIR_expose_event), NULL);
	g_signal_connect(drawNCStick, "expose_event",
	                 G_CALLBACK(drawStick_expose_event), &nc_stick);
	g_signal_connect(drawCCLStick, "expose_event",
	                 G_CALLBACK(drawStick_expose_event), &cc_l_stick);
	g_signal_connect(drawCCRStick, "expose_event",
	                 G_CALLBACK(drawStick_expose_event), &cc_r_stick);
	g_signal_connect(btnRead, "clicked", G_CALLBACK(btnRead_clicked), NULL);
	g_signal_connect(btnWrite, "clicked", G_CALLBACK(btnWrite_clicked), NULL);
	g_signal_connect(btnRWClose, "clicked", G_CALLBACK(btnRWClose_clicked),
	                 NULL);
	g_signal_connect(btnBeep, "clicked", G_CALLBACK(btnBeep_clicked), NULL);

	/* Initialize */
	btn_on.red = 0; btn_on.blue = 0; btn_on.green = 0xFFFF;
	btn_off = gtk_widget_get_style(evUp)->bg[GTK_STATE_NORMAL];

	nc_stick.max = 0xFF;
	cc_l_stick.max = CWIID_CLASSIC_L_STICK_MAX;
	cc_r_stick.max = CWIID_CLASSIC_R_STICK_MAX;

	set_gui_state();
	clear_widgets();
	status("No connection");

	gtk_widget_show(winMain);

	gtk_main();
	gdk_threads_leave();
	return 0;
}

void message(GtkMessageType type, const gchar *message, GtkWindow *parent)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(parent, 0, type, GTK_BUTTONS_OK, message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void status(const gchar *status)
{
	gtk_statusbar_push(GTK_STATUSBAR(statConnection), 0, status);
}

void set_gui_state()
{
	gboolean connected;
	gboolean acc_active;
	gboolean ext_active;

	connected = wiimote ? TRUE : FALSE;
	/* Set Input Widget Sensitivities */
	gtk_widget_set_sensitive(menuConnect, !connected);
	gtk_widget_set_sensitive(menuDisconnect, connected);
	gtk_widget_set_sensitive(chkLED1, connected);
	gtk_widget_set_sensitive(chkLED2, connected);
	gtk_widget_set_sensitive(chkLED3, connected);
	gtk_widget_set_sensitive(chkLED4, connected);
	gtk_widget_set_sensitive(chkRumble, connected);
	gtk_widget_set_sensitive(btnRead, connected);
	gtk_widget_set_sensitive(btnWrite, connected);

	/* Set Button Sensitivities */
	gtk_widget_set_sensitive(lblUp, connected);
	gtk_widget_set_sensitive(lblDown, connected);
	gtk_widget_set_sensitive(lblLeft, connected);
	gtk_widget_set_sensitive(lblRight, connected);
	gtk_widget_set_sensitive(lblA, connected);
	gtk_widget_set_sensitive(lblB, connected);
	gtk_widget_set_sensitive(lblMinus, connected);
	gtk_widget_set_sensitive(lblPlus, connected);
	gtk_widget_set_sensitive(lblHome, connected);
	gtk_widget_set_sensitive(lbl1, connected);
	gtk_widget_set_sensitive(lbl2, connected);

	acc_active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkAcc));
	/* Set Acc Widget Sensitivities */
	gtk_widget_set_sensitive(lblAccX, acc_active);
	gtk_widget_set_sensitive(lblAccY, acc_active);
	gtk_widget_set_sensitive(lblAccZ, acc_active);
	gtk_widget_set_sensitive(lblAccXVal, acc_active);
	gtk_widget_set_sensitive(lblAccYVal, acc_active);
	gtk_widget_set_sensitive(lblAccZVal, acc_active);
	gtk_widget_set_sensitive(progAccX, acc_active);
	gtk_widget_set_sensitive(progAccY, acc_active);
	gtk_widget_set_sensitive(progAccZ, acc_active);
	gtk_widget_set_sensitive(lblAcc, acc_active);
	gtk_widget_set_sensitive(lblRoll, acc_active);
	gtk_widget_set_sensitive(lblPitch, acc_active);
	gtk_widget_set_sensitive(lblAccVal, acc_active);
	gtk_widget_set_sensitive(lblRollVal, acc_active);
	gtk_widget_set_sensitive(lblPitchVal, acc_active);

	/* Set IC Widget Sensitivities */
	gtk_widget_set_sensitive(lblIR,
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkIR)));

	ext_active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkExt));
	/* Set Extension Widget Sensitivities */
	gtk_widget_set_sensitive(lblNCC, ext_active);
	gtk_widget_set_sensitive(lblNCZ, ext_active);
	gtk_widget_set_sensitive(lblNCAccX, ext_active);
	gtk_widget_set_sensitive(lblNCAccY, ext_active);
	gtk_widget_set_sensitive(lblNCAccZ, ext_active);
	gtk_widget_set_sensitive(lblNCAccXVal, ext_active);
	gtk_widget_set_sensitive(lblNCAccYVal, ext_active);
	gtk_widget_set_sensitive(lblNCAccZVal, ext_active);
	gtk_widget_set_sensitive(progNCAccX, ext_active);
	gtk_widget_set_sensitive(progNCAccY, ext_active);
	gtk_widget_set_sensitive(progNCAccZ, ext_active);
	gtk_widget_set_sensitive(lblNCAcc, ext_active);
	gtk_widget_set_sensitive(lblNCRoll, ext_active);
	gtk_widget_set_sensitive(lblNCPitch, ext_active);
	gtk_widget_set_sensitive(lblNCAccVal, ext_active);
	gtk_widget_set_sensitive(lblNCRollVal, ext_active);
	gtk_widget_set_sensitive(lblNCPitchVal, ext_active);
	gtk_widget_set_sensitive(lblCCUp, ext_active);
	gtk_widget_set_sensitive(lblCCDown, ext_active);
	gtk_widget_set_sensitive(lblCCLeft, ext_active);
	gtk_widget_set_sensitive(lblCCRight, ext_active);
	gtk_widget_set_sensitive(lblCCMinus, ext_active);
	gtk_widget_set_sensitive(lblCCPlus, ext_active);
	gtk_widget_set_sensitive(lblCCHome, ext_active);
	gtk_widget_set_sensitive(lblCCA, ext_active);
	gtk_widget_set_sensitive(lblCCB, ext_active);
	gtk_widget_set_sensitive(lblCCX, ext_active);
	gtk_widget_set_sensitive(lblCCY, ext_active);
	gtk_widget_set_sensitive(lblCCZL, ext_active);
	gtk_widget_set_sensitive(lblCCZR, ext_active);
	gtk_widget_set_sensitive(lblCCL, ext_active);
	gtk_widget_set_sensitive(lblCCR, ext_active);
	gtk_widget_set_sensitive(lblCCLVal, ext_active);
	gtk_widget_set_sensitive(lblCCRVal, ext_active);
	gtk_widget_set_sensitive(progCCL, ext_active);
	gtk_widget_set_sensitive(progCCR, ext_active);
}

void clear_widgets()
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(chkLED1), FALSE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(chkLED2), FALSE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(chkLED3), FALSE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(chkLED4), FALSE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(chkRumble), FALSE);

	gtk_statusbar_push(GTK_STATUSBAR(statBattery), 0, "");
	gtk_statusbar_push(GTK_STATUSBAR(statExtension), 0, "");

	clear_acc_widgets();
	clear_ir_data();
	clear_nunchuk_widgets();
	clear_classic_widgets();
}

void clear_acc_widgets()
{
	gtk_label_set_text(GTK_LABEL(lblAccXVal), "0");
	gtk_label_set_text(GTK_LABEL(lblAccYVal), "0");
	gtk_label_set_text(GTK_LABEL(lblAccZVal), "0");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progAccX), 0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progAccY), 0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progAccZ), 0.0);
	gtk_label_set_text(GTK_LABEL(lblAccVal), "0");
	gtk_label_set_text(GTK_LABEL(lblRollVal), "0");
	gtk_label_set_text(GTK_LABEL(lblPitchVal), "0");
}

void clear_ir_data()
{
	int i;

	for (i=0; i < CWIID_IR_SRC_COUNT; i++) {
		ir_data.src[i].pos[CWIID_X] = -1;
		ir_data.src[i].pos[CWIID_Y] = -1;
		ir_data.src[i].size = -1;
	}
	gtk_widget_queue_draw(drawIR);
}

void clear_nunchuk_widgets()
{
	nc_stick.valid = 0;
	gtk_widget_queue_draw(drawNCStick);

	gtk_label_set_text(GTK_LABEL(lblNCAccXVal), "0");
	gtk_label_set_text(GTK_LABEL(lblNCAccYVal), "0");
	gtk_label_set_text(GTK_LABEL(lblNCAccZVal), "0");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progNCAccX), 0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progNCAccY), 0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progNCAccZ), 0.0);
	gtk_label_set_text(GTK_LABEL(lblNCAccVal), "0");
	gtk_label_set_text(GTK_LABEL(lblNCRollVal), "0");
	gtk_label_set_text(GTK_LABEL(lblNCPitchVal), "0");
}

void clear_classic_widgets()
{
	cc_l_stick.valid = 0;
	gtk_widget_queue_draw(drawCCLStick);
	cc_r_stick.valid = 0;
	gtk_widget_queue_draw(drawCCRStick);

	gtk_label_set_text(GTK_LABEL(lblCCLVal), "0");
	gtk_label_set_text(GTK_LABEL(lblCCRVal), "0");
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progCCL), 0.0);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progCCR), 0.0);
}

gboolean winMain_delete_event(void)
{
	menuQuit_activate();
	return FALSE;
}

gboolean winRW_delete_event(void)
{
	btnRWClose_clicked();
	return TRUE;
}

void menuConnect_activate(void)
{
	char reset_bdaddr = 0;

	if (bacmp(&bdaddr, BDADDR_ANY) == 0) {
		reset_bdaddr = 1;
	}
	message(GTK_MESSAGE_INFO,
	        "Put Wiimote in discoverable mode (press 1+2) and press OK",
	         GTK_WINDOW(winMain));
	if ((wiimote = cwiid_open(&bdaddr, CWIID_FLAG_MESG_IFC)) == NULL) {
		message(GTK_MESSAGE_ERROR, "Unable to connect", GTK_WINDOW(winMain));
		status("No connection");
	}
	else if (cwiid_set_mesg_callback(wiimote, &cwiid_callback)) {
		message(GTK_MESSAGE_ERROR, "Error setting callback",
		        GTK_WINDOW(winMain));
		if (cwiid_close(wiimote)) {
			message(GTK_MESSAGE_ERROR, "Error on disconnect",
			        GTK_WINDOW(winMain));
		}
		wiimote = NULL;
		status("No connection");
	}
	else {
		status("Connected");
		if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NONE, &wm_cal)) {
			message(GTK_MESSAGE_ERROR, "Unable to retrieve accelerometer "
			        "calibration", GTK_WINDOW(winMain));
		}
		set_gui_state();
		set_report_mode();
		cwiid_enable(wiimote, CWIID_FLAG_MOTIONPLUS);
		cwiid_request_status(wiimote);
	}

	if (reset_bdaddr) {
		bdaddr = *BDADDR_ANY;
	}
}

void menuDisconnect_activate(void)
{
	if (cwiid_close(wiimote)) {
		message(GTK_MESSAGE_ERROR, "Error on disconnect", GTK_WINDOW(winMain));
	}
	wiimote = NULL;
	status("No connection");
	clear_widgets();
	set_gui_state();
}

void menuQuit_activate(void)
{
	if (wiimote) {
		menuDisconnect_activate();
	}
	gtk_main_quit();
}

void menuRW_activate(void)
{
	gtk_widget_show(winRW);
}

void menuAbout_activate(void)
{
	gtk_show_about_dialog(GTK_WINDOW(winMain),
	                      "name", APP_NAME,
	                      "version", APP_VERSION,
	                      "copyright", APP_COPYRIGHT,
	                      "comments", APP_COMMENTS,
	                      NULL);
}

void chkAcc_toggled(void)
{
	if (wiimote) {
		set_report_mode();
	}
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkAcc))) {
		clear_acc_widgets();
	}
	set_gui_state();
}

void chkIR_toggled(void)
{
	if (wiimote) {
		set_report_mode();
	}
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkIR))) {
		clear_ir_data();
	}
	set_gui_state();
}

void chkExt_toggled(void)
{
	if (wiimote) {
		set_report_mode();
	}
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkExt))) {
		clear_nunchuk_widgets();
		clear_classic_widgets();
	}
	set_gui_state();
}

void chkLED_toggled(void)
{
	uint8_t LED_state;

	if (wiimote) {
		LED_state =
		  (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkLED1))
		    ? CWIID_LED1_ON : 0) |
		  (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkLED2))
		    ? CWIID_LED2_ON : 0) |
		  (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkLED3))
		    ? CWIID_LED3_ON : 0) |
		  (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkLED4))
		    ? CWIID_LED4_ON : 0);
		if (cwiid_set_led(wiimote, LED_state)) {
			message(GTK_MESSAGE_ERROR, "error setting LEDs",
			        GTK_WINDOW(winMain));
		}
	}
}

void chkRumble_toggled(void)
{
	if (wiimote) {
		if (cwiid_set_rumble(wiimote,
		  gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkRumble)))) {
			message(GTK_MESSAGE_ERROR, "error setting rumble",
			        GTK_WINDOW(winMain));
		}
	}
}

void drawIR_expose_event(void)
{
	int i;
	int size;
	gint width, height;

	gdk_window_get_geometry(drawIR->window, NULL, NULL, &width, &height, NULL);

	for (i=0; i < CWIID_IR_SRC_COUNT; i++) {
		if (ir_data.src[i].valid) {
			if (ir_data.src[i].size == -1) {
				size = 3;
			}
			else {
				size = ir_data.src[i].size+1;
			}
			gdk_draw_arc(drawIR->window,
			             drawIR->style->fg_gc[GTK_WIDGET_STATE(drawIR)],
			             TRUE,
						 ir_data.src[i].pos[CWIID_X] * width / CWIID_IR_X_MAX,
						 height - ir_data.src[i].pos[CWIID_Y] * height /
			                      CWIID_IR_Y_MAX,
						 size, size,
						 0, 64 * 360);
		}
	}
}

void drawStick_expose_event(GtkWidget *drawStick, GdkEventExpose *event,
                            struct stick *stick)
{
	gint width, height;

	gdk_window_get_geometry(drawStick->window, NULL, NULL, &width, &height,
	                        NULL);
	gdk_draw_arc(drawStick->window,
	             drawStick->style->fg_gc[GTK_WIDGET_STATE(drawStick)],
				 FALSE,
				 0, 0, width-1, height-1, 0, 64*360);
	if (stick->valid) {
		gdk_draw_arc(drawStick->window,
		             drawStick->style->fg_gc[GTK_WIDGET_STATE(drawStick)],
		             TRUE,
		             (double)stick->x/stick->max*width - 2,
		             (1 - (double)stick->y/stick->max)*height - 2,
		             3, 3, 0, 64*360);
	}
}

void btnRead_clicked(void)
{
	static unsigned char buf[CWIID_MAX_READ_LEN];
	static char txt[CWIID_MAX_READ_LEN*4+50]; /* 3 chars per byte, with
	                                             * plenty extra */
	GtkTextIter text_iter;
	GtkTextMark *p_text_mark;
	char *cursor;
	unsigned int offset, len;
	int flags;
	unsigned int i;

	/* Decode arguments */
	offset = strtol(gtk_entry_get_text(GTK_ENTRY(txtReadOffset)), &cursor, 16);
	if (*cursor != '\0') {
		message(GTK_MESSAGE_ERROR, "Invalid read offset", GTK_WINDOW(winRW));
	}

	len = strtol(gtk_entry_get_text(GTK_ENTRY(txtReadLen)), &cursor, 16);
	if (*cursor != '\0') {
		message(GTK_MESSAGE_ERROR, "Invalid read len", GTK_WINDOW(winRW));
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radReadReg))) {
		flags = CWIID_RW_REG;
	}
	else {
		flags = CWIID_RW_EEPROM;
	}

	/* Make the call */
	if (cwiid_read(wiimote, flags, offset, len, buf)) {
		message(GTK_MESSAGE_ERROR, "Wiimote read error", GTK_WINDOW(winRW));
	}
	else {
		/* construct the hexedit-style string */
		cursor=txt;
		sprintf(cursor, "0x%08X:", offset & ~0xF);
		cursor+=11;
		for (i=0; i < (offset & 0xF); i++) {
			sprintf(cursor, "   ");
			cursor+=3;
		}
		for (i=0; i < len; i++) {
			if ((((i + offset) & 0xF) == 0) && (i!=0)) {
				sprintf(cursor, "\n0x%08X:", offset+i);
				cursor+=12;
			}
			if (((i +offset) & 0x7) == 0) {
				sprintf(cursor, " ");
				cursor++;
			}
			sprintf(cursor, "%02X ", buf[i]);
			cursor+=3;
		}
		sprintf(cursor, "\n\n");

		gtk_text_buffer_get_end_iter(tbRW, &text_iter);
		p_text_mark = gtk_text_buffer_create_mark(tbRW, NULL, &text_iter,
		                                          TRUE);
		gtk_text_buffer_insert(tbRW, &text_iter, txt, -1);
		gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tvRW), p_text_mark, 0.01,
		                             TRUE, 0.0, 0.0);
	}
}

char chartox(char c)
{
	char str[2];
	char *endptr;
	int val;

	str[0] = c;
	str[1] = '\0';
	val = strtol(str, &endptr, 16);
	if (*endptr != '\0') {
		return -1;
	}

	return (char)val;
}

#define MAX_WRITE_LEN	0x20
void btnWrite_clicked(void)
{
	static unsigned char buf[MAX_WRITE_LEN];
	char *cursor, *data;
	uint32_t offset;
	uint16_t len;
	uint8_t flags;

	/* Decode arguments */
	offset = strtol(gtk_entry_get_text(GTK_ENTRY(txtWriteOffset)), &cursor, 16);
	if (*cursor != '\0') {
		message(GTK_MESSAGE_ERROR, "Invalid read offset", GTK_WINDOW(winRW));
	}

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radWriteReg))) {
		flags = CWIID_RW_REG;
	}
	else {
		flags = CWIID_RW_EEPROM;
	}

	data = (char *) gtk_entry_get_text(GTK_ENTRY(txtWriteData));
	cursor = data;
	len = 0;
	while (*cursor != '\0') {
		if (len > MAX_WRITE_LEN) {
			message(GTK_MESSAGE_ERROR, "Message too long", GTK_WINDOW(winRW));
			return;
		}
		/* Trim Leading spaces */
		while (*cursor == ' ') {
			cursor++;
		}
		/* Test for end */
		if (*cursor == '\0') {
			break;
		}
		/* Read first nibble */
		if (!isxdigit((int)*cursor)) {
			message(GTK_MESSAGE_ERROR, "Invalid write data",
			        GTK_WINDOW(winRW));
			return;
		}
		buf[len] = chartox(*cursor)<<4;

		/* Read second nibble */
		cursor++;
		if (!isxdigit((int)*cursor)) {
			message(GTK_MESSAGE_ERROR,
			        "Invalid write data (digits must come in pairs)",
			        GTK_WINDOW(winRW));
			return;
		}
		buf[len] = buf[len] | chartox(*cursor);

		cursor++;
		len++;
	}

	if (len == 0) {
		message(GTK_MESSAGE_ERROR, "No write data", GTK_WINDOW(winRW));
		return;
	}

	/* Make the call */
	if (cwiid_write(wiimote, flags, offset, len, buf)) {
		message(GTK_MESSAGE_ERROR, "Wiimote write error", GTK_WINDOW(winRW));
	}
	else {
		message(GTK_MESSAGE_INFO, "Wiimote write successful",
		        GTK_WINDOW(winRW));
	}
}

void btnRWClose_clicked(void)
{
	gtk_widget_hide(winRW);
}

void btnBeep_clicked(void)
{
	/*if (cwiid_beep(wiimote)) {
		message(GTK_MESSAGE_ERROR, "Wiimote sound error", GTK_WINDOW(winMain));
	}*/
}

void set_report_mode(void)
{
	uint8_t rpt_mode;
	
	rpt_mode = CWIID_RPT_STATUS | CWIID_RPT_BTN;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkIR))) {
		rpt_mode |= CWIID_RPT_IR;
	}
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkAcc))) {
		rpt_mode |= CWIID_RPT_ACC;
	}
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkExt))) {
		rpt_mode |= CWIID_RPT_EXT;
	}
	if (cwiid_set_rpt_mode(wiimote, rpt_mode)) {
		message(GTK_MESSAGE_ERROR, "error setting report mode",
		        GTK_WINDOW(winMain));
	}
}

#define BATTERY_STR_LEN	14	/* "Battery: 100%" + '\0' */
void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
                    union cwiid_mesg mesg_array[], struct timespec *timestamp)
{
	int i;
	char battery[BATTERY_STR_LEN];
	char *ext_str;
	static enum cwiid_ext_type ext_type = CWIID_EXT_NONE;

	gdk_threads_enter();
	for (i=0; i < mesg_count; i++) {
		switch (mesg_array[i].type) {
		case CWIID_MESG_STATUS:
			snprintf(battery, BATTERY_STR_LEN,"Battery:%d%%",
			         (int) (100.0 * mesg_array[i].status_mesg.battery /
			                CWIID_BATTERY_MAX));
			gtk_statusbar_push(GTK_STATUSBAR(statBattery), 0, battery);
			switch (mesg_array[i].status_mesg.ext_type) {
			case CWIID_EXT_NONE:
				ext_str = "No extension";
				break;
			case CWIID_EXT_NUNCHUK:
				ext_str = "Nunchuk";
				if (ext_type != CWIID_EXT_NUNCHUK) {
					if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NUNCHUK,
					                      &nc_cal)) {
						message(GTK_MESSAGE_ERROR,
						        "Unable to retrieve accelerometer calibration",
						        GTK_WINDOW(winMain));
					}
				}
				break;
			case CWIID_EXT_CLASSIC:
				ext_str = "Classic controller";
				break;
			case CWIID_EXT_MOTIONPLUS:
				ext_str = "MotionPlus";
				break;
			case CWIID_EXT_UNKNOWN:
				ext_str = "Unknown extension";
				break;
			}
			gtk_statusbar_push(GTK_STATUSBAR(statExtension), 0, ext_str);
			clear_nunchuk_widgets();
			clear_classic_widgets();
			ext_type = mesg_array[i].status_mesg.ext_type;
			break;
		case CWIID_MESG_BTN:
			cwiid_btn(&mesg_array[i].btn_mesg);
			break;
		case CWIID_MESG_ACC:
			cwiid_acc(&mesg_array[i].acc_mesg);
			break;
		case CWIID_MESG_IR:
			cwiid_ir(&mesg_array[i].ir_mesg);
			break;
		case CWIID_MESG_NUNCHUK:
			cwiid_nunchuk(&mesg_array[i].nunchuk_mesg);
			break;
		case CWIID_MESG_CLASSIC:
			cwiid_classic(&mesg_array[i].classic_mesg);
			break;
		case CWIID_MESG_ERROR:
			menuDisconnect_activate();
			break;
		default:
			break;
		}
	}
	gdk_flush();
	gdk_threads_leave();
}

void cwiid_btn(struct cwiid_btn_mesg *mesg)
{
	gtk_widget_modify_bg(evUp, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_UP) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evDown, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_DOWN) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evLeft, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_LEFT) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evRight, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_RIGHT) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evA, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_A) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evB, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_B) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evMinus, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_MINUS) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evPlus, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_PLUS) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(evHome, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_HOME) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(ev1, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_1) ? &btn_on : &btn_off);
	gtk_widget_modify_bg(ev2, GTK_STATE_NORMAL,
	    (mesg->buttons & CWIID_BTN_2) ? &btn_on : &btn_off);
}

#define LBLVAL_LEN 6
void cwiid_acc(struct cwiid_acc_mesg *mesg)
{
	static gchar str[LBLVAL_LEN];
	double a_x, a_y, a_z, a;
	double roll, pitch;
	
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkAcc))) {
		g_snprintf(str, LBLVAL_LEN, "%X", mesg->acc[CWIID_X]);
		gtk_label_set_text(GTK_LABEL(lblAccXVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progAccX),
		                              (double)mesg->acc[CWIID_X]/0xFF);
		g_snprintf(str, LBLVAL_LEN, "%X", mesg->acc[CWIID_Y]);
		gtk_label_set_text(GTK_LABEL(lblAccYVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progAccY),
		                              (double)mesg->acc[CWIID_Y]/0xFF);
		g_snprintf(str, LBLVAL_LEN, "%X", mesg->acc[CWIID_Z]);
		gtk_label_set_text(GTK_LABEL(lblAccZVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progAccZ),
		                              (double)mesg->acc[CWIID_Z]/0xFF);

		a_x = ((double)mesg->acc[CWIID_X] - wm_cal.zero[CWIID_X]) /
		      (wm_cal.one[CWIID_X] - wm_cal.zero[CWIID_X]);
		a_y = ((double)mesg->acc[CWIID_Y] - wm_cal.zero[CWIID_Y]) /
		      (wm_cal.one[CWIID_Y] - wm_cal.zero[CWIID_Y]);
		a_z = ((double)mesg->acc[CWIID_Z] - wm_cal.zero[CWIID_Z]) /
		      (wm_cal.one[CWIID_Z] - wm_cal.zero[CWIID_Z]);
		a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));

		roll = atan(a_x/a_z);
		if (a_z <= 0.0) {
			roll += PI * ((a_x > 0.0) ? 1 : -1);
		}
		roll *= -1;

		pitch = atan(a_y/a_z*cos(roll));

		g_snprintf(str, LBLVAL_LEN, "%.2f", a);
		gtk_label_set_text(GTK_LABEL(lblAccVal), str);
		g_snprintf(str, LBLVAL_LEN, "%.2f", roll);
		gtk_label_set_text(GTK_LABEL(lblRollVal), str);
		g_snprintf(str, LBLVAL_LEN, "%.2f", pitch);
		gtk_label_set_text(GTK_LABEL(lblPitchVal), str);
	}
}

void cwiid_ir(struct cwiid_ir_mesg *mesg)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkIR))) {
		/* memcpy(&ir_data, mesg, sizeof(struct cwiid_ir_mesg)); */
		ir_data = *mesg;
		gtk_widget_queue_draw(drawIR);
	}
}

void cwiid_nunchuk(struct cwiid_nunchuk_mesg *mesg)
{
	static gchar str[LBLVAL_LEN];
	double a_x, a_y, a_z, a;
	double roll, pitch;
	
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkExt))) {
		gtk_widget_modify_bg(evNCC, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_NUNCHUK_BTN_C) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evNCZ, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_NUNCHUK_BTN_Z) ? &btn_on : &btn_off);

		nc_stick.valid = 1;
		nc_stick.x = mesg->stick[CWIID_X];
		nc_stick.y = mesg->stick[CWIID_Y];
		gtk_widget_queue_draw(drawNCStick);

		g_snprintf(str, LBLVAL_LEN, "%X", mesg->acc[CWIID_X]);
		gtk_label_set_text(GTK_LABEL(lblNCAccXVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progNCAccX),
		                              (double)mesg->acc[CWIID_X]/0xFF);
		g_snprintf(str, LBLVAL_LEN, "%X", mesg->acc[CWIID_Y]);
		gtk_label_set_text(GTK_LABEL(lblNCAccYVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progNCAccY),
		                              (double)mesg->acc[CWIID_Y]/0xFF);
		g_snprintf(str, LBLVAL_LEN, "%X", mesg->acc[CWIID_Z]);
		gtk_label_set_text(GTK_LABEL(lblNCAccZVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progNCAccZ),
		                              (double)mesg->acc[CWIID_Z]/0xFF);

		/* TODO: get nunchuk calibration */
		a_x = ((double)mesg->acc[CWIID_X] - nc_cal.zero[CWIID_X]) /
		      (nc_cal.one[CWIID_X] - nc_cal.zero[CWIID_X]);
		a_y = ((double)mesg->acc[CWIID_Y] - nc_cal.zero[CWIID_Y]) /
		      (nc_cal.one[CWIID_Y] - nc_cal.zero[CWIID_Y]);
		a_z = ((double)mesg->acc[CWIID_Z] - nc_cal.zero[CWIID_Z]) /
		      (nc_cal.one[CWIID_Z] - nc_cal.zero[CWIID_Z]);
		a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
		roll = atan(a_x/a_z);
		if (a_z <= 0.0) {
			roll += PI * ((a_x > 0.0) ? 1 : -1);
		}
		roll *= -1;

		pitch = atan(a_y/a_z*cos(roll));

		g_snprintf(str, LBLVAL_LEN, "%.2f", a);
		gtk_label_set_text(GTK_LABEL(lblNCAccVal), str);
		g_snprintf(str, LBLVAL_LEN, "%.2f", roll);
		gtk_label_set_text(GTK_LABEL(lblNCRollVal), str);
		g_snprintf(str, LBLVAL_LEN, "%.2f", pitch);
		gtk_label_set_text(GTK_LABEL(lblNCPitchVal), str);
	}
}

void cwiid_classic(struct cwiid_classic_mesg *mesg)
{
	static gchar str[LBLVAL_LEN];

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(chkExt))) {
		gtk_widget_modify_bg(evCCUp, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_UP) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCDown, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_DOWN) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCLeft, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_LEFT) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCRight, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_RIGHT) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCMinus, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_MINUS) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCPlus, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_PLUS) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCHome, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_HOME) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCA, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_A) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCB, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_B) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCX, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_X) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCY, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_Y) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCZL, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_ZL) ? &btn_on : &btn_off);
		gtk_widget_modify_bg(evCCZR, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_ZR) ? &btn_on : &btn_off);

		cc_l_stick.valid = 1;
		cc_l_stick.x = mesg->l_stick[CWIID_X];
		cc_l_stick.y = mesg->l_stick[CWIID_Y];
		gtk_widget_queue_draw(drawCCLStick);

		cc_r_stick.valid = 1;
		cc_r_stick.x = mesg->r_stick[CWIID_X];
		cc_r_stick.y = mesg->r_stick[CWIID_Y];
		gtk_widget_queue_draw(drawCCRStick);
		
		g_snprintf(str, LBLVAL_LEN, "%X", mesg->l);
		gtk_label_set_text(GTK_LABEL(lblCCLVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progCCL),
		                              (double)mesg->l/CWIID_CLASSIC_LR_MAX);
		gtk_widget_modify_bg(evCCL, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_L) ? &btn_on : &btn_off);
		g_snprintf(str, LBLVAL_LEN, "%X", mesg->r);
		gtk_label_set_text(GTK_LABEL(lblCCRVal), str);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progCCR),
		                              (double)mesg->r/CWIID_CLASSIC_LR_MAX);
		gtk_widget_modify_bg(evCCR, GTK_STATE_NORMAL,
		    (mesg->buttons & CWIID_CLASSIC_BTN_R) ? &btn_on : &btn_off);
	}
}

