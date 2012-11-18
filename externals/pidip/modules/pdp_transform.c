/*
 *   PiDiP module
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

/*  This object is a port of transform effect from EffecTV
 *  Originally written by clifford smith <nullset@dookie.net>
 *  Pd-fication by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include <math.h>

#define MAX_TABLES 6
static unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

static char   *pdp_transform_version = "pdp_transform: version 0.1, port of transform from EffecTV by clifford smith, adapted by ydegoyon@free.fr ";

typedef struct pdp_transform_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    t_outlet *x_outlet0;
    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int **x_table_list; // mapping tables
    int **x_table_list_u; // mapping tables
    int x_table; // current table
    int x_t;


} t_pdp_transform;

static void pdp_transform_table(t_pdp_transform *x, t_floatarg ftable )
{
    if ( ( ftable >= 0 ) && ( ftable < MAX_TABLES ) )
    {
       x->x_table = ftable;
    }
}

static int pdp_transform_map_from_table(t_pdp_transform *x, int px, int py, int t) 
{
  int xd,yd;

    yd = py + (inline_fastrand() >> 30)-2;
    xd = px + (inline_fastrand() >> 30)-2;
    if (xd > x->x_vwidth) {
      xd-=1;
    }
    return (xd+yd*x->x_vwidth);
}

static int pdp_transform_map_from_table_u(t_pdp_transform *x, int px, int py, int t) 
{
  int xd,yd;

    yd = py + (inline_fastrand() >> 30)-2;
    xd = px + (inline_fastrand() >> 30)-2;
    if (xd > x->x_vwidth) {
      xd-=1;
    }
    return ((xd>>1)+(yd>>1)*(x->x_vwidth>>1));
}

static void pdp_transform_square_table_init(t_pdp_transform *x)
{
  const int size = 16;
  int px, py, tx, ty;

    for(py=0; py<x->x_vheight; py++) 
    {
       ty = py % size - size / 2;
       if((py/size)%2)
       {
         ty = py - ty;
       }
       else
       {
         ty = py + ty;
       }
       if(ty<0) ty = 0;
       if(ty>=x->x_vheight) 
       {
         ty = x->x_vheight - 1;
       }
       for(px=0; px<x->x_vwidth; px++) 
       {
          tx = px % size - size / 2;
          if((px/size)%2)
          {
            tx = px - tx;
          }
          else
          {
            tx = px + tx;
          }
          if(tx<0) tx = 0;
          if(tx>=x->x_vwidth) tx = x->x_vwidth - 1;
          x->x_table_list[5][px+py*x->x_vwidth] = ty*x->x_vwidth+tx;
          x->x_table_list_u[5][px+py*x->x_vwidth] = (ty>>1)*(x->x_vwidth>>1)+(tx>>1);
       }
     }
}

static void pdp_transform_init_tables(t_pdp_transform *x)
{
  int px, py;

    for (py=0;py<x->x_vheight;py++) 
    {
      for (px=0;px<x->x_vwidth;px++) 
      {
        x->x_table_list[0][px+py*x->x_vwidth] = px+py*x->x_vwidth;
        x->x_table_list[1][px+py*x->x_vwidth] = (x->x_vwidth-1-px)+py*x->x_vwidth;
        x->x_table_list[2][px+py*x->x_vwidth] = px+(x->x_vheight-1-py)*x->x_vwidth;
        x->x_table_list[3][px+py*x->x_vwidth] = (x->x_vwidth-1-px)+(x->x_vheight-1-py)*x->x_vwidth;
        x->x_table_list_u[0][px+py*x->x_vwidth]=  (px>>1)+((py*x->x_vwidth)>>2);
        x->x_table_list_u[1][px+py*x->x_vwidth] = (x->x_vwidth>>1)-1-(px>>1)+(py>>1)*(x->x_vwidth>>1);
        x->x_table_list_u[2][px+py*x->x_vwidth] = (px>>1)+((x->x_vheight>>1)-1-(py>>1))*(x->x_vwidth>>1);
        x->x_table_list_u[3][px+py*x->x_vwidth] = ((x->x_vwidth-1-px)>>1)+((x->x_vheight-1-py)>>1)*(x->x_vwidth>>1);
        x->x_table_list[4][px+py*x->x_vwidth] = -2; /* Function */
        x->x_table_list_u[4][px+py*x->x_vwidth] = -2; /* Function */
      }
    }
    pdp_transform_square_table_init(x);
}

