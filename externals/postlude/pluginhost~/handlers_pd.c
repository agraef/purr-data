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
#include <stdlib.h>
#include <stdint.h>     /* for uint8_t      */

#include "jutils.h"
#include "handlers_osc.h"
#include "ph_common.h"

#define DX7_VOICE_SIZE_PACKED   128 /*From hexter_types.h by Sean Bolton */
#define DX7_DUMP_SIZE_BULK      4096+8
#define DX7_BANK_SIZE           32
#define DX7_MAX_PATCH_SIZE      16384
#define ASCII_t           116
#define ASCII_p           112
#define ASCII_n           110
#define ASCII_c           99
#define ASCII_b           98
#define ASCII_a           97
#define TYPE_STRING_SIZE  20
#define OSC_ADDR_MAX      8192

#if DEBUG == 1
#define CHECKSUM_PATCH_FILES_ON_LOAD 1
#endif

#define CLASS_NAME_STR "pluginhost~"


/*From dx7_voice.h by Sean Bolton */
typedef struct _dx7_patch_t {
    uint8_t data[128];
} dx7_patch_t;


/*From dx7_voice_data.c by Sean Bolton */
static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * dx7_bulk_dump_checksum
 ** Taken from dx7_voice_data.c by Sean Bolton **
 */
static int
dx7_bulk_dump_checksum(uint8_t *data, int length)
{
    int sum = 0;
    int i;

    for (i = 0; i < length; sum -= data[i++]);
    return sum & 0x7F;
}

/*
 * encode_7in6
 ** Taken from gui_data.c by Sean Bolton **
 *
 * encode a block of 7-bit data, in base64-ish style
 */
char *encode_7in6(uint8_t *data, int length)
{
    char *buffer;
    int in, reg, above, below, shift, out;
    int outchars = (length * 7 + 5) / 6;
    unsigned int sum = 0;

    if (!(buffer = (char *)malloc(25 + outchars)))
        return NULL;

    out = snprintf(buffer, 12, "%d ", length);

    in = reg = above = below = 0;
    while (outchars) {
        if (above == 6) {
            buffer[out] = base64[reg >> 7];
            reg &= 0x7f;
            above = 0;
            out++;
            outchars--;
        }
        if (below == 0) {
            if (in < length) {
                reg |= data[in] & 0x7f;
                sum += data[in];
            }
            below = 7;
            in++;
        }
        shift = 6 - above;
        if (below < shift) shift = below;
        reg <<= shift;
        above += shift;
        below -= shift;
    }

    snprintf(buffer + out, 12, " %d", sum);

    return buffer;
}



/* end hexter code */


static void ph_ladspa_description(ph *x, t_atom *at, 
        DSSI_Descriptor *psDescriptor){
    at[0].a_w.w_symbol = 
        gensym ((char*)psDescriptor->LADSPA_Plugin->Name); 
    outlet_anything (x->message_out, gensym ("name"), 1, at);
    at[0].a_w.w_symbol = 
        gensym ((char*)psDescriptor->LADSPA_Plugin->Label); 
    outlet_anything (x->message_out, gensym ("label"), 1, at);
    at[0].a_type = A_FLOAT;
    at[0].a_w.w_float = psDescriptor->LADSPA_Plugin->UniqueID; 
    outlet_anything (x->message_out, gensym ("id"), 1, at);
    at[0].a_type = A_SYMBOL;
    at[0].a_w.w_symbol =
        gensym ((char*)psDescriptor->LADSPA_Plugin->Maker);
    outlet_anything (x->message_out, gensym ("maker"), 1, at);
}

