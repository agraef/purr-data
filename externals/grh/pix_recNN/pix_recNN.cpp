/////////////////////////////////////////////////////////////////////////////
//
//   GEM - Graphics Environment for Multimedia
//
//   pix_recNN
//
//   Implementation file
//
//   Copyright (c) 2005 Georg Holzmann <grh@gmx.at>
//   (and of course lot's of other developers for PD and GEM)
//
//   For information on usage and redistribution, and for a DISCLAIMER OF ALL
//   WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////////////////////////

#include "pix_recNN.h"

CPPEXTERN_NEW_WITH_THREE_ARGS(pix_recNN, t_floatarg, A_DEFFLOAT,
            t_floatarg, A_DEFFLOAT, t_floatarg, A_DEFFLOAT)

//----------------------------------------------------------
/* Constructor
 */
  pix_recNN::pix_recNN(t_floatarg arg0=64, t_floatarg arg1=1, t_floatarg arg2=1) :
    m_data_(NULL), m_xsize_(0), m_ysize_(0), m_csize_(0),
    train_on_(false), net_(NULL), temp_pix_(NULL)
{
  // init args ?????????????????????????????????
  neuron_nr_=2048;          //static_cast<int>((arg0<0)?2:arg0);
  memory_=0;
  precision_=2;          //static_cast<int>((arg2<1)?1:arg2);
  //post("arg0: %d, arg1: %d",arg0,arg1);

  // generate the in- and outlet:
  out0_ = outlet_new(this->x_obj, &s_signal);
  inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_signal, &s_signal);

  // set random seed:
  srand( (unsigned)time(NULL) );

  // build the net
  buildNewNet();
}

//----------------------------------------------------------
/* Destructor
 */
pix_recNN::~pix_recNN()
{
  outlet_free(out0_);
  m_data_ = NULL;
  m_xsize_ = 0;
  m_ysize_ = 0;

  // delete net
  delete net_;

  // delete temp_pix_
  for(int i=0; i<neuron_nr_; i++)
    delete[] temp_pix_[i];
  delete[] temp_pix_;
}

//----------------------------------------------------------
/* a helper to build a new net
 */
void pix_recNN::buildNewNet()
{
  try
    {
      if(net_)
  delete net_;

      if(temp_pix_)
  {
    for(int i=0; i<neuron_nr_; i++)
      delete[] temp_pix_[i];
    delete[] temp_pix_;
  }

      // create the net
      net_ = new NNet<RecurrentNeuron,RecurrentNeuron>(3,3,neuron_nr_,memory_,
                   0,TANH,LINEAR);
      if(!net_)
  {
    post("pix_recNN~: no memory for neural nets!");
    net_=NULL;
    return;
  }

      // create the temp_pix
      temp_pix_ = new float*[neuron_nr_];
      if(!temp_pix_)
  {
    post("pix_recNN~: no memory for temp_pix_!");
    temp_pix_=NULL;
    return;
  }
      for(int i=0; i<neuron_nr_; i++)
  {
    temp_pix_[i] = new float[3];
    if(!temp_pix_[i])
      {
        post("pix_recNN~: no memory for temp_pix_!");
        temp_pix_=NULL;
        return;
      }
  }

      // initialize temp_pix_ with 0
      for(int i=0; i<neuron_nr_; i++)
  {
    for(int j=0; j<3; j++)
      {
        temp_pix_[i][j] = 0;
      }
  }

      // init the net
      net_->create();
      net_->initRand(-1,1);
      net_->setRange(255);
      net_->setLearningRate(0.01);
    }
  catch(NNExcept &exc)
     {
       post("pix_recNN: %s", exc.what().c_str());
     }
}

//----------------------------------------------------------
/* processImage
 */
void pix_recNN::processImage(imageStruct &image)
{
  m_data_ = image.data;
  m_xsize_ = image.xsize;
  m_ysize_ = image.ysize;
  m_csize_ = image.csize;
  m_format_ = image.format;
}

//----------------------------------------------------------
/* DSP perform
 */
