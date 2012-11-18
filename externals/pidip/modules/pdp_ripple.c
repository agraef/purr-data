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

/*  This object is an adaptation of ripple effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define MAGIC_THRESHOLD 30

static unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

static int sqrtable[256];
static int sqrt_init=1;
static const int point = 16;
static const int impact = 2;
static const int decay = 8;
static const int loopnum = 2;
static int period = 0;
static int rain_stat = 0;
static unsigned int drop_prob = 0;
static int drop_prob_increment = 0;
static int drops_per_frame_max = 0;
static int drops_per_frame = 0;
static int drop_power = 0;

static char   *pdp_ripple_version = "pdp_ripple: version 0.1, port of ripple from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_ripple_struct
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
    int x_maph;
    int x_mapw;
    int x_mode;
    int x_threshold;
    int *x_map;
    int *x_map1;
    int *x_map2;
    int *x_map3;
    signed char *x_vtable;
    short int *x_diff;
    short int *x_bdata;
    int x_snapshot;

} t_pdp_ripple;

static void pdp_ripple_mode(t_pdp_ripple *x, t_floatarg fmode )
{
   if ( ( fmode == 0 ) || ( fmode == 1 ) )
   {
       x->x_mode = (int)fmode;
   }
}

static void pdp_ripple_threshold(t_pdp_ripple *x, t_floatarg fthreshold )
{
   x->x_threshold = (int)fthreshold;
}

static void pdp_ripple_increment(t_pdp_ripple *x, t_floatarg fincrement )
{
   drop_prob_increment = (int)fincrement;
}

static void pdp_ripple_background(t_pdp_ripple *x )
{
   x->x_snapshot = 1;
}

static void pdp_ripple_free_ressources(t_pdp_ripple *x)
{
  if ( x->x_diff != NULL ) freebytes( x->x_diff, (x->x_vsize + (x->x_vsize>>1))<<1 );
  if ( x->x_bdata ) freebytes( x->x_bdata, (( x->x_vsize + (x->x_vsize>>1))<<1));
  if ( x->x_map ) freebytes(x->x_map, x->x_maph*x->x_mapw*3*sizeof(int));
  if ( x->x_vtable ) freebytes(x->x_vtable, x->x_maph*x->x_mapw*2*sizeof(signed char));
}

static void pdp_ripple_allocate(t_pdp_ripple *x)
{
 int i;

  x->x_diff = (short int*) getbytes((x->x_vsize + (x->x_vsize>>1))<<1);
  x->x_bdata = (short int *) getbytes((( x->x_vsize + (x->x_vsize>>1))<<1));
  x->x_maph = x->x_vheight / 2 + 1;
  x->x_mapw = x->x_vwidth / 2 + 1;
  x->x_map = (int *)getbytes(x->x_maph*x->x_mapw*3*sizeof(int));
  x->x_vtable = (signed char *)getbytes(x->x_maph*x->x_mapw*2*sizeof(signed char));
  if( !x->x_map || !x->x_vtable || !x->x_bdata || !x->x_diff ) {
      post( "pdp_ripple : severe error : cannot allocate buffers" );
  }
  x->x_map1 = x->x_map;
  x->x_map2 = x->x_map + x->x_maph * x->x_mapw;
  x->x_map3 = x->x_map + x->x_mapw * x->x_maph * 2;
}

/* check if there is a real difference with background image */
short int *pdp_ripple_diff(t_pdp_ripple *x, short int *src)
{
   int i;
   int Y;
   int Yb;
   short int *p=NULL;
   short int *pb=NULL;
   short int *r=NULL;
   int v;

   p = src;
   pb = x->x_bdata;
   r = x->x_diff;
   for(i=0; i<(x->x_vsize); i++) {
         Y = (*p);
         Yb = (*pb);
         *r = ( (Yb - Y) > x->x_threshold ) ? (Yb - Y) : 0;
         p++; pb++;
         r++;
   }

   return x->x_diff;
}

static void pdp_ripple_motion_detect(t_pdp_ripple *x, short int *src)
{
  short int *diff;
  int width;
  int *p, *q;
  int px, py, h;

    diff = pdp_ripple_diff(x, src);
    width = x->x_vwidth;
    p = x->x_map1+x->x_mapw+1;
    q = x->x_map2+x->x_mapw+1;
    diff += width+2;

    for(py=x->x_maph-2; py>0; py--) 
    {
      for(px=x->x_mapw-2; px>0; px--) 
      {
         h = (int)*diff;// + (int)*(diff+1) + (int)*(diff+width) + (int)*(diff+width+1);
         if(h>0) {
            *p = h<<(point + impact - 8);
            *q = *p;
         }
         p++;
         q++;
         diff += 2;
      }
      diff += width+2;
      p+=2;
      q+=2;
    }
}

