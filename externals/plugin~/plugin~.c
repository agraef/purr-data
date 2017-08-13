/* plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2000 Jarno Seppänen
   remIXed 2005
   $Id: plugin~.c,v 1.5 2005-04-30 20:30:53 ix9 Exp $

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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plugin~.h"
#include "jutils.h"


#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
static char*s_locale=NULL;
static void plugin_tilde_pushlocale(void)
{
  if(s_locale)verbose(1, "pushing locale '%s'", s_locale);
  s_locale=setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");
}
static void plugin_tilde_poplocale(void)
{
  if(!s_locale)verbose(1, "popping empty locale");
  setlocale(LC_NUMERIC, s_locale);
  s_locale=NULL;
}
#else
static void plugin_tilde_pushlocale(void) {
  static int again=0;
  if(!again) {
    verbose(1, "plugins~: couldn't modify locales (compiled without locale.h)");
    verbose(1, "          if you experience weird characters try running Pd with LANG=C");
  }
  again=1;
}
static void plugin_tilde_poplocale (void) {}
#endif

static int plugin_tilde_have_plugin(Pd_Plugin_Tilde* x);

static t_class* plugin_tilde_class;

void plugin_tilde_setup (void)
{
  /* Make a new Pd class with 2 string creation parameters */
  plugin_tilde_class = class_new (gensym ("plugin~"),
                                  (t_newmethod)plugin_tilde_new,
                                  (t_method)plugin_tilde_free,
                                  sizeof (Pd_Plugin_Tilde),
                                  0,
                                  A_DEFSYM, A_DEFSYM, 0);
  assert (plugin_tilde_class != NULL);

  /* Let's be explicit in not converting the signals in any way */
  assert (sizeof (float) == sizeof (t_float));

  assert (sizeof (float) == sizeof (LADSPA_Data));

  class_addmethod (plugin_tilde_class,(t_method)plugin_tilde_dsp,gensym ("dsp"),A_CANT, 0);
  class_addmethod (plugin_tilde_class,(t_method)plugin_tilde_control,gensym ("control"),A_SYMBOL, A_FLOAT, 0);
  class_addmethod (plugin_tilde_class,(t_method)plugin_tilde_info,gensym ("info"),0);
  class_addmethod (plugin_tilde_class,(t_method)plugin_tilde_list,gensym ("listplugins"),0);
  class_addmethod (plugin_tilde_class,(t_method)plugin_tilde_plug,gensym ("plug"),A_SYMBOL,0);
  class_addmethod (plugin_tilde_class,(t_method)plugin_tilde_active,gensym ("active"),A_FLOAT,0);
  class_addmethod (plugin_tilde_class,(t_method)plugin_tilde_reset,gensym ("reset"),0);
  class_addmethod (plugin_tilde_class,nullfn,gensym ("signal"),0);
}

static void* plugin_tilde_new (t_symbol* s_name, t_symbol* s_lib_name)
{
  Pd_Plugin_Tilde* x = NULL;
  unsigned i = 0;

  /* Allocate object struct */
  x = (Pd_Plugin_Tilde*)pd_new (plugin_tilde_class);
  if(NULL==x)return NULL;
  /* Initialize object struct */
  x->plugin_library = NULL;
  x->plugin_library_filename = NULL;
  x->num_audio_inputs = 2;
  x->num_audio_outputs = 2;
  x->num_control_inputs = 1;
  x->num_control_outputs = 1;
  x->audio_inlets = NULL;
  x->audio_outlets = NULL;
  x->control_outlet = NULL;
  x->dsp_vec = NULL;
  x->dsp_vec_length = 0;
  x->dsp_active = 0;

  if (s_name != &s_) {
    if (s_lib_name->s_name == NULL || strlen (s_lib_name->s_name) == 0)
      x->plugin_library_filename = plugin_tilde_search_plugin (x, s_name->s_name);
    else
      x->plugin_library_filename = strdup (s_lib_name->s_name);

    if (x->plugin_library_filename != NULL) {
      if (plugin_tilde_open_plugin (x,
                                    s_name->s_name,
                                    x->plugin_library_filename,
                                    (unsigned long)sys_getsr ())) {
        error("plugin~: Unable to open plugin '%s' in '%s'", s_name->s_name, x->plugin_library_filename);
        plugin_tilde_ladspa_close_plugin (x);
      } else {
        post("plugin~: \"%s\"", x->plugin.ladspa.type->Name);
      }
    }
  }
    
  /* Create in- and outlet(s) */

  /* Allocate memory for in- and outlet pointers */
  x->audio_inlets = (t_inlet**)calloc (x->num_audio_inputs, sizeof (t_inlet*));
  x->audio_outlets = (t_outlet**)calloc (x->num_audio_outputs, sizeof (t_outlet*));

  /* The first inlet is always there (needn't be created), and is
     used for control messages.  Now, create the rest of the
     inlets for audio signal input. */ 
  for (i = 0; i < x->num_audio_inputs; i++) {
    x->audio_inlets[i] = inlet_new (&x->x_obj,
                                    &x->x_obj.ob_pd,
                                    gensym ("signal"),
                                    gensym ("signal"));
  }

  /* We use the first outlet always for LADSPA/VST parameter control
     messages */
  x->control_outlet = outlet_new (&x->x_obj, gensym ("control"));

  /* The rest of the outlets are used for audio signal output */
  for (i = 0; i < x->num_audio_outputs; i++) {
    x->audio_outlets[i] = outlet_new (&x->x_obj, gensym ("signal"));
  }

  /* Allocate memory for DSP parameters */
  x->dsp_vec_length = x->num_audio_inputs + x->num_audio_outputs + 2;
  x->dsp_vec = (t_int*)calloc (x->dsp_vec_length, sizeof (t_int));

  if(NULL==x->dsp_vec)return NULL;
  return x;
}

