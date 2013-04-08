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

 /*
 * This file may include code from the following sources under the terms
 * of the GNU GPL version 2:
 *
 * plugin~ Copyright (C) 2000 Jarno Seppänen, remIXed 2005
 * liblo 0.25 Copyright (C) 2004 Steve Harris
 *
 * This file may include code from jack-dssi-host under the terms of
 * the following license:
 *
 *  jack-dssi-host.c
 *
 *  Copyright 2004, 2009 Chris Cannam, Steve Harris and Sean Bolton.
 *·
 *  Permission to use, copy, modify, distribute, and sell this software
 *  for any purpose is hereby granted without fee, provided that the
 *  above copyright notice and this permission notice are included in
 *  all copies or substantial portions of the software.
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>     /* for exit()       */
#include <sys/types.h>  /* for fork()       */
#include <signal.h>     /* for kill()       */
#include <dirent.h>     /* for readdir()    */
#include <dlfcn.h>      /* for dlsym()      */
#include <assert.h>

#include "dssi.h"

#include "jutils.h"
#include "ph_common.h"

#define DEBUG_STRING_SIZE 8192

/*From dx7_voice_data.c */
uint8_t dx7_init_performance[DX7_PERFORMANCE_SIZE] = {    0,  0, 0, 2, 0,  0, 0,  0,
    0, 15, 1, 0, 4, 15, 2, 15,
    2,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0
};

static LADSPA_Data ph_get_port_default(ph *x, int port)
{
    LADSPA_Descriptor *plugin = (LADSPA_Descriptor *)x->descriptor->LADSPA_Plugin;
    LADSPA_PortRangeHint hint = plugin->PortRangeHints[port];
    float lower = hint.LowerBound *
        (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor) ? x->sr : 1.0f);
    float upper = hint.UpperBound *
        (LADSPA_IS_HINT_SAMPLE_RATE(hint.HintDescriptor) ? x->sr : 1.0f);

    if (!LADSPA_IS_HINT_HAS_DEFAULT(hint.HintDescriptor)) {
        if (!LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor) ||
                !LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor)) {
            /* No hint, its not bounded, wild guess */
            return 0.0f;
        }

        if (lower <= 0.0f && upper >= 0.0f) {
            /* It spans 0.0, 0.0 is often a good guess */
            return 0.0f;
        }

        /* No clues, return minimum */
        return lower;
    }

    /* Try all the easy ones */

    if (LADSPA_IS_HINT_DEFAULT_0(hint.HintDescriptor)) {
        return 0.0f;
    } else if (LADSPA_IS_HINT_DEFAULT_1(hint.HintDescriptor)) {
        return 1.0f;
    } else if (LADSPA_IS_HINT_DEFAULT_100(hint.HintDescriptor)) {
        return 100.0f;
    } else if (LADSPA_IS_HINT_DEFAULT_440(hint.HintDescriptor)) {
        return 440.0f;
    }

    /* All the others require some bounds */

    if (LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor)) {
        if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint.HintDescriptor)) {
            return lower;
        }
    }
    if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint.HintDescriptor)) {
        if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint.HintDescriptor)) {
            return upper;
        }
        if (LADSPA_IS_HINT_BOUNDED_BELOW(hint.HintDescriptor)) {
            if (LADSPA_IS_HINT_DEFAULT_LOW(hint.HintDescriptor)) {
                return lower * 0.75f + upper * 0.25f;
            } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint.HintDescriptor)) {
                return lower * 0.5f + upper * 0.5f;
            } else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint.HintDescriptor)) {
                return lower * 0.25f + upper * 0.75f;
            }
        }
    }

    /* fallback */
    return 0.0f;
}



