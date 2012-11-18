/////////////////////////////////////////////////////////////////////////////
//
// class RecurrentNeuron
//
//   this is an implementation of one neuron of a Recurrent Neural Network
//   this neuron can have n input values, m values in it's memory and
//   one output value
//   (see NeuralNet documentations for more information)
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


#ifndef _INCLUDE_RECURRENT_NEURON_NET__
#define _INCLUDE_RECURRENT_NEURON_NET__

#include <stdlib.h>
#include <stdexcept>
#include "Neuron.h"

namespace TheBrain
{

//------------------------------------------------------
/* class of one neuron
 */
class RecurrentNeuron : public Neuron
{
 protected:

  /* this determines how much output values the net
   * can remeber
   * these values are fed back as new input
   */
  int memory_;

  /* the weight matrix for the recurrent 
   * values (size: memory_)
   */
  float *LW_;


 public:

  /* Constructor
   */
  RecurrentNeuron(int inputs, int memory);

  /* Destructor
   */
  virtual ~RecurrentNeuron();


  //-----------------------------------------------------
  /* some more get/set methods
   */

  virtual int getMemory() const
  {  return memory_; }

  virtual float *getLW() const
  {  return LW_; }
  virtual float getLW(int index) const
  {  return LW_[index]; }

  virtual void setLW(const float *LW)
  {  for(int i=0; i<inputs_; i++) LW_[i] = LW[i]; }
  virtual void setLW(int index, float value)
  {  LW_[index] = value; }


  //-----------------------------------------------------

  /* creates a new IW-matrix (size: inputs_) and 
   * b1-vector
   * ATTENTION: if they exist they'll be deleted
   */
  virtual void create()
    throw(NNExcept);

  /* inits the weight matrix and the bias vector of
   * the network with random values between [min|max]
   */
  virtual void initRand(const int &min, const int &max)
    throw(NNExcept);

  /* inits the net with given weight matrix and bias
   * (makes a deep copy)
   * ATTENTION: the dimension of IW-pointer must be the same
   *            as the inputs (also for LW) !!!
   */
  virtual void init(const float *IW, const float *LW, float b1)
    throw(NNExcept);

  /* calculates the output with the current IW, b1 values
   * ATTENTION: the array input_data must be in the same
   *            size as inputs_
   */
  virtual float calculate(float *input_data);

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
/*   virtual float trainLMS(const float *input_data,  */
/* 			 const float &target_output); */


  //-----------------------------------------------------
 private:

  /* the storage for the memory data
   */
  float *mem_data_;

  /* this index is used to make something
   * like a simple list or ringbuffer
   */
  int index_;

  /* Copy Construction is not allowed
   */
  RecurrentNeuron(const RecurrentNeuron &src) : Neuron(1)
    { }

  /* assignement operator is not allowed
   */
    const RecurrentNeuron& operator= (const RecurrentNeuron& src)
    { return *this; }
};


} // end of namespace

#endif //_INCLUDE_RECURRENT_NEURON_NET__
