/////////////////////////////////////////////////////////////////////////////
//
//   GEM - Graphics Environment for Multimedia
//
//   pix_linNN
//
//   Implementation file
//
//   Copyright (c) 2004 Georg Holzmann <grh@gmx.at>
//   (and of course lot's of other developers for PD and GEM)
//
//   For information on usage and redistribution, and for a DISCLAIMER OF ALL
//   WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////////////////////////

#include "pix_linNN.h"

CPPEXTERN_NEW_WITH_TWO_ARGS(pix_linNN, t_floatarg, A_DEFFLOAT, t_floatarg, A_DEFFLOAT)

//----------------------------------------------------------
/* Constructor
 */
  pix_linNN::pix_linNN(t_floatarg arg0=64, t_floatarg arg1=1) : 
    m_data_(NULL), m_xsize_(0), m_ysize_(0), m_csize_(0), 
    train_on_(false), net_(NULL)
{
  // init args ?????????????????????????????????
  neuron_nr_=2048;          //static_cast<int>((arg0<0)?2:arg0);
  precision_=2;          //static_cast<int>((arg1<1)?1:arg1);
  //post("arg0: %d, arg1: %d",arg0,arg1);

  // generate the in- and outlet:
  out0_ = outlet_new(this->x_obj, &s_signal);
  inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_signal, &s_signal);

  // set random seed:
  srand( (unsigned)time(NULL) );

  // creates the nets
  net_ = new LinNeuralNet[neuron_nr_](3);
  if(!net_)
    {
      post("pix_linNN~: no memory for neural nets!");
      return;
    }

  for(int i=0; i<neuron_nr_; i++)
    {
      if( !net_[i].createNeurons() )
	{
	  post("pix_linNN~: error in creating the net!");
	  return;
	}
      if( !net_[i].initNetworkRand(-1,1) )
	{
	  post("pix_linNN~: error in initializing the net!");
	  return;
	}	

      net_[i].setRange(255);
      net_[i].setLearningRate(0.01);
    }
}

//----------------------------------------------------------
/* Destructor
 */
pix_linNN::~pix_linNN()
{
  outlet_free(out0_);
  m_data_ = NULL;
  m_xsize_ = 0;
  m_ysize_ = 0;

  // delete weight matrix and bias vector
  delete[] net_;
}

//----------------------------------------------------------
/* processImage
 */
