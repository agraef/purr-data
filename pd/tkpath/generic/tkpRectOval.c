/*
 * tkpRectOval.c --
 *
 *	This file implements rectangle and oval items for canvas widgets.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#include <stdio.h>
#include "tkInt.h"
#include "tkIntPath.h"
#include "tkpCanvas.h"

/*
 * The structure below defines the record for each rectangle/oval item.
 */

typedef struct RectOvalItem  {
    Tk_PathItem header;		/* Generic stuff that's the same for all
				 * types. MUST BE FIRST IN STRUCTURE. */
    Tk_PathOutline outline;	/* Outline structure */
    double bbox[4];		/* Coordinates of bounding box for rectangle
				 * or oval (x1, y1, x2, y2). Item includes x1
				 * and x2 but not y1 and y2. */
    Tk_TSOffset *tsoffsetPtr;
    XColor *fillColor;		/* Color for filling rectangle/oval. */
    XColor *activeFillColor;	/* Color for filling rectangle/oval if state
				 * is active. */
    XColor *disabledFillColor;	/* Color for filling rectangle/oval if state
				 * is disabled. */
    Pixmap fillStipple;		/* Stipple bitmap for filling item. */
    Pixmap activeFillStipple;	/* Stipple bitmap for filling item if state is
				 * active. */
    Pixmap disabledFillStipple;	/* Stipple bitmap for filling item if state is
				 * disabled. */
    GC fillGC;			/* Graphics context for filling item. */
} RectOvalItem;

/*
 * Prototypes for functions defined in this file:
 */

static void		ComputeRectOvalBbox(Tk_PathCanvas canvas,
			    RectOvalItem *rectOvalPtr);
static int		ConfigureRectOval(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, int objc, Tcl_Obj *CONST objv[],
			    int flags);
static int		CreateRectOval(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, int objc, Tcl_Obj *CONST objv[]);
static void		DeleteRectOval(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    Display *display);
static void		DisplayRectOval(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    Display *display, Drawable dst, int x, int y,
			    int width, int height);
static int		OvalToArea(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    double *areaPtr);
static double		OvalToPoint(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    double *pointPtr);
static int		RectOvalCoords(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, int objc, Tcl_Obj *CONST objv[]);
static int		RectOvalToPostscript(Tcl_Interp *interp,
			    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
static int		RectToArea(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    double *areaPtr);
static double		RectToPoint(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    double *pointPtr);
static void		ScaleRectOval(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    double originX, double originY,
			    double scaleX, double scaleY);
static void		TranslateRectOval(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
			    double deltaX, double deltaY);

/*
 * Information used for parsing option specs:
 */

#define PATH_DEF_STATE "normal"

static char *stateStrings[] = {
    "active", "disabled", "normal", "hidden", NULL
};

static Tk_ObjCustomOption dashCO = {
    "dash",			
    Tk_DashOptionSetProc,
    Tk_DashOptionGetProc,
    Tk_DashOptionRestoreProc,
    Tk_DashOptionFreeProc,	
    (ClientData) NULL			
};

static Tk_ObjCustomOption offsetCO = {
    "offset",			
    TkPathOffsetOptionSetProc,
    TkPathOffsetOptionGetProc,
    TkPathOffsetOptionRestoreProc,
    TkPathOffsetOptionFreeProc,	
    (ClientData) (TK_OFFSET_RELATIVE|TK_OFFSET_INDEX)			
};

static Tk_ObjCustomOption pixelCO = {
    "pixel",			
    Tk_PathPixelOptionSetProc,
    Tk_PathPixelOptionGetProc,
    Tk_PathPixelOptionRestoreProc,
    NULL,	
    (ClientData) NULL			
};

static Tk_ObjCustomOption tagsCO = {
    "tags",			
    Tk_PathCanvasTagsOptionSetProc,
    Tk_PathCanvasTagsOptionGetProc,
    Tk_PathCanvasTagsOptionRestoreProc,
    Tk_PathCanvasTagsOptionFreeProc,	
    (ClientData) NULL			
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_CUSTOM, "-activedash", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.activeDashPtr),
	TK_OPTION_NULL_OK, &dashCO, 0},
    {TK_OPTION_COLOR, "-activefill", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, activeFillColor), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-activeoutline", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.activeColor), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BITMAP, "-activeoutlinestipple", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.activeStipple),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BITMAP, "-activestipple", NULL, NULL, 
        NULL, -1, Tk_Offset(RectOvalItem, activeFillStipple), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CUSTOM, "-activewidth", NULL, NULL,
	"0.0", -1, Tk_Offset(RectOvalItem, outline.activeWidth),
	0, &pixelCO, 0},
    {TK_OPTION_CUSTOM, "-dash", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.dashPtr),
	TK_OPTION_NULL_OK, &dashCO, 0},
    {TK_OPTION_PIXELS, "-dashoffset", NULL, NULL,
	"0", -1, Tk_Offset(RectOvalItem, outline.offset),
	0, 0, 0},
    {TK_OPTION_CUSTOM, "-disableddash", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.disabledDashPtr),
	TK_OPTION_NULL_OK, &dashCO, 0},
    {TK_OPTION_COLOR, "-disabledfill", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, disabledFillColor), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_COLOR, "-disabledoutline", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.disabledColor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BITMAP, "-disabledoutlinestipple", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.disabledStipple),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BITMAP, "-disabledstipple", NULL, NULL, 
        NULL, -1, Tk_Offset(RectOvalItem, disabledFillStipple), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CUSTOM, "-disabledwidth", NULL, NULL,
	"0.0", -1, Tk_Offset(RectOvalItem, outline.disabledWidth),
	0, &pixelCO, 0},
    {TK_OPTION_COLOR, "-fill", NULL, NULL,
	"", -1, Tk_Offset(RectOvalItem, fillColor), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CUSTOM, "-offset", NULL, NULL,
	"0,0", -1, Tk_Offset(RectOvalItem, tsoffsetPtr),
	0, &offsetCO, 0},
    {TK_OPTION_COLOR, "-outline", NULL, NULL,
	"black", -1, Tk_Offset(RectOvalItem, outline.color), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CUSTOM, "-outlineoffset", NULL, NULL,
	"0,0", -1, Tk_Offset(RectOvalItem, outline.tsoffsetPtr),
	0, &offsetCO, 0},
    {TK_OPTION_BITMAP, "-outlinestipple", NULL, NULL,
	NULL, -1, Tk_Offset(RectOvalItem, outline.stipple), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", NULL, NULL,
        PATH_DEF_STATE, -1, Tk_Offset(Tk_PathItem, state),
        0, (ClientData) stateStrings, 0},		
    {TK_OPTION_BITMAP, "-stipple", NULL, NULL, 
        NULL, -1, Tk_Offset(RectOvalItem, fillStipple), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_CUSTOM, "-tags", NULL, NULL,
	NULL, -1, Tk_Offset(Tk_PathItem, pathTagsPtr),
	TK_OPTION_NULL_OK, (ClientData) &tagsCO, 0},
    {TK_OPTION_CUSTOM, "-width", NULL, NULL, 
        "1.0", -1, Tk_Offset(RectOvalItem, outline.width), 0, &pixelCO, 0},
    {TK_OPTION_END, NULL, NULL, NULL,           
	NULL, 0, -1, 0, (ClientData) NULL, 0}
};

