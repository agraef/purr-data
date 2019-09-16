#include "MSPd.h"
#include "string.h"

/**/
// NOTE: MUST PREVENT WAVEFORM UPDATE IF ALREADY IN PROGRESS

// 7.23.04 adding bandlimited harmonic count
// 9.2.04 fixed cross fade amplitude bug
// 1.14.06 added apwave message for amp/phase creation

#define OSCIL_MAXIMUM_HARMONICS (1024)
#define OSCIL_DEFAULT_FLEN 8192
#define OSCIL_MAX_FLEN 1048576
#define OSCIL_MAX_HARMS 1024
#define OSCIL_DEFAULT_HARMONICS 10
#define OSCIL_INIT_FREQ 440.0
#define OSCIL_DEFAULT_WAVEFORM "sine"
#define OSCIL_NOFADE 0
#define OSCIL_LINEAR 1
#define OSCIL_POWER 2

#define OBJECT_NAME "oscil~"
#define REV "2.4.06"


static t_class *oscil_class;

typedef struct _oscil
{
    
    t_object x_obj;
    float x_f;
	int table_length;
	float *wavetable;
	int harmonic_count;
	float *harmonic_weights;
	float *harmonic_phases;
	double phase;
	double phase_offset;
	double si_factor;
	double si;
	int bl_harms;
	float piotwo;
	float twopi;
	float sr;
	short mute;
	short connected[4];
	float *old_wavetable;
	short dirty;
	float fade_ms;
	int fade_samples;
	int fade_countdown;
	short fadetype;
	short firsttime;
	short fade_in_progress;
	short interpolate; // flag for synthesis method
    
} t_oscil;

void *oscil_new(t_symbol *s, short argc, t_atom *argv);
static t_int *oscil_perform(t_int *w);
void oscil_dsp(t_oscil *x, t_signal **sp);
void build_waveform(t_oscil *x);
void build_amph_waveform(t_oscil *x);
void oscil_mute(t_oscil *x, t_floatarg flag);
void oscil_sine(t_oscil *x );
void oscil_sawtooth(t_oscil *x);
void oscil_square(t_oscil *x) ;
void oscil_triangle(t_oscil *x);
void oscil_buzz(t_oscil *x );
void oscil_list (t_oscil *x, t_symbol *msg, short argc, t_atom *argv);
void oscil_fadetime (t_oscil *x, t_floatarg fade_ms) ;
void oscil_fadetype(t_oscil *x, t_floatarg ftype);
void oscil_harmcount(t_oscil *x, t_floatarg harms);
void oscil_interpolate(t_oscil *x, t_floatarg tog);
void oscil_dsp_free(t_oscil *x);
void oscil_amph(t_oscil *x, t_symbol *msg, short argc, t_atom *argv);
t_int *oscil_perform_interpolate(t_int *w);


void oscil_tilde_setup(void)
{
    oscil_class = class_new(gensym("oscil~"), (t_newmethod)oscil_new,
                            (t_method)oscil_dsp_free ,sizeof(t_oscil), 0, A_GIMME,0);
    CLASS_MAINSIGNALIN(oscil_class, t_oscil, x_f );
    class_addmethod(oscil_class, (t_method)oscil_dsp, gensym("dsp"), 0);
    class_addmethod(oscil_class, (t_method)oscil_mute, gensym("mute"), A_DEFFLOAT,0);
    class_addmethod(oscil_class, (t_method)oscil_sine, gensym("sine"), 0);
    class_addmethod(oscil_class, (t_method)oscil_triangle, gensym("triangle"), 0);
    class_addmethod(oscil_class, (t_method)oscil_square, gensym("square"), 0);
    class_addmethod(oscil_class, (t_method)oscil_sawtooth, gensym("sawtooth"), 0);
    class_addmethod(oscil_class, (t_method)oscil_buzz, gensym("buzz"), 0);
    class_addmethod(oscil_class, (t_method)oscil_list, gensym("list"), A_GIMME, 0);
    class_addmethod(oscil_class, (t_method)oscil_amph, gensym("amph"), A_GIMME, 0);
    class_addmethod(oscil_class, (t_method)oscil_fadetype, gensym("fadetype"), A_FLOAT, 0);
    class_addmethod(oscil_class, (t_method)oscil_fadetime, gensym("fadetime"), A_FLOAT, 0);
    class_addmethod(oscil_class, (t_method)oscil_harmcount, gensym("harmcount"), A_FLOAT, 0);
    class_addmethod(oscil_class, (t_method)oscil_interpolate, gensym("interpolate"), A_FLOAT, 0);
    potpourri_announce(OBJECT_NAME);
}


