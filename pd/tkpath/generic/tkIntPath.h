/*
 * tkIntPath.h --
 *
 *	Header file for the internals of the tkpath package.
 *
 * Copyright (c) 2005-2008  Mats Bengtsson
 *
 * $Id$
 */

#ifndef INCLUDED_TKINTPATH_H
#define INCLUDED_TKINTPATH_H

#include "tkPath.h"

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * From tclInt.h version 1.118.2.11
 * Ensure WORDS_BIGENDIAN is defined correcly:
 * Needs to happen here in addition to configure to work with
 * fat compiles on Darwin (i.e. ppc and i386 at the same time).
 */
 
#ifndef WORDS_BIGENDIAN
#	ifdef HAVE_SYS_TYPES_H
#		include <sys/types.h>
#	endif
#	ifdef HAVE_SYS_PARAM_H
#		include <sys/param.h>
#	endif
#   ifdef BYTE_ORDER
#		ifdef BIG_ENDIAN
#			if BYTE_ORDER == BIG_ENDIAN
#				define WORDS_BIGENDIAN
#			endif
#		endif
#		ifdef LITTLE_ENDIAN
#			if BYTE_ORDER == LITTLE_ENDIAN
#				undef WORDS_BIGENDIAN
#			endif
#		endif
#	endif
#endif


#ifndef MIN
#	define MIN(a, b) 	(((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#	define MAX(a, b) 	(((a) > (b)) ? (a) : (b))
#endif
#ifndef ABS
#	define ABS(a)    	(((a) >= 0)  ? (a) : -1*(a))
#endif
#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif
#define DEGREES_TO_RADIANS (M_PI/180.0)
#define RADIANS_TO_DEGREES (180.0/M_PI)


/* 
 * This can be useful to estimate the segmentation detail necessary.
 * A conservative measure.
 */
#define TMATRIX_ABS_MAX(mPtr)		MAX(fabs(mPtr->a), MAX(fabs(mPtr->b), MAX(fabs(mPtr->c), fabs(mPtr->d))))

/* 
 * This can be used for simplifying Area and Point functions.
 */
#define TMATRIX_IS_RECTILINEAR(mPtr)   	(fabs(mPtr->b) == 0.0) && (fabs(mPtr->c) == 0.0)

#define TMATRIX_DETERMINANT(mPtr)	(mPtr->a * mPtr->d - mPtr->c * mPtr->d)

/*
 * Iff stroke width is an integer, widthCode=1,2, move coordinate
 * to pixel boundary if even stroke width, widthCode=2,
 * or to pixel center if odd stroke width, widthCode=1.
 */
#define PATH_DEPIXELIZE(widthCode,x)     (!(widthCode) ? (x) : ((int) (floor((x) + 0.001)) + (((widthCode) == 1) ? 0.5 : 0)));

#define GetColorFromPathColor(pcol) 		(((pcol != NULL) && (pcol->color != NULL)) ? pcol->color : NULL )
#define GetGradientMasterFromPathColor(pcol)	(((pcol != NULL) && (pcol->gradientInstPtr != NULL)) ? pcol->gradientInstPtr->masterPtr : NULL )
#define HaveAnyFillFromPathColor(pcol) 		(((pcol != NULL) && ((pcol->color != NULL) || (pcol->gradientInstPtr != NULL))) ? 1 : 0 )

/*
 * So far we use a fixed number of straight line segments when
 * doing various things, but it would be better to use the de Castlejau
 * algorithm to iterate these segments.
 */
#define kPathNumSegmentsCurveTo     	18
#define kPathNumSegmentsQuadBezier 	12
#define kPathNumSegmentsMax		18
#define kPathNumSegmentsEllipse         48

#define kPathUnitTMatrix  {1.0, 0.0, 0.0, 1.0, 0.0, 0.0}

/*
 * Flag bits for gradient and style changes.
 */
enum {
    PATH_GRADIENT_FLAG_CONFIGURE	= (1L << 0),
    PATH_GRADIENT_FLAG_DELETE
};

enum {
    PATH_STYLE_FLAG_CONFIGURE		= (1L << 0),
    PATH_STYLE_FLAG_DELETE
};

extern int gAntiAlias;