static void plugin_tilde_free (Pd_Plugin_Tilde* x)
{
  unsigned i = 0;
  /* Unload LADSPA/VST plugin */
  plugin_tilde_close_plugin (x);

  /* Free DSP parameter memory */
  if (x->dsp_vec != NULL) {
    free (x->dsp_vec);
    x->dsp_vec = NULL;
    x->dsp_vec_length = 0;
  }

  /* Destroy inlets */
  if (x->audio_inlets != NULL) {
    for (i = 0; i < x->num_audio_inputs; i++) {
	    inlet_free (x->audio_inlets[i]);
    }
    free (x->audio_inlets);
    x->audio_inlets = NULL;
  }

  /* Destroy outlets */
  if (x->control_outlet != NULL) {
    outlet_free (x->control_outlet);
    x->control_outlet = NULL;
  }
  if (x->audio_outlets != NULL) {
    for (i = 0; i < x->num_audio_outputs; i++) {
	    outlet_free (x->audio_outlets[i]);
    }
    free (x->audio_outlets);
    x->audio_outlets = NULL;
  }

  if (x->plugin_library_filename != NULL) {
    free ((void*)x->plugin_library_filename);
    x->plugin_library_filename = NULL;
  }
}

static void plugin_tilde_dsp (Pd_Plugin_Tilde* x, t_signal** sp)
{
  unsigned i = 0;
  unsigned long num_samples;

  num_samples = sp[0]->s_n;

  /* Pack vector of parameters for DSP routine */
  x->dsp_vec[0] = (t_int)x;
  x->dsp_vec[1] = (t_int)num_samples;
  /* Inputs are before outputs; ignore the first "null" input */
  for (i = 2; i < x->dsp_vec_length; i++) {
    x->dsp_vec[i] = (t_int)(sp[i - 1]->s_vec);
  }

  /* Connect audio ports with buffers (this is only done when DSP
     processing begins) */
  plugin_tilde_connect_audio (x,
                              (float**)(&x->dsp_vec[2]),
                              (float**)(&x->dsp_vec[2 + x->num_audio_inputs]),
                              num_samples);

  /* add DSP routine to Pd's DSP chain */
  dsp_addv (plugin_tilde_perform, x->dsp_vec_length, x->dsp_vec);
}

static t_int* plugin_tilde_perform (t_int* w)
{
  Pd_Plugin_Tilde* x = NULL;
  t_float** audio_inputs = NULL;
  t_float** audio_outputs = NULL;
  int num_samples = 0;

  /* precondition(s) */
  assert (w != NULL);
 
  /* Unpack DSP parameter vector */
  x = (Pd_Plugin_Tilde*)(w[1]);
  num_samples = (int)(w[2]);
  audio_inputs = (t_float**)(&w[3]);
  audio_outputs = (t_float**)(&w[3 + x->num_audio_inputs]);
  /* Call the LADSPA/VST plugin */
  plugin_tilde_apply_plugin (x);
  return w + (x->dsp_vec_length + 1);
}

void plugin_tilde_emit_control_output (Pd_Plugin_Tilde* x,
                                       const char* name,
                                       float new_value,
                                       int output_port_index)
{
  /* Construct and outlet a "control" message with three Pd atoms */
  t_atom anything_atoms[3];
  anything_atoms[0].a_type = A_SYMBOL;
  anything_atoms[0].a_w.w_symbol = gensym ((char*)name);
  anything_atoms[1].a_type = A_FLOAT;
  anything_atoms[1].a_w.w_float = new_value;
  anything_atoms[2].a_type = A_FLOAT;
  anything_atoms[2].a_w.w_float = output_port_index;
  outlet_anything (x->control_outlet, gensym ("control"), 3, anything_atoms);
}

static void plugin_tilde_control (Pd_Plugin_Tilde* x,
                                  t_symbol* ctrl_name,
                                  t_float ctrl_value)
