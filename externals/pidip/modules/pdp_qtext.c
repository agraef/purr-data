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
 *  Os Mutantes - El Justeciero
 */

#define _GNU_SOURCE
#include <string.h>
#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <wchar.h>
#include <ctype.h>
#include <Imlib2.h>  // imlib2 is required

#define DEFAULT_CAPACITY 10

#define DEFAULT_FONT "Vera/16"

#define PIDIP_TEXT_MODE_STATIC 0
#define PIDIP_TEXT_MODE_SCROLL 1
#define PIDIP_TEXT_MODE_FEED   2
#define PIDIP_TEXT_MODE_SLOW   3

#define PIDIP_ALIGNMENT_CENTER  0
#define PIDIP_ALIGNMENT_LEFT  1
#define PIDIP_ALIGNMENT_RIGHT  2

static char   *pdp_qtext_version = "pdp_qtext: version 0.2 : modified pdp_text by Pablo Martin Caedes";

typedef struct text_frame_struct TEXT_FRAME;
struct text_frame_struct 
{
	char *text_array;
 	TEXT_FRAME *next;
	double time;
};

typedef struct text_layer_struct
{
    int l_xoffset;	// x position
    int l_yoffset;    // y position
    int l_r;		// first color
    int l_g;
    int l_b;
    int l_a;
    int l_2r;	// color for even lines of feed
    int l_2g;
    int l_2b;
    int l_2a;
    int l_borderr;	// color for the border
    int l_borderg;
    int l_borderb;
    t_float l_angle;	// angle
    int l_scroll;
    int l_alignment;	// 0 - center; 1 - left; 2 - right;
    int l_marginh;	// top margin
    int l_marginv;	// bottom margin
    int l_active;	// slot is active?

    int l_feed_turn;  // even or odd turn

    int l_mode;	// work mode: static, scroll, feed/chat, slow
    int l_scroll_speed;	
    int l_upwards;	// whether to put new elements at top or bottom of the queue
    TEXT_FRAME *l_texts;
    TEXT_FRAME *l_last_text;
    int l_ntexts; // number of texts in the chat queue
    Imlib_Font l_font;
} TEXTLAYER;

typedef struct pdp_qtext_struct
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

    int x_nbtexts;
    int x_current;	// currently selected layer
    int x_capacity;	// maximum texts

    // text layers
    TEXTLAYER *x_layers;
    /* imlib data */
    Imlib_Image x_image;

} t_pdp_qtext;

void text_layer_flush(TEXTLAYER *layer)
{
	while(layer->l_texts != NULL)
	{
		TEXT_FRAME *oldframe = layer->l_texts;		
		layer->l_texts = oldframe->next;
		free(oldframe->text_array);
		free(oldframe);
	}
	layer->l_last_text = NULL;
	layer->l_ntexts = 0;

}
void text_layer_add_start(TEXTLAYER *layer,char *text)
{
	TEXT_FRAME *newframe = (TEXT_FRAME*)getbytes(sizeof(TEXT_FRAME));
        newframe->text_array = text;
	newframe->time = 0;

	if (layer->l_texts == NULL)
	{
		layer->l_last_text = newframe;
	        newframe->next = NULL;
	}
	else
		newframe->next = layer->l_texts;
	layer->l_texts = newframe;
	layer->l_ntexts++;
}

void text_layer_add_end(TEXTLAYER *layer, char *text)
{
	TEXT_FRAME *newframe = (TEXT_FRAME*)getbytes(sizeof(TEXT_FRAME));
        newframe->text_array = text;
	newframe->time = 0;
	newframe->next = NULL;

	if (layer->l_texts == NULL)
	        layer->l_texts = newframe;
	else
		layer->l_last_text->next = newframe;
	layer->l_last_text = newframe;
	layer->l_ntexts++;
}
void text_layer_delete_start(TEXTLAYER *layer)
{
	if (layer->l_texts != NULL)
	{
		TEXT_FRAME *oldframe = layer->l_texts;
		//free(oldframe->text_array);
		free(oldframe);
		layer->l_texts = oldframe->next;
	}
	layer->l_ntexts--;
}