enum {
    kPathTextAnchorStart		= 0L,
    kPathTextAnchorMiddle,
    kPathTextAnchorEnd
};

/* These MUST be kept in sync with methodST and unitsST! */
enum {
    kPathGradientMethodPad		= 0L,
    kPathGradientMethodRepeat,
    kPathGradientMethodReflect
};
enum {
    kPathGradientUnitsBoundingBox =	0L,
    kPathGradientUnitsUserSpace
};

enum {
    kPathArcOK,
    kPathArcLine,
    kPathArcSkip
};

typedef struct PathBox {
    double x;
    double y;
    double width;
    double height;
} PathBox;

typedef struct CentralArcPars {
    double cx;
    double cy;
    double rx;
    double ry;
    double theta1;
    double dtheta;
    double phi;
} CentralArcPars;

typedef struct LookupTable {
    int from;
    int to;
} LookupTable;

/*
 * Records used for parsing path to a linked list of primitive 
 * drawing instructions.
 *
 * PathAtom: vaguely modelled after Tk_PathItem. Each atom has a PathAtom record
 * in its first position, padded with type specific data.
 */

typedef struct MoveToAtom {
    PathAtom pathAtom;		/* Generic stuff that's the same for all
                                 * types.  MUST BE FIRST IN STRUCTURE. */
    double x;
    double y;
} MoveToAtom;

typedef struct LineToAtom {
    PathAtom pathAtom;
    double x;
    double y;
} LineToAtom;

typedef struct ArcAtom {
    PathAtom pathAtom;
    double radX;
    double radY;
    double angle;		/* In degrees! */
    char largeArcFlag;
    char sweepFlag;
    double x;
    double y;
} ArcAtom;

typedef struct QuadBezierAtom {
    PathAtom pathAtom;
    double ctrlX;
    double ctrlY;
    double anchorX;
    double anchorY;
} QuadBezierAtom;

typedef struct CurveToAtom {
    PathAtom pathAtom;
    double ctrlX1;
    double ctrlY1;
    double ctrlX2;
    double ctrlY2;
    double anchorX;
    double anchorY;
} CurveToAtom;

typedef struct CloseAtom {
    PathAtom pathAtom;
    double x;
    double y;
} CloseAtom;

typedef struct EllipseAtom {
    PathAtom pathAtom;
    double cx;
    double cy;
    double rx;
    double ry;
} EllipseAtom;

typedef struct RectAtom {
    PathAtom pathAtom;
    double x;
    double y;
    double width;
    double height;
} RectAtom;

/*
 * Flags for 'TkPathStyleMergeStyles'.
 */
 
enum {
    kPathMergeStyleNotFill = 		0L,
    kPathMergeStyleNotStroke
};

/*
 * The actual path drawing commands which are all platform specific.
 */

/* === EB - 23-apr-2010: added function to register coordinate offsets */
/* Should be called before TkPathInit for correct sizing of drawing region*/
void TkPathSetCoordOffsets(double dx, double dy);
/* === */
TkPathContext	TkPathInit(Tk_Window tkwin, Drawable d);
TkPathContext	TkPathInitSurface(int width, int height);
void		TkPathBeginPath(TkPathContext ctx, Tk_PathStyle *stylePtr);
void    	TkPathEndPath(TkPathContext ctx);
void		TkPathMoveTo(TkPathContext ctx, double x, double y);
void		TkPathLineTo(TkPathContext ctx, double x, double y);
void		TkPathArcTo(TkPathContext ctx, double rx, double ry, double angle, 
                    char largeArcFlag, char sweepFlag, double x, double y);
void		TkPathQuadBezier(TkPathContext ctx, double ctrlX, double ctrlY, double x, double y);
void		TkPathCurveTo(TkPathContext ctx, double ctrlX1, double ctrlY1, 
                    double ctrlX2, double ctrlY2, double x, double y);
void		TkPathArcToUsingBezier(TkPathContext ctx, double rx, double ry, 
                    double phiDegrees, char largeArcFlag, char sweepFlag, 
                    double x2, double y2);
