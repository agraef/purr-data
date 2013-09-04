/*
 * tkCanvPpolygon.c --
 *
 *	This file implements polygon and polyline canvas items modelled after its
 *  SVG counterpart. See http://www.w3.org/TR/SVG11/.
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

typedef struct PpolyItem  {
    Tk_PathItemEx headerEx; /* Generic stuff that's the same for all
                             * path types.  MUST BE FIRST IN STRUCTURE. */
    char type;		    /* Polyline or polygon. */
    PathAtom *atomPtr;
    PathRect bbox;	    /* Bounding box with zero width outline.
			     * Untransformed coordinates. */
    PathRect totalBbox;	    /* Bounding box including stroke.
			     * Untransformed coordinates. */
    int maxNumSegments;	    /* Max number of straight segments (for subpath)
			     * needed for Area and Point functions. */
} PpolyItem;

enum {
    kPpolyTypePolyline,
    kPpolyTypePolygon
};


/*
 * Prototypes for procedures defined in this file:
 */

static void	ComputePpolyBbox(Tk_PathCanvas canvas, PpolyItem *ppolyPtr);
static int	ConfigurePpoly(Tcl_Interp *interp, Tk_PathCanvas canvas, 
                        Tk_PathItem *itemPtr, int objc,
                        Tcl_Obj *CONST objv[], int flags);
int		CoordsForPolygonline(Tcl_Interp *interp, Tk_PathCanvas canvas, int closed,
                        int objc, Tcl_Obj *CONST objv[], PathAtom **atomPtrPtr, int *lenPtr);
static int	CreateAny(Tcl_Interp *interp,
                        Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
                        int objc, Tcl_Obj *CONST objv[], char type);
static int	CreatePolyline(Tcl_Interp *interp,
                        Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
                        int objc, Tcl_Obj *CONST objv[]);
static int	CreatePpolygon(Tcl_Interp *interp,
                        Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
                        int objc, Tcl_Obj *CONST objv[]);
static void	DeletePpoly(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display);
static void	DisplayPpoly(Tk_PathCanvas canvas,
                        Tk_PathItem *itemPtr, Display *display, Drawable drawable,
                        int x, int y, int width, int height);
static void	PpolyBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask);
static int	PpolyCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
                        int objc, Tcl_Obj *CONST objv[]);
static int	PpolyToArea(Tk_PathCanvas canvas,
                        Tk_PathItem *itemPtr, double *rectPtr);
static double	PpolyToPoint(Tk_PathCanvas canvas,
                        Tk_PathItem *itemPtr, double *coordPtr);
