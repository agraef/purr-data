/*
 * tkCanvPline.c --
 *
 *	This file implements a line canvas item modelled after its
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
 * The structure below defines the record for each path item.
 */

typedef struct PlineItem  {
    Tk_PathItemEx headerEx; /* Generic stuff that's the same for all
                             * path types.  MUST BE FIRST IN STRUCTURE. */
    PathRect coords;		/* Coordinates (unorders bare bbox). */
    PathRect totalBbox;		/* Bounding box including stroke.
				 * Untransformed coordinates. */
} PlineItem;


/*
 * Prototypes for procedures defined in this file:
 */

static void	ComputePlineBbox(Tk_PathCanvas canvas, PlineItem *plinePtr);
static int	ConfigurePline(Tcl_Interp *interp, Tk_PathCanvas canvas, 
		    Tk_PathItem *itemPtr, int objc,
                    Tcl_Obj *CONST objv[], int flags);
static int	CreatePline(Tcl_Interp *interp,
                    Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static void	DeletePline(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display);
static void	DisplayPline(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display, Drawable drawable,
		    int x, int y, int width, int height);
static void	PlineBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask);
static int	ProcessCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, 
		    Tk_PathItem *itemPtr, int objc, Tcl_Obj *CONST objv[]);
static int	PlineCoords(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static int	PlineToArea(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *rectPtr);
static double	PlineToPoint(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *coordPtr);
static int	PlineToPostscript(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
static void	ScalePline(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double originX, double originY,
		    double scaleX, double scaleY);
static void	TranslatePline(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double deltaX, double deltaY);
static PathAtom * MakePathAtoms(PlineItem *plinePtr);

PATH_STYLE_CUSTOM_OPTION_MATRIX
PATH_STYLE_CUSTOM_OPTION_DASH
PATH_CUSTOM_OPTION_TAGS
PATH_OPTION_STRING_TABLES_STROKE
PATH_OPTION_STRING_TABLES_STATE

static Tk_OptionSpec optionSpecs[] = {
    PATH_OPTION_SPEC_CORE(Tk_PathItemEx),
    PATH_OPTION_SPEC_PARENT,
    PATH_OPTION_SPEC_STYLE_MATRIX(Tk_PathItemEx),
    PATH_OPTION_SPEC_STYLE_STROKE(Tk_PathItemEx, "black"),
    PATH_OPTION_SPEC_END
};

static Tk_OptionTable optionTable = NULL;

/*
 * The structures below defines the 'prect' item type by means
 * of procedures that can be invoked by generic item code.
 */

Tk_PathItemType tkPlineType = {
    "pline",				/* name */
    sizeof(PlineItem),			/* itemSize */
    CreatePline,			/* createProc */
    optionSpecs,			/* optionSpecs */
    ConfigurePline,			/* configureProc */
    PlineCoords,			/* coordProc */
    DeletePline,			/* deleteProc */
    DisplayPline,			/* displayProc */
    0,					/* flags */
    PlineBbox,				/* bboxProc */
    PlineToPoint,			/* pointProc */
    PlineToArea,			/* areaProc */
    PlineToPostscript,			/* postscriptProc */
    ScalePline,				/* scaleProc */
    TranslatePline,			/* translateProc */
    (Tk_PathItemIndexProc *) NULL,	/* indexProc */
    (Tk_PathItemCursorProc *) NULL,	/* icursorProc */
    (Tk_PathItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_PathItemInsertProc *) NULL,	/* insertProc */
    (Tk_PathItemDCharsProc *) NULL,	/* dTextProc */
    (Tk_PathItemType *) NULL,		/* nextPtr */
};

static int		
CreatePline(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[])
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &plinePtr->headerEx;
    int	i;

    if (objc == 0) {
        Tcl_Panic("canvas did not pass any coords\n");
    }

    /*
     * Carry out initialization that is needed to set defaults and to
     * allow proper cleanup after errors during the the remainder of
     * this procedure.
     */
    TkPathInitStyle(&itemExPtr->style);
    itemExPtr->canvas = canvas;
    itemExPtr->styleObj = NULL;
    itemExPtr->styleInst = NULL;
    plinePtr->totalBbox = NewEmptyPathRect();
    
    if (optionTable == NULL) {
	optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    } 
    itemPtr->optionTable = optionTable;
    if (Tk_InitOptions(interp, (char *) plinePtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas)) != TCL_OK) {
        goto error;
    }

    for (i = 1; i < objc; i++) {
        char *arg = Tcl_GetString(objv[i]);
        if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
            break;
        }
    }
    if (ProcessCoords(interp, canvas, itemPtr, i, objv) != TCL_OK) {
        goto error;
    }
    if (ConfigurePline(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
        return TCL_OK;
    }

    error:
    /*
     * NB: We must unlink the item here since the TkPathCanvasItemExConfigure()
     *     link it to the root by default.
     */
    TkPathCanvasItemDetach(itemPtr);
    DeletePline(canvas, itemPtr, Tk_Display(Tk_PathCanvasTkwin(canvas)));
    return TCL_ERROR;
}

