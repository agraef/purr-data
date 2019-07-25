#include "bashfest.h"

void transpose(t_bashfest *x, int slot, int *pcount)
{
  float *inbuf;
  float *outbuf;
  int i;
  int iphs = 0;
  int ip2;
  float m1, m2;
  float phs = 0;
  int out_frames;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;  
  int in_frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  float *params = x->params; 
//  float srate = x->sr;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  int buf_frames = x->buf_frames;
  float tfac;
  
  ++(*pcount);
  tfac = params[ (*pcount)++ ];
  // out_start MUST BE SET WITH RESPECT TO in_start
  
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;

  //  fprintf(stderr,"TRANSPOSE: in %d out %d\n", w->in_start, w->out_start);
  out_frames = (float) in_frames / tfac ;
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }

  for( i = 0; i < out_frames * channels; i += channels ){
    iphs = phs;
    m2 = phs - iphs;
    m1 = 1. - m2;

    if( channels == 1 ){
      *outbuf++ = inbuf[iphs] * m1 + inbuf[ iphs + 1] * m2 ; 

    } else if( channels == 2 ){
      ip2 = iphs * 2;
      *outbuf++ = inbuf[ip2] * m1 + inbuf[ ip2 + 2] * m2 ; 
      *outbuf++ = inbuf[ip2 + 1] * m1 + inbuf[ ip2 + 3] * m2 ; 
    }
    phs += tfac ;

  }

  x->events[slot].sample_frames =  out_frames;
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
  
}


void ringmod(t_bashfest *x, int slot, int *pcount)
{
  float *sinewave = x->sinewave;
  float *inbuf, *outbuf;
  int sinelen = x->sinelen;
  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  float *params = x->params; 
  float srate = x->sr;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
//  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  int i;
  float phase = 0.0;
  float si;
  float rmodFreq;

  ++(*pcount);
  rmodFreq = params[(*pcount)++];

  //  fprintf(stderr,"-*-*- EXECUTING RINGMOD -*-*-\n");

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;


  si = ((float) sinelen / srate) * rmodFreq ;

//  inbuf = inbuf + in_start ;

  for(i = 0; i < frames*channels; i += channels ){
    *outbuf++ = *inbuf++ * sinewave[(int)phase];
    if( channels == 2 ){
      *outbuf++ = *inbuf++ * sinewave[(int)phase];
    }
    phase += si;
    while( phase > sinelen )
      phase -= sinelen;
  } 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
}

void retrograde(t_bashfest *x, int slot, int *pcount)
{

  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
//  float *params = x->params;
//  float srate = x->sr;
  int i ;
  int swap1, swap2;
  float tmpsamp;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
      
  ++(*pcount);
  
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;

  memcpy(outbuf, inbuf, in_frames * channels * sizeof(float) );
    
  if( channels == 1 ){
    for(i = 0; i < (frames/2)  ; i++ ){
      swap2 = (frames - 1 - i);
      tmpsamp = outbuf[i];
      outbuf[i] = outbuf[swap2];
      outbuf[swap2] = tmpsamp;
    }
  }

  /* this would also work for mono, but we'll save a few multiplies */
  else {
    for(i = 0; i < (frames/2)   ; i++ ){
      swap1 = i * channels ;
      swap2 = (frames - 1 - i) * channels;
      tmpsamp = outbuf[swap1];
      outbuf[swap1] = outbuf[swap2];
      outbuf[swap2] = tmpsamp;
      ++swap1;
      ++swap2;  
      tmpsamp = outbuf[swap1];
      outbuf[swap1] = outbuf[swap2];
      outbuf[swap2] = tmpsamp;
    
    }
  } 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
  
}

void comber(t_bashfest *x, int slot, int *pcount)
{
  int channels = x->events[slot].out_channels;
  float *params = x->params; 
  float srate = x->sr;
  float *delayline1 = x->delayline1;  
  float *delayline2 = x->delayline2;  
  float max_delay = x->maxdelay ;
  int buf_frames = x->buf_frames;
  int out_frames ;
  float overhang, revtime, delay ;
  //  float *comb1, *comb2;
  int combsamps;
  int i;
  int fade_frames;
  float fadegain;
  int fadestart;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  /******************************/
  ++(*pcount);
  delay = params[(*pcount)++];
  revtime = params[(*pcount)++];
  overhang = params[(*pcount)++];

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
    
  if( delay <= 0.0 ){
    error("comber got bad delay value\n");
    return;
  }
  if( delay > max_delay ){
    delay = max_delay ;
  }
  if( overhang < COMBFADE )
    overhang = COMBFADE;

  out_frames = in_frames + overhang * srate ;
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }
  
  combsamps = delay * srate + 20 ;
  mycombset(delay,revtime,0,delayline1,srate);
  if( channels == 2 )
    mycombset(delay,revtime,0,delayline2,srate);

  // ADD IN ORIGINAL SIGNAL
  for( i = 0; i < in_frames*channels; i += channels){
    *outbuf++ += mycomb(*inbuf++, delayline1);
    if( channels == 2 ){
      *outbuf++ += mycomb(*inbuf++,delayline2);
    }
  }

  for( i = in_frames * channels; i < out_frames*channels; i += channels){
    *outbuf++ = mycomb( 0.0 , delayline1);
    if( channels == 2 ){
      *outbuf++ = mycomb( 0.0 , delayline2);
    }
  }

  fade_frames = COMBFADE * srate;
  fadestart = (out_frames - fade_frames) * channels ;
  for( i = 0; i < fade_frames * channels; i += channels ){
    fadegain = 1.0 - (float) i / (float) (fade_frames * channels)  ;
    *(inbuf + fadestart + i) *= fadegain;
    if(channels == 2){
      *(inbuf + fadestart + i + 1) *= fadegain;
    }
  }

  x->events[slot].sample_frames = out_frames; 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
  
  
}