static inline void pdp_ripple_drop(t_pdp_ripple *x, int power)
{
  int px, py;
  int *p, *q;

    px = inline_fastrand()%(x->x_mapw-4)+2;
    py = inline_fastrand()%(x->x_maph-4)+2;
    p = x->x_map1 + py*x->x_mapw + px;
    q = x->x_map2 + py*x->x_mapw + px;
    *p = power;
    *q = power;
    *(p-x->x_mapw) = *(p-1) = *(p+1) = *(p+x->x_mapw) = power/2;
    *(p-x->x_mapw-1) = *(p-x->x_mapw+1) = *(p+x->x_mapw-1) = *(p+x->x_mapw+1) = power/4;
    *(q-x->x_mapw) = *(q-1) = *(q+1) = *(q+x->x_mapw) = power/2;
    *(q-x->x_mapw-1) = *(q-x->x_mapw+1) = *(q+x->x_mapw-1) = *(p+x->x_mapw+1) = power/4;
}

static void pdp_ripple_raindrop(t_pdp_ripple *x)
{
  int i;

   if(period == 0) 
   {
      switch(rain_stat) 
      {
        case 0:
           period = (inline_fastrand()>>23)+100;
           drop_prob = 0;
           drop_prob_increment = 0x00ffffff/period;
           drop_power = (-(inline_fastrand()>>28)-2)<<point;
           drops_per_frame_max = 2<<(inline_fastrand()>>30); // 2,4,8 or 16
           rain_stat = 1;
           break;
         case 1:
           drop_prob = 0x00ffffff;
           drops_per_frame = 1;
           drop_prob_increment = 1;
           period = (drops_per_frame_max - 1) * 16;
           rain_stat = 2;
           break;
         case 2:
           period = (inline_fastrand()>>22)+1000;
           drop_prob_increment = 0;
           rain_stat = 3;
           break;
         case 3:
           period = (drops_per_frame_max - 1) * 16;
           drop_prob_increment = -1;
           rain_stat = 4;
           break;
         case 4:
           period = (inline_fastrand()>>24)+60;
           drop_prob_increment = -(drop_prob/period);
           rain_stat = 5;
           break;
         case 5:
         default:
           period = (inline_fastrand()>>23)+500;
           drop_prob = 0;
           rain_stat = 0;
           break;
        }
      }
      switch(rain_stat) 
      {
        default:
        case 0:
          break;
        case 1:
        case 5:
          if((inline_fastrand()>>8)<drop_prob) 
          {
             pdp_ripple_drop(x, drop_power);
          }
          drop_prob += drop_prob_increment;
          break;
        case 2:
        case 3:
        case 4:
          for(i=drops_per_frame/16; i>0; i--)
          {
             pdp_ripple_drop(x, drop_power);
          }
          drops_per_frame += drop_prob_increment;
          break;
        }
        period--;
}

