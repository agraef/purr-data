/*
 * tkCanvPtext.c --
 *
 *	This file implements a text canvas item modelled after its
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

typedef struct PtextItem  {
    Tk_PathItemEx headerEx; /* Generic stuff that's the same for all
                             * path types.  MUST BE FIRST IN STRUCTURE. */
    Tk_PathTextStyle textStyle;
    int textAnchor;
    double x;
    double y;
    PathRect bbox;		/* Bounding box with zero width outline.
				 * Untransformed coordinates. */
    Tcl_Obj *utf8Obj;		/* The actual text to display; UTF-8 */
    int numChars;		/* Length of text in characters. */
    int numBytes;		/* Length of text in bytes. */
    void *custom;		/* Place holder for platform dependent stuff. */
} PtextItem;


/*
 * Prototypes for procedures defined in this file:
 */

static void	ComputePtextBbox(Tk_PathCanvas canvas, PtextItem *ptextPtr);
static int	ConfigurePtext(Tcl_Interp *interp, Tk_PathCanvas canvas, 
		    Tk_PathItem *itemPtr, int objc,
		    Tcl_Obj *CONST objv[], int flags);
static int	CreatePtext(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, struct Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static void	DeletePtext(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display);
static void	DisplayPtext(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, Display *display, Drawable drawable,
		    int x, int y, int width, int height);
static void	PtextBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask);
static int	PtextCoords(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
		    int objc, Tcl_Obj *CONST objv[]);
static int	ProcessPtextCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, 
			Tk_PathItem *itemPtr, int objc, Tcl_Obj *CONST objv[]);
static int	PtextToArea(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *rectPtr);
static double	PtextToPoint(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double *coordPtr);
static int	PtextToPostscript(Tcl_Interp *interp,
		    Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass);
static void	ScalePtext(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double originX, double originY,
		    double scaleX, double scaleY);
static void	TranslatePtext(Tk_PathCanvas canvas,
		    Tk_PathItem *itemPtr, double deltaX, double deltaY);
#if 0
static void	PtextDeleteChars(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
		    int first, int last);
#endif

enum {
    PRECT_OPTION_INDEX_FONTFAMILY	    = (1L << (PATH_STYLE_OPTION_INDEX_END + 0)),
    PRECT_OPTION_INDEX_FONTSIZE		    = (1L << (PATH_STYLE_OPTION_INDEX_END + 1)),
    PRECT_OPTION_INDEX_TEXT		    = (1L << (PATH_STYLE_OPTION_INDEX_END + 2)),
    PRECT_OPTION_INDEX_TEXTANCHOR	    = (1L << (PATH_STYLE_OPTION_INDEX_END + 3)),
};
 
PATH_STYLE_CUSTOM_OPTION_RECORDS
PATH_CUSTOM_OPTION_TAGS
PATH_OPTION_STRING_TABLES_FILL
PATH_OPTION_STRING_TABLES_STROKE
PATH_OPTION_STRING_TABLES_STATE

/*
 * Best would be to extract font information from the named font "TkDefaultFont"
 * but the option defaults need static strings. Perhaps using NULL
 * and extracting family and size dynamically?
 */
#if defined(__WIN32__) || defined(_WIN32) || \
    defined(__CYGWIN__) || defined(__MINGW32__)
#   define DEF_PATHCANVTEXT_FONTFAMILY 		"Tahoma"
#   define DEF_PATHCANVTEXT_FONTSIZE 		"8"
#else
#   if defined(MAC_OSX_TK)
#	define DEF_PATHCANVTEXT_FONTFAMILY 	"Lucida Grande"
#	define DEF_PATHCANVTEXT_FONTSIZE 	"13"
#   else
#	define DEF_PATHCANVTEXT_FONTFAMILY 	"Helvetica"
#	define DEF_PATHCANVTEXT_FONTSIZE 	"12"
#   endif
#endif

/*
 * The enum kPathTextAnchorStart... MUST be kept in sync!
 */
static char *textAnchorST[] = {
    "start", "middle", "end", NULL
};

