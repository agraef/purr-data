#include <libhal.h>
#include <stdio.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <stdlib.h>

typedef struct callback_info {
  void    (*device_added)(char *device_file);
  void    (*device_removed)(char *device_file);
} callback_info;

// callback fns

// this one actually never gets called in my experience <DJM>
void hal_device_added(LibHalContext *ctx, const char *udi) {
  int i, found_joystick;
  char *linux_device_file = NULL;

  printf("device added, udi = %s\n", udi);

  // find out if this device advertises capabilities
  if (libhal_device_property_exists(ctx, udi, "info.capabilities", NULL)) {
    //printf("***** it's a joystick! *****\n");

    // get the capabilities strlist
    char **capabilities = libhal_device_get_property_strlist(ctx, udi, "info.capabilities", NULL);

    // find out if it's a joystick
    found_joystick = 0;
    for (i=0; capabilities[i] != NULL; i++) {
      if (!strcmp (capabilities[i], "input.joystick")) {
         char **linux_device_file_strlist;
         found_joystick = 1;

	// printf("found a joystick!\n");

         // pull out the relevant information (note - in Device Manager, this is reported incorrectly as a strlist,
	 // whereas it actually returns a string - so we make the correct call here
         linux_device_file = libhal_device_get_property_string(ctx, udi, "linux.device_file", NULL);

         if (linux_device_file != NULL) {
            // linux_device_file = linux_device_file_strlist[0];
            printf("found the joystick at: %s\n", linux_device_file);

         } else {
	    // we didn't find the device file, better luck next time

         }
      }
      // printf("got the following: %s\n", capabilities[i]);
    }
  }
}

void hal_device_removed(LibHalContext *ctx, const char *udi) {
  printf("device removed, udi = %s\n", udi);  
}

void hal_device_new_capability(LibHalContext *ctx, const char *udi, const char *capability) {
  char *device;
  callback_info *cbi = (callback_info *)libhal_ctx_get_user_data(ctx);

  printf("device has a new capability, udi = %s, cap = %s\n", udi, capability); 

  if (capability && ((strcmp (capability, "input") == 0))) {
	// 
	if (libhal_device_property_exists(ctx, udi, "input.device", NULL)) {
		device = libhal_device_get_property_string(ctx, udi, "input.device",NULL);

		// this is the callback into my PD C code
		// (*(cbi->device_added))(device);
		printf("new capability, testing: %s\n",device);
	}
  } 
}



gpointer hal_thread_main(gpointer user_data) {
  callback_info *cbi = (callback_info *)(user_data);
  GMainContext *gmc;
  GMainLoop *gml;
  LibHalContext *lhc;
  DBusConnection *connection;

  gmc = g_main_context_new();
  gml = g_main_loop_new(gmc,FALSE);

  if ((lhc = libhal_ctx_new()) == NULL) {
    // complain here (exit)
  }
  
  connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
  if (connection == NULL) {

  }

  // attaches the main loop to dbus, so tha the main loop
  // gets dbus events
  dbus_connection_setup_with_g_main(connection,gmc);

  // tells libhal to use our dbus connection, in order to receive
  // events from hal
  libhal_ctx_set_dbus_connection(lhc,connection);

  if (!libhal_ctx_init (lhc, NULL)) {
    // die
  } 
  
  // handing my custom data structure to libhal context
  libhal_ctx_set_user_data(lhc, cbi);

  libhal_ctx_set_device_added(lhc, hal_device_added);
  libhal_ctx_set_device_removed(lhc,  hal_device_removed);
  libhal_ctx_set_device_new_capability(lhc,  hal_device_new_capability);

  // Get stuck here forever, and ever, and ever....
  g_main_loop_run(gml);
}

GThread *hal_thread(callback_info *cbi) {
  GThread *rv;

  // eventually, we will pass in some user data here (the first NULL)
  // this could be a fn pointer, or a structure with a few fn pointers, 
  // etc, so that 
  rv = g_thread_create(hal_thread_main, cbi, TRUE, NULL);
  return(rv);
}

int main(int argc, char **argv) {
  GThread *gth;
  callback_info * cbi = malloc (sizeof (callback_info));
  
  // Setup glib
  g_type_init();
  if (!g_thread_supported ())
    g_thread_init (NULL);

  dbus_g_thread_init();

  gth = hal_thread(cbi);
  while (1) {
    sleep(1);
  }
}