void flange(t_bashfest *x, int slot, int *pcount)
{
  int i;
  float si;
  float mindel, maxdel;
  float fac1, fac2;
  int dv1[2], dv2[2]; /* cmix bookkeeping */
  float delsamp1, delsamp2 ;
  float delay_time;
  //  float dliget2();
  float speed, feedback, phase, minres, maxres;
  float hangover ;
  int hangframes ;
  
//  float *inbuf = x->events[slot].workbuffer;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
//  int buflen = x->buf_samps;
  float *params = x->params; 
  float srate = x->sr;
//  int in_start = x->events[slot].in_start;
  float *delayline1 = x->delayline1;  
  float *delayline2 = x->delayline2;  
  float max_delay = x->maxdelay ;    
  float *sinewave = x->sinewave;
  int sinelen = x->sinelen ;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;

  ++(*pcount);
  minres = params[(*pcount)++];
  maxres = params[(*pcount)++];
  speed = params[(*pcount)++];
  feedback = params[(*pcount)++];
  phase = params[(*pcount)++];

  hangover = feedback * 0.25 ; // maybe log relation
  hangframes = srate * hangover ;

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;


  if( minres <= 0. || maxres <= 0. ){
    error("flange: got zero frequency resonances as input");
    return;
  }
  mindel = 1.0/maxres;
  maxdel = 1.0/minres;

  if( maxdel > max_delay ){
    maxdel = max_delay;
    error("flange: too large delay time shortened");
  }

  delset2(delayline1, dv1, maxdel,srate);
  if( channels == 2 ){
    delset2(delayline2, dv2, maxdel,srate);
  }


  si = ((float) sinelen/srate) * speed ;

  if( phase > 1.0 ){
    phase = 0;
    error("flange: given > 1 initial phase");
  }
  delsamp1 = delsamp2 = 0;
  phase *= sinelen;
  fac2 = .5 * (maxdel - mindel) ;
  fac1 = mindel + fac2;

  for(i = 0; i < in_frames*channels; i += channels ){
    /* homemade oscillator */
    delay_time = fac1 + fac2 *  sinewave[(int) phase];
    if( delay_time < .00001 ){
      delay_time = .00001;
    }
    phase += si;
    while( phase > sinelen )
      phase -= sinelen;
    delput2( *inbuf + delsamp1*feedback, delayline1, dv1);
    delsamp1 = dliget2(delayline1, delay_time, dv1,srate);
    *outbuf++ = (*inbuf++ + delsamp1) ;
    if( channels == 2 ){
      delput2( *inbuf+delsamp2*feedback, delayline2, dv2);
      delsamp2 = dliget2(delayline2, delay_time, dv2,srate);
      *outbuf++ = (*inbuf++ + delsamp2) ;
    }
  }
  /* NOW DO HANGOVER */
  for(i = 0; i < hangframes*channels; i += channels ){

    delay_time = fac1 + fac2 *  sinewave[ (int) phase ];
    if( delay_time < .00001 ){
      delay_time = .00001;
    }
    phase += si;
    while( phase > sinelen )
      phase -= sinelen;
    delput2( delsamp1*feedback, delayline1, dv1);
    delsamp1 = dliget2(delayline1, delay_time, dv1,srate);
    *outbuf++ = delsamp1 ;
    if( channels == 2 ){
      delput2( delsamp2*feedback, delayline2, dv2);
      delsamp2 = dliget2(delayline2, delay_time, dv2,srate);
      *outbuf++ = delsamp2 ;
    }
  }
  x->events[slot].sample_frames += hangframes;   
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}

void butterme(t_bashfest *x, int slot, int *pcount)
{

  int ftype;
  float cutoff, cf, bw;
  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  float *params = x->params; 
  float srate = x->sr;
  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
//  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;


  ++(*pcount);
  ftype = params[(*pcount)++];
  
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
    
  if(ftype == HIPASS){
    cutoff = params[(*pcount)++];
    butterHipass(inbuf, outbuf, cutoff, frames, channels, srate);
  }
  else if(ftype == LOPASS){
    cutoff = params[(*pcount)++];
    butterLopass(inbuf, outbuf, cutoff, frames, channels, srate);
  }
  else if(ftype == BANDPASS){
    cf = params[(*pcount)++];
    bw = params[(*pcount)++];
    butterBandpass(inbuf, outbuf, cf, bw, frames, channels, srate);
  } else {
    error("%d not a valid Butterworth filter",ftype);
    return;
  }
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
}



