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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ph_common.h"

/*
 * taken from liblo lo_url_get_path by Steve Harris et al
 */
char *osc_get_valid_path(const char *url)
{
    char *path = malloc(strlen(url));

    if (sscanf(url, "osc://%*[^:]:%*[0-9]%s", path)) {
        return path;
    }
    if (sscanf(url, "osc.%*[^:]://%*[^:]:%*[0-9]%s", path) == 1) {
        return path;
    }
    if (sscanf(url, "osc.unix://%*[^/]%s", path) == 1) {
        return path;
    }
    if (sscanf(url, "osc.%*[^:]://%s", path)) {
        return path;
    }

    /* doesnt look like an OSC URL with port number and path*/
    return NULL;
}
/* end liblo code */

void handle_osc_debug(const char *path)
{

    ph_debug_post("got unhandled OSC message:\npath: <%s>\n", path);

}

void handle_osc_program(ph *x, t_atom *argv, unsigned int i)
{
    unsigned long bank;
    unsigned long program; 
    unsigned int n;
    bool found;
    ph_instance *instance;

    bank     = atom_getfloat(&argv[0]);
    program  = atom_getfloat(&argv[1]);
    instance = &x->instances[i];
    found    = false;

    ph_debug_post("%d programs", instance->plugin_pgm_count);

    for (n = 0; n < instance->plugin_pgm_count; ++n) {
        if (instance->plugin_pgms[n].Bank == bank &&
                instance->plugin_pgms[n].Program == program) {
            ph_debug_post("OSC: setting bank %u, program %u, name %s\n",
                    bank, program, instance->plugin_pgms[n].Name);
            found = true;
            break;
        }
    }

    if (!found) {
        pd_error(x, "UI requested unknown program: bank %lu, program %lu: "
                "sending to plugin anyway (plugin should ignore it)\n", 
                bank, program);
    }

    instance->pending_bank_msb   = bank / 128;
    instance->pending_bank_lsb   = bank % 128;
    instance->pending_pgm_change = program;

    ph_debug_post("bank = %d, program = %d, BankMSB = %d BankLSB = %d", 
            bank, program, instance->pending_bank_msb,
            instance->pending_bank_lsb);

    ph_program_change(x, i);

}

void handle_osc_control(ph *x, t_atom *argv, int i)
{
    int port;
    LADSPA_Data value; 
    ph_instance *instance;

    port     = (int)atom_getfloat(&argv[0]);
    value    = atom_getfloat(&argv[1]);
    instance = &x->instances[i];

    x->plugin_control_input[instance->plugin_port_ctlin_numbers[port]] = value;
    ph_debug_post("OSC: port %d = %f", port, value);

}

void handle_osc_midi(ph *x, t_atom *argv, unsigned int i)
{
    pd_error(x, "MIDI over OSC currently unsupported");
}

void handle_osc_configure(ph *x, t_atom *argv, int i)
{
    const char *key;
    const char *value;
    char *message;

    key   = atom_getsymbol(&argv[0])->s_name;
    value = atom_getsymbol(&argv[1])->s_name;

    ph_debug_post("%s()", __FUNCTION__);

    if (!x->descriptor->configure) {
        return;
    } 

    if (!strncmp(key, DSSI_RESERVED_CONFIGURE_PREFIX,
                strlen(DSSI_RESERVED_CONFIGURE_PREFIX))) {
        pd_error(x,"UI for plugin '' attempted to use reserved "
                "configure key \"%s\", ignoring", key);
        return;
    }

    message = x->descriptor->configure(x->instance_handles[i], key, value);

    if (message) {
        pd_error(x, "on configure '%s', plugin '' returned error '%s'",
                key, message);
        free(message);
    }

    ph_query_programs(x, i);

}

void handle_osc_exiting(ph *x, t_atom *argv, int i)
{

    ph_instance *instance;

    instance = &x->instances[i];

    free(instance->ui_osc_control_path);
    free(instance->ui_osc_configure_path);
    free(instance->ui_osc_hide_path);
    free(instance->ui_osc_program_path);
    free(instance->ui_osc_show_path); 
    free(instance->ui_osc_quit_path); 
    instance->ui_osc_control_path   = NULL;
    instance->ui_osc_configure_path = NULL;
    instance->ui_osc_hide_path      = NULL;
    instance->ui_osc_program_path   = NULL;
    instance->ui_osc_show_path      = NULL;
    instance->ui_osc_quit_path      = NULL;
    instance->ui_hidden             = true;

}