static void ph_midibuf_add(ph *x, int type, unsigned int chan, int param, int val)
{

    if(chan > x->n_instances - 1){
        pd_error(x, "note discarded: MIDI data is for a bogus channel");
        return;
    }

    t_int time_ref = x->time_ref;
    t_int mapped;

    //mapped = x->channel_map[chan + 1] - 1;
    /* FIX: get rid of mapping functionality */
    mapped = chan;

    x->midi_event_buf[x->buf_write_index].time.time.tv_sec = 
        (t_int)(clock_gettimesince(time_ref) * .001); 
    x->midi_event_buf[x->buf_write_index].time.time.tv_nsec = 
        (t_int)(clock_gettimesince(time_ref) * 1000); /*actually usec - we can't store this in nsec! */

    if ((type == SND_SEQ_EVENT_NOTEON && val != 0) || 
            type != SND_SEQ_EVENT_NOTEON) {
        x->midi_event_buf[x->buf_write_index].type = type;
        switch (type) {
            case SND_SEQ_EVENT_NOTEON:
                x->midi_event_buf[x->buf_write_index].data.note.channel = mapped;
                x->midi_event_buf[x->buf_write_index].data.note.note = param;
                x->midi_event_buf[x->buf_write_index].data.note.velocity = val;
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                x->midi_event_buf[x->buf_write_index].data.note.channel = mapped;
                x->midi_event_buf[x->buf_write_index].data.note.note = param;
                x->midi_event_buf[x->buf_write_index].data.note.velocity = val;
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                x->midi_event_buf[x->buf_write_index].data.control.channel = mapped;
                x->midi_event_buf[x->buf_write_index].data.control.param = param;
                x->midi_event_buf[x->buf_write_index].data.control.value = val;
                break;
            case SND_SEQ_EVENT_PITCHBEND:
                x->midi_event_buf[x->buf_write_index].data.control.channel = mapped;
                x->midi_event_buf[x->buf_write_index].data.control.param = 0;
                x->midi_event_buf[x->buf_write_index].data.control.value = val;
                break;
            case SND_SEQ_EVENT_CHANPRESS:
                x->midi_event_buf[x->buf_write_index].data.control.channel = mapped;
                x->midi_event_buf[x->buf_write_index].data.control.param = 0;
                x->midi_event_buf[x->buf_write_index].data.control.value = val;
                break;
            case SND_SEQ_EVENT_KEYPRESS:
                x->midi_event_buf[x->buf_write_index].data.note.channel = mapped;
                x->midi_event_buf[x->buf_write_index].data.note.note = param;
                x->midi_event_buf[x->buf_write_index].data.note.velocity = val;
                break;
            case SND_SEQ_EVENT_PGMCHANGE:
                x->instances[mapped].pending_bank_msb = (param - 1) / 128;
                x->instances[mapped].pending_bank_lsb = (param - 1) % 128;
                x->instances[mapped].pending_pgm_change = val;
                x->instances[mapped].ui_needs_pgm_update = 1; 
                ph_debug_post("pgm chabge received in buffer: MSB: %d, LSB %d, prog: %d",
                        x->instances[mapped].pending_bank_msb, x->instances[mapped].pending_bank_lsb, val);

                ph_program_change(x, mapped);
                break;
        }
    }
    else if (type == SND_SEQ_EVENT_NOTEON && val == 0) {
        x->midi_event_buf[x->buf_write_index].type = SND_SEQ_EVENT_NOTEOFF;
        x->midi_event_buf[x->buf_write_index].data.note.channel = mapped;
        x->midi_event_buf[x->buf_write_index].data.note.note = param;
        x->midi_event_buf[x->buf_write_index].data.note.velocity = val;
    }

    ph_debug_post("MIDI received in buffer: chan %d, param %d, val %d, mapped to %d",
            chan, param, val, mapped);

    x->buf_write_index = (x->buf_write_index + 1) % EVENT_BUFSIZE;
}

static void ph_set_control_input_by_index (ph *x,
        unsigned int ctrl_input_index, float value, unsigned int i)
{
    long port, portno;
    t_int argc = 3;
    t_atom argv[argc];
    ph_instance *instance;

    if (ctrl_input_index >= x->plugin_control_ins) {
        pd_error(x, "control port number %d is out of range [1, %d]",
                ctrl_input_index + 1, x->plugin_control_ins);
        return;
    }

    ph_debug_post("ctrl input number = %d", ctrl_input_index);

    port = x->plugin_ctlin_port_numbers[ctrl_input_index];

    instance = &x->instances[i];

    /* TODO - temporary hack */
    if(x->is_dssi) {
        portno = instance->plugin_port_ctlin_numbers[ctrl_input_index + 1];
    } else {
        portno = instance->plugin_port_ctlin_numbers[ctrl_input_index];
    }

    ph_debug_post("Global ctrl input number = %d", ctrl_input_index);
    ph_debug_post("Global ctrl input value = %.2f", value);

    /* set the appropriate control port value */
    x->plugin_control_input[portno] = value;

    /* Update the UI if there is one */
    if(!x->is_dssi){
        return;
    }

    if(instance->ui_osc_control_path == NULL){
        pd_error(x, "unable to send to NULL control path");
        return;
    }

    SETSYMBOL(argv, gensym(instance->ui_osc_control_path));
    SETFLOAT(argv+1, port);
    SETFLOAT(argv+2, value);
    ph_instance_send_osc(x->message_out, instance, argc, argv);

}