t_int* pix_recNN::perform(t_int* w)
{
  pix_recNN *x = GetMyClass((void*)w[1]);
  t_float* in_signal = (t_float*)(w[2]);
  t_float* out_signal = (t_float*)(w[3]);
  int blocksize = (t_int)(w[4]);

  if(blocksize != x->neuron_nr_)
    {
      post("pix_recNN~: neurons and buffersize are different! You MUST have the same neuron nr as the buffersize !!!");
      post("neurons: %d, buffersize: %d", x->neuron_nr_, blocksize);
      return (w+5);
    }


  // some needed data
  long int pix_size = x->m_xsize_ * x->m_ysize_;
  int pix_blocksize  = (blocksize<pix_size)?blocksize:pix_size;

  // splits the frame into slices, so that the average
  // of one slice can be used for the network input
  // there are as much slices as the buffsize is

  float nr = sqrt(blocksize); // the number of slices at the
                              // x- and y-axis

  float x_slice = x->m_xsize_ / nr; // x size of a slice in pixels
  float y_slice = x->m_ysize_ / nr; // x size of a slice in pixels
  int x_slice_int = static_cast<int>( x_slice );
  int y_slice_int = static_cast<int>( y_slice );

  // the number of slices on one axis (is the float nr
  // from above rounded up)
  int slice_nr = static_cast<int>(nr) + 1;


  if (x->m_data_)
  {
    switch(x->m_format_)
    {
    case GL_RGBA:
      {
  for(int n=0; n<pix_blocksize; n++)
    {
      //post("Block %d:",n);

      // calulate the pixel in left upper edge of every slice
      int lu_pix_x = static_cast<int>( (n % slice_nr) * x_slice );
      int lu_pix_y = static_cast<int>( static_cast<int>(n / slice_nr) * y_slice );

      //post("lu_pix: %d, %d", lu_pix_x, lu_pix_y);

      // now sum up all the pixels of one slice and then divide through the
      // number of pixels
      // the storage to sum the pixels:
      unsigned long int temp_data[3] = { 0, 0, 0 };

      // only for optimization:
      int helper1 = x->m_xsize_ * x->m_csize_;
      int add_count = 0;

      for(int i=0; i<x_slice_int; i+=x->precision_)
        {
    for(int j=0; j<y_slice_int; j+=x->precision_)
      {
        // the way to access the pixels: (C=chRed, chBlue, ...)
        //data[Y * xsize * csize + X * csize + C]

        //post("current pixel: %d %d",
        //     ((lu_pix_x+i)%x->m_xsize), ((lu_pix_y+j)%x->m_ysize) );

        temp_data[0] += x->m_data_[
            (lu_pix_y+j) * helper1
                  + (lu_pix_x+i) * x->m_csize_ + chRed ];

        temp_data[1] += x->m_data_[
            ((lu_pix_y+j)) * helper1
                  + ((lu_pix_x+i)) * x->m_csize_ + chGreen ];

        temp_data[2] += x->m_data_[
                  ((lu_pix_y+j)%x->m_ysize_) * helper1
                  + ((lu_pix_x+i)%x->m_xsize_) * x->m_csize_ + chBlue ];

        add_count++;
      }
        }

      x->temp_pix_[n][0] = temp_data[0] / add_count;
      x->temp_pix_[n][1] = temp_data[1] / add_count;
      x->temp_pix_[n][2] = temp_data[2] / add_count;
    }

  // learning, or calculation:
  if(!x->train_on_)
    x->net_->calculate(x->temp_pix_, out_signal);
  else
    x->net_->trainBTT(x->temp_pix_, out_signal, in_signal);

      }
      break;

    default:
      post("RGB only for now");
    }
  }
  else
    {
      pix_blocksize=blocksize;
      while (pix_blocksize--) *out_signal++=0;
    }

  x->train_on_=false;
  return (w+5);
}

//----------------------------------------------------------
/* DSP-Message
 */
