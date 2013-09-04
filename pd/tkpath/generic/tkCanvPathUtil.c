/*
 * tkCanvPathUtil.c --
 *
 *	This file implements a path canvas item modelled after its
 *  SVG counterpart. See http://www.w3.org/TR/SVG11/.
 *
 * Copyright (c) 2008  Mats Bengtsson
 *
 * $Id$
 */

#include "tkPathStyle.h"
#include "tkpCanvas.h"
#include "tkCanvPathUtil.h"

/* For debugging. */
extern Tcl_Interp *gInterp;

/*
 * For wider strokes we must make a more detailed analysis
 * when doing hit tests and area tests.
 */
#define kPathStrokeThicknessLimit 	4.0

#define MAX_NUM_STATIC_SEGMENTS  2000
/* @@@ Should this be moved inside the function instead? */
static double staticSpace[2*MAX_NUM_STATIC_SEGMENTS];


static void		MakeSubPathSegments(PathAtom **atomPtrPtr, double *polyPtr, 
                        int *numPointsPtr, int *numStrokesPtr, TMatrix *matrixPtr);
static int		SubPathToArea(Tk_PathStyle *stylePtr, double *polyPtr, int numPoints,
                        int	numStrokes,	double *rectPtr, int inside);


/*
 *--------------------------------------------------------------
 *
 * CoordsForPointItems --
 *
 *	Used as coordProc for items that have plain single point coords.
 *
 * Results:
 *	Standard tcl result.
 *
 * Side effects:
 *	May store new coords in rectPtr.
 *
 *--------------------------------------------------------------
 */

int		
CoordsForPointItems(
        Tcl_Interp *interp, 
        Tk_PathCanvas canvas, 
        double *pointPtr, 		/* Sets or gets the point here. */
        int objc, 
        Tcl_Obj *CONST objv[])
{
    if (objc == 0) {
        Tcl_Obj *obj = Tcl_NewObj();
        Tcl_Obj *subobj = Tcl_NewDoubleObj(pointPtr[0]);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(pointPtr[1]);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        Tcl_SetObjResult(interp, obj);
    } else if ((objc == 1) || (objc == 2)) {
        double x, y;
        
        if (objc==1) {
            if (Tcl_ListObjGetElements(interp, objv[0], &objc,
                    (Tcl_Obj ***) &objv) != TCL_OK) {
                return TCL_ERROR;
            } else if (objc != 2) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 0 or 2", -1));
                return TCL_ERROR;
            }
        }
        if ((Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[0], &x) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[1], &y) != TCL_OK)) {
            return TCL_ERROR;
        }
        pointPtr[0] = x;
        pointPtr[1] = y;
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 0 or 2", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * CoordsForRectangularItems --
 *
 *	Used as coordProc for items that have rectangular coords.
 *
 * Results:
 *	Standard tcl result.
 *
 * Side effects:
 *	May store new coords in rectPtr.
 *
 *--------------------------------------------------------------
 */

int		
CoordsForRectangularItems(
        Tcl_Interp *interp, 
        Tk_PathCanvas canvas, 
        PathRect *rectPtr, 		/* Sets or gets the box here. */
        int objc, 
        Tcl_Obj *CONST objv[])
{
    if (objc == 0) {
        Tcl_Obj *obj = Tcl_NewObj();
        Tcl_Obj *subobj = Tcl_NewDoubleObj(rectPtr->x1);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(rectPtr->y1);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(rectPtr->x2);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(rectPtr->y2);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        Tcl_SetObjResult(interp, obj);
    } else if ((objc == 1) || (objc == 4)) {
        double x1, y1, x2, y2;
        
        if (objc==1) {
            if (Tcl_ListObjGetElements(interp, objv[0], &objc,
                    (Tcl_Obj ***) &objv) != TCL_OK) {
                return TCL_ERROR;
            } else if (objc != 4) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 0 or 4", -1));
                return TCL_ERROR;
            }
        }
        if ((Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[0], &x1) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[1], &y1) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[2], &x2) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[3], &y2) != TCL_OK)) {
            return TCL_ERROR;
        }
        
        /*
         * Get an approximation of the path's bounding box
         * assuming zero width outline (stroke).
         * Normalize the corners!
         */
        rectPtr->x1 = MIN(x1, x2);
        rectPtr->y1 = MIN(y1, y2);
        rectPtr->x2 = MAX(x1, x2);
        rectPtr->y2 = MAX(y1, y2);
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 0 or 4", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * GetBareArcBbox
 *
 *	Gets an overestimate of the bounding box rectangle of
 * 	an arc defined using central parametrization assuming
 *	zero stroke width.
 * 	Untransformed coordinates!
 *	Note: 1) all angles clockwise direction!
 *	      2) all angles in radians.
 *
 * Results:
 *	A PathRect.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static PathRect
GetBareArcBbox(double cx, double cy, double rx, double ry,
        double theta1, double dtheta, double phi)
{
    PathRect r = {1.0e36, 1.0e36, -1.0e36, -1.0e36};	/* Empty rect. */
    double start, extent, stop, stop2PI;
    double cosStart, sinStart, cosStop, sinStop;
    
    /* Keep 0 <= start, extent < 2pi 
     * and 0 <= stop < 4pi */
    if (dtheta >= 0.0) {
        start = theta1;
        extent = dtheta;
    } else {
        start = theta1 + dtheta;
        extent = -1.0*dtheta;
    }
    if (start < 0.0) {
        start += 2.0*M_PI;
        if (start < 0.0) {
            start += 2.0*M_PI;
        }
    }
    if (start >= 2.0*M_PI) {
        start -= 2.0*M_PI;
    }
    stop = start + extent;
    stop2PI = stop - 2.0*M_PI;
    cosStart = cos(start);
    sinStart = sin(start);
    cosStop = cos(stop);
    sinStop = sin(stop);
    
    /*
     * Compute bbox for phi = 0.
     * Put everything at (0,0) and shift to (cx,cy) at the end.
     * Look for extreme points of arc:
     * 	1) start and stop points
     *	2) any intersections of x and y axes
     * Count both first and second "turns".
     */
                
    IncludePointInRect(&r, rx*cosStart, ry*sinStart);
    IncludePointInRect(&r, rx*cosStop,  ry*sinStop);
    if (((start < M_PI/2.0) && (stop > M_PI/2.0)) || (stop2PI > M_PI/2.0)) {
        IncludePointInRect(&r, 0.0, ry);
    }
    if (((start < M_PI) && (stop > M_PI)) || (stop2PI > M_PI)) {
        IncludePointInRect(&r, -rx, 0.0);
    }
    if (((start < 3.0*M_PI/2.0) && (stop > 3.0*M_PI/2.0)) || (stop2PI > 3.0*M_PI/2.0)) {
        IncludePointInRect(&r, 0.0, -ry);
    }
    if (stop > 2.0*M_PI) {
        IncludePointInRect(&r, rx, 0.0);
    }
    
    /*
     * Rotate the bbox above to get an overestimate of extremas.
     */
    if (fabs(phi) > 1e-6) {
        double cosPhi, sinPhi;
        double x, y;
        PathRect rrot = {1.0e36, 1.0e36, -1.0e36, -1.0e36};
        
        cosPhi = cos(phi);
        sinPhi = sin(phi);
        x = r.x1*cosPhi - r.y1*sinPhi;
        y = r.x1*sinPhi + r.y1*cosPhi;
        IncludePointInRect(&rrot, x, y);
        
        x = r.x2*cosPhi - r.y1*sinPhi;
        y = r.x2*sinPhi + r.y1*cosPhi;
        IncludePointInRect(&rrot, x, y);
        
        x = r.x1*cosPhi - r.y2*sinPhi;
        y = r.x1*sinPhi + r.y2*cosPhi;
        IncludePointInRect(&rrot, x, y);
        
        x = r.x2*cosPhi - r.y2*sinPhi;
        y = r.x2*sinPhi + r.y2*cosPhi;
        IncludePointInRect(&rrot, x, y);

        r = rrot;
    }
    
    /* Shift rect to arc center. */
    r.x1 += cx;
    r.y1 += cy;
    r.x2 += cx;
    r.y2 += cy;
    return r;
}

/*
 *--------------------------------------------------------------
 *
 * GetGenericBarePathBbox
 *
 *	Gets an overestimate of the bounding box rectangle of
 * 	a path assuming zero stroke width.
 * 	Untransformed coordinates!
 *
 * Results:
 *	A PathRect.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

PathRect
GetGenericBarePathBbox(PathAtom *atomPtr)
{
    double x1, y1, x2, y2, x3, y3, x4, y4, x5, y5;
    double currentX, currentY;
    PathRect r = {1.0e36, 1.0e36, -1.0e36, -1.0e36};
    
    currentX = 0.0;
    currentY = 0.0;

    while (atomPtr != NULL) {
    
        switch (atomPtr->type) {
            case PATH_ATOM_M: { 
                MoveToAtom *move = (MoveToAtom *) atomPtr;
                
                IncludePointInRect(&r, move->x, move->y);
                currentX = move->x;
                currentY = move->y;
                break;
            }
            case PATH_ATOM_L: {
                LineToAtom *line = (LineToAtom *) atomPtr;

                IncludePointInRect(&r, line->x, line->y);
                currentX = line->x;
                currentY = line->y;
                break;
            }
            case PATH_ATOM_A: {
                ArcAtom *arc = (ArcAtom *) atomPtr;
                int result;
                double cx, cy, rx, ry;
                double theta1, dtheta;
            
                result = EndpointToCentralArcParameters(
                        currentX, currentY,
                        arc->x, arc->y, arc->radX, arc->radY, 
                        DEGREES_TO_RADIANS * arc->angle, 
                        arc->largeArcFlag, arc->sweepFlag,
                        &cx, &cy, &rx, &ry,
                        &theta1, &dtheta);
                if (result == kPathArcLine) {
                    IncludePointInRect(&r, arc->x, arc->y);
                } else if (result == kPathArcOK) {
                    PathRect arcRect;
                    
                    arcRect = GetBareArcBbox(cx, cy, rx, ry, theta1, dtheta, 
                            DEGREES_TO_RADIANS * arc->angle);
                    IncludePointInRect(&r, arcRect.x1, arcRect.y1);
                    IncludePointInRect(&r, arcRect.x2, arcRect.y2);
                }
                currentX = arc->x;
                currentY = arc->y;
                break;
            }
            case PATH_ATOM_Q: {
                QuadBezierAtom *quad = (QuadBezierAtom *) atomPtr;
                
                x1 = (currentX + quad->ctrlX)/2.0;
                y1 = (currentY + quad->ctrlY)/2.0;
                x2 = (quad->ctrlX + quad->anchorX)/2.0;
                y2 = (quad->ctrlY + quad->anchorY)/2.0;
                IncludePointInRect(&r, x1, y1);
                IncludePointInRect(&r, x2, y2);
                currentX = quad->anchorX;
                currentY = quad->anchorY;
                IncludePointInRect(&r, currentX, currentY);
                break;
            }
            case PATH_ATOM_C: {
                CurveToAtom *curve = (CurveToAtom *) atomPtr;

                x1 = (currentX + curve->ctrlX1)/2.0;
                y1 = (currentY + curve->ctrlY1)/2.0;
                x2 = (curve->ctrlX1 + curve->ctrlX2)/2.0;
                y2 = (curve->ctrlY1 + curve->ctrlY2)/2.0;
                x3 = (curve->ctrlX2 + curve->anchorX)/2.0;
                y3 = (curve->ctrlY2 + curve->anchorY)/2.0;
                IncludePointInRect(&r, x1, y1);
                IncludePointInRect(&r, x3, y3);
                x4 = (x1 + x2)/2.0;
                y4 = (y1 + y2)/2.0;
                x5 = (x2 + x3)/2.0;
                y5 = (y2 + y3)/2.0;
                IncludePointInRect(&r, x4, y4);
                IncludePointInRect(&r, x5, y5);
                currentX = curve->anchorX;
                currentY = curve->anchorY;
                IncludePointInRect(&r, currentX, currentY);
                break;
            }
            case PATH_ATOM_Z: {
                /* empty */
                break;
            }
            case PATH_ATOM_ELLIPSE: {
                EllipseAtom *ell = (EllipseAtom *) atomPtr;
                IncludePointInRect(&r, ell->cx - ell->rx, ell->cy - ell->ry);
                IncludePointInRect(&r, ell->cx + ell->rx, ell->cy + ell->ry);            
                break;
            }
            case PATH_ATOM_RECT: {
                RectAtom *rect = (RectAtom *) atomPtr;
                IncludePointInRect(&r, rect->x, rect->y);
                IncludePointInRect(&r, rect->x + rect->width, rect->y + rect->height);            
                break;
            }
        }
        atomPtr = atomPtr->nextPtr;
    }
    return r;
}

