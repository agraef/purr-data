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

/*  This object lets you capture a portion of the screen
 *  and turn it into pdp packets
 *  ( inspired by ImageMagick code )
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <magick/methods.h>
#include <magick/api.h>
#include <magick/magick.h>
#include <magick/xwindow.h>

#define PDP_DISPLAY_LENGTH 1024

// trick to handle Image Magick compatibility
#if MagickLibVersion >= 0x618
#include "../include/xwindow-private.h"
#include "../include/quantum-private.h"
#endif

static char   *pdp_capture_version = "pdp_capture: version 0.1, capture of screen written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_capture_struct
{
    t_object x_obj;

    t_outlet *x_outlet0;
    int x_packet0;
    short int *x_data;
    t_pdp *x_header;
    int x_displayopen;

    char *x_display;
    int x_screen;
    int x_x;
    int x_y;
    int x_vwidth;
    int x_vheight;
    int x_vsize;

    Image *x_Ximage;
    Display *x_dpy;
} t_pdp_capture;

/***********************************************/
/* this code is borrowed from ImageMagick      */
/* but not exported, sorry, guys and girls,    */
/* i only need that                            */
/***********************************************/

static Window XMyGetSubwindow(t_pdp_capture *o, Display *display, Window window, int x, int y)
{
  Window source_window, target_window;
  int status, x_offset, y_offset;

  assert(display != (Display *) NULL);
  source_window=XRootWindow(display, o->x_screen);
  if (window == (Window) NULL)
  {
    return(source_window);
  }
  target_window=window;
  for ( ; ; )
  {
    status=XTranslateCoordinates(display,source_window,window,x,y, &x_offset,&y_offset,&target_window);
    if (status != 1)
    {
      break;
    }
    if (target_window == (Window) NULL)
    {
      break;
    }
    source_window=window;
    window=target_window;
    x=x_offset;
    y=y_offset;
  }
  if (target_window == (Window) NULL)
  {
    target_window=window;
  }
  return(target_window);
}

static Window XMyClientWindow(t_pdp_capture *o, Display *display,Window target_window)
{
  Atom state, type;  
  int format, status;
  unsigned char *data;
  unsigned long after, number_items;
  Window client_window;

  assert(display != (Display *) NULL);
  state=XInternAtom(display,"WM_STATE",1);
  if (state == (Atom) NULL)
  {
    return(target_window);
  }
  type=(Atom) NULL;
  status=XGetWindowProperty(display,target_window,state,0L,0L,0, (Atom) AnyPropertyType,&type,&format,&number_items,&after,&data);
  if ((status == Success) && (type != (Atom) NULL))
  {
    return(target_window);
  }
  client_window=XWindowByProperty(display,target_window,state);
  if (client_window == (Window) NULL)
  {
    return(target_window);
  }
  return(client_window);
}

