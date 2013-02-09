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

/*  This object is a video cropper
 *  Written by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_cropper_version = "pdp_cropper: a video cropper, version 0.1, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_cropper_struct
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
    int x_csizex;
    int x_csizey;
    int x_csizev;
    unsigned int x_encoding;

    int       x_cropx1;
    int       x_cropx2;
    int       x_cropy1;
    int       x_cropy2;

} t_pdp_cropper;

static void pdp_cropper_cropx1(t_pdp_cropper *x, t_floatarg fcropx1 )
{
    if ( ( fcropx1>=0 ) && ( fcropx1<x->x_vwidth ) )
    {
       x->x_cropx1 = (int)fcropx1;
    }
}

static void pdp_cropper_cropx2(t_pdp_cropper *x, t_floatarg fcropx2 )
{
    if ( ( fcropx2>=0 ) && ( fcropx2<x->x_vwidth ) )
    {
       x->x_cropx2 = (int)fcropx2;
    }
}

static void pdp_cropper_cropy1(t_pdp_cropper *x, t_floatarg fcropy1 )
{
    if ( ( fcropy1>=0 ) && ( fcropy1<x->x_vheight ) )
    {
       x->x_cropy1 = (int)fcropy1;
    }
}

static void pdp_cropper_cropy2(t_pdp_cropper *x, t_floatarg fcropy2 )
{
    if ( ( fcropy2>=0 ) && ( fcropy2<x->x_vheight ) )
    {
       x->x_cropy2 = (int)fcropy2;
    }
}

static void pdp_cropper_process_yv12(t_pdp_cropper *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = NULL;
    short int *newdata = NULL;
    int       i;

    int px, py;
    short int *pY, *pU, *pV;
    short int *pnY, *pnU, *pnV;
    int minx, maxx;
    int miny, maxy;

    /* allocate all ressources */
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) ) 
    {
       x->x_vwidth = header->info.image.width;
       x->x_vheight = header->info.image.height;
       x->x_vsize = x->x_vwidth*x->x_vheight;
       if ( ( x->x_cropx1 <0 ) || ( x->x_cropx1 >= x->x_vwidth ) ) x->x_cropx1 = 0;
       if ( ( x->x_cropx2 <0 ) || ( x->x_cropx2 >= x->x_vwidth ) ) x->x_cropx2 = x->x_vwidth-1;
       if ( ( x->x_cropy1 <0 ) || ( x->x_cropy1 >= x->x_vheight ) ) x->x_cropy1 = 0;
       if ( ( x->x_cropy2 <0 ) || ( x->x_cropy2 >= x->x_vheight ) ) x->x_cropy2 = x->x_vheight-1;
    }

    x->x_csizex = abs ( x->x_cropx2 - x->x_cropx1 );
    if ( x->x_csizex%8 != 0 ) x->x_csizex = x->x_csizex + (8-(x->x_csizex%8)); // align on 8
    x->x_csizey = abs ( x->x_cropy2 - x->x_cropy1 );
    if ( x->x_csizey%8 != 0 ) x->x_csizey = x->x_csizey + (8-(x->x_csizey%8)); // align on 8
    if ( x->x_csizex == 0 ) x->x_csizex = 8;
    if ( x->x_csizey == 0 ) x->x_csizey = 8;
    // post( "pdp_cropper : new image %dx%d", x->x_csizex, x->x_csizey );

    x->x_csizev = x->x_csizex*x->x_csizey;
    x->x_packet1 = pdp_packet_new_image_YCrCb( x->x_csizex, x->x_csizey );
    newheader = pdp_packet_header(x->x_packet1);
    newdata = (short int *)pdp_packet_data(x->x_packet1);

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_csizex;
    newheader->info.image.height = x->x_csizey;

    pY = data;
    pU = (data+x->x_vsize);
    pV = (data+x->x_vsize+(x->x_vsize>>2));
    pnY = newdata;
    pnU = (newdata+x->x_csizev);
    pnV = (newdata+x->x_csizev+(x->x_csizev>>2));

    if ( x->x_cropx1<x->x_cropx2 ) 
    {
       minx = x->x_cropx1;
       maxx = x->x_cropx2;
    }
    else
    {
       minx = x->x_cropx2;
       maxx = x->x_cropx1;
    }

    if ( x->x_cropy1<x->x_cropy2 ) 
    {
       miny = x->x_cropy1;
       maxy = x->x_cropy2;
    }
    else
    {
       miny = x->x_cropy2;
       maxy = x->x_cropy1;
    }

    for(py=miny; py<maxy; py++) 
    {
      for(px=minx; px<maxx; px++) 
      {
         *(pnY+(py-miny)*x->x_csizex+(px-minx)) = *(pY+py*x->x_vwidth+px);
         if ( (py%2==0) && (px%2==0) )
         {
           *(pnU+((py-miny)>>1)*(x->x_csizex>>1)+((px-minx)>>1)) = *(pU+(py>>1)*(x->x_vwidth>>1)+(px>>1));
           *(pnV+((py-miny)>>1)*(x->x_csizex>>1)+((px-minx)>>1)) = *(pV+(py>>1)*(x->x_vwidth>>1)+(px>>1));
         }
      }
    }

    return;
}

static void pdp_cropper_sendpacket(t_pdp_cropper *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_cropper_process(t_pdp_cropper *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_cropper_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_cropper_process_yv12, pdp_cropper_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_cropper_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_cropper_process */
	    break;
	    
	}
    }
}

static void pdp_cropper_input_0(t_pdp_cropper *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = 
          pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
       /* add the process method and callback to the process queue */
       pdp_cropper_process(x);
    }
}

static void pdp_cropper_free(t_pdp_cropper *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_cropper_class;

void *pdp_cropper_new(void)
{
    int i;

    t_pdp_cropper *x = (t_pdp_cropper *)pd_new(pdp_cropper_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("x1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("x2"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("y1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("y2"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_cropx1=-1;
    x->x_cropx2=-1;
    x->x_cropy1=-1;
    x->x_cropy2=-1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_cropper_setup(void)
{
//    post( pdp_cropper_version );
    pdp_cropper_class = class_new(gensym("pdp_cropper"), (t_newmethod)pdp_cropper_new,
    	(t_method)pdp_cropper_free, sizeof(t_pdp_cropper), 0, A_NULL);

    class_addmethod(pdp_cropper_class, (t_method)pdp_cropper_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cropper_class, (t_method)pdp_cropper_cropx1, gensym("x1"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_cropper_class, (t_method)pdp_cropper_cropx2, gensym("x2"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_cropper_class, (t_method)pdp_cropper_cropy1, gensym("y1"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_cropper_class, (t_method)pdp_cropper_cropy2, gensym("y2"),  A_FLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
