#include "../../creb/include/extlib_util.h"

float milliseconds_2_one_minus_realpole(float time)
{
  float r;

  if (time < 0.0f) time = 0.0f;
  r = -expm1(1000.0f * log(ENVELOPE_RANGE) / (sys_getsr() * time));
  if (!(r < 1.0f)) r = 1.0f;

  return r;
}
		  
