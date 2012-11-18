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

/*  This object is a text rendering object for PDP
 *  It uses imlib2 for all graphical operations
 */

/*  Listening to :
 *  Deviants - Nothing Man
 *  Monte Cazzaza - Kick That Habit Man
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <ctype.h>
#include <Imlib2.h>  // imlib2 is required

#define DEFAULT_CAPACITY 10
#define DEFAULT_FONT "Vera/16"

static char   *pdp_text_version = "pdp_text: version 0.2 : text rendering object written by ydegoyon@free.fr";

typedef struct pdp_text_struct
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

    char **x_text_array;
    int *x_xoffsets;
    int *x_yoffsets;
    int *x_r;
    int *x_g;
    int *x_b;
    t_float *x_angle;
    t_float x_alpha;
    int *x_scroll;

    int x_nbtexts;
    int x_current;
    int x_capacity;

        /* imlib data */
    Imlib_Image x_image;
    Imlib_Font x_font;

} t_pdp_text;

        /* add a new text : syntax : text <my%20text> x y */
static void pdp_text_add(t_pdp_text *x, t_symbol *s, int argc, t_atom *argv)
{
 char *pname;
 char *pdname;
 int len;

   if ( x->x_nbtexts >= x->x_capacity )
   {
      post( "pdp_text : sorry, maximum capacity has been reached... try resize" );
      return;
   } 
   
   if ( argc < 3 )
   {
      post( "pdp_text : error in the number of arguments ( minimum is 3 )", argc );
      return;
   }
   if ( argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT ) {
      post( "pdp_text : add : wrong arguments" );
      return;
   }

   // allocate new text area
   len = strlen( argv[0].a_w.w_symbol->s_name ); 
   pdname = x->x_text_array[x->x_nbtexts] = (char *) getbytes( 6*len+1 );
   pname = (char *) getbytes( len+1 );
   memset( pname, 0x0, len+1 );
   memcpy( pname, argv[0].a_w.w_symbol->s_name, len );
   while (*(pname))
   {
      if ( (*pname=='%') && ( isdigit(*(pname+1)) || (*(pname+1)=='%') ) )
      {
        int ivalue;
        int ndigits;
        char  *piname;

         ndigits=0;
         piname=pname+1;
         while ( isdigit( *(piname++) ) ) ndigits++;
         
         // special case %%
         if ( ( pname != argv[0].a_w.w_symbol->s_name ) && ( *(pname+1) == '%' ) )
         {
            *(pdname++)=*(pname++);
            pname++;
            continue;
         } 

         ivalue=atoi(pname+1);

         // encode to utf
         if ( ivalue < 0x7F )
         {
            *(pdname++)=(char)ivalue;
         }
         else if ( ivalue <= 0x7FF )
         {
            *(pdname++)=(char)(192 + (ivalue/64));
            *(pdname++)=(char)(128 + (ivalue%64));
         }
         else if ( ivalue <= 0xFFFF )
         {
            *(pdname++)=(char)(224 + (ivalue/4096));
            *(pdname++)=(char)(128 + ((ivalue/64)%64));
            *(pdname++)=(char)(128 + (ivalue%64));
         }
         else if ( ivalue <= 0x1FFFFF )
         {
            *(pdname++)=(char)(240 + (ivalue/262144));
            *(pdname++)=(char)(128 + ((ivalue/4096)%64));
            *(pdname++)=(char)(128 + ((ivalue/64)%64));
            *(pdname++)=(char)(128 + (ivalue%64));
         }
         else if ( ivalue <= 0x3FFFFFF )
         {
            *(pdname++)=(char)(248 + (ivalue/16777216));
            *(pdname++)=(char)(128 + ((ivalue/262144)%64));
            *(pdname++)=(char)(128 + ((ivalue/4096)%64));
            *(pdname++)=(char)(128 + ((ivalue/64)%64));
            *(pdname++)=(char)(128 + (ivalue%64));
         }
         else if ( ivalue <= 0x7FFFFFFF )
         {
            *(pdname++)=(char)(252 + (ivalue/1073741824));
            *(pdname++)=(char)(128 + ((ivalue/16777216)%64));
            *(pdname++)=(char)(128 + ((ivalue/262144)%64));
            *(pdname++)=(char)(128 + ((ivalue/4096)%64));
            *(pdname++)=(char)(128 + ((ivalue/64)%64));
            *(pdname++)=(char)(128 + (ivalue%64));
         }

         pname+=ndigits+1;
      }
      else if ( !strncmp( pname, "\"", 1 ) ) // quotes are ignored unless %34
      { 
        pname++;
      }
      else
      {
         *(pdname++)=*(pname++);
      } 
   }
   *(pdname)='\0';
   x->x_xoffsets[x->x_nbtexts] = (int)argv[1].a_w.w_float;
   x->x_yoffsets[x->x_nbtexts] = (int)argv[2].a_w.w_float;

   if ( (argc>=4) && (argv[3].a_type == A_FLOAT) )
   {
      x->x_r[x->x_nbtexts] = (int)argv[3].a_w.w_float;
   }
   if ( (argc>=5) && (argv[4].a_type == A_FLOAT) )
   {
      x->x_g[x->x_nbtexts] = (int)argv[4].a_w.w_float;
   }
   if ( (argc>=6) && (argv[5].a_type == A_FLOAT) )
   {
      x->x_b[x->x_nbtexts] = (int)argv[5].a_w.w_float;
   }
   if ( (argc>=7) && (argv[6].a_type == A_FLOAT) )
   {
      x->x_angle[x->x_nbtexts] = argv[6].a_w.w_float;
   }
   if ( (argc>=8) && (argv[7].a_type == A_FLOAT) )
   {
      x->x_scroll[x->x_nbtexts] = (int)argv[7].a_w.w_float;
   }
   
   post( "pdp_text : added text >%s< @ %d (r=%d g=%d b=%d)", 
                x->x_text_array[x->x_nbtexts], x->x_nbtexts,
                   x->x_r[x->x_nbtexts], x->x_g[x->x_nbtexts], x->x_b[x->x_nbtexts] );

   if ( x->x_current == -1 ) x->x_current = x->x_nbtexts;
   x->x_nbtexts++;
   
}