#define PATH_OPTION_SPEC_FONTFAMILY		    \
    {TK_OPTION_STRING, "-fontfamily", NULL, NULL,   \
        DEF_PATHCANVTEXT_FONTFAMILY, -1, Tk_Offset(PtextItem, textStyle.fontFamily),   \
	0, 0, PRECT_OPTION_INDEX_FONTFAMILY}

#define PATH_OPTION_SPEC_FONTSIZE		    \
    {TK_OPTION_DOUBLE, "-fontsize", NULL, NULL,   \
        DEF_PATHCANVTEXT_FONTSIZE, -1, Tk_Offset(PtextItem, textStyle.fontSize),   \
	0, 0, PRECT_OPTION_INDEX_FONTSIZE}

#define PATH_OPTION_SPEC_TEXT		    \
    {TK_OPTION_STRING, "-text", NULL, NULL,   \
        NULL, Tk_Offset(PtextItem, utf8Obj), -1,  \
	TK_OPTION_NULL_OK, 0, PRECT_OPTION_INDEX_TEXT}
	
#define PATH_OPTION_SPEC_TEXTANCHOR		    \
    {TK_OPTION_STRING_TABLE, "-textanchor", NULL, NULL, \
        "start", -1, Tk_Offset(PtextItem, textAnchor),	\
        0, (ClientData) textAnchorST, 0}

static Tk_OptionSpec optionSpecs[] = {
    PATH_OPTION_SPEC_CORE(Tk_PathItemEx),
    PATH_OPTION_SPEC_PARENT,
    PATH_OPTION_SPEC_STYLE_FILL(Tk_PathItemEx, "black"),
    PATH_OPTION_SPEC_STYLE_MATRIX(Tk_PathItemEx),
    PATH_OPTION_SPEC_STYLE_STROKE(Tk_PathItemEx, ""),
    PATH_OPTION_SPEC_FONTFAMILY,
    PATH_OPTION_SPEC_FONTSIZE,
    PATH_OPTION_SPEC_TEXT,
    PATH_OPTION_SPEC_TEXTANCHOR,
    PATH_OPTION_SPEC_END
};

static Tk_OptionTable optionTable = NULL;

/*
 * The structures below defines the 'prect' item type by means
 * of procedures that can be invoked by generic item code.
 */

Tk_PathItemType tkPtextType = {
    "ptext",				/* name */
    sizeof(PtextItem),			/* itemSize */
    CreatePtext,			/* createProc */
    optionSpecs,			/* configSpecs */
    ConfigurePtext,			/* configureProc */
    PtextCoords,			/* coordProc */
    DeletePtext,			/* deleteProc */
    DisplayPtext,			/* displayProc */
    0,					/* flags */
    PtextBbox,				/* bboxProc */
    PtextToPoint,			/* pointProc */
    PtextToArea,			/* areaProc */
    PtextToPostscript,			/* postscriptProc */
    ScalePtext,				/* scaleProc */
    TranslatePtext,			/* translateProc */
    (Tk_PathItemIndexProc *) NULL,	/* indexProc */
    (Tk_PathItemCursorProc *) NULL,	/* icursorProc */
    (Tk_PathItemSelectionProc *) NULL,	/* selectionProc */
    (Tk_PathItemInsertProc *) NULL,	/* insertProc */
    (Tk_PathItemDCharsProc *) NULL,	/* dTextProc */
    (Tk_PathItemType *) NULL,		/* nextPtr */
};
                         

static int		
CreatePtext(Tcl_Interp *interp, Tk_PathCanvas canvas, 
	struct Tk_PathItem *itemPtr,
        int objc, Tcl_Obj *CONST objv[])
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ptextPtr->headerEx;
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
    ptextPtr->bbox = NewEmptyPathRect();
    ptextPtr->utf8Obj = NULL;
    ptextPtr->numChars = 0;
    ptextPtr->numBytes = 0;
    ptextPtr->textAnchor = kPathTextAnchorStart;
    ptextPtr->textStyle.fontFamily = NULL;
    ptextPtr->textStyle.fontSize = 0.0;
    ptextPtr->custom = NULL;
    
    if (optionTable == NULL) {
	optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    } 
    itemPtr->optionTable = optionTable;
    if (Tk_InitOptions(interp, (char *) ptextPtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas)) != TCL_OK) {
        goto error;
    }

    for (i = 1; i < objc; i++) {
        char *arg = Tcl_GetString(objv[i]);
        if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
            break;
        }
    }
    if (ProcessPtextCoords(interp, canvas, itemPtr, i, objv) != TCL_OK) {
        goto error;
    }
    if (ConfigurePtext(interp, canvas, itemPtr, objc-i, objv+i, 0) == TCL_OK) {
        return TCL_OK;
    }

