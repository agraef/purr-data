#include <m_pd.h>

#ifdef PD_MAJOR_VERSION
#include "s_stuff.h"
#else 
#include "m_imp.h"
#endif

#include "linuxhid.h"

#define LINUXMOUSE_DEVICE   "/dev/input/event0"
#define LINUXMOUSE_AXES     3

static char *version = "$Revision: 1.1 $";

/*------------------------------------------------------------------------------
 *  CLASS DEF
 */
static t_class *linuxmouse_class;

typedef struct _linuxmouse {
  t_object            x_obj;
  t_int               x_fd;
  t_symbol            *x_devname;
  int                 x_read_ok;
  int                 x_started;
#ifdef __gnu_linux__
  struct input_event  x_input_event; 
#endif
  t_outlet            *x_axis_out[LINUXMOUSE_AXES];
  t_outlet            *x_button_num_out;
  t_outlet            *x_button_val_out;
  unsigned char       x_buttons;
  unsigned char       x_axes;
} t_linuxmouse;

/*------------------------------------------------------------------------------
 * IMPLEMENTATION                    
 */

/* Actions */
void linuxmouse_stop(t_linuxmouse* x) {
	DEBUG(post("linuxmouse_stop"););

#ifdef __gnu_linux__
   if (x->x_fd >= 0 && x->x_started) { 
		sys_rmpollfn(x->x_fd);
		post("[linuxmouse] stopped");
		x->x_started = 0;
	} 
#endif
}

static int linuxmouse_close(t_linuxmouse *x) {
	DEBUG(post("linuxmouse_close"););

/* just to be safe, stop it first */
	linuxmouse_stop(x);
	
   if (x->x_fd < 0) {
		return 0;
	}
	else {		
		close (x->x_fd);
		post ("[linuxmouse] closed %s",x->x_devname->s_name);
		return 1;
	}
}

static int linuxmouse_open(t_linuxmouse *x, t_symbol *s) {
	int eventType, eventCode;
	char devicename[256] = "Unknown";
#ifdef __gnu_linux__
	unsigned long bitmask[EV_MAX][NBITS(KEY_MAX)];
#endif
	
	DEBUG(post("linuxmouse_open"););
	
	linuxmouse_close(x);
	
	/* set obj device name to parameter otherwise set to default */  
	if ( s != &s_ )  
		x->x_devname = s;
	
#ifdef __gnu_linux__	
	/* open device */
	if (x->x_devname) {
		/* open the device read-only, non-exclusive */
		x->x_fd = open (x->x_devname->s_name, O_RDONLY | O_NONBLOCK);
		/* test if device open */
		if (x->x_fd < 0 ) { 
			post("[linuxmouse] open %s failed",x->x_devname->s_name);
			x->x_fd = -1;
			return 0;
		}
	} else {
		post("[linuxmouse] no device set: %s",x->x_devname->s_name);
		return 1;
	}
	
/* read input_events from the LINUXMOUSE_DEVICE stream 
 * It seems that is just there to flush the event input buffer?
 */
	while (read (x->x_fd, &(x->x_input_event), sizeof(struct input_event)) > -1);
	
	/* get name of device */
	ioctl(x->x_fd, EVIOCGNAME(sizeof(devicename)), devicename);
	post ("Configuring %s on %s.",devicename,x->x_devname->s_name);
	post("\nSupported events:");

	/* get bitmask representing supported events (axes, buttons, etc.) */
	memset(bitmask, 0, sizeof(bitmask));
	ioctl(x->x_fd, EVIOCGBIT(0, EV_MAX), bitmask[0]);
	
	x->x_axes = 0;
	x->x_buttons = 0;
	
	/* cycle through all possible event types */
	for (eventType = 0; eventType < EV_MAX; eventType++) {
		if (test_bit(eventType, bitmask[0])) {
			post(" %s (type %d) ", events[eventType] ? events[eventType] : "?", eventType);
			//	post("Event type %d",eventType);
			
			/* get bitmask representing supported button types */
			ioctl(x->x_fd, EVIOCGBIT(eventType, KEY_MAX), bitmask[eventType]);
			
			/* cycle through all possible event codes (axes, keys, etc.) 
			 * testing to see which are supported  
			 */
			for (eventCode = 0; eventCode < KEY_MAX; eventCode++) {				
				if (test_bit(eventCode, bitmask[eventType])) {
					post("    Event code %d (%s)", eventCode, names[eventType] ? (names[eventType][eventCode] ? names[eventType][eventCode] : "?") : "?");
					
					if ( eventType == EV_KEY ) 
						x->x_buttons++;
					else if  ( eventType == EV_REL ) 
						x->x_axes++;
				}
			}
		}        
	}
	
	post ("\nUsing %d axes and %d buttons.", x->x_axes, x->x_buttons);
	post ("\nWARNING * WARNING * WARNING * WARNING * WARNING * WARNING * WARNING");
	post ("This object is under development!  The interface could change at anytime!");
	post ("As I write cross-platform versions, the interface might have to change.");
	post ("WARNING * WARNING * WARNING * WARNING * WARNING * WARNING * WARNING\n");
#endif
	
	return 1;
}

