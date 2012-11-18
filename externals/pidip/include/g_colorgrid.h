/* Copyright (c) 2002 Yves Degoyon
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* a header for pdp_colorgrid which enables to control
*  2 parameters with the mouse cursor
*/

#ifndef __G_COLORGRID_H
#define __G_COLORGRID_H

typedef struct _pdp_colorgrid
{
    t_object x_obj;
    t_glist *x_glist;
    t_symbol *x_name; 
    t_outlet *x_xoutlet; 
    t_outlet *x_youtlet; 
    t_outlet *x_zoutlet; 
    int x_null; 	/* To dissable resize                             */
    int x_height; 	/* height of the pdp_colorgrid                        */
    t_float x_min; 	/* minimum value of x                        */
    t_float x_max; 	/* max value of x                            */
    int x_width; 	/* width of the pdp_colorgrid                         */
    t_float y_min; 	/* minimum value of y                        */
    t_float y_max; 	/* max value of y                            */
    t_float x_current; 	/* x coordinate of current position          */
    t_float y_current; 	/* y coordinate of current position          */
    int x_selected; 	/* stores selected state                     */
    int x_point; 	/* indicates if a point is plotted           */
    int x_pdp_colorgrid; 	/* indicates if a pdp_colorgrid is requested          */
    t_float x_xstep; 	/* sets the step ( grain ) for x             */
    t_float x_ystep; 	/* sets the step ( grain ) for y             */
    int x_xlines; 	/* number of vertical lines                  */
    int x_ylines; 	/* number of horizontal lines                */
    t_symbol*  x_fname;
} t_pdp_colorgrid;

#endif
