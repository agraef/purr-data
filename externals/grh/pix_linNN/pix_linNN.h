/////////////////////////////////////////////////////////////////////////////
//
//   GEM - Graphics Environment for Multimedia
//
//   pix_linNN~
//   Calculates an audio signal out of a video frame
//   with a linear neural network, which can be trained
//
//   the network has one neuron per audio sample: this neuron has
//   three inputs (a RGB-signal), a weight vector for each of the inputs,
//   a bias value and a linear output function
//   (see LinNeuralNet.h for more info)
//
//   header file
//
//   Copyright (c) 2004 Georg Holzmann <grh@gmx.at>
//   (and of course lot's of other developers for PD and GEM)
//
//   For information on usage and redistribution, and for a DISCLAIMER OF ALL
//   WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _INCLUDE_PIX_LINNN_H__
#define _INCLUDE_PIX_LINNN_H__

#include <string>
#include <sstream>
#include <fstream>
#include "Base/GemPixObj.h"
#include "LinNeuralNet.h"


using std::string;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::istringstream;


/*-----------------------------------------------------------------
 *  CLASS
 * pix_linNN~
 *   
 * calculates an audio signal out of a video frame with
 * a linear neural network
 *   
 * KEYWORDS
 * pix audio
 *   
 * DESCRIPTION
 * 1 signal-outlet
 */
class GEM_EXTERN pix_linNN : public GemPixObj
{
  CPPEXTERN_HEADER(pix_linNN, GemPixObj)

 public:

  /* Constructor
   */
  pix_linNN(t_floatarg arg0, t_floatarg arg1);
    	
 protected:
    	
  /* Destructor
   */  
  virtual ~pix_linNN();


  //-----------------------------------
  /* Image STUFF:
   */
    
  /* The pixBlock with the current image
   *  pixBlock    	m_pixBlock;
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

  /* processImage
   */
  virtual void processImage(imageStruct &image);


  //-----------------------------------
  /* Neural Network STUFF:
   */

  /* the linear neural nets
   * (size: buffsize)
   */
  LinNeuralNet *net_;

  /* training modus on
   * (will only be on for one audio buffer)
   */
  bool train_on_;

  /* the number of neurons, which should be
   * (= size of the array nets_)
   * THE SAME as the audio buffer size
   */
  int neuron_nr_;


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
   * (it saves the neuron_nr_, learning rate
   * IW-matrix and b1-vector of the net)
   */
  virtual void saveNet(string filename);

  /* loads the parameters of the net from file
   * (it loads the neuron_nr_, learning rate
   * IW-matrix and b1-vector of the net)
   */
  virtual void loadNet(string filename);

 private:

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
   * ATTENTION: a new IW-matrix and b1-vector will be initialized
   */
  static void setNeurons(void *data, t_floatarg neurons);
  static void getNeurons(void *data);

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

#endif	// for header file
