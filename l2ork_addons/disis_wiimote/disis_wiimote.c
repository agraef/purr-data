// ===================================================================
// Wiimote external for Puredata by Ivica Ico Bukvic
// Based on work by Mike Wozniewki (Feb 2007), www.mikewoz.com
//
// Requires the L2Ork version of CWiid library (based on version 0.6.00 by L. Donnie Smith)
//
// ===================================================================
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
// ===================================================================

//  ChangeLog:
//  2008-04-14 Florian Krebs 
//  * adapt wiimote external for the actual version of cwiid (0.6.00)

//  ChangeLog:
//  2009-06-09 DISIS (Michael Hawthorne <rustmik2@gmail.com> & Ivica Ico Bukvic <ico@vt.edu>)
//  http://disis.music.vt.edu
//  * Bug-fixes (connecting and disconnecting crashes)
//  * Multithreaded implementation to prevent wiimote from starving PD audio thread
//  * Bang implementation to allow for better data rate control
//  * Updated help file

//  v 0.6.3 Changelog:
//  2009-10-05 DISIS (Ivica Ico Bukvic <ico@vt.edu>)
//  http://disis.music.vt.edu
//  * Total rewrite of the threaded design and tons of clean-up

//  v0.6.4 Changelog:
//  2010-03-21 DISIS (Ivica Ico Bukvic <ico@vt.edu>)
//  http://disis.music.vt.edu
//  * Reworked signalling system to use clock_delay()

//  v0.6.5 Changelog:
//  2010-09-15 DISIS (Ivica Ico Bukvic <ico@vt.edu>)
//  http://disis.music.vt.edu
//  * Added motionplus support
//	* Squashed bugs where expansion was not recognized on connect
//	* Fixed incorrect calibration on connect
//	* Other minor bugs'n'fixes

//  v0.6.6 Changelog:
//  2011-04-13 DISIS (Ivica Ico Bukvic <ico@vt.edu>)
//  http://disis.music.vt.edu
//  * Fixed bug where only one IR source was visible/accessible
//	* Improved redundancy check for Rumble and LED actions

//  v0.7.0 Changelog:
//  2012-02-12 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//  * Improved build system
//  * Made wiimote limit dynamic
//	* Added ability to dynamically (re)connect motionplus after connecting wiimote
//	* Added support for balance board and classic (classic needs testing)
//  * Consolidated data output to one outlet and provided legacy L2Ork abstraction

//  v0.8.0 Changelog:
//  2012-03-04 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//	* Added dynamic polling to libcwiid for automatic detection of extensions
//	* External retains its state even after wiimote is disconnected so upon reconnecting
//	  it is not necessary to reenable various outputs

//  v0.9.0 Changelog:
//  2012-04-10 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//	* Added support for passthrough mode (MPlus&Nunchuk)
//	* Improved motionplus detection logic (implemented directly into cwiid lib)

//  v0.9.1 Changelog:
//  2012-04-11 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//	* Added support for detecting disconnection due to wiimote running out of battery (requires polling status in pd)

//  v1.0.0 Changelog:
//  2012-04-28 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//	* Added support for Plus+Classic

//  v1.0.1 Changelog:
//  2012-05-22 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//	* Added PDL2ORK define to distinguish features available only in pd-l2ork

//  v1.0.2 Changelog:
//  2012-10-27 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//	* Improved involuntary disconnection detection (e.g. due to low battery)
//	* Cleaned up pd-l2ork console messages
//	* Improved disconnect logic to allow re-enabling of settings upon reconnection

//  v1.0.3 Changelog:
//  2013-03-23 DISIS (Ivica Ico Bukvic <ico@vt.edu)
//  http://disis.music.vt.edu
//	* Made sure that the involuntary disconnection detection (e.g. due to low battery) does get reported by showing appropriate connection status from the rightmost outlet

#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <bluetooth/bluetooth.h>
#include <m_pd.h>
#include <math.h>
#include <pthread.h>
#include "cwiid.h"
#define PI	3.14159265358979323
#define WIIMOTE_BALANCE_CALWEIGHT 17.0f
#define HAVE_CWIID_MOTIONPLUS_LOWSPEED

struct acc {
	unsigned char x;
	unsigned char y;
	unsigned char z;
};

// class and struct declarations for wiimote pd external:
static t_class *pd_cwiid_class;
typedef struct _wiimote
{
	t_object x_obj; // standard pd object (must be first in struct)
	
	cwiid_wiimote_t *wiimote; // individual wiimote handle per pd object, represented in libcwiid

	t_float connected;
	int battery;
	int wiimoteID;
	int extensionAttached;

	//Creating separate threads for actions known to cause sample drop-outs
	pthread_t unsafe_t;
	pthread_mutex_t unsafe_mutex;
	pthread_cond_t unsafe_cond;

	t_float unsafe;

	t_float rumble;
	t_float led;
	t_float rpt;
	unsigned char rpt_mode;
	t_float passthrough;
	t_float passthrough_packet;

	t_symbol *addr;

	t_float toggle_acc, toggle_ir, toggle_exp;

	struct acc acc_zero, acc_one; // acceleration
	struct acc nc_acc_zero, nc_acc_one; // nunchuck acceleration
	struct balance_cal balance_cal; // balance board calibration

	// We store atom list for each data type so we don't waste time
	// allocating memory at every callback:
	t_atom btn_atoms[2];				// core stuff
	t_atom old_btn_atoms[2];
	t_atom acc_atoms[3];
	t_atom ir_atoms[4][4];
	t_atom nc_btn_atoms[1];				// nunchuk
	t_atom old_nc_btn_atoms[1];
	t_atom nc_acc_atoms[3];
	t_atom nc_stick_atoms[2];
	t_atom mp_ar_atoms[3];				// motionplus
#ifdef HAVE_CWIID_MOTIONPLUS_LOWSPEED
	t_atom mp_ls_atoms[3];
#endif
	t_atom cl_l_stick_atoms[2];			// classic
	t_atom cl_r_stick_atoms[2];
	t_atom old_cl_buttons_atoms[4];
	t_atom cl_buttons_atoms[4];			// l r button-a button-b
	t_atom balance_atoms[4];			// balance board rt,rb,lt,lb

	//for thread-unsafe operations
	t_clock *x_clock_status;
	t_clock *x_clock_battery;
	
	// outlets:
	t_outlet *outlet_data;
	t_outlet *outlet_status;
	
} t_wiimote;

typedef struct _wiimoteList {
	t_wiimote*x;
	int id;
 	struct _wiimoteList*next;
} t_wiimoteList;

t_wiimoteList *g_wiimoteList = NULL;

static int addWiimoteObject(t_wiimote*x, int id) {
  t_wiimoteList *wl = g_wiimoteList;
  t_wiimoteList *newentry = NULL;
  if(wl != NULL) {
    while(wl->next) {

      if(wl->x == x) {
        pd_error(x, "[disis_wiimote]: already bound to Wii%02d", wl->id);
        return 0;
      }
      if(wl->id == id) {
        pd_error(x, "[disis_wiimote]: another object is already bound to Wii%02d", wl->id);
        return 0;
      }
      wl=wl->next;
    }
  }

  newentry=(t_wiimoteList*)getbytes(sizeof(t_wiimoteList));
  newentry->next=NULL;
  newentry->x=x;
  newentry->id=id;

  if(wl)
    wl->next=newentry;
  else
    g_wiimoteList=newentry;

  return 1;
}

