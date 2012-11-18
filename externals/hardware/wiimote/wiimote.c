// ===================================================================
// Wiimote external for Puredata
// Written by Mike Wozniewski (Feb 2007), www.mikewoz.com
//
// Requires the CWiid library (version 0.6.00) by L. Donnie Smith
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
//  2009-09-14 IOhannes m zmölnig
//    * made it compile without private cwiid-headers


#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <bluetooth/bluetooth.h>
#include <m_pd.h>
#include <math.h>
#include <cwiid.h>

#define PI   3.14159265358979323

// class and struct declarations for wiimote pd external:
static t_class *wiimote_class;
typedef struct _wiimote
{
   t_object x_obj; // standard pd object (must be first in struct)

   cwiid_wiimote_t *wiimote; // individual wiimote handle per pd object, represented in libcwiid

   t_float connected;
   int wiimoteID;

   int reportMode;

   struct acc_cal acc_cal; /* calibration for built-in accelerometer */
#ifdef CWIID_RPT_NUNCHUK
   struct acc_cal nc_acc_cal;  /* calibration for nunchuk accelerometer */
#endif
#ifdef CWIID_RPT_BALANCE
   struct balance_cal balance_cal;
#endif

   // outlets:
   t_outlet *outlet_data;
   t_outlet *outlet_status;

  struct timespec*basetime;
  double baselogicaltime;
} t_wiimote;

static t_clock*g_clock=NULL;



typedef struct _wiimoteList {
  t_wiimote*x;
  int id;
  struct _wiimoteList*next;
} t_wiimoteList;

t_wiimoteList*g_wiimoteList=NULL;