void truncateme(t_bashfest *x, int slot, int *pcount)
{
  float shortdur ;
  int out_frames;
  int i;
  float fadegain ;
  int fade_frames;
  int fadestart;
  float fadeout;
  int channels = x->events[slot].out_channels;
  float *params = x->params; 
  float srate = x->sr;

  float *inbuf, *outbuf;
  int in_start;
  int out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  
  ++(*pcount);
  shortdur = params[ (*pcount)++ ];
  fadeout = params[ (*pcount)++ ];
  fade_frames = fadeout * srate ;
  out_frames = shortdur * srate ;
  if( out_frames >= in_frames ){
    // error("truncation requesting >= original duration, no truncation");
    return;
  }
  
  in_start = x->events[slot].in_start;
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  

  if( fade_frames <= 0 ){
    error("truncation with 0 length fade!");
    return;
  }

  if( fade_frames > out_frames ){
    error("truncation requested fadeout > new duration, adjusting...");
    fade_frames = out_frames;
  }

  memcpy(outbuf, inbuf, in_frames * sizeof(float) );
  
  fadestart = (out_frames - fade_frames) * channels ;

  for( i = 0; i < fade_frames * channels; i += channels ){
    fadegain = 1.0 - (float) i / (float) (fade_frames * channels)  ;
    outbuf[fadestart + i]   *= fadegain;
    if( channels == 2 ){
      outbuf[ fadestart + i + 1] *= fadegain;
    }
  }

  x->events[slot].sample_frames = out_frames ;
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
}

// Pd only - not reentrant - appears that sine wave gets screwed up ???
void sweepreson(t_bashfest *x, int slot, int *pcount)
{
  int i;
  float bwfac;
  float minfreq, maxfreq, speed, phase;
  float q1[5], q2[5];
  float cf, bw;
  float si;
  float fac1, fac2;
//  float inmax, outmax, rescale ;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  float *params = x->params; 
  float srate = x->sr;
  float *sinewave = x->sinewave;
  int sinelen = x->sinelen ;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;  

  
  ++(*pcount);
  minfreq = params[(*pcount)++];
  maxfreq = params[(*pcount)++];
  bwfac = params[(*pcount)++];
  speed = params[(*pcount)++];
  phase = params[(*pcount)++];

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  si = ((float) sinelen / srate) * speed ;

  if( phase > 1.0 ){
    phase = 0;
    error("sweepreson: given > 1 initial phase");
  }

  phase *= sinelen;
  fac2 = .5 * (maxfreq - minfreq) ;
  fac1 = minfreq + fac2;

  cf = fac1 + fac2 * sinewave[(int) phase];
  bw = bwfac * cf;
  rsnset2( cf, bw, 2.0, 0.0, q1, srate );
  if( channels == 2 ){
    rsnset2( cf, bw, 2.0, 0.0, q2, srate );
  } 
  
  for(i = 0; i < in_frames; i++ ){
    // homemade oscillator 

    phase += si;
    while( phase >= sinelen )
      phase -= sinelen;
	

    fac2 = .5 * (maxfreq - minfreq) ;
    fac1 = minfreq + fac2;

    cf = fac1 + fac2 * sinewave[(int) phase];
    bw = bwfac * cf; 
	if(cf < 10 || cf > 8000 || bw < 1 || srate < 100){
		post("danger values, cf %f bw %f sr %f",cf, bw, srate);
	}
   rsnset2( cf, bw, 2.0, 1.0, q1, srate );
   // clicks stop if we don't apply filter above, and if attacks come too fast
   *outbuf++ = reson(*inbuf++, q1);

    if( channels == 2 ){
	
    //  rsnset2( cf, bw, 2.0, 1.0, q2, srate );
	  *outbuf++ = reson(*inbuf++, q2); 
      
    }
  }
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}

