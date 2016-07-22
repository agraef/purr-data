#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "m_pd.h"

#ifndef MAYER_H
#define MAYER_H
#define REAL float
void mayer_realfft(int n, REAL *real);
void mayer_realifft(int n, REAL *real);
#endif

#define PI (float)3.14159265358979323846
#define L2SC (float)3.32192809488736218171

#ifndef CLIP
#define CLIP(a, lo, hi) ( (a)>(lo)?( (a)<(hi)?(a):(hi) ):(lo) )
#endif

// Variables for FFT routine
typedef struct
{
	int nfft;        // size of FFT
	int numfreqs;    // number of frequencies represented (nfft/2 + 1)
	float* fft_data; // array for writing/reading to/from FFT function
} fft_vars;

// Constructor for FFT routine
fft_vars* fft_con(int nfft)
{
	fft_vars* membvars = (fft_vars*) malloc(sizeof(fft_vars));

	membvars->nfft = nfft;
	membvars->numfreqs = nfft/2 + 1;

	membvars->fft_data = (float*) calloc(nfft, sizeof(float));

	return membvars;
}

// Destructor for FFT routine
void fft_des(fft_vars* membvars)
{
	free(membvars->fft_data);

	free(membvars);
}

// Perform forward FFT of real data
// Accepts:
//   x - pointer to struct of FFT variables
//   input - pointer to an array of (real) input values, size nfft
//   output_re - pointer to an array of the real part of the output,
//     size nfft/2 + 1
//   output_im - pointer to an array of the imaginary part of the output,
//     size nfft/2 + 1
void fft_forward(fft_vars* membvars, float* input, float* output_re, float* output_im)
{
	int ti;
	int nfft;
	int hnfft;
	int numfreqs;

	nfft = membvars->nfft;
	hnfft = nfft/2;
	numfreqs = membvars->numfreqs;

	for (ti=0; ti<nfft; ti++) {
		membvars->fft_data[ti] = input[ti];
	}

	mayer_realfft(nfft, membvars->fft_data);

	output_im[0] = 0;
	for (ti=0; ti<hnfft; ti++) {
		output_re[ti] = membvars->fft_data[ti];
		output_im[ti+1] = membvars->fft_data[nfft-1-ti];
	}
	output_re[hnfft] = membvars->fft_data[hnfft];
	output_im[hnfft] = 0;
}

// Perform inverse FFT, returning real data
// Accepts:
//   x - pointer to struct of FFT variables
//   input_re - pointer to an array of the real part of the output,
//     size nfft/2 + 1
//   input_im - pointer to an array of the imaginary part of the output,
//     size nfft/2 + 1
//   output - pointer to an array of (real) input values, size nfft
void fft_inverse(fft_vars* membvars, float* input_re, float* input_im, float* output)
{
	int ti;
	int nfft;
	int hnfft;
	int numfreqs;

	nfft = membvars->nfft;
	hnfft = nfft/2;
	numfreqs = membvars->numfreqs;

	for (ti=0; ti<hnfft; ti++) {
		membvars->fft_data[ti] = input_re[ti];
		membvars->fft_data[nfft-1-ti] = input_im[ti+1];
	}
	membvars->fft_data[hnfft] = input_re[hnfft];

	mayer_realifft(nfft, membvars->fft_data);

	for (ti=0; ti<nfft; ti++) {
		output[ti] = membvars->fft_data[ti];
	}
}

// DONE WITH FFT CODE



void *autotune_class;

typedef struct _autotune 	// Data structure for this object PD
{
    t_object x_obj;
    float x_f;
		
	// parameters

	float fTune;
	float fFixed;
	float fPull;
	float fA;
	float fBb;
	float fB;
	float fC;
	float fDb;
	float fD;
	float fEb;
	float fE;
	float fF;
	float fGb;
	float fG;
	float fAb;
	float fAmount;
	//float fGlide;
	float fSmooth;
	float fScwarp;
	float fLfoamp;
	float fShift;
	float fLforate;
	float fLfoshape;
	float fLfosymm;
	float fLfoquant;
	float fFcorr;
	float fFwarp;
	float fMix;
	float fPitch;
	float fConf;

	fft_vars* fx; 		// member variables for fft routine
	unsigned long fs; 			// Sample rate
	unsigned long cbsize; 		// size of circular buffer
	unsigned long corrsize; 	// cbsize/2 + 1
	unsigned long cbiwr;
	unsigned long cbord;
	float* cbi; // circular input buffer
	float* cbf; // circular formant correction buffer
	float* cbo; // circular output buffer
	float* cbwindow; // hann of length N/2, zeros for the rest
	float* acwinv; // inverse of autocorrelation of window
	float* hannwindow; // length-N hann
	int noverlap;
	float* ffttime;
	float* fftfreqre;
	float* fftfreqim;
	
	// VARIABLES FOR LOW-RATE SECTION
	float aref; 				// A tuning reference (Hz)
	float inpitch; 				// Input pitch (semitones)
	float conf; 				// Confidence of pitch period estimate (between 0 and 1)
	float outpitch; 			// Output pitch (semitones)
	float vthresh; 				// Voiced speech threshold
	float pperiod; 				// Pitch period (seconds)
	float pitch; 				// Pitch (semitones)
	float pitchpers; 			// Pitch persist (semitones)
	float pmax; 				// Maximum allowable pitch period (seconds)
	float pmin; 				// Minimum allowable pitch period (seconds)
	unsigned long nmax; 		// Maximum period index for pitch prd est
	unsigned long nmin; 		// Minimum period index for pitch prd est
	float lrshift; 				// Shift prescribed by low-rate section
	int ptarget; 				// Pitch target, between 0 and 11
  	float sptarget; 			// Smoothed pitch target
	//float sptarget; 			// Smoothed pitch target
	//int wasvoiced; 				// 1 if previous frame was voiced
	//float persistamt; 			// Proportion of previous pitch considered during next voiced period
	//float glidepersist;
	float lfophase;
	
	// VARIABLES FOR PITCH SHIFTER
	//float phprd; 				// phase period
	float phprdd;				// default (unvoiced) phase period
	float inphinc; 				// input phase increment
	float outphinc; 			// input phase increment
	float phincfact; 			// factor determining output phase increment
	float phasein;
	float phaseout;
	float* frag; 				// windowed fragment of speech
	unsigned long fragsize; 	// size of fragment in samples	
	float clockinterval;
	void *periodout, *confout; 	// floatout for pitch
	void *clock;

	// VARIABLES FOR FORMANT CORRECTOR
	unsigned int ford;
	float falph;
	float flamb;
	float* fk;
	float* fb;
	float* fc;
	float* frb;
	float* frc;
	float* fsig;
	float* fsmooth;
	float fhp;
	float flp;
	float flpa;
	float** fbuff;
	float* ftvec;
	float fmute;
	float fmutealph;
	
} t_autotune;	



