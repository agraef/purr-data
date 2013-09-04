/*
 * tkCanvGroup.c --
 *
 *	This file implements a line canvas item modelled after its
 *	SVG counterpart. See http://www.w3.org/TR/SVG11/.
 *
 * Copyright (c) 2008  Mats Bengtsson
 *
 * $Id$
 */

#include <float.h>
#include "tkIntPath.h"
#include "tkpCanvas.h"
#include "tkCanvPathUtil.h"
#include "tkPathStyle.h"

/* For debugging. */
extern Tcl_Interp *gInterp;

enum {
    /* When childs update themself so they set all
     * its ancestors dirty bbox flag so they know
     * when they need to recompute its bbox. */
    GROUP_FLAG_DIRTY_BBOX	    = (1L << 0)	    
};

/*
 * The structure below defines the record for each path item.
 */

typedef struct GroupItem  {
    Tk_PathItemEx headerEx; /* Generic stuff that's the same for all
                             * path types.  MUST BE FIRST IN STRUCTURE. */
    PathRect totalBbox;		/* Bounding box including stroke.
				 * Untransformed coordinates. */
    long flags;			/* Various flags, see enum. */
} GroupItem;


/*
 * Prototypes for procedures defined in this file:
 */

void		GroupUpdateBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr);
static int	ConfigureGroup(Tcl_Interp *interp, Tk_PathCanvas canvas, 
		    Tk_PathItem *itemPtr, int objc,
		    Tcl_Obj *CONST objv[], int flags);
