/* dssi~ - A DSSI host for PD 
 * 
 * Copyright 2006 Jamie Bullock and others 
 *
 * This file incorporates code from the following sources:
 * 
 * jack-dssi-host (BSD-style license): Copyright 2004 Chris Cannam, Steve Harris and Sean Bolton.
 *		   
 * Hexter (GPL license): Copyright (C) 2004 Sean Bolton and others.
 * 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "m_pd.h"
#include "dssi.h"
#include <dlfcn.h>
#include <lo/lo.h> 
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /*for exit()*/
#include <sys/types.h> /* for fork() */
#include <signal.h> /* for kill() */
#include <sys/wait.h> /* for wait() */
#include <dirent.h> /* for readdir() */

#define DX7_VOICE_SIZE_PACKED 	128 /*From hexter_types.h by Sean Bolton */
#define DX7_DUMP_SIZE_BULK 	4096+8


#define VERSION 0.99
#define EVENT_BUFSIZE 1024
#define OSC_BASE_MAX 1024
#define TYPE_STRING_SIZE 20 /* Max size of event type string (must be two more bytes than needed) */
#define DIR_STRING_SIZE 1024 /* Max size of directory string */
#define ASCII_n 110
#define ASCII_p 112
#define ASCII_c 99
#define ASCII_b 98
#define ASCII_t 116
#define ASCII_a 97

#define LOADGUI 0 /* FIX: depracate this */
#ifdef DEBUG
#define CHECKSUM_PATCH_FILES_ON_LOAD 1
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


/*From dx7_voice.h by Sean Bolton */

typedef struct _dx7_patch_t {
    uint8_t data[128];
} dx7_patch_t;

typedef struct _dssi_instance {

    long             currentBank;
    long             currentProgram;
    int              pendingBankLSB;
    int              pendingBankMSB;
    int              pendingProgramChange;

    int plugin_ProgramCount;
    DSSI_Program_Descriptor *pluginPrograms;

    lo_address       uiTarget; /*osc stuff */
    int              ui_hidden;
    int              ui_show;
    int              uiNeedsProgramUpdate;
    char            *ui_osc_control_path;
    char            *ui_osc_configure_path;
    char            *ui_osc_program_path;
    char            *ui_osc_show_path;
    char            *ui_osc_hide_path;
    char            *ui_osc_quit_path;

    int *plugin_PortControlInNumbers; /*not sure if this should go here?*/

    char *osc_url_path;
    pid_t	gui_pid;

} t_dssi_instance;

struct dssi_configure_pair {
    t_int instance;
    char *key,
         *value;
    struct dssi_configure_pair *next;
}; 

typedef struct dssi_configure_pair t_dssi_configure_pair;

typedef struct _port_info {
    t_atom type,
           data_type,
           name,
           lower_bound,
           upper_bound,
           p_default;
} t_port_info;

typedef struct _dssi_tilde {
    t_object  x_obj;
    t_int is_DSSI;
    char *plugin_label;
    char	     *plugin_full_path; /*absolute path to plugin */
    t_canvas *x_canvas; /* pointer to the canvas the object is instantiated on */
    void *plugin_handle;
    char *project_dir; /* project dircetory */
    LADSPA_Handle *instanceHandles; /*was handle*/
    t_dssi_instance *instances; 
    int n_instances;
    unsigned long *instanceEventCounts;
    unsigned char channelMap[128];
    snd_seq_event_t **instanceEventBuffers;

    snd_seq_event_t midiEventBuf[EVENT_BUFSIZE];
    /*static snd_seq_event_t **instanceEventBuffers;*/
    int bufWriteIndex, bufReadIndex;
    pthread_mutex_t midiEventBufferMutex;
    /*static pthread_mutex_t listHandlerMutex = PTHREAD_MUTEX_INITIALIZER;*/

    DSSI_Descriptor_Function desc_func;
    DSSI_Descriptor *descriptor;

    t_port_info *port_info;

    t_int	ports_in, ports_out, ports_controlIn, ports_controlOut;
    t_int plugin_ins;/* total audio input ports for plugin*/
    t_int plugin_outs;/* total audio output ports plugin*/
    t_int plugin_controlIns;/* total control input ports*/
    t_int plugin_controlOuts;/* total control output ports */

    unsigned long *plugin_ControlInPortNumbers; /*Array of input port numbers for the plugin */

    t_float **plugin_InputBuffers, **plugin_OutputBuffers; /* arrays of arrays for buffering audio for each audio port */
    t_float *plugin_ControlDataInput, *plugin_ControlDataOutput; /*arrays for control data for each port (1 item per 'run')*/
    lo_server_thread osc_thread;
    char *osc_url_base;
    char *plugin_basename;

    t_int time_ref; /*logical time reference */
    t_int sr;
    t_float sr_inv;
    t_int blksize;
    t_float f;

    t_outlet **outlets; 
    t_inlet **inlets;
    t_outlet *control_outlet;

    t_dssi_configure_pair *configure_buffer_head;

    t_int dsp; /* boolean dsp setting */
    t_int dsp_loop;

} t_dssi_tilde;

static char *dssi_tilde_send_configure(t_dssi_tilde *x, char *key, 
        char *value, t_int instance);
static int osc_message_handler(const char *path, const char *types, 
        lo_arg **argv, int argc, void *data, void *user_data);
static LADSPA_Data get_port_default(t_dssi_tilde *x, int port);
static void MIDIbuf(int type, int chan, int param, int val, t_dssi_tilde *x);


