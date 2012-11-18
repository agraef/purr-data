/*

   k_vst~, a Pd tilde object for hosting VST plug-ins.

   This is really just the plugin~ source made by Jarno Seppänen,
   but with a few lines changed (very few that is) to make it
   work with vst-plugins using the vstlib.

   The name was changed from plugin~ to k_vst~ to avoid nameclash
   with the plugin~ object running ladspa plugins.

   This object is for i386 non-windows (ie. linux/freebsd) only.

   Copyright (C) 2002 Kjetil S. Matheussen / Notam,
   k.s.matheussen@notam02.no

   VST program change code made by Andrew C. Bulhak (acb at dev.null.org)

   MIDI code by Thomas Charbonnel <thomas@undata.org>.

   Code to allow space in dll names also made by Thomas Charbonnel <thomas@undata.org>.


------------------

   plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2000 Jarno Seppänen
   $Id: k_vst~.c,v 1.1 2004-01-08 14:55:24 ksvalast Exp $

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

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plugin~.h"
#include "plugin~_ladspa.h"
#include "plugin~_vst.h"
#include "version.h"

#define AEFFECTX_H_LINUXWORKAROUND
#include "vst/aeffectx.h"

#if PLUGIN_TILDE_USE_LADSPA
#define PLUGIN_TILDE_BRAND "LADSPA"
#endif
#if PLUGIN_TILDE_USE_VST
#define PLUGIN_TILDE_BRAND "VST"
#endif

static t_class* plugin_tilde_class = NULL;


static char valid_channel (float ch) {
  if (ch < 1.) {
    return 0;
  } else if (ch > 16.) {
    return 15;
  } else {
    return (char)(ch-1);
  }
}

static void send_midi_to_plugin (
				    Pd_Plugin_Tilde* x,
				    char data0,
				    char data1,
				    char data2
				)
{
  struct VstMidiEvent das_event;
  struct VstMidiEvent *pevent=&das_event;

  struct VstEvents events;

  pevent->type = kVstMidiType;
  pevent->byteSize = 24;
  pevent->deltaFrames = 0;
  pevent->flags = 0;
  pevent->detune = 0;
  pevent->noteLength = 0;
  pevent->noteOffset = 0;
  pevent->reserved1 = 0;
  pevent->reserved2 = 0;
  pevent->noteOffVelocity = 0;
  pevent->midiData[0] = data0;
  pevent->midiData[1] = data1;
  pevent->midiData[2] = data2;
  pevent->midiData[3] = 0;


  events.numEvents = 1;
  events.reserved  = 0;
  events.events[0]=(VstEvent*)pevent;
  
  x->plugin.vst.instance->dispatcher(
				     x->plugin.vst.instance,
				     effProcessEvents, 0, 0, &events, 0.0f
				     );
 
}

static void plugin_tilde_ctl (
				 Pd_Plugin_Tilde* x,
				 t_float ctlnum,
				 t_float ctlval,
				 t_float ctlchan
				 )
{
  send_midi_to_plugin(x, 0xb0 | valid_channel(ctlchan), (char)ctlnum, (char)ctlval);  
}

static void plugin_tilde_pitchbend (
				 Pd_Plugin_Tilde* x,
				 t_float pitchvalue,
				 t_float pitchchan
				 )
{
  send_midi_to_plugin(x, 0xe0 | valid_channel(pitchchan), (((int)pitchvalue)>>7) & 127, (int)pitchvalue & 127);  
}

static void plugin_tilde_aftertouch (
				 Pd_Plugin_Tilde* x,
				 t_float atvalue,
				 t_float atchan
				 )
{
  send_midi_to_plugin(x, 0xa0 | valid_channel(atchan), (char)atvalue, 0);  
}



static void plugin_tilde_prg (
				 Pd_Plugin_Tilde* x,
				 t_float prgnum,
				 t_float prgchan
				 )
{
  send_midi_to_plugin(x, 0xc0 | valid_channel(prgchan), (char)prgnum, 0);  
}



static void plugin_tilde_noteon (
				 Pd_Plugin_Tilde* x,
				 t_float notenum,
				 t_float notevel,
				 t_float notechan
				 )
{
  send_midi_to_plugin(x, 0x90 | valid_channel(notechan), (char)notenum, (char)notevel);
}

static void plugin_tilde_noteoff(
				 Pd_Plugin_Tilde* x,
				 t_float notenum,
				 t_float notechan
				 )
{
  send_midi_to_plugin(x, 0x90 | valid_channel(notechan), (char)notenum, 0);
}


void
k_vst_tilde_setup (void)
{
    /* Make a new Pd class with 2 string creation parameters */
    plugin_tilde_class = class_new (gensym ("k_vst~"),
				    (t_newmethod)plugin_tilde_new,
				    (t_method)plugin_tilde_free,
				    sizeof (Pd_Plugin_Tilde),
				    0,
				    A_GIMME, 0);
    assert (plugin_tilde_class != NULL);

    /* Let's be explicit in not converting the signals in any way */
    assert (sizeof (float) == sizeof (t_float));