error:
    /*
     * NB: We must unlink the item here since the TkPathCanvasItemExConfigure()
     *     link it to the root by default.
     */
    TkPathCanvasItemDetach(itemPtr);
    DeletePtext(canvas, itemPtr, Tk_Display(Tk_PathCanvasTkwin(canvas)));
    return TCL_ERROR;
}

static int
ProcessPtextCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[])
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;

    if (objc == 0) {
        Tcl_Obj *obj = Tcl_NewObj();
        Tcl_Obj *subobj = Tcl_NewDoubleObj(ptextPtr->x);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        subobj = Tcl_NewDoubleObj(ptextPtr->y);
        Tcl_ListObjAppendElement(interp, obj, subobj);
        Tcl_SetObjResult(interp, obj);
    } else if (objc < 3) {
        if (objc == 1) {
            if (Tcl_ListObjGetElements(interp, objv[0], &objc,
                    (Tcl_Obj ***) &objv) != TCL_OK) {
                return TCL_ERROR;
            } else if (objc != 2) {
                Tcl_SetObjResult(interp, 
			Tcl_NewStringObj("wrong # coordinates: expected 0 or 2", -1));
                return TCL_ERROR;
            }
        }
        if ((Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[0], &ptextPtr->x) != TCL_OK)
            || (Tk_PathCanvasGetCoordFromObj(interp, canvas, objv[1], &ptextPtr->y) != TCL_OK)) {
            return TCL_ERROR;
        }
    } else {
        Tcl_SetObjResult(interp, 
		Tcl_NewStringObj("wrong # coordinates: expected 0 or 2", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}


static int		
PtextCoords(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[])
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    int result;

    result = ProcessPtextCoords(interp, canvas, itemPtr, objc, objv);
    if ((result == TCL_OK) && (objc > 0) && (objc < 3)) {
	ComputePtextBbox(canvas, ptextPtr);
    }
    return result;
}

void
ComputePtextBbox(Tk_PathCanvas canvas, PtextItem *ptextPtr)
{
    Tk_PathItemEx *itemExPtr = &ptextPtr->headerEx;
    Tk_PathItem *itemPtr = &itemExPtr->header;
    Tk_PathStyle style;
    Tk_PathState state = itemExPtr->header.state;
    double width;
    PathRect bbox, r;

    if(state == TK_PATHSTATE_NULL) {
	state = TkPathCanvasState(canvas);
    }
    if (ptextPtr->utf8Obj == NULL || (state == TK_PATHSTATE_HIDDEN)) {
        itemExPtr->header.x1 = itemExPtr->header.x2 =
        itemExPtr->header.y1 = itemExPtr->header.y2 = -1;
        return;
    }
    style = TkPathCanvasInheritStyle(itemPtr, kPathMergeStyleNotFill);
    r = TkPathTextMeasureBbox(&ptextPtr->textStyle, 
	    Tcl_GetString(ptextPtr->utf8Obj), ptextPtr->custom);
    width = r.x2 - r.x1;
    switch (ptextPtr->textAnchor) {
        case kPathTextAnchorStart: 
            bbox.x1 = ptextPtr->x;
            bbox.x2 = bbox.x1 + width;
            break;
        case kPathTextAnchorMiddle:
            bbox.x1 = ptextPtr->x - width/2;
            bbox.x2 = ptextPtr->x + width/2;
            break;
        case kPathTextAnchorEnd:
            bbox.x1 = ptextPtr->x - width;
            bbox.x2 = ptextPtr->x;
            break;
    }
    bbox.y1 = ptextPtr->y + r.y1;	// r.y1 is negative!
    bbox.y2 = ptextPtr->y + r.y2;
    
    /* Fudge for antialiasing etc. */
    bbox.x1 -= 1.0;
    bbox.y1 -= 1.0;
    bbox.x2 += 1.0;
    bbox.y2 += 1.0;
    if (style.strokeColor) {
        double halfWidth = style.strokeWidth/2;
        bbox.x1 -= halfWidth;
        bbox.y1 -= halfWidth;
        bbox.x2 += halfWidth;
        bbox.x2 += halfWidth;
    }
    ptextPtr->bbox = bbox;
    SetGenericPathHeaderBbox(&itemExPtr->header, style.matrixPtr, &bbox);
    TkPathCanvasFreeInheritedStyle(&style);
}

static int		
ConfigurePtext(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, 
        int objc, Tcl_Obj *CONST objv[], int flags)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ptextPtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;
    Tk_Window tkwin;
    //Tk_PathState state;
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error, mask;

    tkwin = Tk_PathCanvasTkwin(canvas);
    for (error = 0; error <= 1; error++) {
	if (!error) {
	    if (Tk_SetOptions(interp, (char *) ptextPtr, optionTable, 
		    objc, objv, tkwin, &savedOptions, &mask) != TCL_OK) {
		continue;
	    }
	} else {
	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}	
	
	/*
	 * Since we have -fill default equal to black we need to force
	 * setting the fill member of the style.
	 */
	if (TkPathCanvasItemExConfigure(interp, canvas, itemExPtr, mask | PATH_STYLE_OPTION_FILL) != TCL_OK) {
	    continue;
	}
	// @@@ TkPathTextConfig needs to be reworked!
	if (ptextPtr->utf8Obj != NULL) {
	    if (TkPathTextConfig(interp, &(ptextPtr->textStyle), 
		    Tcl_GetString(ptextPtr->utf8Obj), &ptextPtr->custom) != TCL_OK) {
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
    if (ptextPtr->utf8Obj != NULL) {
        ptextPtr->numBytes = Tcl_GetCharLength(ptextPtr->utf8Obj);
        ptextPtr->numChars = Tcl_NumUtfChars(Tcl_GetString(ptextPtr->utf8Obj), 
		ptextPtr->numBytes);
    } else {
        ptextPtr->numBytes = 0;
        ptextPtr->numChars = 0;
    }
#if 0	    // From old code. Needed?
    state = itemPtr->state;
    if (state == TK_PATHSTATE_NULL) {
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
	ComputePtextBbox(canvas, ptextPtr);
	return TCL_OK;
    }
}

static void		
DeletePtext(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ptextPtr->headerEx;
    Tk_PathStyle *stylePtr = &itemExPtr->style;

    if (stylePtr->fill != NULL) {
	TkPathFreePathColor(stylePtr->fill);
    }
    if (itemExPtr->styleInst != NULL) {
	TkPathFreeStyle(itemExPtr->styleInst);
    }
    TkPathTextFree(&(ptextPtr->textStyle), ptextPtr->custom);
    Tk_FreeConfigOptions((char *) ptextPtr, optionTable, 
	    Tk_PathCanvasTkwin(canvas));
}

static void		
DisplayPtext(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, Display *display, Drawable drawable,
        int x, int y, int width, int height)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    Tk_PathItemEx *itemExPtr = &ptextPtr->headerEx;
    Tk_PathStyle style;
    TMatrix m = GetCanvasTMatrix(canvas);
    TkPathContext ctx;
    
    /* === EB - 23-apr-2010: register coordinate offsets */
    TkPathSetCoordOffsets(m.tx, m.ty);
    /* === */
    
    if (ptextPtr->utf8Obj == NULL) {
        return;
    }
    
    /*
     * The defaults for -fill and -stroke differ for the ptext item.
     */
    style = TkPathCanvasInheritStyle(itemPtr, 0);
    if (!(style.mask & PATH_STYLE_OPTION_FILL)) {
	style.fill = itemExPtr->style.fill;
    }
    if (!(style.mask & PATH_STYLE_OPTION_STROKE)) {
	style.strokeColor = itemExPtr->style.strokeColor;
    }
    
    ctx = TkPathInit(Tk_PathCanvasTkwin(canvas), drawable);
    
    TkPathPushTMatrix(ctx, &m);
    if (style.matrixPtr != NULL) {
        TkPathPushTMatrix(ctx, style.matrixPtr);
    }
    TkPathBeginPath(ctx, &style);
    /* @@@ We need to handle gradients as well here!
           Wait to see what the other APIs have to say.
    */
    TkPathTextDraw(ctx, &style, &ptextPtr->textStyle, ptextPtr->bbox.x1, ptextPtr->y, 
            Tcl_GetString(ptextPtr->utf8Obj), ptextPtr->custom);
    TkPathEndPath(ctx);
    TkPathFree(ctx);
    TkPathCanvasFreeInheritedStyle(&style);
}

static void	
PtextBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    ComputePtextBbox(canvas, ptextPtr);
}

static double	
PtextToPoint(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *pointPtr)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    Tk_PathStyle style;
    double dist;

    style = TkPathCanvasInheritStyle(itemPtr, 
	    kPathMergeStyleNotFill | kPathMergeStyleNotStroke);
    dist = PathRectToPointWithMatrix(ptextPtr->bbox, style.matrixPtr, pointPtr);    
    TkPathCanvasFreeInheritedStyle(&style);
    return dist;
}

