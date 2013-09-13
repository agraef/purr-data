/*
 * tkpTrig.c --
 *
 *	This file contains a collection of trigonometry utility routines that
 *	are used by Tk and in particular by the canvas code. It also has
 *	miscellaneous geometry functions used by canvases.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: tkpTrig.c,v 1.2 2008/06/05 14:39:49 matben Exp $
 */
 
/*
 *	Copied here from tkTrig.c when they contain arguments
 *	specific for the canvas implementaion.
 */

#include <stdio.h>
#include "tkInt.h"
#include "tkIntPath.h"
#include "tkpCanvas.h"

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#ifndef PI
#   define PI 3.14159265358979323846
#endif /* PI */


/*
 *--------------------------------------------------------------
 *
 * TkPathIncludePoint --
 *
 *	Given a point and a generic canvas item header, expand the item's
 *	bounding box if needed to include the point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The boudn.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
void
TkPathIncludePoint(
    register Tk_PathItem *itemPtr,/* Item whose bounding box is being
				 * calculated. */
    double *pointPtr)		/* Address of two doubles giving x and y
				 * coordinates of point. */
{
    int tmp;

    tmp = (int) (pointPtr[0] + 0.5);
    if (tmp < itemPtr->x1) {
	itemPtr->x1 = tmp;
    }
    if (tmp > itemPtr->x2) {
	itemPtr->x2 = tmp;
    }
    tmp = (int) (pointPtr[1] + 0.5);
    if (tmp < itemPtr->y1) {
	itemPtr->y1 = tmp;
    }
    if (tmp > itemPtr->y2) {
	itemPtr->y2 = tmp;
    }
}


