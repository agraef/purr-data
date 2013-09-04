/*
 * tkCanvEllipse.c --
 *
 *	This file implements the circle and ellipse canvas items modelled after its
 *	SVG counterpart. See http://www.w3.org/TR/SVG11/.
 *
 * Copyright (c) 2007-2008  Mats Bengtsson
 *
 * $Id$
 */

#include "tkIntPath.h"
#include "tkpCanvas.h"
#include "tkCanvPathUtil.h"
#include "tkPathStyle.h"

/* For debugging. */
extern Tcl_Interp *gInterp;

/*
 * The structure below defines the record for each circle and ellipse item.
 */

typedef struct EllipseItem  {
    Tk_PathItemEx headerEx; /* Generic stuff that's the same for all
                             * path types.  MUST BE FIRST IN STRUCTURE. */
    char type;		    /* Circle or ellipse. */
    double center[2];	    /* Center coord. */
    double rx;		    /* Radius. Circle uses rx for overall radius. */
    double ry;
} EllipseItem;

enum {
    kOvalTypeCircle,
    kOvalTypeEllipse
};

/*
 * Prototypes for procedures defined in this file:
 */

static void	ComputeEllipseBbox(Tk_PathCanvas canvas, EllipseItem *ellPtr);
static int	ConfigureEllipse(Tcl_Interp *interp, Tk_PathCanvas canvas, 
		    Tk_PathItem *itemPtr, int objc,
		    Tcl_Obj *CONST objv[], int flags);