void handle_osc_update(ph *x, t_atom *argv, unsigned int i)
{
    const char *url; 
    const char *path;
    unsigned int n;
    unsigned int ac = 3;
    t_atom av[ac];
    ph_configure_pair *p;
    ph_instance *instance;

    instance = &x->instances[i];
    url      = atom_getsymbol(&argv[0])->s_name;
    p        = x->configure_buffer_head;

    ph_debug_post("OSC: got update request from <%s>, instance %d",
            url, instance);

    path = osc_get_valid_path(url);

    if(path == NULL) {
        pd_error(x, "invalid url: %s", url);
        return;
    }

    if (instance->ui_osc_control_path) {
        free(instance->ui_osc_control_path);
    }
    instance->ui_osc_control_path = malloc(strlen(path) + 10);
    sprintf(instance->ui_osc_control_path, "%s/control", path);

    if (instance->ui_osc_configure_path) {
        free(instance->ui_osc_configure_path);
    }
    instance->ui_osc_configure_path = malloc(strlen(path) + 12);
    sprintf(instance->ui_osc_configure_path, "%s/configure", path);

    if (instance->ui_osc_program_path) {
        free(instance->ui_osc_program_path);
    }
    instance->ui_osc_program_path = malloc(strlen(path) + 10);
    sprintf(instance->ui_osc_program_path, "%s/program", path);

    if (instance->ui_osc_quit_path) {
        free(instance->ui_osc_quit_path);
    }
    instance->ui_osc_quit_path = malloc(strlen(path) + 10);
    sprintf(instance->ui_osc_quit_path, "%s/quit", path);

    if (instance->ui_osc_show_path) {
        free(instance->ui_osc_show_path);
    }
    instance->ui_osc_show_path = malloc(strlen(path) + 10);
    sprintf(instance->ui_osc_show_path, "%s/show", path);

    if (instance->ui_osc_hide_path) {
        free(instance->ui_osc_hide_path);
    }
    instance->ui_osc_hide_path = (char *)malloc(strlen(path) + 10);
    sprintf(instance->ui_osc_hide_path, "%s/hide", path);

    free((char *)path);

    while(p){
        if(p->instance == i) {
            ph_send_configure(x, p->key, p->value, i);
        }
        p = p->next;
    }

    /* Send current bank/program */
    if (instance->pending_pgm_change >= 0) {
        ph_program_change(x, i);
    }

    ph_debug_post("pending_pgm_change = %d", instance->pending_pgm_change);

    if (instance->pending_pgm_change < 0) {
        unsigned long bank;
        unsigned long program;
        ac = 3;

        program = instance->current_pgm;
        bank    = instance->current_bank;
        instance->ui_needs_pgm_update = 0;

        SETSYMBOL(av, gensym(instance->ui_osc_program_path));
        SETFLOAT(av+1, bank);
        SETFLOAT(av+2, program);

        ph_instance_send_osc(x->message_out, instance, ac, av);

    }

    /* Send control ports */
    for (n = 0; n < x->plugin_control_ins; n++) {

        ac = 3;

        SETSYMBOL(av, gensym(instance->ui_osc_control_path));
        SETFLOAT(av+1, x->plugin_ctlin_port_numbers[n]);
        SETFLOAT(av+2, x->plugin_control_input[n]);

        ph_instance_send_osc(x->message_out, instance, ac, av);

        ph_debug_post("Port: %d, Default value: %.2f",
                x->plugin_ctlin_port_numbers[n], x->plugin_control_input[n]);

    }

    /* Send 'show' */
    if (instance->ui_show) {

        ac = 2;

        SETSYMBOL(av, gensym(instance->ui_osc_show_path));
        SETSYMBOL(av, gensym(""));

        ph_instance_send_osc(x->message_out, instance, ac, av);

        instance->ui_hidden = false;
        instance->ui_show = false;
    }
}

