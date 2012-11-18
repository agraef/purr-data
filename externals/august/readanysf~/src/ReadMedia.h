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

#ifndef _READMEDIA_H_
#define _READMEDIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "FifoAudioFrames.h"
#include "FifoVideoFrames.h"


#ifndef _AVDEC_H_
#define _AVDEC_H_
extern "C" {
#include <avdec.h>
}
#endif


#define STATE_EMPTY              0
#define STATE_OPENING           1
#define STATE_READY            2

#define SRC_MAX                 256.0
#define SRC_MIN                 1/256.0

#define CMD_START	3
#define CMD_NULL	0
#define CMD_OPEN 1
#define CMD_QUIT 2

#define SEEK_NOTHING -1
#define SEEK_REWIND -2

class ReadMedia  {

	public:
		ReadMedia();
		~ReadMedia();

		void openFile( char * filename, int vfifosize, int afifosize, int samples_per_frame);

		int decodeAudio( gavl_audio_frame_t *af);
		int decodeVideo( gavl_video_frame_t *vf);
	
		bool isReady(); // see if file is loaded or not
		bool getEOF();

		bool rewind();
		bool rewindNoFlush();
		bool frameSeek(int64_t frames);
		bool pcmSeek(int64_t samples);
		bool timeSeek(double seconds);

		int getAudioSamplerate();
		int getAudioChannelCount();
		int getVideoTimescale();

		double getLengthInSeconds(); 
		gavl_time_t getLengthInGavlTime(); 
		int64_t getLengthInAudioSamples();
		int64_t getLengthInVideoFrames();

		int getAudioStreamCount();
		int getVideoStreamCount();

		// this is used to sync AV frames
		void pealOffVideoFrames(int howmany);

		void dump() { lockState(); if (m_file != NULL) bgav_dump(m_file); unlockState(); }; 
	
		double getATimeInSeconds();
		double getVTimeInSeconds();
		//float getTimeInSeconds();
		float getAudioFifoSizePercentage();

		void copyAudioFormat(gavl_audio_format_t * dst ); 
		void copyVideoFormat(gavl_video_format_t * dst ); 
		void setLoop( bool b); 
		bool getLoop(); 


		//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		// NON-PUBLIC public functions
		//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||

		int64_t pcmSeek();
		int64_t frameSeek();
		//bool seek(gavl_time_t gt);

		void killAVThreads();
		bool startAVThreads();
		void setAudioStreamCount(int s);
		void setVideoStreamCount(int s);

		FifoAudioFrames * getAudioFifo(); 
		FifoVideoFrames * getVideoFifo(); 

		bgav_t * getFile();
		char * getFilename();

		gavl_audio_frame_t * getAudioFrame();
		gavl_video_frame_t * getVideoFrame();
		
		void closeFile();
		void clearFile();

		void setState(int b);
		int getState();
		gavl_audio_format_t * getAudioFormat() { return &m_audio_format;}; 
		gavl_video_format_t * getVideoFormat() { return &m_video_format;}; 

		void setCommand( int c);
		int getCommand();

		void setAEOF(bool b);
		void setVEOF(bool b);
		bool getAEOF();
		bool getVEOF();

		int getSamplesPerFrame();

		int lockAV();
		int unlockAV();
		//int lockAV(const char *s);
		//int unlockAV(const char *s);
		int lockState();
		int unlockState();
		//int lockState(const char *s);
		//int unlockState(const char *s);


		void waitA();
		void waitV();
		void signalAV();
		void signalA();
		void signalV();
		void waitDispatch();
		void signalDispatcher();
	
		bool initFormat();

		void setOpenCallback(void (*oc)(void *), void *v );
		void callOpenCallback();

		void setBufferCallback( bgav_buffer_callback bc, void *v );
		
		bool quitAVThreads();

	private:
		bool quit_av_threads;
		// callbacks for open, input buffer etc.	
		void * m_open_callback_data;	
		void (* m_open_callback)(void * v);
		bgav_buffer_callback m_buffer_callback;
		void * m_buffer_callback_data;			

		// end of file for audio and video
		bool m_aeof;
		bool m_veof;
	
		// seek vars
		int64_t m_pcm_seek;
		int64_t m_frame_seek;
		
		double m_length_in_seconds;
		gavl_time_t m_length_in_gavltime;
		int64_t m_num_samples;
		int64_t m_num_frames;

		// command for dispatcher thread
		int m_cmd;

		// internal state of media reader and 
		// current filename
		int m_state;
		char m_filename[1024];
		bool m_loop;

	
		// current time of audio in seconds
		//double m_atime;
		//double m_vtime;

		int m_afifosize;
		int m_vfifosize;
		int m_audio_stream_count;
		int m_video_stream_count;
		
		bgav_t * m_file;
		bgav_options_t * m_opt;

		// audio stuff
		gavl_audio_frame_t * m_audio_frame;
		gavl_audio_format_t m_audio_format;
	
		//video stuff 
		gavl_video_frame_t * m_video_frame;
		gavl_video_format_t m_video_format;

		FifoAudioFrames  *m_fifoaudio;
		FifoVideoFrames  *m_fifovideo;

		int m_audio_thread_ret;
		int m_video_thread_ret;
		int m_dispatcher_thread_ret;

		pthread_t m_thread_fillaudiofifo;
		pthread_t m_thread_fillvideofifo;
		pthread_t m_thread_dispatch;

		pthread_mutex_t m_condmut_a;
		pthread_mutex_t m_condmut_v;
		pthread_mutex_t m_condmut_dispatch;

		pthread_mutex_t m_state_mut;
		pthread_mutex_t m_av_mut;

		pthread_cond_t m_cond_a;
		pthread_cond_t m_cond_v;
		pthread_cond_t m_cond_dispatch;
		
};

#endif
