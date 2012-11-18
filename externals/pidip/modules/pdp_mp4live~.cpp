/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon (ydegoyon@free.fr )
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*  This object is a mpeg4ip streaming object towards a Darwin or Quicktime streaming server
 *  A lot of this object code is inspired by the code from mpeg4ip
 *  Copyright (c) 2000, 2001, 2002 Dave Mackie, Bill May & others
 *  The rest is written by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>

/* mpeg4ip includes taken from the source tree ( not exported ) */
#include <mp4.h>
#define  DECLARE_CONFIG_VARIABLES
#include "config_set.h"

#undef CONFIG_BOOL
#define CONFIG_BOOL(var, name, defval) \
 { &(var), (name), CONFIG_TYPE_BOOL, (defval), (defval) }
#undef CONFIG_FLOAT
#define CONFIG_FLOAT(var, name, defval) \
 { &(var), (name), CONFIG_TYPE_FLOAT,(float) (defval), (float) (defval) }
#undef CONFIG_INT
#define CONFIG_INT(var, name, defval) \
 { &(var), (name), CONFIG_TYPE_INTEGER,(config_integer_t) (defval), (config_integer_t)(defval) }
#undef CONFIG_STRING
#define CONFIG_STRING(var, name, defval) \
 { &(var), (name), CONFIG_TYPE_STRING, (defval), (defval) }

#include "pdp_mp4config.h"

#undef   DECLARE_CONFIG_VARIABLES
#ifndef debug_message
#define debug_message post 
#endif
#include "rtp_transmitter.h"
#include "pdp_mp4videosource.h"
#include "pdp_mp4audiosource.h"

#define VIDEO_BUFFER_SIZE (1024*1024)
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)
#define AUDIO_PACKET_SIZE (2*1024*2) /* using aac encoding, 2 channels, 2 bytes per sample */

static char   *pdp_mp4live_version = "pdp_mp4live~: version 0.1, an mpeg4ip video streaming object ( ydegoyon@free.fr )";

typedef struct pdp_mp4live_struct
{
    t_object x_obj;
    t_float x_f;

    t_int x_packet0;
    t_int x_dropped;
    t_int x_queue_id;

    t_int x_vwidth;
    t_int x_vheight;
    t_int x_vsize;

    t_outlet *x_outlet_streaming;  // indicates the status of streaming
    t_outlet *x_outlet_nbframes;   // number of frames emitted
    t_outlet *x_outlet_framerate;  // current frame rate

    t_int x_streaming;   // streaming flag
    t_int x_nbframes;    // number of frames emitted
    t_int x_framerate;   // framerate

    t_int x_cursec;   // current second
    t_int x_secondcount; // number of frames emitted in the current second

      /* audio structures */
    short x_audio_buf[2*MAX_AUDIO_PACKET_SIZE]; /* buffer for incoming audio */
    t_int x_audioin_position; // writing position for incoming audio
    t_int x_audio_per_frame;  // number of audio samples to transmit for each frame

      /* mpeg4ip data */
    CLiveConfig *x_mp4Config;
    CRtpTransmitter *x_rtpTransmitter;
    CPDPVideoSource *x_videosource;
    CPDPAudioSource *x_audiosource;

} t_pdp_mp4live;