void		TkPathRect(TkPathContext ctx, double x, double y, double width, double height);
void		TkPathOval(TkPathContext ctx, double cx, double cy, double rx, double ry);
void		TkPathClosePath(TkPathContext ctx);
void		TkPathImage(TkPathContext ctx, Tk_Image image, Tk_PhotoHandle photo, 
                    double x, double y, double width, double height);
int			TkPathTextConfig(Tcl_Interp *interp, Tk_PathTextStyle *textStylePtr, char *utf8, void **customPtr);
void		TkPathTextDraw(TkPathContext ctx, Tk_PathStyle *style, 
                    Tk_PathTextStyle *textStylePtr, double x, double y, char *utf8, void *custom);
void		TkPathTextFree(Tk_PathTextStyle *textStylePtr, void *custom);
PathRect	TkPathTextMeasureBbox(Tk_PathTextStyle *textStylePtr, char *utf8, void *custom);
void    	TkPathSurfaceErase(TkPathContext ctx, double x, double y, double width, double height);
void		TkPathSurfaceToPhoto(Tcl_Interp *interp, TkPathContext ctx, Tk_PhotoHandle photo);

/*
 * General path drawing using linked list of path atoms.
 */
void		TkPathDrawPath(Tk_Window tkwin, Drawable drawable,
                    PathAtom *atomPtr, Tk_PathStyle *stylePtr, TMatrix *mPtr,			
                    PathRect *bboxPtr);
void		TkPathPaintPath(TkPathContext context, PathAtom *atomPtr,
                    Tk_PathStyle *stylePtr, PathRect *bboxPtr);
PathRect	TkPathGetTotalBbox(PathAtom *atomPtr, Tk_PathStyle *stylePtr);

void		TkPathMakePrectAtoms(double *pointsPtr, double rx, double ry, PathAtom **atomPtrPtr);
TkPathColor *	TkPathNewPathColor(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *nameObj);
void		TkPathFreePathColor(TkPathColor *colorPtr);
TkPathColor *	TkPathGetPathColor(Tcl_Interp *interp, Tk_Window tkwin, 
		    Tcl_Obj *nameObj, Tcl_HashTable *tablePtr,
		    TkPathGradientChangedProc *changeProc, ClientData clientData);

/*
 * From the generic tk code normally in tkIntDecls.h 
 */

void		TkPathIncludePoint(register Tk_PathItem *itemPtr, double *pointPtr);
void		TkPathBezierScreenPoints(Tk_PathCanvas canvas, double control[],
		    int numSteps, register XPoint *xPointPtr);
void		TkPathBezierPoints(double control[], int numSteps,
		    register double *coordPtr);
int		TkPathMakeBezierCurve(Tk_PathCanvas canvas, double *pointPtr,
		    int numPoints, int numSteps, XPoint xPoints[], double dblPoints[]);
int		TkPathMakeRawCurve(Tk_PathCanvas canvas, double *pointPtr,
		    int numPoints, int numSteps, XPoint xPoints[], double dblPoints[]);
void		TkPathMakeBezierPostscript(Tcl_Interp *interp, Tk_PathCanvas canvas,
		    double *pointPtr, int numPoints);
void		TkPathMakeRawCurvePostscript(Tcl_Interp *interp, Tk_PathCanvas canvas,
		    double *pointPtr, int numPoints);
void		TkPathFillPolygon(Tk_PathCanvas canvas, double *coordPtr, int numPoints,
		    Display *display, Drawable drawable, GC gc, GC outlineGC);
		
/* 
 * Various stuff.
 */
 
int 		TableLookup(LookupTable *map, int n, int from);
void		PathParseDashToArray(Tk_Dash *dash, double width, int *len, float **arrayPtrPtr);
void 		PathApplyTMatrix(TMatrix *m, double *x, double *y);
void 		PathApplyTMatrixToPoint(TMatrix *m, double in[2], double out[2]);
void		PathInverseTMatrix(TMatrix *m, TMatrix *mi);
void		MMulTMatrix(TMatrix *m1, TMatrix *m2);
void		PathCopyBitsARGB(unsigned char *from, unsigned char *to, 
                    int width, int height, int bytesPerRow);
void		PathCopyBitsBGRA(unsigned char *from, unsigned char *to, 
                    int width, int height, int bytesPerRow);
