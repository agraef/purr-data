/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon  (ydegoyon@free.fr)
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

/*  This object is an adaptation of radioactiv effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define COLORS 32
#define MAGIC_THRESHOLD 40
#define RATIO 0.95
#define DELTA (255/(COLORS/2-1))

#define VIDEO_HWIDTH (x->x_buf_width/2)
#define VIDEO_HHEIGHT (x->x_buf_height/2)

static char   *pdp_radioactiv_version = "pdp_radioactiv: version 0.1, port of radioactiv effect from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_radioactiv_struct
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
    unsigned char *x_blurzoombuf;
    int *x_blurzoomx;
    int *x_blurzoomy;
    int x_buf_width_blocks;
    int x_buf_width;
    int x_buf_height;
    int x_buf_area;
    int x_buf_margin_right;
    int x_buf_margin_left;
    int x_palette[COLORS];
    int x_mode;            /* 0=normal 1=strobe 2=strobe2 3=trigger */
    int x_snap_time;
    int x_snap_interval;
    short int *x_snapframe;
    short int *x_diff;
    short int *x_bdata;
    int x_snapshot;

} t_pdp_radioactiv;

static void pdp_radioactiv_free_ressources(t_pdp_radioactiv *x)
{
   if ( x->x_blurzoombuf ) freebytes ( x->x_blurzoombuf, x->x_buf_area*2 );
   if ( x->x_blurzoomx ) freebytes ( x->x_blurzoomx, x->x_buf_width*sizeof(int) );
   if ( x->x_blurzoomy ) freebytes ( x->x_blurzoomy, x->x_buf_height*sizeof(int) );
   if ( x->x_snapframe ) freebytes ( x->x_snapframe, ( ( x->x_vsize + x->x_vsize>>1 ) << 1 ) );
   if ( x->x_diff ) freebytes( x->x_diff, (x->x_vsize + (x->x_vsize>>1))<<1 );
   if ( x->x_bdata ) freebytes( x->x_bdata, (( x->x_vsize + (x->x_vsize>>1))<<1));
}

static void pdp_radioactiv_allocate(t_pdp_radioactiv *x)
{
 int i;

   x->x_buf_width_blocks = (x->x_vwidth / 32);
   x->x_buf_width = x->x_buf_width_blocks * 32;
   x->x_buf_height = x->x_vheight;
   x->x_buf_area = x->x_buf_width * x->x_buf_height;
   x->x_buf_margin_left = (x->x_vwidth - x->x_buf_width)/2;
   x->x_buf_margin_right = x->x_vwidth - x->x_buf_width - x->x_buf_margin_left;

   x->x_blurzoombuf = (unsigned char *) getbytes (x->x_buf_area*2);
   x->x_blurzoomx = (int *) getbytes (x->x_buf_width*sizeof(int));
   x->x_blurzoomy = (int *) getbytes (x->x_buf_height*sizeof(int));
   x->x_snapframe = (short int *) getbytes ( ( ( x->x_vsize + x->x_vsize>>1 ) << 1 ) );
   x->x_diff = (short int*) getbytes((x->x_vsize + (x->x_vsize>>1))<<1);
   x->x_bdata = (short int *) getbytes((( x->x_vsize + (x->x_vsize>>1))<<1));

   if ( !x->x_blurzoombuf || !x->x_blurzoomx || !x->x_blurzoomy || 
        !x->x_snapframe || !x->x_diff || !x->x_bdata )
   {
      post( "pdp_radioactiv : severe error : cannot allocate buffers !!!" );
      return;
   }
}

/* check if there is a real difference with background image */
static void pdp_radioactiv_diff(t_pdp_radioactiv *x, short int *src)
{
   int i;
   int Yy=0, Yu=0, Yv=0;
   int Yby=0, Ybu=0, Ybv=0;
   short int *p=NULL;
   short int *pb=NULL;
   short int *r=NULL;
   int v;

   p = src;
   pb = x->x_bdata;
   r = x->x_diff;
   for(i=0; i<(x->x_vsize); i++)
   {
         Yy = (*p);
         Yu = (*(p+x->x_vsize+(i>>2)));
         if ( x->x_vsize+(x->x_vsize>>2)+(i>>2) > x->x_vsize+(x->x_vsize>>1) )
         {
            post ("pdp_mosaic : overflow : offset=%d limit=%d", x->x_vsize+(x->x_vsize>>2)+(i>>2),
                   x->x_vsize+(x->x_vsize>>1) );
            return;
         }
         Yv = (*(p+x->x_vsize+(x->x_vsize>>2)+(i>>2)));
         Yby = (*pb);
         Ybu = (*(pb+x->x_vsize+(i>>2)));
         Ybv = (*(pb+x->x_vsize+(x->x_vsize>>2)+(i>>2)));
         if ( !r ) { post( "pdp_mosaic : hey, buffers are not allocated !!" ); return; };
         *r = ( (Yy - Yby) + (Yu - Ybu) + (Yv - Ybv) );
         r++;
   }

}