static int	CreateAny(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[], char type);
static int	CreateCircle(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static int	CreateEllipse(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static void	DeleteEllipse(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display);
static void	DisplayEllipse(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display, Drawable drawable,
		    int x, int y, int width, int height);
static void	EllipseBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
		    int mask);
static int	EllipseCoords(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static int	EllipseToArea(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *rectPtr);
static double	EllipseToPoint(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *coordPtr);
static int	EllipseToPostscript(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
static void	ScaleEllipse(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double originX, double originY,
		    double scaleX, double scaleY);
static void	TranslateEllipse(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double deltaX, double deltaY);


enum {
    ELLIPSE_OPTION_INDEX_RX		    = (1L << (PATH_STYLE_OPTION_INDEX_END + 0)),
    ELLIPSE_OPTION_INDEX_RY		    = (1L << (PATH_STYLE_OPTION_INDEX_END + 1)),
    ELLIPSE_OPTION_INDEX_R		    = (1L << (PATH_STYLE_OPTION_INDEX_END + 2)),
};
 
PATH_STYLE_CUSTOM_OPTION_RECORDS
PATH_CUSTOM_OPTION_TAGS
PATH_OPTION_STRING_TABLES_FILL
PATH_OPTION_STRING_TABLES_STROKE
PATH_OPTION_STRING_TABLES_STATE

#define PATH_OPTION_SPEC_R(typeName)		    \
    {TK_OPTION_DOUBLE, "-rx", NULL, NULL,	    \
        "0.0", -1, Tk_Offset(typeName, rx),	    \
	0, 0, ELLIPSE_OPTION_INDEX_R}

#define PATH_OPTION_SPEC_RX(typeName)		    \
    {TK_OPTION_DOUBLE, "-rx", NULL, NULL,	    \
        "0.0", -1, Tk_Offset(typeName, rx),	    \
	0, 0, ELLIPSE_OPTION_INDEX_RX}

#define PATH_OPTION_SPEC_RY(typeName)		    \
    {TK_OPTION_DOUBLE, "-ry", NULL, NULL,	    \
        "0.0", -1, Tk_Offset(typeName, ry),	    \
	0, 0, ELLIPSE_OPTION_INDEX_RY}

static Tk_OptionSpec optionSpecsCircle[] = {
    PATH_OPTION_SPEC_CORE(Tk_PathItemEx),
    PATH_OPTION_SPEC_PARENT,
    PATH_OPTION_SPEC_STYLE_FILL(Tk_PathItemEx, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(Tk_PathItemEx),
    PATH_OPTION_SPEC_STYLE_STROKE(Tk_PathItemEx, "black"),
    PATH_OPTION_SPEC_R(EllipseItem),
    PATH_OPTION_SPEC_END
};

static Tk_OptionSpec optionSpecsEllipse[] = {
    PATH_OPTION_SPEC_CORE(Tk_PathItemEx),
    PATH_OPTION_SPEC_PARENT,
    PATH_OPTION_SPEC_STYLE_FILL(Tk_PathItemEx, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(Tk_PathItemEx),
    PATH_OPTION_SPEC_STYLE_STROKE(Tk_PathItemEx, "black"),
    PATH_OPTION_SPEC_RX(EllipseItem),
    PATH_OPTION_SPEC_RY(EllipseItem),
    PATH_OPTION_SPEC_END
};

static Tk_OptionTable optionTableCircle = NULL;
static Tk_OptionTable optionTableEllipse = NULL;

/*
 * The structures below define the 'circle' and 'ellipse' item types by means
 * of procedures that can be invoked by generic item code.
 */

Tk_PathItemType tkCircleType = {
    "circle",				/* name */
    sizeof(EllipseItem),		/* itemSize */
    CreateCircle,			/* createProc */
    optionSpecsCircle,			/* optionSpecs */
    ConfigureEllipse,			/* configureProc */
    EllipseCoords,			/* coordProc */
    DeleteEllipse,			/* deleteProc */
    DisplayEllipse,			/* displayProc */
    0,					/* flags */
    EllipseBbox,			/* bboxProc */
    EllipseToPoint,			/* pointProc */
    EllipseToArea,			/* areaProc */
    EllipseToPostscript,		/* postscriptProc */
    ScaleEllipse,			/* scaleProc */
    TranslateEllipse,			/* translateProc */
    (Tk_PathItemIndexProc *) NULL,	/* indexProc */
    (Tk_PathItemCursorProc *) NULL,	/* icursorProc */
    (Tk_PathItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_PathItemInsertProc *) NULL,	/* insertProc */
    (Tk_PathItemDCharsProc *) NULL,	/* dTextProc */
    (Tk_PathItemType *) NULL,		/* nextPtr */
};

Tk_PathItemType tkEllipseType = {
    "ellipse",				/* name */
    sizeof(EllipseItem),		/* itemSize */
    CreateEllipse,			/* createProc */
    optionSpecsEllipse,			/* optionSpecs */
    ConfigureEllipse,			/* configureProc */
    EllipseCoords,			/* coordProc */
    DeleteEllipse,			/* deleteProc */
    DisplayEllipse,			/* displayProc */
    0,					/* flags */
    EllipseBbox,			/* bboxProc */
    EllipseToPoint,			/* pointProc */
    EllipseToArea,			/* areaProc */
    EllipseToPostscript,		/* postscriptProc */
    ScaleEllipse,			/* scaleProc */
    TranslateEllipse,			/* translateProc */
    (Tk_PathItemIndexProc *) NULL,	/* indexProc */
    (Tk_PathItemCursorProc *) NULL,	/* icursorProc */
    (Tk_PathItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_PathItemInsertProc *) NULL,	/* insertProc */
    (Tk_PathItemDCharsProc *) NULL,	/* dTextProc */
    (Tk_PathItemType *) NULL,		/* nextPtr */
};
                        
static int		
CreateCircle(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[])
{
    return CreateAny(interp, canvas, itemPtr, objc, objv, kOvalTypeCircle);
}

static int		
CreateEllipse(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[])
{
    return CreateAny(interp, canvas, itemPtr, objc, objv, kOvalTypeEllipse);
}

static int		
CreateAny(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[], char type)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ellPtr->headerEx;
    Tk_OptionTable optionTable;
    int	i;

    if (objc == 0) {
        Tcl_Panic("canvas did not pass any coords\n");
    }
    gInterp = interp;

    /*
     * Carry out initialization that is needed to set defaults and to
     * allow proper cleanup after errors during the the remainder of
     * this procedure.
     */
    TkPathInitStyle(&itemExPtr->style);
    itemExPtr->canvas = canvas;
    itemExPtr->styleObj = NULL;
    itemExPtr->styleInst = NULL;
    ellPtr->type = type;

    if (ellPtr->type == kOvalTypeCircle) {
	if (optionTableCircle == NULL) {
	    optionTableCircle = Tk_CreateOptionTable(interp, optionSpecsCircle);
	}
	optionTable = optionTableCircle;
    } else {
	if (optionTableEllipse == NULL) {
	    optionTableEllipse = Tk_CreateOptionTable(interp, optionSpecsEllipse);
	}
	optionTable = optionTableEllipse;    
    }
    itemPtr->optionTable = optionTable;
    if (Tk_InitOptions(interp, (char *) ellPtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas)) != TCL_OK) {
        goto error;
    }
    
    for (i = 1; i < objc; i++) {
        char *arg = Tcl_GetString(objv[i]);
        if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
            break;
        }
    }
    if (CoordsForPointItems(interp, canvas, ellPtr->center, i, objv) != TCL_OK) {
        goto error;
    }    
    if (ConfigureEllipse(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
        return TCL_OK;
    }

    error:
    /*
     * NB: We must unlink the item here since the TkPathCanvasItemExConfigure()
     *     link it to the root by default.
     */
    TkPathCanvasItemDetach(itemPtr);
    DeleteEllipse(canvas, itemPtr, Tk_Display(Tk_PathCanvasTkwin(canvas)));
    return TCL_ERROR;
}

static int		
EllipseCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[])
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    int result;

    result = CoordsForPointItems(interp, canvas, ellPtr->center, objc, objv);
    if ((result == TCL_OK) && ((objc == 1) || (objc == 2))) {
        ComputeEllipseBbox(canvas, ellPtr);
    }
    return result;
}

static PathRect
GetBareBbox(EllipseItem *ellPtr)
{
    PathRect bbox;
    
    bbox.x1 = ellPtr->center[0] - ellPtr->rx;
    bbox.y1 = ellPtr->center[1] - ellPtr->ry;
    bbox.x2 = ellPtr->center[0] + ellPtr->rx;
    bbox.y2 = ellPtr->center[1] + ellPtr->ry;
    return bbox;
}

static void
ComputeEllipseBbox(Tk_PathCanvas canvas, EllipseItem *ellPtr)
{
    Tk_PathItemEx *itemExPtr = &ellPtr->headerEx;
    Tk_PathItem *itemPtr = &itemExPtr->header;
    Tk_PathStyle style;
    Tk_PathState state = itemExPtr->header.state;
    PathRect totalBbox, bbox;

    if(state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (state == TK_PATHSTATE_HIDDEN) {
        itemExPtr->header.x1 = itemExPtr->header.x2 =
        itemExPtr->header.y1 = itemExPtr->header.y2 = -1;
        return;
    }
    style = TkPathCanvasInheritStyle(itemPtr, kPathMergeStyleNotFill);
    bbox = GetBareBbox(ellPtr);
    totalBbox = GetGenericPathTotalBboxFromBare(NULL, &style, &bbox);
    SetGenericPathHeaderBbox(&itemExPtr->header, style.matrixPtr, &totalBbox);
    TkPathCanvasFreeInheritedStyle(&style);
}

static int		
ConfigureEllipse(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[], int flags)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ellPtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;
    Tk_Window tkwin;
    //Tk_PathState state;
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int mask, error;

    tkwin = Tk_PathCanvasTkwin(canvas);
    for (error = 0; error <= 1; error++) {
	if (!error) {
	    Tk_OptionTable optionTable;
	    optionTable = (ellPtr->type == kOvalTypeCircle) ? optionTableCircle : optionTableEllipse;
	    if (Tk_SetOptions(interp, (char *) ellPtr, optionTable, 
		    objc, objv, tkwin, &savedOptions, &mask) != TCL_OK) {
		continue;
	    }
	} else {
	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}	
	if (TkPathCanvasItemExConfigure(interp, canvas, itemExPtr, mask) != TCL_OK) {
	    continue;
	}

	/*
	 * If we reach this on the first pass we are OK and continue below.
	 */
	break;
    }
    if (!error) {
	Tk_FreeSavedOptions(&savedOptions);
	stylePtr->mask |= mask;
    }
    
    stylePtr->strokeOpacity = MAX(0.0, MIN(1.0, stylePtr->strokeOpacity));
    stylePtr->fillOpacity   = MAX(0.0, MIN(1.0, stylePtr->fillOpacity));
    ellPtr->rx = MAX(0.0, ellPtr->rx);
    ellPtr->ry = MAX(0.0, ellPtr->ry);
    if (ellPtr->type == kOvalTypeCircle) {
        /* Practical. */
        ellPtr->ry = ellPtr->rx;
    }    
#if 0	    // From old code. Needed?
    state = itemPtr->state;
    if(state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (state == TK_PATHSTATE_HIDDEN) {
        return TCL_OK;
    }
#endif
    if (error) {
	Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    } else {
	ComputeEllipseBbox(canvas, ellPtr);
	return TCL_OK;
    }
}

static void		
DeleteEllipse(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ellPtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;
    Tk_OptionTable optionTable;

    if (stylePtr->fill != NULL) {
	TkPathFreePathColor(stylePtr->fill);
    }
    if (itemExPtr->styleInst != NULL) {
	TkPathFreeStyle(itemExPtr->styleInst);
    }
    optionTable = (ellPtr->type == kOvalTypeCircle) ? optionTableCircle : optionTableEllipse;
    Tk_FreeConfigOptions((char *) itemPtr, optionTable, Tk_PathCanvasTkwin(canvas));
}

static void		
DisplayEllipse(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display, Drawable drawable,
        int x, int y, int width, int height)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    TMatrix m = GetCanvasTMatrix(canvas);
    PathRect bbox;
    PathAtom *atomPtr;
    EllipseAtom ellAtom;
    Tk_PathStyle style;    
    
    /* === EB - 23-apr-2010: register coordinate offsets */
    TkPathSetCoordOffsets(m.tx, m.ty);
    /* === */
    
    /* 
     * We create the atom on the fly to save some memory.
     */    
    atomPtr = (PathAtom *)&ellAtom;
    atomPtr->nextPtr = NULL;
    atomPtr->type = PATH_ATOM_ELLIPSE;
    ellAtom.cx = ellPtr->center[0];
    ellAtom.cy = ellPtr->center[1];
    ellAtom.rx = ellPtr->rx;
    ellAtom.ry = ellPtr->ry;
    
    bbox = GetBareBbox(ellPtr);
    style = TkPathCanvasInheritStyle(itemPtr, 0);
    TkPathDrawPath(Tk_PathCanvasTkwin(canvas), drawable, atomPtr, &style, &m, &bbox);
    TkPathCanvasFreeInheritedStyle(&style);
}

static void	
EllipseBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    ComputeEllipseBbox(canvas, ellPtr);
}

