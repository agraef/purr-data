/*
 * tkCanvGradients.c --
 *
 *	This file implements some glue between gradient objects and the canvas widget.
 *
 * Copyright (c) 2008  Mats Bengtsson
 *
 * TODO: o Add tkwin option here and there so we can free stop colors!
 *
 * $Id$
 */

#include "tkIntPath.h"
#include "tkpCanvas.h"

static char *	kGradientNameBase = "gradient";

static CONST char *gradientCmds[] = {
    "cget", "configure", "create", "delete", "inuse", "names", "type",
    (char *) NULL
};

enum {
    kPathGradientCmdCget	= 0L,
    kPathGradientCmdConfigure,
    kPathGradientCmdCreate,
    kPathGradientCmdDelete,
    kPathGradientCmdInUse,
    kPathGradientCmdNames,
    kPathGradientCmdType
};

/*
 *----------------------------------------------------------------------
 *
 * CanvasGradientObjCmd --
 *
 *	Implements the 'pathName gradient' command using the canvas local state.  
 *
 * Results:
 *	Standard Tcl result
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

int 				
CanvasGradientObjCmd(Tcl_Interp* interp, TkPathCanvas *canvasPtr, 
	int objc, Tcl_Obj* CONST objv[])
{
    int index;
    int result = TCL_OK;
    
    /*
     * objv[2] is the subcommand: cget | configure | create | delete | names | type
     */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "command ?arg arg...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], gradientCmds, "command", 0,
	    &index) != TCL_OK) {
        return TCL_ERROR;
    }
    switch (index) {
	    
        case kPathGradientCmdCget: {            
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "name option");
		return TCL_ERROR;
	    }
	    result = PathGradientCget(interp, canvasPtr->tkwin, objc-3, objv+3,
		    &canvasPtr->gradientTable);
            break;
        }
	    
        case kPathGradientCmdConfigure: {
	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name ?option? ?value option value...?");
		return TCL_ERROR;
	    }
	    result = PathGradientConfigure(interp, canvasPtr->tkwin, objc-3, objv+3, 
		    &canvasPtr->gradientTable);
            break;
        }
	    
        case kPathGradientCmdCreate: {
	    char str[255];

	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "type ?option value...?");
		return TCL_ERROR;
	    }
            sprintf(str, "%s%d", kGradientNameBase, canvasPtr->gradientUid++);
	    result = PathGradientCreate(interp, canvasPtr->tkwin, objc-3, objv+3, 
		    &canvasPtr->gradientTable, str);
            break;
        }
	    
        case kPathGradientCmdDelete: {
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name");
		return TCL_ERROR;
	    }
	    result = PathGradientDelete(interp, objv[3], &canvasPtr->gradientTable);
	    break;
        }

	case kPathGradientCmdInUse: {
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name");
		return TCL_ERROR;
	    }
	    result = PathGradientInUse(interp, objv[3], &canvasPtr->gradientTable);
	    break;
	}
	    
        case kPathGradientCmdNames: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 3, objv, NULL);
		return TCL_ERROR;
	    }
	    PathGradientNames(interp, &canvasPtr->gradientTable);
            break;
        }
	    
        case kPathGradientCmdType: {
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name");
		return TCL_ERROR;
	    }
	    result = PathGradientType(interp, objv[3], &canvasPtr->gradientTable);
            break;
        }
    }
    return result;
}

/* 
 * CanvasGradientsFree --
 *	
 *	Used by canvas Destroy handler to clean up all gradients.
 *	Note that items clean up all their gradient instances themeselves.
 */
void
CanvasGradientsFree(TkPathCanvas *canvasPtr)
{
    Tcl_HashEntry   *hPtr;
    Tcl_HashSearch  search;
    TkPathGradientMaster *gradientPtr = NULL;

    hPtr = Tcl_FirstHashEntry(&canvasPtr->gradientTable, &search);
    while (hPtr != NULL) {
	gradientPtr = (TkPathGradientMaster*) Tcl_GetHashValue(hPtr);
	Tcl_DeleteHashEntry(hPtr);
	PathGradientMasterFree(gradientPtr); 
	hPtr = Tcl_NextHashEntry(&search);
    }
}