static t_wiimote *getWiimoteObject(const int id) {
  t_wiimoteList *wl = g_wiimoteList;
  if(wl == NULL)
    return NULL;

  while(wl) {
    if(id == wl->id) {
      return wl->x;
    }
    wl=wl->next;
  }
  return NULL;
}

static void removeWiimoteObject(const t_wiimote*x) {
  t_wiimoteList *wl = g_wiimoteList;
  t_wiimoteList *last = NULL;
  if(wl == NULL)
    return;

  while(wl) {
    if(x == wl->x) {
      if(last) {
        last->next=wl->next;
      } else {
        g_wiimoteList=wl->next;
      }
      wl->x=NULL;
      wl->id=0;
      wl->next=NULL;
      freebytes(wl, sizeof(t_wiimoteList));

      return;
    }
    last=wl;
    wl=wl->next;
  }
}

// Structure to pass generic parameters into a threaded function.
// Added by VT DISIS
typedef struct
{
	t_wiimote *wiimote;
} threadedFunctionParams;


static void pd_cwiid_tick_status(t_wiimote *x)
{
    outlet_float(x->outlet_status, x->connected);
}

static void pd_cwiid_tick_battery(t_wiimote *x)
{
	t_atom ap[1];
	SETFLOAT(ap+0, (t_float)x->battery);
    outlet_anything(x->outlet_data, gensym("battery"), 1, ap);
}

static void pd_cwiid_debug(t_wiimote *x)
{
	post("\n======================");
	if (x->connected) post("Wiimote (id: %d) is connected.", x->wiimoteID);
	else post("Wiimote (id: %d) is NOT connected.", x->wiimoteID);
	if (x->toggle_acc) post("Acceleration: ON");
	else 			   post("Acceleration: OFF");
	if (x->toggle_ir)  post("IR:           ON");
	else 			   post("IR:           OFF");
	if (x->toggle_exp)  post("Extension:    ON");
	else 			   post("Extension:    OFF");
	if (x->passthrough)  post("Passthrough:  ON");
	else 			   post("Passthrough:  OFF");
	//post("");
	//post("Accelerometer calibration: zero=(%d,%d,%d) one=(%d,%d,%d)",x->acc_zero.x,x->acc_zero.y,x->acc_zero.z,x->acc_one.x,x->acc_one.y,x->acc_one.z);
	//post("Nunchuck calibration:      zero=(%d,%d,%d) one=(%d,%d,%d)",x->nc_acc_zero.x,x->nc_acc_zero.y,x->nc_acc_zero.z,x->nc_acc_one.x,x->nc_acc_one.y,x->nc_acc_one.z);
}

// Button handler:
static void pd_cwiid_btn(t_wiimote *x, struct cwiid_btn_mesg *mesg)
{
	//post("Buttons: %X %X", (mesg->buttons & 0xFF00)>>8, mesg->buttons & 0x00FF);
	SETFLOAT(x->btn_atoms+0, (mesg->buttons & 0xFF00)>>8);
	SETFLOAT(x->btn_atoms+1, mesg->buttons & 0x00FF);
}

// Records acceleration into wiimote object. 
// To retrieve the information in pd, send a bang to input or change output mode to 1
static void pd_cwiid_acc(t_wiimote *x, struct cwiid_acc_mesg *mesg)
{
	if (x->toggle_acc)
	{
		double a_x, a_y, a_z;
		
		a_x = ((double)mesg->acc[CWIID_X] - x->acc_zero.x) / (x->acc_one.x - x->acc_zero.x);
		a_y = ((double)mesg->acc[CWIID_Y] - x->acc_zero.y) / (x->acc_one.y - x->acc_zero.y);
		a_z = ((double)mesg->acc[CWIID_Z] - x->acc_zero.z) / (x->acc_one.z - x->acc_zero.z);
		
		SETFLOAT(x->acc_atoms+0, a_x);
		SETFLOAT(x->acc_atoms+1, a_y);
		SETFLOAT(x->acc_atoms+2, a_z);
	}
}

static void pd_cwiid_ir(t_wiimote *x, struct cwiid_ir_mesg *mesg)
{
	unsigned int i;

	if (x->toggle_ir)
	{
		//post("IR (valid,x,y,size) #%d: %d %d %d %d", i, data->ir_data.ir_src[i].valid, data->ir_data.ir_src[i].x, data->ir_data.ir_src[i].y, data->ir_data.ir_src[i].size);
		for (i=0; i<CWIID_IR_SRC_COUNT; i++)
		{	
			if (mesg->src[i].valid)
			{
				SETFLOAT(x->ir_atoms[i]+0, i);
				SETFLOAT(x->ir_atoms[i]+1, mesg->src[i].pos[CWIID_X]);
				SETFLOAT(x->ir_atoms[i]+2, mesg->src[i].pos[CWIID_Y]);
				SETFLOAT(x->ir_atoms[i]+3, mesg->src[i].size);
			}
			else {
				SETFLOAT(x->ir_atoms[i]+0, -1);
				SETFLOAT(x->ir_atoms[i]+1, 0);
				SETFLOAT(x->ir_atoms[i]+2, 0);
				SETFLOAT(x->ir_atoms[i]+3, 0);
			}
		}
	}
}

static void pd_cwiid_nunchuk(t_wiimote *x, struct cwiid_nunchuk_mesg *mesg)
{
	double a_x, a_y, a_z;

	if (x->extensionAttached == 5)
		x->passthrough_packet -= 1;

	a_x = ((double)mesg->acc[CWIID_X] - x->nc_acc_zero.x) / (x->nc_acc_one.x - x->nc_acc_zero.x);
	a_y = ((double)mesg->acc[CWIID_Y] - x->nc_acc_zero.y) / (x->nc_acc_one.y - x->nc_acc_zero.y);
	a_z = ((double)mesg->acc[CWIID_Z] - x->nc_acc_zero.z) / (x->nc_acc_one.z - x->nc_acc_zero.z);
	
	if (atom_getint(x->nc_btn_atoms) != mesg->buttons) {
		SETFLOAT(x->nc_btn_atoms+0, mesg->buttons);
	}
	
	SETFLOAT(x->nc_acc_atoms+0, a_x);
	SETFLOAT(x->nc_acc_atoms+1, a_y);
	SETFLOAT(x->nc_acc_atoms+2, a_z);	

	SETFLOAT(x->nc_stick_atoms+0, mesg->stick[CWIID_X]);
	SETFLOAT(x->nc_stick_atoms+1, mesg->stick[CWIID_Y]);		
}

