/* Gphoto PD External */
/* Copyright Ben Bogart, 2009/2010 */
/* This program is distributed under the params of the GNU Public License */

///////////////////////////////////////////////////////////////////////////////////
/* This file is part of the Gphoto PD External.                                 */
/*                                                                               */
/* Gphoto PD External is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License as published by          */
/* the Free Software Foundation; either version 2 of the License, or             */
/* (at your option) any later version.                                           */
/*                                                                               */
/* The Gphoto PD External is distributed in the hope that they will be useful,  */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 */
/* GNU General Public License for more details.                                  */
/*                                                                               */
/* You should have received a copy of the GNU General Public License             */
/* along with the Chaos PD Externals; if not, write to the Free Software         */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     */
///////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <m_pd.h>
#include <fcntl.h>
#include <pthread.h>
#include <gphoto2/gphoto2-camera.h>

t_class *gphoto_class;

typedef struct gphoto_struct {
	t_object x_obj;
	t_outlet *connectedOutlet;
	pthread_attr_t threadAttr; //thread attributes, added from DM2 version.
	Camera *camera;
	int connected;

} gphoto_struct;

// Struct to store A_GIMME data passed to thread.
typedef struct gphoto_gimme_struct {
	gphoto_struct *gphoto;
	t_symbol *s;
	int argc;
	t_atom *argv;
} gphoto_gimme_struct;

// Open connection to camera, do autodetection and initialization.
void *openCam(void *gphoto) {
	int gp_ret;

	gp_ret = gp_camera_new (&((gphoto_struct *)gphoto)->camera);
	if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); gp_camera_unref( ((gphoto_struct *)gphoto)->camera ); return(NULL);}
	if (gp_ret == -105) {error("gphoto: Are you sure the camera is supported, connected and powered on?"); gp_camera_unref( ((gphoto_struct *)gphoto)->camera); return(NULL);}
	post("gphoto: Autodetecting Camera.");

	// INIT camera (without context)	
	gp_ret = gp_camera_init (((gphoto_struct *)gphoto)->camera, NULL);
	if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); gp_camera_unref( ((gphoto_struct *)gphoto)->camera); return(NULL);}
	if (gp_ret == -105) {error("gphoto: Are you sure the camera is supported, connected and powered on?"); gp_camera_unref( ((gphoto_struct *)gphoto)->camera); return(NULL);}
	post("gphoto: Connected to Camera.");

	// Send state out 2nd outlet.
	sys_lock();
	outlet_float(((gphoto_struct *)gphoto)->connectedOutlet, 1);
	sys_unlock();

	// Set to connected state.
	((gphoto_struct *)gphoto)->connected = 1;

	return(NULL);	
}

// Wrap Open
static void wrapOpen(gphoto_struct *gphoto) {
	int ret;
	pthread_t thread1;

	// Create thread and pass reference to object struct
	ret = pthread_create( &thread1, &gphoto->threadAttr, openCam, gphoto);

	return;
}

// Close connection to camera (unref)
void *closeCam(void *gphoto) {
	int gp_ret;

	gp_camera_free(((gphoto_struct *)gphoto)->camera);

	// Send state out 2nd outlet.
	sys_lock();
	outlet_float(((gphoto_struct *)gphoto)->connectedOutlet, 0);
	sys_unlock();

	// Set to connected state.
	((gphoto_struct *)gphoto)->connected = 0;
	
	return(NULL);	
}

// Wrap Open
static void wrapClose(gphoto_struct *gphoto) {
	int ret;
	pthread_t thread1;

	// Create thread and pass reference to object struct
	ret = pthread_create( &thread1, &gphoto->threadAttr, closeCam, gphoto);

	return;
}

// TODO use listConfig to make a condigDetails function to get range and step data for cam params.

