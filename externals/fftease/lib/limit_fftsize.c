#include "fftease.h"

//extern void post(const char *fmt, ...);

void limit_fftsize(int *N, int *Nw, char *OBJECT_NAME)
{
	if(*N > MAX_N){
//		post("%s: N set to maximum FFT size of %d",OBJECT_NAME,MAX_N); 
		printf("%s: N set to maximum FFT size of %d",OBJECT_NAME,MAX_N); 
		*N = MAX_N;
	}
	if(*Nw > MAX_Nw){
//		post("%s: Nw set to maximum window size of %d",OBJECT_NAME,MAX_Nw); 
		printf("%s: Nw set to maximum window size of %d",OBJECT_NAME,MAX_Nw); 
		*Nw = MAX_Nw;
	}
}