static int addWiimoteObject(t_wiimote*x, int id) {
  t_wiimoteList*wl=g_wiimoteList;
  t_wiimoteList*newentry=NULL;
  if(NULL!=wl) {
    while(wl->next) {

      if(wl->x == x) {
        pd_error(x, "[wiimote]: already bound to Wii%02d", wl->id);
        return 0;
      }
      if(wl->id == id) {
        pd_error(x, "[wiimote]: another object is already bound to Wii%02d", wl->id);
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
static t_wiimote*getWiimoteObject(const int id) {
  t_wiimoteList*wl=g_wiimoteList;
  if(NULL==wl)
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
  t_wiimoteList*wl=g_wiimoteList;
  t_wiimoteList*last=NULL;
  if(NULL==wl)
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


/* time handling */


static void print_timestamp(struct timespec*timestamp, struct timespec*reference) {
   double t0=timestamp->tv_sec*1000. + (timestamp->tv_nsec) / 1000000.;
   double t1=0;
   double t=0;
   if(reference) {
      t1=reference->tv_sec*1000. + (reference->tv_nsec) / 1000000.;
   }

   t=t0-t1;

   post("timestamp: %f = %f-%f", (t), t0, t1);
}

static void wiimote_setbasetime(t_wiimote*x) {
  if(NULL==x->basetime) {
    x->basetime=(struct timespec*)getbytes(sizeof(struct timespec));
  }

  if (clock_gettime(CLOCK_REALTIME, x->basetime)) {
    // oops
    freebytes(x->basetime, sizeof(struct timespec));
    x->basetime=NULL;
  }

  x->baselogicaltime=clock_getlogicaltime();
}

static double wiimote_timestamp2logicaltime(t_wiimote*x, struct timespec*timestamp) {
  double pddelay=clock_gettimesince(x->baselogicaltime); /* how long in logical time since we have connected to the wii */
  double realdelay=0.;  /* how long (in ms) after we connected to the wiimote this timestamp appeared */
  double delay=0.;
  if(NULL==timestamp || NULL==x->basetime)
    return 0.; /* immediately */

  realdelay=
    (timestamp->tv_sec - x->basetime->tv_sec) * 1000. +
    (timestamp->tv_nsec - x->basetime->tv_nsec) / 1000000.;

  delay=realdelay-pddelay;

#if 0
  print_timestamp(timestamp, x->basetime);
  post("logical time: %f = %f - %f", clock_gettimesince(x->baselogicaltime), now, x->baselogicaltime);
#endif
  //post("diff = %f = %f - %f", realdelay-pddelay, realdelay, pddelay);

  if(delay<0) {
    return 0.;
  }

  return realdelay;
}


typedef struct _wiimoteMsgList {
  union cwiid_mesg*mesg;
  double timestamp;
  t_wiimote*x;
  struct _wiimoteMsgList*next;
} t_wiimoteMsgList;

t_wiimoteMsgList*g_wiimoteMsgList=NULL;

static void addWiimsg(union cwiid_mesg mesg, double timestamp)
{

}



// ==============================================================
/* forward declarations */
static void wiimote_doDisconnect(t_wiimote *x);



// ==============================================================

static void wiimote_debug(t_wiimote *x)
{
   post("\n======================");
   if (x->connected) post("Wiimote (id: %d) is connected.", x->wiimoteID);
   else post("Wiimote (id: %d) is NOT connected.", x->wiimoteID);
  post("acceleration: %s", (x->reportMode & CWIID_RPT_ACC)?"ON":"OFF");
  post("IR: %s", (x->reportMode & CWIID_RPT_IR)?"ON":"OFF");
  post("extensions: %s",  (x->reportMode & CWIID_RPT_EXT)?"ON":"OFF");
   post("");
   post("Accelerometer calibration: zero=(%d,%d,%d) one=(%d,%d,%d)",
          x->acc_cal.zero[CWIID_X], x->acc_cal.zero[CWIID_Y], x->acc_cal.zero[CWIID_Z],
          x->acc_cal.one [CWIID_X], x->acc_cal.one [CWIID_Y], x->acc_cal.one [CWIID_Z]);
   post("Nunchuk calibration:      zero=(%d,%d,%d) one=(%d,%d,%d)",
          x->nc_acc_cal.zero[CWIID_X], x->nc_acc_cal.zero[CWIID_Y], x->nc_acc_cal.zero[CWIID_Z],
          x->nc_acc_cal.one [CWIID_X], x->nc_acc_cal.one [CWIID_Y], x->nc_acc_cal.one [CWIID_Z]);



}

// ==============================================================
static void wiimote_out_status(t_wiimote *x, int state)
{
  t_atom ap[1];

  SETFLOAT(ap+0, state);

  outlet_anything(x->outlet_status, gensym("open"), 1, ap);
}


static void wiimote_cwiid_battery(t_wiimote *x, int battery)
{
   t_atom ap[1];
   t_float bat=(1.f*battery) / CWIID_BATTERY_MAX;

   SETFLOAT(ap+0, bat);

   outlet_anything(x->outlet_data, gensym("battery"), 1, ap);
}

// Button handler:
static void wiimote_cwiid_btn(t_wiimote *x, struct cwiid_btn_mesg *mesg)
{
   t_atom ap[2];
   SETFLOAT(ap+0, (mesg->buttons & 0xFF00)>>8);
   SETFLOAT(ap+1, mesg->buttons & 0x00FF);
   outlet_anything(x->outlet_data, gensym("button"), 2, ap);
}


static void wiimote_cwiid_acc(t_wiimote *x, struct cwiid_acc_mesg *mesg)
{
   double a_x, a_y, a_z;
   t_atom ap[3];

   a_x = ((double)mesg->acc[CWIID_X] - x->acc_cal.zero[CWIID_X]) / (x->acc_cal.one[CWIID_X] - x->acc_cal.zero[CWIID_X]);
   a_y = ((double)mesg->acc[CWIID_Y] - x->acc_cal.zero[CWIID_Y]) / (x->acc_cal.one[CWIID_Y] - x->acc_cal.zero[CWIID_Y]);
   a_z = ((double)mesg->acc[CWIID_Z] - x->acc_cal.zero[CWIID_Z]) / (x->acc_cal.one[CWIID_Z] - x->acc_cal.zero[CWIID_Z]);

   /*
      double a, roll, pitch;
      a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
      roll = atan(a_x/a_z);
      if (a_z <= 0.0) roll += PI * ((a_x > 0.0) ? 1 : -1);
      roll *= -1;
      pitch = atan(a_y/a_z*cos(roll));
   */


   SETFLOAT(ap+0, a_x);
   SETFLOAT(ap+1, a_y);
   SETFLOAT(ap+2, a_z);
   outlet_anything(x->outlet_data, gensym("acceleration"), 3, ap);

}

static void wiimote_cwiid_ir(t_wiimote *x, struct cwiid_ir_mesg *mesg)
{
   unsigned int i;

   //post("IR (valid,x,y,size) #%d: %d %d %d %d", i, data->ir_data.ir_src[i].valid, data->ir_data.ir_src[i].x, data->ir_data.ir_src[i].y, data->ir_data.ir_src[i].size);
   for (i=0; i<CWIID_IR_SRC_COUNT; i++){
      if (mesg->src[i].valid) {
         t_atom ap[4];
         SETFLOAT(ap+0, i);
         SETFLOAT(ap+1, mesg->src[i].pos[CWIID_X]);
         SETFLOAT(ap+2, mesg->src[i].pos[CWIID_Y]);
         SETFLOAT(ap+3, mesg->src[i].size);
         outlet_anything(x->outlet_data, gensym("ir"), 4, ap);
      }
   }
}

#ifdef CWIID_RPT_NUNCHUK
static void wiimote_cwiid_nunchuk(t_wiimote *x, struct cwiid_nunchuk_mesg *mesg)
{
   t_atom ap[4];
   double a_x, a_y, a_z;

   a_x = ((double)mesg->acc[CWIID_X] - x->nc_acc_cal.zero[CWIID_X]) / (x->nc_acc_cal.one[CWIID_X] - x->nc_acc_cal.zero[CWIID_X]);
   a_y = ((double)mesg->acc[CWIID_Y] - x->nc_acc_cal.zero[CWIID_Y]) / (x->nc_acc_cal.one[CWIID_Y] - x->nc_acc_cal.zero[CWIID_Y]);
   a_z = ((double)mesg->acc[CWIID_Z] - x->nc_acc_cal.zero[CWIID_Z]) / (x->nc_acc_cal.one[CWIID_Z] - x->nc_acc_cal.zero[CWIID_Z]);

   /*
   double a, roll, pitch;
   a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
   roll = atan(a_x/a_z);
   if (a_z <= 0.0) roll += PI * ((a_x > 0.0) ? 1 : -1);
   roll *= -1;
   pitch = atan(a_y/a_z*cos(roll));
   */
   if (mesg->buttons & CWIID_NUNCHUK_BTN_C) {}
   if (mesg->buttons & CWIID_NUNCHUK_BTN_Z) {}
   /* nunchuk button */
   SETSYMBOL(ap+0, gensym("button"));
   SETFLOAT (ap+1, (t_float)mesg->buttons);
   outlet_anything(x->outlet_data, gensym("nunchuk"), 2, ap);


   /* nunchuk button */
   SETSYMBOL(ap+0, gensym("acceleration"));
   SETFLOAT (ap+1, a_x);
   SETFLOAT (ap+2, a_y);
   SETFLOAT (ap+3, a_z);
   outlet_anything(x->outlet_data, gensym("nunchuk"), 4, ap);

   /* nunchuk button */
   SETSYMBOL(ap+0, gensym("stick"));
   SETFLOAT (ap+1, mesg->stick[CWIID_X]);
   SETFLOAT (ap+2, mesg->stick[CWIID_Y]);
   outlet_anything(x->outlet_data, gensym("nunchuk"), 3, ap);
}
#endif

#ifdef CWIID_RPT_CLASSIC
static void wiimote_cwiid_classic(t_wiimote *x, struct cwiid_classic_mesg *mesg)
{
   t_atom ap[3];

   //   t_float scale = 1.f / ((uint16_t)0xFFFF);

   SETSYMBOL(ap+0, gensym("left_stick"));
   SETFLOAT (ap+1, mesg->l_stick[CWIID_X]);
   SETFLOAT (ap+2, mesg->l_stick[CWIID_Y]);
   outlet_anything(x->outlet_data, gensym("classic"), 3, ap);

   SETSYMBOL(ap+0, gensym("right_stick"));
   SETFLOAT (ap+1, mesg->r_stick[CWIID_X]);
   SETFLOAT (ap+2, mesg->r_stick[CWIID_Y]);
   outlet_anything(x->outlet_data, gensym("classic"), 3, ap);


   SETSYMBOL(ap+0, gensym("left"));
   SETFLOAT (ap+1, mesg->l);
   outlet_anything(x->outlet_data, gensym("classic"), 2, ap);

   SETSYMBOL(ap+0, gensym("right"));
   SETFLOAT (ap+1, mesg->r);
   outlet_anything(x->outlet_data, gensym("classic"), 2, ap);


   SETSYMBOL(ap+0, gensym("button"));
   SETFLOAT(ap+1, (mesg->buttons & 0xFF00)>>8);
   SETFLOAT(ap+2, mesg->buttons & 0x00FF);

   outlet_anything(x->outlet_data, gensym("classic"), 3, ap);


}
#endif

#ifdef CWIID_RPT_BALANCE
#warning Balance ignores calibration data

static void wiimote_cwiid_balance_output(t_wiimote *x, t_symbol*s, uint16_t value, uint16_t calibration[3])
{
  /*
    1700 appears to be the step the calibrations are against.
    17kg per sensor is 68kg, 1/2 of the advertised Japanese weight limit.
  */
#define WIIMOTE_BALANCE_CALWEIGHT 1700.f
   t_atom ap[2];

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
   SETSYMBOL(ap+0, s);
   SETFLOAT (ap+1, weight);
   outlet_anything(x->outlet_data, gensym("balance"), 2, ap);
}

static void wiimote_cwiid_balance(t_wiimote *x, struct cwiid_balance_mesg *mesg)
{
   wiimote_cwiid_balance_output(x, gensym("right_top"),    mesg->right_top,    x->balance_cal.right_top);
   wiimote_cwiid_balance_output(x, gensym("right_bottom"), mesg->right_bottom, x->balance_cal.right_bottom);
   wiimote_cwiid_balance_output(x, gensym("left_top"),     mesg->left_top,     x->balance_cal.left_top);
   wiimote_cwiid_balance_output(x, gensym("left_bottom"),  mesg->left_bottom,  x->balance_cal.left_bottom);
}
#endif

#ifdef CWIID_RPT_MOTIONPLUS
static void wiimote_cwiid_motionplus(t_wiimote *x, struct cwiid_motionplus_mesg *mesg)
{
   t_atom ap[4];
   t_float scale = 1.f;// / ((uint16_t)0xFFFF);

   t_float phi  = scale*mesg->angle_rate[CWIID_PHI];
   t_float theta= scale*mesg->angle_rate[CWIID_THETA];
   t_float psi  = scale*mesg->angle_rate[CWIID_PSI];

   t_float phi_speed   = 1.;
   t_float theta_speed = 1.;
   t_float psi_speed   = 1.;


   SETSYMBOL(ap+0, gensym("low_speed"));
#ifdef HAVE_CWIID_MOTIONPLUS_LOWSPEED
   phi_speed  = mesg->low_speed[CWIID_PHI];
   theta_speed= mesg->low_speed[CWIID_THETA];
   psi_speed  = mesg->low_speed[CWIID_PSI];
#endif
   SETFLOAT (ap+1, phi_speed);
   SETFLOAT (ap+2, theta_speed);
   SETFLOAT (ap+3, psi_speed);
   outlet_anything(x->outlet_data, gensym("motionplus"), 4, ap);


   SETSYMBOL(ap+0, gensym("angle_rate"));
   SETFLOAT (ap+1, phi);
   SETFLOAT (ap+2, theta);
   SETFLOAT (ap+3, psi);
   outlet_anything(x->outlet_data, gensym("motionplus"), 4, ap);
}
#endif

static void wiimote_cwiid_error(t_wiimote *x, struct cwiid_error_mesg *mesg) {
  switch(mesg->error) {
  case CWIID_ERROR_NONE:
    pd_error(x, "no error");
    break;
  case CWIID_ERROR_DISCONNECT:
    pd_error(x, "disconnect error");
    wiimote_doDisconnect(x);
    break;
  case CWIID_ERROR_COMM:
    pd_error(x, "comm error");
    wiimote_doDisconnect(x);
    break;
  default:
    pd_error(x, "unknown error %d", mesg->error);
  }
}

static void wiimote_cwiid_message(t_wiimote *x, union cwiid_mesg*mesg) {
  if(NULL==x){
    return;
  }
  if(NULL==mesg) {
    return;
  }
   switch (mesg->type) {
   case CWIID_MESG_STATUS:
      wiimote_cwiid_battery(x, mesg->status_mesg.battery);
      switch (mesg->status_mesg.ext_type) {
      case CWIID_EXT_NONE:
         verbose(1, "No extension attached");
         break;
#ifdef CWIID_RPT_NUNCHUK
      case CWIID_EXT_NUNCHUK:
         post("Nunchuk extension attached");
         if(cwiid_get_acc_cal(x->wiimote, CWIID_EXT_NUNCHUK, &x->nc_acc_cal)) {
            post("Unable to retrieve nunchuk calibration");
         } else {
            post("Retrieved nunchuk calibration: zero=(%02d,%02d,%02d) one=(%02d,%02d,%02d)",
                   x->nc_acc_cal.zero[CWIID_X],
                   x->nc_acc_cal.zero[CWIID_Y],
                   x->nc_acc_cal.zero[CWIID_Z],
                   x->nc_acc_cal.one [CWIID_X],
                   x->nc_acc_cal.one [CWIID_Y],
                   x->nc_acc_cal.one [CWIID_Z]);
         }
         break;
#endif
#ifdef CWIID_RPT_CLASSIC
      case CWIID_EXT_CLASSIC:
         post("Classic controller attached. There is no real support for this yet.");
         break;
#endif
#ifdef CWIID_RPT_BALANCE
      case CWIID_EXT_BALANCE:
         if(cwiid_get_balance_cal(x->wiimote, &x->balance_cal)) {
            post("Unable to retrieve balance calibration");
      }
         break;
#endif
#ifdef CWIID_RPT_MOTIONPLUS
      case CWIID_EXT_MOTIONPLUS:
         post("MotionPlus controller attached.");
         /* no calibration needed for MotionPlus */
         break;
#endif
      case CWIID_EXT_UNKNOWN:
         post("Unknown extension attached");
         break;
      default:
	post("ext mesg %d/%d unknown", mesg->type, mesg->status_mesg.ext_type);
         break;
      }
      break;
   case CWIID_MESG_BTN:
      wiimote_cwiid_btn(x, &mesg->btn_mesg);
      break;
   case CWIID_MESG_ACC:
      wiimote_cwiid_acc(x, &mesg->acc_mesg);
      break;
   case CWIID_MESG_IR:
      wiimote_cwiid_ir(x, &mesg->ir_mesg);
      break;
#ifdef CWIID_RPT_NUNCHUK
   case CWIID_MESG_NUNCHUK:
      wiimote_cwiid_nunchuk(x, &mesg->nunchuk_mesg);
      break;
#endif
#ifdef CWIID_RPT_CLASSIC
   case CWIID_MESG_CLASSIC:
      wiimote_cwiid_classic(x, &mesg->classic_mesg);
      // todo
      break;
#endif
#ifdef CWIID_RPT_MOTIONPLUS
   case CWIID_MESG_MOTIONPLUS:
      wiimote_cwiid_motionplus(x, &mesg->motionplus_mesg);
      break;
#endif
#ifdef CWIID_RPT_BALANCE
   case CWIID_MESG_BALANCE:
      wiimote_cwiid_balance(x, &mesg->balance_mesg);
      break;
#endif
   case CWIID_MESG_ERROR:
     wiimote_cwiid_error(x, &mesg->error_mesg);
   default:
      post("mesg %d unknown", (mesg->type));
      break;
   }
}

#if 0
// queuing with sorted lists leads to "Mesg pipe overflow"

static void wiimote_dequeue(void*nada)
{
  /* get all the messages from the queue that are scheduled until now */

  const int dyndelay=0; /* set this to 1 use dynamic delays for compensation of asynchronicity */

  t_wiimoteMsgList*wl=g_wiimoteMsgList;
  t_wiimoteMsgList*next=NULL;
  double now=0;
  double nexttime=0.;

  if(NULL==wl) {
    /* no messages to dequeue; this should never happen */
  }
  while(wl) {
    if(dyndelay) {
      now=clock_gettimesince(wl->x->baselogicaltime);
      if(now+1.<wl->timestamp) {
        /* no more messages to do for now, aborting */
        break;
      }
    }
    next=wl->next;
    wiimote_cwiid_message(wl->x, wl->mesg);
    wl->x=NULL;
    wl->timestamp=0.;
    freebytes(wl->mesg, sizeof( union cwiid_mesg));
    wl->mesg=NULL;
    wl->next=NULL;
    freebytes(wl, sizeof(t_wiimoteMsgList));
    wl=next;
  }

  g_wiimoteMsgList=wl;

  /* reschedule clock */
  if(dyndelay) {
    if(wl) {
      double delay=wl->timestamp - now;
      if(delay<1.)delay=1.;
      clock_delay(g_clock, delay);
    }
  }
}


static void wiimote_queue(t_wiimote*x, union cwiid_mesg*mesg, double timestamp)
{
  /* add mesg to the queue with a Pd timestamp */
  t_wiimoteMsgList*wl=g_wiimoteMsgList;
  t_wiimoteMsgList*lastentry=NULL;

    /* insert the current message into the list */
  t_wiimoteMsgList*newentry=(t_wiimoteMsgList*)getbytes(sizeof(t_wiimoteMsgList));
  newentry->next=NULL;
  newentry->x=x;
  newentry->mesg=(union cwiid_mesg*)getbytes(sizeof( union cwiid_mesg));
  memcpy(newentry->mesg, mesg, (sizeof( union cwiid_mesg)));
  //  post("queueing %x message %x (was %x) at %f", newentry, newentry->mesg, mesg, timestamp);

  newentry->timestamp=timestamp;

  if(NULL!=wl) {
    while(wl->next) {
      if(wl->timestamp>timestamp){
        break;
      }
      lastentry=wl;
      wl=wl->next;
    }
  }

  if(lastentry) {
    newentry->next=lastentry->next;
    lastentry->next=newentry;
  } else {
    // at the beginning
    newentry->next=g_wiimoteMsgList;
    g_wiimoteMsgList=newentry;
  }

  /* reset the clock */
  clock_delay(g_clock, 0);
}
#else
static void wiimote_dequeue(void*nada)
{
  /* flush all the messages from the queue */
  t_wiimoteMsgList*wl=g_wiimoteMsgList;
  t_wiimoteMsgList*next=NULL;

  nada=NULL; /* ignore */

  if(NULL==wl) {
    /* no messages to dequeue */
    return;
  }
  while(wl) {
    next=wl->next;
    wiimote_cwiid_message(wl->x, wl->mesg);
    wl->x=NULL;
    wl->timestamp=0.;
    freebytes(wl->mesg, sizeof( union cwiid_mesg));
    wl->mesg=NULL;
    wl->next=NULL;
    freebytes(wl, sizeof(t_wiimoteMsgList));
    wl=next;
  }

  g_wiimoteMsgList=NULL;
}


static void wiimote_queue(t_wiimote*x, union cwiid_mesg*mesg, double timestamp)
{
  /* add mesg to the queue with a Pd timestamp */
  // t_wiimoteMsgList*wl=g_wiimoteMsgList;

    /* insert the current message into the list */
  t_wiimoteMsgList*newentry=(t_wiimoteMsgList*)getbytes(sizeof(t_wiimoteMsgList));
  newentry->next=g_wiimoteMsgList;
  newentry->x=x;
  newentry->mesg=(union cwiid_mesg*)getbytes(sizeof( union cwiid_mesg));
  memcpy(newentry->mesg, mesg, (sizeof( union cwiid_mesg)));
  newentry->timestamp=timestamp;

  g_wiimoteMsgList=newentry;

  /* reset the clock */
  clock_delay(g_clock, 0);
}
#endif



static void cwiid_error_callback(cwiid_wiimote_t *wiimote, const char*err, va_list ap) {
  t_wiimote *x=NULL;
  if(g_wiimoteList==NULL||wiimote==NULL) {
    post("no wii's known");
    return;
  }
  x=getWiimoteObject(cwiid_get_id(wiimote));
 
  if(NULL==x) {
    if(err) {
      error("wiimote: %s", err);
    } else {
      error("wiimote: unknown error");
    }
  } else {
    if(err) {
      pd_error(x, "wiimote: %s", err);
    } else {
      pd_error(x, "wiimote: unknown error");
    }
  }
}

// The CWiid library invokes a callback function whenever events are
// generated by the wiimote. This function is specified when connecting
// to the wiimote (in the cwiid_open function).

// Unfortunately, the mesg struct passed as an argument to the
// callback does not have a pointer to the wiimote instance, and it
// is thus impossible to know which wiimote-object has invoked the callback.
// For this case we provide a hard-coded set of wrapper callbacks to
// indicate which Pd wiimote instance to control.

static void cwiid_callback(cwiid_wiimote_t *wiimote, int mesg_count,
                    union cwiid_mesg mesg_array[], struct timespec *timestamp)
{
   int i;
  t_wiimote *x=NULL;
  double pd_timestamp=0;

  //  print_timestamp(timestamp, NULL);


  if(g_wiimoteList==NULL||wiimote==NULL) {
    post("no wii's known");
    return;
  }
  x=getWiimoteObject(cwiid_get_id(wiimote));
  if(NULL==x) {
      post("no wiimote loaded: %d%",cwiid_get_id(wiimote));
    return;
   }

  pd_timestamp=wiimote_timestamp2logicaltime(x, timestamp);

  sys_lock();
  for (i=0; i < mesg_count; i++) {
#if 1
    wiimote_queue(x, mesg_array+i, pd_timestamp);
#else
    wiimote_cwiid_message(x, mesg_array+i);
#endif
   }
  sys_unlock();
}

// ==============================================================

static void wiimote_status(t_wiimote *x)
{
  if(x->connected) {
    if (cwiid_request_status(x->wiimote)) {
      pd_error(x, "error requesting status message");
    }
  }
  wiimote_out_status(x, x->connected);
}


static void wiimote_resetReportMode(t_wiimote *x)
{
   if (x->connected)   {
      verbose(1, "changing report mode for Wii%02d to %d", x->wiimoteID, x->reportMode);
      if (cwiid_command(x->wiimote, CWIID_CMD_RPT_MODE, x->reportMode)) {
	pd_error(x, "wiimote: could not set report mode.");
      }
   }
}


static void wiimote_setReportMode(t_wiimote *x, t_floatarg r)
{
   if (r >= 0) {
      x->reportMode = (int) r;
      wiimote_resetReportMode(x);
   }   else {
      return;
   }
}


static void wiimote_report(t_wiimote*x, t_symbol*s, int onoff)
{
  int flag=-1;
  if(gensym("status")==s) flag=CWIID_RPT_STATUS;
  else if(gensym("button")==s) flag=CWIID_RPT_BTN;
  else if(gensym("acceleration")==s) flag=CWIID_RPT_ACC;
  else if(gensym("ir")==s) flag=CWIID_RPT_IR;
#ifdef CWIID_RPT_NUNCHUK
  else if(gensym("nunchuk")==s) flag=CWIID_RPT_NUNCHUK;
#endif
#ifdef CWIID_RPT_CLASSIC
  else if(gensym("classic")==s) flag=CWIID_RPT_CLASSIC;
#endif
#ifdef CWIID_RPT_BALANCE
  else if(gensym("balance")==s) flag=CWIID_RPT_BALANCE;
#endif
#ifdef CWIID_RPT_MOTIONPLUS
  else if(gensym("motionplus")==s) flag=CWIID_RPT_MOTIONPLUS;
#endif
  else if(gensym("ext")==s) flag=CWIID_RPT_EXT;
  else {
    pd_error(x, "unknown report mode '%s'", s->s_name);
  }

#ifdef CWIID_RPT_MOTIONPLUS
  if(CWIID_RPT_MOTIONPLUS==flag) {
      if(x->connected) {
	int err=0;
	if(onoff) {
	  err=cwiid_enable(x->wiimote, CWIID_FLAG_MOTIONPLUS);
	} else {
	  err=cwiid_disable(x->wiimote, CWIID_FLAG_MOTIONPLUS);
	}
	if(err) {
	  pd_error(x, "turning %s motionplus returned %d", (flag?"on":"off"), err);
	}
      }
  }
#endif

  if(flag!=-1) {
    if(onoff) {
      x->reportMode |= flag;
    } else {
      x->reportMode &= ~flag;
    }
  }
   wiimote_resetReportMode(x);
}

static void wiimote_reportAcceleration(t_wiimote *x, t_floatarg f)
{
   wiimote_report(x, gensym("acceleration"), f);
}

static void wiimote_reportIR(t_wiimote *x, t_floatarg f)
{
   wiimote_report(x, gensym("ir"), f);
}

static void wiimote_reportNunchuk(t_wiimote *x, t_floatarg f)
{
   wiimote_report(x, gensym("nunchuk"), f);
}

static void wiimote_reportMotionplus(t_wiimote *x, t_floatarg f)
{
   wiimote_report(x, gensym("motionplus"), f);
}

static void wiimote_setRumble(t_wiimote *x, t_floatarg f)
{
   if (x->connected)
   {
     if (cwiid_command(x->wiimote, CWIID_CMD_RUMBLE, f)) {
       pd_error(x, "wiimote: could not set rumble");
     }
   }
}

static void wiimote_setLED(t_wiimote *x, t_floatarg f)
{
   // some possible values:
   // CWIID_LED0_ON      0x01
   // CWIID_LED1_ON      0x02
   // CWIID_LED2_ON      0x04
   // CWIID_LED3_ON      0x08
   if (x->connected)
   {
     if (cwiid_command(x->wiimote, CWIID_CMD_LED, f)) {
       pd_error(x, "wiimote: could not set LED.");
     }
   }
}



// ==============================================================

// The following function attempts to connect to a wiimote at a
// specific address, provided as an argument. eg, 00:19:1D:70:CE:72
// This address can be discovered by running the following command
// in a console:
//   hcitool scan | grep Nintendo
static void wiimote_doConnect(t_wiimote *x, t_symbol *addr, t_symbol *dongaddr)
{
   bdaddr_t bdaddr;
   unsigned int flags =  CWIID_FLAG_MESG_IFC;

   bdaddr_t  dong_bdaddr;
   bdaddr_t* dong_bdaddr_ptr=&dong_bdaddr;

   if(x->connected) {
     wiimote_doDisconnect(x);
   }

   // determine address:
   if (NULL==addr || addr==gensym("")) {
     verbose(1, "searching for wii...");
     bdaddr = *BDADDR_ANY;
   } else {
     str2ba(addr->s_name, &bdaddr);
     verbose(1, "Connecting to Wii '%s'", addr->s_name);
   }
   post("Press buttons 1 and 2 simultaneously.");

   // determine dongleaddress:
   if (NULL==dongaddr || dongaddr==gensym("")) {
     verbose(1, "using default dongle");
     dong_bdaddr_ptr = NULL;
   }   else {
     verbose(1, "using dongle '%s'", dongaddr->s_name);
     str2ba(dongaddr->s_name, &dong_bdaddr);
   }
   // connect:


#ifdef CWIID_OPEN_WITH_DONGLE
   verbose(1,"wiimote: opening multidongle");
   x->wiimote = cwiid_open(&bdaddr, dong_bdaddr_ptr, flags);
#else
   verbose(1,"wiimote: opening");
   x->wiimote = cwiid_open(&bdaddr, flags);
#endif

  if(NULL==x->wiimote) {
    pd_error(x, "wiimote: unable to connect");
    wiimote_out_status(x, x->connected);
    return;
  }

  if(!addWiimoteObject(x, cwiid_get_id(x->wiimote))) {
    cwiid_close(x->wiimote);
    x->wiimote=NULL;
    wiimote_out_status(x, x->connected);
    return;
  }

  x->wiimoteID= cwiid_get_id(x->wiimote);

  post("wiimote %i is successfully connected", x->wiimoteID);

   if(cwiid_get_acc_cal(x->wiimote, CWIID_EXT_NONE, &x->acc_cal)) {
    post("Unable to retrieve accelerometer calibration");
  } else {
    post("Retrieved wiimote calibration: zero=(%02d,%02d,%02d) one=(%02d,%02d,%02d)",
             x->acc_cal.zero[CWIID_X],
             x->acc_cal.zero[CWIID_Y],
             x->acc_cal.zero[CWIID_Z],
             x->acc_cal.one [CWIID_X],
             x->acc_cal.one [CWIID_Y],
             x->acc_cal.one [CWIID_Z]);
  }

  x->connected = 1;
  wiimote_out_status(x, x->connected);

   x->reportMode |= CWIID_RPT_STATUS;
   x->reportMode |= CWIID_RPT_BTN;
  wiimote_resetReportMode(x);

  if (cwiid_set_mesg_callback(x->wiimote, &cwiid_callback)) {
    pd_error(x, "Unable to set message callback");
  }

  wiimote_setbasetime(x);
}

// The following function attempts to discover a wiimote. It requires
// that the user puts the wiimote into 'discoverable' mode before being
// called. This is done by pressing the red button under the battery
// cover, or by pressing buttons 1 and 2 simultaneously.
// TODO: Without pressing the buttons, I get a segmentation error. So far, I don't know why.

static void wiimote_discover(t_wiimote *x)
{
   post("Put the wiimote into discover mode by pressing buttons 1 and 2 simultaneously.");

   wiimote_doConnect(x, NULL, NULL);
   if (!(x->connected))
   {
     pd_error(x, "could not find any wiimotes. Please ensure that bluetooth is enabled, and that the 'hcitool scan' command lists your Nintendo device.");
   }
}

static void wiimote_doDisconnect(t_wiimote *x)
{

   if (x->connected)
   {
      if (cwiid_close(x->wiimote)) {
	pd_error(x, "wiimote: unable to close connection.");
      }
      else {
	post("disconnect successfull, resetting values");
	removeWiimoteObject(x);
	x->connected = 0;
      }
   } else {
     post("device is not connected");
   }

   wiimote_out_status(x, x->connected);
}


static void wiimote_bang(t_wiimote *x)
{
  wiimote_out_status(x, x->connected);
}

// ==============================================================
// ==============================================================

static void *wiimote_new(t_symbol*s, int argc, t_atom *argv)
{
   t_wiimote *x = (t_wiimote *)pd_new(wiimote_class);

   // create outlets:
   x->outlet_data = outlet_new(&x->x_obj, NULL);
   x->outlet_status = outlet_new(&x->x_obj, NULL);

   // initialize toggles:
   x->connected = 0;
   x->wiimoteID = -1;

  x->basetime=NULL;
  x->baselogicaltime=0.;

  // connect if user provided an address as an argument:
  if (argc==2) {
      post("[%s] connecting to provided address...", s->s_name);
      if (argv->a_type == A_SYMBOL)   {
         wiimote_doConnect(x, NULL, atom_getsymbol(argv));
      } else {
         error("[wiimote] expects either no argument, or a bluetooth address as an argument. eg, 00:19:1D:70:CE:72");
         return NULL;
      }
   }
   return (x);
}


static void wiimote_free(t_wiimote* x)
{
   wiimote_doDisconnect(x);
  /* cleanup the queue */

  /* free the clock */
  if(x->basetime) {
    freebytes(x->basetime, sizeof(struct timespec));
  }

  if(x->outlet_data)outlet_free(x->outlet_data); x->outlet_data=NULL;
  if(x->outlet_status)outlet_free(x->outlet_status); x->outlet_status=NULL;
}

void wiimote_setup(void)
{
  g_clock = clock_new(NULL, (t_method)wiimote_dequeue);

  if (cwiid_set_err(&cwiid_error_callback)) {
    error("wiimote: unable to set error callback");
  }


   wiimote_class = class_new(gensym("wiimote"), (t_newmethod)wiimote_new, (t_method)wiimote_free, sizeof(t_wiimote), CLASS_DEFAULT, A_GIMME, 0);

   class_addmethod(wiimote_class, (t_method) wiimote_debug, gensym("debug"), 0);
   class_addmethod(wiimote_class, (t_method) wiimote_status, gensym("status"), 0);


   /* connection settings */
   class_addmethod(wiimote_class, (t_method) wiimote_doConnect, gensym("connect"), A_DEFSYMBOL, A_DEFSYMBOL, 0);
   class_addmethod(wiimote_class, (t_method) wiimote_doDisconnect, gensym("disconnect"), 0);
   class_addmethod(wiimote_class, (t_method) wiimote_discover, gensym("discover"), 0);


   /* query data */
   class_addbang(wiimote_class, (t_method) wiimote_bang);

   class_addmethod(wiimote_class, (t_method) wiimote_report, gensym("report"), A_SYMBOL, A_FLOAT, 0);

   class_addmethod(wiimote_class, (t_method) wiimote_setReportMode, gensym("setReportMode"), A_FLOAT, 0);
   class_addmethod(wiimote_class, (t_method) wiimote_reportAcceleration, gensym("reportAcceleration"), A_FLOAT, 0);
   class_addmethod(wiimote_class, (t_method) wiimote_reportIR, gensym("reportIR"), A_FLOAT, 0);

   class_addmethod(wiimote_class, (t_method) wiimote_reportNunchuk, gensym("reportNunchuck"), A_FLOAT, 0);
   class_addmethod(wiimote_class, (t_method) wiimote_reportNunchuk, gensym("reportNunchuk"), A_FLOAT, 0);
   class_addmethod(wiimote_class, (t_method) wiimote_reportMotionplus, gensym("reportMotionplus"), A_FLOAT, 0);


   /* set things on the wiimote */
   class_addmethod(wiimote_class, (t_method) wiimote_setRumble, gensym("setRumble"), A_FLOAT, 0);
   class_addmethod(wiimote_class, (t_method) wiimote_setLED, gensym("setLED"), A_FLOAT, 0);

   post("[wiimote]: reading data from the Wii remote controller");
   post("	(c) 2007 Mike Wozniewski");
   post("	(c) 2008-2009 Florian Krebs");
   post("	(c) 2009-2010 IOhannes m zmoelnig");
#ifdef VERSION
   post("	version " VERSION " published under the GNU General Public License");
#else
   post("	published under the GNU General Public License");
#endif
}