// list configuration
void *listConfig(void *threadArgs) {
	int gp_ret, numsections, numchildren, i, j;
	CameraWidget *config = NULL;
	CameraWidget *child = NULL;
	CameraWidget *child2 = NULL;
	CameraWidgetType type;
	const char *childName;

	gp_ret = gp_camera_get_config (((gphoto_gimme_struct *)threadArgs)->gphoto->camera, &config, NULL); // get config from camera
	if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	numsections = gp_widget_count_children(config);
	for (i=0; i<numsections; i++) {
		gp_widget_get_child (config, i, &child);
		gp_widget_get_type (child, &type);
	
		if (type == GP_WIDGET_SECTION) {
			//post("gphoto: Config Section: %s\n", childName);
			numchildren = gp_widget_count_children(child);
			for (j=0; j<numchildren; j++) {
				gp_widget_get_child (child, j, &child2);
				gp_widget_get_name (child2, &childName);
				gp_widget_get_type (child2, &type);	
				
				sys_lock();
				outlet_symbol(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet, gensym(childName));
				sys_unlock();

				//post("gphoto: Config Child: %s\n", childName); // send through outlet?
			}
		}
	}

	// Free memory (not child?)
	gp_widget_unref (config);

	// Send bang out 1st outlet when operation is done.
	sys_lock();
	outlet_bang(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet);
	sys_unlock();

	free(((gphoto_gimme_struct *)threadArgs)->argv); // suggested by Martin Peach, safe to free() from here when we alloc() from parent?

	return(NULL);
}

// Wrap listConfig
static void wrapListConfig(gphoto_struct *gphoto) {
	int ret;
	pthread_t thread1;

	if (gphoto->connected) {

		// instance of structure
		gphoto_gimme_struct *threadArgs = (gphoto_gimme_struct *)malloc(sizeof(gphoto_gimme_struct));

		// packaging arguments into structure
		threadArgs->gphoto = gphoto;

		// Create thread
		ret = pthread_create( &thread1, &gphoto->threadAttr, listConfig, threadArgs);
	} else {
		error("gphoto: ERROR: Not connected.");
	}

	return;
}

// Get configuration (flags) from camera
void *getConfig(void *threadArgs) {
	int gp_ret;
	const char *textVal;
	const int *toggleVal;
	const float *rangeVal;
	float value;

	t_symbol *key;

	CameraWidget *config = NULL;
	CameraWidget *child = NULL;
	CameraWidgetType type;

	key = atom_getsymbol( ((gphoto_gimme_struct *)threadArgs)->argv ); // config key

	gp_ret = gp_camera_get_config (((gphoto_gimme_struct *)threadArgs)->gphoto->camera, &config, NULL); // get config from camera
	if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	gp_ret = gp_widget_get_child_by_name (config, key->s_name, &child); // get item from config
	if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));}

/*		post("types:");
	post("GP_WIDGET_TOGGLE: %d",   GP_WIDGET_TOGGLE);
	post("GP_WIDGET_TEXT: %d",   GP_WIDGET_TEXT);
	post("GP_WIDGET_RANGE: %d",   GP_WIDGET_RANGE);
	post("GP_WIDGET_RADIO: %d",   GP_WIDGET_RADIO);
	post("GP_WIDGET_MENU: %d",   GP_WIDGET_MENU);
	post("GP_WIDGET_BUTTON: %d",   GP_WIDGET_BUTTON);
	post("GP_WIDGET_DATE: %d",   GP_WIDGET_DATE);
	post("GP_WIDGET_WINDOW: %d",   GP_WIDGET_WINDOW);
	post("GP_WIDGET_SECTION: %d",   GP_WIDGET_SECTION);
*/

	gp_ret = gp_widget_get_type (child, &type);
	if (gp_ret != 0) {
		error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));
		error("gphoto: Invalid config key.");
	} else {
		switch (type) {
			case GP_WIDGET_TOGGLE:
				gp_ret = gp_widget_get_value (child, &toggleVal); //  get widget value
				outlet_float(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet, (int) toggleVal);
				break;
			case GP_WIDGET_TEXT:
				gp_ret = gp_widget_get_value (child, &textVal);
				outlet_symbol(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet, gensym(textVal));
				break;
			case GP_WIDGET_RANGE:
				gp_ret = gp_widget_get_value (child, &rangeVal);
				if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));}
				outlet_float(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet, (float) *rangeVal);
				break;
		}
	}

	// Free memory (not child?)
	gp_widget_unref (config);

	// Send bang out 1st outlet when operation is done.
	sys_lock();
	outlet_bang(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet);
	sys_unlock();	

	free(((gphoto_gimme_struct *)threadArgs)->argv); // suggested by Martin Peach, safe to free() from here when we alloc() from parent?

	return(NULL);
}