static void
CopyPoint(double ptSrc[2], double ptDst[2])
{
    ptDst[0] = ptSrc[0];
    ptDst[1] = ptSrc[1];
}

/*
 *--------------------------------------------------------------
 *
 * PathGetMiterPoint --
 *
 *	Given three points forming an angle, compute the
 *	coordinates of the outside point of the mitered corner 
 *	formed by a line of a given width at that angle.
 *
 * Results:
 *	If the angle formed by the three points is less than
 *	11 degrees then 0 is returned and m isn't modified.
 *	Otherwise 1 is returned and the point of the "sharp"
 *	edge is returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
PathGetMiterPoint(
    double p1[],	/* Points to x- and y-coordinates of point
                         * before vertex. */
    double p0[],	/* Points to x- and y-coordinates of vertex
                         * for mitered joint. */
    double p2[],	/* Points to x- and y-coordinates of point
                         * after vertex. */
    double width,	/* Width of line.  */
    double sinThetaLimit,/* Sinus of theta miter limit. */
    double m[])		/* The miter point; the sharp edge. */
{
    double n1[2], n2[2];    /* The normalized vectors. */
    double len1, len2;
    double sinTheta;
    
    /*
     * A little geometry:
     *          p0
     *          /\
     *     n1  /  \ n2
     *        /    \
     *       p1    p2
     *
     * n1 = (p0-p1)/|p0-p1|
     * n2 = (p0-p2)/|p0-p2|
     *
     * theta is the angle between n1 and n2 which is identical
     * to the angle of the corner. We keep 0 <= theta <= PI so
     * that sin(theta) is never negative. If you consider the triangle
     * formed by the bisection (mid line) and any of the lines,
     * then theta/2 is the angle of that triangle.
     * Define:
     *
     * n = (n1+n2)/|n1+n2|
     *
     * Simple geometry gives: 
     *
     * |n1+n2| = 2cos(theta/2)
     *
     * and similar if d is the distance from p0 to the miter point:
     *
     * d = (w/2)/(sin(theta/2)
     *
     * where w is the line width.
     * For the miter point p we then get:
     *                   n1+n2            w/2
     * p = p0 + n*d = ------------- . ------------
     *                2cos(theta/2)   sin(theta/2)
     *
     * Using sin(2a) = 2sin(a)cos(a) we get:
     *
     * p = p0 + w/(2sin(theta)) * (n1 + n2)
     *
     * Use the cross product to get theta as: a x b = |a| |b| sin(angle) as:
     *
     * sin(theta) = |n1x*n2y - n1y*n2x|
     */
    
    /* n1 points from p1 to p0. */
    n1[0] = p0[0] - p1[0];
    n1[1] = p0[1] - p1[1];
    len1 = hypot(n1[0], n1[1]);
    if (len1 < 1e-6) {
        return 0;
    }
    n1[0] /= len1;
    n1[1] /= len1;

    /* n2 points from p2 to p0. */
    n2[0] = p0[0] - p2[0];
    n2[1] = p0[1] - p2[1];
    len2 = hypot(n2[0], n2[1]);
    if (len2 < 1e-6) {
        return 0;
    }
    n2[0] /= len2;
    n2[1] /= len2;
    
    sinTheta = fabs(n1[0]*n2[1] - n1[1]*n2[0]);
    if (sinTheta < sinThetaLimit) {
        return 0;
    }
    m[0] = p0[0] + width/(2.0*sinTheta) * (n1[0] + n2[0]);
    m[1] = p0[1] + width/(2.0*sinTheta) * (n1[1] + n2[1]);
    
    return 1;
}
 
static void
IncludeMiterPointsInRect(double p1[2], double p2[2], double p3[2], PathRect *bounds, 
        double width, double sinThetaLimit)
{
    double	m[2];

    if (PathGetMiterPoint(p1, p2, p3, width, sinThetaLimit, m)) {
        IncludePointInRect(bounds, m[0], m[1]);
    }
}

static PathRect
GetMiterBbox(PathAtom *atomPtr, double width, double miterLimit)
{
    int		npts;
    double 	p1[2], p2[2], p3[2];
    double	current[2], second[2];
    double 	sinThetaLimit;
    PathRect	bounds = {1.0e36, 1.0e36, -1.0e36, -1.0e36};
    
    npts = 0;
    current[0] = 0.0;
    current[1] = 0.0;
	second[0] = 0.0;
    second[1] = 0.0;
    
    /* Find sin(thetaLimit) which is needed to get miter points:
     * miterLimit = 1/sin(theta/2) =approx 2/theta
     */
    if (miterLimit > 8) {
        /* theta:
         * Exact:  0.250655662336
         * Approx: 0.25
         */
        sinThetaLimit = 2.0/miterLimit;
    } else if (miterLimit > 2) {
        sinThetaLimit = sin(2*asin(1.0/miterLimit));
    } else {
        return bounds;
    }
    
    while (atomPtr != NULL) {
    
        switch (atomPtr->type) {
            case PATH_ATOM_M: { 
                MoveToAtom *move = (MoveToAtom *) atomPtr;
                current[0] = move->x;
                current[1] = move->y;
                p1[0] = move->x;
                p1[1] = move->y;
                npts = 1;
                break;
            }
            case PATH_ATOM_L: {
                LineToAtom *line = (LineToAtom *) atomPtr;
                current[0] = line->x;
                current[1] = line->y;
                CopyPoint(p2, p3);
                CopyPoint(p1, p2);
                p1[0] = line->x;
                p1[1] = line->y;
                npts++;
                if (npts >= 3) {
                    IncludeMiterPointsInRect(p1, p2, p3, &bounds, width, sinThetaLimit);
                }
                break;
            }
            case PATH_ATOM_A: {
                ArcAtom *arc = (ArcAtom *) atomPtr;
                current[0] = arc->x;
                current[1] = arc->y;
                /* @@@ TODO */
                break;
            }
            case PATH_ATOM_Q: {
                QuadBezierAtom *quad = (QuadBezierAtom *) atomPtr;
                current[0] = quad->anchorX;
                current[1] = quad->anchorY;
                /* The control point(s) form the tangent lines at ends. */
                CopyPoint(p2, p3);
                CopyPoint(p1, p2);
                p1[0] = quad->ctrlX;
                p1[1] = quad->ctrlY;
                npts++;
                if (npts >= 3) {
                    IncludeMiterPointsInRect(p1, p2, p3, &bounds, width, sinThetaLimit);
                }
                CopyPoint(p1, p2);
                p1[0] = quad->anchorX;
                p1[1] = quad->anchorY;
                npts += 2;
                break;
            }
            case PATH_ATOM_C: {
                CurveToAtom *curve = (CurveToAtom *) atomPtr;
                current[0] = curve->anchorX;
                current[1] = curve->anchorY;
                /* The control point(s) form the tangent lines at ends. */
                CopyPoint(p2, p3);
                CopyPoint(p1, p2);
                p1[0] = curve->ctrlX1;
                p1[1] = curve->ctrlY1;
                npts++;
                if (npts >= 3) {
                    IncludeMiterPointsInRect(p1, p2, p3, &bounds, width, sinThetaLimit);
                }
                p1[0] = curve->ctrlX2;
                p1[1] = curve->ctrlY2;
                p1[0] = curve->anchorX;
                p1[1] = curve->anchorX;
                npts += 2;
                break;
            }
            case PATH_ATOM_Z: {
                CloseAtom *close = (CloseAtom *) atomPtr;
                current[0] = close->x;
                current[1] = close->y;
                CopyPoint(p2, p3);
                CopyPoint(p1, p2);
                p1[0] = close->x;
                p1[1] = close->y;
                npts++;
                if (npts >= 3) {
                    IncludeMiterPointsInRect(p1, p2, p3, &bounds, width, sinThetaLimit);
                }
                /* Check also the joint of first segment with the last segment. */
                CopyPoint(p2, p3);
                CopyPoint(p1, p2);
                CopyPoint(second, p1);
                if (npts >= 3) {
                    IncludeMiterPointsInRect(p1, p2, p3, &bounds, width, sinThetaLimit);
                }
                break;
            }
            case PATH_ATOM_ELLIPSE:
            case PATH_ATOM_RECT: {
                /* Empty. */
                break;
            }
        }
        if (npts == 2) {
            CopyPoint(current, second);
        }
        atomPtr = atomPtr->nextPtr;
    }
    
    return bounds;
}