static void pdp_radioactiv_make_palette(t_pdp_radioactiv *x)
{
  int i;

    for(i=0; i<COLORS/2; i++) 
    {
      x->x_palette[i] = i*DELTA;
    }
    for(i=0; i<COLORS/2; i++) 
    {
      x->x_palette[i+COLORS/2] = 255 | (i*DELTA)<<16 | (i*DELTA)<<8;
    }
    for(i=0; i<COLORS; i++) 
    {
      x->x_palette[i] = x->x_palette[i] & 0xfefeff;
    }
}

/* this table assumes that video_width is times of 32 */
static void pdp_radioactiv_set_table(t_pdp_radioactiv *x)
{
  unsigned int bits;
  int px, py, tx, ty, xx;
  int ptr=0, prevptr=0;

    prevptr = (int)(0.5+RATIO*(-VIDEO_HWIDTH)+VIDEO_HWIDTH);
    for(xx=0; xx<(x->x_buf_width_blocks); xx++)
    {
       bits = 0;
       for(px=0; px<32; px++)
       {
          ptr = (int)(0.5+RATIO*((xx*32)+px-VIDEO_HWIDTH)+VIDEO_HWIDTH);
          bits = bits>>1;
          if(ptr != prevptr) bits |= 0x80000000;
          prevptr = ptr;
       }
       x->x_blurzoomx[xx] = bits;
    }

    ty = (int)(0.5+RATIO*(-VIDEO_HHEIGHT)+VIDEO_HHEIGHT);
    tx = (int)(0.5+RATIO*(-VIDEO_HWIDTH)+VIDEO_HWIDTH);
    xx=(int)(0.5+RATIO*(x->x_buf_width-1-VIDEO_HWIDTH)+VIDEO_HWIDTH);
    x->x_blurzoomy[0] = ty * x->x_buf_width + tx;
    prevptr = ty * x->x_buf_width + xx;
    for(py=1; py<x->x_buf_height; py++)
    {
      ty = (int)(0.5+RATIO*(py-VIDEO_HHEIGHT)+VIDEO_HHEIGHT);
      x->x_blurzoomy[py] = ty * x->x_buf_width + tx - prevptr;
      prevptr = ty * x->x_buf_width + xx;
    }
}

static void pdp_radioactiv_mode(t_pdp_radioactiv *x, t_floatarg fmode )
{
   if ( ( fmode > 0 ) || ( fmode < 4 ) )
   {
      x->x_mode = (int)fmode;
      if(x->x_mode == 3)
      {
        x->x_snap_time = 1;
      }
      else
      {
        x->x_snap_time = 0;
      }
   }
}

static void pdp_radioactiv_snap_time(t_pdp_radioactiv *x, t_floatarg fsnaptime )
{
   if ( fsnaptime > 0 )
   {
      x->x_snap_time = (int) fsnaptime;
   }
}

static void pdp_radioactiv_snap_interval(t_pdp_radioactiv *x, t_floatarg fsnapinterval )
{
   if ( fsnapinterval > 1 ) 
   {
      x->x_snap_interval = (int) fsnapinterval;
   }
}

static void pdp_radioactiv_blur(t_pdp_radioactiv *x)
{
  int px, py;
  int width;
  unsigned char *p, *q;
  unsigned char v;

    width = x->x_buf_width;
    p = x->x_blurzoombuf + width + 1;
    q = p + x->x_buf_area;

    for(py=x->x_buf_height-2; py>0; py--) 
    {
      for(px=x->x_vwidth-2; px>0; px--) 
      {
        v = (*(p-width) + *(p-1) + *(p+1) + *(p+width))/4 - 1;
        if(v == 255) v = 0;
        *q = v;
        p++;
        q++;
      }
      p += 2;
      q += 2;
   }
}

static void pdp_radioactiv_zoom(t_pdp_radioactiv *x)
{
  int b, px, py;
  unsigned char *p, *q;
  int blocks, height;
  int dx;

    p = x->x_blurzoombuf + x->x_buf_area;
    q = x->x_blurzoombuf;
    height = x->x_buf_height;
    blocks = x->x_buf_width_blocks;

    for(py=0; py<height; py++) 
    {
      p += x->x_blurzoomy[py];
      for(b=0; b<blocks; b++) 
      {
        dx = x->x_blurzoomx[b];
        for(px=0; px<32; px++) 
        {
          p += (dx & 1);
          *q++ = *p;
          dx = dx>>1;
        }
      }
    }
}