static void pd_cwiid_classic(t_wiimote *x, struct cwiid_classic_mesg *mesg)
{
	if (x->extensionAttached == 6)
		x->passthrough_packet -= 1;

   SETFLOAT (x->cl_l_stick_atoms+0, mesg->l_stick[CWIID_X]);
   SETFLOAT (x->cl_l_stick_atoms+1, mesg->l_stick[CWIID_Y]);
   SETFLOAT (x->cl_r_stick_atoms+0, mesg->r_stick[CWIID_X]);
   SETFLOAT (x->cl_r_stick_atoms+1, mesg->r_stick[CWIID_Y]);
   SETFLOAT (x->cl_buttons_atoms+0, mesg->l);
   SETFLOAT (x->cl_buttons_atoms+1, mesg->r);
   SETFLOAT (x->cl_buttons_atoms+2, (mesg->buttons & 0xFF00)>>8);
   SETFLOAT (x->cl_buttons_atoms+3, mesg->buttons & 0x00FF);
}

static t_float pd_cwiid_balance_calc(t_wiimote *x, uint16_t value, uint16_t calibration[3])
{
	/*
	17.0 appears to be the step the calibrations are against.
	17kg per sensor is 68kg, 1/2 of the advertised Japanese weight limit.
	*/
	t_float weight=0;
	t_float falue=value;
	if(calibration) {
		if(value<calibration[1]) {
			weight = WIIMOTE_BALANCE_CALWEIGHT * (falue - calibration[0]) / (calibration[1]-calibration[0]);
		} else {
			weight = WIIMOTE_BALANCE_CALWEIGHT * (1 + (falue - calibration[1]) / (calibration[2]-calibration[1]));
		}
	} else {
		weight=falue;
	}
	if (weight < 0.0) weight = 0.0;
	return weight;
}

static void pd_cwiid_balance(t_wiimote *x, struct cwiid_balance_mesg *mesg)
{
	// x->balance_atoms array sstructure: rt, rb, lt, lb
	SETFLOAT(x->balance_atoms+0, pd_cwiid_balance_calc(x, mesg->right_top, x->balance_cal.right_top));
	SETFLOAT(x->balance_atoms+1, pd_cwiid_balance_calc(x, mesg->right_bottom, x->balance_cal.right_bottom));
	SETFLOAT(x->balance_atoms+2, pd_cwiid_balance_calc(x, mesg->left_top, x->balance_cal.left_top));
	SETFLOAT(x->balance_atoms+3, pd_cwiid_balance_calc(x, mesg->left_bottom, x->balance_cal.left_bottom));
}

static void pd_cwiid_motionplus(t_wiimote *x, struct cwiid_motionplus_mesg *mesg)
{
	if (x->extensionAttached == 5)
		x->passthrough_packet += 1;

	SETFLOAT(x->mp_ar_atoms+0, (double)mesg->angle_rate[CWIID_PHI]);
	SETFLOAT(x->mp_ar_atoms+1, (double)mesg->angle_rate[CWIID_THETA]);
	SETFLOAT(x->mp_ar_atoms+2, (double)mesg->angle_rate[CWIID_PSI]);

#ifdef HAVE_CWIID_MOTIONPLUS_LOWSPEED
	SETFLOAT(x->mp_ls_atoms+0, (double)mesg->low_speed[CWIID_PHI]);
	SETFLOAT(x->mp_ls_atoms+1, (double)mesg->low_speed[CWIID_THETA]);
	SETFLOAT(x->mp_ls_atoms+2, (double)mesg->low_speed[CWIID_PSI]);
#endif
}

