/*
 * tkpUtil.c --
 *
 *	This file contains miscellaneous utility functions that are used by
 *	the rest of Tk, such as a function for drawing a focus highlight.
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#include "tkInt.h"
#include "tkIntPath.h"

/*
 * The structure below defines the implementation of the "statekey" Tcl
 * object, used for quickly finding a mapping in a TkStateMap.
 */

/* === EB - 30-apr-2010: commented out the CONST that made build fail with tcl/tk 8.5 */
/* === George Petasis - 7 July 2012: The missing CONST fails with Tk 8.6... */
#if (TK_MAJOR_VERSION >= 8) &&  (TK_MINOR_VERSION >= 6)
CONST
#endif
Tcl_ObjType tkStateKeyObjType = {
    "statekey",			/* name */
    NULL,			/* freeIntRepProc */
    NULL,			/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};
/* === */

static int
GetOffset(Tcl_Interp *interp, ClientData clientData,
    Tcl_Obj *offsetObj, Tk_Window tkwin, Tk_TSOffset *offsetPtr)
{
    char *value = Tcl_GetString(offsetObj);
    Tk_TSOffset tsoffset;
    const char *q, *p;
    int result;

    if ((value == NULL) || (*value == 0)) {
	tsoffset.flags = TK_OFFSET_CENTER|TK_OFFSET_MIDDLE;
	goto goodTSOffset;
    }
    tsoffset.flags = 0;
    p = value;

    switch(value[0]) {
    case '#':
	if (PTR2INT(clientData) & TK_OFFSET_RELATIVE) {
	    tsoffset.flags = TK_OFFSET_RELATIVE;
	    p++;
	    break;
	}
	goto badTSOffset;
    case 'e':
	switch(value[1]) {
	case '\0':
	    tsoffset.flags = TK_OFFSET_RIGHT|TK_OFFSET_MIDDLE;
	    goto goodTSOffset;
	case 'n':
	    if (value[2]!='d' || value[3]!='\0') {
		goto badTSOffset;
	    }
	    tsoffset.flags = INT_MAX;
	    goto goodTSOffset;
	}
    case 'w':
	if (value[1] != '\0') {goto badTSOffset;}
	tsoffset.flags = TK_OFFSET_LEFT|TK_OFFSET_MIDDLE;
	goto goodTSOffset;
    case 'n':
	if ((value[1] != '\0') && (value[2] != '\0')) {
	    goto badTSOffset;
	}
	switch(value[1]) {
	case '\0':
	    tsoffset.flags = TK_OFFSET_CENTER|TK_OFFSET_TOP;
	    goto goodTSOffset;
	case 'w':
	    tsoffset.flags = TK_OFFSET_LEFT|TK_OFFSET_TOP;
	    goto goodTSOffset;
	case 'e':
	    tsoffset.flags = TK_OFFSET_RIGHT|TK_OFFSET_TOP;
	    goto goodTSOffset;
	}
	goto badTSOffset;
    case 's':
	if ((value[1] != '\0') && (value[2] != '\0')) {
	    goto badTSOffset;
	}
	switch(value[1]) {
	case '\0':
	    tsoffset.flags = TK_OFFSET_CENTER|TK_OFFSET_BOTTOM;
	    goto goodTSOffset;
	case 'w':
	    tsoffset.flags = TK_OFFSET_LEFT|TK_OFFSET_BOTTOM;
	    goto goodTSOffset;
	case 'e':
	    tsoffset.flags = TK_OFFSET_RIGHT|TK_OFFSET_BOTTOM;
	    goto goodTSOffset;
	}
	goto badTSOffset;
    case 'c':
	if (strncmp(value, "center", strlen(value)) != 0) {
	    goto badTSOffset;
	}
	tsoffset.flags = TK_OFFSET_CENTER|TK_OFFSET_MIDDLE;
	goto goodTSOffset;
    }
    if ((q = strchr(p,',')) == NULL) {
	if (PTR2INT(clientData) & TK_OFFSET_INDEX) {
	    if (Tcl_GetInt(interp, (char *) p, &tsoffset.flags) != TCL_OK) {
		Tcl_ResetResult(interp);
		goto badTSOffset;
	    }
	    tsoffset.flags |= TK_OFFSET_INDEX;
	    goto goodTSOffset;
	}
	goto badTSOffset;
    }
    *((char *) q) = 0;
    result = Tk_GetPixels(interp, tkwin, (char *) p, &tsoffset.xoffset);
    *((char *) q) = ',';
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tk_GetPixels(interp, tkwin, (char*)q+1, &tsoffset.yoffset) != TCL_OK) {
	return TCL_ERROR;
    }

