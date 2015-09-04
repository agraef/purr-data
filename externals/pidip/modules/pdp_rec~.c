/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon <ydegoyon@free.fr>
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

/*  This object is a video recording object 
 *  It records its input in quicktime format
 */


#include "pdp.h"
#include "pidip_config.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
#ifdef QUICKTIME_NEWER
#include <lqt/lqt.h>
#include <lqt/colormodels.h>
#else
#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>
#endif

#define DEFAULT_FRAME_RATE 25
#define DEFAULT_CHANNELS 2
#define DEFAULT_BITS 8
#define DEFAULT_QUALITY 75 // from 1 to 100
#define MAX_COMP_LENGTH 8
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)

static char   *pdp_rec_version = "pdp_rec~: version 0.1, a video/audio recording object, written by ydegoyon@free.fr";

typedef struct pdp_rec_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    quicktime_t *x_qtfile;
    unsigned char **x_yuvpointers;
    unsigned char *x_yuvbuffer;
    int x_framerate;
    int x_forced_framerate;
    int x_jpeg_quality;
    int x_newfile;
    char  *x_compressor; 
    int x_recflag;
    int x_frameswritten;
    struct timeval x_tstart;
    struct timeval x_tstop;
    struct timeval x_tlastrec;

     /* audio structures */
    int16_t **x_audio_buf; /* buffer for incoming audio */
    int x_audioin_position; // writing position for incoming audio
    char  *x_acompressor;  // audio compressor
    int x_channels;      // audio channels 
    int x_samplerate;    // audio sample rate 
    int x_bits;          // audio bits

} t_pdp_rec;

static void pdp_rec_free_ressources(t_pdp_rec *x)
{
    if ( x->x_yuvpointers ) 
    {
       post( "pdp_rec~ : freeing pointers..." );
       freebytes ( x->x_yuvpointers, 3*sizeof(unsigned char**) );
    }
    if ( x->x_yuvbuffer )
    {
       post( "pdp_rec~ : freeing buffers..." );
       freebytes ( x->x_yuvbuffer, x->x_vsize + (x->x_vsize>>1) );
    }

}

static void pdp_rec_allocate(t_pdp_rec *x)
{
  int i;

    x->x_yuvpointers = (unsigned char**) getbytes ( 3*sizeof(unsigned char**) );
    x->x_yuvbuffer = (unsigned char*) getbytes ( x->x_vsize + (x->x_vsize>>1) );
    x->x_yuvpointers[0] = &x->x_yuvbuffer[0];
    x->x_yuvpointers[2] = &x->x_yuvbuffer[x->x_vsize];
    x->x_yuvpointers[1] = &x->x_yuvbuffer[x->x_vsize + (x->x_vsize>>2)];
}

    /* set video track whenever width or height is changed */
static void pdp_rec_set_video(t_pdp_rec *x)
{
  int ret;

    if ( !x->x_qtfile ) {
       post( "pdp_rec~ : no video recording file is opened !!");
       return;
    }

    if( ( ret = quicktime_set_video(x->x_qtfile, 1, x->x_vwidth, x->x_vheight, x->x_framerate, x->x_compressor) ) != 0) {
       post( "pdp_rec~ : error setting video track ret=%d", ret );
    } else {
       post( "pdp_rec~ : video track set" );
    }

    quicktime_set_copyright(x->x_qtfile, "");
    quicktime_set_name(x->x_qtfile, "Pdp output");
    quicktime_set_info(x->x_qtfile, "File created with PDP/PiDiP");

}

    /* set framerate */
static void pdp_rec_set_framerate(t_pdp_rec *x)
{
  int ret;

    if ( !x->x_qtfile ) {
       post( "pdp_rec~ : no video recording file is opened !!");
       return;
    }

    quicktime_set_framerate(x->x_qtfile, (float)x->x_framerate );
    post( "pdp_rec~ : framerate set to : %d", x->x_framerate );

}

    /* set audio track */
