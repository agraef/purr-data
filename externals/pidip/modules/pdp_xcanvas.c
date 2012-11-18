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

/*  This object is a video canvas which is also handling the graphical operations
 *  ( no need to plug it in pdp_xv )
 *  Written by Yves Degoyon                                 
 */



#include "pdp.h"
#include "pdp_xwindow.h"
#include <X11/extensions/XShm.h>
#include <math.h>

static char   *pdp_xcanvas_version = "pdp_xcanvas: version 0.1, display for several video sources, written by Yves Degoyon (ydegoyon@free.fr)";

#define MAX_CANVAS_INPUT 10
#define PDP_XCANVAS_AUTOCREATE_RETRY 10

typedef struct pdp_xcanvas_struct
{
    t_object x_obj;
    t_float x_f;

    int x_dropped;
    int x_queue_id;

    int x_opacket;

    int x_current;
    t_float x_xmouse;
    t_float x_ymouse;

    int *x_packets;
    int *x_widths;
    int *x_heights;
    t_float *x_xoffsets;
    t_float *x_yoffsets;
    int *x_sizes;

    int x_owidth;
    int x_oheight;
    int x_osize;
    int x_nbinputs;

    // X data
    t_outlet      *x_events;
    t_pdp_xwindow x_xwin;
    t_pdp_xvideo  x_xvid;
    Display       *x_dpy;
    t_symbol      *x_display;
    int           x_initialized;

} t_pdp_xcanvas;

static Bool pdp_xcanvas_shm_completion_event(Display *dpy, XEvent *ev, XPointer arg)
{
  if(ev->type == XShmGetEventBase(dpy) + ShmCompletion ) {
    return True;
  } else {
    return False;
  }
}

static void pdp_canvas_create(t_pdp_xcanvas* x)
{
    int i;
    if(x->x_initialized) return;

    post("pdp_xcanvas: pdp_canvas_create");

    x->x_xwin.winwidth = x->x_owidth;
    x->x_xwin.winheight = x->x_oheight;
    x->x_xvid.width = x->x_owidth;
    x->x_xvid.height = x->x_oheight;

    /* manually open a display */
    if (NULL == (x->x_dpy = XOpenDisplay(x->x_display->s_name))){
        post("pdp_xv: cant open display %s\n",x->x_display->s_name);
        x->x_initialized = false;
        return;
    }

    post("pdp_xcanvas: pdp_xvideo_create_on_display");

    /* create a window on the display */
    if (!(x->x_initialized = pdp_xwindow_create_on_display(&x->x_xwin, x->x_dpy))) goto exit_close_win;

    post("pdp_xcanvas: pdp_xvideo_open_on_display");

    /* open an xv port on the display */
    if (!(x->x_initialized = pdp_xvideo_open_on_display(&x->x_xvid, x->x_dpy))) goto exit_close_dpy;

    x->x_xwin.initialized = True;

    /* done */
    return;

    /* cleanup exits */
 exit_close_win:
    pdp_xwindow_close(&x->x_xwin); // cose window
 exit_close_dpy:
    XCloseDisplay(x->x_dpy);       // close display
    x->x_dpy = 0;
}

static void pdp_xcanvas_destroy(t_pdp_xcanvas* x)
{
    if (x->x_initialized){
        pdp_xvideo_close(&x->x_xvid);      // close xvideo connection
        pdp_xwindow_close(&x->x_xwin);     // close the window
        XCloseDisplay(x->x_dpy);           // close the display connection
        x->x_dpy = 0;
        x->x_initialized = 0;
    }
}

static void pdp_xcanvas_cursor(t_pdp_xcanvas *x, t_floatarg f)
{
    pdp_xwindow_cursor(&x->x_xwin, f);
}