static int		
PtextToArea(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double *areaPtr)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    Tk_PathStyle style;
    int area;
    
    style = TkPathCanvasInheritStyle(itemPtr, 
	    kPathMergeStyleNotFill | kPathMergeStyleNotStroke);
    area = PathRectToAreaWithMatrix(ptextPtr->bbox, style.matrixPtr, areaPtr);
    TkPathCanvasFreeInheritedStyle(&style);
    return area;
}

static int		
PtextToPostscript(Tcl_Interp *interp, Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int prepass)
{
    return TCL_ERROR;
}

static void		
ScalePtext(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double originX, double originY,
        double scaleX, double scaleY)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;

    ptextPtr->x = originX + scaleX*(ptextPtr->x - originX);
    ptextPtr->y = originY + scaleY*(ptextPtr->y - originY);
    ScalePathRect(&ptextPtr->bbox, originX, originY, scaleX, scaleY);
    ScaleItemHeader(itemPtr, originX, originY, scaleX, scaleY);
}

static void		
TranslatePtext(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, double deltaX, double deltaY)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;

    ptextPtr->x += deltaX;
    ptextPtr->y += deltaY;
    TranslatePathRect(&ptextPtr->bbox, deltaX, deltaY);
    TranslateItemHeader(itemPtr, deltaX, deltaY);
}

