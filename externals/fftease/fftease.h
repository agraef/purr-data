/* 32-bit version for Pd */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "m_pd.h"

#define MAX_N 1073741824
#define MAX_Nw MAX_N

#define FFTEASE_ANNOUNCEMENT "<[ FFTease 3.0 ]>   |  "
#define FFTEASE_VERSION "FFTease 3.0 for Pd 32-bit version"
#define FFTEASE_COMPILE_DATE "June 20, 2014"

#define fftease_announce(objname)  post("%s ( %s )",FFTEASE_ANNOUNCEMENT,objname)

#define fftease_version(objectname) post("%s: version %s compiled %s",objectname,FFTEASE_VERSION,FFTEASE_COMPILE_DATE);

#define BIGGER_THAN_MSP_VECTOR 0
#define SMALLER_THAN_MSP_VECTOR 1
#define EQUAL_TO_MSP_VECTOR 2

#define FFTEASE_DEFAULT_FFTSIZE 1024
#define FFTEASE_DEFAULT_OVERLAP 8
#define FFTEASE_DEFAULT_WINFAC 1
	
#define DEFAULT_FFTEASE_FFTSIZE 1024
#define FFTEASE_MAX_FFTSIZE 1073741824

#define FFTEASE_OSCBANK_SCALAR (0.25)
#define FFTEASE_OSCBANK_TABLESIZE (8192)
#define FFTEASE_BYPASS_GAIN (0.5)

#define FFT_FORWARD 1
#define FFT_INVERSE -1

#ifndef PIOVERTWO
#define PIOVERTWO 1.5707963268
#endif
#ifndef TWOPI
#define TWOPI 6.2831853072
#endif
#ifndef PI
#define PI 3.14159265358979
#endif

typedef struct _fftease
{
	int R;
	int	N;
	int	N2;
	int	Nw;
	int	Nw2; 
	int	D; 
	int	in_count;
	int out_count;
	t_float *Wanal;
	t_float *Wsyn;
	t_float *input;
	t_float *Hwin;
	t_float *buffer;
	t_float *channel;
	t_float *output;
	// for convert
	t_float *c_lastphase_in;
	t_float *c_lastphase_out;
	t_float c_fundamental;
	t_float c_factor_in;
	t_float c_factor_out;
	// for oscbank
	int NP;
	t_float P;
	int L;
	int first;
	t_float Iinv;
	t_float *lastamp;
	t_float *lastfreq;
	t_float *bindex;
	t_float *table;
	t_float pitch_increment;
	t_float ffac;
	int hi_bin;
	int lo_bin;
	// for fast fft
	t_float mult;
	t_float *trigland;
	int *bitshuffle;
	int overlap;
	int winfac;
    int last_overlap; // save values to test if memory reallocation needed
    int last_winfac;
    int last_N;
    int last_R;
	t_float synt;
	t_float *internalInputVector; // hold input data from smaller MSP buffers
	t_float *internalOutputVector; // hold output data for smaller MSP buffers
	int operationRepeat; // how many times to do whatever on each perform call
	int operationCount; // keep track of where we are in buffer operation
	int bufferStatus; // relations between MSP vector size and internal buffer size
	int MSPVectorSize; // what it says
	short obank_flag; // resynthesis method flag
	short init_status; // whether initialization has successfully occurred
	short noalias; // inhibit aliasing in oscbank mode
	t_float nyquist; // nyquest frequency == R/2
	short initialized; // set to 0 for the first time in new(); after that it will be 1
} t_fftease;



void fftease_convert(t_fftease *fft);
void fftease_unconvert(t_fftease *fft);
void fftease_rfft( t_float *x, int N, int forward );
void fftease_cfft( t_float *x, int NC, int forward );
void fftease_bitreverse( t_float *x, int N );
void fftease_fold( t_fftease *fft );
void fftease_init_rdft(int n, int *ip, t_float *w);
void fftease_rdft(t_fftease *fft, int isgn);
//void fftease_bitrv2(int n, int *ip, t_float *a);
//void fftease_cftsub(int n, t_float *a, t_float *w);
//void rftsub(int n, t_float *a, int nc, t_float *c);
void fftease_makewt(int nw, int *ip, t_float *w);
void fftease_makect(int nc, int *ip, t_float *c);
void fftease_leanconvert(t_fftease *fft);
void fftease_leanunconvert(t_fftease *fft);
void fftease_makewindows( t_float *H, t_float *A, t_float *S, int Nw, int N, int I );
void fftease_makehamming( t_float *H, t_float *A, t_float *S, int Nw, int N, int I,int odd );
void fftease_makehanning( t_float *H, t_float *A, t_float *S, int Nw, int N, int I,int odd );
void fftease_overlapadd(t_fftease *fft);
void fftease_bloscbank( t_float *S, t_float *O, int D, t_float iD, t_float *lf, t_float *la,
	t_float *bindex, t_float *tab, int len, t_float synt, int lo, int hi );
void fftease_oscbank( t_fftease *fft );
//t_float randf( t_float min, t_float max );
//int randi( int min, int max );
int fftease_power_of_two(int test);
void fftease_limit_fftsize(int *N, int *Nw, char *OBJECT_NAME);
int fftease_fft_size(int testfft);
void fftease_free(t_fftease *fft);
void fftease_init(t_fftease *fft);
int fftease_winfac(int winfac);
int fftease_overlap(int overlap);
void fftease_set_fft_buffers(t_fftease *fft);
void fftease_fftinfo(t_fftease *fft, char *object_name);
int fftease_msp_sanity_check(t_fftease *fft, char *oname);
t_float fftease_randf(t_float min, t_float max);
void fftease_noalias(t_fftease* fft, short flag);
void fftease_oscbank_setbins(t_fftease *fft, t_float lowfreq, t_float highfreq);
void fftease_limited_oscbank(t_fftease *fft, int osclimit, t_float framethresh);
t_float fftease_randf(t_float min, t_float max);
// Penrose extras
t_float fftease_frequencyToIncrement( t_float samplingRate, t_float frequency, int bufferLength );
void fftease_makeSineBuffer( t_float *buffer, int bufferLength );
t_float fftease_bufferOscil( t_float *phase, t_float increment, t_float *buffer, int bufferLength );
float fftease_rrand(int *seed);
float fftease_prand(int *seed);

/*** MSP helper functions, thanks JKC! ***/
/*
void atom_arg_getfloat(float *c, long idx, long ac, t_atom *av);
void atom_arg_getsym(t_symbol **c, long idx, long ac, t_atom *av);

void atom_arg_getfloat(float *c, long idx, long ac, t_atom *av)
{
    if (c&&ac&&av&&(idx<ac)) {
        *c = atom_getfloat(av+idx);
    }
}

void atom_arg_getsym(t_symbol **c, long idx, long ac, t_atom *av)
{
	if (c&&ac&&av&&(idx<ac)) {
		*c = atom_getsymbol(av+idx);
	} 
}
*/

