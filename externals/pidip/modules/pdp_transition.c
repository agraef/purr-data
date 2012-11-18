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

/*  This object is an object allowing transitions between two video sources
 *  "circle", "wipe", "random", "melt", "page" and "blend"
 *  Written by Yves Degoyon                                 
 */

#include "pdp.h"
#include <math.h>

#define BLEND_MAX 200
#define PAGE_RAY  50

static char   *pdp_transition_version = "pdp_transition: version 0.1, two sources transition, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_transition_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_packet;
    int x_dropped;
    int x_queue_id;

    int x_vwidth0;
    int x_vheight0;
    int x_vsize0;

    int x_vwidth1;
    int x_vheight1;
    int x_vsize1;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_transition_mode; // 1 : "circle"
    int x_transition_pending; 

    int x_current_source;
    int x_target_source;

    int x_pos; // current position for transition
    int x_inc; // increment for various mode
    int x_rand;// randomizing argument

} t_pdp_transition;

static int pdp_transition_min( int a, int b )
{
   if ( a == 0 ) return b;
   if ( b == 0 ) return a;
   if ( a < b ) return a;
   else return b;
}

static void pdp_transition_circle(t_pdp_transition *x, t_floatarg finc )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    x->x_transition_mode = 1;
    x->x_transition_pending = 1;
}

static void pdp_transition_wipelr(t_pdp_transition *x, t_floatarg finc, t_floatarg frand )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    if ( (int) frand >= 0 )
    {
       x->x_rand = (int)frand;
    }
    x->x_transition_mode = 2;
    x->x_transition_pending = 1;
}

static void pdp_transition_wiperl(t_pdp_transition *x, t_floatarg finc, t_floatarg frand )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    if ( (int) frand >= 0 )
    {
       x->x_rand = (int)frand;
    }
    x->x_transition_mode = 3;
    x->x_transition_pending = 1;
    x->x_pos = x->x_vwidth;
}

static void pdp_transition_mwipe(t_pdp_transition *x, t_floatarg finc, t_floatarg frand )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    if ( (int) frand >= 0 )
    {
       x->x_rand = (int)frand;
    }
    x->x_transition_mode = 4;
    x->x_transition_pending = 1;
    x->x_pos = 0;
}

static void pdp_transition_wipetd(t_pdp_transition *x, t_floatarg finc, t_floatarg frand )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    if ( (int) frand >= 0 )
    {
       x->x_rand = (int)frand;
    }
    x->x_transition_mode = 5;
    x->x_transition_pending = 1;
    x->x_pos = 0;
}

static void pdp_transition_wipebu(t_pdp_transition *x, t_floatarg finc, t_floatarg frand )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    if ( (int) frand >= 0 )
    {
       x->x_rand = (int)frand;
    }
    x->x_transition_mode = 6;
    x->x_transition_pending = 1;
    x->x_pos = x->x_vheight;
}

static void pdp_transition_random(t_pdp_transition *x, t_floatarg finc )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    x->x_transition_mode = 7;
    x->x_transition_pending = 1;
    x->x_rand = x->x_vsize/(x->x_inc*(pdp_transition_min( x->x_vwidth, 100 )));
}

static void pdp_transition_melt(t_pdp_transition *x, t_floatarg finc )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    x->x_transition_mode = 8;
    x->x_transition_pending = 1;
    x->x_rand = 20;
}

static void pdp_transition_blend(t_pdp_transition *x, t_floatarg finc, t_floatarg frand )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    if ( (int) frand >= 0 )
    {
       x->x_rand = (int)frand;
    }
    x->x_transition_mode = 9;
    x->x_transition_pending = 1;
    x->x_pos = 0;
}

static void pdp_transition_page(t_pdp_transition *x, t_floatarg finc )
{
    if ( x->x_transition_pending )
    {
       post ( "pdp_transition : a transition is already pending, retry later...." );
       return;
    }
    if ( (int) finc > 0 )
    {
       x->x_inc = (int)finc;
    }
    x->x_transition_mode = 10;
    x->x_transition_pending = 1;
    x->x_pos = x->x_vheight;
}