#ifdef __cplusplus
extern "C"
{
#endif

static void pdp_mp4live_add_sink(t_pdp_mp4live *x, CMediaSink* pSink)
{
   if (x->x_videosource)
   {
      x->x_videosource->AddSink(pSink);
   }
   if (x->x_audiosource)
   {
      x->x_audiosource->AddSink(pSink);
   }
}

static void pdp_mp4live_remove_sink(t_pdp_mp4live *x, CMediaSink* pSink)
{
   if (x->x_videosource)
   {
      x->x_videosource->RemoveSink(pSink);
   }
   if (x->x_audiosource)
   {
      x->x_audiosource->RemoveSink(pSink);
   }
}

static void pdp_mp4live_disconnect(t_pdp_mp4live *x)
{
 t_int ret, i;

   if (!x->x_streaming)
   {
     post("pdp_mp4live~ : disconnect request but not connected ... ignored" );
     return;
   }

   if (x->x_audiosource) 
   {
     x->x_audiosource->DoStop();
     delete x->x_audiosource;
     x->x_audiosource = NULL;
   }

   if (x->x_videosource) 
   {
     x->x_videosource->DoStop();
     delete x->x_videosource;
     x->x_videosource = NULL;
   }

   if (x->x_rtpTransmitter) 
   {
     pdp_mp4live_remove_sink(x, x->x_rtpTransmitter);
     x->x_rtpTransmitter->StopThread();
     delete x->x_rtpTransmitter;
     x->x_rtpTransmitter = NULL;
   }

   x->x_streaming = 0;
   outlet_float( x->x_outlet_streaming, x->x_streaming );
   x->x_nbframes = 0;
   outlet_float( x->x_outlet_nbframes, x->x_nbframes );
   x->x_framerate = 0;
   outlet_float( x->x_outlet_framerate, x->x_framerate );
}

static void pdp_mp4live_connect(t_pdp_mp4live *x )
{
  t_int ret, i;

   if (x->x_streaming)
   {
     post("pdp_mp4live~ : connect request but already connected ... ignored" );
     return;
   }

   post("pdp_mp4live~ : creating video source");
   if ( x->x_videosource == NULL )
   {
      x->x_videosource = new CPDPVideoSource();
      x->x_videosource->SetConfig(x->x_mp4Config);
   }
   
   post("pdp_mp4live~ : creating audio source");
   if ( x->x_audiosource == NULL )
   {
       x->x_audiosource = new CPDPAudioSource(x->x_mp4Config);
   }

   post("pdp_mp4live~ : creating rtp transmitter");
   x->x_rtpTransmitter = new CRtpTransmitter(x->x_mp4Config);
   x->x_rtpTransmitter->StartThread();

   post("pdp_mp4live~ : creating audio destination");
   x->x_rtpTransmitter->CreateAudioRtpDestination(0,
                      x->x_mp4Config->GetStringValue(CONFIG_RTP_AUDIO_DEST_ADDRESS),
                      x->x_mp4Config->GetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT),
                      0);

   post("pdp_mp4live~ : creating video destination");
   x->x_rtpTransmitter->CreateVideoRtpDestination(0,
                     x->x_mp4Config->GetStringValue(CONFIG_RTP_DEST_ADDRESS),
                     x->x_mp4Config->GetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT),
                     0);

   post("pdp_mp4live~ : starting rtp");
   if ( x->x_rtpTransmitter )
   {
      pdp_mp4live_add_sink(x, x->x_rtpTransmitter);
      x->x_rtpTransmitter->Start();
   }

   if (x->x_videosource) 
   {
      post("pdp_mp4live~ : starting video source");
      x->x_videosource->DoStart();
      post("pdp_mp4live~ : generating key frame");
      x->x_videosource->GenerateKeyFrame();
   }

   if (x->x_audiosource) 
   {
      post("pdp_mp4live~ : starting audio source");
      x->x_audiosource->DoStart();
   }

   x->x_streaming = 1;
   outlet_float( x->x_outlet_streaming, x->x_streaming );
   x->x_nbframes = 0;
   outlet_float( x->x_outlet_nbframes, x->x_nbframes );
   x->x_framerate = 0;
   outlet_float( x->x_outlet_framerate, x->x_framerate );

}

static void pdp_mp4live_ipaddr(t_pdp_mp4live *x, t_symbol *sIpAddr )
{
  t_int a, b, c, d;

   if ( !strcmp( sIpAddr->s_name, "" ) )
   {
     post("pdp_mp4live~ : wrong ip address" );
     return;
   }

   if ( sscanf( sIpAddr->s_name, "%d.%d.%d.%d", &a, &b, &c, &d ) < 4 )
   {
     post("pdp_mp4live~ : wrong ip address : %s", sIpAddr->s_name );
     return;
   }

   post( "pdp_mp4live~ : setting ip address: %s", sIpAddr->s_name );
   x->x_mp4Config->SetStringValue( CONFIG_RTP_DEST_ADDRESS, sIpAddr->s_name );
   x->x_mp4Config->SetStringValue( CONFIG_RTP_AUDIO_DEST_ADDRESS, sIpAddr->s_name );
}