static unsigned ph_get_param_num (ph *x, const char *str)
/* find out if str points to a parameter number or not and return the
   number or zero.  The number string has to begin with a '#' character */
{
    long num = 0;
    char* strend = NULL;

    if (str == NULL) {
        return 0;
    }
    if (str[0] != '#') {
        return 0;
    }
    num = strtol (&str[1], &strend, 10);
    if (str[1] == 0 || *strend != 0) {
        /* invalid string */
        return 0;
    }
    else if (num >= 1 && num <= (long)x->plugin_control_ins) {
        /* string ok and within range */
        return (unsigned)num;
    }
    else {
        /* number out of range */
        return 0;
    }
}

static void ph_set_control_input_by_name (ph *x,
        const char* name,
        float value,
        unsigned int i)
{
    unsigned port_index = 0;
    unsigned ctrl_input_index = 0;
    int found_port = 0; /* boolean */

    if (name == NULL || strlen (name) == 0) {
        pd_error(x, "no control port name specified");
        return;
    }

    /* compare control name to LADSPA control input ports' names
       case-insensitively */
    found_port = 0;
    ctrl_input_index = 0;
    for (port_index = 0; port_index < x->descriptor->LADSPA_Plugin->PortCount; 
            port_index++)
    {
        LADSPA_PortDescriptor port_type;
        port_type = x->descriptor->LADSPA_Plugin->PortDescriptors[port_index];
        if (LADSPA_IS_PORT_CONTROL (port_type)
                && LADSPA_IS_PORT_INPUT (port_type))
        {
            const char* port_name = NULL;
            unsigned cmp_length = 0;
            port_name = x->descriptor->LADSPA_Plugin->PortNames[port_index];
            cmp_length = MIN (strlen(name), strlen(port_name));
            if (cmp_length != 0
                    && strncasecmp (name, port_name, cmp_length) == 0)
            {
                /* found the first port to match */
                found_port = 1;
                break;
            }
            ctrl_input_index++;
        }
    }

    if (!found_port) {
        pd_error(x, "plugin doesn't have a control input port named \"%s\"", 
                name);
        return;
    }

    ph_set_control_input_by_index (x, ctrl_input_index, value, i);

}

static void ph_ladspa_describe(const char * pcFullFilename, 
        void * pvPluginHandle,
        DSSI_Descriptor_Function fDescriptorFunction, 
        void* user_data,
        int is_dssi) 
{

    ph *x = (((void**)user_data)[0]);
    t_atom at[1];
    DSSI_Descriptor *psDescriptor;
    long lIndex;

    at[0].a_type = A_SYMBOL;
    at[0].a_w.w_symbol = gensym ((char*)pcFullFilename); 
    outlet_anything (x->message_out, gensym ("library"), 1, at);

    if(is_dssi){
        ph_debug_post("DSSI plugin found by listinfo");

        for (lIndex = 0;
                (psDescriptor = (DSSI_Descriptor *)
                 fDescriptorFunction(lIndex)) != NULL; lIndex++) 
            ph_ladspa_description(x, &at[0], psDescriptor);
    }

    else if(!is_dssi)
        lIndex = 0;
    do{
        psDescriptor = ladspa_to_dssi((LADSPA_Descriptor *)fDescriptorFunction(lIndex++));
        if(psDescriptor->LADSPA_Plugin != NULL){
            ph_ladspa_description(x, &at[0], psDescriptor);
            free((DSSI_Descriptor *)psDescriptor);
        }
        else
            break;
    } while(1);
}

static void ph_show(ph *x, unsigned int i, bool toggle)
{
            /* TODO:OSC */
/*
    if(instance->ui_target){
        if (instance->ui_hidden && toggle) {
             lo_send(instance->ui_target, 
                    instance->ui_osc_show_path, ""); 
            instance->ui_hidden = 0;
        }
        else if (!instance->ui_hidden && !toggle) {
                    instance->ui_osc_hide_path, ""); 
            instance->ui_hidden = 1;
        }
    }
    else if(toggle){
        instance->ui_show = 1;
        ph_load_gui(x, instance);

    }
    */
}

static t_int ph_configure_buffer(ph *x, char *key, 
        char *value, unsigned int i)
{

    ph_configure_pair *current;
    ph_configure_pair *p;
    ph_instance       *instance;

    instance = &x->instances[i];
    current  = x->configure_buffer_head;

    while(current){
        if(!strcmp(current->key, key) && current->instance == i) {
            break;
        }
        current = current->next;
    }
    if(current) {
        free(current->value);
    } else {
        current                  = malloc(sizeof(ph_configure_pair));
        current->next            = x->configure_buffer_head;
        current->key             = strdup(key);
        current->instance        = i;
        x->configure_buffer_head = current;
    }
    current->value = strdup(value);
    p = x->configure_buffer_head;

    /*TODO: eventually give ability to query this buffer (to outlet?) */
    while(p){
        ph_debug_post("key: %s", p->key);
        ph_debug_post("val: %s", p->value);
        ph_debug_post("instance: %d", p->instance);
        p = p->next;
    }

    return 0;
}

