#include "fftease.h"

void old_overlapadd( float *I, int N, float *W, float *O, int Nw, int n )

{
 int i ;
    while ( n < 0 )
	n += N ;
    n %= N ;
    for ( i = 0 ; i < Nw ; i++ ) {
	O[i] += I[n]*W[i] ;
	if ( ++n == N )
	    n = 0 ;
    }
}


void old_convert(float *S, float *C, int N2, float *lastphase, float fundamental, float factor )
{
  float 	phase,
		phasediff;
  int 		real,
		imag,
		amp,
		freq;
  float 	a,
		b;
  int 		i;


    for ( i = 0; i <= N2; i++ ) {
      imag = freq = ( real = amp = i<<1 ) + 1;
      a = ( i == N2 ? S[1] : S[real] );
      b = ( i == 0 || i == N2 ? 0. : S[imag] );

      C[amp] = hypot( a, b );
      if ( C[amp] == 0. )
	phasediff = 0.;
      else {
	phasediff = ( phase = -atan2( b, a ) ) - lastphase[i];
	lastphase[i] = phase;
	
	while ( phasediff > PI )
	  phasediff -= TWOPI;
	while ( phasediff < -PI )
	  phasediff += TWOPI;
      }
      C[freq] = phasediff*factor + i*fundamental;
    }
}



void old_unconvert( float *C, float *S, int N2, float *lastphase, float fundamental, float factor )

{
  int 		i,
		real,
		imag,
		amp,
		freq;
  float 	mag,
		phase;
double sin(), cos();

    for ( i = 0; i <= N2; i++ ) {

	imag = freq = ( real = amp = i<<1 ) + 1;

	if ( i == N2 )
	  real = 1;

	mag = C[amp];
	lastphase[i] += C[freq] - i*fundamental;
	phase = lastphase[i]*factor;
	S[real] = mag*cos( phase );

	if ( i != N2 )
	  S[imag] = -mag*sin( phase );

    }

}