void pix_recNN::dspMess(void *data, t_signal** sp)
{
  dsp_add(perform, 4, data, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

//----------------------------------------------------------
/* saves the contents of the current net to file
 */
void pix_recNN::saveNet(string filename)
{
  try
    {
      net_->save(filename);
      post("pix_recNN~: saved to output-file %s", filename.c_str());
    }
  catch(NNExcept &exc)
     {
       post("pix_recNN: %s", exc.what().c_str());
     }
}

//----------------------------------------------------------
/* loads the parameters of the net from file
 */
void pix_recNN::loadNet(string filename)
{
  try
    {
      net_->load(filename);
      post("pix_recNN~: loaded file %s", filename.c_str());
    }
  catch(NNExcept &exc)
     {
       post("pix_recNN: %s", exc.what().c_str());
     }
}

//----------------------------------------------------------
/* setup callback
 */
void pix_recNN::obj_setupCallback(t_class *classPtr)
{
  class_addcreator((t_newmethod)_classpix_recNN, gensym("pix_recNN~"), A_NULL);

  class_addmethod(classPtr, (t_method)pix_recNN::setNeurons,
      gensym("neurons"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::getNeurons,
      gensym("getneurons"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::setMemory,
      gensym("memory"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::getMemory,
      gensym("getmemory"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::setPrecision,
      gensym("precision"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::getPrecision,
      gensym("getprecision"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::setTrainOn,
      gensym("train"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::setLearnrate,
      gensym("learnrate"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::getLearnrate,
      gensym("getlearnrate"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::saveToFile,
      gensym("save"), A_SYMBOL, A_NULL);
  class_addmethod(classPtr, (t_method)pix_recNN::loadFromFile,
      gensym("load"), A_SYMBOL, A_NULL);

  class_addmethod(classPtr, (t_method)pix_recNN::dspMessCallback,
      gensym("dsp"), A_NULL);
  class_addmethod(classPtr, nullfn, gensym("signal"), A_NULL);
}

//----------------------------------------------------------
/* DSP callback
 */
void pix_recNN::dspMessCallback(void *data, t_signal** sp)
{
  GetMyClass(data)->dspMess(data, sp);
}

//----------------------------------------------------------
/* sets the precision
 */
void pix_recNN::setPrecision(void *data, t_floatarg precision)
{
  GetMyClass(data)->precision_ =
    (precision<1) ? 1 : static_cast<int>(precision);
}
void pix_recNN::getPrecision(void *data)
{
  post("pix_recNN~: precision: %d",GetMyClass(data)->precision_);
}

//----------------------------------------------------------
/* method to train the network
 */
void pix_recNN::setTrainOn(void *data)
{
  GetMyClass(data)->train_on_ = true;
}

//----------------------------------------------------------
/* changes the number of neurons
 * (which should be the same as the audio buffer)
 * ATTENTION: a new net will be initialized
 */
void pix_recNN::setNeurons(void *data, t_floatarg neurons)
{
  GetMyClass(data)->neuron_nr_ =
    (neurons<1) ? 1 : static_cast<int>(neurons);

  GetMyClass(data)->buildNewNet();
}
void pix_recNN::getNeurons(void *data)
{
  post("pix_recNN~: nr of neurons: %d (MUST be the same as buffersize!)",
       GetMyClass(data)->neuron_nr_);
}

//----------------------------------------------------------
/* changes the nblock size
 * ATTENTION: a new net will be initialized
 */
void pix_recNN::setMemory(void *data, t_floatarg memory)
{
  GetMyClass(data)->memory_ =
    (memory<0) ? 0 : static_cast<int>(memory);

  GetMyClass(data)->buildNewNet();
}
void pix_recNN::getMemory(void *data)
{
  post("pix_recNN~: memory: %d",
       GetMyClass(data)->memory_);
}

//----------------------------------------------------------
/* sets the learnrate of the net
 */
void pix_recNN::setLearnrate(void *data, t_floatarg learn_rate)
{
  GetMyClass(data)->net_->setLearningRate(learn_rate);
}
void pix_recNN::getLearnrate(void *data)
{
  post("pix_recNN~: learning rate: %f",GetMyClass(data)->net_->getLearningRate());
}

//----------------------------------------------------------
/* FileIO-stuff
 */
void pix_recNN::saveToFile(void *data, t_symbol *filename)
{
  GetMyClass(data)->saveNet(filename->s_name);
}
void pix_recNN::loadFromFile(void *data, t_symbol *filename)
{
  GetMyClass(data)->loadNet(filename->s_name);
}
