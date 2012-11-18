#include <m_pd.h>

#include "linuxhid.h"

#define LINUXEVENT_DEVICE   "/dev/input/event0"

static char *version = "$Revision: 1.1 $";

/*------------------------------------------------------------------------------
 *  CLASS DEF
 */
static t_class *linuxevent_class;

typedef struct _linuxevent {
  t_object            x_obj;
  t_int               x_fd;
  t_symbol            *x_devname;
  t_clock             *x_clock;
  int                 x_read_ok;
  int                 x_started;
  int                 x_delay;
#ifdef __gnu_linux__
  struct input_event  x_input_event; 
#endif
  t_outlet            *x_input_event_time_outlet;
  t_outlet            *x_input_event_type_outlet;
  t_outlet            *x_input_event_code_outlet;
  t_outlet            *x_input_event_value_outlet;
}t_linuxevent;

/*------------------------------------------------------------------------------
 * IMPLEMENTATION                    
 */

void linuxevent_stop(t_linuxevent* x) {
  DEBUG(post("linuxevent_stop"););
  
  if (x->x_fd >= 0 && x->x_started) { 
	  clock_unset(x->x_clock);
	  post("linuxevent: polling stopped");
	  x->x_started = 0;
  }
}

static int linuxevent_close(t_linuxevent *x) {
	DEBUG(post("linuxevent_close"););

/* just to be safe, stop it first */
	linuxevent_stop(x);

   if (x->x_fd <0) return 0;
   close (x->x_fd);
	post ("[linuxevent] closed %s",x->x_devname->s_name);
	
   return 1;
}

static int linuxevent_open(t_linuxevent *x, t_symbol *s) {
  int eventType, eventCode, buttons, rel_axes, abs_axes, ff;
#ifdef __gnu_linux__
  unsigned long bitmask[EV_MAX][NBITS(KEY_MAX)];
#endif
  char devicename[256] = "Unknown";
  DEBUG(post("linuxevent_open");)

  linuxevent_close(x);

  /* set obj device name to parameter 
   * otherwise set to default
   */  
  if (s != &s_)
    x->x_devname = s;
  
#ifdef __gnu_linux__
  /* open device */
  if (x->x_devname) {
	  /* open the device read-only, non-exclusive */
	  x->x_fd = open (x->x_devname->s_name, O_RDONLY | O_NONBLOCK);
	  /* test if device open */
	  if (x->x_fd < 0 ) { 
		  post("[linuxevent] open %s failed",x->x_devname->s_name);
		  x->x_fd = -1;
		  return 0;
	  }
  } else return 1;
  
  /* read input_events from the LINUXEVENT_DEVICE stream 
   * It seems that is just there to flush the event input buffer?
   */
  while (read (x->x_fd, &(x->x_input_event), sizeof(struct input_event)) > -1);
  
  /* get name of device */
  ioctl(x->x_fd, EVIOCGNAME(sizeof(devicename)), devicename);
  post ("Configuring %s on %s",devicename,x->x_devname->s_name);

  /* get bitmask representing supported events (axes, buttons, etc.) */
  memset(bitmask, 0, sizeof(bitmask));
  ioctl(x->x_fd, EVIOCGBIT(0, EV_MAX), bitmask[0]);
  post("\nSupported events:");
    
  rel_axes = 0;
  abs_axes = 0;
  buttons = 0;
  ff = 0;
    
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
      for (eventCode = 0; eventCode < KEY_MAX; eventCode++) 
	if (test_bit(eventCode, bitmask[eventType])) {
	  post("    Event code %d (%s)", eventCode, names[eventType] ? (names[eventType][eventCode] ? names[eventType][eventCode] : "?") : "?");

	  switch(eventType) {
// the API changed at some point...
#ifdef EV_RST
	  case EV_RST:
#else 
	  case EV_SYN:
#endif
	    break;
	  case EV_KEY:
	    buttons++;
	    break;
	  case EV_REL:
	    rel_axes++;
	    break;
	  case EV_ABS:
	    abs_axes++;
	    break;
	  case EV_MSC:
	    break;
	  case EV_LED:
	    break;
	  case EV_SND:
	    break;
	  case EV_REP:
	    break;
	  case EV_FF:
	    ff++;
	    break;
	  }
	}
    }        
  }
    
  post ("\nUsing %d relative axes, %d absolute axes, and %d buttons.", rel_axes, abs_axes, buttons);
  if (ff > 0) post ("Detected %d force feedback types",ff);
  post ("\nWARNING * WARNING * WARNING * WARNING * WARNING * WARNING * WARNING");
  post ("This object is under development!  The interface could change at anytime!");
  post ("As I write cross-platform versions, the interface might have to change.");
  post ("WARNING * WARNING * WARNING * WARNING * WARNING * WARNING * WARNING\n");
