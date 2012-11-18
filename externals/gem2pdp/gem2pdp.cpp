/*
 *  gem2pdp : gem to pdp bridge
 *
 *  Capture the contents of the Gem window and transform it to a PDP Packet whenever a bang is received
 *
 *  Copyright (c) 2003 Yves Degoyon
 *
 */


#include "gem2pdp.h"
#include "yuv.h"

#include "Gem/Manager.h"
#include "Gem/Cache.h"

#if defined(GEM_VERSION_MAJOR) && defined (GEM_VERSION_MINOR) && (GEM_VERSION_MAJOR>0 || GEM_VERSION_MINOR>=91)
# define GEM2PDP_LEGACY_GEM 0
#else
# define GEM2PDP_LEGACY_GEM 1
#endif


CPPEXTERN_NEW(gem2pdp)

gem2pdp :: gem2pdp(void)
{
#if GEM2PDP_LEGACY_GEM
  m_x = GemMan::m_xoffset;
  m_y = GemMan::m_yoffset;
  m_width = GemMan::m_width;
  m_height = GemMan::m_height;
#else
  GemMan::getOffset(&m_x, &m_y);
  GemMan::getDimen(&m_width, &m_height);
#endif
  m_image = NULL;
  m_pdpoutlet = outlet_new(this->x_obj, &s_anything);
}

gem2pdp :: ~gem2pdp()
{
  if (m_image) cleanImage();
}

void gem2pdp :: bangMess()
{
 t_int needNew=0, pbuffers;
 t_int psize, px, py;
 short int *pY, *pU, *pV;
 unsigned char r,g,b,a;
 t_int cpt;
 void *idontknowwhatitis=NULL;

  if ( !GemMan::windowExists() )
  {
    post("gem2pdp : no gem image to snap"); 
    return;
  }

  // update image dimensions
#if GEM2PDP_LEGACY_GEM
  m_x = GemMan::m_xoffset;
  m_y = GemMan::m_yoffset;
  m_width = GemMan::m_width;
  m_height = GemMan::m_height;
#else
  GemMan::getOffset(&m_x, &m_y);
  GemMan::getDimen(&m_width, &m_height);
#endif
  pbuffers = GemMan::m_buffer;
  // GemMan::m_buffer = 1;
  // GemMan::render(idontknowwhatitis);
  // post("gem2pdp : got dimensions : x=%d y=%d w=%d h=%d", m_x, m_y, m_width, m_height);
        
  if (m_width <= 0 || m_height <= 0)
  {
     post("gem2pdp : illegal size : x=%d y=%d w=%d h=%d", m_x, m_y, m_width, m_height);
     return;
   }
   if (m_image)
   {
      if (m_image->xsize != m_width ||
          m_image->ysize != m_height)
      {
         delete [] m_image->data;
         delete m_image;
         m_image = NULL;
         needNew = 1;
      }
   }
   else
   {
      needNew = 1;
   }
   if ( needNew )
   {
     m_image = new imageStruct;
     m_image->xsize = m_width;
     m_image->ysize = m_height;
     m_image->csize = 4;
     m_image->type  = GL_UNSIGNED_BYTE;
     m_image->format = GL_RGBA;
     m_image->data = new unsigned char[m_image->xsize * m_image->ysize * m_image->csize];
     post( "gem2pdp : allocated image : w=%d h=%d", m_image->xsize, m_image->ysize );
   }

   glReadPixels(m_x, m_y, m_image->xsize, m_image->ysize,
    	    	m_image->format, m_image->type, m_image->data);    
           
   // post( "gem2pdp : packet : w=%d h=%d", m_image->xsize, m_image->ysize );
   psize = m_image->xsize*m_image->ysize;
   m_packet0 = pdp_packet_new_image_YCrCb( m_image->xsize, m_image->ysize);
   m_header = pdp_packet_header(m_packet0);
   m_data = (short int *)pdp_packet_data(m_packet0);

   pY = m_data;
   pV = m_data+psize;
   pU = m_data+psize+(psize>>2);
  
   for ( py=0; py<m_image->ysize; py++)
   {
     for ( px=0; px<m_image->xsize; px++)
     {
       cpt=((m_image->ysize-py)*m_image->xsize+px)*4;
       r=m_image->data[cpt++];
       g=m_image->data[cpt++];
       b=m_image->data[cpt++];
       a=m_image->data[cpt];
       // if ( (r!=0) || (g!=0) || (b!=0) ) post( "gem2rgb : r=%d g=%d b=%d", r, g, b );
       *(pY) = yuv_RGBtoY( (r<<16) +  (g<<8) +  b ) << 7;
       *(pV) = ( yuv_RGBtoV( (r<<16) +  (g<<8) +  b ) - 128 ) << 8;
       *(pU) = ( yuv_RGBtoU( (r<<16) +  (g<<8) +  b ) - 128 ) << 8;
       pY++;
       if ( (px%2==0) && (py%2==0) )
       {
         pV++; pU++;
       }
     }
   }

   pdp_packet_pass_if_valid(m_pdpoutlet, &m_packet0);

   // restore buffer state
   // GemMan::m_buffer = pbuffers;
   // GemMan::render(idontknowwhatitis);

   // post("gem2pdp : read image"); 
}

void gem2pdp :: cleanImage()
{
  delete [] m_image->data;
  m_image->data = NULL;
}

void gem2pdp :: obj_setupCallback(t_class *classPtr)
{
  ::post( "gem2pdp : a bridge between GEM and PDP/PiDiP v"GEM2PDP_VERSION" (ydegoyon@free.fr)" );
  class_addmethod(classPtr, (t_method)&gem2pdp::bangMessCallback,
    	    gensym("bang"), A_NULL);
  class_sethelpsymbol( classPtr, gensym("gem2pdp.pd") );
}

void gem2pdp :: bangMessCallback(void *data)
{
  GetMyClass(data)->bangMess();
}

void gem2pdp :: render(GemState *state)
{
  GemMan::render(state);
  return;
}