static void ph_set_port_info(ph *x)
{
    t_int i;

    for (i = 0; i < (t_int)x->descriptor->LADSPA_Plugin->PortCount; i++) {

        x->port_info[i].type.a_type = A_SYMBOL;
        x->port_info[i].data_type.a_type = A_SYMBOL;
        x->port_info[i].name.a_type = A_SYMBOL;
        x->port_info[i].upper_bound.a_type = A_FLOAT;
        x->port_info[i].lower_bound.a_type = A_FLOAT;
        x->port_info[i].p_default.a_type = A_FLOAT;

        LADSPA_PortDescriptor pod =	
            x->descriptor->LADSPA_Plugin->PortDescriptors[i];
        ph_debug_post("Port %d: %s", i, x->descriptor->LADSPA_Plugin->PortNames[i]);

        if (LADSPA_IS_PORT_AUDIO(pod)) {
            x->port_info[i].data_type.a_w.w_symbol = 
                gensym("audio");
            if (LADSPA_IS_PORT_INPUT(pod)){
                x->port_info[i].type.a_w.w_symbol = 
                    gensym("in");
                ++x->plugin_ins;
            }
            else if (LADSPA_IS_PORT_OUTPUT(pod)){
                x->port_info[i].type.a_w.w_symbol = 
                    gensym("out");
                ++x->plugin_outs;
            }
        } 
        else if (LADSPA_IS_PORT_CONTROL(pod)) {
            x->port_info[i].data_type.a_w.w_symbol = 
                gensym("control");
            if (LADSPA_IS_PORT_INPUT(pod)){
                x->port_info[i].type.a_w.w_symbol = 
                    gensym("in");
                ++x->plugin_control_ins;
            }
            else if (LADSPA_IS_PORT_OUTPUT(pod)){
                ++x->plugin_control_outs;
                x->port_info[i].type.a_w.w_symbol = 
                    gensym("out");
            }
        }
        if (LADSPA_IS_HINT_BOUNDED_BELOW(
                    x->descriptor->LADSPA_Plugin->PortRangeHints[i].HintDescriptor))
            x->port_info[i].lower_bound.a_w.w_float = 
                x->descriptor->LADSPA_Plugin->
                PortRangeHints[i].LowerBound;
        else
            x->port_info[i].lower_bound.a_w.w_float = 0;

        if (LADSPA_IS_HINT_BOUNDED_ABOVE(
                    x->descriptor->LADSPA_Plugin->PortRangeHints[i].HintDescriptor))
            x->port_info[i].upper_bound.a_w.w_float = 
                x->descriptor->LADSPA_Plugin->
                PortRangeHints[i].UpperBound;
        else
            x->port_info[i].lower_bound.a_w.w_float = 1;

        x->port_info[i].p_default.a_w.w_float = (float)
            ph_get_port_default(x, i);

        x->port_info[i].name.a_w.w_symbol = 
            gensym ((char *)
                    x->descriptor->LADSPA_Plugin->PortNames[i]);
    }
    ph_debug_post("%d inputs, %d outputs, %d control inputs, %d control outs", x->plugin_ins, x->plugin_outs, x->plugin_control_ins, x->plugin_control_outs);

}

static void ph_assign_ports(ph *x)
{
    unsigned int i;

    ph_debug_post("%d instances", x->n_instances);


    x->plugin_ins *= x->n_instances;
    x->plugin_outs *= x->n_instances;
    x->plugin_control_ins *= x->n_instances;
    x->plugin_control_outs *= x->n_instances;

    ph_debug_post("%d plugin outs", x->plugin_outs);


    x->plugin_input_buffers = 
        (float **)malloc(x->plugin_ins * sizeof(float *));
    x->plugin_output_buffers = 
        (float **)malloc(x->plugin_outs * sizeof(float *));
    x->plugin_control_input = 
        (float *)calloc(x->plugin_control_ins, sizeof(float));
    x->plugin_control_output = 
        (float *)calloc(x->plugin_control_outs, sizeof(float));
    for(i = 0; i < x->plugin_ins; i++)
        x->plugin_input_buffers[i] = 
            (float *)calloc(x->blksize, sizeof(float));
    for(i = 0; i < x->plugin_outs; i++)
        x->plugin_output_buffers[i] = 
            (float *)calloc(x->blksize, sizeof(float));
    x->instance_event_buffers = 
        (snd_seq_event_t **)malloc(x->n_instances * sizeof(snd_seq_event_t *));

    x->instance_handles = (LADSPA_Handle *)malloc(x->n_instances *
            sizeof(LADSPA_Handle));
    x->instance_event_counts = (unsigned long *)malloc(x->n_instances *
            sizeof(unsigned long));

    for(i = 0; i < x->n_instances; i++){
        x->instance_event_buffers[i] = (snd_seq_event_t *)malloc(EVENT_BUFSIZE *
                sizeof(snd_seq_event_t));

        x->instances[i].plugin_port_ctlin_numbers = 
            (int *)malloc(x->descriptor->LADSPA_Plugin->PortCount * 
                    sizeof(int));
    }

    x->plugin_ctlin_port_numbers = 
        (unsigned long *)malloc(sizeof(unsigned long) * x->plugin_control_ins);

    ph_debug_post("Buffers assigned!");


}

static void ph_init_instance(ph *x, unsigned int i)
{

    x->instances[i].plugin_pgm_count = 0;
    x->instances[i].ui_needs_pgm_update = 0;
    x->instances[i].ui_osc_control_path = NULL;
    x->instances[i].ui_osc_configure_path = NULL;
    x->instances[i].ui_osc_program_path = NULL;
    x->instances[i].ui_osc_show_path = NULL;
    x->instances[i].ui_osc_hide_path = NULL;
    x->instances[i].ui_osc_quit_path = NULL;
    x->instances[i].osc_url_path = NULL;
    x->instances[i].current_bank = 0;
    x->instances[i].current_pgm = 0;
    x->instances[i].pending_pgm_change = -1;
    x->instances[i].pending_bank_lsb = -1;
    x->instances[i].pending_bank_msb = -1;
    x->instances[i].ui_hidden = 1;
    x->instances[i].ui_show = 0;
    memcpy(x->instances[i].perf_buffer, &dx7_init_performance, DX7_PERFORMANCE_SIZE);

    //x->instances[i].plugin_port_ctlin_numbers = NULL;
    x->instances[i].plugin_pgms = NULL;

    ph_debug_post("Instance %d initialized!", i);

}

