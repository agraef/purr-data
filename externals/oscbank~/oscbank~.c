/* ------------------------ oscbank~ 0.1 ----------------------------- */
// oscillator bank using 3 seperate float inlets with interpolation
// author - Richie Eakin  reakinator@gmail.com 10-15-2007
/* ----------------------------------------------------------------*/

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef NT
#pragma warning( disable : 4244 )
#define inline
#endif

#define WAVETABLESIZE 65536 //2^16
#define DEFAULT_NPARTIALS 100
#define DEFAULT_interp_incr 0.0045 //per sample, this is 20 ms @ 44k sr 

static t_class *oscbank_class;

//t_partial represents one partial member in the bank
typedef struct _partial
{
    int   index;
    float fCurr;
    float freq;
    float fIncr;
    float aCurr;
    float amp;
    float aIncr;
    float phase;
    unsigned long  nInterp;
} t_partial;

typedef struct _oscbank
{
    t_object x_obj; 	
    float    *wavetable;	
    int      wavetablesize;
    int      got_a_table;
    t_partial *pBank;
    float    infreq;	
    float    inamp;
    float    sampleRate;
    float    sampleperiod; 
    float    interp_incr;
    long     interpSamples;
    int      sp;
    int      nPartials;
} t_oscbank;

/*----- Interpolation Time -----
  milleseconds to interpolate over; so samples = (n*SR)/1000
  divide only when converting the interp time to samples(here),since it 
  is only used as a denominator to find the increment proportion:
  SP= 1/SR, 1/(n*SR/1000) = (1000*SP)/n
*/
static void oscbank_interpMs(t_oscbank *x, t_floatarg n)
{
    
    if(n > 0) x->interp_incr =(1000* x->sampleperiod)/ n ;
    else x->interp_incr = x->sampleperiod; 
    
    x->interpSamples = (unsigned long)((n *.001) * x->sampleRate);
}

static void oscbank_nPartials(t_oscbank *x, t_floatarg n)
{
    x->pBank = (t_partial *)resizebytes( x->pBank, x->nPartials * sizeof(t_partial), \
					 n * sizeof(t_partial));
    x->nPartials = n;
    post("max partials: %d", x->nPartials);
}

static void oscbank_index(t_oscbank *x, t_floatarg in)
{
	int i, iindex;
	iindex = (int)in;
	t_partial *bank = x->pBank; 

	    
     	if( iindex < 0)
	{
	    error("negative index rejected");
	    return;
	}

//TODO: find open index in first loop, use that instead of second loop
//check if continuing partial
  for(i =0; i < x->nPartials; i++)
  {
      if( bank[i].index == iindex)
      {//recaluclate increment slope from current interpolated positions and update goal
	  if(bank[i].aCurr == 0) bank[i].aCurr = 0.0000001;
	  bank[i].fIncr = (x->infreq - bank[i].fCurr) * x->interp_incr;
	  bank[i].aIncr = (x->inamp - bank[i].aCurr) * x->interp_incr;
	  bank[i].freq = x->infreq;
	  bank[i].amp = x->inamp;
	  bank[i].nInterp = x->interpSamples;
	  return;
      }
  } //end continuing partial

  //new partial, see if there is an empty slot for the new partial
  for(i =0; i < x->nPartials; i++)
  {
      if(bank[i].aCurr == 0)
      { //new partial, only ramp amp from zero, 
	  bank[i].index = iindex;
	  bank[i].fCurr = x->infreq;
	  bank[i].fIncr = 0;
	  bank[i].freq = x->infreq;
	  bank[i].amp = x->inamp;
	  bank[i].nInterp = x->interpSamples;
	  bank[i].aCurr = 0.0000001;  
	  bank[i].aIncr = x->inamp * x->interp_incr;
	  return;
      }
  } //end new partial for
 
  //oscbank is full, steal oldest partial (creates a pop) and  ramp amp from zero
  bank[x->sp].index = iindex;
  bank[x->sp].fCurr = x->infreq;
  bank[x->sp].fIncr = 0;
  bank[x->sp].freq = x->infreq;
  bank[x->sp].amp = x->inamp;
  bank[x->sp].nInterp = x->interpSamples;
  bank[x->sp].aCurr = 0.0000001;
  bank[x->sp].aIncr = x->inamp * x->interp_incr; 
  x->sp++;
  if(x->sp == x->nPartials) x->sp = 0;
}

static void oscbank_table(t_oscbank *x, t_symbol *tablename)
{
    if(!x->got_a_table) free(x->wavetable);

    t_garray *a;

    if (!(a = (t_garray *)pd_findbyclass(tablename, garray_class)))
        pd_error(x, "%s: no such array", tablename->s_name);
    else if (!garray_getfloatarray(a, &x->wavetablesize, &x->wavetable))
        pd_error(x, "%s: bad template for tabread", tablename->s_name);
    else //table exists
    {
        post("wavetablesize: %d", x->wavetablesize );
    }
    x->got_a_table = 1;
}

