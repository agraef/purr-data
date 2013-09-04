/*
 * tkpCanvas.c --
 *
 *	This module implements canvas widgets for the Tk toolkit. A canvas
 *	displays a background and a collection of graphical objects such as
 *	rectangles, lines, and texts.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 2008 Mats Bengtsson
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

/* #define USE_OLD_TAG_SEARCH 1 */

#ifdef MAC_OSX_TK
#define TK_PATH_NO_DOUBLE_BUFFERING
#endif

#include "default.h"
#include "tkInt.h"
#include "tkIntPath.h"
#include "tkpCanvas.h"
#ifdef TK_PATH_NO_DOUBLE_BUFFERING
#ifdef MAC_OSX_TK
#include "tkMacOSXInt.h"
#endif
#endif /* TK_PATH_NO_DOUBLE_BUFFERING */

/* For debugging. */
extern Tcl_Interp *gInterp;

/*
 * See tkpCanvas.h for key data structures used to implement canvases.
 */

#ifdef USE_OLD_TAG_SEARCH

/*
 * The structure defined below is used to keep track of a tag search in
 * progress. No field should be accessed by anyone other than StartTagSearch
 * and NextItem.
 */

typedef struct TagSearch {
    TkPathCanvas *canvasPtr;	/* Canvas widget being searched. */
    Tk_Uid tag;			/* Tag to search for. 0 means return all
				 * items. */
    Tk_PathItem *currentPtr;	/* Pointer to last item returned. */
    Tk_PathItem *lastPtr;	/* The item right before the currentPtr is
				 * tracked so if the currentPtr is deleted we
				 * don't have to start from the beginning. */
    int searchOver;		/* Non-zero means NextItem should always
				 * return NULL. */
} TagSearch;

#else /* USE_OLD_TAG_SEARCH */
/*
 * The structure defined below is used to keep track of a tag search in
 * progress. No field should be accessed by anyone other than TagSearchScan,
 * TagSearchFirst, TagSearchNext, TagSearchScanExpr, TagSearchEvalExpr,
 * TagSearchExprInit, TagSearchExprDestroy, TagSearchDestroy.
 * (
 *   Not quite accurate: the TagSearch structure is also accessed from:
 *    CanvasWidgetCmd, FindItems, RelinkItems
 *   The only instances of the structure are owned by:
 *    CanvasWidgetCmd
 *   CanvasWidgetCmd is the only function that calls:
 *    FindItems, RelinkItems
 *   CanvasWidgetCmd, FindItems, RelinkItems, are the only functions that call
 *    TagSearch*
 * )
 */

typedef struct TagSearch {
    TkPathCanvas *canvasPtr;	/* Canvas widget being searched. */
    Tk_PathItem *currentPtr;	/* Pointer to last item returned. */
    Tk_PathItem *lastPtr;	/* The item right before the currentPtr is
				 * tracked so if the currentPtr is deleted we
				 * don't have to start from the beginning. */
    int searchOver;		/* Non-zero means NextItem should always
				 * return NULL. */
    int type;			/* Search type (see #defs below) */
    int id;			/* Item id for searches by id */
    char *string;		/* Tag expression string */
    int stringIndex;		/* Current position in string scan */
    int stringLength;		/* Length of tag expression string */
    char *rewritebuffer;	/* Tag string (after removing escapes) */
    unsigned int rewritebufferAllocated;
				/* Available space for rewrites. */
    TagSearchExpr *expr;	/* Compiled tag expression. */
} TagSearch;

/*
 * Values for the TagSearch type field.
 */

#define SEARCH_TYPE_EMPTY	0	/* Looking for empty tag */
#define SEARCH_TYPE_ID		1	/* Looking for an item by id */
#define SEARCH_TYPE_ALL		2	/* Looking for all items */
#define SEARCH_TYPE_TAG		3	/* Looking for an item by simple tag */
#define SEARCH_TYPE_EXPR	4	/* Compound search */
#define SEARCH_TYPE_ROOT	5	/* Looking for the root item */

#endif /* USE_OLD_TAG_SEARCH */

#define PATH_DEF_STATE "normal"

/* These MUST be kept in sync with enums! X.h */

static char *stateStrings[] = {
    "active", "disabled", "normal", "hidden", NULL
};

static char *tagStyleStrings[] = {
    "exact", "expr", "glob", NULL
};

static Tk_ObjCustomOption offsetCO = {
    "offset",			
    TkPathOffsetOptionSetProc,
    TkPathOffsetOptionGetProc,
    TkPathOffsetOptionRestoreProc,
    TkPathOffsetOptionFreeProc,	
    (ClientData) (TK_OFFSET_RELATIVE|TK_OFFSET_INDEX)			
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_CANVAS_BG_COLOR, -1, Tk_Offset(TkPathCanvas, bgBorder),
	0, (ClientData) DEF_CANVAS_BG_MONO, 0},
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
	NULL, 0, -1, 0, (ClientData) "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
	NULL, 0, -1, 0, (ClientData) "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_CANVAS_BORDER_WIDTH, Tk_Offset(TkPathCanvas, borderWidthPtr),
	Tk_Offset(TkPathCanvas, borderWidth), 0, 0, 0},
    {TK_OPTION_DOUBLE, "-closeenough", "closeEnough", "CloseEnough",
	DEF_CANVAS_CLOSE_ENOUGH, -1, Tk_Offset(TkPathCanvas, closeEnough),
	0, 0, 0},
    {TK_OPTION_BOOLEAN, "-confine", "confine", "Confine",
	DEF_CANVAS_CONFINE, -1, Tk_Offset(TkPathCanvas, confine),
	0, 0, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_CANVAS_CURSOR, -1, Tk_Offset(TkPathCanvas, cursor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-height", "height", "Height",
	DEF_CANVAS_HEIGHT, -1, Tk_Offset(TkPathCanvas, height), 
	0, 0, 0},
    {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_CANVAS_HIGHLIGHT_BG,
	-1, Tk_Offset(TkPathCanvas, highlightBgColorPtr),
	0, 0, 0},
    {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_CANVAS_HIGHLIGHT, -1, Tk_Offset(TkPathCanvas, highlightColorPtr),
	0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_CANVAS_HIGHLIGHT_WIDTH,
	Tk_Offset(TkPathCanvas, highlightWidthPtr),
	Tk_Offset(TkPathCanvas, highlightWidth), 0, 0, 0},
    {TK_OPTION_BORDER, "-insertbackground", "insertBackground", "Foreground",
	DEF_CANVAS_INSERT_BG, -1, Tk_Offset(TkPathCanvas, textInfo.insertBorder), 
	0, 0, 0},
    {TK_OPTION_PIXELS, "-insertborderwidth", "insertBorderWidth",
	"BorderWidth", DEF_CANVAS_INSERT_BD_COLOR, -1,
	Tk_Offset(TkPathCanvas, textInfo.insertBorderWidth), 
	0, (ClientData) DEF_CANVAS_INSERT_BD_MONO, 0},
    {TK_OPTION_INT, "-insertofftime", "insertOffTime", "OffTime",
	DEF_CANVAS_INSERT_OFF_TIME, -1, Tk_Offset(TkPathCanvas, insertOffTime),
	0, 0, 0},
    {TK_OPTION_INT, "-insertontime", "insertOnTime", "OnTime",
	DEF_CANVAS_INSERT_ON_TIME, -1, Tk_Offset(TkPathCanvas, insertOnTime), 
	0, 0, 0},
    {TK_OPTION_PIXELS, "-insertwidth", "insertWidth", "InsertWidth",
	DEF_CANVAS_INSERT_WIDTH, -1, Tk_Offset(TkPathCanvas, textInfo.insertWidth),
	0, 0, 0},
    {TK_OPTION_CUSTOM, "-offset", "offset", "Offset",
	"0,0", -1, Tk_Offset(TkPathCanvas, tsoffsetPtr),
	0, &offsetCO, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_CANVAS_RELIEF, -1, Tk_Offset(TkPathCanvas, relief), 
	0, 0, 0},
    {TK_OPTION_STRING, "-scrollregion", "scrollRegion", "ScrollRegion",
	DEF_CANVAS_SCROLL_REGION, -1, Tk_Offset(TkPathCanvas, regionString),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BORDER, "-selectbackground", "selectBackground", "Foreground",
	DEF_CANVAS_SELECT_COLOR, -1, Tk_Offset(TkPathCanvas, textInfo.selBorder),
	0, (ClientData) DEF_CANVAS_SELECT_MONO, 0},
    {TK_OPTION_PIXELS, "-selectborderwidth", "selectBorderWidth",
	"BorderWidth", DEF_CANVAS_SELECT_BD_COLOR, -1,
	Tk_Offset(TkPathCanvas, textInfo.selBorderWidth),
	0, (ClientData) DEF_CANVAS_SELECT_BD_MONO, 0},
    {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "Background",
	DEF_CANVAS_SELECT_FG_COLOR, -1, Tk_Offset(TkPathCanvas, textInfo.selFgColorPtr),
	TK_OPTION_NULL_OK, (ClientData) DEF_CANVAS_SELECT_FG_MONO, 0},	
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
	PATH_DEF_STATE, -1, Tk_Offset(TkPathCanvas, canvas_state),
	0, (ClientData) stateStrings, 0},
    {TK_OPTION_STRING_TABLE, "-tagstyle", NULL, NULL,
        "expr", -1, Tk_Offset(TkPathCanvas, tagStyle),
        0, (ClientData) tagStyleStrings, 0},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_CANVAS_TAKE_FOCUS, -1, Tk_Offset(TkPathCanvas, takeFocus),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-width", "width", "Width",
	DEF_CANVAS_WIDTH, -1, Tk_Offset(TkPathCanvas, width), 
	0, 0, 0},
    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	DEF_CANVAS_X_SCROLL_CMD, -1, Tk_Offset(TkPathCanvas, xScrollCmd),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-xscrollincrement", "xScrollIncrement",
	"ScrollIncrement",
	DEF_CANVAS_X_SCROLL_INCREMENT, -1, Tk_Offset(TkPathCanvas, xScrollIncrement),
	0, 0, 0},
    {TK_OPTION_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	DEF_CANVAS_Y_SCROLL_CMD, -1, Tk_Offset(TkPathCanvas, yScrollCmd),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-yscrollincrement", "yScrollIncrement",
	"ScrollIncrement",
	DEF_CANVAS_Y_SCROLL_INCREMENT, -1, Tk_Offset(TkPathCanvas, yScrollIncrement),
	0, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL,           
	NULL, 0, -1, 0, (ClientData) NULL, 0}
};

static Tk_OptionTable optionTable = NULL;

/*
 * List of all the item types known at present. This is *global* and is
 * protected by typeListMutex.
 */

static Tk_PathItemType *typeList = NULL;/* NULL means initialization hasn't
					 * been done yet. */
TCL_DECLARE_MUTEX(typeListMutex)

#ifndef USE_OLD_TAG_SEARCH
/*
 * Uids for operands in compiled advanced tag search expressions.
 * Initialization is done by GetStaticUids()
 */

typedef struct {
    Tk_Uid allUid;	    /* "all" */
    Tk_Uid currentUid;	    /* "current" */
    Tk_Uid rootUid;	    /* "root" */
    Tk_Uid andUid;
    Tk_Uid orUid;
    Tk_Uid xorUid;
    Tk_Uid parenUid;
    Tk_Uid negparenUid;
    Tk_Uid endparenUid;
    Tk_Uid tagvalUid;
    Tk_Uid negtagvalUid;
} SearchUids;

static Tcl_ThreadDataKey dataKey;
static SearchUids *	GetStaticUids(void);
#endif /* USE_OLD_TAG_SEARCH */

/*
 * Prototypes for functions defined later in this file:
 */

static void		CanvasBindProc(ClientData clientData,
			    XEvent *eventPtr);
static void		CanvasBlinkProc(ClientData clientData);
static void		CanvasCmdDeletedProc(ClientData clientData);
static void		CanvasDoEvent(TkPathCanvas *canvasPtr, XEvent *eventPtr);
static void		CanvasEventProc(ClientData clientData,
			    XEvent *eventPtr);
static int		CanvasFetchSelection(ClientData clientData, int offset,
			    char *buffer, int maxBytes);
static Tk_PathItem *	CanvasFindClosest(TkPathCanvas *canvasPtr,
			    double coords[2]);
static void		CanvasFocusProc(TkPathCanvas *canvasPtr, int gotFocus);
static void		CanvasLostSelection(ClientData clientData);
static void		CanvasSelectTo(TkPathCanvas *canvasPtr,
			    Tk_PathItem *itemPtr, int index);
static void		CanvasSetOrigin(TkPathCanvas *canvasPtr,
			    int xOrigin, int yOrigin);
static void		CanvasUpdateScrollbars(TkPathCanvas *canvasPtr);
static int		CanvasWidgetCmd(ClientData clientData,
			    Tcl_Interp *interp, int argc,
			    Tcl_Obj *CONST *argv);
static void		PathCanvasWorldChanged(
			    ClientData instanceData);
static int		ConfigureCanvas(Tcl_Interp *interp,
			    TkPathCanvas *canvasPtr, int argc,
			    Tcl_Obj *CONST *argv, int flags);
static void		DestroyCanvas(char *memPtr);
static void		DisplayCanvas(ClientData clientData);
static void		DoItem(Tcl_Interp *interp,
			    Tk_PathItem *itemPtr, Tk_Uid tag);
static void		EventuallyRedrawItem(Tk_PathCanvas canvas,
			    Tk_PathItem *itemPtr);

static Tcl_Obj *	UnshareObj(Tcl_Obj *objPtr);
static Tk_PathItem *	ItemIteratorSubNext(Tk_PathItem *itemPtr, Tk_PathItem *groupPtr);
static void		ItemAddToParent(Tk_PathItem *parentPtr, Tk_PathItem *itemPtr);
static void		ItemDelete(TkPathCanvas *canvasPtr, Tk_PathItem *itemPtr);
static int		ItemCreate(Tcl_Interp *interp, TkPathCanvas *canvasPtr, 
				Tk_PathItemType *typePtr, int isRoot, Tk_PathItem **itemPtrPtr, 
				int objc, Tcl_Obj *CONST objv[]);
static int		ItemGetNumTags(Tk_PathItem *itemPtr);
static void		SetAncestorsDirtyBbox(Tk_PathItem *itemPtr);
			    
static void		DebugGetItemInfo(Tk_PathItem *itemPtr, char *s);

#ifdef USE_OLD_TAG_SEARCH
static int		FindItems(Tcl_Interp *interp, TkPathCanvas *canvasPtr,
			    int argc, Tcl_Obj *CONST *argv,
			    Tcl_Obj *newTagObj, int first);
#else /* USE_OLD_TAG_SEARCH */
static int		FindItems(Tcl_Interp *interp, TkPathCanvas *canvasPtr,
			    int argc, Tcl_Obj *CONST *argv,
			    Tcl_Obj *newTagObj, int first,
			    TagSearch **searchPtrPtr);
#endif /* USE_OLD_TAG_SEARCH */
static int		FindArea(Tcl_Interp *interp, TkPathCanvas *canvasPtr,
			    Tcl_Obj *CONST *argv, Tk_Uid uid, int enclosed);
static double		GridAlign(double coord, double spacing);
static CONST char**	TkGetStringsFromObjs(int argc, Tcl_Obj *CONST *objv);
static void		InitCanvas(void);
#ifdef USE_OLD_TAG_SEARCH
static Tk_PathItem *	NextItem(TagSearch *searchPtr);
#endif /* USE_OLD_TAG_SEARCH */
static void		PickCurrentItem(TkPathCanvas *canvasPtr, XEvent *eventPtr);
static Tcl_Obj *	ScrollFractions(int screen1,
			    int screen2, int object1, int object2);
#ifdef USE_OLD_TAG_SEARCH
static void		RelinkItems(TkPathCanvas *canvasPtr,
			    Tcl_Obj *tag, Tk_PathItem *prevPtr);
static Tk_PathItem *	StartTagSearch(TkPathCanvas *canvasPtr,
			    Tcl_Obj *tag, TagSearch *searchPtr);
#else /* USE_OLD_TAG_SEARCH */
static int		RelinkItems(TkPathCanvas *canvasPtr, Tcl_Obj *tag,
			    Tk_PathItem *prevPtr, TagSearch **searchPtrPtr);
static void 		TagSearchExprInit(TagSearchExpr **exprPtrPtr);
static void		TagSearchExprDestroy(TagSearchExpr *expr);
static void		TagSearchDestroy(TagSearch *searchPtr);
static int		TagSearchScan(TkPathCanvas *canvasPtr,
			    Tcl_Obj *tag, TagSearch **searchPtrPtr);
static int		TagSearchScanExpr(Tcl_Interp *interp,
			    TagSearch *searchPtr, TagSearchExpr *expr);
static int		TagSearchEvalExpr(TagSearchExpr *expr,
			    Tk_PathItem *itemPtr);
static Tk_PathItem *	TagSearchFirst(TagSearch *searchPtr);
static Tk_PathItem *	TagSearchNext(TagSearch *searchPtr);
#endif /* USE_OLD_TAG_SEARCH */

/*
 * The structure below defines canvas class behavior by means of functions
 * that can be invoked from generic window code.
 */

static Tk_ClassProcs canvasClass = {
    sizeof(Tk_ClassProcs),	/* size */
    PathCanvasWorldChanged,	/* worldChangedProc */
};

/*
 * Macros that significantly simplify all code that finds items.
 */

#ifdef USE_OLD_TAG_SEARCH

#define FIRST_CANVAS_ITEM_MATCHING(objPtr,searchPtrPtr,errorExitClause) \
    (itemPtr) = StartTagSearch(canvasPtr,(objPtr),&search)
    
#define FOR_EVERY_CANVAS_ITEM_MATCHING(objPtr,searchPtrPtr,errorExitClause) \
    for ((itemPtr) = StartTagSearch(canvasPtr, (objPtr), &search); \
	    (itemPtr) != NULL; (itemPtr) = NextItem(&search))

#else /* USE_OLD_TAG_SEARCH */

#define FIRST_CANVAS_ITEM_MATCHING(objPtr,searchPtrPtr,errorExitClause) \
    if ((result = TagSearchScan(canvasPtr,(objPtr),(searchPtrPtr))) != TCL_OK) { \
	errorExitClause; \
    } \
    itemPtr = TagSearchFirst(*(searchPtrPtr));
    
#define FOR_EVERY_CANVAS_ITEM_MATCHING(objPtr,searchPtrPtr,errorExitClause) \
    if ((result = TagSearchScan(canvasPtr,(objPtr),(searchPtrPtr))) != TCL_OK) { \
	errorExitClause; \
    } \
    for (itemPtr = TagSearchFirst(*(searchPtrPtr)); \
	    itemPtr != NULL; itemPtr = TagSearchNext(*(searchPtrPtr)))

#endif /* USE_OLD_TAG_SEARCH */


/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasObjCmd --
 *
 *	This function is invoked to process the "canvas" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int argc,			/* Number of arguments. */
    Tcl_Obj *CONST argv[])	/* Argument objects. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkPathCanvas *canvasPtr;
    Tk_Window newWin;
    Tk_PathItem *rootItemPtr;

    if (typeList == NULL) {
	InitCanvas();
    }

    if (argc < 2) {
	Tcl_WrongNumArgs(interp, 1, argv, "pathName ?options?");
	return TCL_ERROR;
    }

    newWin = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(argv[1]), NULL);
    if (newWin == NULL) {
	return TCL_ERROR;
    }

    /*
     * Create the option table for this widget class. If it has already been
     * created, the cached pointer will be returned.
     */

    optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    
    /*
     * Initialize fields that won't be initialized by ConfigureCanvas, or
     * which ConfigureCanvas expects to have reasonable values (e.g. resource
     * pointers).
     */

    canvasPtr = (TkPathCanvas *) ckalloc(sizeof(TkPathCanvas));
    canvasPtr->optionTable = optionTable;
    canvasPtr->tkwin = newWin;
    canvasPtr->display = Tk_Display(newWin);
    canvasPtr->interp = interp;
    canvasPtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(canvasPtr->tkwin), CanvasWidgetCmd,
	    (ClientData) canvasPtr, CanvasCmdDeletedProc);
    canvasPtr->rootItemPtr = NULL;  /* root item created below. */
    canvasPtr->borderWidthPtr = NULL;
    canvasPtr->borderWidth = 0;
    canvasPtr->bgBorder = NULL;
    canvasPtr->relief = TK_RELIEF_FLAT;
    canvasPtr->highlightWidthPtr = NULL;
    canvasPtr->highlightWidth = 0;
    canvasPtr->highlightBgColorPtr = NULL;
    canvasPtr->highlightColorPtr = NULL;
    canvasPtr->inset = 0;
    canvasPtr->pixmapGC = None;
    canvasPtr->width = None;
    canvasPtr->height = None;
    canvasPtr->confine = 0;
    canvasPtr->textInfo.selBorder = NULL;
    canvasPtr->textInfo.selBorderWidth = 0;
    canvasPtr->textInfo.selFgColorPtr = NULL;
    canvasPtr->textInfo.selItemPtr = NULL;
    canvasPtr->textInfo.selectFirst = -1;
    canvasPtr->textInfo.selectLast = -1;
    canvasPtr->textInfo.anchorItemPtr = NULL;
    canvasPtr->textInfo.selectAnchor = 0;
    canvasPtr->textInfo.insertBorder = NULL;
    canvasPtr->textInfo.insertWidth = 0;
    canvasPtr->textInfo.insertBorderWidth = 0;
    canvasPtr->textInfo.focusItemPtr = NULL;
    canvasPtr->textInfo.gotFocus = 0;
    canvasPtr->textInfo.cursorOn = 0;
    canvasPtr->insertOnTime = 0;
    canvasPtr->insertOffTime = 0;
    canvasPtr->insertBlinkHandler = (Tcl_TimerToken) NULL;
    canvasPtr->xOrigin = canvasPtr->yOrigin = 0;
    canvasPtr->drawableXOrigin = canvasPtr->drawableYOrigin = 0;
    canvasPtr->bindingTable = NULL;
    canvasPtr->currentItemPtr = NULL;
    canvasPtr->newCurrentPtr = NULL;
    canvasPtr->closeEnough = 0.0;
    canvasPtr->pickEvent.type = LeaveNotify;
    canvasPtr->pickEvent.xcrossing.x = 0;
    canvasPtr->pickEvent.xcrossing.y = 0;
    canvasPtr->state = 0;
    canvasPtr->xScrollCmd = NULL;
    canvasPtr->yScrollCmd = NULL;
    canvasPtr->scrollX1 = 0;
    canvasPtr->scrollY1 = 0;
    canvasPtr->scrollX2 = 0;
    canvasPtr->scrollY2 = 0;
    canvasPtr->regionString = NULL;
    canvasPtr->xScrollIncrement = 0;
    canvasPtr->yScrollIncrement = 0;
    canvasPtr->scanX = 0;
    canvasPtr->scanXOrigin = 0;
    canvasPtr->scanY = 0;
    canvasPtr->scanYOrigin = 0;
    canvasPtr->hotPtr = NULL;
    canvasPtr->hotPrevPtr = NULL;
    canvasPtr->cursor = None;
    canvasPtr->takeFocus = NULL;
    canvasPtr->pixelsPerMM = WidthOfScreen(Tk_Screen(newWin));
    canvasPtr->pixelsPerMM /= WidthMMOfScreen(Tk_Screen(newWin));
    canvasPtr->flags = 0;
    canvasPtr->nextId = 1;	    /* id = 0 reserved for root item */
    canvasPtr->psInfo = NULL;
    canvasPtr->canvas_state = TK_PATHSTATE_NORMAL;
    canvasPtr->tsoffsetPtr = NULL;
    canvasPtr->styleUid = 0;
    canvasPtr->gradientUid = 0;
#ifndef USE_OLD_TAG_SEARCH
    canvasPtr->bindTagExprs = NULL;
#endif

    Tcl_InitHashTable(&canvasPtr->idTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&canvasPtr->styleTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&canvasPtr->gradientTable, TCL_STRING_KEYS);

    Tk_SetClass(canvasPtr->tkwin, "PathCanvas");
    Tk_SetClassProcs(canvasPtr->tkwin, &canvasClass, (ClientData) canvasPtr);
    Tk_CreateEventHandler(canvasPtr->tkwin,
	    ExposureMask|StructureNotifyMask|FocusChangeMask,
	    CanvasEventProc, (ClientData) canvasPtr);
    Tk_CreateEventHandler(canvasPtr->tkwin, KeyPressMask|KeyReleaseMask
	    |ButtonPressMask|ButtonReleaseMask|EnterWindowMask
	    |LeaveWindowMask|PointerMotionMask|VirtualEventMask,
	    CanvasBindProc, (ClientData) canvasPtr);
    Tk_CreateSelHandler(canvasPtr->tkwin, XA_PRIMARY, XA_STRING,
	    CanvasFetchSelection, (ClientData) canvasPtr, XA_STRING);

    if (Tk_InitOptions(interp, (char *) canvasPtr, optionTable, canvasPtr->tkwin)
	    != TCL_OK) {
	Tk_DestroyWindow(canvasPtr->tkwin);
	return TCL_ERROR;
    }
    if (ConfigureCanvas(interp, canvasPtr, argc-2, argv+2, 0) != TCL_OK) {
	goto error;
    }

    /*
     * Create the root item as a group item.
     * Need to set the tag "root" by hand since its configProc
     * forbids this for the root item.
     */
    ItemCreate(interp, canvasPtr, &tkGroupType, 1, &rootItemPtr, 0, NULL);
    rootItemPtr->pathTagsPtr = TkPathAllocTagsFromObj(NULL, 
	    Tcl_NewStringObj("root", -1));
    canvasPtr->rootItemPtr = rootItemPtr;

    Tcl_SetResult(interp, Tk_PathName(canvasPtr->tkwin), TCL_STATIC);
    return TCL_OK;

  error:
    Tk_DestroyWindow(canvasPtr->tkwin);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * CanvasWidgetCmd --
 *
 *	This function is invoked to process the Tcl command that corresponds
 *	to a widget managed by this module. See the user documentation for
 *	details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