static void pdp_rec_set_audio(t_pdp_rec *x)
{
  int ret;

    if ( !x->x_qtfile ) {
       post( "pdp_rec~ : no video recording file is opened !!");
       return;
    }

    if( ( ret = quicktime_set_audio(x->x_qtfile, x->x_channels, x->x_samplerate, x->x_bits, x->x_acompressor ) ) == 0) 
    {
       post( "pdp_rec~ : error setting audio track ret=%d", ret );
       post( "pdp_rec~ : params : samplerate=%d : compressor=%s : channels=%d : bits=%d", 
                         x->x_samplerate, x->x_acompressor, x->x_channels, x->x_bits );
    } else {
       post( "pdp_rec~ : %d audio track(s) allocated.", ret );
    }

}

    /* set color model : it's hard coded : only one model supported */
static void pdp_rec_set_cmodel(t_pdp_rec *x)
{
  int ret;

    if ( !x->x_qtfile ) {
       post( "pdp_rec~ : no video recording file is opened !!");
       return;
    }

    quicktime_set_cmodel(x->x_qtfile, BC_YUV420P );
    post( "pdp_rec~ : color model set" );

}

static void pdp_rec_set_jpeg(t_pdp_rec *x)
{
    if ( !x->x_qtfile )
    {
       post( "pdp_rec~ : set jpeg : no video recording file is opened !!");
       return;
    }

    if ( strcmp( x->x_compressor, QUICKTIME_JPEG ) )
    {
       post( "pdp_rec~ : set jpeg : the codec is not jpeg right now !!");
       return;
    }
    quicktime_set_jpeg( x->x_qtfile, x->x_jpeg_quality, 1 ); 
    post( "pdp_rec~ : jpeg quality factor set : %d", x->x_jpeg_quality );
}

static void pdp_rec_frame_rate(t_pdp_rec *x, t_floatarg frate )
{
    if ( frate >= 1 )
    {
       x->x_framerate = (int) frate;
       x->x_forced_framerate = 1;
       post( "pdp_rec~ : frame rate set to %d : open a new file to activate it", x->x_framerate );
    }
}

static void pdp_rec_jpeg(t_pdp_rec *x, t_floatarg fjpeg )
{
    if ( ( fjpeg >= 1 ) && ( fjpeg <= 100 ))
    {
       x->x_jpeg_quality = (int) fjpeg;
       post( "pdp_rec~ : jpeg quality set : open a new file to activate it" );
    }
}

static void pdp_rec_compressor(t_pdp_rec *x, t_symbol *scompressor )
{
  char scomp[ MAX_COMP_LENGTH ];

    // check compressor as defined in quicktime.h
    if ( 
         strcmp( scompressor->s_name, "divx")
         && strcmp( scompressor->s_name, "dv")
         && strcmp( scompressor->s_name, "raw")
         && strcmp( scompressor->s_name, "jpeg")
         // && strcmp( scompressor->s_name, "png") // crashes with libquicktime 0.9.1
         // && strcmp( scompressor->s_name, "mjpa") // no output with libquicktime 0.9.1
         && strcmp( scompressor->s_name, "yuv2")
         // && strcmp( scompressor->s_name, "yuv4")  // crashes with libquicktime 0.9.1
         )
    {
      post( "pdp_rec~ : unsupported codec : %s", scompressor->s_name );
      return;
    }

    // map message names to libquicktime names
    if ( !strcmp( scompressor->s_name, "divx") )
    {
       strcpy( scomp, QUICKTIME_DIVX );
    }
    if ( !strcmp( scompressor->s_name, "dv") )
    {
       strcpy( scomp, QUICKTIME_DV );
    }
    if ( !strcmp( scompressor->s_name, "raw") )
    {
       strcpy( scomp, QUICKTIME_RAW );
    }
    if ( !strcmp( scompressor->s_name, "jpeg") )
    {
       strcpy( scomp, QUICKTIME_JPEG );
    }
    if ( !strcmp( scompressor->s_name, "png") )
    {
       strcpy( scomp, QUICKTIME_PNG );
    }
    if ( !strcmp( scompressor->s_name, "mjpa") )
    {
       strcpy( scomp, QUICKTIME_MJPA );
    }
    if ( !strcmp( scompressor->s_name, "yuv2") )
    {
       strcpy( scomp, QUICKTIME_YUV2 );
    }
    if ( !strcmp( scompressor->s_name, "yuv4") )
    {
       strcpy( scomp, QUICKTIME_YUV4 );
    }

    if ( x->x_compressor )
    {
       freebytes( x->x_compressor, strlen( x->x_compressor )+1 );
    }
    x->x_compressor = (char *) getbytes( strlen( scomp ) + 1 );
    strcpy( x->x_compressor, scomp );
    post( "pdp_rec~ : compressor set to %s : open a new file to activate it", scomp );
}

    /* set audio compressor */
