#include "bashfest.h"
#include "stdlib.h"

void putsine (float *arr, int len);
float boundrand(float min, float max);


void putsine (float *arr, int len) 
{
  int i;
  double twopi;
  twopi = 8.0 * atan2(1.,1.);

  for ( i = 0; i < len ; i++) {
    *(arr + i) = sin( twopi * i / len);
  }
}

float boundrand(float min, float max)
{
  return min + (max-min) * ((float)rand()/MY_MAX);
}


void mycombset(float loopt,float rvt,int init,float *a,float srate)
{
  int j;
  
  a[0] =  (3.0 + (loopt * srate + .5));
  a[1] = rvt;
  if(!init) { 
    for(j=3; j<(int)*a; j++)  
      a[j] = 0;
    a[2] = 3;
  }
}

float mycomb(float samp,float *a)
{
  float temp,*aptr;
  if ( a[2] >= (int) a[0]) 
    a[2] = 3;
  aptr = a + (int)a[2];
  a[2]++; 
  temp = *aptr;
  *aptr = *aptr * a[1] + samp;
  return(temp);
}

void setweights(float *a, int len)
{
  float sum = 0.0;
  int i;
  for(i=0;i<len;i++)
    sum += a[i];
  if(sum == 0.0){
    error("zero odds sum");
  }
  for(i=0;i<len;i++)
    a[i] /= sum;
  for(i=1;i<len;i++)
    a[i] += a[i-1];
}

void  delset2(float *a,int *l,float xmax, float srate)
{
  /* delay initialization.  a is address of float array, l is size-2 int 
   * array for bookkeeping variables, xmax, is maximum expected delay */

  int i;
  *l = 0;
  *(l+1) = (int)(xmax * srate + .5);
  for(i = 0; i < *(l+1); i++) *(a+i) = 0;
}

void delput2(float x,float *a,int *l)
{

  /* put value in delay line. See delset. x is float */

  *(a + (*l)++) = x;
  if(*(l) >= *(l+1)) *l -= *(l+1);
}                                                            

float dliget2(float *a,float wait,int *l,float srate)
{
  /* get interpolated value from delay line, wait seconds old */
  register int im1;
  float x = wait * srate;
  register int i = x;
  float frac = x - i;
  i = *l - i;
  im1 = i - 1;
  if(i <= 0) { 
    if(i < 0) i += *(l+1);
    if(i < 0) return(0.);
    if(im1 < 0) im1 += *(l+1);
  }
  return(*(a+i) + frac * (*(a+im1) - *(a+i)));
}

void butterLopass( float *in, float *out, float cutoff, int frames, int channels, float srate)

{
  int channel_to_compute;
  float data[8];

  for( channel_to_compute = 0; channel_to_compute < channels; channel_to_compute++) {
    butset( data );
    lobut(data, cutoff, srate);
    butter_filter( in, out, data, frames, channels, channel_to_compute);
  }

}

void butterBandpass(float *in, float *out, float center, float bandwidth, int frames,int  channels, float srate)
{
  int channel_to_compute;
  float data[8];

  for( channel_to_compute = 0; channel_to_compute < channels; channel_to_compute++) {
    butset( data );
    bpbut(data, center, bandwidth, srate);
    butter_filter( in, out, data, frames, channels, channel_to_compute);
  }

}


void butterHipass(float *in, float *out, float cutoff, int frames,int channels, float srate)
{
  int channel_to_compute;
  float data[8];

  for( channel_to_compute = 0; channel_to_compute < channels; channel_to_compute++) {
    butset( data );
    hibut(data, cutoff, srate);
    butter_filter( in, out, data, frames, channels, channel_to_compute);
  }

}

void butset(float *a)		
{
  a[6] = a[7] = 0.0;
}

void lobut(float *a, float cutoff,float SR)			
{
  register float	 c;

  c = 1.0 / tan( PI * cutoff / SR);
  a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
  a[2] = a[1] + a[1];
  a[3] = a[1];
  a[4] = 2.0 * ( 1.0 - c*c) * a[1];
  a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];

	
}

void hibut(float *a, float cutoff, float SR)			
{

  register float	c;

  c = tan( PI * cutoff / SR);
  a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
  a[2] = -2.0 * a[1];
  a[3] = a[1];
  a[4] = 2.0 * ( c*c - 1.0) * a[1];
  a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];

}

void bpbut(float *a, float formant, float bandwidth,float  SR)
{
  register float  c, d;

  c = 1.0 / tan( PI * bandwidth / SR);
  d = 2.0 * cos( 2.0 * PI * formant / SR);
  a[1] = 1.0 / ( 1.0 + c);
  a[2] = 0.0;
  a[3] = -a[1];
  a[4] = - c * d * a[1];
  a[5] = ( c - 1.0) * a[1];
	
}
/* in array can == out array */