static void pdp_mp4live_aport(t_pdp_mp4live *x, t_floatarg fAudioPort )
{
   if ( ( (t_int) fAudioPort <= 0 ) || ( (t_int) fAudioPort > 65535 ) )
   {
     post("pdp_mp4live~ : wrong audio port : %d", fAudioPort );
     return;
   }

   post( "pdp_mp4live~ : setting audio port: %d", (t_int) fAudioPort );
   x->x_mp4Config->SetIntegerValue( CONFIG_RTP_AUDIO_DEST_PORT, (t_int) fAudioPort );

}

static void pdp_mp4live_vport(t_pdp_mp4live *x, t_floatarg fVideoPort )
{
   if ( ( (t_int) fVideoPort <= 0 ) || ( (t_int) fVideoPort > 65535 ) )
   {
     post("pdp_mp4live~ : wrong video port : %d", fVideoPort );
     return;
   }

   post( "pdp_mp4live~ : setting video port: %d", (t_int) fVideoPort );
   x->x_mp4Config->SetIntegerValue( CONFIG_RTP_VIDEO_DEST_PORT, (t_int) fVideoPort );

}

static void pdp_mp4live_ttl(t_pdp_mp4live *x, t_floatarg fTtl )
{
   if ( ( (t_int) fTtl <= 0 ) || ( (t_int) fTtl > 255 ) )
   {
     post("pdp_mp4live~ : wrong ttl : %d", fTtl );
     return;
   }

   post( "pdp_mp4live~ : setting ttl : %d", (t_int) fTtl );
   x->x_mp4Config->SetIntegerValue( CONFIG_RTP_MCAST_TTL, (t_int) fTtl );

}

static void pdp_mp4live_vwidth(t_pdp_mp4live *x, t_floatarg fWidth )
{
   if ( ( (t_int) fWidth <= 0 ) )
   {
     post("pdp_mp4live~ : wrong width : %d", fWidth );
     return;
   }

   post( "pdp_mp4live~ : setting width : %d", (t_int) fWidth );
   x->x_mp4Config->SetIntegerValue( CONFIG_VIDEO_RAW_WIDTH, (t_int) fWidth );

}

static void pdp_mp4live_vheight(t_pdp_mp4live *x, t_floatarg fHeight )
{
   if ( ( (t_int) fHeight <= 0 ) )
   {
     post("pdp_mp4live~ : wrong height : %d", fHeight );
     return;
   }

   post( "pdp_mp4live~ : setting height : %d", (t_int) fHeight );
   x->x_mp4Config->SetIntegerValue( CONFIG_VIDEO_RAW_HEIGHT, (t_int) fHeight );

}

static void pdp_mp4live_framerate(t_pdp_mp4live *x, t_floatarg fFrameRate )
{
   if ( ( (t_int) fFrameRate <= 0 ) )
   {
     post("pdp_mp4live~ : wrong framerate : %d", fFrameRate );
     return;
   }

   post( "pdp_mp4live~ : setting framerate : %d", (t_int) fFrameRate );
   x->x_mp4Config->SetFloatValue( CONFIG_VIDEO_FRAME_RATE, (t_float) fFrameRate );

}

static void pdp_mp4live_vbitrate(t_pdp_mp4live *x, t_floatarg fVBitrate )
{
   if ( ( (t_int) fVBitrate <= 0 ) )
   {
     post("pdp_mp4live~ : wrong video bit rate : %d", fVBitrate );
     return;
   }

   post( "pdp_mp4live~ : setting video bit rate : %d", (t_int) fVBitrate );
   x->x_mp4Config->SetIntegerValue( CONFIG_VIDEO_BIT_RATE, (t_int) fVBitrate );

}

static void pdp_mp4live_samplerate(t_pdp_mp4live *x, t_floatarg fSampleRate )
{
   if ( ( (t_int) fSampleRate != 44100 ) &&
        ( (t_int) fSampleRate != 22050 ) &&
        ( (t_int) fSampleRate != 11025 ) &&
        ( (t_int) fSampleRate != 8000 )
        )
   {
     post("pdp_mp4live~ : wrong samplerate : %d", fSampleRate );
     return;
   }

   post( "pdp_mp4live~ : setting samplerate : %d", (t_int) fSampleRate );
   x->x_mp4Config->SetIntegerValue( CONFIG_AUDIO_SAMPLE_RATE, (t_int) fSampleRate );

}

