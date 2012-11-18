/////////////////////////////////////////////////////////////////////////////
//
// class NNet
//
//   this is a template for all the nets
//   (see NeuralNet documentations for more information)
//
//   header file
//
//   Copyright (c) 2005 Georg Holzmann <grh@gmx.at>
//
//   
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your option) any later version.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _INCLUDE_NEURAL_TEMPLATE_NET__
#define _INCLUDE_NEURAL_TEMPLATE_NET__

#include "NNActivation.h"
#include "NNException.h"

namespace TheBrain
{

template <class HiddNeuronType,class OutNeuronType>
class NNet
{
 protected:

  /* the number of output values
   * this is automatically also the 
   * number of output neurons !
   */
  int output_val_;

  /* the number of hidden neurons
   * per one output neuron
   * (this net has one hidden layer,
   * so this is the number of hidden
   * neurons is hidden_val_*output_val_) 
   */
  int hidden_val_;

  /* nr of input values per one output neuron
   * (so the number of input values are
   * input_val_*output_val_)
   */
  int input_val_;

  /* the memory of the output layer
   * if you use a recurrent neuron, this
   * determines how much output values the 
   * recurrent neurons can remeber
   * these values are fed back as new input
   */
  int memory_out_;

  /* the memory of the hidden layer
   * if you use a recurrent neuron, this
   * determines how much output values the 
   * recurrent neurons can remeber
   * these values are fed back as new input
   */
  int memory_hidden_;

  /* these are the output neurons
   */
  OutNeuronType *out_neurons_;

  /* these are the hidden neurons
   */
  HiddNeuronType *hidden_neurons_;

  /* function pointer to the activation
   * function of the output neurons
   */
  float (*output_act_f)(float value);

  /* function pointer to the activation
   * function of the hidden neurons
   */
  float (*hidden_act_f)(float value);

  /* function pointer to the derivation of the
   * activation function of the hidden neurons
   */
  float (*hidden_act_f_d)(float value);


 public:

  /* Constructor
   */
  NNet(int input_val=1, int hidden_val=1, int output_val=1, int memory_out=0,
       int memory_hidden=1, int HIDDEN_ACT_FUNC=0, int OUT_ACT_FUNC=0);

  /* Destructor
   */
  virtual ~NNet();


  //-----------------------------------------------------

  /* Set/Get learning rate
   */
  virtual void setLearningRate(float learn_rate);
  virtual float getLearningRate() const;

  /* Set/Get range
   * (see Neuron.h)
   */
  virtual void setRange(float range);
  virtual float getRange() const;

  /* some more get/set methods
   */
  virtual void setOutputVal(int output_val)
    throw();
  virtual int getOutputVal() const;

  virtual void setHiddenVal(int hidden_val)
    throw();
  virtual int getHiddenVal() const;

  virtual void setInputVal(int input_val)
    throw();
  virtual int getInputVal() const;

  virtual void setMemoryOut(int memory)
    throw();
  virtual int getMemoryOut() const;

  virtual void setMemoryHidden(int memory)
    throw();
  virtual int getMemoryHidden() const;


  //-----------------------------------------------------

  /* creates the network
   */
  virtual void create()
    throw(NNExcept);

  /* inits the weight matrix and the bias vector of
   * the network with random values between [min|max]
   */
  virtual void initRand(const int &min, const int &max)
    throw(NNExcept);

  /* calculates the output with the current Net and writes
   * it in the array output_data
   * ATTENTION: array input_data must be a matrix in the form:
   *              float[output_val_][input_val_]
   *            array output_data must be in size output_val_
   *            (there is no checking !!!)
   */
  virtual void calculate(float **input_data, float *output_data);

  /* this method trains the network:
   * input_data is, as above, the input data, output_data is the 
   * output of the current net with input_data, target_output is 
   * the desired output data
   * (this is the a truncated backpropagation through time
   * algorithm to train the network)
   * ATTENTION: array input_data must be a matrix in the form:
   *              float[output_val_][input_val_]
   *            array output_data must be in size output_val_
   *            array target_output  must be in size output_val_
   *            (there is no checking !!!)
   */
  virtual void trainBTT(float **input_data, float *output_data, 
		     float *target_output);


