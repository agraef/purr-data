/*
 * Readanysf  PD object for reading and playing multiple soundfile types
 * from disk and from the web using gmerlin_avdecode
 *
 * Copyright (C) 2003-2010 August Black
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * readanysf.cpp
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>  //IMPORTANT bool
#include <stdlib.h>   // for malloc
#include <stdio.h>	//sprintf
#include <string.h>  //strcmp
#include <math.h>   // ceil

#include "m_pd.h"
#include "ReadMedia.h"

#define MAXSFCHANS 64	// got this from d_soundfile.c in pd/src


static t_class *readanysf_class;

typedef struct readanysf {
	t_object x_obj;
	t_sample *(x_outvec[MAXSFCHANS]);
	t_outlet *outinfo;

	int blocksize; // size of the pd block for this object
	int num_channels;
	int num_frames_in_fifo;
	int num_samples_per_frame;
	unsigned int tick;  // how often to send outlet info
	bool play;
	bool is_opening;
	unsigned int count;
	float src_factor;

	bool do_t2o_audio_convert;
	bool do_i2t_audio_convert;

	int samplesleft;  // how many audio samples left in our curren read buf


	ReadMedia *rm;
	gavl_audio_frame_t * out_audio_frame;
	gavl_audio_frame_t * tmp_audio_frame;
	gavl_audio_frame_t * in_audio_frame;

	gavl_audio_format_t out_audio_format;
	gavl_audio_format_t tmp_audio_format;
	gavl_audio_format_t in_audio_format;

	gavl_audio_converter_t *  i2t_audio_converter;
	gavl_audio_converter_t *  t2o_audio_converter;
	
	pthread_mutex_t mut;	

} t_readanysf;


void m_play(t_readanysf *x) {

	pthread_mutex_lock(&x->mut);
	if (x->rm->isReady() && !x->is_opening ) {	
		// is_opening protects the play variable, which in turn protects 
		// the memory accessed in m_decode_block inside of the perform function
		// as long as play is false, no memory is accessed
		// as long as is_opening is true, play is false and no memory is accessed
		// to be safe, we will protect the is_opening var so that when the open callback
		// is called, and the memory is de-allocated and re-allocated, that memory is not
		// access by the perform function 
		x->play = true;  // this is the only place where play is true
	} else {
		if (x->is_opening ) {
			post("Current file is still starting.");
			post("This probably means that it is a stream and it needs to buffer in from the network.");
		} else {
			post("Current file is either invalid or an unsupported codec.");
		}
	}
	pthread_mutex_unlock(&x->mut);
}

void readanysf_bang(t_readanysf *x) {
	m_play(x);
}


void m_pause(t_readanysf *x) {
	x->play = false;
}

void m_pcm_seek(t_readanysf *x, float f) {
	if (! x->rm->pcmSeek( (long)f) )		
		post("can't seek on this file.");
}	

void m_time_seek(t_readanysf *x, float f) {
	if (! x->rm->timeSeek( (double)f) )		
		post("can't seek on this file.");
}	

void m_tick(t_readanysf *x, float f) {
	if (f >= 0.0) {
		x->tick = (unsigned int) f ;
	}
}	

void m_stop(t_readanysf *x) {
	x->play = false;
	x->samplesleft=0;
	x->count=0;
/*
	if( x->out_audio_frame != NULL)
		gavl_audio_frame_mute_samples(x->out_audio_frame, &x->out_audio_format, x->out_audio_format.samples_per_frame);
	if( x->in_audio_frame != NULL)
		gavl_audio_frame_mute_samples(x->in_audio_frame, &x->in_audio_format, x->in_audio_format.samples_per_frame);
	if( x->tmp_audio_frame != NULL)
		gavl_audio_frame_mute_samples(x->tmp_audio_frame, &x->tmp_audio_format, x->tmp_audio_format.samples_per_frame);
*/

	x->rm->rewind();
}

