/////////////////////////////////////////////////////////////////////////////
//
// class LinNeuralNet
//
//   this is an implementation of a simple linear neural net with one neuron
//   so this net has a Weight-Matrix IW and a bias vector b1
//   this net can have n input values, but only one output value
//   (see NeuralNet documentations for more information)
//
//   header file
//
//   Copyright (c) 2004 Georg Holzmann <grh@gmx.at>
//
//   For information on usage and redistribution, and for a DISCLAIMER OF ALL
//   WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _INCLUDE_LIN_NEURAL_NET__
#define _INCLUDE_LIN_NEURAL_NET__

#include <stdlib.h>
#include <ctime>
//#include "m_pd.h"   // for debug

class LinNeuralNet
{
 protected:

  /* this is the number of input values, which is
   * automatically the netsize and the size of IW
   */
  int netsize_;

  /* the input weight matrix IW
   * (size: netsize )
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
  LinNeuralNet(int netsize);

  /* Destructor
   */
  virtual ~LinNeuralNet();


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
  virtual int getNetsize() const
  {  return netsize_; }
  virtual float *getIW() const
  {  return IW_; }
  virtual void setIW(const float *IW)
  {  for(int i=0; i<netsize_; i++) IW_[i] = IW[i]; }
  virtual float getb1() const
  {  return b1_; }
  virtual void setb1(float b1)
  {  b1_ = b1; }


  //-----------------------------------------------------

  /* creates a new IW-matrix (size: netsize_) and 
   * b1-vector
   * returns false if there's a failure
   * ATTENTION: if they exist they'll be deleted
   */
  virtual bool createNeurons();

  /* inits the weight matrix and the bias vector of
   * the network with random values between [min|max]
   * returns false if there's a failure
   */
  virtual bool initNetworkRand(const int &min, const int &max);

  /* inits the net with a given weight matrix and bias
   * (makes a deep copy)
   * ATTENTION: the dimension of IW-pointer must be the same
   *            as the netsize !!!
   * returns false if there's a failure
   */
  virtual bool initNetwork(const float *IW, float b1);

  /* calculates the output with the current IW, b1 values
   * ATTENTION: the array input_data must be in the same
   *            size as netsize_
   */
  virtual float calculateNet(float *input_data);

  /* this method trains the network:
   * input_data is, as above, the input data, output_data is the 
   * output of the current net with input_data (output_data is not
   * calculated in that method !), target_output is the desired
   * output data
   * (this is the LMS-algorithm to train linear neural networks)
   * returns false if there's a failure
   * ATTENTION: the array input_data must be in the same
   *            size as netsize_
   */
  virtual bool trainNet(float *input_data, const float &output_data, 
			const float &target_output);

 private:
  /* Copy Construction is not allowed
   */
  LinNeuralNet(const LinNeuralNet &src)
    { }

  /* assignement operator is not allowed
   */
  const LinNeuralNet& operator= (const LinNeuralNet& src)
    { return *this; }
};




#endif //_INCLUDE_LIN_NEURAL_NET__