TEXT_FRAME * text_layer_find_last(TEXTLAYER *layer)
{
	TEXT_FRAME *actual= layer->l_texts;
	if (actual == NULL)
		return NULL;
	while(actual->next != NULL)
		actual = actual->next;
	return actual;
}
TEXT_FRAME * text_layer_find_ancestor(TEXTLAYER *layer, TEXT_FRAME *following)
{
	TEXT_FRAME *actual= layer->l_texts;
	if (actual == NULL)
		return NULL;
	while(actual->next != following)
		actual = actual->next;
	return actual;
}

void text_layer_delete_end(TEXTLAYER *layer)
{
	if (layer->l_texts != NULL)
	{
		TEXT_FRAME *oldframe = layer->l_last_text;
		//free(oldframe->text_array);
		if (oldframe == layer->l_texts) // IF IT WAS FIRST
		{
			layer->l_last_text = NULL;
			layer->l_texts = NULL;
			free(oldframe);
		}
		else
		{
			layer->l_last_text = text_layer_find_ancestor(layer,layer->l_last_text);
			layer->l_last_text->next = NULL;
		}
		free(oldframe);
	}
	layer->l_ntexts--;
}

static void pdp_qtext_delete(t_pdp_qtext *x,  t_floatarg fnum  );
        /* add a new text : syntax : text <my%20text> x y */
static void pdp_qtext_add(t_pdp_qtext *x, t_symbol *s, int argc, t_atom *argv)
{
 char *pname;
 char *pdname;
 int len;

   if ( argc < 3 )
   {
      post( "pdp_qtext : error in the number of arguments ( minimum is 3 )", argc );
      return;
   }
   if ( argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT ) {
      post( "pdp_qtext : add : wrong arguments" );
      return;
   }

   // allocate new text area
   len = strlen( argv[0].a_w.w_symbol->s_name ); 
   if ( x->x_current == -1 ) x->x_current = 0;
   
   // get memory
   pdname = (char *) getbytes( len+1 );
   pname = (char *) getbytes( len+1 );
   memset( pname, 0x0, len+1 );
   memcpy( pname, argv[0].a_w.w_symbol->s_name, len );
   
   x->x_layers[x->x_current].l_active = 1;
   // add text to selected queue
   switch ( x->x_layers[x->x_current].l_mode)
   {
	case PIDIP_TEXT_MODE_STATIC:	// REPLACE_CURRENT_TEXT
	    text_layer_delete_start(&x->x_layers[x->x_current]);
	    text_layer_add_start(&x->x_layers[x->x_current],pdname);
	    break;
	case PIDIP_TEXT_MODE_SLOW:	// REPLACE_CURRENT_TEXT
	    text_layer_delete_start(&x->x_layers[x->x_current]);
	    text_layer_add_start(&x->x_layers[x->x_current],pdname);
	    break;
	case PIDIP_TEXT_MODE_SCROLL:	// ADD_TEXT_AT_END
	    text_layer_add_end(&x->x_layers[x->x_current],pdname);
	    break;	
	case PIDIP_TEXT_MODE_FEED:	// ADD_TEXT_AT_END_OR_START
	    if (!x->x_layers[x->x_current].l_upwards)	// DOWNWARDS FEED
		text_layer_add_start(&x->x_layers[x->x_current],pdname);
	    else
		text_layer_add_end(&x->x_layers[x->x_current],pdname);
	    break;
    }
   
   // process new text
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
   //freebytes( pname,len+1 ); NOT SO STRANGE CRASHES XXX
   *(pdname)='\0';

   // get other parameters in command
   x->x_layers[x->x_current].l_xoffset = (int)argv[1].a_w.w_float;
   /*if (x->x_layers[x->x_current].l_mode == PIDIP_TEXT_MODE_SCROLL)
   {
	   x->x_layers[x->x_current].l_yoffset = -10;  // XXX: GET REAL STARTING POSITION
   }*/
   //else 
   //{
       x->x_layers[x->x_current].l_yoffset = (int)argv[2].a_w.w_float;
   //}

   if ( (argc>=4) && (argv[3].a_type == A_FLOAT) )
   {
      x->x_layers[x->x_current].l_r = (int)argv[3].a_w.w_float;
   }
   if ( (argc>=5) && (argv[4].a_type == A_FLOAT) )
   {
      x->x_layers[x->x_current].l_g = (int)argv[4].a_w.w_float;
   }
   if ( (argc>=6) && (argv[5].a_type == A_FLOAT) )
   {
      x->x_layers[x->x_current].l_b = (int)argv[5].a_w.w_float;
   }
   if ( (argc>=7) && (argv[6].a_type == A_FLOAT) )
   {
      x->x_layers[x->x_current].l_angle = argv[6].a_w.w_float;
   }
   if ( (argc>=8) && (argv[7].a_type == A_FLOAT) )
   {
      x->x_layers[x->x_current].l_scroll = (int)argv[7].a_w.w_float;
   }
   
   // post( "pdp_qtext : added text >%s< @ %d", 
   //              x->x_layers[x->x_current].l_texts->text_array, x->x_current);

   /*if ( x->x_current > x->x_chat_base ) 
	   x->x_current = x->x_chat_base;
   x->x_nbtexts = x->x_capacity;	// XXX why is this exactly ?? */
   //x->x_nbtexts++;
   
}

