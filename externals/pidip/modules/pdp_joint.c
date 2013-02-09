/*
 *   PiDiP module
 *   Copyright (c) by Yves Degoyon ( ydegoyon@free.fr )
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

/* This object is an special points detector in a skeleton
 * it only works with a binary skeleton image.
 *
 */

#include "pdp.h"
#include <math.h>
#include <stdio.h>

#ifndef _EiC
#include <opencv/cv.h>
#endif

static char   *pdp_joint_version = "pdp_joint: a skeleton joints detector object version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

#define MAXPOINTS 100

//
// according to the 8 connected pixels,
// gives the point type :
//
// 0 = regular point in a line -- not specific
// 1 = ending point
// 2 = inflection point
// 3 = jonction point
// 255 = theoretically non existing in a well formed skeleton
//

const unsigned char point_type[256] = {
  255,1,1,2,1,2,2,3,
  1,2,2,3,2,3,2,3,
  1,2,2,2,2,3,3,3,
  0,3,3,3,2,2,3,3,
  1,2,2,3,0,3,3,3,
  2,3,2,2,2,3,2,255,
  2,3,3,3,3,3,255,255, // *48
  3,3,3,255,2,255,255,255,
  1,2,0,2,2,3,2,3,     // *64
  2,2,3,2,3,255,3,255,
  2,3,3,3,2,3,255,255, // *80
  3,3,3,255,3,3,255,255,
  1,2,2,2,2,3,2,3,     // *96
  255,255,255,255,255,255,255,255,
  2,3,3,3,2,3,255,255, // *112
  255,255,255,255,255,255,255,255,
  1,0,2,2,2,3,3,3,     // *128
  2,2,3,255,3,3,3,255,
  1,2,2,2,3,3,255,255, // *144
  2,3,3,255,3,3,255,255,
  3,3,3,3,3,3,3,3,
  2,3,3,255,3,3,3,255, // *168
  2,3,3,3,3,3,3,255,
  2,255,3,255,3,3,255,255, // *184
  1,2,2,2,2,3,2,3,
  2,2,3,255,3,3,3,255,
  1,2,2,255,255,3,255,255,
  255,255,255,255,255,255,255,255,
  2,3,3,3,3,3,3,3,    // *208
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255
};

typedef struct
{
  short int x;
  short int y;
  char  state;  // -1:free 0:to be found 1:found
} t_pointloc;

typedef struct pdp_joint_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_packet2;
    int x_queue_id;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    t_atom  plist[3];

    int x_mmove;

    t_pointloc x_newepoints[MAXPOINTS];
    t_pointloc x_cstepoints[MAXPOINTS];

    t_pointloc x_newjpoints[MAXPOINTS];
    t_pointloc x_cstjpoints[MAXPOINTS];

    t_pointloc x_newipoints[3*MAXPOINTS];
    t_pointloc x_cstipoints[3*MAXPOINTS];

    t_outlet *x_outlet0; // output coloured skeleton
    t_outlet *x_outlet1; // output ending points
    t_outlet *x_outlet2; // output junction points
    t_outlet *x_outlet3; // output inflection points

    // OpenCV image
    IplImage *image;
    CvFont font;

} t_pdp_joint;

static void pdp_joint_maxmove(t_pdp_joint *x, t_floatarg fmove )
{
   if ( ( (int)fmove<0 ) || ( (int)fmove>=((x->x_vwidth>x->x_vheight)?x->x_vheight:x->x_vwidth) ) )
   {
      return;
   }
   x->x_mmove = (int)fmove;
}

static int pdp_joint_register_epoint(t_pdp_joint *x, t_floatarg fx, t_floatarg fy )
{
  int i;

    if ( ( fx < 0.0 ) || ( fx > x->x_vwidth ) || ( fy < 0 ) || ( fy > x->x_vheight ) )
    {
       return -1;
    }

    for ( i=0; i<MAXPOINTS; i++)
    {
       if ( x->x_cstepoints[i].state == -1 )
       {
          x->x_cstepoints[i].x = (short int)fx;
          x->x_cstepoints[i].y = (short int)fy;
          x->x_cstepoints[i].state = 1;
          return i;
       }
    }

    // post( "pdp_joint : max markers reached" );
    return -1;
}