static int	PpolyToPostscript(Tcl_Interp *interp,
                        Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
static void	ScalePpoly(Tk_PathCanvas canvas,
                        Tk_PathItem *itemPtr, double originX, double originY,
                        double scaleX, double scaleY);
static void	TranslatePpoly(Tk_PathCanvas canvas,
                        Tk_PathItem *itemPtr, double deltaX, double deltaY);


PATH_STYLE_CUSTOM_OPTION_RECORDS
PATH_CUSTOM_OPTION_TAGS
PATH_OPTION_STRING_TABLES_FILL
PATH_OPTION_STRING_TABLES_STROKE
PATH_OPTION_STRING_TABLES_STATE

static Tk_OptionSpec optionSpecsPolyline[] = {
    PATH_OPTION_SPEC_CORE(Tk_PathItemEx),
    PATH_OPTION_SPEC_PARENT,
    PATH_OPTION_SPEC_STYLE_FILL(Tk_PathItemEx, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(Tk_PathItemEx),
    PATH_OPTION_SPEC_STYLE_STROKE(Tk_PathItemEx, "black"),
    PATH_OPTION_SPEC_END
};

static Tk_OptionSpec optionSpecsPpolygon[] = {
    PATH_OPTION_SPEC_CORE(Tk_PathItemEx),
    PATH_OPTION_SPEC_PARENT,
    PATH_OPTION_SPEC_STYLE_FILL(Tk_PathItemEx, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(Tk_PathItemEx),
    PATH_OPTION_SPEC_STYLE_STROKE(Tk_PathItemEx, "black"),
    PATH_OPTION_SPEC_END
};

static Tk_OptionTable optionTablePolyline = NULL;
static Tk_OptionTable optionTablePpolygon = NULL;

/*
 * The structures below defines the 'polyline' item type by means
 * of procedures that can be invoked by generic item code.
 */

Tk_PathItemType tkPolylineType = {
    "polyline",				/* name */
    sizeof(PpolyItem),			/* itemSize */
    CreatePolyline,			/* createProc */
    optionSpecsPolyline,		/* OptionSpecs */
    ConfigurePpoly,			/* configureProc */
    PpolyCoords,			/* coordProc */
    DeletePpoly,			/* deleteProc */
    DisplayPpoly,			/* displayProc */
    0,					/* flags */
    PpolyBbox,				/* bboxProc */
    PpolyToPoint,			/* pointProc */
    PpolyToArea,			/* areaProc */
    PpolyToPostscript,			/* postscriptProc */
    ScalePpoly,				/* scaleProc */
    TranslatePpoly,			/* translateProc */
    (Tk_PathItemIndexProc *) NULL,	/* indexProc */
    (Tk_PathItemCursorProc *) NULL,	/* icursorProc */
    (Tk_PathItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_PathItemInsertProc *) NULL,	/* insertProc */
    (Tk_PathItemDCharsProc *) NULL,	/* dTextProc */
    (Tk_PathItemType *) NULL,		/* nextPtr */
};

Tk_PathItemType tkPpolygonType = {
    "ppolygon",				/* name */
    sizeof(PpolyItem),			/* itemSize */
    CreatePpolygon,			/* createProc */
    optionSpecsPpolygon,		/* OptionSpecs */
    ConfigurePpoly,			/* configureProc */
    PpolyCoords,			/* coordProc */
    DeletePpoly,			/* deleteProc */
    DisplayPpoly,			/* displayProc */
    0,					/* flags */
    PpolyBbox,				/* bboxProc */
    PpolyToPoint,			/* pointProc */
    PpolyToArea,			/* areaProc */
    PpolyToPostscript,			/* postscriptProc */
    ScalePpoly,				/* scaleProc */
    TranslatePpoly,			/* translateProc */
    (Tk_PathItemIndexProc *) NULL,	/* indexProc */
    (Tk_PathItemCursorProc *) NULL,	/* icursorProc */
    (Tk_PathItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_PathItemInsertProc *) NULL,	/* insertProc */
    (Tk_PathItemDCharsProc *) NULL,	/* dTextProc */
    (Tk_PathItemType *) NULL,		/* nextPtr */
};
 

static int		
CreatePolyline(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[])
{
    return CreateAny(interp, canvas, itemPtr, objc, objv, kPpolyTypePolyline);
}

static int		
CreatePpolygon(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[])
{
    return CreateAny(interp, canvas, itemPtr, objc, objv, kPpolyTypePolygon);
}

static int		
CreateAny(Tcl_Interp *interp, Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[], char type)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ppolyPtr->headerEx;
    Tk_OptionTable optionTable;
    int	i, len;

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
    ppolyPtr->atomPtr = NULL;
    ppolyPtr->type = type;
    ppolyPtr->bbox = NewEmptyPathRect();
    ppolyPtr->totalBbox = NewEmptyPathRect();
    ppolyPtr->maxNumSegments = 0;
    
    if (ppolyPtr->type == kPpolyTypePolyline) {
	if (optionTablePolyline == NULL) {
	    optionTablePolyline = Tk_CreateOptionTable(interp, optionSpecsPolyline);
	}
	optionTable = optionTablePolyline;
    } else {
	if (optionTablePpolygon == NULL) {
	    optionTablePpolygon = Tk_CreateOptionTable(interp, optionSpecsPpolygon);
	}
	optionTable = optionTablePpolygon;    
    }
    itemPtr->optionTable = optionTable;
    if (Tk_InitOptions(interp, (char *) ppolyPtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas)) != TCL_OK) {
        goto error;
    }

    for (i = 1; i < objc; i++) {
        char *arg = Tcl_GetString(objv[i]);
        if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
            break;
        }
    }
    if (CoordsForPolygonline(interp, canvas, 
	    (ppolyPtr->type == kPpolyTypePolyline) ? 0 : 1, 
	    i, objv, &(ppolyPtr->atomPtr), &len) != TCL_OK) {
        goto error;
    }
    ppolyPtr->maxNumSegments = len;
   
    if (ConfigurePpoly(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
        return TCL_OK;
    }

    error:
    /*
     * NB: We must unlink the item here since the TkPathCanvasItemExConfigure()
     *     link it to the root by default.
     */
    TkPathCanvasItemDetach(itemPtr);
    DeletePpoly(canvas, itemPtr, Tk_Display(Tk_PathCanvasTkwin(canvas)));
    return TCL_ERROR;
}

static int		
PpolyCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[])
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    int len, closed;

    closed = (ppolyPtr->type == kPpolyTypePolyline) ? 0 : 1;
    if (CoordsForPolygonline(interp, canvas, closed, objc, objv, 
            &(ppolyPtr->atomPtr), &len) != TCL_OK) {
        return TCL_ERROR;
    }
    ppolyPtr->maxNumSegments = len;
    ComputePpolyBbox(canvas, ppolyPtr);
    return TCL_OK;
}	