/* @@@ Not sure we need two option tables here */
static Tk_OptionTable optionTableRect = NULL;
static Tk_OptionTable optionTableOval = NULL;

/*
 * The structures below defines the rectangle and oval item types by means of
 * functions that can be invoked by generic item code.
 */

Tk_PathItemType tkRectangleType = {
    "rectangle",		/* name */
    sizeof(RectOvalItem),	/* itemSize */
    CreateRectOval,		/* createProc */
    optionSpecs,		/* optionSpecs */
    ConfigureRectOval,		/* configureProc */
    RectOvalCoords,		/* coordProc */
    DeleteRectOval,		/* deleteProc */
    DisplayRectOval,		/* displayProc */
    0,				/* flags */
    NULL,			/* bboxProc */
    RectToPoint,		/* pointProc */
    RectToArea,			/* areaProc */
    RectOvalToPostscript,	/* postscriptProc */
    ScaleRectOval,		/* scaleProc */
    TranslateRectOval,		/* translateProc */
    NULL,			/* indexProc */
    NULL,			/* icursorProc */
    NULL,			/* selectionProc */
    NULL,			/* insertProc */
    NULL,			/* dTextProc */
    NULL,			/* nextPtr */
};

Tk_PathItemType tkOvalType = {
    "oval",			/* name */
    sizeof(RectOvalItem),	/* itemSize */
    CreateRectOval,		/* createProc */
    optionSpecs,		/* configSpecs */
    ConfigureRectOval,		/* configureProc */
    RectOvalCoords,		/* coordProc */
    DeleteRectOval,		/* deleteProc */
    DisplayRectOval,		/* displayProc */
    0,				/* flags */
    NULL,			/* bboxProc */
    OvalToPoint,		/* pointProc */
    OvalToArea,			/* areaProc */
    RectOvalToPostscript,	/* postscriptProc */
    ScaleRectOval,		/* scaleProc */
    TranslateRectOval,		/* translateProc */
    NULL,			/* indexProc */
    NULL,			/* cursorProc */
    NULL,			/* selectionProc */
    NULL,			/* insertProc */
    NULL,			/* dTextProc */
    NULL,			/* nextPtr */
};