void pix_linNN::processImage(imageStruct &image)
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
t_int* pix_linNN::perform(t_int* w)
{
  pix_linNN *x = GetMyClass((void*)w[1]);
  t_float* in_signal = (t_float*)(w[2]);
  t_float* out_signal = (t_float*)(w[3]);
  int blocksize = (t_int)(w[4]);

  if(blocksize != x->neuron_nr_)
    {
      post("pix_linNN~: neurons and buffersize are different! You MUST have the same neuron nr as the buffersize !!!");
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
      for(int n=0; n<pix_blocksize; n++)
      {
	//post("Block %d:",n);

	// calulate the pixel in left upper edge of every slice
	int lu_pix_x = static_cast<int>( (n % slice_nr) * x_slice );
	int lu_pix_y = static_cast<int>( static_cast<int>(n / slice_nr) * y_slice );

	//post("lu_pix: %d, %d", lu_pix_x, lu_pix_y);

	// now sum up all the pixels of one slice and then divide through the
	// number of pixels
	unsigned long int temp_data[3] = { 0, 0, 0 };  // the storage to sum the pixels
	t_float average_pix[3] = { 0, 0, 0 };  // the average of the pixels
	
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
	average_pix[0] = temp_data[0] / add_count;
	average_pix[1] = temp_data[1] / add_count;
	average_pix[2] = temp_data[2] / add_count;

	// the calculation of the network:
	*out_signal = x->net_[n].calculateNet(average_pix);

	//post("%d: RGBav: %f %f %f, out_signal: %f",
	//n,average_pix[0],average_pix[1],average_pix[2],*out_signal);
	
	// learning:
	if(x->train_on_)
	  x->net_[n].trainNet(average_pix, *out_signal, *in_signal);

	out_signal++;
	in_signal++;
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
void pix_linNN::dspMess(void *data, t_signal** sp)
{
  dsp_add(perform, 4, data, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

//----------------------------------------------------------
/* saves the contents of the current net to file
 * (it saves the neuron_nr_, learning rate
 * IW-matrix and b1-vector of the net)
 */
void pix_linNN::saveNet(string filename)
{
  // open and check outfile
  ofstream outfile;
  outfile.open(filename.c_str());
  if(!outfile)
    {
      post("pix_linNN~: failed to open output-file!");
      return;
    }
  
  // write XML-header
  outfile << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>" << endl;

  // start-tag
  outfile << "<linNN>" << endl;

  // neuron_nr_(=size) and learning rate
  outfile << "\t<neurons> " << neuron_nr_ << " </neurons>" << endl;
  outfile << "\t<learnrate> " << net_[0].getLearningRate() 
	  << " </learnrate>" << endl;

  // now the IW-matrix of the neural net
  outfile << "\t<IW>" << endl;
  for(int i=0; i<neuron_nr_; i++)
    {
      outfile << "\t\t" << net_[i].getIW()[0] << " "
	      << net_[i].getIW()[1] << " "
	      << net_[i].getIW()[2] << endl;
    }
  outfile << "\t</IW>" << endl;

  // and the b1-vector
  outfile << "\t<b1>" << endl << "\t\t";
  for(int i=0; i<neuron_nr_; i++)
    {
      outfile << net_[i].getb1() << " ";
    }
  outfile << endl << "\t</b1>" << endl;

  // end-tag
  outfile << "</linNN>" << endl;


  outfile.close();
  post("pix_linNN~: saved to output-file %s", filename.c_str());
  return;
}

//----------------------------------------------------------
/* loads the parameters of the net from file
 * (it loads the neuron_nr_, learning rate
 * IW-matrix and b1-vector of the net)
 */
void pix_linNN::loadNet(string filename)
{
  // temp variables
  float IW[3];
  float b1, learnrate;

  ifstream infile;
  infile.open(filename.c_str());

  if(!infile)
    {
      post("pix_linNN~: cannot open input-file!");
      return;
    }

  post("pix_linNN~: loading input-file %s",filename.c_str());

  int state = 0, IWcount = 0, b1count = 0;
  bool tag=false;
  string line, temp;

  while (getline(infile, line))
    {
      istringstream instream(line);
      instream >> temp;

      // specify the tags
      //post("input: %s",temp.c_str());
      if( temp == "<neurons>" ) 
	{state=1; }
      if( temp == "<learnrate>" ) 
	{state=2; }
      if( temp == "<IW>" ) 
	{state=3; }
      if( temp == "<b1>" ) 
	{state=4; }
      if( !strncmp(temp.c_str(),"</",2) ) 
	{state=0;}

      if( !strncmp(temp.c_str(),"<",1) ) 
	{tag=true; }
      else 
	{tag=false; }

      // make string stream again
      instream.str(line);
      if(tag)
	instream >> temp; // if theres a tag, stream it


      bool go_on=false;
      while(!go_on)
	{
	  // end of a line
	  if(instream.eof() || !state)
	    {
	      go_on=true;
	      break;
	    }


	  // <neuron>
	  if(state == 1)
	    {
	      instream >> neuron_nr_;
	      if(!net_)
		{
		  // creates new nets
		  net_ = new LinNeuralNet[neuron_nr_](3);
		  if(!net_)
		    {
		      post("pix_linNN~: no memory for neural nets!");
		      break;
		    }
		}
	      for(int i=0; i<neuron_nr_; i++)
		{
		  if( !net_[i].createNeurons() )
		    {
		  post("pix_linNN~: error in creating the net!");
		  break;
		    }
		}

	      go_on=false;
	      break;
	    }
	  
	  // <learnrate>
	  if(state == 2)
	    {
	      instream >> learnrate;

	      for(int i=0; i<neuron_nr_; i++)
		net_[i].setLearningRate(learnrate);

	      go_on=false;
	      break;
	    }
	  
	  // <IW>
	  if(state == 3)
	    {
	      instream >> IW[0];
	      instream >> IW[1];
	      instream >> IW[2];
	      
	      if(IWcount<neuron_nr_)
		net_[IWcount++].setIW(IW);
	      else
		{
		  go_on = false;
		  break;
		}
	    }
	  
	  // <b1>
	  if(state == 4)
	    {
	      for(int i=0; i<neuron_nr_; i++)
		{	      
		  instream >> b1;
		  net_[b1count++].setb1(b1);
		}

	      go_on = false;
	      break;
	    }

	  //else:
	  go_on=false;
	  break;
	}
    }

  infile.close();
  return;
}

//----------------------------------------------------------
/* setup callback
 */
void pix_linNN::obj_setupCallback(t_class *classPtr)
{
  class_addcreator((t_newmethod)_classpix_linNN, gensym("pix_linNN~"), A_NULL);

  class_addmethod(classPtr, (t_method)pix_linNN::setNeurons,
		  gensym("neurons"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)pix_linNN::getNeurons,
		  gensym("getneurons"), A_NULL); 
  class_addmethod(classPtr, (t_method)pix_linNN::setPrecision,
		  gensym("precision"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)pix_linNN::getPrecision,
		  gensym("getprecision"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_linNN::setTrainOn,
		  gensym("train"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_linNN::setLearnrate,
		  gensym("learnrate"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)pix_linNN::getLearnrate,
		  gensym("getlearnrate"), A_NULL);
  class_addmethod(classPtr, (t_method)pix_linNN::saveToFile,
		  gensym("save"), A_SYMBOL, A_NULL);
  class_addmethod(classPtr, (t_method)pix_linNN::loadFromFile,
		  gensym("load"), A_SYMBOL, A_NULL);

  class_addmethod(classPtr, (t_method)pix_linNN::dspMessCallback, 
		  gensym("dsp"), A_NULL);
  class_addmethod(classPtr, nullfn, gensym("signal"), A_NULL);
}

//----------------------------------------------------------
/* DSP callback
 */
void pix_linNN::dspMessCallback(void *data, t_signal** sp)
{
  GetMyClass(data)->dspMess(data, sp);
}

//----------------------------------------------------------
/* sets the precision
 */
void pix_linNN::setPrecision(void *data, t_floatarg precision)
{
  GetMyClass(data)->precision_ = 
    (precision<1) ? 1 : static_cast<int>(precision);
}
void pix_linNN::getPrecision(void *data)
{
  post("pix_linNN~: precision: %d",GetMyClass(data)->precision_);
}

//----------------------------------------------------------
/* method to train the network
 */
void pix_linNN::setTrainOn(void *data)
{
  GetMyClass(data)->train_on_ = true; 
}

//----------------------------------------------------------
/* changes the number of neurons
 * (which should be the same as the audio buffer)
 * ATTENTION: a new IW-matrix and b1-vector will be initialized
 */
void pix_linNN::setNeurons(void *data, t_floatarg neurons)
{
  GetMyClass(data)->neuron_nr_ =
    (neurons<1) ? 1 : static_cast<int>(neurons);

  if(GetMyClass(data)->net_)
    delete[] GetMyClass(data)->net_;

  // creates the nets
  GetMyClass(data)->net_ = new LinNeuralNet[GetMyClass(data)->neuron_nr_](3);
  if(!GetMyClass(data)->net_)
    {
      post("pix_linNN~: no memory for neural nets!");
      return;
    }

  for(int i=0; i<GetMyClass(data)->neuron_nr_; i++)
    {
      if( !GetMyClass(data)->net_[i].createNeurons() )
	{
	  post("pix_linNN~: error in creating the net!");
	  return;
	}
      if( !GetMyClass(data)->net_[i].initNetworkRand(-1,1) )
	{
	  post("pix_linNN~: error in initializing the net!");
	  return;
	}
    }
}
void pix_linNN::getNeurons(void *data)
{
  post("pix_linNN~: nr of neurons: %d (MUST be the same as buffersize!)",
       GetMyClass(data)->neuron_nr_);
}

//----------------------------------------------------------
/* sets the learnrate of the net
 */
void pix_linNN::setLearnrate(void *data, t_floatarg learn_rate)
{
  for(int i=0; i<GetMyClass(data)->neuron_nr_; i++)
    GetMyClass(data)->net_[i].setLearningRate(learn_rate);
}
void pix_linNN::getLearnrate(void *data)
{
  post("pix_linNN~: learning rate: %f",GetMyClass(data)->net_[0].getLearningRate());
}

//----------------------------------------------------------
/* FileIO-stuff
 */
void pix_linNN::saveToFile(void *data, t_symbol *filename)
{
  GetMyClass(data)->saveNet(filename->s_name);
}
void pix_linNN::loadFromFile(void *data, t_symbol *filename)
{
  GetMyClass(data)->loadNet(filename->s_name);
}