static Image *XMyGetWindowImage(t_pdp_capture *o, Display *display,const Window window, const unsigned int level)
{
  typedef struct _ColormapInfo
  {
    Colormap colormap;
    XColor *colors;
    struct _ColormapInfo *next;
  } ColormapInfo;

  typedef struct _WindowInfo
  {
    Window window, parent;
    Visual *visual;
    Colormap colormap;
    XSegment bounds;
    RectangleInfo crop_info;
  } WindowInfo;

  IndexPacket index;
  int display_height, display_width, id, status, x_offset, y_offset;
  RectangleInfo crop_info;
  register IndexPacket *indexes;
  register int i;
  static ColormapInfo *colormap_info = (ColormapInfo *) NULL;
  static int max_windows = 0, number_windows = 0;
  static WindowInfo *window_info;
  Window child, root_window;
  XWindowAttributes window_attributes;

  assert(display != (Display *) NULL);
  status=XGetWindowAttributes(display,window,&window_attributes);
  if ((status == 0) || (window_attributes.map_state != IsViewable))
  {
    return((Image *) NULL);
  }
  root_window=XRootWindow(display, o->x_screen );
  (void) XTranslateCoordinates(display,window,root_window,0,0,&x_offset, &y_offset,&child);
  crop_info.x=x_offset;
  crop_info.y=y_offset;
  crop_info.width=window_attributes.width;
  crop_info.height=window_attributes.height;
  if (crop_info.x < 0)
  {
      crop_info.width+=crop_info.x;
      crop_info.x=0;
  }
  if (crop_info.y < 0)
  {
      crop_info.height+=crop_info.y;
      crop_info.y=0;
  }
  display_width=XDisplayWidth(display, o->x_screen);
  if ((int) (crop_info.x+crop_info.width) > display_width)
  {
    crop_info.width=display_width-crop_info.x;
  }
  display_height=XDisplayHeight(display, o->x_screen);
  if ((int) (crop_info.y+crop_info.height) > display_height)
  {
    crop_info.height=display_height-crop_info.y;
  }
  if (number_windows >= max_windows)
  {
      max_windows+=1024;
      if (window_info == (WindowInfo *) NULL)
      {
        window_info=(WindowInfo *) AcquireMemory(max_windows*sizeof(*window_info));
      }
      else
      {
        window_info=(WindowInfo *) ResizeMagickMemory(window_info,max_windows*sizeof(*window_info));
      }
  }
  if (window_info == (WindowInfo *) NULL)
  {
      post("pdp_capture : MemoryAllocationFailed : UnableToReadXImage");
      return((Image *) NULL);
  }
  id=number_windows++;
  window_info[id].window=window;
  window_info[id].visual=window_attributes.visual;
  window_info[id].colormap=window_attributes.colormap;
  window_info[id].bounds.x1=(short) crop_info.x;
  window_info[id].bounds.y1=(short) crop_info.y;
  window_info[id].bounds.x2=(short) (crop_info.x+(int) crop_info.width-1);
  window_info[id].bounds.y2=(short) (crop_info.y+(int) crop_info.height-1);
  crop_info.x-=x_offset;
  crop_info.y-=y_offset;

  window_info[id].crop_info=crop_info;
  if (level != 0)
  {
      unsigned int number_children;
      Window *children;

      status=XQueryTree(display,window,&root_window,&window_info[id].parent, &children,&number_children);
      for (i=0; i < id; i++)
      {
        if ((window_info[i].window == window_info[id].parent) &&
            (window_info[i].visual == window_info[id].visual) &&
            (window_info[i].colormap == window_info[id].colormap))
        {
          if ((window_info[id].bounds.x1 <= window_info[i].bounds.x1) ||
              (window_info[id].bounds.x1 >= window_info[i].bounds.x2) ||
              (window_info[id].bounds.y1 <= window_info[i].bounds.y1) ||
              (window_info[id].bounds.y1 >= window_info[i].bounds.y2))
          {
              number_windows--;
              break;
          }
        }
      }
      if ((status == 1) && (number_children != 0))
      {
          (void) XFree((void *) children);
      }
  }
  if (level <= 1)
  {
      ColormapInfo *next;
      Image *composite_image, *image;
      int y;
      register int j, x;
      register PixelPacket *q;
      register unsigned long pixel;
      unsigned int import, number_colors;
      XColor *colors;
      XImage *ximage;

      image=(Image *) NULL;
      for (id=0; id < number_windows; id++)
      {
        import=(window_info[id].bounds.x2 >= window_info[0].bounds.x1) &&
          (window_info[id].bounds.x1 <= window_info[0].bounds.x2) &&
          (window_info[id].bounds.y2 >= window_info[0].bounds.y1) &&
          (window_info[id].bounds.y1 <= window_info[0].bounds.y2);
        for (j=0; j < id; j++)
        {
          if ((window_info[id].visual == window_info[j].visual) &&
              (window_info[id].colormap == window_info[j].colormap) &&
              (window_info[id].bounds.x1 >= window_info[j].bounds.x1) &&
              (window_info[id].bounds.y1 >= window_info[j].bounds.y1) &&
              (window_info[id].bounds.x2 <= window_info[j].bounds.x2) &&
              (window_info[id].bounds.y2 <= window_info[j].bounds.y2))
          {
            import=0;
          }
          else
          {
            if ((window_info[id].visual != window_info[j].visual) ||
                (window_info[id].colormap != window_info[j].colormap))
            {
              if ((window_info[id].bounds.x2 > window_info[j].bounds.x1) &&
                  (window_info[id].bounds.x1 < window_info[j].bounds.x2) &&
                  (window_info[id].bounds.y2 > window_info[j].bounds.y1) &&
                  (window_info[id].bounds.y1 < window_info[j].bounds.y2))
              {
                import=1;
              }
            }
          }
        }
        if (!import)
        {
          continue;
        }
        // post( "pdp_capture : get image : %ld [%d,%d,%d,%d]", window_info[id].window, 
        //        (int) window_info[id].crop_info.x,
        //        (int) window_info[id].crop_info.y,
        //        (unsigned int) window_info[id].crop_info.width,
        //        (unsigned int) window_info[id].crop_info.height );
        ximage=XGetImage(display,window_info[id].window,
          (int) window_info[id].crop_info.x,(int) window_info[id].crop_info.y,
          (unsigned int) window_info[id].crop_info.width,
          (unsigned int) window_info[id].crop_info.height,AllPlanes,ZPixmap);
        if (ximage == (XImage *) NULL)
        {
          continue;
        }
        number_colors=0;
        colors=(XColor *) NULL;
        if (window_info[id].colormap != (Colormap) NULL)
        {
            ColormapInfo *p;

            number_colors=window_info[id].visual->map_entries;
            for (p=colormap_info; p != (ColormapInfo *) NULL; p=p->next)
            {
              if (p->colormap == window_info[id].colormap)
              {
                break;
              }
            }
            if (p == (ColormapInfo *) NULL)
            {
                colors=(XColor *) AcquireMemory(number_colors*sizeof(XColor));
                if (colors == (XColor *) NULL)
                {
                    XDestroyImage(ximage);
                    return((Image *) NULL);
                }
#if MagickLibVersion >= 0x608
                if ((window_info[id].visual->klass != DirectColor) &&
                    (window_info[id].visual->klass != TrueColor))
#else
                if ((window_info[id].visual->storage_class != DirectColor) &&
                    (window_info[id].visual->storage_class != TrueColor))
#endif
                {
                  for (i=0; i < (int) number_colors; i++)
                  {
                    colors[i].pixel=i;
                    colors[i].pad=0;
                  }
                }
                else
                {
                  unsigned long blue, blue_bit, green, green_bit, red, red_bit;

                    red=0;
                    green=0;
                    blue=0;
                    red_bit=window_info[id].visual->red_mask &
                      (~(window_info[id].visual->red_mask)+1);
                    green_bit=window_info[id].visual->green_mask &
                      (~(window_info[id].visual->green_mask)+1);
                    blue_bit=window_info[id].visual->blue_mask &
                      (~(window_info[id].visual->blue_mask)+1);
                    for (i=0; i < (int) number_colors; i++)
                    {
                      colors[i].pixel=red | green | blue;
                      colors[i].pad=0;
                      red+=red_bit;
                      if (red > window_info[id].visual->red_mask)
                        red=0;
                      green+=green_bit;
                      if (green > window_info[id].visual->green_mask)
                        green=0;
                      blue+=blue_bit;
                      if (blue > window_info[id].visual->blue_mask)
                        blue=0;
                    }
                }
                (void) XQueryColors(display,window_info[id].colormap,colors, (int) number_colors);
                p=(ColormapInfo *) AcquireMemory(sizeof(ColormapInfo));
                if (p == (ColormapInfo *) NULL)
                  return((Image *) NULL);
                p->colormap=window_info[id].colormap;
                p->colors=colors;
                p->next=colormap_info;
                colormap_info=p;
            }
            colors=p->colors;
        }
        composite_image=AllocateImage((ImageInfo *) NULL);
        if (composite_image == (Image *) NULL)
        {
            XDestroyImage(ximage);
            return((Image *) NULL);
        }
#if MagickLibVersion >= 0x608
        if ((window_info[id].visual->klass != DirectColor) &&
            (window_info[id].visual->klass != TrueColor))
#else
        if ((window_info[id].visual->storage_class != DirectColor) &&
            (window_info[id].visual->storage_class != TrueColor))
#endif
        {
          composite_image->storage_class=PseudoClass;
        }
        composite_image->columns=ximage->width;
        composite_image->rows=ximage->height;
        switch (composite_image->storage_class)
        {
          case DirectClass:
          default:
          {
            register unsigned long color, index;
            unsigned long blue_mask, blue_shift, green_mask, green_shift, red_mask, red_shift;

            red_mask=window_info[id].visual->red_mask;
            red_shift=0;
            while ((red_mask & 0x01) == 0)
            {
              red_mask>>=1;
              red_shift++;
            }
            green_mask=window_info[id].visual->green_mask;
            green_shift=0;
            while ((green_mask & 0x01) == 0)
            {
              green_mask>>=1;
              green_shift++;
            }
            blue_mask=window_info[id].visual->blue_mask;
            blue_shift=0;
            while ((blue_mask & 0x01) == 0)
            {
              blue_mask>>=1;
              blue_shift++;
            }
            if ((number_colors != 0) &&
#if MagickLibVersion >= 0x608
                (window_info[id].visual->klass == DirectColor))
#else
                (window_info[id].visual->storage_class == DirectColor))
#endif
            {
              for (y=0; y < (long) composite_image->rows; y++)
              {
                q=SetImagePixels(composite_image,0,y,
                  composite_image->columns,1);
                if (q == (PixelPacket *) NULL)
                  break;
                for (x=0; x < (long) composite_image->columns; x++)
                {
                  pixel=XGetPixel(ximage,x,y);
                  index=(pixel >> red_shift) & red_mask;
                  q->red=ScaleShortToQuantum(colors[index].red);
                  index=(pixel >> green_shift) & green_mask;
                  q->green=ScaleShortToQuantum(colors[index].green);
                  index=(pixel >> blue_shift) & blue_mask;
                  q->blue=ScaleShortToQuantum(colors[index].blue);
                  q++;
                }
                if (!SyncImagePixels(composite_image))
                  break;
              }
            }
            else
            {
              for (y=0; y < (long) composite_image->rows; y++)
              {
                q=SetImagePixels(composite_image,0,y,
                  composite_image->columns,1);
                if (q == (PixelPacket *) NULL)
                  break;
                for (x=0; x < (long) composite_image->columns; x++)
                {
                  pixel=XGetPixel(ximage,x,y);
                  color=(pixel >> red_shift) & red_mask;
                  q->red=ScaleShortToQuantum((65535L*color)/red_mask);
                  color=(pixel >> green_shift) & green_mask;
                  q->green=ScaleShortToQuantum((65535L*color)/green_mask);
                  color=(pixel >> blue_shift) & blue_mask;
                  q->blue=ScaleShortToQuantum((65535L*color)/blue_mask);
                  q++;
                }
                if (!SyncImagePixels(composite_image))
                {
                  break;
                }
              }
            }
            break;
          }
          case PseudoClass:
          {
            if (!AllocateImageColormap(composite_image,number_colors))
            {
                XDestroyImage(ximage);
                DestroyImage(composite_image);
                return((Image *) NULL);
            }
            for (i=0; i < (int) composite_image->colors; i++)
            {
              composite_image->colormap[colors[i].pixel].red=
                ScaleShortToQuantum(colors[i].red);
              composite_image->colormap[colors[i].pixel].green=
                ScaleShortToQuantum(colors[i].green);
              composite_image->colormap[colors[i].pixel].blue=
                ScaleShortToQuantum(colors[i].blue);
            }
            for (y=0; y < (long) composite_image->rows; y++)
            {
              q=SetImagePixels(composite_image,0,y,composite_image->columns,1);
              if (q == (PixelPacket *) NULL)
                break;
              indexes=GetIndexes(composite_image);
              for (x=0; x < (long) composite_image->columns; x++)
              {
                index=(IndexPacket) XGetPixel(ximage,x,y);
                indexes[x]=index;
                *q++=composite_image->colormap[index];
              }
              if (!SyncImagePixels(composite_image))
              {
                break;
              }
            }
            break;
          }
        }
        XDestroyImage(ximage);
        if (image == (Image *) NULL)
        {
            image=composite_image;
            continue;
        }
        (void) XTranslateCoordinates(display,window_info[id].window,window,0,0, &x_offset,&y_offset,&child);
        x_offset-=(int) crop_info.x;
        if (x_offset < 0)
        {
          x_offset=0;
        }
        y_offset-=(int) crop_info.y;
        if (y_offset < 0)
        {
          y_offset=0;
        }
        (void) CompositeImage(image,CopyCompositeOp,composite_image, x_offset,y_offset);
      }
      while (colormap_info != (ColormapInfo *) NULL)
      {
        next=colormap_info->next;
#if MagickLibVersion >= 0x608
        colormap_info->colors=(XColor *) RelinquishMagickMemory(colormap_info->colors);
        colormap_info=(ColormapInfo *) RelinquishMagickMemory(colormap_info);
#else
        LiberateMemory((void **) &colormap_info->colors);
        LiberateMemory((void **) &colormap_info);
#endif
        colormap_info=next;
      }
      /*
        Free resources and restore initial state.
      */
#if MagickLibVersion >= 0x608
      window_info=(WindowInfo *) RelinquishMagickMemory(window_info);
#else
      LiberateMemory((void **) &window_info);
#endif
      window_info=(WindowInfo *) NULL;
      max_windows=0;
      number_windows=0;
      colormap_info=(ColormapInfo *) NULL;
      return(image);
  }
  return((Image *) NULL);
}