static void pdp_xcanvas_process_yv12(t_pdp_xcanvas *x)
{
  int     px, py, ppx, ppy, ii, nbs;
  char      *pY, *pU, *pV;
  char      *ppY, *ppU, *ppV;
  t_pdp     *oheader;
  char      *odata, *pdata;
  t_pdp     *iheader;
  int     mx, dx, my, dy;

  if ( !x->x_initialized )
  {
     pdp_canvas_create( x ) ;
  }

  pY = (char *)x->x_xvid.xshmimage->data;
  pV = (char *)(x->x_xvid.xshmimage->data+x->x_osize);
  pU = (char *)(x->x_xvid.xshmimage->data+x->x_osize+(x->x_osize>>2));

  // paint it black
  for ( py=0; py<x->x_oheight; py++)
  {
    memset( pY+(py*x->x_owidth), 0, x->x_owidth );
    memset( pU+((py>>1)*(x->x_owidth>>1)), -128, (x->x_owidth>>1) ); 
    memset( pV+((py>>1)*(x->x_owidth>>1)), -128, (x->x_owidth>>1) );
  }

  for ( ii=0; ii<x->x_nbinputs; ii++)
  {
    if ( x->x_packets[ii] != -1 )
    {
      if ( x->x_xoffsets[ii] < -x->x_widths[ii] ) continue; 
      if ( x->x_xoffsets[ii] > x->x_owidth ) continue; 
      if ( x->x_yoffsets[ii] < -x->x_heights[ii] ) continue; 
      if ( x->x_yoffsets[ii] > x->x_oheight ) continue; 

      pdata   = (char *)pdp_packet_data(x->x_packets[ii]);
      ppY = pdata;
      ppV = pdata+x->x_sizes[ii];
      ppU = pdata+x->x_sizes[ii]+(x->x_sizes[ii]>>2);

      if ( x->x_xoffsets[ii] < 0 ) 
      {
        mx = -x->x_xoffsets[ii];
        dx = x->x_widths[ii]+x->x_xoffsets[ii];
      }
      else if ( x->x_xoffsets[ii] > x->x_owidth - x->x_widths[ii] ) 
      {
        mx = 0;
        dx = x->x_owidth-x->x_xoffsets[ii];
      }
      else
      {
        mx = 0;
        dx = x->x_widths[ii];
      }

      if ( x->x_yoffsets[ii] < 0 ) 
      {
        my = -x->x_yoffsets[ii];
        dy = x->x_heights[ii]+x->x_yoffsets[ii]-1;
      }
      else if ( x->x_yoffsets[ii] > x->x_oheight - x->x_heights[ii] ) 
      {
        my = 0;
        dy = x->x_oheight-x->x_yoffsets[ii]-1;
      }
      else
      {
        my = 0;
        dy = x->x_heights[ii]-1;
      }

      for ( py=x->x_yoffsets[ii]+my; py<x->x_yoffsets[ii]+dy; py++)
      {
         memcpy( pY+(py*x->x_owidth)+(int)x->x_xoffsets[ii]+mx, 
                     ppY+(py-(int)x->x_yoffsets[ii])*x->x_widths[ii]+mx, dx );
         memcpy( pU+((py>>1)*(x->x_owidth>>1))+((int)(x->x_xoffsets[ii]+mx)>>1), 
                     ppU+((py-(int)x->x_yoffsets[ii])>>1)*(x->x_widths[ii]>>1)+(mx>>1), (dx>>1) );
         memcpy( pV+((py>>1)*(x->x_owidth>>1))+((int)(x->x_xoffsets[ii]+mx)>>1), 
                     ppV+((py-(int)x->x_yoffsets[ii])>>1)*(x->x_widths[ii]>>1)+(mx>>1), (dx>>1) );
      }
    }
  }
  return;
}

static void pdp_xcanvas_display_packet(t_pdp_xcanvas *x)
{
 XEvent    xevent;

  /* receive events & output them */
  pdp_xwindow_send_events(&x->x_xwin, x->x_events);

  /* display */
  XvShmPutImage(x->x_xvid.dpy, x->x_xvid.xv_port, x->x_xwin.win, x->x_xwin.gc, x->x_xvid.xshmimage,
                0, 0, x->x_xvid.width, x->x_xvid.height, 0, 0, 
                x->x_xwin.winwidth, x->x_xwin.winheight, True);
  XIfEvent(x->x_xvid.dpy, &xevent, pdp_xcanvas_shm_completion_event, NULL);
  XFlush(x->x_xvid.dpy);
}

