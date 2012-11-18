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



#include "pdp.h"
#include "pdp_llconv.h"
#include "time.h"
#include "sys/time.h"
#include "pidip_config.h"
#ifdef QUICKTIME_NEWER
#include <lqt/lqt.h>
#include <lqt/colormodels.h>
#else
#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>
#endif

#define     MIN_AUDIO_INPUT           1024  /* we must have at least n chunks to play a steady sound */
#define     OUTPUT_BUFFER_SIZE        128*1024  /* audio output buffer : 128k */
#define     DECODE_PACKET_SIZE        16*1024  /* size of audio data decoded in one call */

typedef struct pdp_yqt_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_curframe;
    t_outlet *x_nbframes;
    t_outlet *x_framerate;
    t_outlet *x_ol;   /* audio left channel  */
    t_outlet *x_or;   /* audio right channel */

    int packet0;
    bool initialized;

    unsigned int x_vwidth;
    unsigned int x_vheight;
    int x_cursec;
    int x_framescount;

    bool loop;

    unsigned char * qt_rows[3];

    unsigned char *qt_frame;
    quicktime_t *qt;
    int qt_cmodel;

    int    x_audio;             /* indicates the existence of an audio track */
    int    x_audio_channels;	  /* number of audio channels of first track   */
    int    x_mono;              /* indicates a mono audio track              */
    int    x_audio_rate;        /* audio rate                                */
    int    x_resampling_factor; /* resampling factor                         */

    t_float *x_outbuffer;         /* buffer to store audio decoded data        */
    int    x_outwriteposition;
    int    x_outreadposition;
    int    x_outunread;
    int    x_outbuffersize;
    t_float  *x_outl;
    t_float  *x_outr;
    t_float  **x_outs;

} t_pdp_yqt;



static void pdp_yqt_close(t_pdp_yqt *x)
{
    if (x->initialized){
	quicktime_close(x->qt);
	free(x->qt_frame);
	x->initialized = false;
    }

}

static void pdp_yqt_open(t_pdp_yqt *x, t_symbol *name)
{
    unsigned int size;

    post("pdp_yqt: opening %s", name->s_name);

    pdp_yqt_close(x);

    x->qt = quicktime_open(name->s_name, 1, 0);

    if (!(x->qt)){
	post("pdp_yqt: error opening qt file");
	x->initialized = false;
	return;
    }

    if (!quicktime_has_video(x->qt)) {
	post("pdp_yqt: no video stream");
	quicktime_close(x->qt);
	x->initialized = false;
	return;
	
    }
    else if (!quicktime_supported_video(x->qt,0)) {
	post("pdp_yqt: unsupported video codec\n");
	quicktime_close(x->qt);
	x->initialized = false;
	return;
    }
    else 
    {
	x->qt_cmodel = BC_YUV420P;
	x->x_vwidth  = quicktime_video_width(x->qt,0);
	x->x_vheight = quicktime_video_height(x->qt,0);
	x->qt_frame = (unsigned char*)malloc(x->x_vwidth*x->x_vheight*4);
	size = x->x_vwidth * x->x_vheight;
	x->qt_rows[0] = &x->qt_frame[0];
	x->qt_rows[2] = &x->qt_frame[size];
	x->qt_rows[1] = &x->qt_frame[size + (size>>2)];
    
	quicktime_set_cmodel(x->qt, x->qt_cmodel);
	x->initialized = true;
	outlet_float(x->x_nbframes, (float)quicktime_video_length(x->qt,0));

    }

    if (!quicktime_has_audio(x->qt)) {
	post("pdp_yqt: warning : no audio stream");
        x->x_audio = 0;
        return;
    }
    
    if ( quicktime_audio_tracks(x->qt) > 1 )
    {
	post("pdp_yqt: warning : more that one audio track, using first one");
    }

    if ( ( x->x_audio_channels = quicktime_track_channels(x->qt, 0) ) != 2 ) {
	x->x_mono=0;
        post("pdp_yqt: track 0 has %d channels", x->x_audio_channels );
        post("pdp_yqt: warning : not a stereo audio track ( audio channels : %d )", x->x_audio_channels ); 
        if ( x->x_audio_channels == 1 ) x->x_mono = 1;
        else x->x_audio_channels = 2;
    } else {
        post("pdp_yqt: track 0 has %d channels", x->x_audio_channels );
    }

    if (!quicktime_supported_audio(x->qt,0)) {
        post("pdp_yqt: warning : audio not supported" ); 
	x->x_audio = 0;
    } else {
	x->x_audio = 1;
    }
   
    if ( x->x_audio )
    {
       post("pdp_yqt: using audio track 0 with %d channels", x->x_audio_channels );
       post("pdp_yqt: audio data is %d bytes, %d kHz compressed with %s", 
		quicktime_audio_bits(x->qt, 0),
		x->x_audio_rate = quicktime_sample_rate(x->qt, 0),
		quicktime_audio_compressor(x->qt, 0) );
       x->x_resampling_factor = ( sys_getsr() / x->x_audio_rate );
       x->x_outreadposition = 0;
       x->x_outwriteposition = 0;
       x->x_outunread = 0;
    }

}