static void pdp_qtext_current(t_pdp_qtext *x, t_floatarg fcurrent )
{
    if ( ( fcurrent >= 0 ) && ( fcurrent < x->x_capacity ) )
    {
       	x->x_current = fcurrent;
    }
}

static void pdp_qtext_textx(t_pdp_qtext *x, t_floatarg fx )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_xoffset = fx;
    }
}

static void pdp_qtext_margin(t_pdp_qtext *x, t_floatarg fx )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_marginh = fx;
    }
}

static void pdp_qtext_marginv(t_pdp_qtext *x, t_floatarg fx )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_marginv = fx;
    }
}

static void pdp_qtext_direction(t_pdp_qtext *x, t_floatarg fx )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_upwards = fx;
       // post( "pdp_qtext : set direction to %d for layer %d", fx, x->x_current );
    }
}

static void pdp_qtext_texty(t_pdp_qtext *x, t_floatarg fy )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_yoffset = fy;
    }
}

static void pdp_qtext_textr(t_pdp_qtext *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_r = fr;
    }
}
static void pdp_qtext_texta(t_pdp_qtext *x, t_floatarg fa )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_a = fa;
    }
}

static void pdp_qtext_textg(t_pdp_qtext *x, t_floatarg fg )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_g = fg;
    }
}

static void pdp_qtext_textb(t_pdp_qtext *x, t_floatarg fb )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_b = fb;
    }
}
static void pdp_qtext_text2r(t_pdp_qtext *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_2r = fr;
    }
}
static void pdp_qtext_text2g(t_pdp_qtext *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_2g = fr;
    }
}
static void pdp_qtext_text2b(t_pdp_qtext *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_2b = fr;
    }
}
static void pdp_qtext_text2a(t_pdp_qtext *x, t_floatarg fa )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_2a = fa;
    }
}
static void pdp_qtext_borderr(t_pdp_qtext *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_borderr = fr;
    }
}
static void pdp_qtext_borderg(t_pdp_qtext *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_borderg = fr;
    }
}
static void pdp_qtext_borderb(t_pdp_qtext *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity) )
    {
       x->x_layers[x->x_current].l_borderb = fr;
    }
}

static void pdp_qtext_angle(t_pdp_qtext *x, t_floatarg fangle )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_angle = fangle;
    }
}

static void pdp_qtext_scroll(t_pdp_qtext *x, t_floatarg fscroll )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
       x->x_layers[x->x_current].l_scroll = fscroll;
    }
}

static void pdp_qtext_center(t_pdp_qtext *x )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
    	x->x_layers[x->x_current].l_alignment = PIDIP_ALIGNMENT_CENTER;
    }
}
static void pdp_qtext_left(t_pdp_qtext *x )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
    	x->x_layers[x->x_current].l_alignment = PIDIP_ALIGNMENT_LEFT;
    }
}
static void pdp_qtext_right(t_pdp_qtext *x )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_capacity ) )
    {
    	x->x_layers[x->x_current].l_alignment = PIDIP_ALIGNMENT_RIGHT;
    }
}