/*
 *--------------------------------------------------------------
 *
 * CreateRectOval --
 *
 *	This function is invoked to create a new rectangle or oval item in a
 *	canvas.
 *
 * Results:
 *	A standard Tcl return value. If an error occurred in creating the
 *	item, then an error message is left in the interp's result; in this
 *	case itemPtr is left uninitialized, so it can be safely freed by the
 *	caller.
 *
 * Side effects:
 *	A new rectangle or oval item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateRectOval(
    Tcl_Interp *interp,		/* For error reporting. */
    Tk_PathCanvas canvas,	/* Canvas to hold new item. */
    Tk_PathItem *itemPtr,	/* Record to hold new item; header has been
				 * initialized by caller. */
    int objc,			/* Number of arguments in objv. */
    Tcl_Obj *CONST objv[])	/* Arguments describing rectangle. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    Tk_OptionTable optionTable;
    int i;

    if (objc == 0) {
	Tcl_Panic("canvas did not pass any coords\n");
    }

    /*
     * Carry out initialization that is needed in order to clean up after
     * errors during the the remainder of this function.
     */

    Tk_PathCreateOutline(&(rectOvalPtr->outline));
    rectOvalPtr->tsoffsetPtr = NULL;
    rectOvalPtr->fillColor = NULL;
    rectOvalPtr->activeFillColor = NULL;
    rectOvalPtr->disabledFillColor = NULL;
    rectOvalPtr->fillStipple = None;
    rectOvalPtr->activeFillStipple = None;
    rectOvalPtr->disabledFillStipple = None;
    rectOvalPtr->fillGC = None;

    /* @@@ Not sure we need two option tables here */
    if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	if (optionTableRect == NULL) {
	    optionTableRect = Tk_CreateOptionTable(interp, optionSpecs);
	} 
	optionTable = optionTableRect;
    } else {
	if (optionTableOval == NULL) {
	    optionTableOval = Tk_CreateOptionTable(interp, optionSpecs);
	} 
	optionTable = optionTableOval;
    }
    itemPtr->optionTable = optionTable;
    if (Tk_InitOptions(interp, (char *) rectOvalPtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas)) != TCL_OK) {
        goto error;
    }

    /*
     * Process the arguments to fill in the item record.
     */

    for (i = 1; i < objc; i++) {
	char *arg = Tcl_GetString(objv[i]);

	if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    break;
	}
    }
    if ((RectOvalCoords(interp, canvas, itemPtr, i, objv) != TCL_OK)) {
	goto error;
    }
    if (ConfigureRectOval(interp, canvas, itemPtr, objc-i, objv+i, 0)
	    == TCL_OK) {
	return TCL_OK;
    }

  error:
    DeleteRectOval(canvas, itemPtr, Tk_Display(Tk_PathCanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalCoords --
 *
 *	This function is invoked to process the "coords" widget command on
 *	rectangles and ovals. See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	Returns TCL_OK or TCL_ERROR, and sets the interp's result.
 *
 * Side effects:
 *	The coordinates for the given item may be changed.
 *
 *--------------------------------------------------------------
 */

static int
RectOvalCoords(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_PathCanvas canvas,	/* Canvas containing item. */
    Tk_PathItem *itemPtr,	/* Item whose coordinates are to be read or
				 * modified. */
    int objc,			/* Number of coordinates supplied in objv. */
    Tcl_Obj *CONST objv[])	/* Array of coordinates: x1,y1,x2,y2,... */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    /*
     * If no coordinates, return the current coordinates (i.e. bounding box).
     */

    if (objc == 0) {
	Tcl_Obj *obj = Tcl_NewObj();

	Tcl_ListObjAppendElement(NULL, obj,
		Tcl_NewDoubleObj(rectOvalPtr->bbox[0]));
	Tcl_ListObjAppendElement(NULL, obj,
		Tcl_NewDoubleObj(rectOvalPtr->bbox[1]));
	Tcl_ListObjAppendElement(NULL, obj,
		Tcl_NewDoubleObj(rectOvalPtr->bbox[2]));
	Tcl_ListObjAppendElement(NULL, obj,
		Tcl_NewDoubleObj(rectOvalPtr->bbox[3]));
	Tcl_SetObjResult(interp, obj);
	return TCL_OK;
    }

    /*
     * If one "coordinate", treat as list of coordinates.
     */

    if (objc == 1) {
	if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		(Tcl_Obj ***) &objv) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    /*
     * Better have four coordinates now. Spit out an error message otherwise.
     */

    if (objc != 4) {
	char buf[64 + TCL_INTEGER_SPACE];

	sprintf(buf, "wrong # coordinates: expected 0 or 4, got %d", objc);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_ERROR;
    }

    /*
     * Parse the coordinates and update our bounding box.
     */

    if ((Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[0],
		&rectOvalPtr->bbox[0]) != TCL_OK)
	    || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[1],
		&rectOvalPtr->bbox[1]) != TCL_OK)
	    || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[2],
		&rectOvalPtr->bbox[2]) != TCL_OK)
	    || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[3],
		&rectOvalPtr->bbox[3]) != TCL_OK)) {
	return TCL_ERROR;
    }
    ComputeRectOvalBbox(canvas, rectOvalPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureRectOval --
 *
 *	This function is invoked to configure various aspects of a rectangle
 *	or oval item, such as its border and background colors.
 *
 * Results:
 *	A standard Tcl result code. If an error occurs, then an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	Configuration information, such as colors and stipple patterns, may be
 *	set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigureRectOval(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_PathCanvas canvas,	/* Canvas containing itemPtr. */
    Tk_PathItem *itemPtr,	/* Rectangle item to reconfigure. */
    int objc,			/* Number of elements in objv. */
    Tcl_Obj *CONST objv[],	/* Arguments describing things to configure. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    Tk_Window tkwin;
    Tk_TSOffset *tsoffset;
    XColor *color;
    Pixmap stipple;
    Tk_PathState state;
    Tk_OptionTable optionTable;
    
    if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	optionTable = optionTableRect;
    } else {
	optionTable = optionTableOval;
    }

    tkwin = Tk_PathCanvasTkwin(canvas);
    if (TCL_OK != Tk_SetOptions(interp, (char *) rectOvalPtr, optionTable, 
	    objc, objv, tkwin, NULL, NULL)) {
	return TCL_ERROR;
    }
    state = itemPtr->state;

    /*
     * A few of the options require additional processing, such as graphics
     * contexts.
     */

    if (rectOvalPtr->outline.activeWidth > rectOvalPtr->outline.width ||
	    (rectOvalPtr->outline.activeDashPtr != NULL &&
		    rectOvalPtr->outline.activeDashPtr->number != 0) ||
	    rectOvalPtr->outline.activeColor != NULL ||
	    rectOvalPtr->outline.activeStipple != None ||
	    rectOvalPtr->activeFillColor != NULL ||
	    rectOvalPtr->activeFillStipple != None) {
	itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
    } else {
	itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
    }

    tsoffset = rectOvalPtr->outline.tsoffsetPtr;
    if (tsoffset != NULL) {
	flags = tsoffset->flags;
	if (flags & TK_OFFSET_LEFT) {
	    tsoffset->xoffset = (int) (rectOvalPtr->bbox[0] + 0.5);
	} else if (flags & TK_OFFSET_CENTER) {
	    tsoffset->xoffset = (int)
	    ((rectOvalPtr->bbox[0]+rectOvalPtr->bbox[2]+1)/2);
	} else if (flags & TK_OFFSET_RIGHT) {
	    tsoffset->xoffset = (int) (rectOvalPtr->bbox[2] + 0.5);
	}
	if (flags & TK_OFFSET_TOP) {
	    tsoffset->yoffset = (int) (rectOvalPtr->bbox[1] + 0.5);
	} else if (flags & TK_OFFSET_MIDDLE) {
	    tsoffset->yoffset = (int)
	    ((rectOvalPtr->bbox[1]+rectOvalPtr->bbox[3]+1)/2);
	} else if (flags & TK_OFFSET_BOTTOM) {
	    tsoffset->yoffset = (int) (rectOvalPtr->bbox[2] + 0.5);
	}
    }
    
    /*
     * Configure the outline graphics context. If mask is non-zero, the gc has
     * changed and must be reallocated, provided that the new settings specify
     * a valid outline (non-zero width and non-NULL color)
     */

    mask = Tk_PathConfigOutlineGC(&gcValues, canvas, itemPtr,
	    &(rectOvalPtr->outline));
    if (mask && \
	    rectOvalPtr->outline.width != 0 && \
	    rectOvalPtr->outline.color != NULL) {
	gcValues.cap_style = CapProjecting;
	mask |= GCCapStyle;
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    } else {
	newGC = None;
    }
    if (rectOvalPtr->outline.gc != None) {
	Tk_FreeGC(Tk_Display(tkwin), rectOvalPtr->outline.gc);
    }
    rectOvalPtr->outline.gc = newGC;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (state == TK_PATHSTATE_HIDDEN) {
	ComputeRectOvalBbox(canvas, rectOvalPtr);
	return TCL_OK;
    }

    color = rectOvalPtr->fillColor;
    stipple = rectOvalPtr->fillStipple;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectOvalPtr->activeFillColor!=NULL) {
	    color = rectOvalPtr->activeFillColor;
	}
	if (rectOvalPtr->activeFillStipple!=None) {
	    stipple = rectOvalPtr->activeFillStipple;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (rectOvalPtr->disabledFillColor!=NULL) {
	    color = rectOvalPtr->disabledFillColor;
	}
	if (rectOvalPtr->disabledFillStipple!=None) {
	    stipple = rectOvalPtr->disabledFillStipple;
	}
    }

    if (color == NULL) {
	newGC = None;
    } else {
	gcValues.foreground = color->pixel;
	if (stipple != None) {
	    gcValues.stipple = stipple;
	    gcValues.fill_style = FillStippled;
	    mask = GCForeground|GCStipple|GCFillStyle;
	} else {
	    mask = GCForeground;
	}
#ifdef MAC_OSX_TK
	/*
	 * Mac OS X CG drawing needs access to the outline linewidth
	 * even for fills (as linewidth controls antialiasing).
	 */
	gcValues.line_width = rectOvalPtr->outline.gc != None ?
		rectOvalPtr->outline.gc->line_width : 0;
	mask |= GCLineWidth;
#endif
	newGC = Tk_GetGC(tkwin, mask, &gcValues);
    }
    if (rectOvalPtr->fillGC != None) {
	Tk_FreeGC(Tk_Display(tkwin), rectOvalPtr->fillGC);
    }
    rectOvalPtr->fillGC = newGC;

    tsoffset = rectOvalPtr->tsoffsetPtr;
    if (tsoffset != NULL) {
	flags = tsoffset->flags;
	if (flags & TK_OFFSET_LEFT) {
	    tsoffset->xoffset = (int) (rectOvalPtr->bbox[0] + 0.5);
	} else if (flags & TK_OFFSET_CENTER) {
	    tsoffset->xoffset = (int)
	    ((rectOvalPtr->bbox[0]+rectOvalPtr->bbox[2]+1)/2);
	} else if (flags & TK_OFFSET_RIGHT) {
	    tsoffset->xoffset = (int) (rectOvalPtr->bbox[2] + 0.5);
	}
	if (flags & TK_OFFSET_TOP) {
	    tsoffset->yoffset = (int) (rectOvalPtr->bbox[1] + 0.5);
	} else if (flags & TK_OFFSET_MIDDLE) {
	    tsoffset->yoffset = (int)
	    ((rectOvalPtr->bbox[1]+rectOvalPtr->bbox[3]+1)/2);
	} else if (flags & TK_OFFSET_BOTTOM) {
	    tsoffset->yoffset = (int) (rectOvalPtr->bbox[3] + 0.5);
	}
    }
    
    ComputeRectOvalBbox(canvas, rectOvalPtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteRectOval --
 *
 *	This function is called to clean up the data structure associated with
 *	a rectangle or oval item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with itemPtr are released.
 *
 *--------------------------------------------------------------
 */

static void
DeleteRectOval(
    Tk_PathCanvas canvas,	/* Info about overall widget. */
    Tk_PathItem *itemPtr,	/* Item that is being deleted. */
    Display *display)		/* Display containing window for canvas. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    Tk_OptionTable optionTable;
    
    if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	optionTable = optionTableRect;
    } else {
	optionTable = optionTableOval;
    }
    if (rectOvalPtr->fillGC != None) {
	Tk_FreeGC(display, rectOvalPtr->fillGC);
    }
    Tk_FreeConfigOptions((char *) rectOvalPtr, optionTable, Tk_PathCanvasTkwin(canvas));
}

/*
 *--------------------------------------------------------------
 *
 * ComputeRectOvalBbox --
 *
 *	This function is invoked to compute the bounding box of all the pixels
 *	that may be drawn as part of a rectangle or oval.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields x1, y1, x2, and y2 are updated in the header for itemPtr.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
ComputeRectOvalBbox(
    Tk_PathCanvas canvas,		/* Canvas that contains item. */
    RectOvalItem *rectOvalPtr)	/* Item whose bbox is to be recomputed. */
{
    int bloat, tmp;
    double dtmp, width;
    Tk_PathState state = rectOvalPtr->header.state;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    width = rectOvalPtr->outline.width;
    if (state == TK_PATHSTATE_HIDDEN) {
	rectOvalPtr->header.x1 = rectOvalPtr->header.y1 =
	rectOvalPtr->header.x2 = rectOvalPtr->header.y2 = -1;
	return;
    }
    if (((TkPathCanvas *)canvas)->currentItemPtr == (Tk_PathItem *)rectOvalPtr) {
	if (rectOvalPtr->outline.activeWidth>width) {
	    width = rectOvalPtr->outline.activeWidth;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (rectOvalPtr->outline.disabledWidth>0) {
	    width = rectOvalPtr->outline.disabledWidth;
	}
    }

    /*
     * Make sure that the first coordinates are the lowest ones.
     */

    if (rectOvalPtr->bbox[1] > rectOvalPtr->bbox[3]) {
	double tmpY = rectOvalPtr->bbox[3];

	rectOvalPtr->bbox[3] = rectOvalPtr->bbox[1];
	rectOvalPtr->bbox[1] = tmpY;
    }
    if (rectOvalPtr->bbox[0] > rectOvalPtr->bbox[2]) {
	double tmpX = rectOvalPtr->bbox[2];

	rectOvalPtr->bbox[2] = rectOvalPtr->bbox[0];
	rectOvalPtr->bbox[0] = tmpX;
    }

    if (rectOvalPtr->outline.gc == None) {
	/*
	 * The Win32 switch was added for 8.3 to solve a problem with ovals
	 * leaving traces on bottom and right of 1 pixel. This may not be the
	 * correct place to solve it, but it works.
	 */

#ifdef __WIN32__
	bloat = 1;
#else
	bloat = 0;
#endif
    } else {
#ifdef MAC_OSX_TK
	/*
	 * Mac OS X CoreGraphics needs correct rounding here otherwise it will
	 * draw outside the bounding box. Probably correct on other platforms
	 * as well?
	 */

	bloat = (int) (width+1.5)/2;
#else
	bloat = (int) (width+1)/2;
#endif
    }

    /*
     * Special note: the rectangle is always drawn at least 1x1 in size, so
     * round up the upper coordinates to be at least 1 unit greater than the
     * lower ones.
     */

    tmp = (int) ((rectOvalPtr->bbox[0] >= 0) ? rectOvalPtr->bbox[0] + .5
	    : rectOvalPtr->bbox[0] - .5);
    rectOvalPtr->header.x1 = tmp - bloat;
    tmp = (int) ((rectOvalPtr->bbox[1] >= 0) ? rectOvalPtr->bbox[1] + .5
	    : rectOvalPtr->bbox[1] - .5);
    rectOvalPtr->header.y1 = tmp - bloat;
    dtmp = rectOvalPtr->bbox[2];
    if (dtmp < (rectOvalPtr->bbox[0] + 1)) {
	dtmp = rectOvalPtr->bbox[0] + 1;
    }
    tmp = (int) ((dtmp >= 0) ? dtmp + .5 : dtmp - .5);
    rectOvalPtr->header.x2 = tmp + bloat;
    dtmp = rectOvalPtr->bbox[3];
    if (dtmp < (rectOvalPtr->bbox[1] + 1)) {
	dtmp = rectOvalPtr->bbox[1] + 1;
    }
    tmp = (int) ((dtmp >= 0) ? dtmp + .5 : dtmp - .5);
    rectOvalPtr->header.y2 = tmp + bloat;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayRectOval --
 *
 *	This function is invoked to draw a rectangle or oval item in a given
 *	drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ItemPtr is drawn in drawable using the transformation information in
 *	canvas.
 *
 *--------------------------------------------------------------
 */

static void
DisplayRectOval(
    Tk_PathCanvas canvas,	/* Canvas that contains item. */
    Tk_PathItem *itemPtr,	/* Item to be displayed. */
    Display *display,		/* Display on which to draw item. */
    Drawable drawable,		/* Pixmap or window in which to draw item. */
    int x, int y, int width, int height)
				/* Describes region of canvas that must be
				 * redisplayed (not used). */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    short x1, y1, x2, y2;
    Pixmap fillStipple;
    Tk_PathState state = itemPtr->state;

    /*
     * Compute the screen coordinates of the bounding box for the item. Make
     * sure that the bbox is at least one pixel large, since some X servers
     * will die if it isn't.
     */

    Tk_PathCanvasDrawableCoords(canvas, rectOvalPtr->bbox[0], rectOvalPtr->bbox[1],
	    &x1, &y1);
    Tk_PathCanvasDrawableCoords(canvas, rectOvalPtr->bbox[2], rectOvalPtr->bbox[3],
	    &x2, &y2);
    if (x2 <= x1) {
	x2 = x1+1;
    }
    if (y2 <= y1) {
	y2 = y1+1;
    }

    /*
     * Display filled part first (if wanted), then outline. If we're
     * stippling, then modify the stipple offset in the GC. Be sure to reset
     * the offset when done, since the GC is supposed to be read-only.
     */

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    fillStipple = rectOvalPtr->fillStipple;
    if (((TkPathCanvas *)canvas)->currentItemPtr == (Tk_PathItem *)rectOvalPtr) {
	if (rectOvalPtr->activeFillStipple != None) {
	    fillStipple = rectOvalPtr->activeFillStipple;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (rectOvalPtr->disabledFillStipple != None) {
	    fillStipple = rectOvalPtr->disabledFillStipple;
	}
    }

    if (rectOvalPtr->fillGC != None) {
	if (fillStipple != None) {
	    int w = 0, h = 0;
	    Tk_TSOffset tsoffset, *tsoffsetPtr;
	    
	    tsoffset.flags = 0;
	    tsoffset.xoffset = 0;
	    tsoffset.yoffset = 0;
	    tsoffsetPtr = rectOvalPtr->tsoffsetPtr;
	    if (tsoffsetPtr != NULL) {
		int flags = tsoffsetPtr->flags;

		if (flags & (TK_OFFSET_CENTER|TK_OFFSET_MIDDLE)) {
		    Tk_SizeOfBitmap(display, fillStipple, &w, &h);
		    if (flags & TK_OFFSET_CENTER) {
			w /= 2;
		    } else {
			w = 0;
		    }
		    if (flags & TK_OFFSET_MIDDLE) {
			h /= 2;
		    } else {
			h = 0;
		    }
		}
		tsoffset = *tsoffsetPtr;
		tsoffset.xoffset -= w;
		tsoffset.yoffset -= h;
	    }
	    Tk_PathCanvasSetOffset(canvas, rectOvalPtr->fillGC, &tsoffset);
	}
	if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	    XFillRectangle(display, drawable, rectOvalPtr->fillGC,
			   x1, y1, (unsigned int) (x2-x1), (unsigned int) (y2-y1));
	} else {
	    XFillArc(display, drawable, rectOvalPtr->fillGC,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1),
		    0, 360*64);
	}
	if (fillStipple != None) {
	    XSetTSOrigin(display, rectOvalPtr->fillGC, 0, 0);
	}
    }

    if (rectOvalPtr->outline.gc != None) {
	Tk_PathChangeOutlineGC(canvas, itemPtr, &(rectOvalPtr->outline));
	if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	    XDrawRectangle(display, drawable, rectOvalPtr->outline.gc,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1));
	} else {
	    XDrawArc(display, drawable, rectOvalPtr->outline.gc,
		    x1, y1, (unsigned) (x2-x1), (unsigned) (y2-y1), 0, 360*64);
	}
	Tk_PathResetOutlineGC(canvas, itemPtr, &(rectOvalPtr->outline));
    }
}