static void pd_cwiid_doBang(t_wiimote *x)
{
	int i;
	t_atom ap[4];

	if (x->toggle_exp == 1 && x->extensionAttached > 0) {

		switch (x->extensionAttached) {
			case 1:
				// nunchuk
				SETSYMBOL(ap+0, gensym("stick"));
				SETFLOAT (ap+1, atom_getfloat(x->nc_stick_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->nc_stick_atoms+1));
				outlet_anything(x->outlet_data, gensym("nunchuk"), 3, ap);

				SETSYMBOL(ap+0, gensym("acceleration"));
				SETFLOAT (ap+1, atom_getfloat(x->nc_acc_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->nc_acc_atoms+1));
				SETFLOAT (ap+3, atom_getfloat(x->nc_acc_atoms+2));
				outlet_anything(x->outlet_data, gensym("nunchuk"), 4, ap);

				if (atom_getfloat(x->old_nc_btn_atoms) != atom_getfloat(x->nc_btn_atoms)) {
					SETSYMBOL(ap+0, gensym("button"));
					SETFLOAT (ap+1, atom_getfloat(x->nc_btn_atoms+0));
					outlet_anything(x->outlet_data, gensym("nunchuk"), 2, ap);
					SETFLOAT(x->old_nc_btn_atoms+0, atom_getfloat(x->nc_btn_atoms));
				}
				break;
			case 2:
				// classic
				SETSYMBOL(ap+0, gensym("left_stick"));
				SETFLOAT (ap+1, atom_getfloat(x->cl_l_stick_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->cl_l_stick_atoms+1));
				outlet_anything(x->outlet_data, gensym("classic"), 3, ap);

				SETSYMBOL(ap+0, gensym("right_stick"));
				SETFLOAT (ap+1, atom_getfloat(x->cl_r_stick_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->cl_r_stick_atoms+1));
				outlet_anything(x->outlet_data, gensym("classic"), 3, ap);

				if (atom_getfloat(x->old_cl_buttons_atoms+0) != atom_getfloat(x->cl_buttons_atoms+0))
				{
					SETSYMBOL(ap+0, gensym("left"));
					SETFLOAT (ap+1, atom_getfloat(x->cl_buttons_atoms+0));
					outlet_anything(x->outlet_data, gensym("classic"), 2, ap);
					SETFLOAT(x->old_cl_buttons_atoms+0, atom_getfloat(x->cl_buttons_atoms+0));
				}

				if (atom_getfloat(x->old_cl_buttons_atoms+1) != atom_getfloat(x->cl_buttons_atoms+1))
				{
					SETSYMBOL(ap+0, gensym("right"));
					SETFLOAT (ap+1, atom_getfloat(x->cl_buttons_atoms+1));
					outlet_anything(x->outlet_data, gensym("classic"), 2, ap);
					SETFLOAT(x->old_cl_buttons_atoms+1, atom_getfloat(x->cl_buttons_atoms+1));
				}

				if (atom_getfloat(x->old_cl_buttons_atoms+2) != atom_getfloat(x->cl_buttons_atoms+2) ||
					atom_getfloat(x->old_cl_buttons_atoms+3) != atom_getfloat(x->cl_buttons_atoms+3)) 
				{
					SETSYMBOL(ap+0, gensym("button"));
					SETFLOAT (ap+1, atom_getfloat(x->cl_buttons_atoms+2));
					SETFLOAT (ap+2, atom_getfloat(x->cl_buttons_atoms+3));
					outlet_anything(x->outlet_data, gensym("classic"), 3, ap);
					SETFLOAT(x->old_cl_buttons_atoms+2, atom_getfloat(x->cl_buttons_atoms+2));
					SETFLOAT(x->old_cl_buttons_atoms+3, atom_getfloat(x->cl_buttons_atoms+3));
				}
				break;
			case 3:
				// balance board
				SETSYMBOL(ap+0, gensym("right_top"));
				SETFLOAT (ap+1, atom_getfloat(x->balance_atoms+0));
				outlet_anything(x->outlet_data, gensym("balance"), 2, ap);

				SETSYMBOL(ap+0, gensym("right_bottom"));
				SETFLOAT (ap+1, atom_getfloat(x->balance_atoms+1));
				outlet_anything(x->outlet_data, gensym("balance"), 2, ap);

				SETSYMBOL(ap+0, gensym("left_top"));
				SETFLOAT (ap+1, atom_getfloat(x->balance_atoms+2));
				outlet_anything(x->outlet_data, gensym("balance"), 2, ap);

				SETSYMBOL(ap+0, gensym("left_bottom"));
				SETFLOAT (ap+1, atom_getfloat(x->balance_atoms+3));
				outlet_anything(x->outlet_data, gensym("balance"), 2, ap);
				break;
			case 4:
				// motionplus
#ifdef HAVE_CWIID_MOTIONPLUS_LOWSPEED
				SETSYMBOL(ap+0, gensym("low_speed"));
				SETFLOAT (ap+1, atom_getfloat(x->mp_ls_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->mp_ls_atoms+1));
				SETFLOAT (ap+3, atom_getfloat(x->mp_ls_atoms+2));
				outlet_anything(x->outlet_data, gensym("motionplus"), 4, ap);
#endif
				SETSYMBOL(ap+0, gensym("angle_rate"));
				SETFLOAT (ap+1, atom_getfloat(x->mp_ar_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->mp_ar_atoms+1));
				SETFLOAT (ap+3, atom_getfloat(x->mp_ar_atoms+2));
				outlet_anything(x->outlet_data, gensym("motionplus"), 4, ap);
				break;
			case 5: //motionplus + nunchuk
				// motionplus
#ifdef HAVE_CWIID_MOTIONPLUS_LOWSPEED
				SETSYMBOL(ap+0, gensym("low_speed"));
				SETFLOAT (ap+1, atom_getfloat(x->mp_ls_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->mp_ls_atoms+1));
				SETFLOAT (ap+3, atom_getfloat(x->mp_ls_atoms+2));
				outlet_anything(x->outlet_data, gensym("motionplus"), 4, ap);
#endif
				SETSYMBOL(ap+0, gensym("angle_rate"));
				SETFLOAT (ap+1, atom_getfloat(x->mp_ar_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->mp_ar_atoms+1));
				SETFLOAT (ap+3, atom_getfloat(x->mp_ar_atoms+2));
				outlet_anything(x->outlet_data, gensym("motionplus"), 4, ap);

				// nunchuk
				SETSYMBOL(ap+0, gensym("stick"));
				SETFLOAT (ap+1, atom_getfloat(x->nc_stick_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->nc_stick_atoms+1));
				outlet_anything(x->outlet_data, gensym("nunchuk"), 3, ap);

				SETSYMBOL(ap+0, gensym("acceleration"));
				SETFLOAT (ap+1, atom_getfloat(x->nc_acc_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->nc_acc_atoms+1));
				SETFLOAT (ap+3, atom_getfloat(x->nc_acc_atoms+2));
				outlet_anything(x->outlet_data, gensym("nunchuk"), 4, ap);

				if (atom_getfloat(x->old_nc_btn_atoms) != atom_getfloat(x->nc_btn_atoms)) {
					SETSYMBOL(ap+0, gensym("button"));
					SETFLOAT (ap+1, atom_getfloat(x->nc_btn_atoms+0));
					outlet_anything(x->outlet_data, gensym("nunchuk"), 2, ap);
					SETFLOAT(x->old_nc_btn_atoms+0, atom_getfloat(x->nc_btn_atoms));
				}
				break;
			case 6:
				// classic
				SETSYMBOL(ap+0, gensym("left_stick"));
				SETFLOAT (ap+1, atom_getfloat(x->cl_l_stick_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->cl_l_stick_atoms+1));
				outlet_anything(x->outlet_data, gensym("classic"), 3, ap);

				SETSYMBOL(ap+0, gensym("right_stick"));
				SETFLOAT (ap+1, atom_getfloat(x->cl_r_stick_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->cl_r_stick_atoms+1));
				outlet_anything(x->outlet_data, gensym("classic"), 3, ap);

				if (atom_getfloat(x->old_cl_buttons_atoms+0) != atom_getfloat(x->cl_buttons_atoms+0))
				{
					SETSYMBOL(ap+0, gensym("left"));
					SETFLOAT (ap+1, atom_getfloat(x->cl_buttons_atoms+0));
					outlet_anything(x->outlet_data, gensym("classic"), 2, ap);
					SETFLOAT(x->old_cl_buttons_atoms+0, atom_getfloat(x->cl_buttons_atoms+0));
				}

				if (atom_getfloat(x->old_cl_buttons_atoms+1) != atom_getfloat(x->cl_buttons_atoms+1))
				{
					SETSYMBOL(ap+0, gensym("right"));
					SETFLOAT (ap+1, atom_getfloat(x->cl_buttons_atoms+1));
					outlet_anything(x->outlet_data, gensym("classic"), 2, ap);
					SETFLOAT(x->old_cl_buttons_atoms+1, atom_getfloat(x->cl_buttons_atoms+1));
				}

				if (atom_getfloat(x->old_cl_buttons_atoms+2) != atom_getfloat(x->cl_buttons_atoms+2) ||
					atom_getfloat(x->old_cl_buttons_atoms+3) != atom_getfloat(x->cl_buttons_atoms+3)) 
				{
					SETSYMBOL(ap+0, gensym("button"));
					SETFLOAT (ap+1, atom_getfloat(x->cl_buttons_atoms+2));
					SETFLOAT (ap+2, atom_getfloat(x->cl_buttons_atoms+3));
					outlet_anything(x->outlet_data, gensym("classic"), 3, ap);
					SETFLOAT(x->old_cl_buttons_atoms+2, atom_getfloat(x->cl_buttons_atoms+2));
					SETFLOAT(x->old_cl_buttons_atoms+3, atom_getfloat(x->cl_buttons_atoms+3));
				}

				// nunchuk
				SETSYMBOL(ap+0, gensym("stick"));
				SETFLOAT (ap+1, atom_getfloat(x->nc_stick_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->nc_stick_atoms+1));
				outlet_anything(x->outlet_data, gensym("nunchuk"), 3, ap);

				SETSYMBOL(ap+0, gensym("acceleration"));
				SETFLOAT (ap+1, atom_getfloat(x->nc_acc_atoms+0));
				SETFLOAT (ap+2, atom_getfloat(x->nc_acc_atoms+1));
				SETFLOAT (ap+3, atom_getfloat(x->nc_acc_atoms+2));
				outlet_anything(x->outlet_data, gensym("nunchuk"), 4, ap);

				if (atom_getfloat(x->old_nc_btn_atoms) != atom_getfloat(x->nc_btn_atoms)) {
					SETSYMBOL(ap+0, gensym("button"));
					SETFLOAT (ap+1, atom_getfloat(x->nc_btn_atoms+0));
					outlet_anything(x->outlet_data, gensym("nunchuk"), 2, ap);
					SETFLOAT(x->old_nc_btn_atoms+0, atom_getfloat(x->nc_btn_atoms));
				}
				break;
			default:
				break;
		}
	}
	if (x->toggle_ir == 1) {
		for (i=0; i<CWIID_IR_SRC_COUNT; i++) {
			if (atom_getfloat(x->ir_atoms[i]+0) != -1.0) {
				SETFLOAT (ap+0, atom_getfloat(x->ir_atoms[i]+0));
				SETFLOAT (ap+1, atom_getfloat(x->ir_atoms[i]+1));
				SETFLOAT (ap+2, atom_getfloat(x->ir_atoms[i]+2));
				SETFLOAT (ap+3, atom_getfloat(x->ir_atoms[i]+3));
				outlet_anything(x->outlet_data, gensym("ir"), 4, ap);
			}
		}
	}
	if (x->toggle_acc == 1) {
		SETFLOAT (ap+0, atom_getfloat(x->acc_atoms+0));
		SETFLOAT (ap+1, atom_getfloat(x->acc_atoms+1));
		SETFLOAT (ap+2, atom_getfloat(x->acc_atoms+2));
		outlet_anything(x->outlet_data, gensym("acceleration"), 3, ap);
	}
	if (x->connected) {
		if (atom_getfloat(x->old_btn_atoms+0) != atom_getfloat(x->btn_atoms+0) ||
			atom_getfloat(x->old_btn_atoms+1) != atom_getfloat(x->btn_atoms+1)) {
			SETFLOAT (ap+0, atom_getfloat(x->btn_atoms+0));
			SETFLOAT (ap+1, atom_getfloat(x->btn_atoms+1));
			outlet_anything(x->outlet_data, gensym("button"), 2, ap);
			SETFLOAT(x->old_btn_atoms+0, atom_getfloat(x->btn_atoms+0));
			SETFLOAT(x->old_btn_atoms+1, atom_getfloat(x->btn_atoms+1));
		}
	}
}