static void pdp_yqt_bang(t_pdp_yqt *x)
{
  unsigned int w, h, nbpixels, packet_size;
  int object, length, pos, i, j;
  short int* data;
  t_pdp* header;
  struct timeval etime;

    if (!(x->initialized)){
	//post("pdp_yqt: no qt file opened");
	return;
    }

    w = x->x_vwidth;
    h = x->x_vheight;
    nbpixels = w * h;
    packet_size = (nbpixels + (nbpixels >> 1)) << 1;

    object = pdp_packet_new_image_YCrCb( x->x_vwidth, x->x_vheight );
    header = pdp_packet_header(object);
    data = (short int *) pdp_packet_data(object);

    header->info.image.encoding = PDP_IMAGE_YV12;
    header->info.image.width = w;
    header->info.image.height = h;

    length = quicktime_video_length(x->qt,0);
    pos = quicktime_video_position(x->qt,0);
    // post("pdp_yqt : video position : %d length =%d", pos, length );

    if (pos >= length){
	pos = (x->loop) ? 0 : length - 1;
        // post("pdp_yqt : setting video position to %d", pos);
	quicktime_set_video_position(x->qt, pos, 0);
	if (x->loop) 
        {
           // post("pdp_yqt : resetting audio position");
           if ( x->x_audio ) quicktime_set_audio_position(x->qt, 0, 0);
           x->x_outreadposition = 0;
           x->x_outwriteposition = 0;
           x->x_outunread = 0;
        }
    }

    lqt_decode_video(x->qt, x->qt_rows, 0);

    switch(x->qt_cmodel){
    case BC_YUV420P:
        pdp_llconv(x->qt_frame, RIF_YVU__P411_U8, data, RIF_YVU__P411_S16, x->x_vwidth, x->x_vheight);
        break;

    case BC_YUV422:
        pdp_llconv(x->qt_frame, RIF_YUYV_P____U8, data, RIF_YVU__P411_S16, x->x_vwidth, x->x_vheight);
        break;

    case BC_RGB888:
        pdp_llconv(x->qt_frame, RIF_RGB__P____U8, data, RIF_YVU__P411_S16, x->x_vwidth, x->x_vheight);
        break;

    default:
        post("pdp_yqt : error on decode: unkown colour model");
        break;
    }

    if ( gettimeofday(&etime, NULL) == -1)
    {
        post("pdp_yqt : could not get time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       outlet_float(x->x_framerate, (float)x->x_framescount);
       x->x_framescount = 0;
    }
    x->x_framescount++;
    
    outlet_float(x->x_curframe, (float)pos);

    pdp_packet_pass_if_valid(x->x_outlet0, &object);

}

static void pdp_yqt_loop(t_pdp_yqt *x, t_floatarg loop)
{
    int loopi = (int)loop;
    x->loop = !(loopi == 0);
}

static void pdp_yqt_frame_cold(t_pdp_yqt *x, t_floatarg frameindex)
{
    int frame = (int)frameindex;
    int length, sample;

    if (!(x->initialized)) return;

    length = quicktime_video_length(x->qt,0);

    frame = (frame >= length) ? length-1 : frame;
    frame = (frame < 0) ? 0 : frame;

    // post("pdp_yqt : frame cold : setting video position to : %d", frame );
    quicktime_set_video_position(x->qt, frame, 0);
    if ( x->x_audio )
    {
      sample = x->x_audio_rate*((float)frame/(float)quicktime_frame_rate (x->qt, 0));
      quicktime_set_audio_position(x->qt, sample, 0 );
      x->x_outreadposition = 0;
      x->x_outwriteposition = 0;
      x->x_outunread = 0;
    }
}

static void pdp_yqt_frame(t_pdp_yqt *x, t_floatarg frameindex)
{
    // pdp_yqt_frame_cold(x, frameindex);
    pdp_yqt_bang(x);
}

static void pdp_yqt_free(t_pdp_yqt *x)
{
  
    pdp_yqt_close(x);

    freebytes(x->x_outbuffer, OUTPUT_BUFFER_SIZE*sizeof(t_float));
    freebytes(x->x_outl, DECODE_PACKET_SIZE*sizeof(t_float));
    freebytes(x->x_outr, DECODE_PACKET_SIZE*sizeof(t_float));
}

t_class *pdp_yqt_class;

void *pdp_yqt_new(void)
{
    t_pdp_yqt *x = (t_pdp_yqt *)pd_new(pdp_yqt_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("frame_cold"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);
    x->x_curframe = outlet_new(&x->x_obj, &s_float);
    x->x_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_framerate = outlet_new(&x->x_obj, &s_float);

    x->x_ol = outlet_new(&x->x_obj, &s_signal);   /* audio left channel  */
    x->x_or = outlet_new(&x->x_obj, &s_signal);   /* audio right channel */

    x->packet0 = -1;

    x->initialized = false;

    x->loop = false;

    // allocate audio buffers
    x->x_outbuffersize = OUTPUT_BUFFER_SIZE;
    x->x_outl = (t_float*) getbytes(DECODE_PACKET_SIZE*sizeof(t_float));
    x->x_outr = (t_float*) getbytes(DECODE_PACKET_SIZE*sizeof(t_float));
    x->x_outs = (t_float**) getbytes( 2*sizeof(t_float*));
    x->x_outs[0] = x->x_outl;
    x->x_outs[1] = x->x_outr;
    x->x_outbuffer = (t_float*) getbytes(OUTPUT_BUFFER_SIZE*sizeof(t_float));

    if ( !x->x_outl || !x->x_outr || !x->x_outbuffer || !x->x_outs )
    {
       post( "pdp_yqt: not allocate buffers" );
       return NULL;
    }
    memset( x->x_outl, 0x0, DECODE_PACKET_SIZE*sizeof(t_float) );
    memset( x->x_outr, 0x0, DECODE_PACKET_SIZE*sizeof(t_float) );
    memset( x->x_outbuffer, 0x0, OUTPUT_BUFFER_SIZE*sizeof(t_float) );

    x->x_outreadposition = 0;
    x->x_outwriteposition = 0;
    x->x_outunread = 0;

    return (void *)x;
}

static t_int *pdp_yqt_perform(t_int *w)
{
     t_pdp_yqt *x = (t_pdp_yqt*) (w[1]);
     t_float *out1 = (t_float *)(w[2]);
     t_float *out2 = (t_float *)(w[3]);
     int n = (int)(w[4]);
     int ret;
     int i = 0;

    // fills in the audio buffer with a chunk if necessary
    if ( (x->initialized) && (quicktime_video_position(x->qt,0)>0) && x->x_audio && ( x->x_outunread < n ) )
    {
      int csize, rsize, i, j;

       // watch remaining size
       rsize = (int ) ( quicktime_audio_length(x->qt, 0) - quicktime_audio_position(x->qt, 0) );
       csize = ( rsize < DECODE_PACKET_SIZE ) ? rsize : DECODE_PACKET_SIZE;

       // post("pdp_yqt : decode one chunk (size=%d)", csize );
       if ( ( lqt_decode_audio(x->qt, NULL, x->x_outs, csize) <0 ) )
       {
	   post("pdp_yqt : could not decode audio data" );
       } else {
         for ( i=0; i<csize; i++ )
         {
            if ( x->x_outunread >= x->x_outbuffersize-2 )
            {
                post( "pdp_yqt: decode audio : too much input ... ignored" );
                continue;
            }
            for ( j=0; j<x->x_resampling_factor; j++ )
            {
               *(x->x_outbuffer+x->x_outwriteposition) = x->x_outl[i];
               x->x_outwriteposition = (x->x_outwriteposition + 1)%x->x_outbuffersize;
               *(x->x_outbuffer+x->x_outwriteposition) =
                   ((x->x_mono)? x->x_outl[i] : x->x_outr[i] );
               x->x_outwriteposition = (x->x_outwriteposition + 1)%x->x_outbuffersize;
               x->x_outunread+=2;
            }
         }
       }
    }

     while( n-- )
     {
        if ( x->x_audio && x->x_outunread >= 2 )
        {
          *out1++=*(x->x_outbuffer+x->x_outreadposition);
          x->x_outreadposition = (x->x_outreadposition + 1)%x->x_outbuffersize;
          *out2++=*(x->x_outbuffer+x->x_outreadposition);
          x->x_outreadposition = (x->x_outreadposition + 1)%x->x_outbuffersize;
          x->x_outunread-=2;
        }
        else
        {
          // should not happen 
          // post( "pdp_yqt: null audio samples" );
          *out1++=0.;
          // *out1++=*(x->x_outbuffer+x->x_outreadposition);
          *out2++=0.;
          // *out2++=*(x->x_outbuffer+x->x_outreadposition);
        }
     }

     return (w+5);
}

static void pdp_yqt_dsp(t_pdp_yqt *x, t_signal **sp)
{
    dsp_add(pdp_yqt_perform, 4, x, sp[1]->s_vec, sp[2]->s_vec, sp[1]->s_n);
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_yqt_setup(void)
{
    pdp_yqt_class = class_new(gensym("pdp_yqt"), (t_newmethod)pdp_yqt_new,
    	(t_method)pdp_yqt_free, sizeof(t_pdp_yqt), 0, A_NULL);

    class_addmethod(pdp_yqt_class, (t_method)pdp_yqt_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_yqt_class, (t_method)pdp_yqt_close, gensym("close"), A_NULL);
    class_addmethod(pdp_yqt_class, (t_method)pdp_yqt_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_yqt_class, (t_method)pdp_yqt_loop, gensym("loop"), A_DEFFLOAT, A_NULL);
    class_addfloat (pdp_yqt_class, (t_method)pdp_yqt_frame);
    class_addmethod(pdp_yqt_class, (t_method)pdp_yqt_frame_cold, gensym("frame_cold"), A_FLOAT, A_NULL);
    class_addmethod(pdp_yqt_class, nullfn, gensym("signal"), 0);
    class_addmethod(pdp_yqt_class, (t_method)pdp_yqt_dsp, gensym("dsp"), 0);
    class_sethelpsymbol( pdp_yqt_class, gensym("pdp_yqt.pd") );

}

#ifdef __cplusplus
}
#endif
