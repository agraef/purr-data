/* plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2000 Jarno Seppänen
   $Id: plugin~_vst.c,v 1.1 2004-01-08 14:55:24 ksvalast Exp $

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
#if PLUGIN_TILDE_USE_VST

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plugin~.h"
#include "plugin~_vst.h"
/* VST header */
#define AEFFECTX_H_LINUXWORKAROUND
#include "vst/aeffectx.h"
//#include "vst/AEffect.h"
/* VST dll helper functions */
#include "vstutils.h"
#ifdef WIN32
#include "win/vitunmsvc.h" /* strncasecmp() */
#include <windows.h> /* GetForegroundWindow() */
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


const char*
plugin_tilde_vst_search_plugin (Pd_Plugin_Tilde* x,
				const char* name)
{
    /* searching through VST libraries not supported */
    error ("plugin~: warning: searching through VST libraries not supported");
    return NULL;
}

int
plugin_tilde_vst_open_plugin (Pd_Plugin_Tilde* x,
			      const char* name,
			      const char* lib_name,
			      unsigned long sample_rate)
{
    unsigned port_index;

    /* precondition(s) */
    assert (x != NULL);
    /* name is unused */
    assert (lib_name != NULL);
    assert (sample_rate != 0);

    /* Initialize object struct */
    x->plugin.vst.instance = NULL;
    x->plugin.vst.audio_inputs = NULL;
    x->plugin.vst.audio_outputs = NULL;
    x->plugin.vst.num_samples = 0;
    x->plugin.vst.editor_open = 0;

#if 0
    /* Attempt to load the plugin. */
    x->plugin_library = vstutils_load_vst_plugin_dll (lib_name);
    if (x->plugin_library == NULL)
    {
	error ("plugin~: Unable to load VST plugin library \"%s\"",
	       lib_name);
	return 1;
    }
#endif

    /* Construct the plugin.  This is supposed to call
       the AudioEffect::AudioEffect() ctor eventually */
    x->plugin.vst.instance
#if 1
      =VSTLIB_new((char*)lib_name);
#else
	= vstutils_init_vst_plugin (x->plugin_library,
				    lib_name,
				    plugin_tilde_vst_audioMaster);
#endif
    if (x->plugin.vst.instance == NULL) {
	error ("plugin~: Unable to instantiate VST plugin from library \"%s\"",
	       lib_name);
	return 1;
    }

    /* Stuff Pd_Plugin_Tilde* x into user field of AEffect for audioMaster() to use */
    x->plugin.vst.instance->user = x;

    /* Call plugin open() (through dispatcher()) in order to ensure
       plugin is properly constructed */
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effOpen,
					0, 0, NULL, 0);


#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~: constructed VST plugin \"%s\" successfully",
	   lib_name);
#endif


    /* Check another strange id */
    if (x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effIdentify,
					    0, 0, NULL, 0)
	!= 'NvEf') {
	error ("plugin~_vst: warning: VST plugin malfunction (effIdentify != 'NvEf')");
    }


    /* Tell the sample rate and frame length to the VST plug-in */
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effSetSampleRate,
					0, 0, NULL, (float)sample_rate);

    /* FIXME just give some value since it will be changed later */
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effSetBlockSize,
					0, 16, NULL, 0);
    /* Find out the number of inputs and outputs needed (all VST
       parameters can be automated i.e. output if the plug-in so wants
       and the GUI can be operated). */
    x->num_audio_inputs = x->plugin.vst.instance->numInputs;
    x->num_audio_outputs = x->plugin.vst.instance->numOutputs;
    x->num_control_inputs = x->plugin.vst.instance->numParams;
    x->num_control_outputs = x->plugin.vst.instance->numParams;

    /* Make sure that processReplacing() is implemented */
    if (x->plugin.vst.instance->flags & effFlagsCanReplacing == 0) {
	error ("plugin~_vst: sorry, this VST plug-in type isn't supported (processReplacing() not implemented)");
	return 1;
    }


    /* Activate the plugin. */
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effMainsChanged,
					0, 1, NULL, 0);


    /* Finally, attempt open editor GUI if available */
    //  plugin_tilde_vst_open_editor (x);


    /* Make sure the user data still is there */
    assert (x->plugin.vst.instance->user == x);

    /* success */
    return 0;
}

void
plugin_tilde_vst_close_plugin (Pd_Plugin_Tilde* x)
{
    /* precondition(s) */
    assert (x != NULL);

    /* Attempt to close editor GUI */
    plugin_tilde_vst_close_editor (x);

    if (x->plugin.vst.instance != NULL)
    {
	/* Deactivate the plugin. */
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effMainsChanged,
					    0, 0, NULL, 0);

	/* Destruct the plugin.  This is supposed to translate to
	   a call to the AudioEffect::~AudioEffect() dtor */
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effClose,
					    0, 0, NULL, 0);
#if 1
	VSTLIB_delete(x->plugin.vst.instance);
#endif
	x->plugin.vst.instance = NULL;
    }


    if (x->plugin_library != NULL)
    {
#if 0
	vstutils_unload_vst_plugin_dll (x->plugin_library);
#endif
	x->plugin_library = NULL;
    }
}

