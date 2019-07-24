
#include "fftease.h"

void fftease_bloscbank( t_float *S, t_float *O, int D, t_float iD, t_float *lf, t_float *la, t_float *index, t_float *tab,
			   int len, t_float synt, int lo, int hi )
{
	int    amp,freq,chan, i;

	t_float    a,ainc,f,finc,address;

	for ( chan = lo; chan < hi; chan++ ) {

		freq = ( amp = ( chan << 1 ) ) + 1;
		if ( S[amp] > synt ){
			finc = ( S[freq] - ( f = lf[chan] ) )* iD;
			ainc = ( S[amp] - ( a = la[chan] ) )* iD;
			address = index[chan];
			for ( i = 0; i < D ; i++ ) {
				O[i] += a*tab[ (int) address ];

				address += f;
				while ( address >= len )
					address -= len;
				while ( address < 0 )
					address += len;
				a += ainc;
				f += finc;
			}
			lf[chan] = S[freq];
			la[chan] = S[amp];
			index[chan] = address;
		}
	}
}