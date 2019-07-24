#include "fftease.h"

t_float frequencyToIncrement( t_float samplingRate, t_float frequency, int bufferLength );
void makeSineBuffer( t_float *buffer, int bufferLength );
t_float bufferOscil( t_float *phase, t_float increment, t_float *buffer, int bufferLength );
