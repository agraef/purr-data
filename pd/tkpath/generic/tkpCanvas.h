/*
 * tkpCanvas.h --
 *
 *	Declarations shared among all the files that implement canvas widgets.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * Copyright (c) 1998 by Scriptics Corporation.
 * Copyright (c) 2008 Mats Bengtsson
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#ifndef _TKPCANVAS
#define _TKPCANVAS

#ifndef _TK
#include "tk.h"
#endif
#include "tkp.h"
#include "tkPath.h"

#ifndef USE_OLD_TAG_SEARCH
typedef struct TagSearchExpr_s TagSearchExpr;

struct TagSearchExpr_s {
    TagSearchExpr *next;	/* For linked lists of expressions - used in
				 * bindings. */
    Tk_Uid uid;			/* The uid of the whole expression. */
    Tk_Uid *uids;		/* Expresion compiled to an array of uids. */
    int allocated;		/* Available space for array of uids. */
    int length;			/* Length of expression. */
    int index;			/* Current position in expression
				 * evaluation. */
    int match;			/* This expression matches event's item's
				 * tags. */
};
#endif /* not USE_OLD_TAG_SEARCH */

/*
 * The record below describes a canvas widget. It is made available to the
 * item functions so they can access certain shared fields such as the overall
 * displacement and scale factor for the canvas.
 */