static t_int *ph_perform(t_int *w)
{
    unsigned int instance;
    unsigned int i;
    unsigned int N;
    int timediff;
    int framediff;
    t_float **inputs;
    t_float **outputs;
    ph *x;

    x       = (ph *)(w[1]);
    N       = (unsigned int)(w[2]);
    inputs  = (t_float **)(&w[3]);
    outputs = (t_float **)(&w[3] + x->plugin_ins);

    if(x->dsp){
        x->dsp_loop = true;

        for(i = 0; i < x->plugin_ins; i++)
            memcpy(x->plugin_input_buffers[i], inputs[i], N * 
                    sizeof(LADSPA_Data));

        for (i = 0; i < x->n_instances; i++)
            x->instance_event_counts[i] = 0;

        for (;x->buf_read_index != x->buf_write_index; x->buf_read_index = 
                (x->buf_read_index + 1) % EVENT_BUFSIZE) {

            instance = x->midi_event_buf[x->buf_read_index].data.note.channel;

            if(instance > x->n_instances){
                pd_error(x,
            "%s: %s: discarding spurious MIDI data, for instance %d", 
                        CLASS_NAME_STR, 
                        x->descriptor->LADSPA_Plugin->Label, 
                        instance);
                ph_debug_post("n_instances = %d", x->n_instances);

                continue;
            }

            if (x->instance_event_counts[instance] == EVENT_BUFSIZE){
                pd_error(x, "MIDI overflow on channel %d", instance);
                continue;
            }

            timediff = (int)(clock_gettimesince(x->time_ref) * 1000) - 
                x->midi_event_buf[x->buf_read_index].time.time.tv_nsec;
            framediff = (int)((t_float)timediff * .000001 / x->sr_inv); 

            if (framediff >= (int)N || framediff < 0) 
                x->midi_event_buf[x->buf_read_index].time.tick = 0;
            else
                x->midi_event_buf[x->buf_read_index].time.tick = 
                    N - framediff - 1;

            x->instance_event_buffers[instance]
                [x->instance_event_counts[instance]] = 
                x->midi_event_buf[x->buf_read_index];
            ph_debug_post("%s, note received on channel %d", 
                    x->descriptor->LADSPA_Plugin->Label, 
                    x->instance_event_buffers[instance]
                    [x->instance_event_counts[instance]].data.note.channel);

            x->instance_event_counts[instance]++; 

            ph_debug_post("Instance event count for instance %d of %d: %d\n",
                    instance + 1, x->n_instances, x->instance_event_counts[instance]);


        }

        i = 0;
        while(i < x->n_instances){
            if(x->instance_handles[i] && 
                    x->descriptor->run_multiple_synths){
                x->descriptor->run_multiple_synths
                    (x->n_instances, x->instance_handles, 
                     (unsigned long)N, x->instance_event_buffers,
                     &x->instance_event_counts[0]);
                break; 
            }
            else if (x->instance_handles[i] && 
                    x->descriptor->run_synth){
                x->descriptor->run_synth(x->instance_handles[i], 
                        (unsigned long)N, x->instance_event_buffers[i],
                        x->instance_event_counts[i]); 
                i++;
            }
            else if (x->instance_handles[i] && 
                    x->descriptor->LADSPA_Plugin->run){
                x->descriptor->LADSPA_Plugin->run
                    (x->instance_handles[i], N);
                i++;
            }
        }


        for(i = 0; i < x->plugin_outs; i++)
            memcpy(outputs[i], (t_float *)x->plugin_output_buffers[i], N * 
                    sizeof(LADSPA_Data));

        x->dsp_loop = false;
    } 
    return w + (x->plugin_ins + x->plugin_outs + 3);
}


/* ======================================== */

void handle_pd_bang(ph *x)
{
    t_atom at[3];

    at[0].a_type = A_FLOAT;
    at[1].a_type = A_SYMBOL;
    at[2].a_type = A_SYMBOL;

    if(x->plugin_label != NULL){
        at[0].a_w.w_float = x->n_instances;
        at[1].a_w.w_symbol = gensym ((char *)x->plugin_label); 
    }
    else{
        at[0].a_w.w_float = 0;
        at[1].a_w.w_symbol = gensym ("plugin"); 
    }	
    at[2].a_w.w_symbol = gensym ("instances"); 
    outlet_anything (x->message_out, gensym ("running"), 3, at);
}