  //-----------------------------------------------------

  /* saves the contents of the current net to file
   */
  virtual void save(string filename)
    throw(NNExcept);

  /* loads the parameters of the net from file
   */
  virtual void load(string filename)
    throw(NNExcept);


  //-----------------------------------------------------
 private:

  /* output of the hidden layer with activation function
   */
  float *hidden_a_;

  /* output of the hidden layer without activation function
   */
  float *hidden_s_;

  /* error signal of the neurons in the hidden layer
   */
  float *hidden_error_;

  /* out signal without activation function
   */
  float out_s_;

  /* error signal of the output layer
   */
  float out_error_;

  /* Copy Construction is not allowed
   */
  NNet(const NNet<HiddNeuronType,OutNeuronType> &src)
    { }

  /* assignement operator is not allowed
   */
  const NNet<HiddNeuronType,OutNeuronType>& operator= 
    (const NNet<HiddNeuronType,OutNeuronType>& src)
    { return *this; }
};


//--------------------------------------------------
/* Constructor
 */
template <class HiddNeuronType, class OutNeuronType>
NNet<HiddNeuronType,OutNeuronType>
  ::NNet(int input_val, int hidden_val, int output_val, int memory_out, 
	 int memory_hidden, int HIDDEN_ACT_FUNC, int OUT_ACT_FUNC)
  : out_neurons_(NULL), hidden_neurons_(NULL), hidden_a_(NULL),
  hidden_s_(NULL), hidden_error_(NULL)
{
  output_val_ = (output_val<1) ? 1 : output_val;
  hidden_val_ = (hidden_val<0) ? 0 : hidden_val;
  input_val_ = (input_val<1) ? 1 : input_val;
  memory_out_ = (memory_out<0) ? 0 : memory_out;
  memory_hidden_ = (memory_hidden<0) ? 0 : memory_hidden;

  // choose hidden activation function:
  switch(HIDDEN_ACT_FUNC)
    {
    case SIGMOID:
      hidden_act_f = act_sigmoid;
      hidden_act_f_d = act_sigmoid_derive;
      break;
    case TANH:
      hidden_act_f = act_tanh;
      hidden_act_f_d = act_tanh_derive;
      break;
    default:
    case LINEAR:
      hidden_act_f = act_linear;
      hidden_act_f_d = act_linear_derive;
      break;
    }

  // choose out function:
  switch(OUT_ACT_FUNC)
    {
    case SIGMOID:
      output_act_f = act_sigmoid;
      break;
    case TANH:
      output_act_f = act_tanh;
      break;
    default:
    case LINEAR:
      output_act_f = act_linear;
      break;
    }
}

//--------------------------------------------------
/* Destructor
 */
template <class HiddNeuronType, class OutNeuronType>
NNet<HiddNeuronType, OutNeuronType>::~NNet()
{
  if(hidden_neurons_)
    delete[] hidden_neurons_;

  if(out_neurons_)
    delete[] out_neurons_;

  if(hidden_a_)
    delete[] hidden_a_;

  if(hidden_s_)
    delete[] hidden_s_;

  if(hidden_error_)
    delete[] hidden_error_;
}

//--------------------------------------------------
/* creates the network
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::create()
  throw(NNExcept)
{
  // delete if they exist
  if(out_neurons_)
    delete[] out_neurons_;
  if(hidden_neurons_)
    delete[] hidden_neurons_;
  if(hidden_a_)
    delete[] hidden_a_;
  if(hidden_s_)
    delete[] hidden_s_;
  if(hidden_error_)
    delete[] hidden_error_;


  out_neurons_ = new OutNeuronType[output_val_](input_val_,memory_out_);
  hidden_neurons_ = new HiddNeuronType[hidden_val_*output_val_](input_val_,memory_hidden_);

  if(!out_neurons_ || !hidden_neurons_)
    throw NNExcept("No memory for Neurons!");

  // create the temporary storage
  hidden_a_ = new float[hidden_val_];
  hidden_s_ = new float[hidden_val_];
  hidden_error_ = new float[hidden_val_];

  if(!hidden_a_ || !hidden_s_ || !hidden_error_)
    throw NNExcept("No memory for Neurons!");


  // create all the neurons
  for(int i=0; i<output_val_; i++)
    out_neurons_[i].create();
  for(int i=0; i<hidden_val_*output_val_; i++)
    hidden_neurons_[i].create();
}

//--------------------------------------------------
/* inits the weight matrix and the bias vector of
 * the network with random values between [min|max]
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::initRand(const int &min, const int &max)
    throw(NNExcept)
{
  if(!out_neurons_)
    throw NNExcept("You must first create the Net!");

  // init all the neurons
  for(int i=0; i<output_val_; i++)
    out_neurons_[i].initRand(min,max);
  for(int i=0; i<hidden_val_*output_val_; i++)
    hidden_neurons_[i].initRand(min,max);
}

//--------------------------------------------------
/* calculates the output with the current Net and writes
 * it in the array output_data
 * ATTENTION: array input_data must be a matrix in the form:
 *              float[output_val_][input_val_]
 *            array output_data must be in size output_val_
 *            (there is no checking !!!)
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::calculate(float **input_data, float *output_data)
{
  for(int i=0; i<output_val_; i++)
    {

      // 1.: calculation of the hidden layer
      for(int j=0; j<hidden_val_; j++)
	{
	  hidden_a_[j] = hidden_act_f( 
	    hidden_neurons_[i*hidden_val_+j].calculate(input_data[i]) );
	}

      // 2.: calculation of the output layer
      *output_data++ = output_act_f( out_neurons_[i].calculate(hidden_a_) );
    }
}

//--------------------------------------------------
/* this method trains the network:
 * input_data is, as above, the input data, output_data is the 
 * output of the current net with input_data, target_output is 
 * the desired output data
 * (this is the a truncated backpropagation through time
 * algorithm to train the network)
 * ATTENTION: array input_data must be a matrix in the form:
 *              float[output_val_][input_val_]
 *            array output_data must be in size output_val_
 *            array target_output  must be in size output_val_
 *            (there is no checking !!!)
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::trainBTT(float **input_data, float *output_data, 
			 float *target_output)
{
  post("train");

  for(int i=0; i<output_val_; i++)
    {


      //---------------------------------------------------------
      // 1. Forward - Pass:
      // 
      // the output of the hidden and the output-layer
      // are calculated and saved (before and after
      // the activation function)

      // calculation of the hidden layer
      for(int j=0; j<hidden_val_; j++)
	{
	  hidden_s_[j] = hidden_neurons_[i*hidden_val_+j].calculate(input_data[i]);
	  hidden_a_[j] = hidden_act_f(hidden_s_[j]);
	}

      // calculation of the output layer
      out_s_ = out_neurons_[i].calculate(hidden_a_);
      output_data[i] = output_act_f(out_s_);
 

      //---------------------------------------------------------
      // 2. Backward - Pass:
      // 
      // calculation of the error signals
      // (they are also stored)

      // output layer
      out_error_ = output_data[i] - target_output[i];
      
      // hidden layer:
      for(int j=0; j<hidden_val_; j++)
	{
	  hidden_error_[j] = hidden_act_f_d( hidden_s_[j]+0.1 ) *
	    ( out_error_ * out_neurons_[i].getIW(j) );
	}


      //---------------------------------------------------------
      // 3. Modification of the weights:

      for(int j=0; j<hidden_val_; j++)
      {
	// output layer:
	out_neurons_[i].setIW(j, 
	       out_neurons_[i].getIW(j) - 
	       getLearningRate() * out_error_ 
	       * hidden_a_[j] );

	// hidden layer:
	for(int k=0; k<input_val_; k++)
	{
	  hidden_neurons_[i*hidden_val_+j].setIW(k,
	       hidden_neurons_[i*hidden_val_+j].getIW(k) -
	       getLearningRate() * hidden_error_[j]
	       * input_data[i][k]/hidden_neurons_[0].getRange() );
	}


	// recurrent part of the hidden layer:
	float delta = getLearningRate() * hidden_error_[j] * hidden_a_[j];
	for(int k=0; k<memory_hidden_; k++)
	  {
	    hidden_neurons_[i*hidden_val_+j].setLW(k,
	       hidden_neurons_[i*hidden_val_+j].getLW(k) - delta);
	  }
      }

      // recurrent part of the output layer:
      float delta = getLearningRate() * out_error_ * output_data[i];
      for(int j=0; j<memory_out_; j++)
	{
	  out_neurons_[i].setLW(j,
	     out_neurons_[i].getLW(j) - delta);
	}
    

    }
}

//--------------------------------------------------
/* saves the contents of the current net to file
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::save(string filename)
  throw(NNExcept)
{

}

//--------------------------------------------------
  /* loads the parameters of the net from file
   */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::load(string filename)
  throw(NNExcept)
{

}

//-----------------------------------------------------
/* Set/Get learning rate
 * (see Neuron.h)
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::setLearningRate(float learn_rate)
{
  learn_rate = (learn_rate<0) ? 0 : learn_rate;

  for(int i=0; i<output_val_; i++)
    out_neurons_[i].setLearningRate(learn_rate);
  for(int i=0; i<hidden_val_*output_val_; i++)
    hidden_neurons_[i].setLearningRate(learn_rate);
}
template <class HiddNeuronType, class OutNeuronType>
float NNet<HiddNeuronType, OutNeuronType>::getLearningRate() const
{
  return out_neurons_[0].getLearningRate();
}

//-----------------------------------------------------
/* Set/Get range
 * (see Neuron.h)
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::setRange(float range)
{
  for(int i=0; i<output_val_; i++)
    out_neurons_[i].setRange(1);

  for(int i=0; i<hidden_val_*output_val_; i++)
    hidden_neurons_[i].setRange(range);
}
template <class HiddNeuronType, class OutNeuronType>
float NNet<HiddNeuronType, OutNeuronType>::getRange() const
{
  return hidden_neurons_[0].getRange();
}

//-----------------------------------------------------
/* get/set output_val_
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::setOutputVal(int output_val)
  throw()
{
  output_val_ = (output_val<1) ? 1 : output_val;

  create();
}
template <class HiddNeuronType, class OutNeuronType>
int NNet<HiddNeuronType,OutNeuronType>::getOutputVal() const
{
  return output_val_;
}

//-----------------------------------------------------
/* get/set hidden_val_
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::setHiddenVal(int hidden_val)
  throw()
{
  hidden_val_ = (hidden_val<1) ? 1 : hidden_val;

  create();
}
template <class HiddNeuronType, class OutNeuronType>
int NNet<HiddNeuronType,OutNeuronType>::getHiddenVal() const
{
  return hidden_val_;
}

//-----------------------------------------------------
/* get/set input_val_
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::setInputVal(int input_val)
    throw()
{
  input_val_ = (input_val<1) ? 1 : input_val;

  create();
}
template <class HiddNeuronType, class OutNeuronType>
int NNet<HiddNeuronType,OutNeuronType>::getInputVal() const
{
  return input_val_;
}

//-----------------------------------------------------
/* get/set memory of the output layer
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::setMemoryOut(int memory)
    throw()
{
  memory_out_ = (memory<0) ? 0 : memory;

  create();
}
template <class HiddNeuronType, class OutNeuronType>
int NNet<HiddNeuronType,OutNeuronType>::getMemoryOut() const
{
  return memory_out_;
}

//-----------------------------------------------------
/* get/set memory of the hidden layer
 */
template <class HiddNeuronType, class OutNeuronType>
void NNet<HiddNeuronType,OutNeuronType>::setMemoryHidden(int memory)
    throw()
{
  memory_hidden_ = (memory<0) ? 0 : memory;

  create();
}
template <class HiddNeuronType, class OutNeuronType>
int NNet<HiddNeuronType,OutNeuronType>::getMemoryHidden() const
{
  return memory_hidden_;
}


} // end of namespace

#endif //_INCLUDE_LIN_NEURAL_NET__