void
plugin_tilde_vst_apply_plugin (Pd_Plugin_Tilde* x)
{
  static int agurk=0;

    /* Run the plugin on Pd's buffers */
    /* FIXME need to implement out-of-place buffers and zero them here
       if processReplacing() isn't implemented */

    x->plugin.vst.instance->processReplacing (x->plugin.vst.instance,
					      x->plugin.vst.audio_inputs,
					      x->plugin.vst.audio_outputs,
					      x->plugin.vst.num_samples);

}

void
plugin_tilde_vst_print (Pd_Plugin_Tilde* x)
{
    unsigned i;
    char display[25];
    long l, nprog;

    printf ("control %d in/out; audio %d in/%d out\n"
	    "Loaded from library \"%s\".\n",
	    x->num_control_inputs,
	    x->num_audio_inputs,
	    x->num_audio_outputs,
	    x->plugin_library_filename);

    /* print the current program name/number */
    l = x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
		    effGetProgram, 0, 0, display, 0);
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
		    effGetProgramName, 0, 0, display, 0);
    nprog = x->plugin.vst.instance->numPrograms;
    printf("Program #%ld (of %ld), \"%s\".\n",l,nprog,display);

    for (i = 0; i < x->num_control_inputs; i++) {
	/* the Steinberg(tm) way... */
	char name[9];
	char label[9];
	if (i == 0) {
	    printf ("Control input/output(s):\n");
	}
	memset (name, 0, 9);
	memset (display, 0, 25);
	memset (label, 0, 9);
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effGetParamName,
					    i, 0, name, 0);
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effGetParamDisplay,
					    i, 0, display, 0);
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effGetParamLabel,
					    i, 0, label, 0);
	printf (" #%d \"%s\" (%s %s)\n",
		i + 1, name, display, label);
    }
}

void
plugin_tilde_vst_reset (Pd_Plugin_Tilde* x)
{
    /* precondition(s) */
    assert (x != NULL);
    /* reset plug-in by first deactivating and then re-activating it */
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effMainsChanged,
					0, 0, NULL, 0);
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effMainsChanged,
					0, 1, NULL, 0);
}

void
plugin_tilde_vst_connect_audio (Pd_Plugin_Tilde* x,
				float** audio_inputs,
				float** audio_outputs,
				unsigned long num_samples)
{
    assert (x != NULL);
    assert (audio_inputs != NULL);
    assert (audio_outputs != NULL);
    x->plugin.vst.audio_inputs = audio_inputs;
    x->plugin.vst.audio_outputs = audio_outputs;
    x->plugin.vst.num_samples = num_samples;
    /* Tell the block size to the VST plug-in */
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effMainsChanged,
					0, 0, NULL, 0);
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effSetBlockSize,
					0, num_samples, NULL, 0);
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effMainsChanged,
					0, 1, NULL, 0);
}

void
plugin_tilde_vst_set_control_input_by_name (Pd_Plugin_Tilde* x,
				    const char* name,
				    float value)
{
    unsigned parm_index;
    int found_port; /* boolean */
    char parm_name[9]; /* the Steinberg(tm) way! */

    /* precondition(s) */
    assert (x != NULL);

    /* compare control name to VST parameters' names
       case-insensitively */
    found_port = 0;
    for (parm_index = 0; parm_index < x->num_control_inputs; parm_index++)
    {
	unsigned cmp_start, cmp_length;
	memset (parm_name, 0, 9);
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effGetParamName,
					    parm_index, 0, parm_name, 0);
	/* skip any initial whitespace */
	cmp_start = 0;
	while (parm_name[cmp_start] != 0
	       && parm_name[cmp_start] == ' ') {
	    cmp_start++;
	}
	cmp_length = MIN (strlen (name), strlen (&parm_name[cmp_start]));
	if (cmp_length != 0
	    && strncasecmp (name, &parm_name[cmp_start], cmp_length) == 0)
	{
	    /* found the first port to match */
	    found_port = 1;
	    break;
	}
    }

    if (!found_port)
    {
	error ("plugin~: VST plugin doesn't have a parameter named \"%s\"",
	       name);
	return;
    }

    plugin_tilde_vst_set_control_input_by_index (x,
						 parm_index,
						 value);
}

void
plugin_tilde_vst_set_control_input_by_index (Pd_Plugin_Tilde* x,
				       unsigned index_,
				       float value)
{
    char name[9]; /* the Steinberg(tm) way! */

    /* precondition(s) */
    assert (x != NULL);
    /* assert (index_ >= 0); causes a warning */
    assert (index_ < x->num_control_inputs);

    /* Limit parameter to range [0, 1] */
    if (value < 0.0 || value > 1.0) {
	if (value < 0.0) {
	    value = 0.0;
	}
	else {
	    value = 1.0;
	}
	error ("plugin~: warning: parameter limited to within [0, 1]");
    }
	
    /* set the appropriate control port value */
    x->plugin.vst.instance->setParameter (x->plugin.vst.instance,
					  index_,
					  value);

#if PLUGIN_TILDE_DEBUG
    memset (name, 0, 9);
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effGetParamName,
					index_, 0, name, 0);
    error ("DEBUG plugin~_vst: control change parameter #%ud: \"%s\" to value %f",
	   index_ + 1, name, value);
