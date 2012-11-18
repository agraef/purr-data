/*
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
 */

#include "ReadMedia.h"
#include <pthread.h>
#include <unistd.h> //usleep


static void *the_thread_dispatcher(void * xp);
static void *the_thread_opener(void * xp);
static void *the_audiofifo_filler(void * xp);
static void *the_videofifo_filler(void * xp);

 
ReadMedia::ReadMedia(  ) {

	m_state = STATE_EMPTY;

	m_audio_frame = NULL;
	m_video_frame = NULL;
	m_aeof = true;
	m_veof = true;
	//m_atime = 0.0;
	//m_vtime = 0.0;
	m_video_stream_count =0;
	m_audio_stream_count =0;
	
	m_pcm_seek = SEEK_NOTHING;
	m_frame_seek = SEEK_NOTHING;
	m_length_in_seconds=0.0;
	m_length_in_gavltime = 0;
	m_num_frames=0;
	m_num_samples=0;

	m_video_format.frame_width=0;
	m_video_format.frame_height=0;
	m_video_format.image_width=0;
	m_video_format.image_height=0;
	m_video_format.pixel_width=0;
	m_video_format.pixel_height=0;
	m_video_format.pixelformat = GAVL_PIXELFORMAT_NONE ;
	m_video_format.frame_duration=0;
	m_video_format.timescale=0;
	m_video_format.framerate_mode=GAVL_FRAMERATE_CONSTANT;
	m_video_format.chroma_placement=GAVL_CHROMA_PLACEMENT_DEFAULT;
	m_video_format.interlace_mode=GAVL_INTERLACE_NONE;
	m_video_format.timecode_format.int_framerate =0;
	m_video_format.timecode_format.flags =0;

	m_audio_format.samples_per_frame = 0;
	m_audio_format.samplerate = 0;
	m_audio_format.num_channels = 0;
	m_audio_format.sample_format = GAVL_SAMPLE_NONE ;
	m_audio_format.interleave_mode = GAVL_INTERLEAVE_NONE;
	m_audio_format.center_level = 1.0;
	m_audio_format.rear_level = 1.0;
	m_audio_format.channel_locations[0] = GAVL_CHID_NONE; // Reset
	

	m_fifoaudio= NULL; // new FifoAudioFrames( frames_in_fifo ,  &tmp_audio_lormat); 
	m_fifovideo= NULL; // new FifoVideoFrames( 12 ,  &output_video_format); 
	m_audio_thread_ret = -1;
	m_video_thread_ret = -1;
	
	m_open_callback = NULL;
	m_open_callback_data = NULL;
	m_buffer_callback = NULL;
	m_buffer_callback_data = NULL;

	quit_av_threads = false;
	m_loop = false;
	
	sprintf(m_filename, "seinettbitte!");

	//bgav stuff
	m_file = NULL;
	m_opt = bgav_options_create();

	bgav_options_set_connect_timeout(m_opt,  5000);
	bgav_options_set_read_timeout(m_opt,     5000);
	bgav_options_set_network_bandwidth(m_opt, 524300);
	bgav_options_set_network_buffer_size(m_opt, 1024*12);
	bgav_options_set_http_shoutcast_metadata (m_opt, 1);
	// set up the reading so that we can seek sample accurately
	bgav_options_set_sample_accurate (m_opt, 1 );


	pthread_cond_init(&m_cond_dispatch, 0);
	pthread_mutex_init(&m_condmut_dispatch, 0);

	pthread_cond_init(&m_cond_a, 0);
	pthread_cond_init(&m_cond_v, 0);
	pthread_mutex_init(&m_condmut_a, 0);
	pthread_mutex_init(&m_condmut_v, 0);

	pthread_mutex_init(&m_av_mut, 0);
	pthread_mutex_init(&m_state_mut, 0);

	// start the dispatcher thread
	m_cmd = CMD_START;
	m_dispatcher_thread_ret = pthread_create(&m_thread_dispatch, NULL, the_thread_dispatcher, (void *)this) ;
	if (m_dispatcher_thread_ret != 0 )
		printf("error starting the readmedia dispatcher thread.\n");

	while( getCommand() != CMD_NULL)	
		signalDispatcher();
	//printf("dispatcher ready ...\n");
}

ReadMedia::~ReadMedia() {
	printf("killing the media..\n");
	setCommand( CMD_QUIT );		
	signalDispatcher();
		

	// signal dispatcher joins the opener and AV threads	
	pthread_join( m_thread_dispatch, NULL);

	//printf("joined dispatcher\n");	
	if (m_audio_frame != NULL) {
		gavl_audio_frame_destroy(m_audio_frame);
	}
	if (m_video_frame != NULL) {
		gavl_video_frame_destroy(m_video_frame);
	}
	if (m_file != NULL) {
		bgav_close(m_file);
	}

	//printf("now, on to deleting fifo...\n");
	if( m_fifoaudio != NULL) delete m_fifoaudio;
	if( m_fifovideo != NULL) delete m_fifovideo;

	// these are created only once	
	bgav_options_destroy(m_opt);

	pthread_cond_destroy(&m_cond_dispatch);
	pthread_mutex_destroy(&m_condmut_dispatch);

	pthread_cond_destroy(&m_cond_a);
	pthread_cond_destroy(&m_cond_v);
	pthread_mutex_destroy(&m_condmut_a);
	pthread_mutex_destroy(&m_condmut_v);

	pthread_mutex_destroy(&m_av_mut);
	pthread_mutex_destroy(&m_state_mut);

	printf("killed the media..\n");
}

