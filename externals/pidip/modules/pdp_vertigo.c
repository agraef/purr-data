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

/*  This object is an adaptation of lens effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_vertigo_version = "pdp_vertigo: version 0.1, port of vertigo from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_vertigo_struct
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
    short int *x_buffer;
    short int *x_current_buffer;
    short int *x_alt_buffer;
    int x_dx;
    int x_dy;
    int x_sx;
    int x_sy;
    double x_phase;
    double x_phase_increment;
    double x_zoomrate;


} t_pdp_vertigo;

static void pdp_vertigo_increment(t_pdp_vertigo *x, t_floatarg fincrement )
{
    x->x_phase_increment = fincrement;
}

static void pdp_vertigo_zoomrate(t_pdp_vertigo *x, t_floatarg fzoomrate )
{
    x->x_zoomrate = (int)fzoomrate;
}

static void pdp_vertigo_allocate(t_pdp_vertigo *x, t_floatarg fnewsize )
{
  int nsize = (int) fnewsize;

    if ( x->x_buffer ) freebytes( x->x_buffer, 2*((x->x_vsize + (x->x_vsize>>1))<<1) );
    x->x_buffer = (short int *) getbytes( 2*(( nsize + (nsize>>1))<<1) );
    if ( x->x_buffer )
    {
       bzero( x->x_buffer, 2*((nsize + (nsize>>1))<<1) );
       x->x_current_buffer = x->x_buffer;
       x->x_alt_buffer = x->x_buffer + (nsize + (nsize>>1));
    }
    x->x_phase = 0;
}

static void pdp_vertigo_set_params(t_pdp_vertigo *x)
{
   double vx, vy;
   double t;
   double X, Y;
   double dizz;

     dizz = sin(x->x_phase) * 10 + sin(x->x_phase*1.9+5) * 5;

     X = x->x_vwidth / 2;
     Y = x->x_vheight / 2;
     t = (X*X + Y*Y) * x->x_zoomrate;
     if( x->x_vwidth > x->x_vheight ) 
     {
       if(dizz >= 0) 
       {
         if(dizz > X) dizz = X;
         vx = (X*(X-dizz) + Y*Y) / t;
       } 
       else 
       {
         if(dizz < -X) dizz = -X;
         vx = (X*(X+dizz) + Y*Y) / t;
       }
       vy = (dizz*Y) / t;
     } 
     else 
     {
       if(dizz >= 0) 
       {
          if(dizz > Y) dizz = Y;
          vx = (X*X + Y*(Y-dizz)) / t;
       } 
       else 
       {
         if(dizz < -Y) dizz = -Y;
         vx = (X*X + Y*(Y+dizz)) / t;
       }
       vy = (dizz*X) / t;
     }
     x->x_dx = vx * 65536;
     x->x_dy = vy * 65536;
     x->x_sx = (-vx * X + vy * Y + X + cos(x->x_phase*5) * 2) * 65536;
     x->x_sy = (-vx * Y - vy * X + Y + sin(x->x_phase*6) * 2) * 65536;

     x->x_phase += x->x_phase_increment;
     if(x->x_phase > 5700000) x->x_phase = 0;
}


static void pdp_vertigo_process_yv12(t_pdp_vertigo *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);

    unsigned int totalnbpixels;
    unsigned int u_offset;
    unsigned int v_offset;
    unsigned int totnbpixels;
    short int *poy, *pou, *pov, *pny, *pnu, *pnv, *pcy, *pcu, *pcv;
    int px, py;
    short int v;
    int ox, oy;
    int i, ninc;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_vertigo_allocate(x, header->info.image.width*header->info.image.height );
        post( "pdp_vertigo : reallocated buffers" );
    }
    
    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    totalnbpixels = x->x_vsize;
    u_offset = x->x_vsize;
    v_offset = x->x_vsize + (x->x_vsize>>2);
    totnbpixels = x->x_vsize + (x->x_vsize>>1);

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    pdp_vertigo_set_params(x);

    poy = data;
    pou = data + x->x_vsize;
    pov = data + x->x_vsize + (x->x_vsize>>2);
    pcy = x->x_current_buffer;
    pcu = x->x_current_buffer + x->x_vsize;
    pcv = x->x_current_buffer + x->x_vsize + (x->x_vsize>>2);
    pny = x->x_alt_buffer;
    pnu = x->x_alt_buffer + x->x_vsize;
    pnv = x->x_alt_buffer + x->x_vsize + (x->x_vsize>>2);
    ninc=0;
    for(py=x->x_vheight; py>0; py--) 
    {
       ox = x->x_sx;
       oy = x->x_sy;
       for(px=x->x_vwidth; px>0; px--) 
       {
          if ( pny >= ( x->x_alt_buffer + x->x_vsize ) )
          {
             post( "pdp_vertigo : abnormal pointer position : pny=%x, start=%x, size=%x ninc=%d px=%d py=%d",
                        pny, x->x_alt_buffer , x->x_vsize-1, ninc, px, py );
             break;
          }
          if ( pnu >= ( x->x_alt_buffer + x->x_vsize + (x->x_vsize>>2) ) ) 
          {
             post( "pdp_vertigo : abnormal pointer position : pnu=%x, start=%x, size=%x ninc=%d px=%d py=%d",
                       pnu, x->x_alt_buffer + x->x_vsize, (x->x_vsize>>2)-1, ninc, px, py );
             break;
          }
          if ( pnv >= ( x->x_alt_buffer + x->x_vsize + (x->x_vsize>>1) ) ) 
          {
             post( "pdp_vertigo : abnormal pointer position : pnv=%x, start=%x, size=%x ninc=%d px=%d py=%d",
                       pnv, x->x_alt_buffer + x->x_vsize + (x->x_vsize>>2), (x->x_vsize>>2)-1, ninc, px, py );
             break;
          }
          i = (oy>>16)*x->x_vwidth + (ox>>16);
          if (i<0) i = 0;
          if ( i >= (x->x_vsize + (x->x_vsize>>1)) ) i = (x->x_vsize + (x->x_vsize>>1))-1;
          v = pcy[i] & 0xffff;
          v = (v * 3) + ((*poy++) & 0xffff);
          *pny++ = (v>>2)<<7;
          if ( (((px+1)%2)==0) && (((py+1)%2)==0) )
          {
              ninc++;
              v = pcu[(i/4)] & 0xffff;
              v = (v * 3) + ((*pou++) & 0xffff);
              *pnu++ = (v>>2);
              v = pcv[(i/4)] & 0xffff;
              v = (v * 3) + ((*pov++) & 0xffff);
              *pnv++ = (v>>2);
          }
          ox += x->x_dx;
          oy += x->x_dy;
       }
       x->x_sx -= x->x_dy;
       x->x_sy += x->x_dx;
    }

    memcpy(newdata, x->x_alt_buffer, (x->x_vsize + (x->x_vsize>>1))<<1);

    poy = x->x_current_buffer;
    x->x_current_buffer = x->x_alt_buffer;
    x->x_alt_buffer = poy;

    return;
}

static void pdp_vertigo_sendpacket(t_pdp_vertigo *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_vertigo_process(t_pdp_vertigo *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_vertigo_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_vertigo_process_yv12, pdp_vertigo_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_vertigo_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_vertigo_process */
	    break;
	    
	}
    }
}

static void pdp_vertigo_input_0(t_pdp_vertigo *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_vertigo_process(x);
    }
}

static void pdp_vertigo_free(t_pdp_vertigo *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

    if ( x->x_buffer ) freebytes( x->x_buffer, 2*((x->x_vsize + (x->x_vsize>>1))<<1) );
}

t_class *pdp_vertigo_class;

void *pdp_vertigo_new(void)
{
    int i;

    t_pdp_vertigo *x = (t_pdp_vertigo *)pd_new(pdp_vertigo_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("increment"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("zoomrate"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_phase = 0;
    x->x_buffer = NULL;
    x->x_phase_increment = 0.02;
    x->x_zoomrate = 1.01;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_vertigo_setup(void)
{
//    post( pdp_vertigo_version );
    pdp_vertigo_class = class_new(gensym("pdp_vertigo"), (t_newmethod)pdp_vertigo_new,
    	(t_method)pdp_vertigo_free, sizeof(t_pdp_vertigo), 0, A_NULL);

    class_addmethod(pdp_vertigo_class, (t_method)pdp_vertigo_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_vertigo_class, (t_method)pdp_vertigo_increment, gensym("increment"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_vertigo_class, (t_method)pdp_vertigo_zoomrate, gensym("zoomrate"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