CanvasWidgetCmd(
    ClientData clientData,	/* Information about canvas widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* Argument objects. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;
    int c, result;
    Tcl_Obj *resultObjPtr;
    Tk_PathItem *itemPtr = NULL;/* Initialization needed only to prevent
				 * compiler warning. */
#ifdef USE_OLD_TAG_SEARCH
    TagSearch search;
#else /* USE_OLD_TAG_SEARCH */
    TagSearch *searchPtr = NULL;/* Allocated by first TagSearchScan, freed by
				 * TagSearchDestroy */
#endif /* USE_OLD_TAG_SEARCH */

    int index;
    static CONST char *optionStrings[] = {
	"addtag",	"ancestors",	"bbox",		"bind",		    "canvasx",
	"canvasy",	"cget",		"children",	"configure",	    "coords",
	"create",	"dchars",	"delete",	
	"depth",	"distance",	"dtag",
	"find",		"firstchild",	"focus",	"gettags",	    
	"gradient",	"icursor",
	"index",	"insert",	"itemcget",	"itemconfigure",    "lastchild",
	"lower",	"move",		"nextsibling",
	"parent",	"prevsibling",	"postscript",	"raise",
	"scale",	"scan",		"select",	"style",	    
	"type",		"types",
	"xview",	"yview",
#if 1
	"debugtree",
#endif
	NULL
    };
    enum options {
	CANV_ADDTAG,	CANV_ANCESTORS,	    CANV_BBOX,		CANV_BIND,	    CANV_CANVASX,
	CANV_CANVASY,	CANV_CGET,	    CANV_CHILDREN,	CANV_CONFIGURE,	    CANV_COORDS,
	CANV_CREATE,	CANV_DCHARS,	    CANV_DELETE,	
	CANV_DEPTH,	CANV_DISTANCE,	    CANV_DTAG,
	CANV_FIND,	CANV_FIRSTCHILD,    CANV_FOCUS,		CANV_GETTAGS,	    
	CANV_GRADIENT,	CANV_ICURSOR,
	CANV_INDEX,	CANV_INSERT,	    CANV_ITEMCGET,	CANV_ITEMCONFIGURE, CANV_LASTCHILD,
	CANV_LOWER,	CANV_MOVE,	    CANV_NEXTSIBLING,
	CANV_PARENT,	CANV_PREVSIBLING,   CANV_POSTSCRIPT,    CANV_RAISE,
	CANV_SCALE,	CANV_SCAN,	    CANV_SELECT,	CANV_STYLE,	    
	CANV_TYPE,	CANV_TYPES,
	CANV_XVIEW,	CANV_YVIEW,
#if 1
	CANV_DEBUGTREE,
#endif
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) canvasPtr);

    result = TCL_OK;
    switch ((enum options) index) {
    case CANV_ADDTAG: {
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tag searchCommand ?arg arg ...?");
	    result = TCL_ERROR;
	    goto done;
	}
#ifdef USE_OLD_TAG_SEARCH
	result = FindItems(interp, canvasPtr, objc, objv, objv[2], 3);
#else /* USE_OLD_TAG_SEARCH */
	result = FindItems(interp, canvasPtr, objc, objv, objv[2], 3, &searchPtr);
#endif /* USE_OLD_TAG_SEARCH */
	break;
    }
    case CANV_ANCESTORS: {
    
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    Tcl_Obj *listPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	    Tcl_Obj *obj;
	    Tk_PathItem *walkPtr;

	    walkPtr = itemPtr->parentPtr;
	    while (walkPtr != NULL) {
		
		/*
		 * Insert items higher in the tree first.
		 */
		obj = Tcl_NewIntObj(walkPtr->id);
		Tcl_ListObjReplace(NULL, listPtr, 0, 0, 1, &obj);
		walkPtr = walkPtr->parentPtr;
	    }
	    Tcl_SetObjResult(interp, listPtr);
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[2]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}
	break;
    }
    case CANV_BBOX: {
	int i, gotAny;
	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;	/* Initializations needed only
						 * to prevent overcautious
						 * compiler warnings. */

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId ?tagOrId ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	gotAny = 0;
	for (i = 2; i < objc; i++) {
	    FOR_EVERY_CANVAS_ITEM_MATCHING(objv[i], &searchPtr, goto done) {
	    
		/*
		 * Groups bbox are only updated lazily, when needed.
		 */
		if (itemPtr->firstChildPtr != NULL) {
		    TkPathCanvasGroupBbox((Tk_PathCanvas) canvasPtr, itemPtr,
			    &itemPtr->x1, &itemPtr->y1, &itemPtr->x2, &itemPtr->y2);
		}	    
		if ((itemPtr->x1 >= itemPtr->x2)
			|| (itemPtr->y1 >= itemPtr->y2)) {
		    continue;
		}
		if (!gotAny) {
		    x1 = itemPtr->x1;
		    y1 = itemPtr->y1;
		    x2 = itemPtr->x2;
		    y2 = itemPtr->y2;
		    gotAny = 1;
		} else {
		    if (itemPtr->x1 < x1) {
			x1 = itemPtr->x1;
		    }
		    if (itemPtr->y1 < y1) {
			y1 = itemPtr->y1;
		    }
		    if (itemPtr->x2 > x2) {
			x2 = itemPtr->x2;
		    }
		    if (itemPtr->y2 > y2) {
			y2 = itemPtr->y2;
		    }
		}
	    }
	}
	if (gotAny) {
	    Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);

	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(x1));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(y1));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(x2));
	    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(y2));
	    Tcl_SetObjResult(interp, listObj);
	}
	break;
    }
    case CANV_BIND: {
	ClientData object;

	if ((objc < 3) || (objc > 5)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId ?sequence? ?command?");
	    result = TCL_ERROR;
	    goto done;
	}

	/*
	 * Figure out what object to use for the binding (individual item vs.
	 * tag).
	 */

	object = 0;
#ifdef USE_OLD_TAG_SEARCH
	if (isdigit(UCHAR(Tcl_GetString(objv[2])[0]))) {
	    int id;
	    char *end;
	    Tcl_HashEntry *entryPtr;

	    id = strtoul(Tcl_GetString(objv[2]), &end, 0);
	    if (*end != 0) {
		goto bindByTag;
	    }
	    entryPtr = Tcl_FindHashEntry(&canvasPtr->idTable, (char *) id);
	    if (entryPtr != NULL) {
		itemPtr = (Tk_PathItem *) Tcl_GetHashValue(entryPtr);
		object = (ClientData) itemPtr;
	    }

	    if (object == 0) {
		Tcl_AppendResult(interp, "item \"", Tcl_GetString(objv[2]),
			"\" doesn't exist", NULL);
		result = TCL_ERROR;
		goto done;
	    }
	} else {
	    bindByTag:
	    object = (ClientData) Tk_GetUid(Tcl_GetString(objv[2]));
	}
#else /* USE_OLD_TAG_SEARCH */
	result = TagSearchScan(canvasPtr, objv[2], &searchPtr);
	if (result != TCL_OK) {
	    goto done;
	}
	if (searchPtr->type == SEARCH_TYPE_ID) {
	    Tcl_HashEntry *entryPtr;

	    entryPtr = Tcl_FindHashEntry(&canvasPtr->idTable,
		    (char *) INT2PTR(searchPtr->id));
	    if (entryPtr != NULL) {
		itemPtr = (Tk_PathItem *) Tcl_GetHashValue(entryPtr);
		object = (ClientData) itemPtr;
	    }

	    if (object == 0) {
		Tcl_AppendResult(interp, "item \"", Tcl_GetString(objv[2]),
			"\" doesn't exist", NULL);
		result = TCL_ERROR;
		goto done;
	    }
	} else {
    	    object = (ClientData) searchPtr->expr->uid;
	}
#endif /* USE_OLD_TAG_SEARCH */

	/*
	 * Make a binding table if the canvas doesn't already have one.
	 */

	if (canvasPtr->bindingTable == NULL) {
	    canvasPtr->bindingTable = Tk_CreateBindingTable(interp);
	}

	if (objc == 5) {
	    int append = 0;
	    unsigned long mask;
	    char* argv4 = Tcl_GetString(objv[4]);

	    if (argv4[0] == 0) {
		result = Tk_DeleteBinding(interp, canvasPtr->bindingTable,
			object, Tcl_GetString(objv[3]));
		goto done;
	    }
#ifndef USE_OLD_TAG_SEARCH
	    if (searchPtr->type == SEARCH_TYPE_EXPR) {
		/*
		 * If new tag expression, then insert in linked list.
		 */

	    	TagSearchExpr *expr, **lastPtr;

		lastPtr = &(canvasPtr->bindTagExprs);
		while ((expr = *lastPtr) != NULL) {
		    if (expr->uid == searchPtr->expr->uid) {
			break;
		    }
		    lastPtr = &(expr->next);
		}
		if (!expr) {
		    /*
		     * Transfer ownership of expr to bindTagExprs list.
		     */

		    *lastPtr = searchPtr->expr;
		    searchPtr->expr->next = NULL;

		    /*
		     * Flag in TagSearch that expr has changed ownership so
		     * that TagSearchDestroy doesn't try to free it.
		     */

		    searchPtr->expr = NULL;
		}
	    }
#endif /* not USE_OLD_TAG_SEARCH */
	    if (argv4[0] == '+') {
		argv4++;
		append = 1;
	    }
	    mask = Tk_CreateBinding(interp, canvasPtr->bindingTable,
		    object, Tcl_GetString(objv[3]), argv4, append);
	    if (mask == 0) {
		result = TCL_ERROR;
		goto done;
	    }
	    if (mask & (unsigned) ~(ButtonMotionMask|Button1MotionMask
		    |Button2MotionMask|Button3MotionMask|Button4MotionMask
		    |Button5MotionMask|ButtonPressMask|ButtonReleaseMask
		    |EnterWindowMask|LeaveWindowMask|KeyPressMask
		    |KeyReleaseMask|PointerMotionMask|VirtualEventMask)) {
		Tk_DeleteBinding(interp, canvasPtr->bindingTable,
			object, Tcl_GetString(objv[3]));
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "requested illegal events; ",
			"only key, button, motion, enter, leave, and virtual ",
			"events may be used", NULL);
		result = TCL_ERROR;
		goto done;
	    }
	} else if (objc == 4) {
	    CONST char *command;

	    command = Tk_GetBinding(interp, canvasPtr->bindingTable,
		    object, Tcl_GetString(objv[3]));
	    if (command == NULL) {
		CONST char *string;

		string = Tcl_GetStringResult(interp);

		/*
		 * Ignore missing binding errors. This is a special hack that
		 * relies on the error message returned by FindSequence in
		 * tkBind.c.
		 */

		if (string[0] != '\0') {
		    result = TCL_ERROR;
		    goto done;
		} else {
		    Tcl_ResetResult(interp);
		}
	    } else {
		Tcl_SetResult(interp, (char *) command, TCL_STATIC);
	    }
	} else {
	    Tk_GetAllBindings(interp, canvasPtr->bindingTable, object);
	}
	break;
    }
    case CANV_CANVASX: {
	int x;
	double grid;
	char buf[TCL_DOUBLE_SPACE];

	if ((objc < 3) || (objc > 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "screenx ?gridspacing?");
	    result = TCL_ERROR;
	    goto done;
	}
	if (Tk_GetPixelsFromObj(interp, canvasPtr->tkwin, objv[2], &x) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (objc == 4) {
	    if (Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[3],
		    &grid) != TCL_OK) {
		result = TCL_ERROR;
		goto done;
	    }
	} else {
	    grid = 0.0;
	}
	x += canvasPtr->xOrigin;
	Tcl_PrintDouble(interp, GridAlign((double) x, grid), buf);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	break;
    }
    case CANV_CANVASY: {
	int y;
	double grid;
	char buf[TCL_DOUBLE_SPACE];

	if ((objc < 3) || (objc > 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "screeny ?gridspacing?");
	    result = TCL_ERROR;
	    goto done;
	}
	if (Tk_GetPixelsFromObj(interp, canvasPtr->tkwin, objv[2], &y) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	if (objc == 4) {
	    if (Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr,
		    objv[3], &grid) != TCL_OK) {
		result = TCL_ERROR;
		goto done;
	    }
	} else {
	    grid = 0.0;
	}
	y += canvasPtr->yOrigin;
	Tcl_PrintDouble(interp, GridAlign((double) y, grid), buf);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	break;
    }
    case CANV_CGET: {
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    result = TCL_ERROR;
	    goto done;
	}
	resultObjPtr = Tk_GetOptionValue(interp, (char *) canvasPtr,
		canvasPtr->optionTable, objv[2], canvasPtr->tkwin);
	if (resultObjPtr == NULL) {
	    goto done;
	} else {
	    Tcl_SetObjResult(interp, resultObjPtr);
	}
	break;
    }
    case CANV_CHILDREN: {
	Tcl_Obj *listObj;
	Tk_PathItem *childPtr;
	
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    listObj = Tcl_NewListObj(0, NULL);
	    childPtr = itemPtr->firstChildPtr;
	    while (childPtr != NULL) {
		Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(childPtr->id));
		childPtr = childPtr->nextPtr;
	    }
	    Tcl_SetObjResult(interp, listObj);
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[3]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}
	break;
    }
    case CANV_CONFIGURE: {
	if (objc <= 3) {
	    resultObjPtr = Tk_GetOptionInfo(interp, (char *) canvasPtr,
		    canvasPtr->optionTable, (objc == 3) ? objv[2] : NULL,
		    canvasPtr->tkwin);
	    if (resultObjPtr == NULL) {
		goto done;
	    } else {
		Tcl_SetObjResult(interp, resultObjPtr);
	    }
	} else {
	    result = ConfigureCanvas(interp, canvasPtr, objc-2, objv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
	break;
    }
    case CANV_COORDS: {
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId ?x y x y ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    if (objc != 3) {
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    }
	    if (itemPtr->typePtr->coordProc != NULL) {
		result = (*itemPtr->typePtr->coordProc)(interp,
			(Tk_PathCanvas) canvasPtr, itemPtr, objc-3, objv+3);
	    }
	    if (objc != 3) {
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    }
	}
	break;
    }
    case CANV_CREATE: {
	Tk_PathItemType *typePtr;
	Tk_PathItemType *matchPtr = NULL;
	Tk_PathItem *itemPtr;
	char *arg;
	int length;

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "type coords ?arg arg ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	arg = Tcl_GetStringFromObj(objv[2], &length);
	c = arg[0];
	Tcl_MutexLock(&typeListMutex);
	for (typePtr = typeList; typePtr != NULL; typePtr = typePtr->nextPtr) {
	    if ((c == typePtr->name[0])
		    && (strncmp(arg, typePtr->name, (unsigned)length) == 0)) {
		if (matchPtr != NULL) {
		    Tcl_MutexUnlock(&typeListMutex);
		badType:
		    Tcl_AppendResult(interp,
			    "unknown or ambiguous item type \"",arg,"\"",NULL);
		    result = TCL_ERROR;
		    goto done;
		}
		matchPtr = typePtr;
	    }
	}
	/*
	 * Can unlock now because we no longer look at the fields of
	 * the matched item type that are potentially modified by
	 * other threads.
	 */
	Tcl_MutexUnlock(&typeListMutex);
	if (matchPtr == NULL) {
	    goto badType;
	}
	if ((strncmp("group", matchPtr->name, (unsigned)length) != 0) && 
		(objc < 4)) {
	    /*
	     * Allow more specific error return. Groups have no coords.
	     */
	    Tcl_WrongNumArgs(interp, 3, objv, "coords ?arg arg ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	typePtr = matchPtr;
	
	result = ItemCreate(interp, canvasPtr, typePtr, 0, &itemPtr, objc-3, objv+3);
	if (result != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	canvasPtr->hotPtr = itemPtr;
	canvasPtr->hotPrevPtr = itemPtr->prevPtr;

	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	canvasPtr->flags |= REPICK_NEEDED;
	Tcl_SetObjResult(interp, Tcl_NewIntObj(itemPtr->id));
	break;
    }
    case CANV_DCHARS: {
	int first, last;
	int x1,x2,y1,y2;

	if ((objc != 4) && (objc != 5)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId first ?last?");
	    result = TCL_ERROR;
	    goto done;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    if ((itemPtr->typePtr->indexProc == NULL)
		    || (itemPtr->typePtr->dCharsProc == NULL)) {
		continue;
	    }
	    result = itemPtr->typePtr->indexProc(interp,
		    (Tk_PathCanvas) canvasPtr, itemPtr, (char *) objv[3],
		    &first);
	    if (result != TCL_OK) {
		goto done;
	    }
	    if (objc == 5) {
		result = itemPtr->typePtr->indexProc(interp,
			(Tk_PathCanvas) canvasPtr, itemPtr, (char *) objv[4],
			&last);
		if (result != TCL_OK) {
		    goto done;
		}
	    } else {
		last = first;
	    }

	    /*
	     * Redraw both item's old and new areas: it's possible that a
	     * delete could result in a new area larger than the old area.
	     * Except if the insertProc sets the TK_ITEM_DONT_REDRAW flag,
	     * nothing more needs to be done.
	     */

	    x1 = itemPtr->x1; y1 = itemPtr->y1;
	    x2 = itemPtr->x2; y2 = itemPtr->y2;
	    itemPtr->redraw_flags &= ~TK_ITEM_DONT_REDRAW;
	    (*itemPtr->typePtr->dCharsProc)((Tk_PathCanvas) canvasPtr,
		    itemPtr, first, last);
	    if (!(itemPtr->redraw_flags & TK_ITEM_DONT_REDRAW)) {
		Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr,
			x1, y1, x2, y2);
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    }
	    itemPtr->redraw_flags &= ~TK_ITEM_DONT_REDRAW;
	}
	break;
    }
    case CANV_DEBUGTREE: {
	Tk_PathItem *walkPtr, *tmpPtr;
	char tmp[256], info[256], *s;
	int depth;

	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, "");
	    result = TCL_ERROR;
	    goto done;
	}
	for (walkPtr = canvasPtr->rootItemPtr; walkPtr != NULL; 
		walkPtr = TkPathCanvasItemIteratorNext(walkPtr)) {
	    depth = 0;
	    tmpPtr = walkPtr;
	    while (tmpPtr->parentPtr != NULL) {
		depth++;
		tmpPtr = tmpPtr->parentPtr;
	    }
	    if (walkPtr->firstChildPtr != NULL) {
		s = "----";
	    } else {
		s = "";
	    }
	    info[0] = '\0';
	    DebugGetItemInfo(walkPtr, info);
	    sprintf(tmp, "%*d%s\t%s (itemPtr=%p)\n", 4*depth+3, walkPtr->id, s, info, walkPtr);
	    Tcl_WriteChars(Tcl_GetChannel(interp, "stdout", NULL), tmp, -1);
	}
	break;
    }
    case CANV_DELETE: {
	int i;
	
	/*
	 * Since deletinga group item implicitly deletes all its children
	 * we may unintentionally try to delete an item more than once.
	 * We therefore flatten (parent = root) all items first.
	 */
	for (i = 2; i < objc; i++) {
	    FOR_EVERY_CANVAS_ITEM_MATCHING(objv[i], &searchPtr, goto done) {
		if (itemPtr->id == 0) {
		    Tcl_SetObjResult(interp, 
			    Tcl_NewStringObj("the root item cannot be deleted", -1));
		    result = TCL_ERROR;
		    goto done;
		}
		/*
		 * This will also delete all its descendants by 
		 * recursive calls.
		 */
		ItemDelete(canvasPtr, itemPtr);
	    }
	}
	break;
    }
    case CANV_DEPTH: {
    	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}    
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
 	if (itemPtr != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(TkPathCanvasGetDepth(itemPtr)));
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[2]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}
	break;
    }
    case CANV_DISTANCE: {
	double point[2], dist;
	
    	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId x y");
	    result = TCL_ERROR;
	    goto done;
	}
	if ((Tcl_GetDoubleFromObj(interp, objv[3], &point[0]) != TCL_OK) ||
		(Tcl_GetDoubleFromObj(interp, objv[4], &point[1]) != TCL_OK)) {
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    dist = (*itemPtr->typePtr->pointProc)((Tk_PathCanvas) canvasPtr, itemPtr, point);
	    Tcl_SetObjResult(interp, Tcl_NewDoubleObj(dist));
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[2]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}
	break;
    }
    case CANV_DTAG: {
	Tk_PathTags *ptagsPtr;
	Tk_Uid tag;
	int i;

	if ((objc != 3) && (objc != 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId ?tagToDelete?");
	    result = TCL_ERROR;
	    goto done;
	}
	if (objc == 4) {
	    tag = Tk_GetUid(Tcl_GetString(objv[3]));
	} else {
	    tag = Tk_GetUid(Tcl_GetString(objv[2]));
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    ptagsPtr = itemPtr->pathTagsPtr;
	    if (ptagsPtr != NULL) {
		for (i = ptagsPtr->numTags-1; i >= 0; i--) {
		    if (ptagsPtr->tagPtr[i] == tag) {
			ptagsPtr->tagPtr[i] = ptagsPtr->tagPtr[ptagsPtr->numTags-1];
			ptagsPtr->numTags--;
		    }
		}
	    }
	}
	break;
    }
    case CANV_FIND: {
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "searchCommand ?arg arg ...?");
	    result = TCL_ERROR;
	    goto done;
	}
#ifdef USE_OLD_TAG_SEARCH
	result = FindItems(interp, canvasPtr, objc, objv, NULL, 2);
#else /* USE_OLD_TAG_SEARCH */
	result = FindItems(interp, canvasPtr, objc, objv, NULL, 2,
		&searchPtr);
#endif /* USE_OLD_TAG_SEARCH */
	break;
    }
    case CANV_FIRSTCHILD: {
	Tk_PathItem *childPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    childPtr = itemPtr->firstChildPtr;
	    if (childPtr != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(childPtr->id));
	    }
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[3]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}	
	break;
    }
    case CANV_FOCUS: {
	if (objc > 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?tagOrId?");
	    result = TCL_ERROR;
	    goto done;
	}
	itemPtr = canvasPtr->textInfo.focusItemPtr;
	if (objc == 2) {
	    if (itemPtr != NULL) {
		char buf[TCL_INTEGER_SPACE];

		sprintf(buf, "%d", itemPtr->id);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
	    }
	    goto done;
	}
	if ((itemPtr != NULL) && (canvasPtr->textInfo.gotFocus)) {
	    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	}
	if (Tcl_GetString(objv[2])[0] == 0) {
	    canvasPtr->textInfo.focusItemPtr = NULL;
	    goto done;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    if (itemPtr->typePtr->icursorProc != NULL) {
		break;
	    }
	}
	if (itemPtr == NULL) {
	    goto done;
	}
	canvasPtr->textInfo.focusItemPtr = itemPtr;
	if (canvasPtr->textInfo.gotFocus) {
	    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	}
	break;
    }
    case CANV_GETTAGS: {
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    int i;
	    Tk_PathTags *ptagsPtr;
	    
#ifdef USE_OLD_CODE
	    for (i = 0; i < itemPtr->numTags; i++) {
		Tcl_AppendElement(interp, (char *) itemPtr->tagPtr[i]);
	    }
#else
	    ptagsPtr = itemPtr->pathTagsPtr;
	    if (ptagsPtr != NULL) {
		for (i = 0; i < ptagsPtr->numTags; i++) {
		    Tcl_AppendElement(interp, (char *) ptagsPtr->tagPtr[i]);
		}
	    }
#endif
	}
	break;
    }
    case CANV_GRADIENT: {
	result = CanvasGradientObjCmd(interp, canvasPtr, objc, objv);
	break;
    }
    case CANV_ICURSOR: {
	int index;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId index");
	    result = TCL_ERROR;
	    goto done;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    if ((itemPtr->typePtr->indexProc == NULL)
		    || (itemPtr->typePtr->icursorProc == NULL)) {
		goto done;
	    }
	    result = itemPtr->typePtr->indexProc(interp,
		    (Tk_PathCanvas) canvasPtr, itemPtr, (char *) objv[3],
		    &index);
	    if (result != TCL_OK) {
		goto done;
	    }
	    (*itemPtr->typePtr->icursorProc)((Tk_PathCanvas) canvasPtr, itemPtr,
		    index);
	    if ((itemPtr == canvasPtr->textInfo.focusItemPtr)
		    && (canvasPtr->textInfo.cursorOn)) {
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    }
	}
	break;
    }
    case CANV_INDEX: {
	int index;
	char buf[TCL_INTEGER_SPACE];

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId string");
	    result = TCL_ERROR;
	    goto done;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    if (itemPtr->typePtr->indexProc != NULL) {
		break;
	    }
	}
	if (itemPtr == NULL) {
	    Tcl_AppendResult(interp, "can't find an indexable item \"",
		    Tcl_GetString(objv[2]), "\"", NULL);
	    result = TCL_ERROR;
	    goto done;
	}
	result = itemPtr->typePtr->indexProc(interp, (Tk_PathCanvas) canvasPtr,
		itemPtr, (char *) objv[3], &index);
	if (result != TCL_OK) {
	    goto done;
	}
	sprintf(buf, "%d", index);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	break;
    }
    case CANV_INSERT: {
	int beforeThis;
	int x1,x2,y1,y2;

	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId beforeThis string");
	    result = TCL_ERROR;
	    goto done;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    if ((itemPtr->typePtr->indexProc == NULL)
		    || (itemPtr->typePtr->insertProc == NULL)) {
		continue;
	    }
	    result = itemPtr->typePtr->indexProc(interp,
		    (Tk_PathCanvas) canvasPtr, itemPtr, (char *) objv[3],
		    &beforeThis);
	    if (result != TCL_OK) {
		goto done;
	    }

	    /*
	     * Redraw both item's old and new areas: it's possible that an
	     * insertion could result in a new area either larger or smaller
	     * than the old area. Except if the insertProc sets the
	     * TK_ITEM_DONT_REDRAW flag, nothing more needs to be done.
	     */

	    x1 = itemPtr->x1; y1 = itemPtr->y1;
	    x2 = itemPtr->x2; y2 = itemPtr->y2;
	    itemPtr->redraw_flags &= ~TK_ITEM_DONT_REDRAW;
	    (*itemPtr->typePtr->insertProc)((Tk_PathCanvas) canvasPtr,
		    itemPtr, beforeThis, (char *) objv[4]);
	    if (!(itemPtr->redraw_flags & TK_ITEM_DONT_REDRAW)) {
		Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr,
			x1, y1, x2, y2);
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    }
	    itemPtr->redraw_flags &= ~TK_ITEM_DONT_REDRAW;
	}
	break;
    }
    case CANV_ITEMCGET: {
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId option");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    resultObjPtr = Tk_GetOptionValue(canvasPtr->interp, (char *) itemPtr, 
		    itemPtr->optionTable, objv[3], canvasPtr->tkwin);
	    if (resultObjPtr == NULL) {
		result = TCL_ERROR;
		goto done;
	    } else {
		Tcl_SetObjResult(interp, resultObjPtr);
	    }
	}
	break;
    }
    case CANV_ITEMCONFIGURE: {
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId ?option value ...?");
	    result = TCL_ERROR;
	    goto done;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    if (objc <= 4) {
		resultObjPtr = Tk_GetOptionInfo(canvasPtr->interp, (char *) itemPtr, 
			itemPtr->optionTable, (objc == 4) ? objv[3] : NULL, 
			canvasPtr->tkwin);
		if (resultObjPtr == NULL) {
		    result = TCL_ERROR;
		    goto done;
		} else {
		    Tcl_SetObjResult(interp, resultObjPtr);
		}
	    } else {
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
		result = (*itemPtr->typePtr->configProc)(interp,
			(Tk_PathCanvas) canvasPtr, itemPtr, objc-3, objv+3,
			TK_CONFIG_ARGV_ONLY);
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
		canvasPtr->flags |= REPICK_NEEDED;
	    }
	    if ((result != TCL_OK) || (objc < 5)) {
		break;
	    }
	}
	break;
    }
    case CANV_LASTCHILD: {
	Tk_PathItem *childPtr;

	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    childPtr = itemPtr->lastChildPtr;
	    if (childPtr != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(childPtr->id));
	    }
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[3]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}	
	break;
    }
    case CANV_LOWER: {
	Tk_PathItem *itemPtr;

	if ((objc != 3) && (objc != 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId ?belowThis?");
	    result = TCL_ERROR;
	    goto done;
	}

	/*
	 * First find the item just after which we'll insert the named items.
	 */

	if (objc == 3) {
	    itemPtr = NULL;
	} else {
	    FIRST_CANVAS_ITEM_MATCHING(objv[3], &searchPtr, goto done);
	    if (itemPtr == NULL) {
		Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[3]),
			"\" doesn't match any items", NULL);
		goto done;
	    }
	    itemPtr = itemPtr->prevPtr;
	}