/*************************************************/
/* this code is adapted from ImageMagick         */
/* mainly because i don't want user interactions */
/* and i want to chose the screen also           */
/*************************************************/

static void pdp_capture_do_capture(t_pdp_capture *x)
{
  Colormap *colormaps;
  Image *image;
  int number_colormaps, number_windows, status, X, Y;
  RectangleInfo crop_info;
  Window *children, client, prior_target, root, target; 
  XTextProperty window_name;
  Window child;
  XWindowAttributes window_attributes;

  /*
    Open X server connection.
  */
  if (!x->x_displayopen)
  {
      post( "pdp_capture : display not open : no capture" );
      return;
  }
  (void) XSetErrorHandler(XError);
  crop_info.x=x->x_x;
  crop_info.y=x->x_y;
  crop_info.width=x->x_vwidth;
  crop_info.height=x->x_vheight;
  root=XRootWindow(x->x_dpy, x->x_screen);
  target=(Window) NULL;
  prior_target=target;

  target = XMyGetSubwindow(x, x->x_dpy,root,x->x_x,x->x_y);

  client=target;   /* obsolete */
  if (target != root)
  {
      unsigned int d;
      status=XGetGeometry(x->x_dpy,target,&root,&X,&X,&d,&d,&d,&d);
      if (status != 0)
      {
          for ( ; ; )
          {
            Window parent;

            status=XQueryTree(x->x_dpy,target,&root,&parent,&children,&d);
            if (status && (children != (Window *) NULL))
            {
              (void) XFree((char *) children);
            }
            if (!status || (parent == (Window) NULL) || (parent == root)) break;
            target=parent;
          }
          client=XMyClientWindow(x, x->x_dpy, target);
          target=client;
          if (prior_target) target=prior_target;
      }
  }
  status=XGetWindowAttributes(x->x_dpy,target,&window_attributes);
  if (status == 0)
  {
     post( "pdp_capture : unable to read window attributes" );
     (void) XCloseDisplay(x->x_dpy);
     return;
  }
  (void) XTranslateCoordinates(x->x_dpy,target,root,0,0,&X,&Y,&child);
  crop_info.x=x->x_x;
  crop_info.y=x->x_y;
  crop_info.width=x->x_vwidth;
  crop_info.height=x->x_vheight;
  target=root;

  number_windows=0;
  status=XGetWMColormapWindows(x->x_dpy,target,&children,&number_windows);
  if ((status == 1) && (number_windows > 0))
  {
      (void) XFree ((char *) children);
  }
  colormaps=XListInstalledColormaps(x->x_dpy,target,&number_colormaps);
  if (number_colormaps > 0)
  {
      (void) XFree((char *) colormaps);
  }

  image=XMyGetWindowImage(x, x->x_dpy, target, 1);
  if (image == (Image *) NULL)
  {
    post( "pdp_capture : unable to read xwindow image" );
  }
  else
  {
     Image *clone_image;

      clone_image=CloneImage(image,0,0,1,&image->exception);
      if (clone_image != (Image *) NULL)
      {
         x->x_Ximage=CropImage(clone_image,&crop_info,&image->exception);
         if (x->x_Ximage != (Image *) NULL)
         {
             DestroyImage(image);
             DestroyImage(clone_image);
             image=x->x_Ximage;
         }
      }
      status=XGetWMName(x->x_dpy,target,&window_name);
  }
  return;
}