void butter_filter(float *in,float *out,float *a, int frames, int channels, int channel)
{

  int i;
  float t ,y ;

  for( i = channel ; i < frames * channels; i+= channels )
    {
      t = *(in + i) - a[4] * a[6] - a[5] * a[7];
      y = t * a[1] + a[2] * a[6] + a[3] * a[7];
      a[7] = a[6];
      a[6] = t;
      *(out + i) = y;
    }
}

void rsnset2(float cf,float bw,float scl,float xinit,float *a,float srate)
{
//  double exp(),cos(),sqrt();
  float c,temp;
  if(!xinit) {
    a[4] = 0;
    a[3] = 0;
  }
  a[2] = exp(-PI2 * bw/srate);
  temp = 1. - a[2];
  c = a[2] + 1;
  a[1] = 4. * a[2]/c * cos(PI2 * cf/srate);
  if(scl < 0) a[0] = 1;
  if(scl) a[0] = sqrt(temp/c*(c*c-a[1]*a[1]));
  if(!scl) a[0] = temp*sqrt(1.-a[1]*a[1]/(4.*a[2]));
}

float reson(float x,float *a)
{
  float temp;
  temp = *a * x + *(a+1) * *(a+3) - *(a+2) * *(a+4);
  *(a+4) = *(a+3);
  *(a+3) = temp;
  return(temp);
}

float allpass(float samp,float *a)
{
  float temp,*aptr;
  if ( a[STARTM1] >= (int) a[0]) a[STARTM1] = START;
  aptr = a + (int)a[STARTM1];
  a[STARTM1] ++; 
  temp = *aptr;
  *aptr = *aptr * a[1] + samp;
  return(temp - a[1] * *aptr);
}

void init_reverb_data(float *a)
{
  a[0] = 2;
  a[1] = -0.61043329;
  a[2] = -1.4582246;
  a[3] = 1;
  a[4] = 0.75887003;
  a[5] = 1;
  a[6] = -0.6922953;
  a[7] = 0;
  a[8] = 0;
  a[9] = 0.035888535;
}

void reverb1me(float *in, float *out, int inFrames, int out_frames, int nchans, 
	       int channel, float revtime, float dry, t_bashfest *x)
{
  float dels[4];// stick into main structure
  float **alpo = x->mini_delay ;
  float a1,a2,a3,a4;
  int i;
//  int alsmp ;
  float *fltdata = x->reverb_ellipse_data;

  int nsects;
  float xnorm;
  LSTRUCT *eel = x->eel;

  float wet;
//  float max;
  float srate = x->sr;
//  float max_del = x->max_mini_delay ;

  wet = cos(1.570796 * dry);
  dry = sin(1.570796 * dry);

  /* combset uses reverb time , mycombset uses feedback */
  for( i = 0; i < 4; i++ ){
    dels[i] = boundrand(.005, .1 );
	if(dels[i] < .005 || dels[i] > 0.1) {
		post("reverb1: bad random delay time: %f",dels[i]);
		dels[i] = .05;
	}
    mycombset(dels[i], revtime, 0, alpo[i], srate);
  }

  ellipset(fltdata,eel,&nsects,&xnorm); 

  for( i = channel ; i < inFrames * nchans; i += nchans ){

    a1 = allpass(in[i], alpo[0]);
    a2 = allpass(in[i], alpo[1]);
    a3 = allpass(in[i], alpo[2]);
    a4 = allpass(in[i], alpo[3]); 
    

    out[i] = in[i] * dry + ellipse((a1+a2+a3+a4), eel, nsects,xnorm) * wet;
  }

  for( i = channel + inFrames * nchans; i < out_frames * nchans; i += nchans ){

    a1 = allpass(0.0, alpo[0]);
    a2 = allpass(0.0, alpo[1]);
    a3 = allpass(0.0, alpo[2]);
    a4 = allpass(0.0, alpo[3]); 

    out[i] =  ellipse((a1+a2+a3+a4), eel, nsects,xnorm) * wet;

  }

}

