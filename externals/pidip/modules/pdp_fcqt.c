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
#include <bzlib.h> // bz2 compression routines

typedef struct pdp_fcqt_struct
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
    int x_vsize;
    int x_fsize; // frames size
    int x_length;
    int x_current_frame;
    int x_cursec;
    int x_framescount;

    unsigned char * qt_rows[3];

    unsigned char * qt_frame;
    quicktime_t *qt;
    int qt_cmodel;

    unsigned int** x_frames;
    unsigned int* x_fsizes;

} t_pdp_fcqt;



static void pdp_fcqt_close(t_pdp_fcqt *x)
{
  int fi;

    if (x->initialized){
	quicktime_close(x->qt);
	if ( x->qt_frame ) freebytes(x->qt_frame, x->x_vsize*3/2);
        for ( fi=0; fi<x->x_length; fi++ )
        {
          if ( x->x_frames[fi] ) freebytes( x->x_frames[fi], x->x_fsizes[fi] );
        }
        if ( x->x_frames ) freebytes( x->x_frames, x->x_length*sizeof(unsigned int*) );
	x->initialized = false;
    }

}

static void pdp_fcqt_open(t_pdp_fcqt *x, t_symbol *name)
{
  int fi, osize, ret;
  unsigned int *odata, *cdata;

    post("pdp_fcqt: opening %s", name->s_name);

    pdp_fcqt_close(x);

    x->qt = quicktime_open(name->s_name, 1, 0); // read=yes, write=no 

    if (!(x->qt)){
	post("pdp_fcqt: error opening qt file");
	x->initialized = false;
	return;
    }

    if (!quicktime_has_video(x->qt)) {
	post("pdp_fcqt: no video stream");
	quicktime_close(x->qt);
	x->initialized = false;
	return;
	
    }
    else if (!quicktime_supported_video(x->qt,0)) {
	post("pdp_fcqt: unsupported video codec\n");
	quicktime_close(x->qt);
	x->initialized = false;
	return;
    }
    else 
    {
	x->qt_cmodel = BC_YUV420P;
	x->x_vwidth  = quicktime_video_width(x->qt,0);
	x->x_vheight = quicktime_video_height(x->qt,0);
	x->x_vsize = x->x_vwidth * x->x_vheight;
	x->qt_frame = (unsigned char*)getbytes(x->x_vsize+(x->x_vsize>>1));
	x->qt_rows[0] = &x->qt_frame[0];
	x->qt_rows[2] = &x->qt_frame[x->x_vsize];
	x->qt_rows[1] = &x->qt_frame[x->x_vsize + (x->x_vsize>>2)];
    
	quicktime_set_cmodel(x->qt, x->qt_cmodel);
	x->initialized = true;
        x->x_length = quicktime_video_length(x->qt,0);
	outlet_float(x->x_nbframes, (float)x->x_length);

    }

    // read all frames
    x->x_current_frame = 0;
    x->x_frames = (unsigned int**) getbytes( x->x_length*sizeof(unsigned int*) );
    x->x_fsizes = (unsigned int*) getbytes( x->x_length*sizeof(int) );
    x->x_fsize = 0;
    if ( !x->x_frames )
    {
      post("pdp_fcqt: couldn't allocate memory for frames(x->x_frames)" );
      quicktime_close(x->qt);
      x->initialized = false;
      return;
    }

    // allocate data used for compression
    osize = ( x->x_vsize + (x->x_vsize>>1) ) << 1;
    odata = (unsigned int*) getbytes( osize );
    cdata = (unsigned int*) getbytes( osize );
    if ( !odata || !cdata )
    {
      post("pdp_fcqt: couldn't allocate memory for frames (odata/cdata)" );
      quicktime_close(x->qt);
      x->initialized = false;
      return;
    }

    for ( fi=0; fi<x->x_length; fi++ )
    {
       lqt_decode_video(x->qt, x->qt_rows, 0);

       switch(x->qt_cmodel){
       case BC_YUV420P:
           pdp_llconv(x->qt_frame, RIF_YVU__P411_U8, odata, RIF_YVU__P411_S16, 
                            x->x_vwidth, x->x_vheight);

           x->x_fsizes[fi] = osize;
           if ( ( ret = BZ2_bzBuffToBuffCompress( (char*)cdata,
                                  &x->x_fsizes[fi],
                                  (char*)odata,
                                  osize,
                                  9, 0, 0 ) ) == BZ_OK )
           {
              post( "pdp_fcqt : bz2 compression (%d)->(%d) gain:%d", 
                         osize, x->x_fsizes[fi], osize/x->x_fsizes[fi]  ); 
              x->x_fsize += x->x_fsizes[fi];
              x->x_frames[fi] = (unsigned int*) getbytes( x->x_fsizes[fi] );
              if ( !x->x_frames[fi] )
              {
                post("pdp_fcqt: couldn't allocate memory for frames" );
                quicktime_close(x->qt);
                x->initialized = false;
                return;
              }
              memcpy( x->x_frames[fi], cdata, x->x_fsizes[fi] );
           }
           else
           {
              post( "pdp_fcqt : bz2 compression failed (ret=%d)", ret );
           }
           break;

       default:
           post("pdp_fcqt : error on decode: unkown colour model");
           break;
       }
    
    }
    
    if ( odata ) freebytes( odata, osize );
    if ( cdata ) freebytes( cdata, osize );

    post("pdp_fcqt: allocated memory for %d frames (size=%db %dM)", 
                   x->x_length, x->x_fsize, x->x_fsize/(1024*1024) );
}


