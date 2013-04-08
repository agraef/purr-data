/* pluginhost~ - A plugin host for Pd
 *
 * Copyright (C) 2012 Jamie Bullock and others
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <stdbool.h>
#include <stdint.h>

#include "m_pd.h"
#include "dssi.h"

#define PH_NAME           "pluginhost~"
#define PH_VERSION        1.0
#define EVENT_BUFSIZE     1024
#define OSC_PORT          9998
#define UI_TARGET_ELEMS   2

/*From hexter_types.h by Sean Bolton */
#define DX7_PERFORMANCE_SIZE 64

#define MIN(a,b) ((a)<(b)?(a):(b))

#ifndef HEADER_PH_COMMON

typedef struct _ph_instance {

    unsigned int     plugin_pgm_count;
    bool             ui_needs_pgm_update;
    char            *ui_osc_control_path;
    char            *ui_osc_configure_path;
    char            *ui_osc_program_path;
    char            *ui_osc_show_path;
    char            *ui_osc_hide_path;
    char            *ui_osc_quit_path;
    char            *osc_url_path;
    long             current_bank;
    long             current_pgm;
    int              pending_pgm_change;
    int              pending_bank_lsb;
    int              pending_bank_msb;
    int              ui_hidden;
    int              ui_show;
    t_atom           ui_target[UI_TARGET_ELEMS]; /* host, port */
    uint8_t          perf_buffer[DX7_PERFORMANCE_SIZE];


    int *plugin_port_ctlin_numbers; /*not sure if this should go here?*/
    DSSI_Program_Descriptor *plugin_pgms;

} ph_instance;

typedef struct ph_configure_pair {

    struct ph_configure_pair *next;
    unsigned int instance;
    char   *value;
    char   *key;

} ph_configure_pair;

typedef struct _port_info {

    t_atom lower_bound;
    t_atom upper_bound;
    t_atom data_type;
    t_atom p_default;
    t_atom type;
    t_atom name;

} ph_port_info;

typedef struct _ph {

    t_object x_obj; /* gah, this has to be first element in the struct, WTF? */

    int sr;
    int blksize;
    int time_ref;
    int ports_in;
    int ports_out;
    int ports_control_in;
    int ports_control_out;
    int buf_write_index;
    int buf_read_index;

    bool is_dssi;
    bool dsp;
    bool dsp_loop;

    char *plugin_basename;
    char *plugin_label;
    char *plugin_full_path;
    char *project_dir;
    void *plugin_handle;
    char *osc_url_base;

    float f;
    float sr_inv;
    float **plugin_input_buffers;
    float **plugin_output_buffers;
    float *plugin_control_input;
    float *plugin_control_output;

    unsigned int osc_port;
    unsigned int n_instances;
    unsigned int plugin_ins;
    unsigned int plugin_outs;
    unsigned int plugin_control_ins;
    unsigned int plugin_control_outs;
    unsigned long *instance_event_counts;
    unsigned long *plugin_ctlin_port_numbers;
    unsigned char channel_map[128];

    DSSI_Descriptor_Function desc_func;
    DSSI_Descriptor *descriptor;
    LADSPA_Handle *instance_handles;

    t_inlet  **inlets;
    t_outlet **outlets;
    t_outlet *message_out;
    t_canvas *x_canvas;

    ph_port_info *port_info;
    ph_instance *instances;
    ph_configure_pair *configure_buffer_head;

    snd_seq_event_t **instance_event_buffers;
    snd_seq_event_t midi_event_buf[EVENT_BUFSIZE];

} ph;

void ph_debug_post(const char *fmt, ...);
void ph_quit_plugin(ph *x);
void ph_init_plugin(ph *x);
void ph_free_plugin(ph *x);
void ph_query_programs(ph *x, unsigned int i);
void ph_program_change(ph *x, unsigned int i);
void ph_instance_send_osc(t_outlet *outlet, ph_instance *instance, 
        t_int argc, t_atom *argv);
void *ph_load_plugin(ph *x, t_int argc, t_atom *argv);
char *ph_send_configure(ph *x, const char *key, const char *value,
        unsigned int i);
DSSI_Descriptor *ladspa_to_dssi(LADSPA_Descriptor *ladspaDesc);

#define HEADER_PH_COMMON
#endif

