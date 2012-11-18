/////////////////////////////////////////////////////////////////////////////
//
//   GEM - Graphics Environment for Multimedia
//
//   pix_recNN~
//   Calculates an audio signal out of a video frame
//   with a recurrent neural network
//
//   (see RecurrentNeuralNet.h for more info)
//
//   header file
//
//   Copyright (c) 2005 Georg Holzmann <grh@gmx.at>
//   (and of course lot's of other developers for PD and GEM)
//
//   For information on usage and redistribution, and for a DISCLAIMER OF ALL
//   WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _INCLUDE_PIX_RECNN_H__
#define _INCLUDE_PIX_RECNN_H__

#include <string>
#include <sstream>
#include <fstream>
#include "Base/GemPixObj.h"
#include "NNet.h"
#include "RecurrentNeuron.h"


using std::string;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::istringstream;

using namespace TheBrain;


/*-----------------------------------------------------------------
 *  CLASS
 * pix_recNN~
 *
 * calculates an audio signal out of a video frame with
 * a recurrent neural network
 *
 * KEYWORDS
 * pix audio
 *
 * DESCRIPTION
 * 1 signal-outlet
 */
class GEM_EXTERN pix_recNN : public GemPixObj
{
  CPPEXTERN_HEADER(pix_recNN, GemPixObj)

 public:

  /* Constructor
   */
  pix_recNN(t_floatarg arg0, t_floatarg arg1, t_floatarg arg2);

 protected:

  /* Destructor
   */
  virtual ~pix_recNN();


  //-----------------------------------
  /* Image STUFF:
   */

  /* The pixBlock with the current image
   *  pixBlock      m_pixBlock;
   */
  unsigned char *m_data_;
  int            m_xsize_;
  int            m_ysize_;
  int            m_csize_;
  int            m_format_;

  /* precision of the image:
   * 1 means every pixel is taken for the calculation,
   * 2 every second pixel, 3 every third, ...
   */
  int precision_;

  /* temporary float for calculation
   */
  float **temp_pix_;

  /* processImage
   */
  virtual void processImage(imageStruct &image);


  //-----------------------------------
  /* Neural Network STUFF:
   */

  /* the neural net
   * (size: buffsize)
   */
  NNet<RecurrentNeuron,RecurrentNeuron> *net_;

  /* training modus on
   * (will only be on for one audio buffer)
   */
  bool train_on_;

  /* the number of neurons, which should be
   * THE SAME as the audio buffer size
   */
  int neuron_nr_;

  /* memory determines, how much results from the past
   * are used to calculate an output value
   * (0 means only the result from the current frame,
   * 2 also from the last frame, etc.)
   */
  int memory_;


  //-----------------------------------
  /* Audio STUFF:
   */

  /* the outlet
   */
  t_outlet *out0_;

  /* DSP perform
   */
  static t_int* perform(t_int* w);

  /* DSP-Message
   */
  virtual void dspMess(void *data, t_signal** sp);


  //-----------------------------------
  /* File IO:
   */

  /* saves the contents of the current net to file
   */
  virtual void saveNet(string filename);

  /* loads the parameters of the net from file
   */
  virtual void loadNet(string filename);

 private:

  /* a helper to build a new net
   */
  virtual void buildNewNet();

  //-----------------------------------
  /* static members
   * (interface to the PD world)
   */

  /* set/get the precision of the image calculation
   */
  static void setPrecision(void *data, t_floatarg precision);
  static void getPrecision(void *data);

  /* method to train the network
   */
  static void setTrainOn(void *data);

  /* changes the number of neurons
   * (which should be the same as the audio buffer)
   * ATTENTION: a new net will be initialized
   */
  static void setNeurons(void *data, t_floatarg neurons);
  static void getNeurons(void *data);

  /* changes the nblock size
   * ATTENTION: a new net will be initialized
   */
  static void setMemory(void *data, t_floatarg memory);
  static void getMemory(void *data);

  /* sets the learnrate of the net
   */
  static void setLearnrate(void *data, t_floatarg learn_rate);
  static void getLearnrate(void *data);

  /* DSP callback
   */
  static void dspMessCallback(void* data, t_signal** sp);

  /* File IO:
   */
  static void saveToFile(void *data, t_symbol *filename);
  static void loadFromFile(void *data, t_symbol *filename);
};

#endif  // for header file
