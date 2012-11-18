/////////////////////////////////////////////////////////////////////////////
//
// class LinNeuralNet
//
//   source file
//
//   Copyright (c) 2004 Georg Holzmann <grh@gmx.at>
//
//   For information on usage and redistribution, and for a DISCLAIMER OF ALL
//   WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////////////////////////

#include "LinNeuralNet.h"

//--------------------------------------------------
/* Constructor
 */
LinNeuralNet::LinNeuralNet(int netsize) : learn_rate_(0), range_(1), IW_(NULL), b1_(0)
{
  // set random seed:
  srand( (unsigned)time(NULL) );

  netsize_ = (netsize<1) ? 1 : netsize;
}

//--------------------------------------------------
/* Destructor
 */
LinNeuralNet::~LinNeuralNet()
{
  if(IW_)
    delete[] IW_;
}

//--------------------------------------------------
/* creates a new IW-matrix (size: netsize_) and 
 * b1-vector
 * ATTENTION: if they exist they'll be deleted
 */
bool LinNeuralNet::createNeurons()
{
  // delete if they exist
  if(IW_)
    delete[] IW_;

  IW_ = new float[netsize_];
  if(!IW_)
    return false;

  return true;
}

//--------------------------------------------------
/* inits the weight matrix and the bias vector of
 * the network with random values between [min|max]
 */
bool LinNeuralNet::initNetworkRand(const int &min, const int &max)
{
  if(!IW_)
    return false;

  // make randomvalue between 0 and 1
  // then map it to the bounds
  b1_ = ((float)rand()/(float)RAND_MAX)*(max-min) + min;

  for(int i=0; i<netsize_; i++)
    {
      IW_[i] = ((float)rand()/(float)RAND_MAX)*(max-min) + min;
    }

  return true;
}

//--------------------------------------------------
/* inits the net with a given weight matrix and bias
 * (makes a deep copy)
 * ATTENTION: the dimension of IW-pointer must be the same
 *            as the netsize !!!
 * returns false if there's a failure
 */
bool LinNeuralNet::initNetwork(const float *IW, float b1)
{
  if(!IW_)
    return false;

  b1_ = b1;

  for(int i=0; i<netsize_; i++)
      IW_[i] = IW[i];

  return true;
}

//--------------------------------------------------
/* calculates the output with the current IW, b1 values
 * ATTENTION: the array input_data must be in the same
 *            size as netsize_
 */
float LinNeuralNet::calculateNet(float *input_data)
{
  if(!IW_)
    return 0;

  float output = 0;

  // multiply the inputs with the weight matrix IW
  // and add the bias vector b1
  for(int i=0; i<netsize_; i++)
      output += input_data[i] * IW_[i];
  
  // map input values to the range
  output /= range_;
  
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
 *            size as netsize_
 */
bool LinNeuralNet::trainNet(float *input_data, const float &output_data, 
			    const float &target_output)
{
  if(!IW_)
    return false;

  // this is the LMS-algorithm to train linear
  // neural networks
  
  // calculate the error signal:
  float error = (target_output - output_data);

  // now change the weights the bias
  for(int i=0; i<netsize_; i++)
    IW_[i] += 2 * learn_rate_ * error * (input_data[i]/range_);

  b1_ += 2 * learn_rate_ * error; 

  return true;
}