static void pdp_radioactiv_blurzoom(t_pdp_radioactiv *x)
{
   pdp_radioactiv_blur(x);
   pdp_radioactiv_zoom(x);
}

static void pdp_radioactiv_process_yv12(t_pdp_radioactiv *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    int px, py;
    short int a, b;
    unsigned char *p;
    short int *diff, *src;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize ) 
    {
        pdp_radioactiv_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_radioactiv_allocate(x);
        post( "pdp_radioactiv : reallocating buffers" );
        pdp_radioactiv_set_table(x);
        post( "pdp_radioactiv : set table" );
    }

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    if ( x->x_bdata && x->x_snapshot )
    {
       x->x_snapshot = 0;
       memcpy( x->x_bdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1);

    if(x->x_mode != 2 || x->x_snap_time <= 0) 
    {
       pdp_radioactiv_diff(x, data);
       if(x->x_mode == 0 || x->x_snap_time <= 0) 
       {
           diff = x->x_diff + x->x_buf_margin_left;
           p = x->x_blurzoombuf;
           for(py=0; py<x->x_buf_height; py++) 
           {
             for(px=0; px<x->x_buf_width; px++) 
             {
               p[px] |= diff[px] >> 3;
             }
             diff += x->x_vwidth;
             p += x->x_buf_width;
           }
           if( ( x->x_mode == 1 ) || ( x->x_mode == 2 )) 
           {
             memcpy(x->x_snapframe, data, ( ( x->x_vsize + x->x_vsize>>1 ) << 1 ) );
           }
       }
    }
    pdp_radioactiv_blurzoom(x);

    if( ( x->x_mode == 1 ) || ( x->x_mode == 2 )) 
    {
       src = x->x_snapframe;
    }
    else
    {
       src = data;
    }

    p = x->x_blurzoombuf;
    for(py=0; py<x->x_vheight; py++) 
    {
      for(px=0; px<x->x_buf_margin_left; px++) 
      {
        *newdata++ = *src++;
      }
      for(px=0; px<x->x_buf_width; px++) 
      {
        a = *src++ & 0xfeff;
        b = x->x_palette[*p++];
        a += b;
        b = a & 0x0100;
        *newdata++ = a | (b - (b >> 8));
      }
      for(px=0; px<x->x_buf_margin_right; px++) 
      {
        *newdata++ = *src++;
      }
   }

   if( ( x->x_mode == 1 ) || ( x->x_mode == 2 ) ) 
   {
      x->x_snap_time--;
      if(x->x_snap_time < 0) {
        x->x_snap_time = x->x_snap_interval;
      }
    }

    return;
}

static void pdp_radioactiv_sendpacket(t_pdp_radioactiv *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_radioactiv_process(t_pdp_radioactiv *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_radioactiv_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_radioactiv_process_yv12, pdp_radioactiv_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_radioactiv_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_radioactiv_process */
	    break;
	    
	}
    }
}

static void pdp_radioactiv_input_0(t_pdp_radioactiv *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_radioactiv_process(x);

    }
}

static void pdp_radioactiv_free(t_pdp_radioactiv *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_radioactiv_free_ressources(x);

}

t_class *pdp_radioactiv_class;

void *pdp_radioactiv_new(void)
{
    int i;

    t_pdp_radioactiv *x = (t_pdp_radioactiv *)pd_new(pdp_radioactiv_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("mode"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("snaptime"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("snapinterval"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_mode = 0; 	/* 0=normal/1=strobe/2=strobe2/3=trigger */
    x->x_blurzoombuf = NULL;
    x->x_snapframe = NULL;
    x->x_snap_time = 0;
    x->x_snap_interval = 3;
    x->x_blurzoombuf = NULL;
    x->x_blurzoomx = NULL;
    x->x_blurzoomy = NULL;
    x->x_snapframe = NULL;
    x->x_diff = NULL;

    pdp_radioactiv_make_palette(x);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_radioactiv_setup(void)
{
//    post( pdp_radioactiv_version );
    pdp_radioactiv_class = class_new(gensym("pdp_radioactiv"), (t_newmethod)pdp_radioactiv_new,
    	(t_method)pdp_radioactiv_free, sizeof(t_pdp_radioactiv), 0, A_NULL);

    class_addmethod(pdp_radioactiv_class, (t_method)pdp_radioactiv_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_radioactiv_class, (t_method)pdp_radioactiv_mode, gensym("mode"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_radioactiv_class, (t_method)pdp_radioactiv_snap_time, gensym("snaptime"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_radioactiv_class, (t_method)pdp_radioactiv_snap_interval, gensym("snapinterval"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