static void pd_cwiid_doDisconnect(t_wiimote *x);

static void pd_cwiid_status(t_wiimote *x)
{
	if (x->connected) {
		/* try to get status--if this returns an error, it means
		   the wiimote has probably disconnected/run out of batteries */
		if (!cwiid_request_status(x->wiimote))
			clock_delay(x->x_clock_battery, 0);
		else
			pd_cwiid_doDisconnect(x);
	}
}

static void pd_cwiid_setRumble(t_wiimote *x, t_floatarg f)
{
	if (x->connected && x->rumble != f) {
		x->rumble = f;

		pthread_mutex_lock(&x->unsafe_mutex);
		pthread_cond_signal(&x->unsafe_cond);
		pthread_mutex_unlock(&x->unsafe_mutex);
	}
}

// The CWiid library invokes a callback function whenever events are
// generated by the wiimote. This function is specified when connecting
// to the wiimote (in the cwiid_open function).

// Unfortunately, the mesg struct passed as an argument to the
// callback does not have a pointer to the wiimote instance, and it
// is thus impossible to know which wiimote has invoked the callback.
// For this case we provide a hard-coded set of wrapper callbacks to
// indicate which Pd wiimote instance to control.

static void pd_cwiid_setReportMode(t_wiimote *x, t_floatarg r);

static void pd_cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
                    union cwiid_mesg mesg_array[], struct timespec *timestamp)
{
	int i;
	t_wiimote *x;
	static unsigned char buf[CWIID_MAX_READ_LEN];
	if (g_wiimoteList == NULL || wiimote == NULL) {
		post("no wiimotes connected");
	}
	else {
		x = getWiimoteObject(cwiid_get_id(wiimote));

		if(x == NULL) {
			post("no wiimote loaded: %d%",cwiid_get_id(wiimote));
			return;
		}	
		
		for (i=0; i < mesg_count; i++)
		{	
			switch (mesg_array[i].type) {
				case CWIID_MESG_STATUS:
					//post("Battery: %d%", (int) (100.0 * mesg_array[i].status_mesg.battery / CWIID_BATTERY_MAX));
					x->battery = (int) (100.0 * mesg_array[i].status_mesg.battery / CWIID_BATTERY_MAX);
					switch (mesg_array[i].status_mesg.ext_type) {
						case CWIID_EXT_NONE:
							if (x->extensionAttached != 0)
								post("No extension attached");
							x->extensionAttached = 0;
							break;
						case CWIID_EXT_NUNCHUK:
							if (x->extensionAttached != 1)
								post("Nunchuck extension attached");
							x->extensionAttached = 1;

							if (cwiid_read(x->wiimote, CWIID_RW_REG | CWIID_RW_DECODE, 0xA40020, 7, buf)) {
								post("Unable to retrieve Nunchuk calibration");
							}
							else {
								x->nc_acc_zero.x = buf[0];
								x->nc_acc_zero.y = buf[1];
								x->nc_acc_zero.z = buf[2];
								x->nc_acc_one.x  = buf[4];
								x->nc_acc_one.y  = buf[5];
								x->nc_acc_one.z  = buf[6];
							}
							if (x->toggle_exp)
								pd_cwiid_setReportMode(x, -1);

							break;
						case CWIID_EXT_CLASSIC:
							if (x->extensionAttached != 2)
								post("Classic controller attached");
							x->extensionAttached = 2;
							if (x->toggle_exp)
								pd_cwiid_setReportMode(x, -1);
							break;
						case CWIID_EXT_BALANCE:
							if (x->extensionAttached != 3)
								post("Balance board attached");
							x->extensionAttached = 3;
							if(cwiid_get_balance_cal(x->wiimote, &x->balance_cal))
								post("Unable to retrieve balance calibration");
							if (x->toggle_exp)
								pd_cwiid_setReportMode(x, -1);
							break;
						case CWIID_EXT_MOTIONPLUS:
							if (x->extensionAttached != 4)
								post("MotionPlus extension attached");
							x->extensionAttached = 4;
							if (x->toggle_exp)
								pd_cwiid_setReportMode(x, -1);
							break;
						case CWIID_EXT_NUNCHUK_MPLUS_PASSTHROUGH:
							if (x->extensionAttached != 5) {
								post("MotionPlus & Nunchuk extensions attached (passthrough)");
								x->extensionAttached = 5;
								/* nunchuk calibration data migrates to A40040 when the passthrough mode is enabled */
								if (cwiid_read(x->wiimote, CWIID_RW_REG, 0xA40040, 7, buf)) {
									post("Unable to retrieve Nunchuk calibration");
								}
								else {
									x->nc_acc_zero.x = buf[0];
									x->nc_acc_zero.y = buf[1];
									x->nc_acc_zero.z = buf[2];
									x->nc_acc_one.x  = buf[4];
									x->nc_acc_one.y  = buf[5];
									x->nc_acc_one.z  = buf[6];
								}
							}
							break;
						case CWIID_EXT_CLASSIC_MPLUS_PASSTHROUGH:
							if (x->extensionAttached != 6) {
								post("MotionPlus & Classic extensions attached (passthrough)");
								x->extensionAttached = 6;
							}
							break;
						case CWIID_EXT_UNKNOWN:
							post("Unknown extension attached");
							if (x->toggle_exp)
								pd_cwiid_setReportMode(x, -1);
							break;						
					}
					break;
				case CWIID_MESG_BTN:
					pd_cwiid_btn(x, &mesg_array[i].btn_mesg);
					break;
				case CWIID_MESG_ACC:
					pd_cwiid_acc(x, &mesg_array[i].acc_mesg);
					break;
				case CWIID_MESG_IR:
					pd_cwiid_ir(x, &mesg_array[i].ir_mesg);
					break;
				case CWIID_MESG_NUNCHUK:
					/* first check if we are in passthrough mode and if so if we've had nunchuk
					   disconnected at the time when the passthrough was enabled as this would've
					   prevented us from grabbing the calibration data from the nunchuk */
					if (x->passthrough_packet > 10 && x->extensionAttached == 5) {
						x->passthrough_packet = 0;
						x->extensionAttached = 1; //any bogus data will do to force probing nc calibration data
						cwiid_request_status(x->wiimote);
					} else {
						pd_cwiid_nunchuk(x, &mesg_array[i].nunchuk_mesg);
					}
					break;
				case CWIID_MESG_CLASSIC:
					pd_cwiid_classic(x, &mesg_array[i].classic_mesg);
					break;
				case CWIID_MESG_BALANCE:
					pd_cwiid_balance(x, &mesg_array[i].balance_mesg);
					break;
				case CWIID_MESG_MOTIONPLUS:
					pd_cwiid_motionplus(x, &mesg_array[i].motionplus_mesg);
					break;
				case CWIID_MESG_ERROR:
					post("Erroneous data packet received, likely due to lost connection");
					pd_cwiid_status(x);
					break;
				default:
					break;
			}
		}
	}
}