static void ph_connect_ports(ph *x, unsigned int i)
{

    unsigned int n;

    for(n = 0; n < x->descriptor->LADSPA_Plugin->PortCount; n++){
        ph_debug_post("PortCount: %d of %d", n, 
                x->descriptor->LADSPA_Plugin->PortCount);

        LADSPA_PortDescriptor pod =
            x->descriptor->LADSPA_Plugin->PortDescriptors[n];

        x->instances[i].plugin_port_ctlin_numbers[n] = -1;

        if (LADSPA_IS_PORT_AUDIO(pod)) {
            if (LADSPA_IS_PORT_INPUT(pod)) {
                x->descriptor->LADSPA_Plugin->connect_port
                    (x->instance_handles[i], n, 
                     x->plugin_input_buffers[x->ports_in++]);
            } 
            else if (LADSPA_IS_PORT_OUTPUT(pod)) {
                x->descriptor->LADSPA_Plugin->connect_port
                    (x->instance_handles[i], n, 
                     x->plugin_output_buffers[x->ports_out++]);
                ph_debug_post("Audio Input port %d connected", x->ports_in);
                ph_debug_post("Audio Output port %d connected", x->ports_out);

            }
        } 
        else if (LADSPA_IS_PORT_CONTROL(pod)) {
            if (LADSPA_IS_PORT_INPUT(pod)) {
                x->plugin_ctlin_port_numbers[x->ports_control_in] = (unsigned long) i;
                x->instances[i].plugin_port_ctlin_numbers[n] = 
                    x->ports_control_in;
                x->plugin_control_input[x->ports_control_in] = 
                    (t_float) ph_get_port_default(x, n);
                ph_debug_post("default for port %d, control_in, %d is %.2f", n,
                        x->ports_control_in, 
                        x->plugin_control_input[x->ports_control_in]);


                x->descriptor->LADSPA_Plugin->connect_port
                    (x->instance_handles[i], n, 
                     &x->plugin_control_input[x->ports_control_in++]);

            } else if (LADSPA_IS_PORT_OUTPUT(pod)) {
                x->descriptor->LADSPA_Plugin->connect_port
                    (x->instance_handles[i], n, 
                     &x->plugin_control_output[x->ports_control_out++]);
            }
            ph_debug_post("control port %d connected", x->ports_control_in);
            ph_debug_post("control port %d connected", x->ports_control_out);

        }
    }
}

static void ph_activate_plugin(ph *x, unsigned int i)
{

    if(x->descriptor->LADSPA_Plugin->activate){
        ph_debug_post("trying to activate instance: %d", i);

        x->descriptor->LADSPA_Plugin->activate(x->instance_handles[i]);
    }
    ph_debug_post("plugin activated!");

}

static void ph_deactivate_plugin(ph *x, unsigned int instance)
{

    if(x->descriptor->LADSPA_Plugin->deactivate) {
        x->descriptor->LADSPA_Plugin->deactivate(x->instance_handles[instance]);
    }
    ph_debug_post("plugin deactivated!");

}

static void ph_cleanup_plugin(ph *x, unsigned int instance)
{
    if (x->descriptor->LADSPA_Plugin &&
            x->descriptor->LADSPA_Plugin->cleanup) {
        x->descriptor->LADSPA_Plugin->cleanup
            (x->instance_handles[instance]);
    }
}

static void ph_get_current_pgm(ph *x, unsigned int i)
{
    t_int argc = 3;
    t_atom argv[argc];
    ph_instance *instance;
    unsigned int pgm;

    instance = &x->instances[i];
    pgm      = instance->current_pgm;

    SETFLOAT(argv, i);
    SETFLOAT(argv+1, instance->plugin_pgms[pgm].Program);
    SETSYMBOL(argv+2, gensym(instance->plugin_pgms[pgm].Name));
    outlet_anything(x->message_out, gensym ("program"), argc, argv);

}

static void ph_init_programs(ph *x, unsigned int i)
{
    ph_instance *instance = &x->instances[i];
    ph_debug_post("Setting up program data");
    ph_query_programs(x, i);

    if (x->descriptor->select_program && instance->plugin_pgm_count > 0) {

        /* select program at index 0 */
        unsigned long bank            = instance->plugin_pgms[0].Bank;
        instance->pending_bank_msb    = bank / 128;
        instance->pending_bank_lsb    = bank % 128;
        instance->pending_pgm_change  = instance->plugin_pgms[0].Program;
        instance->ui_needs_pgm_update = 1;
    }
}

