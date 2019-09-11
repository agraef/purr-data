#include "PenroseRand.h"

float fftease_rrand(int *seed)
{
    int i = ((*seed = *seed * 1103515245 + 12345)>>16) & 077777;
    return((float)i/16384. - 1.);
}

float fftease_prand(int *seed)
{
    int i = ((*seed = *seed * 1103515245 + 12345)>>16) & 077777;
    return((float)i/32768.);
}