void m_init_audio( t_readanysf *x) {
	// Now,. do Audio stuff
	x->rm->copyAudioFormat( &x->in_audio_format); 
	x->in_audio_format.samples_per_frame = x->num_samples_per_frame;

	x->tmp_audio_format.samplerate = x->in_audio_format.samplerate;
	x->tmp_audio_format.samples_per_frame = x->in_audio_format.samples_per_frame;
	x->tmp_audio_format.num_channels = x->out_audio_format.num_channels;
	x->tmp_audio_format.channel_locations[0] = GAVL_CHID_NONE; // Reset
	gavl_set_channel_setup (&x->tmp_audio_format); // Set channel locations

	if (x->in_audio_frame != NULL)
		gavl_audio_frame_destroy(x->in_audio_frame);
	x->in_audio_frame = gavl_audio_frame_create(&x->in_audio_format);

	if (x->tmp_audio_frame != NULL)
		gavl_audio_frame_destroy(x->tmp_audio_frame);
	x->tmp_audio_frame = gavl_audio_frame_create(&x->tmp_audio_format);

	/* m_out_audio_format.samples_per_frame = (m_out_audio_format.samplerate / (double)m_get_audio_format.samplerate) * 
		 m_get_audio_format.samples_per_frame + 10;
	 */

	if (x->i2t_audio_converter == NULL)
		x->i2t_audio_converter = gavl_audio_converter_create( );
	x->do_i2t_audio_convert = gavl_audio_converter_init( x->i2t_audio_converter, &x->in_audio_format, &x->tmp_audio_format); 

	if (x->t2o_audio_converter == NULL)
		x->t2o_audio_converter = gavl_audio_converter_create( );
	x->do_t2o_audio_convert = gavl_audio_converter_init_resample( x->t2o_audio_converter, &x->out_audio_format); 

	// FIXME: this should be protected
	x->src_factor = x->out_audio_format.samplerate / (float) x->in_audio_format.samplerate;
	/*
		 printf("in audio format: \n");
		 gavl_audio_format_dump( &x->in_audio_format);
		 printf("tmp audio format,  converting from input=%d \n", x->do_i2t_audio_convert);
		 gavl_audio_format_dump( &x->tmp_audio_format);
		 printf("output audio format,  converting from tmp=%d \n", x->do_t2o_audio_convert);
		 gavl_audio_format_dump( &x->out_audio_format);
	*/
};


void m_open_callback( void * data) {
	t_atom lst;
	t_readanysf * x = (t_readanysf *)data;

	pthread_mutex_lock(&x->mut);
	x->is_opening = true; // set it here again just to be safe
	
	if (x->rm->isReady() && x->rm->getAudioStreamCount() ) {	

		m_init_audio(x);

		// FIXME:  is it safe to call these here?
		SETFLOAT(&lst, (float)x->rm->getAudioSamplerate() );
		outlet_anything(x->outinfo, gensym("samplerate"), 1, &lst);
		
		SETFLOAT(&lst, x->rm->getLengthInSeconds() );
		outlet_anything(x->outinfo, gensym("length"), 1, &lst);

		outlet_float(x->outinfo, 0.0);

		// ready should be last	
		SETFLOAT(&lst, 1.0 );
		outlet_anything(x->outinfo, gensym("ready"), 1, &lst);
		// set time to 0 again here just to be sure
	} else {
		SETFLOAT(&lst, 0.0 );
		outlet_anything(x->outinfo, gensym("samplerate"), 1, &lst);
		SETFLOAT(&lst, 0.0 );
		outlet_anything(x->outinfo, gensym("length"), 1, &lst);
		SETFLOAT(&lst, 0.0 );
		outlet_anything(x->outinfo, gensym("ready"), 1, &lst);
		outlet_float(x->outinfo, 0.0);
		post("Invalid file or unsupported codec.");
	}
	x->is_opening=false;
	pthread_mutex_unlock(&x->mut);
}

void m_open(t_readanysf *x, t_symbol *s) {

	t_atom lst;
	SETFLOAT(&lst, 0.0 );
	outlet_anything(x->outinfo, gensym("ready"), 1, &lst);

	SETFLOAT(&lst, 0.0 );
	outlet_anything(x->outinfo, gensym("length"), 1, &lst);
	
	outlet_float(x->outinfo, 0.0);

	x->play = false;
	pthread_mutex_lock(&x->mut);
	x->is_opening = true;
	pthread_mutex_unlock(&x->mut);
	x->rm->openFile( s->s_name, 0, x->num_frames_in_fifo, x->num_samples_per_frame );
}

