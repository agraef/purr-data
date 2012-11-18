/* rx7~ -- STK-based sitar synthesis
 * requires STK library
 * Copyleft 2001 Yves Degoyon.
 * Permission is granted to use this software for any purpose provided you
 * keep this copyright notice intact.
 *
 * THE AUTHOR AND HIS EXPLOITERS MAKE NO WARRANTY, EXPRESS OR IMPLIED,
 * IN CONNECTION WITH THIS SOFTWARE.
 *
*/

#include "m_pd.h"
#include "dx7_voice.h"
#include "open_bank.h"
#include "properties.h"
#include "midi_event.h"
#include "mixer.h"
#include "utilities.h"
#include <vector>

// Scaling factors for DSP function - 2^16 sounds best
const double scalefac=1.0/65536.0; 	// 1/2^16
//const double scalefac=1.0/131072.0; 	// 1/2^17 
//const double scalefac=1.0/262144.0; 	// 1/2^18
//const double scalefac=1.0/524288.0; 	// 1/2^19

typedef struct _rx7
{
	t_object x_obj;
	Mixer *x_mixer;
	Midi_Event *midi_event;
	t_float banksize;
} t_rx7;


//Midi_Event midi_event;

static t_class *rx7_class;

static void *rx7_new( void )
{
	t_rx7 *x = (t_rx7 *)pd_new(rx7_class);

	int sr ;
	x->banksize = 0;
	
	outlet_new(&x->x_obj, &s_signal);

	if ( (x->x_mixer = new Mixer) == NULL ) 
	{
		post( "Rx7~: Cannot create RX7 Mixer" );
		return NULL;
	} 
    	else if   ( (x->midi_event = new Midi_Event) == NULL )
    	{
		post("Rx7~: Cannot create Midi Event");
		return NULL;
	}
	else
	{
		x->midi_event->mixer = &(*x->x_mixer); 
		sr = (int) sys_getsr();
		Dx7_Voice::base_freq=sr;
		x->x_mixer->voice_count=32;
		x->x_mixer->voice->base_freq =  sr;
		x->x_mixer->set_mix_frequency(sr);
		x->x_mixer->set_mix_16bits(true);
		x->x_mixer->set_mix_stereo(false);
		
		post( "Rx7~: instrument created.");
		return (x);
	}

}


static void rx7_loadbank(t_rx7* x, t_symbol *s, int argc, t_atom *argv)
{
	
	if (x->x_mixer == NULL) return;
	
	t_symbol *bank;
	
	bank = atom_getsymbolarg(0, argc, argv);
	
	if (!*bank->s_name)
    		return;
        
	vector<Dx7_Voice::Data*> tmp_patch;
	int i;
	
	vector<Dx7_Voice::Data> instruments = Open_Bank::file(bank->s_name);
        int sizeOfInstr = instruments.size();
	
	
	if (tmp_patch.size() > 0)
	{
		// empty tmp patch vector
		freeVector(tmp_patch);
	}
	
	
	for (i=0;i<sizeOfInstr;i++)
	{
		// fill tmp_patch
		tmp_patch.push_back(&instruments[i]);
	}
	
	// copy tmp to bank 
	x->midi_event->bank[0].patch.swap(tmp_patch);
	
	// store bank size
	x->banksize = x->midi_event->bank[0].patch.size();
	
	post("Rx7~: Bank \"%s\" loaded.", *bank);
	//post("Rx7~: Size of Instrument is: %d",sizeOfInstr);
		
}

static void rx7_note(t_rx7 *x, t_symbol *s, int argc, t_atom *argv)
{
	if (x->banksize <= 0) 
	{
		post("Rx7~: No Bank loaded. Please load a bank");
		return;
	}
	int chan_f, key_f, vel_f;
	unsigned char chan, key, vel;
	
	key_f   =  (int) atom_getfloatarg(0, argc, argv);
	vel_f   =  (int) atom_getfloatarg(1, argc, argv);
	chan_f  =  (int) atom_getfloatarg(2, argc, argv);
	
	if (chan_f > 16 || chan_f < 1 )
	{
		post("Channel range exceeded: %d", chan_f);
		return;
	}
	
	chan = static_cast<char>(chan_f);
	key  = static_cast<char>(key_f);
	vel  = static_cast<char>(vel_f);
	

	if (x->midi_event)
	    {
		    if (vel_f != 0)
		    {
			x->midi_event->note_on(chan,key,vel);
			// post ("got NOTE_ON");
		    }
		    else
		    {
			x->midi_event->note_off(chan,key,vel);
			// post ("got NOTE_OFF");
		    }
	    }
	}


static void rx7_program_change(t_rx7 *x, t_symbol *s, int argc, t_atom *argv)
{
    	int chan_f, prog_f;
	unsigned char chan, prog;
	
	prog_f   =  (int) atom_getfloatarg(0, argc, argv);
	chan_f   =  (int) atom_getfloatarg(1, argc, argv);
	chan = static_cast<char>(chan_f);
	prog = static_cast<char>(prog_f);
    	
	if (chan_f > 16 || chan_f < 1)
	{
		post("Rx7~: Channel range exceeded: %d", chan_f);
		return;
	}	
	if ( prog_f >= x->banksize )
	{
		post("Rx7~: No bank data at %d", prog_f);
		return;
	}
	if (x->midi_event)
	    x->midi_event->program_change(chan,prog);
	post("Rx7~: Program Change:  \n\tchan %d \n\tprog %d", chan_f,prog_f);
	
}




static t_int *rx7_perform(t_int *w)
{
	t_rx7* x 	= (t_rx7*)(w[1]);
	t_float *out 	= (t_float *)(w[2]);
	int n 		= (int)(w[3]);
	int i 		= 0;
	Sint32 thing 	= 0;
	
	for (i=0;i<x->x_mixer->voice_count;i++)
	{
		x->x_mixer->voice[i].poll_for_dead_staus();
	}
	
	while ( n-- ) 
	{
		thing = 0;
		
		for (i=0;i<x->x_mixer->voice_count;i++)
		{
			x->x_mixer->voice[i].calculate();
			thing += x->x_mixer->voice[i].get_output();
		}
		
		if (thing == 0) 
		{
			*out = 0.0;
		}
		else
		{
      			*out = scalefac * thing;
		}
		out++;
	}

	return (w+4);
}

static void rx7_dsp(t_rx7 *x, t_signal **sp)
{
	dsp_add(rx7_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

extern "C" void rx7_tilde_setup(void)
{
	rx7_class = class_new( 
	                gensym("rx7~"), 
			(t_newmethod)rx7_new, 
			0, 
			sizeof(t_rx7), 
			CLASS_DEFAULT,
			A_NULL
			);

	class_addmethod(rx7_class, (t_method)rx7_dsp,      gensym("dsp"),  A_NULL);
	class_addmethod(rx7_class, (t_method)rx7_loadbank, gensym("load"), A_GIMME, 0);
	class_addmethod(rx7_class, (t_method)rx7_note, gensym("note"), A_GIMME, 0);
	class_addmethod(rx7_class, (t_method)rx7_program_change, gensym("prog"), A_GIMME, 0);
}
