#include "pv.h"
#include "math.h"

/*
 * make balanced pair of analysis (A) and synthesis (S) windows;
 * window lengths are Nw, FFT length is N, synthesis interpolation
 * factor is I, and osc is true (1) if oscillator bank resynthesis 
 * is specified
 */
void makewindows( float *H, float *A, float *S, int Nw, int N, int I, int osc )
{
 int i ;
 float sum ;

/*
 * basic Hamming windows
 */
    for ( i = 0 ; i < Nw ; i++ )
	H[i] = A[i] = S[i] = 0.54 - 0.46*cos( PV_2PI*i/(Nw - 1) ) ;
/*
 * when Nw > N, also apply interpolating (sinc) windows to
 * ensure that window are 0 at increments of N (the FFT length)
 * away from the center of the analysis window and of I away
 * from the center of the synthesis window
 */
    if ( Nw > N ) {
     float x ;

/*
 * take care to create symmetrical windows
 */
	x = -(Nw - 1)/2. ;
	for ( i = 0 ; i < Nw ; i++, x += 1. )
	    if ( x != 0. ) {
		A[i] *= N*sin( PV_PI*x/N )/(PV_PI*x) ;
		if ( I )
		    S[i] *= I*sin( PV_PI*x/I )/(PV_PI*x) ;
	    }
    }
/*
 * normalize windows for unity gain across unmodified
 * analysis-synthesis procedure
 */
    for ( sum = i = 0 ; i < Nw ; i++ )
	sum += A[i] ;

    for ( i = 0 ; i < Nw ; i++ ) {
     float afac = 2./sum ;
     float sfac = Nw > N ? 1./afac : afac ;
	A[i] *= afac ;
	S[i] *= sfac ;
    }

    if ( Nw <= N && I ) {
	for ( sum = i = 0 ; i < Nw ; i += I )
	    sum += S[i]*S[i] ;
	for ( sum = 1./sum, i = 0 ; i < Nw ; i++ )
	    S[i] *= sum ;
    }
}

void makehamming( float *H, float *A, float *S, int Nw, int N, int I, int osc,int  odd )
{
 int i;
 float sum ;

/*
 * basic Hamming windows
 */
 
 
 if (odd) {
    for ( i = 0 ; i < Nw ; i++ )
	  H[i] = A[i] = S[i] = sqrt(0.54 - 0.46*cos( PV_2PI*i/(Nw - 1) ));
 }
	
 else {

   for ( i = 0 ; i < Nw ; i++ )
	  H[i] = A[i] = S[i] = 0.54 - 0.46*cos( PV_2PI*i/(Nw - 1) );

 }
 	
/*
 * when Nw > N, also apply interpolating (sinc) windows to
 * ensure that window are 0 at increments of N (the FFT length)
 * away from the center of the analysis window and of I away
 * from the center of the synthesis window
 */
    if ( Nw > N ) {
     float x ;

/*
 * take care to create symmetrical windows
 */
	x = -(Nw - 1)/2. ;
	for ( i = 0 ; i < Nw ; i++, x += 1. )
	    if ( x != 0. ) {
		A[i] *= N*sin( PV_PI*x/N )/(PV_PI*x) ;
		if ( I )
		    S[i] *= I*sin( PV_PI*x/I )/(PV_PI*x) ;
	    }
    }
/*
 * normalize windows for unity gain across unmodified
 * analysis-synthesis procedure
 */
    for ( sum = i = 0 ; i < Nw ; i++ )
	sum += A[i] ;

    for ( i = 0 ; i < Nw ; i++ ) {
     float afac = 2./sum ;
     float sfac = Nw > N ? 1./afac : afac ;
	A[i] *= afac ;
	S[i] *= sfac ;
    }

    if ( Nw <= N && I ) {
	for ( sum = i = 0 ; i < Nw ; i += I )
	    sum += S[i]*S[i] ;
	for ( sum = 1./sum, i = 0 ; i < Nw ; i++ )
	    S[i] *= sum ;
    }
}


void makehanning( float *H, float *A, float *S, int Nw, int N, int I, int osc, int odd )
{
 int i;
 float sum ;

/*
 * basic Hanning windows
 */
 
 
 if (odd) {
    for ( i = 0 ; i < Nw ; i++ )
	  H[i] = A[i] = S[i] = sqrt(0.5 * (1. + cos(PV_PI + PV_2PI * i / (Nw - 1))));
 }
	
 else {

   for ( i = 0 ; i < Nw ; i++ )
	  H[i] = A[i] = S[i] = 0.5 * (1. + cos(PV_PI + PV_2PI * i / (Nw - 1)));

 }
 	
/*
 * when Nw > N, also apply interpolating (sinc) windows to
 * ensure that window are 0 at increments of N (the FFT length)
 * away from the center of the analysis window and of I away
 * from the center of the synthesis window
 */
    if ( Nw > N ) {
     float x ;

/*
 * take care to create symmetrical windows
 */
	x = -(Nw - 1)/2. ;
	for ( i = 0 ; i < Nw ; i++, x += 1. )
	    if ( x != 0. ) {
		A[i] *= N*sin( PV_PI*x/N )/(PV_PI*x) ;
		if ( I )
		    S[i] *= I*sin( PV_PI*x/I )/(PV_PI*x) ;
	    }
    }
/*
 * normalize windows for unity gain across unmodified
 * analysis-synthesis procedure
 */
    for ( sum = i = 0 ; i < Nw ; i++ )
	sum += A[i] ;

    for ( i = 0 ; i < Nw ; i++ ) {
     float afac = 2./sum ;
     float sfac = Nw > N ? 1./afac : afac ;
	A[i] *= afac ;
	S[i] *= sfac ;
    }

    if ( Nw <= N && I ) {
	for ( sum = i = 0 ; i < Nw ; i += I )
	    sum += S[i]*S[i] ;
	for ( sum = 1./sum, i = 0 ; i < Nw ; i++ )
	    S[i] *= sum ;
    }
}

