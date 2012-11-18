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

/*  This object is an ASCII art object replacing blocks with ASCII characters
 *  Written by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include "yuv.h"
#include "default.map"
#include <math.h>
#include <stdio.h>

#define LINE_MAX_LENGTH 1024

static char *pdp_ascii_version = "pdp_ascii: version 0.1, ASCII art output written by ydegoyon@free.fr";

typedef struct pdp_ascii_struct
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

    int x_color; // rendering color option
    int x_brightness; // added value for brightness
    t_float x_ratio;    // character to pixel ratio

    int x_charwidth;  // width of characters 
    int x_charheight; // height of characters 
    int x_nbchars;    // number of characters in the map
    char* x_charmaps;   // the table of characters

    FILE  *x_filed;     // charmaps file descriptor

} t_pdp_ascii;

static void pdp_ascii_color(t_pdp_ascii *x, t_floatarg fcolor)
{
   if ( ((int)fcolor == 0) || ((int)fcolor == 1) )
   {
      x->x_color = (int)fcolor;
   }
}

static void pdp_ascii_ratio(t_pdp_ascii *x, t_floatarg fratio)
{
   if ( ( fratio > 0) 
        && ( fratio < x->x_vwidth/x->x_charwidth ) 
        && ( fratio < x->x_vheight/x->x_charheight ) ) 
   {
      x->x_ratio = fratio;
      // post( "psp_ascii : set ratio : %f", x->x_ratio );
   }
}

static void pdp_ascii_brightness(t_pdp_ascii *x, t_floatarg fbrightness)
{
   if ( ((int)fbrightness > 0) && ((int)fbrightness < 255) )
   {
      x->x_brightness = (int)fbrightness;
   }
}

static void pdp_ascii_process_yv12(t_pdp_ascii *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i, pixsum;
    int     px, py, ppx, ppy;
    int     rank, value;
    int     pwidth, pheight, offset;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memset( newdata, 0x00, (x->x_vsize+(x->x_vsize>>1))<<1 );

    pwidth = (int) x->x_charwidth*x->x_ratio;
    if (pwidth==0) pwidth=1;
    // if (pwidth>x->x_vwidth) return;
    pheight = (int) x->x_charheight*x->x_ratio;
    if (pheight==0) pheight=1;
    // if (pheight>x->x_vheight) return;

    // post( "psp_ascii : pwidth=%d, pheight=%d", pwidth, pheight );

    for(py=0; py<x->x_vheight; py+=pheight)
    {
      for(px=0; px<x->x_vwidth; px+=pwidth)
      {
         pixsum = 0;
         for ( ppy=0; ppy<pheight; ppy++ )
         {
           for ( ppx=0; ppx<pwidth; ppx++ )
           {
             if ( ( px+ppx >= x->x_vwidth ) ||
                  ( py+ppy >= x->x_vheight ) )
             {
                break;
             }
             pixsum += (data[(py+ppy)*x->x_vwidth + (px+ppx)]>>7);
           }
         }
         rank = (pixsum/(pheight*pwidth))/2; // set the chosen character
         for ( ppy=0; ppy<pheight; ppy++ )
         {
           for ( ppx=0; ppx<pwidth; ppx++ )
           {
              if ( ( px+ppx >= x->x_vwidth ) ||
                   ( py+ppy >= x->x_vheight ) )
              {
                 break;
              }
              offset = rank*x->x_charwidth*x->x_charheight
                   +((int)(ppy/x->x_ratio))*x->x_charwidth+((int)(ppx/x->x_ratio));
              if ( *(x->x_charmaps+offset) && ( offset < x->x_nbchars*x->x_charwidth*x->x_charheight ) )
              {
                 value = ( (2*rank+x->x_brightness) > 255 ) ? 255 : (2*rank+x->x_brightness);
                 newdata[(py+ppy)*x->x_vwidth+(px+ppx)] = (value)<<7;
                 if ( x->x_color )
                 {
                   newdata[x->x_vsize+((py+ppy)>>1)*(x->x_vwidth>>1)+((px+ppx)>>1)] = 
                        data[x->x_vsize+((py+ppy)>>1)*(x->x_vwidth>>1)+((px+ppx)>>1)];
                   newdata[x->x_vsize+(x->x_vsize>>2)+((py+ppy)>>1)*(x->x_vwidth>>1)+((px+ppx)>>1)] = 
                        data[x->x_vsize+(x->x_vsize>>2)+((py+ppy)>>1)*(x->x_vwidth>>1)+((px+ppx)>>1)];
                 }
              }
           }
         } 
      }
    }

    return;
}

static void pdp_ascii_sendpacket(t_pdp_ascii *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_ascii_process(t_pdp_ascii *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_ascii_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_ascii_process_yv12, pdp_ascii_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_ascii_process */
	    break;
	    
	}
    }

}

static void pdp_ascii_input_0(t_pdp_ascii *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))  
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_ascii_process(x);
    }

}

static void pdp_ascii_free(t_pdp_ascii *x)
{
  int i;

   if ( x->x_charmaps ) free( x->x_charmaps );
   pdp_queue_finish(x->x_queue_id);
   pdp_packet_mark_unused(x->x_packet0);
}

    /* load a new charmaps file */