static void pdp_capture_display(t_pdp_capture *x, t_symbol *s)
{
    if ( x->x_displayopen ) 
    {
      if ( XCloseDisplay(x->x_dpy) == -1 )
      {
         post( "pdp_capture : could not close display" );
      }
    } 
    strcpy( x->x_display, s->s_name );
    if ( ( x->x_dpy = XOpenDisplay( x->x_display ) ) != NULL )
    {
       x->x_displayopen = 1;

       x->x_vwidth=XDisplayWidth(x->x_dpy, x->x_screen);
       x->x_vheight=XDisplayHeight(x->x_dpy, x->x_screen);
       x->x_vsize=x->x_vwidth*x->x_vheight;

    }
}

static void pdp_capture_screen(t_pdp_capture *x, t_floatarg fscreen)
{
   if ( (int)fscreen > 0 )
   {
      x->x_screen = (int) fscreen;
   }
}

static void pdp_capture_x(t_pdp_capture *x, t_floatarg fx)
{
  int width;
  int err;

   if (!x->x_displayopen)
   {
      post( "pdp_capture : display not open : not setting x" );
      return;
   }
   width = XDisplayWidth( x->x_dpy, x->x_screen );
   if ( ( (int)fx > 0 ) && ( (int)fx <= width ) )
   {
      x->x_x = (int) fx;
      if ( x->x_x + x->x_vwidth > width )
      {
         x->x_vwidth = width - x->x_x;
         x->x_vsize = x->x_vwidth * x->x_vheight;
      }
   }
   else
   {
      post( "pdp_capture : x position out of range : [0, %d]", width );
   }
}

