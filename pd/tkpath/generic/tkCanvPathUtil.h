/*
 * tkCanvPathUtil.h --
 *
 *	Header for support functions common to many path canvas items.
 *
 * Copyright (c) 2007-2008  Mats Bengtsson
 *
 * $Id$
 */

#ifndef INCLUDED_TKCANVPATHUTIL_H
#define INCLUDED_TKCANVPATHUTIL_H

#include "tkIntPath.h"

#ifdef __cplusplus
extern "C" {
#endif

int	    CoordsForPointItems(Tcl_Interp *interp, Tk_PathCanvas canvas, 
                    double *pointPtr, int objc, Tcl_Obj *CONST objv[]);
int	    CoordsForRectangularItems(Tcl_Interp *interp, Tk_PathCanvas canvas, 
                    PathRect *rectPtr, int objc, Tcl_Obj *CONST objv[]);
PathRect    GetGenericBarePathBbox(PathAtom *atomPtr);
PathRect    GetGenericPathTotalBboxFromBare(PathAtom *atomPtr, Tk_PathStyle *stylePtr, PathRect *bboxPtr);
void	    SetGenericPathHeaderBbox(Tk_PathItem *headerPtr, TMatrix *mPtr,
                    PathRect *totalBboxPtr);
TMatrix	    GetCanvasTMatrix(Tk_PathCanvas canvas);
PathRect    NewEmptyPathRect(void);
int	    IsPathRectEmpty(PathRect *r);
void	    IncludePointInRect(PathRect *r, double x, double y);
double	    GenericPathToPoint(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Tk_PathStyle *stylePtr,
		    PathAtom *atomPtr, int maxNumSegments, double *pointPtr);
int	    GenericPathToArea(Tk_PathCanvas canvas,	Tk_PathItem *itemPtr, Tk_PathStyle *stylePtr,
                    PathAtom * atomPtr, int maxNumSegments, double *areaPtr);
void	    TranslatePathAtoms(PathAtom *atomPtr, double deltaX, double deltaY);
void	    ScalePathAtoms(PathAtom *atomPtr, double originX, double originY,
                    double scaleX, double scaleY);
void	    TranslatePathRect(PathRect *r, double deltaX, double deltaY);
void	    ScalePathRect(PathRect *r, double originX, double originY,
		    double scaleX, double scaleY);
void	    TranslateItemHeader(Tk_PathItem *itemPtr, double deltaX, double deltaY);
void	    ScaleItemHeader(Tk_PathItem *itemPtr, double originX, double originY,
		    double scaleX, double scaleY);

/*
 * The canvas 'Area' and 'Point' functions.
 */
int	PathPolyLineToArea(double *polyPtr, int numPoints, register double *rectPtr);
double	PathThickPolygonToPoint(int joinStyle, int capStyle, double width, 
			int isclosed, double *polyPtr, int numPoints, double *pointPtr);
double	PathPolygonToPointEx(double *polyPtr, int numPoints, double *pointPtr, 
			int *intersectionsPtr, int *nonzerorulePtr);
double	PathRectToPoint(double rectPtr[], double width, int filled, double pointPtr[]);
int	PathRectToArea(double rectPtr[], double width, int filled, double *areaPtr);
int	PathRectToAreaWithMatrix(PathRect bbox, TMatrix *mPtr, double *areaPtr);
double PathRectToPointWithMatrix(PathRect bbox, TMatrix *mPtr, double *pointPtr);


/*
 * New API option parsing.
 */

#define PATH_DEF_STATE "normal"

/* These MUST be kept in sync with Tk_PathState! */

#define PATH_OPTION_STRING_TABLES_STATE				    \
    static char *stateStrings[] = {				    \
	"active", "disabled", "normal", "hidden", NULL		    \
    };

#define PATH_CUSTOM_OPTION_TAGS					    \
    static Tk_ObjCustomOption tagsCO = {			    \
        "tags",							    \
        Tk_PathCanvasTagsOptionSetProc,				    \
        Tk_PathCanvasTagsOptionGetProc,				    \
        Tk_PathCanvasTagsOptionRestoreProc,			    \
        Tk_PathCanvasTagsOptionFreeProc,			    \
        (ClientData) NULL					    \
    };

#define PATH_OPTION_SPEC_PARENT					    \
    {TK_OPTION_STRING, "-parent", NULL, NULL,			    \
        "0", Tk_Offset(Tk_PathItem, parentObj), -1,		    \
	0, 0, PATH_CORE_OPTION_PARENT}

#define PATH_OPTION_SPEC_CORE(typeName)				    \
    {TK_OPTION_STRING_TABLE, "-state", NULL, NULL,		    \
        PATH_DEF_STATE, -1, Tk_Offset(Tk_PathItem, state),	    \
        0, (ClientData) stateStrings, 0},			    \
    {TK_OPTION_STRING, "-style", (char *) NULL, (char *) NULL,	    \
	"", Tk_Offset(typeName, styleObj), -1,			    \
	TK_OPTION_NULL_OK, 0, PATH_CORE_OPTION_STYLENAME},	    \
    {TK_OPTION_CUSTOM, "-tags", NULL, NULL,			    \
	NULL, -1, Tk_Offset(Tk_PathItem, pathTagsPtr),		    \
	TK_OPTION_NULL_OK, (ClientData) &tagsCO, PATH_CORE_OPTION_TAGS}


#ifdef __cplusplus
}
#endif

#endif      // INCLUDED_TKCANVPATHUTIL_H