static void pdp_mp4live_abitrate(t_pdp_mp4live *x, t_floatarg fABitrate )
{
   if ( ( (t_int) fABitrate <= 0 ) )
   {
     post("pdp_mp4live~ : wrong audio bit rate : %d", fABitrate );
     return;
   }

   post( "pdp_mp4live~ : setting audio bit rate : %d", (t_int) fABitrate );
   x->x_mp4Config->SetIntegerValue( CONFIG_AUDIO_BIT_RATE_KBPS, (t_int) fABitrate );
   x->x_mp4Config->SetIntegerValue( CONFIG_AUDIO_BIT_RATE, ((t_int) fABitrate)*1000 );

}

static void pdp_mp4live_sdp(t_pdp_mp4live *x, t_symbol *sSdpFile )
{
  t_int ret;

   post( "pdp_mp4live~ : setting sdp filename : %s", (char *) sSdpFile->s_name );
   x->x_mp4Config->SetStringValue( CONFIG_SDP_FILE_NAME, (char *) sSdpFile->s_name );

   post( "pdp_mp4live~ : writing sdp file : %s", (char *) sSdpFile->s_name );
   if ( ( ret = GenerateSdpFile( x->x_mp4Config ) ) )
   {
     post( "pdp_mp4live~ : written sdp file : %s", (char *) sSdpFile->s_name );
   }
   else
   {
     post( "pdp_mp4live~ : could not write sdp file : %s", 
            (char *) sSdpFile->s_name );
   }
}

static void pdp_mp4live_process_yv12(t_pdp_mp4live *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    u_int8_t  *data   = (uint8_t *)pdp_packet_data(x->x_packet0);
    u_int8_t  *pY, *pU, *pV;
    struct timeval etime;

        /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth) ||
         ((int)header->info.image.height != x->x_vheight) )
    {
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
    }

    if ( x->x_streaming )
    {
         pY = data;
         pU = data+x->x_vsize;
         pV = data+x->x_vsize+(x->x_vsize>>2);

            /* update frames counter */

         if ( gettimeofday(&etime, NULL) == -1)
         {
            post("pdp_ffmpeg~ : could not read time" );
         }
         if ( etime.tv_sec != x->x_cursec )
         {
            x->x_cursec = etime.tv_sec;
            x->x_framerate = x->x_secondcount;
            x->x_secondcount = 0;
         }
         x->x_nbframes++;
         x->x_secondcount++;

         x->x_videosource->ProcessVideo( pY, pV, pU );

    }
    return;
}

static void pdp_mp4live_killpacket(t_pdp_mp4live *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
}

    /* store audio data in PCM format and stream it */
static t_int *pdp_mp4live_perform(t_int *w)
{
  t_float *in1   = (t_float *)(w[1]);       // left audio inlet
  t_float *in2   = (t_float *)(w[2]);       // right audio inlet 
  t_pdp_mp4live *x = (t_pdp_mp4live *)(w[3]);
  int n = (int)(w[4]);                      // number of samples 
  t_float fsample;
  t_int   isample, i;

    // just fills the buffer ( a pcm buffer )
    while (n--)
    {
       fsample=*(in1++); 
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       isample=(short) (32767.0 * fsample);
       *(x->x_audio_buf+x->x_audioin_position)=isample;
       x->x_audioin_position=(x->x_audioin_position+1)%(2*MAX_AUDIO_PACKET_SIZE); 
       if ( x->x_audioin_position == 2*MAX_AUDIO_PACKET_SIZE-1 ) 
       {
          // post( "pdp_mp4live~ : reaching end of audio buffer" );
       }
       fsample=*(in2++); 
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       isample=(short) (32767.0 * fsample);
       *(x->x_audio_buf+x->x_audioin_position)=isample;
       x->x_audioin_position=(x->x_audioin_position+1)%(2*MAX_AUDIO_PACKET_SIZE); 
       if ( x->x_audioin_position == 2*MAX_AUDIO_PACKET_SIZE-1 ) 
       {
          // post( "pdp_mp4live~ : reaching end of audio buffer" );
       }
    }

    if ( x->x_streaming )
    {
          /* send an audio frame */
       if ( (t_int)(x->x_audioin_position*sizeof(short)) > (t_int)x->x_audio_per_frame )
       {
         x->x_audiosource->ProcessAudio( (u_int8_t*)x->x_audio_buf, 
                        (u_int32_t)x->x_audio_per_frame );

           /* recopy the buffer and set new pointers */
         memcpy( x->x_audio_buf, x->x_audio_buf+(x->x_audio_per_frame/sizeof(short)), 
                    x->x_audioin_position*sizeof(short)-x->x_audio_per_frame ); 
         x->x_audioin_position-=(x->x_audio_per_frame/sizeof(short));
       }
    }
 
    return (w+5);
}