/* Change the value of a named control port of the plug-in */
{
  unsigned parm_num = 0;
  if(!plugin_tilde_have_plugin(x))return;

  if (ctrl_name->s_name == NULL || strlen (ctrl_name->s_name) == 0) {
    pd_error(x, "plugin~: control messages must have a name and a value");
    return;
  }

  parm_num = plugin_tilde_get_parm_number (x, ctrl_name->s_name);
  if (parm_num) {
    plugin_tilde_set_control_input_by_index (x, parm_num - 1, ctrl_value);
  }
  else {
    plugin_tilde_set_control_input_by_name (x, ctrl_name->s_name, ctrl_value);
  }
}

static void plugin_tilde_info (Pd_Plugin_Tilde* x) {

  unsigned port_index = 0;
  t_atom at[5];
  LADSPA_PortDescriptor port_type;
  LADSPA_PortRangeHintDescriptor iHintDescriptor;

  if(!plugin_tilde_have_plugin(x))return;

  for (port_index = 0; port_index < x->plugin.ladspa.type->PortCount; port_index++) {
    port_type = x->plugin.ladspa.type->PortDescriptors[port_index];
    iHintDescriptor = x->plugin.ladspa.type->PortRangeHints[port_index].HintDescriptor;

    t_symbol*xlet=gensym("unknown");
    t_symbol*type=gensym("unknown");
    t_symbol*name=gensym("unknown");

    t_float bound_lo=0.;
    t_float bound_hi=1.;

    if(LADSPA_IS_PORT_INPUT (port_type))
      xlet=gensym("in");
    else if (LADSPA_IS_PORT_OUTPUT (port_type))
      xlet=gensym("out");

    if (LADSPA_IS_PORT_CONTROL (port_type))
      type=gensym("control");
    else if (LADSPA_IS_PORT_AUDIO (port_type))
      type=gensym("audio");

    name=gensym(x->plugin.ladspa.type->PortNames[port_index]);

    if (LADSPA_IS_HINT_BOUNDED_BELOW(iHintDescriptor))
      bound_lo=x->plugin.ladspa.type->PortRangeHints[port_index].LowerBound;
    if (LADSPA_IS_HINT_BOUNDED_ABOVE(iHintDescriptor))
      bound_hi=x->plugin.ladspa.type->PortRangeHints[port_index].UpperBound;

    //    post("port#%d: %s %s %s  %f..%f", port_index, xlet->s_name, type->s_name, name->s_name, bound_lo, bound_hi);

    SETSYMBOL(at+0, xlet);
    SETSYMBOL(at+1, type);
    SETSYMBOL(at+2, name);
    SETFLOAT (at+3, bound_lo);
    SETFLOAT (at+4, bound_hi);

    outlet_anything (x->control_outlet, gensym ("port"), 5, at);
  }
}

static void plugin_tilde_list (Pd_Plugin_Tilde* x) {
  void* user_data[1];
  user_data[0] = x;
  plugin_tilde_pushlocale();
  LADSPAPluginSearch(plugin_tilde_ladspa_describe,(void*)user_data);
  plugin_tilde_poplocale();
}

static void plugin_tilde_ladspa_describe(const char * pcFullFilename, 
                                         void * pvPluginHandle,
                                         LADSPA_Descriptor_Function fDescriptorFunction, void* user_data) {

  Pd_Plugin_Tilde* x = (((void**)user_data)[0]);
  t_atom at[1];
  const LADSPA_Descriptor * psDescriptor;
  long lIndex;


  SETSYMBOL(at, gensym(pcFullFilename));
  outlet_anything (x->control_outlet, gensym ("library"), 1, at);
 
  for (lIndex = 0;
       (psDescriptor = fDescriptorFunction(lIndex)) != NULL;
       lIndex++) {
    SETSYMBOL(at, gensym ((char*)psDescriptor->Name)); 
    outlet_anything (x->control_outlet, gensym ("name"), 1, at);
    SETSYMBOL(at, gensym ((char*)psDescriptor->Label)); 
    outlet_anything (x->control_outlet, gensym ("label"), 1, at);
    SETFLOAT(at,  psDescriptor->UniqueID);
    outlet_anything (x->control_outlet, gensym ("id"), 1, at);
    SETSYMBOL(at, gensym ((char*)psDescriptor->Maker));
    outlet_anything (x->control_outlet, gensym ("maker"), 1, at);
  }
}

static void plugin_tilde_active (Pd_Plugin_Tilde* x, t_float active) {
  x->dsp_active = active;
}

static void plugin_tilde_plug (Pd_Plugin_Tilde* x, t_symbol* plug_name) {
  plugin_tilde_ladspa_close_plugin(x);
  x->plugin_library_filename = NULL;
  x->plugin_library_filename = plugin_tilde_search_plugin (x, plug_name->s_name);
  if (x->plugin_library_filename == NULL)
    error("plugin~: plugin not found in any library");
  if (plugin_tilde_open_plugin (x, plug_name->s_name, x->plugin_library_filename,(unsigned long)sys_getsr ()))
    error("plugin~: Unable to open plugin");
  else {
    post("plugin~: \"%s\"", x->plugin.ladspa.type->Name);
  }
}

static void plugin_tilde_reset (Pd_Plugin_Tilde* x)
{
  plugin_tilde_ladspa_reset (x);
}

