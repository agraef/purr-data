/*
 * tkPathSurface.c --
 *
 *	This file implements style objects used when drawing paths.
 *      See http://www.w3.org/TR/SVG11/.
 *
 * Copyright (c) 2007-2008  Mats Bengtsson
 *
 * $Id$
 */

#include "tkIntPath.h"
#include "tkPathStyle.h"

typedef struct PathSurface {
    TkPathContext ctx;
    char *token;
    int width;
    int height;
} PathSurface;

static Tcl_HashTable 	*surfaceHashPtr = NULL;

static int 	StaticSurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int 	NamesSurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int 	NewSurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int 	SurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
static int 	SurfaceCopyObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);
static int 	SurfaceDestroyObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr);
static void	SurfaceDeletedProc(ClientData clientData);
static int 	SurfaceCreateObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);
static int 	SurfaceEraseObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);

static int	SurfaceCreateEllipse(Tcl_Interp* interp, PathSurface *surfacePtr, int type, int objc, Tcl_Obj* CONST objv[]);
static int	SurfaceCreatePath(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);
static int	SurfaceCreatePimage(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);
static int	SurfaceCreatePline(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);
static int	SurfaceCreatePpoly(Tcl_Interp* interp, PathSurface *surfacePtr, int type, int objc, Tcl_Obj* CONST objv[]);
static int	SurfaceCreatePrect(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);
static int	SurfaceCreatePtext(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]);
static void	SurfaceInitOptions(Tcl_Interp* interp);

static int	uid = 0;
static char	*kSurfaceNameBase = "tkp::surface";

int
SurfaceInit(Tcl_Interp *interp)
{
    surfaceHashPtr = (Tcl_HashTable *) ckalloc( sizeof(Tcl_HashTable) );
    Tcl_InitHashTable(surfaceHashPtr, TCL_STRING_KEYS);

    Tcl_CreateObjCommand(interp, "::tkp::surface",
            StaticSurfaceObjCmd, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    SurfaceInitOptions(interp);
    return TCL_OK;
}

static CONST char *staticSurfaceCmds[] = {
    "names", "new", (char *) NULL
};

enum {
    kPathStaticSurfaceCmdNames	= 0L,
    kPathStaticSurfaceCmdNew
};

static int 
StaticSurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    int index;
    int result = TCL_OK;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "command ?arg arg...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], staticSurfaceCmds, "command", 0,
            &index) != TCL_OK) {
        return TCL_ERROR;
    }
    switch (index) {
        case kPathStaticSurfaceCmdNames: {
            result = NamesSurfaceObjCmd(clientData, interp, objc, objv);
            break;
        }
        case kPathStaticSurfaceCmdNew: {
            result = NewSurfaceObjCmd(clientData, interp, objc, objv);
            break;
        }
    }
    return result;
}

static int 	
NamesSurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    char	    *name;
    Tcl_HashEntry   *hPtr;
    Tcl_Obj	    *listObj;
    Tcl_HashSearch  search;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
    }
    listObj = Tcl_NewListObj(0, NULL);
    hPtr = Tcl_FirstHashEntry(surfaceHashPtr, &search);
    while (hPtr != NULL) {
        name = (char *) Tcl_GetHashKey(surfaceHashPtr, hPtr);
        Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(name, -1));
        hPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_SetObjResult(interp, listObj);
    return TCL_OK;
}

static int 
NewSurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    TkPathContext   ctx;
    PathSurface	    *surfacePtr;
    Tcl_HashEntry   *hPtr;
    char	    str[255];
    int		    width, height;
    int		    isNew;
    int		    result = TCL_OK;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "width height");
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &height) != TCL_OK) {
        return TCL_ERROR;
    }
    
    ctx = TkPathInitSurface(width, height);
    if (ctx == 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("Failed in TkPathInitSurface", -1));
        return TCL_ERROR;
    }

    sprintf(str, "%s%d", kSurfaceNameBase, uid++);
    surfacePtr = (PathSurface *) ckalloc( sizeof(PathSurface) );
    surfacePtr->token = (char *) ckalloc( (unsigned int)strlen(str) + 1 );
    strcpy(surfacePtr->token, str);
    surfacePtr->ctx = ctx;
    surfacePtr->width = width;
    surfacePtr->height = height;
    Tcl_CreateObjCommand(interp, str, SurfaceObjCmd, (ClientData) surfacePtr, SurfaceDeletedProc);

    hPtr = Tcl_CreateHashEntry(surfaceHashPtr, str, &isNew);
    Tcl_SetHashValue(hPtr, surfacePtr);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(str, -1));
    return result;
}