static void pdp_mp4live_dsp(t_pdp_mp4live *x, t_signal **sp)
{
    dsp_add(pdp_mp4live_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_mp4live_process(t_pdp_mp4live *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_mp4live_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_BITMAP_YV12:
            pdp_queue_add(x, (void*) pdp_mp4live_process_yv12, (void*) pdp_mp4live_killpacket, &x->x_queue_id);
            outlet_float( x->x_outlet_nbframes, x->x_nbframes );
            outlet_float( x->x_outlet_framerate, x->x_framerate );
	    break;

	  default:
	    /* don't know the type, so dont pdp_mp4live_process */
            post( "pdp_mp4live~ : hey!! i don't know about that type of image : %d", 
                  pdp_packet_header(x->x_packet0)->info.image.encoding );
	    break;
	    
	}
    }

}

static void pdp_mp4live_input_0(t_pdp_mp4live *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("bitmap/yv12/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_mp4live_process(x);
    }

}

static void pdp_mp4live_free(t_pdp_mp4live *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_mp4live_class;

void *pdp_mp4live_new(void)
{
    int i;

    t_pdp_mp4live *x = (t_pdp_mp4live *)pd_new(pdp_mp4live_class);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("signal"), gensym ("signal"));

    x->x_outlet_streaming = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_framerate = outlet_new(&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_queue_id = -1;
    x->x_nbframes = 0;
    x->x_framerate = 0;
    x->x_secondcount = 0;
    x->x_audioin_position = 0;

    x->x_mp4Config = new CLiveConfig(PdpConfigVariables,
                     sizeof(PdpConfigVariables) / sizeof(SConfigVariable),
                     "none");
    if ( x->x_mp4Config == NULL )
    {
       post( "pdp_mp4live~ : couldn't allocate default config" );
       return NULL;
    }

    x->x_mp4Config->InitializeIndexes();

    x->x_mp4Config->Update();

    // update sample rate with the actual sample rate
    x->x_mp4Config->SetIntegerValue( CONFIG_AUDIO_SAMPLE_RATE, (t_int) sys_getsr() );

    x->x_videosource = NULL;
    x->x_audiosource = NULL;

    x->x_audio_per_frame = AUDIO_PACKET_SIZE;

    return (void *)x;
}


void pdp_mp4live_tilde_setup(void)
{
    // post( pdp_mp4live_version );
    pdp_mp4live_class = class_new(gensym("pdp_mp4live~"), (t_newmethod)pdp_mp4live_new,
    	(t_method)pdp_mp4live_free, sizeof(t_pdp_mp4live), 0, A_NULL);

    CLASS_MAINSIGNALIN(pdp_mp4live_class, t_pdp_mp4live, x_f );
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_dsp, gensym("dsp"), A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_connect, gensym("connect"), A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_ipaddr, gensym("ipaddr"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_aport, gensym("audioport"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_vport, gensym("videoport"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_ttl, gensym("ttl"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_vwidth, gensym("vwidth"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_vheight, gensym("vheight"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_framerate, gensym("framerate"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_vbitrate, gensym("vbitrate"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_samplerate, gensym("samplerate"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_abitrate, gensym("abitrate"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_disconnect, gensym("disconnect"), A_NULL);
    class_addmethod(pdp_mp4live_class, (t_method)pdp_mp4live_sdp, gensym("sdp"), A_SYMBOL, A_NULL);
    class_sethelpsymbol( pdp_mp4live_class, gensym("pdp_mp4live~.pd") );
}

#ifdef __cplusplus
}
#endif
