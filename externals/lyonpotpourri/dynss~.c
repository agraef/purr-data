#include "MSPd.h"

static t_class *dynss_class;

#define MAXPOINTS (64)
#define OBJECT_NAME "dynss~"
#define COMPILE_DATE "9.02.07"
#define OBJECT_VERSION "2.01"

//pd version


typedef struct _dynss
{

	t_object x_obj;
    t_float x_f;
    
	long point_count; // how many points in waveform (including beginning and end point)
	t_double freq; // frequency
	long counter; // count samples
	long period_samples; // how many samples in a period
	float srate; // sampling rate
	long current_point; // which point are we on
	t_double *values;
	long *point_breaks;
	t_double *norm_breaks;
	t_double *xdevs;
	t_double *ydevs;
	t_double devscale_x;
	t_double devscale_y;
} t_dynss;

void dynss_tilde_setup(void);
void *dynss_new(void);
t_int *dynss_perform(t_int *w);
void dynss_dsp(t_dynss *x, t_signal **sp);
void version(void);
float boundrand(float min, float max);
void dynss_init(t_dynss *x,short initialized);
void dynss_devx(t_dynss *x, t_floatarg f);
void dynss_devy(t_dynss *x, t_floatarg f);
void dynss_freq(t_dynss *x, t_floatarg f);
void dynss_printwave(t_dynss *x);
void dynss_new_wave(t_dynss *x);
void dynss_pointcount(t_dynss *x, t_floatarg f);
void dynss_new_amps(t_dynss *x);

void dynss_tilde_setup(void)
{
	t_class *c;
	c = class_new(gensym("dynss~"), (t_newmethod)dynss_new, 0, sizeof(t_dynss), 0,0);
    CLASS_MAINSIGNALIN(c,t_dynss,x_f);
	class_addmethod(c,(t_method)dynss_dsp, gensym("dsp"), A_CANT, 0);
    
	class_addmethod(c,(t_method)version,gensym("version"),0);
	class_addmethod(c,(t_method)dynss_devx,gensym("devx"),A_FLOAT,0);
	class_addmethod(c,(t_method)dynss_devy,gensym("devy"),A_FLOAT,0);
	class_addmethod(c,(t_method)dynss_freq,gensym("freq"),A_FLOAT,0);
	class_addmethod(c,(t_method)dynss_printwave,gensym("printwave"),0);
	class_addmethod(c,(t_method)dynss_new_wave,gensym("new_wave"),0);
	class_addmethod(c,(t_method)dynss_new_amps,gensym("new_amps"),0);
	class_addmethod(c,(t_method)dynss_pointcount,gensym("pointcount"),A_FLOAT,0);
	potpourri_announce(OBJECT_NAME);
	dynss_class = c;
}


void dynss_pointcount(t_dynss *x, t_floatarg f)
{
	if(f >= 2 && f <= MAXPOINTS ){
		x->point_count = (long) f;
		dynss_init(x,0);
	}
}

void dynss_freq(t_dynss *x, t_floatarg f)
{
	x->freq = fabs(f);
}

void dynss_devx(t_dynss *x, t_floatarg f)
{
	x->devscale_x = f;
}

void dynss_devy(t_dynss *x, t_floatarg f)
{
	x->devscale_y = f;
}

void version(void)
{
	post("%s version %s, compiled %s",OBJECT_NAME, OBJECT_VERSION, COMPILE_DATE);
}

void dynss_printwave(t_dynss *x)
{
	int i;
	for(i = 0; i < x->point_count; i++){
		post("point %d break %d norm break %f value %f",i, x->point_breaks[i], x->norm_breaks[i], x->values[i]);
	}
}

void dynss_new_wave(t_dynss *x)
{
	dynss_init(x,0);
}

void dynss_new_amps(t_dynss *x)
{
	int i;
	for(i = 1; i < x->point_count - 1; i++){
		x->values[i] = boundrand(-0.95, 0.95);
	}
}


void *dynss_new(void)
{
	t_dynss *x = (t_dynss *)pd_new(dynss_class);
    outlet_new(&x->x_obj, gensym("signal"));
	x->srate = sys_getsr();
	x->point_count = 6; // including fixed start and endpoint, so just 4 dynamic points
	
	x->values = (t_double *) calloc(MAXPOINTS + 2, sizeof(t_double));
	x->point_breaks = (long *) calloc(MAXPOINTS + 2, sizeof(long));
	x->norm_breaks = (t_double *) calloc(MAXPOINTS + 2, sizeof(t_double));
	x->xdevs = (t_double *) calloc(MAXPOINTS + 2, sizeof(t_double));
	x->ydevs = (t_double *) calloc(MAXPOINTS + 2, sizeof(t_double));
	
	if(! x->srate ){
		x->srate = 44100;
		post("sr autoset to 44100");
	}
	x->freq = 100.0;
	
	srand(time(0));

	dynss_init(x,0); 
	
	return (x);
}


float boundrand(float min, float max)
{
	return min + (max-min) * ((float) (rand() % RAND_MAX)/ (float) RAND_MAX);
}