static int linuxmouse_read(t_linuxmouse *x,int fd) {
	int axis_num = 0;
	t_float button_num = 0;
	
	if (x->x_fd < 0) return 0;
	
#ifdef __gnu_linux__
	while (read (x->x_fd, &(x->x_input_event), sizeof(struct input_event)) > -1) {
		if  ( x->x_input_event.type == EV_REL ) {
			/* Relative Axes Event Type */
			switch ( x->x_input_event.code ) {
				case REL_X:
					axis_num = 0;
					break;
				case REL_Y:
					axis_num = 1;
					break;
				case REL_WHEEL:
					axis_num = 2;
					break;
			}
			outlet_float (x->x_axis_out[axis_num], (int)x->x_input_event.value);	
		}
		else if ( x->x_input_event.type == EV_KEY ) {
			/* key/button event type */
			switch ( x->x_input_event.code ) {
				case BTN_LEFT:
					button_num = 0;
					break;
				case BTN_RIGHT:
					button_num = 1;
					break;
				case BTN_MIDDLE:
					button_num = 2;
					break;
				case BTN_SIDE:
					button_num = 3;
					break;
				case BTN_EXTRA:
					button_num = 4;
					break;
				case BTN_FORWARD:
					button_num = 5;
					break;
				case BTN_BACK:
					button_num = 6;
					break;
			}
			outlet_float (x->x_button_val_out, x->x_input_event.value);
			outlet_float (x->x_button_num_out, button_num);
		}
 	}
#endif

	return 1;    
}

void linuxmouse_start(t_linuxmouse* x) {
	DEBUG(post("linuxmouse_start"););

/* if the device isn't open already, open it */
/* (I'll test this later -HCS) */
/*    if (x->x_fd < 0) linuxmouse_open(x,&s_); */
		
#ifdef __gnu_linux__
   if (x->x_fd >= 0 && !x->x_started) {
		sys_addpollfn(x->x_fd, (t_fdpollfn)linuxmouse_read, x);
		post("[linuxmouse] started");
		x->x_started = 1;
	} else {
		post("You need to set a input device (i.e /dev/input/event0)");
	}
#endif
}

/* setup functions */
static void linuxmouse_free(t_linuxmouse* x) {
	DEBUG(post("linuxmouse_free"););
    
	if (x->x_fd < 0) return;
	linuxmouse_stop(x);	
	close (x->x_fd);
}

static void *linuxmouse_new(t_symbol *s) {
	int i;
	t_linuxmouse *x = (t_linuxmouse *)pd_new(linuxmouse_class);
	
	DEBUG(post("linuxmouse_new"););
	
	post("[linuxmouse] %s, written by Hans-Christoph Steiner <hans@eds.org>",version);  
#ifndef __gnu_linux__
	post("    !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !!");
	post("     This is a dummy, since this object only works with a Linux kernel!");
	post("    !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !!");
#endif
	
	/* init vars */
	x->x_fd = -1;
	x->x_read_ok = 1;
	x->x_started = 0;
	x->x_devname = gensym("/dev/input/event0");

	/* create outlets for each axis */
	for (i = 0; i < LINUXMOUSE_AXES; i++) 
		x->x_axis_out[i] = outlet_new(&x->x_obj, &s_float);
	
	/* create outlets for buttons */
	x->x_button_num_out = outlet_new(&x->x_obj, &s_float);
	x->x_button_val_out = outlet_new(&x->x_obj, &s_float);

	if (!linuxmouse_open(x,s)) return x;
	
	return (x);
}

void linuxmouse_setup(void) {
	DEBUG(post("linuxmouse_setup"););
	linuxmouse_class = class_new(gensym("linuxmouse"), 
										  (t_newmethod)linuxmouse_new, 
										  (t_method)linuxmouse_free,
										  sizeof(t_linuxmouse), 0, A_DEFSYM, 0);
	
	/* add inlet message methods */
	class_addmethod(linuxmouse_class,(t_method) linuxmouse_open,gensym("open"),A_DEFSYM,0);
	class_addmethod(linuxmouse_class,(t_method) linuxmouse_close,gensym("close"),0);
	class_addmethod(linuxmouse_class,(t_method) linuxmouse_start,gensym("start"),0);
	class_addmethod(linuxmouse_class,(t_method) linuxmouse_stop,gensym("stop"),0);	
	class_addmethod(linuxmouse_class,(t_method) linuxmouse_start,gensym("poll"),0);
	class_addmethod(linuxmouse_class,(t_method) linuxmouse_stop,gensym("nopoll"),0);	
}

