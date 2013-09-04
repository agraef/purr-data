/*
 * tkPathUtil.h --
 *
 *	This file contains support functions for tkpath.
 *
 * Copyright (c) 2005-2008  Mats Bengtsson
 *
 * $Id$
 */

#include <float.h>
#include "tkIntPath.h"
#include "tkCanvPathUtil.h"

#define DOUBLE_EQUALS(x,y)      (fabs((x) - (y)) < DBL_EPSILON)

/*
 *--------------------------------------------------------------
 *
 * TkPathMakePrectAtoms --
 *
 *	Makes the path atoms for a rounded rectangle, prect.
 *
 * Results:
 *	None. Path atoms in atomPtrPtr.
 *
 * Side effects:
 *	Path atom memory allocated.
 *
 *--------------------------------------------------------------
 */

void
TkPathMakePrectAtoms(double *pointsPtr, double rx, double ry, PathAtom **atomPtrPtr)
{
    PathAtom *atomPtr = NULL;
    PathAtom *firstAtomPtr = NULL;
    int round = 1;
    double epsilon = 1e-6;
    double x = MIN(pointsPtr[0], pointsPtr[2]);
    double y = MIN(pointsPtr[1], pointsPtr[3]);
    double width = fabs(pointsPtr[0] - pointsPtr[2]);
    double height = fabs(pointsPtr[1] - pointsPtr[3]);

    /* If only one of rx or ry is zero this implies that both shall be nonzero. */
    if (rx < epsilon && ry < epsilon) {
        round = 0;
    } else if (rx < epsilon) {
        rx = ry;
    } else if (ry < epsilon) {
        ry = rx;
    }
    if (round) {
        
        /* There are certain constraints on rx and ry. */
        rx = MIN(rx, width/2.0);
        ry = MIN(ry, height/2.0);
        
        atomPtr = NewMoveToAtom(x+rx, y);
        firstAtomPtr = atomPtr;
        atomPtr->nextPtr = NewLineToAtom(x+width-rx, y);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewArcAtom(rx, ry, 0.0, 0, 1, x+width, y+ry);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewLineToAtom(x+width, y+height-ry);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewArcAtom(rx, ry, 0.0, 0, 1, x+width-rx, y+height);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewLineToAtom(x+rx, y+height);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewArcAtom(rx, ry, 0.0, 0, 1, x, y+height-ry);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewLineToAtom(x, y+ry);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewArcAtom(rx, ry, 0.0, 0, 1, x+rx, y);
        atomPtr = atomPtr->nextPtr;
        atomPtr->nextPtr = NewCloseAtom(x, y);
        *atomPtrPtr = firstAtomPtr;
    } else {
        atomPtr = NewRectAtom(pointsPtr);
        *atomPtrPtr = atomPtr;        
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkPathDrawPath --
 *
 *	This procedure is invoked to draw a line item in a given
 *	drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation
 *	information in canvas.
 *
 *--------------------------------------------------------------
 */

void
TkPathDrawPath(
    Tk_Window tkwin,        /* Tk window. */
    Drawable drawable,      /* Pixmap or window in which to draw
                             * item. */
    PathAtom *atomPtr,      /* The actual path as a linked list
                             * of PathAtoms. */
    Tk_PathStyle *stylePtr, /* The paths style. */
    TMatrix *mPtr,          /* Typically used for canvas offsets. */
    PathRect *bboxPtr)      /* The bare (untransformed) bounding box 
                             * (assuming zero stroke width) */
{
    TkPathContext context;
    
    /*
     * Define the path in the drawable using the path drawing functions.
     * Any transform matrix need to be considered and canvas drawable
     * offset must always be taken into account. Note the order!
     */
     
    context = TkPathInit(tkwin, drawable);
    if (mPtr != NULL) {
        TkPathPushTMatrix(context, mPtr);
    }
    if (stylePtr->matrixPtr != NULL) {
        TkPathPushTMatrix(context, stylePtr->matrixPtr);
    }
    if (TkPathMakePath(context, atomPtr, stylePtr) != TCL_OK) {
        return;
    }
    TkPathPaintPath(context, atomPtr, stylePtr,	bboxPtr);
    TkPathFree(context);
}

/*
 *--------------------------------------------------------------
 *
 * TkPathPaintPath --
 *
 *	This procedure is invoked to paint a path in a given context.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any path defined in the context is painted.
 *
 *--------------------------------------------------------------
 */

void
TkPathPaintPath(
    TkPathContext context, 
    PathAtom *atomPtr,      /* The actual path as a linked list
                             * of PathAtoms. */
    Tk_PathStyle *stylePtr, /* The paths style. */
    PathRect *bboxPtr)
{
    TkPathGradientMaster *gradientPtr = GetGradientMasterFromPathColor(stylePtr->fill);
    
    if (gradientPtr != NULL) {
        TkPathClipToPath(context, stylePtr->fillRule);
        PathGradientPaint(context, bboxPtr, gradientPtr, stylePtr->fillRule);
        
        /* NB: Both CoreGraphics on MacOSX and Win32 GDI (and cairo from 1.0) 
         *     clear the current path when setting clipping. Need therefore
         *     to redo the path. 
         */
        if (TkPathDrawingDestroysPath()) {
            TkPathMakePath(context, atomPtr, stylePtr);
        }
        
        /* We shall remove the path clipping here! */
        TkPathReleaseClipToPath(context);
    }

    if ((GetColorFromPathColor(stylePtr->fill) != NULL) && (stylePtr->strokeColor != NULL)) {
        TkPathFillAndStroke(context, stylePtr);
    } else if (GetColorFromPathColor(stylePtr->fill) != NULL) {
        TkPathFill(context, stylePtr);
    } else if (stylePtr->strokeColor != NULL) {
        TkPathStroke(context, stylePtr);
    }
}

PathRect
TkPathGetTotalBbox(PathAtom *atomPtr, Tk_PathStyle *stylePtr)
{
    PathRect bare, total;
    
    bare = GetGenericBarePathBbox(atomPtr);
    total = GetGenericPathTotalBboxFromBare(atomPtr, stylePtr, &bare);
    return total;
}

/* Return NULL on error and leave error message */

// @@@ OBSOLETE SOON!!!
// As a temporary mean before trashing it we ignore gradients.

TkPathColor *
TkPathNewPathColor(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *nameObj)
{
    char *name;
    TkPathColor *colorPtr;
    XColor *color = NULL;
    
    name = Tcl_GetStringFromObj(nameObj, NULL);
    colorPtr = (TkPathColor *) ckalloc(sizeof(TkPathColor));
    colorPtr->color = NULL;
    colorPtr->gradientInstPtr = NULL;

    color = Tk_AllocColorFromObj(interp, tkwin, nameObj);
    if (color == NULL) {
        char tmp[256];
        ckfree((char *) colorPtr);
        sprintf(tmp, "unrecognized color or gradient name \"%s\"", name);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(tmp, -1));
        return NULL;
    }
    colorPtr->color = color;     
    return colorPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathGetPathColor --
 *
 *	Parses a string in nameObj to either a valid XColor or
 *      looks up a gradient name for the hash table tablePtr.
 *      Makes a new TkPathColor struct from a string value.
 *      Like Tk_GetImage() but for TkPathColor instead of Tk_Image.
 *
 * Results:
 *	Pointer to a TkPathColor struct or returns NULL on error 
 *      and leaves an error message.
 *
 * Side effects:
 *	TkPathColor malloced if OK.
 *
 *--------------------------------------------------------------
 */

TkPathColor *
TkPathGetPathColor(Tcl_Interp *interp, Tk_Window tkwin, 
    Tcl_Obj *nameObj, Tcl_HashTable *tablePtr,
    TkPathGradientChangedProc *changeProc, ClientData clientData)
{
    char *name;
    TkPathColor *colorPtr;
    XColor *color = NULL;
    TkPathGradientInst *gradientInstPtr;
    
    name = Tcl_GetString(nameObj);
    colorPtr = (TkPathColor *) ckalloc(sizeof(TkPathColor));
    
    /*
     * Only one of them can be non NULL.
     */
    colorPtr->color = NULL;
    colorPtr->gradientInstPtr = NULL;
    
    gradientInstPtr = TkPathGetGradient(interp, name, tablePtr, changeProc, clientData);
    if (gradientInstPtr != NULL) {
        colorPtr->gradientInstPtr = gradientInstPtr;
    } else {
        Tcl_ResetResult(interp);
        color = Tk_AllocColorFromObj(interp, tkwin, nameObj);
        if (color == NULL) {
            Tcl_Obj *resultObj;
            ckfree((char *) colorPtr);
            resultObj = Tcl_NewStringObj("unrecognized color or gradient name \"", -1);
            Tcl_AppendStringsToObj(resultObj, name, "\"", (char *) NULL);
            Tcl_SetObjResult(interp, resultObj);
            return NULL;
        }
        colorPtr->color = color;     
    }
    return colorPtr;
}

void
TkPathFreePathColor(TkPathColor *colorPtr)
{
    if (colorPtr != NULL) {
        if (colorPtr->color != NULL) {
            Tk_FreeColor(colorPtr->color);
        } else if (colorPtr->gradientInstPtr != NULL) {
            TkPathFreeGradient(colorPtr->gradientInstPtr);
        }
        ckfree((char *) colorPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * PathCopyBitsARGB, PathCopyBitsBGRA --
 *
 *	Copies bitmap data from these formats to RGBA.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
PathCopyBitsARGB(unsigned char *from, unsigned char *to, 
        int width, int height, int bytesPerRow)
{
    unsigned char *src, *dst;
    int i, j;

    /* Copy XRGB to RGBX in one shot, alphas in a loop. */
    memcpy(to, from+1, height*bytesPerRow-1);
    
    for (i = 0; i < height; i++) {
        src = from + i*bytesPerRow;
        dst = to + i*bytesPerRow;
        /* @@@ Keep ARGB format in photo? */
        for (j = 0; j < width; j++, src += 4, dst += 4) {
            *(dst+3) = *src;
        }
    }
}

void
PathCopyBitsBGRA(unsigned char *from, unsigned char *to, 
        int width, int height, int bytesPerRow)
{
    unsigned char *src, *dst;
    int i, j;
    
    /* Copy BGRA -> RGBA */
    for (i = 0; i < height; i++) {
        src = from + i*bytesPerRow;
        dst = to + i*bytesPerRow;
        for (j = 0; j < width; j++, src += 4) {
            /* RED */
            *dst++ = *(src+2);
            /* GREEN */
            *dst++ = *(src+1);
            /* BLUE */
            *dst++ = *src;
            /* ALPHA */
            *dst++ = *(src+3);
        }
    }
}

/*
 *--------------------------------------------------------------
 *
 * PathCopyBitsPremultipliedAlphaRGBA, PathCopyBitsPremultipliedAlphaARGB --
 *
 *	Copies bitmap data that have alpha premultiplied into a bitmap
 *	with "true" RGB values need for Tk_Photo. The source format is
 *	either RGBA or ARGB, but destination always RGBA used for photos.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
PathCopyBitsPremultipliedAlphaRGBA(unsigned char *from, unsigned char *to, 
        int width, int height, int bytesPerRow)
{
    unsigned char *src, *dst, alpha;
    int i, j;

    /* Copy src RGBA with premulitplied alpha to "plain" RGBA. */
    for (i = 0; i < height; i++) {
        src = from + i*bytesPerRow;
        dst = to + i*bytesPerRow;
        for (j = 0; j < width; j++) {
            alpha = *(src+3);
            if (alpha == 0xFF || alpha == 0x00) {
                memcpy(dst, src, 4);
                src += 4;
                dst += 4;
            } else {
                /* dst = 255*src/alpha */
                *dst++ = (*src++*255)/alpha;
                *dst++ = (*src++*255)/alpha;
                *dst++ = (*src++*255)/alpha;
                *dst++ = alpha;
                src++;
            }
        }
    }
}

// UNTESTED!
void
PathCopyBitsPremultipliedAlphaARGB(unsigned char *from, unsigned char *to, 
        int width, int height, int bytesPerRow)
{
    unsigned char *src, *dst, alpha;
    int i, j;

    /* Copy src ARGB with premulitplied alpha to "plain" RGBA. */
    for (i = 0; i < height; i++) {
        src = from + i*bytesPerRow;
        dst = to + i*bytesPerRow;
        for (j = 0; j < width; j++) {
            alpha = *src;
            if (alpha == 0xFF || alpha == 0x00) {
                memcpy(dst, src+1, 3);
                *(dst+3) = alpha;
                src += 4;
                dst += 4;
            } else {
                /* dst = 255*src/alpha */
                *(dst+3) = alpha;
                src++;
                *dst = ((*src << 8) - *src)/alpha;
                dst++, src++;
                *dst = ((*src << 8) - *src)/alpha;
                dst++, src++;
                *dst = ((*src << 8) - *src)/alpha;
                dst++, dst++, src++;
            }
        }
    }
}

void
PathCopyBitsPremultipliedAlphaBGRA(unsigned char *from, unsigned char *to, 
        int width, int height, int bytesPerRow)
{
    unsigned char *src, *dst, alpha;
    int i, j;

    /* Copy src BGRA with premulitplied alpha to "plain" RGBA. */
    for (i = 0; i < height; i++) {
        src = from + i*bytesPerRow;
        dst = to + i*bytesPerRow;
        for (j = 0; j < width; j++, src += 4) {
            alpha = *(src+3);
            if (alpha == 0xFF || alpha == 0x00) {
                /* RED */
                *dst++ = *(src+2);
                /* GREEN */
                *dst++ = *(src+1);
                /* BLUE */
                *dst++ = *src;
                /* ALPHA */
                *dst++ = *(src+3);
            } else {
                /* dst = 255*src/alpha */
                /* RED */
                *dst++ = (*(src+2)*255)/alpha;
                /* GREEN */
                *dst++ = (*(src+1)*255)/alpha;
                /* BLUE */
                *dst++ = (*(src+0)*255)/alpha;
                /* ALPHA */
                *dst++ = alpha;
            }
        }
    }
}

/* from mozilla */
static double 
CalcVectorAngle(double ux, double uy, double vx, double vy)
{
    double ta = atan2(uy, ux);
    double tb = atan2(vy, vx);
    if (tb >= ta) {
        return tb-ta;
    } else {
        return 2.0*M_PI - (ta-tb);
    }
}

/*
 *--------------------------------------------------------------
 *
 * CentralToEndpointArcParameters
 *
 *	Conversion from center to endpoint parameterization.
 *	All angles in radians!
 *	From: http://www.w3.org/TR/2003/REC-SVG11-20030114
 *
 * Results:
 *	Arc specific return code.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
CentralToEndpointArcParameters(
        double cx, double cy, double rx, double ry,	/* In pars. */
        double theta1, double dtheta, double phi,
        double *x1Ptr, double *y1Ptr, 			/* Out. */
        double *x2Ptr, double *y2Ptr, 
        char *largeArcFlagPtr, char *sweepFlagPtr)	
{
    double theta2;
    double sinPhi, cosPhi;
    double sinTheta1, cosTheta1;
    double sinTheta2, cosTheta2;

    theta2 = theta1 + dtheta;
    sinPhi = sin(phi);
    cosPhi = cos(phi);
    sinTheta1 = sin(theta1);
    cosTheta1 = cos(theta1);
    sinTheta2 = sin(theta2);
    cosTheta2 = cos(theta2);
    
    /* F.6.4 Conversion from center to endpoint parameterization. */
    *x1Ptr = cx + rx * cosTheta1 * cosPhi - ry * sinTheta1 * sinPhi;
    *y1Ptr = cy + rx * cosTheta1 * sinPhi + ry * sinTheta1 * cosPhi;
    *x2Ptr = cx + rx * cosTheta2 * cosPhi - ry * sinTheta2 * sinPhi;
    *y2Ptr = cy + rx * cosTheta2 * sinPhi + ry * sinTheta2 * cosPhi;

    *largeArcFlagPtr = (dtheta > M_PI) ? 1 : 0;
    *sweepFlagPtr = (dtheta > 0.0) ? 1 : 0;

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * EndpointToCentralArcParameters
 *
 *	Conversion from endpoint to center parameterization.
 *	All angles in radians!
 *	From: http://www.w3.org/TR/2003/REC-SVG11-20030114
 *
 * Results:
 *	Arc specific return code.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
EndpointToCentralArcParameters(
        double x1, double y1, double x2, double y2,	/* The endpoints. */
        double rx, double ry,				/* Radius. */
        double phi, char largeArcFlag, char sweepFlag,
        double *cxPtr, double *cyPtr, 			/* Out. */
        double *rxPtr, double *ryPtr,
        double *theta1Ptr, double *dthetaPtr)
{
    double sinPhi, cosPhi;
    double dx, dy;
    double x1dash, y1dash;
    double cxdash, cydash;
    double cx, cy;
    double numerator, root;
    double theta1, dtheta;

    /* 1. Treat out-of-range parameters as described in
     * http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes
     *
     * If the endpoints (x1, y1) and (x2, y2) are identical, then this
     * is equivalent to omitting the elliptical arc segment entirely
     */
    if (DOUBLE_EQUALS(x1, x2) && DOUBLE_EQUALS(y1, y2)) {
        return kPathArcSkip;
    }
    
    /* If rx = 0 or ry = 0 then this arc is treated as a straight line
     * segment (a "lineto") joining the endpoints.
     */
    if (rx == 0.0f || ry == 0.0f) {
        return kPathArcLine;
    }

    /* If rx or ry have negative signs, these are dropped; the absolute
     * value is used instead.
     */
    if (rx < 0.0) rx = -rx;
    if (ry < 0.0) ry = -ry;

    if (largeArcFlag != 0) largeArcFlag = 1;
    if (sweepFlag != 0) sweepFlag = 1;
  
    /* 2. convert to center parameterization as shown in
     * http://www.w3.org/TR/SVG/implnote.html
     */
    sinPhi = sin(phi);
    cosPhi = cos(phi);
    dx = (x1-x2)/2.0;
    dy = (y1-y2)/2.0;
    x1dash =  cosPhi * dx + sinPhi * dy;
    y1dash = -sinPhi * dx + cosPhi * dy;

    /* Compute cx' and cy'. */
    numerator = rx*rx*ry*ry - rx*rx*y1dash*y1dash - ry*ry*x1dash*x1dash;
    if (numerator < 0.0) { 
    
        /* If rx , ry and are such that there is no solution (basically,
         * the ellipse is not big enough to reach from (x1, y1) to (x2,
         * y2)) then the ellipse is scaled up uniformly until there is
         * exactly one solution (until the ellipse is just big enough).
         * 	-> find factor s, such that numerator' with rx'=s*rx and
         *    ry'=s*ry becomes 0 :
         */
        float s = (float) sqrt(1.0 - numerator/(rx*rx*ry*ry));
    
        rx *= s;
        ry *= s;
        root = 0.0;
    } else {
        root = (largeArcFlag == sweepFlag ? -1.0 : 1.0) *
                sqrt( numerator/(rx*rx*y1dash*y1dash + ry*ry*x1dash*x1dash) );
    }
    
    cxdash =  root*rx*y1dash/ry;
    cydash = -root*ry*x1dash/rx;

    /* Compute cx and cy from cx' and cy'. */
    cx = cosPhi * cxdash - sinPhi * cydash + (x1+x2)/2.0;
    cy = sinPhi * cxdash + cosPhi * cydash + (y1+y2)/2.0;

    /* Compute start angle and extent. */
    theta1 = CalcVectorAngle(1.0, 0.0, (x1dash-cxdash)/rx, (y1dash-cydash)/ry);
    dtheta = CalcVectorAngle(
            (x1dash-cxdash)/rx,  (y1dash-cydash)/ry,
            (-x1dash-cxdash)/rx, (-y1dash-cydash)/ry);
    if (!sweepFlag && (dtheta > 0.0)) {
        dtheta -= 2.0*M_PI;
    } else if (sweepFlag && (dtheta < 0.0)) {
        dtheta += 2.0*M_PI;
    }
    *cxPtr = cx;
    *cyPtr = cy;
    *rxPtr = rx; 
    *ryPtr = ry;
    *theta1Ptr = theta1;
    *dthetaPtr = dtheta; 
    
    return kPathArcOK;
}

/*
 *--------------------------------------------------------------
 *
 * TableLooup
 *
 *	Look up an index from a statically allocated table of ints.
 *
 * Results:
 *	integer
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

int 
TableLookup(LookupTable *map, int n, int from)
{
    int i = 0;
    
    while ((i < n) && (from != map[i].from))
        i++;
    if (i == n) {
        return map[0].to;
    } else {
        return map[i].to;
    }
}

/*
 * Miscellaneous matrix utilities.
 */
 
void 
PathApplyTMatrix(TMatrix *m, double *x, double *y)
{
    if (m != NULL) {
        double tmpx = *x;
        double tmpy = *y;
        *x = tmpx*m->a + tmpy*m->c + m->tx;
        *y = tmpx*m->b + tmpy*m->d + m->ty;
    }
}

void 
PathApplyTMatrixToPoint(TMatrix *m, double in[2], double out[2])
{
    if (m == NULL) {
        out[0] = in[0];
        out[1] = in[1];
    } else {
        out[0] = in[0]*m->a + in[1]*m->c + m->tx;
        out[1] = in[0]*m->b + in[1]*m->d + m->ty;
    }
}

void
PathInverseTMatrix(TMatrix *m, TMatrix *mi)
{
    double det;
    
    /* @@@ We need error checking for det = 0 */
    det = m->a * m->d - m->b * m->c;
    mi->a  =  m->d/det;
    mi->b  = -m->b/det;
    mi->c  = -m->c/det;
    mi->d  =  m->a/det;
    mi->tx = (m->c * m->ty - m->d * m->tx)/det;
    mi->ty = (m->b * m->tx - m->a * m->ty)/det;
}

/*
 *----------------------------------------------------------------------
 *
 * MMulTMatrix --
 *
 *	Multiplies (concatenates) two matrices together and puts the
 *      result in m2.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TMatrix m2 modified
 *
 *----------------------------------------------------------------------
 */

void
MMulTMatrix(TMatrix *m1, TMatrix *m2)
{
    if (m1 == NULL) {
        return;
    }
    if (m2 == NULL) {
        /* Panic! */
    } else {
        TMatrix tmp = *m2;
        TMatrix *p = m2;
        
        p->a  = m1->a*tmp.a  + m1->b*tmp.c;
        p->b  = m1->a*tmp.b  + m1->b*tmp.d;
        p->c  = m1->c*tmp.a  + m1->d*tmp.c;
        p->d  = m1->c*tmp.b  + m1->d*tmp.d;
        p->tx = m1->tx*tmp.a + m1->ty*tmp.c + tmp.tx;
        p->ty = m1->tx*tmp.b + m1->ty*tmp.d + tmp.ty;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PathGetTMatrix --
 *
 *	Parses a Tcl list (in string) into a TMatrix record.  
 *
 * Results:
 *	Standard Tcl result
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
PathGetTMatrix(
        Tcl_Interp* interp, 
        CONST char *list, 	/* Object containg the lists for the matrix. */
        TMatrix *matrixPtr)	/* Where to store TMatrix corresponding
                                 * to list. Must be allocated! */
{
    CONST char **argv = NULL;
    CONST char **rowArgv = NULL;
    int i, j, argc, rowArgc;
    int result = TCL_OK;
    double tmp[3][2];

    /* Check matrix consistency. */
    if (Tcl_SplitList(interp, list, &argc, &argv) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    if (argc != 3) {
        Tcl_AppendResult(interp, "matrix \"", list, "\" is inconsistent",
                (char *) NULL);
        result = TCL_ERROR;
        goto bail;
    }
    
    /* Take each row in turn. */
    for (i = 0; i < 3; i++) {
        if (Tcl_SplitList(interp, argv[i], &rowArgc, &rowArgv) != TCL_OK) {
            result = TCL_ERROR;
            goto bail;
        }
        if (rowArgc != 2) {
            Tcl_AppendResult(interp, "matrix \"", list, "\" is inconsistent",
                    (char *) NULL);
            result = TCL_ERROR;
            goto bail;
        }
        for (j = 0; j < 2; j++) {
            if (Tcl_GetDouble(interp, rowArgv[j], &(tmp[i][j])) != TCL_OK) {
                Tcl_AppendResult(interp, "matrix \"", list, "\" is inconsistent",
                        (char *) NULL);
                result = TCL_ERROR;
                goto bail;
            }
        }
        if (rowArgv != NULL) {
            Tcl_Free((char *) rowArgv);
            rowArgv = NULL;
        }
    }
        
    /* Check that the matrix is not close to being singular. */
    if (fabs(tmp[0][0]*tmp[1][1] - tmp[0][1]*tmp[1][0]) < 1e-6) {
        Tcl_AppendResult(interp, "matrix \"", list, "\" is close to singular",
                (char *) NULL);
            result = TCL_ERROR;
            goto bail;
    }
        
    /* Matrix. */
    matrixPtr->a  = tmp[0][0];
    matrixPtr->b  = tmp[0][1];
    matrixPtr->c  = tmp[1][0];
    matrixPtr->d  = tmp[1][1];
    matrixPtr->tx = tmp[2][0];
    matrixPtr->ty = tmp[2][1];
    
bail:
    if (argv != NULL) {
        Tcl_Free((char *) argv);
    }
    if (rowArgv != NULL) {
        Tcl_Free((char *) rowArgv);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * PathGetTclObjFromTMatrix --
 *
 *	Parses a TMatrix record into a list object.
 *
 * Results:
 *	Standard Tcl result
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int
PathGetTclObjFromTMatrix(
        Tcl_Interp* interp, 
        TMatrix *matrixPtr,
        Tcl_Obj **listObjPtrPtr)
{
	Tcl_Obj		*listObj, *subListObj;
    
    /* @@@ Error handling remains. */

    listObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
    if (matrixPtr != NULL) {
        subListObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
        Tcl_ListObjAppendElement(interp, subListObj, Tcl_NewDoubleObj(matrixPtr->a));
        Tcl_ListObjAppendElement(interp, subListObj, Tcl_NewDoubleObj(matrixPtr->b));
        Tcl_ListObjAppendElement(interp, listObj, subListObj);
        
        subListObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
        Tcl_ListObjAppendElement(interp, subListObj, Tcl_NewDoubleObj(matrixPtr->c));
        Tcl_ListObjAppendElement(interp, subListObj, Tcl_NewDoubleObj(matrixPtr->d));
        Tcl_ListObjAppendElement(interp, listObj, subListObj);
        
        subListObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
        Tcl_ListObjAppendElement(interp, subListObj, Tcl_NewDoubleObj(matrixPtr->tx));
        Tcl_ListObjAppendElement(interp, subListObj, Tcl_NewDoubleObj(matrixPtr->ty));
        Tcl_ListObjAppendElement(interp, listObj, subListObj);
    }
    *listObjPtrPtr = listObj;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathGenericCmdDispatcher --
 *
 *	Supposed to be a generic command dispatcher.  
 *
 * Results:
 *	Standard Tcl result
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static CONST char *genericCmds[] = {
    "cget", "configure", "create", "delete", "names",
    (char *) NULL
};

enum {
	kPathGenericCmdCget						= 0L,
    kPathGenericCmdConfigure,
    kPathGenericCmdCreate,
    kPathGenericCmdDelete,
    kPathGenericCmdNames
};

int 
TkPathGenericCmdDispatcher( 
        Tcl_Interp* interp,
        Tk_Window tkwin,
        int objc,
      	Tcl_Obj* CONST objv[],
        char *baseName,
        int *baseNameUIDPtr,
        Tcl_HashTable *hashTablePtr,
        Tk_OptionTable optionTable,
        char *(*createAndConfigProc)(Tcl_Interp *interp, char *name, int objc, Tcl_Obj *CONST objv[]),
        void (*configNotifyProc)(char *recordPtr, int mask, int objc, Tcl_Obj *CONST objv[]),
        void (*freeProc)(Tcl_Interp *interp, char *recordPtr))
{
    char   		*name;
    char 		*recordPtr;
    int 		result = TCL_OK;
    int 		index;
    int			mask;
    Tcl_HashEntry *hPtr;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "command ?arg arg...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], genericCmds, "command", 0,
            &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch (index) {
    
        case kPathGenericCmdCget: {
            Tcl_Obj *resultObjPtr = NULL;
            
    		if (objc != 4) {
				Tcl_WrongNumArgs( interp, 3, objv, "option" );
    			return TCL_ERROR;
    		}
            name = Tcl_GetString(objv[2]);
            hPtr = Tcl_FindHashEntry(hashTablePtr, name);
            if (hPtr == NULL) {
                Tcl_AppendResult(interp, 
                        "object \"", name, "\" doesn't exist", NULL);
                return TCL_ERROR;
            }
            recordPtr = (char *) Tcl_GetHashValue(hPtr);
			resultObjPtr = Tk_GetOptionValue(interp, recordPtr, optionTable, objv[3], tkwin);
			if (resultObjPtr == NULL) {
				result = TCL_ERROR;
			} else {
				Tcl_SetObjResult( interp, resultObjPtr );
			}
            break;
        }
        
        case kPathGenericCmdConfigure: {
			Tcl_Obj *resultObjPtr = NULL;

			if (objc < 3) {
				Tcl_WrongNumArgs(interp, 2, objv, "name ?option? ?value option value...?");
				return TCL_ERROR;
			}
            name = Tcl_GetString(objv[2]);
            hPtr = Tcl_FindHashEntry(hashTablePtr, name);
            if (hPtr == NULL) {
                Tcl_AppendResult(interp, 
                        "object \"", name, "\" doesn't exist", NULL);
                return TCL_ERROR;
            }
            recordPtr = (char *) Tcl_GetHashValue(hPtr);
			if (objc <= 4) {
				resultObjPtr = Tk_GetOptionInfo(interp, recordPtr,
                        optionTable,
                        (objc == 3) ? (Tcl_Obj *) NULL : objv[3],
                        tkwin);
				if (resultObjPtr == NULL) {
					return TCL_ERROR;
                }
				Tcl_SetObjResult(interp, resultObjPtr);
			} else {
				if (Tk_SetOptions(interp, recordPtr, optionTable, objc - 3, objv + 3, 
                        tkwin, NULL, &mask) != TCL_OK) {
					return TCL_ERROR;
                }
                if (configNotifyProc != NULL) {
                    (*configNotifyProc)(recordPtr, mask, objc - 3, objv + 3);
                }
			}
            break;
        }
        
        case kPathGenericCmdCreate: {
        
            /*
             * Create with auto generated unique name.
             */
			char str[255];
			int isNew;

			if (objc < 2) {
				Tcl_WrongNumArgs(interp, 2, objv, "?option value...?");
				return TCL_ERROR;
			}
            sprintf(str, "%s%d", baseName, *baseNameUIDPtr);
            (*baseNameUIDPtr)++;
			recordPtr = (*createAndConfigProc)(interp, str, objc - 2, objv + 2);
			if (recordPtr == NULL) {
				return TCL_ERROR;
            }
            
            if (Tk_InitOptions(interp, recordPtr, optionTable, tkwin) != TCL_OK) {
                ckfree(recordPtr);
                return TCL_ERROR;
            }
            if (Tk_SetOptions(interp, recordPtr, optionTable, 	
                    objc - 2, objv + 2, tkwin, NULL, &mask) != TCL_OK) {
                Tk_FreeConfigOptions(recordPtr, optionTable, tkwin);
                ckfree(recordPtr);
                return TCL_ERROR;
            }
            if (configNotifyProc != NULL) {
                (*configNotifyProc)(recordPtr, mask, objc - 2, objv + 2);
            }

			hPtr = Tcl_CreateHashEntry(hashTablePtr, str, &isNew);
			Tcl_SetHashValue(hPtr, recordPtr);
			Tcl_SetObjResult(interp, Tcl_NewStringObj(str, -1));
            break;
        }
                
        case kPathGenericCmdDelete: {
			if (objc < 3) {
				Tcl_WrongNumArgs(interp, 2, objv, "name");
				return TCL_ERROR;
			}
            name = Tcl_GetString(objv[2]);
            hPtr = Tcl_FindHashEntry(hashTablePtr, name);
            recordPtr = (char *) Tcl_GetHashValue(hPtr);
			if (hPtr != NULL) {
                Tcl_DeleteHashEntry(hPtr);
			}
            Tk_FreeConfigOptions(recordPtr, optionTable, tkwin);
            (*freeProc)(interp, recordPtr);
			break;
        }
        
        case kPathGenericCmdNames: {
			Tcl_Obj *listObj;
			Tcl_HashSearch search;

			listObj = Tcl_NewListObj(0, NULL);
			hPtr = Tcl_FirstHashEntry(hashTablePtr, &search);
			while (hPtr != NULL) {
                name = (char *) Tcl_GetHashKey(hashTablePtr, hPtr);
				Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(name, -1));
				hPtr = Tcl_NextHashEntry(&search);
			}
			Tcl_SetObjResult(interp, listObj);
            break;
        }
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ObjectIsEmpty --
 *
 *	This procedure tests whether the string value of an object is
 *	empty.
 *
 * Results:
 *	The return value is 1 if the string value of objPtr has length
 *	zero, and 0 otherwise.
 *
 * Side effects:
 *	May cause object shimmering, since this function can force a
 *	conversion to a string object.
 *
 *----------------------------------------------------------------------
 */

int
ObjectIsEmpty(
        Tcl_Obj *objPtr)	/* Object to test.  May be NULL. */
{
    int length;

    if (objPtr == NULL) {
        return 1;
    }
    if (objPtr->bytes != NULL) {
        return (objPtr->length == 0);
    }
    Tcl_GetStringFromObj(objPtr, &length);
    return (length == 0);
}

static int
DashConvertToFloats (
    float *d,		/* The resulting dashes. (Out) */	
    CONST char *p,	/* A string of "_-,." */
    size_t n,
    double width)
{
    int result = 0;
    int size;

    if (n < 0) {
        n = strlen(p);
    }
    while (n-- && *p) {
        switch (*p++) {
            case ' ':
                if (result) {
                    if (d) {
                        d[-1] += (float) (width + 1.0);
                    }
                    continue;
                } else {
                    return 0;
                }
                break;
            case '_':
                size = 8;
                break;
            case '-':
                size = 6;
                break;
            case ',':
                size = 4;
                break;
            case '.':
                size = 2;
                break;
            default:
                return -1;
        }
        if (d) {
            *d++ = size * (float) width;
            *d++ = 4 * (float) width;
        }
        result += 2;
    }
    return result;
}

void
PathParseDashToArray(Tk_Dash *dash, double width, int *len, float **arrayPtrPtr)
{    
    char *pt;
    int	i;
    float *arrPtr = NULL;

    if (dash->number == 0) {
        *len = 0;
    } else if (dash->number < 0) {
        
        /* Any of . , - _ verbatim. */
        i = -1*dash->number;
        pt = (i > (int)sizeof(char *)) ? dash->pattern.pt : dash->pattern.array;
        arrPtr = (float *) ckalloc(2*i*sizeof(float));
        i = DashConvertToFloats(arrPtr, pt, i, width);
        if (i < 0) {
            /* This should never happen since syntax already checked. */
            *len = 0;
        } else {
            *len = i;
        }
    } else {
        pt = (dash->number > (int)sizeof(char *)) ? dash->pattern.pt : dash->pattern.array;
        *len = dash->number;
        arrPtr = (float *) ckalloc(dash->number * sizeof(float));
        for (i = 0; i < dash->number; i++) {
        
            /* We could optionally multiply with 'width' here. */
            arrPtr[i] = pt[i];
        }
    }
    *arrayPtrPtr = arrPtr;
}

/*-------------------------------------------------------------------------*/