static unsigned plugin_tilde_get_parm_number (Pd_Plugin_Tilde* x,
                                              const char* str)
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
  else if (num >= 1 && num <= (long)x->num_control_inputs) {
    /* string ok and within range */
    return (unsigned)num;
  }
  else {
    /* number out of range */
    return 0;
  }
}

static const char* plugin_tilde_search_plugin (Pd_Plugin_Tilde* x,
                                               const char* name)
{
  return plugin_tilde_ladspa_search_plugin (x, name);
}

static int plugin_tilde_open_plugin (Pd_Plugin_Tilde* x,
                                     const char* name,
                                     const char* lib_name,
                                     unsigned long sample_rate)
{
  int ret = 0;

  verbose(2, "plugin~: open_plugin (x, \"%s\", \"%s\", %ld);", name, lib_name, sample_rate);

  ret = plugin_tilde_ladspa_open_plugin (x, name, lib_name, sample_rate);

  if (ret == 0) {
    x->dsp_active = 1;
    verbose(1, "plugin~: plugin active");
  }
  //    plugin_tilde_info (x);
  return ret;
}

static void plugin_tilde_close_plugin (Pd_Plugin_Tilde* x)
{

  verbose(2, "plugin~: close_plugin (x)");
  plugin_tilde_ladspa_close_plugin (x);
  verbose(1, "plugin~: destructed plugin successfully");
}

static void plugin_tilde_apply_plugin (Pd_Plugin_Tilde* x)
{
  if (x->dsp_active == 1)
    plugin_tilde_ladspa_apply_plugin (x);
}

static void plugin_tilde_connect_audio (Pd_Plugin_Tilde* x,
                                        float** audio_inputs,
                                        float** audio_outputs,
                                        unsigned long num_samples)
{
  plugin_tilde_ladspa_connect_audio (x, audio_inputs, audio_outputs,
                                     num_samples);
}

static void plugin_tilde_set_control_input_by_name (Pd_Plugin_Tilde* x,
                                                    const char* name,
                                                    float value)
{
  plugin_tilde_ladspa_set_control_input_by_name (x, name, value);
}

static void plugin_tilde_set_control_input_by_index (Pd_Plugin_Tilde* x,
                                                     unsigned index_,
                                                     float value)
/* plugin~.c:535: warning: declaration of `index' shadows global declaration */
{
  plugin_tilde_ladspa_set_control_input_by_index (x, index_, value);
}


const char* plugin_tilde_ladspa_search_plugin (Pd_Plugin_Tilde* x,
                                               const char* name)
{
  char* lib_name = NULL;
  void* user_data[2];

  user_data[0] = (void*)(&lib_name);
  user_data[1] = (void*)name;

  lib_name = NULL;
  plugin_tilde_pushlocale();
  LADSPAPluginSearch (plugin_tilde_ladspa_search_plugin_callback,
                      (void*)user_data);
  plugin_tilde_poplocale();

  /* The callback (allocates and) writes lib_name, if it finds the plugin */
  return lib_name;
}

static void plugin_tilde_ladspa_search_plugin_callback (const char* full_filename,
                                                        void* plugin_handle,
                                                        LADSPA_Descriptor_Function descriptor_function,
                                                        void* user_data)
{
  const LADSPA_Descriptor* descriptor = NULL;
  unsigned plug_index = 0;

  char** out_lib_name = (char**)(((void**)user_data)[0]);
  char* name = (char*)(((void**)user_data)[1]);

  /* Stop searching when a first matching plugin is found */
  if (*out_lib_name == NULL)
    {
      //	vedrbose(1, "plugin~: searching library \"%s\"...", full_filename);

      for (plug_index = 0;
           (descriptor = descriptor_function (plug_index)) != NULL;
           plug_index++)
        {
          //	   verbose(1, "plugin~: label \"%s\"", descriptor->Label);

          if (strcasecmp (name, descriptor->Label) == 0)
            {
              /* found a matching plugin */
              *out_lib_name = strdup (full_filename);

              verbose(1, "plugin~: found plugin \"%s\" in library \"%s\"",
                   name, full_filename);

              /* don't need to look any further */
              break;
            }
        }
    }
}