void oscil_list (t_oscil *x, t_symbol *msg, short argc, t_atom *argv)
{
	short i;
	int harmonic_count = 0;
	float *harmonic_weights = x->harmonic_weights;
	for (i=0; i < argc; i++) {
		harmonic_weights[harmonic_count] = atom_getfloatarg(i, argc, argv);
		++harmonic_count;
	}
	x->harmonic_count = harmonic_count ;
	build_waveform(x);
}

void oscil_amph(t_oscil *x, t_symbol *msg, short argc, t_atom *argv)
{
	short i;
	int harmonic_count = 0;
	float *harmonic_weights = x->harmonic_weights;
	float *harmonic_phases = x->harmonic_phases;
	if(argc < 1){
		return;
	}
	/* DC */
	harmonic_weights[0] = atom_getfloatarg(0, argc, argv);
	harmonic_phases[0] = 0;
	
	harmonic_count = 1;
	for (i=1; i < argc; i += 2) {
		harmonic_weights[harmonic_count] = atom_getfloatarg(i, argc, argv);
		harmonic_phases[harmonic_count] = atom_getfloatarg(i+1, argc, argv);
		++harmonic_count;
	}
	x->harmonic_count = harmonic_count ;
	build_amph_waveform(x);
}


void oscil_fadetime (t_oscil *x, t_floatarg fade_ms)
{
	if(x->fade_countdown) {
		error("oscil: crossfade in progress, cannot update fade time");
		return;
	}
	if( fade_ms < 0.0 || fade_ms > 60000.0 ){
		error("%s: %f is not a good fade time",OBJECT_NAME, fade_ms);
		fade_ms = 50.;
	}
	x->fade_ms = fade_ms;
	x->fade_samples = x->fade_ms * x->sr / 1000.0 ;
}

void oscil_fadetype(t_oscil *x, t_floatarg ftype)
{
    
	if( ftype < 0 || ftype > 2 ) {
		error("%s: unknown type of fade, selecting no fade",OBJECT_NAME);
		ftype = 0;
	}
	x->fadetype = ftype;
}

void oscil_harmcount(t_oscil *x, t_floatarg harms)
{
    
	if( harms < 1 || harms > OSCIL_MAXIMUM_HARMONICS-1) {
		error("%d is out of range and must be between 1 to %d", harms,OSCIL_MAXIMUM_HARMONICS-1 );
		return;
	}
	x->bl_harms = harms + 1;
}

void oscil_mute(t_oscil *x, t_floatarg flag)
{
	x->mute = (short)flag;
}

void oscil_interpolate(t_oscil *x, t_floatarg flag)
{
	x->interpolate = (short)flag;
    //	post("must toggle DACs for this synthesis method");
}


