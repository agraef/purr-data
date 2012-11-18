/////////////////////////////////////////////////////////////////////////////
//
// class Neuron
//
//   source file
//
//   Copyright (c) 2005 Georg Holzmann <grh@gmx.at>
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your option) any later version. 
//
/////////////////////////////////////////////////////////////////////////////

#include "Neuron.h"

namespace TheBrain
{

//--------------------------------------------------
/* Constructor
 */
Neuron::Neuron(int inputs, int dummy)
  : learn_rate_(0), range_(1), IW_(NULL), b1_(0)
{
  inputs_ = (inputs<1) ? 1 : inputs;
}

//--------------------------------------------------
/* Destructor
 */
Neuron::~Neuron()
{
  if(IW_)
    delete[] IW_;
}

//--------------------------------------------------
/* creates a new IW-matrix (size: inputs_) and 
 * b1-vector
 * ATTENTION: if they exist they'll be deleted
 */
void Neuron::create()
  throw(NNExcept)
{
  // delete if they exist
  if(IW_)
    delete[] IW_;

  IW_ = new float[inputs_];
  if(!IW_)
    throw NNExcept("No memory for Neurons!");
}

//--------------------------------------------------
/* inits the weight matrix and the bias vector of
 * the network with random values between [min|max]
 */
void Neuron::initRand(const int &min, const int &max)
  throw(NNExcept)
{
  if(!IW_)
    throw NNExcept("You must first create the Net!");

  // make randomvalue between 0 and 1
  // then map it to the bounds
  b1_ = ((float)rand()/(float)RAND_MAX)*(max-min) + min;

  for(int i=0; i<inputs_; i++)
    {
      IW_[i] = ((float)rand()/(float)RAND_MAX)*(max-min) + min;
    }

  //post("b1: %f, IW: %f %f %f", b1_, IW_[0], IW_[1], IW_[2]);
}

//--------------------------------------------------
/* inits the net with a given weight matrix and bias
 * (makes a deep copy)
 * ATTENTION: the dimension of IW-pointer must be the same
 *            as the inputs !!!
 */
void Neuron::init(const float *IW, float b1)
  throw(NNExcept)
{
  if(!IW_)
    throw NNExcept("You must first create the Net!");

  b1_ = b1;

  for(int i=0; i<inputs_; i++)
      IW_[i] = IW[i];
}

//--------------------------------------------------
/* calculates the output with the current IW, b1 values
 * ATTENTION: the array input_data must be in the same
 *            size as inputs_
 */
float Neuron::calculate(float *input_data)
{
  float output = 0;

  // multiply the inputs with the weight matrix IW
  // and add the bias vector b1
  for(int i=0; i<inputs_; i++)
    {
      output += input_data[i] * IW_[i];
    }

  // map input values to the range
  output /= range_; 
  
  //post("b1: %f, IW: %f %f %f", b1_, IW_[0], IW_[1], IW_[2]);
  //post("range: %f, in: %f %f %f, out: %f",range_,input_data[0],
  //     input_data[1], input_data[2], output+b1_);

  return (output+b1_);
}

//--------------------------------------------------
/* this method trains the network:
 * input_data is, as above, the input data, output_data is the 
 * output of the current net with input_data (output_data is not
 * calculated in that method !), target_output is the desired
 * output data
 * (this is the LMS-algorithm to train linear neural networks)
 * ATTENTION: the array input_data must be in the same
 *            size as inputs_
 * returns the calculated value
 */
// float Neuron::trainLMS(const float *input_data, 
// 		       const float &target_output)
// {
//   float output = 0;

//   // multiply the inputs with the weight matrix IW
//   // and add the bias vector b1
//   for(int i=0; i<inputs_; i++)
//     {
//       output += input_data[i] * IW_[i];
//     }

//   // map input values to the range
//   output /= range_; 

//   output += b1_;

//   //------------

//   // this is the LMS-algorithm to train linear
//   // neural networks
  
//   // calculate the error signal:
//   float error = (target_output - output);

//   // now change the weights the bias
//   for(int i=0; i<inputs_; i++)
//     IW_[i] += 2 * learn_rate_ * error * (input_data[i]/range_);

//   b1_ += 2 * learn_rate_ * error; 

//   //------------

//   return (output);
// }

} // end of namespace
