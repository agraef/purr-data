/*
 * input I is a folded spectrum of length N; output O and
 * synthesis window W are of length Nw--overlap-add windowed,
 * unrotated, unfolded input data into output O
 */

#include "fftease.h"

void fftease_overlapadd(t_fftease *fft)

{
	t_float *buffer = fft->buffer;
	int N = fft->N;
	t_float *Wsyn = fft->Wsyn;
	t_float *output = fft->output;
	int Nw = fft->Nw;
	int out_count = fft->out_count;
	
	int i ;
    while ( out_count < 0 )
		out_count += N ;
    out_count %= N ;
    for ( i = 0 ; i < Nw ; i++ ) {
		output[i] += buffer[out_count] * Wsyn[i];
		if ( ++out_count == N )
			out_count = 0 ;
    }
	fft->out_count = (fft->out_count + fft->D) % fft->Nw;
}