/* TODO:OSC */
#if 0
static void ph_load_gui(ph *x, int instance)
{
    t_int err = 0;
    char *gui_path;
    struct dirent *dir_entry = NULL;
    char *gui_base;
    size_t baselen;
    DIR *dp;
    char *gui_str;

    gui_base = (char *)malloc((baselen = sizeof(char) * (strlen(x->plugin_full_path) - strlen(".so"))) + 1);

    strncpy(gui_base, x->plugin_full_path, baselen);
    gui_base[baselen] = '\0';

    /* don't use strndup - GNU only */
    /*	gui_base = strndup(x->plugin_full_path, baselen);*/
    ph_debug_post("gui_base: %s", gui_base);


    gui_str = (char *)malloc(sizeof(char) * (strlen("channel 00") + 1));
    sprintf (gui_str,"channel %02d", instance);

    ph_debug_post("GUI name string, %s", gui_str);


    if(!(dp = opendir(gui_base))){
        post("pluginhost~: unable to find GUI in %s, continuing without...", gui_base);
        return;
    }
    else {
        while((dir_entry = readdir(dp))){
            if (dir_entry->d_name[0] == '.') continue;
            if (strchr(dir_entry->d_name, '_')){
                if (strstr(dir_entry->d_name, "gtk") ||
                        strstr(dir_entry->d_name, "qt") || 
                        strstr(dir_entry->d_name, "text"))
                    break;
            }
        }
        ph_debug_post("GUI filename: %s", dir_entry->d_name);

    }

    gui_path = (char *)malloc(sizeof(char) * (strlen(gui_base) + strlen("/") + 
                strlen(dir_entry->d_name) + 1));

    sprintf(gui_path, "%s/%s", gui_base, dir_entry->d_name);

    free(gui_base);
    ph_debug_post("gui_path: %s", gui_path);


    /* osc_url_base was of the form:
     * osc.udp://127.0.0.1:9997/dssi
     */
    osc_url = (char *)malloc
        (sizeof(char) * (strlen(x->osc_url_base) + 
                         strlen(instance->osc_url_path) + 2));

    sprintf(osc_url, "%s/%s", x->osc_url_base, 
            instance->osc_url_path);
    post("pluginhost~: instance %d URL: %s",instance, osc_url);
    ph_debug_post("Trying to open GUI!");


    instance->gui_pid = fork();
    if (instance->gui_pid == 0){
        err = execlp(gui_path, gui_path, osc_url, dir_entry->d_name, 
                x->descriptor->LADSPA_Plugin->Label, gui_str, NULL);
        perror("exec failed");
        exit(1); /* terminates the process */ 
    }

    ph_debug_post("errorcode = %d", err);


    free(gui_path);
    free(osc_url);
    free(gui_str);
    if(dp){

        ph_debug_post("directory handle closed = %d", closedir(dp));

    }
}
#endif

static t_int ph_configure_buffer_free(ph *x)
{
    ph_configure_pair *curr, *prev;
    prev = curr = NULL;

    for(curr = x->configure_buffer_head; curr != NULL; curr = curr->next){
        if(prev != NULL)
            free(prev);
        free(curr->key);
        free(curr->value);
        prev = curr;
    }
    free(curr);

    return 0;
}

static void ph_search_plugin_callback (
        const char* full_filename,
        void* plugin_handle,
        DSSI_Descriptor_Function descriptor_function,
        void* user_data,
        int is_dssi)
{
    DSSI_Descriptor* descriptor = NULL;
    unsigned plug_index = 0;

    char** out_lib_name = (char**)(((void**)user_data)[0]);
    char* name = (char*)(((void**)user_data)[1]);

    /* Stop searching when a first matching plugin is found */
    if (*out_lib_name == NULL)
    {
        ph_debug_post("pluginhost~: searching plugin \"%s\"...", full_filename);

        for(plug_index = 0;(is_dssi ? 
                    (descriptor = 
                     (DSSI_Descriptor *)descriptor_function(plug_index)) : 
                    ((DSSI_Descriptor *)(descriptor = 
                        ladspa_to_dssi((LADSPA_Descriptor *)
                            descriptor_function(plug_index)))->LADSPA_Plugin)) 
                != NULL; plug_index++){
            ph_debug_post("pluginhost~: label \"%s\"", descriptor->LADSPA_Plugin->Label);

            if (strcasecmp (name, descriptor->LADSPA_Plugin->Label) 
                    == 0)
            {
                *out_lib_name = strdup (full_filename);
                ph_debug_post("pluginhost~: found plugin \"%s\" in library \"%s\"",
                        name, full_filename);

                /*	if(!is_dssi){
                        free((DSSI_Descriptor *)descriptor);
                        descriptor = NULL;
                        }*/
                break;
            }
            /*	    if (descriptor != NULL){
                    free((DSSI_Descriptor *)descriptor);
                    descriptor = NULL;
                    }*/
        }
    }
}