typedef struct TkPathCanvas {
    Tk_Window tkwin;		/* Window that embodies the canvas. NULL means
				 * that the window has been destroyed but the
				 * data structures haven't yet been cleaned
				 * up.*/
    Display *display;		/* Display containing widget; needed, among
				 * other things, to release resources after
				 * tkwin has already gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with canvas. */
    Tcl_Command widgetCmd;	/* Token for canvas's widget command. */
    Tk_OptionTable optionTable;	/* Table that defines configuration options
				 * available for this widget. */
    Tk_PathItem *rootItemPtr;	/* The root item with id 0, always there. */

    /*
     * Information used when displaying widget:
     */

    Tcl_Obj *borderWidthPtr;	/* Value of -borderWidth option: specifies
				 * width of border in pixels. */
    int borderWidth;		/* Width of 3-D border around window. *
				 * Integer value corresponding to
				 * borderWidthPtr. Always >= 0. */
    Tk_3DBorder bgBorder;	/* Used for canvas background. */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    Tcl_Obj *highlightWidthPtr;	/* Value of -highlightthickness option:
				 * specifies width in pixels of highlight to
				 * draw around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
    int highlightWidth;		/* Integer value corresponding to
				 * highlightWidthPtr. Always >= 0. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight area
				 * when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int inset;			/* Total width of all borders, including
				 * traversal highlight and 3-D border.
				 * Indicates how much interior stuff must be
				 * offset from outside edges to leave room for
				 * borders. */
    GC pixmapGC;		/* Used to copy bits from a pixmap to the
				 * screen and also to clear the pixmap. */
    int width, height;		/* Dimensions to request for canvas window,
				 * specified in pixels. */
    int redrawX1, redrawY1;	/* Upper left corner of area to redraw, in
				 * pixel coordinates. Border pixels are
				 * included. Only valid if REDRAW_PENDING flag
				 * is set. */
    int redrawX2, redrawY2;	/* Lower right corner of area to redraw, in
				 * integer canvas coordinates. Border pixels
				 * will *not* be redrawn. */
    int confine;		/* Non-zero means constrain view to keep as
				 * much of canvas visible as possible. */

    /*
     * Information used to manage the selection and insertion cursor:
     */

    Tk_PathCanvasTextInfo textInfo; /* Contains lots of fields; see tk.h for
				 * details. This structure is shared with the
				 * code that implements individual items. */
    int insertOnTime;		/* Number of milliseconds cursor should spend
				 * in "on" state for each blink. */
    int insertOffTime;		/* Number of milliseconds cursor should spend
				 * in "off" state for each blink. */
    Tcl_TimerToken insertBlinkHandler;
				/* Timer handler used to blink cursor on and
				 * off. */

    /*
     * Transformation applied to canvas as a whole: to compute screen
     * coordinates (X,Y) from canvas coordinates (x,y), do the following:
     *
     * X = x - xOrigin;
     * Y = y - yOrigin;
     */

    int xOrigin, yOrigin;	/* Canvas coordinates corresponding to
				 * upper-left corner of window, given in
				 * canvas pixel units. */
    int drawableXOrigin, drawableYOrigin;
				/* During redisplay, these fields give the
				 * canvas coordinates corresponding to the
				 * upper-left corner of the drawable where
				 * items are actually being drawn (typically a
				 * pixmap smaller than the whole window). */

    /*
     * Information used for event bindings associated with items.
     */

    Tk_BindingTable bindingTable;
				/* Table of all bindings currently defined for
				 * this canvas. NULL means that no bindings
				 * exist, so the table hasn't been created.
				 * Each "object" used for this table is either
				 * a Tk_Uid for a tag or the address of an
				 * item named by id. */
    Tk_PathItem *currentItemPtr;	/* The item currently containing the mouse
				 * pointer, or NULL if none. */
    Tk_PathItem *newCurrentPtr;	/* The item that is about to become the
				 * current one, or NULL. This field is used to
				 * detect deletions of the new current item
				 * pointer that occur during Leave processing
				 * of the previous current item. */
    double closeEnough;		/* The mouse is assumed to be inside an item
				 * if it is this close to it. */
    XEvent pickEvent;		/* The event upon which the current choice of
				 * currentItem is based. Must be saved so that
				 * if the currentItem is deleted, can pick
				 * another. */
    int state;			/* Last known modifier state. Used to defer
				 * picking a new current object while buttons
				 * are down. */

    /*
     * Information used for managing scrollbars:
     */

    char *xScrollCmd;		/* Command prefix for communicating with
				 * horizontal scrollbar. NULL means no
				 * horizontal scrollbar. Malloc'ed. */
    char *yScrollCmd;		/* Command prefix for communicating with
				 * vertical scrollbar. NULL means no vertical
				 * scrollbar. Malloc'ed. */
    int scrollX1, scrollY1, scrollX2, scrollY2;
				/* These four coordinates define the region
				 * that is the 100% area for scrolling (i.e.
				 * these numbers determine the size and
				 * location of the sliders on scrollbars).
				 * Units are pixels in canvas coords. */
    char *regionString;		/* The option string from which scrollX1 etc.
				 * are derived. Malloc'ed. */
    int xScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling. This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */
    int yScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling. This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */

    /*
     * Information used for scanning:
     */

    int scanX;			/* X-position at which scan started (e.g.
				 * button was pressed here). */
    int scanXOrigin;		/* Value of xOrigin field when scan started. */
    int scanY;			/* Y-position at which scan started (e.g.
				 * button was pressed here). */
    int scanYOrigin;		/* Value of yOrigin field when scan started. */

    /*
     * Information used to speed up searches by remembering the last item
     * created or found with an item id search.
     */

    Tk_PathItem *hotPtr;	/* Pointer to "hot" item (one that's been
				 * recently used. NULL means there's no hot
				 * item. */
    Tk_PathItem *hotPrevPtr;	/* Pointer to predecessor to hotPtr (NULL
				 * means item is first in list). This is only
				 * a hint and may not really be hotPtr's
				 * predecessor. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    char *takeFocus;		/* Value of -takefocus option; not used in the
				 * C code, but used by keyboard traversal
				 * scripts. Malloc'ed, but may be NULL. */
    double pixelsPerMM;		/* Scale factor between MM and pixels; used
				 * when converting coordinates. */
    int flags;			/* Various flags; see below for
				 * definitions. */
    int nextId;			/* Number to use as id for next item created
				 * in widget. */
    Tk_PostscriptInfo psInfo;	/* Pointer to information used for generating
				 * Postscript for the canvas. NULL means no
				 * Postscript is currently being generated. */
    Tcl_HashTable idTable;	/* Table of integer indices. */
// @@@ TODO: as pointers instead???
    Tcl_HashTable styleTable;	/* Table for styles.
				 * This defines the namespace for style names. */
    Tcl_HashTable gradientTable;/* Table for gradients. 
				 * This defines the namespace for gradient names. */
    int styleUid;		/* Running integer used to number style tokens. */
    int gradientUid;		/* Running integer used to number gradient tokens. */
    int tagStyle;
    
    /*
     * Additional information, added by the 'dash'-patch
     */

    void *reserved1;
    Tk_PathState canvas_state;	/* State of canvas. */
    void *reserved2;
    void *reserved3;
    Tk_TSOffset *tsoffsetPtr;
#ifndef USE_OLD_TAG_SEARCH
    TagSearchExpr *bindTagExprs;/* Linked list of tag expressions used in
				 * bindings. */
#endif
} TkPathCanvas;