/*
 *--------------------------------------------------------------
 *
 * GetGenericPathTotalBboxFromBare --
 *
 *	This procedure calculates the items total bbox from the 
 *	bare bbox. Untransformed coords!
 *
 * Results:
 *	PathRect.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

PathRect
GetGenericPathTotalBboxFromBare(PathAtom *atomPtr, Tk_PathStyle *stylePtr, PathRect *bboxPtr)
{
    double fudge = 1.0;
    double width = 0.0;
    PathRect rect = *bboxPtr;
        
    if (stylePtr->strokeColor != NULL) {
        width = stylePtr->strokeWidth;
        if (width < 1.0) {
            width = 1.0;
        }
        rect.x1 -= width;
        rect.x2 += width;
        rect.y1 -= width;
        rect.y2 += width;
    }
    
    /* Add the miter corners if necessary. */
    if (atomPtr && (stylePtr->joinStyle == JoinMiter) 
            && (stylePtr->strokeWidth > 1.0)) {
        PathRect miterBox;
        miterBox = GetMiterBbox(atomPtr, width, stylePtr->miterLimit);
        if (!IsPathRectEmpty(&miterBox)) {
            IncludePointInRect(&rect, miterBox.x1, miterBox.y1);
            IncludePointInRect(&rect, miterBox.x2, miterBox.y2);
        }
    }
    
    /*
     * Add one (or two if antialiasing) more pixel of fudge factor just to be safe 
     * (e.g. X may round differently than we do).
     */
     
    if (gAntiAlias) {
        fudge = 2;
    }
    rect.x1 -= fudge;
    rect.x2 += fudge;
    rect.y1 -= fudge;
    rect.y2 += fudge;
    
    return rect;
}

/*
 *--------------------------------------------------------------
 *
 * SetGenericPathHeaderBbox --
 *
 *	This procedure sets the (transformed) bbox in the items header.
 *	It is a (too?) conservative measure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header
 *	for itemPtr.
 *
 *--------------------------------------------------------------
 */

void
SetGenericPathHeaderBbox(
        Tk_PathItem *headerPtr,
        TMatrix *mPtr,
        PathRect *totalBboxPtr)
{
    PathRect rect;
    
    rect = *totalBboxPtr;

    if (mPtr != NULL) {
        double x, y;
        PathRect r = NewEmptyPathRect();

        /* Take each four corners in turn. */
        x = rect.x1, y = rect.y1;
        PathApplyTMatrix(mPtr, &x, &y);
        IncludePointInRect(&r, x, y);

        x = rect.x2, y = rect.y1;
        PathApplyTMatrix(mPtr, &x, &y);
        IncludePointInRect(&r, x, y);

        x = rect.x1, y = rect.y2;
        PathApplyTMatrix(mPtr, &x, &y);
        IncludePointInRect(&r, x, y);

        x = rect.x2, y = rect.y2;
        PathApplyTMatrix(mPtr, &x, &y);
        IncludePointInRect(&r, x, y);
        rect = r;  
    }
    headerPtr->x1 = (int) rect.x1;
    headerPtr->x2 = (int) rect.x2;
    headerPtr->y1 = (int) rect.y1;
    headerPtr->y2 = (int) rect.y2;
}

/*
 *--------------------------------------------------------------
 *
 * GenericPathToPoint --
 *
 *	Computes the distance from a given point to a given
 *	line, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are pointPtr[0] and pointPtr[1] is inside the line.  If the
 *	point isn't inside the line then the return value is the
 *	distance from the point to the line.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
GenericPathToPoint(
    Tk_PathCanvas canvas,	/* Canvas containing item. */
    Tk_PathItem *itemPtr,	/* Item to check against point. */
    Tk_PathStyle *stylePtr,
    PathAtom *atomPtr,
    int maxNumSegments,
    double *pointPtr)		/* Pointer to x and y coordinates. */
{
    int		    numPoints, numStrokes;
    int		    isclosed;
    int		    intersections, nonzerorule;
    int		    sumIntersections = 0, sumNonzerorule = 0;
    double	    *polyPtr;
    double	    bestDist, radius, width, dist;
    Tk_PathState    state = itemPtr->state;
    TMatrix	    *matrixPtr = stylePtr->matrixPtr;

    bestDist = 1.0e36;

    if (state == TK_PATHSTATE_HIDDEN) {
        return bestDist;
    }
    if (!HaveAnyFillFromPathColor(stylePtr->fill) && (stylePtr->strokeColor == NULL)) {
        return bestDist;
    }
    if (atomPtr == NULL) {
        return bestDist;
    }
    
    /* 
     * Do we need more memory or can we use static space? 
     */
    if (maxNumSegments > MAX_NUM_STATIC_SEGMENTS) {
        polyPtr = (double *) ckalloc((unsigned) (2*maxNumSegments*sizeof(double)));
    } else {
        polyPtr = staticSpace;
    }
    width = stylePtr->strokeWidth;
    if (width < 1.0) {
        width = 1.0;
    }
    radius = width/2.0;

    /*
     * Loop through each subpath, creating the approximate polyline,
     * and do the *ToPoint functions.
     *
     * Note: Strokes can be treated independently for each subpath,
     *		 but fills cannot since subpaths may intersect creating
     *		 "holes".
     */
     
    while (atomPtr != NULL) {
        MakeSubPathSegments(&atomPtr, polyPtr, &numPoints, &numStrokes, matrixPtr);
        isclosed = 0;
        if (numStrokes == numPoints) {
            isclosed = 1;
        }        

        /*
         * This gives the min distance to the *stroke* AND the
         * number of intersections of the two types.
         */
        dist = PathPolygonToPointEx(polyPtr, numPoints, pointPtr, 
                &intersections, &nonzerorule);
        sumIntersections += intersections;
        sumNonzerorule += nonzerorule;
        if ((stylePtr->strokeColor != NULL) && (stylePtr->strokeWidth <= kPathStrokeThicknessLimit)) {
        
            /*
             * This gives the distance to a zero width polyline.
             * Use a simple scheme to adjust for a small width.
             */
            dist -= radius;
        }
        if (dist < bestDist) {
            bestDist = dist;
        }
        if (bestDist <= 0.0) {
            bestDist = 0.0;
            goto done;
        }

        /*
         * For wider strokes we must make a more detailed analysis.
         * Yes, there is an infinitesimal overlap to the above just
         * to be on the safe side.
         */
        if ((stylePtr->strokeColor != NULL) && (stylePtr->strokeWidth >= kPathStrokeThicknessLimit)) {
            dist = PathThickPolygonToPoint(stylePtr->joinStyle, stylePtr->capStyle, 
                    width, isclosed, polyPtr, numPoints, pointPtr);
            if (dist < bestDist) {
                bestDist = dist;
            }
            if (bestDist <= 0.0) {
                bestDist = 0.0;
                goto done;
            }
        }
    }        

    /*
     * We've processed all of the points.  
     * EvenOddRule: If the number of intersections is odd, 
     *			the point is inside the polygon.
     * WindingRule (nonzero): If the number of directed intersections
     *			are nonzero, then inside.
     */
    if (HaveAnyFillFromPathColor(stylePtr->fill)) {
        if ((stylePtr->fillRule == EvenOddRule) && (sumIntersections & 0x1)) {
            bestDist = 0.0;
        } else if ((stylePtr->fillRule == WindingRule) && (sumNonzerorule != 0)) {
            bestDist = 0.0;
        }
    }
    
done:
    if (polyPtr != staticSpace) {
        ckfree((char *) polyPtr);
    }
    return bestDist;
}

/*
 *--------------------------------------------------------------
 *
 * GenericPathToArea --
 *
 *	This procedure is called to determine whether an item
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangular area.
 *	
 *	Each subpath is treated in turn. Generate straight line
 *	segments for each subpath and treat it as a polygon.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the
 *	area, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
GenericPathToArea(
    Tk_PathCanvas canvas,   /* Canvas containing item. */
    Tk_PathItem *itemPtr,   /* Item to check against line. */
    Tk_PathStyle *stylePtr,
    PathAtom *atomPtr,
    int maxNumSegments,
    double *areaPtr)	    /* Pointer to array of four coordinates
                             * (x1, y1, x2, y2) describing rectangular
                             * area.  */
{
    int inside;		    /* Tentative guess about what to return,
                             * based on all points seen so far:  one
                             * means everything seen so far was
                             * inside the area;  -1 means everything
                             * was outside the area.  0 means overlap
                             * has been found. */ 
    int		    numPoints = 0;
    int		    numStrokes = 0;
    int		    isclosed = 0;
    double	    *polyPtr;
    double	    currentT[2];
    Tk_PathState    state = itemPtr->state;
    TMatrix	    *matrixPtr = stylePtr->matrixPtr;
    MoveToAtom	    *move;

#if 0
    if(state == TK_PATHSTATE_NULL) {
        state = TkPathCanvasState(canvas);
    }
#endif
    if (state == TK_PATHSTATE_HIDDEN) {
        return -1;
    }
    if ((GetColorFromPathColor(stylePtr->fill) == NULL) && (stylePtr->strokeColor == NULL)) {
        return -1;
    }
    if (atomPtr == NULL) {
        return -1;
    }
    
    /* 
     * Do we need more memory or can we use static space? 
     */
    if (maxNumSegments > MAX_NUM_STATIC_SEGMENTS) {
        polyPtr = (double *) ckalloc((unsigned) (2*maxNumSegments*sizeof(double)));
    } else {
        polyPtr = staticSpace;
    }

    /* A 'M' atom must be first, may show up later as well. */
    if (atomPtr->type != PATH_ATOM_M) {
        return -1;
    }
    move = (MoveToAtom *) atomPtr;
    PathApplyTMatrixToPoint(matrixPtr, &(move->x), currentT);
    
    /*
     * This defines the starting point. It is either -1 or 1. 
     * If any subseqent segment has a different 'inside'
     * then return 0 since one port (in|out)side and another
     * (out|in)side
     */
    inside = -1;
    if ((currentT[0] >= areaPtr[0]) && (currentT[0] <= areaPtr[2])
            && (currentT[1] >= areaPtr[1]) && (currentT[1] <= areaPtr[3])) {
        inside = 1;
    }
    
    while (atomPtr != NULL) {
        MakeSubPathSegments(&atomPtr, polyPtr, &numPoints, &numStrokes, matrixPtr);
        isclosed = 0;
        if (numStrokes == numPoints) {
            isclosed = 1;
        }        
        if (SubPathToArea(stylePtr, polyPtr, numPoints, numStrokes, 
                areaPtr, inside) != inside) {
            inside = 0;
            goto done;
        }
    }

done:
    if (polyPtr != staticSpace) {
        ckfree((char *) polyPtr);
    }
    return inside;
}