static int
ProcessCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[])
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    PathRect *p = &plinePtr->coords;

    if (objc == 0) {
        Tcl_Obj *obj = Tcl_NewObj();
        Tcl_Obj *subobj = Tcl_NewDoubleObj(p->x1);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(p->y1);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(p->x2);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(p->y2);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        Tcl_SetObjResult(interp, obj);
    } else if ((objc == 1) || (objc == 4)) {
        if (objc==1) {
            if (Tcl_ListObjGetElements(interp, objv[0], &objc,
                    (Tcl_Obj ***) &objv) != TCL_OK) {
                return TCL_ERROR;
            } else if (objc != 4) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 0 or 4", -1));
                return TCL_ERROR;
            }
        }
        if ((Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[0], &p->x1) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[1], &p->y1) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[2], &p->x2) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[3], &p->y2) != TCL_OK)) {
            return TCL_ERROR;
        }
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 0 or 4", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int		
PlineCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[])
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    int result;
    
    result = ProcessCoords(interp, canvas, itemPtr, objc, objv);
    if ((result == TCL_OK) && (objc == 1)) {
	ComputePlineBbox(canvas, plinePtr);
    }
    return result;
}

static void
ComputePlineBbox(Tk_PathCanvas canvas, PlineItem *plinePtr)
{
    Tk_PathItemEx *itemExPtr = &plinePtr->headerEx;
    Tk_PathItem *itemPtr = &itemExPtr->header;
    Tk_PathStyle style;
    Tk_PathState state = itemExPtr->header.state;
    PathRect r;

    if(state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (state == TK_PATHSTATE_HIDDEN) {
        itemExPtr->header.x1 = itemExPtr->header.x2 =
        itemExPtr->header.y1 = itemExPtr->header.y2 = -1;
        return;
    }
    style = TkPathCanvasInheritStyle(itemPtr, kPathMergeStyleNotFill);
    r.x1 = MIN(plinePtr->coords.x1, plinePtr->coords.x2);
    r.x2 = MAX(plinePtr->coords.x1, plinePtr->coords.x2);
    r.y1 = MIN(plinePtr->coords.y1, plinePtr->coords.y2);
    r.y2 = MAX(plinePtr->coords.y1, plinePtr->coords.y2);
    plinePtr->totalBbox = GetGenericPathTotalBboxFromBare(NULL, &style, &r);
    SetGenericPathHeaderBbox(&itemExPtr->header, style.matrixPtr, &plinePtr->totalBbox);
    TkPathCanvasFreeInheritedStyle(&style);
}

static int		
ConfigurePline(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[], int flags)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &plinePtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;
    Tk_Window tkwin;
    //Tk_PathState state;
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error, mask;

    tkwin = Tk_PathCanvasTkwin(canvas);
    for (error = 0; error <= 1; error++) {
	if (!error) {
	    if (Tk_SetOptions(interp, (char *) plinePtr, optionTable, 
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
	ComputePlineBbox(canvas, plinePtr);
	return TCL_OK;
    }
}

static PathAtom *
MakePathAtoms(PlineItem *plinePtr)
{
    PathAtom *atomPtr;
                
    atomPtr = NewMoveToAtom(plinePtr->coords.x1, plinePtr->coords.y1);
    atomPtr->nextPtr = NewLineToAtom(plinePtr->coords.x2, plinePtr->coords.y2);
    return atomPtr;
}

static void		
DeletePline(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &plinePtr->headerEx;

    if (itemExPtr->styleInst != NULL) {
	TkPathFreeStyle(itemExPtr->styleInst);
    }
    Tk_FreeConfigOptions((char *) itemPtr, optionTable, Tk_PathCanvasTkwin(canvas));
}

static void		
DisplayPline(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display, Drawable drawable,
        int x, int y, int width, int height)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    TMatrix m = GetCanvasTMatrix(canvas);
    PathRect r;
    PathAtom *atomPtr;
    Tk_PathStyle style;
    
    /* === EB - 23-apr-2010: register coordinate offsets */
    TkPathSetCoordOffsets(m.tx, m.ty);
    /* === */
    
    r.x1 = MIN(plinePtr->coords.x1, plinePtr->coords.x2);
    r.x2 = MAX(plinePtr->coords.x1, plinePtr->coords.x2);
    r.y1 = MIN(plinePtr->coords.y1, plinePtr->coords.y2);
    r.y2 = MAX(plinePtr->coords.y1, plinePtr->coords.y2);

    atomPtr = MakePathAtoms(plinePtr);
    style = TkPathCanvasInheritStyle(itemPtr, kPathMergeStyleNotFill);
    TkPathDrawPath(Tk_PathCanvasTkwin(canvas), drawable, atomPtr, &style, &m, &r);
    TkPathFreeAtoms(atomPtr);
    TkPathCanvasFreeInheritedStyle(&style);
}

static void	
PlineBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    ComputePlineBbox(canvas, plinePtr);
}

static double	
PlineToPoint(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *pointPtr)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    Tk_PathStyle style;
    PathAtom *atomPtr;
    double point;
    
    style = TkPathCanvasInheritStyle(itemPtr, kPathMergeStyleNotFill);

    /* @@@ Perhaps we should do a simplified treatment here instead of the generic. */
    atomPtr = MakePathAtoms(plinePtr);
    point = GenericPathToPoint(canvas, itemPtr, &style, 
            atomPtr, 2, pointPtr);
    TkPathFreeAtoms(atomPtr);
    TkPathCanvasFreeInheritedStyle(&style);
    return point;
}