/*
 *--------------------------------------------------------------
 *
 * TkPathBezierScreenPoints --
 *
 *	Given four control points, create a larger set of XPoints for a Bezier
 *	curve based on the points.
 *
 * Results:
 *	The array at *xPointPtr gets filled in with numSteps XPoints
 *	corresponding to the Bezier spline defined by the four control points.
 *	Note: no output point is generated for the first input point, but an
 *	output point *is* generated for the last input point.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkPathBezierScreenPoints(
    Tk_PathCanvas canvas,	/* Canvas in which curve is to be drawn. */
    double control[],		/* Array of coordinates for four control
				 * points: x0, y0, x1, y1, ... x3 y3. */
    int numSteps,		/* Number of curve points to generate. */
    register XPoint *xPointPtr)	/* Where to put new points. */
{
    int i;
    double u, u2, u3, t, t2, t3;

    for (i = 1; i <= numSteps; i++, xPointPtr++) {
	t = ((double) i)/((double) numSteps);
	t2 = t*t;
	t3 = t2*t;
	u = 1.0 - t;
	u2 = u*u;
	u3 = u2*u;
	Tk_PathCanvasDrawableCoords(canvas,
		(control[0]*u3 + 3.0 * (control[2]*t*u2 + control[4]*t2*u)
		    + control[6]*t3),
		(control[1]*u3 + 3.0 * (control[3]*t*u2 + control[5]*t2*u)
		    + control[7]*t3),
		&xPointPtr->x, &xPointPtr->y);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkPathBezierPoints --
 *
 *	Given four control points, create a larger set of points for a Bezier
 *	curve based on the points.
 *
 * Results:
 *	The array at *coordPtr gets filled in with 2*numSteps coordinates,
 *	which correspond to the Bezier spline defined by the four control
 *	points. Note: no output point is generated for the first input point,
 *	but an output point *is* generated for the last input point.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkPathBezierPoints(
    double control[],		/* Array of coordinates for four control
				 * points: x0, y0, x1, y1, ... x3 y3. */
    int numSteps,		/* Number of curve points to generate. */
    register double *coordPtr)	/* Where to put new points. */
{
    int i;
    double u, u2, u3, t, t2, t3;

    for (i = 1; i <= numSteps; i++, coordPtr += 2) {
	t = ((double) i)/((double) numSteps);
	t2 = t*t;
	t3 = t2*t;
	u = 1.0 - t;
	u2 = u*u;
	u3 = u2*u;
	coordPtr[0] = control[0]*u3
		+ 3.0 * (control[2]*t*u2 + control[4]*t2*u) + control[6]*t3;
	coordPtr[1] = control[1]*u3
		+ 3.0 * (control[3]*t*u2 + control[5]*t2*u) + control[7]*t3;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkPathMakeBezierCurve --
 *
 *	Given a set of points, create a new set of points that fit parabolic
 *	splines to the line segments connecting the original points. Produces
 *	output points in either of two forms.
 *
 *	Note: the name of this function should *not* be taken to mean that it
 *	interprets the input points as directly defining Bezier curves.
 *	Rather, it internally computes a Bezier curve representation of each
 *	parabolic spline segment. (These Bezier curves are then flattened to
 *	produce the points filled into the output arrays.)
 *
 * Results:
 *	Either or both of the xPoints or dblPoints arrays are filled in. The
 *	return value is the number of points placed in the arrays. Note: if
 *	the first and last points are the same, then a closed curve is
 *	generated.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkPathMakeBezierCurve(
    Tk_PathCanvas canvas,	/* Canvas in which curve is to be drawn. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints,		/* Number of points at pointPtr. */
    int numSteps,		/* Number of steps to use for each spline
				 * segments (determines smoothness of
				 * curve). */
    XPoint xPoints[],		/* Array of XPoints to fill in (e.g. for
				 * display). NULL means don't fill in any
				 * XPoints. */
    double dblPoints[])		/* Array of points to fill in as doubles, in
				 * the form x0, y0, x1, y1, .... NULL means
				 * don't fill in anything in this form. Caller
				 * must make sure that this array has enough
				 * space. */
{
    int closed, outputPoints, i;
    int numCoords = numPoints*2;
    double control[8];

    /*
     * If the curve is a closed one then generate a special spline that spans
     * the last points and the first ones. Otherwise just put the first point
     * into the output.
     */

    if (!pointPtr) {
	/*
	 * Of pointPtr == NULL, this function returns an upper limit of the
	 * array size to store the coordinates. This can be used to allocate
	 * storage, before the actual coordinates are calculated.
	 */

	return 1 + numPoints * numSteps;
    }

    outputPoints = 0;
    if ((pointPtr[0] == pointPtr[numCoords-2])
	    && (pointPtr[1] == pointPtr[numCoords-1])) {
	closed = 1;
	control[0] = 0.5*pointPtr[numCoords-4] + 0.5*pointPtr[0];
	control[1] = 0.5*pointPtr[numCoords-3] + 0.5*pointPtr[1];
	control[2] = 0.167*pointPtr[numCoords-4] + 0.833*pointPtr[0];
	control[3] = 0.167*pointPtr[numCoords-3] + 0.833*pointPtr[1];
	control[4] = 0.833*pointPtr[0] + 0.167*pointPtr[2];
	control[5] = 0.833*pointPtr[1] + 0.167*pointPtr[3];
	control[6] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	control[7] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	if (xPoints != NULL) {
	    Tk_PathCanvasDrawableCoords(canvas, control[0], control[1],
		    &xPoints->x, &xPoints->y);
	    TkPathBezierScreenPoints(canvas, control, numSteps, xPoints+1);
	    xPoints += numSteps+1;
	}
	if (dblPoints != NULL) {
	    dblPoints[0] = control[0];
	    dblPoints[1] = control[1];
	    TkPathBezierPoints(control, numSteps, dblPoints+2);
	    dblPoints += 2*(numSteps+1);
	}
	outputPoints += numSteps+1;
    } else {
	closed = 0;
	if (xPoints != NULL) {
	    Tk_PathCanvasDrawableCoords(canvas, pointPtr[0], pointPtr[1],
		    &xPoints->x, &xPoints->y);
	    xPoints += 1;
	}
	if (dblPoints != NULL) {
	    dblPoints[0] = pointPtr[0];
	    dblPoints[1] = pointPtr[1];
	    dblPoints += 2;
	}
	outputPoints += 1;
    }

    for (i = 2; i < numPoints; i++, pointPtr += 2) {
	/*
	 * Set up the first two control points. This is done differently for
	 * the first spline of an open curve than for other cases.
	 */

	if ((i == 2) && !closed) {
	    control[0] = pointPtr[0];
	    control[1] = pointPtr[1];
	    control[2] = 0.333*pointPtr[0] + 0.667*pointPtr[2];
	    control[3] = 0.333*pointPtr[1] + 0.667*pointPtr[3];
	} else {
	    control[0] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	    control[1] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	    control[2] = 0.167*pointPtr[0] + 0.833*pointPtr[2];
	    control[3] = 0.167*pointPtr[1] + 0.833*pointPtr[3];
	}

	/*
	 * Set up the last two control points. This is done differently for
	 * the last spline of an open curve than for other cases.
	 */

	if ((i == (numPoints-1)) && !closed) {
	    control[4] = .667*pointPtr[2] + .333*pointPtr[4];
	    control[5] = .667*pointPtr[3] + .333*pointPtr[5];
	    control[6] = pointPtr[4];
	    control[7] = pointPtr[5];
	} else {
	    control[4] = .833*pointPtr[2] + .167*pointPtr[4];
	    control[5] = .833*pointPtr[3] + .167*pointPtr[5];
	    control[6] = 0.5*pointPtr[2] + 0.5*pointPtr[4];
	    control[7] = 0.5*pointPtr[3] + 0.5*pointPtr[5];
	}

	/*
	 * If the first two points coincide, or if the last two points
	 * coincide, then generate a single straight-line segment by
	 * outputting the last control point.
	 */

	if (((pointPtr[0] == pointPtr[2]) && (pointPtr[1] == pointPtr[3]))
		|| ((pointPtr[2] == pointPtr[4])
		&& (pointPtr[3] == pointPtr[5]))) {
	    if (xPoints != NULL) {
		Tk_PathCanvasDrawableCoords(canvas, control[6], control[7],
			&xPoints[0].x, &xPoints[0].y);
		xPoints++;
	    }
	    if (dblPoints != NULL) {
		dblPoints[0] = control[6];
		dblPoints[1] = control[7];
		dblPoints += 2;
	    }
	    outputPoints += 1;
	    continue;
	}

	/*
	 * Generate a Bezier spline using the control points.
	 */


	if (xPoints != NULL) {
	    TkPathBezierScreenPoints(canvas, control, numSteps, xPoints);
	    xPoints += numSteps;
	}
	if (dblPoints != NULL) {
	    TkPathBezierPoints(control, numSteps, dblPoints);
	    dblPoints += 2*numSteps;
	}
	outputPoints += numSteps;
    }
    return outputPoints;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathMakeRawCurve --
 *
 *	Interpret the given set of points as the raw knots and control points
 *	defining a sequence of cubic Bezier curves. Create a new set of points
 *	that fit these Bezier curves. Output points are produced in either of
 *	two forms.
 *
 * Results:
 *	Either or both of the xPoints or dblPoints arrays are filled in. The
 *	return value is the number of points placed in the arrays.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkPathMakeRawCurve(
    Tk_PathCanvas canvas,	/* Canvas in which curve is to be drawn. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints,		/* Number of points at pointPtr. */
    int numSteps,		/* Number of steps to use for each curve
				 * segment (determines smoothness of
				 * curve). */
    XPoint xPoints[],		/* Array of XPoints to fill in (e.g. for
				 * display). NULL means don't fill in any
				 * XPoints. */
    double dblPoints[])		/* Array of points to fill in as doubles, in
				 * the form x0, y0, x1, y1, .... NULL means
				 * don't fill in anything in this form.
				 * Caller must make sure that this array has
				 * enough space. */
{
    int outputPoints, i;
    int numSegments = (numPoints+1)/3;
    double *segPtr;

    /*
     * The input describes a curve with s Bezier curve segments if there are
     * 3s+1, 3s, or 3s-1 input points. In the last two cases, 1 or 2 initial
     * points from the first curve segment are reused as defining points also
     * for the last curve segment. In the case of 3s input points, this will
     * automatically close the curve.
     */

    if (!pointPtr) {
	/*
	 * If pointPtr == NULL, this function returns an upper limit of the
	 * array size to store the coordinates. This can be used to allocate
	 * storage, before the actual coordinates are calculated.
	 */

	return 1 + numSegments * numSteps;
    }

    outputPoints = 0;
    if (xPoints != NULL) {
	Tk_PathCanvasDrawableCoords(canvas, pointPtr[0], pointPtr[1],
		&xPoints->x, &xPoints->y);
	xPoints += 1;
    }
    if (dblPoints != NULL) {
	dblPoints[0] = pointPtr[0];
	dblPoints[1] = pointPtr[1];
	dblPoints += 2;
    }
    outputPoints += 1;

    /*
     * The next loop handles all curve segments except one that overlaps the
     * end of the list of coordinates.
     */

    for (i=numPoints,segPtr=pointPtr ; i>=4 ; i-=3,segPtr+=6) {
	if (segPtr[0]==segPtr[2] && segPtr[1]==segPtr[3] &&
		segPtr[4]==segPtr[6] && segPtr[5]==segPtr[7]) {
	    /*
	     * The control points on this segment are equal to their
	     * neighbouring knots, so this segment is just a straight line. A
	     * single point is sufficient.
	     */

	    if (xPoints != NULL) {
		Tk_PathCanvasDrawableCoords(canvas, segPtr[6], segPtr[7],
			&xPoints->x, &xPoints->y);
		xPoints += 1;
	    }
	    if (dblPoints != NULL) {
		dblPoints[0] = segPtr[6];
		dblPoints[1] = segPtr[7];
		dblPoints += 2;
	    }
	    outputPoints += 1;
	} else {
	    /*
	     * This is a generic Bezier curve segment.
	     */

	    if (xPoints != NULL) {
		TkPathBezierScreenPoints(canvas, segPtr, numSteps, xPoints);
		xPoints += numSteps;
	    }
	    if (dblPoints != NULL) {
		TkPathBezierPoints(segPtr, numSteps, dblPoints);
		dblPoints += 2*numSteps;
	    }
	    outputPoints += numSteps;
	}
    }

    /*
     * If at this point i>1, then there is some point which has not yet been
     * used. Make another curve segment.
     */

    if (i > 1) {
	int j;
	double control[8];

	/*
	 * Copy the relevant coordinates to control[], so that it can be
	 * passed as a unit to e.g. TkPathBezierPoints.
	 */

	for (j=0; j<2*i; j++) {
	    control[j] = segPtr[j];
	}
	for (; j<8; j++) {
	    control[j] = pointPtr[j-2*i];
	}

	/*
	 * Then we just do the same things as above.
	 */

	if (control[0]==control[2] && control[1]==control[3] &&
		control[4]==control[6] && control[5]==control[7]) {
	    /*
	     * The control points on this segment are equal to their
	     * neighbouring knots, so this segment is just a straight line. A
	     * single point is sufficient.
	     */

	    if (xPoints != NULL) {
		Tk_PathCanvasDrawableCoords(canvas, control[6], control[7],
			&xPoints->x, &xPoints->y);
		xPoints += 1;
	    }
	    if (dblPoints != NULL) {
		dblPoints[0] = control[6];
		dblPoints[1] = control[7];
		dblPoints += 2;
	    }
	    outputPoints += 1;
	} else {
	    /*
	     * This is a generic Bezier curve segment.
	     */

	    if (xPoints != NULL) {
		TkPathBezierScreenPoints(canvas, control, numSteps, xPoints);
		xPoints += numSteps;
	    }
	    if (dblPoints != NULL) {
		TkPathBezierPoints(control, numSteps, dblPoints);
		dblPoints += 2*numSteps;
	    }
	    outputPoints += numSteps;
	}
    }

    return outputPoints;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathMakeBezierPostscript --
 *
 *	This function generates Postscript commands that create a path
 *	corresponding to a given Bezier curve.
 *
 * Results:
 *	None. Postscript commands to generate the path are appended to the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkPathMakeBezierPostscript(
    Tcl_Interp *interp,		/* Interpreter in whose result the Postscript
				 * is to be stored. */
    Tk_PathCanvas canvas,	/* Canvas widget for which the Postscript is
				 * being generated. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints)		/* Number of points at pointPtr. */
{
    int closed, i;
    int numCoords = numPoints*2;
    double control[8];
    char buffer[200];

    /*
     * If the curve is a closed one then generate a special spline that spans
     * the last points and the first ones. Otherwise just put the first point
     * into the path.
     */

    if ((pointPtr[0] == pointPtr[numCoords-2])
	    && (pointPtr[1] == pointPtr[numCoords-1])) {
	closed = 1;
	control[0] = 0.5*pointPtr[numCoords-4] + 0.5*pointPtr[0];
	control[1] = 0.5*pointPtr[numCoords-3] + 0.5*pointPtr[1];
	control[2] = 0.167*pointPtr[numCoords-4] + 0.833*pointPtr[0];
	control[3] = 0.167*pointPtr[numCoords-3] + 0.833*pointPtr[1];
	control[4] = 0.833*pointPtr[0] + 0.167*pointPtr[2];
	control[5] = 0.833*pointPtr[1] + 0.167*pointPtr[3];
	control[6] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	control[7] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	sprintf(buffer, "%.15g %.15g moveto\n%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		control[0], Tk_PathCanvasPsY(canvas, control[1]),
		control[2], Tk_PathCanvasPsY(canvas, control[3]),
		control[4], Tk_PathCanvasPsY(canvas, control[5]),
		control[6], Tk_PathCanvasPsY(canvas, control[7]));
    } else {
	closed = 0;
	control[6] = pointPtr[0];
	control[7] = pointPtr[1];
	sprintf(buffer, "%.15g %.15g moveto\n",
		control[6], Tk_PathCanvasPsY(canvas, control[7]));
    }
    Tcl_AppendResult(interp, buffer, NULL);

    /*
     * Cycle through all the remaining points in the curve, generating a curve
     * section for each vertex in the linear path.
     */

    for (i = numPoints-2, pointPtr += 2; i > 0; i--, pointPtr += 2) {
	control[2] = 0.333*control[6] + 0.667*pointPtr[0];
	control[3] = 0.333*control[7] + 0.667*pointPtr[1];

	/*
	 * Set up the last two control points. This is done differently for
	 * the last spline of an open curve than for other cases.
	 */

	if ((i == 1) && !closed) {
	    control[6] = pointPtr[2];
	    control[7] = pointPtr[3];
	} else {
	    control[6] = 0.5*pointPtr[0] + 0.5*pointPtr[2];
	    control[7] = 0.5*pointPtr[1] + 0.5*pointPtr[3];
	}
	control[4] = 0.333*control[6] + 0.667*pointPtr[0];
	control[5] = 0.333*control[7] + 0.667*pointPtr[1];

	sprintf(buffer, "%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		control[2], Tk_PathCanvasPsY(canvas, control[3]),
		control[4], Tk_PathCanvasPsY(canvas, control[5]),
		control[6], Tk_PathCanvasPsY(canvas, control[7]));
	Tcl_AppendResult(interp, buffer, NULL);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkPathMakeRawCurvePostscript --
 *
 *	This function interprets the input points as the raw knot and control
 *	points for a curve composed of Bezier curve segments, just like
 *	TkPathMakeRawCurve. It generates Postscript commands that create a path
 *	corresponding to this given curve.
 *
 * Results:
 *	None. Postscript commands to generate the path are appended to the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
TkPathMakeRawCurvePostscript(
    Tcl_Interp *interp,		/* Interpreter in whose result the Postscript
				 * is to be stored. */
    Tk_PathCanvas canvas,	/* Canvas widget for which the Postscript is
				 * being generated. */
    double *pointPtr,		/* Array of input coordinates: x0, y0, x1, y1,
				 * etc.. */
    int numPoints)		/* Number of points at pointPtr. */
{
    int i;
    double *segPtr;
    char buffer[200];

    /*
     * Put the first point into the path.
     */

    sprintf(buffer, "%.15g %.15g moveto\n",
	    pointPtr[0], Tk_PathCanvasPsY(canvas, pointPtr[1]));
    Tcl_AppendResult(interp, buffer, NULL);

    /*
     * Loop through all the remaining points in the curve, generating a
     * straight line or curve section for every three of them.
     */

    for (i=numPoints-1,segPtr=pointPtr ; i>=3 ; i-=3,segPtr+=6) {
	if (segPtr[0]==segPtr[2] && segPtr[1]==segPtr[3] &&
		segPtr[4]==segPtr[6] && segPtr[5]==segPtr[7]) {
	    /*
	     * The control points on this segment are equal to their
	     * neighbouring knots, so this segment is just a straight line.
	     */

	    sprintf(buffer, "%.15g %.15g lineto\n",
		    segPtr[6], Tk_PathCanvasPsY(canvas, segPtr[7]));
	} else {
	    /*
	     * This is a generic Bezier curve segment.
	     */

	    sprintf(buffer, "%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		    segPtr[2], Tk_PathCanvasPsY(canvas, segPtr[3]),
		    segPtr[4], Tk_PathCanvasPsY(canvas, segPtr[5]),
		    segPtr[6], Tk_PathCanvasPsY(canvas, segPtr[7]));
	}
	Tcl_AppendResult(interp, buffer, NULL);
    }

    /*
     * If there are any points left that haven't been used, then build the
     * last segment and generate Postscript in the same way for that.
     */

    if (i > 0) {
	int j;
	double control[8];

	for (j=0; j<2*i+2; j++) {
	    control[j] = segPtr[j];
	}
	for (; j<8; j++) {
	    control[j] = pointPtr[j-2*i-2];
	}

	if (control[0]==control[2] && control[1]==control[3] &&
		control[4]==control[6] && control[5]==control[7]) {
	    /*
	     * Straight line.
	     */

	    sprintf(buffer, "%.15g %.15g lineto\n",
		    control[6], Tk_PathCanvasPsY(canvas, control[7]));
	} else {
	    /*
	     * Bezier curve segment.
	     */

	    sprintf(buffer, "%.15g %.15g %.15g %.15g %.15g %.15g curveto\n",
		    control[2], Tk_PathCanvasPsY(canvas, control[3]),
		    control[4], Tk_PathCanvasPsY(canvas, control[5]),
		    control[6], Tk_PathCanvasPsY(canvas, control[7]));
	}
	Tcl_AppendResult(interp, buffer, NULL);
    }
}


