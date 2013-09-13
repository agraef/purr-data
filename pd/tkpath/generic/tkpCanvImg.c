/*
 * tkCanvImg.c --
 *
 *	This file implements image items for canvas widgets.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: tkpCanvImg.c,v 1.2 2008/06/21 14:58:42 matben Exp $
 */

#include <stdio.h>
#include "tkInt.h"
#include "tkIntPath.h"
#include "tkpCanvas.h"

/*
 * The structure below defines the record for each image item.
 */

typedef struct ImageItem  {
    Tk_PathItem header;		/* Generic stuff that's the same for all
				 * types. MUST BE FIRST IN STRUCTURE. */
    Tk_PathCanvas canvas;		/* Canvas containing the image. */
    double x, y;		/* Coordinates of positioning point for
				 * image. */
    Tk_Anchor anchor;		/* Where to anchor image relative to (x,y). */
    char *imageString;		/* String describing -image option
				 * (malloc-ed). NULL means no image right
				 * now. */
    char *activeImageString;	/* String describing -activeimage option.
				 * NULL means no image right now. */
    char *disabledImageString;	/* String describing -disabledimage option.
				 * NULL means no image right now. */
    Tk_Image image;		/* Image to display in window, or NULL if no
				 * image at present. */
    Tk_Image activeImage;	/* Image to display in window, or NULL if no
				 * image at present. */
    Tk_Image disabledImage;	/* Image to display in window, or NULL if no
				 * image at present. */
} ImageItem;

/*
 * Information used for parsing configuration specs. If you change any of the
 * default strings, be sure to change the corresponding default values in
 * CreateLine.
 */
 
#define PATH_DEF_STATE "normal"

/* These MUST be kept in sync with enums! X.h */

static char *stateStrings[] = {
    "active", "disabled", "normal", "hidden", NULL
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
    {TK_OPTION_STRING, "-activeimage", NULL, NULL,
	NULL, -1, Tk_Offset(ImageItem, activeImageString), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_ANCHOR, "-anchor", NULL, NULL,
	"center", -1, Tk_Offset(ImageItem, anchor), 0, 0, 0},
    {TK_OPTION_STRING, "-disabledimage", NULL, NULL,
	NULL, -1, Tk_Offset(ImageItem, disabledImageString), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-image", NULL, NULL,
	NULL, -1, Tk_Offset(ImageItem, imageString), 
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", NULL, NULL,
        PATH_DEF_STATE, -1, Tk_Offset(Tk_PathItem, state),
        0, (ClientData) stateStrings, 0},		
    {TK_OPTION_CUSTOM, "-tags", NULL, NULL,
	NULL, -1, Tk_Offset(Tk_PathItem, pathTagsPtr),
	TK_OPTION_NULL_OK, (ClientData) &tagsCO, 0},
    {TK_OPTION_END, NULL, NULL, NULL,           
	NULL, 0, -1, 0, (ClientData) NULL, 0}
};

static Tk_OptionTable optionTable = NULL;

/*
 * Prototypes for functions defined in this file:
 */

static void		ImageChangedProc(ClientData clientData,
			    int x, int y, int width, int height, int imgWidth,
			    int imgHeight);
static int		ImageCoords(Tcl_Interp *interp,
			    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int argc,
			    Tcl_Obj *CONST argv[]);