// ==============================================================

static void pd_cwiid_setReportMode(t_wiimote *x, t_floatarg r)
{
	if (x->connected) {
		if (r >= 0) x->rpt_mode = (unsigned char) r;
		else {
			x->rpt_mode = CWIID_RPT_STATUS | CWIID_RPT_BTN;
			if (x->toggle_ir) x->rpt_mode |= CWIID_RPT_IR;
			if (x->toggle_acc) x->rpt_mode |= CWIID_RPT_ACC;
			if (x->toggle_exp) {
				switch (x->extensionAttached) {
					case 1:
						x->rpt_mode |= CWIID_RPT_NUNCHUK;
						break;
					case 2:
						x->rpt_mode |= CWIID_RPT_CLASSIC;
						break;
					case 3:
						x->rpt_mode |= CWIID_RPT_BALANCE;
						break;
					case 4:
						x->rpt_mode |= CWIID_RPT_MOTIONPLUS;
						break;
					case 5:
						x->rpt_mode |= CWIID_RPT_NUNCHUK;
						x->rpt_mode |= CWIID_RPT_MOTIONPLUS;
					case 6:
						x->rpt_mode |= CWIID_RPT_CLASSIC;
						x->rpt_mode |= CWIID_RPT_MOTIONPLUS;
					default:
						break;
				}
			}
		}

		pthread_mutex_lock(&x->unsafe_mutex);
		pthread_cond_signal(&x->unsafe_cond);
		pthread_mutex_unlock(&x->unsafe_mutex);
	}
}

static void pd_cwiid_reportAcceleration(t_wiimote *x, t_floatarg f)
{
	x->toggle_acc = f;
	pd_cwiid_setReportMode(x, -1);
}

static void pd_cwiid_reportIR(t_wiimote *x, t_floatarg f)
{
	x->toggle_ir = f;
	pd_cwiid_setReportMode(x, -1);
}

static void pd_cwiid_reportExpansion(t_wiimote *x, t_floatarg f)
{
	x->toggle_exp = f;
	pd_cwiid_setReportMode(x, -1);
}

static void pd_cwiid_togglePassthrough(t_wiimote *x, t_floatarg f)
{
	x->passthrough = f;
	pd_cwiid_setReportMode(x, -1);
}

static void *pd_cwiid_pthreadForAudioUnfriendlyOperations(void *ptr)
{
	threadedFunctionParams *rPars = (threadedFunctionParams*)ptr;
	t_wiimote *x = rPars->wiimote;
	t_float local_led = 0;
	t_float local_rumble = 0;
	t_float local_passthrough = 0;
	unsigned char local_rpt_mode = x->rpt_mode;

	while(x->unsafe > -1) {
		pthread_mutex_lock(&x->unsafe_mutex);
		if ((local_led == x->led) && (local_rumble == x->rumble) && (local_rpt_mode == x->rpt_mode)) {
			if (x->unsafe == 1) x->unsafe = 0; //signal that the thread init is complete
			pthread_cond_wait(&x->unsafe_cond, &x->unsafe_mutex);
		}

		if (local_led != x->led) {
			local_led = x->led;
			// some possible values:
			// CWIID_LED0_ON		0x01
			// CWIID_LED1_ON		0x02
			// CWIID_LED2_ON		0x04
			// CWIID_LED3_ON		0x08
			if (cwiid_command(x->wiimote, CWIID_CMD_LED, local_led)) {
				//post("wiiremote error: problem setting LED.");
			}
		}
		if (local_rumble != x->rumble) {
			local_rumble = x->rumble;
			if (cwiid_command(x->wiimote, CWIID_CMD_RUMBLE, local_rumble)) {
				//post("wiiremote error: problem setting rumble.");
			}
		}
		if (local_rpt_mode != x->rpt_mode) {
			local_rpt_mode = x->rpt_mode;
			local_passthrough = x->passthrough;

			if (cwiid_set_rpt_mode(x->wiimote, local_rpt_mode)) {
				//post("wiimote error: problem setting report mode.");
			}
			cwiid_toggle_passthrough_mode(x->wiimote, local_passthrough);
		}
		if (local_passthrough != x->passthrough) {
			local_passthrough = x->passthrough;
			cwiid_toggle_passthrough_mode(x->wiimote, local_passthrough);
		}
		pthread_mutex_unlock(&x->unsafe_mutex);
	}
	pthread_exit(0);
}

static void pd_cwiid_setLED(t_wiimote *x, t_floatarg f)
{
	if (x->connected && x->led != f) {
		x->led = f;

		pthread_mutex_lock(&x->unsafe_mutex);
		pthread_cond_signal(&x->unsafe_cond);
		pthread_mutex_unlock(&x->unsafe_mutex);
	}
}

// The following function attempts to connect to a wiimote at a
// specific address, provided as an argument. eg, 00:19:1D:70:CE:72
// This address can be discovered by running the following command
// in a console:
//   hcitool scan | grep Nintendo
#ifdef PDL2ORK
extern int sys_flushtogui(void);
#endif

