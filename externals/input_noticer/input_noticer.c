/*
 * input_noticer - input noticer external for pure-data
 *
 * David Merrill <dmerrill@media.mit.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <libhal.h>
#include <stdio.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdlib.h>
#include <string.h>
#include "input_noticer.h"

static char *version = "$Revision: 1.2 $";
#define MAX_INPUT_DEVICES 32

/*------------------------------------------------------------------------------
 *  CLASS DEF
 */

typedef struct callback_info {
  void    (*device_added)(char *device_file);
  void    (*device_removed)(char *device_file);
} callback_info;

static t_class *input_noticer_class;

typedef struct _input_noticer {
	t_object		x_obj;
	GMainContext	*gmc;
	GMainLoop		*gml;
	callback_info	*cbi;
	LibHalContext	*lhc;
	DBusConnection	*connection;
	char			*capability;
	char			*product_substring;
	int				device_idx;
	t_outlet		*notify_out;
	char			*last_notification_sent;
	GThread			*gthread;
	int				started;
} t_input_noticer;

int udi_matches_device(LibHalContext *ctx, const char *udi);
static void output_list(t_input_noticer *x, t_symbol *s, int argc, t_atom *argv);

/*------------------------------------------------------------------------------
 * IMPLEMENTATION                    
 */

static void output_inputpath(t_input_noticer *x, int idx, char *path)
{
	t_atom t[2];

	// set up the output array
  	SETFLOAT(&(t[0]),idx); 
  	SETSYMBOL(&(t[1]),gensym(path));

	// output a list
  	outlet_list(x->notify_out, &s_list, 2, t);
}

int scmp(const void *sp1, const void *sp2 )
{
    return( strcmp(*(char **)sp1, *(char **)sp2) );
}

void scan_for_devices(LibHalContext *ctx) {
	char **		input_devices;
	int		num_input_devices;
	int		i,j;
	int		this_device_idx;
  	DBusError	dbus_error;
	char *linux_device_file = NULL;
	char *found_devices[MAX_INPUT_DEVICES];
	t_input_noticer *x = libhal_ctx_get_user_data(ctx);
    
    /// can't do anything here if we don't have a pointer back to our struct
    if (x == NULL) return;

	// not really using this, but why not initialize... :)
    dbus_error_init (&dbus_error);

	// grab an array of all devices that match the capability we're looking for
    input_devices = libhal_find_device_by_capability (ctx, "input", &num_input_devices, &dbus_error);
	if (dbus_error_is_set (&dbus_error))
	{
		post("could not find existing networking devices: %s\n", dbus_error.message);
		dbus_error_free (&dbus_error);
		return;		
	}

	if (input_devices) 
        {	
		// we found at least one
		this_device_idx = 0;
		for (i = 0; i < num_input_devices; i++)
		{
			if (udi_matches_device(ctx, input_devices[i])) 
			{
				post("found a %s",x->product_substring);

				// get the linux.device_file
				linux_device_file = libhal_device_get_property_string(ctx, input_devices[i],"linux.device_file", NULL);
				
				// store the linux device file
				found_devices[this_device_idx] = linux_device_file;

				this_device_idx++;
			}

		}
		// sort the devices alphabetically, so they will stay in the same order
		qsort (found_devices, this_device_idx, sizeof (char *), scmp);
		for (i = 0; i < this_device_idx; i++)
		{
			output_inputpath(x, i, found_devices[i]);
			libhal_free_string(found_devices[i]);
		}
		libhal_free_string_array(input_devices);
        } else {
		post("no input devices found!");
        }
}

// this function checks the UDI returned from libhal against the device product string
// that we are looking
int udi_matches_device(LibHalContext *ctx, const char *udi) {
	t_input_noticer *x = libhal_ctx_get_user_data(ctx);
	int i;
	char *capability = malloc (strlen("input") + strlen(x->capability) + 2);
	char *temp1;

	sprintf(capability,"input.%s",x->capability);
	temp1 = libhal_device_get_property_string (ctx, udi, "info.product", NULL);
	if (libhal_device_property_exists(ctx, udi, "info.capabilities", NULL)) 
	{
		char **capabilities = libhal_device_get_property_strlist(ctx, udi, "info.capabilities", NULL);
    		for (i=0; capabilities[i] != NULL; i++) 
		{
			//post("looking for %s, now checking capability #%d, %s",capability,i,capabilities[i]);
      			if (!strcmp (capabilities[i], capability)) 
			{
         			char *temp = libhal_device_get_property_string (ctx, udi, "info.product", NULL);
				if (temp != NULL && strstr(temp, x->product_substring)) // if product string matches up
				{
					libhal_free_string_array(capabilities);
					libhal_free_string (temp);
					free(capability);
					return 1;					
				} else {
					// product string does not match up
				}

				libhal_free_string (temp);
         		}
		}
		libhal_free_string_array(capabilities);
        } else {
        	// no capabilities found
        }
        free(capability);
	return 0;
}

// this callback gets called whenever HAL notices a new device being added
void hal_device_added(LibHalContext *ctx, const char *udi) {
  t_input_noticer *x = libhal_ctx_get_user_data(ctx);

  if (x != NULL && x->started) {
  	if (udi_matches_device(ctx,udi)) {
  		scan_for_devices(ctx);
  	} else {
        	// post("nope");
  	}
  }
}