void *oscil_new(t_symbol *s, short argc, t_atom *argv)
{
	float init_freq;
	t_symbol *init_waveform_symbol;

    t_oscil *x = (t_oscil *)pd_new(oscil_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal") );
    outlet_new(&x->x_obj, gensym("signal") );
    
    /*
     SET DEFAULTS IN ADVANCE
     added 4.14.2003
     */
    
    init_freq = OSCIL_INIT_FREQ;
    x->table_length = OSCIL_DEFAULT_FLEN;
    init_waveform_symbol = gensym(OSCIL_DEFAULT_WAVEFORM);
    x->bl_harms = OSCIL_DEFAULT_HARMONICS;
    x->table_length = OSCIL_DEFAULT_FLEN ;
    x->phase_offset = 0.0;
    x->interpolate = 0;
    
    if( argc > 0 ){
        init_freq = atom_getfloatarg(0, argc, argv);
        if( ! init_freq ){
            error("%s: zero initial frequency, resetting to 440",OBJECT_NAME);
            init_freq = 440 ;
        }
        
    }
    if( argc > 1 ){
        x->table_length = atom_getfloatarg(1, argc, argv);
        //    post("table length is %d", x->table_length );
    }
    if( argc > 2 ){
        init_waveform_symbol = atom_getsymbolarg(2, argc, argv);
    } else {
        init_waveform_symbol = gensym( OSCIL_DEFAULT_WAVEFORM );
        
    }
    
    if( argc > 3 ) {
        x->bl_harms = atom_getfloatarg(3, argc, argv);
        if( x->bl_harms > 1024 ){
            error("%s: too many harmonics - limit is 1024",OBJECT_NAME);
            x->bl_harms = 1024;
        }
    }
    else {
        x->bl_harms = OSCIL_DEFAULT_HARMONICS ;
    }
    
    if( x->table_length < 4 ){
        x->table_length = OSCIL_DEFAULT_FLEN ;
    }
    if( x->table_length > OSCIL_MAX_FLEN ){
        x->table_length = OSCIL_MAX_FLEN;
        error("%s: Exceeded maximum - setting function length to %d",OBJECT_NAME,OSCIL_MAX_FLEN);
    } if( x->bl_harms < 1 || x->bl_harms > OSCIL_MAXIMUM_HARMONICS ){
        x->bl_harms = OSCIL_DEFAULT_HARMONICS ;
        error("%s: Bad parameters. Bandlimited waveforms will have %d partials.",
              OBJECT_NAME,OSCIL_DEFAULT_HARMONICS);
    }
    
    
	x->fade_in_progress = 0;
    
	x->bl_harms = x->bl_harms + 1;
	x->piotwo = 2. * atan(1.0);
	x->twopi = 8.0 * atan(1.0);
    
	
	x->old_wavetable = (float *) t_getbytes( x->table_length * sizeof(float) );
    
	x->wavetable = (float *) t_getbytes( x->table_length * sizeof(float) );
	x->harmonic_weights = (float *) t_getbytes( OSCIL_MAXIMUM_HARMONICS * sizeof(float) );
	x->harmonic_phases = (float *) t_getbytes( OSCIL_MAXIMUM_HARMONICS * sizeof(float) );
	x->phase = 0;
	x->mute = 0;
	x->dirty = 0;
	x->sr = sys_getsr();
	if( ! x->sr ){
		x->sr = 44100;
		error("zero sampling rate - set to 44100");
	}
	x->si_factor = (float) x->table_length / x->sr;
	x->si = init_freq * x->si_factor ;
	x->fade_countdown = 0;
	x->fade_ms = 50. ;
	x->fade_samples = x->fade_ms * x->sr / 1000.0 ;
	x->fadetype = OSCIL_LINEAR;
    
	x->firsttime = 1;
	
	if (init_waveform_symbol == gensym("triangle")) {
		oscil_triangle( x );
	} else if (init_waveform_symbol == gensym("square")) {
		oscil_square( x );
	} else if (init_waveform_symbol == gensym("sawtooth")) {
		oscil_sawtooth( x );
	} else if (init_waveform_symbol == gensym("buzz")) {
		oscil_buzz( x );
	} else { // default to sine wave
		oscil_sine( x );
	}
    
	x->firsttime = 0;
    
	// post("Additive synthesis oscil [4.14.2003a] (as described in Audio Programming)");
    
    return (x);
}

void build_amph_waveform( t_oscil *x )
{
	float rescale;
	int i, j;
	float max = 0.0;
	float *wavetable = x->wavetable;
	float *old_wavetable = x->old_wavetable;
	float *harmonic_weights = x->harmonic_weights;
	float *harmonic_phases = x->harmonic_phases;
	int harmonic_count = x->harmonic_count;
	int table_length = x->table_length;
	float twopi = x->twopi;
    //	float testsum = 0.0;
	float addphase;
    
	if( x->fade_in_progress ){
		// error("Crossfade in progress. Cannot generate waveform");
		// do not use because this will happen too often
		return;
	}
	
    
	if( harmonic_count < 1 ){
		error("%s: no harmonics specified, waveform not created.",OBJECT_NAME);
		return;
	}
	
	if( x->fadetype && ! x->firsttime ){
		x->fade_countdown = x->fade_samples;
		x->fade_in_progress = 1;
	}
	/*
     for( i = 0; i < table_length ; i++ ){
     old_wavetable[i] = wavetable[i];
     }
     */
	memcpy(old_wavetable, wavetable, table_length * sizeof(float) );
    
	x->dirty = 1 ;
	
	// add DC in directly (usually 0.0)
	for( i = 0; i < table_length; i++ ){
		wavetable[i] = harmonic_weights[0];
	}
	// sum all specified harmonics
	for( i = 1 ; i < harmonic_count; i++ ){
		if( harmonic_weights[i] ){
			addphase = twopi * harmonic_phases[i];
            //			post("amp %f phase %f twopi phase %f",harmonic_weights[i],harmonic_phases[i],addphase);
			for( j = 0; j < table_length; j++ ){
				wavetable[j] +=
                harmonic_weights[i] * sin( twopi * ((float)i * ((float)j/(float)table_length)) + addphase ) ;
			}
		}
	}
	// determine maximum amplitude.
	max = 0;
	for( j = 0; j < table_length; j++ ){
		if( max < fabs(wavetable[j]) ){
			max = fabs(wavetable[j]) ;
		}
	}
	// restore last table
	if( max == 0.0 ){
		for( j = 0; j < table_length; j++ ){
			wavetable[j] = old_wavetable[j];
		}
		error("all zero function ignored");
		x->dirty = 0;
		return;
	}
	// normalize waveform to maximum amplitude of 1.0
	rescale = 1.0 / max ;
    
	for( j = 0; j < table_length; j++ ){
		wavetable[j] *= rescale ;
	}
	x->dirty = 0;
}

void build_waveform( t_oscil *x ) {
	float rescale;
	int i, j;
	float max = 0.0;
	float *wavetable = x->wavetable;
	float *old_wavetable = x->old_wavetable;
	float *harmonic_weights = x->harmonic_weights;
	int harmonic_count = x->harmonic_count;
	int table_length = x->table_length;
	float twopi = x->twopi;
    //	float testsum = 0.0;
    
	if( x->fade_in_progress ){
		// error("Crossfade in progress. Cannot generate waveform");
		// do not use because this will happen too often
		return;
	}
	
    
	if( harmonic_count < 1 ){
		error("no harmonics specified, waveform not created.");
		return;
	}
	
	if( x->fadetype && ! x->firsttime ){
		x->fade_countdown = x->fade_samples;
		x->fade_in_progress = 1;
	}
	/*
     for( i = 0; i < table_length ; i++ ){
     old_wavetable[i] = wavetable[i];
     }
     */
	memcpy(old_wavetable, wavetable, table_length * sizeof(float) );
	
	x->dirty = 1 ;
	
	// add DC in directly (usually 0.0)
	for( i = 0; i < table_length; i++ ){
		wavetable[i] = harmonic_weights[0];
	}
	// sum all specified harmonics
	for( i = 1 ; i < harmonic_count; i++ ){
		if( harmonic_weights[i] ){
			for( j = 0; j < table_length; j++ ){
				wavetable[j] += harmonic_weights[i] * sin( twopi * ( (float) i * ((float) j /(float)table_length)) ) ;
			}
		}
	}
	// determine maximum amplitude. Since waveform is symmetric, we could only look for positive maximum.
	max = 0;
	for( j = 0; j < table_length; j++ ){
		if( max < fabs(wavetable[j]) ){
			max = fabs(wavetable[j]) ;
		}
	}
	// restore last table
	if( max == 0.0 ){
		for( j = 0; j < table_length; j++ ){	// could use memcpy here
			wavetable[j] = old_wavetable[j];
		}
		error("all zero function ignored");
		x->dirty = 0;
		return;
	}
	// normalize waveform to maximum amplitude of 1.0
	rescale = 1.0 / max ;
    
	for( j = 0; j < table_length; j++ ){
		wavetable[j] *= rescale ;
	}
	x->dirty = 0;
}

// interpolation
t_int *oscil_perform_interpolate(t_int *w)
{
	t_oscil *x = (t_oscil *) (w[1]);
	t_float *freq_vec = (t_float *)(w[2]);
	t_float *phase_vec = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int) w[5];
    
	double si_factor = x->si_factor;
	double si = x->si ;
	double phase = x->phase;
	double phase_offset = x->phase_offset;
	int table_length = x->table_length;
	float *wavetable = x->wavetable;
	float *old_wavetable = x->old_wavetable;
	short *connected = x->connected ;
	int fade_countdown = x->fade_countdown;
	int iphase, iphase2;
	int fade_samples = x->fade_samples;
	short fadetype = x->fadetype;
	float m1, m2;
	float frac;
	double fphase;
	float sample1, sample2, outsamp1, outsamp2;
	
	float piotwo = x->piotwo;
	
	if( x->mute ){
		while (n--) {
			*out++ = 0.0;
		}
		return (w+6);
	}
	/* interpolated loop */
	if(x->interpolate){
		while (n--) {
			if(connected[0])
				si = *freq_vec++ * si_factor;
			
			if(connected[1]){
				phase_offset = (float)table_length * *phase_vec++;
                
			}
            
			fphase = (phase + phase_offset);
			while( fphase >= table_length ) {
				fphase -= table_length;
			}
			while( fphase < 0 ) {
				fphase += table_length;
			}
			
			iphase = floor(fphase);
			iphase2 = (iphase+1)%table_length ;
			frac = fphase - iphase;
			
			
			if( x->dirty ){
				sample1 = old_wavetable[ iphase ];
				sample2 = old_wavetable[ iphase2 ];
				*out++ = sample1 + frac * (sample2 - sample1);
			}
			else if( fade_countdown ){
                
				sample1 = wavetable[ iphase ];
				sample2 = wavetable[ iphase2 ];
				outsamp1 = sample1 + frac * (sample2 - sample1);
                
				sample1 = old_wavetable[ iphase ];
				sample2 = old_wavetable[ iphase2 ];
				outsamp2 = sample1 + frac * (sample2 - sample1);
                
				m2 = (float) fade_countdown / (float) fade_samples ;
				m1 = 1.0 - m2 ;
				--fade_countdown;
				if( fadetype == 1 ){
					*out++ = m1 * outsamp1 + m2 * outsamp2 ;
				}
				else if( fadetype == 2 ) {
					m1 *= piotwo;
					*out++ = sin(m1) * outsamp1 + cos(m1) * outsamp2;
				}
			} else {
				sample1 = wavetable[ iphase ];
				sample2 = wavetable[ iphase2 ];
				*out++ = sample1 + frac * (sample2 - sample1) ;
			}
            
            
			phase += si;
			while( phase >= table_length ) {
				phase -= table_length ;
			}
			while( phase < 0 ) {
				phase += table_length ;
			}
            
		}
	}
    
    /* non-interpolated loop */
	else {
		while (n--) {
			if(connected[0])
				si = *freq_vec++ * si_factor;
			
			if(connected[1]){
				phase_offset = (float)table_length * *phase_vec++;
                
			}
            
			iphase = (int)(phase + phase_offset);
			while( iphase >= table_length ) {
				iphase -= table_length;
			}
			while( iphase < 0 ) {
				iphase += table_length;
			}
			
			if( x->dirty ){
				*out++ = old_wavetable[ iphase ] ;
			}
			else if( fade_countdown ){
				m2 = (float) fade_countdown / (float) fade_samples ;
				m1 = 1.0 - m2 ;
				--fade_countdown;
				if( fadetype == 1 ){
					*out++ = m1 * wavetable[iphase] + m2 * old_wavetable[ iphase ] ;
				}
				else if( fadetype == 2 ) {
					m1 *= piotwo;
					*out++ = sin(m1) * wavetable[iphase] + cos(m1) * old_wavetable[ iphase ] ;
				}
			} else {
				*out++ = wavetable[ iphase ] ;
			}
            
            
			phase += si;
			while( phase >= table_length ) {
				phase -= table_length ;
			}
			while( phase < 0 ) {
				phase += table_length ;
			}
            
		}
	}
    
	if( ! fade_countdown ){
		x->fade_in_progress = 0;
	}
	x->fade_countdown = fade_countdown;
    x->phase = phase;
    x->phase_offset = phase_offset;
	return (w+6);
}


