#include "fftease.h"
// extern void post(const char *fmt, ...);
#define PARANOID 0

void fftease_limited_oscbank( t_fftease *fft, int osclimit, t_float framethresh)
{
	int    amp,freq,chan, n;
	
	t_float    a,ainc,f,finc,address;
	int D = fft->D;
	int I = D;
	int L = fft->L;
	t_float synt = fft->synt;
	t_float P  = fft->P; 
	int R = fft->R;
	int N2 = fft->N2;
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
	int oscnt = 0;
#ifdef PARANOID	
	if(! fft->init_status ){ // means memory alloc in effect
		goto exit;
	}
#endif
	
	if(R == 0){
		post("FFTeaseLib: limited oscbank got 0 SR");
		return;
	}

	if(lo_bin < 0 || hi_bin > N2){
		post("FFTeaseLib: limited oscbank: bad bins: %d %d",lo_bin,hi_bin);
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
	if(maxamp > framethresh){
		localthresh = synt * maxamp;
	} else {
		localthresh = synt * framethresh; // watch this line!
	}
	
	
    for ( chan = lo_bin; chan < hi_bin; chan++ ) {
#ifdef PARANOID	
		if(! fft->init_status ){ // means memory alloc in effect
			goto exit;
		}
#endif
		freq = ( amp = ( chan << 1 ) ) + 1;
		if(noalias){
			if( channel[freq] * P >= nyquist )
				channel[amp] = 0;
		}
		if ( channel[amp] > localthresh ){ 
			++oscnt;
#ifdef PARANOID	
			if(! fft->init_status ){ // means memory alloc in effect
				goto exit;
			}
#endif
			if(oscnt > osclimit){
				goto exit;
			}
			channel[freq] *= pitch_increment;
			finc = ( channel[freq] - ( f = lastfreq[chan] ) )*Iinv;
			ainc = ( channel[amp] - ( a = lastamp[chan] ) )*Iinv;
			address = bindex[chan];
			// this was the bug - somewhere bindex was not properly initialized!
			//i_address = (int) address;
			if( address < 0 || address >= L){
				address = 0;
				// post("limited oscbank: bad address");
			}			
			for ( n = 0; n < I; n++ ) {
#ifdef PARANOID	
				if(! fft->init_status ){ // means memory alloc in effect
					goto exit;
				}
#endif
// this is a suspected bug line:
					/*
				iAddress = (int) address;
				if( iAddress == L ){
					iAddress = L - 1;
				}
				
			
				if( iAddress < 0 || iAddress >= L ){
					post("limited oscbank: bad address: %d", iAddress);
				} else {
					output[n] += a*table[ iAddress ];
				}
				*/
				// skip excessive paranoia for efficiency
				output[n] += a*table[ (int) address ]; // this WILL go to L, so tab needs a guardpoint
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