// Wrap getConfig
static void wrapGetConfig(gphoto_struct *gphoto, t_symbol *s, int argc, t_atom *argv) {
	int ret;
	pthread_t thread1;

	if (gphoto->connected) {

		// instance of structure
		gphoto_gimme_struct *threadArgs = (gphoto_gimme_struct *)malloc(sizeof(gphoto_gimme_struct));

		// packaging arguments into structure
		threadArgs->gphoto = gphoto;
		threadArgs->s = s;
		threadArgs->argc = argc;
		threadArgs->argv = malloc(sizeof(*argv)*argc);		// allocate new memory space for arguments.
		memcpy(threadArgs->argv, argv, sizeof(*argv)*argc);	// copy the arguments into new space.

		// Create thread
		ret = pthread_create( &thread1, &gphoto->threadAttr, getConfig, threadArgs);
	} else {
		error("gphoto: ERROR: Not connected.");
	}

	return;
}

void *setConfig(void *threadArgs) {
	int gp_ret;
	float floatValue;
	t_symbol *key;
	t_int *intValue;
	CameraWidget *config = NULL;
	CameraWidget *child = NULL;
	CameraWidgetType type;

	sys_lock();
	key = atom_getsymbol( ((gphoto_gimme_struct *)threadArgs)->argv ); // config key
	sys_unlock();

	gp_ret = gp_camera_get_config (((gphoto_gimme_struct *)threadArgs)->gphoto->camera, &config, NULL); // get config from camera
	if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	gp_ret = gp_widget_get_child_by_name (config, key->s_name, &child); // get item from config
	if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	gp_ret = gp_widget_get_type (child, &type);
	if (gp_ret != 0) {
		error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));
		error("gphoto: Invalid config key.");
	} else {
		switch (type) {
			case GP_WIDGET_TOGGLE:
				sys_lock();
				intValue = atom_getint( ((gphoto_gimme_struct *)threadArgs)->argv+1);
				sys_unlock();

				gp_ret = gp_widget_set_value (child, &intValue); //  set widget value
				if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));}

				gp_ret = gp_camera_set_config (((gphoto_gimme_struct *)threadArgs)->gphoto->camera, config, NULL); // set new config
				if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));}
				break;

			case GP_WIDGET_RANGE:
				floatValue = atom_getfloat( ((gphoto_gimme_struct *)threadArgs)->argv+1 );

				gp_ret = gp_widget_set_value (child, &floatValue); //  set widget value
				if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));}

				gp_ret = gp_camera_set_config (((gphoto_gimme_struct *)threadArgs)->gphoto->camera, config, NULL); // set new config
				if (gp_ret != 0) {error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret));}

				break;
		}
	}

	// Free memory
	gp_widget_unref (config);
	
	// Send bang out 1st outlet when operation is done.
	sys_lock();
	outlet_bang(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet);
	sys_unlock();

	free(((gphoto_gimme_struct *)threadArgs)->argv); // suggested by Martin Peach, safe to free() from here when we alloc() from parent?

	return(NULL);
}

// Wrap setConfig
// TODO is there a way to have one wrapper for all funcs?
static void wrapSetConfig(gphoto_struct *gphoto,  t_symbol *s, int argc, t_atom *argv) {
	int ret;
	pthread_t thread1;

	if (gphoto->connected) {

		// instance of structure
		gphoto_gimme_struct *threadArgs = (gphoto_gimme_struct *)malloc(sizeof(gphoto_gimme_struct));

		// packaging arguments into structure
		threadArgs->gphoto = gphoto;
		threadArgs->s = s;
		threadArgs->argc = argc;
		threadArgs->argv = malloc(sizeof(*argv)*argc);		// allocate new memory space for arguments.
		memcpy(threadArgs->argv, argv, sizeof(*argv)*argc);	// copy the arguments into new space.

		// Create thread
		ret = pthread_create( &thread1, &gphoto->threadAttr, setConfig, threadArgs);
	} else {
		error("gphoto: ERROR: Not connected.");
	}

	return;
}

