/* Pd 32-bit FFTease 3.0 */

#include "fftease.h"

static t_class *cross_class;

#define OBJECT_NAME "cross~"

typedef struct _cross
{
	t_object x_obj;
    float x_f;
	t_fftease *fft;
	t_fftease *fft2; 
	t_float threshie;
	short mute;//flag
	short autonorm;// for self gain regulation
    t_float normult; // adjusted multiplier on a per-frame basis
} t_cross;

void *cross_new(t_symbol *s, int argc, t_atom *argv);
t_int *cross_perform(t_int *w);
void cross_dsp(t_cross *x, t_signal **sp);
void *cross_new(t_symbol *s, int argc, t_atom *argv);
void cross_init(t_cross *x);
void cross_fftinfo(t_cross *x);
void cross_mute(t_cross *x, t_floatarg toggle);
void cross_autonorm(t_cross *x, t_floatarg toggle);
void cross_free(t_cross *x);

void cross_tilde_setup(void)
{
    t_class *c;
    c = class_new(gensym("cross~"), (t_newmethod)cross_new,
                  (t_method)cross_free,sizeof(t_cross), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(c, t_cross, x_f);
	class_addmethod(c,(t_method)cross_dsp,gensym("dsp"),0);
	class_addmethod(c,(t_method)cross_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(c,(t_method)cross_autonorm, gensym("autonorm"),  A_FLOAT, 0);
    cross_class = c;
    fftease_announce(OBJECT_NAME);
}

void cross_autonorm(t_cross *x, t_floatarg toggle)
{
	x->autonorm = (short) toggle;
}

void cross_fftsize(t_cross *x, t_floatarg f)
{	
	x->fft->N = (int) f;
	x->fft2->N = (int) f;
	cross_init(x);
}

void cross_mute(t_cross *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void cross_free(t_cross *x)
{
	fftease_free(x->fft);
	fftease_free(x->fft2);
    free(x->fft);
    free(x->fft2);
}

void *cross_new(t_symbol *s, int argc, t_atom *argv)
{
	t_fftease *fft, *fft2;
	t_cross *x = (t_cross *)pd_new(cross_class);


    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
	outlet_new(&x->x_obj, gensym("signal"));
	x->fft = (t_fftease *) calloc(1,sizeof(t_fftease));
	x->fft2 = (t_fftease *) calloc(1,sizeof(t_fftease));
	fft = x->fft;
	fft2 = x->fft2;	
	fft->initialized = 0;
	fft2->initialized = 0;
	
	fft->N = FFTEASE_DEFAULT_FFTSIZE;
	fft->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft->winfac = FFTEASE_DEFAULT_WINFAC;
	fft2->N = FFTEASE_DEFAULT_FFTSIZE;
	fft2->overlap = FFTEASE_DEFAULT_OVERLAP;
	fft2->winfac = FFTEASE_DEFAULT_WINFAC;
    if(argc > 0){ fft->N = fft2->N = (int) atom_getfloatarg(0, argc, argv); }
    if(argc > 1){ fft->overlap = fft2->overlap = (int) atom_getfloatarg(1, argc, argv); }
	return x;
}

void cross_init(t_cross *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	short initialized;

	initialized = fft->initialized;
	
	fftease_init(fft);
	fftease_init(fft2);

	if(!initialized){
		x->threshie = .001 ;
		x->autonorm = 0;
        x->mute = 0;
	}
}

void do_cross(t_cross *x)
{
	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int i;
	int N2 = fft->N2;
	t_float a1, b1, a2, b2;
	t_float *buffer1 = fft->buffer;
	t_float *buffer2 = fft2->buffer;
	t_float *channel1 = fft->channel;
	short autonorm = x->autonorm;
	int N = fft->N;
	t_float mult = fft->mult;
	int even, odd;
	t_float gainer;
	t_float threshie = x->threshie;
	t_float ingain = 0;
	t_float outgain, rescale;
	t_float mymult;

	fftease_fold(fft);
	fftease_fold(fft2);
	fftease_rdft(fft,1);
	fftease_rdft(fft2,1);

	/* changing algorithm for window flexibility */
	if(autonorm){
		ingain = 0;
		for(i = 0; i < N; i+=2){
			ingain += hypot(buffer1[i], buffer1[i+1]);
		}
	}
	
	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		
		a1 = ( i == N2 ? *(buffer1+1) : *(buffer1+even) );
		b1 = ( i == 0 || i == N2 ? 0. : *(buffer1+odd) );
		a2 = ( i == N2 ? *(buffer2+1) : *(buffer2+even) );
		b2 = ( i == 0 || i == N2 ? 0. : *(buffer2+odd) );
		gainer = hypot(a2, b2);
		if( gainer > threshie ) 
			*(channel1+even) = hypot( a1, b1 ) * gainer;
		*(channel1+odd) = -atan2( b1, a1 );
		*(buffer1+even) = *(channel1+even) * cos( *(channel1+odd) );
		if ( i != N2 )
			*(buffer1+odd) = -(*(channel1+even)) * sin( *(channel1+odd) );
		
	}
	if(autonorm){
		outgain = 0;
		for(i = 0; i < N; i+=2){
			outgain += hypot(buffer1[i], buffer1[i+1]);
		}
		if(ingain <= .0000001){
			// post("gain emergency!");
			rescale = 1.0;
		} else {
			rescale = ingain / outgain;
		} 
		//post("ingain %f outgain %f rescale %f",ingain, outgain, rescale);
		x->normult = mult * rescale;
	}  else {
		x->normult = mult;
        //post("mymult: %f", mymult);
	}
	fftease_rdft(fft, -1);
	fftease_overlapadd(fft);
}

t_int *cross_perform(t_int *w)
{
	int i, j;
    t_cross *x = (t_cross *) (w[1]);
	t_float *MSPInputVector1 = (t_float *)(w[2]);
	t_float *MSPInputVector2 = (t_float *)(w[3]);
	t_float *threshold = (t_float *)(w[4]);
	t_float *MSPOutputVector = (t_float *)(w[5]);

	t_fftease *fft = x->fft;
	t_fftease *fft2 = x->fft2;
	int MSPVectorSize = fft->MSPVectorSize;
	int operationRepeat = fft->operationRepeat;
	int operationCount = fft->operationCount;
	t_float *internalInputVector1 = fft->internalInputVector;
	t_float *internalInputVector2 = fft2->internalInputVector;
	t_float *internalOutputVector = fft->internalOutputVector;
	t_float *inputOne = fft->input;
	t_float *inputTwo = fft2->input;
	t_float *output = fft->output;
	int D = fft->D;
	int Nw = fft->Nw;
	t_float mult = fft->mult;	
	
	if(x->mute){
		for(i=0; i < MSPVectorSize; i++){ MSPOutputVector[i] = 0.0; }
        return w+6;
	}

    x->threshie = *threshold;

	if( fft->bufferStatus == EQUAL_TO_MSP_VECTOR ){
        memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
        memcpy(inputOne + (Nw - D), MSPInputVector1, D * sizeof(t_float));
        memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
        memcpy(inputTwo + (Nw - D), MSPInputVector2, D * sizeof(t_float));
        
		do_cross(x);
        mult = x->normult;
		for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
        memcpy(output, output + D, (Nw-D) * sizeof(t_float));
        for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
	}
	else if( fft->bufferStatus == SMALLER_THAN_MSP_VECTOR ) {
		for( i = 0; i < operationRepeat; i++ ){
            memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
            memcpy(inputOne + (Nw-D), MSPInputVector1 + (D*i), D * sizeof(t_float));
            memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
            memcpy(inputTwo + (Nw-D), MSPInputVector2 + (D*i), D * sizeof(t_float));
            
			do_cross(x);
            mult = x->normult;
			for ( j = 0; j < D; j++ ){ *MSPOutputVector++ = output[j] * mult; }
            memcpy(output, output + D, (Nw-D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
	}
	else if( fft->bufferStatus == BIGGER_THAN_MSP_VECTOR ) {
        memcpy(internalInputVector1 + (operationCount * MSPVectorSize), MSPInputVector1, MSPVectorSize * sizeof(t_float));
        memcpy(internalInputVector2 + (operationCount * MSPVectorSize), MSPInputVector2, MSPVectorSize * sizeof(t_float));
        memcpy(MSPOutputVector, internalOutputVector + (operationCount * MSPVectorSize), MSPVectorSize * sizeof(t_float));
        
		operationCount = (operationCount + 1) % operationRepeat;
        
        if( operationCount == 0 ) {
            memcpy(inputOne, inputOne + D, (Nw - D) * sizeof(t_float));
            memcpy(inputOne + (Nw - D), internalInputVector1, D * sizeof(t_float));
            memcpy(inputTwo, inputTwo + D, (Nw - D) * sizeof(t_float));
            memcpy(inputTwo + (Nw - D), internalInputVector2, D * sizeof(t_float));
            
            do_cross(x);
            mult = x->normult;
            for ( j = 0; j < D; j++ ){ internalOutputVector[j] = output[j] * mult; }
            memcpy(output, output + D, (Nw - D) * sizeof(t_float));
            for(j = (Nw-D); j < Nw; j++){ output[j] = 0.0; }
		}
		fft->operationCount = operationCount;
	}
    return w+6;
}

void cross_dsp(t_cross *x, t_signal **sp)
{
    int reset_required = 0;
    int maxvectorsize = sys_getblksize();
    int samplerate = sys_getsr();
    
    t_fftease *fft = x->fft;
    t_fftease *fft2 = x->fft2;
    
    if(fft->R != samplerate || fft->MSPVectorSize != maxvectorsize || fft->initialized == 0){
        reset_required = 1;
    }
	if(!samplerate)
        return;
    
	if(fft->MSPVectorSize != maxvectorsize){
		fft->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft);
		fft2->MSPVectorSize = maxvectorsize;
		fftease_set_fft_buffers(fft2);
	}
	if(fft->R != samplerate ){
		fft->R = samplerate;
        fft2->R = samplerate;
	}
    if(reset_required){
        cross_init(x);
    }
    if(fftease_msp_sanity_check(fft,OBJECT_NAME)) {
        dsp_add(cross_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}