static void pdp_text_current(t_pdp_text *x, t_floatarg fcurrent )
{
    if ( ( fcurrent >= 0 ) && ( fcurrent < x->x_nbtexts ) )
    {
       x->x_current = fcurrent;
    }
}

static void pdp_text_textx(t_pdp_text *x, t_floatarg fx )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbtexts ) )
    {
       x->x_xoffsets[ x->x_current ] = fx;
    }
}

static void pdp_text_texty(t_pdp_text *x, t_floatarg fy )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbtexts ) )
    {
       x->x_yoffsets[ x->x_current ] = fy;
    }
}

static void pdp_text_textr(t_pdp_text *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbtexts ) )
    {
       x->x_r[ x->x_current ] = fr;
    }
}

static void pdp_text_textg(t_pdp_text *x, t_floatarg fg )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbtexts ) )
    {
       x->x_g[ x->x_current ] = fg;
    }
}

static void pdp_text_textb(t_pdp_text *x, t_floatarg fb )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbtexts ) )
    {
       x->x_b[ x->x_current ] = fb;
    }
}

static void pdp_text_angle(t_pdp_text *x, t_floatarg fangle )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbtexts ) )
    {
       x->x_angle[ x->x_current ] = fangle;
    }
}

static void pdp_text_alpha(t_pdp_text *x, t_floatarg falpha )
{
    if ( ( falpha  >= 0. ) && ( falpha <= 1. ) )
    {
       x->x_alpha = falpha;
    }
}

static void pdp_text_scroll(t_pdp_text *x, t_floatarg fscroll )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbtexts ) )
    {
       x->x_scroll[ x->x_current ] = fscroll;
    }
}

static void pdp_text_dither(t_pdp_text *x, t_floatarg fdither )
{
    imlib_context_set_dither( (char)fdither );
}

static void pdp_text_blend(t_pdp_text *x, t_floatarg fblend )
{
    imlib_context_set_blend( (char)fblend );
}