void slidecomb(t_bashfest *x, int slot, int *pcount)
{
  float overhang, feedback, delay1, delay2;
//  int combsamps;
  int i;
  int fade_frames;
  float fadegain;
  int fadestart;
  int dv1[2], dv2[2];		/* cmix bookkeeping */
  float delsamp1 = 0, delsamp2 = 0;
  float m1, m2;
  float delay_time;
  int out_frames ;
    
  int channels = x->events[slot].out_channels;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;
//  float *sinewave = x->sinewave;
//  int sinelen = x->sinelen ;
  float max_delay = x->maxdelay;
  float *delayline1 = x->delayline1;  
  float *delayline2 = x->delayline2;  
  
  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;  
  
  ++(*pcount);
  delay1 = params[(*pcount)++];
  delay2 = params[(*pcount)++];
  feedback = params[(*pcount)++];
  overhang = params[(*pcount)++];

// post("del1 %f del2 %f srate %f",delay1,delay2, srate);

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
    
  if( overhang < COMBFADE )
    overhang = COMBFADE;

  
  out_frames = in_frames + overhang * srate ;
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }
  
  delset2(delayline1, dv1, max_delay, srate);
  if( channels == 2 ){
    delset2(delayline2, dv2, max_delay, srate);
  }



  for( i = 0; i < in_frames*channels; i += channels){
    m2 = (float) i / (float) (out_frames * channels) ;
    m1 = 1. - m2;
    delay_time = delay1 * m1 + delay2 * m2 ;
    delput2(*inbuf +delsamp1*feedback, delayline1, dv1);
    delsamp1 = dliget2(delayline1, delay_time, dv1, srate);
    *outbuf++ = *inbuf++ + delsamp1;
    if( channels == 2 ){
      delput2( *inbuf + delsamp2*feedback, delayline2, dv2);
      delsamp2 = dliget2(delayline2, delay_time, dv2, srate);
      *outbuf++ = *inbuf++ + delsamp2 ;
    }
  }

  for( i = in_frames * channels; i < out_frames*channels; i += channels){
    m2 = (float) i / (float) (out_frames * channels) ;
    m1 = 1. - m2;
    delay_time = delay1 * m1 + delay2 * m2 ;
    delput2( delsamp1*feedback, delayline1, dv1);
    *outbuf++ = delsamp1 = dliget2( delayline1, delay_time, dv1, srate );
    if( channels == 2 ){
      delput2( delsamp2*feedback, delayline2, dv2);
      *outbuf++ = delsamp2 = dliget2( delayline2, delay_time, dv2, srate );
    }
  }

  fade_frames = COMBFADE * srate;
  fadestart = (out_frames - fade_frames) * channels ;
  for( i = 0; i < fade_frames * channels; i += channels ){
    fadegain = 1.0 - (float) i / (float) (fade_frames * channels)  ;
    *(outbuf + fadestart + i) *= fadegain;
    if( channels == 2 ){
      *(outbuf + fadestart + i + 1) *= fadegain;
    }
  }
  x->events[slot].sample_frames = out_frames;  
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}
// still a crash whore in Pd: 

void reverb1(t_bashfest *x, int slot, int *pcount)
{

  float revtime, overhang;
  int channel_to_compute;
  float drygain;
  int out_frames;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
      
  ++(*pcount);
  revtime = params[(*pcount)++];
  if( revtime >= 1. ){
    error("reverb1 does not like feedback values over 1.");
    revtime = .99 ;
  }
  overhang = params[(*pcount)++];
  drygain = params[(*pcount)++];


  out_frames = in_frames + srate * overhang;
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  
  for( channel_to_compute = 0; channel_to_compute < channels; channel_to_compute++) {
    reverb1me( inbuf, outbuf, in_frames, out_frames, channels, channel_to_compute, revtime, drygain, x);
  }
  

  x->events[slot].sample_frames = out_frames;
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
  
}

void ellipseme(t_bashfest *x, int slot, int *pcount)
{
  int i,j;
  int nsects;
  float xnorm;
  int filtercode ;
  float *fltdata;

//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
//  int buf_frames = x->buf_frames;
  float *params = x->params; 
//  float srate = x->sr;
  float **flts = x->ellipse_data;
  LSTRUCT *eel = x->eel;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
    
  ++(*pcount);
  filtercode = params[(*pcount)++];

  if( filtercode >= ELLIPSE_FILTER_COUNT ){
    error("there is no %d ellipse data",filtercode);
    return;
  };
  fltdata = flts[ filtercode ];

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  for( j = 0; j < channels; j++) {
    ellipset(fltdata,eel,&nsects,&xnorm); 
    for( i = j; i < in_frames * channels ; i += channels ){
      outbuf[i] = ellipse(inbuf[i], eel, nsects,xnorm);
    }
  } 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}

void feed1me(t_bashfest *x, int slot, int *pcount)
{
//  int i;
  float mindelay, maxdelay, speed1, speed2;
  float phz1 = .13, phz2 = .251;
  float dur;
  float minfeedback = .1, maxfeedback = .7;
  float desired_dur;
  float overhang;
  /* main variables */

//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;
  int out_frames;
  /* process specfic*/
  int flen = x->feedfunclen ;
  float *func1 = x->feedfunc1;
  float *func2 = x->feedfunc2;
  float *func3 = x->feedfunc3;
  float *func4 = x->feedfunc4;
  float my_max_delay = x->max_mini_delay;
  float *sinewave = x->sinewave;
  int sinelen = x->sinelen ;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  
  ++(*pcount);
  mindelay = params[ (*pcount)++ ];
  maxdelay = params[ (*pcount)++ ];
  speed1 = params[ (*pcount)++ ];
  speed2 = params[ (*pcount)++ ];
  overhang = params[ (*pcount)++ ];

  if( maxdelay > my_max_delay ){
    error("feed1: too high max delay, adjusted");
    maxdelay = my_max_delay ;
  }
  dur = in_frames / srate ;
  desired_dur = dur + overhang;
  out_frames = srate * desired_dur ;
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }
  
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;

  funcgen1( func1, flen, desired_dur, mindelay, maxdelay,
	    speed1, speed2, 1.0, 1.0, &phz1, &phz2, sinewave, sinelen);

  phz1 /= (float) flen; phz2 /= (float) flen;


  funcgen1( func2, flen, desired_dur, mindelay*.5, maxdelay*2.0,
	    speed1*1.25, speed2*.75, 1.0, 1.0, &phz1, &phz2, sinewave, sinelen);

  phz1 /= (float) flen; phz2 /= (float) flen;


  funcgen1( func3, flen, desired_dur, minfeedback, maxfeedback,
	    speed1*.35, speed2*1.25, 1.0, 1.0, &phz1, &phz2, sinewave, sinelen);

  phz1 /= (float) flen; phz2 /= (float) flen;

  funcgen1( func4,flen, desired_dur, minfeedback, maxfeedback,
	    speed1*.55, speed2*2.25, 1.0, 1.0, &phz1, &phz2, sinewave, sinelen);
		
  feed1( inbuf, outbuf, in_frames, out_frames, channels, func1, func2, func3, func4, flen, dur, my_max_delay, x);

  x->events[slot].sample_frames = out_frames; 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}

