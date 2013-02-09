/*
 *   Pure Data Packet module.
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



#include "pdp.h"
#include "pidip_config.h"
#include "pdp_llconv.h"
#include "time.h"
#include "sys/time.h"
#ifdef QUICKTIME_NEWER
#include <lqt/lqt.h>
#include <lqt/colormodels.h>
#else
#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>
#endif

typedef struct pdp_fqt_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_curframe;
    t_outlet *x_nbframes;
    t_outlet *x_framerate;

    int packet0;
    bool initialized;

    int x_vwidth;
    int x_vheight;
    int x_size;
    int x_fsize; // frames size
    int x_length;
    int x_current_frame;
    int x_cursec;
    int x_framescount;

    unsigned char *qt_rows[3];

    unsigned char *qt_frame;
    quicktime_t *qt;
    int qt_cmodel;

    unsigned char **x_frames;
    int* x_fsizes;

} t_pdp_fqt;



static void pdp_fqt_close(t_pdp_fqt *x)
{
  int fi;

    if (x->initialized){
	quicktime_close(x->qt);
	if ( x->qt_frame ) freebytes(x->qt_frame, x->x_size*3/2);
        for ( fi=0; fi<x->x_length; fi++ )
        {
          if ( x->x_frames[fi] ) freebytes( x->x_frames[fi], x->x_fsizes[fi] );
        }
        if ( x->x_frames ) freebytes( x->x_frames, x->x_length*sizeof(unsigned char*) );
	x->initialized = false;
    }

}

static void pdp_fqt_open(t_pdp_fqt *x, t_symbol *name)
{
  int fi;

    post("pdp_fqt: opening %s", name->s_name);

    pdp_fqt_close(x);

    x->qt = quicktime_open(name->s_name, 1, 0); // read=yes, write=no 

    if (!(x->qt)){
	post("pdp_fqt: error opening qt file");
	x->initialized = false;
	return;
    }

    if (!quicktime_has_video(x->qt)) {
	post("pdp_fqt: no video stream");
	quicktime_close(x->qt);
	x->initialized = false;
	return;
	
    }
    else if (!quicktime_supported_video(x->qt,0)) {
	post("pdp_fqt: unsupported video codec\n");
	quicktime_close(x->qt);
	x->initialized = false;
	return;
    }
    else 
    {
	x->qt_cmodel = BC_YUV420P;
	x->x_vwidth  = quicktime_video_width(x->qt,0);
	x->x_vheight = quicktime_video_height(x->qt,0);
	x->x_size = x->x_vwidth * x->x_vheight;
	x->qt_frame = (unsigned char*)getbytes(x->x_size*3/2);
	x->qt_rows[0] = &x->qt_frame[0];
	x->qt_rows[2] = &x->qt_frame[x->x_size];
	x->qt_rows[1] = &x->qt_frame[x->x_size + (x->x_size>>2)];
    
	quicktime_set_cmodel(x->qt, x->qt_cmodel);
	x->initialized = true;
        x->x_length = quicktime_video_length(x->qt,0);
	outlet_float(x->x_nbframes, (float)x->x_length);

    }

    // read all frames
    x->x_current_frame = 0;
    x->x_frames = (unsigned char**) getbytes( x->x_length*sizeof(unsigned char*) );
    x->x_fsizes = (int*) getbytes( x->x_length*sizeof(int) );
    x->x_fsize = 0;
    if ( !x->x_frames )
    {
      post("pdp_fqt: couldn't allocate memory for frames" );
      quicktime_close(x->qt);
      x->initialized = false;
      return;
    }

    for ( fi=0; fi<x->x_length; fi++ )
    {
       x->x_fsizes[fi] = (x->x_size)+((x->x_vwidth>>1)*(x->x_vheight>>1)<<1);
       x->x_fsize += x->x_fsizes[fi];
       x->x_frames[fi] = (unsigned char*) getbytes( x->x_fsizes[fi] );
       if ( !x->x_frames[fi] )
       {
         post("pdp_fqt: couldn't allocate memory for frames" );
         quicktime_close(x->qt);
         x->initialized = false;
         return;
       }

       lqt_decode_video(x->qt, x->qt_rows, 0);

       switch(x->qt_cmodel){
       case BC_YUV420P:
           memcpy(x->x_frames[fi], x->qt_frame, x->x_fsizes[fi] );
           break;

       default:
           post("pdp_fqt : error on decode: unkown colour model");
           break;
       }
    
    }

    post("pdp_fqt: allocated memory for %d frames (size=%db %dM)", 
                   x->x_length, x->x_fsize, x->x_fsize/(1024*1024) );
}


static void pdp_fqt_bang(t_pdp_fqt *x)
{
  int object;
  short int* data;
  t_pdp* header;
  struct timeval etime;

    if (!(x->initialized)){
	post("pdp_fqt: no qt file opened");
	return;
    }

    object = pdp_packet_new_bitmap_yv12( x->x_vwidth, x->x_vheight );
    header = pdp_packet_header(object);
    data = (short int *) pdp_packet_data(object);

    header->info.image.encoding = PDP_BITMAP_YV12;
    header->info.image.width = x->x_vwidth;
    header->info.image.height = x->x_vheight;

    // post( "pdp_fqt : current frame : %d size : %d", 
    //        x->x_current_frame, x->x_fsizes[x->x_current_frame] );
    memcpy( data, x->x_frames[x->x_current_frame], x->x_fsizes[x->x_current_frame] );

    if ( gettimeofday(&etime, NULL) == -1)
    {
        post("pdp_fqt : could not get time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       outlet_float(x->x_framerate, (float)x->x_framescount);
       x->x_framescount = 0;
    }
    x->x_framescount++;

    x->x_current_frame = ( x->x_current_frame + 1 ) % x->x_length;
    outlet_float(x->x_curframe, (float)x->x_current_frame);
    pdp_packet_pass_if_valid(x->x_outlet0, &object);

}

static void pdp_fqt_frame_cold(t_pdp_fqt *x, t_floatarg frameindex)
{
  int frame = (int)frameindex;

    if (!(x->initialized)) return;

    frame = (frame >= x->x_length) ? x->x_length-1 : frame;
    frame = (frame < 0) ? 0 : frame;

    // post("pdp_fqt : frame cold : setting video position to : %d", frame );
    x->x_current_frame = frame;

}

static void pdp_fqt_frame(t_pdp_fqt *x, t_floatarg frameindex)
{
    // pdp_fqt_frame_cold(x, frameindex);
    pdp_fqt_bang(x);
}

static void pdp_fqt_free(t_pdp_fqt *x)
{
    pdp_fqt_close(x);
}

t_class *pdp_fqt_class;

void *pdp_fqt_new(void)
{
    t_pdp_fqt *x = (t_pdp_fqt *)pd_new(pdp_fqt_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("frame_cold"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);
    x->x_curframe = outlet_new(&x->x_obj, &s_float);
    x->x_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_framerate = outlet_new(&x->x_obj, &s_float);

    x->packet0 = -1;

    x->initialized = false;

    return (void *)x;
}

#ifdef __cplusplus
extern "C"
{
#endif


void pdp_fqt_setup(void)
{
    pdp_fqt_class = class_new(gensym("pdp_fqt"), (t_newmethod)pdp_fqt_new,
    	(t_method)pdp_fqt_free, sizeof(t_pdp_fqt), 0, A_NULL);

    class_addmethod(pdp_fqt_class, (t_method)pdp_fqt_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_fqt_class, (t_method)pdp_fqt_close, gensym("close"), A_NULL);
    class_addmethod(pdp_fqt_class, (t_method)pdp_fqt_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addfloat (pdp_fqt_class, (t_method)pdp_fqt_frame);
    class_addmethod(pdp_fqt_class, (t_method)pdp_fqt_frame_cold, gensym("frame_cold"), A_FLOAT, A_NULL);
    class_addmethod(pdp_fqt_class, nullfn, gensym("signal"), 0);


}

#ifdef __cplusplus
}
#endif