static const char* plugin_tilde_search_plugin_by_label (ph *x,
        const char *name)
{
    char* lib_name = NULL;
    void* user_data[2];

    user_data[0] = (void*)(&lib_name);
    user_data[1] = (void*)name;
    ph_debug_post("search plugin by label: '%s'\n", name);


    lib_name = NULL;
    LADSPAPluginSearch (ph_search_plugin_callback,
            (void*)user_data);

    /* The callback (allocates and) writes lib_name, if it finds the plugin */
    return lib_name;

}

static void osc_setup(ph *x, unsigned int i)
{
    ph_instance *instance = &x->instances[i];

    if(i == 0){
        x->osc_port = OSC_PORT;
    }
    instance->osc_url_path = malloc(sizeof(char) * 
            (strlen(x->plugin_basename) + 
             strlen(x->descriptor->LADSPA_Plugin->Label) + 
             //strlen("chan00") + 3));
             6 + 3));
    sprintf(instance->osc_url_path, "%s/%s/chan%02d", x->plugin_basename, 
            x->descriptor->LADSPA_Plugin->Label, i); 
    ph_debug_post("OSC Path is: %s", instance->osc_url_path);

}

/* ==================================== */

void ph_debug_post(const char *fmt, ...)
{
#if DEBUG == 1
    unsigned int currpos;
    char     newfmt[DEBUG_STRING_SIZE];
    char     result[DEBUG_STRING_SIZE];
    size_t   fmt_length;
    va_list  args;

    fmt_length = strlen(fmt);

    sprintf(newfmt, "%s: ", PH_NAME);
    strncat(newfmt, fmt, fmt_length);
    currpos         = strlen(PH_NAME) + 2 + fmt_length;
    newfmt[currpos] = '\0';

    va_start(args, fmt);
    vsprintf(result, newfmt, args);
    va_end(args);

    post(result);
#endif
}

void ph_quit_plugin(ph *x)
{

    unsigned int i;
    t_atom argv[2];
    t_int argc;
    ph_instance *instance;

    argc = 2;

    for(i = 0; i < x->n_instances; i++) {
        instance = &x->instances[i];
         if(x->is_dssi){
            argc = 2;
            if(instance->ui_osc_quit_path != NULL) {
                SETSYMBOL(argv, gensym(instance->ui_osc_quit_path));  
                SETSYMBOL(argv+1, gensym(""));
                ph_instance_send_osc(x->message_out, instance, argc, argv);
            }
         }
         ph_deactivate_plugin(x, i);
         ph_cleanup_plugin(x, i);
    }
}

void ph_query_programs(ph *x, unsigned int i)
{
    unsigned int n;
    ph_instance *instance = &x->instances[i];
    ph_debug_post("querying programs");

    /* free old lot */
    if (instance->plugin_pgms != NULL) {
        for (n = 0; n < instance->plugin_pgm_count; n++) {
            free((void *)instance->plugin_pgms[n].Name);
        }
        free(instance->plugin_pgms);
        instance->plugin_pgms = NULL;
        instance->plugin_pgm_count = 0;
    }

    instance->pending_bank_lsb = -1;
    instance->pending_bank_msb = -1;
    instance->pending_pgm_change = -1;

    if (x->descriptor->get_program &&
            x->descriptor->select_program) {

        /* Count the plugins first */
        for (n = 0; x->descriptor->
                get_program(x->instance_handles[i], n); ++n);

        if (n > 0) {
            instance->plugin_pgm_count = n;
            instance->plugin_pgms = malloc(n * sizeof(DSSI_Program_Descriptor));
            while (n > 0) {
                const DSSI_Program_Descriptor *descriptor;
                --n;
                descriptor = x->descriptor->get_program(
                        x->instance_handles[i], n);
                instance->plugin_pgms[n].Bank = descriptor->Bank;
                instance->plugin_pgms[n].Program = descriptor->Program;
                instance->plugin_pgms[n].Name = strdup(descriptor->Name);
                ph_debug_post("program %d is MIDI bank %lu program %lu,"
                        " named '%s'",i,
                        instance->plugin_pgms[n].Bank,
                        instance->plugin_pgms[n].Program,
                        instance->plugin_pgms[n].Name);
            }
        } else {
            assert(instance->plugin_pgm_count == 0);
        }
    }
}