static int		ImageToArea(Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, double *rectPtr);
static double		ImageToPoint(Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, double *coordPtr);
static int		ImageToPostscript(Tcl_Interp *interp,
			    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
static void		ComputeImageBbox(Tk_PathCanvas canvas, ImageItem *imgPtr);
static int		ConfigureImage(Tcl_Interp *interp,
			    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int argc,
			    Tcl_Obj *CONST argv[], int flags);
static int		CreateImage(Tcl_Interp *interp,
			    Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
			    int argc, Tcl_Obj *CONST argv[]);
static void		DeleteImage(Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, Display *display);
static void		DisplayImage(Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, Display *display, Drawable dst,
			    int x, int y, int width, int height);
static void		ScaleImage(Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, double originX, double originY,
			    double scaleX, double scaleY);
static void		TranslateImage(Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr, double deltaX, double deltaY);

/*
 * The structures below defines the image item type in terms of functions that
 * can be invoked by generic item code.
 */

Tk_PathItemType tkImageType = {
    "image",			/* name */
    sizeof(ImageItem),		/* itemSize */
    CreateImage,		/* createProc */
    optionSpecs,		/* optionSpecs */
    ConfigureImage,		/* configureProc */
    ImageCoords,		/* coordProc */
    DeleteImage,		/* deleteProc */
    DisplayImage,		/* displayProc */
    0,				/* flags */
    NULL,			/* bboxProc */
    ImageToPoint,		/* pointProc */
    ImageToArea,		/* areaProc */
    ImageToPostscript,		/* postscriptProc */
    ScaleImage,			/* scaleProc */
    TranslateImage,		/* translateProc */
    NULL,			/* indexProc */
    NULL,			/* icursorProc */
    NULL,			/* selectionProc */
    NULL,			/* insertProc */
    NULL,			/* dTextProc */
    NULL,			/* nextPtr */
};

/*
 *--------------------------------------------------------------
 *
 * CreateImage --
 *
 *	This function is invoked to create a new image item in a canvas.
 *
 * Results:
 *	A standard Tcl return value. If an error occurred in creating the
 *	item, then an error message is left in the interp's result; in this
 *	case itemPtr is left uninitialized, so it can be safely freed by the
 *	caller.
 *
 * Side effects:
 *	A new image item is created.
 *
 *--------------------------------------------------------------
 */

static int
CreateImage(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_PathCanvas canvas,		/* Canvas to hold new item. */
    Tk_PathItem *itemPtr,		/* Record to hold new item; header has been
				 * initialized by caller. */
    int objc,			/* Number of arguments in objv. */
    Tcl_Obj *CONST objv[])	/* Arguments describing rectangle. */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;
    int i;

    if (objc == 0) {
	Tcl_Panic("canvas did not pass any coords\n");
    }

    /*
     * Initialize item's record.
     */

    imgPtr->canvas = canvas;
    imgPtr->anchor = TK_ANCHOR_CENTER;
    imgPtr->imageString = NULL;
    imgPtr->activeImageString = NULL;
    imgPtr->disabledImageString = NULL;
    imgPtr->image = NULL;
    imgPtr->activeImage = NULL;
    imgPtr->disabledImage = NULL;

    if (optionTable == NULL) {
	optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    } 
    itemPtr->optionTable = optionTable;
    if (Tk_InitOptions(interp, (char *) imgPtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas)) != TCL_OK) {
        goto error;
    }

    /*
     * Process the arguments to fill in the item record. Only 1 (list) or 2 (x
     * y) coords are allowed.
     */

    if (objc == 1) {
	i = 1;
    } else {
	char *arg = Tcl_GetString(objv[1]);
	i = 2;
	if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    i = 1;
	}
    }
    if ((ImageCoords(interp, canvas, itemPtr, i, objv) != TCL_OK)) {
	goto error;
    }
    if (ConfigureImage(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
	return TCL_OK;
    }

  error:
    DeleteImage(canvas, itemPtr, Tk_Display(Tk_PathCanvasTkwin(canvas)));
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * ImageCoords --
 *
 *	This function is invoked to process the "coords" widget command on
 *	image items. See the user documentation for details on what it does.
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
ImageCoords(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_PathCanvas canvas,		/* Canvas containing item. */
    Tk_PathItem *itemPtr,		/* Item whose coordinates are to be read or
				 * modified. */
    int objc,			/* Number of coordinates supplied in objv. */
    Tcl_Obj *CONST objv[])	/* Array of coordinates: x1, y1, x2, y2, ... */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;

    if (objc == 0) {
	Tcl_Obj *obj = Tcl_NewObj();

	Tcl_Obj *subobj = Tcl_NewDoubleObj(imgPtr->x);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	subobj = Tcl_NewDoubleObj(imgPtr->y);
	Tcl_ListObjAppendElement(interp, obj, subobj);
	Tcl_SetObjResult(interp, obj);
    } else if (objc < 3) {
	if (objc==1) {
	    if (Tcl_ListObjGetElements(interp, objv[0], &objc,
		    (Tcl_Obj ***) &objv) != TCL_OK) {
		return TCL_ERROR;
	    } else if (objc != 2) {
		char buf[64];

		sprintf(buf, "wrong # coordinates: expected 2, got %d", objc);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
		return TCL_ERROR;
	    }
	}
	if ((Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[0], &imgPtr->x) != TCL_OK)
		|| (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[1],
  		    &imgPtr->y) != TCL_OK)) {
	    return TCL_ERROR;
	}
	ComputeImageBbox(canvas, imgPtr);
    } else {
	char buf[64];

	sprintf(buf, "wrong # coordinates: expected 0 or 2, got %d", objc);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ConfigureImage --
 *
 *	This function is invoked to configure various aspects of an image
 *	item, such as its anchor position.
 *
 * Results:
 *	A standard Tcl result code. If an error occurs, then an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	Configuration information may be set for itemPtr.
 *
 *--------------------------------------------------------------
 */

static int
ConfigureImage(
    Tcl_Interp *interp,		/* Used for error reporting. */
    Tk_PathCanvas canvas,	/* Canvas containing itemPtr. */
    Tk_PathItem *itemPtr,	/* Image item to reconfigure. */
    int objc,			/* Number of elements in objv.  */
    Tcl_Obj *CONST objv[],	/* Arguments describing things to configure. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    Tk_Window tkwin;
    Tk_Image image;
    int error;

    tkwin = Tk_PathCanvasTkwin(canvas);
    
    /*
     * The following loop is potentially executed twice. During the first pass
     * configuration options get set to their new values. If there is an error
     * in this pass, we execute a second pass to restore all the options to
     * their previous values.
     */

    for (error = 0; error <= 1; error++) {
	if (!error) {
	    /*
	     * First pass: set options to new values.
	     */

	    if (Tk_SetOptions(interp, (char *) imgPtr, 
		    optionTable, objc, objv, tkwin, 
		    &savedOptions, NULL) != TCL_OK) {
		continue;
	    }
	} else {
	    /*
	     * Second pass: restore options to old values.
	     */

	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}
	
	/*
	 * Create the image. Save the old image around and don't free it until
	 * after the new one is allocated. This keeps the reference count from
	 * going to zero so the image doesn't have to be recreated if it hasn't
	 * changed.
	 */
	
	if (imgPtr->activeImageString != NULL) {
	    itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
	} else {
	    itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
	}
	
	/* image */
	if (imgPtr->imageString != NULL) {
	    image = Tk_GetImage(interp, tkwin, imgPtr->imageString,
		    ImageChangedProc, (ClientData) imgPtr);
	    if (image == NULL) {
		continue;
	    }
	} else {
	    image = NULL;
	}
	if (imgPtr->image != NULL) {
	    Tk_FreeImage(imgPtr->image);
	}
	imgPtr->image = image;
	
	/* active image */
	if (imgPtr->activeImageString != NULL) {
	    image = Tk_GetImage(interp, tkwin, imgPtr->activeImageString,
		    ImageChangedProc, (ClientData) imgPtr);
	    if (image == NULL) {
		continue;
	    }
	} else {
	    image = NULL;
	}
	if (imgPtr->activeImage != NULL) {
	    Tk_FreeImage(imgPtr->activeImage);
	}
	imgPtr->activeImage = image;
	
	/* disabled image */
	if (imgPtr->disabledImageString != NULL) {
	    image = Tk_GetImage(interp, tkwin, imgPtr->disabledImageString,
		    ImageChangedProc, (ClientData) imgPtr);
	    if (image == NULL) {
		continue;
	    }
	} else {
	    image = NULL;
	}
	if (imgPtr->disabledImage != NULL) {
	    Tk_FreeImage(imgPtr->disabledImage);
	}
	imgPtr->disabledImage = image;
	break;
    }
    if (!error) {
	Tk_FreeSavedOptions(&savedOptions);
    }
    ComputeImageBbox(canvas, imgPtr);
    if (error) {
	Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    } else {
	return TCL_OK;
    }
}

/*
 *--------------------------------------------------------------
 *
 * DeleteImage --
 *
 *	This function is called to clean up the data structure associated with
 *	a image item.
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
DeleteImage(
    Tk_PathCanvas canvas,		/* Info about overall canvas widget. */
    Tk_PathItem *itemPtr,		/* Item that is being deleted. */
    Display *display)		/* Display containing window for canvas. */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;

    if (imgPtr->image != NULL) {
	Tk_FreeImage(imgPtr->image);
    }
    if (imgPtr->activeImage != NULL) {
	Tk_FreeImage(imgPtr->activeImage);
    }
    if (imgPtr->disabledImage != NULL) {
	Tk_FreeImage(imgPtr->disabledImage);
    }
    Tk_FreeConfigOptions((char *) imgPtr, optionTable, Tk_PathCanvasTkwin(canvas));
}