static t_int *oscil_perform(t_int *w)
{
	t_oscil *x = (t_oscil *) (w[1]);
	t_float *freq_vec = (t_float *)(w[2]);
	t_float *phase_vec = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int) w[5];
    
	double si_factor = x->si_factor;
	double si = x->si ;
	double phase = x->phase;
	double phase_offset = x->phase_offset;
	int table_length = x->table_length;
	float *wavetable = x->wavetable;
	float *old_wavetable = x->old_wavetable;
	short *connected = x->connected ;
	int fade_countdown = x->fade_countdown;
	int iphase;
	int fade_samples = x->fade_samples;
	short fadetype = x->fadetype;
	float m1, m2;
	float piotwo = x->piotwo;
	
	if( x->mute ){
		while (n--) {
			*out++ = 0.0;
		}
		return (w+6);
	}
	
	while (n--) {
		if(connected[0])
			si = *freq_vec++ * si_factor;
		
		if(connected[1]){
			phase_offset = (float)table_length * *phase_vec++;
            
		}
        
		iphase = (int)(phase + phase_offset);
		while( iphase >= table_length ) {
			iphase -= table_length;
		}
		while( iphase < 0 ) {
			iphase += table_length;
		}
		
		if( x->dirty ){
			*out++ = old_wavetable[ iphase ] ;
		}
		else if( fade_countdown ){
			m2 = (float) fade_countdown / (float) fade_samples ;
			m1 = 1.0 - m2 ;
			--fade_countdown;
			if( fadetype == 1 ){
				*out++ = m1 * wavetable[iphase] + m2 * old_wavetable[ iphase ] ;
			}
			else if( fadetype == 2 ) {
				m1 *= piotwo;
				*out++ = sin(m1) * wavetable[iphase] + cos(m1) * old_wavetable[ iphase ] ;
			}
		} else {
			*out++ = wavetable[ iphase ] ;
		}
        
        
		phase += si;
		while( phase >= table_length ) {
			phase -= table_length ;
		}
		while( phase < 0 ) {
			phase += table_length ;
		}
        
	}
	if( ! fade_countdown ){
		x->fade_in_progress = 0;
	}
	x->fade_countdown = fade_countdown;
    x->phase = phase;
    x->phase_offset = phase_offset;
	return (w+6);
}