/*
 * Flag bits for canvases:
 *
 * REDRAW_PENDING -		1 means a DoWhenIdle handler has already been
 *				created to redraw some or all of the canvas.
 * REDRAW_BORDERS - 		1 means that the borders need to be redrawn
 *				during the next redisplay operation.
 * REPICK_NEEDED -		1 means DisplayCanvas should pick a new
 *				current item before redrawing the canvas.
 * GOT_FOCUS -			1 means the focus is currently in this widget,
 *				so should draw the insertion cursor and
 *				traversal highlight.
 * CURSOR_ON -			1 means the insertion cursor is in the "on"
 *				phase of its blink cycle. 0 means either we
 *				don't have the focus or the cursor is in the
 *				"off" phase of its cycle.
 * UPDATE_SCROLLBARS -		1 means the scrollbars should get updated as
 *				part of the next display operation.
 * LEFT_GRABBED_ITEM -		1 means that the mouse left the current item
 *				while a grab was in effect, so we didn't
 *				change canvasPtr->currentItemPtr.
 * REPICK_IN_PROGRESS -		1 means PickCurrentItem is currently
 *				executing. If it should be called recursively,
 *				it should simply return immediately.
 * BBOX_NOT_EMPTY -		1 means that the bounding box of the area that
 *				should be redrawn is not empty.
 * CANVAS_DELETED -
 */

#define REDRAW_PENDING		(1 << 0)
#define REDRAW_BORDERS		(1 << 1)
#define REPICK_NEEDED		(1 << 2)
#define GOT_FOCUS		(1 << 3)
#define CURSOR_ON		(1 << 4)
#define UPDATE_SCROLLBARS	(1 << 5)
#define LEFT_GRABBED_ITEM	(1 << 6)
#define REPICK_IN_PROGRESS	(1 << 7)
#define BBOX_NOT_EMPTY		(1 << 8)
#define CANVAS_DELETED		(1 << 9)

/*
 * Flag bits for canvas items (redraw_flags):
 *
 * FORCE_REDRAW -		1 means that the new coordinates of some item
 *				are not yet registered using
 *				Tk_PathCanvasEventuallyRedraw(). It should still
 *				be done by the general canvas code.
 */

#define FORCE_REDRAW		8

/*
 * This is an extended item record that is used for the new
 * path based items to allow more generic code to be used for them
 * since all of them (?) anyhow include a Tk_PathStyle record.
 */
 
typedef struct Tk_PathItemEx  {
    Tk_PathItem header;	    /* Generic stuff that's the same for all
                             * types.  MUST BE FIRST IN STRUCTURE. */
    Tk_PathCanvas canvas;   /* Canvas containing item. */
    Tk_PathStyle style;	    /* Contains most drawing info. */
    Tcl_Obj *styleObj;	    /* Object with style name. */
    TkPathStyleInst *styleInst;
			    /* The referenced style instance from styleObj. */

    /*
     *------------------------------------------------------------------
     * Starting here is additional type-specific stuff; see the declarations
     * for individual types to see what is part of each type. The actual space
     * below is determined by the "itemInfoSize" of the type's Tk_PathItemType
     * record.
     *------------------------------------------------------------------
     */
} Tk_PathItemEx;

/*
 * Canvas-related functions that are shared among Tk modules but not exported
 * to the outside world:
 */