static int pdp_joint_register_jpoint(t_pdp_joint *x, t_floatarg fx, t_floatarg fy )
{
  int i;

    if ( ( fx < 0.0 ) || ( fx > x->x_vwidth ) || ( fy < 0 ) || ( fy > x->x_vheight ) )
    {
       return -1;
    }

    for ( i=0; i<MAXPOINTS; i++)
    {
       if ( x->x_cstjpoints[i].state == -1 )
       {
          x->x_cstjpoints[i].x = (short int)fx;
          x->x_cstjpoints[i].y = (short int)fy;
          x->x_cstjpoints[i].state = 1;
          return i;
       }
    }

    // post( "pdp_joint : max markers reached" );
    return -1;
}

static int pdp_joint_register_ipoint(t_pdp_joint *x, t_floatarg fx, t_floatarg fy )
{
  int i;

    if ( ( fx < 0.0 ) || ( fx > x->x_vwidth ) || ( fy < 0 ) || ( fy > x->x_vheight ) )
    {
       return -1;
    }

    for ( i=0; i<3*MAXPOINTS; i++)
    {
       if ( x->x_cstipoints[i].state == -1 )
       {
          x->x_cstipoints[i].x = (short int)fx;
          x->x_cstipoints[i].y = (short int)fy;
          x->x_cstipoints[i].state = 1;
          return i;
       }
    }

    // post( "pdp_joint : max markers reached" );
    return -1;
}

static void pdp_joint_clear(t_pdp_joint *x)
{
  int j;

    for (j=0;j<MAXPOINTS;j++)
    {
       x->x_cstepoints[j].state=-1;
       x->x_newepoints[j].state=-1;
       x->x_cstjpoints[j].state=-1;
       x->x_newjpoints[j].state=-1;
    }
    for (j=0;j<3*MAXPOINTS;j++)
    {
       x->x_cstipoints[j].state=-1;
       x->x_newipoints[j].state=-1;
    }
}

