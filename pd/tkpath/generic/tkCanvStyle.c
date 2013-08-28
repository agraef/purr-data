/*
 * tkCanvStyle.c --
 *
 *	This file implements some glue between style objects and the canvas widget.
 *
 * Copyright (c) 2008  Mats Bengtsson
 *
 * $Id: tkCanvStyle.c,v 1.2 2008/06/05 12:51:58 matben Exp $
 */

#include "tkIntPath.h"
#include "tkpCanvas.h"

static char *	kStyleNameBase = "style";

static CONST char *styleCmds[] = {
    "cget", "configure", "create", "delete", "inuse", "names",
    (char *) NULL
};

enum {
    kPathStyleCmdCget	= 0L,
    kPathStyleCmdConfigure,
    kPathStyleCmdCreate,
    kPathStyleCmdDelete,
    kPathStyleCmdInUse,
    kPathStyleCmdNames
};

/*
 *----------------------------------------------------------------------
 *
 * CanvasStyleObjCmd --
 *
 *	Implements the 'pathName style' command using the canvas local state.  
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
CanvasStyleObjCmd(Tcl_Interp* interp, TkPathCanvas *canvasPtr, 
	int objc, Tcl_Obj* CONST objv[])
{
    int index;
    int result = TCL_OK;
    
    /*
     * objv[2] is the subcommand: cget | configure | create | delete | names
     */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "command ?arg arg...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], styleCmds, "command", 0,
	    &index) != TCL_OK) {
        return TCL_ERROR;
    }
    switch (index) {
	    
        case kPathStyleCmdCget: {            
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "name option");
		return TCL_ERROR;
	    }
	    result = PathStyleCget(interp, canvasPtr->tkwin, objc-3, objv+3,
		    &canvasPtr->styleTable);
            break;
        }
	    
        case kPathStyleCmdConfigure: {
	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name ?option? ?value option value...?");
		return TCL_ERROR;
	    }
	    result = PathStyleConfigure(interp, canvasPtr->tkwin, objc-3, objv+3, 
		    &canvasPtr->styleTable, &canvasPtr->gradientTable);
            break;
        }
	    
        case kPathStyleCmdCreate: {
	    char str[255];

	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?option value...?");
		return TCL_ERROR;
	    }
            sprintf(str, "%s%d", kStyleNameBase, canvasPtr->styleUid++);
	    result = PathStyleCreate(interp, canvasPtr->tkwin, objc-3, objv+3, 
		    &canvasPtr->styleTable, &canvasPtr->gradientTable, str);
            break;
        }
	    
        case kPathStyleCmdDelete: {
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name");
		return TCL_ERROR;
	    }
	    result = PathStyleDelete(interp, objv[3], &canvasPtr->styleTable,
		    canvasPtr->tkwin);
	    break;
        }

	case kPathStyleCmdInUse: {
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name");
		return TCL_ERROR;
	    }
	    result = PathStyleInUse(interp, objv[3], &canvasPtr->styleTable);
	    break;
	}
	    
        case kPathStyleCmdNames: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 3, objv, NULL);
		return TCL_ERROR;
	    }
	    PathStyleNames(interp, &canvasPtr->styleTable);
            break;
        }
    }
    return result;
}