goodTSOffset:
    /*
     * Below is a hack to allow the stipple/tile offset to be stored in the
     * internal tile structure. Most of the times, offsetPtr is a pointer to
     * an already existing tile structure. However if this structure is not
     * already created, we must do it with Tk_GetTile()!!!!;
     */

    memcpy(offsetPtr, &tsoffset, sizeof(Tk_TSOffset));
    return TCL_OK;

badTSOffset:
    Tcl_AppendResult(interp, "bad offset \"", value,
	    "\": expected \"x,y\"", NULL);
    if (PTR2INT(clientData) & TK_OFFSET_RELATIVE) {
	Tcl_AppendResult(interp, ", \"#x,y\"", NULL);
    }
    if (PTR2INT(clientData) & TK_OFFSET_INDEX) {
	Tcl_AppendResult(interp, ", <index>", NULL);
    }
    Tcl_AppendResult(interp, ", n, ne, e, se, s, sw, w, nw, or center", NULL);
    return TCL_ERROR;
}

/* Return NULL on error and leave error message */

static Tk_TSOffset *
PathOffsetNew(Tcl_Interp *interp, ClientData clientData, Tk_Window tkwin, Tcl_Obj *offsetObj)
{
    Tk_TSOffset *offsetPtr;
    
    offsetPtr = (Tk_TSOffset *) ckalloc(sizeof(Tk_TSOffset));
    if (GetOffset(interp, clientData, offsetObj, tkwin, offsetPtr) != TCL_OK) {
	ckfree((char *) offsetPtr);
	return NULL;;
    }
    return offsetPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPathOffsetOptionSetProc --
 *
 *	Converts the offset of a stipple or tile into the Tk_TSOffset
 *	structure.
 *
 *----------------------------------------------------------------------
 */

int TkPathOffsetOptionSetProc(
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
    Tk_TSOffset *newPtr = NULL;
    
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
	    newPtr = PathOffsetNew(interp, clientData, tkwin, valuePtr);
	    if (newPtr == NULL) {
		return TCL_ERROR;
	    }
        }
	*((Tk_TSOffset **) oldInternalPtr) = *((Tk_TSOffset **) internalPtr);
	*((Tk_TSOffset **) internalPtr) = newPtr;
    }
    return TCL_OK;
}

Tcl_Obj *
TkPathOffsetOptionGetProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *recordPtr,		/* Pointer to widget record. */
    int internalOffset)		/* Offset within *recordPtr containing the
				 * value. */
{
    Tk_TSOffset *offsetPtr;
    char buffer[32], *p;

    offsetPtr = *((Tk_TSOffset **) (recordPtr + internalOffset));
    buffer[0] = '\0';
    if (offsetPtr->flags & TK_OFFSET_INDEX) {
	if (offsetPtr->flags >= INT_MAX) {
	    strcat(buffer, "end");
	} else {
	    sprintf(buffer, "%d", offsetPtr->flags & ~TK_OFFSET_INDEX);
	}
	goto end;
    }
    if (offsetPtr->flags & TK_OFFSET_TOP) {
	if (offsetPtr->flags & TK_OFFSET_LEFT) {
	    strcat(buffer, "nw");
	    goto end;
	} else if (offsetPtr->flags & TK_OFFSET_CENTER) {
	    strcat(buffer, "n");
	    goto end;
	} else if (offsetPtr->flags & TK_OFFSET_RIGHT) {
	    strcat(buffer, "ne");
	    goto end;
	}
    } else if (offsetPtr->flags & TK_OFFSET_MIDDLE) {
	if (offsetPtr->flags & TK_OFFSET_LEFT) {
	    strcat(buffer, "w");
	    goto end;
	} else if (offsetPtr->flags & TK_OFFSET_CENTER) {
	    strcat(buffer, "center");
	    goto end;
	} else if (offsetPtr->flags & TK_OFFSET_RIGHT) {
	    strcat(buffer, "e");
	    goto end;
	}
    } else if (offsetPtr->flags & TK_OFFSET_BOTTOM) {
	if (offsetPtr->flags & TK_OFFSET_LEFT) {
	    strcat(buffer, "sw");
	    goto end;
	} else if (offsetPtr->flags & TK_OFFSET_CENTER) {
	    strcat(buffer, "s");
	    goto end;
	} else if (offsetPtr->flags & TK_OFFSET_RIGHT) {
	    strcat(buffer, "se");
	    goto end;
	}
    }
    p = buffer;
    if (offsetPtr->flags & TK_OFFSET_RELATIVE) {
	strcat(buffer , "#");
	p++;
    }
    sprintf(p, "%d,%d", offsetPtr->xoffset, offsetPtr->yoffset);
    
end:
    return Tcl_NewStringObj(buffer, -1);
}