void *captureImage(void *threadArgs) {
	int gp_ret, fd;
	CameraFile *camerafile;
	CameraFilePath camera_file_path;
	t_symbol *filename;

	sys_lock(); 
	filename = atom_getsymbol( ((gphoto_gimme_struct *)threadArgs)->argv ); // destination filename
	sys_unlock(); 

	gp_ret = gp_camera_capture(((gphoto_gimme_struct *)threadArgs)->gphoto->camera, GP_CAPTURE_IMAGE, &camera_file_path, NULL); 
	if (gp_ret != 0) {sys_lock(); error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); sys_unlock(); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	fd = open( filename->s_name, O_CREAT | O_WRONLY, 0644); // create file descriptor

	gp_ret = gp_file_new_from_fd(&camerafile, fd); // create gphoto file from descriptor
	if (gp_ret != 0) {sys_lock(); error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); sys_unlock(); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	gp_ret = gp_camera_file_get(((gphoto_gimme_struct *)threadArgs)->gphoto->camera, camera_file_path.folder, camera_file_path.name,
		     GP_FILE_TYPE_NORMAL, camerafile, NULL); // get file from camera
	if (gp_ret != 0) {sys_lock(); error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); sys_unlock(); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	gp_file_unref(camerafile); // clear camerafile
	
	gp_ret = gp_camera_file_delete(((gphoto_gimme_struct *)threadArgs)->gphoto->camera, camera_file_path.folder, camera_file_path.name, NULL);
	if (gp_ret != 0) {sys_lock(); error("gphoto: ERROR: %s\n", gp_result_as_string(gp_ret)); sys_unlock(); gp_camera_unref(((gphoto_gimme_struct *)threadArgs)->gphoto->camera); return(NULL);}

	//close(fd); // close file descriptor # gphoto devs say this is uneeded.
	
	// Send bang out 2nd outlet when operation is done.
	sys_lock();
	outlet_bang(((gphoto_gimme_struct *)threadArgs)->gphoto->x_obj.ob_outlet);
	sys_unlock();

	free(((gphoto_gimme_struct *)threadArgs)->argv); // suggested by Martin Peach, safe to free() from here when we alloc() from parent?

	return(NULL);
}

// Wrap captureImage
// TODO is there a way to have one wrapper for all funcs?
static void wrapCaptureImage(gphoto_struct *gphoto,  t_symbol *s, int argc, t_atom *argv) {
	int ret;
	pthread_t thread1;

	if (gphoto->connected) {

		// instance of structure
		gphoto_gimme_struct *threadArgs = (gphoto_gimme_struct *)malloc(sizeof(gphoto_gimme_struct));

		// packaging arguments into structure
		threadArgs->gphoto = gphoto;
		threadArgs->s = s;
		threadArgs->argc = argc;
		threadArgs->argv = malloc(sizeof(*argv)*argc);		// allocate new memory space for arguments.
		memcpy(threadArgs->argv, argv, sizeof(*argv)*argc);	// copy the arguments into new space.

		// Create thread
		ret = pthread_create( &thread1, &gphoto->threadAttr, captureImage, threadArgs);
	} else {
		error("gphoto: ERROR: Not connected.");
	}

	return;
}

static void *gphoto_new(void) {
	gphoto_struct *gphoto = (gphoto_struct *) pd_new(gphoto_class);
	outlet_new(&gphoto->x_obj, NULL);
	gphoto->connectedOutlet = outlet_new(&gphoto->x_obj, &s_float);

	// When we create a thread, make sure it is deatched, from DM2 version.
	pthread_attr_init(&gphoto->threadAttr);
	pthread_attr_setdetachstate(&gphoto->threadAttr, PTHREAD_CREATE_DETACHED);

	// Initially the external is not "connected"
	gphoto->connected = 0;
	
	return (void *)gphoto;
}

// Destructor to cleanup camera if it's still open.
static void *gphoto_free(gphoto_struct *gphoto) {
	if (!gphoto->connected) {
		gp_camera_free(gphoto->camera);
	}
}
	

void gphoto_setup(void) {
	gphoto_class = class_new(gensym("gphoto"), (t_newmethod) gphoto_new, 0, sizeof(gphoto_struct), 0, CLASS_DEFAULT, 0);
	class_addmethod(gphoto_class, (t_method) wrapOpen, gensym("open"), 0);
	class_addmethod(gphoto_class, (t_method) wrapClose, gensym("close"), 0);
	class_addmethod(gphoto_class, (t_method) wrapGetConfig, gensym("getconfig"), A_GIMME, 0);
	class_addmethod(gphoto_class, (t_method) wrapSetConfig, gensym("setconfig"), A_GIMME, 0);
	class_addmethod(gphoto_class, (t_method) wrapCaptureImage, gensym("capture"), A_GIMME, 0);
	class_addmethod(gphoto_class, (t_method) wrapListConfig, gensym("listconfig"), 0);

	post("Gphoto: SVN version w/ gp_file_free()");
}

