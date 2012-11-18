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

/*  This object is an interface to aalib ( http://aa-project.sourceforge.net/aalib/ ) 
 *  converting an image to ASCII art
 *  Written by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <aalib.h>

#define MAX_OPTIONS 20
#define MAX_OPTION_LENGTH 20

static char   *pdp_aa_version = "pdp_aa: version 0.1, ASCII art output written by ydegoyon@free.fr ";

typedef struct pdp_aa_struct
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

      /* aalib structures */
    aa_context  *x_context;   // a lot of things and image data
    aa_renderparams  x_renderparams;  // rendering parameters
    char* x_driver;    // name of driver
    int x_render;    // rendering option

    char **x_aa_options;  // aa options passed as arguments
    int x_nb_options;    // number of aa options

} t_pdp_aa;

static void pdp_aa_allocate(t_pdp_aa *x)
{
   if ( !strcmp( x->x_driver, "X11" ) )
   {
      x->x_context = aa_init(&X11_d, &aa_defparams, NULL);
   }
   else if ( !strcmp( x->x_driver, "slang" ) )
   {
      x->x_context = aa_init(&slang_d, &aa_defparams, NULL);
   }
   else if ( !strcmp( x->x_driver, "stdout" ) )
   {
      x->x_context = aa_init(&stdout_d, &aa_defparams, NULL);
   }
   else if ( !strcmp( x->x_driver, "stderr" ) )
   {
      x->x_context = aa_init(&stderr_d, &aa_defparams, NULL);
   }
   else
   {
      post( "pdp_aa : unsupported driver : %s : using X11", x->x_driver ); 
      strcpy( x->x_driver, "X11" );
      pdp_aa_allocate( x );
      return;
   }
   
   if (x->x_context == NULL) 
   {
       post("pdp_aa : severe error : cannot initialize aalib !!!");
       return;
   }
   else
   {
       post("pdp_aa : initialized context");
   }
   aa_setfont( x->x_context, &aa_font8 );
}

static void pdp_aa_free_ressources(t_pdp_aa *x)
{
   // if ( x->x_context ) aa_close( x->x_context ); // this crashes unfortunately
}

static void pdp_aa_render(t_pdp_aa *x, t_floatarg frender)
{
   if ( ((int)frender == 0) || ((int)frender == 1) )
   {
      x->x_render = (int)frender;
   }
}

static void pdp_aa_driver(t_pdp_aa *x, t_symbol *sdriver)
{
   if ( ( !strcmp( sdriver->s_name, "X11" ) ) ||
        ( !strcmp( sdriver->s_name, "slang" ) ) ||
        ( !strcmp( sdriver->s_name, "stdout" ) ) ||
        ( !strcmp( sdriver->s_name, "stderr" ) ) )
   {
      strcpy( x->x_driver, sdriver->s_name );
   }
   else
   {
      post( "pdp_aa : unsupported driver : %s : using X11", sdriver->s_name ); 
      strcpy( x->x_driver, "X11" );
   }
   pdp_aa_free_ressources(x);
   pdp_aa_allocate(x);
}

static void pdp_aa_process_yv12(t_pdp_aa *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i, pixsum;
    int     px, py, ppx, ppy;
    int     hratio, wratio;

    if (  ( (int)header->info.image.width != x->x_vwidth ) ||
          ( (int)header->info.image.height != x->x_vheight ) )
    {
       pdp_aa_free_ressources( x );
       x->x_vwidth = header->info.image.width;
       x->x_vheight = header->info.image.height;
       x->x_vsize = x->x_vwidth*x->x_vheight;
       pdp_aa_allocate( x );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    if ( aa_imgwidth(x->x_context) < x->x_vwidth )
    {
       wratio = x->x_vwidth / aa_imgwidth(x->x_context);
    }
    else
    {
       wratio = 1;
    }
    if ( aa_imgheight(x->x_context) < x->x_vheight )
    {
       hratio = x->x_vheight / aa_imgheight(x->x_context);
    }
    else
    {
       hratio = 1;
    }

    for(py=1; py<x->x_vheight; py+=hratio)
    {
      for(px=0; px<x->x_vwidth; px+=wratio)
      {
         pixsum = 0;
         for ( ppy=0; ppy<hratio; ppy++ )
         {
           for ( ppx=0; ppx<wratio; ppx++ )
           {
             pixsum += (data[(py+ppy)*x->x_vwidth + (px+ppx)]>>7);
           }
         }
         aa_putpixel(x->x_context, px/wratio, py/hratio, pixsum/(wratio*hratio));
      }
    }

    if ( x->x_render )
    {
       aa_fastrender(x->x_context, 0, 0, aa_scrwidth(x->x_context), aa_scrheight(x->x_context));
       aa_flush( x->x_context );
    }

    // post( "pdp_aa : ascii text : %s", x->x_context->textbuffer );

    memcpy( newdata, data, (x->x_vsize+(x->x_vsize>>1))<<1 );

    return;
}

static void pdp_aa_sendpacket(t_pdp_aa *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_aa_process(t_pdp_aa *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_aa_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_aa_process_yv12, pdp_aa_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_aa_process */
	    break;
	    
	}
    }

}

static void pdp_aa_input_0(t_pdp_aa *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))  
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_aa_process(x);
    }

}

static void pdp_aa_free(t_pdp_aa *x)
{
  int i;

   pdp_queue_finish(x->x_queue_id);
   pdp_packet_mark_unused(x->x_packet0);
   pdp_aa_free_ressources(x);
   for (i=0; i<MAX_OPTIONS; i++)
   {
       if (x->x_aa_options[ i ]) freebytes( x->x_aa_options[ i ], MAX_OPTION_LENGTH );
   }
   if (x->x_aa_options) freebytes( x->x_aa_options, MAX_OPTIONS*sizeof(char*) );
   if (x->x_driver) freebytes( x->x_driver, MAX_OPTION_LENGTH ); 
}

t_class *pdp_aa_class;

void *pdp_aa_new(void)
{
  int i;

   t_pdp_aa *x = (t_pdp_aa *)pd_new(pdp_aa_class);

   x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
   x->x_packet0 = -1;
   x->x_packet1 = -1;
   x->x_queue_id = -1;
   x->x_driver = (char*) getbytes( MAX_OPTION_LENGTH ); 
   strcpy( x->x_driver, "X11" );
   x->x_render = 1;

   // aa_setsupported( x->x_context, AA_EXTENDED );
   x->x_aa_options = (char **) getbytes( MAX_OPTIONS*sizeof(char*) );
   for (i=0; i<MAX_OPTIONS; i++)
   {
       x->x_aa_options[ i ] = (char*) getbytes( MAX_OPTION_LENGTH );
   }

   return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_aa_setup(void)
{
    // post( pdp_aa_version );
    pdp_aa_class = class_new(gensym("pdp_aa"), (t_newmethod)pdp_aa_new,
    	(t_method)pdp_aa_free, sizeof(t_pdp_aa), 0, A_NULL);

    class_addmethod(pdp_aa_class, (t_method)pdp_aa_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_aa_class, (t_method)pdp_aa_driver, gensym("driver"),  A_SYMBOL, A_NULL);
    class_addmethod(pdp_aa_class, (t_method)pdp_aa_render, gensym("render"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