void feed1(float *inbuf, float *outbuf, int in_frames, int out_frames,int channels, float *functab1,
	   float *functab2,float *functab3,float *functab4,int funclen, 
	   float duration, float maxDelay, t_bashfest *x)
{
  int i;
  float srate = x->sr;
  float *delayLine1a = x->mini_delay[0];
  float *delayLine2a = x->mini_delay[1];
  float *delayLine1b = x->mini_delay[2];
  float *delayLine2b = x->mini_delay[3];
  int dv1a[2], dv2a[2];		/* cmix bookkeeping */
  int dv1b[2], dv2b[2];		/* cmix bookkeeping */
  float delsamp1a=0, delsamp2a=0 ;
  float delsamp1b=0, delsamp2b=0 ;
  float delay1, delay2, feedback1, feedback2;
  float funcSi, funcPhs;
  float putsamp;

  /***************************/

  funcPhs = 0.;

  // read once during note


  funcSi = ((float) funclen / srate) / duration ;


  delset2(delayLine1a, dv1a, maxDelay,srate);
  delset2(delayLine2a, dv2a, maxDelay,srate);

  if( channels == 2 ){
    delset2(delayLine1b, dv1b, maxDelay,srate);
    delset2(delayLine2b, dv2b, maxDelay,srate);
  }


  for(i = 0; i < out_frames*channels; i += channels ){
    // buffer loop 

    delay1 = functab1[ (int) funcPhs ];
    delay2 = functab2[ (int) funcPhs ];
    feedback1 = functab3[ (int) funcPhs ];
    feedback2 = functab4[ (int) funcPhs ];

    funcPhs += funcSi;
    if( funcPhs >= (float) funclen )
      funcPhs = 0;

    putsamp = i < in_frames * channels ? inbuf[i] + delsamp1a*feedback1 : 0.0;
	outbuf[i] = putsamp; // zero instead ??
    
    delput2( putsamp, delayLine1a, dv1a);
    delsamp1a = dliget2(delayLine1a, delay1, dv1a,srate);

    putsamp = delsamp1a+delsamp2a*feedback2 ;

    delput2( putsamp, delayLine2a, dv2a);
	delsamp2a = dliget2(delayLine2a, delay2, dv2a, srate);
    outbuf[i] += delsamp2a;
    

    if( channels == 2 ){
      putsamp = i < in_frames * channels ? inbuf[i+1] + delsamp1a*feedback1 : 0.0;
	  outbuf[i+1] = putsamp;
      delput2( putsamp, delayLine1b, dv1b);
      delsamp1b = dliget2(delayLine1b, delay1, dv1b, srate);
      putsamp = delsamp1b+delsamp2b*feedback2;
      delput2( putsamp, delayLine2b, dv2b);
	  delsamp2b = dliget2(delayLine2b, delay2, dv2b, srate);
      outbuf[i+1] += delsamp2b;
    }

  }

}

void setflamfunc1(float *arr, int flen)
{
  int i;
  float x;
  for ( i = 0; i < flen; i++){
    x = (float)i / (float) flen ;
    *(arr + i) = ((x - 1) / (x + 1)) * -1.  ;

  }
}


void setExpFlamFunc(float *arr, int flen, float v1,float v2,float alpha)
{
  int i;

  if( alpha == 0 )
    alpha = .00000001 ;

  for ( i = 0; i < flen; i++){
    *(arr + i) = v1 + (v2-v1) * ((1-exp((float)i*alpha/((float)flen-1.)))/(1-exp(alpha)));
  }
}

void funcgen1(float *outArray, int outlen, float duration, float outMin, float outMax,
	 float speed1, float speed2, float gain1, float gain2, float *phs1, float *phs2, 
	 float *sine, int sinelen)
{
  float si1, si2;
  float localSR;
  int i;

  localSR = duration * (float) outlen ;
  *phs1 *= (float) sinelen;
  *phs2 *= (float) sinelen;
  si1 = ((float)sinelen/localSR)  * speed1;
  si2 = ((float)sinelen/localSR)  * speed2;

  for( i = 0; i < outlen; i++ ){
    *(outArray + i) = oscil(gain1, si1, sine, sinelen, phs1) ;
    *(outArray + i) += oscil(gain2, si2, sine, sinelen, phs2) ;
  }
  normtab( outArray, outArray, outMin, outMax, outlen);

}


void normtab(float *inarr,float *outarr, float min, float max, int len)
{
  int i;

  float imin=9999999999., imax=-9999999999.;

  for(i = 0; i < len ; i++){
    if( imin > inarr[i] ) 
      imin = inarr[i];
    if( imax < inarr[i] ) 
      imax = inarr[i];
  }
  for(i = 0; i < len; i++ )
    outarr[i] = mapp(inarr[i], imin, imax, min, max);
  
}

float mapp(float in,float imin,float imax,float omin,float omax)
{
  if( imax == 0.0 )
    {
      return 0.0 ;
    }
  return( omin+((omax-omin)*((in-imin)/(imax-imin))) );
}

float oscil(float amp,float si,float *farray,int len,float *phs)
{
  register int i =  *phs;   
  *phs += si;            
  while(*phs >= len)
    *phs -= len;     
  return(*(farray+i) * amp);
}