void ph_program_change(ph *x, unsigned int i)
{
    /* jack-dssi-host queues program changes by using  pending program change variables. In the audio callback, if a program change is received via MIDI it over writes the pending value (if any) set by the GUI. If unset, or processed the value will default back to -1. The following call to select_program is then made. I don't think it eventually needs to be done this way - i.e. do we need 'pending'? */ 
    ph_instance *instance;
    t_int argc = 3;
    t_atom argv[argc];

    instance = &x->instances[i];

    ph_debug_post("executing program change");

    if (instance->pending_pgm_change >= 0){
        if (instance->pending_bank_lsb >= 0) {
            if (instance->pending_bank_msb >= 0) {
                instance->current_bank =
                    instance->pending_bank_lsb + 128 * instance->pending_bank_msb;
            } else {
                instance->current_bank = instance->pending_bank_lsb + 
                    128 * (instance->current_bank / 128);
            }
        } else if (instance->pending_bank_msb >= 0) {
            instance->current_bank = 
                (instance->current_bank % 128) + 128 * instance->pending_bank_msb;
        }

        instance->current_pgm = instance->pending_pgm_change;

        if (x->descriptor->select_program) {
            x->descriptor->select_program(x->instance_handles[i],
                    instance->current_bank, instance->current_pgm);
        }
        if (instance->ui_needs_pgm_update){
            ph_debug_post("Updating GUI program");

            /* TODO - this is a hack to make text ui work*/
            if(x->is_dssi){
                // FIX: need to check this because if we don't have a UI,
                // update didn't get called
                if (false) {
                    SETSYMBOL(argv, gensym(instance->ui_osc_program_path));
                    SETFLOAT(argv+1, instance->current_bank);
                    SETFLOAT(argv+2, instance->current_pgm);
                    ph_instance_send_osc(x->message_out, instance, argc, argv);
                }
            }

        }
        instance->ui_needs_pgm_update = 0;
        instance->pending_pgm_change = -1;
        instance->pending_bank_msb = -1;
        instance->pending_bank_lsb = -1;
    }
    ph_get_current_pgm(x, i);
}

char *ph_send_configure(ph *x, const char *key, const char *value, 
        unsigned int i)
{

    char *debug;

    debug =   x->descriptor->configure(x->instance_handles[i], key, value);
    /* TODO:OSC */
    /* if(instance->ui_target != NULL && x->is_dssi) {
            lo_send(instance->ui_target, 
                instance->ui_osc_configure_path,
                "ss", key, value);
        }
                */
    ph_query_programs(x, i);

    return debug;
}

void ph_instance_send_osc(t_outlet *outlet, ph_instance *instance, 
        t_int argc, t_atom *argv)
{

    outlet_anything(outlet, gensym("connect"), UI_TARGET_ELEMS, 
            instance->ui_target);
    outlet_anything(outlet, gensym("send"), argc, argv);
    outlet_anything(outlet, gensym("disconnect"), 0, NULL);

}