static void oscbank_print(t_oscbank *x)
{
    t_partial *bank = x->pBank; 
    
    post("#.  Index,  Freq,  Amp");
    int i;
    
    for(i=0; i < x->nPartials; i++)//for every partial
    {
	if(bank[i].aCurr)
	{
	    post("%d. index: %d,freq: %f,amp: %f", i, bank[i].index, 
		 bank[i].freq,  bank[i].amp );
	}
    }
}

//TODO: this is crashing shit... whhhaaat
static void oscbank_reset(t_oscbank *x)
{
    memset(x->pBank, 0, x->nPartials * sizeof(t_partial));
}

static t_int *oscbank_perform(t_int *w)
{
    t_oscbank *x = (t_oscbank *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    t_int n = (t_int)(w[3]);
    t_int i, sample;    
    t_float phaseincrement;
    t_float sample_sum, freq, amp;
    t_int	lookup;
    t_partial *bank = x->pBank; 

    //clear output buffer so we can add to it starting at 0
    memset( out , 0, n *sizeof( t_float ));

    for(i=0; i < x->nPartials; i++)//for every partial
    {
	if(bank[i].aCurr != 0)
	{
	    for(sample = 0; sample < n; sample++)//and every sample..
	    {
		if(bank[i].nInterp > 0)
		{
		    bank[i].fCurr += bank[i].fIncr;
		    bank[i].aCurr += bank[i].aIncr;
		    --bank[i].nInterp;
		}
		else
		{
		    bank[i].fCurr = bank[i].freq;
		    bank[i].aCurr = bank[i].amp;
		}
		
		// get the phase increment freq = cyc/sec,
		//sr = samp/sec, phaseinc = cyc/samp = freq/sr = freq * sampleperiod
		phaseincrement = bank[i].fCurr * x->sampleperiod;
		bank[i].phase += phaseincrement;
		while(bank[i].phase >= 1.0f) //..and wrap
		    bank[i].phase -= 1.0f;
		while(bank[i].phase < 0.0f)
		    bank[i].phase += 1.0f;
		
		lookup = (int)(x->wavetablesize * bank[i].phase); 
		
		*(out+sample) += *(x->wavetable + lookup) * bank[i].aCurr; 
	    }//end for samples
	} //end if x->index
    }//end for partials
    return (w+4);
}

static void oscbank_dsp(t_oscbank *x, t_signal **sp)
{
    x->sampleRate =  sp[0]->s_sr;
    x->sampleperiod = 1 / x->sampleRate;
    dsp_add(oscbank_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void *oscbank_new(void)
{
    t_oscbank *x = (t_oscbank *)pd_new(oscbank_class);
    
    float twopi, size;
    int i;

    outlet_new(&x->x_obj, gensym("signal"));
    floatinlet_new(&x->x_obj, &x->infreq);
    floatinlet_new(&x->x_obj, &x->inamp);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("interp"));

    //hardcoded because dsp hasn't been turned on yet
    //prevents devide by zero in oscbank_index()
    x->sampleRate = 48000;
    x->sampleperiod = 1 / x->sampleRate;
    oscbank_interpMs( x, 20.0); 

    x->got_a_table = 0;
    x->sp = 0;
    x->nPartials = DEFAULT_NPARTIALS;
    x->pBank = (t_partial *)getbytes( x->nPartials * sizeof(t_partial));
    memset(x->pBank, 0, x->nPartials * sizeof(t_partial));
	   
    twopi = 8.0f * atan(1.0f);
    x->wavetablesize = WAVETABLESIZE;
    float *sinewave;
    sinewave = (t_float *)malloc(x->wavetablesize * sizeof(t_float));
    for(i = 0; i < x->wavetablesize; i++)
	sinewave[i] = sin(twopi * (float)i/ x->wavetablesize);
    
    x->wavetable = &sinewave[0];
    
    return (x);
}

static void oscbank_free(t_oscbank *x)
{
    free(x->pBank);
    if(!x->got_a_table)
	free(x->wavetable);
}

void oscbank_tilde_setup(void)
{
    oscbank_class = class_new(gensym("oscbank~"),(t_newmethod)oscbank_new,\
	      (t_method)oscbank_free,sizeof(t_oscbank), 0, A_DEFFLOAT, 0);
    class_addfloat(oscbank_class, oscbank_index);
    class_addmethod(oscbank_class, (t_method)oscbank_table, gensym("table"), A_SYMBOL);
    class_addmethod(oscbank_class, (t_method)oscbank_interpMs, gensym("interp"), A_FLOAT, 0);
    class_addmethod(oscbank_class, (t_method)oscbank_dsp, gensym("dsp"), A_CANT, (t_atomtype)0);
    class_addmethod(oscbank_class, (t_method)oscbank_print, gensym("print"), 0);
    class_addmethod(oscbank_class, (t_method)oscbank_reset, gensym("reset"), 0);
    class_addmethod(oscbank_class, (t_method)oscbank_nPartials, gensym("partials"), A_FLOAT, 0);
}