int ReadMedia::decodeVideo( gavl_video_frame_t * vf ) {

	// check state first, if state is ready, we can check the other vars without locking
	lockState();
	if (m_state != STATE_READY || m_video_stream_count < 1 || m_fifovideo == NULL ) {
		unlockState();
		return -1;
	}

	if (!m_fifovideo->Get( vf )  ) {
		if ( m_veof ) {
			m_frame_seek = SEEK_NOTHING;
			unlockState();
			signalV();
			return 0;
		} else {
			//printf("Couldn't get a video frame, videofifo is %f full\n", m_fifovideo->getSizePercentage()); // this can only happen if the fifo is empty
			unlockState();
			signalV();
			return -1; // return with error
		}
	}

	//m_vtime = 	vf->timestamp / (double)m_video_format.timescale;
	unlockState();
	signalV();
	return 1 ;
}

int ReadMedia::decodeAudio( gavl_audio_frame_t * af ) {
	lockState();
	if (m_state != STATE_READY || m_audio_stream_count < 1 || m_fifoaudio == NULL ) {
		unlockState();
		return -1;
	}

	if ( !m_fifoaudio->Get( af )  ) {
		if ( m_aeof ) {
			m_pcm_seek = SEEK_NOTHING;
			unlockState();
			signalA();
			return 0;
		} else {
			//printf("Couldn't get an audio frame, audiofifo is %f full.\n", m_fifoaudio->getSizePercentage()); // this can only happen if the fifo is empty
			unlockState();
			signalA();
			return -1;
		}
	}

	//m_atime = af->timestamp / (double)m_audio_format.samplerate;
	unlockState();
	signalA();
	return 1 ;
}

bool ReadMedia::rewind() {
	// NOTE!! Always check for stream count before setting aeof or veof
	lockState();
	if ( m_state == STATE_READY && m_file != NULL) {
		//printf("ReadMedia::rewind(), valid_samples=%d\n", m_audio_frame->valid_samples);
		m_pcm_seek = SEEK_REWIND;	
		if(m_audio_stream_count) m_aeof = false;
		//m_atime = 0;
		m_frame_seek=SEEK_REWIND;
		if (m_video_stream_count) m_veof = false;
		//m_vtime=0;

		unlockState();
		if (m_audio_stream_count) {
			signalA();  // only signal one or the other here
			//printf("ReadMedia::rewind(), signaled audio\n");
			// we are gong to flush here even though it is flushed
			// during the audio or video thread.  This will ensure our fifo 
			// is clean and empty.  Otherwise, the audio thread may be waiting
			// at the bottom of its loop and not make it to the flush part even 
			// though we signalled it.
			if (m_fifoaudio)
				m_fifoaudio->Flush();
		}
		else if (m_video_stream_count) {
			signalV();
			if (m_fifovideo)
				m_fifovideo->Flush();
		}

		return true;
	}
	unlockState();
	return false;
}

gavl_time_t ReadMedia::getLengthInGavlTime() {
	gavl_time_t time = 0;
	lockState();
	time = m_length_in_gavltime;
	unlockState();
	return time;
}

double ReadMedia::getLengthInSeconds() {
	double secs = 0.0;
	lockState();
	secs = m_length_in_seconds;
	unlockState();
	return secs;
}

int64_t ReadMedia::getLengthInAudioSamples() {
	int64_t samples = 0;
	lockState();
	samples = m_num_samples;
	unlockState();
	return samples;
}

int64_t ReadMedia::getLengthInVideoFrames() {
	int64_t frames = 0;
	lockState();
	frames = m_num_frames;
	unlockState();
	return frames;
}

bool ReadMedia::frameSeek( int64_t frames ) {
	lockState();
	if (m_state == STATE_READY && m_file && bgav_can_seek( m_file ) && frames >= 0 && frames < m_num_frames ) {
		m_frame_seek = frames;	
		unlockState();
		signalAV();
		return true;
	} else {
		m_frame_seek = SEEK_NOTHING;
		unlockState();
		return false;
	}
}

// NOT PUBLIC
int64_t ReadMedia::frameSeek() {
	int64_t tmp=SEEK_NOTHING;
	lockState();
	tmp = m_frame_seek;	
	m_frame_seek = SEEK_NOTHING;
	unlockState();
	return tmp;
}

bool ReadMedia::pcmSeek( int64_t samples ) {
	lockState();
	if (m_state == STATE_READY && m_file && bgav_can_seek( m_file) && samples >= 0 && samples < m_num_samples ) {
		m_pcm_seek = samples;	
		unlockState();
		signalAV();
		return true;
	} else {
		m_pcm_seek = SEEK_NOTHING;
		unlockState();
		return false;
	}
}

