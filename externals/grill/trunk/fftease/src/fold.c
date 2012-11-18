/*
 * multiply current input I by window W (both of length Nw);
 * using modulus arithmetic, fold and rotate windowed input
 * into output array O of (FFT) length N according to current
 * input time n
 */
void fold( float *I, float  *W, int Nw, float *O, int N, int n )
{
#if 1
    int i;
    for ( i = 0; i < N; i++ ) O[i] = 0.;

    while ( n < 0 ) n += N;
    n %= N;

    for ( i = 0; i < Nw; i++ ) {
		O[n] += I[i]*W[i];
		if ( ++n == N )  n = 0;
    }
#else
   int i;

    for ( i = 0; i < N; i++ )
	O[i] = 0.;

    while ( n < 0 )
	n += N;
    n %= N;
    for ( i = 0; i < Nw; i++ ) {
	O[n] += I[i]*W[i];
	if ( ++n == N )
	    n = 0;
    }
#endif
}


/*
 * input I is a folded spectrum of length N; output O and
 * synthesis window W are of length Nw--overlap-add windowed,
 * unrotated, unfolded input data into output O
 */
void overlapadd( float *I, int N, float *W, float *O, int Nw, int n )
{
#if 1
	int i ;
    while ( n < 0 ) n += N ;
    n %= N ;

    for ( i = 0 ; i < Nw ; i++ ) {
		O[i] += I[n]*W[i] ;
		if ( ++n == N ) n = 0 ;
    }
#else
 int i ;
    while ( n < 0 )
	n += N ;
    n %= N ;
    for ( i = 0 ; i < Nw ; i++ ) {
	O[i] += I[n]*W[i] ;
	if ( ++n == N )
	    n = 0 ;
    }

#endif
}