static void pdp_ascii_load(t_pdp_ascii *x, t_symbol *sfile)
{
 char *lineread = (char*) getbytes( LINE_MAX_LENGTH );
 char *word1 = (char*) getbytes( LINE_MAX_LENGTH );
 char *word2 = (char*) getbytes( LINE_MAX_LENGTH );
 char *word3 = (char*) getbytes( LINE_MAX_LENGTH );
 int charwidth, charheight, nbchars, nblines;
 int nbexpdata;
 char *pdata;
 char charread;

  // opening new charmaps file
  if ( ( x->x_filed = fopen( sfile->s_name, "r" ) ) == NULL )
  {
       error( "pdp_ascii : cannot open >%s<", sfile->s_name);
       return;
  }
  post( "pdp_ascii : opened >%s<", sfile->s_name);

  // read the new dimensions of the charmaps
  charwidth = -1;
  charheight = -1;
  nbchars = -1;
  nblines = 0;
  while ( lineread[0] != '{' )
  {
     if ( fgets( lineread, LINE_MAX_LENGTH, x->x_filed ) == 0 )
     {
       post( "pdp_ascii : abnormal end of file encountered..." );
       goto closeandreturn;
     }

     if ( strncmp( lineread, "#define", 7 ) == 0 )
     {
        sscanf( lineread, "%s %s %s", word1, word2, word3 );
        // post( "pdp_ascii : definition : %s = %s", word2, word3 );
        if ( !strcmp( word2, "CHARWIDTH" ) )
        {
           charwidth = atoi( word3 );
        }
        if ( !strcmp( word2, "CHARHEIGHT" ) )
        {
           charheight = atoi( word3 );
        }
        if ( !strcmp( word2, "NBCHARS" ) )
        {
           nbchars = atoi( word3 );
        }
     }

     nblines++;
     if ( nblines>20 ) break;
  }

  if ( ( charwidth > 0 ) && ( charheight > 0 ) && ( nbchars > 0 ) )
  {
    post( "pdp_ascii : new dimensions : %d %dx%d characters", nbchars, charwidth, charheight );
    if ( x->x_charmaps ) free( x->x_charmaps );
    x->x_charwidth = charwidth;
    x->x_charheight = charheight;
    x->x_nbchars = nbchars;
    x->x_charmaps = (char*)malloc(x->x_nbchars*x->x_charwidth*x->x_charheight*sizeof(char));
  }
  else
  {
    post( "pdp_ascii : wrong file format : couldn't read new dimensions" );
    goto closeandreturn;
  }

  nbexpdata = nbchars*charwidth*charheight;
  pdata = x->x_charmaps;

  while ( ( ( charread = fgetc( x->x_filed ) ) != EOF ) && ( nbexpdata > 0 ) )
  {
    switch ( charread )
    {
      case '0':
      case '1':
        // post( "pdp_ascii : read %c", charread );
        *pdata++ = ( charread == '0' )?0:1;
        nbexpdata--;
        break;

      default:
        break;
    }
  }

closeandreturn :

  // closing opened file
  if ( x->x_filed != NULL ) 
  {
    if(fclose(x->x_filed) < 0)
    {
      perror( "pdp_ascii : closing file" );
    }
    x->x_filed = NULL;
    return;
  }
}

t_class *pdp_ascii_class;

void *pdp_ascii_new(void)
{
  int i;

   t_pdp_ascii *x = (t_pdp_ascii *)pd_new(pdp_ascii_class);
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ratio"));

   x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
   x->x_packet0 = -1;
   x->x_packet1 = -1;
   x->x_queue_id = -1;
   x->x_color = 1;
   x->x_ratio = 1.;
   x->x_brightness = 25;

   x->x_charwidth = CHARWIDTH;
   x->x_charheight = CHARHEIGHT;
   x->x_nbchars = NBCHARS;
   x->x_charmaps = (char*)malloc(x->x_nbchars*x->x_charwidth*x->x_charheight*sizeof(char));
   memcpy( (void*)x->x_charmaps, (void*)charmaps, x->x_nbchars*x->x_charwidth*x->x_charheight*sizeof(char) );

   return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ascii_setup(void)
{
    // post( pdp_ascii_version );
    pdp_ascii_class = class_new(gensym("pdp_ascii"), (t_newmethod)pdp_ascii_new,
    	(t_method)pdp_ascii_free, sizeof(t_pdp_ascii), 0, A_NULL);

    class_addmethod(pdp_ascii_class, (t_method)pdp_ascii_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ascii_class, (t_method)pdp_ascii_color, gensym("color"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ascii_class, (t_method)pdp_ascii_brightness, gensym("brightness"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ascii_class, (t_method)pdp_ascii_ratio, gensym("ratio"),  A_DEFFLOAT, A_NULL);
    class_addmethod( pdp_ascii_class, (t_method)pdp_ascii_load, gensym("load"), A_SYMBOL, 0);


}

#ifdef __cplusplus
}
#endif