// NOT PUBLIC
int64_t ReadMedia::pcmSeek() {
	int64_t tmp=SEEK_NOTHING;
	lockState();
	tmp = m_pcm_seek;	
	m_pcm_seek = SEEK_NOTHING;
	unlockState();
	return tmp;
}

bool ReadMedia::timeSeek(double seconds) {
	gavl_time_t gt = gavl_seconds_to_time(  seconds ) ;        

	lockState();
	if (m_state == STATE_READY && m_file && bgav_can_seek( m_file) && seconds >= 0.0 && seconds < m_length_in_seconds ) {
		if (m_audio_stream_count) {
			m_pcm_seek = gavl_time_to_samples(m_audio_format.samplerate, gt );
			if (m_pcm_seek >= m_num_samples || m_pcm_seek < 0) 
				m_pcm_seek = SEEK_NOTHING;
			unlockState();
			signalAV();
			return true;	
		} else if ( m_video_stream_count  && m_video_format.framerate_mode == GAVL_FRAMERATE_CONSTANT ) {
			m_frame_seek =	gavl_time_to_frames( m_video_format.timescale, m_video_format.frame_duration,  gt );
			if (m_frame_seek >= m_num_frames || m_frame_seek < 0 )
				m_frame_seek = SEEK_NOTHING;
			unlockState();
			signalAV();
			return true;
		}
	}
	unlockState();
	return false;
}

bool ReadMedia::quitAVThreads() {
	bool b =false;
	lockState();
	b = quit_av_threads;
	unlockState();
	return b;
}

void ReadMedia::openFile( char * fn, int vsize, int asize, int spf) {
	lockState();
	/*
	if (  strcmp(m_filename, fn) == 0  && m_state == STATE_READY) {
		printf("%s is already open for action. \n", m_filename);
		unlockState();
		callOpenCallback();
		return;
	}
	*/
	// signal the dispatcher that we want an new file
	m_audio_format.samples_per_frame = spf ;
	m_afifosize = asize;
	m_vfifosize = vsize;
	sprintf(m_filename, "%s", fn);

	m_cmd = CMD_OPEN ;

	unlockState();
	signalDispatcher();
}

void ReadMedia::copyAudioFormat(gavl_audio_format_t * dst ){ 
	lockState();
	//if (m_state == STATE_READY)
	gavl_audio_format_copy(dst, &m_audio_format);
	unlockState();
}

void ReadMedia::copyVideoFormat(gavl_video_format_t * dst ){ 
	lockState();
	//if (m_state == STATE_READY)
	gavl_video_format_copy(	dst, &m_video_format);
	unlockState();
}

int ReadMedia::getAudioSamplerate() { 
	int sr=0;  
	lockState();
	//if (m_state == STATE_READY )
		sr = m_audio_format.samplerate;
	unlockState();
	return sr;
}

int ReadMedia::getAudioChannelCount() { 
	int ch=0;
	lockState();	
	//if (m_state == STATE_READY )
		ch = m_audio_format.num_channels;
	unlockState();
	return ch;
}

int ReadMedia::getVideoTimescale() { 
	int t=0;
	lockState();	
	t = m_video_format.timescale;
	unlockState();
	return t;
}


char * ReadMedia::getFilename() {
	return  m_filename;
}

bgav_t * ReadMedia::getFile() {return  m_file;}
FifoAudioFrames * ReadMedia::getAudioFifo() { return m_fifoaudio; } 
FifoVideoFrames * ReadMedia::getVideoFifo() { return m_fifovideo; } 
gavl_audio_frame_t * ReadMedia::getAudioFrame() { return m_audio_frame;}
gavl_video_frame_t * ReadMedia::getVideoFrame() { return m_video_frame;}


void ReadMedia::setState(int b) { 
	lockState(); 
	m_state = b; 
	unlockState();
}

int ReadMedia::getState() { 
	int s=STATE_EMPTY;
	lockState();
	s = m_state;
	unlockState();
	return s;
}

bool ReadMedia::isReady() { if ( getState() == STATE_READY) return true; else return false;}
// no need to lock on these
//double ReadMedia::getATimeInSeconds() { return m_atime;};
//double ReadMedia::getVTimeInSeconds() { return m_vtime;};
/*
float ReadMedia::getTimeInSeconds() { 
	lockState();
	if (m_audio_stream_count > 0 ) {
		unlockState();
		return m_atime;
	} else {
		// FIXME :  see if the following is really true
		unlockState();
		return m_vtime;
	}
};
*/

float ReadMedia::getAudioFifoSizePercentage() { 
	float f=0.0;
	lockState();
	if (m_fifoaudio)	
		f = m_fifoaudio->getSizePercentage();
	unlockState();
	return f;
}