static void pdp_capture_y(t_pdp_capture *x, t_floatarg fy)
{
  int height;
  int err;

   if (!x->x_displayopen)
   {
      post( "pdp_capture : display not open : not setting y" );
      return;
   }
   height = XDisplayHeight( x->x_dpy, x->x_screen );
   if ( ( (int)fy > 0 ) && ( (int)fy <= height ) )
   {
      x->x_y = (int) fy;
      if ( x->x_y + x->x_vheight > height )
      {
         x->x_vheight = height - x->x_y;
         x->x_vsize = x->x_vwidth * x->x_vheight;
      }
   }
   else
   {
      post( "pdp_capture : y position out of range : [0, %d]", height );
   }
}

static void pdp_capture_width(t_pdp_capture *x, t_floatarg fwidth)
{
  int width;
  int err;

   if (!x->x_displayopen)
   {
      post( "pdp_capture : display not open : not setting width" );
      return;
   }
   width = XDisplayWidth( x->x_dpy, x->x_screen );
   if ( ( (int)fwidth > 0 ) && ( (int)fwidth <= (width-x->x_x) ) )
   {
      x->x_vwidth = (int) fwidth;
      x->x_vsize = x->x_vwidth * x->x_vheight;
   }
   else
   {
      post( "pdp_capture : width out of range : [0, %d]", width-x->x_x );
   }
}