/*
 *--------------------------------------------------------------
 *
 * ComputeImageBbox --
 *
 *	This function is invoked to compute the bounding box of all the pixels
 *	that may be drawn as part of a image item. This function is where the
 *	child image's placement is computed.
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
ComputeImageBbox(
    Tk_PathCanvas canvas,		/* Canvas that contains item. */
    ImageItem *imgPtr)		/* Item whose bbox is to be recomputed. */
{
    int width, height;
    int x, y;
    Tk_Image image;
    Tk_PathState state = imgPtr->header.state;

    if(state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    image = imgPtr->image;
    if (((TkPathCanvas *)canvas)->currentItemPtr == (Tk_PathItem *)imgPtr) {
	if (imgPtr->activeImage != NULL) {
	    image = imgPtr->activeImage;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (imgPtr->disabledImage != NULL) {
	    image = imgPtr->disabledImage;
	}
    }

    x = (int) (imgPtr->x + ((imgPtr->x >= 0) ? 0.5 : - 0.5));
    y = (int) (imgPtr->y + ((imgPtr->y >= 0) ? 0.5 : - 0.5));

    if ((state == TK_PATHSTATE_HIDDEN) || (image == None)) {
	imgPtr->header.x1 = imgPtr->header.x2 = x;
	imgPtr->header.y1 = imgPtr->header.y2 = y;
	return;
    }

    /*
     * Compute location and size of image, using anchor information.
     */

    Tk_SizeOfImage(image, &width, &height);
    switch (imgPtr->anchor) {
    case TK_ANCHOR_N:
	x -= width/2;
	break;
    case TK_ANCHOR_NE:
	x -= width;
	break;
    case TK_ANCHOR_E:
	x -= width;
	y -= height/2;
	break;
    case TK_ANCHOR_SE:
	x -= width;
	y -= height;
	break;
    case TK_ANCHOR_S:
	x -= width/2;
	y -= height;
	break;
    case TK_ANCHOR_SW:
	y -= height;
	break;
    case TK_ANCHOR_W:
	y -= height/2;
	break;
    case TK_ANCHOR_NW:
	break;
    case TK_ANCHOR_CENTER:
	x -= width/2;
	y -= height/2;
	break;
    }

    /*
     * Store the information in the item header.
     */

    imgPtr->header.x1 = x;
    imgPtr->header.y1 = y;
    imgPtr->header.x2 = x + width;
    imgPtr->header.y2 = y + height;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayImage --
 *
 *	This function is invoked to draw a image item in a given drawable.
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
DisplayImage(
    Tk_PathCanvas canvas,		/* Canvas that contains item. */
    Tk_PathItem *itemPtr,		/* Item to be displayed. */
    Display *display,		/* Display on which to draw item. */
    Drawable drawable,		/* Pixmap or window in which to draw item. */
    int x, int y, int width, int height)
				/* Describes region of canvas that must be
				 * redisplayed (not used). */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;
    short drawableX, drawableY;
    Tk_Image image;
    Tk_PathState state = itemPtr->state;

    if (state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    image = imgPtr->image;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (imgPtr->activeImage != NULL) {
	    image = imgPtr->activeImage;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (imgPtr->disabledImage != NULL) {
	    image = imgPtr->disabledImage;
	}
    }

    if (image == NULL) {
	return;
    }

    /*
     * Translate the coordinates to those of the image, then redisplay it.
     */

    Tk_PathCanvasDrawableCoords(canvas, (double) x, (double) y,
	    &drawableX, &drawableY);
    Tk_RedrawImage(image, x - imgPtr->header.x1, y - imgPtr->header.y1,
	    width, height, drawable, drawableX, drawableY);
}

/*
 *--------------------------------------------------------------
 *
 * ImageToPoint --
 *
 *	Computes the distance from a given point to a given rectangle, in
 *	canvas units.
 *
 * Results:
 *	The return value is 0 if the point whose x and y coordinates are
 *	coordPtr[0] and coordPtr[1] is inside the image. If the point isn't
 *	inside the image then the return value is the distance from the point
 *	to the image.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static double
ImageToPoint(
    Tk_PathCanvas canvas,		/* Canvas containing item. */
    Tk_PathItem *itemPtr,		/* Item to check against point. */
    double *coordPtr)		/* Pointer to x and y coordinates. */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;
    double x1, x2, y1, y2, xDiff, yDiff;

    x1 = imgPtr->header.x1;
    y1 = imgPtr->header.y1;
    x2 = imgPtr->header.x2;
    y2 = imgPtr->header.y2;

    /*
     * Point is outside rectangle.
     */

    if (coordPtr[0] < x1) {
	xDiff = x1 - coordPtr[0];
    } else if (coordPtr[0] > x2)  {
	xDiff = coordPtr[0] - x2;
    } else {
	xDiff = 0;
    }

    if (coordPtr[1] < y1) {
	yDiff = y1 - coordPtr[1];
    } else if (coordPtr[1] > y2)  {
	yDiff = coordPtr[1] - y2;
    } else {
	yDiff = 0;
    }

    return hypot(xDiff, yDiff);
}

/*
 *--------------------------------------------------------------
 *
 * ImageToArea --
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

static int
ImageToArea(
    Tk_PathCanvas canvas,		/* Canvas containing item. */
    Tk_PathItem *itemPtr,		/* Item to check against rectangle. */
    double *rectPtr)		/* Pointer to array of four coordinates
				 * (x1,y1,x2,y2) describing rectangular
				 * area. */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;

    if ((rectPtr[2] <= imgPtr->header.x1)
	    || (rectPtr[0] >= imgPtr->header.x2)
	    || (rectPtr[3] <= imgPtr->header.y1)
	    || (rectPtr[1] >= imgPtr->header.y2)) {
	return -1;
    }
    if ((rectPtr[0] <= imgPtr->header.x1)
	    && (rectPtr[1] <= imgPtr->header.y1)
	    && (rectPtr[2] >= imgPtr->header.x2)
	    && (rectPtr[3] >= imgPtr->header.y2)) {
	return 1;
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * ImageToPostscript --
 *
 *	This function is called to generate Postscript for image items.
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs in
 *	generating Postscript then an error message is left in interp->result,
 *	replacing whatever used to be there. If no error occurs, then
 *	Postscript for the item is appended to the result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
ImageToPostscript(
    Tcl_Interp *interp,		/* Leave Postscript or error message here. */
    Tk_PathCanvas canvas,		/* Information about overall canvas. */
    Tk_PathItem *itemPtr,		/* Item for which Postscript is wanted. */
    int prepass)		/* 1 means this is a prepass to collect font
				 * information; 0 means final Postscript is
				 * being created.*/
{
    ImageItem *imgPtr = (ImageItem *)itemPtr;
    Tk_Window canvasWin = Tk_PathCanvasTkwin(canvas);

    char buffer[256];
    double x, y;
    int width, height;
    Tk_Image image;
    Tk_PathState state = itemPtr->state;

    if(state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }

    image = imgPtr->image;
    if (((TkPathCanvas *)canvas)->currentItemPtr == itemPtr) {
	if (imgPtr->activeImage != NULL) {
	    image = imgPtr->activeImage;
	}
    } else if (state == TK_PATHSTATE_DISABLED) {
	if (imgPtr->disabledImage != NULL) {
	    image = imgPtr->disabledImage;
	}
    }
    if (image == NULL) {
	/*
	 * Image item without actual image specified.
	 */

        return TCL_OK;
    }
    Tk_SizeOfImage(image, &width, &height);

    /*
     * Compute the coordinates of the lower-left corner of the image, taking
     * into account the anchor position for the image.
     */

    x = imgPtr->x;
    y = Tk_PathCanvasPsY(canvas, imgPtr->y);

    switch (imgPtr->anchor) {
    case TK_ANCHOR_NW:			   y -= height;		break;
    case TK_ANCHOR_N:	   x -= width/2.0; y -= height;		break;
    case TK_ANCHOR_NE:	   x -= width;	   y -= height;		break;
    case TK_ANCHOR_E:	   x -= width;	   y -= height/2.0;	break;
    case TK_ANCHOR_SE:	   x -= width;				break;
    case TK_ANCHOR_S:	   x -= width/2.0;			break;
    case TK_ANCHOR_SW:						break;
    case TK_ANCHOR_W:			   y -= height/2.0;	break;
    case TK_ANCHOR_CENTER: x -= width/2.0; y -= height/2.0;	break;
    }

    if (!prepass) {
	sprintf(buffer, "%.15g %.15g", x, y);
	Tcl_AppendResult(interp, buffer, " translate\n", NULL);
    }

    return Tk_PostscriptImage(image, interp, canvasWin,
	    ((TkPathCanvas *) canvas)->psInfo, 0, 0, width, height, prepass);
}

/*
 *--------------------------------------------------------------
 *
 * ScaleImage --
 *
 *	This function is invoked to rescale an item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The item referred to by itemPtr is rescaled so that the following
 *	transformation is applied to all point coordinates:
 *		x' = originX + scaleX*(x-originX)
 *		y' = originY + scaleY*(y-originY)
 *
 *--------------------------------------------------------------
 */

static void
ScaleImage(
    Tk_PathCanvas canvas,		/* Canvas containing rectangle. */
    Tk_PathItem *itemPtr,		/* Rectangle to be scaled. */
    double originX, double originY,
				/* Origin about which to scale rect. */
    double scaleX,		/* Amount to scale in X direction. */
    double scaleY)		/* Amount to scale in Y direction. */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;

    imgPtr->x = originX + scaleX*(imgPtr->x - originX);
    imgPtr->y = originY + scaleY*(imgPtr->y - originY);
    ComputeImageBbox(canvas, imgPtr);
}