void ReadMedia::pealOffVideoFrames( int howmany) { 
	lockAV();
	if (m_fifovideo == NULL) {
		unlockAV();
		return;
	}
	int max = howmany > m_fifovideo->getSize() ? m_fifovideo->getSize() : howmany;
	for (int i=0;i< max; i++) {
		m_fifovideo->Get();
		//printf("pealing of a video frame size = %d\n", m_fifovideo->getSize());
	}
	unlockAV();
} 

void ReadMedia::setAEOF(bool b) { 
	lockState();
	m_aeof = b;
	m_pcm_seek = SEEK_NOTHING;
	unlockState();
}
void ReadMedia::setVEOF(bool b) { 
	lockState();
	m_veof = b;
	m_frame_seek = SEEK_NOTHING;
	unlockState();
}


bool ReadMedia::getEOF() { 
	bool tmp = true;
	lockState();
	if (m_state == STATE_READY)
		tmp = (m_aeof && m_veof);
	unlockState();
	//printf("ReadMedia:getEOF=%d, aeof=%d, veof=%d\n", tmp, m_aeof, m_veof);
	return tmp;
}

void ReadMedia::setLoop( bool b) {
	lockState();
	m_loop = b;	
	unlockState();
}
bool ReadMedia::getLoop() {
	bool tmp = true;
	lockState();
	if (m_file && bgav_can_seek( m_file) ) {
		tmp = m_loop;	
	} else {
		tmp = false; // we can't loop on file, return false 
		// but leave the loop var untouched
	}
	unlockState();
	return tmp;
}

// NOT PUBILC
// only used in fifo filler
bool ReadMedia::getAEOF() {
	bool tmp = false;
	lockState();
	tmp = m_aeof;
	unlockState();
	return tmp;
}

// NOT PUBLIC
bool ReadMedia::getVEOF() { 
	bool tmp = false;
	lockState();
	tmp =  m_veof;
	unlockState();
	return tmp;
}

int ReadMedia::getSamplesPerFrame() { 
	int spf=0;
	lockState();
	spf = m_audio_format.samples_per_frame;
	unlockState();
	return spf;
}

int ReadMedia::lockAV() { 
	//printf("locking AV\n"); 
  return pthread_mutex_lock(&m_av_mut);
}
int ReadMedia::unlockAV() { 
	//printf("unlocking AV\n");
	return pthread_mutex_unlock(&m_av_mut);
}

int ReadMedia::lockState() { 
	//	printf("locking state.\n"); 
	return pthread_mutex_lock(&m_state_mut);
}
int ReadMedia::unlockState() { 
	//printf("unlocking state.\n"); 
	return pthread_mutex_unlock(&m_state_mut);
}


void ReadMedia::waitA() { pthread_cond_wait( &m_cond_a, &m_condmut_a); }
void ReadMedia::waitV() { pthread_cond_wait( &m_cond_v, &m_condmut_v); }
void ReadMedia::signalAV() { signalA(); signalV(); }
void ReadMedia::signalA() { pthread_cond_signal( &m_cond_a); }
void ReadMedia::signalV() { pthread_cond_signal( &m_cond_v); }

void ReadMedia::signalDispatcher() { pthread_cond_signal( &m_cond_dispatch); }
void ReadMedia::waitDispatch() { pthread_cond_wait( &m_cond_dispatch, &m_condmut_dispatch); }

void ReadMedia::setOpenCallback(void (*oc)(void *), void *v ) {
	lockState();// do we need this?
	m_open_callback = oc; 
	m_open_callback_data = v;  
	unlockState();
}	
void ReadMedia::callOpenCallback() {	
	// do NOT lock on the callback, user must do call public functions
	// that also lock
	if(m_open_callback != NULL) 
		m_open_callback( m_open_callback_data);
 };

void ReadMedia::setBufferCallback(bgav_buffer_callback bc, void *v ) { 
	lockState();
	m_buffer_callback = bc; 
	m_buffer_callback_data = v;  
	// set up callbacks.
	if (m_buffer_callback) {
		bgav_options_set_buffer_callback( m_opt, m_buffer_callback, m_buffer_callback_data);
	}
	unlockState();
};




int ReadMedia::getAudioStreamCount() {
	int asc=0;
	lockState();
	asc = m_audio_stream_count;
	unlockState();
	return asc;
} 
int ReadMedia::getVideoStreamCount() {
	int vsc=0;
	lockState();
	vsc = m_video_stream_count;
	unlockState();
	return vsc;
}


// NOT PUBLIC
void ReadMedia::setAudioStreamCount(int s){
	lockState();
	m_audio_stream_count=s;
	if (s == 0) // no audio streams, we are already at audio eof
		m_aeof = true;
	else m_aeof = false;
	unlockState();
}
// NOT PUBLIC
void ReadMedia::setVideoStreamCount(int s){
	lockState();
	m_video_stream_count=s; 
	if (s==0) // if there are no video streams, video is at eof
		m_veof = true; 
	else m_veof =false;
	unlockState();
}

// NOT PUBLIC
void ReadMedia::setCommand(int s){
	lockState();
	m_cmd = s;
	unlockState();
}