static void pdp_qtext_delete(t_pdp_qtext *x,  t_floatarg fnum  )
{
  int i;
  char *lostword;

  text_layer_flush(&x->x_layers[x->x_current]);
  /*if ( ( fnum>=0 ) && ( fnum<x->x_capacity )  )
  {
     if (fnum >= x->x_chat_base && x->x_active[x->x_chat_base])
     {
       lostword = x->x_text_array[ (int)fnum];
       fnum = x->x_chat_base;
       if (x->upwards)
       	  x->x_chat_turn++;
       for ( i=(int)x->x_chat_base; i<x->x_chat_base+x->x_chat_texts-1; i++ )
       {
          x->x_text_array[ i ] = x->x_text_array[ i+1 ];
       }
       x->x_active[x->x_chat_base+x->x_chat_texts-1]=0;
       x->x_chat_texts--;
       free( lostword );
       x->x_nbtexts--;
     }
     else if (x->x_active[(int)fnum])
     {
       lostword = x->x_text_array[ (int)fnum];
       x->x_active[(int)fnum]=0;
       free( lostword );
       x->x_nbtexts--;
     }
   }*/
}
static void pdp_qtext_clear(t_pdp_qtext *x )
{
    text_layer_flush(&x->x_layers[x->x_current]);
    // must free the texts XXX
    x->x_nbtexts = 0;
}
static void pdp_qtext_mode(t_pdp_qtext *x, t_symbol *s)
{
	if      (s == gensym("static")) x->x_layers[x->x_current].l_mode = PIDIP_TEXT_MODE_STATIC;
	else if      (s == gensym("scroll")) x->x_layers[x->x_current].l_mode = PIDIP_TEXT_MODE_SCROLL;
	else if      (s == gensym("feed")) x->x_layers[x->x_current].l_mode = PIDIP_TEXT_MODE_FEED;
	else if      (s == gensym("slow")) x->x_layers[x->x_current].l_mode = PIDIP_TEXT_MODE_SLOW;
}

static void pdp_qtext_resize(t_pdp_qtext *x,  t_floatarg fnewsize  )
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

  TEXTLAYER * layers;
    if ( (int) fnewsize<=0 ) return;

    // allocate new structures
    layers = (TEXTLAYER*) getbytes( x->x_capacity*sizeof(TEXTLAYER) );

    for ( i=0; i<fnewsize; i++ )
    {
       layers[i].l_r = layers[i].l_g = layers[i].l_b = layers[i].l_a = 255;
       layers[i].l_2r = layers[i].l_2g = layers[i].l_2b = layers[i].l_2a = 255;
       layers[i].l_borderr = x->x_layers[i].l_borderg = x->x_layers[i].l_borderb = 255;
       layers[i].l_xoffset = 0;
       layers[i].l_yoffset = 0;
       layers[i].l_marginh = 0;
       layers[i].l_marginv = 0;
       layers[i].l_active = 0;
       layers[i].l_alignment = PIDIP_ALIGNMENT_LEFT;
       layers[i].l_mode = PIDIP_TEXT_MODE_STATIC;
       layers[i].l_font = imlib_context_get_font();
       layers[i].l_feed_turn = 0;
       layers[i].l_upwards = 0;
       layers[i].l_ntexts = 0;
       layers[i].l_texts = NULL;
       layers[i].l_last_text = NULL;
    }

    if ( fnewsize < x->x_nbtexts )
    {
       post( "pdp_qtext : new size is too small : texts lost !!" );
       csize = fnewsize;
    }
    else
    {
       csize = x->x_nbtexts;
    }

    // copy all values
    for ( i=0; i<csize; i++ )
    {
       layers[i].l_r=x->x_layers[i].l_r;
       layers[i].l_g=x->x_layers[i].l_g;
       layers[i].l_b=x->x_layers[i].l_b;
       layers[i].l_a=x->x_layers[i].l_a;
       layers[i].l_2r=x->x_layers[i].l_2r;
       layers[i].l_2g=x->x_layers[i].l_2g;
       layers[i].l_2b=x->x_layers[i].l_2b;
       layers[i].l_2a=x->x_layers[i].l_2a;
       layers[i].l_borderr=x->x_layers[i].l_borderr;
       layers[i].l_borderg=x->x_layers[i].l_borderg;
       layers[i].l_borderb=x->x_layers[i].l_borderb;
       layers[i].l_xoffset=x->x_layers[i].l_xoffset;
       layers[i].l_yoffset=x->x_layers[i].l_yoffset;
       layers[i].l_marginh=x->x_layers[i].l_marginh;
       layers[i].l_marginv=x->x_layers[i].l_marginv;
       layers[i].l_active=x->x_layers[i].l_active;
       layers[i].l_alignment=x->x_layers[i].l_alignment;
       layers[i].l_mode=x->x_layers[i].l_mode;
       layers[i].l_font=x->x_layers[i].l_font;
       layers[i].l_feed_turn=x->x_layers[i].l_feed_turn;
       layers[i].l_upwards=x->x_layers[i].l_upwards;
       layers[i].l_ntexts=x->x_layers[i].l_ntexts;
       layers[i].l_texts=x->x_layers[i].l_texts;
       layers[i].l_last_text=x->x_layers[i].l_last_text;
    }
  
    // free old structures
    if ( x->x_layers ) freebytes( x->x_layers, x->x_capacity*sizeof(TEXTLAYER) );
    if ((int)fnewsize < x->x_capacity)
    {
	    // do something with left over texts XXX
    }

    // set new structures
    x->x_layers = layers;
    x->x_nbtexts = csize;
    x->x_capacity = (int) fnewsize;
    x->x_current = 0;
    post( "pdp_qtext : resized to %d", (int) fnewsize );
}