int plugin_tilde_ladspa_open_plugin (Pd_Plugin_Tilde* x,
                                     const char* name,
                                     const char* lib_name,
                                     unsigned long sample_rate)
{
  /* precondition(s) */
  assert (x != NULL);
  assert (lib_name != NULL);
  assert (name != NULL);
  assert (sample_rate != 0);

  /* Initialize object struct */
  x->plugin.ladspa.type = NULL;
  x->plugin.ladspa.instance = NULL;
  x->plugin.ladspa.control_input_values = NULL;
  x->plugin.ladspa.control_output_values = NULL;
  x->plugin.ladspa.control_input_ports = NULL;
  x->plugin.ladspa.control_output_ports = NULL;
  x->plugin.ladspa.prev_control_output_values = NULL;
  x->plugin.ladspa.prev_control_output_values_invalid = 1;
  x->plugin.ladspa.outofplace_audio_outputs = NULL;
  x->plugin.ladspa.actual_audio_outputs = NULL;
  x->plugin.ladspa.num_samples = 0;
  x->plugin.ladspa.sample_rate = sample_rate;

  /* Attempt to load the plugin. */
  plugin_tilde_pushlocale();
  x->plugin_library = loadLADSPAPluginLibrary (lib_name);
  if (x->plugin_library == NULL)
    {
      /* error */
      plugin_tilde_poplocale();
      error("plugin~: Unable to load LADSPA plugin library \"%s\"",
           lib_name);
      return 1;
    }
  x->plugin.ladspa.type = findLADSPAPluginDescriptor (x->plugin_library,
                                                      lib_name,
                                                      name);
  plugin_tilde_poplocale();
  if (x->plugin.ladspa.type == NULL)
    {
      error("plugin~: Unable to find LADSPA plugin \"%s\" within library \"%s\"",
           name, lib_name);
      return 1;
    }

  /* Construct the plugin. */
  x->plugin.ladspa.instance
    = x->plugin.ladspa.type->instantiate (x->plugin.ladspa.type,
                                          sample_rate);
  if (x->plugin.ladspa.instance == NULL)
    {
      /* error */
      error("plugin~: Unable to instantiate LADSPA plugin \"%s\"",
           x->plugin.ladspa.type->Name);
      return 1;
    }

  verbose(1, "plugin~: constructed plugin \"%s\" successfully", x->plugin.ladspa.type->Name);

  /* Find out the number of inputs and outputs needed. */
  plugin_tilde_ladspa_count_ports (x);

  /* Allocate memory for control values */
  if (plugin_tilde_ladspa_alloc_control_memory (x)) {
    error("plugin~: out of memory");
    return 1; /* error */
  }

  /* Connect control ports with buffers */
  plugin_tilde_ladspa_connect_control_ports (x);

  /* Activate the plugin. */
  if (x->plugin.ladspa.type->activate != NULL)
    {
      x->plugin.ladspa.type->activate (x->plugin.ladspa.instance);
    }

  /* success */
  return 0;
}

void plugin_tilde_ladspa_close_plugin (Pd_Plugin_Tilde* x)
{
  /* precondition(s) */
  if (x->plugin.ladspa.instance != NULL)
    {
      /* Deactivate the plugin. */
      if (x->plugin.ladspa.type->deactivate != NULL)
        {
          x->plugin.ladspa.type->deactivate (x->plugin.ladspa.instance);
        }

      /* Destruct the plugin. */
      x->plugin.ladspa.type->cleanup (x->plugin.ladspa.instance);
      x->plugin.ladspa.instance = NULL;
    }

  /* Free the control value memory */
  plugin_tilde_ladspa_free_control_memory (x);

  if (x->plugin_library != NULL)
    {
      unloadLADSPAPluginLibrary (x->plugin_library);
      x->plugin_library = NULL;
      x->plugin.ladspa.type = NULL;
    }

  /* Free the out-of-place memory */
  plugin_tilde_ladspa_free_outofplace_memory (x);
}

void plugin_tilde_ladspa_apply_plugin (Pd_Plugin_Tilde* x)
{
  unsigned i;

  /* Run the plugin on Pd's buffers */
  x->plugin.ladspa.type->run (x->plugin.ladspa.instance,
                              x->plugin.ladspa.num_samples);

  /* Copy out-of-place buffers to Pd buffers if used */
  if (x->plugin.ladspa.outofplace_audio_outputs != NULL)
    {
      for (i = 0; i < x->num_audio_outputs; i++)
        {
          unsigned j;
          for (j = 0; j < (unsigned)x->plugin.ladspa.num_samples; j++)
            {
              x->plugin.ladspa.actual_audio_outputs[i][j]
                = x->plugin.ladspa.outofplace_audio_outputs[i][j];
            }
        }
    }

  /* Compare control output values to previous and send control
     messages, if necessary */
  for (i = 0; i < x->num_control_outputs; i++)
    {
      /* Check whether the prev values have been initialized; if
         not, send a control message for each of the control outputs */
      if ((x->plugin.ladspa.control_output_values[i]
           != x->plugin.ladspa.prev_control_output_values[i])
          || x->plugin.ladspa.prev_control_output_values_invalid)
        {
          /* Emit a control message */
          plugin_tilde_emit_control_output (x,
                                            x->plugin.ladspa.type->PortNames[x->plugin.ladspa.control_output_ports[i]],
                                            x->plugin.ladspa.control_output_values[i],
                                            i);
          /* Update the corresponding control monitoring value */
          x->plugin.ladspa.prev_control_output_values[i] = x->plugin.ladspa.control_output_values[i];
        }
    }
  x->plugin.ladspa.prev_control_output_values_invalid = 0;
}