void dynss_init(t_dynss *x,short initialized)
{
	int i,j;
	float findex;
	long point_count = x->point_count;
	t_double *values = x->values;
	t_double *norm_breaks = x->norm_breaks;
	long *point_breaks = x->point_breaks;
	t_double *ydevs = x->ydevs;
	t_double *xdevs =x->xdevs;

	
    if(!initialized){
		x->period_samples = (long)(x->srate / x->freq);
		x->counter = 0;
		x->current_point = 0;
		x->devscale_x = 0.0; // no evolution by default
		x->devscale_y = 0.0;
		// post("there are %d samples per period, srate %f freq %f", x->period_samples, x->srate, x->freq);
		norm_breaks[0] = 0.0;
		norm_breaks[point_count - 1] = 1.0;
		values[0] = values[point_count - 1] = 0.0;
		for(i = 1; i < point_count - 1; i++){
			values[i] = boundrand(-1.0, 1.0);
			norm_breaks[i] = boundrand(0.05,0.95);
		}
		// now sort into order (insertion sort)

		for(i = 1; i < point_count; i++){
			findex = norm_breaks[i];
			j = i;
			while( j > 0 && norm_breaks[j-1] > findex){
				norm_breaks[j] = norm_breaks[j-1];
				j = j - 1;
			}
			norm_breaks[j] = findex;
				
		}
		// now generate sample break points;
		for(i = 0; i < point_count; i++){
			point_breaks[i] = (long) ( (float)x->period_samples * norm_breaks[i] );
			// post("%i %f %f",point_breaks[i], norm_breaks[i], values[i]);
		}
		// set y deviation maxes
		for(i = 0; i < point_count; i++){
			ydevs[i] = boundrand(0.0,0.99);
			xdevs[i] = boundrand(0.0,0.99);
			// post("rands: %f %f",ydevs[i],xdevs[i]);
		}
    } else {
		
    }
}


t_int *dynss_perform(t_int *w)
{
	//int i, j, k;
	//	float outval;
	int i,j;
	float findex1,findex2;
	t_dynss *x = (t_dynss *) (w[1]);
//	t_float *inlet = (t_float *) (w[2]);
	t_float *outlet = (t_float *) (w[3]);
	int n = (int) w[4];
	t_double *values = x->values;
	t_double *norm_breaks = x->norm_breaks;
	long *point_breaks = x->point_breaks;
	t_double *ydevs = x->ydevs;
	t_double *xdevs = x->xdevs;
	long counter = x->counter;
	long period_samples = x->period_samples;
	long current_point = x->current_point;
	long point_count = x->point_count;
	float sample;
	float frak;
	float dev, newval;
	long segsamps;
	
	while(n--){
		if( counter == point_breaks[current_point + 1]){
			sample = values[current_point + 1];
			++current_point;
			if(current_point > point_count - 1){
				current_point = 0;
				counter = 0;
			}
		} 
	
		else {
			segsamps = point_breaks[current_point + 1] - point_breaks[current_point];
			if( segsamps <= 1){
				frak = 0.0;
			} 
			else {
				frak = (float)(counter - point_breaks[current_point]) / (float)segsamps;
				//post("frak %f counter %d point break %d diff %d",frak, counter, point_breaks[current_point],counter - point_breaks[current_point] );
				if( frak < 0.0 || frak > 1.0 ){
					post("bad fraction: %f",frak);
					post("current point: %d", current_point);
					post("segsamps %d counter %d current break %d next break %d", segsamps, counter, point_breaks[current_point], point_breaks[current_point + 1]);
				}
			}
			if(current_point < 0 || current_point > point_count - 1){
				post("ERROR: dss had bad current point!");
				sample = 0;
			} else {
				sample = values[current_point] + (values[current_point+1] - values[current_point]) * frak;
				
			}
		}
		++counter;
		if(counter >= period_samples){
			counter = 0;
			current_point = 0;
			if( x->freq > 0.0 ){
				period_samples = x->srate / x->freq;
			}
			// nudge waveform
			for(i = 1; i < point_count - 1; i++){
				dev = boundrand(-1.0,1.0) * ydevs[i] * x->devscale_y;
				newval = values[i] + dev;
				// clip
				newval = newval > 0.95 ? 0.95 : newval;
				newval = newval < -0.95 ? -0.95 : newval;
				values[i] = newval;
			}
			for(i = 1; i < point_count - 1; i++){
				dev = boundrand(-1.0,1.0) * xdevs[i] * x->devscale_x;
				newval = norm_breaks[i] + dev;
				// clip
				newval = newval < 0.05 ? 0.05 : newval;
				newval = newval > 0.95 ? 0.95: newval;
				norm_breaks[i] = newval;
				/* if(norm_breaks[i] < norm_breaks[i-1]){
					norm_breaks[i] = norm_breaks[i-1] + 0.03; // disallow point jumps for now
				} */
			}
			// now sort them
			
			for(i = 1; i < point_count; i++){
				findex1 = norm_breaks[i];
				findex2 = values[i];
				j = i;
				while( j > 0 && norm_breaks[j-1] > findex1){
					norm_breaks[j] = norm_breaks[j-1];
					values[j] = values[j-1];
					j = j - 1;
				}
				norm_breaks[j] = findex1;
				values[j] = findex2;
			}
			
			// now generate sample breaks
			for(i = 0; i < point_count; i++){
				point_breaks[i] = (long) ( (float)period_samples * norm_breaks[i] );
			}
		}
		*outlet++ = sample;
	}
	x->counter = counter;
	x->current_point = current_point;	
	x->period_samples = period_samples;
	return (w + 5);
}


void dynss_dsp(t_dynss *x, t_signal **sp)
{
	if(x->srate != sp[0]->s_sr) {
		x->srate = sp[0]->s_sr;
	}
	dsp_add(dynss_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec,(t_int)sp[0]->s_n);
}
