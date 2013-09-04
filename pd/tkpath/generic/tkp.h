/*
 * tkp.h --
 *
 *		This file includes stuff from tk.h which we need
 *		in a modified form and to make the tkp::canvas self contained.
 *
 * $Id$
 */

#ifndef INCLUDED_TKP_H
#define INCLUDED_TKP_H

#include "tk.h"

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Tk_PathCanvas_ is just a dummy which is never defined anywhere.
 * This happens to work because Tk_PathCanvas is a pointer.
 * Its reason is to hide the internals of TkPathCanvas to item code.
 */
typedef struct Tk_PathCanvas_ *Tk_PathCanvas;


/*
 *--------------------------------------------------------------
 *
 * Procedure prototypes and structures used for defining new canvas items:
 *
 *--------------------------------------------------------------
 */

typedef enum {
    TK_PATHSTATE_NULL = -1, TK_PATHSTATE_ACTIVE, TK_PATHSTATE_DISABLED,
    TK_PATHSTATE_NORMAL, TK_PATHSTATE_HIDDEN
} Tk_PathState;


typedef struct Tk_PathSmoothMethod {
    char *name;
    int (*coordProc) _ANSI_ARGS_((Tk_PathCanvas canvas,
		double *pointPtr, int numPoints, int numSteps,
		XPoint xPoints[], double dblPoints[]));
    void (*postscriptProc) _ANSI_ARGS_((Tcl_Interp *interp,
		Tk_PathCanvas canvas, double *coordPtr,
		int numPoints, int numSteps));
} Tk_PathSmoothMethod;

/*
 * For each item in a canvas widget there exists one record with the following
 * structure. Each actual item is represented by a record with the following
 * stuff at its beginning, plus additional type-specific stuff after that.
 */

#define TK_PATHTAG_SPACE 3

typedef struct Tk_PathTags {
    Tk_Uid *tagPtr;		/* Pointer to array of tags. */
    int tagSpace;		/* Total amount of tag space available at
				 * tagPtr. */
    int numTags;		/* Number of tag slots actually used at
				 * *tagPtr. */
} Tk_PathTags;

typedef struct Tk_PathItem {
    int id;			/* Unique identifier for this item (also
				 * serves as first tag for item). */
    Tk_OptionTable optionTable;	/* Option table */
    struct Tk_PathItem *nextPtr;/* Next sibling in display list of this group.
				 * Later items in list are drawn on
				 * top of earlier ones. */
    struct Tk_PathItem *prevPtr;/* Previous sibling in display list of this group. */
    struct Tk_PathItem *parentPtr;  
				/* Parent of item or NULL if root. */
    struct Tk_PathItem *firstChildPtr;  
				/* First child item, only for groups. */
    struct Tk_PathItem *lastChildPtr;	
				/* Last child item, only for groups. */
    Tcl_Obj *parentObj;		/*   */
    Tk_PathTags *pathTagsPtr;	/* Allocated struct for storing tags.
				 * This is needed by the custom option handling. */

//#ifdef USE_OLD_CODE
    Tk_Uid staticTagSpace[TK_PATHTAG_SPACE];
				/* Built-in space for limited # of tags. */
    Tk_Uid *tagPtr;		/* Pointer to array of tags. Usually points to
				 * staticTagSpace, but may point to malloc-ed
				 * space if there are lots of tags. */
    int tagSpace;		/* Total amount of tag space available at
				 * tagPtr. */
    int numTags;		/* Number of tag slots actually used at
				 * *tagPtr. */
//#endif

    struct Tk_PathItemType *typePtr;/* Table of procedures that implement this
				 * type of item. */
    int x1, y1, x2, y2;		/* Bounding box for item, in integer canvas
				 * units. Set by item-specific code and
				 * guaranteed to contain every pixel drawn in
				 * item. Item area includes x1 and y1 but not
				 * x2 and y2. */
    Tk_PathState state;		/* State of item. */
    char *reserved1;		/* reserved for future use */
    int redraw_flags;		/* Some flags used in the canvas */

    /*
     *------------------------------------------------------------------
     * Starting here is additional type-specific stuff; see the declarations
     * for individual types to see what is part of each type. The actual space
     * below is determined by the "itemInfoSize" of the type's Tk_PathItemType
     * record.
     *------------------------------------------------------------------
     */
} Tk_PathItem;