static CONST char *surfaceCmds[] = {
    "copy", 	"create", 	"destroy", 
    "erase", 	"height", 	"width",
    (char *) NULL
};

enum {
    kPathSurfaceCmdCopy		= 0L,
    kPathSurfaceCmdCreate,
    kPathSurfaceCmdDestroy,
    kPathSurfaceCmdErase,
    kPathSurfaceCmdHeight,
    kPathSurfaceCmdWidth
};

static int 
SurfaceObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])
{
    PathSurface *surfacePtr = (PathSurface *) clientData;
    int 	index;
    int 	result = TCL_OK;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "command ?arg arg...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], surfaceCmds, "command", 0,
            &index) != TCL_OK) {
        return TCL_ERROR;
    }
    switch (index) {
        case kPathSurfaceCmdCopy: {
            result = SurfaceCopyObjCmd(interp, surfacePtr, objc, objv);
            break;
        }
        case kPathSurfaceCmdCreate: {
            result = SurfaceCreateObjCmd(interp, surfacePtr, objc, objv);
            break;
        }
        case kPathSurfaceCmdDestroy: {
            result = SurfaceDestroyObjCmd(interp, surfacePtr);
            break;
        }
        case kPathSurfaceCmdErase: {
            result = SurfaceEraseObjCmd(interp, surfacePtr, objc, objv);
            break;
        }
        case kPathSurfaceCmdHeight:
        case kPathSurfaceCmdWidth: {
            if (objc != 2) {
                Tcl_WrongNumArgs(interp, 2, objv, NULL);
                return TCL_ERROR;
            }
            Tcl_SetObjResult(interp, Tcl_NewIntObj(
                    (index == kPathSurfaceCmdHeight) ? surfacePtr->height : surfacePtr->width));
            break;
        }
    }    
    return result;
}

static int 
SurfaceCopyObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[])
{
    Tk_PhotoHandle photo;
    
    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "image");
        return TCL_ERROR;
    }
    photo = Tk_FindPhoto( interp, Tcl_GetString(objv[2]) );
    if (photo == NULL) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("didn't find that image", -1));
        return TCL_ERROR;
    }
    TkPathSurfaceToPhoto(interp, surfacePtr->ctx, photo);
    Tcl_SetObjResult(interp, objv[2]);
    return TCL_OK;
}

static int 
SurfaceDestroyObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr)
{
    Tcl_DeleteCommand(interp, surfacePtr->token);
    return TCL_OK;
}

static void
SurfaceDeletedProc(ClientData clientData)
{
    PathSurface *surfacePtr = (PathSurface *) clientData;
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(surfaceHashPtr, surfacePtr->token);
    if (hPtr != NULL) {
        Tcl_DeleteHashEntry(hPtr);
    }
    TkPathFree(surfacePtr->ctx);
    ckfree(surfacePtr->token);
    ckfree((char *)surfacePtr);
}

// @@@ TODO: should we have a group item?

static CONST char *surfaceItemCmds[] = {
    "circle",    "ellipse",  "path", 
    "pimage",    "pline",    "polyline", 
    "ppolygon",  "prect",    "ptext",
    (char *) NULL
};

enum {
    kPathSurfaceItemCircle	= 0L,
    kPathSurfaceItemEllipse,
    kPathSurfaceItemPath,
    kPathSurfaceItemPimage,
    kPathSurfaceItemPline,
    kPathSurfaceItemPolyline,
    kPathSurfaceItemPpolygon,
    kPathSurfaceItemPrect,
    kPathSurfaceItemPtext
};

static int 
SurfaceCreateObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[])
{
    int index;
    int result = TCL_OK;

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "type ?arg arg...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], surfaceItemCmds, "type", 0,
            &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch (index) {
        case kPathSurfaceItemCircle:
        case kPathSurfaceItemEllipse: {
            result = SurfaceCreateEllipse(interp, surfacePtr, index, objc, objv);
            break;
        }
        case kPathSurfaceItemPath: {
            result = SurfaceCreatePath(interp, surfacePtr, objc, objv);
            break;
        }
        case kPathSurfaceItemPimage: {
            result = SurfaceCreatePimage(interp, surfacePtr, objc, objv);
            break;
        }
        case kPathSurfaceItemPline: {
            result = SurfaceCreatePline(interp, surfacePtr, objc, objv);
            break;
        }
        case kPathSurfaceItemPolyline:
        case kPathSurfaceItemPpolygon: {
            result = SurfaceCreatePpoly(interp, surfacePtr, index, objc, objv);
            break;
        }
        case kPathSurfaceItemPrect: {
            result = SurfaceCreatePrect(interp, surfacePtr, objc, objv);
            break;
        }
        case kPathSurfaceItemPtext: {
            result = SurfaceCreatePtext(interp, surfacePtr, objc, objv);
            break;
        }
    }
    return result;
}

