/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon (ydegoyon@free.fr)
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

/*  This object is a quicktime stream picker object
 *  A lot of this object code is inspired by the code from mpeg4ip
 *  Copyright (c) 2000, 2001, 2002 Dave Mackie, Bill May & others
 *  The rest is written by Yves Degoyon ( ydegoyon@free.fr )
 */


#include "pdp_mp4player~.h"

static char   *pdp_mp4player_version = "pdp_mp4player~: version 0.1, a mpeg4ip stream decoder ( ydegoyon@free.fr).";

#ifdef __cplusplus
extern "C"
{
#endif

static void pdp_mp4player_audio(t_pdp_mp4player *x, t_floatarg faudio )
{
   if ( ( faudio == 0. ) || ( faudio == 1. ) )
   {
      x->x_audio = (int)faudio;
   }
}

static void pdp_mp4player_overtcp(t_pdp_mp4player *x, t_floatarg fovertcp )
{
   if ( ( fovertcp == 0. ) || ( fovertcp == 1. ) )
   {
     x->x_rtpovertcp = (t_int)fovertcp;
     config.set_config_value(CONFIG_USE_RTP_OVER_RTSP, x->x_rtpovertcp);
     if ( x->x_rtpovertcp )
     {
        post("pdp_mp4player~ : using rtp over rtsp (tcp)" );
     }
     else
     {
        post("pdp_mp4player~ : using rtp mode (udp)" );
     }
   }
} 

static void pdp_mp4player_priority(t_pdp_mp4player *x, t_floatarg fpriority )
{
   if ( ( x->x_priority >= MIN_PRIORITY ) && ( x->x_priority <= MAX_PRIORITY ) )
   {
     x->x_priority = (int)fpriority;
   }
}

static void pdp_mp4player_vwidth(t_pdp_mp4player *x, t_floatarg fWidth )
{
   if ( ( (t_int) fWidth <= 0 ) )
   {
     post("pdp_mp4player~ : wrong width : %d", fWidth );
     return;
   }

   post( "pdp_mp4player~ : setting width : %d", (t_int) fWidth );
   config.set_config_value( CONFIG_VIDEO_RAW_WIDTH, (t_int) fWidth );

}

static void pdp_mp4player_vheight(t_pdp_mp4player *x, t_floatarg fHeight )
{
   if ( ( (t_int) fHeight <= 0 ) )
   {
     post("pdp_mp4player~ : wrong height : %d", fHeight );
     return;
   }

   post( "pdp_mp4player~ : setting height : %d", (t_int) fHeight );
   config.set_config_value( CONFIG_VIDEO_RAW_HEIGHT, (t_int) fHeight );

}

static void pdp_mp4player_disconnect(t_pdp_mp4player *x)
{
   if (!x->x_streaming)
   {
     post("pdp_mp4player~ : close request but no stream is played ... ignored" );
     return;
   }

   x->x_streaming = 0;
   x->x_newpicture = 0;

   outlet_float( x->x_outlet_streaming, x->x_streaming );
   x->x_nbframes = 0;
   outlet_float( x->x_outlet_nbframes, x->x_nbframes );
   x->x_framerate = 0;
   outlet_float( x->x_outlet_framerate, x->x_framerate );

   post( "pdp_mp4player~ : deleting session" );
   delete x->x_psession;
   post( "pdp_mp4player~ : deleting semaphore" );
   SDL_DestroySemaphore(x->x_psem);
}

static void *pdp_mp4player_decode(void *tdata)
{
  t_pdp_mp4player *x = (t_pdp_mp4player*)tdata;
  struct sched_param schedprio;
  t_int pmin, pmax;
  struct timespec twait, mwait;

    twait.tv_sec = 0;
    twait.tv_nsec = 10000000; // 10 ms

    schedprio.sched_priority = 0;
#ifdef __gnu_linux__
    if ( sched_setscheduler(0, SCHED_OTHER, &schedprio) == -1)
    {
       post("pdp_mp4player~ : couldn't set scheduler for decoding thread.\n");
    }
#endif
    if ( setpriority( PRIO_PROCESS, 0, x->x_priority ) < 0 )
    {
       post("pdp_mp4player~ : couldn't set priority to %d for decoding thread.\n", x->x_priority );
    }
    else
    {
       post("pdp_mp4player~ : priority set to %d for thread %d.\n", x->x_priority, x->x_decodechild );
    }

    while ( x->x_streaming )
    {
      x->x_decodingstate = x->x_psession->sync_thread(x->x_decodingstate);
      nanosleep( &twait, NULL ); // nothing to read, just wait
    }

    post( "pdp_mp4player~ : decoding thread %d exiting....", x->x_decodechild );
    x->x_decodechild = 0;
    pthread_exit(NULL);
}


static void pdp_mp4player_connect(t_pdp_mp4player *x, t_symbol *s)
{
  t_int ret, i;
  char buffer[1024];
  char errmsg[512]; 
  pthread_attr_t decode_child_attr;

   if ( x->x_streaming ) 
   {
     post("pdp_mp4player~ : connection request but a connection is pending ... disconnecting" );
     pdp_mp4player_disconnect(x);
   }

   if ( x->x_url ) free( x->x_url );
   x->x_url = (char*) malloc( strlen( s->s_name ) + 1 );
   strcpy( x->x_url, s->s_name );

   x->x_psem = SDL_CreateSemaphore(0);
   snprintf(buffer, sizeof(buffer), "pdp_mp4player~ - %s", x->x_url);
   x->x_psession = new CPlayerSession(&x->x_queue, x->x_psem, buffer, x);
   if (x->x_psession == NULL) 
   {
     post("pdp_mp4player~ : FATAL : could not create session" );
     return;
   }
   
   ret = parse_name_for_session(x->x_psession, x->x_url, errmsg, sizeof(errmsg), NULL);
   if (ret < 0) 
   {
     post("pdp_mp4player~ : FATAL : wrong url : %s : reason : %s", x->x_url, errmsg );
     delete x->x_psession;
     return;
   }

   if (ret > 0) 
   {
     post("pdp_mp4player~ : %s", errmsg );
   }

   x->x_psession->set_up_sync_thread();

   if (x->x_psession->play_all_media(TRUE) != 0) {
      post("pdp_mp4player~ : FATAL : couldn't play all medias" );
      delete x->x_psession;
      return;
   } 
   
   // launch decoding thread
   if ( pthread_attr_init( &decode_child_attr ) < 0 )
   {
      post( "pdp_mp4player~ : could not launch decoding thread" );
      perror( "pthread_attr_init" );
      return;
   }
   if ( pthread_create( &x->x_decodechild, &decode_child_attr, pdp_mp4player_decode, x ) < 0 )
   {
      post( "pdp_mp4player~ : could not launch decoding thread" );
      perror( "pthread_create" );
      return;
   }

   post("pdp_mp4player~ : session started" );
   x->x_streaming = 1;

   return;
}

    /* decode the stream to fill up buffers */
static t_int *pdp_mp4player_perform(t_int *w)
{
  t_float *out1   = (t_float *)(w[1]);       // left audio inlet
  t_float *out2   = (t_float *)(w[2]);       // right audio inlet 
  t_pdp_mp4player *x = (t_pdp_mp4player *)(w[3]);
  int n = (int)(w[4]);                      // number of samples 
  short sampleL, sampleR;
  struct timeval etime;
  t_int sn;

    x->x_blocksize = n;

    // just read the buffer
    if ( x->x_audioon )
    {
      sn=0;
      while (n--) 
      {
        sampleL=x->x_audio_in[ sn++ ];
        *(out1) = ((t_float)sampleL)/32768.0;
        if ( DEFAULT_CHANNELS == 1 )
        {
          *(out2) = *(out1);
        }
        if ( DEFAULT_CHANNELS == 2 )
        {
          sampleR=x->x_audio_in[ sn++ ];
          *(out2) = ((t_float)sampleR)/32768.0;
        }
        out1++;
        out2++;
      }
      x->x_audioin_position-=sn;
      memcpy( &x->x_audio_in[0], &x->x_audio_in[sn], (4*MAX_AUDIO_PACKET_SIZE-sn-1)*sizeof(short) );
      // post( "pdp_mp4player~ : audio in position : %d", x->x_audioin_position );
      if ( x->x_audioin_position <= sn )
      {
         x->x_audioon = 0;
         // post( "pdp_mp4player~ : audio off" );
      }
    }
    else
    {
      // post("pdp_mp4player~ : no available audio" );
      while (n--)
      {
        *(out1++) = 0.0;
        *(out2++) = 0.0;
      }
    }	

    // check if the framerate has been exceeded
    if ( gettimeofday(&etime, NULL) == -1)
    {
       post("pdp_mp4player~ : could not read time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       outlet_float( x->x_outlet_framerate, x->x_secondcount );
       x->x_secondcount = 0;
    }

    if ( x->x_newpicture )
    {
      x->x_packet = pdp_packet_new_image_YCrCb( x->x_vwidth, x->x_vheight );
      x->x_data = (short int *)pdp_packet_data(x->x_packet);
      memcpy( x->x_data, x->x_datav, (x->x_vsize + (x->x_vsize>>1))<<1 );
      pdp_packet_pass_if_valid(x->x_pdp_out, &x->x_packet);

      // update streaming status
      outlet_float( x->x_outlet_streaming, x->x_streaming );
      x->x_nbframes++;
      x->x_secondcount++;
      outlet_float( x->x_outlet_nbframes, x->x_nbframes );
      x->x_newpicture = 0;
    }

    return (w+5);
}

static void pdp_mp4player_dsp(t_pdp_mp4player *x, t_signal **sp)
{
    dsp_add(pdp_mp4player_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_mp4player_free(t_pdp_mp4player *x)
{
  int i;

    if ( x->x_streaming )
    {
       pdp_mp4player_disconnect(x);
    }
    post( "pdp_mp4player~ : freeing object" );
    pdp_packet_mark_unused(x->x_packet);

    // remove invalid global ports
    close_plugins();
}

t_class *pdp_mp4player_class;

void *pdp_mp4player_new(void)
{
    int i;

    t_pdp_mp4player *x = (t_pdp_mp4player *)pd_new(pdp_mp4player_class);

    x->x_pdp_out = outlet_new(&x->x_obj, &s_anything);

    x->x_outlet_left = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet_right = outlet_new(&x->x_obj, &s_signal);

    x->x_outlet_streaming = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_framerate = outlet_new(&x->x_obj, &s_float);

    x->x_packet = -1;
    x->x_nbframes = 0;
    x->x_cursec = 0;
    x->x_secondcount = 0;
    x->x_audioin_position = 0;
    x->x_blocksize = MIN_AUDIO_SIZE;
    x->x_priority = DEFAULT_PRIORITY;
    x->x_decodechild = 0;
    x->x_newpicture = 0;

    x->x_vwidth = -1;
    x->x_vheight = -1;
    x->x_datav = NULL;

    memset( &x->x_audio_in[0], 0x0, 4*MAX_AUDIO_PACKET_SIZE*sizeof(short) );

    // initialize mpeg4hippies
    initialize_plugins();
    config.read_config_file();
    rtsp_set_error_func(player_library_message);
    rtsp_set_loglevel(config.get_config_value(CONFIG_RTSP_DEBUG));
    rtp_set_error_msg_func(player_library_message);
    rtp_set_loglevel(config.get_config_value(CONFIG_RTP_DEBUG));
    sdp_set_error_func(player_library_message);
    sdp_set_loglevel(config.get_config_value(CONFIG_SDP_DEBUG));
    http_set_error_func(player_library_message);
    http_set_loglevel(config.get_config_value(CONFIG_HTTP_DEBUG));

    x->x_rtpovertcp = 0;
    config.set_config_value(CONFIG_USE_RTP_OVER_RTSP, x->x_rtpovertcp);

    return (void *)x;
}


void pdp_mp4player_tilde_setup(void)
{
    // post( pdp_mp4player_version );
    pdp_mp4player_class = class_new(gensym("pdp_mp4player~"), (t_newmethod)pdp_mp4player_new,
    	(t_method)pdp_mp4player_free, sizeof(t_pdp_mp4player), 0, A_NULL);

    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_dsp, gensym("dsp"), A_NULL);
    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_connect, gensym("connect"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_disconnect, gensym("disconnect"), A_NULL);
    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_audio, gensym("audio"), A_FLOAT, A_NULL);
    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_overtcp, gensym("overtcp"), A_FLOAT, A_NULL);
    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_priority, gensym("priority"), A_FLOAT, A_NULL);
    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_vwidth, gensym("vwidth"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mp4player_class, (t_method)pdp_mp4player_vheight, gensym("vheight"), A_DEFFLOAT, A_NULL);
    class_sethelpsymbol( pdp_mp4player_class, gensym("pdp_mp4player~.pd") );

}

#ifdef __cplusplus
}
#endif