/*
 *--------------------------------------------------------------
 *
 * ArcSegments --
 *
 *	Given the arc parameters it makes a sequence if line segments.
 *	All angles in radians!
 *	Note that segments are transformed!
 *
 * Results:
 *	The array at *coordPtr gets filled in with 2*numSteps
 *	coordinates, which correspond to the arc.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
ArcSegments(
    CentralArcPars *arcPars,
    TMatrix *matrixPtr,
    int includeFirst,		/* Should the first point be included? */
    int numSteps,		/* Number of curve segments to
                                 * generate.  */
    register double *coordPtr)	/* Where to put new points. */
{
    int i;
    int istart = 1 - includeFirst;
    double cosPhi, sinPhi;
    double cosAlpha, sinAlpha;
    double alpha, dalpha, theta1;
    double cx, cy, rx, ry;
    
    cosPhi = cos(arcPars->phi);
    sinPhi = sin(arcPars->phi);
    cx = arcPars->cx;
    cy = arcPars->cy;
    rx = arcPars->rx;
    ry = arcPars->ry;
    theta1 = arcPars->theta1;
    dalpha = arcPars->dtheta/numSteps;

    for (i = istart; i <= numSteps; i++, coordPtr += 2) {
        alpha = theta1 + i*dalpha;
        cosAlpha = cos(alpha);
        sinAlpha = sin(alpha);
        coordPtr[0] = cx + rx*cosAlpha*cosPhi - ry*sinAlpha*sinPhi;
        coordPtr[1] = cy + rx*cosAlpha*sinPhi + ry*sinAlpha*cosPhi;
        PathApplyTMatrix(matrixPtr, coordPtr, coordPtr+1);
    }
}

/* 
 * Get maximum number of segments needed to describe path. 
 * Needed to see if we can use static space or need to allocate more.
 */

static int
GetArcNumSegments(double currentX, double currentY, ArcAtom *arc)
{
    int result;
    int ntheta, nlength;
    int numSteps;			/* Number of curve points to
					 * generate.  */
    double cx, cy, rx, ry;
    double theta1, dtheta;

    result = EndpointToCentralArcParameters(
            currentX, currentY,
            arc->x, arc->y, arc->radX, arc->radY, 
            DEGREES_TO_RADIANS * arc->angle, 
            arc->largeArcFlag, arc->sweepFlag,
            &cx, &cy, &rx, &ry,
            &theta1, &dtheta);
    if (result == kPathArcLine) {
        return 2;
    } else if (result == kPathArcSkip) {
        return 0;
    }

    /* Estimate the number of steps needed. 
     * Max 10 degrees or length 50.
     */
    ntheta = (int) (dtheta/5.0 + 0.5);
    nlength = (int) (0.5*(rx + ry)*dtheta/50 + 0.5);
    numSteps = MAX(4, MAX(ntheta, nlength));;
    return numSteps;
}

/*
 *--------------------------------------------------------------
 *
 * CurveSegments --
 *
 *	Given four control points, create a larger set of points
 *	for a cubic Bezier spline based on the points.
 *
 * Results:
 *	The array at *coordPtr gets filled in with 2*numSteps
 *	coordinates, which correspond to the Bezier spline defined
 *	by the four control points.  
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
CurveSegments(
    double control[],		/* Array of coordinates for four
                             * control points:  x0, y0, x1, y1,
                             * ... x3 y3. */
    int includeFirst,		/* Should the first point be included? */
    int numSteps,			/* Number of curve segments to
                             * generate.  */
    register double *coordPtr)		/* Where to put new points. */
{
    int i;
    int istart = 1 - includeFirst;
    double u, u2, u3, t, t2, t3;
    
    /*
     * We should use the 'de Castlejau' algorithm to iterate
     * line segments until a certain tolerance.
     */

    for (i = istart; i <= numSteps; i++, coordPtr += 2) {
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
 * QuadBezierSegments --
 *
 *	Given three control points, create a larger set of points
 *	for a quadratic Bezier spline based on the points.
 *
 * Results:
 *	The array at *coordPtr gets filled in with 2*numSteps
 *	coordinates, which correspond to the quadratic Bezier spline defined
 *	by the control points.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
QuadBezierSegments(
    double control[],			/* Array of coordinates for three
                                 * control points:  x0, y0, x1, y1,
                                 * x2, y2. */
    int includeFirst,			/* Should the first point be included? */
    int numSteps,				/* Number of curve segments to
                                 * generate.  */
    register double *coordPtr)	/* Where to put new points. */
{
    int i;
    int istart = 1 - includeFirst;
    double u, u2, t, t2;

    for (i = istart; i <= numSteps; i++, coordPtr += 2) {
        t = ((double) i)/((double) numSteps);
        t2 = t*t;
        u = 1.0 - t;
        u2 = u*u;
        coordPtr[0] = control[0]*u2 + 2.0 * control[2]*t*u + control[4]*t2;
        coordPtr[1] = control[1]*u2 + 2.0 * control[3]*t*u + control[5]*t2;
    }
}

static void
EllipseSegments(
    double center[],
    double rx, double ry,
    double angle,				/* Angle of rotated ellipse. */
    int numSteps,				/* Number of curve segments to
                                 * generate.  */
    register double *coordPtr)	/* Where to put new points. */
{
    double phi, delta;
    double cosA, sinA;
    double cosPhi, sinPhi;

    cosA = cos(angle);
    sinA = sin(angle);
    delta = 2*M_PI/(numSteps-1);
    
    for (phi = 0.0; phi <= 2*M_PI+1e-6; phi += delta, coordPtr += 2) {
        cosPhi = cos(phi);
        sinPhi = sin(phi);
        coordPtr[0] = center[0] + rx*cosA*cosPhi - ry*sinA*sinPhi;
        coordPtr[1] = center[1] + rx*sinA*cosPhi + ry*cosA*sinPhi;
    }
}

/*
 *--------------------------------------------------------------
 *
 * AddArcSegments, AddQuadBezierSegments, AddCurveToSegments,
 *   AddEllipseToSegments --
 *
 *	Adds a number of points along the arc (curve) to coordPtr
 *	representing straight line segments.
 *
 * Results:
 *	Number of points added. 
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
AddArcSegments(
    TMatrix *matrixPtr,
    double current[2],		/* Current point. */
    ArcAtom *arc,
    double *coordPtr)		/* Where to put the points. */
{
    int result;
    int numPoints;
    CentralArcPars arcPars;
    double cx, cy, rx, ry;
    double theta1, dtheta;
            
    /*
     * Note: The arc parametrization used cannot generally
     * be transformed. Need to transform each line segment separately!
     */
    
    result = EndpointToCentralArcParameters(
            current[0], current[1],
            arc->x, arc->y, arc->radX, arc->radY, 
            DEGREES_TO_RADIANS * arc->angle, 
            arc->largeArcFlag, arc->sweepFlag,
            &cx, &cy, &rx, &ry,
            &theta1, &dtheta);
    if (result == kPathArcLine) {
        double pts[2];

        pts[0] = arc->x;
        pts[1] = arc->y;
        PathApplyTMatrix(matrixPtr, pts, pts+1);
        coordPtr[0] = pts[0];
        coordPtr[1] = pts[1];
        return 1;
    } else if (result == kPathArcSkip) {
        return 0;
    }

    arcPars.cx = cx;
    arcPars.cy = cy;
    arcPars.rx = rx;
    arcPars.ry = ry;
    arcPars.theta1 = theta1;
    arcPars.dtheta = dtheta;
    arcPars.phi = arc->angle;

    numPoints = GetArcNumSegments(current[0], current[1], arc);    
    ArcSegments(&arcPars, matrixPtr, 0, numPoints, coordPtr);

    return numPoints;
}

static int
AddQuadBezierSegments(
    TMatrix *matrixPtr,
    double current[2],		/* Current point. */
    QuadBezierAtom *quad,
    double *coordPtr)		/* Where to put the points. */
{
    int numPoints;			/* Number of curve points to
                             * generate.  */
    double control[6];

    PathApplyTMatrixToPoint(matrixPtr, current, control);
    PathApplyTMatrixToPoint(matrixPtr, &(quad->ctrlX), control+2);
    PathApplyTMatrixToPoint(matrixPtr, &(quad->anchorX), control+4);

    numPoints = kPathNumSegmentsQuadBezier;
    QuadBezierSegments(control, 0, numPoints, coordPtr);

    return numPoints;
}

static int
AddCurveToSegments(
    TMatrix *matrixPtr,
    double current[2],			/* Current point. */
    CurveToAtom *curve,
    double *coordPtr)
{
    int numSteps;				/* Number of curve points to
                                 * generate.  */
    double control[8];

    PathApplyTMatrixToPoint(matrixPtr, current, control);
    PathApplyTMatrixToPoint(matrixPtr, &(curve->ctrlX1), control+2);
    PathApplyTMatrixToPoint(matrixPtr, &(curve->ctrlX2), control+4);
    PathApplyTMatrixToPoint(matrixPtr, &(curve->anchorX), control+6);

    numSteps = kPathNumSegmentsCurveTo;
    CurveSegments(control, 1, numSteps, coordPtr);
    
    return numSteps;
}

static int
AddEllipseToSegments(
    TMatrix *matrixPtr,
    EllipseAtom *ellipse,
    double *coordPtr)
{
    int numSteps;
    double rx, ry, angle;
    double c[2], crx[2], cry[2];
    double p[2];

    /* 
     * We transform the three points: c, c+rx, c+ry
     * and then compute the parameters for the transformed ellipse.
     * This is because an affine transform of an ellipse is still an ellipse.
     */
    p[0] = ellipse->cx;
    p[1] = ellipse->cy;
    PathApplyTMatrixToPoint(matrixPtr, p, c);
    p[0] = ellipse->cx + ellipse->rx;
    p[1] = ellipse->cy;
    PathApplyTMatrixToPoint(matrixPtr, p, crx);
    p[0] = ellipse->cx;
    p[1] = ellipse->cy + ellipse->ry;
    PathApplyTMatrixToPoint(matrixPtr, p, cry);
    rx = hypot(crx[0]-c[0], crx[1]-c[1]);
    ry = hypot(cry[0]-c[0], cry[1]-c[1]);
    angle = atan2(crx[1]-c[1], crx[0]-c[0]);
    
    /* Note we add 1 here since we need both start and stop points. 
     * Small things wont need so many segments.
     * Approximate circumference: 4(rx+ry)
     */
    if (rx+ry < 2.1) {
        numSteps = 1;
    } else if (rx+ry < 4) {
        numSteps = 3;
    } else if (rx+ry < kPathNumSegmentsEllipse) {
        numSteps = (int)(rx+ry+2);
    } else {
        numSteps = kPathNumSegmentsEllipse + 1;
    }
    EllipseSegments(c, rx, ry, angle, numSteps, coordPtr);

    return numSteps;
}

static int
AddRectToSegments(
    TMatrix *matrixPtr,
    RectAtom *rect,
    double *coordPtr)
{
    int i;
    double p[8];
    
    p[0] = rect->x;
    p[1] = rect->y;
    p[2] = rect->x + rect->width;
    p[3] = rect->y;
    p[4] = rect->x + rect->width;
    p[5] = rect->y + rect->height;
    p[6] = rect->x;
    p[7] = rect->y + rect->height;
    
    for (i = 0; i <= 8; i += 2, coordPtr += 2) {
        PathApplyTMatrix(matrixPtr, p, p+1);
        coordPtr[0] = p[0];
        coordPtr[1] = p[1];
    }
    return 4;
}

/*
 *--------------------------------------------------------------
 *
 * MakeSubPathSegments --
 *
 *	Supposed to be a generic segment generator that can be used 
 *	by both Area and Point functions.
 *
 * Results:
 *	Points filled into polyPtr...
 *
 * Side effects:
 *	Pointer *atomPtrPtr may be updated.
 *
 *--------------------------------------------------------------
 */

static void
MakeSubPathSegments(PathAtom **atomPtrPtr, double *polyPtr, 
        int *numPointsPtr, int *numStrokesPtr, TMatrix *matrixPtr)
{
    int 	first = 1;
    int		numPoints;
    int		numStrokes;
    int		numAdded;
    int		isclosed = 0;
    double 	current[2];	/* Current untransformed point. */
    double	*currentTPtr;	/* Pointer to the transformed current point. */
    double	*coordPtr;
    PathAtom 	*atomPtr;
    
    /* @@@ 	Note that for unfilled paths we could have made a progressive
     *     	area (point) check which may be faster since we may stop when 0 (overlapping).
     *	   	For filled paths we cannot rely on this since the area rectangle
     *		may be entirely enclosed in the path and still overlapping.
     *		(Need better explanation!)
     */
    
    /*
     * Check each segment of the path.
     * Any transform matrix is applied at the last stage when comparing to rect.
     * 'current' is always untransformed coords.
     */

    current[0] = 0.0;
    current[1] = 0.0;
    numPoints = 0;
    numStrokes = 0;
    isclosed = 0;
    atomPtr = *atomPtrPtr;
    coordPtr = NULL;
    
    while (atomPtr != NULL) {

        switch (atomPtr->type) {
            case PATH_ATOM_M: {
                MoveToAtom *move = (MoveToAtom *) atomPtr;
            
                /* A 'M' atom must be first, may show up later as well. */
                
                if (first) {
                    coordPtr = polyPtr;
                    current[0] = move->x;
                    current[1] = move->y;
                    PathApplyTMatrixToPoint(matrixPtr, current, coordPtr);
                    currentTPtr = coordPtr;
                    coordPtr += 2;
                    numPoints = 1;
                } else {
                
                    /*  
                     * We have finalized a subpath.
                     */
                    goto done;
                }
                first = 0;
                break;
            }
            case PATH_ATOM_L: {
                LineToAtom *line = (LineToAtom *) atomPtr;
                
                PathApplyTMatrixToPoint(matrixPtr, &(line->x), coordPtr);
                current[0] = line->x;
                current[1] = line->y;
                currentTPtr = coordPtr;
                coordPtr += 2;
                numPoints++;;
                break;
            }
            case PATH_ATOM_A: {
                ArcAtom *arc = (ArcAtom *) atomPtr;
                
                numAdded = AddArcSegments(matrixPtr, current, arc, coordPtr);
                coordPtr += 2 * numAdded;
                numPoints += numAdded;
                current[0] = arc->x;
                current[1] = arc->y;
                currentTPtr = coordPtr;
                break;
            }
            case PATH_ATOM_Q: {
                QuadBezierAtom *quad = (QuadBezierAtom *) atomPtr;
                
                numAdded = AddQuadBezierSegments(matrixPtr, current,
                        quad, coordPtr);
                coordPtr += 2 * numAdded;
                numPoints += numAdded;
                current[0] = quad->anchorX;
                current[1] = quad->anchorY;
                currentTPtr = coordPtr;
                break;
            }
            case PATH_ATOM_C: {
                CurveToAtom *curve = (CurveToAtom *) atomPtr;
                
                numAdded = AddCurveToSegments(matrixPtr, current,
                        curve, coordPtr);
                coordPtr += 2 * numAdded;
                numPoints += numAdded;
                current[0] = curve->anchorX;
                current[1] = curve->anchorY;
                currentTPtr = coordPtr;
                break;
            }
            case PATH_ATOM_Z: {
                CloseAtom *close = (CloseAtom *) atomPtr;
            
                /* Just add the first point to the end. */
                coordPtr[0] = polyPtr[0];
                coordPtr[1] = polyPtr[1];
                coordPtr += 2;
                numPoints++;
                current[0]  = close->x;
                current[1]  = close->y;
                isclosed = 1;
                break;
            }
            case PATH_ATOM_ELLIPSE: {
                EllipseAtom *ellipse = (EllipseAtom *) atomPtr;

                if (first) {
                    coordPtr = polyPtr;
                }
                numAdded = AddEllipseToSegments(matrixPtr, ellipse, coordPtr);
                coordPtr += 2 * numAdded;
                numPoints += numAdded;
                if (first) {
                    /* Not sure about this. Never used anyway! */
                    current[0]  = ellipse->cx + ellipse->rx;
                    current[1]  = ellipse->cy;
                }
                break;
            }
            case PATH_ATOM_RECT: {
                RectAtom *rect = (RectAtom *) atomPtr;
                
                if (first) {
                    coordPtr = polyPtr;
                }
                numAdded = AddRectToSegments(matrixPtr, rect, coordPtr);
                coordPtr += 2 * numAdded;
                numPoints += numAdded;
                current[0] = rect->x;
                current[1] = rect->y;
                break;
            }
        }
        atomPtr = atomPtr->nextPtr;
    }

done:
    if (numPoints > 1) {
        if (isclosed) {
            numStrokes = numPoints;
        } else {
            numStrokes = numPoints - 1;
        }
    }
    *numPointsPtr = numPoints;
    *numStrokesPtr = numStrokes;
    *atomPtrPtr = atomPtr;

    return;
}

/*
 *--------------------------------------------------------------
 *
 * SubPathToArea --
 *
 *	This procedure is called to determine whether a subpath
 *	lies entirely inside, entirely outside, or overlapping
 *	a given rectangular area.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the
 *	area, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
SubPathToArea(
    Tk_PathStyle *stylePtr,
    double 	*polyPtr, 
    int 	numPoints, 	/* Total number of points. First one
                                 * is duplicated in the last. */
    int		numStrokes,	/* The number of strokes which is one less
                                 * than numPoints if path not closed. */
    double 	*rectPtr, 
    int 	inside)		/* This is the current inside status. */
{
    double width;
    
    /* @@@ There is an open question how a closed unfilled polygon
     *	completely enclosing the area rect should be counted.
     *	While the tk canvas polygon item counts it as intersecting (0),
     *	the line item counts it as outside (-1).
     */
    
    if (HaveAnyFillFromPathColor(stylePtr->fill)) {
    
        /* This checks a closed polygon with zero width for inside.
         * If area rect completely enclosed it returns intersecting (0).
         */
        if (TkPolygonToArea(polyPtr, numPoints, rectPtr) != inside) {
            return 0;
        }
    }
    if (stylePtr->strokeColor != NULL) {
        width = stylePtr->strokeWidth;
        if (width < 1.0) {
            width = 1.0;
        }
        if (stylePtr->strokeWidth > kPathStrokeThicknessLimit) {
            if (TkThickPolyLineToArea(polyPtr, numPoints, 
                    width, stylePtr->capStyle, 
                    stylePtr->joinStyle, rectPtr) != inside) {
                return 0;
            }
        } else {
	    if (PathPolyLineToArea(polyPtr, numPoints, rectPtr) != inside) {
                return 0;
            }
        }
    }
    return inside;
}