void flam1(t_bashfest *x, int slot, int *pcount)
{
//  int channel_to_compute;
  int attacks;
  float gain2;
  float gainatten;
  float delay;
  float gain = 1.0;
  int i, j, k, delaysamps, delayoffset = 0;
//  float inputmax;
  int delay_frames;
  /* main variables */
  float *inbuf;
  float *outbuf;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  int buflen = x->buf_samps;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int out_frames;
  int halfbuffer = x->halfbuffer;
  /* process specfic*/
  ++(*pcount);  
  attacks = params[(*pcount)++];
  gain2 = params[(*pcount)++];
  gainatten = params[(*pcount)++];
  delay = params[(*pcount)++];



  if( attacks <= 1 ){
    error("flam1: too few attacks: %d",attacks);
    return;
  }

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;

  delay_frames = srate * delay + 0.5;
  delaysamps = channels * delay_frames;
  out_frames = in_frames + (srate * delay * (float) (attacks - 1));
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }

  for( i = 0; i < out_frames * channels; i++ ){
    outbuf[i] = 0.0 ;
  }  

  for(i = 0; i < attacks; i++ ){
    if(in_frames + delay_frames * i >= out_frames){
      // error("breaking at attack %d",i);
      break;
    }
    for(j = 0; j < in_frames * channels; j += channels ){
      for( k = 0; k < channels; k++ ){
		outbuf[j + k + delayoffset] += *(inbuf +j + k) * gain;
      }
    }
    delayoffset += delaysamps;
    if( i == 0 ){
      gain = gain2;
    } else {
      gain *= gainatten;
    }    
  }

  x->events[slot].sample_frames = out_frames; 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;    
}

void flam2(t_bashfest *x, int slot, int *pcount)
{
  //  int channel_to_compute;
  int attacks;
  float gain2;
  float gainatten;
  float delay1,delay2;
  float gain = 1.0;
  int i, j, k, delaysamps, delayoffset = 0;
  int f_endpoint;
//  float inputmax, outputmax, rescale;
  int delay_frames;
  float now = 0.0;
  int findex;
  float inval;
  float curdelay;
  /* main variables */
  float *inbuf;
  float *outbuf;
  int out_frames;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  int buflen = x->buf_samps;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int halfbuffer = x->halfbuffer;
  float *flamfunc1 = x->flamfunc1;
  int flamfunclen = x->flamfunc1len;
  /* process specfic*/

  ++(*pcount);
  attacks = params[(*pcount)++];
  gain2 = params[(*pcount)++];
  gainatten = params[(*pcount)++];
  delay1 = params[(*pcount)++];
  delay2 = params[(*pcount)++];

  if( attacks <= 1 ){
    error("flam2: recieved too few attacks: %d",attacks);
    return;
  }
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  for( i = 0; i < attacks - 1; i++ ){
    findex = ((float)i/(float)attacks) * (float)flamfunclen ;
    inval = flamfunc1[findex];
    curdelay = mapp(inval, 0., 1., delay2, delay1);
    now += curdelay;
  }
  out_frames = in_frames + (srate * now);
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }  
  
  for( i = 0; i < out_frames * channels; i++ ){
    outbuf[i] = 0.0 ;
  }  

  f_endpoint = in_frames;
  // first time delay_offset is zero
  for( i = 0; i < attacks; i++ ){
    findex = ((float)i/(float)attacks) * (float)flamfunclen ;
    inval = flamfunc1[findex];
    curdelay = mapp(inval, 0., 1., delay2, delay1);

    delay_frames = srate * curdelay + 0.5;
    delaysamps = delay_frames * channels;
    if(f_endpoint >= out_frames){
      // error("flam2: breaking at attack %d",i);
      break;
    }
    for(j = 0; j < in_frames * channels; j += channels ){
      for( k = 0; k < channels; k++ ){
		outbuf[j + k + delayoffset] += *(inbuf + j + k) * gain;
      }
    }
    delayoffset += delaysamps;
    f_endpoint = in_frames + delayoffset/channels;
    if( i == 0 ){
      gain = gain2;
    } else {
      gain *= gainatten;
    }    
  }

  x->events[slot].sample_frames = out_frames; 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;    
}