static Tk_OptionTable 	gOptionTableCircle;
static Tk_OptionTable 	gOptionTableEllipse;
static Tk_OptionTable 	gOptionTablePath;
static Tk_OptionTable 	gOptionTablePimage;
static Tk_OptionTable 	gOptionTablePline;
static Tk_OptionTable 	gOptionTablePolyline;
static Tk_OptionTable 	gOptionTablePpolygon;
static Tk_OptionTable 	gOptionTablePrect;
static Tk_OptionTable 	gOptionTablePtext;

PATH_STYLE_CUSTOM_OPTION_RECORDS

#define PATH_OPTION_SPEC_R(typeName)			    \
    {TK_OPTION_DOUBLE, "-r", (char *) NULL, (char *) NULL,  \
	"0.0", -1, Tk_Offset(typeName, rx), 0, 0, 0}

#define PATH_OPTION_SPEC_RX(typeName)			    \
    {TK_OPTION_DOUBLE, "-rx", (char *) NULL, (char *) NULL, \
        "0.0", -1, Tk_Offset(typeName, rx), 0, 0, 0}

#define PATH_OPTION_SPEC_RY(typeName)			    \
    {TK_OPTION_DOUBLE, "-ry", (char *) NULL, (char *) NULL, \
        "0.0", -1, Tk_Offset(typeName, ry), 0, 0, 0}

typedef struct SurfGenericItem {
    Tcl_Obj *styleObj;
    Tk_PathStyle style;
} SurfGenericItem;

