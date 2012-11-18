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

/*  This object saves a snaphot to a file 
 *  Image type is specified by extension
 *  It uses imlib2 for all graphical operations
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <ctype.h>
#include <pthread.h>
#include <Imlib2.h>  // imlib2 is required

static char   *pdp_imgsaver_version = "pdp_imgsaver: version 0.1 : image snapshot object written by ydegoyon@free.fr ";

typedef struct pdp_imgsaver_struct
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

    int x_save_pending;

        /* imlib data */
    Imlib_Image x_image;
    DATA32     *x_imdata;
    int       x_iwidth;
    int       x_iheight;

    t_symbol    *x_filename;
    short int   *x_datas;
    pthread_t    x_savechild;    /* thread id for the saving child              */

} t_pdp_imgsaver;

        /* do save the image */
static void *pdp_imgsaver_do_save(void *tdata)
{
  Imlib_Load_Error imliberr;
  t_pdp_imgsaver *x = (t_pdp_imgsaver*) tdata;
  int px, py;
  short int *pY, *pU, *pV;
  unsigned char y, u, v;

   if ( ( x->x_vwidth == 0 ) || ( x->x_vheight == 0 ) )
   {
      post( "pdp_imgsaver : tried to save an image but no video is playing" );
      x->x_save_pending = 0;
      x->x_filename = NULL;
      return NULL;
   }

   x->x_image = imlib_create_image( x->x_vwidth, x->x_vheight );
   if ( x->x_image == NULL )
   {
      post( "pdp_imgsaver : severe error : could not allocate image !!" );
      x->x_save_pending = 0;
      x->x_filename = NULL;
      return NULL;
   }
   imlib_context_set_image(x->x_image);

   x->x_imdata = imlib_image_get_data();
   x->x_iwidth = imlib_image_get_width();
   x->x_iheight = imlib_image_get_height();

   pY = x->x_datas;
   pV = x->x_datas+x->x_vsize;
   pU = x->x_datas+x->x_vsize+(x->x_vsize>>2);
   for ( py=0; py<x->x_iheight; py++ )
   {
     for ( px=0; px<x->x_iwidth; px++ )
     {
       y = *(pY)>>7;
       v = (*(pV)>>8)+128;
       u = (*(pU)>>8)+128;
       x->x_imdata[ py*x->x_iwidth + px ] = yuv_YUVtoRGB( y, u, v );
       pY++;
       if ( (px%2==0) && (py%2==0) )
       {
         pV++;pU++;
       }
     }
   }

   post( "pdp_imgsaver : saving image to : %s", x->x_filename->s_name );
   imlib_save_image_with_error_return(x->x_filename->s_name, &imliberr );
   if ( imliberr != IMLIB_LOAD_ERROR_NONE )
   {
      post( "pdp_imgsaver : severe error : could not save image (err=%d)!!", imliberr );
   }
   else
   {
      post( "pdp_imgsaver : saved to : %s", x->x_filename->s_name );
   }

   //if ( x->x_image != NULL ) 
   //{
   //   imlib_free_image();
   //}
   x->x_image = NULL;

   x->x_save_pending = 0;
   x->x_filename = NULL;

   post( "pdp_imgsaver : saving thread %d finished", x->x_savechild );
   x->x_savechild = 0;

   return NULL;

}

        /* launched the thread to save the image */
static void pdp_imgsaver_save(t_pdp_imgsaver *x, t_symbol *filename)
{
 pthread_attr_t save_child_attr;

   if ( x->x_save_pending  ) 
   {
      post( "pdp_imgsaver : a save operation is currently running...retry later" );
      return;
   }
   x->x_save_pending = 1;
   x->x_filename = filename;
   //if ( x->x_image != NULL ) 
   //{
   //   imlib_free_image();
   //}
   x->x_image = NULL;

   // launch saving thread
   if ( pthread_attr_init( &save_child_attr ) < 0 ) 
   {
        post( "pdp_imgsaver : could not launch save thread" );
        perror( "pthread_attr_init" );
        return;
   }
   if ( pthread_attr_setdetachstate( &save_child_attr, PTHREAD_CREATE_DETACHED ) < 0 ) {
        post( "pdp_imgsaver : could not launch save thread" );
        perror( "pthread_attr_setdetachstate" );
        return;
   }
   if ( pthread_create( &x->x_savechild, &save_child_attr, pdp_imgsaver_do_save, x ) < 0 ) {
        post( "pdp_imgsaver : could not launch save thread" );
        perror( "pthread_create" );
        return;
   }
   else
   {
        post( "pdp_imgsaver : saving thread %d launched", (int)x->x_savechild );
   }
}

static void pdp_imgsaver_allocate(t_pdp_imgsaver *x)
{
   x->x_datas = (short int *)getbytes((( x->x_vsize + (x->x_vsize>>1))<<1));
}

static void pdp_imgsaver_free_ressources(t_pdp_imgsaver *x)
{
   if ( x->x_datas ) freebytes( x->x_datas, (( x->x_vsize + (x->x_vsize>>1))<<1));
}

static void pdp_imgsaver_process_yv12(t_pdp_imgsaver *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     px, py;
    t_float   alpha, factor;
    unsigned  char y, u, v;
    short int *pY, *pU, *pV;

    if ( ( (int)(header->info.image.width) != x->x_vwidth ) ||
         ( (int)(header->info.image.height) != x->x_vheight ) )
    {
         pdp_imgsaver_free_ressources(x);	    
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         pdp_imgsaver_allocate(x);
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize+(x->x_vsize>>1))<<1 );
    if ( !x->x_save_pending )
    {
      memcpy( x->x_datas, data, (x->x_vsize+(x->x_vsize>>1))<<1 );
    }

    return;
}

static void pdp_imgsaver_sendpacket(t_pdp_imgsaver *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_imgsaver_process(t_pdp_imgsaver *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_imgsaver_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_imgsaver_process_yv12, pdp_imgsaver_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_imgsaver_process */
	    break;
	    
	}
    }

}

static void pdp_imgsaver_input_0(t_pdp_imgsaver *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_imgsaver_process(x);

    }

}

static void pdp_imgsaver_free(t_pdp_imgsaver *x)
{
  int i;

    pdp_imgsaver_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_imgsaver_class;

void *pdp_imgsaver_new(void)
{
    int i;

    t_pdp_imgsaver *x = (t_pdp_imgsaver *)pd_new(pdp_imgsaver_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_datas = NULL;
    x->x_queue_id = -1;
    x->x_image = NULL;

    x->x_vwidth = 0;
    x->x_vheight = 0;

    x->x_save_pending = 0;
    x->x_filename = NULL;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_imgsaver_setup(void)
{

    // post( pdp_imgsaver_version );
    pdp_imgsaver_class = class_new(gensym("pdp_imgsaver"), (t_newmethod)pdp_imgsaver_new,
    	(t_method)pdp_imgsaver_free, sizeof(t_pdp_imgsaver), 0, A_NULL);

    class_addmethod(pdp_imgsaver_class, (t_method)pdp_imgsaver_input_0, gensym("pdp"),  
                             A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgsaver_class, (t_method)pdp_imgsaver_save, gensym("save"),  A_SYMBOL, A_NULL);


}

#ifdef __cplusplus
}
#endif