static void pdp_capture_height(t_pdp_capture *x, t_floatarg fheight)
{
  int height;
  int err;

   if (!x->x_displayopen)
   {
      post( "pdp_capture : display not open : not setting height" );
      return;
   }
   height = XDisplayWidth( x->x_dpy, x->x_screen );
   if ( ( (int)fheight > 0 ) && ( (int)fheight <= (height-x->x_y) ) )
   {
      x->x_vheight = (int) fheight;
      x->x_vsize = x->x_vwidth * x->x_vheight;
   }
   else
   {
      post( "pdp_capture : width out of range : [0, %d]", height-x->x_y );
   }
}

static void pdp_capture_sendpacket(t_pdp_capture *x)
{
    /* unregister and propagate if valid dest packet */
    if (x->x_packet0 != -1 )
    {
        pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet0);
    }
}

static void pdp_capture_bang(t_pdp_capture *x)
{
   PixelPacket pixel;
   short int *pY, *pU, *pV;
   unsigned char y, u, v;
   int px, py, r, g, b;
   long number_pixels;

    // capture the image and output a PDP packet
    pdp_capture_do_capture( x );

    x->x_vwidth = x->x_Ximage->columns;
    x->x_vheight = x->x_Ximage->rows;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    x->x_packet0 = pdp_packet_new_image_YCrCb( x->x_vwidth, x->x_vheight );
    x->x_data = (short int *)pdp_packet_data(x->x_packet0);
    x->x_header = pdp_packet_header(x->x_packet0);

    x->x_header->info.image.encoding = PDP_IMAGE_YV12;
    x->x_header->info.image.width = x->x_vwidth;
    x->x_header->info.image.height = x->x_vheight;

    number_pixels=(long) GetPixelCacheArea(x->x_Ximage);
    
    // post( "pdp_capture : capture done : w=%d h=%d pixels=%ld", x->x_vwidth, x->x_vheight, number_pixels );

    pY = x->x_data;
    pV = x->x_data+x->x_vsize;
    pU = x->x_data+x->x_vsize+(x->x_vsize>>2);
    for ( py=0; py<x->x_vheight; py++ )
    {
       for ( px=0; px<x->x_vwidth; px++ )
       {
          pixel = GetOnePixel(x->x_Ximage, px, py);
          // scale values
          r = (pixel.red*255)/65535;
          g = (pixel.green*255)/65535;
          b = (pixel.blue*255)/65535;
          // post( "pdp_capture : pixel : [%d, %d] : %d,%d,%d", px, py, r, g, b );
          y = yuv_RGBtoY(((r)<<16)+((g)<<8)+(b));
          u = yuv_RGBtoU(((r)<<16)+((g)<<8)+(b));
          v = yuv_RGBtoV(((r)<<16)+((g)<<8)+(b));

          *(pY) = y<<7;
          if ( (px%2==0) && (py%2==0) )
          {
              *(pV) = (v-128)<<8;
              *(pU) = (u-128)<<8;
          }
          pY++;
          if ( (px%2==0) && (py%2==0) )
          {
            pV++;pU++;
          }
       }
    }

    // output the new packet
    pdp_capture_sendpacket( x );

    DestroyImage( x->x_Ximage );
}