/*
 * Flag bits for canvases (redraw_flags):
 *
 * TK_ITEM_STATE_DEPENDANT -	1 means that object needs to be redrawn if the
 *				canvas state changes.
 * TK_ITEM_DONT_REDRAW - 	1 means that the object redraw is already been
 *				prepared, so the general canvas code doesn't
 *				need to do that any more.
 */

#define TK_ITEM_STATE_DEPENDANT		1
#define TK_ITEM_DONT_REDRAW		2

/*
 * Records of the following type are used to describe a type of item (e.g.
 * lines, circles, etc.) that can form part of a canvas widget.
 */

typedef int	Tk_PathItemCreateProc(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int argc,
		    Tcl_Obj *const objv[]);
typedef int	Tk_PathItemConfigureProc(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int argc,
		    Tcl_Obj *const objv[], int flags);
typedef int	Tk_PathItemCoordProc(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int argc,
		    Tcl_Obj *const argv[]);
typedef void	Tk_PathItemDeleteProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display);
typedef void	Tk_PathItemDisplayProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display, Drawable dst,
		    int x, int y, int width, int height);
typedef void	TkPathItemBboxProc(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
		    int mask);
typedef double	Tk_PathItemPointProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *pointPtr);
typedef int	Tk_PathItemAreaProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *rectPtr);
typedef int	Tk_PathItemPostscriptProc(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
typedef void	Tk_PathItemScaleProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double originX, double originY,
		    double scaleX, double scaleY);
typedef void	Tk_PathItemTranslateProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double deltaX, double deltaY);
typedef int	Tk_PathItemIndexProc(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, char *indexString,
		    int *indexPtr);
typedef void	Tk_PathItemCursorProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, int index);
typedef int	Tk_PathItemSelectionProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, int offset, char *buffer,
		    int maxBytes);
typedef void	Tk_PathItemInsertProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, int beforeThis, char *string);
typedef void	Tk_PathItemDCharsProc(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, int first, int last);

#ifndef __NO_OLD_CONFIG

typedef struct Tk_PathItemType {
    char *name;			/* The name of this type of item, such as
				 * "line". */
    int itemSize;		/* Total amount of space needed for item's
				 * record. */
    Tk_PathItemCreateProc *createProc;
				/* Procedure to create a new item of this
				 * type. */
    Tk_OptionSpec *optionSpecs;	/* Pointer to array of option specs for
				 * this type. Used for returning option
				 * info. */
    Tk_PathItemConfigureProc *configProc;
				/* Procedure to call to change configuration
				 * options. */
    Tk_PathItemCoordProc *coordProc;/* Procedure to call to get and set the item's
				 * coordinates. */
    Tk_PathItemDeleteProc *deleteProc;
				/* Procedure to delete existing item of this
				 * type. */
    Tk_PathItemDisplayProc *displayProc;
				/* Procedure to display items of this type. */
    int alwaysRedraw;		/* Non-zero means displayProc should be called
				 * even when the item has been moved
				 * off-screen. */
    TkPathItemBboxProc *bboxProc;
				/* Procedure that is invoked by group items
				 * on its children when it has reconfigured in
				 * any way that affect the childrens bbox display. */
    Tk_PathItemPointProc *pointProc;
				/* Computes distance from item to a given
				 * point. */
    Tk_PathItemAreaProc *areaProc;	
				/* Computes whether item is inside, outside,
				 * or overlapping an area. */
    Tk_PathItemPostscriptProc *postscriptProc;
				/* Procedure to write a Postscript description
				 * for items of this type. */
    Tk_PathItemScaleProc *scaleProc;/* Procedure to rescale items of this type. */
    Tk_PathItemTranslateProc *translateProc;
				/* Procedure to translate items of this
				 * type. */
    Tk_PathItemIndexProc *indexProc;/* Procedure to determine index of indicated
				 * character. NULL if item doesn't support
				 * indexing. */
    Tk_PathItemCursorProc *icursorProc;
				/* Procedure to set insert cursor posn to just
				 * before a given position. */
    Tk_PathItemSelectionProc *selectionProc;
				/* Procedure to return selection (in STRING
				 * format) when it is in this item. */
    Tk_PathItemInsertProc *insertProc;
				/* Procedure to insert something into an
				 * item. */
    Tk_PathItemDCharsProc *dCharsProc;
				/* Procedure to delete characters from an
				 * item. */
    struct Tk_PathItemType *nextPtr;/* Used to link types together into a list. */
    char *reserved1;		/* Reserved for future extension. */
    int reserved2;		/* Carefully compatible with */
    char *reserved3;		/* Jan Nijtmans dash patch */
    char *reserved4;
} Tk_PathItemType;