void *ph_load_plugin(ph *x, t_int argc, t_atom *argv)
{
    char *plugin_basename = NULL,
         *plugin_full_path = NULL,
         *tmpstr,
         *plugin_label,
         plugin_dir[MAXPDSTRING];

    ph_debug_post("argc = %d", argc);

    unsigned int i;
    int stop;
    int fd;
    size_t pathlen;

    stop = 0;

    if (!argc){
        pd_error(x, "no arguments given, please supply a path");
        return x;
    }

    char *argstr = strdup(argv[0].a_w.w_symbol->s_name);

    if(strstr(argstr, ":") != NULL){
        tmpstr = strtok(argstr, ":");
        plugin_full_path = strdup(tmpstr);
        plugin_label = strtok(NULL, ":");
        // first part of the string is empty, i.e. ':mystring'
        if (plugin_label == NULL) {
            x->plugin_label = plugin_full_path;
            plugin_full_path = NULL;
        } else {
            x->plugin_label = strdup(plugin_label);
        }
    } else { 
        x->plugin_label = strdup(argstr);
        tmpstr = (char *)plugin_tilde_search_plugin_by_label(x, x->plugin_label);
        if(tmpstr) {
            plugin_full_path = strdup(tmpstr);
        }
    }

    free(argstr);
    ph_debug_post("plugin path = %s", plugin_full_path);
    ph_debug_post("plugin name = %s", x->plugin_label);

    if(plugin_full_path == NULL){
        pd_error(x, "can't get path to plugin");
        return x;
    }

    x->plugin_full_path = (char *)plugin_full_path;

    /* search for it in the 'canvas' path, which
     * includes the Pd search dirs and any 'extra' paths set with
     * [declare] */
    fd = canvas_open(x->x_canvas, plugin_full_path, "",
            plugin_dir, &plugin_basename, MAXPDSTRING, 0);

    if (fd >= 0) {
        ph_debug_post("plugin directory is %s, filename is %s", 
                plugin_dir, plugin_basename);

        x->plugin_basename = strdup(plugin_basename);
        pathlen = strlen(plugin_dir);
        tmpstr = &plugin_dir[pathlen];
        sprintf(tmpstr, "/%s", plugin_basename);
        tmpstr = plugin_dir;
        x->plugin_handle = loadLADSPAPluginLibrary(tmpstr);
    } else {
        /* try to load as is: this will work if plugin_full_path is an
         * absolute path, or the name of a library that is in DSSI_PATH
         * or LADSPA_PATH environment variables */
        x->plugin_handle = loadLADSPAPluginLibrary(plugin_full_path);
    }

    if (x->plugin_handle == NULL) {
        error("pluginhost~: can't find plugin in Pd paths, " 
                "try using [declare] to specify the path.");
        return x;
    }

    tmpstr = strdup(plugin_full_path);
    /* Don't bother working out the plugin name if we used canvas_open() 
     * to get the path */
    if(plugin_basename == NULL){
        if(!strstr(tmpstr, ".so")){
            pd_error(x, "invalid plugin path, must end in .so");
            return x;
        }
        plugin_basename = strtok((char *)tmpstr, "/");
        while(strstr(plugin_basename, ".so") == NULL) {
            plugin_basename = strtok(NULL, "/");
        }
        x->plugin_basename = strdup(plugin_basename);
        ph_debug_post("plugin basename = %s", x->plugin_basename);
    }
    free(tmpstr);
    if((x->desc_func = (DSSI_Descriptor_Function)dlsym(x->plugin_handle,
                    "dssi_descriptor"))){
        x->is_dssi = true;
        x->descriptor = (DSSI_Descriptor *)x->desc_func(0);
    }
    else if((x->desc_func = 
            (DSSI_Descriptor_Function)dlsym(x->plugin_handle,
                "ladspa_descriptor"))){
        x->is_dssi = false;
        x->descriptor = ladspa_to_dssi((LADSPA_Descriptor *)x->desc_func(0));
    }

    if(argc >= 2) {
        x->n_instances = (t_int)argv[1].a_w.w_float;
    } else {
        x->n_instances = 1;
    }

    ph_debug_post("n_instances = %d", x->n_instances);

    x->instances = (ph_instance *)malloc(sizeof(ph_instance) * 
            x->n_instances);

    if(!x->descriptor){
        pd_error(x, "error: couldn't get plugin descriptor");
        return x;
    }

    ph_debug_post("%s loaded successfully!", 
            x->descriptor->LADSPA_Plugin->Label);

    x->port_info = (ph_port_info *)malloc
        (x->descriptor->LADSPA_Plugin->PortCount * 
         sizeof(ph_port_info));

    ph_set_port_info(x);
    ph_assign_ports(x);

    for(i = 0; i < x->n_instances; i++){
        x->instance_handles[i] = 
            x->descriptor->LADSPA_Plugin->
            instantiate(x->descriptor->LADSPA_Plugin, x->sr);
        if (!x->instance_handles[i]){
            pd_error(x, "instantiation of instance %d failed", i);
            stop = 1;
            break;
        }
    }

    if(!stop){
        for(i = 0;i < x->n_instances; i++)
            ph_init_instance(x, i);
        for(i = 0;i < x->n_instances; i++)
            ph_connect_ports(x, i); 
        for(i = 0;i < x->n_instances; i++)
            ph_activate_plugin(x, i);

        if(x->is_dssi){
            for(i = 0;i < x->n_instances; i++)
                osc_setup(x, i);
#if LOADGUI
            for(i = 0;i < x->n_instances; i++)
                ph_load_gui(x, i);
#endif

            for(i = 0;i < x->n_instances; i++)
                ph_init_programs(x, i);

            for(i = 0; i < x->n_instances && i < 128; i++){
                x->channel_map[i] = i;
            }
        }
    }

    x->message_out = outlet_new (&x->x_obj, gensym("control"));

    if(x->plugin_outs){
        x->outlets = (t_outlet **)getbytes(x->plugin_outs * sizeof(t_outlet *)); 
        for(i = 0;i < x->plugin_outs; i++)
            x->outlets[i] = outlet_new(&x->x_obj, &s_signal);
    }
    else {
        pd_error(x, "plugin has no outputs");
    }
    if(x->plugin_ins){
        x->inlets = (t_inlet **)getbytes(x->plugin_ins * sizeof(t_inlet *)); 
        for(i = 0;i < x->plugin_ins; i++) {
            x->inlets[i] = inlet_new(&x->x_obj, &x->x_obj.ob_pd, 
                    &s_signal, &s_signal);
        }
    }
    else {
        pd_error(x, "plugin has no inputs");
    }

    x->dsp = true;
    post("%s: %d instances of %s, ready.", PH_NAME, x->n_instances, 
            x->plugin_label);

    return (void *)x;
}

