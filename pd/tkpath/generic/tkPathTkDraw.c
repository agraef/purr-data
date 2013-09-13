/*
 * tkPathTkDraw.c --
 *
 *		This file implements a path canvas item modelled after its
 *      SVG counterpart. See http://www.w3.org/TR/SVG11/.
 *		
 * 	Note:
 *		This is supposed to be a minimal implementation using
 *		Tk drawing only. It fails in a number of places such as
 *		filled and overlapping subpaths.
 *
 * Copyright (c) 2005-2008  Mats Bengtsson
 *
 * $Id: tkPathTkDraw.c,v 1.25 2008/05/22 06:18:21 matben Exp $
 */

#include "tkIntPath.h"

#define _PATH_N_BUFFER_POINTS 		2000

extern int gAntiAlias;

extern void	CurveSegments(double control[], int includeFirst, int numSteps, register double *coordPtr);

/*
 * Each subpath is reconstructed as a number of straight line segments.
 * These are always stored as transformed coordinates.
 */
typedef struct _PathSegments {
    int 			npoints;	/* Number of points in points array. */
    double 			*points;	/* The actual point coordinates. */
    int				size;		/* The number of _points_ allocated in points array. */	
    int				isclosed;
    struct _PathSegments *next;
} _PathSegments;

/*
 * A placeholder for the context we are working in.
 * The current and lastMove are always original untransformed coordinates.
 */
typedef struct TkPathContext_ {
    Display 		*display;
    Drawable 		drawable;
    double 			current[2];
    double 			lastMove[2];
    int				hasCurrent;
    TMatrix 		*m;
    _PathSegments 	*segm;
    _PathSegments 	*currentSegm;
} TkPathContext_;


static TkPathContext_* _NewPathContext(Tk_Window tkwin, Drawable drawable)
{
    TkPathContext_ *ctx;
    
    ctx = (TkPathContext_ *) ckalloc(sizeof(TkPathContext_));
    ctx->display = Tk_Display(tkwin);
    ctx->drawable = drawable;
    ctx->current[0] = 0.0;
    ctx->current[1] = 0.0;
    ctx->lastMove[0] = 0.0;
    ctx->lastMove[1] = 0.0;
    ctx->hasCurrent = 0;
    ctx->m = NULL;
    ctx->segm = NULL;
    ctx->currentSegm = NULL;
    return ctx;
}

static _PathSegments* _NewPathSegments(void)
{
    _PathSegments *segm;
    
    segm = (_PathSegments *) ckalloc(sizeof(_PathSegments));
    segm->npoints = 0;
    segm->points = (double *) ckalloc((unsigned) (2*_PATH_N_BUFFER_POINTS*sizeof(double)));
    segm->size = _PATH_N_BUFFER_POINTS;
    segm->isclosed = 0;
    segm->next = NULL;
    return segm;
}

static void _PathContextFree(TkPathContext_ *ctx)
{
    _PathSegments *tmpSegm, *segm;

    segm = ctx->segm;
    while (segm != NULL) {
        tmpSegm = segm;
        segm = tmpSegm->next;
        ckfree((char *) tmpSegm->points);
        ckfree((char *) tmpSegm);
    }
    if (ctx->m != NULL) {
        ckfree((char *) ctx->m);
    }
    ckfree((char *) ctx);
}

static void _CheckCoordSpace(_PathSegments *segm, int numPoints)
{
    if (segm->npoints + numPoints >= segm->size) {
        double *points;
        points = (double *) ckrealloc((char *)segm->points, 2*(segm->size + _PATH_N_BUFFER_POINTS)*sizeof(double));
        segm->points = points;
    }
}

TkPathContext TkPathInit(Tk_Window tkwin, Drawable d)
{
    return (TkPathContext) _NewPathContext(tkwin, d);
}

TkPathContext TkPathInitSurface(int width, int height)
{

}

void
TkPathPushTMatrix(TkPathContext ctx, TMatrix *m)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;

    if (m == NULL) {
        return;
    }
    if (context->m == NULL) {
        context->m = (TMatrix *) ckalloc(sizeof(TMatrix));
        *(context->m) = *m;
    } else {
        TMatrix tmp = *(context->m);
        TMatrix *p = context->m;
        
        p->a  = m->a*tmp.a  + m->b*tmp.c;
        p->b  = m->a*tmp.b  + m->b*tmp.d;
        p->c  = m->c*tmp.a  + m->d*tmp.c;
        p->d  = m->c*tmp.b  + m->d*tmp.d;
        p->tx = m->tx*tmp.a + m->ty*tmp.c + tmp.tx;
        p->ty = m->tx*tmp.b + m->ty*tmp.d + tmp.ty;
    }
}

void TkPathSaveState(TkPathContext ctx)
{

}

void TkPathRestoreState(TkPathContext ctx)
{

}

void TkPathBeginPath(TkPathContext ctx, Tk_PathStyle *style)
{
    /* TkPathContext_ *context = (TkPathContext_ *) ctx; */
    /* empty */
}