void handle_pd_list(ph *x, t_symbol *s, int argc, t_atom *argv)
{
    char msg_type[TYPE_STRING_SIZE];
    int ev_type = 0;
    atom_string(argv, msg_type, TYPE_STRING_SIZE);
    int chan = (int)atom_getfloatarg(1, argc, argv) - 1;
    int param = (int)atom_getfloatarg(2, argc, argv);
    int val = (int)atom_getfloatarg(3, argc, argv);
    int n_instances = x->n_instances;

    switch (msg_type[0]){
        case ASCII_n: ev_type = SND_SEQ_EVENT_NOTEON;
                      break;
        case ASCII_c: ev_type = SND_SEQ_EVENT_CONTROLLER;
                      break;
        case ASCII_p: ev_type = SND_SEQ_EVENT_PGMCHANGE;
                      break;
        case ASCII_b: ev_type = SND_SEQ_EVENT_PITCHBEND;
                      break;
        case ASCII_t: ev_type = SND_SEQ_EVENT_CHANPRESS;
                      break;
        case ASCII_a: ev_type = SND_SEQ_EVENT_KEYPRESS;
                      break;
    }
    ph_debug_post("initial midi NOTE:, arg1 = %d, arg2 = %d, arg3 = %d, arg4 = %d",ev_type,chan,param,val);

    if(ev_type != 0) {
        if(chan >= 0) {
            ph_midibuf_add(x, ev_type, chan, param, val);
        } else {
            while(n_instances--) {
                ph_midibuf_add(x, ev_type, n_instances, param, val);
            }
        }
    }
}

void handle_pd_dsp(ph *x, t_signal **sp)
{
    if(!x->n_instances){
        return;
    }


    t_int *dsp_vector, i, M;

    M = x->plugin_ins + x->plugin_outs + 2;

    dsp_vector = (t_int *) getbytes(M * sizeof(t_int));

    dsp_vector[0] = (t_int)x;
    dsp_vector[1] = (t_int)sp[0]->s_n;

    for(i = 2; i < M; i++)
        dsp_vector[i] = (t_int)sp[i - 1]->s_vec;

    dsp_addv(ph_perform, M, dsp_vector);

}