void m_speed(t_readanysf *x, float f) {
	//	x->rm->setSpeed( f );
	if (f > SRC_MAX) 
		return;
	if (f < SRC_MIN)
		return;
	// lock on src_factor because it is set called during open callback
	// we can be sure then it won't conflict
	pthread_mutex_lock(&x->mut);
	x->src_factor = 1.0/f;
	pthread_mutex_unlock(&x->mut);
}

void m_loop(t_readanysf *x, float f) {
	if ( f == 0)
		x->rm->setLoop( false );
	else 
		x->rm->setLoop( true );
	post("looping = %d", x->rm->getLoop());
}


static void *readanysf_new(t_float f, t_float f2, t_float f3 ) {
  
  int nchannels = (int)f;
  int nframes = (int)f2;
  int nsamples = (int)f3;
  int i;
  t_atom lst;

  // if the external is created without any options
  if (nchannels <=0)
	  nchannels = 2;

  if (nframes <=0)
	  nframes = 24;

  if (nsamples <=0)
	  nsamples = sys_getblksize();

  t_readanysf *x = (t_readanysf *)pd_new(readanysf_class);
	x->blocksize=0;
  x->num_channels = nchannels;
  x->num_frames_in_fifo = nframes;
  x->num_samples_per_frame = nsamples;
  x->tick = 1000;
  x->play =false; 
	x->is_opening=false;
  x->count = 0;
	x->src_factor = 1.0;
	x->do_t2o_audio_convert = false;
	x->do_i2t_audio_convert = false;
	x->samplesleft = 0;

  x->rm = NULL; 

	x->out_audio_frame=NULL;	
	x->tmp_audio_frame=NULL;	
	x->in_audio_frame=NULL;	

	// set up the audio formats.  Need to also set them in dsp call
	x->tmp_audio_format.samplerate = sys_getsr();
	x->tmp_audio_format.sample_format = GAVL_SAMPLE_FLOAT ;
	x->tmp_audio_format.interleave_mode = GAVL_INTERLEAVE_NONE;
	x->tmp_audio_format.num_channels = x->num_channels;
	x->tmp_audio_format.channel_locations[0] = GAVL_CHID_NONE; // Reset
	x->tmp_audio_format.samples_per_frame = x->num_samples_per_frame;

	x->out_audio_format.samplerate = sys_getsr();
	x->out_audio_format.sample_format = GAVL_SAMPLE_FLOAT ;
	x->out_audio_format.interleave_mode = GAVL_INTERLEAVE_NONE;
	x->out_audio_format.num_channels = x->num_channels;
	x->out_audio_format.channel_locations[0] = GAVL_CHID_NONE; // Reset


	x->i2t_audio_converter=NULL;
	x->t2o_audio_converter=NULL;

	pthread_mutex_init(&x->mut, 0);

	for (i=0; i < nchannels; i++) {
  	outlet_new(&x->x_obj,  gensym("signal"));
  }
  x->outinfo = outlet_new(&x->x_obj, &s_anything);
  SETFLOAT(&lst, 0.0 );
  outlet_anything(x->outinfo, gensym("ready"), 1, &lst);
  
  // set time to 0.0
  outlet_float(x->outinfo, 0.0);
	if (x->rm == NULL) {
		x->rm = new ReadMedia ( ); // (int)sys_getsr(), x->num_channels, x->num_frames_in_fifo, x->num_samples_per_frame);
		post("Created new readanysf~ with %d channels and internal buffer of %d * %d = %d", x->num_channels,
				x->num_frames_in_fifo, x->num_samples_per_frame, x->num_frames_in_fifo *  x->num_samples_per_frame);
	}
	x->rm->setOpenCallback( m_open_callback, (void *)x); 

  return (void *)x;
}