#if PLUGIN_TILDE_USE_LADSPA
    assert (sizeof (float) == sizeof (LADSPA_Data));
#endif

    /* Set the callback for DSP events; this is a standard Pd message */
    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_dsp,
		     gensym ("dsp"),
		     0);

    /* Set the callback for "control" messages in the first inlet;
       this is a message of our own for changing LADSPA control
       ports/VST parameters */
    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_ctl,
		     gensym ("ctl"),
		     A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_pitchbend,
		     gensym ("pitchbend"),
		     A_DEFFLOAT, A_DEFFLOAT, 0);

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_aftertouch,
		     gensym ("aftertouch"),
		     A_DEFFLOAT, A_DEFFLOAT, 0);

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_prg,
		     gensym ("prg"),
		     A_DEFFLOAT, A_DEFFLOAT, 0);


    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_noteon,
		     gensym ("noteon"),
		     A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_noteoff,
		     gensym ("noteoff"),
		     A_DEFFLOAT, A_DEFFLOAT, 0);

    /* Set the callback for "control" messages in the first inlet;
       this is a message of our own for changing LADSPA control
       ports/VST parameters */
    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_control,
		     gensym ("control"),
		     A_DEFSYM, A_DEFFLOAT, 0);

    /* Register a callback for "print" messages in the first inlet;
       this is a message for printing information on the plug-in */
    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_print,
		     gensym ("print"),
		     0);

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_opengui,
		     gensym ("opengui"),
		     0);

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_closegui,
		     gensym ("closegui"),
		     0);

    /* Register a callback for "reset" messages in the first inlet;
       this is a message for resetting plug-in state */
    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_reset,
		     gensym ("reset"),
		     0);

    /* Register a callback for setting the program */

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_program,
		     gensym ("program"),
		     A_DEFFLOAT, 0);

    class_addmethod (plugin_tilde_class,
		     (t_method)plugin_tilde_programname,
		     gensym ("programname"),
		     A_DEFSYM, 0);

    /* We have to make a "null" callback for signal input to the first
       inlet or otherwise Pd'll gracefully fuck the inlets up */
    class_addmethod (plugin_tilde_class,
		     nullfn,
		     gensym ("signal"),
		     0);
}

