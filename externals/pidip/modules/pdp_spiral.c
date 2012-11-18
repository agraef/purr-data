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

/*  This object is an adaptation of warp effect from effectv
 *  copyright (c) 2001 Sam Mertens.
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define PLANE_POWER         (4)     // 2 exp 4 = 16
#define WAVE_COUNT_POWER    (3)     // 2 exp 3 = 8
#define WAVE_LENGTH_POWER   (9)     // 2 exp 9 = 512

#define PLANES              (1 << PLANE_POWER)  // 16
#define PLANE_MASK          (PLANES - 1)
#define PLANE_MAX           (PLANES - 1)

#define WAVE_COUNT          (1 << WAVE_COUNT_POWER)   // 8
#define WAVE_MASK           (WAVE_COUNT - 1)
#define WAVE_MAX            (WAVE_COUNT - 1)

#define WAVE_LENGTH         (1 << WAVE_LENGTH_POWER) // 512
#define WAVE_LENGTH_MASK    (WAVE_LENGTH - 1)


#define WAVE_CONCENTRIC_A       0
#define WAVE_SAWTOOTH_UP        1
#define WAVE_SAWTOOTH_DOWN      2
#define WAVE_TRIANGLE           3

#define WAVE_SINUS              4
#define WAVE_CONCENTRIC_B       5
#define WAVE_LENS               6
#define WAVE_FLAT               7

/* The *_OFFSET predefines are just precalculations.  There shouldn't normally
** be any need to change them.
*/

#define WAVE_CONCENTRIC_A_OFFSET    (WAVE_CONCENTRIC_A * WAVE_LENGTH)
#define WAVE_SAW_UP_OFFSET          (WAVE_SAWTOOTH_UP * WAVE_LENGTH)
#define WAVE_SAW_DOWN_OFFSET        (WAVE_SAWTOOTH_DOWN * WAVE_LENGTH)
#define WAVE_TRIANGLE_OFFSET        (WAVE_TRIANGLE * WAVE_LENGTH)

#define WAVE_CONCENTRIC_B_OFFSET    (WAVE_CONCENTRIC_B * WAVE_LENGTH)
#define WAVE_LENS_OFFSET            (WAVE_LENS * WAVE_LENGTH)
#define WAVE_SINUS_OFFSET           (WAVE_SINUS * WAVE_LENGTH)
#define WAVE_FLAT_OFFSET            (WAVE_FLAT * WAVE_LENGTH)

#define WAVE_ELEMENT_SIZE       (sizeof(char))
#define WAVE_TABLE_SIZE         (WAVE_COUNT * WAVE_LENGTH * WAVE_ELEMENT_SIZE)

#define FOCUS_INCREMENT_PRESET  (M_PI/2.0)

static char   *pdp_spiral_version = "pdp_spiral: version 0.1, port of spiral from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

static char* the_wave_table = NULL;

typedef struct pdp_spiral_struct
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
    short int *x_planetable[PLANES];
    int x_plane;
    int *x_depthmap;
    int x_mode;
    int x_focus_x;
    int x_focus_y;
    int x_cursor_state;
    int x_cursor_local;
    int x_toggle_xor;
    int x_animate_focus;
    int x_focus_interval;
    int x_focus_counter;
    unsigned int x_depth_shift; // Cheesy way to adjust intensity
    int  x_focus_radius;
    double x_focus_degree;
    double x_focus_increment;

} t_pdp_spiral;

static void pdp_spiral_define_waves(t_pdp_spiral *x) 
{
  int   i, w, iw;
  double  sinus_val = M_PI/2.0;

  if (NULL == the_wave_table) return;

  w = ((int)sqrt(x->x_vheight * x->x_vheight + x->x_vwidth * x->x_vwidth));
  for (i=0; i<WAVE_LENGTH; i++)
  {
    the_wave_table[WAVE_FLAT_OFFSET + i] = 0;

    the_wave_table[WAVE_SAW_UP_OFFSET + i] = i & PLANE_MASK;
    the_wave_table[WAVE_SAW_DOWN_OFFSET + i] = PLANE_MAX - (i & PLANE_MASK);
    if (i & PLANES)
    {
       the_wave_table[WAVE_TRIANGLE_OFFSET + i] = (~i) & PLANE_MASK;
    }
    else
    {
       the_wave_table[WAVE_TRIANGLE_OFFSET + i] = i & PLANE_MASK;
    }

    iw = i / (w/(PLANES*2));

    if (iw & PLANES)
    {
       the_wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = (~iw) & PLANE_MASK;
    }
    else
    {
       the_wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = iw & PLANE_MASK;
    }

    the_wave_table[WAVE_CONCENTRIC_B_OFFSET + i] = (i*PLANES)/w;
    the_wave_table[WAVE_LENS_OFFSET + i] = i >> 3;
    the_wave_table[WAVE_SINUS_OFFSET + i] = ((PLANES/2) +
               (int)((PLANES/2 - 1) * sin(sinus_val))) & PLANE_MASK;
    sinus_val += M_PI/PLANES;
  }

}