void plugin_tilde_ladspa_reset (Pd_Plugin_Tilde* x)
{
  /* precondition(s) */
  if(!plugin_tilde_have_plugin(x))return;

  if (x->plugin.ladspa.type->activate != NULL
      && x->plugin.ladspa.type->deactivate == NULL)
    {
      verbose(1, "plugin~: Warning: Plug-in defines activate() method but no deactivate() method");
    }

  /* reset plug-in by first deactivating and then re-activating it */
  if (x->plugin.ladspa.type->deactivate != NULL)
    {
      x->plugin.ladspa.type->deactivate (x->plugin.ladspa.instance);
    }
  if (x->plugin.ladspa.type->activate != NULL)
    {
      x->plugin.ladspa.type->activate (x->plugin.ladspa.instance);
    }
}

void plugin_tilde_ladspa_connect_audio (Pd_Plugin_Tilde* x,
                                        float** audio_inputs,
                                        float** audio_outputs,
                                        unsigned long num_samples)
{
  unsigned port_index = 0;
  unsigned input_count = 0;
  unsigned output_count = 0;

  if(!plugin_tilde_have_plugin(x))return;

  /* Allocate out-of-place memory if needed */
  if (plugin_tilde_ladspa_alloc_outofplace_memory (x, num_samples)) {
    error("plugin~: out of memory");
    return;
  }

  if (x->plugin.ladspa.outofplace_audio_outputs != NULL) {
    x->plugin.ladspa.actual_audio_outputs = audio_outputs;
    audio_outputs = x->plugin.ladspa.outofplace_audio_outputs;
  }

  input_count = 0;
  output_count = 0;
  for (port_index = 0; port_index < x->plugin.ladspa.type->PortCount; port_index++)
    {
      LADSPA_PortDescriptor port_type;
      port_type = x->plugin.ladspa.type->PortDescriptors[port_index];
      if (LADSPA_IS_PORT_AUDIO (port_type))
        {
          if (LADSPA_IS_PORT_INPUT (port_type))
            {
              x->plugin.ladspa.type->connect_port (x->plugin.ladspa.instance,
                                                   port_index,
                                                   (LADSPA_Data*)audio_inputs[input_count]);
              input_count++;
            }
          else if (LADSPA_IS_PORT_OUTPUT (port_type))
            {
              x->plugin.ladspa.type->connect_port (x->plugin.ladspa.instance,
                                                   port_index,
                                                   (LADSPA_Data*)audio_outputs[output_count]);
              output_count++;
            }
        }
    }

  x->plugin.ladspa.num_samples = num_samples;
}