static void pdp_transition_process_yv12(t_pdp_transition *x)
{
    t_pdp     *header0 = pdp_packet_header(x->x_packet0);
    short int *data0   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *header1 = pdp_packet_header(x->x_packet1);
    short int *data1 = (short int *)pdp_packet_data(x->x_packet1);
    t_pdp     *header;
    short int *data;
    int     tsource, cx=0, cy=0;
    int     px, py, rvalue=0, h1pos, h2pos, xcent, ycent;
    t_float   factor, alpha;
    int       i;
    short int *poY, *poV, *poU, *p0Y, *p0V, *p0U, *p1Y, *p1V, *p1U;

    if ( header0 )
    {
      x->x_vwidth0 = header0->info.image.width;
      x->x_vheight0 = header0->info.image.height;
      x->x_vsize0 = x->x_vwidth0*x->x_vheight0;
    }
    else
    {
      x->x_vwidth0 = x->x_vheight0 = x->x_vsize0 = 0;
    }

    if ( header1 )
    {
      x->x_vwidth1 = header1->info.image.width;
      x->x_vheight1 = header1->info.image.height;
      x->x_vsize1 = x->x_vwidth1*x->x_vheight1;
    }
    else
    {
      x->x_vwidth1 = x->x_vheight1 = x->x_vsize1 = 0;
    }

    x->x_vwidth = pdp_transition_min( x->x_vwidth0 , x->x_vwidth1);
    x->x_vheight = pdp_transition_min( x->x_vheight0 , x->x_vheight1);
    x->x_vsize = x->x_vwidth*x->x_vheight;
    // post( "pdp_transition : resulting frame : %dx%d", x->x_vwidth, x->x_vheight );

    x->x_packet = pdp_packet_new_image_YCrCb( x->x_vwidth, x->x_vheight );

    header = pdp_packet_header(x->x_packet);
    data = (short int *)pdp_packet_data(x->x_packet);

    header->info.image.encoding = PDP_IMAGE_YV12;
    header->info.image.width = x->x_vwidth;
    header->info.image.height = x->x_vheight;

    poY = data;
    poV = data+x->x_vsize;
    poU = data+x->x_vsize+(x->x_vsize>>2);
    if ( x->x_current_source == 0 )
    {
      if ( x->x_vsize0 > 0 ) memcpy( data, data0, (x->x_vsize+(x->x_vsize>>1))<<1 );
      p0Y = data0;
      p0V = data0+x->x_vsize0;
      p0U = data0+x->x_vsize0+(x->x_vsize0>>2);
      p1Y = data1;
      p1V = data1+x->x_vsize1;
      p1U = data1+x->x_vsize1+(x->x_vsize1>>2);
    }
    else
    {
      if ( x->x_vsize1 > 0 ) memcpy( data, data1, (x->x_vsize+(x->x_vsize>>1))<<1 );
      p0Y = data1;
      p0V = data1+x->x_vsize0;
      p0U = data1+x->x_vsize0+(x->x_vsize0>>2);
      p1Y = data0;
      p1V = data0+x->x_vsize1;
      p1U = data0+x->x_vsize1+(x->x_vsize1>>2);
    }
    if ( ( x->x_transition_pending ) && ( x->x_vsize0 > 0 ) && ( x->x_vsize1 > 0 ) )
    {
      switch ( x->x_transition_mode )
      {
        case 1: // circle
          for ( py=0; py<x->x_vheight; py++ )
          {
            for ( px=0; px<x->x_vwidth; px++ )
            {
               cx = px-(x->x_vwidth/2);          
               cy = py-(x->x_vheight/2);          
               if ( cx*cx + cy*cy < x->x_pos*x->x_pos )
               {
                 *(poY) = *(p1Y);     
                 *(poU) = *(p1U);     
                 *(poV) = *(p1V);     
               }
               poY++; p1Y++;
               if ( (px%2==0) && (py%2==0) )
               {
                 poU++; poV++;
                 p1U++; p1V++;
               }
            }
          }
          if ( ( x->x_pos > (x->x_vwidth/2) ) && ( x->x_pos > (x->x_vheight/2) ) )
          {
            post( "pdp_transition : circle transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos += x->x_inc;
          break;

        case 2: // wipelr
          for ( py=0; py<x->x_vheight; py++ )
          {
	    rvalue = (int)(((float) x->x_rand )*( (float)random() ) / RAND_MAX);
            for ( px=0; px<x->x_vwidth; px++ )
            {
               if ( px <= x->x_pos + rvalue )
               {
                 *(poY) = *(p1Y);     
                 *(poU) = *(p1U);     
                 *(poV) = *(p1V);     
               }
               poY++; p1Y++;
               if ( (px%2==0) && (py%2==0) )
               {
                 poU++; poV++;
                 p1U++; p1V++;
               }
            }
          }
          if ( x->x_pos > x->x_vwidth )
          {
            post( "pdp_transition : wipelr transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos += x->x_inc;
          break;

        case 3: // wiperl
          for ( py=0; py<x->x_vheight; py++ )
          {
	    rvalue = (int)(((float) x->x_rand )*( (float)random() ) / RAND_MAX);
            for ( px=0; px<x->x_vwidth; px++ )
            {
               if ( px >= x->x_pos + rvalue )
               {
                 *(poY) = *(p1Y);     
                 *(poU) = *(p1U);     
                 *(poV) = *(p1V);     
               }
               poY++; p1Y++;
               if ( (px%2==0) && (py%2==0) )
               {
                 poU++; poV++;
                 p1U++; p1V++;
               }
            }
          }
          if ( x->x_pos <= 0 )
          {
            post( "pdp_transition : wiperl transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos -= x->x_inc;
          break;

        case 4: // mwipe
          for ( px=0; px<x->x_vwidth; px++ )
          {
	    rvalue = (int)(((float) x->x_rand )*( (float)random() ) / RAND_MAX);
            for ( py=0; py<x->x_vheight; py++ )
            {
               if ( py <= x->x_pos + rvalue )
               {
                 *(poY) = *(p1Y);     
                 *(poU) = *(p1U);     
                 *(poV) = *(p1V);     
               }
               poY++; p1Y++;
               if ( (px%2==0) && (py%2==0) )
               {
                 poU++; poV++;
                 p1U++; p1V++;
               }
            }
          }
          if ( x->x_pos >= x->x_vheight )
          {
            post( "pdp_transition : mwipe transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos += x->x_inc;
          break;

        case 5: // wipetd
          for ( px=0; px<x->x_vwidth; px++ )
          {
	    rvalue = (int)(((float) x->x_rand )*( (float)random() ) / RAND_MAX);
            for ( py=0; py<x->x_vheight; py++ )
            {
               if ( py <= x->x_pos + rvalue )
               {
                 *(poY+py*x->x_vwidth+px) = *(p1Y+py*x->x_vwidth1+px);     
                 *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1U+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
                 *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1V+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
               }
            }
          }
          if ( x->x_pos >= x->x_vheight )
          {
            post( "pdp_transition : wipetd transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos += x->x_inc;
          break;

        case 6: // wipebu
          for ( px=0; px<x->x_vwidth; px++ )
          {
	    rvalue = (int)(((float) x->x_rand )*( (float)random() ) / RAND_MAX);
            for ( py=0; py<x->x_vheight; py++ )
            {
               if ( py >= x->x_pos + rvalue )
               {
                 *(poY+py*x->x_vwidth+px) = *(p1Y+py*x->x_vwidth1+px);     
                 *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1U+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
                 *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1V+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
               }
            }
          }
          if ( x->x_pos <= 0 )
          {
            post( "pdp_transition : wipebu transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos -= x->x_inc;
          break;

        case 7: // random
          for ( px=0; px<x->x_vwidth; px++ )
          {
            for ( py=0; py<x->x_vheight; py++ )
            {
	       rvalue = (int)(((float) x->x_rand )*( (float)random() ) / RAND_MAX);
               if ( rvalue <= x->x_pos )
               {
                 *(poY+py*x->x_vwidth+px) = *(p1Y+py*x->x_vwidth1+px);     
                 *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1U+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
                 *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1V+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
               }
            }
          }
          if ( x->x_pos >= x->x_rand )
          {
            post( "pdp_transition : wipebu transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos += x->x_inc;
          break;

        case 8: // melt
          for ( px=0; px<x->x_vwidth; px++ )
          {
	    rvalue = (int)(((float) x->x_rand )*( (float)random() ) / RAND_MAX)+(abs(px-x->x_vwidth/2)/5);
            for ( py=0; py<x->x_vheight; py++ )
            {
               if ( py <= x->x_pos + rvalue )
               {
                 *(poY+py*x->x_vwidth+px) = 0;
                 *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0; 
                 *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0;
               }
            }
          }
          if ( x->x_pos >= x->x_vheight )
          {
            post( "pdp_transition : melt transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos += x->x_inc;
          break;

        case 9: // blend
          for ( px=0; px<x->x_vwidth; px++ )
          {
            for ( py=0; py<x->x_vheight; py++ )
            {
	       rvalue = (((float) x->x_rand )*( (float)random() ) / RAND_MAX);
	       factor = ( (float) x->x_pos + rvalue ) / BLEND_MAX;
               *(poY+py*x->x_vwidth+px) = 
		       (int)((1.0-factor)*(*(poY+py*x->x_vwidth+px)) + 
		                 factor*(*(p1Y+py*x->x_vwidth1+px)));     
               *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
		       (int)((1.0-factor)*(*(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1))) + 
		                 factor*(*(p1U+(py>>1)*(x->x_vwidth1>>1)+(px>>1))));     
               *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
		       (int)((1.0-factor)*(*(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1))) + 
		                 factor*(*(p1V+(py>>1)*(x->x_vwidth1>>1)+(px>>1))));     
            }
          }
          if ( x->x_pos >= BLEND_MAX )
          {
            post( "pdp_transition : blend transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos += x->x_inc;
          break;

        case 10: // page
          xcent = (int) ( (float) ( x->x_vheight - x->x_pos ) * (( float ) x->x_vwidth ) /
                  ( 2.0 * ( (float) x->x_vheight ) ) );
          ycent = (int) ( ( (float) x->x_pos + (float) x->x_vheight ) / 2.0 );
          for ( px=0; px<x->x_vwidth; px++ )
          {
            for ( py=0; py<x->x_vheight; py++ )
            {
               if ( ( ((float) py)-
                    ((float)x->x_vheight)/((float)x->x_vwidth)*((float)px) ) >= x->x_pos )
               {
                 *(poY+py*x->x_vwidth+px) = *(p1Y+py*x->x_vwidth1+px);     
                 *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1U+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
                 *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
			 *(p1V+(py>>1)*(x->x_vwidth1>>1)+(px>>1));     
               }
	       else
	       {
		  h1pos = ((float) py) + ((float)x->x_vheight)/((float)x->x_vwidth)*((float)px);
		  h2pos = ((float) py) - ((float)x->x_vheight)/((float)x->x_vwidth)*((float)px);
		  alpha = atan( ( (float) x->x_vheight ) / ( (float) x->x_vwidth ) );
                  if ( ( h1pos <= x->x_vheight ) &&
		       ( h1pos >= ( (float) x->x_vheight - PAGE_RAY*cos(alpha) ) ) &&
		       ( h2pos >= ( x->x_pos - PAGE_RAY*sin(alpha) ) ) &&
		       ( pow ( px-xcent, 2 ) + pow ( py-(ycent-PAGE_RAY), 2 ) 
		                        >= pow( PAGE_RAY*cos(alpha), 2 ) ) )
		  {
                     *(poY+py*x->x_vwidth+px) = 0xff<<7;     
                     *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0xff<<8;
                     *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0xff<<8;
		  }
                  if ( ( h1pos >= x->x_vheight ) &&
		       ( h1pos <= ( (float) x->x_vheight + PAGE_RAY*cos(alpha) ) ) &&
		       ( h2pos >= ( x->x_pos - PAGE_RAY*sin(alpha) ) ) &&
		       ( pow ( px-(xcent+PAGE_RAY), 2 ) + pow ( py-ycent, 2 ) 
		                        >= pow( PAGE_RAY*sin(alpha), 2 ) ) )
		  {
                     *(poY+py*x->x_vwidth+px) = 0xff<<7;     
                     *(poU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0xff<<8;
                     *(poV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0xff<<8;
		  }
	       }
            }
          }
          if ( x->x_pos <= -x->x_vheight )
          {
            post( "pdp_transition : page transition finished" );
            x->x_transition_pending = 0;
            x->x_transition_mode = 0;
            tsource = x->x_current_source;
            x->x_current_source = x->x_target_source;
            x->x_target_source = tsource;
            x->x_pos = 0;
          }
          x->x_pos -= x->x_inc;
          break;

        default:
          break;
      }
    }

    return;
}

static void pdp_transition_sendpacket0(t_pdp_transition *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet);
}

static void pdp_transition_sendpacket1(t_pdp_transition *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet);
}

static void pdp_transition_process0(t_pdp_transition *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_transition_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_transition_process_yv12, pdp_transition_sendpacket0, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_transition_process */
	    break;
	    
	}
    }
}

static void pdp_transition_process1(t_pdp_transition *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet1))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_transition_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet1)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_transition_process_yv12, pdp_transition_sendpacket1, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_transition_process */
	    break;
	    
	}
    }
}

static void pdp_transition_input_0(t_pdp_transition *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
    {
      /* release the packet */
      if ( x->x_packet0 != -1 )
      {
        pdp_packet_mark_unused(x->x_packet0);
        x->x_packet0 = -1;
      }
      x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_transition_process0(x);

    }
}

static void pdp_transition_input_1(t_pdp_transition *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))  
    {
      /* release the packet */
      if ( x->x_packet1 != -1 )
      {
        pdp_packet_mark_unused(x->x_packet1);
        x->x_packet1 = -1;
      }
      x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet1, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet1) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_transition_process1(x);

    }
}

static void pdp_transition_free(t_pdp_transition *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_packet_mark_unused(x->x_packet1);
}

t_class *pdp_transition_class;

void *pdp_transition_new(void)
{
    int i;

    t_pdp_transition *x = (t_pdp_transition *)pd_new(pdp_transition_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"),  gensym("pdp1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"),  gensym("pdp2"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_packet = -1;
    x->x_queue_id = -1;

    x->x_transition_mode = 0;
    x->x_transition_pending = 0;

    x->x_current_source = 0;
    x->x_target_source = 1;

    x->x_pos = 0;
    x->x_inc = 1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_transition_setup(void)
{
    // post( pdp_transition_version );
    pdp_transition_class = class_new(gensym("pdp_transition"), (t_newmethod)pdp_transition_new,
    	(t_method)pdp_transition_free, sizeof(t_pdp_transition), 0, A_NULL);

    class_addmethod(pdp_transition_class, (t_method)pdp_transition_input_0, gensym("pdp1"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_input_1, gensym("pdp2"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_circle, gensym("circle"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_wipelr, gensym("wipelr"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_wiperl, gensym("wiperl"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_mwipe, gensym("mwipe"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_wipetd, gensym("wipetd"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_wipebu, gensym("wipebu"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_random, gensym("random"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_melt, gensym("melt"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_blend, gensym("blend"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_transition_class, (t_method)pdp_transition_page, gensym("page"),  A_DEFFLOAT, A_NULL);



}

#ifdef __cplusplus
}
#endif
