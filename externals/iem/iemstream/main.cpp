/*************************************************************
 *
 *    streaming external for PD
 *
 * File: main.cpp
 *
 * Description: PD class definition
 *
 * Author: Thomas Grill (t.grill@gmx.net)
 *
 *************************************************************/


//! Version number of this external
#define __STREAM_VERSION "0.1.0"


// prevent MSVC "extern" warning
#ifdef _MSC_VER
#pragma warning( disable : 4091 ) 
#endif

// headers contained in the PD distribution
#include <m_pd.h>
#include <pthread.h>

// include stream class
#include "streamogg.h"


//! PD class
static t_class *amp_class = NULL;

//! PD object structure
struct t_amp
{
	t_object obj;
	Stream *stream;

	int channels;
	int vecsz;
	float srate;
	t_sample **outsigs;
	t_outlet *dumpout;
	t_clock *clk;
	int clkrate;
};

/*! periodic clock function for output of buffer filling status */
static void amp_clock(t_amp *x)
{
	outlet_float(x->dumpout,x->stream->getFilling());

	// retrigger clock
	if(x->clkrate) clock_delay(x->clk,x->clkrate);
}

/*! PD object constructor 
	\param fchannels	number of channels (stream can have less)
*/
static void *amp_new(t_floatarg fchannels)
{
	int channels = (int)fchannels;
	if(channels == 0) channels = 2;
	else
		if(channels < 1) return NULL;

	// make PD object
    t_amp *x = (t_amp *)pd_new(amp_class);
    
	x->channels = channels;
	x->outsigs = new t_sample *[channels];
	x->stream = new StreamOGG;

	// make signal outlets
    for(int i = 0; i < channels; i++)
    	outlet_new(&x->obj, gensym("signal"));
	// additional dump outlet (e.g. buffer filling status)
	x->dumpout = outlet_new(&x->obj, gensym("anything"));

	// set clock function
	x->clk = clock_new(x,(t_method)amp_clock);
	x->clkrate = 0;

	return x;
 }

/*! PD object destructor */
static void amp_free(t_amp *x)
{
	delete[] x->outsigs;
	delete x->stream;
	clock_free(x->clk);
}

/*! DSP function -> delegate processing to stream */
static t_int *amp_perform(t_int *w)
{
    t_amp *x = (t_amp *)w[1];

	x->stream->doGet(x->channels,x->outsigs,x->vecsz,x->srate);

	return w+2;
}

/*! set up DSP */
static void amp_dsp(t_amp *x, t_signal **sp)
{
	x->vecsz = sp[0]->s_n;
	x->srate = sp[0]->s_sr;

	// store outlet signal vectors
	for(int i = 0; i < x->channels; ++i)
		x->outsigs[i] = sp[i]->s_vec;

	dsp_add(amp_perform, 1, x);
}

/*! Connect to stream
	\param s	URL of stream
*/
static void amp_connect(t_amp *x,const t_symbol *s)
{
	x->stream->doInit(s->s_name);
}

/*! Disconnect from stream */
static void amp_disconnect(t_amp *x)
{
	x->stream->doExit();
}

/*! Set/Clear debug mode 
	\param f	0/1 ... debug mode off/on
*/
static void amp_debug(t_amp *x,t_floatarg f)
{
	x->stream->debug = (f != 0);
}

/*! Set tick rate for buffer filling status
	\param f	tick rate in ms
*/
static void amp_tick(t_amp *x,t_floatarg f)
{
	ASSERT(f >= 0);
	x->clkrate = (int)f;
	if(f) clock_delay(x->clk,f);
	else clock_unset(x->clk);
}

/*! Set decoder FIFO size */
static void amp_strbuf(t_amp *x,t_floatarg f)
{
	x->stream->setStreamBufSize((int)f);
}

/*! Set decoder chunk size */
static void amp_strchunk(t_amp *x,t_floatarg f)
{
	x->stream->setStreamBufChunk((int)f);
}

/*! Set decoder filling threshold ratio */
static void amp_strthresh(t_amp *x,t_floatarg f)
{
	x->stream->setStreamBufThresh(f);
}

/*! Output number of stream channels to dump outlet */
static void amp_getchannels(t_amp *x)
{
	t_atom a; 
	SETFLOAT(&a,x->stream->getChannels());
	outlet_anything(x->dumpout,gensym("channels"),1,&a);
}

/*! Output sample rate to dump outlet */
static void amp_getsrate(t_amp *x)
{
	t_atom a; 
	SETFLOAT(&a,x->stream->getSamplerate());
	outlet_anything(x->dumpout,gensym("srate"),1,&a);
}

/*! Output bitrate to dump outlet */
static void amp_getbrate(t_amp *x)
{
	t_atom a; 
	SETFLOAT(&a,x->stream->getBitrate());
	outlet_anything(x->dumpout,gensym("brate"),1,&a);
}

/*! Output a stream tag to dump outlet 
	\param sym	name of tag to output
*/
static void amp_gettag(t_amp *x,const t_symbol *sym)
{
	t_atom a[2]; 
	SETSYMBOL(a+0,const_cast<t_symbol *>(sym));
	SETSYMBOL(a+1,gensym(const_cast<char *>(x->stream->getTag(sym->s_name).c_str())));
	outlet_anything(x->dumpout,gensym("tag"),2,a);
}


/*! External setup routine of PD class
    \note Must be exported from shared library
*/
extern "C" 
#ifdef _MSC_VER
__declspec(dllexport)
#endif
void stream_setup()
{
    // post some message to the console
	post("Stream, version " __STREAM_VERSION ", (C)2003 IEM Graz");
	post("objects: amp~");
	post("");

    // register the xmlrpc class
	amp_class = class_new(gensym("amp~"),(t_newmethod)amp_new,(t_method)amp_free,sizeof(t_amp),0, A_DEFFLOAT,A_NULL);

	// register methods
	class_addmethod(amp_class, (t_method)amp_dsp, gensym("dsp"), A_NULL);

    class_addmethod(amp_class, (t_method)amp_connect, gensym("connect"), A_SYMBOL, A_NULL);
	class_addmethod(amp_class, (t_method)amp_disconnect, gensym("disconnect"), A_NULL);

    class_addmethod(amp_class, (t_method)amp_debug, gensym("debug"), A_FLOAT, A_NULL);
    class_addmethod(amp_class, (t_method)amp_tick, gensym("tick"), A_FLOAT, A_NULL);

    class_addmethod(amp_class, (t_method)amp_strbuf, gensym("strbuf"), A_FLOAT, A_NULL);
    class_addmethod(amp_class, (t_method)amp_strchunk, gensym("strchunk"), A_FLOAT, A_NULL);
    class_addmethod(amp_class, (t_method)amp_strthresh, gensym("strthresh"), A_FLOAT, A_NULL);

    class_addmethod(amp_class, (t_method)amp_getchannels, gensym("getchannels"), A_NULL);
    class_addmethod(amp_class, (t_method)amp_getsrate, gensym("getsrate"), A_NULL);
    class_addmethod(amp_class, (t_method)amp_getbrate, gensym("getbrate"), A_NULL);

    class_addmethod(amp_class, (t_method)amp_gettag, gensym("gettag"), A_SYMBOL,A_NULL);
}
