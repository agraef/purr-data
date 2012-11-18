/* plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2000 Jarno Seppänen
   $Id: plugin~.h,v 1.1 2004-01-08 14:55:24 ksvalast Exp $

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

#ifndef __PLUGIN_TILDE_H__
#define __PLUGIN_TILDE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "config.h"

/* Pd header */
#ifndef MAXPDSTRING /* lame */
#include "m_pd.h"
#endif /* MAXPDSTRING */

#if 0
#  if PLUGIN_TILDE_USE_LADSPA
#    include "plugin~_ladspa.h"
#  endif
#  if PLUGIN_TILDE_USE_VST
#    include "plugin~_vst.h"
#  endif
#endif /* 0 */
/*
 * Now I've moved the following two plug-in-architecture-specific structures
 * here because having them in plugin~_ladspa.h and plugin~_vst.h proper will
 * result in a cyclical header dependency
 */
#if PLUGIN_TILDE_USE_LADSPA
#include "ladspa/ladspa.h"
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
#endif /* PLUGIN_TILDE_USE_LADSPA */

#if PLUGIN_TILDE_USE_VST
#include "vst/AEffect.h"
typedef struct
{
    AEffect*		instance;

    /* audio wire buffer information */
    float**		audio_inputs;
    float**		audio_outputs;
    unsigned long	num_samples;

    int			editor_open;

} Plugin_Tilde_Vst;
#endif /* PLUGIN_TILDE_USE_VST */

typedef struct
{
    /* Pd's way of object-oriented programming */
    t_object		x_obj;
    t_clock*		x_clock;

    /* Access to LADSPA/VST plugins */
    void*		plugin_library;
    const char*		plugin_library_filename; /* only for diagnostics */
    union {
#if PLUGIN_TILDE_USE_LADSPA
	Plugin_Tilde_Ladspa	ladspa;
#endif
#if PLUGIN_TILDE_USE_VST
	Plugin_Tilde_Vst	vst;
#endif
    }			plugin;

    /* Plugin information */
    unsigned		num_audio_inputs;
    unsigned		num_audio_outputs;
    unsigned		num_control_inputs;
    unsigned		num_control_outputs;

    /* Pointers to our Pd in- and outlets */
    t_inlet**		audio_inlets;
    t_outlet**		audio_outlets;
    t_outlet*		control_outlet;

    /* Pd's way of passing parameters to the DSP routine */
    t_int*		dsp_vec;
    unsigned		dsp_vec_length;

} Pd_Plugin_Tilde;

/* Object construction and destruction */
void		plugin_tilde_setup (void);
static void*	plugin_tilde_new (t_symbol* s_name, int argc, t_atom *argv);
static void	plugin_tilde_free (Pd_Plugin_Tilde* x);
static void	plugin_tilde_tick (Pd_Plugin_Tilde* x);

/* DSP callbacks */
static void	plugin_tilde_dsp (Pd_Plugin_Tilde* x, t_signal** sp);
static t_int*	plugin_tilde_perform (t_int* w);

/* Plugin callback for sending control output messages */
void	plugin_tilde_emit_control_output (Pd_Plugin_Tilde* x,
					  const char* name,
					  float new_value);

/* First inlet message callback for "control" messages */
static void	plugin_tilde_control (Pd_Plugin_Tilde* x,
				      t_symbol* ctrl_name,
				      t_float ctrl_value);

static void plugin_tilde_opengui (Pd_Plugin_Tilde* x);

static void plugin_tilde_closegui (Pd_Plugin_Tilde* x);

/* First inlet message callback for "control" messages */
static void	plugin_tilde_print (Pd_Plugin_Tilde* x);

/* First inlet message callback for "reset" messages */
static void	plugin_tilde_reset (Pd_Plugin_Tilde* x);

static unsigned	plugin_tilde_get_parm_number (Pd_Plugin_Tilde* x,
					      const char* str);

/* internal API to wrap the different plug-in interfaces */
static const char*	plugin_tilde_search_plugin (Pd_Plugin_Tilde* x,
						    const char* name);
static int	plugin_tilde_open_plugin (Pd_Plugin_Tilde* x,
					  const char* name,
					  const char* lib_name,
					  unsigned long sample_rate);
static void	plugin_tilde_close_plugin (Pd_Plugin_Tilde* x);
static void	plugin_tilde_apply_plugin (Pd_Plugin_Tilde* x);

static void	plugin_tilde_connect_audio (Pd_Plugin_Tilde* x,
					    float** audio_inputs,
					    float** audio_outputs,
					    unsigned long num_samples);
static void	plugin_tilde_set_control_input_by_name (Pd_Plugin_Tilde* x,
						const char* name,
						float value);
static void	plugin_tilde_set_control_input_by_index (Pd_Plugin_Tilde* x,
						unsigned index_,
						float value);
/*static float	plugin_tilde_get_control_input (Pd_Plugin_Tilde* x,
						unsigned int index);*/
static void	plugin_tilde_update_gui (Pd_Plugin_Tilde* x);

static void     plugin_tilde_program (Pd_Plugin_Tilde* x, t_float prog_num);
static void     plugin_tilde_programname (Pd_Plugin_Tilde* x, t_symbol* progname);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLUGIN_TILDE_H__ */
/* EOF */