static void pd_cwiid_doConnect(t_wiimote *x, t_symbol *dongaddr)
{
	if (!x->connected) {

		//int i;
   		bdaddr_t bdaddr;
		unsigned int flags =  CWIID_FLAG_MESG_IFC;

		bdaddr_t dong_bdaddr;
#ifdef CWIID_OPEN_WITH_DONGLE
   		bdaddr_t* dong_bdaddr_ptr=&dong_bdaddr;
#endif

		static unsigned char buf[CWIID_MAX_READ_LEN];
		x->addr = dongaddr;

		// determine address:
		if (x->addr==gensym("NULL")) {
			post("Searching automatically...");		
			bdaddr = *BDADDR_ANY;
		}
		else {
			str2ba(x->addr->s_name, &bdaddr);
			post("Connecting to a wiimote with MAC address %s...", dongaddr->s_name);
		}

		// determine dongleaddress:
		if (NULL==dongaddr || dongaddr==gensym("")) {
			verbose(1, "using default dongle");
#ifdef CWIID_OPEN_WITH_DONGLE
			dong_bdaddr_ptr = NULL;
#endif
		}   else {
			verbose(1, "using dongle '%s'", dongaddr->s_name);
			str2ba(dongaddr->s_name, &dong_bdaddr);
		}

#ifdef PDL2ORK
		// send all console posts before connecting (including instructions on how to connect)
		sys_flushtogui();
#endif

		// connect:
#ifdef CWIID_OPEN_WITH_DONGLE
		verbose(1,"wiimote: opening multidongle");
		x->wiimote = cwiid_open(&bdaddr, dong_bdaddr_ptr, flags);
#else
		verbose(1,"wiimote: opening");
		x->wiimote = cwiid_open(&bdaddr, flags);
#endif

		if (x->wiimote == NULL) {
			post("Error: could not find and/or connect to a wiimote. Please ensure that bluetooth is enabled, and that the 'hcitool scan' command lists your Nintendo device.");
		} else {

			if(!addWiimoteObject(x, cwiid_get_id(x->wiimote))) {
				cwiid_close(x->wiimote);
				x->wiimote=NULL;
				clock_delay(x->x_clock_status, 0);
				return;
			}

			x->wiimoteID = cwiid_get_id(x->wiimote);

			post("wiimote %i has successfully connected", x->wiimoteID);

			if (cwiid_read(x->wiimote, CWIID_RW_EEPROM, 0x16, 7, buf)) {
				post("Unable to retrieve accelerometer calibration");
			} else {
				x->acc_zero.x = buf[0];
				x->acc_zero.y = buf[1];
				x->acc_zero.z = buf[2];
				x->acc_one.x  = buf[4];
				x->acc_one.y  = buf[5];
				x->acc_one.z  = buf[6];
				//post("Retrieved wiimote calibration: zero=(%.1f,%.1f,%.1f) one=(%.1f,%.1f,%.1f)",buf[0],buf[2],buf[3],buf[4],buf[5],buf[6]);
			}
			if (cwiid_set_mesg_callback(x->wiimote, &pd_cwiid_callback)) {
				post("Connection error: Unable to set message callback");
			}
			else {
				x->connected = 1;
				pd_cwiid_setReportMode(x,-1);
				//cwiid_enable(x->wiimote, CWIID_FLAG_MOTIONPLUS);
				cwiid_request_status(x->wiimote);

				clock_delay(x->x_clock_status, 0);
				clock_delay(x->x_clock_battery, 0);

				// send brief rumble to acknowledge connect
				// and give a bit of a wait before doing so
				usleep(500000);
				pd_cwiid_setRumble(x, 1);
				usleep(250000);
				pd_cwiid_setRumble(x, 0);
			}
		}
	}
}

// The following function attempts to discover a wiimote. It requires
// that the user puts the wiimote into 'discoverable' mode before being
// called. This is done by pressing the red button under the battery
// cover, or by pressing buttons 1 and 2 simultaneously.

static void pd_cwiid_discover(t_wiimote *x)
{
	if (!x->connected) {
		post("Put the wiimote into discover mode by pressing buttons 1 and 2 simultaneously");	
		pd_cwiid_doConnect(x, gensym("NULL"));
	}
	else {
		post("connect: device already connected!");
	}
}

static void pd_cwiid_doDisconnect(t_wiimote *x)
{
	int i;

	if (x->connected)
	{
		//cwiid_disable(x->wiimote, CWIID_FLAG_MOTIONPLUS);

		x->rpt_mode = 0;

		pthread_mutex_lock(&x->unsafe_mutex);
		pthread_cond_signal(&x->unsafe_cond);
		pthread_mutex_unlock(&x->unsafe_mutex);

		if (cwiid_close(x->wiimote)) {
			post("wiimote error: unable to close connection.");
		} 
		else {
			post("disconnect successful.");
			removeWiimoteObject(x);
			// reinitialize values:
			//g_wiimoteList[x->wiimoteID] = NULL;

			//x->toggle_acc = 0;
			//x->toggle_ir = 0;
			//x->toggle_exp = 0;

			x->battery = 0;

			SETFLOAT(x->acc_atoms+0, 0);
			SETFLOAT(x->acc_atoms+1, 0);
			SETFLOAT(x->acc_atoms+2, 0);
			for (i=0; i<CWIID_IR_SRC_COUNT; i++)
			{
				SETFLOAT(x->ir_atoms[i]+0, -1);
				SETFLOAT(x->ir_atoms[i]+1, 0);
				SETFLOAT(x->ir_atoms[i]+2, 0);
				SETFLOAT(x->ir_atoms[i]+3, 0);
			}
			SETFLOAT(x->nc_acc_atoms+0, 0);
			SETFLOAT(x->nc_acc_atoms+1, 0);
			SETFLOAT(x->nc_acc_atoms+2, 0);
			SETFLOAT(x->nc_stick_atoms+0, 0);
			SETFLOAT(x->nc_stick_atoms+1, 0);

			SETFLOAT(x->mp_ar_atoms+0, 0);
			SETFLOAT(x->mp_ar_atoms+1, 0);
			SETFLOAT(x->mp_ar_atoms+2, 0);

#ifdef HAVE_CWIID_MOTIONPLUS_LOWSPEED
			SETFLOAT(x->mp_ls_atoms+0, 0);
			SETFLOAT(x->mp_ls_atoms+1, 0);
			SETFLOAT(x->mp_ls_atoms+2, 0);
#endif

			SETFLOAT(x->cl_l_stick_atoms+0, 0);
			SETFLOAT(x->cl_l_stick_atoms+1, 0);

			SETFLOAT(x->cl_r_stick_atoms+0, 0);
			SETFLOAT(x->cl_r_stick_atoms+1, 0);

			SETFLOAT(x->old_cl_buttons_atoms+0, 0);
			SETFLOAT(x->old_cl_buttons_atoms+1, 0);
			SETFLOAT(x->old_cl_buttons_atoms+2, 0);
			SETFLOAT(x->old_cl_buttons_atoms+3, 0);

			SETFLOAT(x->cl_buttons_atoms+0, 0);
			SETFLOAT(x->cl_buttons_atoms+1, 0);
			SETFLOAT(x->cl_buttons_atoms+2, 0);
			SETFLOAT(x->cl_buttons_atoms+3, 0);

			SETFLOAT(x->balance_atoms+0, 0);
			SETFLOAT(x->balance_atoms+1, 0);
			SETFLOAT(x->balance_atoms+2, 0);
			SETFLOAT(x->balance_atoms+3, 0);

			x->connected = 0;
			x->wiimoteID = -1;
			x->extensionAttached = -1;
			x->passthrough_packet = 0;

			x->addr = gensym("NULL");
			x->wiimote = NULL;
//			x->rpt = 0;
//			x->unsafe = 0;
//			x->rumble = 0;
//			x->led = 0;
//			x->rpt_mode = -1;

			//signal disconnect on the outlet
			clock_delay(x->x_clock_status, 0);
		}
	}
	else post("disconnect: device is not connected!");
}