static void pdp_joint_process_grey(t_pdp_joint *x)
{
  t_pdp         *header = pdp_packet_header(x->x_packet1);
  unsigned char *data = (unsigned char*)pdp_packet_data(x->x_packet1);
  t_pdp         *newheader;
  unsigned char *newdata;
  int ix, iy;
  int i, j;
  int jmask;
  int nbepoints;
  int nbjpoints;
  int nbipoints;
  CvScalar pixel;
  char tindex[4];

  /* allocate all ressources */
  if ( ( (int)header->info.image.width != x->x_vwidth ) ||
       ( (int)header->info.image.height != x->x_vheight ) )
  {
      x->x_vwidth = header->info.image.width;
      x->x_vheight = header->info.image.height;
      x->x_vsize = x->x_vwidth*x->x_vheight;

      cvReleaseImage(&x->image);
      x->image = cvCreateImage(cvSize(x->x_vwidth,x->x_vheight), IPL_DEPTH_8U, 3);
  }

  x->x_packet2 = pdp_packet_new_bitmap_rgb( x->x_vwidth, x->x_vheight );
  newheader = pdp_packet_header(x->x_packet2);
  newdata = (unsigned char*)pdp_packet_data(x->x_packet2);

  cvZero( x->image );

  // for storing new points
  for (j=0;j<MAXPOINTS;j++)
  {
     x->x_newepoints[j].state=-1; // no point
     if ( x->x_cstepoints[j].state == 1 )
     {
       x->x_cstepoints[j].state=0;  // to be found
     }
     x->x_newjpoints[j].state=-1; // no point
     if ( x->x_cstjpoints[j].state == 1 )
     {
       x->x_cstjpoints[j].state=0;  // to be found
     }
     x->x_newipoints[j].state=-1; // no point
     if ( x->x_cstipoints[j].state == 1 )
     {
       x->x_cstipoints[j].state=0;  // to be found
     }
  }
  nbepoints=0;
  nbjpoints=0;
  nbipoints=0;

  // don't treat borders, resize your frame to (+8,+8) to treat them if you like
  for ( iy=1; iy<x->x_vheight-1; iy++ )
  {
    for ( ix=1; ix<x->x_vwidth-1; ix++ )
    {
       jmask=(data[(iy-1)*x->x_vwidth+(ix-1)]?1:0) + (data[(iy-1)*x->x_vwidth+ix]?2:0) + (data[(iy-1)*x->x_vwidth+(ix+1)]?4:0) +
             (data[iy*x->x_vwidth+(ix-1)]?8:0) + (data[iy*x->x_vwidth+(ix+1)]?16:0) + 
             (data[(iy+1)*x->x_vwidth+(ix-1)]?32:0) + (data[(iy+1)*x->x_vwidth+ix]?64:0) + (data[(iy+1)*x->x_vwidth+(ix+1)]?128:0); 

       if ( (jmask<0) || (jmask>255) )
       {
          post( "pdp_joint : wrong mask ( weird) : %d", jmask );
          continue;
       }

       if ( ( point_type[jmask] == 0 ) && ( data[iy*x->x_vwidth+ix]>0 ) )
       {
          // white
          pixel.val[0]=255;
          pixel.val[1]=255;
          pixel.val[2]=255;
          cvSet2D(x->image, iy, ix, pixel);  
       }

       if ( ( point_type[jmask] == 1 ) && ( data[iy*x->x_vwidth+ix]>0 ) ) // extremity
       {
          // green
          x->x_newepoints[nbepoints].state=0;
          x->x_newepoints[nbepoints].x=ix;
          x->x_newepoints[nbepoints].y=iy;
          nbepoints++;
       }

       if ( ( point_type[jmask] == 2 ) && ( data[iy*x->x_vwidth+ix]>0 ) ) // inflection point
       {
          // yellow
          pixel.val[0]=255;
          pixel.val[1]=255;
          pixel.val[2]=0;
          cvSet2D(x->image, iy, ix, pixel);  
          x->x_newipoints[nbepoints].state=0;
          x->x_newipoints[nbepoints].x=ix;
          x->x_newipoints[nbepoints].y=iy;
          nbipoints++;
       }

       if ( point_type[jmask] == 3 ) // jonction point
       {
          // blue
          x->x_newjpoints[nbepoints].state=0;
          x->x_newjpoints[nbepoints].x=ix;
          x->x_newjpoints[nbepoints].y=iy;
          nbjpoints++;
       }

       // wrong skeleton points -- like errors but not really
       if ( ( point_type[jmask] == 255 ) && ( data[iy*x->x_vwidth+ix]>0 ) )
       {
          // red
          pixel.val[0]=255;
          pixel.val[1]=0;
          pixel.val[2]=0;
          cvSet2D(x->image, iy, ix, pixel);  
       }
    }
  }

  // find the corresponding ending points
  for ( j=0; j<MAXPOINTS; j++ )
  {
    float odist, dist;
    int oi;

     if ( x->x_cstepoints[j].state == -1 ) continue; // no point

     dist=(x->x_vwidth>x->x_vheight)?x->x_vwidth:x->x_vheight;
     oi=-1;

     for ( i=0; i<MAXPOINTS; i++ )
     {
       if ( x->x_newepoints[i].state == -1 ) continue; // no point

       odist=sqrt( pow( x->x_newepoints[i].x - x->x_cstepoints[j].x, 2 ) + pow( x->x_newepoints[i].y - x->x_cstepoints[j].y, 2 ) );

       // search for the closest old point
       // that is likely to be this one
       if ( odist < x->x_mmove )
       {
          if ( odist < dist )
          {
            oi=i;
            dist=odist;
          }
       }
     }

     if ( oi == -1 )
     {
        x->x_cstepoints[j].state = -1; // lost point
     }
     else
     {
        x->x_cstepoints[j].x = x->x_newepoints[oi].x;
        x->x_cstepoints[j].y = x->x_newepoints[oi].y;
        x->x_cstepoints[j].state = 1;
        x->x_newepoints[oi].state = 1;

        sprintf( tindex, "%d", j );
        cvPutText( x->image, tindex, cvPoint(x->x_cstepoints[j].x,x->x_cstepoints[j].y), &x->font, CV_RGB(0,255,0));
        SETFLOAT(&x->plist[0], oi);
        SETFLOAT(&x->plist[1], x->x_cstepoints[j].x);
        SETFLOAT(&x->plist[2], x->x_cstepoints[j].y);
        outlet_list( x->x_outlet1, 0, 3, x->plist );
     }
  }
 
  // register new ending points
  for ( j=0; j<MAXPOINTS; j++ )
  {
    int oi;

     if ( x->x_newepoints[j].state == 0 ) 
     {
        oi = pdp_joint_register_epoint(x, x->x_newepoints[j].x, x->x_newepoints[j].y);
        sprintf( tindex, "%d", oi );
        cvPutText( x->image, tindex, cvPoint(x->x_newepoints[j].x,x->x_newepoints[j].y), &x->font, CV_RGB(0,255,0));
        SETFLOAT(&x->plist[0], oi);
        SETFLOAT(&x->plist[1], x->x_newepoints[j].x);
        SETFLOAT(&x->plist[2], x->x_newepoints[j].y);
        outlet_list( x->x_outlet1, 0, 3, x->plist );
     }
  }

  // find the corresponding jonction points
  for ( j=0; j<MAXPOINTS; j++ )
  {
    float odist, dist;
    int oi;

     if ( x->x_cstjpoints[j].state == -1 ) continue; // no point

     dist=(x->x_vwidth>x->x_vheight)?x->x_vwidth:x->x_vheight;
     oi=-1;

     for ( i=0; i<MAXPOINTS; i++ )
     {
       if ( x->x_newjpoints[i].state == -1 ) continue; // no point

       odist=sqrt( pow( x->x_newjpoints[i].x - x->x_cstjpoints[j].x, 2 ) + pow( x->x_newjpoints[i].y - x->x_cstjpoints[j].y, 2 ) );

       // search for the closest old point
       // that is likely to be this one
       if ( odist < x->x_mmove )
       {
          if ( odist < dist )
          {
            oi=i;
            dist=odist;
          }
       }
     }

     if ( oi == -1 )
     {
        x->x_cstjpoints[j].state = -1; // lost point
     }
     else
     {
        x->x_cstjpoints[j].x = x->x_newjpoints[oi].x;
        x->x_cstjpoints[j].y = x->x_newjpoints[oi].y;
        x->x_cstjpoints[j].state = 1;
        x->x_newjpoints[oi].state = 1;

        sprintf( tindex, "%d", j );
        cvPutText( x->image, tindex, cvPoint(x->x_cstjpoints[j].x,x->x_cstjpoints[j].y), &x->font, CV_RGB(255,0,0));
        SETFLOAT(&x->plist[0], oi);
        SETFLOAT(&x->plist[1], x->x_cstjpoints[j].x);
        SETFLOAT(&x->plist[2], x->x_cstjpoints[j].y);
        outlet_list( x->x_outlet2, 0, 3, x->plist );
     }
  }
 
  // register new jonction points
  for ( j=0; j<MAXPOINTS; j++ )
  {
    int oi;

     if ( x->x_newjpoints[j].state == 0 ) 
     {
        oi = pdp_joint_register_jpoint(x, x->x_newjpoints[j].x, x->x_newjpoints[j].y);
        sprintf( tindex, "%d", oi );
        cvPutText( x->image, tindex, cvPoint(x->x_newjpoints[j].x,x->x_newjpoints[j].y), &x->font, CV_RGB(0,255,0));
        SETFLOAT(&x->plist[0], oi);
        SETFLOAT(&x->plist[1], x->x_newjpoints[j].x);
        SETFLOAT(&x->plist[2], x->x_newjpoints[j].y);
        outlet_list( x->x_outlet2, 0, 3, x->plist );
     }
  }

  // find the corresponding inflection points
  for ( j=0; j<3*MAXPOINTS; j++ )
  {
    float odist, dist;
    int oi;

     if ( x->x_cstipoints[j].state == -1 ) continue; // no point

     dist=(x->x_vwidth>x->x_vheight)?x->x_vwidth:x->x_vheight;
     oi=-1;

     for ( i=0; i<3*MAXPOINTS; i++ )
     {
       if ( x->x_newipoints[i].state == -1 ) continue; // no point

       odist=sqrt( pow( x->x_newipoints[i].x - x->x_cstipoints[j].x, 2 ) + pow( x->x_newipoints[i].y - x->x_cstipoints[j].y, 2 ) );

       // search for the closest old point
       // that is likely to be this one
       if ( odist < x->x_mmove )
       {
          if ( odist < dist )
          {
            oi=i;
            dist=odist;
          }
       }
     }

     if ( oi == -1 )
     {
        x->x_cstipoints[j].state = -1; // lost point
     }
     else
     {
        x->x_cstipoints[j].x = x->x_newjpoints[oi].x;
        x->x_cstipoints[j].y = x->x_newjpoints[oi].y;
        x->x_cstipoints[j].state = 1;
        x->x_newipoints[oi].state = 1;

        SETFLOAT(&x->plist[0], oi);
        SETFLOAT(&x->plist[1], x->x_cstipoints[j].x);
        SETFLOAT(&x->plist[2], x->x_cstipoints[j].y);
        outlet_list( x->x_outlet3, 0, 3, x->plist );
     }
  }
 
  // register new inflection points
  for ( j=0; j<3*MAXPOINTS; j++ )
  {
    int oi;

     if ( x->x_newipoints[j].state == 0 ) 
     {
        oi = pdp_joint_register_ipoint(x, x->x_newipoints[j].x, x->x_newipoints[j].y);
        SETFLOAT(&x->plist[0], oi);
        SETFLOAT(&x->plist[1], x->x_newipoints[j].x);
        SETFLOAT(&x->plist[2], x->x_newipoints[j].y);
        outlet_list( x->x_outlet3, 0, 3, x->plist );
     }
  }

  memcpy( newdata, x->image->imageData, x->x_vsize*3 );

  return;
}

