/////////////////////////////////////////////////////////////////////////////
//
// class Neuron
//
//   this is an implementation of one neuron of a Neural Network
//   so this neuron has a Weight-Matrix IW and a bias vector b1
//   this neuron can have n input values, but only one output value
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


#ifndef _INCLUDE_NEURON_NET__
#define _INCLUDE_NEURON_NET__

#include <stdlib.h>
#include <stdexcept>
#include "NNException.h"
#include "m_pd.h" //debug

namespace TheBrain
{

//------------------------------------------------------
/* class of one neuron
 */
class Neuron
{
 protected:

  /* this is the number of input values, which is
   * automatically the input and the size of IW
   */
  int inputs_;

  /* the input weight matrix IW
   * (size: inputs )
   */
  float *IW_;

  /* the bias vector b1
   */
  float b1_;

  /* the learning rate of the net
   */
  float learn_rate_;

  /* the range of the input values should be from 0
   * to range_
   * outputvalues are from -1 to 1
   */
  float range_;


 public:

  /* Constructor
   */
  Neuron(int inputs, int dummy=0);

  /* Destructor
   */
  virtual ~Neuron();


  //-----------------------------------------------------

  /* Set/Get learning rate
   */
  virtual void setLearningRate(float learn_rate)
  {  learn_rate_=learn_rate; }
  virtual float getLearningRate() const
  {  return learn_rate_; }

  /* Set/Get range
   */
  virtual void setRange(float range)
  {  range_=range; }
  virtual float getRange() const
  {  return range_; }

  /* some more get/set methods
   */

  virtual int getInputs() const
  {  return inputs_; }

  virtual float *getIW() const
  {  return IW_; }
  virtual float getIW(int index) const
  {  return IW_[index]; }
  
  virtual void setIW(const float *IW)
  {  for(int i=0; i<inputs_; i++) IW_[i] = IW[i]; }
  virtual void setIW(int index, float value)
  {  IW_[index] = value; }
  
  virtual float getb1() const
  {  return b1_; }
  virtual void setb1(float b1)
  {  b1_ = b1; }


  /* dummies
   */

  virtual int getMemory() const
  {  return 0; }

  virtual float *getLW() const
  {  return NULL; }
  virtual float getLW(int index) const
  {  return 0; }

  virtual void setLW(const float *LW)
  {   }
  virtual void setLW(int index, float value)
  {   }


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

  /* inits the net with a given weight matrix and bias
   * (makes a deep copy)
   * ATTENTION: the dimension of IW-pointer must be the same
   *            as the inputs !!!
   */
  virtual void init(const float *IW, float b1)
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
   * returns the calculated value
   */
/*   virtual float trainLMS(const float *input_data,  */
/* 			 const float &target_output); */


  //-----------------------------------------------------
 private:

  /* Copy Construction is not allowed
   */
  Neuron(const Neuron &src)
    { }

  /* assignement operator is not allowed
   */
  const Neuron& operator= (const Neuron& src)
    { return *this; }
};


} // end of namespace

#endif //_INCLUDE_NEURON_NET__
