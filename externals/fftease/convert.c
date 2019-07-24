#include "fftease.h"


/* S is a spectrum in rfft format, i.e., it contains N real values
 arranged as real followed by imaginary values, except for first
 two values, which are real parts of 0 and Nyquist frequencies;
 convert first changes these into N/2+1 PAIRS of magnitude and
 phase values to be stored in output array C; the phases are then
 unwrapped and successive phase differences are used to compute
 estimates of the instantaneous frequencies for each phase vocoder
 analysis channel; decimation rate D and sampling rate R are used
 to render these frequency values directly in Hz. */


void fftease_convert(t_fftease *fft)
{
	t_float *buffer = fft->buffer;
	t_float *channel = fft->channel;
	int N2 = fft->N2;
	t_float *lastphase = fft->c_lastphase_in;
	t_float fundamental = fft->c_fundamental;
	t_float factor = fft->c_factor_in;
	
	t_float 	phase, phasediff;
	int real,imag,amp,freq;
	t_float a,b;
	int i;
	
	
    for ( i = 0; i <= N2; i++ ) {
		imag = freq = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? buffer[1] : buffer[real] );
		b = ( i == 0 || i == N2 ? 0. : buffer[imag] );
		
		channel[amp] = hypot( a, b );
		if ( channel[amp] == 0. )
			phasediff = 0.;
		else {
			phasediff = ( phase = -atan2( b, a ) ) - lastphase[i];
			lastphase[i] = phase;
			
			while ( phasediff > PI )
				phasediff -= TWOPI;
			while ( phasediff < -PI )
				phasediff += TWOPI;
		}
		channel[freq] = phasediff*factor + i*fundamental;
    }
}
