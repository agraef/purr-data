#include "fftease.h"

void fftease_leanconvert(t_fftease *fft)

{

	int		real, imag, amp, phase;
	t_float		a, b;
	int		i;
	t_float *buffer = fft->buffer;
	t_float *channel = fft->channel;
	int N2 = fft->N2;

	for ( i = 0; i <= N2; i++ ) {
		imag = phase = ( real = amp = i<<1 ) + 1;
		a = ( i == N2 ? buffer[1] : buffer[real] );
		b = ( i == 0 || i == N2 ? 0. : buffer[imag] );
		channel[amp] = hypot( a, b );
		channel[phase] = -atan2( b, a );
	}
}

