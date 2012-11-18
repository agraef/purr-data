/////////////////////////////////////////////////////////////////////////////
//
// class RecurrentNeuron
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

#include "RecurrentNeuron.h"

namespace TheBrain
{

//--------------------------------------------------
/* Constructor
 */
RecurrentNeuron::RecurrentNeuron(int inputs, int memory)
  : Neuron(inputs), LW_(NULL), mem_data_(NULL)
{
  memory_ = (memory<0) ? 1 : memory+1;
}

//--------------------------------------------------
/* Destructor
 */
RecurrentNeuron::~RecurrentNeuron()
{
   if(LW_)
     delete[] LW_;

   if(mem_data_)
     delete[] mem_data_;
}

//--------------------------------------------------
/* creates a new IW-matrix (size: inputs_) and 
 * b1-vector
 * ATTENTION: if they exist they'll be deleted
 */
void RecurrentNeuron::create()
  throw(NNExcept)
{
  // delete if they exist
  if(IW_)
    delete[] IW_;
  if(LW_)
    delete[] LW_;
  if(mem_data_)
    delete[] mem_data_;

  IW_ = new float[inputs_];
  LW_ = new float[memory_];
  mem_data_ = new float[memory_];

  if(!IW_ || !LW_ || !mem_data_)
    throw NNExcept("No memory for Neurons!");

  index_=0;
}

//--------------------------------------------------
/* inits the weight matrix and the bias vector of
 * the network with random values between [min|max]
 */
void RecurrentNeuron::initRand(const int &min, const int &max)
  throw(NNExcept)
{
  if(!IW_ || !LW_)
    throw NNExcept("You must first create the Net!");

  // make randomvalue between 0 and 1
  // then map it to the bounds
  b1_ = ((float)rand()/(float)RAND_MAX)*(max-min) + min;

  for(int i=0; i<inputs_; i++)
    {
      IW_[i] = ((float)rand()/(float)RAND_MAX)*(max-min) + min;
    }
  for(int i=0; i<memory_; i++)
    {
      //LW_[i] = ((float)rand()/(float)RAND_MAX)*(max-min) + min;
      LW_[i] = ((float)rand()/(float)RAND_MAX)*(min);
    }
}

//--------------------------------------------------
/* inits the net with given weight matrix and bias
 * (makes a deep copy)
 * ATTENTION: the dimension of IW-pointer must be the same
 *            as the inputs (also for LW) !!!
 */
void RecurrentNeuron::init(const float *IW, const float *LW, float b1)
  throw(NNExcept)
{
  if(!IW_ || !LW_)
    throw NNExcept("You must first create the Net!");

  b1_ = b1;

  for(int i=0; i<inputs_; i++)
      IW_[i] = IW[i];
  for(int i=0; i<memory_; i++)
      LW_[i] = LW[i];
}

//--------------------------------------------------
/* calculates the output with the current IW, b1 values
 * ATTENTION: the array input_data must be in the same
 *            size as inputs_
 */
float RecurrentNeuron::calculate(float *input_data)
{
  float output = 0;

  // multiply the inputs with the weight matrix IW
  for(int i=0; i<inputs_; i++)
    {
      output += input_data[i] * IW_[i];
    }

  // map input values to the range
  output /= range_; 

  // multiply memory with weight matrix LW
  // the index is used to make something
  // like a simple list or ringbuffer
  for(int i=0; i<memory_; i++)
    {
      output += mem_data_[index_] * LW_[i];
      index_ = (index_+1) % memory_;
    }

  // now add bias
  output += b1_;

  // finally save the new output in memory
  mem_data_[index_] = output;
  index_ = (index_+1) % memory_;

  //post("input: %f %f %f, IW: %f %f %f, b: %f",
  //      input_data[0], input_data[1], input_data[2],
  //      IW_[0], IW_[1], IW_[2], b1_);
  //post("output: %f",output);

  return (output);
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
 * returns the calculated output
 */
// float RecurrentNeuron::trainLMS(const float *input_data, 
// 				const float &target_output)
// {
//   // calculate output value:

//   float output = 0;

//   // multiply the inputs with the weight matrix IW
//   for(int i=0; i<inputs_; i++)
//     {
//       output += input_data[i] * IW_[i];
//     }

//   // map input values to the range
//   output /= range_; 

//   // multiply memory with weight matrix LW
//   // the index is used to make something
//   // like a simple list or ringbuffer
//   for(int i=0; i<memory_; i++)
//     {
//       output += mem_data_[index_] * LW_[i];
//       index_ = (index_+1) % memory_;
//     }

//   // now add bias
//   output += b1_;

//   //----------------

//   // this is the LMS-algorithm to train linear
//   // neural networks

//   // calculate the error signal:
//   float error = (target_output - output);

//   // now change IW
//   for(int i=0; i<inputs_; i++)
//     IW_[i] += 2 * learn_rate_ * error * (input_data[i]/range_);

//   // change LW
//   for(int i=0; i<memory_; i++)
//     {
//       LW_[i] += 2 * learn_rate_ * error * mem_data_[index_];
//       index_ = (index_+1) % memory_;     
//     }

//   // and the bias
//   b1_ += 2 * learn_rate_ * error; 

//   //-----------------

//   // finally save the new output in memory
//   mem_data_[index_] = output;
//   index_ = (index_+1) % memory_;

//   return (output);
// }


} // end of namespace