void ph_init_plugin(ph *x)
{

    x->port_info                 = NULL;
    x->descriptor                = NULL;
    x->instance_event_counts     = NULL;
    x->instances                 = NULL;
    x->instance_handles          = NULL;
    x->osc_url_base              = NULL;
    x->configure_buffer_head     = NULL;
    x->project_dir               = NULL;
    x->outlets                   = NULL;
    x->inlets                    = NULL;
    x->message_out               = NULL;
    x->plugin_handle             = NULL;
    x->plugin_full_path          = NULL;
    x->plugin_label              = NULL;
    x->plugin_basename           = NULL;
    x->plugin_control_input      = NULL;
    x->plugin_control_output     = NULL;
    x->plugin_input_buffers      = NULL;
    x->plugin_output_buffers     = NULL;
    x->plugin_ctlin_port_numbers = NULL;
    x->plugin_ins                = 0;
    x->plugin_outs               = 0;
    x->plugin_control_ins        = 0;
    x->plugin_control_outs       = 0;
    x->is_dssi                   = 0;
    x->n_instances               = 0;
    x->dsp                       = 0;
    x->dsp_loop                  = 0;
    x->ports_in                  = 0;
    x->ports_out                 = 0;
    x->ports_control_in          = 0;
    x->ports_control_out         = 0;
    x->buf_write_index           = 0;
    x->buf_read_index            = 0;

}

void ph_free_plugin(ph *x)
{
    unsigned int i;
    unsigned int n;

    if(x->plugin_label != NULL) {
        free((char *)x->plugin_label);
    }

    if(x->plugin_handle == NULL) {
        return;
    }

    free((LADSPA_Handle)x->instance_handles);
    free(x->plugin_ctlin_port_numbers); 
    free((t_float *)x->plugin_input_buffers);
    free(x->instance_event_counts);
    free(x->plugin_control_input);
    free(x->plugin_control_output);

    i = x->n_instances;

    while(i--){
        ph_instance *instance = &x->instances[i];

        /* TODO:OSC */
        /*
           if(instance->gui_pid){
           ph_debug_post("Killing GUI process PID = %d", instance->gui_pid);

           kill(instance->gui_pid, SIGINT);
           } */
        if (instance->plugin_pgms) {
            for (n = 0; n < instance->plugin_pgm_count; n++) {
                free((void *)instance->plugin_pgms[n].Name);
            }
            free(instance->plugin_pgms);
            instance->plugin_pgms = NULL;
            instance->plugin_pgm_count = 0;
        }
        free(x->instance_event_buffers[i]);
        if(x->is_dssi){
            free(instance->ui_osc_control_path);
            free(instance->ui_osc_configure_path);
            free(instance->ui_osc_program_path);
            free(instance->ui_osc_show_path);
            free(instance->ui_osc_hide_path);
            free(instance->ui_osc_quit_path);
            free(instance->osc_url_path);
        }
        free(instance->plugin_port_ctlin_numbers);
        if(x->plugin_outs) {
            free(x->plugin_output_buffers[i]);
        }
    }
    if(x->is_dssi) {
        if(x->project_dir != NULL) {
            free(x->project_dir);
        }
        free(x->osc_url_base);
        ph_configure_buffer_free(x);
    }
    free((snd_seq_event_t *)x->instance_event_buffers);
    free(x->instances);
    free((t_float *)x->plugin_output_buffers);

    if(x->plugin_ins){
        for(n = 0; n < x->plugin_ins; n++) {
            inlet_free((t_inlet *)x->inlets[n]);
        }
        freebytes(x->inlets, x->plugin_ins * sizeof(t_inlet *));
    }

    if(x->plugin_outs){
        for(n = 0; n < x->plugin_outs; n++) {
            outlet_free((t_outlet *)x->outlets[n]);
        }
        freebytes(x->outlets, x->plugin_outs * sizeof(t_outlet *));
    }
    if(x->message_out) {
        outlet_free(x->message_out);
    }
    if(x->plugin_basename) {
        free(x->plugin_basename);
    }
    if(x->port_info) {
        free(x->port_info);
    }
}

DSSI_Descriptor *ladspa_to_dssi(LADSPA_Descriptor *ladspaDesc)
{
    DSSI_Descriptor *dssiDesc;
    dssiDesc = (DSSI_Descriptor *)calloc(1, sizeof(DSSI_Descriptor));
    ((DSSI_Descriptor *)dssiDesc)->DSSI_API_Version = 1;
    ((DSSI_Descriptor *)dssiDesc)->LADSPA_Plugin = 
        (LADSPA_Descriptor *)ladspaDesc;
    return (DSSI_Descriptor *)dssiDesc;
}