void		PathCopyBitsPremultipliedAlphaRGBA(unsigned char *from, unsigned char *to, 
                    int width, int height, int bytesPerRow);
void		PathCopyBitsPremultipliedAlphaARGB(unsigned char *from, unsigned char *to, 
                    int width, int height, int bytesPerRow);
void		PathCopyBitsPremultipliedAlphaBGRA(unsigned char *from, unsigned char *to, 
                    int width, int height, int bytesPerRow);

int		ObjectIsEmpty(Tcl_Obj *objPtr);
int		PathGetTMatrix(Tcl_Interp* interp, CONST char *list, TMatrix *matrixPtr);
int		PathGetTclObjFromTMatrix(Tcl_Interp* interp, TMatrix *matrixPtr,
                    Tcl_Obj **listObjPtrPtr);

int		EndpointToCentralArcParameters(
                    double x1, double y1, double x2, double y2,	/* The endpoints. */
                    double rx, double ry,				/* Radius. */
                    double phi, char largeArcFlag, char sweepFlag,
                    double *cxPtr, double *cyPtr, 			/* Out. */
                    double *rxPtr, double *ryPtr,
                    double *theta1Ptr, double *dthetaPtr);

MODULE_SCOPE int    TkPathGenericCmdDispatcher( 
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
			void (*freeProc)(Tcl_Interp *interp, char *recordPtr));
void		    PathStyleInit(Tcl_Interp* interp);
void		    PathGradientInit(Tcl_Interp* interp);
MODULE_SCOPE void   TkPathStyleMergeStyles(Tk_PathStyle *srcStyle, Tk_PathStyle *dstStyle, 
			long flags);
MODULE_SCOPE int    TkPathStyleMergeStyleStatic(Tcl_Interp* interp, Tcl_Obj *styleObj, 
			Tk_PathStyle *dstStyle, long flags);
MODULE_SCOPE void   PathGradientPaint(TkPathContext ctx, PathRect *bbox, 
			TkPathGradientMaster *gradientStylePtr, int fillRule);


MODULE_SCOPE Tk_PathSmoothMethod	tkPathBezierSmoothMethod;

MODULE_SCOPE int    Tk_PathCanvasObjCmd(ClientData clientData,
			Tcl_Interp *interp, int argc, Tcl_Obj *const objv[]);

/*
 * Gradient support functions.
 */
 
MODULE_SCOPE int    PathGradientCget(Tcl_Interp *interp, Tk_Window tkwin, 
			int objc, Tcl_Obj * CONST objv[], 
			Tcl_HashTable *hashTablePtr);
MODULE_SCOPE int    PathGradientConfigure(Tcl_Interp *interp, Tk_Window tkwin, 
			int objc, Tcl_Obj * CONST objv[], 
			Tcl_HashTable *hashTablePtr);
MODULE_SCOPE int    PathGradientCreate(Tcl_Interp *interp, Tk_Window tkwin, 
			int objc, Tcl_Obj * CONST objv[],
			Tcl_HashTable *hashTablePtr, char *tokenName);
MODULE_SCOPE int    PathGradientDelete(Tcl_Interp *interp, Tcl_Obj *obj, 
			Tcl_HashTable *hashTablePtr);
MODULE_SCOPE int    PathGradientInUse(Tcl_Interp *interp, Tcl_Obj *obj, Tcl_HashTable *tablePtr);
MODULE_SCOPE void   PathGradientNames(Tcl_Interp *interp, Tcl_HashTable *hashTablePtr);
MODULE_SCOPE int    PathGradientType(Tcl_Interp *interp, Tcl_Obj *obj, 
			Tcl_HashTable *hashTablePtr);
MODULE_SCOPE void   PathGradientMasterFree(TkPathGradientMaster *gradientPtr);

/*
 * Style support functions.
 */

MODULE_SCOPE int    PathStyleCget(Tcl_Interp *interp, Tk_Window tkwin, 
			int objc, Tcl_Obj * CONST objv[], 
			Tcl_HashTable *hashTablePtr);
MODULE_SCOPE int    PathStyleConfigure(Tcl_Interp *interp, Tk_Window tkwin, 
			int objc, Tcl_Obj * CONST objv[], 
			Tcl_HashTable *hashTablePtr, Tcl_HashTable *gradTablePtr);
