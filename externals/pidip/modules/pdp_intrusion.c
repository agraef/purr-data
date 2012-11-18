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

/*  This object is an adaptation of hologram effect from effectv
 *  but, in these paranoid times, i found it funnier to rename it as intrusion
 *  because it can detect moving objects ( targets ?? )                                     
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define NB_IMAGES 4
#define MAGIC_THRESHOLD 10
static unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

static char   *pdp_intrusion_version = "pdp_intrusion: version 0.1, inspired by hologram from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_intrusion_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    unsigned int x_noisepattern[256]; // noise pattern
    int x_vwidth;
    int x_vheight;
    int x_vsize;
    unsigned int x_encoding;
    short int *x_images[NB_IMAGES];
    int x_rtimage;
    short int *x_diff;
    short int *x_bdata;
    int       x_threshold;
    int       x_phase;
    int       x_loopcount;
    int       x_threshfreq;

} t_pdp_intrusion;

/* check if there is a real difference with background image */
short int *pdp_intrusion_diff(t_pdp_intrusion *x, short int *src)
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
         *r = (Yb - Y);
         p++; pb++;
         r++;
   }

   return x->x_diff;
}

static void pdp_intrusion_threshold(t_pdp_intrusion *x, t_floatarg fthreshold )
{
    if ( fthreshold>0 && fthreshold<255 )
    {
       x->x_threshold = ((int)fthreshold ) << 8;
    }
}

static void pdp_intrusion_background(t_pdp_intrusion *x )
{
  int i, j;

    if ( ( x->x_images[0] == NULL ) ||
       ( x->x_images[1] == NULL ) ||
       ( x->x_images[2] == NULL ) ||
       ( x->x_images[3] == NULL ) ) 
    {
       post( "pdp_intrusion_background : no images available !! " );
       return;
    }
    post( "pdp_intrusion : setting background" );

    memcpy( x->x_bdata, x->x_images[0], (( x->x_vsize + (x->x_vsize>>1))<<1)); 
    
    for( i=1; i<NB_IMAGES; i++ )
    {
      for ( j=0; j<(x->x_vsize+(x->x_vsize>>1)); j++ )
      {
       x->x_bdata[j] = (x->x_bdata[j]&x->x_images[i][j])+((x->x_bdata[j]^x->x_images[i][j])>>1);
      }
    }
}

static void pdp_intrusion_allocate(t_pdp_intrusion *x, int newsize)
{
 int i;

  for ( i=0; i<NB_IMAGES; i++ )
  {
    if ( x->x_images[i] != NULL )
    {
       freebytes( x->x_images[i], (x->x_vsize + (x->x_vsize>>1))<<1 );
    }
  }
  if ( x->x_diff != NULL )
  {
     freebytes( x->x_diff, (x->x_vsize + (x->x_vsize>>1))<<1 );
  }
  if ( x->x_bdata ) freebytes( x->x_bdata, (( x->x_vsize + (x->x_vsize>>1))<<1));

  x->x_vsize = newsize;
  for ( i=0; i<NB_IMAGES; i++ )
  {
     x->x_images[i] = (short int*) getbytes((x->x_vsize + (x->x_vsize>>1))<<1);
  }
  x->x_diff = (short int*) getbytes((x->x_vsize + (x->x_vsize>>1))<<1);
  x->x_bdata = (short int *)getbytes((( x->x_vsize + (x->x_vsize>>1))<<1));
}