// ==============================================================
// ==============================================================

static void *pd_cwiid_new(t_symbol* s, int argc, t_atom *argv)
{
	int i;

	post("DISIS threaded implementation of wiimote object v.1.0.3");
	t_wiimote *x = (t_wiimote *)pd_new(pd_cwiid_class);
	
	// create outlets:
	x->outlet_data = outlet_new(&x->x_obj, &s_list);

	// status outlet:
	x->outlet_status = outlet_new(&x->x_obj, &s_float);

	// initialize toggles:
	x->toggle_acc = 0;
	x->toggle_ir = 0;
	x->toggle_exp = 0;

	x->battery = 0;

	// initialize values:
	SETFLOAT(x->acc_atoms+0, 0);
	SETFLOAT(x->acc_atoms+1, 0);
	SETFLOAT(x->acc_atoms+2, 0);
	for (i=0; i<CWIID_IR_SRC_COUNT; i++)
	{
		SETFLOAT(x->ir_atoms[i]+0, -1);
		SETFLOAT(x->ir_atoms[i]+1, 0);
		SETFLOAT(x->ir_atoms[i]+2, 0);
		SETFLOAT(x->ir_atoms[i]+3, 0);
	}
	SETFLOAT(x->nc_acc_atoms+0, 0);
	SETFLOAT(x->nc_acc_atoms+1, 0);
	SETFLOAT(x->nc_acc_atoms+2, 0);
	SETFLOAT(x->nc_stick_atoms+0, 0);
	SETFLOAT(x->nc_stick_atoms+1, 0);

	SETFLOAT(x->mp_ar_atoms+0, 0);
	SETFLOAT(x->mp_ar_atoms+1, 0);
	SETFLOAT(x->mp_ar_atoms+2, 0);

#ifdef HAVE_CWIID_MOTIONPLUS_LOWSPEED
	SETFLOAT(x->mp_ls_atoms+0, 0);
	SETFLOAT(x->mp_ls_atoms+1, 0);
	SETFLOAT(x->mp_ls_atoms+2, 0);
#endif

	SETFLOAT(x->cl_l_stick_atoms+0, 0);
	SETFLOAT(x->cl_l_stick_atoms+1, 0);

	SETFLOAT(x->cl_r_stick_atoms+0, 0);
	SETFLOAT(x->cl_r_stick_atoms+1, 0);

	SETFLOAT(x->old_cl_buttons_atoms+0, 0);
	SETFLOAT(x->old_cl_buttons_atoms+1, 0);
	SETFLOAT(x->old_cl_buttons_atoms+2, 0);
	SETFLOAT(x->old_cl_buttons_atoms+3, 0);

	SETFLOAT(x->cl_buttons_atoms+0, 0);
	SETFLOAT(x->cl_buttons_atoms+1, 0);
	SETFLOAT(x->cl_buttons_atoms+2, 0);
	SETFLOAT(x->cl_buttons_atoms+3, 0);

	SETFLOAT(x->balance_atoms+0, 0);
	SETFLOAT(x->balance_atoms+1, 0);
	SETFLOAT(x->balance_atoms+2, 0);
	SETFLOAT(x->balance_atoms+3, 0);

	x->connected = 0;
	x->wiimoteID = -1;
	x->extensionAttached = -1;

	x->rumble = 0;
	x->led = 0;
	x->addr = gensym("NULL");
	x->rpt = 0;
	x->rpt_mode = -1;
	x->passthrough = 0;
	x->passthrough_packet = 0;

	x->x_clock_status = clock_new(x, (t_method)pd_cwiid_tick_status);
	x->x_clock_battery = clock_new(x, (t_method)pd_cwiid_tick_battery);

	// prep the secondary thread init variable
	x->unsafe = 1;

	// spawn threads for actions known to cause sample drop-outs
	threadedFunctionParams rPars;
	rPars.wiimote = x;
	pthread_mutex_init(&x->unsafe_mutex, NULL);
	pthread_cond_init(&x->unsafe_cond, NULL);
	pthread_create( &x->unsafe_t, NULL, (void *) &pd_cwiid_pthreadForAudioUnfriendlyOperations, (void *) &rPars);

	// wait until other thread has properly intialized so that
	// rPars do not get destroyed before the thread has gotten its
	// pointer information
	while(x->unsafe == 1) {
		sched_yield();
	}

	// connect if user provided an address as an argument:
	if (argc==2)
	{
		post("conecting to provided address...");
		if (argv->a_type == A_SYMBOL)
		{
			pd_cwiid_doConnect(x, atom_getsymbol(argv));
		} else {
			error("[disis_wiimote] expects either no argument, or a bluetooth address as an argument. eg, 00:19:1D:70:CE:72");
			return NULL;
		}
	}
	return (x);
}


static void pd_cwiid_free(t_wiimote* x)
{
	if (x->connected) {
		pd_cwiid_doDisconnect(x);
	}

	x->unsafe = -1;

	pthread_mutex_lock(&x->unsafe_mutex);
	pthread_cond_signal(&x->unsafe_cond);
	pthread_mutex_unlock(&x->unsafe_mutex);

	pthread_join(x->unsafe_t, NULL);
	pthread_mutex_destroy(&x->unsafe_mutex);

	clock_free(x->x_clock_status);
	clock_free(x->x_clock_battery);
}

void disis_wiimote_setup(void)
{
	pd_cwiid_class = class_new(gensym("disis_wiimote"), (t_newmethod)pd_cwiid_new, (t_method)pd_cwiid_free, sizeof(t_wiimote), CLASS_DEFAULT, A_GIMME, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_debug, gensym("debug"), 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_doConnect, gensym("connect"), A_SYMBOL, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_doDisconnect, gensym("disconnect"), 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_discover, gensym("discover"), 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_setReportMode, gensym("setReportMode"), A_DEFFLOAT, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_reportAcceleration, gensym("reportAcceleration"), A_DEFFLOAT, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_reportExpansion, gensym("reportExpansion"), A_DEFFLOAT, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_reportIR, gensym("reportIR"), A_DEFFLOAT, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_togglePassthrough, gensym("togglePassthrough"), A_DEFFLOAT, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_setRumble, gensym("setRumble"), A_DEFFLOAT, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_setLED, gensym("setLED"), A_DEFFLOAT, 0);
	class_addmethod(pd_cwiid_class, (t_method) pd_cwiid_status, gensym("status"), 0);
	class_addbang(pd_cwiid_class, pd_cwiid_doBang);
}