static void pdp_joint_sendpacket(t_pdp_joint *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet2);

    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    pdp_packet_mark_unused(x->x_packet1);
    x->x_packet1 = -1;
}

static void pdp_joint_process(t_pdp_joint *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet1))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_joint_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet1)->info.image.encoding)
        {

	case PDP_BITMAP_GREY:
            pdp_queue_add(x, pdp_joint_process_grey, pdp_joint_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_joint_process */
	    break;
	    
	}
    }

}

static void pdp_joint_input_0(t_pdp_joint *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/grey/*") );
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet1, (int)x->x_packet0, pdp_gensym("bitmap/grey/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_joint_process(x);
    }
}

static void pdp_joint_free(t_pdp_joint *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_packet_mark_unused(x->x_packet1);

    cvReleaseImage(&x->image);
}

t_class *pdp_joint_class;

void *pdp_joint_new(void)
{
    int i, j;

    t_pdp_joint *x = (t_pdp_joint *)pd_new(pdp_joint_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("passes"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet2 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet3 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_packet2 = -1;

    x->x_vwidth = 320;
    x->x_vheight = 240;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    x->image = cvCreateImage(cvSize(x->x_vwidth,x->x_vheight), IPL_DEPTH_8U, 3);
    // initialize font
    cvInitFont( &x->font, CV_FONT_HERSHEY_PLAIN, 0.5, 0.5, 0, 1, 8 );

    x->x_mmove = 20;

    pdp_joint_clear(x);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_joint_setup(void)
{
    // post( pdp_joint_version );
    pdp_joint_class = class_new(gensym("pdp_joint"), (t_newmethod)pdp_joint_new,
    	(t_method)pdp_joint_free, sizeof(t_pdp_joint), 0, A_NULL);

    class_addmethod(pdp_joint_class, (t_method)pdp_joint_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_joint_class, (t_method)pdp_joint_maxmove, gensym("maxmove"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_joint_class, (t_method)pdp_joint_clear, gensym("clear"), A_NULL);
}

#ifdef __cplusplus
}
#endif
