#include "fftease.h"

extern void post(const char *fmt, ...);

void fftease_limit_fftsize(int *N, int *Nw, char *OBJECT_NAME)
{
	if(*N > MAX_N){
		// post("%s: N set to maximum FFT size of %d",OBJECT_NAME,MAX_N);
		*N = MAX_N;
	}
	if(*Nw > MAX_Nw){
	// 	post("%s: Nw set to maximum window size of %d",OBJECT_NAME,MAX_Nw);
		*Nw = MAX_Nw;
	}
}

int fftease_FFT_size( int testfft )
{
int test = 2;
	if( testfft <= 0 )
		return DEFAULT_FFTEASE_FFTSIZE;
	while( test < testfft && test < FFTEASE_MAX_FFTSIZE){
		test *= 2;
	}
	if( test != testfft ){
		post("incorrect FFT size specified, using %d", test);
	}
	if( test == FFTEASE_MAX_FFTSIZE){
		post("fftsize capped at maximum: %d", test);
	}
	return test;
}