void pdp_spiral_create_map(t_pdp_spiral *x)
{
    int px, py, rel_x, rel_y, yy;
    float x_ratio;
    float y_ratio;
    int v, i, wave_offset;

    if ( x->x_vsize == -1 ) 
    {
       post( "pdp_spiral : create_map : no video data" );
       return;
    }

    /*
    ** The following code generates the default depth map.
    */
    i = 0;
    wave_offset = x->x_mode * WAVE_LENGTH;

    x_ratio = 320.0 / x->x_vwidth;
    y_ratio = 240.0 / x->x_vheight;

    for (py=0; py<x->x_vheight; py++) 
    {
        rel_y = (x->x_focus_y - py) * y_ratio;
        yy = rel_y * rel_y;

        for(px=0; px<x->x_vwidth; px++) 
        {
            rel_x = (x->x_focus_x - px) * x_ratio;
            v = ((int)sqrt(yy + rel_x*rel_x)) & WAVE_LENGTH_MASK;
            x->x_depthmap[i++] = the_wave_table[wave_offset + v] >> x->x_depth_shift;
        }
    }

    return;
}

static void pdp_spiral_mode(t_pdp_spiral *x, t_floatarg fmode )
{
   if ( ( fmode > 0 ) || ( fmode < WAVE_COUNT ) )
   {
      x->x_mode = (int)fmode;
      pdp_spiral_create_map(x);
   }
}

static void pdp_spiral_focus_x(t_pdp_spiral *x, t_floatarg ffocusx )
{
   if ( ( ffocusx > 0 ) || ( ffocusx < x->x_vwidth ) )
   {
      x->x_focus_x = (int)ffocusx;
      pdp_spiral_create_map(x);
   }
}

static void pdp_spiral_focus_y(t_pdp_spiral *x, t_floatarg ffocusy )
{
   if ( ( ffocusy > 0 ) || ( ffocusy < x->x_vwidth ) )
   {
      x->x_focus_y = (int)ffocusy;
      pdp_spiral_create_map(x);
   }
}

static void pdp_spiral_depth_shift(t_pdp_spiral *x, t_floatarg fdepthshift )
{
   if ( ( fdepthshift > 0 ) || ( fdepthshift < 5 ) )
   {
      x->x_depth_shift = (int)fdepthshift;
      pdp_spiral_create_map(x);
   }
}

static void pdp_spiral_focus_interval(t_pdp_spiral *x, t_floatarg finterval )
{
   if ( ( finterval > 0 ) || ( finterval < 60 ) )
   {
      x->x_focus_interval = (int)finterval;
   }
}

static void pdp_spiral_focus_increment(t_pdp_spiral *x, t_floatarg fincrement )
{
   if ( fincrement > 0 )
   {
      x->x_focus_increment = (int)fincrement;
   }
}

static void pdp_spiral_toggle_xor(t_pdp_spiral *x, t_floatarg fxor )
{
   if ( ( fxor ==0 ) || ( fxor == 1 ) )
   {
      x->x_toggle_xor = (int)fxor;
   }
}

static void pdp_spiral_animate_focus(t_pdp_spiral *x, t_floatarg fafocus )
{
    if ( ( fafocus ==0 ) || ( fafocus == 1 ) )
    {
      x->x_animate_focus = (int)fafocus;
    }
}

static void pdp_spiral_free_ressources(t_pdp_spiral *x)
{
    if ( the_wave_table ) free ( the_wave_table );
    if ( x->x_buffer ) free ( x->x_buffer );
    if ( x->x_depthmap ) free (  x->x_depthmap );
}

static void pdp_spiral_allocate(t_pdp_spiral *x)
{
  int i;

    the_wave_table = (char*) malloc (WAVE_TABLE_SIZE);
    x->x_focus_radius = x->x_vwidth / 2;

    // allocate space for the frame buffers.  A lot of memory is required -
    // with the default settings, it totals nearly 5 megs.
    x->x_buffer = (short int *) malloc ( ( ( x->x_vsize + x->x_vsize>>1 ) << 1 ) * 2 * PLANES);

    // set up the array of pointers to the frame buffers
    for(i=0;i<PLANES;i++) 
    {
       x->x_planetable[i] = &x->x_buffer[ ( ( x->x_vsize + x->x_vsize>>1 ) << 1 ) * i];
    }

    x->x_depthmap = (int*) malloc (  x->x_vsize * sizeof ( int ) );

    if ( !the_wave_table || !x->x_buffer || !x->x_depthmap )
    {
       post( "pdp_spiral : severe error : cannot allocate buffers !!! ");
       return;
    }
}