int m_get_frame( t_readanysf *x ) {
	int ret =0;	
	ret = x->rm->decodeAudio(x->in_audio_frame);
	if (ret != 1) // EOF
		return ret;

	if (x->do_i2t_audio_convert) {
		gavl_audio_convert( x->i2t_audio_converter, x->in_audio_frame, x->tmp_audio_frame) ;
		x->tmp_audio_frame->valid_samples = x->in_audio_frame->valid_samples;
	} else {
		gavl_audio_frame_copy(&x->in_audio_format, x->tmp_audio_frame,  x->in_audio_frame, 
				0,0, x->in_audio_frame->valid_samples, x->in_audio_frame->valid_samples) ;
		x->tmp_audio_frame->valid_samples = x->in_audio_frame->valid_samples;
	}

	if ( x->do_t2o_audio_convert  ) { // should be true all of the time
		gavl_audio_converter_resample( x->t2o_audio_converter, x->tmp_audio_frame, x->out_audio_frame, x->src_factor );
		//  Don't know why, but on the first conversion, I get one extra sample
		//  THIS SHOULD NOT HAPPEN...this is a fix for now..check it out later.
		//if (x->src_factor == 1.0 && x->out_audio_frame->valid_samples > x->num_samples_per_frame) { 
		//	printf("Got wierd return value for audio frames,  taf->vs %d, oaf->vs %d, src_factor=%f\n",
		//			x->tmp_audio_frame->valid_samples, x->out_audio_frame->valid_samples, x->src_factor);
			//x->samplesleft = x->out_audio_frame->valid_samples = x->num_samples_per_frame;
		//} else {
		x->samplesleft = x->out_audio_frame->valid_samples;
		//}
	} else {
		// copy the samples to the output
		gavl_audio_frame_copy(&x->tmp_audio_format, x->out_audio_frame, x->tmp_audio_frame, 
				0,0, x->tmp_audio_frame->valid_samples, x->tmp_audio_frame->valid_samples) ;
		//printf("copying taf to oaf,  taf->vs %d, oaf->vs %d\n", taf->valid_samples, oaf->valid_samples);
		x->samplesleft = x->out_audio_frame->valid_samples = x->tmp_audio_frame->valid_samples;
	}
	return ret;
}

int m_decode_block( t_readanysf * x ) {
	int i=0,j=0, samps_done=0;
	int samps_to_do = x->blocksize;

	while( samps_to_do > 0) {
		if ( samps_to_do <= x->samplesleft) {
			//if (x->out_audio_frame->valid_samples < x->samplesleft) 	
			//	printf("error\n");
			// copy our samples out to the pd audio buffer
			for (i = 0; i < x->num_channels; i++) {
				for (j = 0; j <  samps_to_do ;  j++) {
					x->x_outvec[i][samps_done + j] = x->out_audio_frame->channels.f[i][ x->out_audio_frame->valid_samples - x->samplesleft +j ];
				}
			}
			x->samplesleft -= samps_to_do;
			samps_done += samps_to_do;
			samps_to_do = 0;
			break;
		} else if ( x->samplesleft > 0 ) {
			//if( x->out_audio_frame->valid_samples  < x->samplesleft)
			//	printf("valid_samples < samplesleft, shouldn't happen\n");
			for (i = 0; i < x->num_channels; i++) {
				for (j = 0; j <  x->samplesleft;  j++) {
					x->x_outvec[i][samps_done + j] = x->out_audio_frame->channels.f[i][ x->out_audio_frame->valid_samples - x->samplesleft +j ];
				}
			}
			samps_to_do = samps_to_do - x->samplesleft;	
			samps_done += x->samplesleft;
			x->samplesleft = 0;
		} else { // samplesleft is zero
			int ret = m_get_frame(x);
			if (ret == 0) {
				return samps_done;
			} else if (ret == -1) {
				//printf("error getting frame...must be seeking\n");
				return ret;
			} 
		}
	} 
	return samps_done;
}

static t_int *readanysf_perform(t_int *w) {
	t_readanysf *x = (t_readanysf *) (w[1]);
	int i=0,j=0;
	int samples_returned = 0;
	t_atom lst;


	if (x->play ) { // play protects the memory accessed in m_decode_block
		samples_returned = m_decode_block( x );	
		if (samples_returned == 0 ) { // EOF
			m_stop(x);
			outlet_bang(x->outinfo);
		} else if (samples_returned == -1) {
			// error in getting audio, normally from seeking
			samples_returned=0;
		}
	} 
	
	for (i = 0; i < x->num_channels; i++) {
		for (j = samples_returned; j < x->blocksize;  j++) {
			x->x_outvec[i][j] = 0.0;
		}
	}


	// just set some variables
	if ( ++x->count > x->tick ) {
		SETFLOAT (&lst, x->rm->getAudioFifoSizePercentage() );
		outlet_anything(x->outinfo, gensym("cache"), 1, &lst);
		if (x->play) {
			outlet_float( x->outinfo,
					gavl_time_to_seconds(gavl_time_unscale(x->in_audio_format.samplerate, x->in_audio_frame->timestamp))
			);
		}
		x->count = 0;
	}
	
	return (w+2);	
}