// NOT PUBLIC
int ReadMedia::getCommand() {
	int cmd= CMD_NULL;
	lockState();
	cmd = m_cmd;
	unlockState();	
	return cmd;
}


// NOT PUBLIC
// Clears the bgav_t struct, destroying and then creating it
// NO NEED TO DO AV LOCK, only called in the opener thread
void ReadMedia::clearFile() {
	if (m_file != NULL)
		bgav_close( m_file );

	m_file = bgav_create();
	bgav_options_copy( bgav_get_options( m_file ) , m_opt);
	m_aeof = true;
	m_veof = true;
	m_pcm_seek = SEEK_NOTHING;
	m_frame_seek = SEEK_NOTHING;
}

// NOT PUBLIC
// Closes the bgav_t struct, destroying it and setting to NULL
// NO NEED TO DO AV LOCK, only called in the opener thread
void ReadMedia::closeFile() {
	if (m_file != NULL)
		bgav_close( m_file );
	m_file = NULL;
	m_aeof = true;
	m_veof = true;
	m_pcm_seek = SEEK_NOTHING;
	m_frame_seek = SEEK_NOTHING;

	sprintf(m_filename, "seinettbitte!");
}


// NOT PUBLIC
// killAVThreads is only called from the opener thread
// and from the destructor
// THIS IS NOT REALLY A PUBLIC function
void ReadMedia::killAVThreads() {

	lockState();
	m_state = STATE_EMPTY;
	quit_av_threads = true;
	unlockState();

	signalAV();
	signalAV();

	// FIRST need to join the threads that exist and are running
	if (m_audio_thread_ret == 0) {
		pthread_join( m_thread_fillaudiofifo, NULL);
		//pthread_detach( m_thread_fillaudiofifo);
	}
	if (m_video_thread_ret == 0){
		pthread_join( m_thread_fillvideofifo, NULL);
		//pthread_detach( m_thread_fillvideofifo);
	}
	m_audio_thread_ret = -1;
	m_video_thread_ret = -1;

	// no need to lock here 	
	quit_av_threads = false;

}

// NOT PUBLIC
// Only called from the opener thread
bool ReadMedia::startAVThreads() {

	if (m_audio_thread_ret == 0 || m_video_thread_ret == 0 ) {
		// ouch!, we have running AV threads, this is not good	
		return false;
	}

	if (m_audio_stream_count > 0) {
		m_audio_thread_ret = pthread_create(&m_thread_fillaudiofifo, NULL, the_audiofifo_filler, (void *)this);
		if (m_audio_thread_ret != 0 ) {
			printf("ReadMedia:: problem starting the audio thread\n");
			return false;
		}
	}

	if (m_video_stream_count > 0) {
		m_video_thread_ret =	pthread_create(&m_thread_fillvideofifo, NULL, the_videofifo_filler, (void *)this);
	 if (m_video_thread_ret != 0 ) {
		 printf("ReadMedia::  problem starting the video thread\n");
		 return false;	
		}
	}
	return true;
}
	