void expflam(t_bashfest *x, int slot, int *pcount)
{
  int attacks;
  float gain2;
  float gainatten;
  float delay1,delay2;
  float gain = 1.0;
  int i, j, k, delaysamps, delayoffset = 0, f_endpoint;
//  float inputmax, outputmax, rescale;
  int delay_frames;
  float now = 0.0;
//  int findex;
//  float inval;
  float curdelay;
  float slope;
  /* main variables */
  float *inbuf;
  float *outbuf;
  int out_frames;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  int buflen = x->buf_samps;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int halfbuffer = x->halfbuffer;
  float *expfunc = x->feedfunc1;
//  int funclen = x->feedfunclen;
  /* process specfic*/

  ++(*pcount);
  attacks = params[(*pcount)++];
  gain2 = params[(*pcount)++];
  gainatten = params[(*pcount)++];
  delay1 = params[(*pcount)++];
  delay2 = params[(*pcount)++];
  slope = params[(*pcount)++];

  if( attacks <= 1 ){
    error("expflam: recieved too few attacks: %d",attacks);
    return;
  }
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;

  setExpFlamFunc(expfunc, attacks, delay1, delay2, slope);
    
  for( i = 0; i < attacks - 1; i++ ){
    now += expfunc[i];
  }
  
  out_frames = in_frames + (srate * now);
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }  
  
  for( i = 0; i < out_frames * channels; i++ ){
    outbuf[i] = 0.0 ;
  }  

  f_endpoint = in_frames;
  
  for( i = 0; i < attacks; i++ ){
    curdelay = expfunc[i];
    delay_frames = srate * curdelay + 0.5;
    delaysamps = delay_frames * channels;
    if(f_endpoint >= out_frames){
   // error("expflam: breaking at attack %d",i);
      break;
    }
    for(j = 0; j < in_frames * channels; j += channels ){
      for( k = 0; k < channels; k++ ){
		outbuf[j + k + delayoffset] += *(inbuf + j + k) * gain;
      }
    }
    delayoffset += delaysamps;
    f_endpoint = in_frames + delayoffset/channels;
    if( i == 0 ){
      gain = gain2;
    } else {
      gain *= gainatten;
    }    
  }

  x->events[slot].sample_frames = out_frames; 
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;    
}

void comb4(t_bashfest *x, int slot, int *pcount)
{
  float overhang, revtime ;
  int i, j, k;
  int fadeFrames;
  float fadegain;
  int fadestart;
  float input_sample;
  float rez;
  /* main variables */

  int out_frames;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;

  /* process specific */
  CMIXCOMB *combies = x->combies;
  float maxloop = x->max_comb_lpt;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  
  ++(*pcount);
  
  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  for( j = 0; j < 4; j++ ){
    rez = params[(*pcount)++] ;
    if( rez == 0.0){
      error("comb4: 0 resonance frequency not allowed");
      return;
    }
    if( 1./rez > maxloop ){
      error("comb4: %f is too long loop",1./rez);
      return;
    }
    combies[j].lpt = 1. / rez ;
  }

  revtime = params[(*pcount)++];
  overhang = params[(*pcount)++];
  if( overhang < COMBFADE )
    overhang = COMBFADE;
  out_frames = in_frames + overhang * srate;
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  }  
  for( j = 0; j < 4; j++ ){
    mycombset( combies[j].lpt, revtime, 0, combies[j].arr, srate);
  }

  inbuf = x->events[slot].workbuffer + in_start;
  
  for( j = 0; j < channels; j++ ){
    for( i = 0; i < in_frames * channels; i += channels ){
      input_sample = *(inbuf + i + j) ; // we can move inside loop
      *(outbuf + i + j ) = 0.0; // comment out to leave original sound into it
      for( k = 0; k < 4; k++ ){
		*(outbuf + i + j) += mycomb(input_sample, combies[k].arr);
      }
    }
  }
  for( i = in_frames * channels; i < out_frames * channels; i += channels ){
    for( j = 0; j < channels; j++ ){
      *(outbuf + i + j) = 0.0;
      for( k = 0; k < 4; k++ ){
		*(outbuf +i+j) += mycomb(0.0,combies[k].arr);
      }
    }
  } 
  fadeFrames = COMBFADE * srate; // ok - this is just the fadeout
  fadestart = (out_frames - fadeFrames) * channels ;
  for( i = 0; i < fadeFrames * channels; i += channels ){
    fadegain = 1.0 - (float) i / (float) (fadeFrames * channels)  ;
    *(outbuf + fadestart + i) *= fadegain;
    if( channels == 2 ){
      *(outbuf + fadestart + i + 1) *= fadegain;
    }
  }
  killdc(outbuf, out_frames, channels, x);
  x->events[slot].sample_frames = out_frames;
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
     
}