//prototypes for methods
void *autotune_new(t_symbol *s, int argc, t_atom *argv);
void autotune_free(t_autotune *x);	
void autotune_init(t_autotune *x, unsigned long sr);
t_int *autotune_perform(t_int *w);
void autotune_dsp(t_autotune *x, t_signal **sp, short *count);
void autotune_assist(t_autotune *x, void *b, long m, long a, char *s);
//void autotune_setparam(t_autotune *x, t_symbol *m, short argc, t_atom *argv);
void autotune_list(t_autotune *x, t_symbol *m, short argc, t_atom *argv);
void autotune_mix(t_autotune *x, t_floatarg f);
void autotune_shift(t_autotune *x, t_floatarg f);
void autotune_pull(t_autotune *x, t_floatarg f);
void autotune_pull_pitch(t_autotune *x, t_floatarg f);
void autotune_tune(t_autotune *x, t_floatarg f);
void autotune_correction(t_autotune *x, t_floatarg f);
void autotune_smooth(t_autotune *x, t_floatarg f);
void autotune_lfo_depth(t_autotune *x, t_floatarg f);
void autotune_lfo_rate(t_autotune *x, t_floatarg f);
void autotune_lfo_shape(t_autotune *x, t_floatarg f);
void autotune_lfo_symmetry(t_autotune *x, t_floatarg f);
void autotune_formant_correction(t_autotune *x, t_floatarg f);
void autotune_formant_warp(t_autotune *x, t_floatarg f);
void autotune_scale_warp(t_autotune *x, t_floatarg f);
void autotune_confidence(t_autotune *x, t_floatarg f);
void autotune_processclock(t_autotune *x);