void readanysf_dsp(t_readanysf *x, t_signal **sp) {
	int i=0;
	if (x->blocksize != sp[0]->s_n) {
		x->blocksize = sp[0]->s_n;

		x->tmp_audio_format.samplerate = sys_getsr();
		x->tmp_audio_format.sample_format = GAVL_SAMPLE_FLOAT ;
		x->tmp_audio_format.interleave_mode = GAVL_INTERLEAVE_NONE;
		x->tmp_audio_format.num_channels = x->num_channels;
		x->tmp_audio_format.channel_locations[0] = GAVL_CHID_NONE; // Reset
		x->tmp_audio_format.samples_per_frame = x->num_samples_per_frame;

		x->out_audio_format.samplerate = sys_getsr();
		x->out_audio_format.sample_format = GAVL_SAMPLE_FLOAT ;
		x->out_audio_format.interleave_mode = GAVL_INTERLEAVE_NONE;
		x->out_audio_format.num_channels = x->num_channels;
		x->out_audio_format.channel_locations[0] = GAVL_CHID_NONE; // Reset

		// leave enough room in our out format and frame for resampling
		x->out_audio_format.samples_per_frame = x->num_samples_per_frame * SRC_MAX +10;
		gavl_set_channel_setup (&x->out_audio_format); // Set channel locations
		
		if(x->out_audio_frame != NULL)
			gavl_audio_frame_destroy( x->out_audio_frame);
		x->out_audio_frame = gavl_audio_frame_create(&x->out_audio_format);
		//printf("created new out frame in readanysf_dsp\n");
  	post("pd blocksize=%d, spf=%d", x->blocksize, x->num_samples_per_frame);
	}

  for (i = 0; i < x->num_channels; i++)
	  x->x_outvec[i] = sp[i]->s_vec;
		    
  dsp_add(readanysf_perform, 1, x);
	
}

static void readanysf_free(t_readanysf *x) {
	// delete the readany objs

	if (x->in_audio_frame != NULL) gavl_audio_frame_destroy(x->in_audio_frame);
	if (x->tmp_audio_frame != NULL) gavl_audio_frame_destroy(x->tmp_audio_frame);
	if (x->out_audio_frame != NULL) gavl_audio_frame_destroy(x->out_audio_frame);
	
	if (x->i2t_audio_converter != NULL)
		gavl_audio_converter_destroy(x->i2t_audio_converter);
	
	if (x->t2o_audio_converter != NULL)
		gavl_audio_converter_destroy(x->t2o_audio_converter);

	pthread_mutex_destroy(&x->mut);

	delete x->rm;
	x->rm = NULL;
}

extern "C" void readanysf_tilde_setup(void) {

  readanysf_class = class_new(gensym("readanysf~"), (t_newmethod)readanysf_new,
  	(t_method)readanysf_free, sizeof(t_readanysf), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);

  class_addmethod(readanysf_class, (t_method)readanysf_dsp, gensym("dsp"), A_NULL);
  class_addmethod(readanysf_class, (t_method)m_open, gensym("open"), A_SYMBOL, A_NULL);
  class_addmethod(readanysf_class, (t_method)m_play, gensym("play"),  A_NULL);
  class_addmethod(readanysf_class, (t_method)m_pause, gensym("pause"),  A_NULL);
  class_addmethod(readanysf_class, (t_method)m_stop, gensym("stop"),  A_NULL);
  class_addmethod(readanysf_class, (t_method)m_tick, gensym("tick"), A_FLOAT, A_NULL);
  class_addmethod(readanysf_class, (t_method)m_speed, gensym("speed"), A_FLOAT, A_NULL);
  class_addmethod(readanysf_class, (t_method)m_loop, gensym("loop"), A_FLOAT, A_NULL);
  class_addmethod(readanysf_class, (t_method)m_pcm_seek, gensym("pcm_seek"), A_FLOAT, A_NULL);
  class_addmethod(readanysf_class, (t_method)m_time_seek, gensym("time_seek"), A_FLOAT, A_NULL);
  class_addbang(readanysf_class, readanysf_bang);

}