bool ReadMedia::initFormat() {

	const gavl_audio_format_t * open_audio_format;
	const gavl_video_format_t * open_video_format;

	// we use the m_vfifosize to see if the user app wants video or not
	// then, we set m_video_stream_count to 0 if he doesn't want video
	if (m_video_stream_count > 0 && m_vfifosize > 0) {
		open_video_format = bgav_get_video_format(m_file, 0);

		if (open_video_format->pixelformat == GAVL_PIXELFORMAT_NONE) {
			printf("!!!sorry, pixelformat is not recognized.\n");
			return false;
		}

		// let's check to see if the formats are the same, if they are the same
		// there is no reason to recreate the fifo or frames
		if ( gavl_video_formats_equal( &m_video_format, open_video_format) == 0 ) { 	
			// the formats are different
			gavl_video_format_copy (&m_video_format, open_video_format);
			if (m_video_frame != NULL)
				gavl_video_frame_destroy(m_video_frame);
			m_video_frame = gavl_video_frame_create(&m_video_format);
			gavl_video_frame_clear( m_video_frame, &m_video_format);
			if (m_fifovideo != NULL)
				delete m_fifovideo;
			m_fifovideo=  new FifoVideoFrames( m_vfifosize ,  &m_video_format); 
		}
	} else {
		m_video_stream_count = 0;
		m_veof = true;
	}

	// we use the m_afifosize to see if the user app wants audio or not
	// then, we set m_audio_stream_count to 0 if he doesn't want audio
	if (m_audio_stream_count > 0 && m_afifosize > 0) {  
		open_audio_format = bgav_get_audio_format(m_file, 0);    
	
		// we can get audio formats that are unkown
		if ( open_audio_format->sample_format == GAVL_SAMPLE_NONE) {
			printf("sorry, this file has unsupported audio.\n"); 
			return false;	
		}

		if ( gavl_audio_formats_equal(&m_audio_format, open_audio_format) == 0 ) { 	
			// audio formats are different
			// save the old spf
			int spf = m_audio_format.samples_per_frame; 
			gavl_audio_format_copy(&m_audio_format, open_audio_format);

			if (m_audio_frame != NULL) {
				gavl_audio_frame_destroy(m_audio_frame);
			}

			// set it back to original
			m_audio_format.samples_per_frame = spf ;

			m_audio_frame = gavl_audio_frame_create(&m_audio_format);
	
			gavl_audio_frame_mute( m_audio_frame, &m_audio_format);
			if( m_fifoaudio != NULL )
				delete m_fifoaudio;
			m_fifoaudio = new FifoAudioFrames( m_afifosize , &m_audio_format); 
		}
	} else {
		// user doesn't want audio
		m_audio_stream_count = 0;
		m_aeof=true;
	}


	m_length_in_gavltime = bgav_get_duration ( m_file, 0);;
	m_length_in_seconds = gavl_time_to_seconds(  m_length_in_gavltime );
	m_num_samples = 0;
	m_num_frames = 0;

	if (m_audio_stream_count) {
		if ( bgav_can_seek_sample(m_file) == 1 ) {
			m_num_samples=	bgav_audio_duration ( m_file, 0) ;
	 } else { 
			m_num_samples=	gavl_time_to_samples( m_audio_format.samplerate ,  bgav_get_duration ( m_file, 0) );
		}
	}

	// set frames   WE NEED TO take care here for non-constant frame-rates
	if(m_video_stream_count) {
		if ( bgav_can_seek_sample(m_file) == 1  && m_video_format.framerate_mode == GAVL_FRAMERATE_CONSTANT) { 
			m_num_frames =	bgav_video_duration ( m_file, 0)/ m_video_format.frame_duration;
		} else if ( bgav_can_seek_sample(m_file) == 1  && m_video_format.framerate_mode == GAVL_FRAMERATE_VARIABLE ) {
			// FIXME what to do with non constant frame rates?
			m_num_frames=0;
		} else { 
			m_num_frames =	gavl_time_to_frames( m_video_format.timescale, m_video_format.frame_duration ,  bgav_get_duration ( m_file, 0) );
		}
	}

  //	printf("m_num_frames =%lld, duration = %lld , vid_duration=%lld\n", 
	//		m_num_frames, bgav_get_duration ( m_file, 0),  bgav_video_duration ( m_file, 0) );
	// set seconds
	if ( bgav_can_seek_sample(m_file) == 1) {
		gavl_time_t atime=0,vtime=0;
		if ( m_audio_stream_count ) 
			atime =  gavl_samples_to_time( m_audio_format.samplerate, m_num_samples );
		if (m_video_stream_count &&  m_video_format.frame_duration > 0) {
			vtime =  gavl_frames_to_time( m_video_format.timescale, m_video_format.frame_duration, m_num_frames );
		} else if ( m_video_stream_count  ) { // non constant framerate			
			vtime = bgav_video_duration( m_file, 0);
		}
		// else rely on audio time
		m_length_in_gavltime = atime > vtime ? atime :vtime;
		m_length_in_seconds = gavl_time_to_seconds( m_length_in_gavltime );
		//printf("atime=%ld,  vtime=%ld, l_in_sec=%f\n", atime, vtime, m_length_in_seconds);
	} 

	m_pcm_seek = SEEK_NOTHING;
	m_frame_seek = SEEK_NOTHING;

	return true;
}


void *the_thread_dispatcher(void *xp) {

	ReadMedia *rm = (ReadMedia *)xp;
	int cmd = CMD_NULL;
	pthread_t thread_open;
	int start_thread_ret = -1;
	cmd = rm->getCommand();

	while ( cmd != CMD_QUIT ) {
		if (cmd == CMD_OPEN) {
			// We already check in the openFile function if the user is trying to open
			// a file that is already open.

			// join the opener thread, this will protect again any other calls to open
			if (start_thread_ret == 0 )
				pthread_join( thread_open, NULL ); 
			
			// we join the AV threads
			// the opener thread will start the AV threads anew upon success
			rm->killAVThreads();

			start_thread_ret = pthread_create(&thread_open, NULL, the_thread_opener, (void *)rm) ;
			if (start_thread_ret != 0 )
				printf( "Failed to create m_thread_open thread.\n");

		}	
		if (rm->getCommand() == CMD_QUIT)
			break;
		rm->setCommand( CMD_NULL);
		rm->waitDispatch();
		cmd = rm->getCommand();
	}
	
	//printf("dispatcher: joining thread open\n");
	if( start_thread_ret == 0 )
		pthread_join( thread_open , NULL); 
	//printf("dispatcher: joined thread open, joining AV's\n");
	rm->killAVThreads();
	//printf("dispatcher: joined  AV's\n");
	pthread_exit(NULL);	
};