#endif

/*
 * The following structure provides information about the selection and the
 * insertion cursor. It is needed by only a few items, such as those that
 * display text. It is shared by the generic canvas code and the item-specific
 * code, but most of the fields should be written only by the canvas generic
 * code.
 */

typedef struct Tk_PathCanvasTextInfo {
    Tk_3DBorder selBorder;	/* Border and background for selected
				 * characters. Read-only to items.*/
    int selBorderWidth;		/* Width of border around selection. Read-only
				 * to items. */
    XColor *selFgColorPtr;	/* Foreground color for selected text.
				 * Read-only to items. */
    Tk_PathItem *selItemPtr;	/* Pointer to selected item. NULL means
				 * selection isn't in this canvas. Writable by
				 * items. */
    int selectFirst;		/* Character index of first selected
				 * character. Writable by items. */
    int selectLast;		/* Character index of last selected character.
				 * Writable by items. */
    Tk_PathItem *anchorItemPtr;	/* Item corresponding to "selectAnchor": not
				 * necessarily selItemPtr. Read-only to
				 * items. */
    int selectAnchor;		/* Character index of fixed end of selection
				 * (i.e. "select to" operation will use this
				 * as one end of the selection). Writable by
				 * items. */
    Tk_3DBorder insertBorder;	/* Used to draw vertical bar for insertion
				 * cursor. Read-only to items. */
    int insertWidth;		/* Total width of insertion cursor. Read-only
				 * to items. */
    int insertBorderWidth;	/* Width of 3-D border around insert cursor.
				 * Read-only to items. */
    Tk_PathItem *focusItemPtr;	/* Item that currently has the input focus, or
				 * NULL if no such item. Read-only to items. */
    int gotFocus;		/* Non-zero means that the canvas widget has
				 * the input focus. Read-only to items.*/
    int cursorOn;		/* Non-zero means that an insertion cursor
				 * should be displayed in focusItemPtr.
				 * Read-only to items.*/
} Tk_PathCanvasTextInfo;

/*
 * Structures used for Dashing and Outline.
 */
typedef struct Tk_PathDash {
    int number;
    float *array;
} Tk_PathDash;

/*
 * Bit fields in Tk_Offset->flags:
 */

#if 0

#define TK_OFFSET_INDEX		1
#define TK_OFFSET_RELATIVE	2
#define TK_OFFSET_LEFT		4
#define TK_OFFSET_CENTER	8
#define TK_OFFSET_RIGHT		16
#define TK_OFFSET_TOP		32
#define TK_OFFSET_MIDDLE	64
#define TK_OFFSET_BOTTOM	128

#endif

typedef struct Tk_PathOutline {
    GC gc;			/* Graphics context. */
    double width;		/* Width of outline. */
    double activeWidth;		/* Width of outline. */
    double disabledWidth;	/* Width of outline. */
    int offset;			/* Dash offset. */
    Tk_Dash *dashPtr;		/* Dash pattern. */
    Tk_Dash *activeDashPtr;	/* Dash pattern if state is active. */
    Tk_Dash *disabledDashPtr;	/* Dash pattern if state is disabled. */

    VOID *reserved1;		/* Reserved for future expansion. */
    VOID *reserved2;
    VOID *reserved3;
    Tk_TSOffset *tsoffsetPtr;	/* Stipple offset for outline. */
    XColor *color;		/* Outline color. */
    XColor *activeColor;	/* Outline color if state is active. */
    XColor *disabledColor;	/* Outline color if state is disabled. */
    Pixmap stipple;		/* Outline Stipple pattern. */
    Pixmap activeStipple;	/* Outline Stipple pattern if state is
				 * active. */
    Pixmap disabledStipple;	/* Outline Stipple pattern if state is
				 * disabled. */
} Tk_PathOutline;


