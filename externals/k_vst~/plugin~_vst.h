/* plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2000 Jarno Seppänen
   $Id: plugin~_vst.h,v 1.1 2004-01-08 14:55:24 ksvalast Exp $

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

#ifndef __PLUGIN_TILDE_VST_H__
#define __PLUGIN_TILDE_VST_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "config.h"

#if PLUGIN_TILDE_USE_VST

#include "plugin~.h"

/* VST header */
#include "vst/AEffect.h"

#if 0 /* moved to plugin~.h because of cyclical header dependency */
typedef struct
{
    AEffect*		instance;

    /* audio wire buffer information */
    float**		audio_inputs;
    float**		audio_outputs;
    unsigned long	num_samples;

    int			editor_open;
} Plugin_Tilde_Vst;
#endif /* moved to plugin~.h because of cyclical header dependency */


/* subroutines to wrap the VST interface */
const char*	plugin_tilde_vst_search_plugin (Pd_Plugin_Tilde* x,
						const char* name);
int	plugin_tilde_vst_open_plugin (Pd_Plugin_Tilde* x,
				      const char* name,
				      const char* lib_name,
				      unsigned long sample_rate);

void  plugin_tilde_vst_close_editor (Pd_Plugin_Tilde* x);
void  plugin_tilde_vst_open_editor (Pd_Plugin_Tilde* x);

void	plugin_tilde_vst_close_plugin (Pd_Plugin_Tilde* x);
void	plugin_tilde_vst_apply_plugin (Pd_Plugin_Tilde* x);

void	plugin_tilde_vst_print (Pd_Plugin_Tilde* x);
void	plugin_tilde_vst_reset (Pd_Plugin_Tilde* x);

void	plugin_tilde_vst_connect_audio (Pd_Plugin_Tilde* x,
					float** audio_inputs,
					float** audio_outputs,
					unsigned long num_samples);
void	plugin_tilde_vst_set_control_input_by_name (Pd_Plugin_Tilde* x,
					    const char* name,
					    float value);
void	plugin_tilde_vst_set_control_input_by_index (Pd_Plugin_Tilde* x,
					       unsigned index_,
					       float value);


/* subroutines needed by the VST interface */
static long	plugin_tilde_vst_audioMaster (AEffect* effect,
					      long opcode,
					      long index,
					      long value,
					      void* ptr,
					      float opt);

  /*
static void	plugin_tilde_vst_open_editor (Pd_Plugin_Tilde* x);
static void	plugin_tilde_vst_close_editor (Pd_Plugin_Tilde* x);
  */

void		plugin_tilde_vst_update_gui (Pd_Plugin_Tilde* x);

void		plugin_tilde_vst_set_program (Pd_Plugin_Tilde*, long);
long		plugin_tilde_vst_get_program (Pd_Plugin_Tilde*);
long		plugin_tilde_vst_get_num_of_programs (Pd_Plugin_Tilde*);
void		plugin_tilde_vst_set_program_name (Pd_Plugin_Tilde*, const char*);

#endif /* PLUGIN_TILDE_USE_VST */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLUGIN_TILDE_VST_H__ */
/* EOF */