static void pdp_text_antialias(t_pdp_text *x, t_floatarg fantialias )
{
    imlib_context_set_anti_alias( (char)fantialias );
}

static void pdp_text_clear(t_pdp_text *x )
{
    x->x_nbtexts = 0;
}

static void pdp_text_delete(t_pdp_text *x,  t_floatarg fnum  )
{
  int i;
  char *lostword;

    if ( ( fnum>0 ) && ( fnum<=x->x_nbtexts ) )
    {
       lostword = x->x_text_array[ (int)fnum-1 ];
       for ( i=(int)fnum; i<x->x_nbtexts; i++ )
       {
          x->x_text_array[ i-1 ] = x->x_text_array[ i ];
          x->x_xoffsets[ i-1 ] = x->x_xoffsets[ i ];
          x->x_yoffsets[ i-1 ] = x->x_yoffsets[ i ];
          x->x_r[ i-1 ] = x->x_r[ i ];
          x->x_g[ i-1 ] = x->x_g[ i ];
          x->x_b[ i-1 ] = x->x_b[ i ];
          x->x_angle[ i-1 ] = x->x_angle[ i ];
	  x->x_scroll[ i-1 ] = x->x_scroll[ i ];
       }
       x->x_nbtexts--,
       free( lostword );
    }
}

static void pdp_text_resize(t_pdp_text *x,  t_floatarg fnewsize  )
{
  char **text_array;
  int *xoffsets;
  int *yoffsets;
  int *r;
  int *g;
  int *b;
  t_float *angle;
  int *scroll;

  int i, csize;

    if ( (int) fnewsize<=0 ) return;

    // allocate new structures
    text_array = (char**) getbytes( fnewsize*sizeof(char*) );
    xoffsets = (int*) getbytes( fnewsize*sizeof(int) );
    yoffsets = (int*) getbytes( fnewsize*sizeof(int) );
    r = (int*) getbytes( fnewsize*sizeof(int) );
    g = (int*) getbytes( fnewsize*sizeof(int) );
    b = (int*) getbytes( fnewsize*sizeof(int) );
    angle = (t_float*) getbytes( fnewsize*sizeof(t_float) );
    scroll = (int*) getbytes( fnewsize*sizeof(int) );


    for ( i=0; i<fnewsize; i++ )
    {
       r[i] = g[i] = b[i] = 255;
    }

    if ( fnewsize < x->x_nbtexts )
    {
       post( "pdp_text : new size is too small : texts lost !!" );
       csize = fnewsize;
    }
    else
    {
       csize = x->x_nbtexts;
    }

    // copy all values
    for ( i=0; i<csize; i++ )
    {
        text_array[i] = (char*) malloc( strlen( x->x_text_array[i] ) + 1 );
        strcpy( text_array[i], x->x_text_array[i] );
        free( x->x_text_array[i] );
        xoffsets[i] = x->x_xoffsets[i];
        yoffsets[i] = x->x_yoffsets[i];
        r[i] = x->x_r[i];
        g[i] = x->x_g[i];
        b[i] = x->x_b[i];
        angle[i] = x->x_angle[i];
        scroll[i] = x->x_scroll[i];
    }
  
    // free old structures
    if ( x->x_text_array ) freebytes( x->x_text_array, x->x_capacity*sizeof(char*) );
    if ( x->x_xoffsets ) freebytes( x->x_xoffsets, x->x_capacity*sizeof(int) );
    if ( x->x_yoffsets ) freebytes( x->x_yoffsets, x->x_capacity*sizeof(int) );
    if ( x->x_r ) freebytes( x->x_r, x->x_capacity*sizeof(int) );
    if ( x->x_g ) freebytes( x->x_g, x->x_capacity*sizeof(int) );
    if ( x->x_b ) freebytes( x->x_b, x->x_capacity*sizeof(int) );
    if ( x->x_angle) freebytes( x->x_angle, x->x_capacity*sizeof(t_float) );
    if ( x->x_scroll) freebytes( x->x_scroll, x->x_capacity*sizeof(int) );

    // set new structures
    x->x_text_array = text_array;
    x->x_xoffsets = xoffsets;
    x->x_yoffsets = yoffsets;
    x->x_r = r;
    x->x_g = g;
    x->x_b = b;
    x->x_angle = angle;
    x->x_scroll = scroll;
    x->x_nbtexts = csize;
    x->x_capacity = (int) fnewsize;
    if ( x->x_nbtexts > 0 )
    {
       x->x_current = 0;
    }
    else
    {
       x->x_current = -1;
    }
    post( "pdp_text : resized to %d", (int) fnewsize );
}