static void pdp_qtext_font(t_pdp_qtext *x, t_symbol *sfont  )
{
  Imlib_Font font;

    font = imlib_load_font(sfont->s_name);
    if ( !font )
    {
       pd_error( x, "[%s] error: could not load font: '%s'", 
                 class_getname(*(t_pd *)x), sfont->s_name );
       return;
    }
    imlib_context_set_font( font );
    x->x_layers[x->x_current].l_font = font;
}

static void pdp_qtext_allocate(t_pdp_qtext *x)
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

static void pdp_qtext_free_ressources(t_pdp_qtext *x)
{
   // if ( x->x_image != NULL ) imlib_free_image();
}

void qtext_line_draw(t_pdp_qtext *x,char *text,int text_width,int text_height,int tlayer, int ti, int linenumber,double stime)
{
   int base = tlayer;
   double newyoffset = 0;
   int yfinalposition;
   int alpha = x->x_layers[base].l_a;
   if (x->x_layers[base].l_mode == PIDIP_TEXT_MODE_SCROLL)
   {
	if ((x->x_layers[base].l_upwards))
	   newyoffset = -stime+x->x_vheight;
	else
   	    newyoffset = stime;
	if (stime<50)
	    alpha = (alpha*(stime/50));
	if (stime>350)
	    alpha = alpha-(alpha*((stime-350)/50));
   }
   yfinalposition = newyoffset + x->x_layers[base].l_yoffset + ((linenumber)*(text_height)) + (sin(x->x_layers[base].l_angle) * x->x_layers[base].l_scroll);
   /*if (ti>=x->x_chat_base)
	   base = x->x_chat_base;*/
   if (((text_height)*(linenumber+1)) + x->x_layers[base].l_yoffset + x->x_layers[base].l_marginv < x->x_vheight)		// CABE EN LOS  MARGENES ESPECIFICADOS (DE ALTO)
   {
       if ((x->x_layers[base].l_feed_turn+ti)%2&&(x->x_layers[base].l_mode==PIDIP_TEXT_MODE_FEED))
              imlib_context_set_color( x->x_layers[base].l_2r, x->x_layers[base].l_2g, x->x_layers[base].l_2b, x->x_layers[base].l_2a);
       else
              imlib_context_set_color( x->x_layers[base].l_r, x->x_layers[base].l_g, x->x_layers[base].l_b, alpha );
       if (x->x_layers[base].l_alignment == PIDIP_ALIGNMENT_CENTER)   
       {
       		imlib_text_draw( x->x_layers[base].l_xoffset - (0.5*text_width) + (cos(x->x_layers[base].l_angle) * x->x_layers[base].l_scroll), 
                        yfinalposition, 
                        text );
       }
       else		// left aligned text
       {
	       // CODE FOR A NOT SO NICE BORDER
              /*imlib_context_set_color( 0, 0, 0, 255 );
	      int borde = 2;
       		imlib_text_draw( x->x_xoffsets[base] + (cos(x->x_angle[base]) * x->x_scroll[base])+borde, 
                x->x_yoffsets[base] + (linenumber*(text_height)) + (sin(x->x_angle[base]) * x->x_scroll[base])-borde, 
                text );
       		imlib_text_draw( x->x_xoffsets[base] + (cos(x->x_angle[base]) * x->x_scroll[base])-borde, 
                x->x_yoffsets[base] + (linenumber*(text_height)) + (sin(x->x_angle[base]) * x->x_scroll[base])+borde, 
                text );
       		imlib_text_draw( x->x_xoffsets[base] + (cos(x->x_angle[base]) * x->x_scroll[base])-borde, 
                x->x_yoffsets[base] + (linenumber*(text_height)) + (sin(x->x_angle[base]) * x->x_scroll[base])-borde, 
                text );
       		imlib_text_draw( x->x_xoffsets[base] + (cos(x->x_angle[base]) * x->x_scroll[base])+borde, 
                x->x_yoffsets[base] + (linenumber*(text_height)) + (sin(x->x_angle[base]) * x->x_scroll[base])+borde, 
                text );*/
	  // SET ALTERNATING COLORS
       	  imlib_text_draw( x->x_layers[base].l_xoffset + (cos(x->x_layers[base].l_angle) * x->x_layers[base].l_scroll), 
                           yfinalposition,text);
       }
   }
}