/*
 * Functions normally in the tk stubs table.
 */
 
/* From tkpCanvUtil.c */

Tk_Window	Tk_PathCanvasTkwin(Tk_PathCanvas canvas);
void		Tk_CreatePathItemType(Tk_PathItemType *typePtr);
void		Tk_PathCreateSmoothMethod(Tcl_Interp * interp, 
			    Tk_PathSmoothMethod * method);   
int		Tk_PathConfigOutlineGC(XGCValues *gcValues, Tk_PathCanvas canvas,
			    Tk_PathItem *item, Tk_PathOutline *outline);
int		Tk_PathChangeOutlineGC(Tk_PathCanvas canvas, Tk_PathItem *item,
			    Tk_PathOutline *outline);
int		Tk_PathResetOutlineGC(Tk_PathCanvas canvas, Tk_PathItem *item,
			    Tk_PathOutline *outline);
int		Tk_PathCanvasPsOutline(Tk_PathCanvas canvas, Tk_PathItem *item,
			    Tk_PathOutline *outline);
void		Tk_PathCanvasDrawableCoords(Tk_PathCanvas canvas,
			    double x, double y, short *drawableXPtr, short *drawableYPtr);
void		Tk_PathCanvasWindowCoords(Tk_PathCanvas canvas,
			    double x, double y, short *screenXPtr, short *screenYPtr);
int		Tk_PathCanvasGetCoord(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    CONST char *string, double *doublePtr);
int		Tk_PathCanvasGetCoordFromObj(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    Tcl_Obj *obj, double *doublePtr);
void		Tk_PathCanvasSetStippleOrigin(Tk_PathCanvas canvas, GC gc);
void		Tk_PathCanvasSetOffset(Tk_PathCanvas canvas, GC gc, Tk_TSOffset *offset);
Tk_PathCanvasTextInfo *	Tk_PathCanvasGetTextInfo(Tk_PathCanvas canvas);
int		Tk_PathCanvasTagsParseProc( ClientData clientData, Tcl_Interp *interp,
			    Tk_Window tkwin, CONST char *value, char *widgRec, int offset);
char *		Tk_PathCanvasTagsPrintProc(ClientData clientData, Tk_Window tkwin,
			    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr);
void		Tk_PathCreateSmoothMethod(Tcl_Interp *interp, Tk_PathSmoothMethod *smooth);
void		Tk_PathCreateOutline(Tk_PathOutline *outline);
void		Tk_PathDeleteOutline(Display *display, Tk_PathOutline *outline);
    
int		Tk_PathCanvasTagsOptionSetProc(ClientData clientData, Tcl_Interp *interp,
			    Tk_Window tkwin, Tcl_Obj **value, char *recordPtr,
			    int internalOffset, char *oldInternalPtr, int flags);
Tcl_Obj *	Tk_PathCanvasTagsOptionGetProc(ClientData clientData, Tk_Window tkwin,
			    char *recordPtr, int internalOffset);
void		Tk_PathCanvasTagsOptionRestoreProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);
void		Tk_PathCanvasTagsOptionFreeProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr);
    
/* From tkpCanvas.c */

void		Tk_PathCanvasEventuallyRedraw(Tk_PathCanvas canvas,
			    int x1, int y1, int x2, int y2);
int		Tk_PathCanvasPsColor(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    XColor *colorPtr);
int		Tk_PathCanvasPsFont(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    Tk_Font tkfont);
int		Tk_PathCanvasPsBitmap(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    Pixmap bitmap, int startX, int startY, int width, int height);
int		Tk_PathCanvasPsStipple(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    Pixmap bitmap);
double		Tk_PathCanvasPsY(Tk_PathCanvas canvas, double y);
void		Tk_PathCanvasPsPath(Tcl_Interp *interp, Tk_PathCanvas canvas,
			    double *coordPtr, int numPoints);



#ifdef __cplusplus
}
#endif

#endif		// end INCLUDED_TKP_H