static void pdp_transform_free_ressources(t_pdp_transform *x)
{
  int i;

    // free tables
    for(i=0;i<MAX_TABLES;i++)
    {
       if ( x->x_table_list[i] ) freebytes( x->x_table_list[i], x->x_vsize*sizeof(int) );
       if ( x->x_table_list_u[i] ) freebytes( x->x_table_list_u[i], x->x_vsize*sizeof(int) );
    }
}

static void pdp_transform_allocate(t_pdp_transform *x)
{
  int i;

    // allocate tables
    for(i=0;i<MAX_TABLES;i++)
    {
       x->x_table_list[i] = (int *) getbytes( x->x_vsize*sizeof(int) );
       x->x_table_list_u[i] = (int *) getbytes( x->x_vsize*sizeof(int) );
    }
}

static void pdp_transform_process_yv12(t_pdp_transform *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i, iu;
    int     px, py;
    int     d, o, du=0, ou;
    short int *pY, *pU, *pV, *pnY, *pnU, *pnV;

    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth) ||
         ((int)header->info.image.height != x->x_vheight) )
    {
        pdp_transform_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_transform_allocate(x);
        post( "pdp_transform : reallocated buffers" );
        pdp_transform_init_tables(x);
        post( "pdp_transform : initialized tables" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    x->x_t++;
 
    pnY = newdata;
    pnV = newdata+x->x_vsize;
    pnU = newdata+x->x_vsize+(x->x_vsize>>2);
    iu = 0;
    for(py=0; py<x->x_vheight; py++)
    {
      for(px=0; px<x->x_vwidth; px++)
      {
         d = x->x_table_list[x->x_table][py*x->x_vwidth+px];
         if ( (px%2==0) && (py%2==0) ) 
         {
           du = x->x_table_list_u[x->x_table][py*x->x_vwidth+px];
           iu++;
         }
         if ( d==-2 )
         {
            d = pdp_transform_map_from_table( x, px, py, x->x_t );
            du = pdp_transform_map_from_table_u( x, px, py, x->x_t );
         }
         if ( d < 0) {
              o = 0;
              ou = 0;
         } else {
              o = d;
              ou = du;
         }
         *pnY++ = *(data+o);
         if ( (px%2==0) && (py%2==0) ) 
         {
           *pnV++ = *(data+x->x_vsize+ou);
           *pnU++ = *(data+x->x_vsize+(x->x_vsize>>2)+ou);
         }
      }
    }

    return;
}

static void pdp_transform_sendpacket(t_pdp_transform *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_transform_process(t_pdp_transform *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_transform_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_transform_process_yv12, pdp_transform_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_transform_process */
	    break;
	    
	}
    }

}

static void pdp_transform_input_0(t_pdp_transform *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_transform_process(x);

    }

}

static void pdp_transform_free(t_pdp_transform *x)
{
  int i;

    pdp_transform_free_ressources(x);
    if ( x->x_table_list ) freebytes(x->x_table_list, MAX_TABLES * sizeof(int *));
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_transform_class;

void *pdp_transform_new(void)
{
    int i;

    t_pdp_transform *x = (t_pdp_transform *)pd_new(pdp_transform_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("table"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_table_list = (int **) getbytes(MAX_TABLES * sizeof(int *));
    x->x_table_list_u = (int **) getbytes(MAX_TABLES * sizeof(int *));
    x->x_t = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_transform_setup(void)
{
//    post( pdp_transform_version );
    pdp_transform_class = class_new(gensym("pdp_transform"), (t_newmethod)pdp_transform_new,
    	(t_method)pdp_transform_free, sizeof(t_pdp_transform), 0, A_NULL);

    class_addmethod(pdp_transform_class, (t_method)pdp_transform_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transform_class, (t_method)pdp_transform_table, gensym("table"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
