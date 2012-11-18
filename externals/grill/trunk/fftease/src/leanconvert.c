#include "pv.h"

void leanconvert( float *S, float *C, int N2 , int amp, int ph)
{
#if 1
	register int i;

	float a = fabs(S[0]);  /* real value at f=0 */
	float b = fabs(S[1]);  /* real value at f=Nyquist */

	C[0] = a;
	C[1] = 0;
	S += 2,C += 2;

	if(amp && ph) {
		for ( i = 1; i < N2; i++,S += 2,C += 2 ) {
			C[0] = hypot( S[0], S[1] );
			C[1] = -atan2( S[1], S[0] );
		}
	}
	else if(amp) {
		for ( i = 1; i < N2; i++,S += 2,C += 2 )
			C[0] = hypot( S[0], S[1] );
	}
	else if(ph) {
		for ( i = 1; i < N2; i++,S += 2,C += 2 )
			C[1] = -atan2( S[1], S[0] );
	}

	C[0] = b;
	C[1] = 0;
#else

 int		real, imag,
		amp, phase;
 float		a, b;
  int		i;
	
 for ( i = 0; i <= N2; i++ ) {
   imag = phase = ( real = amp = i<<1 ) + 1;
   a = ( i == N2 ? S[1] : S[real] );
   b = ( i == 0 || i == N2 ? 0. : S[imag] );
   C[amp] = hypot( a, b );
   C[phase] = -atan2( b, a );
 }
#endif
}


/* unconvert essentially undoes what convert does, i.e., it
  turns N2+1 PAIRS of amplitude and frequency values in
  C into N2 PAIR of complex spectrum data (in rfft format)
  in output array S; sampling rate R and interpolation factor
  I are used to recompute phase values from frequencies */

void leanunconvert( float *C, float *S, int N2 )
{
#if 1
	register int i;

	S[0] = fabs(C[0]);
	S[1] = fabs(C[N2*2]);
	S += 2,C += 2;

	for (i = 1; i < N2; i++,S += 2,C += 2 ) {
		S[0] = C[0] * cos( C[1] );
		S[1] = -C[0] * sin( C[1] );
	}
#else
 int		real, imag,
		amp, phase;
  float		a, b;
  register int		i;
  
  for ( i = 0; i <= N2; i++ ) {
    imag = phase = ( real = amp = i<<1 ) + 1;
    S[real] = *(C+amp) * cos( *(C+phase) );
    if ( i != N2 )
      S[imag] = -*(C+amp) * sin( *(C+phase) );
  }

#endif
}