void handle_pd_dssi(ph *x, t_symbol *s, int argc, t_atom *argv) 
{
    if (!x->is_dssi) {
        pd_error(x, "plugin is not a DSSI plugin, operation not supported");
    }

    char msg_type[TYPE_STRING_SIZE];
    char *debug;
    char *filename;
    char *filepath;
    char *key;
    char *value;
    char *temp;
    char mydir[MAXPDSTRING];
    int instance = -1;
    int pathlen;
    int toggle;
    int fd;
    int n_instances = x->n_instances;
    int count;
    int chan;
    int maxpatches;
    unsigned int i;
    t_float val;
    long filelength = 0;
    unsigned char *raw_patch_data = NULL;
    FILE *fp = NULL;
    size_t filename_length, key_size, value_size;
    dx7_patch_t patchbuf[DX7_BANK_SIZE];
    dx7_patch_t *firstpatch;
    atom_string(argv, msg_type, TYPE_STRING_SIZE);
    debug = NULL;
    key = NULL;	
    value = NULL;
    maxpatches = 128; 
    firstpatch = &patchbuf[0];
    val = 0;

    /*TODO: Temporary - at the moment we always load the first 32 patches to 0 */
    if(strcmp(msg_type, "configure")){
        instance = (int)atom_getfloatarg(2, argc, argv) - 1;

        if(!strcmp(msg_type, "load") && x->descriptor->configure){
            filename = argv[1].a_w.w_symbol->s_name;
            pd_error(x, "loading patch: %s for instance %d", 
                    filename, instance);

            if(!strcmp(x->descriptor->LADSPA_Plugin->Label, "hexter") || 
                    !strcmp(x->descriptor->LADSPA_Plugin->Label, "hexter6"))		{

                key = malloc(10 * sizeof(char)); /* holds "patchesN" */
                strcpy(key, "patches0");

                /* TODO: duplicates code from load_plugin() */
                fd = canvas_open(x->x_canvas, filename, "",
                        mydir, &filename, MAXPDSTRING, 0);

                if(fd >= 0){
                    filepath = mydir;
                    pathlen = strlen(mydir);
                    temp = &mydir[pathlen];
                    sprintf(temp, "/%s", filename);
                    fp = fopen(filepath, "rb");
                }
                else{
                    pd_error(x, "unable to get file descriptor");
                }

                /*From dx7_voice_data by Sean Bolton */
                if(fp == NULL){
                    pd_error(x, "unable to open patch file: %s", filename);
                    return;
                }
                if (fseek(fp, 0, SEEK_END) || 
                        (filelength = ftell(fp)) == -1 ||
                        fseek(fp, 0, SEEK_SET)) {
                    pd_error(x, "couldn't get length of patch file: %s",
                            filename);
                    fclose(fp);
                    return;
                }
                if (filelength == 0) {
                    pd_error(x, "patch file has zero length");
                    fclose(fp);
                    return;
                } else if (filelength > DX7_MAX_PATCH_SIZE) {
                    pd_error(x, "patch file is too large");
                    fclose(fp);
                    return;
                }
                if (!(raw_patch_data = (unsigned char *)malloc(filelength))) {
                    pd_error(x, "couldn't allocate memory for raw patch file");
                    fclose(fp);
                    return;
                }
                if (fread(raw_patch_data, 1, filelength, fp) 
                        != (size_t)filelength) {
                    pd_error(x, "short read on patch file: %s", filename);
                    free(raw_patch_data);
                    fclose(fp);
                    return;
                }
                fclose(fp);
                ph_debug_post("Patch file length is %ul", filelength);

                /* figure out what kind of file it is */
                filename_length = strlen(filename);
                if (filename_length > 4 &&
                        !strcmp(filename + filename_length - 4, ".dx7") &&
                        filelength % DX7_VOICE_SIZE_PACKED == 0) {  
                    /* It's a raw DX7 patch bank */
                    ph_debug_post("Raw DX7 format patch bank passed");

                    count = filelength / DX7_VOICE_SIZE_PACKED;
                    count = count > maxpatches ? maxpatches : count;

                    memcpy(firstpatch, raw_patch_data, count * 
                            DX7_VOICE_SIZE_PACKED);

                } else if (filelength > 6 &&
                        raw_patch_data[0] == 0xf0 &&
                        raw_patch_data[1] == 0x43 &&
                        /*This was used to fix some problem with Galaxy exports - possibly dump in worng format. It is not needed, but it did work, so in future, we may be able to support more formats not just DX7 */
                        /*   ((raw_patch_data[2] & 0xf0) == 0x00 || 
                             raw_patch_data[2] == 0x7e) &&*/
                        (raw_patch_data[2] & 0xf0) == 0x00 && 
                        raw_patch_data[3] == 0x09 &&
                        (raw_patch_data[4] == 0x10 || 
                         raw_patch_data[4] == 0x20) &&  
                        /* 0x10 is actual, 0x20 matches typo in manual */
                        raw_patch_data[5] == 0x00) {  
                    /* It's a DX7 sys-ex 32 voice dump */

                    ph_debug_post("SYSEX header check passed");


                    if (filelength != DX7_DUMP_SIZE_BULK ||
                            raw_patch_data[DX7_DUMP_SIZE_BULK - 1] != 0xf7) {
                        pd_error(x, "badly formatted DX7 32 voice dump!");
                        count = 0;

#ifdef CHECKSUM_PATCH_FILES_ON_LOAD
                    } else if (dx7_bulk_dump_checksum(&raw_patch_data[6],
                                DX7_VOICE_SIZE_PACKED * 32) !=
                            raw_patch_data[DX7_DUMP_SIZE_BULK - 2]) {

                        pd_error(x, "DX7 32 voice dump with bad checksum!");
                        count = 0; 
#endif


                    } else {

                        count = 32;
                        if (count > maxpatches)
                            count = maxpatches;
                        memcpy(firstpatch, raw_patch_data + 6, count * DX7_VOICE_SIZE_PACKED);

                    }
                } else {

                    /* unsuccessful load */
                    pd_error(x, "unknown patch bank file format!");
                    count = 0;

                }

                free(raw_patch_data);

                if(count == 32)
                    value = encode_7in6((uint8_t *)&patchbuf[0].data[0], 
                            count * DX7_VOICE_SIZE_PACKED);

            }
            else if(!strcmp(x->descriptor->LADSPA_Plugin->Label, 
                        "FluidSynth-DSSI")){
                key = malloc(6 * sizeof(char));
                strcpy(key, "load");
                value = filename;
            }
            else{
                pd_error(x, "%s patches are not supported", 
                        x->descriptor->LADSPA_Plugin->Label);
            }
        }

        if(!strcmp(msg_type, "dir") && x->descriptor->configure){
            pathlen = strlen(argv[1].a_w.w_symbol->s_name) + 2;
            x->project_dir = malloc((pathlen) * sizeof(char));
            atom_string(&argv[1], x->project_dir, pathlen);
            pd_error(x, "project directory for instance %d has been set to: %s", instance, x->project_dir);
            key = DSSI_PROJECT_DIRECTORY_KEY;
            value = x->project_dir;
        } else if (!strcmp(msg_type, "dir")) {
            pd_error(x, "%s: %s %s: operation not supported", 
                    CLASS_NAME_STR,
                    msg_type, 
                    argv[1].a_w.w_symbol->s_name);
        }

        if(!strcmp(msg_type, "show") || !strcmp(msg_type, "hide")){
            instance = (int)atom_getfloatarg(1, argc, argv) - 1;
            if(!strcmp(msg_type, "show"))
                toggle = 1;
            else
                toggle = 0;

            if(instance == -1){
                while(n_instances--)
                    ph_show(x, n_instances, toggle);
            }
            else
                ph_show(x, instance, toggle);
        }

        if(!strcmp(msg_type, "remap")) {
            /* remap channel to instance */
            for(i = 0; i < x->n_instances && i < 128; i++){
                chan = (int)atom_getfloatarg(1 + i, argc, argv);
                pd_error(x, "remapped MIDI channel %d to %d", 1+i, chan);
                x->channel_map[i+1] = chan;
            }
        }

    }

    /*Use this to send arbitrary configure message to plugin */
    else if(!strcmp(msg_type, "configure")){
        key = 
            (char *)malloc(key_size = (strlen(argv[1].a_w.w_symbol->s_name) + 2) * sizeof(char)); 
        atom_string(&argv[1], key, key_size);
        if(argc >= 3){	
            if (argv[2].a_type == A_FLOAT){
                val = atom_getfloatarg(2, argc, argv);
                value = (char *)malloc(TYPE_STRING_SIZE * 
                        sizeof(char));
                sprintf(value, "%.2f", val);
            }
            else if(argv[2].a_type == A_SYMBOL){
                value = 
                    (char *)malloc(value_size = 
                            (strlen(argv[2].a_w.w_symbol->s_name) + 2) * 
                            sizeof(char)); 
                atom_string(&argv[2], value, value_size);
            }		

        }	

        if(argc == 4 && argv[3].a_type == A_FLOAT)
            instance = atom_getfloatarg(3, argc, argv) - 1;
        else if (n_instances)
            instance = -1;
    }

    if(key != NULL && value != NULL){

        if(instance == -1){
            while(n_instances--){
                debug = ph_send_configure(x, key, value, n_instances);
                ph_configure_buffer(x, key, value, n_instances);
            }
        }
        /*TODO: Put some error checking in here to make sure instance is valid*/
        /* FIX: this is all a big hack (putting UI stuff in the host). Either
         * hexter needs to expose these settings somehow (e.g. as configure 
         * key-value pairs), or we need a [hexter_ui] external/patch that 
         * mirrors the functionality of hexter_gtk */
        else{
            if(!strcmp(key, "pitch_bend_range")){
                x->instances[instance].perf_buffer[3] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "portamento_time"))  {
                x->instances[instance].perf_buffer[5] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "mod_wheel_sensitivity"))  {
                x->instances[instance].perf_buffer[9] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "mod_wheel_assign"))  {
                x->instances[instance].perf_buffer[10] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "foot_sensitivity")) {
                x->instances[instance].perf_buffer[11] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "foot_assign")) {
                x->instances[instance].perf_buffer[12] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "pressure_sensitivity")) {
                x->instances[instance].perf_buffer[13] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "pressure_assign")) {
                x->instances[instance].perf_buffer[14] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "breath_sensitivity")) {
                x->instances[instance].perf_buffer[15] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else if (!strcmp(key, "breath_assign")) {
                x->instances[instance].perf_buffer[16] = atoi(value);
                char *p = encode_7in6(x->instances[instance].perf_buffer,
                        DX7_PERFORMANCE_SIZE);
                debug = ph_send_configure(x, "performance", p, instance);
            } else {
                debug = ph_send_configure(x, key, value, instance);
            }
            ph_configure_buffer(x, key, value, instance);
        }
    }
    ph_debug_post("The plugin returned %s", debug);

}

