/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* fftbuf~ fades between two buffers. it is intended to be used as fft          */
/* synthesis tool...                                                            */
/*                                                                              */
/*                                                                              */
/* fftbuf~ uses the flext C++ layer for Max/MSP and PD externals.               */
/* get it at http://www.parasitaere-kapazitaeten.de/PD/ext                      */
/* thanks to Thomas Grill                                                       */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* See file LICENSE for further informations on licensing terms.                */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* coded while listening to: Jérôme Noetinger/ErikM: What a Wonderful World     */
/*                           Cosmos: Tears                                      */
/*                           Burkhard Stangl/Dieb13: eh                         */
/*                                                                              */



#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error upgrade your flext version!!!!!!
#endif


class fftbuf: public flext_dsp
{
  FLEXT_HEADER(fftbuf,flext_dsp);

public: // constructor
  
  fftbuf(int);
  ~fftbuf();

protected:
  virtual void m_signal (int n, float *const *in, float *const *out);
  bool check(buffer *);

  
  void set_buf(int argc,  t_atom *argv); //selects a new buffer
  void set_line(int argc,  t_atom *argv); //sets the time for the transformance
  
  template<typename T>
  void clear(T *); //destroys an object
  
  void perform(); //starts transformation
  
  int blknumber(); //number of blocks that a performance needs

private:
  FLEXT_CALLBACK_V(set_buf);
  FLEXT_CALLBACK_V(set_line);
  FLEXT_CALLBACK(perform);

  float *ins;
  float *outs;

  t_symbol * bufname; 
  buffer * buf;
    
  int delay; //delay for fading from local buffer to new one
  t_sample * data; // pointer to array of samples
  t_sample * offset; // pointer to array of samples

  int bs; //blocksize+1
  int sr; //samplerate
  int counter;
};


FLEXT_LIB_DSP_1("bufline~",fftbuf,int);

fftbuf::fftbuf(int arg):
  buf(NULL),data(NULL),sr(Samplerate()),delay(0),counter(0)
{
  bs=arg+1;
  AddInAnything();
  AddOutSignal();
  FLEXT_ADDMETHOD_(0,"set",set_buf);
  FLEXT_ADDMETHOD_(0,"line",set_line);
  FLEXT_ADDBANG(0,perform);

  
  data= new t_sample[bs];
  offset= new t_sample[bs];
  ZeroSamples(data,bs);
  ZeroSamples(offset,bs);
}


fftbuf::~fftbuf()
{
  delete data;
  delete offset;
}


void fftbuf::m_signal(int n, t_float *const *in, t_float *const *out)
{
  if (check(buf))
    {
      outs = out[0];
      
      if (counter!=0)
	{
	  n=n/2+1;
      while (--n)
	data[n] = data[n] - offset[n];
      
      
      /*      for(int i=0;i!=bs;++i)
	      {
	data[i] = data[i] - offset[i];
	}
      */
      
      --counter;
	}
      
      CopySamples(out[0],data,bs);
    }
  else
    CopySamples(out[0],data,bs);
  
}

//perform und dsp gleichzeitig?!?


void fftbuf::perform()
{
  counter=blknumber();
  if (counter)
    {
      for(int i=0;i!=bs;++i)
	{
	  offset[i]=(data[i]-*(buf->Data()+i))/counter; 
	}
      
    }
  else
    {
      CopySamples(data,buf->Data(),bs);
    }
}

void fftbuf::set_buf(int argc, t_atom *argv)
{
  if(argc == 0)
    {
      post("No buffer selected!!!");
      return;
    }
  if (argc == 1 && IsSymbol(argv[0]))
    {
      clear(buf);
      bufname=GetSymbol(argv[0]);
      
      buf= new buffer(bufname);
      
      if(!buf->Ok())
	{
	  post("buffer %s is currently not valid",bufname);
	}
    }
  else if ((argc == 2 && IsSymbol(argv[0]) && 
	    (IsInt(argv[1]) || IsFloat(argv[1]))))
    {
      clear(buf);
      bufname=GetSymbol(argv[0]);
      
      buf= new buffer(bufname);
      
      if(!buf->Ok())
	{
	  post("buffer %s is currently not valid",bufname);
	  return;
	}
      delay=GetInt(argv[1]);
    }
}

template<typename T>
inline void fftbuf::clear(T* buf)
{
  if (buf)
    {
      delete buf;
      buf=NULL;
    }
}

inline bool fftbuf::check(buffer * buf) 
{
  if (buf==NULL)
    return false;
//code taken from the flext tutorial (buffer 1) by thomas grill

  if(buf->Update()) 
    {
      // buffer parameters have been updated
      if(buf->Valid()) 
	{
	  post("%s (%s) - updated buffer reference",
	       thisName(),GetString(thisTag()));
	  return true;
	}
      else 
	{
	  post("%s (%s) - buffer has become invalid",
	       thisName(),GetString(thisTag()));
	  return false;
	}
      }
  else
    return true;		
}

void fftbuf::set_line(int argc, t_atom *argv)
{
  if(argc==1 && (IsInt(argv[0]) || IsFloat(argv[0])))
    {
      delay=GetInt(argv[0]);
    }
  else
    post("syntax incorrect");
}

inline int fftbuf::blknumber()
{
  post("%i %i %i",delay,bs,sr);
  post("blknumber: %i",delay*bs/sr);

  return delay*bs/sr; //ms/sample
}