/*---------------------------*/

/*
 *--------------------------------------------------------------
 *
 * TranslatePathAtoms --
 *
 *	This procedure is called to translate a linked list of path atoms.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Path atoms changed.
 *
 *--------------------------------------------------------------
 */

void
TranslatePathAtoms(
    PathAtom *atomPtr,
    double deltaX,				/* Amount by which item is to be */
    double deltaY)              /* moved. */
{
    while (atomPtr != NULL) {
        switch (atomPtr->type) {
            case PATH_ATOM_M: {
                MoveToAtom *move = (MoveToAtom *) atomPtr;
                
                move->x += deltaX;
                move->y += deltaY;
                break;
            }
            case PATH_ATOM_L: {
                LineToAtom *line = (LineToAtom *) atomPtr;
                
                line->x += deltaX;
                line->y += deltaY;
                break;
            }
            case PATH_ATOM_A: {
                ArcAtom *arc = (ArcAtom *) atomPtr;
                
                arc->x += deltaX;
                arc->y += deltaY;
                break;
            }
            case PATH_ATOM_Q: {
                QuadBezierAtom *quad = (QuadBezierAtom *) atomPtr;
                
                quad->ctrlX += deltaX;
                quad->ctrlY += deltaY;
                quad->anchorX += deltaX;
                quad->anchorY += deltaY;
                break;
            }
            case PATH_ATOM_C: {
                CurveToAtom *curve = (CurveToAtom *) atomPtr;

                curve->ctrlX1 += deltaX;
                curve->ctrlY1 += deltaY;
                curve->ctrlX2 += deltaX;
                curve->ctrlY2 += deltaY;
                curve->anchorX += deltaX;
                curve->anchorY += deltaY;
                break;
            }
            case PATH_ATOM_Z: {
                CloseAtom *close = (CloseAtom *) atomPtr;
                
                close->x += deltaX;
                close->y += deltaY;
                break;
            }
            case PATH_ATOM_ELLIPSE:
            case PATH_ATOM_RECT: {
                Tcl_Panic("PATH_ATOM_ELLIPSE PATH_ATOM_RECT are not supported for TranslatePathAtoms");
                break;
            }
        }
        atomPtr = atomPtr->nextPtr;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ScalePathAtoms --
 *
 *	This procedure is called to scale a linked list of path atoms.
 *	The following transformation is applied to all point
 *	coordinates:
 *	x' = originX + scaleX*(x-originX)
 *	y' = originY + scaleY*(y-originY)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Path atoms changed.
 *
 *--------------------------------------------------------------
 */

void
ScalePathAtoms(
    PathAtom *atomPtr,
    double originX, double originY,	/* Origin about which to scale rect. */
    double scaleX,			/* Amount to scale in X direction. */
    double scaleY)			/* Amount to scale in Y direction. */
{
    while (atomPtr != NULL) {
        switch (atomPtr->type) {
            case PATH_ATOM_M: {
                MoveToAtom *move = (MoveToAtom *) atomPtr;
                
                move->x = originX + scaleX*(move->x - originX);
                move->y = originY + scaleY*(move->y - originY);
                break;
            }
            case PATH_ATOM_L: {
                LineToAtom *line = (LineToAtom *) atomPtr;
                
                line->x = originX + scaleX*(line->x - originX);
                line->y = originY + scaleY*(line->y - originY);
                break;
            }
            case PATH_ATOM_A: {
                ArcAtom *arc = (ArcAtom *) atomPtr;
		/*
		 * @@@ TODO: This is a very much simplified math which is WRONG!
		 */
		if (fabs(fmod(arc->angle, 180.0)) < 0.001) {
		    arc->radX = scaleX*arc->radX;
		    arc->radY = scaleY*arc->radY;
		} else if (fabs(fmod(arc->angle, 90.0)) < 0.001) {
		    arc->radX = scaleY*arc->radX;
		    arc->radY = scaleX*arc->radY;
		} else {
		    double angle;
		    double nx, ny;
		    
		    if (scaleX == 0.0) Tcl_Panic("singularity when scaling arc atom");
		    angle = atan(scaleY/scaleX * tan(arc->angle * DEGREES_TO_RADIANS));
		    nx = cos(arc->angle * DEGREES_TO_RADIANS);
		    ny = sin(arc->angle * DEGREES_TO_RADIANS);
		    
		    arc->angle = angle * RADIANS_TO_DEGREES;
		    arc->radX = arc->radX * hypot( scaleX*nx, scaleY*ny);
		    arc->radY = arc->radY * hypot(-scaleX*ny, scaleY*nx);
		    /* DebugPrintf(gInterp, 1, "arc->angle=%f, nx=%f, ny=%f, arc->radX=%f, arc->radY=%f\n", 
			    arc->angle, nx, ny, arc->radX, arc->radY); */

		}
		arc->x = originX + scaleX*(arc->x - originX);
		arc->y = originY + scaleY*(arc->y - originY);
                break;
            }
            case PATH_ATOM_Q: {
                QuadBezierAtom *quad = (QuadBezierAtom *) atomPtr;
                
                quad->ctrlX = originX + scaleX*(quad->ctrlX - originX);
                quad->ctrlY = originY + scaleY*(quad->ctrlY - originY);
                quad->anchorX = originX + scaleX*(quad->anchorX - originX);
                quad->anchorY = originY + scaleY*(quad->anchorY - originY);
                break;
            }
            case PATH_ATOM_C: {
                CurveToAtom *curve = (CurveToAtom *) atomPtr;

                curve->ctrlX1 = originX + scaleX*(curve->ctrlX1 - originX);
                curve->ctrlY1 = originY + scaleY*(curve->ctrlY1 - originY);
                curve->ctrlX2 = originX + scaleX*(curve->ctrlX2 - originX);
                curve->ctrlY2 = originY + scaleY*(curve->ctrlY2 - originY);
                curve->anchorX = originX + scaleX*(curve->anchorX - originX);
                curve->anchorY = originY + scaleY*(curve->anchorY - originY);
                break;
            }
            case PATH_ATOM_Z: {
                CloseAtom *close = (CloseAtom *) atomPtr;
                
                close->x = originX + scaleX*(close->x - originX);
                close->y = originY + scaleY*(close->y - originY);
                break;
            }
            case PATH_ATOM_ELLIPSE:
            case PATH_ATOM_RECT: {
                Tcl_Panic("PATH_ATOM_ELLIPSE PATH_ATOM_RECT are not supported for ScalePathAtoms");
                break;
            }
        }
        atomPtr = atomPtr->nextPtr;
    }
}

/*------------------*/

TMatrix
GetCanvasTMatrix(Tk_PathCanvas canvas)
{
    short originX, originY;
    TMatrix m = kPathUnitTMatrix;
    
    /* @@@ Any scaling involved as well??? */
    Tk_PathCanvasDrawableCoords(canvas, 0.0, 0.0, &originX, &originY);
    m.tx = originX;
    m.ty = originY;    
    return m;
}

PathRect
NewEmptyPathRect(void)
{
    PathRect r;
    
    r.x1 = 1.0e36;
    r.y1 = 1.0e36;
    r.x2 = -1.0e36;
    r.y2 = -1.0e36;
    return r;
}

int
IsPathRectEmpty(PathRect *r)
{
    if ((r->x2 >= r->x1) && (r->y2 >= r->y1)) {
        return 0;
    } else {
        return 1;
    }
}

void
IncludePointInRect(PathRect *r, double x, double y)
{
    r->x1 = MIN(r->x1, x);
    r->y1 = MIN(r->y1, y);
    r->x2 = MAX(r->x2, x);
    r->y2 = MAX(r->y2, y);
}

void
TranslatePathRect(PathRect *r, double deltaX, double deltaY)
{
    r->x1 += deltaX;
    r->x2 += deltaX;
    r->y1 += deltaY;
    r->y2 += deltaY;
}

void
ScalePathRect(PathRect *r, double originX, double originY,
        double scaleX, double scaleY)
{
    r->x1 = originX + scaleX*(r->x1 - originX);
    r->x2 = originX + scaleX*(r->x2 - originX);
    r->y1 = originY + scaleY*(r->y1 - originY);
    r->y2 = originY + scaleY*(r->y2 - originY);
}

void
TranslateItemHeader(Tk_PathItem *itemPtr, double deltaX, double deltaY)
{
    /* @@@ TODO: Beware for cumulated round-off errors! */
    /* If all coords == -1 the item is hidden. */
    if ((itemPtr->x1 != -1) || (itemPtr->x2 != -1) ||
	    (itemPtr->y1 != -1) || (itemPtr->y2 != -1)) {
	itemPtr->x1 += (int) deltaX;
	itemPtr->x2 += (int) deltaX;
	itemPtr->y1 += (int) deltaY;
	itemPtr->y2 += (int) deltaY;
    }
}

void
ScaleItemHeader(Tk_PathItem *itemPtr, double originX, double originY,
        double scaleX, double scaleY)
{
    /* @@@ TODO: Beware for cumulated round-off errors! */
    /* If all coords == -1 the item is hidden. */
    if ((itemPtr->x1 != -1) || (itemPtr->x2 != -1) ||
	    (itemPtr->y1 != -1) || (itemPtr->y2 != -1)) {
	int min, max;
	
	itemPtr->x1 = (int) (originX + scaleX*(itemPtr->x1 - originX));
	itemPtr->x2 = (int) (originX + scaleX*(itemPtr->x2 - originX));
	itemPtr->y1 = (int) (originY + scaleY*(itemPtr->y1 - originY));
	itemPtr->y2 = (int) (originY + scaleY*(itemPtr->y2 - originY));
	
	min = MIN(itemPtr->x1, itemPtr->x2);
	max = MAX(itemPtr->x1, itemPtr->x2);
	itemPtr->x1 = min;
	itemPtr->x2 = max;
	min = MIN(itemPtr->y1, itemPtr->y2);
	max = MAX(itemPtr->y1, itemPtr->y2);
	itemPtr->y1 = min;
	itemPtr->y2 = max;
    }
}

/*
 *--------------------------------------------------------------
 *
 * PathPolyLineToArea --
 *
 *	Determine whether an open polygon lies entirely inside, entirely
 *	outside, or overlapping a given rectangular area.
 * 	Identical to TkPolygonToArea except that it returns outside (-1)
 *	if completely encompassing the area rect.
 *
 * Results:
 *	-1 is returned if the polygon given by polyPtr and numPoints
 *	is entirely outside the rectangle given by rectPtr.  0 is
 *	returned if the polygon overlaps the rectangle, and 1 is
 *	returned if the polygon is entirely inside the rectangle.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
PathPolyLineToArea(
    double *polyPtr,	    /* Points to an array coordinates for
                             * closed polygon:  x0, y0, x1, y1, ...
                             * The polygon may be self-intersecting. */
    int numPoints,	    /* Total number of points at *polyPtr. */
    register double *rectPtr)	/* Points to coords for rectangle, in the
                             * order x1, y1, x2, y2.  X1 and y1 must
                             * be lower-left corner. */
{
    int state;		    /* State of all edges seen so far (-1 means
                             * outside, 1 means inside, won't ever be
                             * 0). */
    int count;
    register double *pPtr;

    /*
     * Iterate over all of the edges of the polygon and test them
     * against the rectangle.  Can quit as soon as the state becomes
     * "intersecting".
     */

    state = TkLineToArea(polyPtr, polyPtr+2, rectPtr);
    if (state == 0) {
        return 0;
    }
    for (pPtr = polyPtr+2, count = numPoints-1; count >= 2;
            pPtr += 2, count--) {
        if (TkLineToArea(pPtr, pPtr+2, rectPtr) != state) {
            return 0;
        }
    }
    return state;
}

/*
 *--------------------------------------------------------------
 *
 * PathThickPolygonToPoint --
 *
 *	Computes the distance from a given point to a given
 *	thick polyline (open or closed), in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are pointPtr[0] and pointPtr[1] is inside the line.  If the
 *	point isn't inside the line then the return value is the
 *	distance from the point to the line.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
PathThickPolygonToPoint(
    int joinStyle, int capStyle, 
    double width,
    int isclosed,
    double *polyPtr,	/* Points to an array coordinates for
                         * the polygon:  x0, y0, x1, y1, ...
                         * The polygon may be self-intersecting. */
    int numPoints,	/* Total number of points at *polyPtr. */
    double *pointPtr)	/* Points to coords for point. */
{
    int count;
    int project;
    int testrounding;
    int changedMiterToBevel;	/* Non-zero means that a mitered corner
                                 * had to be treated as beveled after all
                                 * because the angle was < 11 degrees. */
    double bestDist;		/* Closest distance between point and
                                 * any edge in polygon. */
    double dist, radius;
    double *coordPtr;
    double poly[10];
    
    bestDist = 1.0e36;
    radius = width/2.0;
    project = 0;
    if (!isclosed) {
        project = (capStyle == CapProjecting);
    }

    /*
     * The overall idea is to iterate through all of the edges of
     * the line, computing a polygon for each edge and testing the
     * point against that polygon.  In addition, there are additional
     * tests to deal with rounded joints and caps.
     */

    changedMiterToBevel = 0;
    for (count = numPoints, coordPtr = polyPtr; count >= 2;
            count--, coordPtr += 2) {
    
        /*
         * If rounding is done around the first point then compute
         * the distance between the point and the point.
         */
        testrounding = 0;
        if (isclosed) {
            testrounding = (joinStyle == JoinRound);
        } else {
            testrounding = (((capStyle == CapRound) && (count == numPoints))
                    || ((joinStyle == JoinRound) && (count != numPoints)));
        }    
        if (testrounding) {
            dist = hypot(coordPtr[0] - pointPtr[0], coordPtr[1] - pointPtr[1])
                    - radius;
            if (dist <= 0.0) {
                bestDist = 0.0;
                goto donepoint;
            } else if (dist < bestDist) {
                bestDist = dist;
            }
        }
    
        /*
         * Compute the polygonal shape corresponding to this edge,
         * consisting of two points for the first point of the edge
         * and two points for the last point of the edge.
         */
    
        if (count == numPoints) {
            TkGetButtPoints(coordPtr+2, coordPtr, (double) width,
                    project, poly, poly+2);
        } else if ((joinStyle == JoinMiter) && !changedMiterToBevel) {
            poly[0] = poly[6];
            poly[1] = poly[7];
            poly[2] = poly[4];
            poly[3] = poly[5];
        } else {
            TkGetButtPoints(coordPtr+2, coordPtr, (double) width, 0,
                    poly, poly+2);
    
            /*
             * If this line uses beveled joints, then check the distance
             * to a polygon comprising the last two points of the previous
             * polygon and the first two from this polygon;  this checks
             * the wedges that fill the mitered joint.
             */
    
            if ((joinStyle == JoinBevel) || changedMiterToBevel) {
                poly[8] = poly[0];
                poly[9] = poly[1];
                dist = TkPolygonToPoint(poly, 5, pointPtr);
                if (dist <= 0.0) {
                    bestDist = 0.0;
                    goto donepoint;
                } else if (dist < bestDist) {
                    bestDist = dist;
                }
                changedMiterToBevel = 0;
            }
        }
        if (count == 2) {
            TkGetButtPoints(coordPtr, coordPtr+2, (double) width,
                    project, poly+4, poly+6);
        } else if (joinStyle == JoinMiter) {
            if (TkGetMiterPoints(coordPtr, coordPtr+2, coordPtr+4,
                    (double) width, poly+4, poly+6) == 0) {
                changedMiterToBevel = 1;
                TkGetButtPoints(coordPtr, coordPtr+2, (double) width,
                        0, poly+4, poly+6);
            }
        } else {
            TkGetButtPoints(coordPtr, coordPtr+2, (double) width, 0,
                    poly+4, poly+6);
        }
        poly[8] = poly[0];
        poly[9] = poly[1];
        dist = TkPolygonToPoint(poly, 5, pointPtr);
        if (dist <= 0.0) {
            bestDist = 0.0;
            goto donepoint;
        } else if (dist < bestDist) {
            bestDist = dist;
        }
    }
        
    /*
     * If caps are rounded, check the distance to the cap around the
     * final end point of the line.
     */
    if (!isclosed && (capStyle == CapRound)) {
        dist = hypot(coordPtr[0] - pointPtr[0], coordPtr[1] - pointPtr[1])
                - width/2.0;
        if (dist <= 0.0) {
            bestDist = 0.0;
            goto donepoint;
        } else if (dist < bestDist) {
            bestDist = dist;
        }
    }

donepoint:

    return bestDist;
}