void compdist(t_bashfest *x, int slot, int *pcount)
{
  float cutoff, maxmult;
  int lookupflag;
  int channel_to_compute;
  float maxamp;
  /* main variables */

//  int out_frames;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
//  int buf_frames = x->buf_frames;
  float *params = x->params; 
//  float srate = x->sr;
  /* function specific*/
  int range = x->tf_len;
  float *table = x->transfer_function;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  
  ++(*pcount);
  cutoff = params[(*pcount)++];
  maxmult = params[(*pcount)++];
  lookupflag = params[(*pcount)++];
  

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  maxamp = getmaxamp(inbuf, in_frames*channels) ;

  if(lookupflag){
    set_distortion_table(table, cutoff, maxmult, range);
  }
  
  for( channel_to_compute = 0; channel_to_compute < channels; channel_to_compute++) {
    do_compdist(inbuf, outbuf, in_frames, channels, channel_to_compute, 
		cutoff, maxmult, lookupflag, table, range, maxamp);
  }  
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}

void ringfeed(t_bashfest *x, int slot, int *pcount)
{
  float overhang;
  int i, j;
  int fade_frames;
  float fadegain;
  int fadestart;
  float input_sample;
  float rez ;
  /* main variables */
  int out_frames;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;
  /*function specific*/
  float *sinewave = x->sinewave;
  int sinelen = x->sinelen ;
  CMIXCOMB *combies = x->combies;
  CMIXRESON *resies = x->resies;
  CMIXOSC oscar = x->oscar;
  float maxloop = x->max_comb_lpt;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  
  ++(*pcount);

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  oscar.func = sinewave;
  oscar.len = sinelen;
  oscar.si = params[(*pcount)++] * ((float)oscar.len / srate);
  oscar.phs = 0;
  rez = params[(*pcount)++] ;
  if( rez > 0 )
    combies[0].lpt = 1. / rez ;
  else error("zero comb resonance is bad luck");
  if(combies[0].lpt > maxloop)
    error("ringfeed does not appreciate looptimes as large as %f",combies[0].lpt);

  combies[0].rvbt = params[(*pcount)++] ;
  if(combies[0].rvbt >= 1.0) {
    error("ringfeed dislikes feedback values >= 1");
    combies[0].rvbt = .99 ;
  }
  resies[0].cf = params[(*pcount)++];
  resies[0].bw  = resies[0].cf * params[(*pcount)++];
  overhang = params[(*pcount)++] ;
  
  inbuf = x->events[slot].workbuffer + in_start;

  for( i = 0; i < channels ; i++ ){
    mycombset( combies[0].lpt, combies[0].rvbt, 0, combies[i].arr,srate);
    rsnset2(resies[0].cf, resies[0].bw, RESON_NO_SCL, 0., resies[i].q, srate);
  }

  /* MINIMUM OVERHANG */

  if( overhang < COMBFADE )
    overhang = COMBFADE;

  out_frames = in_frames + overhang * srate ;
  if( out_frames > buf_frames / 2 ){
    out_frames = buf_frames / 2 ;
  } 
  /* INPUT LOOP */
  for( i = 0; i < in_frames * channels; i += channels ){
    for( j = 0; j < channels; j++ ){
      input_sample = *(inbuf + i + j ) ;

      input_sample *= oscil(1.0, oscar.si, oscar.func, oscar.len, &oscar.phs);
      input_sample += mycomb(input_sample, combies[j].arr);
      *(outbuf +i+j) = reson(input_sample, resies[j].q);
    }
  }
  
  /* COMB TAILS */

  for( i = in_frames * channels; i < out_frames * channels; i += channels ){
    for( j = 0; j < channels; j++ ){
      *(outbuf +i+j) = reson(mycomb( 0.0, combies[j].arr), resies[j].q );
    }
  }

  /* FADE OUT ON MIX */

  fade_frames = COMBFADE * srate;
  fadestart = (out_frames - fade_frames) * channels ;
  for( i = 0; i < fade_frames * channels; i += channels ){
    fadegain = 1.0 - (float) i / (float) (fade_frames * channels)  ;
    *(outbuf + fadestart + i) *= fadegain;
    if( channels == 2 ){
      *(outbuf + fadestart + i + 1) *= fadegain;
    }
  }
  x->events[slot].sample_frames = out_frames;
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}

