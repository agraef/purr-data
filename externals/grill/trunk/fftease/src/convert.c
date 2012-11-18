#include "pv.h"

/* S is a spectrum in rfft format, i.e., it contains N real values
   arranged as real followed by imaginary values, except for first
   two values, which are real parts of 0 and Nyquist frequencies;
   convert first changes these into N/2+1 PAIRS of magnitude and
   phase values to be stored in output array C; the phases are then
   unwrapped and successive phase differences are used to compute
   estimates of the instantaneous frequencies for each phase vocoder
   analysis channel; decimation rate D and sampling rate R are used
   to render these frequency values directly in Hz. */

void convert(float *S, float *C, int N2, float *lastphase,  float fundamental, float factor )
{
#if 1
	float phase,phasediff;
	int even,odd;
	float a,b;
	int i;

	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;
		a = ( i == N2 ? S[1] : S[even] );
		b = ( i == 0 || i == N2 ? 0. : S[odd] );

		C[even] = hypot( a, b );
		if ( C[even] == 0. )
			phasediff = 0.;
		else {
			phase = -atan2( b, a );
			phasediff = fmod(phase - lastphase[i] + (PV_2PI+PV_PI), PV_2PI)-PV_PI;
			lastphase[i] = phase;
		}

		C[odd] = phasediff*factor + i*fundamental;
	}
#else
  float 	phase,
		phasediff;
  int 		real,
		imag,
		amp,
		freq;
  float 	a,
		b;
  int 		i;

  float myTWOPI, myPI;

  myTWOPI = 8.*atan(1.);
  myPI = 4.*atan(1.);


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
	
	// TG: DANGEROUS!!!! (and slow, if lastphase not correctly initialized)
	while ( phasediff > myPI )
	  phasediff -= myTWOPI;
	while ( phasediff < -myPI )
	  phasediff += myTWOPI;
      }
      C[freq] = phasediff*factor + i*fundamental;
      /*
      if( i > 8 && i < 12 ) {
	fprintf(stderr,"convert freq %d: %f\n",i, C[freq]);
      }
      */
    }
#endif
}


void unconvert(float  *C, float *S, int N2, float *lastphase, float fundamental,  float factor )
{
#if 1
	int i,even,odd;
	float mag,phase;

	for ( i = 0; i <= N2; i++ ) {
		odd = ( even = i<<1 ) + 1;

		mag = C[even];
		lastphase[i] += C[odd] - i*fundamental;
		phase = lastphase[i]*factor;

		if(i != N2) {
			S[even] = mag*cos( phase );
			S[odd] = -mag*sin( phase );
		}
		else
			S[1] = mag*cos( phase );
	}
#else
  int 		i,
		real,
		imag,
		amp,
		freq;
  float 	mag,
		phase;

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
	/*
      if( i == 10 ) {
	fprintf(stderr,"unconvert: amp: %f freq: %f funda %f fac %f\n", C[amp],C[freq],fundamental,factor);
      }
	*/
    }

#endif
}