/*
 *--------------------------------------------------------------
 *
 * PathPolygonToPointEx --
 *
 *	Compute the distance from a point to a polygon. This is
 *	essentially identical to TkPolygonToPoint with two exceptions:
 *	1) 	It returns the closest distance to the *stroke*,
 *		any fill unrecognized.
 *	2)	It returns both number of total intersections, and
 *		the number of directed crossings, nonzerorule.
 *
 * Results:
 *	The return value is 0.0 if the point referred to by
 *	pointPtr is within the polygon referred to by polyPtr
 *	and numPoints.  Otherwise the return value is the
 *	distance of the point from the polygon.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
PathPolygonToPointEx(
    double *polyPtr,	/* Points to an array coordinates for
                         * the polygon:  x0, y0, x1, y1, ...
                         * The polygon may be self-intersecting.
                         * If a fillRule is used the last point
                         * must duplicate the first one. */
    int numPoints,	/* Total number of points at *polyPtr. */
    double *pointPtr,	/* Points to coords for point. */
    int *intersectionsPtr,/* (out) The number of intersections. */
    int *nonzerorulePtr)/* (out) The number of intersections
			 * considering crossing direction. */
{
    double bestDist;	/* Closest distance between point and
                         * any edge in polygon. */
    int intersections;	/* Number of edges in the polygon that
                         * intersect a ray extending vertically
                         * upwards from the point to infinity. */
    int nonzerorule;	/* As 'intersections' except that it adds
                         * one if crossing right to left, and
                         * subtracts one if crossing left to right. */
    int count;
    register double *pPtr;

    /*
     * Iterate through all of the edges in the polygon, updating
     * bestDist and intersections.
     *
     * TRICKY POINT:  when computing intersections, include left
     * x-coordinate of line within its range, but not y-coordinate.
     * Otherwise if the point lies exactly below a vertex we'll
     * count it as two intersections.
     */

    bestDist = 1.0e36;
    intersections = 0;
    nonzerorule = 0;

    for (count = numPoints, pPtr = polyPtr; count > 1; count--, pPtr += 2) {
        double x, y, dist;
    
        /*
         * Compute the point on the current edge closest to the point
         * and update the intersection count.  This must be done
         * separately for vertical edges, horizontal edges, and
         * other edges.
         */
    
        if (pPtr[2] == pPtr[0]) {
    
            /*
             * Vertical edge.
             */
    
            x = pPtr[0];
            if (pPtr[1] >= pPtr[3]) {
                y = MIN(pPtr[1], pointPtr[1]);
                y = MAX(y, pPtr[3]);
            } else {
                y = MIN(pPtr[3], pointPtr[1]);
                y = MAX(y, pPtr[1]);
            }
        } else if (pPtr[3] == pPtr[1]) {
    
            /*
             * Horizontal edge.
             */
    
            y = pPtr[1];
            if (pPtr[0] >= pPtr[2]) {
                x = MIN(pPtr[0], pointPtr[0]);
                x = MAX(x, pPtr[2]);
                if ((pointPtr[1] < y) && (pointPtr[0] < pPtr[0])
                        && (pointPtr[0] >= pPtr[2])) {
                    intersections++;
                    nonzerorule++;
                }
            } else {
                x = MIN(pPtr[2], pointPtr[0]);
                x = MAX(x, pPtr[0]);
                if ((pointPtr[1] < y) && (pointPtr[0] < pPtr[2])
                        && (pointPtr[0] >= pPtr[0])) {
                    intersections++;
                    nonzerorule--;
                }
            }
        } else {
            double m1, b1, m2, b2;
            int lower;		/* Non-zero means point below line. */
    
            /*
             * The edge is neither horizontal nor vertical.  Convert the
             * edge to a line equation of the form y = m1*x + b1.  Then
             * compute a line perpendicular to this edge but passing
             * through the point, also in the form y = m2*x + b2.
             */
    
            m1 = (pPtr[3] - pPtr[1])/(pPtr[2] - pPtr[0]);
            b1 = pPtr[1] - m1*pPtr[0];
            m2 = -1.0/m1;
            b2 = pointPtr[1] - m2*pointPtr[0];
            x = (b2 - b1)/(m1 - m2);
            y = m1*x + b1;
            if (pPtr[0] > pPtr[2]) {
                if (x > pPtr[0]) {
                    x = pPtr[0];
                    y = pPtr[1];
                } else if (x < pPtr[2]) {
                    x = pPtr[2];
                    y = pPtr[3];
                }
            } else {
                if (x > pPtr[2]) {
                    x = pPtr[2];
                    y = pPtr[3];
                } else if (x < pPtr[0]) {
                    x = pPtr[0];
                    y = pPtr[1];
                }
            }
            lower = (m1*pointPtr[0] + b1) > pointPtr[1];
            if (lower && (pointPtr[0] >= MIN(pPtr[0], pPtr[2]))
                    && (pointPtr[0] < MAX(pPtr[0], pPtr[2]))) {
                intersections++;
                if (pPtr[0] >= pPtr[2]) {
                    nonzerorule++;
                } else {
                    nonzerorule--;
                }
            }
        }
    
        /*
         * Compute the distance to the closest point, and see if that
         * is the best distance seen so far.
         */
    
        dist = hypot(pointPtr[0] - x, pointPtr[1] - y);
        if (dist < bestDist) {
            bestDist = dist;
        }
    }
    *intersectionsPtr = intersections;
    *nonzerorulePtr = nonzerorule;
    
    return bestDist;
}