void autotune_tilde_setup(void)
{
	autotune_class = class_new(gensym("autotune~"), (t_newmethod)autotune_new, 
							   (t_method)autotune_free ,sizeof(t_autotune),0,A_GIMME,0);
	CLASS_MAINSIGNALIN(autotune_class, t_autotune, x_f );
	class_addmethod(autotune_class,(t_method)autotune_dsp, gensym("dsp"),0);
	class_addmethod(autotune_class,(t_method)autotune_assist, gensym("assist"),0);
	class_addmethod(autotune_class,(t_method)autotune_list,gensym("list"),A_GIMME,0);
	class_addmethod(autotune_class,(t_method)autotune_mix,gensym("mix"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_shift,gensym("shift"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_pull,gensym("pull"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_pull_pitch,gensym("pullpitch"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_tune,gensym("tune"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_correction,gensym("correct"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_smooth,gensym("smooth"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_lfo_depth,gensym("lfodepth"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_lfo_rate,gensym("lforate"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_lfo_shape,gensym("lfoshape"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_lfo_symmetry,gensym("lfosym"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_formant_correction,gensym("fcorr"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_formant_warp,gensym("warp"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_scale_warp,gensym("scwarp"),A_FLOAT,0);
	class_addmethod(autotune_class,(t_method)autotune_confidence,gensym("confidence"),A_FLOAT,0);
	post("autotune~ v.0.9.2");
	post("Ivica Ico Bukvic 2016");
}


int autotune_power_of_two (int x)
{
	 while (((x % 2) == 0) && x > 1) /* While x is even and > 1 */
	 	x /= 2;
	 return (x == 1);
}


// Create - Contruction of signal inlets and outlets
void *autotune_new(t_symbol *s, int argc, t_atom *argv)
{
    unsigned long sr;
    int fft_window = 0;
    int fft_hop = 0;

    t_autotune *x = (t_autotune *)pd_new(autotune_class);

    if (argc && argv->a_type == A_FLOAT) // got FFT window
    {
        fft_window = (int)atom_getint(argv);

        if (fft_window < 0) fft_window = 0;
        if (!autotune_power_of_two(fft_window)) fft_window = 0;
        argv++;
        argc--;
    }

    if (argc && argv->a_type == A_FLOAT) // got hop number
    {
        fft_hop = (int)atom_getint(argv);

        if (fft_hop < 2 || fft_hop > 32) fft_hop = 0;
        argv++;
        argc--;
    }

    if (fft_window != 0)
    	x->cbsize = fft_window;

    if (fft_hop != 0)
    	x->noverlap = fft_hop;
	
	if(sys_getsr()) sr = sys_getsr();
	else sr = 44100;
	
	autotune_init(x , sr);
	
	// second argument = number of signal inlets
    //dsp_setup((t_object *)x, 1);
	//inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"));
	//inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	
	//floatinlet_new (&x->x_obj, &x->fAmount);
	//inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("correct"));

	//floatinlet_new (&x->x_obj, &x->fGlide);

	//floatinlet_new (&x->x_obj, &x->fSmooth);
	//inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("smooth"));

	//floatinlet_new (&x->x_obj, &x->fMix);
	//inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("mix"));

	//floatinlet_new (&x->x_obj, &x->fShift);
	//inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("shift"));

	//floatinlet_new (&x->x_obj, &x->fTune);
	//inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("tune"));

	//floatinlet_new (&x->x_obj, &x->fPersist);
	//symbolinlet_new (&x->x_obj, &x->fAmount);
	
	outlet_new(&x->x_obj, gensym("signal"));
	//x->f_out = outlet_new(&x->x_obj, &s_float);
	x->confout = outlet_new(&x->x_obj, &s_float);
	x->periodout = outlet_new(&x->x_obj, &s_float);
	//x->confout = outlet_new(x, "float");
	//x->periodout = outlet_new(x, "float");
    x->clock = clock_new(x,(t_method)autotune_processclock);
	x->clockinterval = 10.;
    
    return (x);									// Return the pointer
}

// Destroy
void autotune_free(t_autotune *x)
{
	//dsp_free((t_object *)x);		// Always call dsp_free first in this routine
	unsigned int ti;
	clock_unset(x->clock);
	clock_free(x->clock); 
	fft_des(x->fx);
	freebytes(x->cbi,0);
	freebytes(x->cbf,0);
	freebytes(x->cbo,0);
	//freebytes(x->cbonorm,0);
	freebytes(x->cbwindow,0);
	freebytes(x->hannwindow,0);
	freebytes(x->acwinv,0);
	freebytes(x->frag,0);
	freebytes(x->ffttime,0);
	freebytes(x->fftfreqre,0);
	freebytes(x->fftfreqim,0);
	free(x->fk);
	free(x->fb);
	free(x->fc);
	free(x->frb);
	free(x->frc);
	free(x->fsmooth);
	free(x->fsig);
	for (ti=0; ti<x->ford; ti++) {
		free(x->fbuff[ti]);
	}
	free(x->fbuff);
	free(x->ftvec);
}

void autotune_init(t_autotune *x,unsigned long sr)
{
	unsigned long ti;

	x->fs = sr;
	x->aref = 440;
	x->fTune = x->aref;
	
	if (x->cbsize == 0)
	{
		if (x->fs >=88200) {
			x->cbsize = 4096;
		}
		else {
			x->cbsize = 2048;
		}
	}
	x->corrsize = x->cbsize / 2 + 1;
	
	x->pmax = 1/(float)70;  // max and min periods (ms)
	x->pmin = 1/(float)2400; // eventually may want to bring these out as sliders
	
	x->pperiod = x->pmax;
	
	x->nmax = (unsigned long)(x->fs * x->pmax);
	if (x->nmax > x->corrsize) {
		x->nmax = x->corrsize;
	}
	x->nmin = (unsigned long)(x->fs * x->pmin);
	
	x->cbi = (float*) calloc(x->cbsize, sizeof(float));
	x->cbf = (float*) calloc(x->cbsize, sizeof(float));
	x->cbo = (float*) calloc(x->cbsize, sizeof(float));
	//x->cbonorm = (float*) calloc(x->cbsize, sizeof(float));
	
	x->cbiwr = 0;
	x->cbord = 0;

	x->lfophase = 0;

	// Initialize formant corrector
	x->ford = 7; // should be sufficient to capture formants
	x->falph = pow(0.001, (float) 80 / (x->fs));
	x->flamb = -(0.8517*sqrt(atan(0.06583*x->fs))-0.1916); // or about -0.88 @ 44.1kHz
	x->fk = calloc(x->ford, sizeof(float));
	x->fb = calloc(x->ford, sizeof(float));
	x->fc = calloc(x->ford, sizeof(float));
	x->frb = calloc(x->ford, sizeof(float));
	x->frc = calloc(x->ford, sizeof(float));
	x->fsig = calloc(x->ford, sizeof(float));
	x->fsmooth = calloc(x->ford, sizeof(float));
	x->fhp = 0;
	x->flp = 0;
	x->flpa = pow(0.001, (float) 10 / (x->fs));
	x->fbuff = (float**) malloc((x->ford)*sizeof(float*));
	for (ti=0; ti<x->ford; ti++) {
		x->fbuff[ti] = calloc(x->cbsize, sizeof(float));
	}
	x->ftvec = calloc(x->ford, sizeof(float));
	x->fmute = 1;
	x->fmutealph = pow(0.001, (float)1 / (x->fs));
	
	// Standard raised cosine window, max height at N/2
	x->hannwindow = (float*) calloc(x->cbsize, sizeof(float));
	for (ti=0; ti<x->cbsize; ti++) {
		x->hannwindow[ti] = -0.5*cos(2*PI*ti/(x->cbsize - 1)) + 0.5;
	}
	
	// Generate a window with a single raised cosine from N/4 to 3N/4
	x->cbwindow = (float*) calloc(x->cbsize, sizeof(float));
	for (ti=0; ti<(x->cbsize / 2); ti++) {
		x->cbwindow[ti+x->cbsize/4] = -0.5*cos(4*PI*ti/(x->cbsize - 1)) + 0.5;
	}
	
	if (x->noverlap == 0)
		x->noverlap = 4;

	//fprintf(stderr,"%d %d\n", x->cbsize, x->noverlap);
	
	x->fx = fft_con(x->cbsize);
	
	x->ffttime = (float*) calloc(x->cbsize, sizeof(float));
	x->fftfreqre = (float*) calloc(x->corrsize, sizeof(float));
	x->fftfreqim = (float*) calloc(x->corrsize, sizeof(float));
	
	
	// ---- Calculate autocorrelation of window ----
	x->acwinv = (float*) calloc(x->cbsize, sizeof(float));
	for (ti=0; ti<x->cbsize; ti++) {
		x->ffttime[ti] = x->cbwindow[ti];
	}
	fft_forward(x->fx, x->cbwindow, x->fftfreqre, x->fftfreqim);
	for (ti=0; ti<x->corrsize; ti++) {
		x->fftfreqre[ti] = (x->fftfreqre[ti])*(x->fftfreqre[ti]) + (x->fftfreqim[ti])*(x->fftfreqim[ti]);
		x->fftfreqim[ti] = 0;
	}
	fft_inverse(x->fx, x->fftfreqre, x->fftfreqim, x->ffttime);
	for (ti=1; ti<x->cbsize; ti++) {
		x->acwinv[ti] = x->ffttime[ti]/x->ffttime[0];
		if (x->acwinv[ti] > 0.000001) {
			x->acwinv[ti] = (float)1/x->acwinv[ti];
		}
		else {
			x->acwinv[ti] = 0;
		}
	}
	x->acwinv[0] = 1;
	// ---- END Calculate autocorrelation of window ----	
	
	x->lrshift = 0;
	x->ptarget = 0;
	x->sptarget = 0;
	//x->sptarget = 0;
	//x->wasvoiced = 0;
	//x->persistamt = 0;
	
	//x->glidepersist = 100; // 100 ms glide persist
	
	x->vthresh = 0.7;  //  The voiced confidence (unbiased peak) threshold level
	
	// Pitch shifter initialization
	x->phprdd = 0.01; // Default period
	//x->phprd = x->phprdd;
	x->inphinc = (float)1/(x->phprdd * x->fs);
	x->phincfact = 1;
	x->phasein = 0;
	x->phaseout = 0;
	x->frag = (float*) calloc(x->cbsize, sizeof(float));
	x->fragsize = 0;
}

//*********************//
// DSP Methods PD //
//*********************//

void autotune_dsp(t_autotune *x, t_signal **sp, short *count)
{
	clock_delay(x->clock, 0.);
	
	if(x->fs != sp[0]->s_sr)  autotune_init(x, sp[0]->s_sr);
	
	dsp_add(autotune_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}


//*************************//
// Perform Routine PD//
//*************************//
t_int *autotune_perform(t_int *w)
{
	t_autotune *x = (t_autotune *)(w[1]); // object is first arg 
	t_float *in = (t_float *)(w[2]);
	t_float *out = (t_float *)(w[3]);
	unsigned long SampleCount = (unsigned long)(w[4]);
	
	// copy struct variables to local
	
	/*float fA = x->fA;
	float fBb = x->fBb;
	float fB = x->fB;
	float fC = x->fC;
	float fDb = x->fDb;
	float fD = x->fD;
	float fEb = x->fEb;
	float fE = x->fE;
	float fF = x->fF;
	float fGb = x->fGb;
	float fG = x->fG;
	float fAb = x->fAb;*/
	//float fGlide = x->fGlide;
	//float fPersist = x->glidepersist;
	
	int iNotes[12];
	int iPitch2Note[12];
	int iNote2Pitch[12];
	int numNotes;

	float fAmount = x->fAmount;
	float fSmooth = x->fSmooth * 0.8;
	float fTune = x->fTune;
	iNotes[0] = (int) x->fA;
	iNotes[1] = (int) x->fBb;
	iNotes[2] = (int) x->fB;
	iNotes[3] = (int) x->fC;
	iNotes[4] = (int) x->fDb;
	iNotes[5] = (int) x->fD;
	iNotes[6] = (int) x->fEb;
	iNotes[7] = (int) x->fE;
	iNotes[8] = (int) x->fF;
	iNotes[9] = (int) x->fGb;
	iNotes[10] = (int) x->fG;
	iNotes[11] = (int) x->fAb;
	float fFixed = x->fFixed;
	float fPull = x->fPull;
	float fShift = x->fShift;
	int iScwarp = x->fScwarp;
	float fLfoamp = x->fLfoamp;
	float fLforate = x->fLforate;
	float fLfoshape = x->fLfoshape;
	float fLfosymm = x->fLfosymm;
	int iLfoquant = x->fLfoquant;
	int iFcorr = x->fFcorr;
	float fFwarp = x->fFwarp;
	float fMix = x->fMix;
	
	//x->aref = (float)440*pow(2,fTune/12);
	
	unsigned long int lSampleIndex;
	
	unsigned long N = x->cbsize;
	unsigned long Nf = x->corrsize;
	unsigned long fs = x->fs;

	float pmax = x->pmax;
	float pmin = x->pmin;
	unsigned long nmax = x->nmax;
	unsigned long nmin = x->nmin;
	
	//float pperiod = x->pperiod;
	//float pitch = x->pitch;
	
		//
	
	volatile long int ti;
	volatile long int ti2;
	volatile long int ti3;
	volatile long int ti4;
	volatile float tf;
	volatile float tf2;
	volatile float tf3;

	// Variables for cubic spline interpolator
	volatile float indd;
	volatile int ind0;
	volatile int ind1;
	volatile int ind2;
	volatile int ind3;
	volatile float vald;
	volatile float val0;
	volatile float val1;
	volatile float val2;
	volatile float val3;

	volatile int lowersnap;
	volatile int uppersnap;
	volatile float lfoval;

	volatile float pperiod;
	volatile float inpitch;
	volatile float conf;
	volatile float outpitch;
	volatile float aref;
	volatile float fa;
	volatile float fb;
	volatile float fc;
	volatile float fk;
	volatile float flamb;
	volatile float frlamb;
	volatile float falph;
	volatile float foma;
	volatile float f1resp;
	volatile float f0resp;
	volatile float flpa;
	volatile int ford;

	// Some logic for the semitone->scale and scale->semitone conversion
	// If no notes are selected as being in the scale, instead snap to all notes
	ti2 = 0;
	for (ti=0; ti<12; ti++) {
		if (iNotes[ti]>=0) {
			iPitch2Note[ti] = ti2;
			iNote2Pitch[ti2] = ti;
			ti2 = ti2 + 1;
		}
		else {
			iPitch2Note[ti] = -1;
		}
	}
	numNotes = ti2;
	while (ti2<12) {
		iNote2Pitch[ti2] = -1;
		ti2 = ti2 + 1;
	}
	if (numNotes==0) {
		for (ti=0; ti<12; ti++) {
			iNotes[ti] = 1;
			iPitch2Note[ti] = ti;
			iNote2Pitch[ti] = ti;
		}
		numNotes = 12;
	}
	iScwarp = (iScwarp + numNotes*5)%numNotes;

	ford = x->ford;
	falph = x->falph;
	foma = (float)1 - falph;
	flpa = x->flpa;
	flamb = x->flamb;
	tf = pow((float)2,fFwarp/2)*(1+flamb)/(1-flamb);
	frlamb = (tf - 1)/(tf + 1);

	x->aref = (float)fTune;

	N = x->cbsize;
	Nf = x->corrsize;
	fs = x->fs;

	pmax = x->pmax;
	pmin = x->pmin;
	nmax = x->nmax;
	nmin = x->nmin;

	aref = x->aref;
	pperiod = x->pmax;
	inpitch = x->inpitch;
	conf = x->conf;
	outpitch = x->outpitch;

	
	//******************//
	//  MAIN DSP LOOP   //
	//******************//
	for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++)  
	{
		
		// load data into circular buffer
		tf = (float) *(in++);
		ti4 = x->cbiwr;
		//fprintf(stderr,"ti4=%d N=%d\n", ti4, N);
		x->cbi[ti4] = tf;
		/*x->cbiwr++;
		if (x->cbiwr >= N) {
			x->cbiwr = 0;
		}*/
		
		if (iFcorr>=1) {
			// Somewhat experimental formant corrector
			//  formants are removed using an adaptive pre-filter and
			//  re-introduced after pitch manipulation using post-filter
			// tf is signal input
			fa = tf - x->fhp; // highpass pre-emphasis filter
			x->fhp = tf;
			fb = fa;
			for (ti=0; ti<(long)ford; ti++) {
				x->fsig[ti] = fa*fa*foma + x->fsig[ti]*falph;
				fc = (fb-x->fc[ti])*flamb + x->fb[ti];
				x->fc[ti] = fc;
				x->fb[ti] = fb;
				fk = fa*fc*foma + x->fk[ti]*falph;
				x->fk[ti] = fk;
				tf = fk/(x->fsig[ti] + 0.000001);
				tf = tf*foma + x->fsmooth[ti]*falph;
				x->fsmooth[ti] = tf;
				x->fbuff[ti][ti4] = tf;
				fb = fc - tf*fa;
				fa = fa - tf*fc;
			}
			x->cbf[ti4] = fa;
			// Now hopefully the formants are reduced
			// More formant correction code at the end of the DSP loop
		}
		else {
			x->cbf[ti4] = tf;
		}

		//fprintf(stderr,"x->cbf[ti4]=%f\n", x->cbf[ti4]);

	    // Input write pointer logic
	    x->cbiwr++;
	    if (x->cbiwr >= N) {
	      x->cbiwr = 0;
	    }

		
		// ********************//
		// * Low-rate section *//
		// ********************//

		//fprintf(stderr,"overlap=%d outpitch=%f inpitch=%f\n", (x->cbiwr)%(N/x->noverlap), outpitch, inpitch);
		//fprintf(stderr,"outpitch=%f inpitch=%f\n", outpitch, inpitch);
		
		// Every N/noverlap samples, run pitch estimation / correction code
		if ((x->cbiwr)%(N/x->noverlap) == 0) {
			
			//fprintf(stderr,"ti4=%d N=%d\n", ti4, N);
			// ---- Obtain autocovariance ---- //
			
			// Window and fill FFT buffer
			ti2 = (long) x->cbiwr;
			for (ti=0; ti<(long)N; ti++) {
				x->ffttime[ti] = (float)(x->cbi[(ti2-ti)%N]*x->cbwindow[ti]);
			}
			
			// Calculate FFT
			fft_forward(x->fx, x->ffttime, x->fftfreqre, x->fftfreqim);
			
			// Remove DC
			x->fftfreqre[0] = 0;
			x->fftfreqim[0] = 0;
			
			// Take magnitude squared
			for (ti=1; ti< (long) Nf; ti++) {
				x->fftfreqre[ti] = (x->fftfreqre[ti])*(x->fftfreqre[ti]) + (x->fftfreqim[ti])*(x->fftfreqim[ti]);
				x->fftfreqim[ti] = 0;
			}
			
			// Calculate IFFT
			fft_inverse(x->fx, x->fftfreqre, x->fftfreqim, x->ffttime);
			
			// Normalize
			for (ti=1; ti<(long)N; ti++) {
				x->ffttime[ti] = x->ffttime[ti] / x->ffttime[0];
			}
			x->ffttime[0] = 1;
			
			//  ---- END Obtain autocovariance ----
			
			
			//  ---- Calculate pitch and confidence ----
			
			// Calculate pitch period
			//   Pitch period is determined by the location of the max (biased)
			//   peak within a given range
			//   Confidence is determined by the corresponding unbiased height
			tf2 = 0;
			pperiod = pmin;
			for (ti=nmin; ti<(long)nmax; ti++) {
				ti2 = ti-1;
				ti3 = ti+1;
				if (ti2<0) {
					ti2 = 0;
				}
				if (ti3>(long)Nf) {
					ti3 = Nf;
				}
				tf = x->ffttime[ti];
				
				if (tf>x->ffttime[ti2] && tf>=x->ffttime[ti3] && tf>tf2) {
					tf2 = tf;
					ti4 = ti;
					//conf = tf*x->acwinv[ti];
					//pperiod = (float)ti/fs;
				}
			}
			if (tf2>0) {
				conf = tf2*x->acwinv[ti4];
				if (ti4>0 && ti4<(long)Nf) {
					// Find the center of mass in the vicinity of the detected peak
					tf = x->ffttime[ti4-1]*(ti4-1);
					tf = tf + x->ffttime[ti4]*(ti4);
					tf = tf + x->ffttime[ti4+1]*(ti4+1);
					tf = tf/(x->ffttime[ti4-1] + x->ffttime[ti4] + x->ffttime[ti4+1]);
					pperiod = tf/fs;
				}
				else {
					pperiod = (float)ti4/fs;
				}
			}
			
			// Convert to semitones
			tf = (float) -12*log10((float)aref*pperiod)*L2SC;
			//fprintf(stderr,"tf=%f aref=%f pperiod=%f\n", tf, aref, pperiod);
			//post("pperiod=%f conf=%f\n", pperiod, conf);
			float pp_test = x->pperiod/(x->pperiod - pperiod);
			if (pp_test < 0.5 || pp_test > 2)
				pp_test = 1;
			else
				pp_test = 0;
			if (conf>=x->vthresh && tf == tf) { // second check is for NANs
				inpitch = tf;
				x->inpitch = tf; // update pitch only if voiced
				x->pperiod = pperiod;
			}
			x->conf = conf;

			x->fPitch = inpitch;
			x->fConf = conf;

			//x->pitch = pitch;
			//x->pperiod = pperiod;
			//x->conf = conf;
			
			//  ---- END Calculate pitch and confidence ----
			
			
			/*
			//  ---- Determine pitch target ----
			
			// If voiced
			if (conf>=x->vthresh) {
				// TODO: Scale sliders
				// Determine pitch target
				tf = -1;
				tf2 = 0;
				tf3 = 0;
				for (ti=0; ti<12; ti++) {
					switch (ti) {
						case 0:
							tf2 = fA;
							break;
						case 1:
							tf2 = fBb;
							break;
						case 2:
							tf2 = fB;
							break;
						case 3:
							tf2 = fC;
							break;
						case 4:
							tf2 = fDb;
							break;
						case 5:
							tf2 = fD;
							break;
						case 6:
							tf2 = fEb;
							break;
						case 7:
							tf2 = fE;
							break;
						case 8:
							tf2 = fF;
							break;
						case 9:
							tf2 = fGb;
							break;
						case 10:
							tf2 = fG;
							break;
						case 11:
							tf2 = fAb;
							break;
					}
					// 	  if (ti==x->ptarget) {
					// 	    tf2 = tf2 + 0.01; // add a little hysteresis
					// 	  }
					tf2 = tf2 - (float)fabs( (pitch-(float)ti)/6 - 2*floorf(((pitch-(float)ti)/12 + 0.5)) ); // like a likelihood function
					if (tf2>=tf) {                                                                           // that we're maximizing
						tf3 = (float)ti;                                                                       // to find the target pitch
						tf = tf2;
					}
				}
				x->ptarget = tf3;
				
				// Glide persist
				if (x->wasvoiced == 0) {
					x->wasvoiced = 1;
					tf = x->persistamt;
					x->sptarget = (1-tf)*x->ptarget + tf*x->sptarget;
					x->persistamt = 1;
				}
				
				// Glide on circular scale
				tf3 = (float)x->ptarget - x->sptarget;
				tf3 = tf3 - (float)12*floorf(tf3/12 + 0.5);
				if (fGlide>0) {
					tf2 = (float)1-pow((float)1/24, (float)N * 1000/ (x->noverlap*fs*fGlide));
				}
				else {
					tf2 = 1;
				}
				x->sptarget = x->sptarget + tf3*tf2;
			}
			// If not voiced
			else {
				x->wasvoiced = 0;
				
				// Keep track of persist amount
				if (fPersist>0) {
					tf = pow((float)1/2, (float)N * 1000/ (x->noverlap*fs*fPersist));
				}
				else {
					tf = 0;
				}
				x->persistamt = x->persistamt * tf; // Persist amount decays exponentially
			}
			// END If voiced
			
			//  ---- END Determine pitch target ----
			
			
			// ---- Determine correction to feed to the pitch shifter ----
			tf = x->sptarget - pitch; // Correction amount
			tf = tf - (float)12*floorf(tf/12 + 0.5); // Never do more than +- 6 semitones of correction
			if (conf<x->vthresh) {
				tf = 0;
			}
			x->lrshift = fShift + fAmount*tf;  // Add in pitch shift slider
			
			
			// ---- Compute variables for pitch shifter that depend on pitch ---
			x->phincfact = (float)pow(2, x->lrshift/12);
			if (conf>=x->vthresh) {  // Keep old period when unvoiced
				x->inphinc = (float)1/(pperiod*fs);
				x->phprd = pperiod*2;
			}
		}
		// ************************
		// * END Low-Rate Section *
		// ************************
		*/
			//fprintf(stderr,"%f %f %f %f", inpitch, outpitch, pperiod, ti4);

			//  ---- Modify pitch in all kinds of ways! ----

			outpitch = inpitch;

			//fprintf(stderr,"outpitch=%f\n", outpitch);

			// Pull to fixed pitch

			// when fPull is 1 (legacy behavior which picks absolute pitch in respect to A intonation)
			if (fPull <= 1)
			{
				outpitch = (1-fPull)*outpitch + fPull*fFixed;
			}
			else
			{
				// Special pull case when fPull is 2
				/*if (fFixed < 0)
					while (fFixed < 0)
						fFixed += 12;
				else if (fFixed > 12)
					while (fFixed > 12)
						fFixed -= 12;*/

				float inpitch_norm = inpitch;
				if (inpitch_norm < 6)
					while (inpitch_norm < 6)
						inpitch_norm += 12;
				else if (inpitch_norm > 6)
					while (inpitch_norm > 6)
						inpitch_norm -= 12;
				/*float a = fFixed - inpitch_norm;
				float b = fFixed - 12 - inpitch_norm;
				float c = fFixed + 12 - inpitch_norm;
				float result = a;
				if (abs(b) < abs(result)) result = b;
				if (abs(c) < abs(result)) result = c;
				outpitch = inpitch + result;*/
				float a = inpitch - inpitch_norm;
				float b = inpitch - 12 - inpitch_norm;
				float c = inpitch + 12 - inpitch_norm;
				//post("a=%f b=%f c=%f in_norm=%f\n", a, b, c, inpitch_norm);
				float result = a;
				if (abs(b) < abs(result)) result = b;
				if (abs(c) < abs(result)) result = c;
				outpitch = result + fFixed;
				//fprintf(stderr,"outpitch=%f inpitch=%f in_norm=%f\n", outpitch, inpitch, inpitch_norm);

			}

			// -- Convert from semitones to scale notes --
			ti = (int)(outpitch/12 + 32) - 32; // octave
			tf = outpitch - ti*12; // semitone in octave
			ti2 = (int)tf;
			ti3 = ti2 + 1;
			// a little bit of pitch correction logic, since it's a convenient place for it
			if (iNotes[ti2%12]<0 || iNotes[ti3%12]<0) { // if between 2 notes that are more than a semitone apart
				lowersnap = 1;
				uppersnap = 1;
			}
			else {
				lowersnap = 0;
				uppersnap = 0;
				if (iNotes[ti2%12]==1) { // if specified by user
					lowersnap = 1;
				}
				if (iNotes[ti3%12]==1) { // if specified by user
					uppersnap = 1;
				}
			}
			// (back to the semitone->scale conversion)
			// finding next lower pitch in scale
			while (iNotes[(ti2+12)%12]<0) {
				ti2 = ti2 - 1;
			}
			// finding next higher pitch in scale
			while (iNotes[ti3%12]<0) {
				ti3 = ti3 + 1;
			}
			tf = (tf-ti2)/(ti3-ti2) + iPitch2Note[(ti2+12)%12];
			if (ti2<0) {
				tf = tf - numNotes;
			}
			outpitch = tf + numNotes*ti;
			// -- Done converting to scale notes --

			// The actual pitch correction
			ti = (int)(outpitch+128) - 128;
			tf = outpitch - ti - 0.5;
			ti2 = ti3-ti2;
			if (ti2>2) { // if more than 2 semitones apart, put a 2-semitone-like transition halfway between
				tf2 = (float)ti2/2;
			}
			else {
				tf2 = (float)1;
			}
			if (fSmooth<0.001) {
				tf2 = tf*tf2/0.001;
			}
			else {
				tf2 = tf*tf2/fSmooth;
			}
			if (tf2<-0.5) tf2 = -0.5;
			if (tf2>0.5) tf2 = 0.5;
			tf2 = 0.5*sin(PI*tf2) + 0.5; // jumping between notes using horizontally-scaled sine segment
			tf2 = tf2 + ti;
			if ( (tf<0.5 && lowersnap) || (tf>=0.5 && uppersnap) ) {
				outpitch = fAmount*tf2 + ((float)1-fAmount)*outpitch;
			}

			// Add in pitch shift
			outpitch = outpitch + fShift;

			// LFO logic
			tf = fLforate*N/(x->noverlap*fs);
			if (tf>1) tf=1;
			x->lfophase = x->lfophase + tf;
			if (x->lfophase>1) x->lfophase = x->lfophase-1;
			lfoval = x->lfophase;
			tf = (fLfosymm + 1)/2;
			if (tf<=0 || tf>=1) {
				if (tf<=0) lfoval = 1-lfoval;
			}
			else {
				if (lfoval<=tf) {
					lfoval = lfoval/tf;
				}
				else {
					lfoval = 1 - (lfoval-tf)/(1-tf);
				}
			}
			if (fLfoshape>=0) {
				// linear combination of cos and line
				lfoval = (0.5 - 0.5*cos(lfoval*PI))*fLfoshape + lfoval*(1-fLfoshape);
				lfoval = fLfoamp*(lfoval*2 - 1);
			}
			else {
				// smoosh the sine horizontally until it's squarish
				tf = 1 + fLfoshape;
				if (tf<0.001) {
					lfoval = (lfoval - 0.5)*2/0.001;
				}
				else {
					lfoval = (lfoval - 0.5)*2/tf;
				}
				if (lfoval>1) lfoval = 1;
				if (lfoval<-1) lfoval = -1;
				lfoval = fLfoamp*sin(lfoval*PI*0.5);
			}
			// add in quantized LFO
			if (iLfoquant>=1) {
				outpitch = outpitch + (int)(numNotes*lfoval + numNotes + 0.5) - numNotes;
			}


			// Convert back from scale notes to semitones
			outpitch = outpitch + iScwarp; // output scale rotate implemented here
			ti = (int)(outpitch/numNotes + 32) - 32;
			tf = outpitch - ti*numNotes;
			ti2 = (int)tf;
			ti3 = ti2 + 1;
			outpitch = iNote2Pitch[ti3%numNotes] - iNote2Pitch[ti2];
			if (ti3>=numNotes) {
				outpitch = outpitch + 12;
			}
			outpitch = outpitch*(tf - ti2) + iNote2Pitch[ti2];
			outpitch = outpitch + 12*ti;
			outpitch = outpitch - (iNote2Pitch[iScwarp] - iNote2Pitch[0]); //more scale rotation here

			// add in unquantized LFO
			if (iLfoquant<=0) {
				outpitch = outpitch + lfoval*2;
			}

			if (outpitch<-36) outpitch = -48;
			if (outpitch>24) outpitch = 24;

			x->outpitch = outpitch;

			//  ---- END Modify pitch in all kinds of ways! ----

			// Compute variables for pitch shifter that depend on pitch
			x->inphinc = aref*pow(2,inpitch/12)/fs;
			x->outphinc = aref*pow(2,outpitch/12)/fs;
			x->phincfact = x->outphinc/x->inphinc;
		}
		// ************************
		// * END Low-Rate Section *
		// ************************
		
		
		// *****************
		// * Pitch Shifter *
		// *****************
		
	    // Pitch shifter (kind of like a pitch-synchronous version of Fairbanks' technique)
	    //   Note: pitch estimate is naturally N/2 samples old
		x->phasein = x->phasein + x->inphinc;
		x->phaseout = x->phaseout + x->inphinc*x->phincfact;
		
		//   If it happens that there are no snippets placed at the output, grab a new snippet!
		/*     if (x->cbonorm[((long int)x->cbord + (long int)(N/2*(1 - (float)1 / x->phincfact)))%N] < 0.2) { */
		/*       post( "help!"); */
		/*       x->phasein = 1; */
		/*       x->phaseout = 1; */
		/*     } */
		
		//   When input phase resets, take a snippet from N/2 samples in the past
		if (x->phasein >= 1) {
			x->phasein = x->phasein - 1;
			ti2 = x->cbiwr - (long int)N/2;
			for (ti=-((long int)N)/2; ti<(long int)N/2; ti++) {
				x->frag[ti%N] = x->cbi[(ti + ti2)%N];
			}
		}
		
		//   When output phase resets, put a snippet N/2 samples in the future
		if (x->phaseout >= 1) {
			x->fragsize = x->fragsize*2;
			if (x->fragsize >= N) {
				x->fragsize = N;
			}
			x->phaseout = x->phaseout - 1;
			ti2 = x->cbord + N/2;
			ti3 = (long int)(((float)x->fragsize) / x->phincfact);
			if (ti3>=(long int)N/2) {
				ti3 = N/2 - 1;
			}
			for (ti=-ti3/2; ti<(ti3/2); ti++) {
				tf = x->hannwindow[(long int)N/2 + ti*(long int)N/ti3];
				// 3rd degree polynomial interpolator - based on eqns from Hal Chamberlin's book
				indd = x->phincfact*ti;
				ind1 = (int)indd;
				ind2 = ind1+1;
				ind3 = ind1+2;
				ind0 = ind1-1;
				val0 = x->frag[(ind0+N)%N];
				val1 = x->frag[(ind1+N)%N];
				val2 = x->frag[(ind2+N)%N];
				val3 = x->frag[(ind3+N)%N];
				vald = 0;
				vald = vald - (float)0.166666666667 * val0 * (indd - ind1) * (indd - ind2) * (indd - ind3);
				vald = vald + (float)0.5 * val1 * (indd - ind0) * (indd - ind2) * (indd - ind3);
				vald = vald - (float)0.5 * val2 * (indd - ind0) * (indd - ind1) * (indd - ind3);
				vald = vald + (float)0.166666666667 * val3 * (indd - ind0) * (indd - ind1) * (indd - ind2);
				x->cbo[(ti + ti2 + N)%N] = x->cbo[(ti + ti2 + N)%N] + vald*tf;
			}
			x->fragsize = 0;
		}
		x->fragsize++;

		//   Get output signal from buffer
		tf = x->cbo[x->cbord];
		/*//   Normalize
		if (tf>0.5) {
			tf = (float)1/tf;
		}
		else {
			tf = 1;
		}*/
		//tf = tf*x->cbo[x->cbord]; // read buffer
		tf = x->cbo[x->cbord];
		x->cbo[x->cbord] = 0; // erase for next cycle
		//x->cbonorm[x->cbord] = 0;
		x->cbord++; // increment read pointer
		if (x->cbord >= N) {
			x->cbord = 0;
		}
		
		// *********************
		// * END Pitch Shifter *
		// *********************
		
		ti4 = (x->cbiwr + 2)%N;
		if (iFcorr>=1) {
			// The second part of the formant corrector
			// This is a post-filter that re-applies the formants, designed
			//   to result in the exact original signal when no pitch
			//   manipulation is performed.
			// tf is signal input
			// gotta run it 3 times because of a pesky delay free loop
			//  first time: compute 0-response
			tf2 = tf;
			fa = 0;
			fb = fa;
			for (ti=0; ti<ford; ti++) {
				fc = (fb-x->frc[ti])*frlamb + x->frb[ti];
				tf = x->fbuff[ti][ti4];
				fb = fc - tf*fa;
				x->ftvec[ti] = tf*fc;
				fa = fa - x->ftvec[ti];
			}
			tf = -fa;
			for (ti=ford-1; ti>=0; ti--) {
				tf = tf + x->ftvec[ti];
			}
			f0resp = tf;
			//  second time: compute 1-response
			fa = 1;
			fb = fa;
			for (ti=0; ti<ford; ti++) {
				fc = (fb-x->frc[ti])*frlamb + x->frb[ti];
				tf = x->fbuff[ti][ti4];
				fb = fc - tf*fa;
				x->ftvec[ti] = tf*fc;
				fa = fa - x->ftvec[ti];
			}
			tf = -fa;
			for (ti=ford-1; ti>=0; ti--) {
				tf = tf + x->ftvec[ti];
			}
			f1resp = tf;
			//  now solve equations for output, based on 0-response and 1-response
			tf = (float)2*tf2;
			tf2 = tf;
			tf = ((float)1 - f1resp + f0resp);
			if (tf!=0) {
				tf2 = (tf2 + f0resp) / tf;
			}
			else {
				tf2 = 0;
			}
			//  third time: update delay registers
			fa = tf2;
			fb = fa;
			for (ti=0; ti<ford; ti++) {
				fc = (fb-x->frc[ti])*frlamb + x->frb[ti];
				x->frc[ti] = fc;
				x->frb[ti] = fb;
				tf = x->fbuff[ti][ti4];
				fb = fc - tf*fa;
				fa = fa - tf*fc;
			}
			tf = tf2;
			tf = tf + flpa*x->flp;  // lowpass post-emphasis filter
			x->flp = tf;
			// Bring up the gain slowly when formant correction goes from disabled
			// to enabled, while things stabilize.
			if (x->fmute>0.5) {
				tf = tf*(x->fmute - 0.5)*2;
			}
			else {
				tf = 0;
			}
			tf2 = x->fmutealph;
			x->fmute = (1-tf2) + tf2*x->fmute;
			// now tf is signal output
			// ...and we're done messing with formants
		}
		else {
			x->fmute = 0;
		}
		
		// Write audio to output of plugin
		// Mix (blend between original (delayed) =0 and shifted/corrected =1)
		*(out++) = fMix*tf + (1-fMix)*x->cbi[ti4];
		//*(pfOutput++) = (float) fMix*tf + (1-fMix)*x->cbi[(x->cbiwr - N + 1)%N];
		
	}

    return (w + 5); // always add one more than the 2nd argument in dsp_add()
}

void autotune_mix(t_autotune *x, t_floatarg f)
{
	x->fMix   = CLIP(f,0.,1.);
	//post("mix=%f", x->fMix);
}

void autotune_shift(t_autotune *x, t_floatarg f)
{
	x->fShift   = CLIP(f,-12.,12.);
	//post("shift=%f", x->fShift);
}

void autotune_pull(t_autotune *x, t_floatarg f)
{
	x->fPull   = CLIP(f,0.,2.);
	//post("pull=%f", x->fPull);
}

void autotune_pull_pitch(t_autotune *x, t_floatarg f)
{
	x->fFixed  = CLIP(f,-36.,36.);
	//post("pullpitch=%f", x->fFixed);
}

void autotune_tune(t_autotune *x, t_floatarg f)
{
	x->fTune  = CLIP(f,400.,480.);
	//post("tune=%f", x->fTune);
}

void autotune_correction(t_autotune *x, t_floatarg f)
{
	x->fAmount   = CLIP(f,0.,1.);
	//post("correction=%f", x->fAmount);
}

void autotune_smooth(t_autotune *x, t_floatarg f)
{
	x->fSmooth   = CLIP(f,0.,1.);
	//post("smooth=%f", x->fSmooth);
}

void autotune_lfo_depth(t_autotune *x, t_floatarg f)
{
	x->fLfoamp   = CLIP(f,0.,1.);
	//post("smooth=%f", x->fSmooth);
}

void autotune_lfo_rate(t_autotune *x, t_floatarg f)
{
	x->fLforate   = CLIP(f,0., x->fs/2);
	//post("lfo_rate=%f", x->fLforate);
}

void autotune_lfo_shape(t_autotune *x, t_floatarg f)
{
	//square -> sine ->tri (-1 to 1)
	x->fLfoshape   = CLIP(f,-1.,1.);
	//post("lfo_shape=%f", x->fLfoshape);
}

void autotune_lfo_symmetry(t_autotune *x, t_floatarg f)
{
	x->fLfosymm   = CLIP(f,-1.,1.);
	//post("symmetry=%f", x->fLfosymm);
}

void autotune_formant_correction(t_autotune *x, t_floatarg f)
{
	x->fFcorr   = CLIP((int)f, 0.,1.);
	//post("fcorr=%f", x->fFcorr);
}

void autotune_formant_warp(t_autotune *x, t_floatarg f)
{
	x->fFwarp   = CLIP(f,-1.,1.);
	//post("warp=%f", x->fFwarp);
}

void autotune_scale_warp(t_autotune *x, t_floatarg f)
{
	x->fScwarp   = CLIP(f,-1.,1.);
	//post("scwarp=%f", x->fScwarp);
}

void autotune_confidence(t_autotune *x, t_floatarg f)
{
	x->vthresh   = CLIP(f,0.,1.);
	//post("vthresh=%f", x->vthresh);
}

void autotune_list(t_autotune *x, t_symbol *m, short argc, t_atom *argv)
{
	if(argc== 12)
	{
		x->fA     = CLIP(atom_getfloat(argv), -1., 1.)  ;
		x->fBb    = CLIP(atom_getfloat(argv+1), -1., 1.)  ;
		x->fB     = CLIP(atom_getfloat(argv+2), -1., 1.)  ;
		x->fC     = CLIP(atom_getfloat(argv+3), -1., 1.)  ;  
		x->fDb    = CLIP(atom_getfloat(argv+4), -1., 1.)  ;  
		x->fD     = CLIP(atom_getfloat(argv+5), -1., 1.)  ;  
		x->fEb    = CLIP(atom_getfloat(argv+6), -1., 1.)  ;  
		x->fE     = CLIP(atom_getfloat(argv+7), -1., 1.)  ;  
		x->fF     = CLIP(atom_getfloat(argv+8), -1., 1.)  ;  
		x->fGb    = CLIP(atom_getfloat(argv+9), -1., 1.)  ;  
		x->fG     = CLIP(atom_getfloat(argv+10), -1., 1.)  ;  
		x->fAb    = CLIP(atom_getfloat(argv+11), -1., 1.)  ;
		/*post("pitches: %d %d %d %d %d %d %d %d %d %d %d %d",
			(int)x->fA, (int)x->fBb, (int)x->fB, (int)x->fC, (int)x->fDb,
			(int)x->fD, (int)x->fEb, (int)x->fE, (int)x->fF, (int)x->fGb,
			(int)x->fG, (int)x->fAb);*/
	}
	else {post("bad list"); };

}

void autotune_processclock(t_autotune *x)
{
	clock_delay(x->clock, x->clockinterval); // schedule the next clock
	
	outlet_float(x->confout, x->conf);
	outlet_float(x->periodout, 1.f / x->pperiod);
}

void autotune_assist(t_autotune *x, void *b, long m, long a, char *s)
{
	if (m == 1) //input
	{
		sprintf(s,"Signal Input, Messages");
	}
	else
	{
		switch (a) {	
		case 0:
			sprintf(s,"Signal Output");
			break;
		case 1:
				sprintf(s,"confidence (0-1)(float)");
				break;
		case 2:
				
				sprintf(s,"frequency (hz) (float)");
				break;
				
		}
	}
	
}
