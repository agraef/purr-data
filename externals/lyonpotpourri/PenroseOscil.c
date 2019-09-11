#include <math.h>
#include "PenroseOscil.h"


float frequencyToIncrement( float samplingRate, float frequency, int bufferLength ) {

  return (frequency / samplingRate) * (float) bufferLength;
} 

void makeSineBuffer( float *buffer, int bufferLength ) {
  
  int   i;

  float myTwoPi = 8. * atan(1.);

  for ( i=0; i <= bufferLength; i++ )
    *(buffer+i) = sin( myTwoPi * ((float) i / (float) bufferLength) );

  return;
}


float bufferOscil( float *phase, float increment, float *buffer,
                   int bufferLength )
{

  float sample;

  while ( *phase > bufferLength )
    *phase -= bufferLength;

  while ( *phase < 0. )
    *phase += bufferLength;

  sample = *( buffer + (int) (*phase) );

  *phase += increment;

  return sample;
}