#ifdef USE_OLD_TAG_SEARCH
	RelinkItems(canvasPtr, objv[2], itemPtr);
#else /* USE_OLD_TAG_SEARCH */
	result = RelinkItems(canvasPtr, objv[2], itemPtr, &searchPtr);
#endif /* USE_OLD_TAG_SEARCH */
	break;
    }
    case CANV_MOVE: {
	double xAmount, yAmount;

	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId xAmount yAmount");
	    result = TCL_ERROR;
	    goto done;
	}
	if ((Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[3],
		&xAmount) != TCL_OK) || (Tk_PathCanvasGetCoordFromObj(interp,
		(Tk_PathCanvas) canvasPtr, objv[4], &yAmount) != TCL_OK)) {
	    result = TCL_ERROR;
	    goto done;
	}
	
        /* === EB - 22-apr-2010: round the deltas to the nearest integer to avoid round-off errors */
        xAmount = (double)((int)(xAmount + (xAmount > 0 ? 0.5 : -0.5)));
        yAmount = (double)((int)(yAmount + (yAmount > 0 ? 0.5 : -0.5)));
        /* === */

	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    (void) (*itemPtr->typePtr->translateProc)((Tk_PathCanvas) canvasPtr,
		    itemPtr,  xAmount, yAmount);
	    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    canvasPtr->flags |= REPICK_NEEDED;
	}
	break;
    }
    case CANV_NEXTSIBLING: {
	Tk_PathItem *nextPtr;

	// @@@ TODO: add optional argument like TreeCtrl has.
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    nextPtr = itemPtr->nextPtr;
	    if (nextPtr != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(nextPtr->id));
	    }
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[2]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}	
	break;
    }
    case CANV_PARENT: {
	int id;
    
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "id");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {	
	    if (itemPtr->id == 0) {
		id = -1;    // @@@ TODO: What else to return? */
	    } else {
		id = itemPtr->parentPtr->id;
	    }
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(id));
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[2]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}
	break;
    }
    case CANV_POSTSCRIPT: {
	result = TkCanvPostscriptCmd(canvasPtr, interp, objc, objv);
	break;
    }
    case CANV_PREVSIBLING: {
	Tk_PathItem *prevPtr;

	// @@@ TODO: add optional argument like TreeCtrl has.
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    prevPtr = itemPtr->prevPtr;
	    if (prevPtr != NULL) {
		Tcl_SetObjResult(interp, Tcl_NewIntObj(prevPtr->id));
	    }
	} else {
	    Tcl_AppendResult(interp, "tag \"", Tcl_GetString(objv[2]),
		    "\" doesn't match any items", NULL);
	    goto done;
	}	
	break;
    }
    case CANV_RAISE: {
	Tk_PathItem *prevPtr;

	if ((objc != 3) && (objc != 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId ?aboveThis?");
	    result = TCL_ERROR;
	    goto done;
	}

	/*
	 * First find the item just after which we'll insert the named items.
	 */

	if (objc == 3) {
	    prevPtr = canvasPtr->rootItemPtr->lastChildPtr;
	} else {
	    prevPtr = NULL;
	    FOR_EVERY_CANVAS_ITEM_MATCHING(objv[3], &searchPtr, goto done) {
		prevPtr = itemPtr;
	    }
	    if (prevPtr == NULL) {
		Tcl_AppendResult(interp, "tagOrId \"", Tcl_GetString(objv[3]),
			"\" doesn't match any items", NULL);
		result = TCL_ERROR;
		goto done;
	    }
	}
#ifdef USE_OLD_TAG_SEARCH
	RelinkItems(canvasPtr, objv[2], prevPtr);
#else /* USE_OLD_TAG_SEARCH */
	result = RelinkItems(canvasPtr, objv[2], prevPtr, &searchPtr);
#endif /* USE_OLD_TAG_SEARCH */
	break;
    }
    case CANV_SCALE: {
	double xOrigin, yOrigin, xScale, yScale;

	if (objc != 7) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tagOrId xOrigin yOrigin xScale yScale");
	    result = TCL_ERROR;
	    goto done;
	}
	if ((Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr,
		    objv[3], &xOrigin) != TCL_OK)
		|| (Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr,
		    objv[4], &yOrigin) != TCL_OK)
		|| (Tcl_GetDoubleFromObj(interp, objv[5], &xScale) != TCL_OK)
		|| (Tcl_GetDoubleFromObj(interp, objv[6], &yScale) != TCL_OK)) {
	    result = TCL_ERROR;
	    goto done;
	}
	if ((xScale == 0.0) || (yScale == 0.0)) {
	    Tcl_SetResult(interp, "scale factor cannot be zero", TCL_STATIC);
	    result = TCL_ERROR;
	    goto done;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done) {
	    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    (void) (*itemPtr->typePtr->scaleProc)((Tk_PathCanvas) canvasPtr,
		    itemPtr, xOrigin, yOrigin, xScale, yScale);
	    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	    canvasPtr->flags |= REPICK_NEEDED;
	}
	break;
    }
    case CANV_SCAN: {
	int x, y, gain = 10;
	static CONST char *optionStrings[] = {
	    "mark", "dragto", NULL
	};

	if (objc < 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "mark|dragto x y ?dragGain?");
	    result = TCL_ERROR;
	} else if (Tcl_GetIndexFromObj(interp, objv[2], optionStrings,
		"scan option", 0, &index) != TCL_OK) {
	    result = TCL_ERROR;
	} else if ((objc != 5) && (objc != 5+index)) {
	    Tcl_WrongNumArgs(interp, 3, objv, index?"x y ?gain?":"x y");
	    result = TCL_ERROR;
	} else if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK)
		|| (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)){
	    result = TCL_ERROR;
	} else if ((objc == 6) &&
		(Tcl_GetIntFromObj(interp, objv[5], &gain) != TCL_OK)) {
	    result = TCL_ERROR;
	} else if (!index) {
	    canvasPtr->scanX = x;
	    canvasPtr->scanXOrigin = canvasPtr->xOrigin;
	    canvasPtr->scanY = y;
	    canvasPtr->scanYOrigin = canvasPtr->yOrigin;
	} else {
	    int newXOrigin, newYOrigin, tmp;

	    /*
	     * Compute a new view origin for the canvas, amplifying the
	     * mouse motion.
	     */

	    tmp = canvasPtr->scanXOrigin - gain*(x - canvasPtr->scanX)
		    - canvasPtr->scrollX1;
	    newXOrigin = canvasPtr->scrollX1 + tmp;
	    tmp = canvasPtr->scanYOrigin - gain*(y - canvasPtr->scanY)
		    - canvasPtr->scrollY1;
	    newYOrigin = canvasPtr->scrollY1 + tmp;
	    CanvasSetOrigin(canvasPtr, newXOrigin, newYOrigin);
	}
	break;
    }
    case CANV_SELECT: {
	int index, optionindex;
	static CONST char *optionStrings[] = {
	    "adjust", "clear", "from", "item", "to", NULL
	};
	enum options {
	    CANV_ADJUST, CANV_CLEAR, CANV_FROM, CANV_ITEM, CANV_TO
	};

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option ?tagOrId? ?arg?");
	    result = TCL_ERROR;
	    goto done;
	}
	if (objc >= 4) {
	    FOR_EVERY_CANVAS_ITEM_MATCHING(objv[3], &searchPtr, goto done) {
		if ((itemPtr->typePtr->indexProc != NULL)
			&& (itemPtr->typePtr->selectionProc != NULL)){
		    break;
		}
	    }
	    if (itemPtr == NULL) {
		Tcl_AppendResult(interp,
			"can't find an indexable and selectable item \"",
			Tcl_GetString(objv[3]), "\"", NULL);
		result = TCL_ERROR;
		goto done;
	    }
	}
	if (objc == 5) {
	    result = itemPtr->typePtr->indexProc(interp,
		    (Tk_PathCanvas) canvasPtr, itemPtr, (char *) objv[4],
		    &index);
	    if (result != TCL_OK) {
		goto done;
	    }
	}
	if (Tcl_GetIndexFromObj(interp, objv[2], optionStrings,
		"select option", 0, &optionindex) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}
	switch ((enum options) optionindex) {
	case CANV_ADJUST:
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "tagOrId index");
		result = TCL_ERROR;
		goto done;
	    }
	    if (canvasPtr->textInfo.selItemPtr == itemPtr) {
		if (index < (canvasPtr->textInfo.selectFirst
			+ canvasPtr->textInfo.selectLast)/2) {
		    canvasPtr->textInfo.selectAnchor =
			    canvasPtr->textInfo.selectLast + 1;
		} else {
		    canvasPtr->textInfo.selectAnchor =
			    canvasPtr->textInfo.selectFirst;
		}
	    }
	    CanvasSelectTo(canvasPtr, itemPtr, index);
	    break;
	case CANV_CLEAR:
	    if (objc != 3) {
		Tcl_AppendResult(interp, 3, objv, NULL);
		result = TCL_ERROR;
		goto done;
	    }
	    if (canvasPtr->textInfo.selItemPtr != NULL) {
		EventuallyRedrawItem((Tk_PathCanvas) canvasPtr,
			canvasPtr->textInfo.selItemPtr);
		canvasPtr->textInfo.selItemPtr = NULL;
	    }
	    goto done;
	    break;
	case CANV_FROM:
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "tagOrId index");
		result = TCL_ERROR;
		goto done;
	    }
	    canvasPtr->textInfo.anchorItemPtr = itemPtr;
	    canvasPtr->textInfo.selectAnchor = index;
	    break;
	case CANV_ITEM:
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 3, objv, NULL);
		result = TCL_ERROR;
		goto done;
	    }
	    if (canvasPtr->textInfo.selItemPtr != NULL) {
		Tcl_SetObjResult(interp,
			Tcl_NewIntObj(canvasPtr->textInfo.selItemPtr->id));
	    }
	    break;
	case CANV_TO:
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "tagOrId index");
		result = TCL_ERROR;
		goto done;
	    }
	    CanvasSelectTo(canvasPtr, itemPtr, index);
	    break;
	}
	break;
    }
    case CANV_STYLE: {
	result = CanvasStyleObjCmd(interp, canvasPtr, objc, objv);
	break;
    }
    case CANV_TYPE: {
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "tag");
	    result = TCL_ERROR;
	    goto done;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[2], &searchPtr, goto done);
	if (itemPtr != NULL) {
	    Tcl_SetResult(interp, itemPtr->typePtr->name, TCL_STATIC);
	}
	break;
    }
    case CANV_TYPES: {
	Tk_PathItemType *typePtr;
    	Tcl_Obj *listObj;
	
 	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, "");
	    result = TCL_ERROR;
	    goto done;
	}
	listObj = Tcl_NewListObj(0, NULL);
	Tcl_MutexLock(&typeListMutex);
	for (typePtr = typeList; typePtr != NULL; 
		typePtr = typePtr->nextPtr) {
	    Tcl_ListObjAppendElement(interp, listObj, 
		    Tcl_NewStringObj(typePtr->name, -1));
	}
	Tcl_MutexUnlock(&typeListMutex);
	Tcl_SetObjResult(interp, listObj);
	break;
    }
    case CANV_XVIEW: {
	int count, type;
	int newX = 0;		/* Initialization needed only to prevent
				 * gcc warnings. */
	double fraction;

	if (objc == 2) {
	    Tcl_SetObjResult(interp, ScrollFractions(
		    canvasPtr->xOrigin + canvasPtr->inset,
		    canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin)
		    - canvasPtr->inset, canvasPtr->scrollX1,
		    canvasPtr->scrollX2));
	} else {
	    CONST char **args = TkGetStringsFromObjs(objc, objv);
	    type = Tk_GetScrollInfo(interp, objc, args, &fraction, &count);
	    if (args != NULL) {
		ckfree((char *) args);
	    }
	    switch (type) {
	    case TK_SCROLL_ERROR:
		result = TCL_ERROR;
		goto done;
	    case TK_SCROLL_MOVETO:
		newX = canvasPtr->scrollX1 - canvasPtr->inset
			+ (int) (fraction * (canvasPtr->scrollX2
			- canvasPtr->scrollX1) + 0.5);
		break;
	    case TK_SCROLL_PAGES:
		newX = (int) (canvasPtr->xOrigin + count * .9
			* (Tk_Width(canvasPtr->tkwin) - 2*canvasPtr->inset));
		break;
	    case TK_SCROLL_UNITS:
		if (canvasPtr->xScrollIncrement > 0) {
		    newX = canvasPtr->xOrigin
			    + count*canvasPtr->xScrollIncrement;
		} else {
		    newX = (int) (canvasPtr->xOrigin + count * .1
			    * (Tk_Width(canvasPtr->tkwin)
			    - 2*canvasPtr->inset));
		}
		break;
	    }
	    CanvasSetOrigin(canvasPtr, newX, canvasPtr->yOrigin);
	}
	break;
    }
    case CANV_YVIEW: {
	int count, type;
	int newY = 0;		/* Initialization needed only to prevent
				 * gcc warnings. */
	double fraction;

	if (objc == 2) {
	    Tcl_SetObjResult(interp, ScrollFractions(
		    canvasPtr->yOrigin + canvasPtr->inset,
		    canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin)
		    - canvasPtr->inset,
		    canvasPtr->scrollY1, canvasPtr->scrollY2));
	} else {
	    CONST char **args = TkGetStringsFromObjs(objc, objv);
	    type = Tk_GetScrollInfo(interp, objc, args, &fraction, &count);
	    if (args != NULL) {
		ckfree((char *) args);
	    }
	    switch (type) {
	    case TK_SCROLL_ERROR:
		result = TCL_ERROR;
		goto done;
	    case TK_SCROLL_MOVETO:
		newY = canvasPtr->scrollY1 - canvasPtr->inset
			+ (int) (fraction*(canvasPtr->scrollY2
			- canvasPtr->scrollY1) + 0.5);
		break;
	    case TK_SCROLL_PAGES:
		newY = (int) (canvasPtr->yOrigin + count * .9
			* (Tk_Height(canvasPtr->tkwin)
			- 2*canvasPtr->inset));
		break;
	    case TK_SCROLL_UNITS:
		if (canvasPtr->yScrollIncrement > 0) {
		    newY = canvasPtr->yOrigin
			    + count*canvasPtr->yScrollIncrement;
		} else {
		    newY = (int) (canvasPtr->yOrigin + count * .1
			    * (Tk_Height(canvasPtr->tkwin)
			    - 2*canvasPtr->inset));
		}
		break;
	    }
	    CanvasSetOrigin(canvasPtr, canvasPtr->xOrigin, newY);
	}
	break;
    }
    }

  done:
#ifndef USE_OLD_TAG_SEARCH
    TagSearchDestroy(searchPtr);
#endif /* not USE_OLD_TAG_SEARCH */
    Tcl_Release((ClientData) canvasPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyCanvas --
 *
 *	This function is invoked by Tcl_EventuallyFree or Tcl_Release to clean
 *	up the internal structure of a canvas at a safe time (when no-one is
 *	using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the canvas is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyCanvas(
    char *memPtr)		/* Info about canvas widget. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) memPtr;
    Tk_PathItem *itemPtr, *prevItemPtr, *lastPtr = NULL;
#ifndef USE_OLD_TAG_SEARCH
    TagSearchExpr *expr, *next;
#endif

    /*
     * Free up all of the items in the canvas.
     * NB: We need to traverse the tree from the last item
     *     until reached the root item.
     */

    for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	lastPtr = itemPtr;
    }
    for (itemPtr = lastPtr; itemPtr != NULL; ) {
        prevItemPtr = TkPathCanvasItemIteratorPrev(itemPtr);
	(*itemPtr->typePtr->deleteProc)((Tk_PathCanvas) canvasPtr, itemPtr,
		canvasPtr->display);
	ckfree((char *) itemPtr);
        itemPtr = prevItemPtr;
    }

    /*
     * Free up all the stuff that requires special handling, then let
     * Tk_FreeOptions handle all the standard option-related stuff.
     */

    Tcl_DeleteHashTable(&canvasPtr->idTable);
    
    // @@@ TODO: tkwin = NULL!
    PathStylesFree(canvasPtr->tkwin, &canvasPtr->styleTable);
    Tcl_DeleteHashTable(&canvasPtr->styleTable);
    
    CanvasGradientsFree(canvasPtr);
    Tcl_DeleteHashTable(&canvasPtr->gradientTable);
    
    if (canvasPtr->pixmapGC != None) {
	Tk_FreeGC(canvasPtr->display, canvasPtr->pixmapGC);
    }
#ifndef USE_OLD_TAG_SEARCH
    expr = canvasPtr->bindTagExprs;
    while (expr) {
	next = expr->next;
	TagSearchExprDestroy(expr);
	expr = next;
    }
#endif /* USE_OLD_TAG_SEARCH */
    Tcl_DeleteTimerHandler(canvasPtr->insertBlinkHandler);
    if (canvasPtr->bindingTable != NULL) {
	Tk_DeleteBindingTable(canvasPtr->bindingTable);
    }    
    Tk_FreeConfigOptions((char *) canvasPtr, canvasPtr->optionTable,
	    canvasPtr->tkwin);
    canvasPtr->tkwin = NULL;
    ckfree((char *) canvasPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureCanvas --
 *
 *	This function is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a canvas
 *	widget.
 *
 * Results:
 *	The return value is a standard Tcl result. If TCL_ERROR is returned,
 *	then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width, etc. get set
 *	for canvasPtr; old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureCanvas(
    Tcl_Interp *interp,		/* Used for error reporting. */
    TkPathCanvas *canvasPtr,	/* Information about widget; may or may not
				 * already have values for some fields. */
    int objc,			/* Number of valid entries in objv. */
    Tcl_Obj *CONST objv[],	/* Argument objects. */
    int flags)			/* Flags to pass to Tk_ConfigureWidget. */
{
    XGCValues gcValues;
    GC newGC;
    Tk_SavedOptions savedOptions;
    Tcl_Obj *errorResult = NULL;
    int error;

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

	    if (Tk_SetOptions(interp, (char *) canvasPtr,
		    canvasPtr->optionTable, objc, objv,
		    canvasPtr->tkwin, &savedOptions, NULL) != TCL_OK) {
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
	if ((canvasPtr->flags & CANVAS_DELETED)) {

	    /*
	     * From tkButton.c_
	     * Somehow canvas was deleted - just abort now. [Bug #824479]
	     */
	    return TCL_ERROR;
	}
   
	/*
	 * Recompute the scroll region.
	 */
	 // @@@ TODO: Revise this code since I'm not completely sure!!!

	canvasPtr->scrollX1 = 0;
	canvasPtr->scrollY1 = 0;
	canvasPtr->scrollX2 = 0;
	canvasPtr->scrollY2 = 0;
	if (canvasPtr->regionString != NULL) {
	    int argc2;
	    CONST char **argv2;

	    if (Tcl_SplitList(canvasPtr->interp, canvasPtr->regionString,
		    &argc2, &argv2) != TCL_OK) {
		ckfree((char *) argv2);
		continue;
	    }
	    if (argc2 != 4) {
		Tcl_AppendResult(interp, "bad scrollRegion \"",
			canvasPtr->regionString, "\"", NULL);
		ckfree((char *) argv2);
		continue;
	    }
	    if ((Tk_GetPixels(canvasPtr->interp, canvasPtr->tkwin,
			argv2[0], &canvasPtr->scrollX1) != TCL_OK)
		    || (Tk_GetPixels(canvasPtr->interp, canvasPtr->tkwin,
			argv2[1], &canvasPtr->scrollY1) != TCL_OK)
		    || (Tk_GetPixels(canvasPtr->interp, canvasPtr->tkwin,
			argv2[2], &canvasPtr->scrollX2) != TCL_OK)
		    || (Tk_GetPixels(canvasPtr->interp, canvasPtr->tkwin,
			argv2[3], &canvasPtr->scrollY2) != TCL_OK)) {
		ckfree((char *) argv2);
		continue;
	    }
	}
   
        /*
	 * A few options need special processing, such as setting the background
	 * from a 3-D border and creating a GC for copying bits to the screen.
	 */
	 
	Tk_SetBackgroundFromBorder(canvasPtr->tkwin, canvasPtr->bgBorder);

	if (canvasPtr->highlightWidth < 0) {
	    canvasPtr->highlightWidth = 0;
	}
	canvasPtr->inset = canvasPtr->borderWidth + canvasPtr->highlightWidth;

	gcValues.function = GXcopy;
	gcValues.graphics_exposures = False;
	gcValues.foreground = Tk_3DBorderColor(canvasPtr->bgBorder)->pixel;
	newGC = Tk_GetGC(canvasPtr->tkwin,
		GCFunction|GCGraphicsExposures|GCForeground, &gcValues);
	if (canvasPtr->pixmapGC != None) {
	    Tk_FreeGC(canvasPtr->display, canvasPtr->pixmapGC);
	}
	canvasPtr->pixmapGC = newGC;

	/*
	 * Reset the desired dimensions for the window.
	 */

	Tk_GeometryRequest(canvasPtr->tkwin, canvasPtr->width + 2*canvasPtr->inset,
		canvasPtr->height + 2*canvasPtr->inset);

	/*
	 * Restart the cursor timing sequence in case the on-time or off-time just
	 * changed.
	 */

	if (canvasPtr->textInfo.gotFocus) {
	    CanvasFocusProc(canvasPtr, 1);
	}
   
	// @@@ TODO: I don't see anywhere this is used. Nothing in man page. */
	if (canvasPtr->tsoffsetPtr != NULL) {
	    flags = canvasPtr->tsoffsetPtr->flags;
	    if (flags & TK_OFFSET_LEFT) {
		canvasPtr->tsoffsetPtr->xoffset = 0;
	    } else if (flags & TK_OFFSET_CENTER) {
		canvasPtr->tsoffsetPtr->xoffset = canvasPtr->width/2;
	    } else if (flags & TK_OFFSET_RIGHT) {
		canvasPtr->tsoffsetPtr->xoffset = canvasPtr->width;
	    }
	    if (flags & TK_OFFSET_TOP) {
		canvasPtr->tsoffsetPtr->yoffset = 0;
	    } else if (flags & TK_OFFSET_MIDDLE) {
		canvasPtr->tsoffsetPtr->yoffset = canvasPtr->height/2;
	    } else if (flags & TK_OFFSET_BOTTOM) {
		canvasPtr->tsoffsetPtr->yoffset = canvasPtr->height;
	    }
	}
	
	/*
	 * If we reach this on the first pass we are OK and continue below.
	 */
	break;
    }
    if (!error) {
	Tk_FreeSavedOptions(&savedOptions);
    }
    
    /*
     * Reset the canvas's origin (this is a no-op unless confine mode has just
     * been turned on or the scroll region has changed).
     */

    CanvasSetOrigin(canvasPtr, canvasPtr->xOrigin, canvasPtr->yOrigin);
    canvasPtr->flags |= UPDATE_SCROLLBARS|REDRAW_BORDERS;
    Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr,
	    canvasPtr->xOrigin, canvasPtr->yOrigin,
	    canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin),
	    canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin));
    if (error) {
	Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);
	return TCL_ERROR;
    } else {
	return TCL_OK;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PathCanvasWorldChanged --
 *
 *	This function is called when the world has changed in some way and the
 *	widget needs to recompute all its graphics contexts and determine its
 *	new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Configures all items in the canvas with a empty argc/argv, for the
 *	side effect of causing all the items to recompute their geometry and
 *	to be redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
PathCanvasWorldChanged(
    ClientData instanceData)	/* Information about widget. */
{
    TkPathCanvas *canvasPtr;
    Tk_PathItem *itemPtr;
    int result;

    canvasPtr = (TkPathCanvas *) instanceData;
    itemPtr = canvasPtr->rootItemPtr;
    for ( ; itemPtr != NULL; itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	result = (*itemPtr->typePtr->configProc)(canvasPtr->interp,
		(Tk_PathCanvas) canvasPtr, itemPtr, 0, NULL,
		TK_CONFIG_ARGV_ONLY);
	if (result != TCL_OK) {
	    Tcl_ResetResult(canvasPtr->interp);
	}
    }
    canvasPtr->flags |= REPICK_NEEDED;
    Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr,
	    canvasPtr->xOrigin, canvasPtr->yOrigin,
	    canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin),
	    canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin));
}