void TkPathMoveTo(TkPathContext ctx, double x, double y)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    double *coordPtr;
    _PathSegments *segm;

    segm = _NewPathSegments();
    if (context->segm == NULL) {
        context->segm = segm;
    } else {
        context->currentSegm->next = segm;
    }
    context->currentSegm = segm;
    context->hasCurrent = 1;
    context->current[0] = x;
    context->current[1] = y;
    context->lastMove[0] = x;
    context->lastMove[1] = y;
    coordPtr = segm->points;
    PathApplyTMatrixToPoint(context->m, context->current, coordPtr);
    segm->npoints = 1;
}

void TkPathLineTo(TkPathContext ctx, double x, double y)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    double *coordPtr;
    _PathSegments *segm;
    
    segm = context->currentSegm;
    _CheckCoordSpace(segm, 1);
    context->current[0] = x;
    context->current[1] = y;
    coordPtr = segm->points + 2*segm->npoints;
    PathApplyTMatrixToPoint(context->m, context->current, coordPtr);    
    (segm->npoints)++;
}

void TkPathQuadBezier(TkPathContext ctx, double ctrlX, double ctrlY, double x, double y)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    double cx, cy;
    double x31, y31, x32, y32;
    
    cx = context->current[0];
    cy = context->current[1];

    // conversion of quadratic bezier curve to cubic bezier curve: (mozilla/svg)
    /* Unchecked! Must be an approximation! */
    x31 = cx + (ctrlX - cx) * 2 / 3;
    y31 = cy + (ctrlY - cy) * 2 / 3;
    x32 = ctrlX + (x - ctrlX) / 3;
    y32 = ctrlY + (y - ctrlY) / 3;

    TkPathCurveTo(ctx, x31, y31, x32, y32, x, y);
}

void TkPathCurveTo(TkPathContext ctx, double x1, double y1, 
        double x2, double y2, double x, double y)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    int numSteps;
    double *coordPtr;
    double control[8];
    double xc, yc;
    _PathSegments *segm;

    xc = x;
    yc = y;

    PathApplyTMatrixToPoint(context->m, context->current, control);
    PathApplyTMatrix(context->m, &x1, &y1);
    PathApplyTMatrix(context->m, &x2, &y2);
    PathApplyTMatrix(context->m, &x, &y);
    control[2] = x1;
    control[3] = y1;
    control[4] = x2;
    control[5] = y2;
    control[6] = x;
    control[7] = y;

    numSteps = kPathNumSegmentsCurveTo;
    segm = context->currentSegm;
    _CheckCoordSpace(segm, numSteps);
    coordPtr = segm->points + 2*segm->npoints;
    CurveSegments(control, 0, numSteps, coordPtr);
    segm->npoints += numSteps;
    context->current[0] = xc;
    context->current[1] = yc;
}

void TkPathArcTo(TkPathContext ctx,
        double rx, double ry, 
        double phiDegrees, 	/* The rotation angle in degrees! */
        char largeArcFlag, char sweepFlag, double x, double y)
{
    TkPathArcToUsingBezier(ctx, rx, ry, phiDegrees, largeArcFlag, sweepFlag, x, y);
}

void
TkPathRect(TkPathContext ctx, double x, double y, double width, double height)
{
    TkPathMoveTo(ctx, x, y);
    TkPathLineTo(ctx, x+width, y);
    TkPathLineTo(ctx, x+width, y+height);
    TkPathLineTo(ctx, x, y+height);
    TkPathClosePath(ctx);
}

void
TkPathOval(TkPathContext ctx, double cx, double cy, double rx, double ry)
{
    /* @@@ I'm sure this could be made much more efficient. */
    TkPathMoveTo(ctx, cx+rx, cy);
    TkPathArcToUsingBezier(ctx, rx, ry, 0.0, 1, 1, cx-rx, cy);
    TkPathArcToUsingBezier(ctx, rx, ry, 0.0, 1, 1, cx+rx, cy);
    TkPathClosePath(ctx);
}

void
TkPathImage(TkPathContext ctx, Tk_Image image, Tk_PhotoHandle photo, double x, double y, double width, double height)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    int iwidth, iheight;

    if (image == NULL) {
        return;
    }
    PathApplyTMatrix(context->m, &x, &y);
    Tk_SizeOfImage(image, &iwidth, &iheight);
    Tk_RedrawImage(image, 0, 0, iwidth, iheight, context->drawable, (int)x, (int)y);
}

void TkPathClosePath(TkPathContext ctx)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    double *coordPtr;
    _PathSegments *segm;

    segm = context->currentSegm;
    _CheckCoordSpace(segm, 1);
    segm->isclosed = 1;
    context->current[0] = context->lastMove[0];
    context->current[1] = context->lastMove[1];
    coordPtr = segm->points + 2*segm->npoints;
    PathApplyTMatrixToPoint(context->m, context->current, coordPtr);    
    (segm->npoints)++;
}

/*
 * There is no need to reproduce the Tk drawing code here since we can't do
 * anything different.
 */
 
int
TkPathTextConfig(Tcl_Interp *interp, Tk_PathTextStyle *textStylePtr, char *utf8, void **customPtr)
{
    return TCL_OK;
}

