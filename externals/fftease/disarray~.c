#include "MSPd.h"
#include "fftease.h"

#define MEMPAD (1024)

#if MSP
void *disarray_class;
#endif 

#if PD
static t_class *disarray_class;
#endif

#define OBJECT_NAME "disarray~"


typedef struct _disarray
{
#if MSP
  t_pxobject x_obj;
#endif
#if PD
  t_object x_obj;
  float x_f;
#endif  
    int R;
	int	N;
	int	N2;
	int	Nw;
	int	Nw2; 
	int	D; 
	int	i;
	int	inCount;
    float *Wanal;	
   	float *Wsyn;	
    float *input;	
    float *Hwin;
    float *buffer;
    float *channel;
    float *output;
    //
    int *shuffle_in;
    int *shuffle_out;
    int shuffle_count;
    int max_bin;
   //
    float mult; 
    float *trigland;
    int *bitshuffle;
    //
    float top_frequency;
    int overlap;
    int winfac;
    float c_fundamental;

    //
	void *list_outlet;
	t_atom *list_data;
	short mute;
	short bypass;
	short lock;
} t_disarray;

void *disarray_new(t_symbol *msg, short argc, t_atom *argv);
t_int *offset_perform(t_int *w);
t_int *disarray_perform(t_int *w);
void disarray_dsp(t_disarray *x, t_signal **sp, short *count);
void disarray_assist(t_disarray *x, void *b, long m, long a, char *s);
void switch_count (t_disarray *x, t_floatarg i);
void iswitch_count (t_disarray *x, t_int i);
void disarray_topfreq (t_disarray *x, t_floatarg freq);
void reset_shuffle( t_disarray *x );
void disarray_showstate( t_disarray *x );
void disarray_list (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
void disarray_setstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv);
int rand_index( int max);
void disarray_mute(t_disarray *x, t_floatarg toggle);
void disarray_bypass(t_disarray *x, t_floatarg toggle);
void disarray_tilde_setup(void);
void disarray_free(t_disarray *x);

void disarray_overlap(t_disarray *x, t_floatarg o);
void disarray_winfac(t_disarray *x, t_floatarg o);
void disarray_fftinfo(t_disarray *x);
void disarray_init(t_disarray *x, short initialized);