/*
 *--------------------------------------------------------------
 *
 * RectToPoint --
 *
 *	Computes the distance from a given point to a given rectangle, in
 *	canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	coordPtr[0] and coordPtr[1] is inside the rectangle. If the point
 *	isn't inside the rectangle then the return value is the distance from
 *	the point to the rectangle. If itemPtr is filled, then anywhere in the
 *	interior is considered "inside"; if itemPtr isn't filled, then
 *	"inside" means only the area occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
RectToPoint(
    Tk_PathCanvas canvas,	/* Canvas containing item. */
    Tk_PathItem *itemPtr,	/* Item to check against point. */
    double *pointPtr)		/* Pointer to x and y coordinates. */
{
    RectOvalItem *rectPtr = (RectOvalItem *) itemPtr;
    double xDiff, yDiff, x1, y1, x2, y2, inc, tmp;
    double width;
    Tk_PathState state = itemPtr->state;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    width = rectPtr->outline.width;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectPtr->outline.activeWidth>width) {
	    width = rectPtr->outline.activeWidth;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (rectPtr->outline.disabledWidth>0) {
	    width = rectPtr->outline.disabledWidth;
	}
    }

    /*
     * Generate a new larger rectangle that includes the border width, if
     * there is one.
     */

    x1 = rectPtr->bbox[0];
    y1 = rectPtr->bbox[1];
    x2 = rectPtr->bbox[2];
    y2 = rectPtr->bbox[3];
    if (rectPtr->outline.gc != None) {
	inc = width/2.0;
	x1 -= inc;
	y1 -= inc;
	x2 += inc;
	y2 += inc;
    }

    /*
     * If the point is inside the rectangle, handle specially: distance is 0
     * if rectangle is filled, otherwise compute distance to nearest edge of
     * rectangle and subtract width of edge.
     */

    if ((pointPtr[0] >= x1) && (pointPtr[0] < x2)
	    && (pointPtr[1] >= y1) && (pointPtr[1] < y2)) {
	if ((rectPtr->fillGC != None) || (rectPtr->outline.gc == None)) {
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
 * OvalToPoint --
 *
 *	Computes the distance from a given point to a given oval, in canvas
 *	units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	coordPtr[0] and coordPtr[1] is inside the oval. If the point isn't
 *	inside the oval then the return value is the distance from the point
 *	to the oval. If itemPtr is filled, then anywhere in the interior is
 *	considered "inside"; if itemPtr isn't filled, then "inside" means only
 *	the area occupied by the outline.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static double
OvalToPoint(
    Tk_PathCanvas canvas,	/* Canvas containing item. */
    Tk_PathItem *itemPtr,	/* Item to check against point. */
    double *pointPtr)		/* Pointer to x and y coordinates. */
{
    RectOvalItem *ovalPtr = (RectOvalItem *) itemPtr;
    double width;
    int filled;
    Tk_PathState state = itemPtr->state;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    width = (double) ovalPtr->outline.width;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (ovalPtr->outline.activeWidth>width) {
	    width = (double) ovalPtr->outline.activeWidth;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (ovalPtr->outline.disabledWidth>0) {
	    width = (double) ovalPtr->outline.disabledWidth;
	}
    }


    filled = ovalPtr->fillGC != None;
    if (ovalPtr->outline.gc == None) {
	width = 0.0;
	filled = 1;
    }
    return TkOvalToPoint(ovalPtr->bbox, width, filled, pointPtr);
}

/*
 *--------------------------------------------------------------
 *
 * RectToArea --
 *
 *	This function is called to determine whether an item lies entirely
 *	inside, entirely outside, or overlapping a given rectangle.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area given by
 *	rectPtr, 0 if it overlaps, and 1 if it is entirely inside the given
 *	area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
RectToArea(
    Tk_PathCanvas canvas,	/* Canvas containing item. */
    Tk_PathItem *itemPtr,	/* Item to check against rectangle. */
    double *areaPtr)		/* Pointer to array of four coordinates (x1,
				 * y1, x2, y2) describing rectangular area. */
{
    RectOvalItem *rectPtr = (RectOvalItem *) itemPtr;
    double halfWidth;
    double width;
    Tk_PathState state = itemPtr->state;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    width = rectPtr->outline.width;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectPtr->outline.activeWidth>width) {
	    width = rectPtr->outline.activeWidth;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (rectPtr->outline.disabledWidth>0) {
	    width = rectPtr->outline.disabledWidth;
	}
    }

    halfWidth = width/2.0;
    if (rectPtr->outline.gc == None) {
	halfWidth = 0.0;
    }

    if ((areaPtr[2] <= (rectPtr->bbox[0] - halfWidth))
	    || (areaPtr[0] >= (rectPtr->bbox[2] + halfWidth))
	    || (areaPtr[3] <= (rectPtr->bbox[1] - halfWidth))
	    || (areaPtr[1] >= (rectPtr->bbox[3] + halfWidth))) {
	return -1;
    }
    if ((rectPtr->fillGC == None) && (rectPtr->outline.gc != None)
	    && (areaPtr[0] >= (rectPtr->bbox[0] + halfWidth))
	    && (areaPtr[1] >= (rectPtr->bbox[1] + halfWidth))
	    && (areaPtr[2] <= (rectPtr->bbox[2] - halfWidth))
	    && (areaPtr[3] <= (rectPtr->bbox[3] - halfWidth))) {
	return -1;
    }
    if ((areaPtr[0] <= (rectPtr->bbox[0] - halfWidth))
	    && (areaPtr[1] <= (rectPtr->bbox[1] - halfWidth))
	    && (areaPtr[2] >= (rectPtr->bbox[2] + halfWidth))
	    && (areaPtr[3] >= (rectPtr->bbox[3] + halfWidth))) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * OvalToArea --
 *
 *	This function is called to determine whether an item lies entirely
 *	inside, entirely outside, or overlapping a given rectangular area.
 *
 * Results:
 *	-1 is returned if the item is entirely outside the area given by
 *	rectPtr, 0 if it overlaps, and 1 if it is entirely inside the given
 *	area.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static int
OvalToArea(
    Tk_PathCanvas canvas,	/* Canvas containing item. */
    Tk_PathItem *itemPtr,	/* Item to check against oval. */
    double *areaPtr)		/* Pointer to array of four coordinates (x1,
				 * y1, x2, y2) describing rectangular area. */
{
    RectOvalItem *ovalPtr = (RectOvalItem *) itemPtr;
    double oval[4], halfWidth;
    int result;
    double width;
    Tk_PathState state = itemPtr->state;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    width = ovalPtr->outline.width;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (ovalPtr->outline.activeWidth>width) {
	    width = ovalPtr->outline.activeWidth;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (ovalPtr->outline.disabledWidth>0) {
	    width = ovalPtr->outline.disabledWidth;
	}
    }

    /*
     * Expand the oval to include the width of the outline, if any.
     */

    halfWidth = width/2.0;
    if (ovalPtr->outline.gc == None) {
	halfWidth = 0.0;
    }
    oval[0] = ovalPtr->bbox[0] - halfWidth;
    oval[1] = ovalPtr->bbox[1] - halfWidth;
    oval[2] = ovalPtr->bbox[2] + halfWidth;
    oval[3] = ovalPtr->bbox[3] + halfWidth;

    result = TkOvalToArea(oval, areaPtr);

    /*
     * If the rectangle appears to overlap the oval and the oval isn't filled,
     * do one more check to see if perhaps all four of the rectangle's corners
     * are totally inside the oval's unfilled center, in which case we should
     * return "outside".
     */

    if ((result == 0) && (ovalPtr->outline.gc != None)
	    && (ovalPtr->fillGC == None)) {
	double centerX, centerY, height;
	double xDelta1, yDelta1, xDelta2, yDelta2;

	centerX = (ovalPtr->bbox[0] + ovalPtr->bbox[2])/2.0;
	centerY = (ovalPtr->bbox[1] + ovalPtr->bbox[3])/2.0;
	width = (ovalPtr->bbox[2] - ovalPtr->bbox[0])/2.0 - halfWidth;
	height = (ovalPtr->bbox[3] - ovalPtr->bbox[1])/2.0 - halfWidth;
	xDelta1 = (areaPtr[0] - centerX)/width;
	xDelta1 *= xDelta1;
	yDelta1 = (areaPtr[1] - centerY)/height;
	yDelta1 *= yDelta1;
	xDelta2 = (areaPtr[2] - centerX)/width;
	xDelta2 *= xDelta2;
	yDelta2 = (areaPtr[3] - centerY)/height;
	yDelta2 *= yDelta2;
	if (((xDelta1 + yDelta1) < 1.0)
		&& ((xDelta1 + yDelta2) < 1.0)
		&& ((xDelta2 + yDelta1) < 1.0)
		&& ((xDelta2 + yDelta2) < 1.0)) {
	    return -1;
	}
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * ScaleRectOval --
 *
 *	This function is invoked to rescale a rectangle or oval item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The rectangle or oval referred to by itemPtr is rescaled so that the
 *	following transformation is applied to all point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleRectOval(
    Tk_PathCanvas canvas,	/* Canvas containing rectangle. */
    Tk_PathItem *itemPtr,	/* Rectangle to be scaled. */
    double originX, double originY,
				/* Origin about which to scale rect. */
    double scaleX,		/* Amount to scale in X direction. */
    double scaleY)		/* Amount to scale in Y direction. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    rectOvalPtr->bbox[0] = originX + scaleX*(rectOvalPtr->bbox[0] - originX);
    rectOvalPtr->bbox[1] = originY + scaleY*(rectOvalPtr->bbox[1] - originY);
    rectOvalPtr->bbox[2] = originX + scaleX*(rectOvalPtr->bbox[2] - originX);
    rectOvalPtr->bbox[3] = originY + scaleY*(rectOvalPtr->bbox[3] - originY);
    ComputeRectOvalBbox(canvas, rectOvalPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateRectOval --
 *
 *	This function is called to move a rectangle or oval by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the rectangle or oval is offset by (xDelta, yDelta),
 *	and the bounding box is updated in the generic part of the item
 *	structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateRectOval(
    Tk_PathCanvas canvas,	/* Canvas containing item. */
    Tk_PathItem *itemPtr,	/* Item that is being moved. */
    double deltaX, double deltaY)
				/* Amount by which item is to be moved. */
{
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;

    rectOvalPtr->bbox[0] += deltaX;
    rectOvalPtr->bbox[1] += deltaY;
    rectOvalPtr->bbox[2] += deltaX;
    rectOvalPtr->bbox[3] += deltaY;
    ComputeRectOvalBbox(canvas, rectOvalPtr);
}

/*
 *--------------------------------------------------------------
 *
 * RectOvalToPostscript --
 *
 *	This function is called to generate Postscript for rectangle and oval
 *	items.
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs in
 *	generating Postscript then an error message is left in the interp's
 *	result, replacing whatever used to be there. If no error occurs, then
 *	Postscript for the rectangle is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
RectOvalToPostscript(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_PathCanvas canvas,	/* Information about overall canvas. */
    Tk_PathItem *itemPtr,	/* Item for which Postscript is wanted. */
    int prepass)		/* 1 means this is a prepass to collect font
				 * information; 0 means final Postscript is
				 * being created. */
{
    char pathCmd[500];
    RectOvalItem *rectOvalPtr = (RectOvalItem *) itemPtr;
    double y1, y2;
    XColor *color;
    XColor *fillColor;
    Pixmap fillStipple;
    Tk_PathState state = itemPtr->state;

    y1 = Tk_PathCanvasPsY(canvas, rectOvalPtr->bbox[1]);
    y2 = Tk_PathCanvasPsY(canvas, rectOvalPtr->bbox[3]);

    /*
     * Generate a string that creates a path for the rectangle or oval. This
     * is the only part of the function's code that is type-specific.
     */

    if (rectOvalPtr->header.typePtr == &tkRectangleType) {
	sprintf(pathCmd, "%.15g %.15g moveto %.15g 0 rlineto 0 %.15g rlineto %.15g 0 rlineto closepath\n",
		rectOvalPtr->bbox[0], y1,
		rectOvalPtr->bbox[2]-rectOvalPtr->bbox[0], y2-y1,
		rectOvalPtr->bbox[0]-rectOvalPtr->bbox[2]);
    } else {
	sprintf(pathCmd, "matrix currentmatrix\n%.15g %.15g translate %.15g %.15g scale 1 0 moveto 0 0 1 0 360 arc\nsetmatrix\n",
		(rectOvalPtr->bbox[0] + rectOvalPtr->bbox[2])/2, (y1 + y2)/2,
		(rectOvalPtr->bbox[2] - rectOvalPtr->bbox[0])/2, (y1 - y2)/2);
    }

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    color = rectOvalPtr->outline.color;
    fillColor = rectOvalPtr->fillColor;
    fillStipple = rectOvalPtr->fillStipple;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (rectOvalPtr->outline.activeColor!=NULL) {
	    color = rectOvalPtr->outline.activeColor;
	}
	if (rectOvalPtr->activeFillColor!=NULL) {
	    fillColor = rectOvalPtr->activeFillColor;
	}
	if (rectOvalPtr->activeFillStipple!=None) {
	    fillStipple = rectOvalPtr->activeFillStipple;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (rectOvalPtr->outline.disabledColor!=NULL) {
	    color = rectOvalPtr->outline.disabledColor;
	}
	if (rectOvalPtr->disabledFillColor!=NULL) {
	    fillColor = rectOvalPtr->disabledFillColor;
	}
	if (rectOvalPtr->disabledFillStipple!=None) {
	    fillStipple = rectOvalPtr->disabledFillStipple;
	}
    }

    /*
     * First draw the filled area of the rectangle.
     */

    if (fillColor != NULL) {
	Tcl_AppendResult(interp, pathCmd, NULL);
	if (Tk_PathCanvasPsColor(interp, canvas, fillColor) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (fillStipple != None) {
	    Tcl_AppendResult(interp, "clip ", NULL);
	    if (Tk_PathCanvasPsStipple(interp, canvas, fillStipple) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (color != NULL) {
		Tcl_AppendResult(interp, "grestore gsave\n", NULL);
	    }
	} else {
	    Tcl_AppendResult(interp, "fill\n", NULL);
	}
    }

    /*
     * Now draw the outline, if there is one.
     */

    if (color != NULL) {
	Tcl_AppendResult(interp, pathCmd, "0 setlinejoin 2 setlinecap\n",
		NULL);
	if (Tk_PathCanvasPsOutline(canvas, itemPtr,
		&(rectOvalPtr->outline))!= TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