static void pdp_xcanvas_process(t_pdp_xcanvas *x, int ni)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packets[ni]))
   	&& (PDP_BITMAP == header->type)){
    
	/* pdp_xcanvas_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packets[ni])->info.image.encoding){

	case PDP_BITMAP_YV12:
            pdp_queue_add(x, pdp_xcanvas_process_yv12, pdp_xcanvas_display_packet, &x->x_queue_id);
	    break;

	default:
	    post( "pdp_xcanvas : unknow image type : %d", 
                   pdp_packet_header(x->x_packets[ni])->info.image.encoding );
	    break;
	    
	}
    }
}

static void pdp_xcanvas_offset(t_pdp_xcanvas *x, t_floatarg ni, t_floatarg xoffset, t_floatarg yoffset)
{
  if ( ( ni < 1 ) || ( ni > x->x_nbinputs ) )
  {
     post( "pdp_xcanvas : offset : wrong source : %d : must be between 1 and %d", ni, x->x_nbinputs );
     return;
  }
  x->x_xoffsets[(int)ni-1] = xoffset;
  x->x_yoffsets[(int)ni-1] = yoffset;
}

static void pdp_xcanvas_select(t_pdp_xcanvas *x, t_floatarg X, t_floatarg Y)
{
 int ii;

  x->x_current = -1;
  X = X*x->x_owidth;
  Y = Y*x->x_oheight;
  // post( "pdp_xcanvas : select %f %f", X, Y );
  for ( ii=0; ii<x->x_nbinputs; ii++)
  {
    if ( x->x_packets[ii] != -1 )
    {
      if ( (X >= x->x_xoffsets[ii]) && ( X < x->x_xoffsets[ii] + x->x_widths[ii] )
           && (Y >= x->x_yoffsets[ii]) && ( Y < x->x_yoffsets[ii] + x->x_heights[ii] )
         ) 
      {
         x->x_current = ii;
         x->x_xmouse = X;
         x->x_ymouse = Y;
      }
    }
  }
}

static void pdp_xcanvas_drag(t_pdp_xcanvas *x, t_floatarg X, t_floatarg Y)
{
  X = X*x->x_owidth;
  Y = Y*x->x_oheight;
  // post( "pdp_xcanvas : drag %f %f", X, Y );
  if ( x->x_current != -1 )
  {
     x->x_xoffsets[ x->x_current ] += (X-x->x_xmouse);
     x->x_yoffsets[ x->x_current ] += (Y-x->x_ymouse);
     x->x_xmouse = X;
     x->x_ymouse = Y;
  }
}

static void pdp_xcanvas_unselect(t_pdp_xcanvas *x)
{
  x->x_current = -1;
}

static void pdp_xcanvas_input(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f, int ni)
{
  t_pdp     *header;
  short int *data;

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
    {
      /* release the packet */
      if ( x->x_packets[ni] != -1 )
      {
        pdp_packet_mark_unused(x->x_packets[ni]);
        x->x_packets[ni] = -1;
      }
      x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packets[ni], (int)f, pdp_gensym("bitmap/yv12/*") );
      if ( x->x_packets[ni] != -1 )
      {
        header = pdp_packet_header(x->x_packets[ni]);
        x->x_widths[ni] = header->info.image.width;
        x->x_heights[ni] = header->info.image.height;
        x->x_sizes[ni] = x->x_widths[ni]*x->x_heights[ni];
      }
    }

    if ((s == gensym("process")) && (-1 != x->x_packets[ni]) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_xcanvas_process(x, ni);
    }
}

static void pdp_xcanvas_input0(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 0);
}

static void pdp_xcanvas_input1(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 1);
}

static void pdp_xcanvas_input2(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 2);
}

static void pdp_xcanvas_input3(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 3);
}

static void pdp_xcanvas_input4(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 4);
}

static void pdp_xcanvas_input5(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 5);
}

static void pdp_xcanvas_input6(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 6);
}

static void pdp_xcanvas_input7(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 7);
}

static void pdp_xcanvas_input8(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 8);
}

static void pdp_xcanvas_input9(t_pdp_xcanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_xcanvas_input(x, s, f, 9);
}

static void pdp_xcanvas_free(t_pdp_xcanvas *x)
{
 int ii;

  pdp_queue_finish(x->x_queue_id);
  for ( ii=0; ii<x->x_nbinputs; ii++)
  {
    pdp_packet_mark_unused(x->x_packets[ii]);
  }
  pdp_packet_mark_unused(x->x_opacket);
  if ( x->x_packets ) freebytes( x->x_packets, x->x_nbinputs*sizeof(int) );
  if ( x->x_widths ) freebytes( x->x_widths, x->x_nbinputs*sizeof(int) );
  if ( x->x_heights ) freebytes( x->x_heights, x->x_nbinputs*sizeof(int) );
  if ( x->x_sizes ) freebytes( x->x_sizes, x->x_nbinputs*sizeof(int) );
  if ( x->x_xoffsets ) freebytes( x->x_xoffsets, x->x_nbinputs*sizeof(t_float) );
  if ( x->x_yoffsets ) freebytes( x->x_yoffsets, x->x_nbinputs*sizeof(t_float) );
  pdp_xcanvas_destroy(x);
  pdp_xvideo_free(&x->x_xvid);
  pdp_xwindow_free(&x->x_xwin);
}