static void pdp_fcqt_bang(t_pdp_fcqt *x)
{
  int object, ret;
  unsigned int dsize, psize;
  short int* data;
  t_pdp* header;
  struct timeval etime;

    if (!(x->initialized)){
	//post("pdp_fcqt: no qt file opened");
	return;
    }

    object = pdp_packet_new_image_YCrCb( x->x_vwidth, x->x_vheight );
    header = pdp_packet_header(object);
    data = (short int *) pdp_packet_data(object);

    header->info.image.encoding = PDP_IMAGE_YV12;
    header->info.image.width = x->x_vwidth;
    header->info.image.height = x->x_vheight;

    x->x_current_frame = ( x->x_current_frame + 1 ) % x->x_length;
    // post( "pdp_fcqt : current frame : %d", x->x_current_frame );

    psize = (x->x_vsize+(x->x_vsize>>1))<<1;
    dsize = psize;
    if ( ( ret = BZ2_bzBuffToBuffDecompress( (char*)data,
                  &dsize,
                  (char *) x->x_frames[x->x_current_frame],
                  x->x_fsizes[x->x_current_frame],
                  0, 0 ) ) == BZ_OK )
    {
       // post( "pdp_fcqt : decompressed frame %d : (%d)->(%d)",
       //             x->x_current_frame, x->x_fsizes[x->x_current_frame], dsize );
       if ( dsize != psize )
       {
          post( "pdp_fcqt : warning : decompressed size seems wrong! : (%d)->(%d) (should be %d)",
                     x->x_fsizes[x->x_current_frame], dsize, psize );
       }
    }
    else
    {
       post( "pdp_fcqt : bz2 decompression failed (ret=%d)", ret );
       return;
    }

    if ( gettimeofday(&etime, NULL) == -1)
    {
        post("pdp_fcqt : could not get time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       outlet_float(x->x_framerate, (float)x->x_framescount);
       x->x_framescount = 0;
    }
    x->x_framescount++;

    outlet_float(x->x_curframe, (float)x->x_current_frame);
    pdp_packet_pass_if_valid(x->x_outlet0, &object);

}

static void pdp_fcqt_frame_cold(t_pdp_fcqt *x, t_floatarg frameindex)
{
    int frame = (int)frameindex;
    int length;


    if (!(x->initialized)) return;

    frame = (frame >= x->x_length) ? x->x_length-1 : frame;
    frame = (frame < 0) ? 0 : frame;

    // post("pdp_fcqt : frame cold : setting video position to : %d", frame );
    x->x_current_frame = frame;
}

static void pdp_fcqt_frame(t_pdp_fcqt *x, t_floatarg frameindex)
{
    // pdp_fcqt_frame_cold(x, frameindex);
    pdp_fcqt_bang(x);
}

static void pdp_fcqt_free(t_pdp_fcqt *x)
{
    pdp_fcqt_close(x);
}

t_class *pdp_fcqt_class;

void *pdp_fcqt_new(void)
{
    t_pdp_fcqt *x = (t_pdp_fcqt *)pd_new(pdp_fcqt_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("frame_cold"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);
    x->x_curframe = outlet_new(&x->x_obj, &s_float);
    x->x_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_framerate = outlet_new(&x->x_obj, &s_float);

    x->packet0 = -1;
    x->x_cursec = -1;
    x->x_framescount = 0;

    x->initialized = false;

    return (void *)x;
}

#ifdef __cplusplus
extern "C"
{
#endif


void pdp_fcqt_setup(void)
{
    pdp_fcqt_class = class_new(gensym("pdp_fcqt"), (t_newmethod)pdp_fcqt_new,
    	(t_method)pdp_fcqt_free, sizeof(t_pdp_fcqt), 0, A_NULL);

    class_addmethod(pdp_fcqt_class, (t_method)pdp_fcqt_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_fcqt_class, (t_method)pdp_fcqt_close, gensym("close"), A_NULL);
    class_addmethod(pdp_fcqt_class, (t_method)pdp_fcqt_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addfloat (pdp_fcqt_class, (t_method)pdp_fcqt_frame);
    class_addmethod(pdp_fcqt_class, (t_method)pdp_fcqt_frame_cold, gensym("frame_cold"), A_FLOAT, A_NULL);
    class_addmethod(pdp_fcqt_class, nullfn, gensym("signal"), 0);


}

#ifdef __cplusplus
}
#endif