void oscil_sawtooth(t_oscil *x)
{
    int i;
    float sign = 1.0;
    
	x->harmonic_weights[0] = 0.0; // DC
	x->harmonic_count = x->bl_harms;
	for( i = 1 ; i < x->bl_harms; i++ ){
		x->harmonic_weights[i] = sign * 1.0/(float)i;
		sign *= -1. ;
	}
	build_waveform(x);
}
void oscil_triangle(t_oscil *x)
{
    int i;
    float sign = 1.0;
	x->harmonic_weights[0] = 0.0; // DC
	x->harmonic_count = x->bl_harms;
	for( i = 1 ; i < x->bl_harms; i += 2 ){
		x->harmonic_weights[i] = sign * 1.0/((float)i * (float)i);
		x->harmonic_weights[i + 1] = 0.0;
		sign *= -1;
	}
	build_waveform(x);
}

void oscil_sine(t_oscil *x)
{
	x->harmonic_weights[0] = 0.0;
	x->harmonic_weights[1] = 1.0;
	x->harmonic_count = 2;
	build_waveform(x);
}

void oscil_square(t_oscil *x)
{
    int i;
	x->harmonic_weights[0] = 0.0; // DC
	x->harmonic_count = x->bl_harms;
	for( i = 1 ; i < x->bl_harms  ; i += 2 ){
		x->harmonic_weights[i] = 1.0/(float)i;
		x->harmonic_weights[i + 1] = 0.0;
	}
	build_waveform(x);
}

