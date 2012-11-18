//#=====================================================================================
//#
//#       Filename:  main.cpp
//#
//#    Description:  fluid~ - external for PD and Max/MSP
//#                           useless without the cool fluidsynth from
//#                           www.fluidsynth.org
//#
//#        Version:  1.0
//#        Created:  04/04/03
//#       Revision:  none
//#
//#         Author:  Frank Barknecht  (fbar)
//#          Email:  fbar@footils.org
//#      Copyright:  Frank Barknecht , 2003
//#
//#
//#        This program is free software; you can redistribute it and/or modify
//#    it under the terms of the GNU General Public License as published by
//#    the Free Software Foundation; either version 2 of the License, or
//#    (at your option) any later version.
//#
//#    This program is distributed in the hope that it will be useful,
//#    but WITHOUT ANY WARRANTY; without even the implied warranty of
//#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#    GNU General Public License for more details.
//#
//#    You should have received a copy of the GNU General Public License
//#    along with this program; if not, write to the Free Software
//#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//#
//#=====================================================================================


#include <flext.h>
#include <fluidsynth.h>

// check flext version (actually this runs with flext since 0.3, I guess, but
// better be sure...
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


// A flext dsp external ("tilde object") inherits from the class flext_dsp 
class fluid: 
	public flext_dsp
{
	FLEXT_HEADER_S(fluid, flext_dsp, setup)

	public:
		fluid(int argc, t_atom *argv) 
        : synth(NULL)
		{
			AddInAnything();         // slurp anything
			AddOutSignal(2);         // 2 audio out [ == AddOutSignal(2) ]
			fluid::fluid_init(argc, argv);
	
		} // end of constructor
		~fluid()
		{
			if ( synth != NULL )
			        delete_fluid_synth(synth);
		}
	
	
		virtual void m_help()
		{
			const char * helptext = 
			"_ __fluid~_ _  a soundfont external for Pd and Max/MSP \n"
			"_ argument: \"/path/to/soundfont.sf\" to load on object creation\n"
			"_ messages: \n"
			"load /path/to/soundfont.sf2  --- Loads a Soundfont \n"
			"note 0 0 0                   --- Play note. Arguments: \n"
			"                                 channel-# note-#  veloc-#\n"
			"n 0 0 0                      --- Play note, same as above\n"
			"0 0 0                        --- Play note, same as above\n"
			"control 0 0 0                --- set controller\n"
			"c       0 0 0                --- set controller, shortcut\n"
			"prog 0 0                     --- progam change, \n"
			"                                 args: channel-# prog-#\n"
			"p    0 0                     --- program change, shortcut\n"

			;
			post("%s", helptext);

		}
	protected:

        static void setup(t_classid c)
        {
			FLEXT_CADDMETHOD_(c,0,"init",  fluid_init);
        	FLEXT_CADDMETHOD_(c,0,"load", fluid_load);
			FLEXT_CADDMETHOD_(c,0,"note", fluid_note);
			FLEXT_CADDMETHOD_(c,0,"prog", fluid_program_change);
			FLEXT_CADDMETHOD_(c,0,"control", fluid_control_change);
			FLEXT_CADDMETHOD_(c,0,"bend", fluid_pitch_bend);
			FLEXT_CADDMETHOD_(c,0,"bank",  fluid_bank);
			FLEXT_CADDMETHOD_(c,0,"gen",  fluid_gen);
			
			// list input calls fluid_note(...)
			FLEXT_CADDMETHOD_(c,0, "list",  fluid_note);
			
			// some alias shortcuts:
			FLEXT_CADDMETHOD_(c,0,"n",  fluid_note); 
			FLEXT_CADDMETHOD_(c,0,"p",  fluid_program_change);
			FLEXT_CADDMETHOD_(c,0,"c",  fluid_control_change);
			FLEXT_CADDMETHOD_(c,0,"cc", fluid_control_change);
			FLEXT_CADDMETHOD_(c,0,"b",  fluid_pitch_bend);
        }

		// here we declare the virtual DSP function
		virtual void m_signal(int n, float *const *in, float *const *out);
		
	private:	
		fluid_synth_t *synth;
		
		FLEXT_CALLBACK_V(fluid_load)
		void fluid_load(int argc, t_atom *argv);
		
		FLEXT_CALLBACK_V(fluid_note)
		void fluid_note(int argc, t_atom *argv);
		
		FLEXT_CALLBACK_V(fluid_program_change)
		void fluid_program_change(int argc, t_atom *argv);
		
		FLEXT_CALLBACK_V(fluid_control_change)
		void fluid_control_change(int argc, t_atom *argv);
		
		FLEXT_CALLBACK_V(fluid_pitch_bend)
		void fluid_pitch_bend(int argc, t_atom *argv);
		
		FLEXT_CALLBACK_V(fluid_bank)
		void fluid_bank(int argc, t_atom *argv);
		
        FLEXT_CALLBACK_V(fluid_gen)
		void fluid_gen(int argc, t_atom *argv);
		
		FLEXT_CALLBACK_V(fluid_init)
		void fluid_init(int argc, t_atom *argv);

}; // end of class declaration for fluid


