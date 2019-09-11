#include "fftease.h"

void fftease_oscbank( t_fftease *fft )
{
	int    amp,freq,chan, n;
	
	t_float    a,ainc,f,finc,address;
	int D = fft->D;
	int I = D;
	int L = fft->L;
	t_float synt = fft->synt;
	t_float P  = fft->P;
	int R = fft->R;
	t_float Iinv = 1./fft->D;
	t_float pitch_increment = fft->pitch_increment;
	t_float *table = fft->table;
	t_float *lastamp = fft->lastamp ;
	t_float *lastfreq = fft->lastfreq ;
	t_float *bindex = fft->bindex;
	t_float *channel = fft->channel;
	t_float *output = fft->output;
	int hi_bin = fft->hi_bin;
	int lo_bin = fft->lo_bin;
	t_float maxamp = 0.0;
    t_float localthresh, testamp;
	short noalias = fft->noalias;
	t_float nyquist = fft->nyquist;
	if(! fft->init_status ){ // means memory alloc in effect
		goto exit;
	}
	if(R == 0){
		post("oscbank got 0 SR");
		return;
	}
	pitch_increment = P * (t_float) L / (t_float) R;
	
	if( synt > 0.0 ){
		maxamp = 0.0;
		for ( chan = lo_bin; chan < hi_bin; chan++ ){
			amp = chan << 1;
			testamp = fabs( channel[amp] );
			if( testamp > maxamp )
				maxamp = testamp;
		}
	}
	localthresh = synt * maxamp;
	
    for ( chan = lo_bin; chan < hi_bin; chan++ ) {
		if(! fft->init_status ){ // means memory alloc in effect
			goto exit;
		}
		freq = ( amp = ( chan << 1 ) ) + 1;
		if(noalias){
			if( channel[freq] * P >= nyquist )
				channel[amp] = 0;
		}
		if ( channel[amp] > localthresh ){ 
			channel[freq] *= pitch_increment;
			finc = ( channel[freq] - ( f = lastfreq[chan] ) )*Iinv;
			ainc = ( channel[amp] - ( a = lastamp[chan] ) )*Iinv;
			address = bindex[chan];
			if( address < 0 || address >= L){
				address = 0.0;
				// post("limited oscbank: bad address");
			}
			for ( n = 0; n < I; n++ ) {
				// taking this out now:
				
				if(! fft->init_status ){ // means memory alloc in effect
					goto exit;
				}
				
				output[n] += a*table[ (int) address ];
				address += f;
				while ( address >= L )
					address -= L;
				while ( address < 0 )
					address += L;
				a += ainc;
				f += finc;
			}
			lastfreq[chan] = channel[freq];
			lastamp[chan] = channel[amp];
			bindex[chan] = address;
		}
    }
exit:;
}