static void pdp_text_font(t_pdp_text *x, t_symbol *sfont  )
{
  Imlib_Font font;

    font = imlib_load_font(sfont->s_name);
    if ( !font )
    {
       pd_error( x, "[%s] error: could not load font: '%s'", 
                 class_getname(*(t_pd *)x), sfont->s_name );
       return;
    }
    x->x_font = font;
}

static void pdp_text_allocate(t_pdp_text *x)
{
   x->x_image = imlib_create_image( x->x_vwidth, x->x_vheight );
   if ( x->x_image == NULL )
   {
       pd_error( x, "[%s] error: could not allocate image!!", 
                 class_getname(*(t_pd *)x) );
       return;
   }
   imlib_context_set_image(x->x_image);
}

static void pdp_text_free_ressources(t_pdp_text *x)
{
   // if ( x->x_image != NULL ) imlib_free_image();
}

static void pdp_text_process_yv12(t_pdp_text *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     ti;
    int     px, py;
    unsigned char y, u, v;
    DATA32    *imdata;
    DATA32    bgcolor;
    short int *pY, *pU, *pV;
    int	text_width, text_height;

    if ( ( (int)(header->info.image.width) != x->x_vwidth ) ||
         ( (int)(header->info.image.height) != x->x_vheight ) )
    {
         pdp_text_free_ressources(x);
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         pdp_text_allocate(x);
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize+(x->x_vsize>>1))<<1 );

    // draw all texts
    if ( x->x_image != NULL ) imlib_context_set_image(x->x_image);
    imlib_image_clear();
    imlib_context_set_direction(IMLIB_TEXT_TO_ANGLE);
    imdata = imlib_image_get_data();
    bgcolor = imdata[0];

    for (ti=0; ti<x->x_nbtexts; ti++)
    {
       imlib_context_set_angle( x->x_angle[ti] );
       imlib_context_set_color( x->x_r[ti], x->x_g[ti], x->x_b[ti], 255 );

       imlib_context_set_font( x->x_font );
       imlib_get_text_size( x->x_text_array[ti], &text_width, &text_height);

       imlib_text_draw( x->x_xoffsets[ti] - (0.5*text_width) + (cos(x->x_angle[ti]) * x->x_scroll[ti]), 
                        x->x_yoffsets[ti] - (0.5*text_height) + (sin(x->x_angle[ti]) * x->x_scroll[ti]), 
                        x->x_text_array[ti] );
    }

    pY = newdata;
    pV = newdata+x->x_vsize;
    pU = newdata+x->x_vsize+(x->x_vsize>>2);
    for ( py=0; py<x->x_vheight; py++ )
    {
       for ( px=0; px<x->x_vwidth; px++ )
       {
          if ( imdata[py*x->x_vwidth+px] != bgcolor )
          {
            y = yuv_RGBtoY(imdata[py*x->x_vwidth+px]);
            u = yuv_RGBtoU(imdata[py*x->x_vwidth+px]);
            v = yuv_RGBtoV(imdata[py*x->x_vwidth+px]);
          
            *(pY) = ((y)<<7)*x->x_alpha + (*pY)*(1-x->x_alpha);
            if ( (px%2==0) && (py%2==0) )
            {
              *(pV) = ((v-128)<<8)*x->x_alpha + *(pV)*(1-x->x_alpha);
              *(pU) = ((u-128)<<8)*x->x_alpha + *(pU)*(1-x->x_alpha);
            }
          }
          pY++;
          if ( (px%2==0) && (py%2==0) )
          {
            pV++;pU++;
          }
       }
    }

    return;
}