void resonadsr(t_bashfest *x, int slot, int *pcount)
{
  int i;
  float bwfac;
  float q1[5], q2[5];
  float cf, bw;
  float si;
  float notedur;
  float phase = 0.;
//  int j = 0;
  /* main variables */

//  int out_frames;
//  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
//  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;


  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  
  /*function specific*/
  CMIXADSR *a = x->adsr;
  int funclen = a->len;
  float *adsrfunc = a->func;

  ++(*pcount);
  a->a = params[(*pcount)++];
  a->d = params[(*pcount)++];
  a->r = params[(*pcount)++];
  a->v1 = params[(*pcount)++];
  a->v2 = params[(*pcount)++];
  a->v3 = params[(*pcount)++];
  a->v4 = params[(*pcount)++];
  bwfac = params[(*pcount)++];

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;
  outbuf = x->events[slot].workbuffer + out_start;
  
  notedur = (float) in_frames / srate ;
  a->s = notedur - (a->a+a->d+a->r);
  if( a->s <= 0.0 ){
    a->a=a->d=a->s=a->r= notedur/ 4. ;
  }
  buildadsr(a);
  si = ((float) funclen / srate) / notedur ;

  phase = 0;

  rsnset2(adsrfunc[(int)phase], adsrfunc[(int) phase]*bwfac, 2.0, 0.0, q1, srate);
  if( channels == 2 ){
    rsnset2( adsrfunc[(int)phase], adsrfunc[(int) phase]*bwfac, 2.0, 0.0, q2, srate );
  }
  
  for(i = 0; i < in_frames*channels; i += channels ){
    phase += si;
    if( phase > funclen - 1)
      phase = funclen - 1;	/* stop at end of function */

    cf = adsrfunc[ (int) phase ];
    bw = bwfac * cf ;
    rsnset2( cf, bw, 2.0, 1.0, q1, srate );
    outbuf[i] = reson(inbuf[i], q1);
    if( channels == 2 ){
      rsnset2( cf, bw, 2.0, 1.0, q2, srate );
      outbuf[i+1] = reson(inbuf[i+1], q2);
    }
  }
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;

}

void stv(t_bashfest *x, int slot, int *pcount)
{
  int i,j;
  /* main variables */

//  int out_frames;
  int frames = x->events[slot].sample_frames;
  int channels = x->events[slot].out_channels;
//  int buf_frames = x->buf_frames;
  float *params = x->params; 
  float srate = x->sr;
  /*function specific*/
  float *sinewave = x->sinewave;
  int sinelen = x->sinelen ;
  float *delayline1 = x->delayline1;  
  float *delayline2 = x->delayline2;  
  float max_delay = x->maxdelay ;
  CMIXOSC osc1, osc2; // put into main object structure
  float mindel, maxdel;
  float fac1, fac2;
  int dv1[2], dv2[2]; /* cmix bookkeeping */
  float delay_time;
  float speed1, speed2, depth ;
//  float max;

  float *inbuf, *outbuf;
  int in_start = x->events[slot].in_start;
  int out_start = x->events[slot].out_start;
//  int in_frames = x->events[slot].sample_frames;
  int buflen = x->buf_samps;
  int halfbuffer = x->halfbuffer;
  
  ++(*pcount);
  speed1 = params[(*pcount)++];
  speed2 = params[(*pcount)++];
  depth = params[(*pcount)++];

  out_start = (in_start + halfbuffer) % buflen ;
  inbuf = x->events[slot].workbuffer + in_start;    
  outbuf = x->events[slot].workbuffer + out_start;  

  mindel = .001;
  maxdel = depth;

  if( maxdel > max_delay ){
    maxdel = max_delay;
  }

  delset2(delayline1, dv1, max_delay,srate);
  delset2(delayline2, dv2, max_delay,srate);

  fac2 = .5 * (maxdel - mindel) ;
  fac1 = mindel + fac2;

  osc1.func = sinewave;
  osc1.len = sinelen;
  osc1.si = ((float) sinelen / srate ) * speed1 ;
  osc1.phs = 0;
  osc1.amp = fac2;

  osc2.func = sinewave;
  osc2.len = sinelen;
  osc2.si = ((float) sinelen / srate ) * speed2 ;
  osc2.phs = 0;
  osc2.amp = fac2;

  if( channels == 1 ){
    for(i = 0, j = 0; i < frames; i++, j+=2 ){
      
      delay_time = fac1 +
	oscil(osc1.amp, osc1.si, osc1.func, osc1.len, &osc1.phs);
      delput2( inbuf[i], delayline1, dv1);
      outbuf[j] = dliget2(delayline1, delay_time, dv1,srate);
      
      delay_time = fac1 +
	oscil(osc2.amp, osc2.si, osc2.func, osc2.len, &osc2.phs);
      delput2( inbuf[i], delayline2, dv2);
      outbuf[j + 1] = dliget2(delayline2, delay_time, dv2,srate);    
    }
  }
  else if( channels == 2 ){
    for(i = 0; i < frames*2; i += 2 ){
      delay_time = fac1 +
	oscil(osc1.amp, osc1.si, osc1.func, osc1.len, &osc1.phs);
      delput2( inbuf[i], delayline1, dv1);
      outbuf[i] = dliget2(delayline1, delay_time, dv1,srate);
      
      delay_time = fac1 +
	oscil(osc2.amp, osc2.si, osc2.func, osc2.len, &osc2.phs);
      delput2( inbuf[i + 1], delayline2, dv2);
      outbuf[i + 1] = dliget2(delayline2, delay_time, dv2,srate);    
      
    }
  }
  x->events[slot].out_start = in_start;
  x->events[slot].in_start = (x->events[slot].out_start + halfbuffer) % buflen ;
  x->events[slot].out_channels = 2; // we are now stereo, regardless of what we were before
}