#if MSP
void main(void)
{
    setup((t_messlist **)&disarray_class, (method)disarray_new, 
    (method)disarray_free, (short)sizeof(t_disarray), 0, A_GIMME, 0);
    addmess((method)disarray_dsp, "dsp", A_CANT, 0);
    addint((method)iswitch_count);
    addbang((method)reset_shuffle);
    addmess((method)disarray_showstate,"showstate",0);
    addmess ((method)disarray_list, "list", A_GIMME, 0);
    addmess ((method)disarray_setstate, "setstate", A_GIMME, 0);
    addmess((method)disarray_assist,"assist",A_CANT,0);
    addmess ((method)disarray_mute, "mute", A_FLOAT, 0);
    addmess ((method)disarray_topfreq, "topfreq", A_FLOAT, 0);
    addmess ((method)disarray_bypass, "bypass", A_LONG, 0);
    addmess((method)disarray_overlap, "overlap",  A_DEFFLOAT, 0);
    addmess((method)disarray_winfac, "winfac",  A_DEFFLOAT, 0);
    addmess((method)switch_count, "switch_count",  A_DEFFLOAT, 0);
    addmess((method)disarray_fftinfo, "fftinfo", 0);
  
    dsp_initclass();
	  post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

#if PD
void disarray_tilde_setup(void)
{
  disarray_class = class_new(gensym("disarray~"), (t_newmethod)disarray_new, 
			 (t_method)disarray_free ,sizeof(t_disarray), 0,A_GIMME,0);
  CLASS_MAINSIGNALIN(disarray_class, t_disarray, x_f);
  class_addmethod(disarray_class, (t_method)disarray_dsp, gensym("dsp"), 0);
  class_addmethod(disarray_class, (t_method)disarray_showstate, gensym("showstate"), 0);
  class_addmethod(disarray_class, (t_method)disarray_list, gensym("list"), A_GIMME, 0);
  class_addmethod(disarray_class, (t_method)disarray_mute, gensym("mute"), A_FLOAT, 0);
  class_addmethod(disarray_class, (t_method)disarray_topfreq, gensym("topfreq"), A_FLOAT, 0);
  class_addmethod(disarray_class, (t_method)switch_count, gensym("switch_count"), A_FLOAT, 0);
  class_addmethod(disarray_class, (t_method)reset_shuffle, gensym("bang"),  0);
  class_addmethod(disarray_class, (t_method)disarray_overlap, gensym("overlap"), A_DEFFLOAT,0);
  class_addmethod(disarray_class, (t_method)disarray_winfac, gensym("winfac"), A_DEFFLOAT,0);
  class_addmethod(disarray_class, (t_method)disarray_fftinfo, gensym("fftinfo"), 0);
	post("%s %s",OBJECT_NAME,FFTEASE_ANNOUNCEMENT);
}
#endif

void disarray_free(t_disarray *x)
{
#if MSP
    dsp_free((t_pxobject *) x);
#endif
     free(x->Wanal);	
     free(x->Wsyn);	
     free(x->input);	
     free(x->Hwin);
     free(x->buffer);
     free(x->channel);
     free(x->output);
     free(x->bitshuffle);
     free(x->trigland);
     free(x->list_data) ;
}


void disarray_overlap(t_disarray *x, t_floatarg o)
{
  if(!power_of_two(o)){
    error("%f is not a power of two",o);
    return;
  }
  x->overlap = (int)o;
  disarray_init(x,1);
}

void disarray_winfac(t_disarray *x, t_floatarg f)
{
  if(!power_of_two(f)){
    error("%f is not a power of two",f);
    return;
  }
  x->winfac = (int)f;
  disarray_init(x,2); /* calling lighter reinit routine */
}

void disarray_fftinfo( t_disarray *x )
{
  if( ! x->overlap ){
    post("zero overlap!");
    return;
  }
  post("%s: FFT size %d, hopsize %d, windowsize %d", OBJECT_NAME, x->N, x->N/x->overlap, x->Nw);
}
void disarray_topfreq (t_disarray *x, t_floatarg freq)
{
 float funda = (float) x->R / (2. * (float) x->N) ;
 float curfreq;
 
   
  if( freq < funda || freq > 20000) {
  	freq = 1000.0 ;
  }
  x->max_bin = 1;  
  curfreq = 0;
  while( curfreq < freq ) {
   	 ++(x->max_bin);
    curfreq += funda ;
 }

}

void disarray_assist (t_disarray *x, void *b, long msg, long arg, char *dst)
{
	if (msg==1) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Input "); break;
		}
	} else if (msg==2) {
		switch (arg) {
			case 0: sprintf(dst,"(signal) Output "); break;
			case 1: sprintf(dst,"(list) Current State "); break;
		}
	}
}

void *disarray_new(t_symbol *msg, short argc, t_atom *argv)
{
#if MSP
   t_disarray *x = (t_disarray *)newobject(disarray_class);
  	x->list_outlet = listout((t_pxobject *)x);
    dsp_setup((t_pxobject *)x,1);
    outlet_new((t_pxobject *)x, "signal");
#endif

#if PD
  t_disarray *x = (t_disarray *)pd_new(disarray_class);
  outlet_new(&x->x_obj, gensym("signal"));
  x->list_outlet = outlet_new(&x->x_obj,gensym("list"));
#endif

// INITIALIZATIONS

  	
   	srand( time( 0 ) );

  x->D = sys_getblksize();
  x->R = sys_getsr(); 
  x->top_frequency = atom_getfloatarg(0,argc,argv);
  x->overlap = atom_getintarg(1,argc,argv);
  x->winfac = atom_getintarg(2,argc,argv);
  
  if(!power_of_two(x->overlap))
  	x->overlap = 4;
  if(!power_of_two(x->winfac))
  	x->winfac = 1; 
  disarray_init(x,0);
    return (x);
}