// DRAW ALL TEXTS

static void pdp_qtext_draw_all_texts(t_pdp_qtext *x)
{
    int	text_width, text_height;
    int chat_lines = 0;        // total lines of chat rendered
    int linenumber;	// lines of chat we have rendered in current message
    int feed_mes = 0;	// messages rendered
    int     tlayer;

    // draw all texts
    imlib_context_set_direction(IMLIB_TEXT_TO_ANGLE);

    for (tlayer=0; tlayer<x->x_capacity; tlayer++)
    {
    TEXT_FRAME *curr_text = x->x_layers[tlayer].l_texts;
    while (curr_text != NULL) {
     if (x->x_layers[tlayer].l_active)
     {
       int base=tlayer;
       linenumber = 0;
       int ti = feed_mes;
       imlib_context_set_angle( x->x_layers[base].l_angle );
       imlib_context_set_font( x->x_layers[base].l_font );
       if (x->x_layers[base].l_mode == PIDIP_TEXT_MODE_FEED)
       {
	  if ((x->x_layers[base].l_feed_turn+ti)%2)
              imlib_context_set_color( x->x_layers[base].l_r, x->x_layers[base].l_g, x->x_layers[base].l_b, x->x_layers[base].l_a );
	  else
              imlib_context_set_color( x->x_layers[base].l_r, x->x_layers[base].l_g, x->x_layers[base].l_b, x->x_layers[base].l_a );
       }
       else
       {
              imlib_context_set_color( x->x_layers[base].l_r, x->x_layers[base].l_g, x->x_layers[base].l_b, x->x_layers[base].l_a );
       }

       imlib_context_set_font( x->x_layers[base].l_font );
       imlib_get_text_size( curr_text->text_array, &text_width, &text_height);

       // code which separates the message in lines
       if (text_width + x->x_layers[base].l_xoffset + x->x_layers[base].l_marginh > x->x_vwidth)
       {      // MESSAGE CAN'T BE DRAWN IN A LINE
	      char *token = NULL, *cp;
	      cp = strdup (curr_text->text_array);
	      if (x->x_layers[base].l_mode == PIDIP_TEXT_MODE_SLOW && curr_text->time/10<strlen(curr_text->text_array))
		      cp[(int)(curr_text->time/10)] = '\0';
	      token = strtok (cp, " .,;:");
	      char buf[256];
	      char *position = buf;
	      char prev[256];
	      int wordcount = 0;
	      while (token != NULL)
	      {
		strcpy(prev,buf);
		wordcount++;
		if (wordcount != 1)
			*(position++) = ' ';
	        position = stpcpy(position,token);
       		imlib_get_text_size( buf, &text_width, &text_height);
		if (text_width +  x->x_layers[base].l_xoffset + x->x_layers[base].l_marginh> x->x_vwidth)
		{	// YA TENEMOS UNA CADENA PARA LA 1 LINEA
			char * textofinal;
			if (wordcount == 1)
				textofinal = buf;
			else
				textofinal = prev;
       			imlib_get_text_size( textofinal, &text_width, &text_height);
	       			qtext_line_draw(x,textofinal, text_width,text_height,tlayer,feed_mes,chat_lines+linenumber,curr_text->time);
			if (wordcount == 1)
			{
				position = stpcpy(buf,"");
				wordcount=0;
			}
			else
			{
				position = stpcpy(buf,token);
				wordcount=1;
			}
			linenumber++;
		}
	      	token = strtok (NULL, " .,;:");
	      }
	      // draw the rests in the buffer
	      if (strlen(buf)>0)
	      {
	        imlib_get_text_size( buf, &text_width, &text_height);
	        qtext_line_draw(x,buf, text_width,text_height,tlayer,feed_mes,chat_lines+linenumber,curr_text->time);
                linenumber++;
	      }
	      free(cp);
       }
       else	// MESSAGE CAN BE DRAWN IN A LINE
       {
	       char *cp;
	      	cp = strdup (curr_text->text_array);
	      if (x->x_layers[base].l_mode == PIDIP_TEXT_MODE_SLOW && curr_text->time/10<strlen(curr_text->text_array))
		      cp[(int)(curr_text->time/10)] = '\0';
	       qtext_line_draw(x,cp, text_width,text_height,tlayer,feed_mes,chat_lines,curr_text->time);
	       linenumber=1;
	      free(cp);
       }
       // if we are in chat mode remember number of lines
       if (x->x_layers[base].l_mode == PIDIP_TEXT_MODE_FEED )
		chat_lines+=linenumber;
     }
     feed_mes++;
     curr_text->time++;
     if (curr_text->time > 400 && x->x_layers[tlayer].l_mode == PIDIP_TEXT_MODE_SCROLL && curr_text == x->x_layers[tlayer].l_texts)
     {
	 text_layer_delete_start(&x->x_layers[tlayer]);
     }
     curr_text = curr_text->next;
    } // while(curr_text != NULL);
    } // for (tlayer=0; tlayer<x->x_capacity; tlayer++)
}