static void pdp_spiral_move_focus(t_pdp_spiral *x)
{
    x->x_focus_counter++;
    //  We'll only switch maps every X frames.
    if (x->x_focus_interval <= x->x_focus_counter)
    {
      x->x_focus_counter = 0;
      x->x_focus_x = (x->x_focus_radius * cos(x->x_focus_degree)) + (x->x_vwidth/2);
      x->x_focus_y = (x->x_focus_radius * sin(x->x_focus_degree*2.0)) + (x->x_vheight/2);
      pdp_spiral_create_map(x);
      x->x_focus_degree += x->x_focus_increment;
      if ((2.0*M_PI) <= x->x_focus_degree)
      {
         x->x_focus_degree -= (2.0*M_PI);
      }
    }
}

static void pdp_spiral_process_yv12(t_pdp_spiral *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i, iu;

    unsigned int totalnbpixels;
    unsigned int u_offset;
    unsigned int v_offset;
    unsigned int totnbpixels;

    int px, py;
    int cf;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_spiral_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        x->x_focus_x = (x->x_vwidth/2);
        x->x_focus_y = (x->x_vheight/2);
        x->x_plane = PLANE_MAX;
        pdp_spiral_allocate(x);
        post( "pdp_spiral : reallocated buffers" );
        pdp_spiral_define_waves( x );
        pdp_spiral_create_map( x );
        post( "pdp_spiral : set wave table" );
    }

    totalnbpixels = x->x_vsize;
    u_offset = x->x_vsize;
    v_offset = x->x_vsize + (x->x_vsize>>2);
    totnbpixels = x->x_vsize + (x->x_vsize>>1);

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    // post( "pdp_spiral : buffer=%x limit=%x dest=%x size=%d data=%x plane=%d", 
    //       x->x_buffer, x->x_buffer + ( ( x->x_vsize + x->x_vsize>>1 ) << 1 ) * PLANES - 1, 
    //       x->x_planetable[x->x_plane], ( ( x->x_vsize + x->x_vsize>>1) << 1 ), data, x->x_plane );
    memcpy( x->x_planetable[x->x_plane], data,  ( ( x->x_vsize + x->x_vsize>>1 ) << 1 ) );

    if (x->x_animate_focus)
    {
        pdp_spiral_move_focus(x);
    }

    i = 0;
    iu = 0;
    for(py = 0; py < x->x_vheight; py++) 
    {
      for(px = 0; px < x->x_vwidth; px++) 
      {
        cf = (x->x_plane + x->x_depthmap[i]) & PLANE_MASK;
        newdata[i] = (x->x_planetable[cf])[i];
        // u & v are untouched
        newdata[x->x_vsize+iu] = data[x->x_vsize+iu];
        newdata[x->x_vsize+(x->x_vsize>>2)+iu] = data[x->x_vsize+(x->x_vsize>>2)+iu];
        i++;
        if ( (px%2==0) && (py%2==0) )
        {
           iu++;
        }
      }
    }

    x->x_plane--;
    x->x_plane &= PLANE_MASK;

    return;
}

static void pdp_spiral_sendpacket(t_pdp_spiral *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_spiral_process(t_pdp_spiral *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_spiral_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_spiral_process_yv12, pdp_spiral_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_spiral_process */
	    break;
	    
	}
    }
}

static void pdp_spiral_input_0(t_pdp_spiral *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_spiral_process(x);
    }
}

static void pdp_spiral_free(t_pdp_spiral *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_spiral_free_ressources(x);
}

t_class *pdp_spiral_class;

void *pdp_spiral_new(void)
{
    int i;

    t_pdp_spiral *x = (t_pdp_spiral *)pd_new(pdp_spiral_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("mode"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("focus_x"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("focus_y"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("depth_shift"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("focus_interval"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("focus_increment"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("toggle_xor"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("animate_focus"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_mode = 0;
    x->x_cursor_state = 0;
    x->x_cursor_local = 0;
    x->x_toggle_xor = 0;
    x->x_animate_focus = 0;
    x->x_focus_interval = 6;
    x->x_focus_counter = 0;
    x->x_depth_shift = 0; // Cheesy way to adjust intensity
    x->x_focus_radius = 100;
    x->x_focus_degree = 1.0;
    x->x_focus_increment = FOCUS_INCREMENT_PRESET;
    x->x_buffer = NULL;
    x->x_depthmap = NULL;
    the_wave_table = NULL;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_spiral_setup(void)
{
//    post( pdp_spiral_version );
    pdp_spiral_class = class_new(gensym("pdp_spiral"), (t_newmethod)pdp_spiral_new,
    	(t_method)pdp_spiral_free, sizeof(t_pdp_spiral), 0, A_NULL);

    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_mode, gensym("mode"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_focus_x, gensym("focus_x"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_focus_y, gensym("focus_y"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_depth_shift, gensym("depth_shift"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_depth_shift, gensym("focus_interval"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_depth_shift, gensym("focus_increment"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_depth_shift, gensym("toggle_xor"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spiral_class, (t_method)pdp_spiral_depth_shift, gensym("animate_focus"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
