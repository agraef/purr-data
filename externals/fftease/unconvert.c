#include "fftease.h"



//void fftease_unconvert( t_float *C, t_float *S, int N2, t_float *lastphase, t_float fundamental, t_float factor )
//	fftease_unconvert( channel, buffer, N2, c_lastphase_out, c_fundamental, c_factor_out  );

void fftease_unconvert(t_fftease *fft)

{
	t_float *channel = fft->channel;
	t_float *buffer = fft->buffer;
	int N2 = fft->N2;
	t_float *lastphase = fft->c_lastphase_out;
	t_float fundamental = fft->c_fundamental;
	t_float factor = fft->c_factor_out;
	
	int 	i, real, imag, amp, freq;
	t_float 	mag, phase;

	
    for ( i = 0; i <= N2; i++ ) {
		
		imag = freq = ( real = amp = i<<1 ) + 1;
		
		if ( i == N2 )
			real = 1;
		
		mag = channel[amp];
		lastphase[i] += channel[freq] - i*fundamental;
		phase = lastphase[i]*factor;
		buffer[real] = mag*cos( phase );
		
		if ( i != N2 )
			buffer[imag] = -mag*sin( phase );
		
    }
	
}