void
ComputePpolyBbox(Tk_PathCanvas canvas, PpolyItem *ppolyPtr)
{
    Tk_PathItemEx *itemExPtr = &ppolyPtr->headerEx;
    Tk_PathItem *itemPtr = &itemExPtr->header;
    Tk_PathStyle style;
    Tk_PathState state = itemExPtr->header.state;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if ((ppolyPtr->atomPtr == NULL) || (state == TK_PATHSTATE_HIDDEN)) {
        itemExPtr->header.x1 = itemExPtr->header.x2 =
        itemExPtr->header.y1 = itemExPtr->header.y2 = -1;
        return;
    }
    style = TkPathCanvasInheritStyle(itemPtr, kPathMergeStyleNotFill);
    ppolyPtr->bbox = GetGenericBarePathBbox(ppolyPtr->atomPtr);
    ppolyPtr->totalBbox = GetGenericPathTotalBboxFromBare(ppolyPtr->atomPtr,
            &style, &ppolyPtr->bbox);
    SetGenericPathHeaderBbox(&itemExPtr->header, style.matrixPtr, &ppolyPtr->totalBbox);
    TkPathCanvasFreeInheritedStyle(&style);
}

static int		
ConfigurePpoly(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[], int flags)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ppolyPtr->headerEx;
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
	    optionTable = (ppolyPtr->type == kPpolyTypePolyline) ? optionTablePolyline : optionTablePpolygon;
	    if (Tk_SetOptions(interp, (char *) ppolyPtr, optionTable, 
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
	ComputePpolyBbox(canvas, ppolyPtr);
	return TCL_OK;
    }
}

static void		
DeletePpoly(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ppolyPtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;
    Tk_OptionTable optionTable;

    if (stylePtr->fill != NULL) {
	TkPathFreePathColor(stylePtr->fill);
    }
    if (itemExPtr->styleInst != NULL) {
	TkPathFreeStyle(itemExPtr->styleInst);
    }
    if (ppolyPtr->atomPtr != NULL) {
        TkPathFreeAtoms(ppolyPtr->atomPtr);
        ppolyPtr->atomPtr = NULL;
    }
    optionTable = (ppolyPtr->type == kPpolyTypePolyline) ? optionTablePolyline : optionTablePpolygon;
    Tk_FreeConfigOptions((char *) itemPtr, optionTable, Tk_PathCanvasTkwin(canvas));
}

static void		
DisplayPpoly(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display, Drawable drawable,
        int x, int y, int width, int height)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    TMatrix m = GetCanvasTMatrix(canvas);
    Tk_PathStyle style;
    
    /* === EB - 23-apr-2010: register coordinate offsets */
    TkPathSetCoordOffsets(m.tx, m.ty);
    /* === */
    
    style = TkPathCanvasInheritStyle(itemPtr, 0);
    TkPathDrawPath(Tk_PathCanvasTkwin(canvas), drawable, ppolyPtr->atomPtr, &style,
            &m, &ppolyPtr->bbox);
    TkPathCanvasFreeInheritedStyle(&style);
}

static void	
PpolyBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    ComputePpolyBbox(canvas, ppolyPtr);
}

static double	
PpolyToPoint(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *pointPtr)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    Tk_PathStyle style;
    double dist;
    long flags;

    flags = (ppolyPtr->type == kPpolyTypePolyline) ? kPathMergeStyleNotFill : 0;
    style = TkPathCanvasInheritStyle(itemPtr, flags);
    dist = GenericPathToPoint(canvas, itemPtr, &style, ppolyPtr->atomPtr, 
            ppolyPtr->maxNumSegments, pointPtr);
    TkPathCanvasFreeInheritedStyle(&style);
    return dist;
}

static int		
PpolyToArea(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *areaPtr)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;
    Tk_PathStyle style;
    int area;
    long flags;

    flags = (ppolyPtr->type == kPpolyTypePolyline) ? kPathMergeStyleNotFill : 0;
    style = TkPathCanvasInheritStyle(itemPtr, flags);    
    area = GenericPathToArea(canvas, itemPtr, &style, 
            ppolyPtr->atomPtr, ppolyPtr->maxNumSegments, areaPtr);
    TkPathCanvasFreeInheritedStyle(&style);            
    return area;
}

static int		
PpolyToPostscript(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass)
{
    return TCL_ERROR;
}