t_class *pdp_xcanvas_class;

void *pdp_xcanvas_new(t_symbol *s, int argc, t_atom *argv)
{
  t_pdp_xcanvas *x = (t_pdp_xcanvas *)pd_new(pdp_xcanvas_class);
  int ii;
  char *imes[32];

  if ( argc != 3 )
  {
    post( "pdp_xcanvas : wrong constructor : pdp_xcanvas <width> <height> <nb inputs> (argc=%d)", argc);
    return NULL;
  }
  if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT ||
       argv[2].a_type != A_FLOAT )
  {
    post( "pdp_xcanvas : wrong constructor : pdp_xcanvas <width> <height> <nb inputs>");
    return NULL;
  }

  x->x_events = outlet_new(&x->x_obj, &s_anything);

  x->x_owidth = ( (int) argv[0].a_w.w_float / 8 ) * 8;  // round to a multiple of 8
  x->x_oheight = ( (int) argv[1].a_w.w_float / 8 ) * 8; // round to a multiple of 8
  x->x_osize = x->x_owidth*x->x_oheight;
  x->x_nbinputs = (int) argv[2].a_w.w_float;

  if ( x->x_owidth < 0 )
  {
    post( "pdp_xcanvas : wrong width : %d", x->x_owidth);
    return NULL;
  }
  if ( x->x_oheight < 0 )
  {
    post( "pdp_xcanvas : wrong height : %d", x->x_oheight);
    return NULL;
  }
  if ( x->x_nbinputs < 0 )
  {
    post( "pdp_xcanvas : wrong number of inputs : %d", x->x_nbinputs);
    return NULL;
  }
  if ( x->x_nbinputs > MAX_CANVAS_INPUT )
  {
    post( "pdp_xcanvas : number of inputs is too high : %d : only %d supported", 
          x->x_nbinputs, MAX_CANVAS_INPUT);
    return NULL;
  }

  post ( "pdp_xcanvas : new %dx%d canvas with %d inputs", x->x_owidth, x->x_oheight, x->x_nbinputs );

  x->x_packets = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_widths = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_heights = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_sizes = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_xoffsets = ( t_float* ) getbytes( x->x_nbinputs*sizeof(t_float) );
  x->x_yoffsets = ( t_float* ) getbytes( x->x_nbinputs*sizeof(t_float) );

  x->x_opacket = pdp_packet_new_image_YCrCb( x->x_owidth, x->x_oheight );

  for ( ii=0; ii<x->x_nbinputs; ii++)
  {
    sprintf( (char*)imes, "pdp%d", ii );
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"), gensym((char*)imes) );
    x->x_packets[ii] = -1;
    x->x_xoffsets[ii] = 0.;
    x->x_yoffsets[ii] = 0.;
  }
  x->x_current = -1;

  pdp_xwindow_init(&x->x_xwin);
  pdp_xvideo_init(&x->x_xvid);

  x->x_display = gensym(":0");
  x->x_dpy = 0;
  x->x_initialized = 0;

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_xcanvas_setup(void)
{
 char *imes[32];

  // post( pdp_xcanvas_version );
  pdp_xcanvas_class = class_new(gensym("pdp_xcanvas"), (t_newmethod)pdp_xcanvas_new,
    	(t_method)pdp_xcanvas_free, sizeof(t_pdp_xcanvas), 0, A_GIMME, A_NULL);


  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input0, gensym("pdp0"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input1, gensym("pdp1"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input2, gensym("pdp2"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input3, gensym("pdp3"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input4, gensym("pdp4"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input5, gensym("pdp5"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input6, gensym("pdp6"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input7, gensym("pdp7"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input8, gensym("pdp8"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_input9, gensym("pdp9"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_offset, gensym("offset"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_select, gensym("select"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_drag, gensym("drag"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_unselect, gensym("unselect"), A_NULL);
  class_addmethod(pdp_xcanvas_class, (t_method)pdp_xcanvas_cursor, gensym("cursor"), A_FLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