void handle_pd_control (ph *x, t_symbol* ctrl_name, t_float ctrl_value, 
        t_float instance)
/* Change the value of a named control port of the plug-in */
{
    unsigned param = 0;
    int i = instance - 1;
    unsigned int n = x->n_instances;

    if (i > (int)x->n_instances || i < -1){
        pd_error(x, "control: invalid instance number %d", i);
        return;
    }

    ph_debug_post("Received LADSPA control data for instance %d", i);

    if (ctrl_name->s_name == NULL || strlen (ctrl_name->s_name) == 0) {
        pd_error(x, "control messages must have a name and a value");
        return;
    }
    param = ph_get_param_num(x, ctrl_name->s_name);
    if (param) {
        if(i >= 0) {
            ph_set_control_input_by_index (x, param - 1, ctrl_value, i);
        } else if (i == -1) {
            while(n--) { 
                ph_set_control_input_by_index (x, param - 1, ctrl_value, n);
            }
        }
    } else if (i >= 0) {
        ph_set_control_input_by_name (x, ctrl_name->s_name, 
                ctrl_value, i);
    } else if (i == -1) {
        while(n--) {
            ph_set_control_input_by_name (x, ctrl_name->s_name, ctrl_value, n);
        }
    }
}

void handle_pd_info (ph *x)
{
    unsigned int i, 
                 ctrl_portno, 
                 audio_portno;
    t_atom argv[7];

    ctrl_portno = audio_portno = 0;

    if (x->descriptor == NULL)
        return;

    for(i = 0; i < x->descriptor->LADSPA_Plugin->PortCount; i++){
        memcpy(&argv[0], &x->port_info[i].type, 
                sizeof(t_atom));
        memcpy(&argv[1], &x->port_info[i].data_type, 
                sizeof(t_atom));
        memcpy(&argv[3], &x->port_info[i].name, 
                sizeof(t_atom));
        memcpy(&argv[4], &x->port_info[i].lower_bound, 
                sizeof(t_atom));
        memcpy(&argv[5], &x->port_info[i].upper_bound, 
                sizeof(t_atom));
        memcpy(&argv[6], &x->port_info[i].p_default, 
                sizeof(t_atom));
        argv[2].a_type = A_FLOAT;
        if(!strcmp(argv[1].a_w.w_symbol->s_name, "control"))
            argv[2].a_w.w_float = (t_float)++ctrl_portno;

        else if(!strcmp(argv[1].a_w.w_symbol->s_name, "audio"))
            argv[2].a_w.w_float = (t_float)++audio_portno;

        outlet_anything (x->message_out, gensym ("port"), 7, argv);
    }
}

