/* plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2000 Jarno Seppänen
   $Id: plugin~_ladspa.h,v 1.1 2004-01-08 14:55:24 ksvalast Exp $

   This file is part of plugin~.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

#ifndef __PLUGIN_TILDE_LADSPA_H__
#define __PLUGIN_TILDE_LADSPA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "config.h"

#if PLUGIN_TILDE_USE_LADSPA

#include "plugin~.h"

/* LADSPA header */
#include "ladspa/ladspa.h"

#if 0 /* moved to plugin~.h because of cyclical header dependency */
typedef struct
{
    const LADSPA_Descriptor*	type;
    LADSPA_Handle*	instance;

    /* Memory to pass async control data to/from the plugin */
    float*		control_input_values;
    float*		control_output_values;
    /* Used for monitoring changes in the values */
    float*		prev_control_output_values;
    int			prev_control_output_values_invalid;

    /* Pointers to signal memory for out-of-place processing */
    float**		outofplace_audio_outputs;
    float**		actual_audio_outputs;	/* real audio outputs for out-of-place */

    unsigned long	num_samples;
    unsigned long	sample_rate;

} Plugin_Tilde_Ladspa;
#endif /* moved to plugin~.h because of cyclical header dependency */


/* subroutines to wrap the LADSPA interface */
const char*	plugin_tilde_ladspa_search_plugin (Pd_Plugin_Tilde* x,
						   const char* name);
int	plugin_tilde_ladspa_open_plugin (Pd_Plugin_Tilde* x,
					 const char* name,
					 const char* lib_name,
					 unsigned long sample_rate);
void	plugin_tilde_ladspa_close_plugin (Pd_Plugin_Tilde* x);
void	plugin_tilde_ladspa_apply_plugin (Pd_Plugin_Tilde* x);

void	plugin_tilde_ladspa_print (Pd_Plugin_Tilde* x);
void	plugin_tilde_ladspa_reset (Pd_Plugin_Tilde* x);

void	plugin_tilde_ladspa_connect_audio (Pd_Plugin_Tilde* x,
					   float** audio_inputs,
					   float** audio_outputs,
					   unsigned long num_samples);
void	plugin_tilde_ladspa_set_control_input_by_name (Pd_Plugin_Tilde* x,
					       const char* name,
					       float value);
void	plugin_tilde_ladspa_set_control_input_by_index (Pd_Plugin_Tilde* x,
					       unsigned index_,
					       float value);
/*float	plugin_tilde_ladspa_get_control_input (Pd_Plugin_Tilde* x,
					       const char* name);*/
/* Control output is handled with plugin_tilde_emit_control_output() callback */

/* Local subroutines */
static void	plugin_tilde_ladspa_search_plugin_callback (const char* full_filename,
							    void* plugin_handle,
							    LADSPA_Descriptor_Function descriptor_function,
							    void* user_data);
static void	plugin_tilde_ladspa_count_ports (Pd_Plugin_Tilde* x);
static void	plugin_tilde_ladspa_connect_control_ports (Pd_Plugin_Tilde* x);

static int	plugin_tilde_ladspa_alloc_outofplace_memory (Pd_Plugin_Tilde* x, unsigned long buflen);
static void	plugin_tilde_ladspa_free_outofplace_memory (Pd_Plugin_Tilde* x);
static int	plugin_tilde_ladspa_alloc_control_memory (Pd_Plugin_Tilde* x);
static void	plugin_tilde_ladspa_free_control_memory (Pd_Plugin_Tilde* x);

#endif /* PLUGIN_TILDE_USE_LADSPA */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLUGIN_TILDE_LADSPA_H__ */
/* EOF */