static void		
ScalePpoly(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double originX, double originY,
        double scaleX, double scaleY)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;

    ScalePathAtoms(ppolyPtr->atomPtr, originX, originY, scaleX, scaleY);
    ScalePathRect(&ppolyPtr->bbox, originX, originY, scaleX, scaleY);
    ScalePathRect(&ppolyPtr->totalBbox, originX, originY, scaleX, scaleY);
    ScaleItemHeader(itemPtr, originX, originY, scaleX, scaleY);
}

static void		
TranslatePpoly(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double deltaX, double deltaY)
{
    PpolyItem *ppolyPtr = (PpolyItem *) itemPtr;

    TranslatePathAtoms(ppolyPtr->atomPtr, deltaX, deltaY);
    TranslatePathRect(&ppolyPtr->bbox, deltaX, deltaY);
    TranslatePathRect(&ppolyPtr->totalBbox, deltaX, deltaY);
    TranslateItemHeader(itemPtr, deltaX, deltaY);
}

/*
 *--------------------------------------------------------------
 *
 * CoordsForPolygonline --
 *
 *		Used as coordProc for polyline and polygon items.
 *
 * Results:
 *		Standard tcl result.
 *
 * Side effects:
 *		May store new atoms in atomPtrPtr and max number of points
 *		in lenPtr.
 *
 *--------------------------------------------------------------
 */

int		
CoordsForPolygonline(
    Tcl_Interp *interp, 
    Tk_PathCanvas canvas, 
    int closed,				/* Polyline (0) or polygon (1) */
    int objc, 
    Tcl_Obj *CONST objv[],
    PathAtom **atomPtrPtr,
    int *lenPtr)
{
    PathAtom *atomPtr = *atomPtrPtr;

    if (objc == 0) {
        Tcl_Obj *obj = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        
        while (atomPtr != NULL) {
            switch (atomPtr->type) {
                case PATH_ATOM_M: { 
                    MoveToAtom *move = (MoveToAtom *) atomPtr;
                    Tcl_ListObjAppendElement(interp, obj, Tcl_NewDoubleObj(move->x));
                    Tcl_ListObjAppendElement(interp, obj, Tcl_NewDoubleObj(move->y));
                    break;
                }
                case PATH_ATOM_L: {
                    LineToAtom *line = (LineToAtom *) atomPtr;
                    Tcl_ListObjAppendElement(interp, obj, Tcl_NewDoubleObj(line->x));
                    Tcl_ListObjAppendElement(interp, obj, Tcl_NewDoubleObj(line->y));
                    break;
                }
                case PATH_ATOM_Z: {
                
                    break;
                }
                default: {
                    /* empty */
                }
            }
            atomPtr = atomPtr->nextPtr;
        }
        Tcl_SetObjResult(interp, obj);
        return TCL_OK;
    }
    if (objc == 1) {
        if (Tcl_ListObjGetElements(interp, objv[0], &objc,
            (Tcl_Obj ***) &objv) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (objc & 1) {
        char buf[64 + TCL_INTEGER_SPACE];
        sprintf(buf, "wrong # coordinates: expected an even number, got %d", objc);
        Tcl_SetResult(interp, buf, TCL_VOLATILE);
        return TCL_ERROR;
    } else if (objc < 4) {
        char buf[64 + TCL_INTEGER_SPACE];
        sprintf(buf, "wrong # coordinates: expected at least 4, got %d", objc);
        Tcl_SetResult(interp, buf, TCL_VOLATILE);
        return TCL_ERROR;
    } else {
        int 	i;
        double	x, y;
        double	firstX = 0.0, firstY = 0.0;
        PathAtom *firstAtomPtr = NULL;
    
        /*
        * Free any old stuff.
        */
        if (atomPtr != NULL) {
            TkPathFreeAtoms(atomPtr);
            atomPtr = NULL;
        }
        for (i = 0; i < objc; i += 2) {
            if (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[i], &x) != TCL_OK) {
                /* @@@ error recovery? */
                return TCL_ERROR;
            }
            if (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[i+1], &y) != TCL_OK) {
                return TCL_ERROR;
            }
            if (i == 0) {
                firstX = x;
                firstY = y;
                atomPtr = NewMoveToAtom(x, y);
                firstAtomPtr = atomPtr;
            } else {
                atomPtr->nextPtr = NewLineToAtom(x, y);
                atomPtr = atomPtr->nextPtr;
            }
        }
        if (closed) {
            atomPtr->nextPtr = NewCloseAtom(firstX, firstY);
        }
        *atomPtrPtr = firstAtomPtr;
        *lenPtr = i/2 + 2;
    }
    return TCL_OK;
}

/*----------------------------------------------------------------------*/

