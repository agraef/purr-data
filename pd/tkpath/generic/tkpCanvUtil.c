/*
 * tkpCanvUtil.c --
 *
 *	This file contains a collection of utility functions used by the
 *	implementations of various canvas item types.
 *
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#include "tkInt.h"
#include "tkpCanvas.h"
#include "tkIntPath.h"
#include "tkPathStyle.h"
#include <assert.h>

/*
 * Structures defined only in this file.
 */

typedef struct SmoothAssocData {
    struct SmoothAssocData *nextPtr;
				/* Pointer to next SmoothAssocData. */
    Tk_PathSmoothMethod smooth;	/* Name and functions associated with this
				 * option. */
} SmoothAssocData;

Tk_PathSmoothMethod tkPathBezierSmoothMethod = {
    "true",
    TkPathMakeBezierCurve,
    (void (*) (Tcl_Interp *interp, Tk_PathCanvas canvas, double *coordPtr,
	    int numPoints, int numSteps)) TkPathMakeBezierPostscript,
};
static Tk_PathSmoothMethod tkPathRawSmoothMethod = {
    "raw",
    TkPathMakeRawCurve,
    (void (*) (Tcl_Interp *interp, Tk_PathCanvas canvas, double *coordPtr,
	    int numPoints, int numSteps)) TkPathMakeRawCurvePostscript,
};

/*
 * Function forward-declarations.
 */

static void		    SmoothMethodCleanupProc(ClientData clientData,
				Tcl_Interp *interp);
static SmoothAssocData *    InitSmoothMethods(Tcl_Interp *interp);
static int		    FindSmoothMethod(Tcl_Interp *interp, Tcl_Obj *valueObj,
				Tk_PathSmoothMethod **smoothPtr);
static int		    DashConvert(char *l, CONST char *p, int n,
				double width);
static void		    TranslateAndAppendCoords(TkPathCanvas *canvPtr,
				double x, double y, XPoint *outArr, int numOut);

static Tk_Dash *	    TkDashNew(Tcl_Interp *interp, Tcl_Obj *dashObj);
static void		    TkDashFree(Tk_Dash *dashPtr);

#ifndef ABS
#	define ABS(a)    	(((a) >= 0)  ? (a) : -1*(a))
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tk_PathCanvasTkwin --
 *
 *	Given a token for a canvas, this function returns the widget that
 *	represents the canvas.
 *
 * Results:
 *	The return value is a handle for the widget.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_PathCanvasTkwin(
    Tk_PathCanvas canvas)		/* Token for the canvas. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    return canvasPtr->tkwin;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PathCanvasDrawableCoords --
 *
 *	Given an (x,y) coordinate pair within a canvas, this function
 *	returns the corresponding coordinates at which the point should
 *	be drawn in the drawable used for display.
 *
 * Results:
 *	There is no return value. The values at *drawableXPtr and
 *	*drawableYPtr are filled in with the coordinates at which x and y
 *	should be drawn. These coordinates are clipped to fit within a
 *	"short", since this is what X uses in most cases for drawing.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PathCanvasDrawableCoords(
    Tk_PathCanvas canvas,		/* Token for the canvas. */
    double x,			/* Coordinates in canvas space. */
    double y,
    short *drawableXPtr,	/* Screen coordinates are stored here. */
    short *drawableYPtr)
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    double tmp;

    tmp = x - canvasPtr->drawableXOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*drawableXPtr = 32767;
    } else if (tmp < -32768) {
	*drawableXPtr = -32768;
    } else {
	*drawableXPtr = (short) tmp;
    }

    tmp = y - canvasPtr->drawableYOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*drawableYPtr = 32767;
    } else if (tmp < -32768) {
	*drawableYPtr = -32768;
    } else {
	*drawableYPtr = (short) tmp;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PathCanvasWindowCoords --
 *
 *	Given an (x,y) coordinate pair within a canvas, this function returns
 *	the corresponding coordinates in the canvas's window.
 *
 * Results:
 *	There is no return value. The values at *screenXPtr and *screenYPtr
 *	are filled in with the coordinates at which (x,y) appears in the
 *	canvas's window. These coordinates are clipped to fit within a
 *	"short", since this is what X uses in most cases for drawing.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PathCanvasWindowCoords(
    Tk_PathCanvas canvas,		/* Token for the canvas. */
    double x,			/* Coordinates in canvas space. */
    double y,
    short *screenXPtr,		/* Screen coordinates are stored here. */
    short *screenYPtr)
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    double tmp;

    tmp = x - canvasPtr->xOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*screenXPtr = 32767;
    } else if (tmp < -32768) {
	*screenXPtr = -32768;
    } else {
	*screenXPtr = (short) tmp;
    }

    tmp = y - canvasPtr->yOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    if (tmp > 32767) {
	*screenYPtr = 32767;
    } else if (tmp < -32768) {
	*screenYPtr = -32768;
    } else {
	*screenYPtr = (short) tmp;
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasGetCoord --
 *
 *	Given a string, returns a floating-point canvas coordinate
 *	corresponding to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result. If TCL_OK is
 *	returned, then everything went well and the canvas coordinate is
 *	stored at *doublePtr; otherwise TCL_ERROR is returned and an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasGetCoord(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_PathCanvas canvas,		/* Canvas to which coordinate applies. */
    CONST char *string,		/* Describes coordinate (any screen coordinate
				 * form may be used here). */
    double *doublePtr)		/* Place to store converted coordinate. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;

    if (Tk_GetScreenMM(canvasPtr->interp, canvasPtr->tkwin, string,
	    doublePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *doublePtr *= canvasPtr->pixelsPerMM;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasGetCoordFromObj --
 *
 *	Given a string, returns a floating-point canvas coordinate
 *	corresponding to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result. If TCL_OK is
 *	returned, then everything went well and the canvas coordinate is
 *	stored at *doublePtr; otherwise TCL_ERROR is returned and an error
 *	message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasGetCoordFromObj(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_PathCanvas canvas,		/* Canvas to which coordinate applies. */
    Tcl_Obj *obj,		/* Describes coordinate (any screen coordinate
				 * form may be used here). */
    double *doublePtr)		/* Place to store converted coordinate. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;

    if (Tk_GetMMFromObj(canvasPtr->interp, canvasPtr->tkwin, obj,
	    doublePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *doublePtr *= canvasPtr->pixelsPerMM;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PathCanvasSetStippleOrigin --
 *
 *	This function sets the stipple origin in a graphics context so that
 *	stipples drawn with the GC will line up with other stipples previously
 *	drawn in the canvas.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The graphics context is modified.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PathCanvasSetStippleOrigin(
    Tk_PathCanvas canvas,		/* Token for a canvas. */
    GC gc)			/* Graphics context that is about to be used
				 * to draw a stippled pattern as part of
				 * redisplaying the canvas. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;

    XSetTSOrigin(canvasPtr->display, gc, -canvasPtr->drawableXOrigin,
	    -canvasPtr->drawableYOrigin);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PathCanvasSetOffset --
 *
 *	This function sets the stipple offset in a graphics context so that
 *	stipples drawn with the GC will line up with other stipples with the
 *	same offset.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The graphics context is modified.
 *
 *----------------------------------------------------------------------
 */