#if 0	// TODO
static void
PtextDeleteChars(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int first, int last)
{
    PtextItem *ptextPtr = (PtextItem *) itemPtr;
    int byteIndex, byteCount, charsRemoved;
    char *new, *text;

    text = ptextPtr->utf8;
    if (first < 0) {
        first = 0;
    }
    if (last >= ptextPtr->numChars) {
        last = ptextPtr->numChars - 1;
    }
    if (first > last) {
        return;
    }
    charsRemoved = last + 1 - first;

    byteIndex = Tcl_UtfAtIndex(text, first) - text;
    byteCount = Tcl_UtfAtIndex(text + byteIndex, charsRemoved) - (text + byteIndex);
    
    new = (char *) ckalloc((unsigned) (ptextPtr->numBytes + 1 - byteCount));
    memcpy(new, text, (size_t) byteIndex);
    strcpy(new + byteIndex, text + byteIndex + byteCount);

    ckfree(text);
    ptextPtr->utf8 = new;
    ptextPtr->numChars -= charsRemoved;
    ptextPtr->numBytes -= byteCount;
    
    //TkPathTextConfig(interp, &(ptextPtr->textStyle), ptextPtr->utf8, &(ptextPtr->custom));
    ComputePtextBbox(canvas, ptextPtr);
    return;
}
#endif

/*----------------------------------------------------------------------*/