#endif
}

/*  change the plugin's program; takes a program index */
void
plugin_tilde_vst_set_program (Pd_Plugin_Tilde* x, long prog)
{
	long nprog = x->plugin.vst.instance->numPrograms;
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
			                    effSetProgram, 0, prog%nprog, 0, 0);
}

long
plugin_tilde_vst_get_program (Pd_Plugin_Tilde* x)
{
	return x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
			                           effGetProgram, 0, 0, 0, 0);
}

long
plugin_tilde_vst_get_num_of_programs (Pd_Plugin_Tilde* x)
{
	return x->plugin.vst.instance->numPrograms;
}

void
plugin_tilde_vst_set_program_name (Pd_Plugin_Tilde* x, const char* progname)
{
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
		effSetProgramName, 0, 0, (void*)progname, 0);
}

static long
plugin_tilde_vst_audioMaster (AEffect* effect,
			      long opcode,
			      long index,
			      long value,
			      void* ptr,
			      float opt)
{
    char param_name[9];

#if 0 /*PLUGIN_TILDE_DEBUG*/
    error ("DEBUG plugin~_vst: audioMaster(0x%p, %ld, %ld, %ld, 0x%p, %f)",
	   effect, opcode, index, value, ptr, opt);
#endif

    switch (opcode) {
	case audioMasterAutomate:
	    assert (effect != NULL);
	    assert (effect->user != NULL); /* this is Pd_Plugin_Tilde* */
	    effect->setParameter (effect, index, opt);
	    /* Send "control" messages from here */
	    memset (param_name, 0, 9);
	    effect->dispatcher (effect, effGetParamName, index, 0, param_name, 0);
	    plugin_tilde_emit_control_output (effect->user, param_name, opt);
	    return 0;
	    break;
	case audioMasterVersion:
	    return 1;
	    break;
	case audioMasterCurrentId:
	    return 0;
	    break;
	case audioMasterIdle:
	    effect->dispatcher (effect, effEditIdle, 0, 0, NULL, 0);
	    return 0;
	    break;
	case audioMasterPinConnected:
	    /* return 0="true" for all inquiries for now */
	    return 0;
	    break;
    }
#if 0 /*PLUGIN_TILDE_DEBUG*/
    error ("DEBUG plugin~_vst: warning: unsupported audioMaster opcode");
#endif
    return 0;
}

void plugin_tilde_vst_open_editor (Pd_Plugin_Tilde* x)
{
  x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
				      effEditOpen,
				      0, 0, NULL, 0);
  
#if 0 /* FIXME doesn't work */
#ifdef WIN32
    HWND parent;

    /* precondition(s) */
    assert (x != NULL);

#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~_vst: open_editor()");
#endif

    if ((x->plugin.vst.instance->flags & effFlagsHasEditor) == 0
	|| x->plugin.vst.editor_open == 1) {
	/* no editor or editor already open */
	return;
    }

    /* Hmph, don't know about the Pd window, so give the desktop as
       the parent to the plug-in; we could actually use
       GetForegroundWindow(), since the user is currently typing into
       the pd subpatch window */
    /*parent = GetDesktopWindow ();*/
    parent = GetForegroundWindow ();
    /*parent = NULL;*/

    /* Open the editor! */
    if (!x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					     effEditOpen,
					     0, 0, parent, 0)) {
	/* no luck error */
	error ("plugin~_vst: couldn't open editor");
    } else {
	x->plugin.vst.editor_open = 1;
    }
#endif /* WIN32 */
#endif /* FIXME doesn't work */
}

void
plugin_tilde_vst_close_editor (Pd_Plugin_Tilde* x)
{
  x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
				      effEditClose,
				      0, 0, NULL, 0);

#if 0 /* FIXME doesn't work */
#ifdef WIN32

    /* precondition(s) */
    assert (x != NULL);

#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~_vst: close_editor()");
#endif

    if ((x->plugin.vst.instance->flags & effFlagsHasEditor) == 0
	|| x->plugin.vst.editor_open == 0) {
	/* no editor or it isn't open */
	return;
    }

    /* Close the editor */
    x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					effEditClose,
					0, 0, NULL, 0);
    x->plugin.vst.editor_open = 0;

#endif /* WIN32 */
#endif /* FIXME doesn't work */
}

void
plugin_tilde_vst_update_gui (Pd_Plugin_Tilde* x)
{
#if 0 /* FIXME doesn't work */
#ifdef WIN32

    /* precondition(s) */
    assert (x != NULL);

#if PLUGIN_TILDE_DEBUG
    error ("DEBUG plugin~_vst: update_gui()");
#endif
    if (x->plugin.vst.editor_open) {
	x->plugin.vst.instance->dispatcher (x->plugin.vst.instance,
					    effEditIdle,
					    0, 0, NULL, 0);
    }
#endif /* WIN32 */
#endif /* FIXME doesn't work */
}

#endif /* PLUGIN_TILDE_USE_VST */

/* EOF */