// Before we can run our fluid-class in PD, the object has to be registered as a
// PD object. Otherwise it would be a simple C++-class, and what good would
// that be for?  Registering is made easy with the FLEXT_NEW_* macros defined
// in flext.h. 

FLEXT_NEW_DSP_V("fluid~", fluid)


void fluid::fluid_load(int argc, t_atom *argv)
{
	if (synth == NULL) 
	{
		post("No fluidsynth, duh");
		return;
	}
	
	if (argc >= 1 && IsSymbol(argv[0]))	
	{
		const char* filename = GetString(argv[0]);
		if ( fluid_synth_sfload(synth, filename, 0) >= 0)
		{
			post("Loaded Soundfont: %s", filename);
			fluid_synth_program_reset(synth);
		}
	}
}

void fluid::fluid_note(int argc, t_atom *argv)
{
	if (synth == NULL) return;
	if (argc == 3)
	{
		int   chan, key, vel;
		chan  = GetAInt(argv[0]);
		key   = GetAInt(argv[1]);
		vel   = GetAInt(argv[2]); 
		fluid_synth_noteon(synth,chan-1,key,vel);
	}
}

void fluid::fluid_program_change(int argc, t_atom *argv)
{	
	if (synth == NULL) return;
	if (argc == 2)
	{
		int   chan, prog;
		chan  = GetAInt(argv[0]);
		prog  = GetAInt(argv[1]);
		fluid_synth_program_change(synth,chan-1,prog);
	}
}

void fluid::fluid_control_change(int argc, t_atom *argv)
{
	if (synth == NULL) return;
	if (argc == 3)
	{
		int   chan, ctrl, val;
		chan  = GetAInt(argv[0]);
		ctrl  = GetAInt(argv[1]);
		val   = GetAInt(argv[2]);
		fluid_synth_cc(synth,chan-1,ctrl,val);
	}
}

void fluid::fluid_pitch_bend(int argc, t_atom *argv)
{
	if (synth == NULL) return;
	if (argc == 2)
	{
		int   chan, val;
		chan  = GetAInt(argv[0]);
		val   = GetAInt(argv[1]);
		fluid_synth_pitch_bend(synth, chan-1, val);
	}
}

void fluid::fluid_bank(int argc, t_atom *argv)
{
	if (synth == NULL) return;
	if (argc == 2)
	{	
		int   chan, bank;
		chan  = GetAInt(argv[0]);
		bank  = GetAInt(argv[1]);
		fluid_synth_bank_select(synth, chan-1, bank);
	}
}

void fluid::fluid_gen(int argc, t_atom *argv)
{
	if (synth == NULL) return;
	if (argc == 3)
	{	
		int   chan, param;
        float value;
        chan  = GetAInt(argv[0]);
		param = GetAInt(argv[1]);
		value = GetAFloat(argv[2]);
		fluid_synth_set_gen(synth, chan-1, param, value);
	}
}


void fluid::fluid_init(int argc, t_atom *argv)
{
	if (synth != NULL) 
		delete_fluid_synth(synth);

	float sr=Samplerate();
	
//	if (sr != 44100.f) 
//	{
//		post("Current samplerate %.0f != 44100", sr);
//		// post("WARNING: fluid~ might be out of tune!");
//	}
	
	fluid_settings_t * settings = NULL;
	settings = new_fluid_settings();
	
	// fluid_settings_setstr(settings, "audio.driver", "float");
	
	// settings:
	fluid_settings_setnum(settings, "synth.midi-channels", 16);
	fluid_settings_setnum(settings, "synth.polyphony", 256);
	fluid_settings_setnum(settings, "synth.gain", 0.600000);
	fluid_settings_setnum(settings, "synth.sample-rate", 44100.000000);
	fluid_settings_setstr(settings, "synth.chorus.active", "no");
	fluid_settings_setstr(settings, "synth.reverb.active", "no");
	fluid_settings_setstr(settings, "synth.ladspa.active", "no");

	if (sr != 0)
	{
		fluid_settings_setnum(settings, "synth.sample-rate", sr);
	}
	

	// Create fluidsynth instance:
	synth = new_fluid_synth(settings);
	
	if ( synth == NULL )
	{
			post("fluid~: couldn't create synth\n");
	}
	
	// try to load argument as soundfont
	fluid_load(argc, argv);


	if (settings != NULL )
		delete_fluid_settings(settings);
	
	// We're done constructing:
	if (synth)
		post("-- fluid~ with flext ---");

}


// Now we define our DSP function. It gets this arguments:
// 
// int n: length of signal vector. Loop over this for your signal processing.
// float *const *in, float *const *out: 
//          These are arrays of the signals in the objects signal inlets rsp.
//          oulets. We come to that later inside the function.

void fluid::m_signal(int n, float *const *in, float *const *out)
{
	
	if (synth == NULL) return;
	
	float *left  = out[0];
	float *right = out[1];
    // Did I put this in???
    //	left[0] = 1;
    //	right[0] = 1;
	fluid_synth_write_float(synth, n, left, 0, 1, right, 0, 1); 
	
}  // end m_signal