void oscil_buzz(t_oscil *x)
{
    int i;
	x->harmonic_weights[0] = 0.0;
	x->harmonic_count = x->bl_harms;
	for( i = 1 ; i < x->bl_harms; i++ ){
		x->harmonic_weights[i] = 1.0;
	}
	build_waveform(x);
}

void oscil_dsp_free(t_oscil *x)
{

	t_freebytes(x->wavetable, x->table_length * sizeof(float));
	t_freebytes(x->old_wavetable, x->table_length * sizeof(float));
	t_freebytes(x->harmonic_weights, OSCIL_MAXIMUM_HARMONICS * sizeof(float));
	t_freebytes(x->harmonic_phases, OSCIL_MAXIMUM_HARMONICS * sizeof(float));
}

void oscil_dsp(t_oscil *x, t_signal **sp)
{
	if(! x->sr ){
		x->sr = 44100;
	}
	if( x->sr != sp[0]->s_sr ){
		if(! sp[0]->s_sr){
			error("oscil~: Zero sampling rate reported!");
			return;
		}
		x->si *= sp[0]->s_sr / x->sr ;
		x->sr = sp[0]->s_sr;
		x->si_factor = (float) x->table_length / x->sr;	
	}

	x->connected[0] = 1;
	x->connected[1] = 1; 

	x->phase = 0.0;
    
	dsp_add(oscil_perform_interpolate, 5, x, 
			sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, (t_int)sp[0]->s_n);
    
	
}

