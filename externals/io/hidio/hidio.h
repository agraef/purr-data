#ifndef _HIDIO_H
#define _HIDIO_H

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define LOG_DEBUG 7
#define LOG_INFO 6
#define LOG_WARNING 4
#define LOG_ERR 3
#define vsnprintf _vsnprintf
#include <windows.h>
#else
#include <sys/syslog.h>
#endif /* _WIN32 */

#ifdef _MSC_VER /* this only applies to Microsoft compilers */
#pragma warning (disable: 4305 4244 4761)
#endif /* _MSC_VER */

#ifdef __linux__
#include <linux/types.h>
#endif /* __linux__ */

#ifdef PD
#include <m_pd.h>
#else /* Max */
#include "ext.h"
#include "ext_obex.h"
#include "commonsyms.h"
/* some Pd specific types need definition in Max */
typedef long t_int;
typedef double t_float;
typedef double t_floatarg;
typedef void t_outlet;
typedef void t_clock;
#define getbytes(s) malloc(s)
#define freebytes(p, s) free(p)
#define MAXPDSTRING 512
#define pd_error(x, b) error(b)
#define SETSYMBOL SETSYM
#endif /* PD */


#define HIDIO_MAJOR_VERSION 0
#define HIDIO_MINOR_VERSION 1

/* static char *version = "$Revision: 1.20 $"; */

/*------------------------------------------------------------------------------
 * MACRO DEFINES
 */

#ifndef CLIP
#define CLIP(a, lo, hi) ( (a)>(lo)?( (a)<(hi)?(a):(hi) ):(lo) )
#endif /* NOT CLIP */

/*------------------------------------------------------------------------------
 * GLOBAL DEFINES
 */

#define DEFAULT_DELAY 5

/* this is set to simplify data structures (arrays instead of linked lists) */
#define MAX_DEVICES 128

/* I think 64 is the limit per device as defined in the OS <hans@at.or.at> */
#define MAX_ELEMENTS 64

/* this is limited so that the object doesn't cause a click getting too many
 * events from the OS's event queue.  On Mac OS X, this is set in on the
 * kernel level in HID_Utilities_External.h with the constant
 * kDeviceQueueSize */
#define MAX_EVENTS_PER_POLL 50


/* -----------------------------------------------------------------------------
 *  CLASS DEF */
typedef struct _hidio 
{
	t_object            x_obj;
#ifndef PD /* Max */
	void				*x_obex;
#endif 
#ifdef _WIN32
	void				*x_hid_device;
#endif 
#ifdef __linux__
	t_int               x_fd;
#endif 
	void                *x_ff_device;
	short               x_device_number;
	short               x_instance;
	t_int               x_has_ff;
	t_int               x_started;
	t_int               x_device_open;
	t_int               x_delay;
	t_clock             *x_clock;
	t_outlet            *x_data_outlet;
	t_outlet            *x_status_outlet;
} t_hidio;



/*------------------------------------------------------------------------------
 *  GLOBAL VARIABLES
 */

/* count the number of instances of this object so that certain free()
 * functions can be called only after the final instance is detroyed.
 */
extern t_int hidio_instance_count;

/* this is used to test for the first instance to execute */
extern double last_execute_time[MAX_DEVICES];

extern unsigned short global_debug_level;

/* built up when the elements of an open device are enumerated */
typedef struct _hid_element
{
#ifdef __linux__
    /* GNU/Linux store type and code to compare against */
    __u16 linux_type;
    __u16 linux_code;
#endif /* __linux__ */
#ifdef _WIN32
	/* this stores the UsagePage and UsageID */
	unsigned short usage_page;
	unsigned short usage_id;
#endif /* _WIN32 */
#ifdef __APPLE__
    void *pHIDElement;  /* pRecElement on Mac OS X */
#endif /* __APPLE__ */
    t_symbol *type; /* Linux "type"; HID "usagePage", but using hidio scheme */
    t_symbol *name; /* Linux "code"; HID "usage", but using hidio scheme */
    unsigned char polled; /* is it polled or queued? */
    unsigned char relative; /* relative data gets output everytime */
    t_int min; /* from device report */
    t_int max; /* from device report */
    t_float instance; /* usage page/usage instance # (e.g. [absolute x 2 163( */
	t_atom output_message[3]; /* pre-generated message for hidio_output_event */
    t_int value; /* output the sum of events in a poll for relative axes */
    t_int previous_value; /*only output on change on abs and buttons */
} t_hid_element;