MODULE_SCOPE int	    TkCanvPostscriptCmd(TkPathCanvas *canvasPtr,
				Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int	    TkPathCanvTranslatePath(TkPathCanvas *canvPtr,
				int numVertex, double *coordPtr, int closed,
				XPoint *outPtr);
MODULE_SCOPE Tk_PathTags *  TkPathAllocTagsFromObj(Tcl_Interp *interp, Tcl_Obj *valuePtr);
MODULE_SCOPE int	    TkPathCanvasFindGroup(Tcl_Interp *interp, Tk_PathCanvas canvas, 
				Tcl_Obj *parentObj, Tk_PathItem **parentPtrPtr);
MODULE_SCOPE void	    TkPathCanvasSetParent(Tk_PathItem *parentPtr, Tk_PathItem *itemPtr);
MODULE_SCOPE int	    TkPathCanvasGetDepth(Tk_PathItem *itemPtr);
MODULE_SCOPE Tk_PathStyle   TkPathCanvasInheritStyle(Tk_PathItem *itemPtr, long flags);
MODULE_SCOPE TMatrix	    TkPathCanvasInheritTMatrix(Tk_PathItem *itemPtr);
MODULE_SCOPE void	    TkPathCanvasFreeInheritedStyle(Tk_PathStyle *stylePtr);
MODULE_SCOPE Tcl_HashTable *TkPathCanvasGradientTable(Tk_PathCanvas canvas);
MODULE_SCOPE Tcl_HashTable *TkPathCanvasStyleTable(Tk_PathCanvas canvas);
MODULE_SCOPE Tk_PathState   TkPathCanvasState(Tk_PathCanvas canvas);
MODULE_SCOPE Tk_PathItem *  TkPathCanvasCurrentItem(Tk_PathCanvas canvas);
MODULE_SCOPE void	    TkPathCanvasGroupBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
				int *x1Ptr, int *y1Ptr, int *x2Ptr, int *y2Ptr);
MODULE_SCOPE void	    TkPathCanvasUpdateGroupBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr);
MODULE_SCOPE void	    TkPathCanvasSetGroupDirtyBbox(Tk_PathItem *itemPtr);
MODULE_SCOPE Tk_PathItem *  TkPathCanvasItemIteratorNext(Tk_PathItem *itemPtr);
MODULE_SCOPE Tk_PathItem *  TkPathCanvasItemIteratorPrev(Tk_PathItem *itemPtr);
MODULE_SCOPE int	    TkPathCanvasItemExConfigure(Tcl_Interp *interp, Tk_PathCanvas canvas, 
				    Tk_PathItemEx *itemExPtr, int mask);
MODULE_SCOPE void	    TkPathCanvasItemDetach(Tk_PathItem *itemPtr);
	
MODULE_SCOPE void	    GroupItemConfigured(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask);
MODULE_SCOPE void	    CanvasTranslateGroup(Tk_PathCanvas canvas, 
				Tk_PathItem *itemPtr, double deltaX, double deltaY);
MODULE_SCOPE void	    CanvasScaleGroup(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
				double originX, double originY, double scaleX, double scaleY);
MODULE_SCOPE int	    CanvasGradientObjCmd(Tcl_Interp* interp, TkPathCanvas *canvasPtr, 
				int objc, Tcl_Obj* CONST objv[]);
MODULE_SCOPE int	    CanvasStyleObjCmd(Tcl_Interp* interp, TkPathCanvas *canvasPtr, 
				int objc, Tcl_Obj* CONST objv[]);

MODULE_SCOPE void	    CanvasSetParentToRoot(Tk_PathItem *itemPtr);
MODULE_SCOPE void	    PathGradientChangedProc(ClientData clientData, int flags);
MODULE_SCOPE void	    PathStyleChangedProc(ClientData clientData, int flags);

MODULE_SCOPE void	    CanvasGradientsFree(TkPathCanvas *canvasPtr);

/*
 * Standard item types provided by Tk:
 */

MODULE_SCOPE Tk_PathItemType tkArcType, tkBitmapType, tkImageType, tkLineType;
MODULE_SCOPE Tk_PathItemType tkOvalType, tkPolygonType;
MODULE_SCOPE Tk_PathItemType tkRectangleType, tkTextType, tkWindowType;

/* 
 * tkpath specific item types.
 */
 
MODULE_SCOPE Tk_PathItemType tkPathType;
MODULE_SCOPE Tk_PathItemType tkPrectType;
MODULE_SCOPE Tk_PathItemType tkPlineType;
MODULE_SCOPE Tk_PathItemType tkPolylineType;
MODULE_SCOPE Tk_PathItemType tkPpolygonType;
MODULE_SCOPE Tk_PathItemType tkCircleType;
MODULE_SCOPE Tk_PathItemType tkEllipseType;
MODULE_SCOPE Tk_PathItemType tkPimageType;
MODULE_SCOPE Tk_PathItemType tkPtextType;
MODULE_SCOPE Tk_PathItemType tkGroupType;

#endif /* _TKPCANVAS */