/*
 *--------------------------------------------------------------
 *
 * PathRectToPoint --
 *
 *	Computes the distance from a given point to a given
 *	rectangle, in canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates
 *	are pointPtr[0] and pointPtr[1] is inside the rectangle.  If the
 *	point isn't inside the rectangle then the return value is the
 *	distance from the point to the rectangle.  If item is filled,
 *	then anywhere in the interior is considered "inside"; if
 *	item isn't filled, then "inside" means only the area
 *	occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
PathRectToPoint(
    double rectPtr[], 	/* Bare rectangle. */
    double width, 	/* Width of stroke, or 0. */
    int filled, 	/* Is rectangle filled. */
    double pointPtr[])	/* Pointer to x and y coordinates. */
{
    double xDiff, yDiff, x1, y1, x2, y2, inc, tmp;

    /*
     * Generate a new larger rectangle that includes the border
     * width, if there is one.
     */

	inc = width/2.0;
    x1 = rectPtr[0] - inc;
    y1 = rectPtr[1] - inc;
    x2 = rectPtr[2] + inc;
    y2 = rectPtr[3] + inc;

    /*
     * If the point is inside the rectangle, handle specially:
     * distance is 0 if rectangle is filled, otherwise compute
     * distance to nearest edge of rectangle and subtract width
     * of edge.
     */

    if ((pointPtr[0] >= x1) && (pointPtr[0] < x2)
            && (pointPtr[1] >= y1) && (pointPtr[1] < y2)) {
        //if (filled || (rectPtr->outline.gc == None)) {
        if (filled) {
            return 0.0;
        }
        xDiff = pointPtr[0] - x1;
        tmp = x2 - pointPtr[0];
        if (tmp < xDiff) {
            xDiff = tmp;
        }
        yDiff = pointPtr[1] - y1;
        tmp = y2 - pointPtr[1];
        if (tmp < yDiff) {
            yDiff = tmp;
        }
        if (yDiff < xDiff) {
            xDiff = yDiff;
        }
        xDiff -= width;
        if (xDiff < 0.0) {
            return 0.0;
        }
        return xDiff;
    }

    /*
     * Point is outside rectangle.
     */

    if (pointPtr[0] < x1) {
        xDiff = x1 - pointPtr[0];
    } else if (pointPtr[0] > x2)  {
        xDiff = pointPtr[0] - x2;
    } else {
        xDiff = 0;
    }

    if (pointPtr[1] < y1) {
        yDiff = y1 - pointPtr[1];
    } else if (pointPtr[1] > y2)  {
        yDiff = pointPtr[1] - y2;
    } else {
        yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 *--------------------------------------------------------------
 *
 * PathRectToArea --
 *
 *	This procedure is called to determine whether an rectangle
 *	lies entirely inside, entirely outside, or overlapping
 *	another given rectangle.
 *
 * Results:
 *	-1 is returned if the rectangle is entirely outside the area
 *	given by rectPtr, 0 if it overlaps, and 1 if it is entirely
 *	inside the given area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
PathRectToArea(    
    double rectPtr[], 	/* Bare rectangle. */
    double width, 	/* Width of stroke, or 0. */
    int filled, 	/* Is rectangle filled. */
    double *areaPtr)	/* Pointer to array of four coordinates
                         * (x1, y1, x2, y2) describing rectangular
                         * area.  */
{
    double halfWidth = width/2.0;

    if ((areaPtr[2] <= (rectPtr[0] - halfWidth))
            || (areaPtr[0] >= (rectPtr[2] + halfWidth))
            || (areaPtr[3] <= (rectPtr[1] - halfWidth))
            || (areaPtr[1] >= (rectPtr[3] + halfWidth))) {
        return -1;
    }
    if (!filled && (width > 0.0)
            && (areaPtr[0] >= (rectPtr[0] + halfWidth))
            && (areaPtr[1] >= (rectPtr[1] + halfWidth))
            && (areaPtr[2] <= (rectPtr[2] - halfWidth))
            && (areaPtr[3] <= (rectPtr[3] - halfWidth))) {
        return -1;
    }
    if ((areaPtr[0] <= (rectPtr[0] - halfWidth))
            && (areaPtr[1] <= (rectPtr[1] - halfWidth))
            && (areaPtr[2] >= (rectPtr[2] + halfWidth))
            && (areaPtr[3] >= (rectPtr[3] + halfWidth))) {
        return 1;
    }
    return 0;
}

int
PathRectToAreaWithMatrix(PathRect bbox, TMatrix *mPtr, double *areaPtr)
{
    int rectiLinear = 0;
    double rect[4];

    if (mPtr == NULL) {
        rectiLinear = 1;
        rect[0] = bbox.x1;
        rect[1] = bbox.y1;
        rect[2] = bbox.x2;
        rect[3] = bbox.y2;
    } else if (TMATRIX_IS_RECTILINEAR(mPtr)) {
        rectiLinear = 1;
        rect[0] = mPtr->a * bbox.x1 + mPtr->tx;
        rect[1] = mPtr->d * bbox.y1 + mPtr->ty;
        rect[2] = mPtr->a * bbox.x2 + mPtr->tx;
        rect[3] = mPtr->d * bbox.y2 + mPtr->ty;
    }
    if (rectiLinear) {
        return PathRectToArea(rect, 0.0, 1, areaPtr);
    } else {
        double polyPtr[10];
    
        /* polyPtr: Points to an array coordinates for closed polygon:  x0, y0, x1, y1, ... */
        /* Construct all four corners. */
        polyPtr[0] = bbox.x1, polyPtr[1] = bbox.y1;
        polyPtr[2] = bbox.x2, polyPtr[3] = bbox.y1;
        polyPtr[4] = bbox.x2, polyPtr[5] = bbox.y2;
        polyPtr[6] = bbox.x1, polyPtr[7] = bbox.y2;
        PathApplyTMatrix(mPtr, polyPtr, polyPtr+1);       
        PathApplyTMatrix(mPtr, polyPtr+2, polyPtr+3);       
        PathApplyTMatrix(mPtr, polyPtr+4, polyPtr+5);       
        PathApplyTMatrix(mPtr, polyPtr+6, polyPtr+7); 

        return TkPolygonToArea(polyPtr, 4, areaPtr);
    }
}

double
PathRectToPointWithMatrix(PathRect bbox, TMatrix *mPtr, double *pointPtr) 
{
    int rectiLinear = 0;
    double dist;
    double rect[4];

    if (mPtr == NULL) {
        rectiLinear = 1;
        rect[0] = bbox.x1;
        rect[1] = bbox.y1;
        rect[2] = bbox.x2;
        rect[3] = bbox.y2;
    } else if (TMATRIX_IS_RECTILINEAR(mPtr)) {
        rectiLinear = 1;
        rect[0] = mPtr->a * bbox.x1 + mPtr->tx;
        rect[1] = mPtr->d * bbox.y1 + mPtr->ty;
        rect[2] = mPtr->a * bbox.x2 + mPtr->tx;
        rect[3] = mPtr->d * bbox.y2 + mPtr->ty;
    }
    if (rectiLinear) {
        dist = PathRectToPoint(rect, 0.0, 1, pointPtr);
    } else {
        int intersections, rule;
        double polyPtr[10];
        
        /* Construct all four corners. 
         * First and last must be identical since closed.
         */
        polyPtr[0] = bbox.x1, polyPtr[1] = bbox.y1;
        polyPtr[2] = bbox.x2, polyPtr[3] = bbox.y1;
        polyPtr[4] = bbox.x2, polyPtr[5] = bbox.y2;
        polyPtr[6] = bbox.x1, polyPtr[7] = bbox.y2;
        PathApplyTMatrix(mPtr, polyPtr, polyPtr+1);       
        PathApplyTMatrix(mPtr, polyPtr+2, polyPtr+3);       
        PathApplyTMatrix(mPtr, polyPtr+4, polyPtr+5);       
        PathApplyTMatrix(mPtr, polyPtr+6, polyPtr+7); 
        polyPtr[8] = polyPtr[0], polyPtr[9] = polyPtr[1];
    
        dist = PathPolygonToPointEx(polyPtr, 5, pointPtr, &intersections, &rule);
        if (intersections % 2 == 1) {
            dist = 0.0;
        }
    }
    return dist;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathCanvasItemExConfigure --
 *
 *      Takes care of the custom item configuration of the Tk_PathItemEx
 *	part of any item with style.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkPathCanvasItemExConfigure(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItemEx *itemExPtr, int mask)
{
    Tk_Window tkwin;
    Tk_PathItem *parentPtr;
    Tk_PathItem *itemPtr = (Tk_PathItem *) itemExPtr;
    Tk_PathStyle *stylePtr = &itemExPtr->style;

    tkwin = Tk_PathCanvasTkwin(canvas);
    if (mask & PATH_CORE_OPTION_PARENT) {
	if (TkPathCanvasFindGroup(interp, canvas, itemPtr->parentObj, &parentPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	TkPathCanvasSetParent(parentPtr, itemPtr);
    } else if ((itemPtr->id != 0) && (itemPtr->parentPtr == NULL)) {
	/*
	 * If item not root and parent not set we must set it to root by default.
	 */
	CanvasSetParentToRoot(itemPtr);
    }
    
    /*
     * If we have got a style name it's options take precedence
     * over the actual path configuration options. This is how SVG does it.
     * Good or bad?
     */
    if (mask & PATH_CORE_OPTION_STYLENAME) {
	TkPathStyleInst *styleInst = NULL;
	
	if (itemExPtr->styleObj != NULL) {
	    styleInst = TkPathGetStyle(interp, Tcl_GetString(itemExPtr->styleObj),
		    TkPathCanvasStyleTable(canvas), PathStyleChangedProc,
		    (ClientData) itemExPtr);
	    if (styleInst == NULL) {
		return TCL_ERROR;
	    }
	} else {
	    styleInst = NULL;
	}
	if (itemExPtr->styleInst != NULL) {
	    TkPathFreeStyle(itemExPtr->styleInst);
	}
	itemExPtr->styleInst = styleInst;    
    } 
    
    /*
     * Just translate the 'fillObj' (string) to a TkPathColor.
     * We MUST have this last in the chain of custom option checks!
     */
    if (mask & PATH_STYLE_OPTION_FILL) {
	TkPathColor *fillPtr = NULL;
	
	if (stylePtr->fillObj != NULL) {
	    fillPtr = TkPathGetPathColor(interp, tkwin, stylePtr->fillObj,
		    TkPathCanvasGradientTable(canvas), PathGradientChangedProc,
		    (ClientData) itemExPtr);
	    if (fillPtr == NULL) {
		return TCL_ERROR;
	    }
	} else {
	    fillPtr = NULL;
	}
	/* Free any old and store the new. */
	if (stylePtr->fill != NULL) {
	    TkPathFreePathColor(stylePtr->fill);
	}
	stylePtr->fill = fillPtr;
    }
    return TCL_OK;
}

void	
PathGradientChangedProc(ClientData clientData, int flags)
{
    Tk_PathItemEx *itemExPtr = (Tk_PathItemEx *)clientData;
    Tk_PathItem *itemPtr = (Tk_PathItem *) itemExPtr;
    Tk_PathStyle *stylePtr = &(itemExPtr->style);
        
    if (flags) {
	if (flags & PATH_GRADIENT_FLAG_DELETE) {
	    TkPathFreePathColor(stylePtr->fill);	
	    stylePtr->fill = NULL;
	    Tcl_DecrRefCount(stylePtr->fillObj);
	    stylePtr->fillObj = NULL;
	}
	if (itemPtr->typePtr == &tkGroupType) {
	    GroupItemConfigured(itemExPtr->canvas, itemPtr, 
		    PATH_STYLE_OPTION_FILL);
	} else {
	    Tk_PathCanvasEventuallyRedraw(itemExPtr->canvas,
		    itemExPtr->header.x1, itemExPtr->header.y1,
		    itemExPtr->header.x2, itemExPtr->header.y2);
	    }
    }
}

void	
PathStyleChangedProc(ClientData clientData, int flags)
{
    Tk_PathItemEx *itemExPtr = (Tk_PathItemEx *)clientData;
    Tk_PathItem *itemPtr = (Tk_PathItem *) itemExPtr;
        
    if (flags) {
	if (flags & PATH_STYLE_FLAG_DELETE) {
	    TkPathFreeStyle(itemExPtr->styleInst);	
	    itemExPtr->styleInst = NULL;
	    Tcl_DecrRefCount(itemExPtr->styleObj);
	    itemExPtr->styleObj = NULL;
	}
	if (itemPtr->typePtr == &tkGroupType) {
	    GroupItemConfigured(itemExPtr->canvas, itemPtr, 
		    PATH_CORE_OPTION_STYLENAME); // Not completely correct...
	} else {
	    Tk_PathCanvasEventuallyRedraw(itemExPtr->canvas,
		    itemExPtr->header.x1, itemExPtr->header.y1,
		    itemExPtr->header.x2, itemExPtr->header.y2);
	    }
    }
}

/*--------------------------------------------------------------------------*/