void
Tk_PathCanvasSetOffset(
    Tk_PathCanvas canvas,	/* Token for a canvas. */
    GC gc,			/* Graphics context that is about to be used
				 * to draw a stippled pattern as part of
				 * redisplaying the canvas. */
    Tk_TSOffset *offset)	/* Offset (may be NULL pointer)*/
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    int flags = 0;
    int x = - canvasPtr->drawableXOrigin;
    int y = - canvasPtr->drawableYOrigin;

    if (offset != NULL) {
	flags = offset->flags;
	x += offset->xoffset;
	y += offset->yoffset;
    }
    if ((flags & TK_OFFSET_RELATIVE) && !(flags & TK_OFFSET_INDEX)) {
	Tk_SetTSOrigin(canvasPtr->tkwin, gc, x - canvasPtr->xOrigin,
		y - canvasPtr->yOrigin);
    } else {
	XSetTSOrigin(canvasPtr->display, gc, x, y);
    }
}

int
TkPathCanvasGetDepth(Tk_PathItem *itemPtr)
{
    int depth = 0;
    Tk_PathItem *walkPtr = itemPtr;

    while (walkPtr->parentPtr != NULL) {
	depth++;
	walkPtr = walkPtr->parentPtr;
    }
    return depth;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathCanvasInheritStyle --
 *
 *	This function returns the style which is inherited from the
 *      parents of the itemPtr using cascading from the root item.
 *	Must use TkPathCanvasFreeInheritedStyle when done.
 *
 * Results:
 *	Tk_PathStyle.
 *
 * Side effects:
 *	May allocate memory for matrix.
 *
 *----------------------------------------------------------------------
 */

Tk_PathStyle
TkPathCanvasInheritStyle(Tk_PathItem *itemPtr, long flags)
{
    int depth, i, anyMatrix = 0;
    Tk_PathItem *walkPtr;
    Tk_PathItemEx *itemExPtr;
    Tk_PathItemEx **parents;
    Tk_PathStyle style;
    TMatrix matrix = kPathUnitTMatrix;
    
    depth = TkPathCanvasGetDepth(itemPtr);
    parents = (Tk_PathItemEx **) ckalloc(depth*sizeof(Tk_PathItemEx *));

    walkPtr = itemPtr, i = 0;
    while (walkPtr->parentPtr != NULL) {
	parents[i] = (Tk_PathItemEx *) walkPtr->parentPtr;
	walkPtr = walkPtr->parentPtr, i++;
    }
    
    /*
     * Cascade the style from the root item to the closest parent.
     * Start by just making a copy of the root's style.
     */
    itemExPtr = parents[depth-1];
    style = itemExPtr->style;
    
    for (i = depth-1; i >= 0; i--) {
	itemExPtr = parents[i];
	
	/* The order of these two merges decides which take precedence. */
	if (i < depth-1) {
	    TkPathStyleMergeStyles(&itemExPtr->style, &style, flags);
	}
	if (itemExPtr->styleInst != NULL) {
	    TkPathStyleMergeStyles(itemExPtr->styleInst->masterPtr, &style, flags);
	}
	if (style.matrixPtr != NULL) {
	    anyMatrix = 1;
	    MMulTMatrix(style.matrixPtr, &matrix);
	}
	/*
	 * We set matrix to NULL to detect if set in group.
	 */
	style.matrixPtr = NULL;
    }
    
    /*
     * Merge the parents style with the actual items style.
     * The order of these two merges decides which take precedence.
     */
    itemExPtr = (Tk_PathItemEx *) itemPtr;
    TkPathStyleMergeStyles(&itemExPtr->style, &style, flags);
    if (itemExPtr->styleInst != NULL) {
	TkPathStyleMergeStyles(itemExPtr->styleInst->masterPtr, &style, flags);
    }    
    if (style.matrixPtr != NULL) {
	anyMatrix = 1;
	MMulTMatrix(style.matrixPtr, &matrix);
    }
    if (anyMatrix) {
        style.matrixPtr = (TMatrix *) ckalloc(sizeof(TMatrix));
	memcpy(style.matrixPtr, &matrix, sizeof(TMatrix));
    }
    ckfree((char *) parents);
    return style;
}

void
TkPathCanvasFreeInheritedStyle(Tk_PathStyle *stylePtr)
{
    if (stylePtr->matrixPtr != NULL) {
	ckfree((char *) stylePtr->matrixPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathCanvasInheritTMatrix --
 *
 *	Does the same job as TkPathCanvasInheritStyle but for the
 *	TMatrix only. No memory allocated.
 *	Note that we don't do the last step of concatenating the items
 *	own TMatrix since that depends on its specific storage.
 *
 * Results:
 *	TMatrix.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TMatrix
TkPathCanvasInheritTMatrix(Tk_PathItem *itemPtr)
{
    int depth, i;
    Tk_PathItem *walkPtr;
    Tk_PathItemEx *itemExPtr;
    Tk_PathItemEx **parents;
    Tk_PathStyle *stylePtr;
    TMatrix matrix = kPathUnitTMatrix, *matrixPtr = NULL;

    depth = TkPathCanvasGetDepth(itemPtr);
    parents = (Tk_PathItemEx **) ckalloc(depth*sizeof(Tk_PathItemEx *));

    walkPtr = itemPtr, i = 0;
    while (walkPtr->parentPtr != NULL) {
	parents[i] = (Tk_PathItemEx *) walkPtr->parentPtr;
	walkPtr = walkPtr->parentPtr, i++;
    }

    for (i = depth-1; i >= 0; i--) {
	itemExPtr = parents[i];
	
	/* The order of these two merges decides which take precedence. */
	matrixPtr = itemExPtr->style.matrixPtr;
	if (itemExPtr->styleInst != NULL) {
	    stylePtr = itemExPtr->styleInst->masterPtr;
	    if (stylePtr->mask & PATH_STYLE_OPTION_MATRIX) {
		matrixPtr = stylePtr->matrixPtr;
	    }
	}
	if (matrixPtr != NULL) {
	    MMulTMatrix(matrixPtr, &matrix);
	}	
    }
    ckfree((char *) parents);
    return matrix;
}

/* TkPathCanvasGradientTable etc.: this is just accessor functions to hide
   the internals of the TkPathCanvas */
   
Tcl_HashTable *
TkPathCanvasGradientTable(Tk_PathCanvas canvas)
{
    return &((TkPathCanvas *)canvas)->gradientTable;
}

Tcl_HashTable *
TkPathCanvasStyleTable(Tk_PathCanvas canvas)
{
    return &((TkPathCanvas *)canvas)->styleTable;
}

Tk_PathState
TkPathCanvasState(Tk_PathCanvas canvas)
{
    return ((TkPathCanvas *)canvas)->canvas_state;
}

Tk_PathItem *
TkPathCanvasCurrentItem(Tk_PathCanvas canvas)
{
    return ((TkPathCanvas *)canvas)->currentItemPtr;
}

Tk_PathItem *
TkPathCanvasParentItem(Tk_PathItem *itemPtr)
{
    return itemPtr->parentPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PathCanvasGetTextInfo --
 *
 *	This function returns a pointer to a structure containing information
 *	about the selection and insertion cursor for a canvas widget. Items
 *	such as text items save the pointer and use it to share access to the
 *	information with the generic canvas code.
 *
 * Results:
 *	The return value is a pointer to the structure holding text
 *	information for the canvas. Most of the fields should not be modified
 *	outside the generic canvas code; see the user documentation for
 *	details.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_PathCanvasTextInfo *
Tk_PathCanvasGetTextInfo(
    Tk_PathCanvas canvas)	/* Token for the canvas widget. */
{
    return &((TkPathCanvas *) canvas)->textInfo;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathAllocTagsFromObj --
 *
 *	Create a new Tk_PathTags record and fill it with a tag object list.
 *
 * Results:
 *	A pointer to Tk_PathTags record or NULL if failed.
 *
 * Side effects:
 *	New Tk_PathTags possibly allocated.
 *
 *----------------------------------------------------------------------
 */

Tk_PathTags *
TkPathAllocTagsFromObj(
	Tcl_Interp *interp, 
	Tcl_Obj *valuePtr)	/* If NULL we just create an empty Tk_PathTags struct. */
{
    Tk_PathTags *tagsPtr;
    int objc, i, len;
    Tcl_Obj **objv;
    
    if (ObjectIsEmpty(valuePtr)) {
	objc = 0;
    } else if (Tcl_ListObjGetElements(interp, valuePtr, &objc, &objv) != TCL_OK) {
	return NULL;
    }
    len = MAX(objc, TK_PATHTAG_SPACE);
    tagsPtr = (Tk_PathTags *) ckalloc(sizeof(Tk_PathTags));
    tagsPtr->tagSpace = len;
    tagsPtr->numTags = objc;
    tagsPtr->tagPtr = (Tk_Uid *) ckalloc((unsigned) (len * sizeof(Tk_Uid)));
    for (i = 0; i < objc; i++) {
	tagsPtr->tagPtr[i] = Tk_GetUid(Tcl_GetStringFromObj(objv[i], NULL));
    }
    return tagsPtr;
}

static void
TkPathFreeTags(Tk_PathTags *tagsPtr)
{
    if (tagsPtr->tagPtr != NULL) {
	ckfree((char *) tagsPtr->tagPtr);
    }    
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasTagsOptionSetProc --
 *
 *	This function is invoked during option processing to handle "-tags"
 *	options for canvas items.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The tags for a given item get replaced by those indicated in the value
 *	argument.
 *
 *--------------------------------------------------------------
 */

int Tk_PathCanvasTagsOptionSetProc(
    ClientData clientData,
    Tcl_Interp *interp,	    /* Current interp; may be used for errors. */
    Tk_Window tkwin,	    /* Window for which option is being set. */
    Tcl_Obj **value,	    /* Pointer to the pointer to the value object.
                             * We use a pointer to the pointer because
                             * we may need to return a value (NULL). */
    char *recordPtr,	    /* Pointer to storage for the widget record. */
    int internalOffset,	    /* Offset within *recordPtr at which the
                               internal value is to be stored. */
    char *oldInternalPtr,   /* Pointer to storage for the old value. */
    int flags)		    /* Flags for the option, set Tk_SetOptions. */
{
    char *internalPtr;	    /* Points to location in record where
                             * internal representation of value should
                             * be stored, or NULL. */
    Tcl_Obj *valuePtr;
    Tk_PathTags *newPtr = NULL;
    
    valuePtr = *value;
    if (internalOffset >= 0) {
        internalPtr = recordPtr + internalOffset;
    } else {
        internalPtr = NULL;
    }
    if ((flags & TK_OPTION_NULL_OK) && ObjectIsEmpty(valuePtr)) {
	valuePtr = NULL;
	newPtr = NULL;
    }
    if (internalPtr != NULL) {
	if (valuePtr != NULL) {
	    newPtr = TkPathAllocTagsFromObj(interp, valuePtr);
	    if (newPtr == NULL) {
		return TCL_ERROR;
	    }
        }
	*((Tk_PathTags **) oldInternalPtr) = *((Tk_PathTags **) internalPtr);
	*((Tk_PathTags **) internalPtr) = newPtr;
    }
    return TCL_OK;
}

Tcl_Obj *
Tk_PathCanvasTagsOptionGetProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *recordPtr,		/* Pointer to widget record. */
    int internalOffset)		/* Offset within *recordPtr containing the
				 * value. */
{
    Tk_PathTags	*tagsPtr;
    Tcl_Obj 	*listObj;
    int		i;
    
    tagsPtr = *((Tk_PathTags **) (recordPtr + internalOffset));
    listObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
    if (tagsPtr != NULL) {
	for (i = 0; i < tagsPtr->numTags; i++) {
	    Tcl_ListObjAppendElement(NULL, listObj, 
				     Tcl_NewStringObj((char *) tagsPtr->tagPtr[i], -1));
	}
    }
    return listObj;
}

void
Tk_PathCanvasTagsOptionRestoreProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr,		/* Pointer to storage for value. */
    char *oldInternalPtr)	/* Pointer to old value. */
{
    *(Tk_PathTags **)internalPtr = *(Tk_PathTags **)oldInternalPtr;
}

void
Tk_PathCanvasTagsOptionFreeProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr)		/* Pointer to storage for value. */
{
    Tk_PathTags	*tagsPtr;
    
    tagsPtr = *((Tk_PathTags **) internalPtr);
    if (tagsPtr != NULL) {
	TkPathFreeTags(tagsPtr);
        ckfree(*((char **) internalPtr));
        *((char **) internalPtr) = NULL;
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasTagsParseProc --
 *
 *	This function is invoked during option processing to handle "-tags"
 *	options for canvas items.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The tags for a given item get replaced by those indicated in the value
 *	argument.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasTagsParseProc(
    ClientData clientData,	/* Not used.*/
    Tcl_Interp *interp,		/* Used for reporting errors. */
    Tk_Window tkwin,		/* Window containing canvas widget. */
    CONST char *value,		/* Value of option (list of tag names). */
    char *widgRec,		/* Pointer to record for item. */
    int offset)			/* Offset into item (ignored). */
{
    register Tk_PathItem *itemPtr = (Tk_PathItem *) widgRec;
    int argc, i;
    CONST char **argv;
    Tk_Uid *newPtr;

    /*
     * Break the value up into the individual tag names.
     */

    if (Tcl_SplitList(interp, value, &argc, &argv) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Make sure that there's enough space in the item to hold the tag names.
     */

    if (itemPtr->tagSpace < argc) {
	newPtr = (Tk_Uid *) ckalloc((unsigned) (argc * sizeof(Tk_Uid)));
	for (i = itemPtr->numTags-1; i >= 0; i--) {
	    newPtr[i] = itemPtr->tagPtr[i];
	}
	if (itemPtr->tagPtr != itemPtr->staticTagSpace) {
	    ckfree((char *) itemPtr->tagPtr);
	}
	itemPtr->tagPtr = newPtr;
	itemPtr->tagSpace = argc;
    }
    itemPtr->numTags = argc;
    for (i = 0; i < argc; i++) {
	itemPtr->tagPtr[i] = Tk_GetUid(argv[i]);
    }
    ckfree((char *) argv);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasTagsPrintProc --
 *
 *	This function is invoked by the Tk configuration code to produce a
 *	printable string for the "-tags" configuration option for canvas
 *	items.
 *
 * Results:
 *	The return value is a string describing all the tags for the item
 *	referred to by "widgRec". In addition, *freeProcPtr is filled in with
 *	the address of a function to call to free the result string when it's
 *	no longer needed (or NULL to indicate that the string doesn't need to
 *	be freed).
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

char *
Tk_PathCanvasTagsPrintProc(
    ClientData clientData,	/* Ignored. */
    Tk_Window tkwin,		/* Window containing canvas widget. */
    char *widgRec,		/* Pointer to record for item. */
    int offset,			/* Ignored. */
    Tcl_FreeProc **freeProcPtr)	/* Pointer to variable to fill in with
				 * information about how to reclaim storage
				 * for return string. */
{
    register Tk_PathItem *itemPtr = (Tk_PathItem *) widgRec;

    if (itemPtr->numTags == 0) {
	*freeProcPtr = NULL;
	return "";
    }
    if (itemPtr->numTags == 1) {
	*freeProcPtr = NULL;
	return (char *) itemPtr->tagPtr[0];
    }
    *freeProcPtr = TCL_DYNAMIC;
    return Tcl_Merge(itemPtr->numTags, (CONST char **) itemPtr->tagPtr);
}

/* Return NULL on error and leave error message */

static Tk_Dash *
TkDashNew(Tcl_Interp *interp, Tcl_Obj *dashObj)
{
    Tk_Dash *dashPtr;
    
    dashPtr = (Tk_Dash *) ckalloc(sizeof(Tk_Dash));
    /*
     * NB: Tk_GetDash tries to free any existing pattern unless we zero this.
     */
    dashPtr->number = 0;
    if (Tk_GetDash(interp, Tcl_GetString(dashObj), dashPtr) != TCL_OK) {
	goto error;
    }
    return dashPtr;
    
error:
    TkDashFree(dashPtr);
    return NULL;
}

static void
TkDashFree(Tk_Dash *dashPtr)
{
    if (dashPtr != NULL) {
	if (ABS(dashPtr->number) > sizeof(char *)) {
	    ckfree((char *) dashPtr->pattern.pt);
	}
	ckfree((char *) dashPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_DashOptionSetProc, Tk_DashOptionGetProc, 
 *	Tk_DashOptionRestoreProc, Tk_DashOptionRestoreProc --
 *
 *	These functions are invoked during option processing to handle 
 *	"-dash", "-activedash" and "-disableddash" 
 *	options for canvas objects.
 *
 * Results:
 *	According to the Tk_ObjCustomOption struct.
 *
 * Side effects:
 *	Memory allocated or freed.
 *
 *--------------------------------------------------------------
 */

int Tk_DashOptionSetProc(
    ClientData clientData,
    Tcl_Interp *interp,	    /* Current interp; may be used for errors. */
    Tk_Window tkwin,	    /* Window for which option is being set. */
    Tcl_Obj **value,	    /* Pointer to the pointer to the value object.
                             * We use a pointer to the pointer because
                             * we may need to return a value (NULL). */
    char *recordPtr,	    /* Pointer to storage for the widget record. */
    int internalOffset,	    /* Offset within *recordPtr at which the
                               internal value is to be stored. */
    char *oldInternalPtr,   /* Pointer to storage for the old value. */
    int flags)		    /* Flags for the option, set Tk_SetOptions. */
{
    char *internalPtr;	    /* Points to location in record where
                             * internal representation of value should
                             * be stored, or NULL. */
    Tcl_Obj *valuePtr;
    Tk_Dash *newPtr = NULL;
    
    valuePtr = *value;
    if (internalOffset >= 0) {
        internalPtr = recordPtr + internalOffset;
    } else {
        internalPtr = NULL;
    }
    if ((flags & TK_OPTION_NULL_OK) && ObjectIsEmpty(valuePtr)) {
	valuePtr = NULL;
	newPtr = NULL;
    }
    if (internalPtr != NULL) {
	if (valuePtr != NULL) {
	    newPtr = TkDashNew(interp, valuePtr);
	    if (newPtr == NULL) {
		return TCL_ERROR;
	    }
        }
	*((Tk_Dash **) oldInternalPtr) = *((Tk_Dash **) internalPtr);
	*((Tk_Dash **) internalPtr) = newPtr;
    }
    return TCL_OK;
}

Tcl_Obj *
Tk_DashOptionGetProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *recordPtr,		/* Pointer to widget record. */
    int internalOffset)		/* Offset within *recordPtr containing the
				 * value. */
{
    Tk_Dash *dashPtr;
    Tcl_Obj *objPtr = NULL;
    char *buffer = NULL;
    char *p;
    int i;

    dashPtr = *((Tk_Dash **) (recordPtr + internalOffset));
    
    if (dashPtr != NULL) {	
	i = dashPtr->number;
	if (i < 0) {
	    i = -i;
	    buffer = (char *) ckalloc((unsigned int) (i+1));
	    p = (i > (int)sizeof(char *)) ? dashPtr->pattern.pt : dashPtr->pattern.array;
	    memcpy(buffer, p, (unsigned int) i);
	    buffer[i] = 0;
	} else if (!i) {
	    buffer = (char *) ckalloc(1);
	    buffer[0] = '\0';
	} else {
	    buffer = (char *)ckalloc((unsigned int) (4*i));
	    p = (i > (int)sizeof(char *)) ? dashPtr->pattern.pt : dashPtr->pattern.array;
	    sprintf(buffer, "%d", *p++ & 0xff);
	    while(--i) {
		sprintf(buffer+strlen(buffer), " %d", *p++ & 0xff);
	    }
	}
	objPtr = Tcl_NewStringObj(buffer, -1);
    }
    if (buffer != NULL) {
	ckfree((char *) buffer);
    }
    return objPtr;
}

void
Tk_DashOptionRestoreProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr,		/* Pointer to storage for value. */
    char *oldInternalPtr)	/* Pointer to old value. */
{
    *(Tk_Dash **)internalPtr = *(Tk_Dash **)oldInternalPtr;
}

void
Tk_DashOptionFreeProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr)		/* Pointer to storage for value. */
{
    if (*((char **) internalPtr) != NULL) {
        TkDashFree(*(Tk_Dash **) internalPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * InitSmoothMethods --
 *
 *	This function is invoked to set up the initial state of the list of
 *	"-smooth" methods. It should only be called when the list installed
 *	in the interpreter is NULL.
 *
 * Results:
 *	Pointer to the start of the list of default smooth methods.
 *
 * Side effects:
 *	A linked list of smooth methods is created and attached to the
 *	interpreter's association key "smoothPathMethod"
 *
 *--------------------------------------------------------------
 */

static SmoothAssocData *
InitSmoothMethods(
    Tcl_Interp *interp)
{
    SmoothAssocData *methods, *ptr;

    methods = (SmoothAssocData *) ckalloc(sizeof(SmoothAssocData));
    methods->smooth.name = tkPathRawSmoothMethod.name;
    methods->smooth.coordProc = tkPathRawSmoothMethod.coordProc;
    methods->smooth.postscriptProc = tkPathRawSmoothMethod.postscriptProc;

    methods->nextPtr = (SmoothAssocData *) ckalloc(sizeof(SmoothAssocData));

    ptr = methods->nextPtr;
    ptr->smooth.name = tkPathBezierSmoothMethod.name;
    ptr->smooth.coordProc = tkPathBezierSmoothMethod.coordProc;
    ptr->smooth.postscriptProc = tkPathBezierSmoothMethod.postscriptProc;
    ptr->nextPtr = NULL;

    Tcl_SetAssocData(interp, "smoothPathMethod", SmoothMethodCleanupProc,
	    (ClientData) methods);
    return methods;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCreateSmoothMethod --
 *
 *	This function is invoked to add additional values for the "-smooth"
 *	option to the list.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	In the future "-smooth <name>" will be accepted as smooth method for
 *	the line and polygon.
 *
 *--------------------------------------------------------------
 */

void
Tk_PathCreateSmoothMethod(
    Tcl_Interp *interp,
    Tk_PathSmoothMethod *smooth)
{
    SmoothAssocData *methods, *typePtr2, *prevPtr, *ptr;
    methods = (SmoothAssocData *) Tcl_GetAssocData(interp, "smoothPathMethod",
	    NULL);

    /*
     * Initialize if we were not previously initialized.
     */

    if (methods == NULL) {
	methods = InitSmoothMethods(interp);
    }

    /*
     * If there's already a smooth method with the given name, remove it.
     */

    for (typePtr2 = methods, prevPtr = NULL; typePtr2 != NULL;
	    prevPtr = typePtr2, typePtr2 = typePtr2->nextPtr) {
	if (!strcmp(typePtr2->smooth.name, smooth->name)) {
	    if (prevPtr == NULL) {
		methods = typePtr2->nextPtr;
	    } else {
		prevPtr->nextPtr = typePtr2->nextPtr;
	    }
	    ckfree((char *) typePtr2);
	    break;
	}
    }
    ptr = (SmoothAssocData *) ckalloc(sizeof(SmoothAssocData));
    ptr->smooth.name = smooth->name;
    ptr->smooth.coordProc = smooth->coordProc;
    ptr->smooth.postscriptProc = smooth->postscriptProc;
    ptr->nextPtr = methods;
    Tcl_SetAssocData(interp, "smoothPathMethod", SmoothMethodCleanupProc,
	    (ClientData) ptr);
}

/*
 *----------------------------------------------------------------------
 *
 * SmoothMethodCleanupProc --
 *
 *	This function is invoked whenever an interpreter is deleted to
 *	cleanup the smooth methods.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Smooth methods are removed.
 *
 *----------------------------------------------------------------------
 */

static void
SmoothMethodCleanupProc(
    ClientData clientData,	/* Points to "smoothPathMethod" AssocData for the
				 * interpreter. */
    Tcl_Interp *interp)		/* Interpreter that is being deleted. */
{
    SmoothAssocData *ptr, *methods = (SmoothAssocData *) clientData;

    while (methods != NULL) {
	methods = (ptr = methods)->nextPtr;
	ckfree((char *) ptr);
    }
}

static int
FindSmoothMethod(Tcl_Interp *interp, 
    Tcl_Obj *valueObj,
    Tk_PathSmoothMethod **smoothPtr)	/* Place to store converted result. */
{
    Tk_PathSmoothMethod *smooth = NULL;
    int b;
    char *value;
    size_t length;
    SmoothAssocData *methods;

    value = Tcl_GetString(valueObj);
    length = strlen(value);
    methods = (SmoothAssocData *) Tcl_GetAssocData(interp, "smoothPathMethod",
	    NULL);

    /*
     * Not initialized yet; fix that now.
     */

    if (methods == NULL) {
	methods = InitSmoothMethods(interp);
    }

    /*
     * Backward compatability hack.
     */

    if (strncmp(value, "bezier", length) == 0) {
	smooth = &tkPathBezierSmoothMethod;
    }

    /*
     * Search the list of installed smooth methods.
     */

    while (methods != NULL) {
	if (strncmp(value, methods->smooth.name, length) == 0) {
	    if (smooth != NULL) {
		Tcl_AppendResult(interp, "ambiguous smooth method \"", value,
			"\"", NULL);
		return TCL_ERROR;
	    }
	    smooth = &methods->smooth;
	}
	methods = methods->nextPtr;
    }
    if (smooth) {
	*smoothPtr = smooth;
	return TCL_OK;
    }

    /*
     * Did not find it. Try parsing as a boolean instead.
     */

    if (Tcl_GetBooleanFromObj(interp, valueObj, &b) != TCL_OK) {
	return TCL_ERROR;
    }
    *smoothPtr = b ? &tkPathBezierSmoothMethod : NULL;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathSmoothOptionSetProc --
 *
 *	This function is invoked during option processing to handle "-smooth"
 *	options for canvas items.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	The smooth option for a given item gets replaced by the value
 *	indicated in the value argument.
 *
 *--------------------------------------------------------------
 */

int 
TkPathSmoothOptionSetProc(
    ClientData clientData,
    Tcl_Interp *interp,	    /* Current interp; may be used for errors. */
    Tk_Window tkwin,	    /* Window for which option is being set. */
    Tcl_Obj **value,	    /* Pointer to the pointer to the value object.
                             * We use a pointer to the pointer because
                             * we may need to return a value (NULL). */
    char *recordPtr,	    /* Pointer to storage for the widget record. */
    int internalOffset,	    /* Offset within *recordPtr at which the
                               internal value is to be stored. */
    char *oldInternalPtr,   /* Pointer to storage for the old value. */
    int flags)		    /* Flags for the option, set Tk_SetOptions. */
{
    char *internalPtr;	    /* Points to location in record where
                             * internal representation of value should
                             * be stored, or NULL. */
    Tcl_Obj *valuePtr;
    Tk_PathSmoothMethod *newPtr = NULL;
    
    valuePtr = *value;
    if (internalOffset >= 0) {
        internalPtr = recordPtr + internalOffset;
    } else {
        internalPtr = NULL;
    }
    if ((flags & TK_OPTION_NULL_OK) && ObjectIsEmpty(valuePtr)) {
	valuePtr = NULL;
	newPtr = NULL;
    }
    if (internalPtr != NULL) {
	if (valuePtr != NULL) {
	    if (FindSmoothMethod(interp, valuePtr, &newPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
        }
	*((Tk_PathSmoothMethod **) oldInternalPtr) = *((Tk_PathSmoothMethod **) internalPtr);
	*((Tk_PathSmoothMethod **) internalPtr) = newPtr;
    }
    return TCL_OK;
}

Tcl_Obj *
TkPathSmoothOptionGetProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *recordPtr,		/* Pointer to widget record. */
    int internalOffset)		/* Offset within *recordPtr containing the
				 * value. */
{
    Tk_PathSmoothMethod *smooth;
    
    smooth = *((Tk_PathSmoothMethod **) (recordPtr + internalOffset));
    return (smooth) ? Tcl_NewStringObj(smooth->name, -1) : Tcl_NewBooleanObj(0);
}

void
TkPathSmoothOptionRestoreProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr,		/* Pointer to storage for value. */
    char *oldInternalPtr)	/* Pointer to old value. */
{
    *(Tk_PathSmoothMethod **)internalPtr = *(Tk_PathSmoothMethod **)oldInternalPtr;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCreateOutline
 *
 *	This function initializes the Tk_PathOutline structure with default
 *	values.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

void
Tk_PathCreateOutline(
    Tk_PathOutline *outline)	/* Outline structure to be filled in. */
{
    outline->gc = None;
    outline->width = 1.0;
    outline->activeWidth = 0.0;
    outline->disabledWidth = 0.0;
    outline->offset = 0;
    outline->dashPtr = NULL;
    outline->activeDashPtr = NULL;
    outline->disabledDashPtr = NULL;
    outline->tsoffsetPtr = NULL;
    outline->color = NULL;
    outline->activeColor = NULL;
    outline->disabledColor = NULL;
    outline->stipple = None;
    outline->activeStipple = None;
    outline->disabledStipple = None;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathDeleteOutline
 *
 *	This function frees all memory that might be allocated and referenced
 *	in the Tk_PathOutline structure.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

/* @@@ I don't belive this should ever be called since the memory is handled by Option! */

void
Tk_PathDeleteOutline(
    Display *display,		/* Display containing window. */
    Tk_PathOutline *outline)
{
    if (outline->gc != None) {
	Tk_FreeGC(display, outline->gc);
        outline->gc = None;
    }
    if (outline->color != NULL) {
	Tk_FreeColor(outline->color);
        outline->color = NULL;
    }
    if (outline->activeColor != NULL) {
	Tk_FreeColor(outline->activeColor);
        outline->activeColor = NULL;
    }
    if (outline->disabledColor != NULL) {
	Tk_FreeColor(outline->disabledColor);
        outline->disabledColor = NULL;
    }
    if (outline->stipple != None) {
	Tk_FreeBitmap(display, outline->stipple);
        outline->stipple = None;
    }
    if (outline->activeStipple != None) {
	Tk_FreeBitmap(display, outline->activeStipple);
        outline->activeStipple = None;
    }
    if (outline->disabledStipple != None) {
	Tk_FreeBitmap(display, outline->disabledStipple);
        outline->disabledStipple = None;
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathConfigOutlineGC
 *
 *	This function should be called in the canvas object during the
 *	configure command. The graphics context description in gcValues is
 *	updated according to the information in the dash structure, as far as
 *	possible.
 *
 * Results:
 *	The return-value is a mask, indicating which elements of gcValues have
 *	been updated. 0 means there is no outline.
 *
 * Side effects:
 *	GC information in gcValues is updated.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathConfigOutlineGC(
    XGCValues *gcValues,
    Tk_PathCanvas canvas,
    Tk_PathItem *item,
    Tk_PathOutline *outline)
{
    int mask = 0;
    double width;
    Tk_Dash *dashPtr;
    XColor *color;
    Pixmap stipple;
    Tk_PathState state = item->state;

    if (outline->width < 0.0) {
	outline->width = 0.0;
    }
    if (outline->activeWidth < 0.0) {
	outline->activeWidth = 0.0;
    }
    if (outline->disabledWidth < 0) {
	outline->disabledWidth = 0.0;
    }
    if (state==TK_PATHSTATE_HIDDEN) {
	return 0;
    }

    width = outline->width;
    if (width < 1.0) {
	width = 1.0;
    }
    dashPtr = outline->dashPtr;
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (((TkPathCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth>width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDashPtr != NULL) {
	    dashPtr = outline->activeDashPtr;
	}
	if (outline->activeColor!=NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple!=None) {
	    stipple = outline->activeStipple;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (outline->disabledWidth>0) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDashPtr != NULL) {
	    dashPtr = outline->disabledDashPtr;
	}
	if (outline->disabledColor!=NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple!=None) {
	    stipple = outline->disabledStipple;
	}
    }

    if (color==NULL) {
	return 0;
    }

    gcValues->line_width = (int) (width + 0.5);
    if (color != NULL) {
	gcValues->foreground = color->pixel;
	mask = GCForeground|GCLineWidth;
	if (stipple != None) {
	    gcValues->stipple = stipple;
	    gcValues->fill_style = FillStippled;
	    mask |= GCStipple|GCFillStyle;
	}
    }
    if (mask && (dashPtr != NULL)) {
	gcValues->line_style = LineOnOffDash;
	gcValues->dash_offset = outline->offset;
	if (dashPtr->number >= 2) {
	    gcValues->dashes = 4;
	} else if (dashPtr->number > 0) {
	    gcValues->dashes = dashPtr->pattern.array[0];
	} else {
	    gcValues->dashes = (char) (4 * width);
	}
	mask |= GCLineStyle|GCDashList|GCDashOffset;
    }
    return mask;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathChangeOutlineGC
 *
 *	Updates the GC to represent the full information of the dash
 *	structure. Partly this is already done in Tk_PathConfigOutlineGC(). This
 *	function should be called just before drawing the dashed item.
 *
 * Results:
 *	1 if there is a stipple pattern, and 0 otherwise.
 *
 * Side effects:
 *	GC is updated.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathChangeOutlineGC(
    Tk_PathCanvas canvas,
    Tk_PathItem *item,
    Tk_PathOutline *outline)
{
    CONST char *p;
    double width;
    Tk_Dash *dashPtr;
    XColor *color;
    Pixmap stipple;
    Tk_PathState state = item->state;

    width = outline->width;
    if (width < 1.0) {
	width = 1.0;
    }
    dashPtr = outline->dashPtr;
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (((TkPathCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth > width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDashPtr != NULL) {
	    dashPtr = outline->activeDashPtr;
	}
	if (outline->activeColor != NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple != None) {
	    stipple = outline->activeStipple;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (outline->disabledWidth > width) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDashPtr != NULL) {
	    dashPtr = outline->disabledDashPtr;
	}
	if (outline->disabledColor != NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple != None) {
	    stipple = outline->disabledStipple;
	}
    }
    if (color == NULL) {
	return 0;
    }
    if (dashPtr != NULL) {
	if ((dashPtr->number <- 1) ||
		((dashPtr->number == -1) && (dashPtr->pattern.array[1] != ','))) {
	    char *q;
	    int i = -dashPtr->number;

	    p = (i > (int)sizeof(char *)) ? dashPtr->pattern.pt : dashPtr->pattern.array;
	    q = (char *) ckalloc(2*(unsigned int)i);
	    i = DashConvert(q, p, i, width);
	    XSetDashes(((TkPathCanvas *)canvas)->display, outline->gc,
		    outline->offset, q, i);
	    ckfree(q);
	} else if (dashPtr->number > 2 || (dashPtr->number == 2 &&
		(dashPtr->pattern.array[0] != dashPtr->pattern.array[1]))) {
	    p = (dashPtr->number > (int)sizeof(char *))
		    ? dashPtr->pattern.pt : dashPtr->pattern.array;
	    XSetDashes(((TkPathCanvas *)canvas)->display, outline->gc,
		    outline->offset, p, dashPtr->number);
	}
    }
    if (stipple != None) {
	int w=0; int h=0;
	Tk_TSOffset *tsoffset = outline->tsoffsetPtr;
	int flags = tsoffset->flags;
	if (!(flags & TK_OFFSET_INDEX) &&
		(flags & (TK_OFFSET_CENTER|TK_OFFSET_MIDDLE))) {
	    Tk_SizeOfBitmap(((TkPathCanvas *)canvas)->display, stipple, &w, &h);
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
	tsoffset->xoffset -= w;
	tsoffset->yoffset -= h;
	Tk_PathCanvasSetOffset(canvas, outline->gc, tsoffset);
	tsoffset->xoffset += w;
	tsoffset->yoffset += h;
	return 1;
    }
    return 0;
}


/*
 *--------------------------------------------------------------
 *
 * Tk_PathResetOutlineGC
 *
 *	Restores the GC to the situation before Tk_ChangeDashGC() was called.
 *	This function should be called just after the dashed item is drawn,
 *	because the GC is supposed to be read-only.
 *
 * Results:
 *	1 if there is a stipple pattern, and 0 otherwise.
 *
 * Side effects:
 *	GC is updated.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathResetOutlineGC(
    Tk_PathCanvas canvas,
    Tk_PathItem *item,
    Tk_PathOutline *outline)
{
    char dashList;
    double width;
    Tk_Dash *dashPtr;
    XColor *color;
    Pixmap stipple;
    Tk_PathState state = item->state;

    width = outline->width;
    if (width < 1.0) {
	width = 1.0;
    }
    dashPtr = outline->dashPtr;
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (((TkPathCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth > width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDashPtr != NULL) {
	    dashPtr = outline->activeDashPtr;
	}
	if (outline->activeColor != NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple != None) {
	    stipple = outline->activeStipple;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (outline->disabledWidth > width) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDashPtr != NULL) {
	    dashPtr = outline->disabledDashPtr;
	}
	if (outline->disabledColor != NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple != None) {
	    stipple = outline->disabledStipple;
	}
    }
    if (color == NULL) {
	return 0;
    }

    if (dashPtr != NULL) {
	if ((dashPtr->number > 2) || (dashPtr->number < -1) || (dashPtr->number == 2 &&
		(dashPtr->pattern.array[0] != dashPtr->pattern.array[1])) ||
		((dashPtr->number == -1) && (dashPtr->pattern.array[1] != ','))) {
	    if (dashPtr->number < 0) {
		dashList = (int) (4 * width + 0.5);
	    } else if (dashPtr->number < 3) {
		dashList = dashPtr->pattern.array[0];
	    } else {
		dashList = 4;
	    }
	    XSetDashes(((TkPathCanvas *)canvas)->display, outline->gc,
		    outline->offset, &dashList , 1);
	}
    }
    if (stipple != None) {
	XSetTSOrigin(((TkPathCanvas *)canvas)->display, outline->gc, 0, 0);
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasPsOutline
 *
 *	Creates the postscript command for the correct Outline-information
 *	(width, dash, color and stipple).
 *
 * Results:
 *	TCL_OK if succeeded, otherwise TCL_ERROR.
 *
 * Side effects:
 *	canvas->interp->result contains the postscript string, or an error
 *	message if the result was TCL_ERROR.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasPsOutline(
    Tk_PathCanvas canvas,
    Tk_PathItem *item,
    Tk_PathOutline *outline)
{
    char string[41];
    char pattern[11];
    int i;
    char *ptr;
    char *str = string;
    char *lptr = pattern;
    Tcl_Interp *interp = ((TkPathCanvas *)canvas)->interp;
    double width;
    Tk_Dash *dashPtr;
    XColor *color;
    Pixmap stipple;
    Tk_PathState state = item->state;

    width = outline->width;
    dashPtr = outline->dashPtr;
    color = outline->color;
    stipple = outline->stipple;
    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    if (((TkPathCanvas *)canvas)->currentItemPtr == item) {
	if (outline->activeWidth > width) {
	    width = outline->activeWidth;
	}
	if (outline->activeDashPtr != NULL) {
	    dashPtr = outline->activeDashPtr;
	}
	if (outline->activeColor != NULL) {
	    color = outline->activeColor;
	}
	if (outline->activeStipple != None) {
	    stipple = outline->activeStipple;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (outline->disabledWidth > 0) {
	    width = outline->disabledWidth;
	}
	if (outline->disabledDashPtr != NULL) {
	    dashPtr = outline->disabledDashPtr;
	}
	if (outline->disabledColor != NULL) {
	    color = outline->disabledColor;
	}
	if (outline->disabledStipple != None) {
	    stipple = outline->disabledStipple;
	}
    }
    sprintf(string, "%.15g setlinewidth\n", width);
    Tcl_AppendResult(interp, string, NULL);

    if (dashPtr != NULL) {
	if (dashPtr->number > 10) {
	    str = (char *)ckalloc((unsigned int) (1 + 4*dashPtr->number));
	} else if (dashPtr->number < -5) {
	    str = (char *)ckalloc((unsigned int) (1 - 8*dashPtr->number));
	    lptr = (char *)ckalloc((unsigned int) (1 - 2*dashPtr->number));
	}
	ptr = (ABS(dashPtr->number) > sizeof(char *)) ?
		dashPtr->pattern.pt : dashPtr->pattern.array;
	if (dashPtr->number > 0) {
	    char *ptr0 = ptr;

	    sprintf(str, "[%d", *ptr++ & 0xff);
	    i = dashPtr->number-1;
	    while (i--) {
		sprintf(str+strlen(str), " %d", *ptr++ & 0xff);
	    }
	    Tcl_AppendResult(interp, str, NULL);
	    if (dashPtr->number&1) {
		Tcl_AppendResult(interp, " ", str+1, NULL);
	    }
	    sprintf(str, "] %d setdash\n", outline->offset);
	    Tcl_AppendResult(interp, str, NULL);
	    ptr = ptr0;
	} else if (dashPtr->number < 0) {
	    if ((i = DashConvert(lptr, ptr, -dashPtr->number, width)) != 0) {
		char *lptr0 = lptr;

		sprintf(str, "[%d", *lptr++ & 0xff);
		while (--i) {
		    sprintf(str+strlen(str), " %d", *lptr++ & 0xff);
		}
		Tcl_AppendResult(interp, str, NULL);
		sprintf(str, "] %d setdash\n", outline->offset);
		Tcl_AppendResult(interp, str, NULL);
		lptr = lptr0;
	    }
	} else {
	    Tcl_AppendResult(interp, "[] 0 setdash\n", NULL);
	}
    } else {
	Tcl_AppendResult(interp, "[] 0 setdash\n", NULL);
    }

    if (str != string) {
	ckfree(str);
    }
    if (lptr != pattern) {
	ckfree(lptr);
    }
    if (Tk_PathCanvasPsColor(interp, canvas, color) != TCL_OK) {
	return TCL_ERROR;
    }
    if (stipple != None) {
	Tcl_AppendResult(interp, "StrokeClip ", NULL);
	if (Tk_PathCanvasPsStipple(interp, canvas, stipple) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "stroke\n", NULL);
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DashConvert
 *
 *	Converts a character-like dash-list (e.g. "-..") into an X11-style. l
 *	must point to a string that holds room to at least 2*n characters. If
 *	l == NULL, this function can be used for syntax checking only.
 *
 * Results:
 *	The length of the resulting X11 compatible dash-list. -1 if failed.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

static int
DashConvert(
    char *l,			/* Must be at least 2*n chars long, or NULL to
				 * indicate "just check syntax". */
    CONST char *p,		/* String to parse. */
    int n,			/* Length of string to parse, or -1 to
				 * indicate that strlen() should be used. */
    double width)		/* Width of line. */
{
    int result = 0;
    int size, intWidth;

    if (n<0) {
	n = (int) strlen(p);
    }
    intWidth = (int) (width + 0.5);
    if (intWidth < 1) {
	intWidth = 1;
    }
    while (n-- && *p) {
	switch (*p++) {
	case ' ':
	    if (result) {
		if (l) {
		    l[-1] += intWidth + 1;
		}
		continue;
	    }
	    return 0;
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
	if (l) {
	    *l++ = size * intWidth;
	    *l++ = 4 * intWidth;
	}
	result += 2;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TranslateAndAppendCoords --
 *
 *	This is a helper routine for TkPathCanvTranslatePath() below.
 *
 *	Given an (x,y) coordinate pair within a canvas, this function computes
 *	the corresponding coordinates at which the point should be drawn in
 *	the drawable used for display. Those coordinates are then written into
 *	outArr[numOut*2] and outArr[numOut*2+1].
 *
 * Results:
 *	There is no return value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
TranslateAndAppendCoords(
    TkPathCanvas *canvPtr,		/* The canvas. */
    double x,			/* Coordinates in canvas space. */
    double y,
    XPoint *outArr,		/* Write results into this array */
    int numOut)			/* Num of prior entries in outArr[] */
{
    double tmp;

    tmp = x - canvPtr->drawableXOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    outArr[numOut].x = (short) tmp;

    tmp = y - canvPtr->drawableYOrigin;
    if (tmp > 0) {
	tmp += 0.5;
    } else {
	tmp -= 0.5;
    }
    outArr[numOut].y = (short) tmp;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathCanvTranslatePath
 *
 *	Translate a line or polygon path so that all vertices are within a
 *	rectangle that is 1000 pixels larger than the total size of the canvas
 *	window. This will prevent pixel coordinates from overflowing the
 *	16-bit integer size limitation imposed by most windowing systems.
 *
 *	coordPtr must point to an array of doubles, two doubles per vertex.
 *	There are a total of numVertex vertices, or 2*numVertex entries in
 *	coordPtr. The result vertices written into outArr have their
 *	coordinate origin shifted to canvPtr->drawableXOrigin by
 *	canvPtr->drawableYOrigin. There might be as many as 3 times more
 *	output vertices than there are input vertices. The calling function
 *	should allocate space accordingly.
 *
 *	This routine limits the width and height of a canvas window to 31767
 *	pixels. At the highest resolution display devices available today (210
 *	ppi in Jan 2003) that's a window that is over 13 feet wide and tall.
 *	Should be enough for the near future.
 *
 * Results:
 *	Clipped and translated path vertices are written into outArr[]. There
 *	might be as many as twice the vertices in outArr[] as there are in
 *	coordPtr[]. The return value is the number of vertices actually
 *	written into outArr[].
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */

int
TkPathCanvTranslatePath(
    TkPathCanvas *canvPtr,		/* The canvas */
    int numVertex,		/* Number of vertices specified by
				 * coordArr[] */
    double *coordArr,		/* X and Y coordinates for each vertex */
    int closedPath,		/* True if this is a closed polygon */
    XPoint *outArr)		/* Write results here, if not NULL */
{
    int numOutput = 0;		/* Number of output coordinates */
    double lft, rgh;		/* Left and right sides of the bounding box */
    double top, btm;		/* Top and bottom sizes of the bounding box */
    double *tempArr;		/* Temporary storage used by the clipper */
    double *a, *b, *t;		/* Pointers to parts of the temporary
				 * storage */
    int i, j;			/* Loop counters */
    int maxOutput;		/* Maximum number of outputs that we will
				 * allow */
    double limit[4];		/* Boundries at which clipping occurs */
    double staticSpace[480];	/* Temp space from the stack */

    /*
     * Constrain all vertices of the path to be within a box that is no larger
     * than 32000 pixels wide or height. The top-left corner of this clipping
     * box is 1000 pixels above and to the left of the top left corner of the
     * window on which the canvas is displayed.
     *
     * This means that a canvas will not display properly on a canvas window
     * that is larger than 31000 pixels wide or high. That is not a problem
     * today, but might someday become a factor for ultra-high resolutions
     * displays.
     *
     * The X11 protocol allows us (in theory) to expand the size of the
     * clipping box to 32767 pixels. But we have found experimentally that
     * XFree86 sometimes fails to draw lines correctly if they are longer than
     * about 32500 pixels. So we have left a little margin in the size to mask
     * that bug.
     */

    lft = canvPtr->xOrigin - 1000.0;
    top = canvPtr->yOrigin - 1000.0;
    rgh = lft + 32000.0;
    btm = top + 32000.0;

    /*
     * Try the common case first - no clipping. Loop over the input
     * coordinates and translate them into appropriate output coordinates.
     * But if a vertex outside of the bounding box is seen, break out of the
     * loop.
     *
     * Most of the time, no clipping is needed, so this one loop is sufficient
     * to do the translation.
     */

    for (i=0; i<numVertex; i++){
	double x, y;

	x = coordArr[i*2];
	y = coordArr[i*2+1];
	if (x<lft || x>rgh || y<top || y>btm) {
	    break;
	}
	TranslateAndAppendCoords(canvPtr, x, y, outArr, numOutput++);
    }
    if (i == numVertex){
	assert(numOutput == numVertex);
	return numOutput;
    }

    /*
     * If we reach this point, it means that some clipping is required. Begin
     * by allocating some working storage - at least 6 times as much space as
     * coordArr[] requires. Divide this space into two separate arrays a[] and
     * b[]. Initialize a[] to be equal to coordArr[].
     */

    if (numVertex*12 <= (int)(sizeof(staticSpace)/sizeof(staticSpace[0]))) {
	tempArr = staticSpace;
    } else {
	tempArr = (double *)ckalloc(numVertex*12*sizeof(tempArr[0]));
    }
    for (i=0; i<numVertex*2; i++){
	tempArr[i] = coordArr[i];
    }
    a = tempArr;
    b = &tempArr[numVertex*6];

    /*
     * We will make four passes through the input data. On each pass, we copy
     * the contents of a[] over into b[]. As we copy, we clip any line
     * segments that extend to the right past xClip then we rotate the
     * coordinate system 90 degrees clockwise. After each pass is complete, we
     * interchange a[] and b[] in preparation for the next pass.
     *
     * Each pass clips line segments that extend beyond a single side of the
     * bounding box, and four passes rotate the coordinate system back to its
     * original value. I'm not an expert on graphics algorithms, but I think
     * this is called Cohen-Sutherland polygon clipping.
     *
     * The limit[] array contains the xClip value used for each of the four
     * passes.
     */

    limit[0] = rgh;
    limit[1] = -top;
    limit[2] = -lft;
    limit[3] = btm;

    /*
     * This is the loop that makes the four passes through the data.
     */

    maxOutput = numVertex*3;
    for (j=0; j<4; j++){
	double xClip = limit[j];
	int inside = a[0]<xClip;
	double priorY = a[1];
	numOutput = 0;

	/*
	 * Clip everything to the right of xClip. Store the results in b[]
	 * rotated by 90 degrees clockwise.
	 */

	for (i=0; i<numVertex; i++){
	    double x = a[i*2];
	    double y = a[i*2+1];

	    if (x >= xClip) {
		/*
		 * The current vertex is to the right of xClip.
		 */

		if (inside) {
		    /*
		     * If the current vertex is to the right of xClip but the
		     * previous vertex was left of xClip, then draw a line
		     * segment from the previous vertex to until it intersects
		     * the vertical at xClip.
		     */

		    double x0, y0, yN;

		    assert(i > 0);
		    x0 = a[i*2-2];
		    y0 = a[i*2-1];
		    yN = y0 + (y - y0)*(xClip-x0)/(x-x0);
		    b[numOutput*2] = -yN;
		    b[numOutput*2+1] = xClip;
		    numOutput++;
		    assert(numOutput <= maxOutput);
		    priorY = yN;
		    inside = 0;
		} else if (i == 0) {
		    /*
		     * If the first vertex is to the right of xClip, add a
		     * vertex that is the projection of the first vertex onto
		     * the vertical xClip line.
		     */

		    b[0] = -y;
		    b[1] = xClip;
		    numOutput = 1;
		    priorY = y;
		}
	    } else {
		/*
		 * The current vertex is to the left of xClip
		 */
		if (!inside) {
		    /* If the current vertex is on the left of xClip and one
		     * or more prior vertices where to the right, then we have
		     * to draw a line segment along xClip that extends from
		     * the spot where we first crossed from left to right to
		     * the spot where we cross back from right to left.
		     */

		    double x0, y0, yN;

		    assert(i > 0);
		    x0 = a[i*2-2];
		    y0 = a[i*2-1];
		    yN = y0 + (y - y0)*(xClip-x0)/(x-x0);
		    if (yN != priorY) {
			b[numOutput*2] = -yN;
			b[numOutput*2+1] = xClip;
			numOutput++;
			assert(numOutput <= maxOutput);
		    }
		    inside = 1;
		}
		b[numOutput*2] = -y;
		b[numOutput*2+1] = x;
		numOutput++;
		assert(numOutput <= maxOutput);
	    }
	}

	/*
	 * Interchange a[] and b[] in preparation for the next pass.
	 */

	t = a;
	a = b;
	b = t;
	numVertex = numOutput;
    }

    /*
     * All clipping is now finished. Convert the coordinates from doubles into
     * XPoints and translate the origin for the drawable.
     */

    for (i=0; i<numVertex; i++){
	TranslateAndAppendCoords(canvPtr, a[i*2], a[i*2+1], outArr, i);
    }
    if (tempArr != staticSpace) {
	ckfree((char *) tempArr);
    }
    return numOutput;
}


/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
