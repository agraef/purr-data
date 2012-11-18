/////////////////////////////////////////////////////////////////////////////
//
// NNActivation.h
//
//   all the activation functions of the neurons
//
//   header file
//
//   Copyright (c) 2005 Georg Holzmann <grh@gmx.at>
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your option) any later version.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _INCLUDE_ACTIVATION_NET__
#define _INCLUDE_ACTIVATION_NET__


#include <math.h>

namespace TheBrain
{

//------------------------------------------------------
/* implementation of the different activation functions
 * and it's derivations
 */

/* Linear activation function.
 * span: -inf < y < inf
 * y = x
*/
#define LINEAR 0

/* Sigmoid activation function.
 * span: 0 < y < 1
 * y = 1/(1 + exp(-x)), y' = y*(1 - y)
 */
#define SIGMOID 1

/* Symmetric sigmoid activation function, aka. tanh.
 * span: -1 < y < 1
 * y = tanh(x) = 2/(1 + exp(-2*x)) - 1, d = 1-(y*y)
*/
#define TANH 2

// linear function
float act_linear(float value)
{ return value; }

// derivation of the linear function
float act_linear_derive(float value)
{ return 1; }

// sigmoid function
float act_sigmoid(float value)
{ return (1.0f/(1.0f + exp(-value))); }

// derivation of the sigmoid function
float act_sigmoid_derive(float value)
{ return (value * (1.0f - value)); }

// tanh function
float act_tanh(float value)
{ return (2.0f/(1.0f + exp(-2.0f * value)) - 1.0f); }

// derivation of the tanh function
float act_tanh_derive(float value)
{ return (1.0f - (value*value)); }


} // end of namespace 

#endif // _INCLUDE_ACTIVATION_NET__
