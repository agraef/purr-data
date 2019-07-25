#include "MSPd.h"
#include "ugens.h"
#include <string.h>
/* calling codes for DSP modules */
#define TRANSPOSE 0
#define RINGMOD 1
#define FLANGE 2
#define BUTTER 3
#define TRUNCATE 4
#define SWEEPRESON 5
#define COMB 6
#define SLIDECOMB 7
#define REVERB1 8
#define ELLIPSE 9
#define COMPDIST 10
#define FEED1 11
#define RETRO 12
#define FLAM1 13
#define FLAM2 14
#define EXPFLAM 15
#define COMB4 16
#define RINGFEED 17
#define RESONADSR 18
#define STV 19	
//////
#define ROOT2 (1.4142135623730950488)
#define PI2 (6.2831853071795862319959)
// #define BUFFER_SIZE (1<<15)
#define LOPASS 0
#define HIPASS 1
#define BANDPASS 2
#define COMBFADE (.04 )
#define MAXFILTER 12 /*must be at least as big as see below */
#define ELLIPSE_FILTER_COUNT 11 /*actual number of filters*/
#define MAX_COEF 48
#define MY_MAX 2147483647.0 /* for rand() */
/*data types */

typedef struct
{
	float *data;//contains cycle data
	int len;//length of array
	int p;//position pointer
} t_cycle;

typedef struct
{
  long phase; // current phase in frames
  double phasef; // current phase in frames
  float gain; // gain for this note
  float gainL;// left gain
  float gainR;// right gain
  short status;// status of this event slot
  float *workbuffer;//sample processing space (both input and output)
  float *inbuf;//pointer to input part of workbuffer
  float *outbuf;//pointer to output part of workbuffer
  int in_start;// location in workbuffer to read from input
  int out_start;// location in workbuffer to write output
  int sample_frames;//actual size in frames of sample, which changes if it gets bigger
  int countdown;//latency counter before we actually start sending out samples
  int out_channels; //number of channels per frame of output
  short completed;//did the defer call do its thing?


} t_event;

typedef struct _bashfest
{
  t_object x_obj;
  float x_f;  
  float sr; // sampling rate
  t_symbol *wavename; // name of waveform buffer
  short hosed; // buffers are bad
  float fadeout; // fadeout time in sample frames (if truncation)
  float sync; // input from groove sync signal
  float increment; // read increment
  int most_recent_event; // position in array where last note was initiated
  long b_nchans; // channels of buffer
  long b_valid; // state of buffer
  long b_frames; // number of frames in sample buffer
  t_word *b_samples; // pointer samples in buffer
  int overlap_max; // max number of simultaneous plays 
  t_event *events; //note attacks
  int active_events; // how many currently activated notes?
  int buf_samps;//total samples in workbuffer
  int halfbuffer;//buf_samps / 2
  int buf_frames;// number of sample frames in workbuffer
  int latency_samples;// amount of samples to count down before playing sample
  float *params; // parameter list
  float *odds;// odds for each process happening
  int max_process_per_note;//what it says
  int min_process_per_note;//ditto
  int new_slot;//position for newest note
  float new_gain;//recently assigned gain
  short verbose;//toggle Max window error reporting
  float work_buffer_size;// size in ms of work buffers
  t_cycle tcycle;//contains an optional transposition cycle
  short block_dsp;//flag to turn off all dsp and play straight from MSP buffer
  short sound_lock;//keep current processed sound in buffer
  short grab;//flag to copy immediate processed buffer into MSP buffer
  char sound_name[256];
  float *trigger_vec;//stores incoming trigger vectors
  int vs;//Max/MSP vector size
  
  /* stuff for bashfest DSP */
  float *sinewave;
  int sinelen;
  short mute;
  float maxdelay;
  float *delayline1;
  float *delayline2;
  LSTRUCT *eel; // for ellipse processor
  float *mini_delay[4]; // small delay lines for allpass filter
  float max_mini_delay ;
  float *transfer_function;
  int tf_len; // length of transfer function
  float *feedfunc1;
  float *feedfunc2;
  float *feedfunc3;
  float *feedfunc4;
  int feedfunclen;
  int flamfunc1len;
  float *flamfunc1;
  CMIXCOMB *combies;
  CMIXADSR *adsr;
  float max_comb_lpt;
  float *reverb_ellipse_data;
  float **ellipse_data;
  float *dcflt;
  CMIXOSC oscar;
  CMIXRESON resies[2];

} t_bashfest;



/*function prototypes*/
void putsine (float *arr, int len);
float boundrand(float min, float max);
void mycombset(float loopt,float rvt,int init,float *a,float srate);
float mycomb(float samp,float *a);
void setweights(float *a, int len);
void delset2(float *a,int *l,float xmax, float srate);
void delput2(float x,float *a,int *l);
float dliget2(float *a,float dwait,int *l,float srate);
void butterLopass( float *in, float *out, float cutoff, int frames, int channels, float srate);
void butterBandpass(float *in, float *out,  float center, float bandwidth, int frames,int  channels, float srate);
void butterHipass(float *in, float *out,  float cutoff, int frames,int channels, float srate);
void butset(float *a);
void lobut(float *a, float cutoff,float SR);
void hibut(float *a, float cutoff, float SR);
void bpbut(float *a, float formant, float bandwidth, float SR);
void butter_filter(float *in,float *out,float *a, int frames, int channels, int channel);
void rsnset2(float cf,float bw,float scl,float xinit,float *a,float srate);
float reson(float x,float *a);

void ellipset(float *list, LSTRUCT *eel, int  *nsects, float *xnorm);
float ellipse(float x, LSTRUCT *eel, int nsects, float xnorm);
float allpass(float samp,float *a);
void init_reverb_data(float *a);
void init_ellipse_data(float **a);

void setExpFlamFunc(float *arr, int flen, float v1,float v2,float alpha);
void setflamfunc1(float *arr, int flen);
void funcgen1(float *outArray, int outlen, float duration, float outMin, float outMax,
	 float speed1, float speed2, float gain1, float gain2, float *phs1, float *phs2, 
	 float *sine, int sinelen);
void normtab(float *inarr,float *outarr, float min, float max, int len);
float mapp(float in,float imin,float imax,float omin,float omax);
float oscil(float amp,float si,float *farray,int len,float *phs);
void set_dcflt(float *a);

void set_distortion_table(float *arr, float cut, float max, int len);
float dlookup(float samp,float *arr,int len);
void do_compdist(float *in,float *out,int sampFrames,int nchans,int channel, 
	    float cutoff,float maxmult,int lookupflag,float *table,int range,float bufMaxamp);
float getmaxamp(float *arr, int len);
void buildadsr(CMIXADSR *a);
/*bashfest dsp functions */
void feed1(float *inbuf, float *outbuf, int in_frames, int out_frames,int channels, float *functab1,
	   float *functab2,float *functab3,float *functab4,int funclen, 
	   float duration, float maxDelay, t_bashfest *x);
void reverb1me(float *in, float *out, int inFrames, int out_frames, int nchans, 
	       int channel, float revtime, float dry, t_bashfest *x);
	       void killdc( float *inbuf, int in_frames, int channels, t_bashfest *x);