static int	CreateGroup(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static void	DeleteGroup(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display);
static void	DisplayGroup(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display, Drawable drawable,
		    int x, int y, int width, int height);
static void	GroupBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int flags);
static int	GroupCoords(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static int	GroupToArea(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *rectPtr);
static double	GroupToPoint(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *coordPtr);
static int	GroupToPostscript(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
static void	ScaleGroup(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double originX, double originY,
		    double scaleX, double scaleY);
static void	TranslateGroup(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double deltaX, double deltaY);


PATH_STYLE_CUSTOM_OPTION_RECORDS
PATH_CUSTOM_OPTION_TAGS
PATH_OPTION_STRING_TABLES_FILL
PATH_OPTION_STRING_TABLES_STROKE
PATH_OPTION_STRING_TABLES_STATE

static Tk_OptionSpec optionSpecs[] = {
    PATH_OPTION_SPEC_CORE(Tk_PathItemEx),
    PATH_OPTION_SPEC_PARENT,
    PATH_OPTION_SPEC_STYLE_FILL(Tk_PathItemEx, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(Tk_PathItemEx),
    PATH_OPTION_SPEC_STYLE_STROKE(Tk_PathItemEx, "black"),
    PATH_OPTION_SPEC_END
};

static Tk_OptionTable optionTable = NULL;

/*
 * The structures below defines the 'prect' item type by means
 * of procedures that can be invoked by generic item code.
 */

Tk_PathItemType tkGroupType = {
    "group",				/* name */
    sizeof(GroupItem),			/* itemSize */
    CreateGroup,			/* createProc */
    optionSpecs,			/* optionSpecs */
    ConfigureGroup,			/* configureProc */
    GroupCoords,			/* coordProc */
    DeleteGroup,			/* deleteProc */
    DisplayGroup,			/* displayProc */
    0,					/* flags */
    GroupBbox,				/* bboxProc */
    GroupToPoint,			/* pointProc */
    GroupToArea,			/* areaProc */
    GroupToPostscript,			/* postscriptProc */
    ScaleGroup,				/* scaleProc */
    TranslateGroup,			/* translateProc */
    (Tk_PathItemIndexProc *) NULL,	/* indexProc */
    (Tk_PathItemCursorProc *) NULL,	/* icursorProc */
    (Tk_PathItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_PathItemInsertProc *) NULL,	/* insertProc */
    (Tk_PathItemDCharsProc *) NULL,	/* dTextProc */
    (Tk_PathItemType *) NULL,		/* nextPtr */
};


static int	
CreateGroup(Tcl_Interp *interp,
	Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[])
{
    GroupItem *groupPtr = (GroupItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &groupPtr->headerEx;

    /*
     * Carry out initialization that is needed to set defaults and to
     * allow proper cleanup after errors during the the remainder of
     * this procedure.
     */
    TkPathInitStyle(&itemExPtr->style);
    itemExPtr->canvas = canvas;
    itemExPtr->styleObj = NULL;
    itemExPtr->styleInst = NULL;
    groupPtr->totalBbox = NewEmptyPathRect();
    groupPtr->flags = 0L;
    itemExPtr->header.x1 = itemExPtr->header.x2 =
    itemExPtr->header.y1 = itemExPtr->header.y2 = -1;
    
    if (optionTable == NULL) {
	optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    } 
    itemPtr->optionTable = optionTable;
    if (Tk_InitOptions(interp, (char *) groupPtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas)) != TCL_OK) {
        goto error;
    }
    if (ConfigureGroup(interp, canvas, itemPtr, objc, objv, 0) == TCL_OK) {
        return TCL_OK;
    }

error:
    /*
     * NB: We must unlink the item here since the TkPathCanvasItemExConfigure()
     *     link it to the root by default.
     */
    TkPathCanvasItemDetach(itemPtr);
    DeleteGroup(canvas, itemPtr, Tk_Display(Tk_PathCanvasTkwin(canvas)));
    return TCL_ERROR;
}

static int	
ConfigureGroup(Tcl_Interp *interp, Tk_PathCanvas canvas, 
	Tk_PathItem *itemPtr, int objc,
        Tcl_Obj *CONST objv[], int flags)
{
    GroupItem *groupPtr = (GroupItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &groupPtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;
    Tk_Window tkwin;
    //Tk_PathState state;
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error, mask;

    tkwin = Tk_PathCanvasTkwin(canvas);
    for (error = 0; error <= 1; error++) {
	if (!error) {
	    if (Tk_SetOptions(interp, (char *) groupPtr, optionTable, 
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
	 * Root item's -tags and -parent is read only.
	 */
	if (itemPtr->id == 0) {
	    if (mask & PATH_CORE_OPTION_PARENT) {
		Tcl_SetObjResult(interp, 
			Tcl_NewStringObj("root items -parent is not configurable", -1));
		continue;
	    }
	    if (mask & PATH_CORE_OPTION_TAGS) {
		Tcl_SetObjResult(interp, 
			Tcl_NewStringObj("root items -tags is not configurable", -1));	
		continue;
	    }
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
    
    /*
     * We must notify all children to update themself
     * since they may inherit features.
     */
    if (!error) {
	GroupItemConfigured(canvas, itemPtr, mask);
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
	return TCL_OK;
    }
}

static void	
DeleteGroup(Tk_PathCanvas canvas,
    Tk_PathItem *itemPtr, Display *display)
{
    GroupItem *groupPtr = (GroupItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &groupPtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;

    if (stylePtr->fill != NULL) {
	TkPathFreePathColor(stylePtr->fill);
    }
    if (itemExPtr->styleInst != NULL) {
	TkPathFreeStyle(itemExPtr->styleInst);
    }
    Tk_FreeConfigOptions((char *) itemPtr, optionTable, Tk_PathCanvasTkwin(canvas));
}

static void	
DisplayGroup(Tk_PathCanvas canvas,
    Tk_PathItem *itemPtr, Display *display, Drawable drawable,
    int x, int y, int width, int height)
{
    /* Empty. */
}

static void	
GroupBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int flags)
{
    /* Empty. */
}

static int	
GroupCoords(Tcl_Interp *interp,
    Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
    int objc, Tcl_Obj *CONST objv[])
{
    Tcl_SetObjResult(interp, 
	    Tcl_NewStringObj("group items have no coords", -1));
    return TCL_ERROR;
}

static int	
GroupToArea(Tk_PathCanvas canvas,
	Tk_PathItem *itemPtr, double *rectPtr)
{
    /*
     * This says that the group is entirely outside any area.
     */
    return -1;
}

static double	
GroupToPoint(Tk_PathCanvas canvas,
	Tk_PathItem *itemPtr, double *coordPtr)
{
    /*
     * This says that the group is nowhere.
     */
    return DBL_MAX;
}

static int	
GroupToPostscript(Tcl_Interp *interp,
	Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass)
{
    return TCL_ERROR;
}

static void	
ScaleGroup(Tk_PathCanvas canvas,
	Tk_PathItem *itemPtr, double originX, double originY,
	double scaleX, double scaleY)
{
    CanvasScaleGroup(canvas, itemPtr, originX, originY, scaleX, scaleY);
    /* @@@ TODO: we could handle bbox ourselves? */
}

static void	
TranslateGroup(Tk_PathCanvas canvas,
	Tk_PathItem *itemPtr, double deltaX, double deltaY)
{
    CanvasTranslateGroup(canvas, itemPtr, deltaX, deltaY);
    /* @@@ TODO: we could handle bbox ourselves? */
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathCanvasSetGroupDirtyBbox --
 *
 *	This function is invoked by canvas code to tell us that one or
 *	more of our childrens have changed somehow so that our bbox
 *	need to be recomputed next time TkPathCanvasUpdateGroupBbox
 *	is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkPathCanvasSetGroupDirtyBbox(Tk_PathItem *itemPtr)
{
    GroupItem *groupPtr = (GroupItem *) itemPtr;
    groupPtr->flags &= GROUP_FLAG_DIRTY_BBOX;
}

void	
TkPathCanvasUpdateGroupBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr)
{
    GroupItem *groupPtr = (GroupItem *) itemPtr;

    if (groupPtr->flags & GROUP_FLAG_DIRTY_BBOX) {
	TkPathCanvasGroupBbox(canvas, itemPtr,
		&itemPtr->x1, &itemPtr->y1, &itemPtr->x2, &itemPtr->y2);    
	groupPtr->flags &= ~GROUP_FLAG_DIRTY_BBOX;
    }
}