/*
 * This is a very crude trick to get TkpClipDrawableToRect.
 */ 
#ifdef TK_PATH_NO_DOUBLE_BUFFERING

void
TkpClipDrawableToRect(
    Display *display,
    Drawable d,
    int x, int y,
    int width, int height)
{
    MacDrawable *macDraw = (MacDrawable *) d;

    if (width < 0 && height < 0) {
	macDraw->drawRect = CGRectNull;
	macDraw->flags &= ~TK_CLIPPED_DRAW;
    } else {
	macDraw->drawRect = CGRectMake(x, y, width, height);
	macDraw->flags |= TK_CLIPPED_DRAW;
    }
}

#endif

/*
 *--------------------------------------------------------------
 *
 * DisplayCanvas --
 *
 *	This function redraws the contents of a canvas window. It is invoked
 *	as a do-when-idle handler, so it only runs when there's nothing else
 *	for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayCanvas(
    ClientData clientData)	/* Information about widget. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;
    Tk_Window tkwin = canvasPtr->tkwin;
    Tk_PathItem *itemPtr;
    Pixmap pixmap;
    int screenX1, screenX2, screenY1, screenY2, width, height;
    int flags;

    if (canvasPtr->flags & CANVAS_DELETED) {
	return;
    }
    if (!Tk_IsMapped(tkwin)) {
	goto done;
    }

    /*
     * Choose a new current item if that is needed (this could cause event
     * handlers to be invoked).
     */

    while (canvasPtr->flags & REPICK_NEEDED) {
	Tcl_Preserve((ClientData) canvasPtr);
	canvasPtr->flags &= ~REPICK_NEEDED;
	PickCurrentItem(canvasPtr, &canvasPtr->pickEvent);
	flags = canvasPtr->flags;
	Tcl_Release((ClientData) canvasPtr);
	if (flags & CANVAS_DELETED) {
	    return;
	}
    }

    /*
     * Scan through the item list, registering the bounding box for all items
     * that didn't do that for the final coordinates yet. This can be
     * determined by the FORCE_REDRAW flag.
     */

    for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	if (itemPtr->redraw_flags & FORCE_REDRAW) {
	    itemPtr->redraw_flags &= ~FORCE_REDRAW;
	    EventuallyRedrawItem((Tk_PathCanvas)canvasPtr, itemPtr);
	    itemPtr->redraw_flags &= ~FORCE_REDRAW;
	}
    }
    
    /*
     * Compute the intersection between the area that needs redrawing and the
     * area that's visible on the screen.
     */

    if ((canvasPtr->redrawX1 < canvasPtr->redrawX2)
	    && (canvasPtr->redrawY1 < canvasPtr->redrawY2)) {
	screenX1 = canvasPtr->xOrigin + canvasPtr->inset;
	screenY1 = canvasPtr->yOrigin + canvasPtr->inset;
	screenX2 = canvasPtr->xOrigin + Tk_Width(tkwin) - canvasPtr->inset;
	screenY2 = canvasPtr->yOrigin + Tk_Height(tkwin) - canvasPtr->inset;
	if (canvasPtr->redrawX1 > screenX1) {
	    screenX1 = canvasPtr->redrawX1;
	}
	if (canvasPtr->redrawY1 > screenY1) {
	    screenY1 = canvasPtr->redrawY1;
	}
	if (canvasPtr->redrawX2 < screenX2) {
	    screenX2 = canvasPtr->redrawX2;
	}
	if (canvasPtr->redrawY2 < screenY2) {
	    screenY2 = canvasPtr->redrawY2;
	}
	if ((screenX1 >= screenX2) || (screenY1 >= screenY2)) {
	    goto borders;
	}

	width = screenX2 - screenX1;
	height = screenY2 - screenY1;

#ifndef TK_PATH_NO_DOUBLE_BUFFERING
	/*
	 * Redrawing is done in a temporary pixmap that is allocated here and
	 * freed at the end of the function. All drawing is done to the
	 * pixmap, and the pixmap is copied to the screen at the end of the
	 * function. The temporary pixmap serves two purposes:
	 *
	 * 1. It provides a smoother visual effect (no clearing and gradual
	 *    redraw will be visible to users).
	 * 2. It allows us to redraw only the objects that overlap the redraw
	 *    area. Otherwise incorrect results could occur from redrawing
	 *    things that stick outside of the redraw area (we'd have to
	 *    redraw everything in order to make the overlaps look right).
	 *
	 * Some tricky points about the pixmap:
	 *
	 * 1. We only allocate a large enough pixmap to hold the area that has
	 *    to be redisplayed. This saves time in in the X server for large
	 *    objects that cover much more than the area being redisplayed:
	 *    only the area of the pixmap will actually have to be redrawn.
	 * 2. Some X servers (e.g. the one for DECstations) have troubles with
	 *    with characters that overlap an edge of the pixmap (on the DEC
	 *    servers, as of 8/18/92, such characters are drawn one pixel too
	 *    far to the right). To handle this problem, make the pixmap a bit
	 *    larger than is absolutely needed so that for normal-sized fonts
	 *    the characters that overlap the edge of the pixmap will be
	 *    outside the area we care about.
	 */

	canvasPtr->drawableXOrigin = screenX1 - 30;
	canvasPtr->drawableYOrigin = screenY1 - 30;
	pixmap = Tk_GetPixmap(Tk_Display(tkwin), Tk_WindowId(tkwin),
	    (screenX2 + 30 - canvasPtr->drawableXOrigin),
	    (screenY2 + 30 - canvasPtr->drawableYOrigin),
	    Tk_Depth(tkwin));
#else
	canvasPtr->drawableXOrigin = canvasPtr->xOrigin;
	canvasPtr->drawableYOrigin = canvasPtr->yOrigin;
	pixmap = Tk_WindowId(tkwin);
	TkpClipDrawableToRect(Tk_Display(tkwin), pixmap,
		screenX1 - canvasPtr->xOrigin, screenY1 - canvasPtr->yOrigin,
		width, height);
#endif /* TK_PATH_NO_DOUBLE_BUFFERING */

	/*
	 * Clear the area to be redrawn.
	 */

	XFillRectangle(Tk_Display(tkwin), pixmap, canvasPtr->pixmapGC,
		screenX1 - canvasPtr->drawableXOrigin,
		screenY1 - canvasPtr->drawableYOrigin, (unsigned int) width,
		(unsigned int) height);

	/*
	 * Scan through the item list, redrawing those items that need it. An
	 * item must be redraw if either (a) it intersects the smaller
	 * on-screen area or (b) it intersects the full canvas area and its
	 * type requests that it be redrawn always (e.g. so subwindows can be
	 * unmapped when they move off-screen).
	 */

	for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
		itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	    if ((itemPtr->x1 >= screenX2)
		    || (itemPtr->y1 >= screenY2)
		    || (itemPtr->x2 < screenX1)
		    || (itemPtr->y2 < screenY1)) {
		if (!(itemPtr->typePtr->alwaysRedraw & 1)
			|| (itemPtr->x1 >= canvasPtr->redrawX2)
			|| (itemPtr->y1 >= canvasPtr->redrawY2)
			|| (itemPtr->x2 < canvasPtr->redrawX1)
			|| (itemPtr->y2 < canvasPtr->redrawY1)) {
		    continue;
		}
	    }
	    if (itemPtr->state == TK_PATHSTATE_HIDDEN ||
		(itemPtr->state == TK_PATHSTATE_NULL &&
		 canvasPtr->canvas_state == TK_PATHSTATE_HIDDEN)) {
		continue;
	    }
	    (*itemPtr->typePtr->displayProc)((Tk_PathCanvas) canvasPtr, itemPtr,
		    canvasPtr->display, pixmap, screenX1, screenY1, width,
		    height);
	}

#ifndef TK_PATH_NO_DOUBLE_BUFFERING
	/*
	 * Copy from the temporary pixmap to the screen, then free up the
	 * temporary pixmap.
	 */

	XCopyArea(Tk_Display(tkwin), pixmap, Tk_WindowId(tkwin),
		canvasPtr->pixmapGC,
		screenX1 - canvasPtr->drawableXOrigin,
		screenY1 - canvasPtr->drawableYOrigin,
		(unsigned int) width, (unsigned int) height,
		screenX1 - canvasPtr->xOrigin, screenY1 - canvasPtr->yOrigin);
	Tk_FreePixmap(Tk_Display(tkwin), pixmap);
#else
	TkpClipDrawableToRect(Tk_Display(tkwin), pixmap, 0, 0, -1, -1);