void *the_thread_opener(void *xp) {
	ReadMedia *rm = NULL;
	int num_urls=0, num_tracks=0, audio_stream_count=0, video_stream_count =0;

	rm = (ReadMedia *)xp;
	rm->setState(STATE_OPENING);

	//  AFTER WE KILL THE THREADS, THERE IS NO NEED TO LOCK AV mutex
	//  ALL functions that want to get info on the file, seek, 
	//  or decode a frame, MUST first check the state.  If the
	//  state is STATE_READY, then it can perform it's function, 
	//  locking on the necessary variables that might conflict with
	//  the AV.   

	// clearFile  deletes old file and creates new File
	rm->clearFile();
	if(!bgav_open(rm->getFile(), rm->getFilename())) {
		printf( "Could not open file %s\n", rm->getFilename());
		rm->setState( STATE_EMPTY );
		rm->closeFile();
		rm->callOpenCallback();
		pthread_exit(NULL);
		//return NULL;
	} else {
		printf("opened %s\n", rm->getFilename());
	}

	// check to see if it is a redirector
	if(bgav_is_redirector( rm->getFile() )) {
		num_urls = bgav_redirector_get_num_urls( rm->getFile() );
		printf( "Found redirector with %d urls inside, we will try to use the first one.\n", num_urls);
		printf( "Name %d: %s\n", 1, bgav_redirector_get_name(rm->getFile() , 0));
		printf("URL %d: %s\n",  1, bgav_redirector_get_url(rm->getFile(), 0));
		sprintf(rm->getFilename(), "%s", bgav_redirector_get_url(rm->getFile(), 0) );
		rm->clearFile();
		if (!bgav_open( rm->getFile(),  rm->getFilename() )) {
			printf("Could not open redirector\n");
			rm->setState( STATE_EMPTY );
			rm->closeFile();
			rm->callOpenCallback();
			pthread_exit(NULL);
			//return NULL;
		} else {
			printf("opened redirector %s\n", rm->getFilename());
		}
	}
	
	num_tracks = bgav_num_tracks(rm->getFile());
	if ( num_tracks ) {
		bgav_select_track(rm->getFile(), 0);
	} else {
		printf("No tracks associated with file:%s\n", rm->getFilename() );
		rm->setState( STATE_EMPTY );
		rm->closeFile();
		rm->callOpenCallback();
		pthread_exit(NULL);
	}

	audio_stream_count = bgav_num_audio_streams(rm->getFile(), 0);
	if( audio_stream_count )
		bgav_set_audio_stream(rm->getFile(), 0, BGAV_STREAM_DECODE);

	video_stream_count = bgav_num_video_streams(rm->getFile(), 0);
	if( video_stream_count )
		bgav_set_video_stream(rm->getFile(), 0, BGAV_STREAM_DECODE);
	
	rm->setVideoStreamCount(video_stream_count);
	rm->setAudioStreamCount(audio_stream_count);
	
	//printf("astream_count = %d, vstream_count=%d\n", audio_stream_count, video_stream_count);
	if(!bgav_start(rm->getFile())) {
		printf( "failed to start file\n");
		rm->setState( STATE_EMPTY );
		rm->closeFile();
		rm->callOpenCallback();
		pthread_exit(NULL) ;
		//return NULL;
	}

	if( !rm->initFormat() ){
		rm->setState( STATE_EMPTY );
		rm->closeFile();
		rm->callOpenCallback();
		pthread_exit(NULL) ;
	}

	if( !rm->startAVThreads() ){
		rm->setState( STATE_EMPTY );
		rm->closeFile();
		rm->callOpenCallback();
		pthread_exit(NULL) ;
	}

	// AV threads are now running, blocking will be necessary
	// STATE_READY and callOpenCallback is set/called in the 
	// fifo fill callbacks
	rm->signalAV();
	rm->signalAV(); //extra signal for second thread
	pthread_exit(NULL);
	//return NULL;
}