static void pdp_qtext_process_yv12(t_pdp_qtext *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     ti;
    int     px, py;
    unsigned char y, u, v;
    DATA32    *imdata;
    DATA32    bgcolor=0;
    short int *pY, *pU, *pV;
    int	text_width, text_height;
    int chat_lines = 0;

    // ensure the prepared surface is the same size as the image
    if ( ( (int)(header->info.image.width) != x->x_vwidth ) ||
         ( (int)(header->info.image.height) != x->x_vheight ) )
    {
         pdp_qtext_free_ressources(x);
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         pdp_qtext_allocate(x);
    }

    // prepare packets
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    // prepare image
    if ( x->x_image != NULL ) imlib_context_set_image(x->x_image);
    imdata = imlib_image_get_data();

    // copy incoming packet to Imlib image
    yuv_Y122RGB( data, imdata, x->x_vwidth, x->x_vheight );

    // draw all texts to imlib surface
    pdp_qtext_draw_all_texts(x);
    
    // copy Imlib image to outgoing packet
    yuv_RGB2Y12( imdata, newdata, x->x_vwidth, x->x_vheight );

    return;
}

static void pdp_qtext_sendpacket(t_pdp_qtext *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_qtext_process(t_pdp_qtext *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_qtext_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_qtext_process_yv12, pdp_qtext_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;
	  default:
	    /* don't know the type, so dont pdp_qtext_process */
	    break;
	    
	}
    }

}