static void pdp_rec_acompressor(t_pdp_rec *x, t_symbol *scompressor )
{
  char scomp[ MAX_COMP_LENGTH ];

    // check compressor as defined in quicktime.h
    if ( 
         strcmp( scompressor->s_name, "twos")
         // && strcmp( scompressor->s_name, "ima4") // produces a lot of errors ( libquicktime 0.9.1 )
         && strcmp( scompressor->s_name, "raw") 
         // && strcmp( scompressor->s_name, "ulaw") // produces a lot of errors ( libquicktime 0.9.1 )
         // && strcmp( scompressor->s_name, "ogg")  // produces a lot of errors ( libquicktime 0.9.1 )
       )
    {
      post( "pdp_rec~ : unsupported codec : %s", scompressor->s_name );
      return;
    }

    // map message names to libquicktime names
    if ( !strcmp( scompressor->s_name, "raw") )
    {
       strcpy( scomp, QUICKTIME_RAW );
    }
    if ( !strcmp( scompressor->s_name, "ima4") )
    {
       strcpy( scomp, QUICKTIME_IMA4 );
    }
    if ( !strcmp( scompressor->s_name, "twos") )
    {
       strcpy( scomp, QUICKTIME_TWOS );
    }
    if ( !strcmp( scompressor->s_name, "ulaw") )
    {
       strcpy( scomp, QUICKTIME_ULAW );
    }
    if ( !strcmp( scompressor->s_name, "ogg") )
    {
       strcpy( scomp, QUICKTIME_VORBIS );
    }

    if ( x->x_compressor )
    {
       freebytes( x->x_compressor, strlen( x->x_compressor )+1 );
    }
    x->x_compressor = (char *) getbytes( strlen( scomp ) + 1 );
    strcpy( x->x_compressor, scomp );
    post( "pdp_rec~ : audio compressor set to %s : open a new file to activate it", scomp );
}

    /* close a video file */
static void pdp_rec_close(t_pdp_rec *x)
{
  int ret;

    if ( x->x_qtfile ) {
       if( ( ret = quicktime_close(x->x_qtfile) ) != 0 ) {
          post( "pdp_rec~ : error closing file ret=%d", ret );
       } else {
          post( "pdp_rec~ : closed video file" );
          x->x_qtfile = NULL;
       }
    }
}

    /* open a new video file */
static void pdp_rec_open(t_pdp_rec *x, t_symbol *sfile)
{
  int ret=0;

    // close previous video file if existing
    pdp_rec_close(x);

    if ( x->x_recflag ) {
       x->x_recflag = 0;
    }

    if ( ( x->x_qtfile = quicktime_open(sfile->s_name, 0, 1) ) == NULL )
    {
       error( "pdp_rec~ : cannot open >%s<", sfile->s_name);
       error( "pdp_rec~ : ret=%d", ret );
       x->x_qtfile = NULL;
       return;
    } else {
       x->x_frameswritten = 0;
       post( "pdp_rec~ : opened >%s<", sfile->s_name);
       x->x_newfile = 1;
    }

}

   /* start recording */
static void pdp_rec_start(t_pdp_rec *x)
{
    if ( !x->x_qtfile ) {
       post("pdp_rec~ : start received but no file has been opened ... ignored.");
       return;
    }

    if ( x->x_recflag == 1 ) {
       post("pdp_rec~ : start received but recording is started ... ignored.");
       return;
    }

    if ( gettimeofday(&x->x_tstart, NULL) == -1)
    {
       post("pdp_rec~ : could not set start time" );
    }

    x->x_recflag = 1;
    post("pdp_rec~ : start recording");
}

   /* stop recording */
