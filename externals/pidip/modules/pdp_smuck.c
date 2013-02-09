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

/*  This object is an adaptation of smuck effect from veejay
 *  But it it inspired by effectv's transform ( mode 5 )
 *  Originally written by Niels Elburg 
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define MAX_N 100

static int fastrand_val=0;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

static char   *pdp_smuck_version = "pdp_smuck: version 0.1, port of smuck from veejay( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_smuck_struct
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

    int x_n; // transform number

} t_pdp_smuck;

static void pdp_smuck_free_ressources(t_pdp_smuck *x)
{ 
     // nothing
}

static void pdp_smuck_allocate(t_pdp_smuck *x)
{
     // nothing
}

static void pdp_smuck_n(t_pdp_smuck *x, t_floatarg fn )
{
  if ( ( fn >= 0 ) && ( fn < MAX_N ) )
  { 
     x->x_n = fn;
  }
}

static void pdp_smuck_process_yv12(t_pdp_smuck *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     px, py, pxx, pyy;
    short     int *pnY, *pnU, *pnV;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_smuck_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_smuck_allocate(x);
        post( "pdp_smuck : reallocated buffers" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    pnY = newdata;
    pnV = newdata+x->x_vsize;
    pnU = newdata+x->x_vsize+(x->x_vsize>>2);

    for (py = 0; py < x->x_vheight; py++) {
      for (px = 0; px < x->x_vwidth; px++) {
        pyy = py + (inline_fastrand() >> x->x_n) - 2;
        pxx = px + (inline_fastrand() >> x->x_n) - 2;
        if (pxx > x->x_vwidth)
	    pxx = x->x_vwidth;
	if ( pxx < 0 ) pxx = 0;
        if (pyy > x->x_vheight)
	    pyy = x->x_vheight;
	if ( pyy < 0 ) pyy = 0;
	*pnY++ = *( data + pyy*x->x_vwidth + pxx );
        if ( (px%2==0) && (py%2==0) )
        {
	  *pnU++ = *( data + x->x_vsize + ( (pyy>>1)*(x->x_vwidth>>1) + (pxx>>2) )  );
	  *pnV++ = *( data + x->x_vsize + (x->x_vsize>>2) + ( (pyy>>1)*(x->x_vwidth>>1) + (pxx>>2) ) );
        }
      }
    }

    return;
}

static void pdp_smuck_sendpacket(t_pdp_smuck *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_smuck_process(t_pdp_smuck *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_smuck_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_smuck_process_yv12, pdp_smuck_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_smuck_process */
	    break;
	    
	}
    }
}

static void pdp_smuck_input_0(t_pdp_smuck *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_smuck_process(x);
    }
}

static void pdp_smuck_free(t_pdp_smuck *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_smuck_free_ressources(x);
}

t_class *pdp_smuck_class;

void *pdp_smuck_new(void)
{
    int i;

    t_pdp_smuck *x = (t_pdp_smuck *)pd_new(pdp_smuck_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("n"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_n = 30;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_smuck_setup(void)
{
//    post( pdp_smuck_version );
    pdp_smuck_class = class_new(gensym("pdp_smuck"), (t_newmethod)pdp_smuck_new,
    	(t_method)pdp_smuck_free, sizeof(t_pdp_smuck), 0, A_NULL);

    class_addmethod(pdp_smuck_class, (t_method)pdp_smuck_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_smuck_class, (t_method)pdp_smuck_n, gensym("n"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