static int	
GetPointCoords(Tcl_Interp *interp, double *pointPtr, int objc, Tcl_Obj *CONST objv[])
{
    if ((objc == 1) || (objc == 2)) {
        double x, y;
        
        if (objc==1) {
            if (Tcl_ListObjGetElements(interp, objv[0], &objc,
                    (Tcl_Obj ***) &objv) != TCL_OK) {
                return TCL_ERROR;
            } else if (objc != 2) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 2", -1));
                return TCL_ERROR;
            }
        }
        if ((Tcl_GetDoubleFromObj(interp, objv[0], &x) != TCL_OK)
		|| (Tcl_GetDoubleFromObj(interp, objv[1], &y) != TCL_OK)) {
            return TCL_ERROR;
        }
        pointPtr[0] = x;
        pointPtr[1] = y;
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 2", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GetTwoPointsCoords(Tcl_Interp *interp, double *pointsPtr, int objc, Tcl_Obj *CONST objv[])
{
    if ((objc == 1) || (objc == 4)) {
        double x1, y1, x2, y2;
        
        if (objc==1) {
            if (Tcl_ListObjGetElements(interp, objv[0], &objc,
                    (Tcl_Obj ***) &objv) != TCL_OK) {
                return TCL_ERROR;
            } else if (objc != 4) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 4", -1));
                return TCL_ERROR;
            }
        }
        if ((Tcl_GetDoubleFromObj(interp, objv[0], &x1) != TCL_OK)
            || (Tcl_GetDoubleFromObj(interp, objv[1], &y1) != TCL_OK)
            || (Tcl_GetDoubleFromObj(interp, objv[2], &x2) != TCL_OK)
            || (Tcl_GetDoubleFromObj(interp, objv[3], &y2) != TCL_OK)) {
            return TCL_ERROR;
        }
        pointsPtr[0] = x1;
        pointsPtr[1] = y1;
        pointsPtr[2] = x2;
        pointsPtr[3] = y2;
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected 4", -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
MakePolyAtoms(Tcl_Interp *interp, int closed, int objc, Tcl_Obj *CONST objv[], PathAtom **atomPtrPtr)
{
    PathAtom *atomPtr = NULL;

    if (objc == 1) {
        if (Tcl_ListObjGetElements(interp, objv[0], &objc,
            (Tcl_Obj ***) &objv) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (objc & 1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected an even number", -1));
        return TCL_ERROR;
    } else if (objc < 4) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # coordinates: expected at least 4", -1));
        return TCL_ERROR;
    } else {
        int 	i;
        double	x, y;
        double	firstX = 0.0, firstY = 0.0;
        PathAtom *firstAtomPtr = NULL;

        for (i = 0; i < objc; i += 2) {
            if ((Tcl_GetDoubleFromObj(interp, objv[i], &x) != TCL_OK)
                    || (Tcl_GetDoubleFromObj(interp, objv[i+1], &y) != TCL_OK)) {
                TkPathFreeAtoms(atomPtr);
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
    }
    return TCL_OK;
}

static int
GetFirstOptionIndex(int objc, Tcl_Obj* CONST objv[])
{
    int i;
    for (i = 1; i < objc; i++) {
        char *arg = Tcl_GetString(objv[i]);
        if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
            break;
        }
    }
    return i;
}

static int
SurfaceParseOptions(Tcl_Interp *interp, char *recordPtr, 
        Tk_OptionTable table, int objc, Tcl_Obj* CONST objv[])
{
    Tk_Window tkwin = Tk_MainWindow(interp);    
    if (Tk_InitOptions(interp, recordPtr, table, tkwin) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tk_SetOptions(interp, recordPtr, table, 	
            objc, objv, tkwin, NULL, NULL) != TCL_OK) {
        Tk_FreeConfigOptions(recordPtr, table, tkwin);
        return TCL_ERROR;
    }
    return TCL_OK;
}

typedef struct SurfEllipseItem {
    Tcl_Obj *styleObj;
    Tk_PathStyle style;
    double rx, ry;
} SurfEllipseItem;

PATH_OPTION_STRING_TABLES_FILL
PATH_OPTION_STRING_TABLES_STROKE

static Tk_OptionSpec circleOptionSpecs[] = {
    PATH_OPTION_SPEC_STYLENAME(SurfEllipseItem),
    PATH_OPTION_SPEC_STYLE_FILL(SurfEllipseItem, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfEllipseItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfEllipseItem, "black"),
    PATH_OPTION_SPEC_R(SurfEllipseItem),
    PATH_OPTION_SPEC_END
};

static Tk_OptionSpec ellipseOptionSpecs[] = {
    PATH_OPTION_SPEC_STYLENAME(SurfEllipseItem),
    PATH_OPTION_SPEC_STYLE_FILL(SurfEllipseItem, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfEllipseItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfEllipseItem, "black"),
    PATH_OPTION_SPEC_RX(SurfEllipseItem),
    PATH_OPTION_SPEC_RY(SurfEllipseItem),
    PATH_OPTION_SPEC_END
};

static int	
SurfaceCreateEllipse(Tcl_Interp* interp, PathSurface *surfacePtr, int type, int objc, Tcl_Obj* CONST objv[])
{
    TkPathContext 	context = surfacePtr->ctx;
    int			i;
    double		center[2];
    PathAtom 		*atomPtr;
    EllipseAtom 	ellAtom;
    PathRect		bbox;
    SurfEllipseItem	ellipse;
    Tk_PathStyle	*style = &ellipse.style;
    Tk_PathStyle	mergedStyle;
    int			result = TCL_OK;

    ellipse.styleObj = NULL;
    i = GetFirstOptionIndex(objc, objv);
    TkPathInitStyle(style);
    if (GetPointCoords(interp, center, i-3, objv+3) != TCL_OK) {
        goto bail;
    }
    if (SurfaceParseOptions(interp, (char *)&ellipse, 
            (type == kPathSurfaceItemCircle) ? gOptionTableCircle : gOptionTableEllipse, 
            objc-i, objv+i) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    if (style->fillObj != NULL) {
	style->fill = TkPathGetPathColorStatic(interp, Tk_MainWindow(interp), style->fillObj);
	if (style->fill == NULL) {
	    result = TCL_ERROR;
	    goto bail;	
	}
    }
    
    /*
     * NB: We *copy* the style for temp usage.
     *     Only values and pointers are copied so we shall not free this style.
     */
    mergedStyle = ellipse.style;
    if (TkPathStyleMergeStyleStatic(interp, ellipse.styleObj, &mergedStyle, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    ellipse.rx = MAX(0.0, ellipse.rx);
    ellipse.ry = MAX(0.0, ellipse.ry);
    atomPtr = (PathAtom *)&ellAtom;
    atomPtr->nextPtr = NULL;
    atomPtr->type = PATH_ATOM_ELLIPSE;
    ellAtom.cx = center[0];
    ellAtom.cy = center[1];
    ellAtom.rx = ellipse.rx;
    ellAtom.ry = (type == kPathSurfaceItemCircle) ? ellipse.rx : ellipse.ry;
    TkPathSaveState(context);
    TkPathPushTMatrix(context, mergedStyle.matrixPtr);
    if (TkPathMakePath(context, atomPtr, &mergedStyle) != TCL_OK) {
        TkPathRestoreState(context);
        result = TCL_ERROR;
        goto bail;
    }
    bbox = TkPathGetTotalBbox(atomPtr, &mergedStyle);
    TkPathPaintPath(context, atomPtr, &mergedStyle, &bbox);
    TkPathRestoreState(context);

bail:
    TkPathDeleteStyle(&ellipse.style);
    Tk_FreeConfigOptions((char *)&ellipse, 
	    (type == kPathSurfaceItemCircle) ? gOptionTableCircle : gOptionTableEllipse,
	    Tk_MainWindow(interp));
    return result;
}

static Tk_OptionSpec pathOptionSpecs[] = {
    PATH_OPTION_SPEC_STYLENAME(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_FILL(SurfGenericItem, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfGenericItem, "black"),
    PATH_OPTION_SPEC_END
};

static int
SurfaceCreatePath(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]) 
{
    TkPathContext 	context = surfacePtr->ctx;
    PathAtom 		*atomPtr = NULL;
    PathRect		bbox;
    SurfGenericItem	item;
    Tk_PathStyle	*style = &item.style;
    Tk_PathStyle	mergedStyle;
    int			len;
    int			result = TCL_OK;
    
    item.styleObj = NULL;
    TkPathInitStyle(&item.style);
    if (TkPathParseToAtoms(interp, objv[3], &atomPtr, &len) != TCL_OK) {
        return TCL_ERROR;
    }
    if (SurfaceParseOptions(interp, (char *)&item, gOptionTablePath, objc-4, objv+4) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    if (style->fillObj != NULL) {
	style->fill = TkPathGetPathColorStatic(interp, Tk_MainWindow(interp), style->fillObj);
	if (style->fill == NULL) {
	    result = TCL_ERROR;
	    goto bail;	
	}
    }
    mergedStyle = item.style;
    if (TkPathStyleMergeStyleStatic(interp, item.styleObj, &mergedStyle, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    TkPathSaveState(context);
    TkPathPushTMatrix(context, mergedStyle.matrixPtr);
    if (TkPathMakePath(context, atomPtr, &mergedStyle) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    bbox = TkPathGetTotalBbox(atomPtr, &mergedStyle);
    TkPathPaintPath(context, atomPtr, &mergedStyle, &bbox);
    
bail:
    TkPathDeleteStyle(style);
    TkPathFreeAtoms(atomPtr);
    TkPathRestoreState(context);
    Tk_FreeConfigOptions((char *)&item, gOptionTablePath, Tk_MainWindow(interp));
    return result;
}

typedef struct SurfPimageItem {
    char *imageName;
    double height;
    double width;
    TMatrix *matrixPtr;
    Tcl_Obj *styleObj;	    /* We only use matrixPtr from style. */
} SurfPimageItem;

static Tk_OptionSpec pimageOptionSpecs[] = {
    {TK_OPTION_DOUBLE, "-height", (char *) NULL, (char *) NULL,
        "0", -1, Tk_Offset(SurfPimageItem, height), 0, 0, 0},
    {TK_OPTION_CUSTOM, "-matrix", (char *) NULL, (char *) NULL,
	(char *) NULL, -1, Tk_Offset(SurfPimageItem, matrixPtr),
	TK_OPTION_NULL_OK, (ClientData) &matrixCO, 0},
    {TK_OPTION_STRING, "-image", (char *) NULL, (char *) NULL,
        "", -1, Tk_Offset(SurfPimageItem, imageName), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-style", (char *) NULL, (char *) NULL,
        "", Tk_Offset(SurfPimageItem, styleObj), -1, TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-width", (char *) NULL, (char *) NULL,
        "0", -1, Tk_Offset(SurfPimageItem, width), 0, 0, 0},
    PATH_OPTION_SPEC_END
};

static int
SurfaceCreatePimage(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]) 
{
    TkPathContext 	context = surfacePtr->ctx;
    SurfPimageItem	item;
    Tk_Image		image;
    Tk_PhotoHandle	photo;
    Tk_PathStyle	style;
    double		point[2];
    int			i;
    int			result = TCL_OK;

    item.imageName = NULL;
    item.matrixPtr = NULL;
    TkPathInitStyle(&style);
    i = GetFirstOptionIndex(objc, objv);
    if (GetPointCoords(interp, point, i-3, objv+3) != TCL_OK) {
        return TCL_ERROR;
    }
    if (SurfaceParseOptions(interp, (char *)&item, gOptionTablePimage, objc-i, objv+i) != TCL_OK) {
        return TCL_ERROR;
    }    
    style.matrixPtr = item.matrixPtr;
    if (TkPathStyleMergeStyleStatic(interp, item.styleObj, &style, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    if (item.imageName != NULL) {
        photo = Tk_FindPhoto(interp, item.imageName);
        if (photo == NULL) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("no photo with the given name", -1));
	    result = TCL_ERROR;
	    goto bail;
         }
        image = Tk_GetImage(interp, Tk_MainWindow(interp), item.imageName, NULL, (ClientData) NULL);
        TkPathSaveState(context);
        TkPathPushTMatrix(context, style.matrixPtr);
        TkPathImage(context, image, photo, point[0], point[1], item.width, item.height);
        Tk_FreeImage(image);
        TkPathRestoreState(context);
    }

bail:
    Tk_FreeConfigOptions((char *)&item, gOptionTablePimage, Tk_MainWindow(interp));
    return result;
}

static Tk_OptionSpec plineOptionSpecs[] = {
    PATH_OPTION_SPEC_STYLENAME(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfGenericItem, "black"),
    PATH_OPTION_SPEC_END
};

static int
SurfaceCreatePline(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]) 
{
    TkPathContext 	context = surfacePtr->ctx;
    int			i;
    PathRect		bbox;
    SurfGenericItem	item;
    PathAtom 		*atomPtr = NULL;
    Tk_PathStyle	mergedStyle;
    double		points[4];
    int			result = TCL_OK;
    
    item.styleObj = NULL;
    i = GetFirstOptionIndex(objc, objv);
    TkPathInitStyle(&item.style);
    if (GetTwoPointsCoords(interp, points, i-3, objv+3) != TCL_OK) {
        return TCL_ERROR;
    }
    if (SurfaceParseOptions(interp, (char *)&item, gOptionTablePline, objc-i, objv+i) != TCL_OK) {
        return TCL_ERROR;
    }
    mergedStyle = item.style;
    if (TkPathStyleMergeStyleStatic(interp, item.styleObj, &mergedStyle, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    atomPtr = NewMoveToAtom(points[0], points[1]);
    atomPtr->nextPtr = NewLineToAtom(points[2], points[3]);
    TkPathSaveState(context);
    TkPathPushTMatrix(context, mergedStyle.matrixPtr);
    if (TkPathMakePath(context, atomPtr, &mergedStyle) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    bbox = TkPathGetTotalBbox(atomPtr, &mergedStyle);
    TkPathPaintPath(context, atomPtr, &mergedStyle, &bbox);
    
bail:
    TkPathDeleteStyle(&item.style);
    TkPathFreeAtoms(atomPtr);
    TkPathRestoreState(context);
    Tk_FreeConfigOptions((char *)&item, gOptionTablePline, Tk_MainWindow(interp));
    return result;
}

static Tk_OptionSpec polylineOptionSpecs[] = {
    PATH_OPTION_SPEC_STYLENAME(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfGenericItem, "black"),
    PATH_OPTION_SPEC_END
};

static Tk_OptionSpec ppolygonOptionSpecs[] = {
    PATH_OPTION_SPEC_STYLENAME(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_FILL(SurfGenericItem, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfGenericItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfGenericItem, "black"),
    PATH_OPTION_SPEC_END
};

static int	
SurfaceCreatePpoly(Tcl_Interp* interp, PathSurface *surfacePtr, int type, int objc, Tcl_Obj* CONST objv[])
{
    TkPathContext 	context = surfacePtr->ctx;
    int			i;
    PathRect		bbox;
    SurfGenericItem	item;
    Tk_PathStyle	*style = &item.style;
    Tk_PathStyle	mergedStyle;
    PathAtom 		*atomPtr = NULL;
    int			result = TCL_OK;

    item.styleObj = NULL;
    i = GetFirstOptionIndex(objc, objv);
    TkPathInitStyle(style);
    if (MakePolyAtoms(interp, (type == kPathSurfaceItemPolyline) ? 0 : 1, 
            i-3, objv+3, &atomPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (SurfaceParseOptions(interp, (char *)&item, 
            (type == kPathSurfaceItemPolyline) ? gOptionTablePolyline : gOptionTablePpolygon, 
            objc-i, objv+i) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    if (style->fillObj != NULL) {
	style->fill = TkPathGetPathColorStatic(interp, Tk_MainWindow(interp), style->fillObj);
	if (style->fill == NULL) {
	    result = TCL_ERROR;
	    goto bail;	
	}
    }
    mergedStyle = item.style;
    if (TkPathStyleMergeStyleStatic(interp, item.styleObj, &mergedStyle, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    TkPathSaveState(context);
    TkPathPushTMatrix(context, mergedStyle.matrixPtr);
    if (TkPathMakePath(context, atomPtr, &mergedStyle) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    bbox = TkPathGetTotalBbox(atomPtr, &mergedStyle);
    TkPathPaintPath(context, atomPtr, &mergedStyle, &bbox);
    
bail:
    TkPathDeleteStyle(style);
    TkPathFreeAtoms(atomPtr);
    TkPathRestoreState(context);
    Tk_FreeConfigOptions((char *)&item, 
	    (type == kPathSurfaceItemPolyline) ? gOptionTablePolyline : gOptionTablePpolygon, 
	    Tk_MainWindow(interp));
    return result;
}

typedef struct SurfPrectItem {
    Tcl_Obj *styleObj;
    Tk_PathStyle style;
    double rx, ry;
} SurfPrectItem;

static Tk_OptionSpec prectOptionSpecs[] = {
    PATH_OPTION_SPEC_STYLENAME(SurfPrectItem),
    PATH_OPTION_SPEC_STYLE_FILL(SurfPrectItem, ""),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfPrectItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfPrectItem, "black"),
    PATH_OPTION_SPEC_RX(SurfPrectItem),
    PATH_OPTION_SPEC_RY(SurfPrectItem),
    PATH_OPTION_SPEC_END
};

static int	
SurfaceCreatePrect(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[])
{
    TkPathContext 	context = surfacePtr->ctx;
    int			i;
    SurfPrectItem	prect;
    Tk_PathStyle	*style = &prect.style;
    Tk_PathStyle	mergedStyle;
    PathRect		bbox;
    PathAtom 		*atomPtr = NULL;
    double		points[4];
    int			result = TCL_OK;

    prect.styleObj = NULL;
    i = GetFirstOptionIndex(objc, objv);
    TkPathInitStyle(style);
    if (GetTwoPointsCoords(interp, points, i-3, objv+3) != TCL_OK) {
        return TCL_ERROR;
    }
    if (SurfaceParseOptions(interp, (char *)&prect, gOptionTablePrect, objc-i, objv+i) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    if (style->fillObj != NULL) {
	style->fill = TkPathGetPathColorStatic(interp, Tk_MainWindow(interp), style->fillObj);
	if (style->fill == NULL) {
	    result = TCL_ERROR;
	    goto bail;	
	}
    }
    mergedStyle = prect.style;
    if (TkPathStyleMergeStyleStatic(interp, prect.styleObj, &mergedStyle, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    prect.rx = MAX(0.0, prect.rx);
    prect.ry = MAX(0.0, prect.ry);
    TkPathSaveState(context);
    TkPathPushTMatrix(context, mergedStyle.matrixPtr);
    TkPathMakePrectAtoms(points, prect.rx, prect.ry, &atomPtr);
    if (TkPathMakePath(context, atomPtr, &mergedStyle) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    bbox = TkPathGetTotalBbox(atomPtr, &mergedStyle);
    TkPathPaintPath(context, atomPtr, &mergedStyle, &bbox);
    
bail:
    TkPathDeleteStyle(&prect.style);
    TkPathFreeAtoms(atomPtr);
    TkPathRestoreState(context);
    Tk_FreeConfigOptions((char *)&prect, gOptionTablePrect, Tk_MainWindow(interp));
    return result;
}

typedef struct SurfPtextItem {
    Tcl_Obj *styleObj;
    Tk_PathStyle style;
    Tk_PathTextStyle textStyle;
    int textAnchor;
    double x;
    double y;
    char *utf8;				/* The actual text to display; UTF-8 */
} SurfPtextItem;

static char *textAnchorST[] = {
    "start", "middle", "end", (char *) NULL
};

static Tk_OptionSpec ptextOptionSpecs[] = {
    {TK_OPTION_STRING, "-fontfamily", (char *) NULL, (char *) NULL,
        "Helvetica", -1, Tk_Offset(SurfPtextItem, textStyle.fontFamily), 
        TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-fontsize", (char *) NULL, (char *) NULL,
        "12.0", -1, Tk_Offset(SurfPtextItem,  textStyle.fontSize), 0, 0, 0},
    {TK_OPTION_STRING, "-text", (char *) NULL, (char *) NULL,
        "", -1, Tk_Offset(SurfPtextItem, utf8), 
        TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING_TABLE, "-textanchor", (char *) NULL, (char *) NULL,
        "start", -1, Tk_Offset(SurfPtextItem, textAnchor),
        0, (ClientData) textAnchorST, 0},
    PATH_OPTION_SPEC_STYLENAME(SurfPtextItem),
    PATH_OPTION_SPEC_STYLE_FILL(SurfPtextItem, "black"),
    PATH_OPTION_SPEC_STYLE_MATRIX(SurfPtextItem),
    PATH_OPTION_SPEC_STYLE_STROKE(SurfPtextItem, ""),
    PATH_OPTION_SPEC_END
};

static int
SurfaceCreatePtext(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[]) 
{
    TkPathContext   context = surfacePtr->ctx;
    int		    i;
    double	    point[2];
    SurfPtextItem   item;
    Tk_PathStyle    *style = &item.style;
    Tk_PathStyle    mergedStyle;
    PathRect	    r;
    void	    *custom = NULL;
    int		    result = TCL_OK;
        
    item.styleObj = NULL;
    item.textAnchor = kPathTextAnchorStart;
    item.utf8 = NULL;
    item.textStyle.fontFamily = NULL;
    i = GetFirstOptionIndex(objc, objv);
    TkPathInitStyle(&item.style);
    if (GetPointCoords(interp, point, i-3, objv+3) != TCL_OK) {
        return TCL_ERROR;
    }
    if (SurfaceParseOptions(interp, (char *)&item, gOptionTablePtext, objc-i, objv+i) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    if (style->fillObj != NULL) {
	style->fill = TkPathGetPathColorStatic(interp, Tk_MainWindow(interp), style->fillObj);
	if (style->fill == NULL) {
	    result = TCL_ERROR;
	    goto bail;	
	}
    }
    if (TkPathTextConfig(interp, &item.textStyle, item.utf8, &custom) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    mergedStyle = item.style;
    if (TkPathStyleMergeStyleStatic(interp, item.styleObj, &mergedStyle, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto bail;
    }
    r = TkPathTextMeasureBbox(&item.textStyle, item.utf8, custom);
    switch (item.textAnchor) {
        case kPathTextAnchorMiddle:
            point[0] -= (r.x2 - r.x1)/2;
            break;
        case kPathTextAnchorEnd:
            point[0] -= (r.x2 - r.x1);
            break;
    }
    TkPathSaveState(context);
    TkPathPushTMatrix(context, mergedStyle.matrixPtr);
    TkPathBeginPath(context, &mergedStyle);
    TkPathTextDraw(context, &mergedStyle, &item.textStyle, point[0], point[1], item.utf8, custom);
    TkPathEndPath(context);
    TkPathTextFree(&item.textStyle, custom);
    
bail:
    TkPathDeleteStyle(style);
    TkPathRestoreState(context);
    Tk_FreeConfigOptions((char *)&item, gOptionTablePtext, Tk_MainWindow(interp));
    return result;
}

static void
SurfaceInitOptions(Tcl_Interp* interp)
{
    gOptionTableCircle = Tk_CreateOptionTable(interp, circleOptionSpecs);
    gOptionTableEllipse = Tk_CreateOptionTable(interp, ellipseOptionSpecs);
    gOptionTablePath = Tk_CreateOptionTable(interp, pathOptionSpecs);
    gOptionTablePimage = Tk_CreateOptionTable(interp, pimageOptionSpecs);
    gOptionTablePline = Tk_CreateOptionTable(interp, plineOptionSpecs);
    gOptionTablePolyline = Tk_CreateOptionTable(interp, polylineOptionSpecs);
    gOptionTablePpolygon = Tk_CreateOptionTable(interp, ppolygonOptionSpecs);
    gOptionTablePrect = Tk_CreateOptionTable(interp, prectOptionSpecs);
    gOptionTablePtext = Tk_CreateOptionTable(interp, ptextOptionSpecs);
}

static int 
SurfaceEraseObjCmd(Tcl_Interp* interp, PathSurface *surfacePtr, int objc, Tcl_Obj* CONST objv[])
{
    double x, y, width, height;
    
    if (objc != 6) {
        Tcl_WrongNumArgs(interp, 2, objv, "x y width height");
        return TCL_ERROR;
    }
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK) ||
            (Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK) ||
            (Tcl_GetDoubleFromObj(interp, objv[4], &width) != TCL_OK) ||
            (Tcl_GetDoubleFromObj(interp, objv[5], &height) != TCL_OK)) {
        return TCL_ERROR;
    }
    TkPathSurfaceErase(surfacePtr->ctx, x, y, width, height);
    return TCL_OK;
}