static void pdp_ripple_process_yv12(t_pdp_ripple *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    unsigned int totalnbpixels;
    unsigned int u_offset;
    unsigned int v_offset;
    unsigned int totnbpixels;

    int px, py;
    int dx, dy;
    int h, v;
    int width, height;
    int *p, *q, *r;
    signed char *vp;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_ripple_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_ripple_allocate(x);
        post( "pdp_ripple : reallocated buffers" );
    }

    if ( x->x_bdata && x->x_snapshot )
    {
       x->x_snapshot = 0;
       memcpy( x->x_bdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );
    }

    totalnbpixels = x->x_vsize;
    u_offset = x->x_vsize;
    v_offset = x->x_vsize + (x->x_vsize>>2);
    totnbpixels = x->x_vsize + (x->x_vsize>>1);

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

    if ( x->x_mode ) 
    {
       pdp_ripple_motion_detect(x, data);
    } 
    else 
    {
       pdp_ripple_raindrop(x);
    }

    /* simulate surface wave */
    width = x->x_mapw;
    height = x->x_maph;

    /* This function is called only 30 times per second. To increase a speed
     * of wave, iterates this loop several times. */
    for(i=loopnum; i>0; i--) 
    {
        /* wave simulation */
        p = x->x_map1 + width + 1;
        q = x->x_map2 + width + 1;
        r = x->x_map3 + width + 1;
        for(py=height-2; py>0; py--) 
        {
          for(px=width-2; px>0; px--) 
          {
             h = *(p-width-1) + *(p-width+1) + *(p+width-1) + *(p+width+1)
                + *(p-width) + *(p-1) + *(p+1) + *(p+width) - (*p)*9;
             h = h >> 3;
             v = *p - *q;
             v += h - (v >> decay);
             *r = v + *p;
             p++;
             q++;
             r++;
           }
           p += 2;
           q += 2;
           r += 2;
         }

         /* low pass filter */
         p = x->x_map3 + width + 1;
         q = x->x_map2 + width + 1;
         for(py=height-2; py>0; py--) 
         {
            for(px=width-2; px>0; px--) 
            {
               h = *(p-width) + *(p-1) + *(p+1) + *(p+width) + (*p)*60;
               *q = h >> 6;
               p++;
               q++;
            }
            p+=2;
            q+=2;
          }

          p = x->x_map1;
          x->x_map1 = x->x_map2;
          x->x_map2 = p;
    }

    vp = x->x_vtable;
    p = x->x_map1;
    for(py=height-1; py>0; py--) 
    {
      for(px=width-1; px>0; px--) 
      {
         /* difference of the height between two voxel. They are twiced to
          * emphasise the wave. */
         vp[0] = sqrtable[((p[0] - p[1])>>(point-1))&0xff];
         vp[1] = sqrtable[((p[0] - p[width])>>(point-1))&0xff];
         p++;
         vp+=2;
       }
       p++;
       vp+=2;
    }

    height = x->x_vheight;
    width = x->x_vwidth;
    vp = x->x_vtable;

    /* draw refracted image. The vector table is stretched. */
    for(py=0; py<height; py+=2) 
    {
         for(px=0; px<width; px+=2) 
         {
            h = (int)vp[0];
            v = (int)vp[1];
            dx = px + h;
            dy = py + v;
            if(dx<0) dx=0;
            if(dy<0) dy=0;
            if(dx>=width) dx=width-1;
            if(dy>=height) dy=height-1;
            newdata[0] = data[dy*width+dx];

            i = dx;

            dx = px + 1 + (h+(int)vp[2])/2;
            if(dx<0) dx=0;
            if(dx>=width) dx=width-1;
            newdata[1] = data[dy*width+dx];

            dy = py + 1 + (v+(int)vp[x->x_mapw*2+1])/2;
            if(dy<0) dy=0;
            if(dy>=height) dy=height-1;
            newdata[width] = data[dy*width+i];

            newdata[width+1] = data[dy*width+dx];
            newdata+=2;
            vp+=2;
          }
          newdata += width;
          vp += 2;
    }

    return;
}

static void pdp_ripple_sendpacket(t_pdp_ripple *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_ripple_process(t_pdp_ripple *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_ripple_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_ripple_process_yv12, pdp_ripple_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_ripple_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_ripple_process */
	    break;
	    
	}
    }
}

static void pdp_ripple_input_0(t_pdp_ripple *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_ripple_process(x);
    }
}

static void pdp_ripple_free(t_pdp_ripple *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_ripple_free_ressources(x);
}

t_class *pdp_ripple_class;

void *pdp_ripple_new(void)
{
    int i;

    t_pdp_ripple *x = (t_pdp_ripple *)pd_new(pdp_ripple_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("mode"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_bang, gensym("background"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("threshold"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("increment"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_mode = 0;
    x->x_vsize = -1;
    x->x_snapshot = 1;
    x->x_threshold = MAGIC_THRESHOLD;

    if ( sqrt_init )
    {
       sqrt_init = 0;
       for(i=0; i<128; i++) {
         sqrtable[i] = i*i;
       }
       for(i=1; i<=128; i++) {
         sqrtable[256-i] = -i*i;
       }
    }

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ripple_setup(void)
{
//    post( pdp_ripple_version );
    pdp_ripple_class = class_new(gensym("pdp_ripple"), (t_newmethod)pdp_ripple_new,
    	(t_method)pdp_ripple_free, sizeof(t_pdp_ripple), 0, A_NULL);

    class_addmethod(pdp_ripple_class, (t_method)pdp_ripple_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ripple_class, (t_method)pdp_ripple_mode, gensym("mode"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ripple_class, (t_method)pdp_ripple_background, gensym("background"), A_NULL);
    class_addmethod(pdp_ripple_class, (t_method)pdp_ripple_threshold, gensym("threshold"), A_FLOAT, A_NULL);
    class_addmethod(pdp_ripple_class, (t_method)pdp_ripple_increment, gensym("increment"), A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