/*
 *--------------------------------------------------------------
 *
 * TranslateImage --
 *
 *	This function is called to move an item by a given amount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The position of the item is offset by (xDelta, yDelta), and the
 *	bounding box is updated in the generic part of the item structure.
 *
 *--------------------------------------------------------------
 */

static void
TranslateImage(
    Tk_PathCanvas canvas,		/* Canvas containing item. */
    Tk_PathItem *itemPtr,		/* Item that is being moved. */
    double deltaX, double deltaY)
				/* Amount by which item is to be moved. */
{
    ImageItem *imgPtr = (ImageItem *) itemPtr;

    imgPtr->x += deltaX;
    imgPtr->y += deltaY;
    ComputeImageBbox(canvas, imgPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImageChangedProc --
 *
 *	This function is invoked by the image code whenever the manager for an
 *	image does something that affects the image's size or how it is
 *	displayed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the canvas to get redisplayed.
 *
 *----------------------------------------------------------------------
 */

static void
ImageChangedProc(
    ClientData clientData,	/* Pointer to canvas item for image. */
    int x, int y,		/* Upper left pixel (within image) that must
				 * be redisplayed. */
    int width, int height,	/* Dimensions of area to redisplay (may be <=
				 * 0). */
    int imgWidth, int imgHeight)/* New dimensions of image. */
{
    ImageItem *imgPtr = (ImageItem *) clientData;

    /*
     * If the image's size changed and it's not anchored at its northwest
     * corner then just redisplay the entire area of the image. This is a bit
     * over-conservative, but we need to do something because a size change
     * also means a position change.
     */

    if (((imgPtr->header.x2 - imgPtr->header.x1) != imgWidth)
	    || ((imgPtr->header.y2 - imgPtr->header.y1) != imgHeight)) {
	x = y = 0;
	width = imgWidth;
	height = imgHeight;
	Tk_PathCanvasEventuallyRedraw(imgPtr->canvas, imgPtr->header.x1,
		imgPtr->header.y1, imgPtr->header.x2, imgPtr->header.y2);
    }
    ComputeImageBbox(imgPtr->canvas, imgPtr);
    Tk_PathCanvasEventuallyRedraw(imgPtr->canvas, imgPtr->header.x1 + x,
	    imgPtr->header.y1 + y, (int) (imgPtr->header.x1 + x + width),
	    (int) (imgPtr->header.y1 + y + height));
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