static void pdp_capture_free(t_pdp_capture *x)
{
  int i;

    if ( x->x_packet0 != -1 )
    {
      pdp_packet_mark_unused(x->x_packet0);
    }
    if ( x->x_displayopen ) 
    {
      if ( XCloseDisplay(x->x_dpy) == -1 )
      {
         post( "pdp_capture : could not close display" );
      }
    } 
}

t_class *pdp_capture_class;

void *pdp_capture_new(void)
{
    int i;

    t_pdp_capture *x = (t_pdp_capture *)pd_new(pdp_capture_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("x"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("y"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("width"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("height"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;

    x->x_display = (char *) malloc( PDP_DISPLAY_LENGTH );
    strcpy( x->x_display, ":0.0" );

    x->x_displayopen = 0;
    if ( ( x->x_dpy = XOpenDisplay( x->x_display ) ) != NULL )
    {
       x->x_displayopen = 1;

       x->x_vwidth=XDisplayWidth(x->x_dpy, x->x_screen);
       x->x_vheight=XDisplayWidth(x->x_dpy, x->x_screen);
       x->x_vsize=x->x_vwidth*x->x_vheight;

    }

    x->x_screen = 0;
    x->x_x = 0;
    x->x_y = 0;
    x->x_vwidth = 320;
    x->x_vheight = 240;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_capture_setup(void)
{
    // post( pdp_capture_version );
    pdp_capture_class = class_new(gensym("pdp_capture"), (t_newmethod)pdp_capture_new,
    	(t_method)pdp_capture_free, sizeof(t_pdp_capture), 0, A_NULL);

    class_addmethod(pdp_capture_class, (t_method)pdp_capture_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_capture_class, (t_method)pdp_capture_display, gensym("display"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_capture_class, (t_method)pdp_capture_screen, gensym("screen"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_capture_class, (t_method)pdp_capture_x, gensym("x"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_capture_class, (t_method)pdp_capture_y, gensym("y"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_capture_class, (t_method)pdp_capture_width, gensym("width"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_capture_class, (t_method)pdp_capture_height, gensym("height"), A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