static void pdp_qtext_input_0(t_pdp_qtext *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
    {
	switch(pdp_packet_header((int)f)->info.image.encoding)
        {
	  case PDP_IMAGE_YV12:
          x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
	  break;
	}
    }
    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_qtext_process(x);

    }

}

static void pdp_qtext_free(t_pdp_qtext *x)
{
  int i;

    pdp_qtext_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_qtext_class;

void *pdp_qtext_new(void)
{
  int i;

   t_pdp_qtext *x = (t_pdp_qtext *)pd_new(pdp_qtext_class);
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("current"));
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textx"));
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("texty"));
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textr"));
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textg"));
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("textb"));
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("angle"));
   inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("scroll"));

   x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
   x->x_packet0 = -1;
   x->x_packet1 = -1;
   x->x_queue_id = -1;
   x->x_image = NULL;


   x->x_capacity = DEFAULT_CAPACITY;


   x->x_layers = (TEXTLAYER*) getbytes( x->x_capacity*sizeof(TEXTLAYER) );

   for ( i=0; i<x->x_capacity; i++ )
    {
       x->x_layers[i].l_r = x->x_layers[i].l_g = x->x_layers[i].l_b = x->x_layers[i].l_a = 255;
       x->x_layers[i].l_2r = x->x_layers[i].l_2g = x->x_layers[i].l_2b = x->x_layers[i].l_2a = 255;
       x->x_layers[i].l_borderr = x->x_layers[i].l_borderg = x->x_layers[i].l_borderb = 255;
       x->x_layers[i].l_xoffset = 0;
       x->x_layers[i].l_yoffset = 0;
       x->x_layers[i].l_marginh = 0;
       x->x_layers[i].l_scroll_speed = 1;
       x->x_layers[i].l_marginv = 0;
       x->x_layers[i].l_active = 0;
       x->x_layers[i].l_alignment = PIDIP_ALIGNMENT_LEFT;
       x->x_layers[i].l_mode = PIDIP_TEXT_MODE_STATIC;
       x->x_layers[i].l_font = imlib_context_get_font();
       x->x_layers[i].l_feed_turn = 0;
       x->x_layers[i].l_upwards = 0;
       x->x_layers[i].l_ntexts = 0;
       x->x_layers[i].l_texts = NULL;
       x->x_layers[i].l_last_text = NULL;
    }
    x->x_current = 0;
    x->x_nbtexts = 0;
    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_qtext_setup(void)
{
  Imlib_Font font;

    // post( pdp_qtext_version );
    pdp_qtext_class = class_new(gensym("pdp_qtext"), (t_newmethod)pdp_qtext_new,
    	(t_method)pdp_qtext_free, sizeof(t_pdp_qtext), 0, A_NULL);

    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_input_0, gensym("pdp"),  
                             A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_add, gensym("text"),  A_GIMME, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_current, gensym("current"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_textx, gensym("textx"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_texty, gensym("texty"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_textr, gensym("textr"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_textg, gensym("textg"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_textb, gensym("textb"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_texta, gensym("texta"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_text2r, gensym("text2r"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_text2g, gensym("text2g"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_text2b, gensym("text2b"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_text2a, gensym("text2a"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_borderr, gensym("borderr"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_borderg, gensym("borderg"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_borderb, gensym("borderb"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_clear, gensym("clear"),  A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_delete, gensym("delete"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_resize, gensym("resize"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_font, gensym("font"),  A_SYMBOL, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_angle, gensym("angle"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_scroll, gensym("scroll"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_left, gensym("left"),  A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_right, gensym("right"),  A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_center, gensym("center"),  A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_margin, gensym("margin"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_marginv, gensym("marginv"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_direction, gensym("direction"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_qtext_class, (t_method)pdp_qtext_mode, gensym("layermode"),  A_SYMBOL, A_NULL);

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
              post( "[pdp_qtext] error loading font : %s", fname );
           }
           else
           {
              post( "[pdp_qtext] using font : %s", fname );
              break;
           }
        }
    }
    imlib_context_set_font( font );
}

#ifdef __cplusplus
}
#endif