/* mostly for status querying */
extern unsigned short device_count;

/* store element structs to eliminate symbol table lookups, etc. */
extern t_hid_element *element[MAX_DEVICES][MAX_ELEMENTS];
/* number of active elements per device */
extern unsigned short element_count[MAX_DEVICES]; 


/* Each instance registers itself with a hidio_instances[] linked list when it
 * opens a device.  Whichever instance gets the events from the OS will then
 * go thru this linked list and call its output function. */

/* basic element of a linked list of hidio instances */
typedef struct _hidio_instance
{
    t_hidio *x;
    struct _hidio_instance *x_next;
} t_hidio_instance;

/* array of linked lists of instances wanting events from a given device. */
extern t_hidio_instance *hidio_instances[MAX_DEVICES]; 


/*------------------------------------------------------------------------------
 *  FUNCTION PROTOTYPES FOR DIFFERENT PLATFORMS
 */

/* support functions */
void debug_post(t_int debug_level, const char *fmt, ...);
void debug_error(t_hidio *x, t_int debug_level, const char *fmt, ...);
void hidio_output_event(t_hidio *x, t_hid_element *output_data);


/* generic, cross-platform functions implemented in a separate file for each
 * platform 
 */
extern t_int hidio_open_device(t_hidio *x, short device_number);
extern t_int hidio_close_device(t_hidio *x);
extern void hidio_build_device_list(void);
extern void hidio_get_events(t_hidio *x);
extern void hidio_write_event_symbol_int(t_hidio *x, t_symbol *type, t_int code, 
                                           t_int instance, t_int value);
extern void hidio_write_event_symbols(t_hidio *x, t_symbol *type, t_symbol *code, 
                                      t_int instance, t_int value);
extern void hidio_write_event_ints(t_hidio *x, t_int type, t_int code, 
                                     t_int instance, t_int value);
extern void hidio_devices(t_hidio* x); /* print device list to the console */
extern void hidio_elements(t_hidio* x); /* print element list to the console */
extern void hidio_print(t_hidio* x); /* print info to the console */
extern void hidio_platform_specific_info(t_hidio *x); /* device info on the status outlet */
extern void hidio_platform_specific_free(t_hidio *x);
extern void *hidio_platform_specific_new(t_hidio *x);
extern short get_device_number_by_id(unsigned short vendor_id, unsigned short product_id);
/* TODO: this function should probably accept the single unsigned for the combined usage_page and usage, instead of two separate variables */
extern short get_device_number_from_usage(short device_number, 
										unsigned short usage_page, 
										unsigned short usage);

/*==============================================================================
 * event symbols array sizes
 *==============================================================================
 */

#define ABSOLUTE_ARRAY_MAX      16
#define BUTTON_ARRAY_MAX        128
#define KEY_ARRAY_MAX           256
#define LED_ARRAY_MAX           77
#define PID_ARRAY_MAX           256
#define RELATIVE_ARRAY_MAX      16


/*==============================================================================
 * symbol pointers for pre-generated event symbols
 *============================================================================*/

extern t_symbol *ps_absolute, *ps_button, *ps_key, *ps_led, *ps_pid, *ps_relative;

extern t_symbol *absolute_symbols[ABSOLUTE_ARRAY_MAX];
extern t_symbol *button_symbols[BUTTON_ARRAY_MAX];
extern t_symbol *key_symbols[KEY_ARRAY_MAX];
extern t_symbol *led_symbols[LED_ARRAY_MAX];
extern t_symbol *pid_symbols[PID_ARRAY_MAX];
extern t_symbol *relative_symbols[RELATIVE_ARRAY_MAX];

extern void generate_event_symbols();
extern void generate_type_symbols();


/*==============================================================================
 * macros for compiling for Max/MSP using the Pd API
 *============================================================================*/

#ifndef PD /* Max */
# define SETSYMBOL(atom, s)  atom_setsym(atom, s)
# define SETFLOAT(atom, f)   atom_setlong(atom, (long)f)
#endif /* NOT PD */


#endif  /* NOT _HIDIO_H */