MODULE_SCOPE int    PathStyleCreate(Tcl_Interp *interp, Tk_Window tkwin, 
			int objc, Tcl_Obj * CONST objv[],
			Tcl_HashTable *styleTablePtr, 
			Tcl_HashTable *gradTablePtr, char *tokenName);
MODULE_SCOPE int    PathStyleDelete(Tcl_Interp *interp, Tcl_Obj *obj, 
			Tcl_HashTable *hashTablePtr, Tk_Window tkwin);
MODULE_SCOPE int    PathStyleInUse(Tcl_Interp *interp, Tcl_Obj *obj, Tcl_HashTable *tablePtr);
MODULE_SCOPE void   PathStyleNames(Tcl_Interp *interp, Tcl_HashTable *hashTablePtr);

/*
 * As TK_OPTION_PIXELS but double internal value instead of int.
 */
 
MODULE_SCOPE int	Tk_PathPixelOptionSetProc(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj **value,	
			    char *recordPtr, int internalOffset, char *oldInternalPtr, int flags);
MODULE_SCOPE Tcl_Obj *	Tk_PathPixelOptionGetProc(ClientData clientData,
			    Tk_Window tkwin, char *recordPtr, int internalOffset);
MODULE_SCOPE void	Tk_PathPixelOptionRestoreProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);

MODULE_SCOPE int	Tk_DashOptionSetProc(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj **value,
			    char *recordPtr, int internalOffset, char *oldInternalPtr,
			    int flags);
MODULE_SCOPE Tcl_Obj *	Tk_DashOptionGetProc(ClientData clientData,
			    Tk_Window tkwin, char *recordPtr, int internalOffset);
MODULE_SCOPE void	Tk_DashOptionRestoreProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);
MODULE_SCOPE void	Tk_DashOptionFreeProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr);

MODULE_SCOPE int	TkPathOffsetOptionSetProc(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj **value,
			    char *recordPtr, int internalOffset, char *oldInternalPtr,
			    int flags);
MODULE_SCOPE Tcl_Obj *	TkPathOffsetOptionGetProc(ClientData clientData,
			    Tk_Window tkwin, char *recordPtr, int internalOffset);
MODULE_SCOPE void	TkPathOffsetOptionRestoreProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);
MODULE_SCOPE void	TkPathOffsetOptionFreeProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr);

MODULE_SCOPE int	TkPathSmoothOptionSetProc(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj **value,
			    char *recordPtr, int internalOffset, char *oldInternalPtr,
			    int flags);
MODULE_SCOPE Tcl_Obj *	TkPathSmoothOptionGetProc(ClientData clientData,
			    Tk_Window tkwin, char *recordPtr, int internalOffset);
MODULE_SCOPE void	TkPathSmoothOptionRestoreProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);

MODULE_SCOPE int	TkPathPostscriptImage(Tcl_Interp *interp, Tk_Window tkwin,
			    Tk_PostscriptInfo psInfo, XImage *ximage,
			    int x, int y, int width, int height);
MODULE_SCOPE void	PathStylesFree(Tk_Window tkwin, Tcl_HashTable *hashTablePtr);
MODULE_SCOPE TkPathColor *  TkPathGetPathColorStatic(Tcl_Interp *interp, 
			    Tk_Window tkwin, Tcl_Obj *nameObj);

/* 
 * Support functions for gradient instances.
 */
MODULE_SCOPE TkPathGradientInst *TkPathGetGradient(Tcl_Interp *interp, CONST char *name, 
			    Tcl_HashTable *tablePtr, TkPathGradientChangedProc *changeProc, 
			    ClientData clientData);
MODULE_SCOPE void	TkPathFreeGradient(TkPathGradientInst *gradientPtr);
MODULE_SCOPE void	TkPathGradientChanged(TkPathGradientMaster *masterPtr, int flags);

MODULE_SCOPE void	TkPathStyleChanged(Tk_PathStyle *masterPtr, int flags);

/*
 * end block for C++
 */
    
#ifdef __cplusplus
}
#endif

#endif      // INCLUDED_TKINTPATH_H