void *the_audiofifo_filler( void * xp) {
	int samples_returned=0;
	ReadMedia *rm = (ReadMedia *)xp;
	int first = true;
	int dovideo = rm->getVideoStreamCount();	
	int spf = rm->getSamplesPerFrame();
	int samplerate = rm->getAudioSamplerate();
	int64_t seekto = SEEK_NOTHING;
	int can_seek = bgav_can_seek ( rm->getFile() );
	int can_seek_sample = bgav_can_seek_sample ( rm->getFile() );
	
	while ( !rm->quitAVThreads() ) {
		//while ( rm->getAudioFifo() != NULL && rm->getAudioFifo()->FreeSpace() && !rm->getAEOF() ) {
		while ( rm->getAudioFifo()->FreeSpace() && !rm->getAEOF() ) {

			if (rm->quitAVThreads() ) pthread_exit(NULL) ; //return NULL;
			
			rm->lockAV();
			// check to see if we need to seek
			// if this is set, we already know we can seek on this file 
			// and don't need to check with bgav_can_seek
			if ( can_seek ) {
				seekto = rm->pcmSeek();
				if (  seekto != SEEK_NOTHING ) {
					if ( seekto == SEEK_REWIND)  {
						// bgav_seek_audio ONLY seeks on the audio stream
						seekto = 0;
					//	bgav_seek_scaled(rm->getFile(), &seekto, samplerate);
					} //else {
					bgav_seek_scaled(rm->getFile(), &seekto, samplerate);
					//}
					rm->getAudioFifo()->Flush();
					if (dovideo && rm->getVideoFifo() ) {
						 rm->getVideoFifo()->Flush();
						 rm->signalV();
					}
				}
			}

			samples_returned = bgav_read_audio(rm->getFile(), rm->getAudioFrame(), 0,  spf );
			//rm->unlockAV();

			if (samples_returned == 0 ) {
				if( rm->getLoop() ) {
					if ( can_seek ) {
						// Now, rewind the file, don't flush the fifo's
						if (can_seek_sample) { // only seek on audio stream
							bgav_seek_audio(rm->getFile(), 0, 0);
						} else { 
							seekto =0;
							bgav_seek_scaled(rm->getFile(), &seekto, samplerate);
							if (dovideo && rm->getVideoFifo() ) {
								rm->setVEOF(false);
								rm->signalV();
							}
						}	
						// SAVE THIS FOR ANOTHER TIME, OVERLAPPED SMOOTH LOOPING 
						//if ( rm->getLoop() > 1 && bgav_read_audio(rm->getFile(), rm->getAudioFrame(), 0,  spf ) ) {
						// add to the fifo, overlapping
						//		rm->getAudioFifo()->AppendOverlap( rm->getAudioFrame(), 5 );
						//	}	
						
					} else { // this file is not seekable, what do we do?
						printf("cannot seek on file, but we want to loop. setting end of file.\n");
						rm->setAEOF(true);
					}
				} else {
					rm->setAEOF(true);
				}
				rm->unlockAV();
				break;
			}
			rm->unlockAV();
			if( !rm->getAudioFifo()->Append( rm->getAudioFrame() ))
				printf("problem with appending Audio Frame\n");
		}
		if (first && !dovideo) {
			rm->setState( STATE_READY );
			rm->callOpenCallback();
			first = false;
		}
		if (rm->quitAVThreads() ) pthread_exit(NULL); //return NULL;
		rm->waitA();
	}
	pthread_exit(NULL);//return NULL;
}

void *the_videofifo_filler( void * xp) {
	ReadMedia *rm = (ReadMedia *)xp;
	int ret = 0;
	int first = true;
	int doaudio = rm->getAudioStreamCount();	
	int64_t seekto =SEEK_NOTHING;
	int can_seek = bgav_can_seek ( rm->getFile() );
	int can_seek_sample = bgav_can_seek_sample ( rm->getFile() );
	int timescale = rm->getVideoTimescale();
	
	while (!rm->quitAVThreads() ) {
	
		while ( rm->getVideoFifo() !=NULL && rm->getVideoFifo()->FreeSpace() && !rm->getVEOF() ) {

			if (rm->quitAVThreads() ) pthread_exit(NULL);//return NULL;

			rm->lockAV();
			// check to see if we need to seek
			// if this is set, we already know we can seek on this file 
			// and don't need to check with bgav_can_seek
			if (can_seek) {
				seekto = rm->frameSeek();
				if ( seekto  >= 0 ) {  
					if (doaudio && rm->getAudioFifo() ) rm->getAudioFifo()->Flush();
					rm->getVideoFifo()->Flush();
					bgav_seek_scaled(rm->getFile(), &seekto, timescale);
				} else if (seekto == SEEK_REWIND && !doaudio) {
					// rewind is a special case  for Video.
					rm->getVideoFifo()->Flush();
					seekto =0;
					bgav_seek_scaled(rm->getFile(), &seekto, timescale);
				}
			}
				
			ret = bgav_read_video(rm->getFile(), rm->getVideoFrame(), 0 );

			if ( !ret ) {
				// only loop from video if there is no audio
				// audio controls loop timing
				if ( rm->getLoop() ){
					if ( can_seek ) {
						if (doaudio && can_seek_sample) {
							// seek on video stream only if we have audio
							// audio and video seeking are separate in sample accurate 
							// seeking.  If no sample accurate seeking, then let audio
							// handle seeking
							bgav_seek_video(rm->getFile(), 0,0);
						} else if (!doaudio) {	
							// if we don't have audio...just seek to 0
							seekto=0;
							bgav_seek_scaled(rm->getFile(), &seekto, timescale);
						}
					} else { // if (can_seek)
						printf ("We want to loop video, but we cannot seek on this video stream,setting VEOF\n");
						rm->setVEOF(true);
					}
				} else {
					rm->setVEOF(true);
				}
				rm->unlockAV();
				break;
			} 
			rm->unlockAV();
			if( !rm->getVideoFifo()->Append( rm->getVideoFrame() ))
				printf("problem with appending VideoFrame\n");
		}
		// on the first time 'round we will call the open callback
		// if there is no video in the file, the audio will handle it.
		if (first) {
			rm->setState( STATE_READY );
			rm->callOpenCallback();
			first = false;
		}

		if (rm->quitAVThreads() ) pthread_exit(NULL); //return NULL;
		rm->waitV();
	}
	pthread_exit(NULL); //return NULL;
}