static void pdp_text_sendpacket(t_pdp_text *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_text_process(t_pdp_text *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_text_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_text_process_yv12, pdp_text_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_text_process */
	    break;
	    
	}
    }

}

static void pdp_text_input_0(t_pdp_text *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_text_process(x);

    }

}

static void pdp_text_free(t_pdp_text *x)
{
  int i;

    pdp_text_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_text_class;

void *pdp_text_new(void)
{
    int i;

    t_pdp_text *x = (t_pdp_text *)pd_new(pdp_text_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("current"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("texty"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textr"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textg"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textb"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("angle"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("scroll"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("alpha"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;
    x->x_image = NULL;

    x->x_font = imlib_context_get_font();

    x->x_capacity = DEFAULT_CAPACITY;


    x->x_text_array = (char**) getbytes( x->x_capacity*sizeof(char*) );
    x->x_xoffsets = (int*) getbytes( x->x_capacity*sizeof(int) );
    x->x_yoffsets = (int*) getbytes( x->x_capacity*sizeof(int) );
    x->x_r = (int*) getbytes( x->x_capacity*sizeof(int) );
    x->x_g = (int*) getbytes( x->x_capacity*sizeof(int) );
    x->x_b = (int*) getbytes( x->x_capacity*sizeof(int) );
    x->x_angle = (t_float*) getbytes( x->x_capacity*sizeof(t_float) );
    x->x_scroll = (int*) getbytes( x->x_capacity*sizeof(int) );

   for ( i=0; i<x->x_capacity; i++ )
    {
       x->x_r[i] = x->x_g[i] = x->x_b[i] = 255;
    }

    x->x_nbtexts = 0;
    x->x_current = -1;
    x->x_alpha = 1.;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_text_setup(void)
{
  Imlib_Font font;

    // post( pdp_text_version );
    pdp_text_class = class_new(gensym("pdp_text"), (t_newmethod)pdp_text_new,
    	(t_method)pdp_text_free, sizeof(t_pdp_text), 0, A_NULL);

    class_addmethod(pdp_text_class, (t_method)pdp_text_input_0, gensym("pdp"),  
                             A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_add, gensym("text"),  A_GIMME, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_current, gensym("current"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_textx, gensym("textx"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_texty, gensym("texty"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_textr, gensym("textr"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_textg, gensym("textg"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_textb, gensym("textb"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_clear, gensym("clear"),  A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_delete, gensym("delete"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_resize, gensym("resize"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_font, gensym("font"),  A_SYMBOL, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_angle, gensym("angle"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_alpha, gensym("alpha"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_scroll, gensym("scroll"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_dither, gensym("dither"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_blend, gensym("blend"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_text_class, (t_method)pdp_text_antialias, gensym("antialias"),  A_DEFFLOAT, A_NULL);

    imlib_add_path_to_font_path("/usr/X11R6/lib/X11/fonts/TTF");
#ifdef __APPLE__
    imlib_add_path_to_font_path("/System/Library/Fonts");
    imlib_add_path_to_font_path("/Library/Fonts");
    imlib_add_path_to_font_path("/sw/share/imlib2/data/fonts");
    imlib_add_path_to_font_path("/sw/lib/X11/fonts/msttf");
#else
    imlib_add_path_to_font_path("/var/lib/defoma/x-ttcidfont-conf.d/dirs/TrueType");
#endif
    font = imlib_load_font(DEFAULT_FONT);
    if ( !font )
    {
      char **ifonts;
      int nbfonts, jf;
      char fname[64];

        ifonts = imlib_list_fonts( &nbfonts ); 
        for ( jf=0; jf<nbfonts; jf++ )
        {
           sprintf( fname, "%s/14", ifonts[jf] );
           font = imlib_load_font( fname );
           if ( !font )
           {
              post( "[pdp_text] error loading font : %s", fname );
           }
           else
           {
              post( "[pdp_text] using font : %s", fname );
              break;
           }
        }
    }
    imlib_context_set_font( font );
}

#ifdef __cplusplus
}
#endif