void hal_device_removed(LibHalContext *ctx, const char *udi) {
  // post("device removed, udi = %s\n", udi);  
}

// I haven't seen this one get called... <Dm>
void hal_device_new_capability(LibHalContext *ctx, const char *udi, const char *capability) {
	// post("new device capability, udi = %s\n", udi);  
}

void input_noticer_stop(t_input_noticer* x) {
  DEBUG(post("input_noticer_stop"););

  /* Signal the HAL listener to stop */
  g_main_loop_quit(x->gml);

  /* Wait until it has actually stopped */
  g_thread_join(x->gthread);
}

static int input_noticer_close(t_input_noticer *x) {
	DEBUG(post("input_noticer_close"););
	
	input_noticer_stop(x);

   if (x->product_substring) free(x->product_substring);
   if (x->capability) free(x->capability);
   
   return 1;
}

static int input_noticer_open(t_input_noticer *x, t_symbol *s) {
  DEBUG(post("input_noticer_open");)

  // close it down, if running already
  input_noticer_close(x);
  
  return 1;
}

void input_noticer_start(t_input_noticer* x) {
  post("input_noticer: started");

  x->started = 1;
  
  // do first scan here (NOT in input_noticer_new, can't generate output from there)
  scan_for_devices(x->lhc);
}

gpointer input_noticer_thread_main(gpointer user_data) {
  t_input_noticer *x = (t_input_noticer *) user_data;
	
  /* Run the main loop.  We stay here until g_main_quit() is called */
  g_main_loop_run(x->gml);

  return user_data;
}

/* teardown functions */
static void input_noticer_free(t_input_noticer* x) {
  DEBUG(post("input_noticer_free");)
}

// removes double-quotes from a string, and returns a copy of it, otherwise unharmed
static char *remove_quotes(char *input_str)
{
	char *rv, *tp;
	unsigned int i;

	// post ("removing quotes from %s", input_str);
		
	if (input_str != NULL)
	{
		rv = malloc ((strlen(input_str) + 1) * sizeof(char));
		tp = rv;
		for (i=0; i < strlen(input_str); i++) {
			if (input_str[i] != '"') {
				*tp = input_str[i];
				tp++;
			}
		}
		*tp = '\0';
	} else {
		return NULL;
	}
	
	post ("returning %s", rv);
	return rv;
}

/* setup functions */
static void *input_noticer_new(t_symbol *capability, t_symbol *product_substring) {
  int i;
  t_input_noticer *x = (t_input_noticer *)pd_new(input_noticer_class);

  post("[input_noticer] %s, written by David Merrill <dmerrill@media.mit.edu>",version);  

  /* init vars */
  x->gmc = NULL;
  x->cbi = NULL;
  x->lhc = NULL;
  x->connection = NULL;
  x->notify_out = NULL;
  x->last_notification_sent = NULL;
  x->started = 0;
  x->capability = remove_quotes((char *)capability->s_name);
  x->product_substring = remove_quotes((char *)product_substring->s_name);

  // create outlet for notifying
  x->notify_out = outlet_new(&x->x_obj, 0); // list outlet

  // Setup glib and dbus for threading
  g_type_init();
  if (!g_thread_supported ())
    g_thread_init (NULL);
  dbus_g_thread_init();

  // create a context for the callback functions
  x->gmc = g_main_context_new();
  x->gml = g_main_loop_new(x->gmc, FALSE);

  // create a libhal context
  if ((x->lhc = libhal_ctx_new()) == NULL) {
    // complain here (exit)
    DEBUG(post("input_noticer_open: error, could not create a libhal context!");)
  }
  
  // get the dbus connection
  x->connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
  if (x->connection == NULL) {
    // complain here (exit)
    DEBUG(post("input_noticer_open: error, could not get the DBUS connection!");)
  }

  // attaches the main loop to dbus, so that the main loop
  // gets dbus events
  dbus_connection_setup_with_g_main(x->connection,x->gmc);

  // tells libhal to use our dbus connection, in order to receive
  // events from hal
  libhal_ctx_set_dbus_connection(x->lhc,x->connection);

  if (!libhal_ctx_init (x->lhc, NULL)) {
    DEBUG(post("input_noticer_open: error, could not init libhal!");)
  } 
  
  // handing my custom data structure to libhal context, so that the callback functions
  // can get to it
  libhal_ctx_set_user_data(x->lhc, x);

  libhal_ctx_set_device_added(x->lhc, hal_device_added);
  libhal_ctx_set_device_removed(x->lhc,  hal_device_removed);
  libhal_ctx_set_device_new_capability(x->lhc,  hal_device_new_capability);

  /* Create the thread that listens for HAL events */
  x->gthread = g_thread_create(input_noticer_thread_main, x, TRUE, NULL);
 
  return (void *)x;
}

void input_noticer_setup(void) {
  // DEBUG(post("input_noticer_setup");)

  // define how the object gets instantiated
  // example: [input_noticer joystick "SideWinder Dual Strike"]
  input_noticer_class = class_new(	
  			gensym("input_noticer"), 
			(t_newmethod)input_noticer_new, 
			(t_method)input_noticer_free,
			sizeof(t_input_noticer),
			CLASS_DEFAULT,
			A_DEFSYMBOL,
			A_DEFSYMBOL,
			0);

  class_addbang(input_noticer_class, input_noticer_start);
}