static void pdp_intrusion_process_yv12(t_pdp_intrusion *x)
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
    short int *diff;
    short int *sy, *su, *sv, t;
    short int *sby, *sbu, *sbv;
    short int *sny, *snu, *snv;
    int Y=0, U=0, V=0;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize ) 
    {
        pdp_intrusion_allocate(x, header->info.image.width*header->info.image.height );
        post( "pdp_intrusion : reallocating buffers" );
    }

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;
    x->x_encoding = header->info.image.encoding;

    totalnbpixels = x->x_vsize;
    u_offset = x->x_vsize;
    v_offset = x->x_vsize + (x->x_vsize>>2);
    totnbpixels = x->x_vsize + (x->x_vsize>>1);

    newheader->info.image.encoding = x->x_encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    /* copy images if necessary */
    memcpy( x->x_images[x->x_rtimage], data, (( x->x_vsize + (x->x_vsize>>1))<<1)); 
    x->x_rtimage=(x->x_rtimage+1)%4;

    if ( !x->x_bdata ) return;

    /* check pixels which has changed */
    diff = pdp_intrusion_diff(x, data);

    sy = data;
    su = (data+x->x_vsize);
    sv = (data+x->x_vsize+(x->x_vsize>>2));
    sby = x->x_bdata;
    sbu = (x->x_bdata+x->x_vsize);
    sbv = (x->x_bdata+x->x_vsize+(x->x_vsize>>2));
    sny = newdata;
    snu = (newdata+x->x_vsize);
    snv = (newdata+x->x_vsize+(x->x_vsize>>2));

    for(py=1; py<x->x_vheight; py++) 
    {
     if(((py+x->x_phase) & 0x7f)<0x58) 
     {
        for(px=0; px<x->x_vwidth; px++) 
        {
          if ( sv >= data + x->x_vsize + (x->x_vsize>>1 ) ) break;
          if(*diff > x->x_threshold)
          {
              t = x->x_noisepattern[inline_fastrand()>>24];
              Y = (*sy) + t;
              U = (*su) + t;
              V = (*sv) + t;
              Y = (Y>>1)-100;
              U = (U>>1)-100;
              V = V>>2;
              Y += ((*sby)>>1)+((rand()%255)<<8);
              U += ((*sbu)>>1)+((rand()%255)<<8);
              V += ((*sbv)>>1)+((rand()%255)<<8);
              // clipping
              if((Y>>8)<20) Y=20<<8;
              if((U>>8)<-108) U=-108<<8;
              if((V>>8)<-108) V=-108<<8;
              if((Y>>8)>255) Y = 255;
              if((U>>8)>128) U = 128;
              if((V>>8)>128) V = 128;
              *sny = Y;
              *snu = U;
              *snv = V;
           } 
           else 
           {
              *sny = *sy;
              *snu = *su;
              *snv = *sv;
           }
           diff++; sy++; sby++; sny++;
           if ( ((px+1)%2==0) && ((py+1)%2==0))
           {
              su++; sbu++; snu++;
              sv++; sbv++; snv++;
           }
        }
      } 
      else 
      {
        for(px=0; px<x->x_vwidth; px++) 
        {
           if ( sv >= data + x->x_vsize + (x->x_vsize>>1 ) ) break;
           if(*diff > x->x_threshold){
              t = x->x_noisepattern[inline_fastrand()>>24];
              Y = (*sy) + t;
              U = (*su) + t;
              V = (*sv) + t;
              Y = (Y>>1)-100;
              U = (U>>1)-100;
              V = V>>2;
              Y += ((*sby)>>1)+((rand()%255)<<8);
              U += ((*sbu)>>1)+((rand()%255)<<8);
              V += ((*sbv)>>1)+((rand()%255)<<8);
              if((Y>>8)<0) Y=0;
              if((U>>8)<-128) U=-128<<8;
              if((V>>8)<-128) V=-128<<8;
              if((Y>>8)>255) Y = 255;
              if((U>>8)>128) U = 128;
              if((V>>8)>128) V = 128;
              *sny = Y;
              *snu = U;
              *snv = V;
           } else {
              *sny = *sy;
              *snu = *su;
              *snv = *sv;
           }
           diff++; sy++; sby++; sny++;
           if ( ((px+1)%2==0) && ((py+1)%2==0) )
           {
              su++; sbu++; snu++;
              sv++; sbv++; snv++;
           }
         }
       }
    }

    x->x_phase-=37;
    
    return;
}

static void pdp_intrusion_sendpacket(t_pdp_intrusion *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_intrusion_process(t_pdp_intrusion *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_intrusion_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_intrusion_process_yv12, pdp_intrusion_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_intrusion_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_intrusion_process */
	    break;
	    
	}
    }
}

static void pdp_intrusion_input_0(t_pdp_intrusion *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_intrusion_process(x);

    }
}

static void pdp_intrusion_free(t_pdp_intrusion *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

    for (i=0; i<NB_IMAGES; i++ )
    {
       if ( x->x_images[i] ) freebytes( x->x_images[i], (x->x_vsize + (x->x_vsize>>1))<<1 );
    }

}

t_class *pdp_intrusion_class;

void *pdp_intrusion_new(void)
{
    int i;

    t_pdp_intrusion *x = (t_pdp_intrusion *)pd_new(pdp_intrusion_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_bang, gensym("background"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("threshold"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_threshold = MAGIC_THRESHOLD<<8;
    x->x_phase = 0;
    x->x_bdata = NULL;
    x->x_diff = NULL;
    x->x_vsize = -1;
    x->x_loopcount = 0;
    x->x_threshfreq = 10;

    // initialize noise pattern
    for(i=0; i<256; i++) 
    {
       x->x_noisepattern[i] = (i * i * i / 40000)* i / 256;
    }

    // initialize images
    for(i=0; i<NB_IMAGES; i++) 
    {
       x->x_images[i] = NULL;
    }
    x->x_rtimage=0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_intrusion_setup(void)
{
//    post( pdp_intrusion_version );
    pdp_intrusion_class = class_new(gensym("pdp_intrusion"), (t_newmethod)pdp_intrusion_new,
    	(t_method)pdp_intrusion_free, sizeof(t_pdp_intrusion), 0, A_NULL);

    class_addmethod(pdp_intrusion_class, (t_method)pdp_intrusion_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_intrusion_class, (t_method)pdp_intrusion_background, gensym("background"),  A_NULL);
    class_addmethod(pdp_intrusion_class, (t_method)pdp_intrusion_threshold, gensym("threshold"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