#endif
  
  return 1;
}

static int linuxevent_read(t_linuxevent *x,int fd) {
	if (x->x_fd < 0) return 0;

#ifdef __gnu_linux__
	while (read (x->x_fd, &(x->x_input_event), sizeof(struct input_event)) > -1) {
		outlet_float (x->x_input_event_value_outlet, (int)x->x_input_event.value);
		outlet_float (x->x_input_event_code_outlet, x->x_input_event.code);
		outlet_float (x->x_input_event_type_outlet, x->x_input_event.type);
		/* input_event.time is a timeval struct from <sys/time.h> */
		/*   outlet_float (x->x_input_event_time_outlet, x->x_input_event.time); */
	}
#endif
  
	if (x->x_started) {
		clock_delay(x->x_clock, x->x_delay);
	}

	return 1;    
}

/* Actions */
static void linuxevent_float(t_linuxevent* x) {
    DEBUG(post("linuxevent_float");)
   
}

void linuxevent_delay(t_linuxevent* x, t_float f)  {
	DEBUG(post("linuxevent_DELAY %f",f);)
		
/*	if the user sets the delay less than zero, reset to default */
	if ( f > 0 ) {	
		x->x_delay = (int)f;
	} else {
		x->x_delay = DEFAULT_DELAY;
	}
}

void linuxevent_start(t_linuxevent* x) {
	DEBUG(post("linuxevent_start"););
  
   if (x->x_fd >= 0 && !x->x_started) {
		clock_delay(x->x_clock, DEFAULT_DELAY);
		post("linuxevent: polling started");
		x->x_started = 1;
	} else {
		post("You need to set a input device (i.e /dev/input/event0)");
	}
}

/* setup functions */
static void linuxevent_free(t_linuxevent* x) {
  DEBUG(post("linuxevent_free");)
    
  if (x->x_fd < 0) return;

  linuxevent_stop(x);
  clock_free(x->x_clock);
  close (x->x_fd);
}

static void *linuxevent_new(t_symbol *s) {
  int i;
  t_linuxevent *x = (t_linuxevent *)pd_new(linuxevent_class);

  DEBUG(post("linuxevent_new");)

  post("[linuxevent] %s, written by Hans-Christoph Steiner <hans@eds.org>",version);  
#ifndef __linux__
	post("    !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !!");
	post("     This is a dummy, since this object only works with a Linux kernel!");
	post("    !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !! WARNING !!");
#endif

  /* init vars */
  x->x_fd = -1;
  x->x_read_ok = 1;
  x->x_started = 0;
  x->x_delay = DEFAULT_DELAY;
  x->x_devname = gensym(LINUXEVENT_DEVICE);

  x->x_clock = clock_new(x, (t_method)linuxevent_read);
  
  /* create outlets for each axis */
  x->x_input_event_time_outlet = outlet_new(&x->x_obj, &s_float);
  x->x_input_event_type_outlet = outlet_new(&x->x_obj, &s_float);
  x->x_input_event_code_outlet = outlet_new(&x->x_obj, &s_float);
  x->x_input_event_value_outlet = outlet_new(&x->x_obj, &s_float);
  
  /* set to the value from the object argument, if that exists */
  if (s != &s_)
	  x->x_devname = s;
  
  /* Open the device and save settings */
  
  if (!linuxevent_open(x,s)) return x;
  
  return (x);
}

void linuxevent_setup(void) {
  DEBUG(post("linuxevent_setup");)
  linuxevent_class = class_new(gensym("linuxevent"), 
			     (t_newmethod)linuxevent_new, 
			     (t_method)linuxevent_free,
			     sizeof(t_linuxevent),0,A_DEFSYM,0);

  /* add inlet datatype methods */
  class_addfloat(linuxevent_class,(t_method) linuxevent_float);
  class_addbang(linuxevent_class,(t_method) linuxevent_read);

  /* add inlet message methods */
  class_addmethod(linuxevent_class,(t_method) linuxevent_delay,gensym("delay"),A_DEFFLOAT,0);
  class_addmethod(linuxevent_class,(t_method) linuxevent_open,gensym("open"),A_DEFSYM,0);
  class_addmethod(linuxevent_class,(t_method) linuxevent_close,gensym("close"),0);
  class_addmethod(linuxevent_class,(t_method) linuxevent_start,gensym("start"),0);
  class_addmethod(linuxevent_class,(t_method) linuxevent_start,gensym("poll"),0);
  class_addmethod(linuxevent_class,(t_method) linuxevent_stop,gensym("stop"),0);
  class_addmethod(linuxevent_class,(t_method) linuxevent_stop,gensym("nopoll"),0);
}

