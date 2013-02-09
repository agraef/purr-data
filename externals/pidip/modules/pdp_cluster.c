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
#include <math.h>

static char   *pdp_cluster_version = "pdp_cluster: version 0.1, color clusterization (YUV colorspace) for color blob extraction, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_cluster_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1; // for main colors

    int x_packet0;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_groupby;
    int x_cvalues; // possible values
    int x_nbcolors;

    int x_ignoreblack;
    int x_luminosity;
    
    short int *x_colorhist;
    t_atom  plist[4]; // list of main clustered colors ( index, y, u, v )

} t_pdp_cluster;

static void pdp_cluster_process_yv12(t_pdp_cluster *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data = (short int*)pdp_packet_data(x->x_packet0);
    int       i;

    int px, py, pc, pcc;
    int idx, max;
    int y, u, v;
    short int *pfY, *pfU, *pfV;

    /* allocate all ressources */
    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    pfY = data;
    pfV = data+x->x_vsize;
    pfU = data+x->x_vsize+(x->x_vsize>>2);
    memset( x->x_colorhist, 0x0, pow(x->x_cvalues,3)*sizeof(short int) );

    for(py=0; py<x->x_vheight; py++) 
    {
      for(px=0; px<x->x_vwidth; px++) 
      {
         // only u and v channels
         y = (*(pfY+py*x->x_vwidth+px))>>7;
         u = (*(pfU+(py>>1)*(x->x_vwidth>>1)+(px>>1))>>8)+128;
         v = (*(pfV+(py>>1)*(x->x_vwidth>>1)+(px>>1))>>8)+128;
         y = y-(y%x->x_groupby)+(x->x_groupby/2);
         u = u-(u%x->x_groupby)+(x->x_groupby/2);
         v = v-(v%x->x_groupby)+(x->x_groupby/2);
         if ( y>255 || y<0 )
         {
            post( "pdp_cluster : weird y value : %d", y );
            y=0;
         }
         if ( u>255 || u<0 )
         {
            post( "pdp_cluster : weird u value : %d", u );
            u=0;
         }
         if ( v>255 || v<0 )
         {
            post( "pdp_cluster : weird v value : %d", v );
            v=0;
         }
         *(pfY+py*x->x_vwidth+px) = y<<7;
         *(pfU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = ((u-128)<<8);
         *(pfV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = ((v-128)<<8);

         if ( x->x_luminosity )
         {
           idx=(y/x->x_groupby)*pow(x->x_cvalues,2)+(u/x->x_groupby)*x->x_cvalues+(v/x->x_groupby);
         }
         else
         {
           idx=(u/x->x_groupby)*x->x_cvalues+(v/x->x_groupby);
         }
         x->x_colorhist[idx]++;
      }
    }

    for(pc=0; pc<x->x_nbcolors; pc++) 
    {
       idx=-1;
       max=0;
       for(pcc=x->x_ignoreblack; pcc<pow(x->x_cvalues,3); pcc++) 
       {
          if ( x->x_colorhist[pcc] > max )
          {
             max = x->x_colorhist[pcc];
             idx = pcc;
          }
       }

       if ( max > 0 )
       {
         x->x_colorhist[idx]=0;

         if ( x->x_luminosity )
         {
           v = idx%x->x_cvalues;
           u = ((idx-v)/x->x_cvalues)%x->x_cvalues;
           y = (idx-v-u*x->x_cvalues)/pow(x->x_cvalues,2);
         }
         else
         {
           y = 255/x->x_groupby;
           v = idx%x->x_cvalues;
           u = (idx-v)/x->x_cvalues;
         }

         SETFLOAT(&x->plist[0], pc);
         SETFLOAT(&x->plist[1], y*x->x_groupby+x->x_groupby/2);
         SETFLOAT(&x->plist[2], u*x->x_groupby+x->x_groupby/2);
         SETFLOAT(&x->plist[3], v*x->x_groupby+x->x_groupby/2);
         outlet_list( x->x_outlet1, 0, 4, x->plist );
       }
    }

    return;
}

static void pdp_cluster_sendpacket(t_pdp_cluster *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet0);

}

static void pdp_cluster_process(t_pdp_cluster *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type))
   {
    
	/* pdp_cluster_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_cluster_process_yv12, pdp_cluster_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_cluster_process */
	    break;
	    
	}
    }
}

static void pdp_cluster_input_0(t_pdp_cluster *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_cluster_process(x);

    }
}

static void pdp_cluster_groupby(t_pdp_cluster *x, t_floatarg f)
{
   if ( (int)f<10 || (int)f>255 )
   {
      post( "pdp_cluster : wrong groupby factor : %d", (int)f );
      return;
   }

   free( x->x_colorhist );

   x->x_groupby=(int)f;
   x->x_cvalues = (255/x->x_groupby)+1;
   // post( "pdp_cluster : possible values : %d", x->x_cvalues );

   x->x_colorhist = (short int*)malloc(pow(x->x_cvalues,3)*sizeof(short int));
   memset( x->x_colorhist, 0x0, pow(x->x_cvalues,3)*sizeof(short int) );
}

static void pdp_cluster_maincolors(t_pdp_cluster *x, t_floatarg f)
{
   if ( (int)f<0 )
   {
      post( "pdp_cluster : wrong number of main colors : %d", (int)f );
      return;
   }

   x->x_nbcolors=(int)f;
}

static void pdp_cluster_ignore_black(t_pdp_cluster *x, t_floatarg f)
{
   if ( (int)f!=0 && (int)f!=1 )
   {
      return;
   }

   x->x_ignoreblack=(int)f;
}

static void pdp_cluster_luminosity(t_pdp_cluster *x, t_floatarg f)
{
   if ( (int)f!=0 && (int)f!=1 )
   {
      return;
   }

   x->x_luminosity=(int)f;
}

static void pdp_cluster_free(t_pdp_cluster *x)
{
  int i;

   pdp_queue_finish(x->x_queue_id);
   pdp_packet_mark_unused(x->x_packet0);
   free( x->x_colorhist );
}

t_class *pdp_cluster_class;

void *pdp_cluster_new(void)
{
    int i;

    t_pdp_cluster *x = (t_pdp_cluster *)pd_new(pdp_cluster_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_queue_id = -1;

    x->x_groupby = 10;
    x->x_cvalues = (255/x->x_groupby)+1;

    x->x_nbcolors = 5;
    x->x_ignoreblack = 1;
    x->x_luminosity = 0;

    x->x_colorhist = (short int*)malloc(pow(x->x_cvalues,3)*sizeof(short int));
    memset( x->x_colorhist, 0x0, pow(x->x_cvalues,3)*sizeof(short int) );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_cluster_setup(void)
{
//    post( pdp_cluster_version );
    pdp_cluster_class = class_new(gensym("pdp_cluster"), (t_newmethod)pdp_cluster_new,
    	(t_method)pdp_cluster_free, sizeof(t_pdp_cluster), 0, A_NULL);

    class_addmethod(pdp_cluster_class, (t_method)pdp_cluster_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cluster_class, (t_method)pdp_cluster_groupby, gensym("groupby"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cluster_class, (t_method)pdp_cluster_maincolors, gensym("maincolors"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cluster_class, (t_method)pdp_cluster_ignore_black, gensym("ignoreblack"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cluster_class, (t_method)pdp_cluster_luminosity, gensym("luminosity"),  A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