void
TkPathOffsetOptionRestoreProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr,		/* Pointer to storage for value. */
    char *oldInternalPtr)	/* Pointer to old value. */
{
    *(Tk_TSOffset **)internalPtr = *(Tk_TSOffset **)oldInternalPtr;
}

void
TkPathOffsetOptionFreeProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr)		/* Pointer to storage for value. */
{
    if (*((char **) internalPtr) != NULL) {
	ckfree((char *) *((char **) internalPtr));
    }
}

/*
 *--------------------------------------------------------------
 *
 * GetDoublePixels --
 *
 *	Given a string, returns the number of pixels corresponding
 *	to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result.  If
 *	TCL_OK is returned, then everything went well and the
 *	pixel distance is stored at *doublePtr;  otherwise
 *	TCL_ERROR is returned and an error message is left in
 *	interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetDoublePixels(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    Tk_Window tkwin,		/* Window whose screen determines conversion
				 * from centimeters and other absolute
				 * units. */
    CONST char *string,		/* String describing a number of pixels. */
    double *doublePtr)		/* Place to store converted result. */
{
    char *end;
    double d;

    d = strtod((char *) string, &end);
    if (end == string) {
	error:
	Tcl_AppendResult(interp, "bad screen distance \"", string,
		"\"", (char *) NULL);
	return TCL_ERROR;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
	end++;
    }
    switch (*end) {
	case 0:
	    break;
	case 'c':
	    d *= 10*WidthOfScreen(Tk_Screen(tkwin));
	    d /= WidthMMOfScreen(Tk_Screen(tkwin));
	    end++;
	    break;
	case 'i':
	    d *= 25.4*WidthOfScreen(Tk_Screen(tkwin));
	    d /= WidthMMOfScreen(Tk_Screen(tkwin));
	    end++;
	    break;
	case 'm':
	    d *= WidthOfScreen(Tk_Screen(tkwin));
	    d /= WidthMMOfScreen(Tk_Screen(tkwin));
	    end++;
	    break;
	case 'p':
	    d *= (25.4/72.0)*WidthOfScreen(Tk_Screen(tkwin));
	    d /= WidthMMOfScreen(Tk_Screen(tkwin));
	    end++;
	    break;
	default:
	    goto error;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
	end++;
    }
    if (*end != 0) {
	goto error;
    }
    *doublePtr = d;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_PathPixelOptionSetProc --
 *
 *	As TK_OPTION_PIXELS but for double value instead of int.
 *
 * Results:
 *	The return value is a standard Tcl return result.  If
 *	TCL_OK is returned, then everything went well and the
 *	pixel distance is stored at *doublePtr;  otherwise
 *	TCL_ERROR is returned and an error message is left in
 *	interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int Tk_PathPixelOptionSetProc(
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
    double newPixels;
    int result;
    
    valuePtr = *value;
    if (internalOffset >= 0) {
        internalPtr = recordPtr + internalOffset;
    } else {
        internalPtr = NULL;
    }
    if ((flags & TK_OPTION_NULL_OK) && ObjectIsEmpty(valuePtr)) {
	valuePtr = NULL;
	newPixels = 0.0;
    }
    if (internalPtr != NULL) {
	if (valuePtr != NULL) {
	    result = GetDoublePixels(interp, tkwin, Tcl_GetString(valuePtr), &newPixels);
	    if (result != TCL_OK) {
		return TCL_ERROR;
	    } else if (newPixels < 0.0) {
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
			"bad screen distance \"", value, "\"", NULL);
		return TCL_ERROR;
	    }
        }
	*((double *) oldInternalPtr) = *((double *) internalPtr);
	*((double *) internalPtr) = newPixels;
    }
    return TCL_OK;
}

Tcl_Obj *
Tk_PathPixelOptionGetProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *recordPtr,		/* Pointer to widget record. */
    int internalOffset)		/* Offset within *recordPtr containing the
				 * value. */
{
    return Tcl_NewDoubleObj(*((double *) (recordPtr + internalOffset)));
}

void
Tk_PathPixelOptionRestoreProc(
    ClientData clientData,
    Tk_Window tkwin,
    char *internalPtr,		/* Pointer to storage for value. */
    char *oldInternalPtr)	/* Pointer to old value. */
{
    *(double **)internalPtr = *(double **)oldInternalPtr;
}