#endif /* TK_PATH_NO_DOUBLE_BUFFERING */
    }

    /*
     * Draw the window borders, if needed.
     */

  borders:
    if (canvasPtr->flags & REDRAW_BORDERS) {
	canvasPtr->flags &= ~REDRAW_BORDERS;
	if (canvasPtr->borderWidth > 0) {
	    Tk_Draw3DRectangle(tkwin, Tk_WindowId(tkwin),
		    canvasPtr->bgBorder, canvasPtr->highlightWidth,
		    canvasPtr->highlightWidth,
		    Tk_Width(tkwin) - 2*canvasPtr->highlightWidth,
		    Tk_Height(tkwin) - 2*canvasPtr->highlightWidth,
		    canvasPtr->borderWidth, canvasPtr->relief);
	}
	if (canvasPtr->highlightWidth != 0) {
	    GC fgGC, bgGC;

	    bgGC = Tk_GCForColor(canvasPtr->highlightBgColorPtr,
		    Tk_WindowId(tkwin));
	    if (canvasPtr->textInfo.gotFocus) {
		fgGC = Tk_GCForColor(canvasPtr->highlightColorPtr,
			Tk_WindowId(tkwin));
	    	TkpDrawHighlightBorder(tkwin, fgGC, bgGC,
			canvasPtr->highlightWidth, Tk_WindowId(tkwin));
	    } else {
	    	TkpDrawHighlightBorder(tkwin, bgGC, bgGC,
			canvasPtr->highlightWidth, Tk_WindowId(tkwin));
	    }
	}
    }

  done:
    canvasPtr->flags &= ~(REDRAW_PENDING|BBOX_NOT_EMPTY);
    canvasPtr->redrawX1 = canvasPtr->redrawX2 = 0;
    canvasPtr->redrawY1 = canvasPtr->redrawY2 = 0;
    if (canvasPtr->flags & UPDATE_SCROLLBARS) {
	CanvasUpdateScrollbars(canvasPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * CanvasEventProc --
 *
 *	This function is invoked by the Tk dispatcher for various events on
 *	canvases.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up.
 *	When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
CanvasEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;

    if (eventPtr->type == Expose) {
	int x, y;

	x = eventPtr->xexpose.x + canvasPtr->xOrigin;
	y = eventPtr->xexpose.y + canvasPtr->yOrigin;
	Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr, x, y,
		x + eventPtr->xexpose.width,
		y + eventPtr->xexpose.height);
	if ((eventPtr->xexpose.x < canvasPtr->inset)
		|| (eventPtr->xexpose.y < canvasPtr->inset)
		|| ((eventPtr->xexpose.x + eventPtr->xexpose.width)
		    > (Tk_Width(canvasPtr->tkwin) - canvasPtr->inset))
		|| ((eventPtr->xexpose.y + eventPtr->xexpose.height)
		    > (Tk_Height(canvasPtr->tkwin) - canvasPtr->inset))) {
	    canvasPtr->flags |= REDRAW_BORDERS;
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (!(canvasPtr->flags & CANVAS_DELETED)) {
	    canvasPtr->flags |= CANVAS_DELETED;
	    Tcl_DeleteCommandFromToken(canvasPtr->interp,
		    canvasPtr->widgetCmd);
	    if (canvasPtr->flags & REDRAW_PENDING) {
		Tcl_CancelIdleCall(DisplayCanvas, (ClientData) canvasPtr);
	    }
	    Tcl_EventuallyFree((ClientData) canvasPtr,
		    (Tcl_FreeProc *) DestroyCanvas);
	}
    } else if (eventPtr->type == ConfigureNotify) {
	canvasPtr->flags |= UPDATE_SCROLLBARS;

	/*
	 * The call below is needed in order to recenter the canvas if it's
	 * confined and its scroll region is smaller than the window.
	 */

	CanvasSetOrigin(canvasPtr, canvasPtr->xOrigin, canvasPtr->yOrigin);
	Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr, canvasPtr->xOrigin,
		canvasPtr->yOrigin,
		canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin),
		canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin));
	canvasPtr->flags |= REDRAW_BORDERS;
    } else if (eventPtr->type == FocusIn) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    CanvasFocusProc(canvasPtr, 1);
	}
    } else if (eventPtr->type == FocusOut) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    CanvasFocusProc(canvasPtr, 0);
	}
    } else if (eventPtr->type == UnmapNotify) {
	Tk_PathItem *itemPtr;

	/*
	 * Special hack: if the canvas is unmapped, then must notify all items
	 * with "alwaysRedraw" set, so that they know that they are no longer
	 * displayed.
	 */

	for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
		itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	    if (itemPtr->typePtr->alwaysRedraw & 1) {
		(*itemPtr->typePtr->displayProc)((Tk_PathCanvas) canvasPtr,
			itemPtr, canvasPtr->display, None, 0, 0, 0, 0);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CanvasCmdDeletedProc --
 *
 *	This function is invoked when a widget command is deleted. If the
 *	widget isn't already in the process of being destroyed, this command
 *	destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
CanvasCmdDeletedProc(
    ClientData clientData)	/* Pointer to widget record for widget. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;

    /*
     * This function could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this function destroys the
     * widget.
     */

    if (!(canvasPtr->flags & CANVAS_DELETED)) {
 	Tk_DestroyWindow(canvasPtr->tkwin);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasEventuallyRedraw --
 *
 *	Arrange for part or all of a canvas widget to redrawn at some
 *	convenient time in the future.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The screen will eventually be refreshed.
 *
 *--------------------------------------------------------------
 */

void
Tk_PathCanvasEventuallyRedraw(
    Tk_PathCanvas canvas,	/* Information about widget. */
    int x1, int y1,		/* Upper left corner of area to redraw. Pixels
				 * on edge are redrawn. */
    int x2, int y2)		/* Lower right corner of area to redraw.
				 * Pixels on edge are not redrawn. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    Tk_Window tkwin = canvasPtr->tkwin;

    if ((canvasPtr->flags & CANVAS_DELETED) || !Tk_IsMapped(tkwin)) {
	return;
    }
    if ((x1 >= x2) || (y1 >= y2) ||
 	    (x2 < canvasPtr->xOrigin) || (y2 < canvasPtr->yOrigin) ||
	    (x1 >= canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin)) ||
	    (y1 >= canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin))) {
	return;
    }
    if (canvasPtr->flags & BBOX_NOT_EMPTY) {
	if (x1 <= canvasPtr->redrawX1) {
	    canvasPtr->redrawX1 = x1;
	}
	if (y1 <= canvasPtr->redrawY1) {
	    canvasPtr->redrawY1 = y1;
	}
	if (x2 >= canvasPtr->redrawX2) {
	    canvasPtr->redrawX2 = x2;
	}
	if (y2 >= canvasPtr->redrawY2) {
	    canvasPtr->redrawY2 = y2;
	}
    } else {
	canvasPtr->redrawX1 = x1;
	canvasPtr->redrawY1 = y1;
	canvasPtr->redrawX2 = x2;
	canvasPtr->redrawY2 = y2;
	canvasPtr->flags |= BBOX_NOT_EMPTY;
    }
    if (!(canvasPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayCanvas, (ClientData) canvasPtr);
	canvasPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *--------------------------------------------------------------
 *
 * EventuallyRedrawItem --
 *
 *	Arrange for part or all of a canvas widget to redrawn at some
 *	convenient time in the future.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The screen will eventually be refreshed.
 *
 *--------------------------------------------------------------
 */

static void
EventuallyRedrawItem(
    Tk_PathCanvas canvas,		/* Information about widget. */
    Tk_PathItem *itemPtr)		/* Item to be redrawn. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    if ((itemPtr->x1 >= itemPtr->x2) || (itemPtr->y1 >= itemPtr->y2) ||
 	    (itemPtr->x2 < canvasPtr->xOrigin) ||
	    (itemPtr->y2 < canvasPtr->yOrigin) ||
	    (itemPtr->x1 >= canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin)) ||
	    (itemPtr->y1 >= canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin))) {
	if (!(itemPtr->typePtr->alwaysRedraw & 1)) {
	    return;
	}
    }
    if (!(itemPtr->redraw_flags & FORCE_REDRAW)) {
	if (canvasPtr->flags & BBOX_NOT_EMPTY) {
	    if (itemPtr->x1 <= canvasPtr->redrawX1) {
		canvasPtr->redrawX1 = itemPtr->x1;
	    }
	    if (itemPtr->y1 <= canvasPtr->redrawY1) {
		canvasPtr->redrawY1 = itemPtr->y1;
	    }
	    if (itemPtr->x2 >= canvasPtr->redrawX2) {
		canvasPtr->redrawX2 = itemPtr->x2;
	    }
	    if (itemPtr->y2 >= canvasPtr->redrawY2) {
		canvasPtr->redrawY2 = itemPtr->y2;
	    }
	} else {
	    canvasPtr->redrawX1 = itemPtr->x1;
	    canvasPtr->redrawY1 = itemPtr->y1;
	    canvasPtr->redrawX2 = itemPtr->x2;
	    canvasPtr->redrawY2 = itemPtr->y2;
	    canvasPtr->flags |= BBOX_NOT_EMPTY;
	}
	itemPtr->redraw_flags |= FORCE_REDRAW;
    }
    SetAncestorsDirtyBbox(itemPtr);
    if (!(canvasPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayCanvas, (ClientData) canvasPtr);
	canvasPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GroupItemConfigured --
 *
 *	Schedules all children of a group for redisplay in a recursive way.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A number of items scheduled for redisplay.
 *
 *----------------------------------------------------------------------
 */

void
GroupItemConfigured(Tk_PathCanvas canvas, Tk_PathItem *itemPtr, int mask)
{
    Tk_PathItem *walkPtr;
    
    for (walkPtr = itemPtr->firstChildPtr; walkPtr != NULL; walkPtr = walkPtr->nextPtr) {
	EventuallyRedrawItem(canvas, walkPtr);
	if (walkPtr->typePtr->bboxProc != NULL) {
	    (*walkPtr->typePtr->bboxProc)(canvas, walkPtr, mask);
	    /*
	     * Only if the item responds to the bboxProc we need to redraw it.
	     */
	    EventuallyRedrawItem(canvas, walkPtr);
	}
	if (walkPtr->typePtr == &tkGroupType) {
	    /*
	     * Call ourself recursively for each group.
	     * @@@ An alternative would be to have this call in the group's
	     *     own bbox proc.
	     */
	    GroupItemConfigured(canvas, walkPtr, mask);
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * Tk_CreatePathItemType --
 *
 *	This function may be invoked to add a new kind of canvas element to
 *	the core item types supported by Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, the new item type will be useable in canvas widgets
 *	(e.g. typePtr->name can be used as the item type in "create" widget
 *	commands). If there was already a type with the same name as in
 *	typePtr, it is replaced with the new type.
 *
 *--------------------------------------------------------------
 */

void
Tk_CreatePathItemType(
    Tk_PathItemType *typePtr)	/* Information about item type; storage must
				 * be statically allocated (must live
				 * forever). */
{
    Tk_PathItemType *typePtr2, *prevPtr;

    if (typeList == NULL) {
	InitCanvas();
    }

    /*
     * If there's already an item type with the given name, remove it.
     */

    Tcl_MutexLock(&typeListMutex);
    for (typePtr2 = typeList, prevPtr = NULL; typePtr2 != NULL;
	    prevPtr = typePtr2, typePtr2 = typePtr2->nextPtr) {
	if (strcmp(typePtr2->name, typePtr->name) == 0) {
	    if (prevPtr == NULL) {
		typeList = typePtr2->nextPtr;
	    } else {
		prevPtr->nextPtr = typePtr2->nextPtr;
	    }
	    break;
	}
    }
    typePtr->nextPtr = typeList;
    typeList = typePtr;
    Tcl_MutexUnlock(&typeListMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_PathGetItemTypes --
 *
 *	This function returns a pointer to the list of all item types. Note
 *	that this is inherently thread-unsafe, but since item types are only
 *	ever registered very rarely this is unlikely to be a problem in
 *	practice.
 *
 * Results:
 *	The return value is a pointer to the first in the list of item types
 *	currently supported by canvases.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_PathItemType *
Tk_PathGetItemTypes(void)
{
    if (typeList == NULL) {
	InitCanvas();
    }
    return typeList;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathCanvasSetParent --
 *
 *	Appends an item as the last sibling to a parent item.
 *	May unlink any existing linkage.
 *
 * Results:
 *	Standard tcl result.
 *
 * Side effects:
 *	Links in item in display list.
 *
 *----------------------------------------------------------------------
 */

void
TkPathCanvasSetParent(Tk_PathItem *parentPtr, Tk_PathItem *itemPtr)
{

    /*
     * Unlink any present parent, then link in again.
     */
    if (itemPtr->parentPtr != NULL) {
	TkPathCanvasItemDetach(itemPtr);
    }
    ItemAddToParent(parentPtr, itemPtr);
    
    /* 
     * We may have configured -parent with a tag but need to return an id. 
     */
    itemPtr->parentObj = UnshareObj(itemPtr->parentObj);
    Tcl_SetIntObj(itemPtr->parentObj, parentPtr->id);
}

void
CanvasSetParentToRoot(Tk_PathItem *itemPtr)
{
    Tk_PathItemEx *itemExPtr = (Tk_PathItemEx *)itemPtr;
    Tk_PathCanvas canvas = itemExPtr->canvas;
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    TkPathCanvasSetParent(canvasPtr->rootItemPtr, itemPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathCanvasFindGroup --
 *
 *	Searches for the first group item described by the tagOrId parentObj.
 *
 * Results:
 *	Standard tcl result. parentPtrPtr filled in on success.
 *
 * Side effects:
 *	Leaves any error result in interp.
 *
 *----------------------------------------------------------------------
 */

int
TkPathCanvasFindGroup(Tcl_Interp *interp, Tk_PathCanvas canvas, 
	Tcl_Obj *parentObj, Tk_PathItem **parentPtrPtr)
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    Tk_PathItem *parentPtr;
    int result = TCL_OK;
    TagSearch *searchPtr = NULL;/* Allocated by first TagSearchScan, freed by
				 * TagSearchDestroy */

    if (parentObj != NULL) {
        if ((result = TagSearchScan(canvasPtr, parentObj, &searchPtr)) != TCL_OK) {
	    return TCL_ERROR;
	}
	parentPtr = TagSearchFirst(searchPtr);
	if (parentPtr == NULL) {
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
		    "tag \"", Tcl_GetString(parentObj),
		    "\" doesn't match any items", NULL);
	    result = TCL_ERROR;
	} else if (strcmp(parentPtr->typePtr->name, "group") != 0) {
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
		    "tag \"", Tcl_GetString(parentObj),
		    "\" is not a group item", NULL);
	    result = TCL_ERROR;
	} else {
	    *parentPtrPtr = parentPtr;
	}
	TagSearchDestroy(searchPtr);
    }
    return result;
}

void	    
CanvasTranslateGroup(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
	double deltaX, double deltaY)
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    Tk_PathItem *walkPtr;
    
    /* === EB - 22-apr-2010: round the deltas to the nearest integer to avoid round-off errors */
    deltaX = (double)((int)(deltaX + (deltaX > 0 ? 0.5 : -0.5)));
    deltaY = (double)((int)(deltaY + (deltaY > 0 ? 0.5 : -0.5)));
    /* === */
    
    /*
     * Invoke all its childs translateProc. Any child groups will call this
     * function recursively.
     */
    for (walkPtr = itemPtr->firstChildPtr; walkPtr != NULL; walkPtr = walkPtr->nextPtr) {
	EventuallyRedrawItem(canvas, walkPtr);
	(void) (*walkPtr->typePtr->translateProc)(canvas, walkPtr, deltaX, deltaY);
	EventuallyRedrawItem(canvas, walkPtr);
	canvasPtr->flags |= REPICK_NEEDED;
    }
}

void	    
CanvasScaleGroup(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
	double originX, double originY, double scaleX, double scaleY)
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) canvas;
    Tk_PathItem *walkPtr;
    
    /*
     * Invoke all its childs scaleProc. Any child groups will call this
     * function recursively.
     */
    for (walkPtr = itemPtr->firstChildPtr; walkPtr != NULL; walkPtr = walkPtr->nextPtr) {
	EventuallyRedrawItem(canvas, walkPtr);
	(void) (*walkPtr->typePtr->scaleProc)(canvas, walkPtr, 
		originX, originY, scaleX, scaleY);
	EventuallyRedrawItem(canvas, walkPtr);
	canvasPtr->flags |= REPICK_NEEDED;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetAncestorsDirtyBbox --
 *
 *	Used by items when they need a redisplay for some reason
 *	so that its ancestor groups know that they need to compute
 *	a new bbox when requested.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Groups get their dirty bbox flag set.
 *
 *----------------------------------------------------------------------
 */

static void
SetAncestorsDirtyBbox(Tk_PathItem *itemPtr)
{
    Tk_PathItem *walkPtr;

    walkPtr = itemPtr->parentPtr;
    while (walkPtr != NULL) {
	TkPathCanvasSetGroupDirtyBbox(walkPtr);
	walkPtr = walkPtr->parentPtr;
    }
}

void
TkPathCanvasGroupBbox(Tk_PathCanvas canvas, Tk_PathItem *itemPtr,
	int *x1Ptr, int *y1Ptr, int *x2Ptr, int *y2Ptr)
{
    int gotAny = 0;
    Tk_PathItem *walkPtr;
    int x1 = -1, y1 = -1, x2 = -1, y2 = -1;
   
    for (walkPtr = itemPtr->firstChildPtr; walkPtr != NULL; 
	    walkPtr = walkPtr->nextPtr) {

	/* 
	 * Make sure sub groups have its bbox updated. 
	 * We may be called recursively.
	 */
	if (walkPtr->firstChildPtr != NULL) {
	    TkPathCanvasUpdateGroupBbox(canvas, walkPtr);
	}
	if ((walkPtr->x1 >= walkPtr->x2)
		|| (walkPtr->y1 >= walkPtr->y2)) {
	    continue;
	}
	if (!gotAny) {
	    x1 = walkPtr->x1;
	    y1 = walkPtr->y1;
	    x2 = walkPtr->x2;
	    y2 = walkPtr->y2;
	    gotAny = 1;
	} else {
	    if (walkPtr->x1 < x1) { x1 = walkPtr->x1; }
	    if (walkPtr->y1 < y1) { y1 = walkPtr->y1; }
	    if (walkPtr->x2 > x2) { x2 = walkPtr->x2; }
	    if (walkPtr->y2 > y2) { y2 = walkPtr->y2; }
	}
    }
    *x1Ptr = x1, *y1Ptr = y1, *x2Ptr = x2, *y2Ptr = y2;
}

/*
 *----------------------------------------------------------------------
 *
 * InitCanvas --
 *
 *	This function is invoked to perform once-only-ever initialization for
 *	the module, such as setting up the type table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
InitCanvas(void)
{
    Tcl_MutexLock(&typeListMutex);
    if (typeList != NULL) {
	Tcl_MutexUnlock(&typeListMutex);
	return;
    }
    typeList = &tkRectangleType;
    tkRectangleType.nextPtr = &tkTextType;
    tkTextType.nextPtr = &tkLineType;
    tkLineType.nextPtr = &tkPolygonType;
    tkPolygonType.nextPtr = &tkImageType;
    tkImageType.nextPtr = &tkOvalType;
    tkOvalType.nextPtr = &tkBitmapType;
    tkBitmapType.nextPtr = &tkArcType;
    tkArcType.nextPtr = &tkWindowType;
    tkWindowType.nextPtr = NULL;
    
    /* 
     * tkpath specific item types.
     */ 

    tkWindowType.nextPtr = &tkPathType;
    tkPathType.nextPtr = &tkPrectType;
    tkPrectType.nextPtr = &tkPlineType;
    tkPlineType.nextPtr = &tkPolylineType;
    tkPolylineType.nextPtr = &tkPpolygonType;
    tkPpolygonType.nextPtr = &tkCircleType;
    tkCircleType.nextPtr = &tkEllipseType;
    tkEllipseType.nextPtr = &tkPimageType;
    tkPimageType.nextPtr = &tkPtextType;
    tkPtextType.nextPtr = &tkGroupType;
    tkGroupType.nextPtr = NULL;
   
    Tcl_MutexUnlock(&typeListMutex);
}

/*
 *--------------------------------------------------------------
 *
 * ItemCreate --
 *
 *	Creates a new item, configures it, and add it to the display tree.
 *	It remains for the calling code to call EventuallyRedrawItem
 *	and a few more canvas admin stuff.
 *	The item's creatProc may leave any error message in interp.
 *
 * Results:
 *	Standard Tcl result and a new item pointer in itemPtrPtr.
 *
 * Side effects:
 *	Item allocated, configured, and linked into display list.
 *
 *--------------------------------------------------------------
 */

static int
ItemCreate(Tcl_Interp *interp, TkPathCanvas *canvasPtr, 
	Tk_PathItemType *typePtr, int isRoot, Tk_PathItem **itemPtrPtr, 
	int objc, Tcl_Obj *CONST objv[])
{
    Tk_PathItem *itemPtr;
    Tcl_HashEntry *entryPtr;
    int isNew = 0;
    int result;

    itemPtr = (Tk_PathItem *) ckalloc((unsigned) typePtr->itemSize);
    if (isRoot) {
	itemPtr->id = 0;
    } else {
	itemPtr->id = canvasPtr->nextId;
	canvasPtr->nextId++;
    }
    itemPtr->typePtr = typePtr;
    itemPtr->state = TK_PATHSTATE_NULL;
    itemPtr->redraw_flags = 0;
    itemPtr->optionTable = NULL;
    itemPtr->pathTagsPtr = NULL;
    itemPtr->nextPtr = NULL;
    itemPtr->prevPtr = NULL;
    itemPtr->firstChildPtr = NULL;
    itemPtr->lastChildPtr = NULL;
    
    /* 
     * This is just to be able to detect if createProc processes
     * any -parent option.
     * NB: It is absolutely vital to set parentObj to NULL
     *     else option free bails.
     */
    itemPtr->parentPtr = NULL;
    itemPtr->parentObj = NULL;
    
    result = (*typePtr->createProc)(interp, (Tk_PathCanvas) canvasPtr,
	    itemPtr, objc, objv);
    if (result != TCL_OK) {
	ckfree((char *) itemPtr);
	return TCL_ERROR;
    }
    entryPtr = Tcl_CreateHashEntry(&canvasPtr->idTable,
	    (char *) INT2PTR(itemPtr->id), &isNew);
    Tcl_SetHashValue(entryPtr, itemPtr);

    /*
     * If item's createProc didn't put it in the display list we do.
     * Typically done only for the tk::canvas items which don't have
     * a -parent option.
     */
    if (!isRoot && (itemPtr->parentPtr == NULL)) {
	ItemAddToParent(canvasPtr->rootItemPtr, itemPtr);
    }
    itemPtr->redraw_flags |= FORCE_REDRAW;
    *itemPtrPtr = itemPtr;
    
    return TCL_OK;
}

static Tcl_Obj *UnshareObj(Tcl_Obj *objPtr)
{
    if (Tcl_IsShared(objPtr)) {
	Tcl_Obj *newObj = Tcl_DuplicateObj(objPtr);
	Tcl_DecrRefCount(objPtr);
	Tcl_IncrRefCount(newObj);
	return newObj;
    }
    return objPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathCanvasItemIteratorNext --
 *
 *	Convinience function to obtain the next item in the item tree.
 *
 * Results:
 *	Tk_PathItem pointer.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

Tk_PathItem *	
TkPathCanvasItemIteratorNext(Tk_PathItem *itemPtr)
{
    if (itemPtr->firstChildPtr != NULL) {
	return itemPtr->firstChildPtr;
    } 
    while (itemPtr->nextPtr == NULL) {
	itemPtr = itemPtr->parentPtr;
	if (itemPtr == NULL) {	    /* root item */
	    return NULL;
	}
    }
    return itemPtr->nextPtr;
}

Tk_PathItem *	
TkPathCanvasItemIteratorPrev(Tk_PathItem *itemPtr)
{
    Tk_PathItem *walkPtr;
    
    if (itemPtr->parentPtr == NULL) {	/* root item */
	return NULL;
    } else {
	walkPtr = itemPtr->parentPtr;
	if (itemPtr->prevPtr != NULL) {
	    walkPtr = itemPtr->prevPtr;
	    while (walkPtr != NULL && walkPtr->lastChildPtr != NULL) {
		walkPtr = walkPtr->lastChildPtr;
	    }
	}
	return walkPtr;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ItemIteratorSubNext --
 *
 *	Convinience function to obtain the next item in the item tree.
 *	It is similar to ItemIteratorSubNext except that it is limited
 *	to descendants of groupPtr.
 *
 * Results:
 *	Tk_PathItem pointer.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static Tk_PathItem *	
ItemIteratorSubNext(Tk_PathItem *itemPtr, Tk_PathItem *groupPtr)
{
    Tk_PathItem *stopPtr = groupPtr->parentPtr;
    
    if (itemPtr->firstChildPtr != NULL) {
	return itemPtr->firstChildPtr;
    } 
    while (itemPtr->nextPtr == NULL) {
	itemPtr = itemPtr->parentPtr;
	if (itemPtr == NULL) {	    /* root item */
	    return NULL;
	} else if (stopPtr == itemPtr->parentPtr) {
	    return NULL;
	}
    }
    return itemPtr->nextPtr;
}

static int		
ItemGetNumTags(Tk_PathItem *itemPtr)
{
    if (itemPtr->pathTagsPtr != NULL) {
	return itemPtr->pathTagsPtr->numTags;
    } else {
	return 0;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkPathCanvasItemDetach --
 *
 *	Splice out (unlink) an item from the display list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Item will be unlinked from the display list.
 *
 *--------------------------------------------------------------
 */

void		
TkPathCanvasItemDetach(Tk_PathItem *itemPtr)
{
    Tk_PathItem *parentPtr;
    
    if (itemPtr->prevPtr != NULL) {
	itemPtr->prevPtr->nextPtr = itemPtr->nextPtr;
    }
    if (itemPtr->nextPtr != NULL) {
	itemPtr->nextPtr->prevPtr = itemPtr->prevPtr;
    }
    parentPtr = itemPtr->parentPtr;
    if ((parentPtr != NULL) && (parentPtr->firstChildPtr == itemPtr)) {
	parentPtr->firstChildPtr = itemPtr->nextPtr;
	if (parentPtr->firstChildPtr == NULL) {
	    parentPtr->lastChildPtr = NULL;
	}
    }
    if ((parentPtr != NULL) && (parentPtr->lastChildPtr == itemPtr)) {
	parentPtr->lastChildPtr = itemPtr->prevPtr;
    }
    
    /* 
     * This signals an orfan item. 
     */
    itemPtr->nextPtr = itemPtr->prevPtr = itemPtr->parentPtr = NULL;
}

/*
 *--------------------------------------------------------------
 *
 * ItemAddToParent --
 *
 *	Appends an item as the last sibling to a parent item.
 *	It doesn't do any unlinking from a previous tree position.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Display list updated.
 *
 *--------------------------------------------------------------
 */

static void		
ItemAddToParent(Tk_PathItem *parentPtr, Tk_PathItem *itemPtr)
{
    itemPtr->nextPtr = NULL;
    itemPtr->prevPtr = parentPtr->lastChildPtr;
    if (parentPtr->lastChildPtr != NULL) {
	parentPtr->lastChildPtr->nextPtr = itemPtr;
    } else {
	parentPtr->firstChildPtr = itemPtr;
    }
    parentPtr->lastChildPtr = itemPtr;
    itemPtr->parentPtr = parentPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ItemDelete --
 *
 *	Recursively frees all resources associated with an Item and its
 *	descendants and removes it from display list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Items are removed from their parent and freed.
 *
 *----------------------------------------------------------------------
 */

static void
ItemDelete(TkPathCanvas *canvasPtr, Tk_PathItem *itemPtr)
{
    Tcl_HashEntry *entryPtr;
    
    /*
     * Remove any children by recursively calling us.
     * NB: This is very tricky code! Children updates
     *     the itemPtr->firstChildPtr here via calls
     *     to TkPathCanvasItemDetach.
     */
    while (itemPtr->firstChildPtr != NULL) {
	ItemDelete(canvasPtr, itemPtr->firstChildPtr);
    }

    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
    if (canvasPtr->bindingTable != NULL) {
	Tk_DeleteAllBindings(canvasPtr->bindingTable,
			     (ClientData) itemPtr);
    }
    
    /*
     * The item type deleteProc is responsible for calling 
     * Tk_FreeConfigOptions which will implicitly also clean up
     * the Tk_PathTags via its custom free proc.
     */
    (*itemPtr->typePtr->deleteProc)((Tk_PathCanvas) canvasPtr, itemPtr,
				    canvasPtr->display);

    entryPtr = Tcl_FindHashEntry(&canvasPtr->idTable,
				 (char *) INT2PTR(itemPtr->id));
    Tcl_DeleteHashEntry(entryPtr);
    TkPathCanvasItemDetach(itemPtr);
    
    if (itemPtr == canvasPtr->currentItemPtr) {
	canvasPtr->currentItemPtr = NULL;
	canvasPtr->flags |= REPICK_NEEDED;
    }
    if (itemPtr == canvasPtr->newCurrentPtr) {
	canvasPtr->newCurrentPtr = NULL;
	canvasPtr->flags |= REPICK_NEEDED;
    }
    if (itemPtr == canvasPtr->textInfo.focusItemPtr) {
	canvasPtr->textInfo.focusItemPtr = NULL;
    }
    if (itemPtr == canvasPtr->textInfo.selItemPtr) {
	canvasPtr->textInfo.selItemPtr = NULL;
    }
    if ((itemPtr == canvasPtr->hotPtr)
	    || (itemPtr == canvasPtr->hotPrevPtr)) {
	canvasPtr->hotPtr = NULL;
    }
    ckfree((char *) itemPtr);
}

static void
DebugGetItemInfo(Tk_PathItem *itemPtr, char *s)
{
    Tk_PathItem *p = itemPtr;
    char tmp[256];
    
    sprintf(tmp, " parentPtr->id=%d\t", (p->parentPtr ? p->parentPtr->id : -1));
    strcat(s, tmp);
    sprintf(tmp, " prevPtr->id=%d\t", (p->prevPtr ? p->prevPtr->id : -1));
    strcat(s, tmp);
    sprintf(tmp, " nextPtr->id=%d\t", (p->nextPtr ? p->nextPtr->id : -1));
    strcat(s, tmp);
    sprintf(tmp, " firstChildPtr->id=%d\t", (p->firstChildPtr ? p->firstChildPtr->id : -1));
    strcat(s, tmp);
    sprintf(tmp, " lastChildPtr->id=%d\t", (p->lastChildPtr ? p->lastChildPtr->id : -1));
    strcat(s, tmp);
}
        
#ifdef USE_OLD_TAG_SEARCH
/*
 *--------------------------------------------------------------
 *
 * StartTagSearch --
 *
 *	This function is called to initiate an enumeration of all items in a
 *	given canvas that contain a given tag.
 *
 * Results:
 *	The return value is a pointer to the first item in canvasPtr that
 *	matches tag, or NULL if there is no such item. The information at
 *	*searchPtr is initialized such that successive calls to NextItem will
 *	return successive items that match tag.
 *
 * Side effects:
 *	SearchPtr is linked into a list of searches in progress on canvasPtr,
 *	so that elements can safely be deleted while the search is in
 *	progress. EndTagSearch must be called at the end of the search to
 *	unlink searchPtr from this list.
 *
 *--------------------------------------------------------------
 */

static Tk_PathItem *
StartTagSearch(
    TkPathCanvas *canvasPtr,	/* Canvas whose items are to be searched. */
    Tcl_Obj *tagObj,		/* Object giving tag value. */
    TagSearch *searchPtr)	/* Record describing tag search; will be
				 * initialized here. */
{
    int id;
    Tk_PathItem *itemPtr, *lastPtr;
    Tk_Uid *tagPtr;
    Tk_Uid uid;
    char *tag = Tcl_GetString(tagObj);
    int count;
    TkWindow *tkwin;
    TkDisplay *dispPtr;
    Tk_PathTags *ptagsPtr;

    tkwin = (TkWindow *) canvasPtr->tkwin;
    dispPtr = tkwin->dispPtr;

    /*
     * Initialize the search.
     */

    searchPtr->canvasPtr = canvasPtr;
    searchPtr->searchOver = 0;

    /*
     * Find the first matching item in one of several ways. If the tag is a
     * number then it selects the single item with the matching identifier.
     * In this case see if the item being requested is the hot item, in which
     * case the search can be skipped.
     */

    if (isdigit(UCHAR(*tag))) {
	char *end;
	Tcl_HashEntry *entryPtr;

	dispPtr->numIdSearches++;
	id = strtoul(tag, &end, 0);
	if (*end == 0) {
	    itemPtr = canvasPtr->hotPtr;
	    lastPtr = canvasPtr->hotPrevPtr;
	    if ((itemPtr == NULL) || (itemPtr->id != id) || (lastPtr == NULL)
		    || (TkPathCanvasItemIteratorNext(lastPtr) != itemPtr)) {
		dispPtr->numSlowSearches++;
		entryPtr = Tcl_FindHashEntry(&canvasPtr->idTable, (char *) id);
		if (entryPtr != NULL) {
		    itemPtr = (Tk_PathItem *)Tcl_GetHashValue(entryPtr);
		    lastPtr = itemPtr->prevPtr;
		} else {
		    lastPtr = itemPtr = NULL;
		}
	    }
	    searchPtr->lastPtr = lastPtr;
	    searchPtr->searchOver = 1;
	    canvasPtr->hotPtr = itemPtr;
	    canvasPtr->hotPrevPtr = lastPtr;
	    return itemPtr;
	}
    }

    searchPtr->tag = uid = Tk_GetUid(tag);
    if (uid == Tk_GetUid("all")) {
	/*
	 * All items match.
	 */

	searchPtr->tag = NULL;
	searchPtr->lastPtr = NULL;
	searchPtr->currentPtr = canvasPtr->rootItemPtr;
	return canvasPtr->rootItemPtr;
    } else if (uid == Tk_GetUid("root")) {
	itemPtr = canvasPtr->rootItemPtr;
	lastPtr = NULL;
	searchPtr->searchOver = 1;
	searchPtr->currentPtr = itemPtr:
	searchPtr->lastPtr = lastPtr;
	canvasPtr->hotPtr = itemPtr;
	canvasPtr->hotPrevPtr = lastPtr;
	return itemPtr;
    }

    /*
     * None of the above. Search for an item with a matching tag.
     */
    for (lastPtr = NULL, itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    lastPtr = itemPtr, itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	ptagsPtr = itemPtr->pathTagsPtr;
	if (ptagsPtr != NULL) {
	    for (tagPtr = ptagsPtr->tagPtr, count = ptagsPtr->numTags;
		    count > 0; tagPtr++, count--) {
		if (*tagPtr == uid) {
		    searchPtr->lastPtr = lastPtr;
		    searchPtr->currentPtr = itemPtr;
		    return itemPtr;
		}
	    }
	}
    }
    searchPtr->lastPtr = lastPtr;
    searchPtr->searchOver = 1;
    return NULL;
}

/*
 *--------------------------------------------------------------
 *
 * NextItem --
 *
 *	This function returns successive items that match a given tag; it
 *	should be called only after StartTagSearch has been used to begin a
 *	search.
 *
 * Results:
 *	The return value is a pointer to the next item that matches the tag
 *	specified to StartTagSearch, or NULL if no such item exists.
 *	*SearchPtr is updated so that the next call to this function will
 *	return the next item.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static Tk_PathItem *
NextItem(
    TagSearch *searchPtr)	/* Record describing search in progress. */
{
    Tk_PathItem *itemPtr, *lastPtr;
    int count;
    Tk_Uid uid;
    Tk_Uid *tagPtr;
    Tk_PathTags *ptagsPtr;

    /*
     * Find next item in list (this may not actually be a suitable one to
     * return), and return if there are no items left.
     */

    lastPtr = searchPtr->lastPtr;
    if (lastPtr == NULL) {
	itemPtr = searchPtr->canvasPtr->rootItemPtr;
    } else {
	itemPtr = TkPathCanvasItemIteratorNext(lastPtr);
    }

    if ((itemPtr == NULL) || (searchPtr->searchOver)) {
	searchPtr->searchOver = 1;
	return NULL;
    }
    if (itemPtr != searchPtr->currentPtr) {
	/*
	 * The structure of the list has changed. Probably the previously-
	 * returned item was removed from the list. In this case, don't
	 * advance lastPtr; just return its new successor (i.e. do nothing
	 * here).
	 */
    } else {
	lastPtr = itemPtr;
	itemPtr = TkPathCanvasItemIteratorNext(lastPtr);
    }

    /*
     * Handle special case of "all" search by returning next item.
     */

    uid = searchPtr->tag;
    if (uid == NULL) {
	searchPtr->lastPtr = lastPtr;
	searchPtr->currentPtr = itemPtr;
	return itemPtr;
    }

    /*
     * Look for an item with a particular tag.
     */
    for ( ; itemPtr != NULL; lastPtr = itemPtr, itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	ptagsPtr = itemPtr->pathTagsPtr;
	if (ptagsPtr != NULL) {
	    for (tagPtr = ptagsPtr->tagPtr, count = ptagsPtr->numTags;
		 count > 0; tagPtr++, count--) {
		if (*tagPtr == uid) {
		    searchPtr->lastPtr = lastPtr;
		    searchPtr->currentPtr = itemPtr;
		    return itemPtr;
		}
	    }
	}
    }
    searchPtr->lastPtr = lastPtr;
    searchPtr->searchOver = 1;
    return NULL;
}

#else /* !USE_OLD_TAG_SEARCH */
/*
 *----------------------------------------------------------------------
 *
 * GetStaticUids --
 *
 *	This function is invoked to return a structure filled with the Uids
 *	used when doing tag searching. If it was never before called in the
 *	current thread, it initializes the structure for that thread (uids are
 *	only ever local to one thread [Bug 1114977]).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static SearchUids *
GetStaticUids(void)
{
    SearchUids *searchUids = (SearchUids *)
	    Tcl_GetThreadData(&dataKey, sizeof(SearchUids));

    if (searchUids->allUid == NULL) {
	searchUids->allUid       = Tk_GetUid("all");
	searchUids->currentUid   = Tk_GetUid("current");
	searchUids->rootUid	 = Tk_GetUid("root");
	searchUids->andUid       = Tk_GetUid("&&");
	searchUids->orUid        = Tk_GetUid("||");
	searchUids->xorUid       = Tk_GetUid("^");
	searchUids->parenUid     = Tk_GetUid("(");
	searchUids->endparenUid  = Tk_GetUid(")");
	searchUids->negparenUid  = Tk_GetUid("!(");
	searchUids->tagvalUid    = Tk_GetUid("!!");
	searchUids->negtagvalUid = Tk_GetUid("!");
    }
    return searchUids;
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchExprInit --
 *
 *	This function allocates and initializes one TagSearchExpr struct.
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static void
TagSearchExprInit(
    TagSearchExpr **exprPtrPtr)
{
    TagSearchExpr* expr = *exprPtrPtr;

    if (! expr) {
	expr = (TagSearchExpr *) ckalloc(sizeof(TagSearchExpr));
	expr->allocated = 0;
	expr->uids = NULL;
	expr->next = NULL;
    }
    expr->uid = NULL;
    expr->index = 0;
    expr->length = 0;
    *exprPtrPtr = expr;
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchExprDestroy --
 *
 *	This function destroys one TagSearchExpr structure.
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static void
TagSearchExprDestroy(
    TagSearchExpr *expr)
{
    if (expr) {
    	if (expr->uids) {
	    ckfree((char *)expr->uids);
	}
	ckfree((char *)expr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchScan --
 *
 *	This function is called to initiate an enumeration of all items in a
 *	given canvas that contain a tag that matches the tagOrId expression.
 *
 * Results:
 *	The return value indicates if the tagOrId expression was successfully
 *	scanned (syntax). The information at *searchPtr is initialized such
 *	that a call to TagSearchFirst, followed by successive calls to
 *	TagSearchNext will return items that match tag.
 *
 * Side effects:
 *	SearchPtr is linked into a list of searches in progress on canvasPtr,
 *	so that elements can safely be deleted while the search is in
 *	progress.
 *
 *--------------------------------------------------------------
 */

static int
TagSearchScan(
    TkPathCanvas *canvasPtr,	/* Canvas whose items are to be searched. */
    Tcl_Obj *tagObj,		/* Object giving tag value. */
    TagSearch **searchPtrPtr)	/* Record describing tag search; will be
				 * initialized here. */
{
    char *tag = Tcl_GetString(tagObj);
    int i;
    TagSearch *searchPtr;

    /*
     * Initialize the search.
     */

    if (*searchPtrPtr) {
	searchPtr = *searchPtrPtr;
    } else {
	/*
	 * Allocate primary search struct on first call.
	 */

	*searchPtrPtr = searchPtr = (TagSearch *) ckalloc(sizeof(TagSearch));
	searchPtr->expr = NULL;

	/*
	 * Allocate buffer for rewritten tags (after de-escaping).
	 */

	searchPtr->rewritebufferAllocated = 100;
	searchPtr->rewritebuffer =
                          (char *) ckalloc(searchPtr->rewritebufferAllocated);
    }
    TagSearchExprInit(&(searchPtr->expr));

    /*
     * How long is the tagOrId?
     */

    searchPtr->stringLength = (int) strlen(tag);

    /*
     * Make sure there is enough buffer to hold rewritten tags.
     */

    if ((unsigned int)searchPtr->stringLength >=
	    searchPtr->rewritebufferAllocated) {
	searchPtr->rewritebufferAllocated = searchPtr->stringLength + 100;
	searchPtr->rewritebuffer = (char *)
		ckrealloc(searchPtr->rewritebuffer,
		searchPtr->rewritebufferAllocated);
    }

    /*
     * Initialize search.
     */

    searchPtr->canvasPtr = canvasPtr;
    searchPtr->searchOver = 0;
    searchPtr->type = SEARCH_TYPE_EMPTY;

    /*
     * Find the first matching item in one of several ways. If the tag is a
     * number then it selects the single item with the matching identifier.
     * In this case see if the item being requested is the hot item, in which
     * case the search can be skipped.
     */

    if (searchPtr->stringLength && isdigit(UCHAR(*tag))) {
	char *end;

	searchPtr->id = strtoul(tag, &end, 0);
	if (*end == 0) {
	    searchPtr->type = SEARCH_TYPE_ID;
	    return TCL_OK;
	}
    }

    /*
     * For all other tags and tag expressions convert to a UID. This UID is
     * kept forever, but this should be thought of as a cache rather than as a
     * memory leak.
     */
    searchPtr->expr->uid = Tk_GetUid(tag);

    /*
     * Short circuit impossible searches for null tags.
     */

    if (searchPtr->stringLength == 0) {
	return TCL_OK;
    }

    /*
     * Pre-scan tag for at least one unquoted "&&" "||" "^" "!"
     *   if not found then use string as simple tag
     */

    for (i = 0; i < searchPtr->stringLength ; i++) {
	if (tag[i] == '"') {
	    i++;
	    for ( ; i < searchPtr->stringLength; i++) {
		if (tag[i] == '\\') {
		    i++;
		    continue;
		}
		if (tag[i] == '"') {
		    break;
		}
	    }
	} else if ((tag[i] == '&' && tag[i+1] == '&')
		|| (tag[i] == '|' && tag[i+1] == '|')
		|| (tag[i] == '^')
		|| (tag[i] == '!')) {
	    searchPtr->type = SEARCH_TYPE_EXPR;
	    break;
	}
    }

    searchPtr->string = tag;
    searchPtr->stringIndex = 0;
    if (searchPtr->type == SEARCH_TYPE_EXPR) {
	/*
	 * An operator was found in the prescan, so now compile the tag
	 * expression into array of Tk_Uid flagging any syntax errors found.
	 */

	if (TagSearchScanExpr(canvasPtr->interp, searchPtr,
		searchPtr->expr) != TCL_OK) {
	    /*
	     * Syntax error in tag expression. The result message was set by
	     * TagSearchScanExpr.
	     */

	    return TCL_ERROR;
	}
	searchPtr->expr->length = searchPtr->expr->index;
    } else if (searchPtr->expr->uid == GetStaticUids()->allUid) {
	/*
	 * All items match.
	 */

	searchPtr->type = SEARCH_TYPE_ALL;
    } else if (searchPtr->expr->uid == GetStaticUids()->rootUid) {
	searchPtr->type = SEARCH_TYPE_ROOT;
    } else {
	/*
	 * Optimized single-tag search
	 */

	searchPtr->type = SEARCH_TYPE_TAG;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchDestroy --
 *
 *	This function destroys any dynamic structures that may have been
 *	allocated by TagSearchScan.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Deallocates memory.
 *
 *--------------------------------------------------------------
 */

static void
TagSearchDestroy(
    TagSearch *searchPtr)	/* Record describing tag search */
{
    if (searchPtr) {
	TagSearchExprDestroy(searchPtr->expr);
	ckfree((char *)searchPtr->rewritebuffer);
	ckfree((char *)searchPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchScanExpr --
 *
 *	This recursive function is called to scan a tag expression and compile
 *	it into an array of Tk_Uids.
 *
 * Results:
 *	The return value indicates if the tagOrId expression was successfully
 *	scanned (syntax). The information at *searchPtr is initialized such
 *	that a call to TagSearchFirst, followed by successive calls to
 *	TagSearchNext will return items that match tag.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
TagSearchScanExpr(
    Tcl_Interp *interp,		/* Current interpreter. */
    TagSearch *searchPtr,	/* Search data */
    TagSearchExpr *expr)	/* compiled expression result */
{
    int looking_for_tag;	/* When true, scanner expects next char(s) to
				 * be a tag, else operand expected */
    int found_tag;		/* One or more tags found */
    int found_endquote;		/* For quoted tag string parsing */
    int negate_result;		/* Pending negation of next tag value */
    char *tag;			/* Tag from tag expression string */
    char c;
    SearchUids *searchUids;	/* Collection of uids for basic search
				 * expression terms. */

    searchUids = GetStaticUids();
    negate_result = 0;
    found_tag = 0;
    looking_for_tag = 1;
    while (searchPtr->stringIndex < searchPtr->stringLength) {
	c = searchPtr->string[searchPtr->stringIndex++];

	if (expr->allocated == expr->index) {
	    expr->allocated += 15;
	    if (expr->uids) {
		expr->uids = (Tk_Uid *)
			ckrealloc((char *)(expr->uids),
			(expr->allocated)*sizeof(Tk_Uid));
	    } else {
		expr->uids = (Tk_Uid *)
			ckalloc((expr->allocated)*sizeof(Tk_Uid));
	    }
	}

	if (looking_for_tag) {

	    switch (c) {
	    case ' ':	/* ignore unquoted whitespace */
	    case '\t':
	    case '\n':
	    case '\r':
		break;

	    case '!':	/* negate next tag or subexpr */
		if (looking_for_tag > 1) {
		    Tcl_AppendResult(interp,
			    "Too many '!' in tag search expression",
			    NULL);
		    return TCL_ERROR;
		}
		looking_for_tag++;
		negate_result = 1;
		break;

	    case '(':	/* scan (negated) subexpr recursively */
		if (negate_result) {
		    expr->uids[expr->index++] = searchUids->negparenUid;
		    negate_result = 0;
		} else {
		    expr->uids[expr->index++] = searchUids->parenUid;
		}
		if (TagSearchScanExpr(interp, searchPtr, expr) != TCL_OK) {
		    /*
		     * Result string should be already set by nested call to
		     * tag_expr_scan()
		     */

		    return TCL_ERROR;
		}
		looking_for_tag = 0;
		found_tag = 1;
		break;

	    case '"':	/* quoted tag string */
		if (negate_result) {
		    expr->uids[expr->index++] = searchUids->negtagvalUid;
		    negate_result = 0;
		} else {
		    expr->uids[expr->index++] = searchUids->tagvalUid;
		}
		tag = searchPtr->rewritebuffer;
		found_endquote = 0;
		while (searchPtr->stringIndex < searchPtr->stringLength) {
		    c = searchPtr->string[searchPtr->stringIndex++];
		    if (c == '\\') {
			c = searchPtr->string[searchPtr->stringIndex++];
		    }
		    if (c == '"') {
			found_endquote = 1;
			break;
		    }
		    *tag++ = c;
		}
		if (! found_endquote) {
		    Tcl_AppendResult(interp,
			    "Missing endquote in tag search expression",
			    NULL);
		    return TCL_ERROR;
		}
		if (! (tag - searchPtr->rewritebuffer)) {
		    Tcl_AppendResult(interp,
			    "Null quoted tag string in tag search expression",
			    NULL);
		    return TCL_ERROR;
		}
		*tag++ = '\0';
		expr->uids[expr->index++] =
			Tk_GetUid(searchPtr->rewritebuffer);
		looking_for_tag = 0;
		found_tag = 1;
		break;

	    case '&':	/* illegal chars when looking for tag */
	    case '|':
	    case '^':
	    case ')':
		Tcl_AppendResult(interp,
			"Unexpected operator in tag search expression",
			NULL);
		return TCL_ERROR;

	    default:	/* unquoted tag string */
		if (negate_result) {
		    expr->uids[expr->index++] = searchUids->negtagvalUid;
		    negate_result = 0;
		} else {
		    expr->uids[expr->index++] = searchUids->tagvalUid;
		}
		tag = searchPtr->rewritebuffer;
		*tag++ = c;

		/*
		 * Copy rest of tag, including any embedded whitespace.
		 */

		while (searchPtr->stringIndex < searchPtr->stringLength) {
		    c = searchPtr->string[searchPtr->stringIndex];
		    if (c == '!' || c == '&' || c == '|' || c == '^'
			    || c == '(' || c == ')' || c == '"') {
			break;
		    }
		    *tag++ = c;
		    searchPtr->stringIndex++;
		}

		/*
		 * Remove trailing whitespace.
		 */

		while (1) {
		    c = *--tag;

		    /*
		     * There must have been one non-whitespace char, so this
		     * will terminate.
		     */

		    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
			break;
		    }
		}
		*++tag = '\0';
		expr->uids[expr->index++] =
			Tk_GetUid(searchPtr->rewritebuffer);
		looking_for_tag = 0;
		found_tag = 1;
	    }

	} else {    /* ! looking_for_tag */
	    switch (c) {
	    case ' ':	/* ignore whitespace */
	    case '\t':
	    case '\n':
	    case '\r':
		break;

	    case '&':	/* AND operator */
		c = searchPtr->string[searchPtr->stringIndex++];
		if (c != '&') {
		    Tcl_AppendResult(interp,
			    "Singleton '&' in tag search expression",
			    NULL);
		    return TCL_ERROR;
		}
		expr->uids[expr->index++] = searchUids->andUid;
		looking_for_tag = 1;
		break;

	    case '|':	/* OR operator */
		c = searchPtr->string[searchPtr->stringIndex++];
		if (c != '|') {
		    Tcl_AppendResult(interp,
			    "Singleton '|' in tag search expression",
			    NULL);
		    return TCL_ERROR;
		}
		expr->uids[expr->index++] = searchUids->orUid;
		looking_for_tag = 1;
		break;

	    case '^'  :	/* XOR operator */
		expr->uids[expr->index++] = searchUids->xorUid;
		looking_for_tag = 1;
		break;

	    case ')'  :	/* end subexpression */
		expr->uids[expr->index++] = searchUids->endparenUid;
		goto breakwhile;

	    default:	/* syntax error */
		Tcl_AppendResult(interp,
			"Invalid boolean operator in tag search expression",
			NULL);
		return TCL_ERROR;
	    }
	}
    }

  breakwhile:
    if (found_tag && ! looking_for_tag) {
	return TCL_OK;
    }
    Tcl_AppendResult(interp, "Missing tag in tag search expression", NULL);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchEvalExpr --
 *
 *	This recursive function is called to eval a tag expression.
 *
 * Results:
 *	The return value indicates if the tagOrId expression successfully
 *	matched the tags of the current item.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
TagSearchEvalExpr(
    TagSearchExpr *expr,	/* Search expression */
    Tk_PathItem *itemPtr)	/* Item being test for match */
{
    int looking_for_tag;	/* When true, scanner expects next char(s) to
				 * be a tag, else operand expected. */
    int negate_result;		/* Pending negation of next tag value */
    Tk_Uid uid;
    Tk_Uid *tagPtr;
    Tk_PathTags *ptagsPtr;
    int count;
    int result;			/* Value of expr so far */
    int parendepth;
    SearchUids *searchUids;	/* Collection of uids for basic search
				 * expression terms. */

    searchUids = GetStaticUids();
    result = 0;  /* just to keep the compiler quiet */

    negate_result = 0;
    looking_for_tag = 1;
    while (expr->index < expr->length) {
	uid = expr->uids[expr->index++];
	if (looking_for_tag) {
	    if (uid == searchUids->tagvalUid) {
/*
 *		assert(expr->index < expr->length);
 */
		uid = expr->uids[expr->index++];
		result = 0;

		/*
		 * set result 1 if tag is found in item's tags
		 */

		ptagsPtr = itemPtr->pathTagsPtr;
		if (ptagsPtr != NULL) {
		    for (tagPtr = ptagsPtr->tagPtr, count = ptagsPtr->numTags;
			    count > 0; tagPtr++, count--) {
			if (*tagPtr == uid) {
			    result = 1;
			    break;
			}
		    }
		}
	    } else if (uid == searchUids->negtagvalUid) {
		negate_result = ! negate_result;
/*
 *		assert(expr->index < expr->length);
 */
		uid = expr->uids[expr->index++];
		result = 0;

		/*
		 * set result 1 if tag is found in item's tags
		 */
		ptagsPtr = itemPtr->pathTagsPtr;
		if (ptagsPtr != NULL) {
		    for (tagPtr = ptagsPtr->tagPtr, count = ptagsPtr->numTags;
			    count > 0; tagPtr++, count--) {
			if (*tagPtr == uid) {
			    result = 1;
			    break;
			}
		    }
		}
	    } else if (uid == searchUids->parenUid) {
		/*
		 * Evaluate subexpressions with recursion
		 */

		result = TagSearchEvalExpr(expr, itemPtr);

	    } else if (uid == searchUids->negparenUid) {
		negate_result = ! negate_result;

		/*
		 * Evaluate subexpressions with recursion
		 */

		result = TagSearchEvalExpr(expr, itemPtr);
/*
 *	    } else {
 *		assert(0);
 */
	    }
	    if (negate_result) {
		result = ! result;
		negate_result = 0;
	    }
	    looking_for_tag = 0;
	} else {    /* ! looking_for_tag */
	    if (((uid == searchUids->andUid) && (!result)) ||
		    ((uid == searchUids->orUid) && result)) {
		/*
		 * Short circuit expression evaluation.
		 *
		 * if result before && is 0, or result before || is 1, then
		 * the expression is decided and no further evaluation is
		 * needed.
		 */

		parendepth = 0;
		while (expr->index < expr->length) {
		    uid = expr->uids[expr->index++];
		    if (uid == searchUids->tagvalUid ||
			    uid == searchUids->negtagvalUid) {
			expr->index++;
			continue;
		    }
		    if (uid == searchUids->parenUid ||
			    uid == searchUids->negparenUid) {
			parendepth++;
			continue;
		    }
		    if (uid == searchUids->endparenUid) {
			parendepth--;
			if (parendepth < 0) {
			    break;
			}
		    }
		}
		return result;

	    } else if (uid == searchUids->xorUid) {
		/*
		 * If the previous result was 1 then negate the next result.
		 */

		negate_result = result;

	    } else if (uid == searchUids->endparenUid) {
		return result;
/*
 *	    } else {
 *		assert(0);
 */
	    }
	    looking_for_tag = 1;
	}
    }
/*
 *  assert(! looking_for_tag);
 */
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchFirst --
 *
 *	This function is called to get the first item item that matches a
 *	preestablished search predicate that was set by TagSearchScan.
 *
 * Results:
 *	The return value is a pointer to the first item, or NULL if there is
 *	no such item. The information at *searchPtr is updated such that
 *	successive calls to TagSearchNext will return successive items.
 *
 * Side effects:
 *	SearchPtr is linked into a list of searches in progress on canvasPtr,
 *	so that elements can safely be deleted while the search is in
 *	progress.
 *
 *--------------------------------------------------------------
 */

static Tk_PathItem *
TagSearchFirst(
    TagSearch *searchPtr)	/* Record describing tag search */
{
    Tk_PathItem *itemPtr, *lastPtr;
    Tk_Uid uid, *tagPtr;
    Tk_PathTags *ptagsPtr;
    int count;

    /*
     * Short circuit impossible searches for null tags.
     */

    if (searchPtr->stringLength == 0) {
	return NULL;
    }

    /*
     * Find the first matching item in one of several ways. If the tag is a
     * number then it selects the single item with the matching identifier.
     * In this case see if the item being requested is the hot item, in which
     * case the search can be skipped.
     */

    if (searchPtr->type == SEARCH_TYPE_ID) {
	Tcl_HashEntry *entryPtr;

	itemPtr = searchPtr->canvasPtr->hotPtr;
	lastPtr = searchPtr->canvasPtr->hotPrevPtr;
	if ((itemPtr == NULL) || (itemPtr->id != searchPtr->id)
		|| (lastPtr == NULL) || (TkPathCanvasItemIteratorNext(lastPtr) != itemPtr)) {
	    entryPtr = Tcl_FindHashEntry(&searchPtr->canvasPtr->idTable,
		    (char *) INT2PTR(searchPtr->id));
	    if (entryPtr != NULL) {
		itemPtr = (Tk_PathItem *)Tcl_GetHashValue(entryPtr);
		lastPtr = TkPathCanvasItemIteratorPrev(itemPtr);
	    } else {
		lastPtr = itemPtr = NULL;
	    }
	}
	searchPtr->lastPtr = lastPtr;
	searchPtr->searchOver = 1;
	searchPtr->canvasPtr->hotPtr = itemPtr;
	searchPtr->canvasPtr->hotPrevPtr = lastPtr;
	return itemPtr;
    }

    if (searchPtr->type == SEARCH_TYPE_ALL) {
	/*
	 * All items match.
	 */

	searchPtr->lastPtr = NULL;
	searchPtr->currentPtr = searchPtr->canvasPtr->rootItemPtr;
	return searchPtr->canvasPtr->rootItemPtr;
    }
    if (searchPtr->type == SEARCH_TYPE_ROOT) {
	itemPtr = searchPtr->canvasPtr->rootItemPtr;
	lastPtr = NULL;
	searchPtr->lastPtr = lastPtr;
	searchPtr->searchOver = 1;
	searchPtr->canvasPtr->hotPtr = itemPtr;
	searchPtr->canvasPtr->hotPrevPtr = lastPtr;
	return itemPtr;
    }

    if (searchPtr->type == SEARCH_TYPE_TAG) {
	/*
	 * Optimized single-tag search
	 */

	uid = searchPtr->expr->uid;
	for (lastPtr = NULL, itemPtr = searchPtr->canvasPtr->rootItemPtr;
		itemPtr != NULL; lastPtr = itemPtr, itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	    ptagsPtr = itemPtr->pathTagsPtr;
	    if (ptagsPtr != NULL) {
		for (tagPtr = ptagsPtr->tagPtr, count = ptagsPtr->numTags;
		     count > 0; tagPtr++, count--) {
		    if (*tagPtr == uid) {
			searchPtr->lastPtr = lastPtr;
			searchPtr->currentPtr = itemPtr;
			return itemPtr;
		    }
		}
	    }
	}
    } else {

	/*
	 * None of the above. Search for an item matching the tag expression.
	 */

	for (lastPtr = NULL, itemPtr = searchPtr->canvasPtr->rootItemPtr;
		itemPtr != NULL; lastPtr = itemPtr, itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	    searchPtr->expr->index = 0;
	    if (TagSearchEvalExpr(searchPtr->expr, itemPtr)) {
		searchPtr->lastPtr = lastPtr;
		searchPtr->currentPtr = itemPtr;
		return itemPtr;
	    }
	}
    }
    searchPtr->lastPtr = lastPtr;
    searchPtr->searchOver = 1;
    return NULL;
}

/*
 *--------------------------------------------------------------
 *
 * TagSearchNext --
 *
 *	This function returns successive items that match a given tag; it
 *	should be called only after TagSearchFirst has been used to begin a
 *	search.
 *
 * Results:
 *	The return value is a pointer to the next item that matches the tag
 *	expr specified to TagSearchScan, or NULL if no such item exists.
 *	*SearchPtr is updated so that the next call to this function will
 *	return the next item.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static Tk_PathItem *
TagSearchNext(
    TagSearch *searchPtr)	/* Record describing search in progress. */
{
    Tk_PathItem *itemPtr, *lastPtr;
    Tk_PathTags *ptagsPtr;
    Tk_Uid uid, *tagPtr;
    int count;

    /*
     * Find next item in list (this may not actually be a suitable one to
     * return), and return if there are no items left.
     */

    lastPtr = searchPtr->lastPtr;
    if (lastPtr == NULL) {
	itemPtr = searchPtr->canvasPtr->rootItemPtr;
    } else {
	itemPtr = TkPathCanvasItemIteratorNext(lastPtr);
    }
    if ((itemPtr == NULL) || (searchPtr->searchOver)) {
	searchPtr->searchOver = 1;
	return NULL;
    }
    if (itemPtr != searchPtr->currentPtr) {
	/*
	 * The structure of the list has changed. Probably the previously-
	 * returned item was removed from the list. In this case, don't
	 * advance lastPtr; just return its new successor (i.e. do nothing
	 * here).
	 */
    } else {
	lastPtr = itemPtr;
	itemPtr = TkPathCanvasItemIteratorNext(lastPtr);
    }

    if (searchPtr->type == SEARCH_TYPE_ALL) {
	/*
	 * All items match.
	 */

	searchPtr->lastPtr = lastPtr;
	searchPtr->currentPtr = itemPtr;
	return itemPtr;
    }

    if (searchPtr->type == SEARCH_TYPE_TAG) {
	/*
	 * Optimized single-tag search
	 */

	uid = searchPtr->expr->uid;
	for (; itemPtr != NULL; lastPtr = itemPtr, itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	    ptagsPtr = itemPtr->pathTagsPtr;
	    if (ptagsPtr != NULL) {
		for (tagPtr = ptagsPtr->tagPtr, count = ptagsPtr->numTags;
		     count > 0; tagPtr++, count--) {
		    if (*tagPtr == uid) {
			searchPtr->lastPtr = lastPtr;
			searchPtr->currentPtr = itemPtr;
			return itemPtr;
		    }
		}
	    }
	}
	searchPtr->lastPtr = lastPtr;
	searchPtr->searchOver = 1;
	return NULL;
    }

    /*
     * Else.... evaluate tag expression
     */

    for ( ; itemPtr != NULL; lastPtr = itemPtr, itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	searchPtr->expr->index = 0;
	if (TagSearchEvalExpr(searchPtr->expr, itemPtr)) {
	    searchPtr->lastPtr = lastPtr;
	    searchPtr->currentPtr = itemPtr;
	    return itemPtr;
	}
    }
    searchPtr->lastPtr = lastPtr;
    searchPtr->searchOver = 1;
    return NULL;
}
#endif /* USE_OLD_TAG_SEARCH */

/*
 *--------------------------------------------------------------
 *
 * DoItem --
 *
 *	This is a utility function called by FindItems. It either adds
 *	itemPtr's id to the result forming in interp, or it adds a new tag to
 *	itemPtr, depending on the value of tag.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If tag is NULL then itemPtr's id is added as a list element to the
 *	interp's result; otherwise tag is added to itemPtr's list of tags.
 *
 *--------------------------------------------------------------
 */

static void
DoItem(
    Tcl_Interp *interp,		/* Interpreter in which to (possibly) record
				 * item id. */
    Tk_PathItem *itemPtr,	/* Item to (possibly) modify. */
    Tk_Uid tag)			/* Tag to add to those already present for
				 * item, or NULL. */
{
    Tk_Uid *tagPtr;
    Tk_PathTags *ptagsPtr;
    int count;

    /*
     * Handle the "add-to-result" case and return, if appropriate.
     */

    if (tag == NULL) {
	char msg[TCL_INTEGER_SPACE];

	sprintf(msg, "%d", itemPtr->id);
	Tcl_AppendElement(interp, msg);
	return;
    }
    
    /*
     * Do not add if already there.
     */

    ptagsPtr = itemPtr->pathTagsPtr;
    if (ptagsPtr != NULL) {
	for (tagPtr = ptagsPtr->tagPtr, count = ptagsPtr->numTags;
	     count > 0; tagPtr++, count--) {
	    if (tag == *tagPtr) {
		return;
	    }
	}
    }

    /*
     * Grow the tag space if there's no more room left in the current block.
     */
    
    if (itemPtr->pathTagsPtr == NULL) {
	ptagsPtr = TkPathAllocTagsFromObj(NULL, NULL);
	itemPtr->pathTagsPtr = ptagsPtr;
	tagPtr = ptagsPtr->tagPtr;
    } else {
	ptagsPtr = itemPtr->pathTagsPtr;
	if (ptagsPtr->tagSpace == ptagsPtr->numTags) {
	    Tk_Uid *newTagPtr;

	    ptagsPtr->tagSpace += 5;
	    newTagPtr = (Tk_Uid *)
		    ckalloc((unsigned) (ptagsPtr->tagSpace * sizeof(Tk_Uid)));
	    memcpy((void *) newTagPtr, ptagsPtr->tagPtr,
		    ptagsPtr->numTags * sizeof(Tk_Uid));
	    ckfree((char *) ptagsPtr->tagPtr);
	    ptagsPtr->tagPtr = newTagPtr;
	}
	
	/* NB: This returns the first free tag address. */
	tagPtr = &ptagsPtr->tagPtr[ptagsPtr->numTags];
    }

    /*
     * Add in the new tag.
     */

    *tagPtr = tag;
    ptagsPtr->numTags++;
}

/*
 *--------------------------------------------------------------
 *
 * FindItems --
 *
 *	This function does all the work of implementing the "find" and
 *	"addtag" options of the canvas widget command, which locate items that
 *	have certain features (location, tags, position in display list, etc.)
 *
 * Results:
 *	A standard Tcl return value. If newTag is NULL, then a list of ids
 *	from all the items that match objc/objv is returned in the interp's
 *	result. If newTag is NULL, then the normal the interp's result is an
 *	empty string. If an error occurs, then the interp's result will hold
 *	an error message.
 *
 * Side effects:
 *	If newTag is non-NULL, then all the items that match the information
 *	in objc/objv have that tag added to their lists of tags.
 *
 *--------------------------------------------------------------
 */

static int
FindItems(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    TkPathCanvas *canvasPtr,	/* Canvas whose items are to be searched. */
    int objc,			/* Number of entries in argv. Must be greater
				 * than zero. */
    Tcl_Obj *CONST *objv,	/* Arguments that describe what items to
				 * search for (see user doc on "find" and
				 * "addtag" options). */
    Tcl_Obj *newTag,		/* If non-NULL, gives new tag to set on all
				 * found items; if NULL, then ids of found
				 * items are returned in the interp's
				 * result. */
    int first			/* For error messages: gives number of
				 * elements of objv which are already
				 * handled. */
#ifndef USE_OLD_TAG_SEARCH
    ,TagSearch **searchPtrPtr	/* From CanvasWidgetCmd local vars*/
#endif /* not USE_OLD_TAG_SEARCH */
    )
{
#ifdef USE_OLD_TAG_SEARCH
    TagSearch search;
#endif /* USE_OLD_TAG_SEARCH */
    Tk_PathItem *itemPtr;
    Tk_Uid uid;
    int index, result;
    static CONST char *optionStrings[] = {
	"above", "all", "below", "closest",
	"enclosed", "overlapping", "withtag", NULL
    };
    enum options {
	CANV_ABOVE, CANV_ALL, CANV_BELOW, CANV_CLOSEST,
	CANV_ENCLOSED, CANV_OVERLAPPING, CANV_WITHTAG
    };

    if (newTag != NULL) {
	uid = Tk_GetUid(Tcl_GetString(newTag));
    } else {
	uid = NULL;
    }
    if (Tcl_GetIndexFromObj(interp, objv[first], optionStrings,
	    "search command", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum options) index) {
    case CANV_ABOVE: {
	Tk_PathItem *lastPtr = NULL;

	if (objc != first+2) {
	    Tcl_WrongNumArgs(interp, first+1, objv, "tagOrId");
	    return TCL_ERROR;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[first+1], searchPtrPtr,
		return TCL_ERROR) {
	    lastPtr = itemPtr;
	}

	/* We constrain this to siblings. */
	if ((lastPtr != NULL) && (lastPtr->nextPtr != NULL)) {
	    DoItem(interp, lastPtr->nextPtr, uid);
	}
	break;
    }
    case CANV_ALL:
	if (objc != first+1) {
	    Tcl_WrongNumArgs(interp, first+1, objv, NULL);
	    return TCL_ERROR;
	}
	for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
		itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	    DoItem(interp, itemPtr, uid);
	}
	break;

    case CANV_BELOW:
	if (objc != first+2) {
	    Tcl_WrongNumArgs(interp, first+1, objv, "tagOrId");
	    return TCL_ERROR;
	}
	FIRST_CANVAS_ITEM_MATCHING(objv[first+1], searchPtrPtr,
		return TCL_ERROR);
	if (itemPtr != NULL) {
	
	    /* We constrain this to siblings. */
	    if (itemPtr->prevPtr != NULL) {
		DoItem(interp, itemPtr->prevPtr, uid);
	    }
	}
	break;
    case CANV_CLOSEST: {
	double closestDist;
	Tk_PathItem *startPtr, *closestPtr;
	double coords[2], halo;
	int x1, y1, x2, y2;

	if ((objc < first+3) || (objc > first+5)) {
	    Tcl_WrongNumArgs(interp, first+1, objv, "x y ?halo? ?start?");
	    return TCL_ERROR;
	}
	if ((Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[first+1],
		&coords[0]) != TCL_OK) || (Tk_PathCanvasGetCoordFromObj(interp,
		(Tk_PathCanvas) canvasPtr, objv[first+2], &coords[1]) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if (objc > first+3) {
	    if (Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[first+3],
		    &halo) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (halo < 0.0) {
		Tcl_AppendResult(interp, "can't have negative halo value \"",
			Tcl_GetString(objv[3]), "\"", NULL);
		return TCL_ERROR;
	    }
	} else {
	    halo = 0.0;
	}

	/*
	 * Find the item at which to start the search.
	 */

	startPtr = canvasPtr->rootItemPtr;
	if (objc == first+5) {
	    FIRST_CANVAS_ITEM_MATCHING(objv[first+4], searchPtrPtr,
		    return TCL_ERROR);
	    if (itemPtr != NULL) {
		startPtr = itemPtr;
	    }
	}

	/*
	 * The code below is optimized so that it can eliminate most items
	 * without having to call their item-specific functions. This is done
	 * by keeping a bounding box (x1, y1, x2, y2) that an item's bbox must
	 * overlap if the item is to have any chance of being closer than the
	 * closest so far.
	 */

	itemPtr = startPtr;
	while(itemPtr && (itemPtr->state == TK_PATHSTATE_HIDDEN ||
		(itemPtr->state == TK_PATHSTATE_NULL &&
		canvasPtr->canvas_state == TK_PATHSTATE_HIDDEN))) {
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr);
	}
	if (itemPtr == NULL) {
	    return TCL_OK;
	}
	closestDist = (*itemPtr->typePtr->pointProc)((Tk_PathCanvas) canvasPtr,
		itemPtr, coords) - halo;
	if (closestDist < 0.0) {
	    closestDist = 0.0;
	}
	while (1) {
	    double newDist;

	    /*
	     * Update the bounding box using itemPtr, which is the new closest
	     * item.
	     */

	    x1 = (int) (coords[0] - closestDist - halo - 1);
	    y1 = (int) (coords[1] - closestDist - halo - 1);
	    x2 = (int) (coords[0] + closestDist + halo + 1);
	    y2 = (int) (coords[1] + closestDist + halo + 1);
	    closestPtr = itemPtr;

	    /*
	     * Search for an item that beats the current closest one. Work
	     * circularly through the canvas's item list until getting back to
	     * the starting item.
	     */

	    while (1) {
		itemPtr = TkPathCanvasItemIteratorNext(itemPtr);
		if (itemPtr == NULL) {
		    itemPtr = canvasPtr->rootItemPtr;
		}
		if (itemPtr == startPtr) {
		    DoItem(interp, closestPtr, uid);
		    return TCL_OK;
		}
		if (itemPtr->state == TK_PATHSTATE_HIDDEN ||
			(itemPtr->state == TK_PATHSTATE_NULL &&
			canvasPtr->canvas_state == TK_PATHSTATE_HIDDEN)) {
		    continue;
		}
		if ((itemPtr->x1 >= x2) || (itemPtr->x2 <= x1)
			|| (itemPtr->y1 >= y2) || (itemPtr->y2 <= y1)) {
		    continue;
		}
		newDist = (*itemPtr->typePtr->pointProc)((Tk_PathCanvas) canvasPtr,
			itemPtr, coords) - halo;
		if (newDist < 0.0) {
		    newDist = 0.0;
		}
		if (newDist <= closestDist) {
		    closestDist = newDist;
		    break;
		}
	    }
	}
	break;
    }
    case CANV_ENCLOSED:
	if (objc != first+5) {
	    Tcl_WrongNumArgs(interp, first+1, objv, "x1 y1 x2 y2");
	    return TCL_ERROR;
	}
	return FindArea(interp, canvasPtr, objv+first+1, uid, 1);
    case CANV_OVERLAPPING:
	if (objc != first+5) {
	    Tcl_WrongNumArgs(interp, first+1, objv, "x1 y1 x2 y2");
	    return TCL_ERROR;
	}
	return FindArea(interp, canvasPtr, objv+first+1, uid, 0);
    case CANV_WITHTAG:
	if (objc != first+2) {
	    Tcl_WrongNumArgs(interp, first+1, objv, "tagOrId");
	    return TCL_ERROR;
	}
	FOR_EVERY_CANVAS_ITEM_MATCHING(objv[first+1], searchPtrPtr,
		return TCL_ERROR) {
	    DoItem(interp, itemPtr, uid);
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * FindArea --
 *
 *	This function implements area searches for the "find" and "addtag"
 *	options.
 *
 * Results:
 *	A standard Tcl return value. If newTag is NULL, then a list of ids
 *	from all the items overlapping or enclosed by the rectangle given by
 *	objc is returned in the interp's result. If newTag is NULL, then the
 *	normal the interp's result is an empty string. If an error occurs,
 *	then the interp's result will hold an error message.
 *
 * Side effects:
 *	If uid is non-NULL, then all the items overlapping or enclosed by the
 *	area in objv have that tag added to their lists of tags.
 *
 *--------------------------------------------------------------
 */

static int
FindArea(
    Tcl_Interp *interp,		/* Interpreter for error reporting and result
				 * storing. */
    TkPathCanvas *canvasPtr,	/* Canvas whose items are to be searched. */
    Tcl_Obj *CONST *objv,	/* Array of four arguments that give the
				 * coordinates of the rectangular area to
				 * search. */
    Tk_Uid uid,			/* If non-NULL, gives new tag to set on all
				 * found items; if NULL, then ids of found
				 * items are returned in the interp's
				 * result. */
    int enclosed)		/* 0 means overlapping or enclosed items are
				 * OK, 1 means only enclosed items are OK. */
{
    double rect[4], tmp;
    int x1, y1, x2, y2;
    Tk_PathItem *itemPtr;

    if ((Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[0],
		&rect[0]) != TCL_OK)
	    || (Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[1],
		&rect[1]) != TCL_OK)
	    || (Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[2],
		&rect[2]) != TCL_OK)
	    || (Tk_PathCanvasGetCoordFromObj(interp, (Tk_PathCanvas) canvasPtr, objv[3],
		&rect[3]) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (rect[0] > rect[2]) {
	tmp = rect[0]; rect[0] = rect[2]; rect[2] = tmp;
    }
    if (rect[1] > rect[3]) {
	tmp = rect[1]; rect[1] = rect[3]; rect[3] = tmp;
    }

    /*
     * Use an integer bounding box for a quick test, to avoid calling
     * item-specific code except for items that are close.
     */

    x1 = (int) (rect[0]-1.0);
    y1 = (int) (rect[1]-1.0);
    x2 = (int) (rect[2]+1.0);
    y2 = (int) (rect[3]+1.0);
    for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	if (itemPtr->state == TK_PATHSTATE_HIDDEN || (itemPtr->state == TK_PATHSTATE_NULL &&
		canvasPtr->canvas_state == TK_PATHSTATE_HIDDEN)) {
	    continue;
	}
	if ((itemPtr->x1 >= x2) || (itemPtr->x2 <= x1)
		|| (itemPtr->y1 >= y2) || (itemPtr->y2 <= y1)) {
	    continue;
	}
	if ((*itemPtr->typePtr->areaProc)((Tk_PathCanvas) canvasPtr, itemPtr, rect)
		>= enclosed) {
	    DoItem(interp, itemPtr, uid);
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * RelinkItems --
 *
 *	Move one or more items to a different place in the display order for a
 *	canvas.
 *	Only items with same parent as prevPtr will be moved. Items matching
 *	tag but with different parent will be silently ignored.
 *	If we didn't do this we would break the tree hierarchy structure
 *	which would create a mess!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The items identified by "tag" are moved so that they are all together
 *	in the display list and immediately after prevPtr. The order of the
 *	moved items relative to each other is not changed.
 *
 *--------------------------------------------------------------
 */

#ifdef USE_OLD_TAG_SEARCH
static void
RelinkItems(
    TkPathCanvas *canvasPtr,	/* Canvas to be modified. */
    Tcl_Obj *tag,		/* Tag identifying items to be moved in the
				 * redisplay list. */
    Tk_PathItem *prevPtr)	/* Reposition the items so that they go just
				 * after this item (NULL means put at
				 * beginning of list). */
#else /* USE_OLD_TAG_SEARCH */
static int
RelinkItems(
    TkPathCanvas *canvasPtr,	/* Canvas to be modified. */
    Tcl_Obj *tag,		/* Tag identifying items to be moved in the
				 * redisplay list. */
    Tk_PathItem *prevPtr,	/* Reposition the items so that they go just
				 * after this item (NULL means put at
				 * beginning of list). */
    TagSearch **searchPtrPtr)	/* From CanvasWidgetCmd local vars */
#endif /* USE_OLD_TAG_SEARCH */
{
    Tk_PathItem *itemPtr;
#ifdef USE_OLD_TAG_SEARCH
    TagSearch search;
#endif /* USE_OLD_TAG_SEARCH */
    Tk_PathItem *firstMovePtr, *lastMovePtr;
    Tk_PathItem *parentPtr, *rootItemPtr;
    int result;

    rootItemPtr = canvasPtr->rootItemPtr;
    if (prevPtr == rootItemPtr) {
#ifdef USE_OLD_TAG_SEARCH
	return;
#else /* USE_OLD_TAG_SEARCH */
	return TCL_OK;
#endif /* USE_OLD_TAG_SEARCH */
    }	

    /*
     * Keep track of parentPtr for the selection of items.
     * prevPtr equal to NULL means use the root item as parent.
     * This keeps compatiblity with old canvas.
     */
    if (prevPtr != NULL) {
	parentPtr = prevPtr->parentPtr;
    } else {
	parentPtr = rootItemPtr;
    }

    /*
     * Find all of the items to be moved and remove them from the list, making
     * an auxiliary list running from firstMovePtr to lastMovePtr. Record
     * their areas for redisplay.
     */
    firstMovePtr = lastMovePtr = NULL;
    FOR_EVERY_CANVAS_ITEM_MATCHING(tag, searchPtrPtr, return TCL_ERROR) {
	if (itemPtr->parentPtr == NULL) {
	    continue;
	}
	if (itemPtr->parentPtr != parentPtr) {
	    continue;
	}
	if (itemPtr == prevPtr) {
	    /*
	     * Item after which insertion is to occur is being moved! Switch
	     * to insert after its predecessor.
	     */

	    prevPtr = prevPtr->prevPtr;
	}

	/*
	 * Detach (splice out) item to be moved.
	 */
	if (itemPtr->parentPtr->firstChildPtr == itemPtr) {
	    itemPtr->parentPtr->firstChildPtr = itemPtr->nextPtr;
	}
	if (itemPtr->parentPtr->lastChildPtr == itemPtr) {
	    itemPtr->parentPtr->lastChildPtr = itemPtr->prevPtr;
	}
	if (itemPtr->prevPtr != NULL) {
	    itemPtr->prevPtr->nextPtr = itemPtr->nextPtr;
	}
	if (itemPtr->nextPtr != NULL) {
	    itemPtr->nextPtr->prevPtr = itemPtr->prevPtr;
	}
	
	/*
	 * Place moved item as the last item of the
	 * moved linked list.
	 */
	if (firstMovePtr == NULL) {
	    itemPtr->prevPtr = NULL;
	    itemPtr->nextPtr = NULL;
	    firstMovePtr = itemPtr;
	} else {
	    itemPtr->prevPtr = lastMovePtr;
	    lastMovePtr->nextPtr = itemPtr;
	}
	lastMovePtr = itemPtr;
	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
	canvasPtr->flags |= REPICK_NEEDED;
    }

    if (firstMovePtr == NULL) {
#ifdef USE_OLD_TAG_SEARCH
	return;
#else /* USE_OLD_TAG_SEARCH */
	return TCL_OK;
#endif /* USE_OLD_TAG_SEARCH */
    }
    
    /*
     * Insert the list of to-be-moved items back into the canvas's at the
     * desired position.
     */
    firstMovePtr->prevPtr = prevPtr;
    if (prevPtr != NULL) {
	if (prevPtr->nextPtr != NULL) {
	    prevPtr->nextPtr->prevPtr = lastMovePtr;
	}
	lastMovePtr->nextPtr = prevPtr->nextPtr;
	prevPtr->nextPtr = firstMovePtr;	
    } else {
	if (parentPtr->firstChildPtr != NULL) {
	    parentPtr->firstChildPtr->prevPtr = lastMovePtr;
	}
	lastMovePtr->nextPtr = parentPtr->firstChildPtr;
        parentPtr->firstChildPtr = firstMovePtr;
    }
    if (parentPtr->lastChildPtr == prevPtr) {
	parentPtr->lastChildPtr = lastMovePtr;
    }

#ifndef USE_OLD_TAG_SEARCH
    return TCL_OK;
#endif /* not USE_OLD_TAG_SEARCH */
}

/*
 *--------------------------------------------------------------
 *
 * CanvasBindProc --
 *
 *	This function is invoked by the Tk dispatcher to handle events
 *	associated with bindings on items.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the command invoked as part of the binding (if there was
 *	any).
 *
 *--------------------------------------------------------------
 */

static void
CanvasBindProc(
    ClientData clientData,	/* Pointer to canvas structure. */
    XEvent *eventPtr)		/* Pointer to X event that just happened. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;

    Tcl_Preserve((ClientData) canvasPtr);

    /*
     * This code below keeps track of the current modifier state in
     * canvasPtr>state. This information is used to defer repicks of the
     * current item while buttons are down.
     */

    if ((eventPtr->type == ButtonPress) || (eventPtr->type == ButtonRelease)) {
	int mask;

	switch (eventPtr->xbutton.button) {
	case Button1:
	    mask = Button1Mask;
	    break;
	case Button2:
	    mask = Button2Mask;
	    break;
	case Button3:
	    mask = Button3Mask;
	    break;
	case Button4:
	    mask = Button4Mask;
	    break;
	case Button5:
	    mask = Button5Mask;
	    break;
	default:
	    mask = 0;
	    break;
	}

	/*
	 * For button press events, repick the current item using the button
	 * state before the event, then process the event. For button release
	 * events, first process the event, then repick the current item using
	 * the button state *after* the event (the button has logically gone
	 * up before we change the current item).
	 */

	if (eventPtr->type == ButtonPress) {
	    /*
	     * On a button press, first repick the current item using the
	     * button state before the event, the process the event.
	     */

	    canvasPtr->state = eventPtr->xbutton.state;
	    PickCurrentItem(canvasPtr, eventPtr);
	    canvasPtr->state ^= mask;
	    CanvasDoEvent(canvasPtr, eventPtr);
	} else {
	    /*
	     * Button release: first process the event, with the button still
	     * considered to be down. Then repick the current item under the
	     * assumption that the button is no longer down.
	     */

	    canvasPtr->state = eventPtr->xbutton.state;
	    CanvasDoEvent(canvasPtr, eventPtr);
	    eventPtr->xbutton.state ^= mask;
	    canvasPtr->state = eventPtr->xbutton.state;
	    PickCurrentItem(canvasPtr, eventPtr);
	    eventPtr->xbutton.state ^= mask;
	}
	goto done;
    } else if ((eventPtr->type == EnterNotify)
	    || (eventPtr->type == LeaveNotify)) {
	canvasPtr->state = eventPtr->xcrossing.state;
	PickCurrentItem(canvasPtr, eventPtr);
	goto done;
    } else if (eventPtr->type == MotionNotify) {
	canvasPtr->state = eventPtr->xmotion.state;
	PickCurrentItem(canvasPtr, eventPtr);
    }
    CanvasDoEvent(canvasPtr, eventPtr);

  done:
    Tcl_Release((ClientData) canvasPtr);
}

/*
 *--------------------------------------------------------------
 *
 * PickCurrentItem --
 *
 *	Find the topmost item in a canvas that contains a given location and
 *	mark the the current item. If the current item has changed, generate a
 *	fake exit event on the old current item, a fake enter event on the new
 *	current item item and force a redraw of the two items. Canvas items
 *	that are hidden or disabled are ignored.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current item for canvasPtr may change. If it does, then the
 *	commands associated with item entry and exit could do just about
 *	anything. A binding script could delete the canvas, so callers should
 *	protect themselves with Tcl_Preserve and Tcl_Release.
 *
 *--------------------------------------------------------------
 */

static void
PickCurrentItem(
    TkPathCanvas *canvasPtr,	/* Canvas widget in which to select current
				 * item. */
    XEvent *eventPtr)		/* Event describing location of mouse cursor.
				 * Must be EnterWindow, LeaveWindow,
				 * ButtonRelease, or MotionNotify. */
{
    double coords[2];
    int buttonDown;
    Tk_PathItem *prevItemPtr;
#ifndef USE_OLD_TAG_SEARCH
    SearchUids *searchUids = GetStaticUids();
#endif

    /*
     * Check whether or not a button is down. If so, we'll log entry and exit
     * into and out of the current item, but not entry into any other item.
     * This implements a form of grabbing equivalent to what the X server does
     * for windows.
     */

    buttonDown = canvasPtr->state
	    & (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask);
    if (!buttonDown) {
	canvasPtr->flags &= ~LEFT_GRABBED_ITEM;
    }

    /*
     * Save information about this event in the canvas. The event in the
     * canvas is used for two purposes:
     *
     * 1. Event bindings: if the current item changes, fake events are
     *    generated to allow item-enter and item-leave bindings to trigger.
     * 2. Reselection: if the current item gets deleted, can use the saved
     *    event to find a new current item.
     *
     * Translate MotionNotify events into EnterNotify events, since that's
     * what gets reported to item handlers.
     */

    if (eventPtr != &canvasPtr->pickEvent) {
	if ((eventPtr->type == MotionNotify)
		|| (eventPtr->type == ButtonRelease)) {
	    canvasPtr->pickEvent.xcrossing.type = EnterNotify;
	    canvasPtr->pickEvent.xcrossing.serial = eventPtr->xmotion.serial;
	    canvasPtr->pickEvent.xcrossing.send_event
		    = eventPtr->xmotion.send_event;
	    canvasPtr->pickEvent.xcrossing.display = eventPtr->xmotion.display;
	    canvasPtr->pickEvent.xcrossing.window = eventPtr->xmotion.window;
	    canvasPtr->pickEvent.xcrossing.root = eventPtr->xmotion.root;
	    canvasPtr->pickEvent.xcrossing.subwindow = None;
	    canvasPtr->pickEvent.xcrossing.time = eventPtr->xmotion.time;
	    canvasPtr->pickEvent.xcrossing.x = eventPtr->xmotion.x;
	    canvasPtr->pickEvent.xcrossing.y = eventPtr->xmotion.y;
	    canvasPtr->pickEvent.xcrossing.x_root = eventPtr->xmotion.x_root;
	    canvasPtr->pickEvent.xcrossing.y_root = eventPtr->xmotion.y_root;
	    canvasPtr->pickEvent.xcrossing.mode = NotifyNormal;
	    canvasPtr->pickEvent.xcrossing.detail = NotifyNonlinear;
	    canvasPtr->pickEvent.xcrossing.same_screen
		    = eventPtr->xmotion.same_screen;
	    canvasPtr->pickEvent.xcrossing.focus = False;
	    canvasPtr->pickEvent.xcrossing.state = eventPtr->xmotion.state;
	} else  {
	    canvasPtr->pickEvent = *eventPtr;
	}
    }

    /*
     * If this is a recursive call (there's already a partially completed call
     * pending on the stack; it's in the middle of processing a Leave event
     * handler for the old current item) then just return; the pending call
     * will do everything that's needed.
     */

    if (canvasPtr->flags & REPICK_IN_PROGRESS) {
	return;
    }

    /*
     * A LeaveNotify event automatically means that there's no current object,
     * so the check for closest item can be skipped.
     */

    coords[0] = canvasPtr->pickEvent.xcrossing.x + canvasPtr->xOrigin;
    coords[1] = canvasPtr->pickEvent.xcrossing.y + canvasPtr->yOrigin;
    if (canvasPtr->pickEvent.type != LeaveNotify) {
	canvasPtr->newCurrentPtr = CanvasFindClosest(canvasPtr, coords);
    } else {
	canvasPtr->newCurrentPtr = NULL;
    }

    if ((canvasPtr->newCurrentPtr == canvasPtr->currentItemPtr)
	    && !(canvasPtr->flags & LEFT_GRABBED_ITEM)) {
	/*
	 * Nothing to do:  the current item hasn't changed.
	 */

	return;
    }

    /*
     * Simulate a LeaveNotify event on the previous current item and an
     * EnterNotify event on the new current item. Remove the "current" tag
     * from the previous current item and place it on the new current item.
     */

    if ((canvasPtr->newCurrentPtr != canvasPtr->currentItemPtr)
	    && (canvasPtr->currentItemPtr != NULL)
	    && !(canvasPtr->flags & LEFT_GRABBED_ITEM)) {
	XEvent event;
	Tk_PathItem *itemPtr = canvasPtr->currentItemPtr;
	Tk_PathTags *ptagsPtr;
	int i;

	event = canvasPtr->pickEvent;
	event.type = LeaveNotify;

	/*
	 * If the event's detail happens to be NotifyInferior the binding
	 * mechanism will discard the event. To be consistent, always use
	 * NotifyAncestor.
	 */

	event.xcrossing.detail = NotifyAncestor;
	canvasPtr->flags |= REPICK_IN_PROGRESS;
	CanvasDoEvent(canvasPtr, &event);
	canvasPtr->flags &= ~REPICK_IN_PROGRESS;

	/*
	 * The check below is needed because there could be an event handler
	 * for <LeaveNotify> that deletes the current item.
	 */

	if ((itemPtr == canvasPtr->currentItemPtr) && !buttonDown && 
		(itemPtr->pathTagsPtr != NULL)) {
	    ptagsPtr = itemPtr->pathTagsPtr;
	    for (i = ptagsPtr->numTags-1; i >= 0; i--) {
#ifdef USE_OLD_TAG_SEARCH
		if (ptagsPtr->tagPtr[i] == Tk_GetUid("current"))
#else /* USE_OLD_TAG_SEARCH */
		if (ptagsPtr->tagPtr[i] == searchUids->currentUid)
#endif /* USE_OLD_TAG_SEARCH */
		    /* then */ {
		    ptagsPtr->tagPtr[i] = ptagsPtr->tagPtr[ptagsPtr->numTags-1];
		    ptagsPtr->numTags--;
		    break;
		}
	    }
	}

	/*
	 * Note: during CanvasDoEvent above, it's possible that
	 * canvasPtr->newCurrentPtr got reset to NULL because the item was
	 * deleted.
	 */
    }
    if ((canvasPtr->newCurrentPtr != canvasPtr->currentItemPtr) && buttonDown) {
	canvasPtr->flags |= LEFT_GRABBED_ITEM;
	return;
    }

    /*
     * Special note: it's possible that canvasPtr->newCurrentPtr ==
     * canvasPtr->currentItemPtr here. This can happen, for example, if
     * LEFT_GRABBED_ITEM was set.
     */

    prevItemPtr = canvasPtr->currentItemPtr;
    canvasPtr->flags &= ~LEFT_GRABBED_ITEM;
    canvasPtr->currentItemPtr = canvasPtr->newCurrentPtr;
    if (prevItemPtr != NULL && prevItemPtr != canvasPtr->currentItemPtr &&
	    (prevItemPtr->redraw_flags & TK_ITEM_STATE_DEPENDANT)) {
	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, prevItemPtr);
	(*prevItemPtr->typePtr->configProc)(canvasPtr->interp,
		(Tk_PathCanvas) canvasPtr, prevItemPtr, 0, NULL,
		TK_CONFIG_ARGV_ONLY);
    }
    if (canvasPtr->currentItemPtr != NULL) {
	XEvent event;

#ifdef USE_OLD_TAG_SEARCH
	DoItem(NULL, canvasPtr->currentItemPtr, Tk_GetUid("current"));
#else /* USE_OLD_TAG_SEARCH */
	DoItem(NULL, canvasPtr->currentItemPtr, searchUids->currentUid);
#endif /* USE_OLD_TAG_SEA */
	if ((canvasPtr->currentItemPtr->redraw_flags & TK_ITEM_STATE_DEPENDANT &&
		prevItemPtr != canvasPtr->currentItemPtr)) {
	    (*canvasPtr->currentItemPtr->typePtr->configProc)(canvasPtr->interp,
		    (Tk_PathCanvas) canvasPtr, canvasPtr->currentItemPtr, 0, NULL,
		    TK_CONFIG_ARGV_ONLY);
	    EventuallyRedrawItem((Tk_PathCanvas) canvasPtr,
		    canvasPtr->currentItemPtr);
	}
	event = canvasPtr->pickEvent;
	event.type = EnterNotify;
	event.xcrossing.detail = NotifyAncestor;
	CanvasDoEvent(canvasPtr, &event);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CanvasFindClosest --
 *
 *	Given x and y coordinates, find the topmost canvas item that is
 *	"close" to the coordinates. Canvas items that are hidden or disabled
 *	are ignored.
 *
 * Results:
 *	The return value is a pointer to the topmost item that is close to
 *	(x,y), or NULL if no item is close.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tk_PathItem *
CanvasFindClosest(
    TkPathCanvas *canvasPtr,	/* Canvas widget to search. */
    double coords[2])		/* Desired x,y position in canvas, not screen,
				 * coordinates.) */
{
    Tk_PathItem *itemPtr;
    Tk_PathItem *bestPtr;
    int x1, y1, x2, y2;

    x1 = (int) (coords[0] - canvasPtr->closeEnough);
    y1 = (int) (coords[1] - canvasPtr->closeEnough);
    x2 = (int) (coords[0] + canvasPtr->closeEnough);
    y2 = (int) (coords[1] + canvasPtr->closeEnough);

    bestPtr = NULL;
    for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	if (itemPtr->state == TK_PATHSTATE_HIDDEN || itemPtr->state==TK_PATHSTATE_DISABLED ||
		(itemPtr->state == TK_PATHSTATE_NULL && (canvasPtr->canvas_state == TK_PATHSTATE_HIDDEN ||
		canvasPtr->canvas_state == TK_PATHSTATE_DISABLED))) {
	    continue;
	}
	if ((itemPtr->x1 > x2) || (itemPtr->x2 < x1)
		|| (itemPtr->y1 > y2) || (itemPtr->y2 < y1)) {
	    continue;
	}
	if ((*itemPtr->typePtr->pointProc)((Tk_PathCanvas) canvasPtr,
		itemPtr, coords) <= canvasPtr->closeEnough) {
	    bestPtr = itemPtr;
	}
    }
    return bestPtr;
}

/*
 *--------------------------------------------------------------
 *
 * CanvasDoEvent --
 *
 *	This function is called to invoke binding processing for a new event
 *	that is associated with the current item for a canvas.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the bindings for the canvas. A binding script could delete
 *	the canvas, so callers should protect themselves with Tcl_Preserve and
 *	Tcl_Release.
 *
 *--------------------------------------------------------------
 */

static void
CanvasDoEvent(
    TkPathCanvas *canvasPtr,	/* Canvas widget in which event occurred. */
    XEvent *eventPtr)		/* Real or simulated X event that is to be
				 * processed. */
{
#define NUM_STATIC 3
    ClientData staticObjects[NUM_STATIC];
    ClientData *objectPtr;
    int numObjects, i;
    int numTags;
    Tk_PathItem *itemPtr;
    Tk_PathTags *ptagsPtr;
#ifndef USE_OLD_TAG_SEARCH
    TagSearchExpr *expr;
    int numExprs;
    SearchUids *searchUids = GetStaticUids();
#endif /* not USE_OLD_TAG_SEARCH */

    if (canvasPtr->bindingTable == NULL) {
	return;
    }

    itemPtr = canvasPtr->currentItemPtr;
    if ((eventPtr->type == KeyPress) || (eventPtr->type == KeyRelease)) {
	itemPtr = canvasPtr->textInfo.focusItemPtr;
    }
    if (itemPtr == NULL) {
	return;
    }
    ptagsPtr = itemPtr->pathTagsPtr;
    numTags = ItemGetNumTags(itemPtr);

#ifdef USE_OLD_TAG_SEARCH
    /*
     * Set up an array with all the relevant objects for processing this
     * event. The relevant objects are (a) the event's item, (b) the tags
     * associated with the event's item, and (c) the tag "all". If there are a
     * lot of tags then malloc an array to hold all of the objects.
     */

    numObjects = numTags + 2;

#else /* USE_OLD_TAG_SEARCH */
    /*
     * Set up an array with all the relevant objects for processing this
     * event. The relevant objects are:
     * (a) the event's item,
     * (b) the tags associated with the event's item,
     * (c) the expressions that are true for the event's item's tags, and
     * (d) the tag "all".
     *
     * If there are a lot of tags then malloc an array to hold all of the
     * objects.
     */

    /*
     * Flag and count all expressions that match item's tags.
     */

    numExprs = 0;
    expr = canvasPtr->bindTagExprs;
    while (expr) {
	expr->index = 0;
    	expr->match = TagSearchEvalExpr(expr, itemPtr);
	if (expr->match) {
	    numExprs++;
	}
	expr = expr->next;
    }
    numObjects = numTags + numExprs + 2;

#endif /* not USE_OLD_TAG_SEARCH */

    if (numObjects <= NUM_STATIC) {
	objectPtr = staticObjects;
    } else {
	objectPtr = (ClientData *) ckalloc((unsigned)
		(numObjects * sizeof(ClientData)));
    }
#ifdef USE_OLD_TAG_SEARCH
    objectPtr[0] = (ClientData) Tk_GetUid("all");
#else /* USE_OLD_TAG_SEARCH */
    objectPtr[0] = (ClientData) searchUids->allUid;
#endif /* USE_OLD_TAG_SEARCH */

    if (ptagsPtr != NULL) {
	for (i = ptagsPtr->numTags-1; i >= 0; i--) {
	    objectPtr[i+1] = (ClientData) ptagsPtr->tagPtr[i];
	}
    }
    objectPtr[numTags+1] = (ClientData) itemPtr;

#ifndef USE_OLD_TAG_SEARCH
    /*
     * Copy uids of matching expressions into object array
     */

    i = numTags + 2;
    expr = canvasPtr->bindTagExprs;
    while (expr) {
    	if (expr->match) {
	    objectPtr[i++] = (int *) expr->uid;
	}
	expr = expr->next;
    }
#endif /* not USE_OLD_TAG_SEARCH */

    /*
     * Invoke the binding system, then free up the object array if it was
     * malloc-ed.
     */

    if (canvasPtr->tkwin != NULL) {
	Tk_BindEvent(canvasPtr->bindingTable, eventPtr, canvasPtr->tkwin,
		numObjects, objectPtr);
    }
    if (objectPtr != staticObjects) {
	ckfree((char *) objectPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CanvasBlinkProc --
 *
 *	This function is called as a timer handler to blink the insertion
 *	cursor off and on.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off, redisplay gets invoked, and this
 *	function reschedules itself.
 *
 *----------------------------------------------------------------------
 */

static void
CanvasBlinkProc(
    ClientData clientData)	/* Pointer to record describing entry. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;

    if (!canvasPtr->textInfo.gotFocus || (canvasPtr->insertOffTime == 0)) {
	return;
    }
    if (canvasPtr->textInfo.cursorOn) {
	canvasPtr->textInfo.cursorOn = 0;
	canvasPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		canvasPtr->insertOffTime, CanvasBlinkProc,
		(ClientData) canvasPtr);
    } else {
	canvasPtr->textInfo.cursorOn = 1;
	canvasPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		canvasPtr->insertOnTime, CanvasBlinkProc,
		(ClientData) canvasPtr);
    }
    if (canvasPtr->textInfo.focusItemPtr != NULL) {
	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr,
		canvasPtr->textInfo.focusItemPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CanvasFocusProc --
 *
 *	This function is called whenever a canvas gets or loses the input
 *	focus. It's also called whenever the window is reconfigured while it
 *	has the focus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off.
 *
 *----------------------------------------------------------------------
 */

static void
CanvasFocusProc(
    TkPathCanvas *canvasPtr,	/* Canvas that just got or lost focus. */
    int gotFocus)		/* 1 means window is getting focus, 0 means
				 * it's losing it. */
{
    Tcl_DeleteTimerHandler(canvasPtr->insertBlinkHandler);
    if (gotFocus) {
	canvasPtr->textInfo.gotFocus = 1;
	canvasPtr->textInfo.cursorOn = 1;
	if (canvasPtr->insertOffTime != 0) {
	    canvasPtr->insertBlinkHandler = Tcl_CreateTimerHandler(
		    canvasPtr->insertOffTime, CanvasBlinkProc,
		    (ClientData) canvasPtr);
	}
    } else {
	canvasPtr->textInfo.gotFocus = 0;
	canvasPtr->textInfo.cursorOn = 0;
	canvasPtr->insertBlinkHandler = (Tcl_TimerToken) NULL;
    }
    if (canvasPtr->textInfo.focusItemPtr != NULL) {
	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr,
		canvasPtr->textInfo.focusItemPtr);
    }
    if (canvasPtr->highlightWidth > 0) {
	canvasPtr->flags |= REDRAW_BORDERS;
	if (!(canvasPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayCanvas, (ClientData) canvasPtr);
	    canvasPtr->flags |= REDRAW_PENDING;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CanvasSelectTo --
 *
 *	Modify the selection by moving its un-anchored end. This could make
 *	the selection either larger or smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *----------------------------------------------------------------------
 */

static void
CanvasSelectTo(
    TkPathCanvas *canvasPtr,	/* Information about widget. */
    Tk_PathItem *itemPtr,	/* Item that is to hold selection. */
    int index)			/* Index of element that is to become the
				 * "other" end of the selection. */
{
    int oldFirst, oldLast;
    Tk_PathItem *oldSelPtr;

    oldFirst = canvasPtr->textInfo.selectFirst;
    oldLast = canvasPtr->textInfo.selectLast;
    oldSelPtr = canvasPtr->textInfo.selItemPtr;

    /*
     * Grab the selection if we don't own it already.
     */

    if (canvasPtr->textInfo.selItemPtr == NULL) {
	Tk_OwnSelection(canvasPtr->tkwin, XA_PRIMARY, CanvasLostSelection,
		(ClientData) canvasPtr);
    } else if (canvasPtr->textInfo.selItemPtr != itemPtr) {
	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr,
		canvasPtr->textInfo.selItemPtr);
    }
    canvasPtr->textInfo.selItemPtr = itemPtr;

    if (canvasPtr->textInfo.anchorItemPtr != itemPtr) {
	canvasPtr->textInfo.anchorItemPtr = itemPtr;
	canvasPtr->textInfo.selectAnchor = index;
    }
    if (canvasPtr->textInfo.selectAnchor <= index) {
	canvasPtr->textInfo.selectFirst = canvasPtr->textInfo.selectAnchor;
	canvasPtr->textInfo.selectLast = index;
    } else {
	canvasPtr->textInfo.selectFirst = index;
	canvasPtr->textInfo.selectLast = canvasPtr->textInfo.selectAnchor - 1;
    }
    if ((canvasPtr->textInfo.selectFirst != oldFirst)
	    || (canvasPtr->textInfo.selectLast != oldLast)
	    || (itemPtr != oldSelPtr)) {
	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr, itemPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * CanvasFetchSelection --
 *
 *	This function is invoked by Tk to return part or all of the selection,
 *	when the selection is in a canvas widget. This function always returns
 *	the selection as a STRING.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored at buffer.
 *	Buffer is filled (or partially filled) with a NULL-terminated string
 *	containing part or all of the selection, as given by offset and
 *	maxBytes.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
CanvasFetchSelection(
    ClientData clientData,	/* Information about canvas widget. */
    int offset,			/* Offset within selection of first character
				 * to be returned. */
    char *buffer,		/* Location in which to place selection. */
    int maxBytes)		/* Maximum number of bytes to place at buffer,
				 * not including terminating NULL
				 * character. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;

    if (canvasPtr->textInfo.selItemPtr == NULL) {
	return -1;
    }
    if (canvasPtr->textInfo.selItemPtr->typePtr->selectionProc == NULL) {
	return -1;
    }
    return (*canvasPtr->textInfo.selItemPtr->typePtr->selectionProc)(
	    (Tk_PathCanvas) canvasPtr, canvasPtr->textInfo.selItemPtr, offset,
	    buffer, maxBytes);
}

/*
 *----------------------------------------------------------------------
 *
 * CanvasLostSelection --
 *
 *	This function is called back by Tk when the selection is grabbed away
 *	from a canvas widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is marked as
 *	not containing a selection.
 *
 *----------------------------------------------------------------------
 */

static void
CanvasLostSelection(
    ClientData clientData)	/* Information about entry widget. */
{
    TkPathCanvas *canvasPtr = (TkPathCanvas *) clientData;

    if (canvasPtr->textInfo.selItemPtr != NULL) {
	EventuallyRedrawItem((Tk_PathCanvas) canvasPtr,
		canvasPtr->textInfo.selItemPtr);
    }
    canvasPtr->textInfo.selItemPtr = NULL;
}

/*
 *--------------------------------------------------------------
 *
 * GridAlign --
 *
 *	Given a coordinate and a grid spacing, this function computes the
 *	location of the nearest grid line to the coordinate.
 *
 * Results:
 *	The return value is the location of the grid line nearest to coord.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static double
GridAlign(
    double coord,		/* Coordinate to grid-align. */
    double spacing)		/* Spacing between grid lines. If <= 0 then no
				 * alignment is done. */
{
    if (spacing <= 0.0) {
	return coord;
    }
    if (coord < 0) {
	return -((int) ((-coord)/spacing + 0.5)) * spacing;
    }
    return ((int) (coord/spacing + 0.5)) * spacing;
}

/*
 *----------------------------------------------------------------------
 *
 * ScrollFractions --
 *
 *	Given the range that's visible in the window and the "100% range" for
 *	what's in the canvas, return a list of two doubles representing the
 *	scroll fractions. This function is used for both x and y scrolling.
 *
 * Results:
 *	A List Tcl_Obj with two real numbers (Double Tcl_Objs) containing the
 *	scroll fractions (between 0 and 1) corresponding to the other
 *	arguments.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
ScrollFractions(
    int screen1,		/* Lowest coordinate visible in the window. */
    int screen2,		/* Highest coordinate visible in the window. */
    int object1,		/* Lowest coordinate in the object. */
    int object2)		/* Highest coordinate in the object. */
{
    Tcl_Obj *buffer[2];
    double range, f1, f2;

    range = object2 - object1;
    if (range <= 0) {
	f1 = 0;
	f2 = 1.0;
    } else {
	f1 = (screen1 - object1)/range;
	if (f1 < 0) {
	    f1 = 0.0;
	}
	f2 = (screen2 - object1)/range;
	if (f2 > 1.0) {
	    f2 = 1.0;
	}
	if (f2 < f1) {
	    f2 = f1;
	}
    }
    buffer[0] = Tcl_NewDoubleObj(f1);
    buffer[1] = Tcl_NewDoubleObj(f2);
    return Tcl_NewListObj(2, buffer);
}

/*
 *--------------------------------------------------------------
 *
 * CanvasUpdateScrollbars --
 *
 *	This function is invoked whenever a canvas has changed in a way that
 *	requires scrollbars to be redisplayed (e.g. the view in the canvas has
 *	changed).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there are scrollbars associated with the canvas, then their
 *	scrolling commands are invoked to cause them to redisplay. If errors
 *	occur, additional Tcl commands may be invoked to process the errors.
 *
 *--------------------------------------------------------------
 */

static void
CanvasUpdateScrollbars(
    TkPathCanvas *canvasPtr)	/* Information about canvas. */
{
    int result;
    Tcl_Interp *interp;
    int xOrigin, yOrigin, inset, width, height;
    int scrollX1, scrollX2, scrollY1, scrollY2;
    char *xScrollCmd, *yScrollCmd;

    /*
     * Save all the relevant values from the canvasPtr, because it might be
     * deleted as part of either of the two calls to Tcl_VarEval below.
     */

    interp = canvasPtr->interp;
    Tcl_Preserve((ClientData) interp);
    xScrollCmd = canvasPtr->xScrollCmd;
    if (xScrollCmd != NULL) {
	Tcl_Preserve((ClientData) xScrollCmd);
    }
    yScrollCmd = canvasPtr->yScrollCmd;
    if (yScrollCmd != NULL) {
	Tcl_Preserve((ClientData) yScrollCmd);
    }
    xOrigin = canvasPtr->xOrigin;
    yOrigin = canvasPtr->yOrigin;
    inset = canvasPtr->inset;
    width = Tk_Width(canvasPtr->tkwin);
    height = Tk_Height(canvasPtr->tkwin);
    scrollX1 = canvasPtr->scrollX1;
    scrollX2 = canvasPtr->scrollX2;
    scrollY1 = canvasPtr->scrollY1;
    scrollY2 = canvasPtr->scrollY2;
    canvasPtr->flags &= ~UPDATE_SCROLLBARS;
    if (canvasPtr->xScrollCmd != NULL) {
	Tcl_Obj *fractions = ScrollFractions(xOrigin + inset,
		xOrigin + width - inset, scrollX1, scrollX2);
	result = Tcl_VarEval(interp, xScrollCmd, " ", Tcl_GetString(fractions),
		NULL);
	Tcl_DecrRefCount(fractions);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(interp);
	}
	Tcl_ResetResult(interp);
	Tcl_Release((ClientData) xScrollCmd);
    }

    if (yScrollCmd != NULL) {
	Tcl_Obj *fractions = ScrollFractions(yOrigin + inset,
		yOrigin + height - inset, scrollY1, scrollY2);
	result = Tcl_VarEval(interp, yScrollCmd, " ", Tcl_GetString(fractions),
		NULL);
	Tcl_DecrRefCount(fractions);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(interp);
	}
	Tcl_ResetResult(interp);
	Tcl_Release((ClientData) yScrollCmd);
    }
    Tcl_Release((ClientData) interp);
}

/*
 *--------------------------------------------------------------
 *
 * CanvasSetOrigin --
 *
 *	This function is invoked to change the mapping between canvas
 *	coordinates and screen coordinates in the canvas window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The canvas will be redisplayed to reflect the change in view. In
 *	addition, scrollbars will be updated if there are any.
 *
 *--------------------------------------------------------------
 */

static void
CanvasSetOrigin(
    TkPathCanvas *canvasPtr,	/* Information about canvas. */
    int xOrigin,		/* New X origin for canvas (canvas x-coord
				 * corresponding to left edge of canvas
				 * window). */
    int yOrigin)		/* New Y origin for canvas (canvas y-coord
				 * corresponding to top edge of canvas
				 * window). */
{
    int left, right, top, bottom, delta;

    /*
     * If scroll increments have been set, round the window origin to the
     * nearest multiple of the increments. Remember, the origin is the place
     * just inside the borders, not the upper left corner.
     */

    if (canvasPtr->xScrollIncrement > 0) {
	if (xOrigin >= 0) {
	    xOrigin += canvasPtr->xScrollIncrement/2;
	    xOrigin -= (xOrigin + canvasPtr->inset)
		    % canvasPtr->xScrollIncrement;
	} else {
	    xOrigin = (-xOrigin) + canvasPtr->xScrollIncrement/2;
	    xOrigin = -(xOrigin - (xOrigin - canvasPtr->inset)
		    % canvasPtr->xScrollIncrement);
	}
    }
    if (canvasPtr->yScrollIncrement > 0) {
	if (yOrigin >= 0) {
	    yOrigin += canvasPtr->yScrollIncrement/2;
	    yOrigin -= (yOrigin + canvasPtr->inset)
		    % canvasPtr->yScrollIncrement;
	} else {
	    yOrigin = (-yOrigin) + canvasPtr->yScrollIncrement/2;
	    yOrigin = -(yOrigin - (yOrigin - canvasPtr->inset)
		    % canvasPtr->yScrollIncrement);
	}
    }

    /*
     * Adjust the origin if necessary to keep as much as possible of the
     * canvas in the view. The variables left, right, etc. keep track of how
     * much extra space there is on each side of the view before it will stick
     * out past the scroll region.  If one side sticks out past the edge of
     * the scroll region, adjust the view to bring that side back to the edge
     * of the scrollregion (but don't move it so much that the other side
     * sticks out now). If scroll increments are in effect, be sure to adjust
     * only by full increments.
     */

    if ((canvasPtr->confine) && (canvasPtr->regionString != NULL)) {
	left = xOrigin + canvasPtr->inset - canvasPtr->scrollX1;
	right = canvasPtr->scrollX2
		- (xOrigin + Tk_Width(canvasPtr->tkwin) - canvasPtr->inset);
	top = yOrigin + canvasPtr->inset - canvasPtr->scrollY1;
	bottom = canvasPtr->scrollY2
		- (yOrigin + Tk_Height(canvasPtr->tkwin) - canvasPtr->inset);
	if ((left < 0) && (right > 0)) {
	    delta = (right > -left) ? -left : right;
	    if (canvasPtr->xScrollIncrement > 0) {
		delta -= delta % canvasPtr->xScrollIncrement;
	    }
	    xOrigin += delta;
	} else if ((right < 0) && (left > 0)) {
	    delta = (left > -right) ? -right : left;
	    if (canvasPtr->xScrollIncrement > 0) {
		delta -= delta % canvasPtr->xScrollIncrement;
	    }
	    xOrigin -= delta;
	}
	if ((top < 0) && (bottom > 0)) {
	    delta = (bottom > -top) ? -top : bottom;
	    if (canvasPtr->yScrollIncrement > 0) {
		delta -= delta % canvasPtr->yScrollIncrement;
	    }
	    yOrigin += delta;
	} else if ((bottom < 0) && (top > 0)) {
	    delta = (top > -bottom) ? -bottom : top;
	    if (canvasPtr->yScrollIncrement > 0) {
		delta -= delta % canvasPtr->yScrollIncrement;
	    }
	    yOrigin -= delta;
	}
    }

    if ((xOrigin == canvasPtr->xOrigin) && (yOrigin == canvasPtr->yOrigin)) {
	return;
    }

    /*
     * Tricky point: must redisplay not only everything that's visible in the
     * window's final configuration, but also everything that was visible in
     * the initial configuration. This is needed because some item types, like
     * windows, need to know when they move off-screen so they can explicitly
     * undisplay themselves.
     */

    Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr,
	    canvasPtr->xOrigin, canvasPtr->yOrigin,
	    canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin),
	    canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin));
    canvasPtr->xOrigin = xOrigin;
    canvasPtr->yOrigin = yOrigin;
    canvasPtr->flags |= UPDATE_SCROLLBARS;
    Tk_PathCanvasEventuallyRedraw((Tk_PathCanvas) canvasPtr,
	    canvasPtr->xOrigin, canvasPtr->yOrigin,
	    canvasPtr->xOrigin + Tk_Width(canvasPtr->tkwin),
	    canvasPtr->yOrigin + Tk_Height(canvasPtr->tkwin));
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetStringsFromObjs --
 *
 * Results:
 *	Converts object list into string list.
 *
 * Side effects:
 *	Memory is allocated for the objv array, which must be freed using
 *	ckfree() when no longer needed.
 *
 *----------------------------------------------------------------------
 */

// @@@ TODO: this shouldn't be needed when fully objectified!

/* ARGSUSED */
static CONST char **
TkGetStringsFromObjs(
    int objc,
    Tcl_Obj *CONST objv[])
{
    register int i;
    CONST char **argv;
    if (objc <= 0) {
	return NULL;
    }
    argv = (CONST char **) ckalloc((objc+1) * sizeof(char *));
    for (i = 0; i < objc; i++) {
	argv[i] = Tcl_GetString(objv[i]);
    }
    argv[objc] = 0;
    return argv;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasPsColor --
 *
 *	This function is called by individual canvas items when they want to
 *	set a color value for output. Given information about an X color, this
 *	function will generate Postscript commands to set up an appropriate
 *	color in Postscript.
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in interp->result. If no error occurs, then
 *	additional Postscript will be appended to interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasPsColor(
    Tcl_Interp *interp,		/* Interpreter for returning Postscript or
				 * error message. */
    Tk_PathCanvas canvas,	/* Information about canvas. */
    XColor *colorPtr)		/* Information about color. */
{
    return Tk_PostscriptColor(interp, ((TkPathCanvas *) canvas)->psInfo,
	    colorPtr);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasPsFont --
 *
 *	This function is called by individual canvas items when they want to
 *	output text. Given information about an X font, this function will
 *	generate Postscript commands to set up an appropriate font in
 *	Postscript.
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in interp->result. If no error occurs, then
 *	additional Postscript will be appended to the interp->result.
 *
 * Side effects:
 *	The Postscript font name is entered into psInfoPtr->fontTable if it
 *	wasn't already there.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasPsFont(
    Tcl_Interp *interp,		/* Interpreter for returning Postscript or
				 * error message. */
    Tk_PathCanvas canvas,	/* Information about canvas. */
    Tk_Font tkfont)		/* Information about font in which text is to
				 * be printed. */
{
    return Tk_PostscriptFont(interp, ((TkPathCanvas *) canvas)->psInfo, tkfont);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasPsBitmap --
 *
 *	This function is called to output the contents of a sub-region of a
 *	bitmap in proper image data format for Postscript (i.e. data between
 *	angle brackets, one bit per pixel).
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in interp->result. If no error occurs, then
 *	additional Postscript will be appended to interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasPsBitmap(
    Tcl_Interp *interp,		/* Interpreter for returning Postscript or
				 * error message. */
    Tk_PathCanvas canvas,	/* Information about canvas. */
    Pixmap bitmap,		/* Bitmap for which to generate Postscript. */
    int startX, int startY,	/* Coordinates of upper-left corner of
				 * rectangular region to output. */
    int width, int height)	/* Size of rectangular region. */
{
    return Tk_PostscriptBitmap(interp, ((TkPathCanvas *) canvas)->tkwin,
	    ((TkPathCanvas *) canvas)->psInfo, bitmap, startX, startY,
	    width, height);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasPsStipple --
 *
 *	This function is called by individual canvas items when they have
 *	created a path that they'd like to be filled with a stipple pattern.
 *	Given information about an X bitmap, this function will generate
 *	Postscript commands to fill the current clip region using a stipple
 *	pattern defined by the bitmap.
 *
 * Results:
 *	Returns a standard Tcl return value. If an error occurs then an error
 *	message will be left in interp->result. If no error occurs, then
 *	additional Postscript will be appended to interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Tk_PathCanvasPsStipple(
    Tcl_Interp *interp,		/* Interpreter for returning Postscript or
				 * error message. */
    Tk_PathCanvas canvas,	/* Information about canvas. */
    Pixmap bitmap)		/* Bitmap to use for stippling. */
{
    return Tk_PostscriptStipple(interp, ((TkPathCanvas *) canvas)->tkwin,
	    ((TkPathCanvas *) canvas)->psInfo, bitmap);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasPsY --
 *
 *	Given a y-coordinate in canvas coordinates, this function returns a
 *	y-coordinate to use for Postscript output.
 *
 * Results:
 *	Returns the Postscript coordinate that corresponds to "y".
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

double
Tk_PathCanvasPsY(
    Tk_PathCanvas canvas,	/* Token for canvas on whose behalf Postscript
				 * is being generated. */
    double y)			/* Y-coordinate in canvas coords. */
{
    return Tk_PostscriptY(y, ((TkPathCanvas *) canvas)->psInfo);
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathCanvasPsPath --
 *
 *	Given an array of points for a path, generate Postscript commands to
 *	create the path.
 *
 * Results:
 *	Postscript commands get appended to what's in interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

void
Tk_PathCanvasPsPath(
    Tcl_Interp *interp,		/* Put generated Postscript in this
				 * interpreter's result field. */
    Tk_PathCanvas canvas,	/* Canvas on whose behalf Postscript is being
				 * generated. */
    double *coordPtr,		/* Pointer to first in array of 2*numPoints
				 * coordinates giving points for path. */
    int numPoints)		/* Number of points at *coordPtr. */
{
    Tk_PostscriptPath(interp, ((TkPathCanvas *) canvas)->psInfo,
	    coordPtr, numPoints);
}

/*
 * PathCanvasGradientChanged: find all matching items with this gradient and redisplay.
 * If gradient deleted we must also update items style.
 */
 
void
PathCanvasGradientChanged(TkPathCanvas *canvasPtr, Tcl_Obj *gradientObj, int flags)
{
    Tk_PathItem *itemPtr;

    for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {

	    // EventuallyRedrawItem((Tk_PathCanvas)canvasPtr, itemPtr);

    }
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