void killdc( float *inbuf, int in_frames, int channels, t_bashfest *x)
{
  int i,j=1;
  LSTRUCT *eel = x->eel;
  int nsects;
  float xnorm;
  float *dcflt = x->dcflt;
  
  /* float dcflt[64] =
     {3, -1.9999924    , -1.9992482    ,  1.0000000 
     ,  .99928019    ,
     -1.9999956    , -1.9964080    ,  1.0000000    ,  .99645999    ,
     -1.9999994    , -1.9805074    ,  1.0000000    ,  .98069401    ,
     .98817413E+00};*/

  for( j = 0; j < channels; j++) {
    ellipset(dcflt,eel,&nsects,&xnorm); 
    
    for( i = j; i < in_frames * channels ; i += channels ){
      inbuf[i] = ellipse(inbuf[i], eel, nsects,xnorm);
    }
  }
}

void set_dcflt(float *a)
{
  a[0] = 3;
  a[1] = -1.9999924;
  a[2] = -1.9992482;
  a[3] = 1;
  a[4] = 0.99928019;
  a[5] = -1.9999956;
  a[6] = -1.996408;
  a[7] = 1;
  a[8] = 0.99645999;
  a[9] = -1.9999994;
  a[10] = -1.9805074;
  a[11] = 1;
  a[12] = 0.98069401;
  a[13] = 0.98817413;
}

void set_distortion_table(float *arr, float cut, float max, int len)
{
  int i, len2;
  float samp;

  len2 = len>>1 ;
  for( i = len2; i < len; i++ ){
    samp = (float)(i - len2) / (float) len2 ; 
    if( samp > cut )
      samp = mapp( samp, cut, 1.0,  cut, max );
    *(arr + i) = samp;
  }
  for( i = 0; i < len2; i++ )
    *(arr + i) = - *(arr + len - (i+1));
}

float dlookup(float samp,float *arr,int len) 
{
  return arr[(int) (((samp+1.0)/2.0) * (float) len)];

}

void do_compdist(float *in,float *out,int sampFrames,int nchans,int channel, 
	    float cutoff,float maxmult,int lookupflag,float *table,int range,float bufMaxamp)
{

  int i;

  float rectsamp;

  for( i = channel ; i < sampFrames * nchans; i+= nchans )
    {
	
      if( lookupflag){
	*(out + i) = dlookup( *(in + i)/bufMaxamp, table, range );
      } else {
	rectsamp = fabs( *(in + i) ) / bufMaxamp;
	if( rectsamp > cutoff ){
	  *(in + i) = *(out + i) * 
	    mapp( rectsamp, cutoff, 1.0, cutoff, maxmult);
	}
      }
    }
}

float getmaxamp(float *arr, int len) 
{
  int i;
  float max = 0;

  for(i = 0; i < len; i++ ){
    if( fabs(arr[i]) > max )
      max = fabs(arr[i]);
  }
  return max;
}

void buildadsr(CMIXADSR *a)
{
  float A = a->a;
  float D = a->d;
  float S = a->s;
  float R = a->r;
  float f1 = a->v1;
  float f2 = a->v2;
  float f3 = a->v3;
  float f4 = a->v4;

  int funclen = a->len;
  float *func = a->func;
  float total;
  int ipoint = 0;
  int i;
  int segs[4];
  float m1,m2;
  total = A + D + S + R ;

  segs[0] = (A/total) * funclen;
  segs[1] = (D/total) * funclen;
  segs[2] = (S/total) * funclen;
  segs[3] = funclen - (segs[0]+segs[1]+segs[2]);

  if( f1 > 20000. || f1 < -20000. ){
    f1 = 250.0;
  }
  if( f2 > 20000. || f2 < -20000. ){
    f2 = 1250.0;
  }
  if( f3 > 20000. || f3 < -20000. ){
    f3 = 950.0;
  }
  if( f4 > 20000. || f4 < -20000. ){
    f4 = f1;
  }

  if( segs[0] <= 0 || segs[1] <= 0 || segs[2] <= 0 || segs[3] <= 0 ){

    for( i = 0; i < 4; i++ ){
      segs[i] = funclen / 4;
    }
  }

  for( i = 0 ; i < segs[0]; i++ ){
    m1 = 1.-(float)i/(float)(segs[0]);
    m2 = 1. - m1;
    *(func +i ) = f1 * m1 + f2 * m2;
  }
  ipoint = i;

  for( i = 0 ; i < segs[1]; i++ ){
    m1 = 1.-(float)i/(float)(segs[1]);
    m2 = 1. - m1;
    *(func + i + ipoint) = f2 * m1 + f3 * m2;
  }
  ipoint += i;

  for( i = 0 ; i < segs[2]; i++ ){
    m1 = 1.-(float)i/(float)(segs[2]);
    m2 = 1. - m1;
    *(func + i + ipoint) = f3;
  }
  ipoint += i;

  for( i = 0 ; i < segs[3]; i++ ){
    m1 = 1.-(float)i/(float)(segs[3]);
    m2 = 1. - m1;
    *(func + ipoint + i) = f3 * m1 + f4 * m2;
  }
  ipoint += i;

}