static double	
EllipseToPoint(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *pointPtr)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    Tk_PathStyle style;
    TMatrix *mPtr;
    double bareOval[4];
    double width, dist;
    int rectiLinear = 0;
    int haveDist = 0;
    int filled;
    
    style = TkPathCanvasInheritStyle(itemPtr, 0);
    filled = HaveAnyFillFromPathColor(style.fill);
    width = 0.0;
    if (style.strokeColor != NULL) {
        width = style.strokeWidth;
    }
    mPtr = style.matrixPtr;
    if (mPtr == NULL) {
        rectiLinear = 1;
        bareOval[0] = ellPtr->center[0] - ellPtr->rx;
        bareOval[1] = ellPtr->center[1] - ellPtr->ry;
        bareOval[2] = ellPtr->center[0] + ellPtr->rx;
        bareOval[3] = ellPtr->center[1] + ellPtr->ry;
        
        /* For tiny points make it simple. */
        if ((ellPtr->rx <= 2.0) && (ellPtr->ry <= 2.0)) {
            dist = hypot(ellPtr->center[0] - pointPtr[0], ellPtr->center[1] - pointPtr[1]);
            dist = MAX(0.0, dist - (ellPtr->rx + ellPtr->ry)/2.0);
            haveDist = 1;
        }
    } else if (TMATRIX_IS_RECTILINEAR(mPtr)) {
        double rx, ry;
    
        /* This is a situation we can treat in a simplified way. Apply the transform here. */
        rectiLinear = 1;
        bareOval[0] = mPtr->a * (ellPtr->center[0] - ellPtr->rx) + mPtr->tx;
        bareOval[1] = mPtr->d * (ellPtr->center[1] - ellPtr->ry) + mPtr->ty;
        bareOval[2] = mPtr->a * (ellPtr->center[0] + ellPtr->rx) + mPtr->tx;
        bareOval[3] = mPtr->d * (ellPtr->center[1] + ellPtr->ry) + mPtr->ty;

        /* For tiny points make it simple. */
        rx = fabs(bareOval[0] - bareOval[2])/2.0;
        ry = fabs(bareOval[1] - bareOval[3])/2.0;
        if ((rx <= 2.0) && (ry <= 2.0)) {
            dist = hypot((bareOval[0] + bareOval[2]/2.0) - pointPtr[0], 
                    (bareOval[1] + bareOval[3]/2.0) - pointPtr[1]);
            dist = MAX(0.0, dist - (rx + ry)/2.0);
            haveDist = 1;
        }
    }
    if (!haveDist) {
        if (rectiLinear) {
            dist = TkOvalToPoint(bareOval, width, filled, pointPtr);
        } else {
            PathAtom *atomPtr;
            EllipseAtom ellAtom;
        
            /* 
            * We create the atom on the fly to save some memory.
            */    
            atomPtr = (PathAtom *)&ellAtom;
            atomPtr->nextPtr = NULL;
            atomPtr->type = PATH_ATOM_ELLIPSE;
            ellAtom.cx = ellPtr->center[0];
            ellAtom.cy = ellPtr->center[1];
            ellAtom.rx = ellPtr->rx;
            ellAtom.ry = ellPtr->ry;
            dist = GenericPathToPoint(canvas, itemPtr, &style, atomPtr, 
                    kPathNumSegmentsEllipse+1, pointPtr);
        }
    }
    TkPathCanvasFreeInheritedStyle(&style);
    return dist;
}