static void*
plugin_tilde_new (t_symbol* s_name, int argc, t_atom *argv)
{
    char buf[255];
    char name[255];
    int z;
    int offset = 0;
    Pd_Plugin_Tilde* x = NULL;
    unsigned i = 0;

    /* Allocate object struct */
    x = (Pd_Plugin_Tilde*)pd_new (plugin_tilde_class);
    assert (x != NULL);

    /* Initialize object struct */
    x->plugin_library = NULL;
    x->plugin_library_filename = NULL;
    x->num_audio_inputs = 0;
    x->num_audio_outputs = 0;
    x->num_control_inputs = 0;
    x->num_control_outputs = 0;
    x->audio_inlets = NULL;
    x->audio_outlets = NULL;
    x->control_outlet = NULL;
    x->dsp_vec = NULL;
    x->dsp_vec_length = 0;

    for (z = 0; z < argc; z++) {
	atom_string(&argv[z], buf, 255);
	if (z == 0)
	{
	    snprintf(&name[offset], 255, "%s", buf);
	    offset += strnlen(buf, 255); 
	} else {
	    if (255-offset > 0) {
		snprintf(&name[offset], 255-offset, " %s", buf);
		offset += strnlen(buf, 255);
	    }
	}
    }
    
    /* Construct the clock */
    x->x_clock = clock_new (x, (t_method)plugin_tilde_tick);
    assert (x->x_clock != NULL);

#if PLUGIN_TILDE_USE_LADSPA
    assert (&name[0] != NULL);
    if (&name[0] == NULL || strlen (&name[0]) == 0) {
	/* Search for the plugin library */
	x->plugin_library_filename = plugin_tilde_search_plugin (x, &name[0]);
	if (x->plugin_library_filename == NULL) {
	    error ("plugin~: " PLUGIN_TILDE_BRAND " plugin not found in any library");
	    goto PLUGIN_TILDE_NEW_RETURN_NULL;
	}
    }
    else {
	/* Search in the given plugin library */
	x->plugin_library_filename = strdup (&name[0]);
    }
#endif /* PLUGIN_TILDE_USE_LADSPA */
#if PLUGIN_TILDE_USE_VST
    /* Remember plugin library filename */
    x->plugin_library_filename = strdup (&name[0]);
#endif /* PLUGIN_TILDE_USE_VST */

    /* Load LADSPA/VST plugin */
    if (plugin_tilde_open_plugin (x,
				  &name[0],
				  x->plugin_library_filename,
				  (unsigned long)sys_getsr ())) {
	error ("plugin~: Unable to open " PLUGIN_TILDE_BRAND " plugin");
	goto PLUGIN_TILDE_NEW_RETURN_NULL;
    }

    /* Start the clock (used for plug-in GUI update) */
    plugin_tilde_tick (x);

    /* Create in- and outlet(s) */

    /* Allocate memory for in- and outlet pointers */
    x->audio_inlets = (t_inlet**)calloc (x->num_audio_inputs, sizeof (t_inlet*));
    x->audio_outlets = (t_outlet**)calloc (x->num_audio_outputs, sizeof (t_outlet*));
    assert (x->audio_inlets != NULL && x->audio_outlets != NULL);

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
    assert (x->dsp_vec != NULL);

    return x;

    /* erroneous returns */
 PLUGIN_TILDE_NEW_RETURN_NULL:
    if (x->plugin_library_filename != NULL) {
	free ((void*)x->plugin_library_filename);
	x->plugin_library_filename = NULL;
    }
    if (x->x_clock != NULL) {
	clock_free (x->x_clock);
	x->x_clock = NULL;
    }
    return NULL; /* Indicate error to Pd */
}