static void pdp_rec_stop(t_pdp_rec *x)
{
    if ( !x->x_qtfile ) {
       post("pdp_rec~ : stop received but no file has been opened ... ignored.");
       return;
    }

    if ( x->x_recflag == 0 ) {
       post("pdp_rec~ : stop received but recording is stopped ... ignored.");
       return;
    }

    if ( gettimeofday(&x->x_tstop, NULL) == -1)
    {
       post("pdp_rec~ : could set stop time" );
    }

    // calculate frame rate if it hasn't been set
    if ( !x->x_forced_framerate )
    {
      if ( ( x->x_tstop.tv_sec - x->x_tstart.tv_sec ) > 0 )
      {
        x->x_framerate = x->x_frameswritten / ( x->x_tstop.tv_sec - x->x_tstart.tv_sec );
      }
      else
      {
        x->x_framerate = DEFAULT_FRAME_RATE;
      }
    }

    pdp_rec_set_framerate(x);

    x->x_recflag = 0;
    pdp_rec_close(x);

    post("pdp_rec~ : stop recording");
}

    /* store audio data in PCM format in a buffer for now */
static t_int *pdp_rec_perform(t_int *w)
{
  t_float *in1   = (t_float *)(w[1]);       // left audio inlet
  t_float *in2   = (t_float *)(w[2]);       // right audio inlet
  t_pdp_rec *x = (t_pdp_rec *)(w[3]);
  int n = (int)(w[4]);                      // number of samples
  t_float fsample;
  int   isample, i;

   if ( x->x_recflag ) 
   {

    // just fills the buffer
    while (n--)
    {
       fsample=*(in1++);
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       isample=(short) (32767.0 * fsample);
       x->x_audio_buf[0][x->x_audioin_position]=isample;
       fsample=*(in2++);
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       isample=(short) (32767.0 * fsample);
       x->x_audio_buf[1][x->x_audioin_position]=isample;
       x->x_audioin_position=(x->x_audioin_position+1)%(MAX_AUDIO_PACKET_SIZE);
       if ( x->x_audioin_position == MAX_AUDIO_PACKET_SIZE-1 )
       {
          post( "pdp_rec~ : reaching end of audio buffer" );
       }
    }

  }

  return (w+5);
}

static void pdp_rec_dsp(t_pdp_rec *x, t_signal **sp)
{
    dsp_add(pdp_rec_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_rec_process_yv12(t_pdp_rec *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet0);
  short int *data   = (short int *)pdp_packet_data(x->x_packet0);
  int     i, ret;
  int     px, py;
  unsigned short *poy, *pou, *pov;
  struct timeval trec;
  int     nbaudiosamples, nbusecs, nbrecorded;
  t_float   fframerate=0.0;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    /* setting video track */
    if ( x->x_qtfile && x->x_recflag )
    {
      if ( ( (int)(header->info.image.width) != x->x_vwidth ) || 
           ( (int)(header->info.image.height) != x->x_vheight ) || 
           ( x->x_newfile ) )
      {
	 pdp_rec_free_ressources(x);
         x->x_newfile = 0;
         if ( x->x_qtfile ) {
            pdp_rec_set_video(x);
            pdp_rec_set_audio(x);
            pdp_rec_set_cmodel(x);
            if ( !strcmp( x->x_compressor, QUICKTIME_JPEG ) )
            {
               pdp_rec_set_jpeg(x);
            }
         }
	 pdp_rec_allocate(x);
      }

      if ( x->x_frameswritten == 0 )
      {
        if ( gettimeofday(&x->x_tlastrec, NULL) == -1)
        {
           post("pdp_rec~ : could set stop time" );
        }
      }

      for (i=0; i<x->x_vsize; i++)
      {
        x->x_yuvbuffer[i] = data[i]>>7;
      }
      for (i=x->x_vsize; i<(x->x_vsize+(x->x_vsize>>1)); i++)
      {
        x->x_yuvbuffer[i] = ((data[i]>>8)+128);
      }
      
      if ( ( ret = quicktime_encode_video(x->x_qtfile, x->x_yuvpointers, 0) ) != 0 )
      {
         post( "pdp_rec~ : error writing frame : ret=%d", ret );
      }
      else
      {
         x->x_frameswritten++;
      }

      // calculate the number of audio samples to output
      if ( gettimeofday(&trec, NULL) == -1)
      {
         post("pdp_rec~ : could set stop time" );
      }
      // calculate time diff in micro seconds
      nbusecs = ( trec.tv_usec - x->x_tlastrec.tv_usec ) + ( trec.tv_sec - x->x_tlastrec.tv_sec )*1000000;
      nbaudiosamples = (sys_getsr()*1000000)/nbusecs;
      memcpy( &x->x_tlastrec, &trec, sizeof( struct timeval) );

      if ( x->x_audioin_position > nbaudiosamples )
      {
         nbrecorded = nbaudiosamples;
      }
      else
      {
         nbrecorded = x->x_audioin_position;
      }

      if ( ( ret = quicktime_encode_audio(x->x_qtfile, x->x_audio_buf, NULL, nbrecorded) ) != 0 )
      {
         post( "pdp_rec~ : error writing audio data : ret=%d", ret );
      }
      else
      {
         memcpy( &x->x_audio_buf[0][0], &x->x_audio_buf[0][nbrecorded], x->x_audioin_position-nbrecorded );
         memcpy( &x->x_audio_buf[1][0], &x->x_audio_buf[1][nbrecorded], x->x_audioin_position-nbrecorded );
         x->x_audioin_position -= nbrecorded;
         // post ( "pdp_rec~ : recorded %d samples.", nbrecorded );
      }
    }

    return;
}

static void pdp_rec_killpacket(t_pdp_rec *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
}

static void pdp_rec_process(t_pdp_rec *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_rec_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            if ( x->x_qtfile && x->x_recflag )
            {
              outlet_float( x->x_obj.ob_outlet, x->x_frameswritten );
            }
            pdp_queue_add(x, pdp_rec_process_yv12, pdp_rec_killpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_rec_process */
	    break;
	    
	}
    }

}