static int		
EllipseToArea(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *areaPtr)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;
    Tk_PathStyle style;
    TMatrix *mPtr;
    double bareOval[4], halfWidth;
    int rectiLinear = 0;
    int result;
    
    style = TkPathCanvasInheritStyle(itemPtr, 0);
    halfWidth = 0.0;
    if (style.strokeColor != NULL) {
        halfWidth = style.strokeWidth/2.0;
    }
    mPtr = style.matrixPtr;
    if (mPtr == NULL) {
        rectiLinear = 1;
        bareOval[0] = ellPtr->center[0] - ellPtr->rx;
        bareOval[1] = ellPtr->center[1] - ellPtr->ry;
        bareOval[2] = ellPtr->center[0] + ellPtr->rx;
        bareOval[3] = ellPtr->center[1] + ellPtr->ry;
    } else if (TMATRIX_IS_RECTILINEAR(mPtr)) {
    
        /* This is a situation we can treat in a simplified way. Apply the transform here. */
        rectiLinear = 1;
        bareOval[0] = mPtr->a * (ellPtr->center[0] - ellPtr->rx) + mPtr->tx;
        bareOval[1] = mPtr->d * (ellPtr->center[1] - ellPtr->ry) + mPtr->ty;
        bareOval[2] = mPtr->a * (ellPtr->center[0] + ellPtr->rx) + mPtr->tx;
        bareOval[3] = mPtr->d * (ellPtr->center[1] + ellPtr->ry) + mPtr->ty;
    }
    
    if (rectiLinear) {
        double oval[4];
        
        /* @@@ Assuming untransformed strokes */
        oval[0] = bareOval[0] - halfWidth;
        oval[1] = bareOval[1] - halfWidth;
        oval[2] = bareOval[2] + halfWidth;
        oval[3] = bareOval[3] + halfWidth;

        result = TkOvalToArea(oval, areaPtr);
    
        /*
         * If the rectangle appears to overlap the oval and the oval
         * isn't filled, do one more check to see if perhaps all four
         * of the rectangle's corners are totally inside the oval's
         * unfilled center, in which case we should return "outside".
         */
        if ((result == 0) && (style.strokeColor != NULL)
                && !HaveAnyFillFromPathColor(style.fill)) {
            double width, height;
            double xDelta1, yDelta1, xDelta2, yDelta2;
        
            width = (bareOval[2] - bareOval[0])/2.0 - halfWidth;
            height = (bareOval[3] - bareOval[1])/2.0 - halfWidth;
            if ((width <= 0.0) || (height <= 0.0)) {
                return 0;
            }
            xDelta1 = (areaPtr[0] - ellPtr->center[0])/width;
            xDelta1 *= xDelta1;
            yDelta1 = (areaPtr[1] - ellPtr->center[1])/height;
            yDelta1 *= yDelta1;
            xDelta2 = (areaPtr[2] - ellPtr->center[0])/width;
            xDelta2 *= xDelta2;
            yDelta2 = (areaPtr[3] - ellPtr->center[1])/height;
            yDelta2 *= yDelta2;
            if (((xDelta1 + yDelta1) < 1.0)
                    && ((xDelta1 + yDelta2) < 1.0)
                    && ((xDelta2 + yDelta1) < 1.0)
                    && ((xDelta2 + yDelta2) < 1.0)) {
                result = -1;
            }
        }
    } else {
        PathAtom *atomPtr;
        EllipseAtom ellAtom;
    
        /* 
         * We create the atom on the fly to save some memory.
         */    
        atomPtr = (PathAtom *)&ellAtom;
        atomPtr->nextPtr = NULL;
        atomPtr->type = PATH_ATOM_ELLIPSE;
        ellAtom.cx = ellPtr->center[0];
        ellAtom.cy = ellPtr->center[1];
        ellAtom.rx = ellPtr->rx;
        ellAtom.ry = ellPtr->ry;
        result = GenericPathToArea(canvas, itemPtr, &style, atomPtr, 
                kPathNumSegmentsEllipse+1, areaPtr);
    }
    TkPathCanvasFreeInheritedStyle(&style);
    return result;
}

static int		
EllipseToPostscript(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass)
{
    return TCL_ERROR;
}

static void		
ScaleEllipse(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double originX, double originY,
        double scaleX, double scaleY)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;

    ellPtr->center[0] = originX + scaleX*(ellPtr->center[0] - originX);
    ellPtr->center[1] = originY + scaleY*(ellPtr->center[1] - originY);
    ellPtr->rx *= scaleX;
    ellPtr->ry *= scaleY;
    ScaleItemHeader(itemPtr, originX, originY, scaleX, scaleY);
}

static void		
TranslateEllipse(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double deltaX, double deltaY)
{
    EllipseItem *ellPtr = (EllipseItem *) itemPtr;

    ellPtr->center[0] += deltaX;
    ellPtr->center[1] += deltaY;
    TranslateItemHeader(itemPtr, deltaX, deltaY);
}

/*----------------------------------------------------------------------*/