void handle_pd_reset(ph *x, t_float i)
{
    unsigned int n;
    const LADSPA_Descriptor *ladspa;

    ladspa = x->descriptor->LADSPA_Plugin;

    for(n = 0; n < x->n_instances; n++) {
        if ((int)i == -1 || n == (int)i) {
            if (ladspa->deactivate && ladspa->activate){
                ladspa->deactivate(x->instance_handles[n]);
                ladspa->activate(x->instance_handles[n]);
            }
        }
    }
}

void handle_pd_listplugins (ph *x)
{
    void* user_data[1];
    user_data[0] = x;
    LADSPAPluginSearch(ph_ladspa_describe,(void*)user_data);
}

void handle_pd_osc(ph *x, t_symbol *s, int argc, t_atom *argv) 
{

    unsigned int i;
    const char *method;
    char path[OSC_ADDR_MAX];
    ph_instance *instance;

    instance = NULL;

    atom_string(argv, path, TYPE_STRING_SIZE);

    if (strncmp(path, "/dssi/", 6)){
        handle_osc_debug(path);
    }

    for (i = 0; i < x->n_instances; i++) {
        instance = &x->instances[i];
        if (!strncmp(path + 6, instance->osc_url_path,
                    strlen(instance->osc_url_path))) {
            break;
        }
    }

    if(instance == NULL) {
        pd_error(x, "instance not found");
        return;
    }

    if (!instance->osc_url_path){
        handle_osc_debug(path);
    }

    method = path + 6 + strlen(instance->osc_url_path);

    if (*method != '/' || *(method + 1) == 0){
        handle_osc_debug(path);
    }

    method++;

    switch(argc) {
        case 2:

            if (!strcmp(method, "configure") && 
                    argv[1].a_type == A_SYMBOL &&
                    argv[2].a_type == A_SYMBOL) {
                handle_osc_configure(x, &argv[1], i);
            } else if (!strcmp(method, "control") &&
                    argv[1].a_type == A_FLOAT &&
                    argv[2].a_type == A_FLOAT) {
                handle_osc_control(x, argv, i);
            } else if (!strcmp(method, "program") && 
                    argv[1].a_type == A_FLOAT &&
                    argv[2].a_type == A_FLOAT) {
                handle_osc_program(x, argv, i);
            }
            break;

        case 1:
            if (!strcmp(method, "midi")) {
                handle_osc_midi(x, argv, i);
            } else if (!strcmp(method, "update") &&
                    argv[1].a_type == A_SYMBOL) {
                handle_osc_update(x, argv, i);
            }
            break;
        case 0:
            if (!strcmp(method, "exiting")) {
                handle_osc_exiting(x, argv, i);
            }
            break;
        default:
            handle_osc_debug(path);
            break;
    }
}