void disarray_init(t_disarray *x, short initialized)
{
	int i;
	float curfreq;
	
	
	if(!power_of_two(x->winfac)){
		x->winfac = 1;
	}
	if(!power_of_two(x->overlap)){
		x->overlap = 4;
	}	
	
	x->N = x->D * x->overlap;
	x->Nw = x->N * x->winfac;	
    limit_fftsize(&x->N,&x->Nw,OBJECT_NAME);
	x->N2 = (x->N)>>1;
	x->Nw2 = (x->Nw)>>1;
	x->inCount = -(x->Nw);
	
	x->c_fundamental =  (float) x->R/(float)( (x->N2)<<1 );
	x->mult = 1. / (float) x->N;
	
	x->lock = 1; 
	
	if(initialized == 0){
		x->Wanal = (float *) calloc( (MAX_Nw) , sizeof(float));	
		x->Wsyn = (float *) calloc( (MAX_Nw), sizeof(float));	
		x->input = (float *) calloc( (MAX_Nw), sizeof(float));	
		x->Hwin = (float *) calloc( (MAX_Nw), sizeof(float));
		x->output = (float *) calloc( (MAX_Nw) , sizeof(float));
		x->buffer = (float *) calloc( (MAX_N), sizeof(float));
		x->channel = (float *) calloc( ((MAX_N+2)) , sizeof(float));
		x->bitshuffle = (int *) calloc( ((MAX_N * 2)) , sizeof(int));
		x->trigland = (float *) calloc( ((MAX_N * 2)) , sizeof(float));
		x->shuffle_in = (int *) calloc( (MAX_N2), sizeof(int) ) ;
		x->shuffle_out = (int *) calloc( (MAX_N2), sizeof(int) ) ;
		x->list_data = (t_atom *) calloc( ((MAX_N+2)) , sizeof(t_atom) ) ;
		x->mute = 0;
		x->bypass = 0;
		x->shuffle_count = 0;
		
	} else {
		memset((char *)x->input,0,x->Nw * sizeof(float));
		memset((char *)x->output,0,x->Nw * sizeof(float));
		memset((char *)x->buffer,0,x->N * sizeof(float));
		memset((char *)x->channel,0,(x->N+2) * sizeof(float));
	}
	
	
	init_rdft( x->N, x->bitshuffle, x->trigland);
	makewindows( x->Hwin, x->Wanal, x->Wsyn, x->Nw, x->N, x->D);
	
	
	if(initialized != 2){
		if( x->top_frequency < x->c_fundamental || x->top_frequency > 20000) {
			x->top_frequency = 1000.0 ;
		}
		x->max_bin = 1;  
		curfreq = 0;
		while( curfreq < x->top_frequency ) {
			++(x->max_bin);
			curfreq += x->c_fundamental ;
		}
		
		reset_shuffle(x); // set shuffle lookup
		x->shuffle_count = 0;
	}
	x->lock = 0;
}


void disarray_mute(t_disarray *x, t_floatarg toggle)
{
  x->mute = (short)toggle;
//  post("muted: %d", x->mute);	
}

void disarray_bypass(t_disarray *x, t_floatarg toggle)
{
  x->bypass = (short)toggle;	
}