static void pdp_rec_input_0(t_pdp_rec *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
        x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_rec_process(x);
    }

}

static void pdp_rec_free(t_pdp_rec *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    // close video file if existing
    pdp_rec_close(x);
    pdp_rec_free_ressources(x);
    for ( i=0; i<x->x_channels; i++)
    {
       if ( x->x_audio_buf[i] ) freebytes( x->x_audio_buf[i], MAX_AUDIO_PACKET_SIZE*sizeof(int16_t) );
    }
    if ( x->x_audio_buf ) freebytes( x->x_audio_buf, x->x_channels*sizeof(int16_t*) );
    
}

t_class *pdp_rec_class;

void *pdp_rec_new(void)
{
  int i;

    t_pdp_rec *x = (t_pdp_rec *)pd_new(pdp_rec_class);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("signal"), gensym ("signal"));
    outlet_new (&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_qtfile = NULL;
    x->x_framerate = DEFAULT_FRAME_RATE;
    x->x_forced_framerate = 0;
    x->x_compressor = (char*) getbytes( strlen(QUICKTIME_DIVX)+1 );    
    strcpy( x->x_compressor, QUICKTIME_DIVX );

     /* audio defaults */
    x->x_acompressor = (char*) getbytes( strlen(QUICKTIME_TWOS)+1 );    
    strcpy( x->x_acompressor, QUICKTIME_TWOS );
    x->x_samplerate = sys_getsr();
    x->x_channels = DEFAULT_CHANNELS;
    x->x_bits = DEFAULT_BITS;

    x->x_audio_buf = (int16_t**) getbytes( x->x_channels*sizeof(int16_t*) );
    for ( i=0; i<x->x_channels; i++)
    {
       x->x_audio_buf[i] = (int16_t*) getbytes( MAX_AUDIO_PACKET_SIZE*sizeof(int16_t) );
    }

    x->x_newfile = 0;
    x->x_yuvbuffer = NULL;
    x->x_yuvpointers = NULL;
    x->x_jpeg_quality = DEFAULT_QUALITY;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_rec_tilde_setup(void)
{
    // post( pdp_rec_version );
    pdp_rec_class = class_new(gensym("pdp_rec~"), (t_newmethod)pdp_rec_new,
    	(t_method)pdp_rec_free, sizeof(t_pdp_rec), 0, A_NULL);

    CLASS_MAINSIGNALIN(pdp_rec_class, t_pdp_rec, x_f );
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_close, gensym("close"), A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_frame_rate, gensym("framerate"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_compressor, gensym("compressor"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_acompressor, gensym("acompressor"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_jpeg, gensym("jpeg"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_start, gensym("start"), A_NULL);
    class_addmethod(pdp_rec_class, (t_method)pdp_rec_stop, gensym("stop"), A_NULL);


}

#ifdef __cplusplus
}
#endif