void
TkPathTextDraw(TkPathContext ctx, Tk_PathStyle *style, Tk_PathTextStyle *textStylePtr, double x, double y, char *utf8, void *custom)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;

}

void
TkPathTextFree(Tk_PathTextStyle *textStylePtr, void *custom)
{

}

PathRect
TkPathTextMeasureBbox(Tk_PathTextStyle *textStylePtr, char *utf8, void *custom)
{
    PathRect r = {0, 0, 0, 0};
    return r;
}

void    	
TkPathSurfaceErase(TkPathContext ctx, double x, double y, double width, double height)
{

}

void
TkPathSurfaceToPhoto(Tcl_Interp *interp, TkPathContext ctx, Tk_PhotoHandle photo)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;

}

void TkPathClipToPath(TkPathContext ctx, int fillRule)
{
    /* empty */
}

void TkPathReleaseClipToPath(TkPathContext ctx)
{
    /* empty */
}

/* @@@ This is a very much simplified version of TkPathCanvTranslatePath that
 * doesn't do any clipping and no translation since we do that with
 * the more general affine matrix transform.
 */
static void _DoubleCoordsToXPointArray(int npoints, double *coordArr, XPoint *outArr)
{
    int i;
    double x, y;
    
    for(i = 0; i < npoints; i++){
        x = coordArr[i*2];
        y = coordArr[i*2+1];
    
        if (x > 0) {
            x += 0.5;
        } else {
            x -= 0.5;
        }
        outArr[i].x = (short) x;
    
        if (y > 0) {
            y += 0.5;
        } else {
            y -= 0.5;
        }
        outArr[i].y = (short) y;
    }
}

void TkPathStroke(TkPathContext ctx, Tk_PathStyle *style)
{       
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    int numPoints;
    XPoint *pointPtr;
    _PathSegments *segm;
    
    segm = context->segm;
    while (segm != NULL) {
        numPoints = segm->npoints;
        pointPtr = (XPoint *)ckalloc((unsigned)(numPoints * sizeof(XPoint)));
        _DoubleCoordsToXPointArray(numPoints, segm->points, pointPtr);
        XDrawLines(context->display, context->drawable, style->strokeGC, pointPtr, numPoints,
                CoordModeOrigin);
        ckfree((char *) pointPtr);
        segm = segm->next;
    }
}

void TkPathFill(TkPathContext ctx, Tk_PathStyle *style)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    int numPoints;
    XPoint *pointPtr;
    _PathSegments *segm;
    
    segm = context->segm;
    while (segm != NULL) {
        numPoints = segm->npoints;
        pointPtr = (XPoint *)ckalloc((unsigned)(numPoints * sizeof(XPoint)));
        _DoubleCoordsToXPointArray(numPoints, segm->points, pointPtr);
        XFillPolygon(context->display, context->drawable, style->fillGC, pointPtr, numPoints,
                Complex, CoordModeOrigin);
        ckfree((char *) pointPtr);
        segm = segm->next;
    }
}

void TkPathFillAndStroke(TkPathContext ctx, Tk_PathStyle *style)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    int numPoints;
    XPoint *pointPtr;
    _PathSegments *segm;
    
    segm = context->segm;
    while (segm != NULL) {
        numPoints = segm->npoints;
        pointPtr = (XPoint *)ckalloc((unsigned)(numPoints * sizeof(XPoint)));
        _DoubleCoordsToXPointArray(numPoints, segm->points, pointPtr);
        XFillPolygon(context->display, context->drawable, style->fillGC, pointPtr, numPoints,
                Complex, CoordModeOrigin);
        XDrawLines(context->display, context->drawable, style->strokeGC, pointPtr, numPoints,
                CoordModeOrigin);
        ckfree((char *) pointPtr);
        segm = segm->next;
    }
}

void TkPathEndPath(TkPathContext ctx)
{
    /* empty */
}

void TkPathFree(TkPathContext ctx)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    _PathContextFree(context);
}

int TkPathDrawingDestroysPath(void)
{
    return 0;
}

int		
TkPathPixelAlign(void)
{
    return 1;
}

int TkPathGetCurrentPosition(TkPathContext ctx, PathPoint *pt)
{
    TkPathContext_ *context = (TkPathContext_ *) ctx;
    pt->x = context->current[0];
    pt->y = context->current[1];
    return TCL_OK;
}

int TkPathBoundingBox(TkPathContext ctx, PathRect *rPtr)
{
    return TCL_ERROR;
}

void TkPathPaintLinearGradient(TkPathContext ctx, PathRect *bbox, LinearGradientFill *fillPtr, int fillRule, TMatrix *mPtr)
{    
    /* TkPathContext_ *context = (TkPathContext_ *) ctx; */
    /* The Tk X11 compatibility layer does not have tha ability to set up
     * clipping to pixmap which is needed here, I believe. 
     */
}
            
void
TkPathPaintRadialGradient(TkPathContext ctx, PathRect *bbox, RadialGradientFill *fillPtr, int fillRule, TMatrix *mPtr)
{
}