static int		
PlineToArea(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *areaPtr)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;
    Tk_PathStyle style;
    PathAtom *atomPtr;
    int area;
    
    style = TkPathCanvasInheritStyle(itemPtr, kPathMergeStyleNotFill);

    /* @@@ Perhaps we should do a simplified treatment here instead of the generic. */
    atomPtr = MakePathAtoms(plinePtr);
    area = GenericPathToArea(canvas, itemPtr, &style, 
            atomPtr, 2, areaPtr);
    TkPathFreeAtoms(atomPtr);
    TkPathCanvasFreeInheritedStyle(&style);
    return area;
}

static int		
PlineToPostscript(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass)
{
    return TCL_ERROR;	/* @@@ Anyone? */
}

static void		
ScalePline(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double originX, double originY,
        double scaleX, double scaleY)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;

    ScalePathRect(&plinePtr->totalBbox, originX, originY, scaleX, scaleY);
    ScalePathRect(&plinePtr->coords, originX, originY, scaleX, scaleY);
    ScaleItemHeader(itemPtr, originX, originY, scaleX, scaleY);
}

static void		
TranslatePline(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double deltaX, double deltaY)
{
    PlineItem *plinePtr = (PlineItem *) itemPtr;

    /* Just translate the bbox as well. */
    TranslatePathRect(&plinePtr->totalBbox, deltaX, deltaY);
    TranslatePathRect(&plinePtr->coords, deltaX, deltaY);
    TranslateItemHeader(itemPtr, deltaX, deltaY);
}

/*----------------------------------------------------------------------*/