t_int *disarray_perform(t_int *w)
{
  t_disarray *x = (t_disarray *) (w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  int n = w[4];
  
  int R = x->R;
  int Nw = x->Nw;
  int N = x->N ;
  int N2 = x-> N2;
  int Nw2 = x->Nw2;
  float *Wanal = x->Wanal;
  float *Wsyn = x->Wsyn;
  float *Hwin = x->Hwin;
  	
  float *input = x->input; 
  float *output = x->output;
  float *buffer = x->buffer;
  float *channel = x->channel;
  int		i,j;
  int inCount = x->inCount;
				
  int	D = x->D;
  float tmp;

  int shuffle_count = x->shuffle_count;

  float mult = x->mult ;
  int *bitshuffle = x->bitshuffle;
  float *trigland = x->trigland ;
  int *shuffle_in = x->shuffle_in;
  int *shuffle_out = x->shuffle_out;
  
  if( x->mute || x->lock ){
    while( n-- ){
      *out++ = 0.0;
    }
    return (w+5); 
  }
  
  if( x->bypass ){
    while( n-- ){
      *out++ = *in++ * 0.5; // gain compensation
    }
    return (w+5);
  }

  inCount += D;

  for ( j = 0 ; j < Nw - D ; j++ ){
    input[j] = input[j+D];
  }
  for ( j = Nw - D; j < Nw; j++ ) {
    input[j] = *in++;
  }

  fold(input, Wanal, Nw, buffer, N, inCount);	
  rdft(N, 1, buffer, bitshuffle, trigland);
    
  leanconvert(buffer, channel, N2);

	for( i = 0; i < shuffle_count ; i++){
		tmp = channel[ shuffle_in[ i ] * 2 ];
		channel[ shuffle_in[ i ] * 2]  = channel[ shuffle_out[ i ] * 2];
		channel[ shuffle_out[ i ] * 2]  = tmp;
	}
	
  leanunconvert( channel, buffer,  N2 );

  rdft( N, -1, buffer, bitshuffle, trigland );
  overlapadd( buffer, N, Wsyn, output, Nw, inCount);

  for ( j = 0; j < D; j++ )
    *out++ = output[j] * mult;

  for ( j = 0; j < Nw - D; j++ )
    output[j] = output[j+D];
			
  for ( j = Nw - D; j < Nw; j++ )
    output[j] = 0.;

  /* restore state variables */
  x->inCount = inCount % Nw;

  return (w+5);
}		

void iswitch_count(t_disarray *x, t_int i)
{
	switch_count(x,(t_floatarg)i);
}

void switch_count (t_disarray *x, t_floatarg i)
{
	if( i < 0 ){
		 i = 0;
	}
	if( i > x->N2 ) {
		i = x->N2;
	}
	x->shuffle_count = i;
}

void reset_shuffle (t_disarray *x)
{
int i;
int temp, p1, p2;
int max;

//post("max bin %d",x->max_bin);
max = x->max_bin;
	for( i = 0; i < x->N2; i++ ) {
		x->shuffle_out[i] = x->shuffle_in[i] = i ;
	}
	
	for( i = 0; i < 10000; i++ ) {
		p1 = x->shuffle_out[ rand_index( max ) ];
		p2 = x->shuffle_out[ rand_index( max ) ];
		temp = x->shuffle_out[ p1 ];
		x->shuffle_out[ p1 ] = x->shuffle_out[ p2 ];
		x->shuffle_out[ p2 ] = temp;
	}
	
}

int rand_index( int max) {
	return ( rand() % max );
}

void disarray_dsp(t_disarray *x, t_signal **sp, short *count)
{

  if(x->D != sp[0]->s_n ||x->D != sp[0]->s_n ) {
    x->R = sp[0]->s_sr;
    x->D = sp[0]->s_n;
    disarray_init(x,1);
  }
		dsp_add(disarray_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec,  sp[0]->s_n);
}

// ENTER STORED SHUFFLE
void disarray_list (t_disarray *x, t_symbol *msg, short argc, t_atom *argv) {
	short i;
	int ival;
	x->shuffle_count = argc;
	for (i=0; i < argc; i++) {

    ival = (int)atom_getfloatarg(i,argc,argv);


		if ( ival < x->N2 ) {
			x->shuffle_out[ i ] = ival;
		} else {
			post ("%d out of range",ival);
		}
	}
	return;
}

void disarray_setstate (t_disarray *x, t_symbol *msg, short argc, t_atom *argv) {
  short i;
  int ival;
  
  x->shuffle_count = argc;
  for (i=0; i < argc; i++) {
	  ival = atom_getfloatarg(i,argc,argv);

    if ( ival < x->N2 && ival >= 0) {
      x->shuffle_out[ i ] = ival;
    } else {
    	error("%s: %d is out of range",OBJECT_NAME, ival);
    }
  }
  return;
}

// REPORT CURRENT SHUFFLE STATUS
void disarray_showstate (t_disarray *x ) {

  t_atom *list_data = x->list_data;

  short i;
#if MSP
  for( i = 0; i < x->shuffle_count; i++ ) {
    SETLONG(list_data+i,x->shuffle_out[i]);
  }
#endif

#if PD
  for( i = 0; i < x->shuffle_count; i++ ) {
    SETFLOAT(list_data+i,(float)x->shuffle_out[i]);
  }
#endif	
  outlet_list(x->list_outlet,0,x->shuffle_count,list_data);

  return;
}