void plugin_tilde_ladspa_set_control_input_by_name (Pd_Plugin_Tilde* x,
                                                    const char* name,
                                                    float value)
{
  unsigned port_index = 0;
  unsigned ctrl_input_index = 0;
  int found_port = 0; /* boolean */

  /* precondition(s) */
  assert (x != NULL);

  if (name == NULL || strlen (name) == 0) {
    pd_error(x, "plugin~: no control port name specified");
    return;
  }

  if(NULL==x->plugin.ladspa.type) {
    error("plugin~: unable to determine LADSPA type");
    return;
  }


  /* compare control name to LADSPA control input ports' names
     case-insensitively */
  found_port = 0;
  ctrl_input_index = 0;
  for (port_index = 0; port_index < x->plugin.ladspa.type->PortCount; port_index++)
    {
      LADSPA_PortDescriptor port_type;
      port_type = x->plugin.ladspa.type->PortDescriptors[port_index];
      if (LADSPA_IS_PORT_CONTROL (port_type)
          && LADSPA_IS_PORT_INPUT (port_type))
        {
          const char* port_name = NULL;
          unsigned cmp_length = 0;
          port_name = x->plugin.ladspa.type->PortNames[port_index];
          cmp_length = MIN (strlen (name), strlen (port_name));
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

  if (!found_port)
    {
      error("plugin~: plugin doesn't have a control input port named \"%s\"",
           name);
      return;
    }

  plugin_tilde_ladspa_set_control_input_by_index (x,
                                                  ctrl_input_index,
                                                  value);
}

void plugin_tilde_ladspa_set_control_input_by_index (Pd_Plugin_Tilde* x,
                                                     unsigned ctrl_input_index,
                                                     float value)
{
  unsigned port_index = 0;
  unsigned ctrl_input_count = 0;
  int found_port = 0; /* boolean */
 
  /* precondition(s) */
  assert (x != NULL);
  /* assert (ctrl_input_index >= 0); causes a warning */
  /* assert (ctrl_input_index < x->num_control_inputs); */

  if(NULL==x->plugin.ladspa.type) {
    error("plugin~: unable to determine LADSPA type");
    return;
  }

  if (ctrl_input_index >= x->num_control_inputs) {
    error("plugin~: control port number %d is out of range [1, %d]",
         ctrl_input_index + 1, x->num_control_inputs);
    return;
  }

  /* bound parameter value */
  /* sigh, need to find the N'th ctrl input port by hand */
  found_port = 0;
  ctrl_input_count = 0;
  for (port_index = 0; port_index < x->plugin.ladspa.type->PortCount; port_index++)
    {
      LADSPA_PortDescriptor port_type;
      port_type = x->plugin.ladspa.type->PortDescriptors[port_index];
      if (LADSPA_IS_PORT_CONTROL (port_type)
          && LADSPA_IS_PORT_INPUT (port_type))
        {
          if (ctrl_input_index == ctrl_input_count) {
            found_port = 1;
            break;
          }
          ctrl_input_count++;
        }
    }
  if (!found_port) {
    error("plugin~: plugin doesn't have %ud control input ports",
	       ctrl_input_index + 1);
    return;
  }

  /* out of bounds rules WTF!!!!~
     if (x->plugin.ladspa.type->PortRangeHints != NULL) {
     const LADSPA_PortRangeHint* hint
     = &x->plugin.ladspa.type->PortRangeHints[port_index];
     if (LADSPA_IS_HINT_BOUNDED_BELOW (hint->HintDescriptor)) {
     bounded_from_below = 1;
     lower_bound = hint->LowerBound;
     if (LADSPA_IS_HINT_SAMPLE_RATE (hint->HintDescriptor)) {
     assert (x->plugin.ladspa.sample_rate != 0);
     lower_bound *= (float)x->plugin.ladspa.sample_rate;
     }
     }
     if (LADSPA_IS_HINT_BOUNDED_ABOVE (hint->HintDescriptor)) {
     bounded_from_above = 1;
     upper_bound = hint->UpperBound;
     if (LADSPA_IS_HINT_SAMPLE_RATE (hint->HintDescriptor)) {
     assert (x->plugin.ladspa.sample_rate != 0);
     upper_bound *= (float)x->plugin.ladspa.sample_rate;
     }
     }
     }
     bounded = 0;
     if (bounded_from_below && value < lower_bound) {
     value = lower_bound;
     bounded = 1;
     }
     if (bounded_from_above && value > upper_bound) {
     value = upper_bound;
     bounded = 1;
     } */

  /* set the appropriate control port value */
  x->plugin.ladspa.control_input_values[ctrl_input_index] = value;

  //    verbose(1, "plugin~: control change control input port #%ud to value %f", ctrl_input_index + 1, value);
}

static void plugin_tilde_ladspa_count_ports (Pd_Plugin_Tilde* x)
{
  unsigned i = 0;

  x->num_audio_inputs = 0;
  x->num_audio_outputs = 0;
  x->num_control_inputs = 0;
  x->num_control_outputs = 0;

  for (i = 0; i < x->plugin.ladspa.type->PortCount; i++)
    {
      LADSPA_PortDescriptor port_type;
      port_type = x->plugin.ladspa.type->PortDescriptors[i];

      if (LADSPA_IS_PORT_AUDIO (port_type))
        {
          if (LADSPA_IS_PORT_INPUT (port_type))
            {
              x->num_audio_inputs++;
            }
          else if (LADSPA_IS_PORT_OUTPUT (port_type))
            {
              x->num_audio_outputs++;
            }
        }
      else if (LADSPA_IS_PORT_CONTROL (port_type))
        {
          if (LADSPA_IS_PORT_INPUT (port_type))
            {
              x->num_control_inputs++;
            }
          else if (LADSPA_IS_PORT_OUTPUT (port_type))
            {
              x->num_control_outputs++;
            }
        }
    }

  verbose(1, "plugin~: plugin ports: audio %d/%d ctrl %d/%d",
       x->num_audio_inputs, x->num_audio_outputs,
       x->num_control_inputs, x->num_control_outputs);
}

static void plugin_tilde_ladspa_connect_control_ports (Pd_Plugin_Tilde* x)
{
  unsigned port_index = 0;
  unsigned input_count = 0;
  unsigned output_count = 0;

  input_count = 0;
  output_count = 0;
  for (port_index = 0; port_index < x->plugin.ladspa.type->PortCount; port_index++)
    {
      LADSPA_PortDescriptor port_type;
      port_type = x->plugin.ladspa.type->PortDescriptors[port_index];

      if (LADSPA_IS_PORT_CONTROL (port_type))
        {
          if (LADSPA_IS_PORT_INPUT (port_type))
            {
              x->plugin.ladspa.type->connect_port (x->plugin.ladspa.instance,
                                                   port_index,
                                                   &x->plugin.ladspa.control_input_values[input_count]);
              x->plugin.ladspa.control_input_ports[input_count] = port_index;
              input_count++;
            }
          else if (LADSPA_IS_PORT_OUTPUT (port_type))
            {
              x->plugin.ladspa.type->connect_port (x->plugin.ladspa.instance,
                                                   port_index,
                                                   &x->plugin.ladspa.control_output_values[output_count]);
              x->plugin.ladspa.control_output_ports[output_count] = port_index;

              output_count++;
            }
        }
    }
}

static int plugin_tilde_ladspa_alloc_outofplace_memory (Pd_Plugin_Tilde* x, unsigned long buflen)
{
  assert (x != NULL);

  plugin_tilde_ladspa_free_outofplace_memory (x);

  if (LADSPA_IS_INPLACE_BROKEN (x->plugin.ladspa.type->Properties))
    {
      unsigned i = 0;

      x->plugin.ladspa.outofplace_audio_outputs = (t_float**)
        calloc (x->num_audio_outputs, sizeof (t_float*));
      if (x->plugin.ladspa.outofplace_audio_outputs == NULL) {
        return 1; /* error */
      }

      for (i = 0; i < x->num_audio_outputs; i++)
        {
          x->plugin.ladspa.outofplace_audio_outputs[i] = (t_float*)
            calloc (buflen, sizeof (t_float));
          if (x->plugin.ladspa.outofplace_audio_outputs[i] == NULL) {
            /* FIXME free got buffers? */
            return 1; /* error */
          }
        }
    }
  return 0; /* success */
}

static void plugin_tilde_ladspa_free_outofplace_memory (Pd_Plugin_Tilde* x)
{
  assert (x != NULL);

  if (x->plugin.ladspa.outofplace_audio_outputs != NULL)
    {
      unsigned i = 0;
      for (i = 0; i < x->num_audio_outputs; i++)
        {
          free (x->plugin.ladspa.outofplace_audio_outputs[i]);
        }
      free (x->plugin.ladspa.outofplace_audio_outputs);
      x->plugin.ladspa.outofplace_audio_outputs = NULL;
    }
}

static int plugin_tilde_ladspa_alloc_control_memory (Pd_Plugin_Tilde* x)
{
  x->plugin.ladspa.control_input_values = NULL;
  x->plugin.ladspa.control_input_ports = NULL;
  if (x->num_control_inputs > 0)
    {
      x->plugin.ladspa.control_input_values = (float*)calloc
        (x->num_control_inputs, sizeof (float));
      x->plugin.ladspa.control_input_ports = (int*)calloc
        (x->num_control_inputs, sizeof (int));
      if (x->plugin.ladspa.control_input_values == NULL
          || x->plugin.ladspa.control_input_ports == NULL) {
        return 1; /* error */
      }
    }
  x->plugin.ladspa.control_output_values = NULL;
  x->plugin.ladspa.control_output_ports = NULL;
  x->plugin.ladspa.prev_control_output_values = NULL;
  if (x->num_control_outputs > 0)
    {
      x->plugin.ladspa.control_output_values = (float*)calloc
        (x->num_control_outputs, sizeof (float));
      x->plugin.ladspa.control_output_ports = (int*)calloc
        (x->num_control_outputs, sizeof (int));
      x->plugin.ladspa.prev_control_output_values = (float*)calloc
        (x->num_control_outputs, sizeof (float));
      if (x->plugin.ladspa.control_output_values == NULL
          || x->plugin.ladspa.prev_control_output_values == NULL
          || x->plugin.ladspa.control_output_ports == NULL) {
        return 1; /* error */
      }
    }
  /* Indicate initial conditions */
  x->plugin.ladspa.prev_control_output_values_invalid = 1;
  return 0; /* success */
}

static void plugin_tilde_ladspa_free_control_memory (Pd_Plugin_Tilde* x)
{
  if (x->plugin.ladspa.control_input_values != NULL)
    {
      free (x->plugin.ladspa.control_input_values);
      x->plugin.ladspa.control_input_values = NULL;
    }
  if (x->plugin.ladspa.control_output_values != NULL)
    {
      free (x->plugin.ladspa.control_output_values);
      x->plugin.ladspa.control_output_values = NULL;
    }
  if (x->plugin.ladspa.prev_control_output_values != NULL)
    {
      free (x->plugin.ladspa.prev_control_output_values);
      x->plugin.ladspa.prev_control_output_values = NULL;
    }
  if (x->plugin.ladspa.control_input_ports != NULL)
    {
      free (x->plugin.ladspa.control_input_ports);
      x->plugin.ladspa.control_input_ports = NULL;
    }
  if (x->plugin.ladspa.control_output_ports != NULL)
    {
      free (x->plugin.ladspa.control_output_ports);
      x->plugin.ladspa.control_output_ports = NULL;
    }
}

static int plugin_tilde_have_ladspa_plugin(Pd_Plugin_Tilde* x) {
  if(NULL==x->plugin.ladspa.instance) {
    error("plugin~: LADSPA instance not found");
    return 0;
  }

  if(NULL==x->plugin.ladspa.type) {
    error("plugin~: LADSPA type not found");
    return 0;
  }

  return 1;
}

static int plugin_tilde_have_plugin(Pd_Plugin_Tilde* x) {
  if(NULL==x)return 0;

  if (NULL==x->plugin_library || x->plugin_library_filename == NULL) {
    error("plugin~: plugin not found");
    return 0;
  }

  if(NULL==x->plugin.ladspa.type) {
    error("plugin~: unable to determine LADSPA type");
    return 0;
  }

  return  plugin_tilde_have_ladspa_plugin(x);

}
