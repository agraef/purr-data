
float frequencyToIncrement( float samplingRate, float frequency,
			    int bufferLength );

void makeSineBuffer( float *buffer, int bufferLength );

float bufferOscil( float *phase, float increment, float *buffer,
                   int bufferLength );