static void
plugin_tilde_free (Pd_Plugin_Tilde* x)
{
    unsigned i = 0;

    /* precondition(s) */
    assert (x != NULL);

    /* Stop and destruct the clock */
    clock_unset (x->x_clock);
    clock_free (x->x_clock);
    x->x_clock = NULL;

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

static void
plugin_tilde_tick (Pd_Plugin_Tilde* x)
{
    /* precondition(s) */
    assert (x != NULL);

    /* Issue a GUI update (FIXME should use separate GUI thread) */
    plugin_tilde_update_gui (x);

    /* Schedule next update */
    clock_delay (x->x_clock, 100); /* FIXME period OK? */
}

static void
plugin_tilde_dsp (Pd_Plugin_Tilde* x, t_signal** sp)
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

static t_int*
plugin_tilde_perform (t_int* w)
{
    unsigned i = 0;
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

void
plugin_tilde_emit_control_output (Pd_Plugin_Tilde* x,
				  const char* name,
				  float new_value)
{
    /* Construct and outlet a "control" message with two Pd atoms */
    t_atom anything_atoms[2];
    anything_atoms[0].a_type = A_SYMBOL;
    anything_atoms[0].a_w.w_symbol = gensym ((char*)name);
    anything_atoms[1].a_type = A_FLOAT;
    anything_atoms[1].a_w.w_float = new_value;
    outlet_anything (x->control_outlet, gensym ("control"), 2, anything_atoms);
}

static void
plugin_tilde_control (Pd_Plugin_Tilde* x,
		      t_symbol* ctrl_name,
		      t_float ctrl_value)
     /* Change the value of a named control port of the plug-in */
{
    unsigned parm_num = 0;

    /* precondition(s) */
    assert (x != NULL);
    /* FIXME we assert that the plug-in is already properly opened */
    if (ctrl_name->s_name == NULL || strlen (ctrl_name->s_name) == 0) {
	error ("plugin~: control messages must have a name and a value");
	return;
    }
    if (ctrl_name->s_name[0]=='/'){
      parm_num = atoi(ctrl_name->s_name+1);
    }else{
      parm_num = plugin_tilde_get_parm_number (x, ctrl_name->s_name);
    }
    if (parm_num) {
	plugin_tilde_set_control_input_by_index (x, parm_num - 1, ctrl_value);
    }
    else {
	plugin_tilde_set_control_input_by_name (x, ctrl_name->s_name, ctrl_value);
    }
}

static void plugin_tilde_opengui (Pd_Plugin_Tilde* x){
#if PLUGIN_TILDE_USE_VST
  plugin_tilde_vst_open_editor(x);
#endif
}

static void plugin_tilde_closegui (Pd_Plugin_Tilde* x){
#if PLUGIN_TILDE_USE_VST
  plugin_tilde_vst_close_editor(x);
#endif
}

static void
plugin_tilde_print (Pd_Plugin_Tilde* x)
     /* Print plug-in name, port names and other information */

/*
stereo_amp: "Stereo amplifier"; control 1 in/0 out; audio 2 in/2 out
Control inputs:
Control outputs:
Audio inputs:
Audio outputs:
 */
{
    /* precondition(s) */
    assert (x != NULL);

#if 1
    printf("This is k_vst~ version %s. Made by Kjetil S. Matheussen, but ~99.9 percent based on code\n",PLUGIN_TILDE_VERSION);
    printf("written by Jarno Seppänen.\n\n");
#else
    printf ("This is plugin~ version %s -- NO WARRANTY -- Copyright (C) 2000 Jarno Seppänen\n",
	    PLUGIN_TILDE_VERSION);
#endif
#if PLUGIN_TILDE_USE_LADSPA
    plugin_tilde_ladspa_print (x);
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_print (x);
#endif
}

static void
plugin_tilde_reset (Pd_Plugin_Tilde* x)
{
    /* precondition(s) */
    assert (x != NULL);
#if PLUGIN_TILDE_USE_LADSPA
    plugin_tilde_ladspa_reset (x);
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_reset (x);
#endif
}

static unsigned
plugin_tilde_get_parm_number (Pd_Plugin_Tilde* x,
			      const char* str)
/* find out if str points to a parameter number or not and return the
   number or zero.  The number string has to begin with a '#' character */
{
    long num = 0;
    char* strend = NULL;
    
    assert (x != NULL);
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

static const char*
plugin_tilde_search_plugin (Pd_Plugin_Tilde* x,
			    const char* name)
{
#if PLUGIN_TILDE_USE_LADSPA
    return plugin_tilde_ladspa_search_plugin (x, name);
#endif
#if PLUGIN_TILDE_USE_VST
    return plugin_tilde_vst_search_plugin (x, name);
#endif
}

static int
plugin_tilde_open_plugin (Pd_Plugin_Tilde* x,
			  const char* name,
			  const char* lib_name,
			  unsigned long sample_rate)
{
    int ret = 0;

#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~: open_plugin (x, \"%s\", \"%s\", %ld);",
	   name, lib_name, sample_rate);
#endif

#if PLUGIN_TILDE_USE_LADSPA
    ret = plugin_tilde_ladspa_open_plugin (x, name, lib_name, sample_rate);
#endif
#if PLUGIN_TILDE_USE_VST
    ret = plugin_tilde_vst_open_plugin (x, name, lib_name, sample_rate);
#endif

#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~: plugin active");
#endif

#if PLUGIN_TILDE_VERBOSE
    plugin_tilde_print (x);
#endif
    return ret;
}

static void
plugin_tilde_close_plugin (Pd_Plugin_Tilde* x)
{
#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~: close_plugin (x)");
#endif

#if PLUGIN_TILDE_USE_LADSPA
    plugin_tilde_ladspa_close_plugin (x);
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_close_plugin (x);
#endif

#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~: destructed plugin successfully");
#endif
}

static void
plugin_tilde_apply_plugin (Pd_Plugin_Tilde* x)
{
#if PLUGIN_TILDE_USE_LADSPA
    plugin_tilde_ladspa_apply_plugin (x);
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_apply_plugin (x);
#endif
}

static void
plugin_tilde_connect_audio (Pd_Plugin_Tilde* x,
			    float** audio_inputs,
			    float** audio_outputs,
			    unsigned long num_samples)
{
#if PLUGIN_TILDE_USE_LADSPA
    plugin_tilde_ladspa_connect_audio (x, audio_inputs, audio_outputs,
				       num_samples);
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_connect_audio (x, audio_inputs, audio_outputs,
				    num_samples);
#endif
}

static void
plugin_tilde_set_control_input_by_name (Pd_Plugin_Tilde* x,
					const char* name,
					float value)
{
#if PLUGIN_TILDE_USE_LADSPA
    plugin_tilde_ladspa_set_control_input_by_name (x, name, value);
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_set_control_input_by_name (x, name, value);
#endif
}

static void
plugin_tilde_set_control_input_by_index (Pd_Plugin_Tilde* x,
					 unsigned index_,
					 float value)
/* plugin~.c:535: warning: declaration of `index' shadows global declaration */
{
#if PLUGIN_TILDE_USE_LADSPA
    plugin_tilde_ladspa_set_control_input_by_index (x, index_, value);
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_set_control_input_by_index (x, index_, value);
#endif
}

static void
plugin_tilde_update_gui (Pd_Plugin_Tilde* x)
{
#if PLUGIN_TILDE_USE_LADSPA
    /* FIXME LADSPA doesn't support GUI's at the moment */
#endif
#if PLUGIN_TILDE_USE_VST
    plugin_tilde_vst_update_gui (x);
#endif
}

static void
plugin_tilde_program (Pd_Plugin_Tilde* x, t_float prog_num)
	/* set the plugin's controls to one of its presets */
{
#if PLUGIN_TILDE_USE_LADSPA
#endif
#if PLUGIN_TILDE_USE_VST
	plugin_tilde_vst_set_program(x, (long) prog_num);
#endif
}

static void
plugin_tilde_programname (Pd_Plugin_Tilde* x, t_symbol* progname)
	/* set the name of the current program in the plugin */
{
#if PLUGIN_TILDE_USE_LADSPA
#endif
#if PLUGIN_TILDE_USE_VST
    if (progname->s_name == NULL || strlen (progname->s_name) == 0) {
	error ("plugin~: control messages must have a name and a value");
	return;
    }
    plugin_tilde_vst_set_program_name(x, progname->s_name);
#endif
}


/* EOF */
