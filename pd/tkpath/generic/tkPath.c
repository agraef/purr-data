/*
 * tkPath.c --
 *
 *		This file implements a path drawing model
 *      SVG counterpart. See http://www.w3.org/TR/SVG11/.
 *		It contains the generic parts that do not refer to the canvas.
 *
 * Copyright (c) 2005-2008  Mats Bengtsson
 *
 * $Id$
 */

#include "tkIntPath.h"

/* For debugging. */
extern Tcl_Interp *gInterp;

static const char kPathSyntaxError[] = "syntax error in path definition";


int 	
PixelAlignObjCmd(ClientData clientData, Tcl_Interp* interp,
        int objc, Tcl_Obj* CONST objv[])
{
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(TkPathPixelAlign()));    
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * GetPathInstruction --
 *
 *		Gets the path instruction at position index of objv.
 *		If unrecognized instruction returns PATH_NEXT_ERROR.
 *
 * Results:
 *		A PATH_NEXT_* result.
 *
 * Side effects:
 *		None.
 *
 *--------------------------------------------------------------
 */

static int
GetPathInstruction(Tcl_Interp *interp, Tcl_Obj *CONST objv[], int index, char *c) 
{
    int len;
    int result;
    char *str;
    
    *c = '\0';
    str = Tcl_GetStringFromObj(objv[index], &len);
    if (isalpha(str[0])) {
        if (len != 1) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
            result = PATH_NEXT_ERROR;
        } else {
            switch (str[0]) {
                case 'M': case 'm': case 'L': case 'l':
                case 'H': case 'h': case 'V': case 'v':
                case 'A': case 'a': case 'Q': case 'q':
                case 'T': case 't': case 'C': case 'c':
                case 'S': case 's': case 'Z': case 'z':
                    result = PATH_NEXT_INSTRUCTION;
                    *c = str[0];
                    break;
                default:
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
                    result = PATH_NEXT_ERROR;
                    break;
            }
        }
    } else {
        result = PATH_NEXT_OTHER;
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * GetPathDouble, GetPathBoolean, GetPathPoint, GetPathTwoPoints,
 * GetPathThreePoints, GetPathArcParameters --
 *
 *		Gets a certain number of numbers from objv.
 *		Increments indexPtr by the number of numbers extracted
 *		if succesful, else it is unchanged.
 *
 * Results:
 *		A standard tcl result.
 *
 * Side effects:
 *		None.
 *
 *--------------------------------------------------------------
 */

static int
GetPathDouble(Tcl_Interp *interp, Tcl_Obj *CONST objv[], int len, int *indexPtr, double *zPtr) 
{
    int result;

    if (*indexPtr > len - 1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
        result = TCL_ERROR;
    } else {
        result = Tcl_GetDoubleFromObj(interp, objv[*indexPtr], zPtr);
        if (result == TCL_OK) {
            (*indexPtr)++;
        }
    }
    return result;
}

static int
GetPathBoolean(Tcl_Interp *interp, Tcl_Obj *CONST objv[], int len, int *indexPtr, char *boolPtr) 
{
    int result;
    int boolean;

    if (*indexPtr > len - 1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
        result = TCL_ERROR;
    } else {
        result = Tcl_GetBooleanFromObj(interp, objv[*indexPtr], &boolean);
        if (result == TCL_OK) {
            (*indexPtr)++;
            *boolPtr = boolean;
        }
    }
    return result;
}

static int
GetPathPoint(Tcl_Interp *interp, Tcl_Obj *CONST objv[], int len, int *indexPtr, 
        double *xPtr, double *yPtr)
{
    int result = TCL_OK;
    int indIn = *indexPtr;
    
    if (*indexPtr > len - 2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
        result = TCL_ERROR;
    } else if (Tcl_GetDoubleFromObj(interp, objv[(*indexPtr)++], xPtr) != TCL_OK) {
        *indexPtr = indIn;
        result = TCL_ERROR;
    } else if (Tcl_GetDoubleFromObj(interp, objv[(*indexPtr)++], yPtr) != TCL_OK) {
        *indexPtr = indIn;
        result = TCL_ERROR;
    }
    return result;
}

static int
GetPathTwoPoints(Tcl_Interp *interp, Tcl_Obj *CONST objv[], int len, int *indexPtr, 
        double *x1Ptr, double *y1Ptr, double *x2Ptr, double *y2Ptr)
{
    int result;
    int indIn = *indexPtr;

    result = GetPathPoint(interp, objv, len, indexPtr, x1Ptr, y1Ptr);
    if (result == TCL_OK) {
        if (GetPathPoint(interp, objv, len, indexPtr, x2Ptr, y2Ptr) != TCL_OK) {
            *indexPtr = indIn;
            result = TCL_ERROR;
        }
    }
    return result;
}

static int
GetPathThreePoints(Tcl_Interp *interp, Tcl_Obj *CONST objv[], int len, int *indexPtr, 
        double *x1Ptr, double *y1Ptr, double *x2Ptr, double *y2Ptr,
        double *x3Ptr, double *y3Ptr)
{
    int result;
    int indIn = *indexPtr;

    result = GetPathPoint(interp, objv, len, indexPtr, x1Ptr, y1Ptr);
    if (result == TCL_OK) {
        if (GetPathPoint(interp, objv, len, indexPtr, x2Ptr, y2Ptr) != TCL_OK) {
            *indexPtr = indIn;
            result = TCL_ERROR;
        } else if (GetPathPoint(interp, objv, len, indexPtr, x3Ptr, y3Ptr) != TCL_OK) {
            *indexPtr = indIn;
            result = TCL_ERROR;
        }
    }
    return result;
}

static int
GetPathArcParameters(Tcl_Interp *interp, Tcl_Obj *CONST objv[], int len, int *indexPtr,
        double *radXPtr, double *radYPtr, double *anglePtr, 
        char *largeArcFlagPtr, char *sweepFlagPtr, 
        double *xPtr, double *yPtr)
{
    int result;
    int indIn = *indexPtr;

    result = GetPathPoint(interp, objv, len, indexPtr, radXPtr, radYPtr);
    if (result == TCL_OK) {
        if (GetPathDouble(interp, objv, len, indexPtr, anglePtr) != TCL_OK) {
            *indexPtr = indIn;
            result = TCL_ERROR;
        } else if (GetPathBoolean(interp, objv, len, indexPtr, largeArcFlagPtr) != TCL_OK) {
            *indexPtr = indIn;
            result = TCL_ERROR;
        } else if (GetPathBoolean(interp, objv, len, indexPtr, sweepFlagPtr) != TCL_OK) {
            *indexPtr = indIn;
            result = TCL_ERROR;
        } else if (GetPathPoint(interp, objv, len, indexPtr, xPtr, yPtr) != TCL_OK) {
            *indexPtr = indIn;
            result = TCL_ERROR;
        } 
    }
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * NewMoveToAtom, NewLineToAtom, NewArcAtom, NewQuadBezierAtom,
 * NewCurveToAtom, NewCloseAtom --
 *
 *		Creates a PathAtom of the specified type using the given
 *		parameters. It updates the currentX and currentY.
 *
 * Results:
 *		A PathAtom pointer.
 *
 * Side effects:
 *		Memory allocated.
 *
 *--------------------------------------------------------------
 */

PathAtom *
NewMoveToAtom(double x, double y)
{
    PathAtom *atomPtr;
    MoveToAtom *moveToAtomPtr;

    moveToAtomPtr = (MoveToAtom *) ckalloc((unsigned) (sizeof(MoveToAtom)));
    atomPtr = (PathAtom *) moveToAtomPtr;
    atomPtr->type = PATH_ATOM_M;
    atomPtr->nextPtr = NULL;
    moveToAtomPtr->x = x;
    moveToAtomPtr->y = y;
    return atomPtr;
}

PathAtom *
NewLineToAtom(double x, double y)
{
    PathAtom *atomPtr;
    LineToAtom *lineToAtomPtr;

    lineToAtomPtr = (LineToAtom *) ckalloc((unsigned) (sizeof(LineToAtom)));
    atomPtr = (PathAtom *) lineToAtomPtr;
    atomPtr->type = PATH_ATOM_L;
    atomPtr->nextPtr = NULL;
    lineToAtomPtr->x = x;
    lineToAtomPtr->y = y;
    return atomPtr;
}

PathAtom *
NewArcAtom(double radX, double radY, 
        double angle, char largeArcFlag, char sweepFlag, double x, double y)
{
    PathAtom *atomPtr;
    ArcAtom *arcAtomPtr;

    arcAtomPtr = (ArcAtom *) ckalloc((unsigned) (sizeof(ArcAtom)));
    atomPtr = (PathAtom *) arcAtomPtr;
    atomPtr->type = PATH_ATOM_A;
    atomPtr->nextPtr = NULL;    
    arcAtomPtr->radX = radX;
    arcAtomPtr->radY = radY;
    arcAtomPtr->angle = angle;
    arcAtomPtr->largeArcFlag = largeArcFlag;
    arcAtomPtr->sweepFlag = sweepFlag;
    arcAtomPtr->x = x;
    arcAtomPtr->y = y;
    return atomPtr;
}

PathAtom *
NewQuadBezierAtom(double ctrlX, double ctrlY, double anchorX, double anchorY)
{
    PathAtom *atomPtr;
    QuadBezierAtom *quadBezierAtomPtr;

    quadBezierAtomPtr = (QuadBezierAtom *) ckalloc((unsigned) (sizeof(QuadBezierAtom)));
    atomPtr = (PathAtom *) quadBezierAtomPtr;
    atomPtr->type = PATH_ATOM_Q;
    atomPtr->nextPtr = NULL;
    quadBezierAtomPtr->ctrlX = ctrlX;
    quadBezierAtomPtr->ctrlY = ctrlY;
    quadBezierAtomPtr->anchorX = anchorX;
    quadBezierAtomPtr->anchorY = anchorY;
    return atomPtr;
}

PathAtom *
NewCurveToAtom(double ctrlX1, double ctrlY1, double ctrlX2, double ctrlY2, 
        double anchorX, double anchorY)
{
    PathAtom *atomPtr;
    CurveToAtom *curveToAtomPtr;

    curveToAtomPtr = (CurveToAtom *) ckalloc((unsigned) (sizeof(CurveToAtom)));
    atomPtr = (PathAtom *) curveToAtomPtr;
    atomPtr->type = PATH_ATOM_C;
    atomPtr->nextPtr = NULL;
    curveToAtomPtr->ctrlX1 = ctrlX1;
    curveToAtomPtr->ctrlY1 = ctrlY1;
    curveToAtomPtr->ctrlX2 = ctrlX2;
    curveToAtomPtr->ctrlY2 = ctrlY2;
    curveToAtomPtr->anchorX = anchorX;
    curveToAtomPtr->anchorY = anchorY;
    return atomPtr;
}

PathAtom *
NewRectAtom(double pointsPtr[])
{
    PathAtom *atomPtr;
    RectAtom *rectAtomPtr;

    rectAtomPtr = (RectAtom *) ckalloc((unsigned) (sizeof(RectAtom)));
    atomPtr = (PathAtom *) rectAtomPtr;    
    atomPtr->nextPtr = NULL;
    atomPtr->type = PATH_ATOM_RECT;
    rectAtomPtr->x = pointsPtr[0];
    rectAtomPtr->y = pointsPtr[1];
    rectAtomPtr->width = pointsPtr[2] - pointsPtr[0];
    rectAtomPtr->height = pointsPtr[3] - pointsPtr[1];
    return atomPtr;
}

PathAtom *
NewCloseAtom(double x, double y)
{
    PathAtom *atomPtr;
    CloseAtom *closeAtomPtr;

    closeAtomPtr = (CloseAtom *) ckalloc((unsigned) (sizeof(CloseAtom)));
    atomPtr = (PathAtom *) closeAtomPtr;
    atomPtr->type = PATH_ATOM_Z;
    atomPtr->nextPtr = NULL;
    closeAtomPtr->x = x;
    closeAtomPtr->y = y;
    return atomPtr;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathParseToAtoms
 *
 *		Takes a tcl list of values which defines the path item and
 *		parses them into a linked list of path atoms.
 *
 * Results:
 *		A standard Tcl result.
 *
 * Side effects:
 *		None
 *
 *--------------------------------------------------------------
 */

int
TkPathParseToAtoms(Tcl_Interp *interp, Tcl_Obj *listObjPtr, PathAtom **atomPtrPtr, int *lenPtr)
{
    char 	currentInstr;		/* current instruction (M, l, c, etc.) */
    char 	lastInstr;			/* previous instruction */
    int 	len;
    int 	currentInd;
    int 	index;
    int 	next;
    int 	relative;
    double 	currentX, currentY;	/* current point */
    double 	startX, startY;		/* the current moveto point */
    double 	ctrlX, ctrlY;		/* last control point, for s, S, t, T */
    double 	x, y;
    Tcl_Obj **objv;
    PathAtom *atomPtr = NULL;
    PathAtom *currentAtomPtr = NULL;
    
    *atomPtrPtr = NULL;
    currentX = 0.0;
    currentY = 0.0;
    startX = 0.0;
    startY = 0.0;
    ctrlX = 0.0;
    ctrlY = 0.0;
    lastInstr = 'M';	/* If first instruction is missing it defaults to M ? */
    relative = 0;
        
    if (Tcl_ListObjGetElements(interp, listObjPtr, lenPtr, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    len = *lenPtr;
    
    /* First some error checking. Necessary??? */
    if (len < 3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "path specification too short", -1));
        return TCL_ERROR;
    }
    if ((GetPathInstruction(interp, objv, 0, &currentInstr) != PATH_NEXT_INSTRUCTION) || 
            (toupper(currentInstr) != 'M')) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
                "path must start with M or m", -1));
        return TCL_ERROR;
    }
    currentInd = 1;
    if (GetPathPoint(interp, objv, len, &currentInd, &x, &y) != TCL_OK) {
        return TCL_ERROR;
    }
    currentInd = 0;
     
    while (currentInd < len) {

        next = GetPathInstruction(interp, objv, currentInd, &currentInstr);
        if (next == PATH_NEXT_ERROR) {
            goto error;
        } else if (next == PATH_NEXT_INSTRUCTION) {
            relative = islower(currentInstr);
            currentInd++;
        } else if (next == PATH_NEXT_OTHER) {
        
            /* Use rule to find instruction to use. */
            if (lastInstr == 'M') {
                currentInstr = 'L';
            } else if (lastInstr == 'm') {
                currentInstr = 'l';
            } else {
                currentInstr = lastInstr;
            }
            relative = islower(currentInstr);
        }
        index = currentInd;
        
        switch (currentInstr) {
        
            case 'M': case 'm': {
                if (GetPathPoint(interp, objv, len, &index, &x, &y) != TCL_OK) {
                    goto error;
                }
                if (relative) {
                    x += currentX;
                    y += currentY;
                }    
                atomPtr = NewMoveToAtom(x, y);
                if (currentAtomPtr == NULL) {
                    *atomPtrPtr = atomPtr;
                } else {
                    currentAtomPtr->nextPtr = atomPtr;
                }
                currentAtomPtr = atomPtr;
                currentX = x;
                currentY = y;
                startX = x;
                startY = y;
                break;
            }
            
            case 'L': case 'l': {
                if (index > len - 2) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
                    goto error;
                }
                if (GetPathPoint(interp, objv, len, &index, &x, &y) == TCL_OK) {
                    if (relative) {
                        x += currentX;
                        y += currentY;
                    }    
                    atomPtr = NewLineToAtom(x, y);
                    currentAtomPtr->nextPtr = atomPtr;
                    currentAtomPtr = atomPtr;
                    currentX = x;
                    currentY = y;
                } else {
                    goto error;
                }
                break;
            }
            
            case 'A': case 'a': {
                double radX, radY, angle;
                char largeArcFlag, sweepFlag;
                
                if (GetPathArcParameters(interp, objv, len, &index,
                        &radX, &radY, &angle, &largeArcFlag, &sweepFlag,
                        &x, &y) == TCL_OK) {
                    if (relative) {
                        x += currentX;
                        y += currentY;
                    }    
                    atomPtr = NewArcAtom(radX, radY, angle, largeArcFlag, sweepFlag, x, y);
                    currentAtomPtr->nextPtr = atomPtr;
                    currentAtomPtr = atomPtr;
                    currentX = x;
                    currentY = y;
                } else {
                    goto error;
                }
                break;
            }
            
            case 'C': case 'c': {
                double x1, y1, x2, y2;	/* The two control points. */
                
                if (index > len - 6) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
                    goto error;
                }
                if (GetPathThreePoints(interp, objv, len, &index, &x1, &y1, &x2, &y2, &x, &y) == TCL_OK) {
                    if (relative) {
                        x1 += currentX;
                        y1 += currentY;
                        x2 += currentX;
                        y2 += currentY;
                        x  += currentX;
                        y  += currentY;
                    }    
                    atomPtr = NewCurveToAtom(x1, y1, x2, y2, x, y);
                    currentAtomPtr->nextPtr = atomPtr;
                    currentAtomPtr = atomPtr;
                    ctrlX = x2; 	/* Keep track of the last control point. */
                    ctrlY = y2;
                    currentX = x;
                    currentY = y;
                } else {
                    goto error;
                }
                break;
            }
            
            case 'S': case 's': {
                double x1, y1;	/* The first control point. */
                double x2, y2;	/* The second control point. */
                
                if ((toupper(lastInstr) == 'C') || (toupper(lastInstr) == 'S')) {
                    /* The first controlpoint is the reflection of the last one about the current point: */
                    x1 = 2 * currentX - ctrlX;
                    y1 = 2 * currentY - ctrlY;                    
                } else {
                    /* The first controlpoint is equal to the current point: */
                    x1 = currentX;
                    y1 = currentY;
                }
                if (index > len - 4) {
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(kPathSyntaxError, -1));
                    goto error;
                }
                if (GetPathTwoPoints(interp, objv, len, &index, &x2, &y2, &x, &y) == TCL_OK) {
                    if (relative) {
                        x2 += currentX;
                        y2 += currentY;
                        x  += currentX;
                        y  += currentY;
                    }    
                    atomPtr = NewCurveToAtom(x1, y1, x2, y2, x, y);
                    currentAtomPtr->nextPtr = atomPtr;
                    currentAtomPtr = atomPtr;
                    ctrlX = x2; 	/* Keep track of the last control point. */
                    ctrlY = y2;
                    currentX = x;
                    currentY = y;
                } else {
                    goto error;
                }
                break;
            }
            
            case 'Q': case 'q': {
                double x1, y1;	/* The control point. */
                
                if (GetPathTwoPoints(interp, objv, len, &index, &x1, &y1, &x, &y) == TCL_OK) {
                    if (relative) {
                        x1 += currentX;
                        y1 += currentY;
                        x  += currentX;
                        y  += currentY;
                    }    
                    atomPtr = NewQuadBezierAtom(x1, y1, x, y);
                    currentAtomPtr->nextPtr = atomPtr;
                    currentAtomPtr = atomPtr;
                    ctrlX = x1; 	/* Keep track of the last control point. */
                    ctrlY = y1;
                    currentX = x;
                    currentY = y;
                } else {
                    goto error;
                }
                break;
            }
            
            case 'T': case 't': {
                double x1, y1;	/* The control point. */
                
                if ((toupper(lastInstr) == 'Q') || (toupper(lastInstr) == 'T')) {
                    /* The controlpoint is the reflection of the last one about the current point: */
                    x1 = 2 * currentX - ctrlX;
                    y1 = 2 * currentY - ctrlY;                    
                } else {
                    /* The controlpoint is equal to the current point: */
                    x1 = currentX;
                    y1 = currentY;
                }
                if (GetPathPoint(interp, objv, len, &index, &x, &y) == TCL_OK) {
                    if (relative) {
                        x  += currentX;
                        y  += currentY;
                    }    
                    atomPtr = NewQuadBezierAtom(x1, y1, x, y);
                    currentAtomPtr->nextPtr = atomPtr;
                    currentAtomPtr = atomPtr;
                    ctrlX = x1; 	/* Keep track of the last control point. */
                    ctrlY = y1;
                    currentX = x;
                    currentY = y;
                } else {
                    goto error;
                }
                break;
            }
            
            case 'H': {
                while ((index < len) && 
                        (GetPathDouble(interp, objv, len, &index, &x) == TCL_OK))
                    ;
                atomPtr = NewLineToAtom(x, currentY);
                currentAtomPtr->nextPtr = atomPtr;
                currentAtomPtr = atomPtr;
                currentX = x;
                break;
            }
            
            case 'h': {
                double z;
                
                x = currentX;
                while ((index < len) &&
                        (GetPathDouble(interp, objv, len, &index, &z) == TCL_OK)) {
                    x += z;
                }
                atomPtr = NewLineToAtom(x, currentY);
                currentAtomPtr->nextPtr = atomPtr;
                currentAtomPtr = atomPtr;
                currentX = x;
                break;
            }
            
            case 'V': {
                while ((index < len) && 
                        (GetPathDouble(interp, objv, len, &index, &y) == TCL_OK))
                    ;
                atomPtr = NewLineToAtom(currentX, y);
                currentAtomPtr->nextPtr = atomPtr;
                currentAtomPtr = atomPtr;
                currentY = y;
                break;
            }
            
            case 'v': {
                double z;
                
                y = currentY;
                while ((index < len) &&
                        (GetPathDouble(interp, objv, len, &index, &z) == TCL_OK)) {
                    y += z;
                }
                atomPtr = NewLineToAtom(currentX, y);
                currentAtomPtr->nextPtr = atomPtr;
                currentAtomPtr = atomPtr;
                currentY = y;
                break;
            }
            
            case 'Z': case 'z': {
                atomPtr = NewCloseAtom(startX, startY);
                currentAtomPtr->nextPtr = atomPtr;
                currentAtomPtr = atomPtr;
                currentX = startX;
                currentY = startY;
                break;
            }
            
            default: {
                Tcl_SetObjResult(interp, Tcl_NewStringObj(
                        "unrecognized path instruction", -1));
                goto error;
            }
        }
        currentInd = index;
        lastInstr = currentInstr;
    }
    
    /* When we parse coordinates there may be some junk result
     * left in the interpreter to be cleared out. */
    Tcl_ResetResult(interp);
    return TCL_OK;
    
error:

    TkPathFreeAtoms(*atomPtrPtr);
    *atomPtrPtr = NULL;
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathFreeAtoms
 *
 *		Frees up all memory allocated for the path atoms.
 *
 * Results:
 *		None.
 *
 * Side effects:
 *		None.
 *
 *--------------------------------------------------------------
 */

void
TkPathFreeAtoms(PathAtom *pathAtomPtr)
{
    PathAtom *tmpAtomPtr;

    while (pathAtomPtr != NULL) {
        tmpAtomPtr = pathAtomPtr;
        pathAtomPtr = tmpAtomPtr->nextPtr;
        ckfree((char *) tmpAtomPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkPathNormalize
 *
 *		Takes a list of PathAtoms and creates a tcl list where
 *		elements have a standard form. All upper case instructions,
 *		no repeates.
 *
 * Results:
 *		A standard Tcl result.
 *
 * Side effects:
 *		New list returned in listObjPtrPtr.
 *
 *--------------------------------------------------------------
 */

int
TkPathNormalize(Tcl_Interp *interp, PathAtom *atomPtr, Tcl_Obj **listObjPtrPtr)
{
    Tcl_Obj *normObjPtr;    

    normObjPtr = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );

    while (atomPtr != NULL) {
    
        switch (atomPtr->type) {
            case PATH_ATOM_M: { 
                MoveToAtom *move = (MoveToAtom *) atomPtr;
                
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewStringObj("M", -1));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(move->x));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(move->y));
                break;
            }
            case PATH_ATOM_L: {
                LineToAtom *line = (LineToAtom *) atomPtr;
                
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewStringObj("L", -1));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(line->x));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(line->y));
                break;
            }
            case PATH_ATOM_A: {
                ArcAtom *arc = (ArcAtom *) atomPtr;
                
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewStringObj("A", -1));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(arc->radX));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(arc->radY));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(arc->angle));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewBooleanObj(arc->largeArcFlag));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewBooleanObj(arc->sweepFlag));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(arc->x));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(arc->y));
                break;
            }
            case PATH_ATOM_Q: {
                QuadBezierAtom *quad = (QuadBezierAtom *) atomPtr;
                
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewStringObj("Q", -1));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(quad->ctrlX));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(quad->ctrlY));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(quad->anchorX));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(quad->anchorY));
                break;
            }
            case PATH_ATOM_C: {
                CurveToAtom *curve = (CurveToAtom *) atomPtr;

                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewStringObj("C", -1));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(curve->ctrlX1));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(curve->ctrlY1));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(curve->ctrlX2));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(curve->ctrlY2));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(curve->anchorX));
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewDoubleObj(curve->anchorY));
                break;
            }
            case PATH_ATOM_Z: {
                Tcl_ListObjAppendElement(interp, normObjPtr, Tcl_NewStringObj("Z", -1));
                break;
            }
            case PATH_ATOM_ELLIPSE:
            case PATH_ATOM_RECT: {
                /* Empty. */
                break;
            }
        }
        atomPtr = atomPtr->nextPtr;
    }
    *listObjPtrPtr = normObjPtr;
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathMakePath
 *
 *		Defines the path using the PathAtom.
 *
 * Results:
 *		A standard Tcl result.
 *
 * Side effects:
 *		Defines the current path in drawable.
 *
 *--------------------------------------------------------------
 */

int
TkPathMakePath(
    TkPathContext context,
    PathAtom *atomPtr,
    Tk_PathStyle *stylePtr)
{
    TkPathBeginPath(context, stylePtr);

    while (atomPtr != NULL) {
    
        switch (atomPtr->type) {
            case PATH_ATOM_M: { 
                MoveToAtom *move = (MoveToAtom *) atomPtr;
                TkPathMoveTo(context, move->x, move->y);
                break;
            }
            case PATH_ATOM_L: {
                LineToAtom *line = (LineToAtom *) atomPtr;                
                TkPathLineTo(context, line->x, line->y);
                break;
            }
            case PATH_ATOM_A: {
                ArcAtom *arc = (ArcAtom *) atomPtr;
                TkPathArcTo(context, arc->radX, arc->radY, arc->angle, 
                        arc->largeArcFlag, arc->sweepFlag,
                        arc->x, arc->y);
                break;
            }
            case PATH_ATOM_Q: {
                QuadBezierAtom *quad = (QuadBezierAtom *) atomPtr;
                TkPathQuadBezier(context, 
                        quad->ctrlX, quad->ctrlY,
                        quad->anchorX, quad->anchorY);
                break;
            }
            case PATH_ATOM_C: {
                CurveToAtom *curve = (CurveToAtom *) atomPtr;
                TkPathCurveTo(context, 
                        curve->ctrlX1, curve->ctrlY1,
                        curve->ctrlX2, curve->ctrlY2,
                        curve->anchorX, curve->anchorY);
                break;
            }
            case PATH_ATOM_Z: {
                TkPathClosePath(context);
                break;
            }
            case PATH_ATOM_ELLIPSE: {
                EllipseAtom *ell = (EllipseAtom *) atomPtr;
                TkPathOval(context, ell->cx, ell->cy, ell->rx, ell->ry);
                break;
            }
            case PATH_ATOM_RECT: {
                RectAtom *rect = (RectAtom *) atomPtr;
                TkPathRect(context, rect->x, rect->y, rect->width, rect->height);
                break;
            }
        }
        atomPtr = atomPtr->nextPtr;
    }
    TkPathEndPath(context);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkPathArcToUsingBezier
 *
 *		Translates an ArcTo drawing into a sequence of CurveTo.
 *		Helper function for the platform specific drawing code.
 *
 * Results:
 *		None.
 *
 * Side effects:
 *		None.
 *
 *--------------------------------------------------------------
 */

void
TkPathArcToUsingBezier(TkPathContext ctx,
        double rx, double ry, 
        double phiDegrees, 	/* The rotation angle in degrees! */
        char largeArcFlag, char sweepFlag, double x2, double y2)
{
    int result;
    int i, segments;
    double x1, y1;
    double cx, cy;
    double theta1, dtheta, phi;
    double sinPhi, cosPhi;
    double delta, t;
    PathPoint pt;
    
    TkPathGetCurrentPosition(ctx, &pt);
    x1 = pt.x;
    y1 = pt.y;

    /* All angles except phi is in radians! */
    phi = phiDegrees * DEGREES_TO_RADIANS;
    
    /* Check return value and take action. */
    result = EndpointToCentralArcParameters(x1, y1,
            x2, y2, rx, ry, phi, largeArcFlag, sweepFlag,
            &cx, &cy, &rx, &ry,
            &theta1, &dtheta);
    if (result == kPathArcSkip) {
		return;
	} else if (result == kPathArcLine) {
		TkPathLineTo(ctx, x2, y2);
		return;
    }
    sinPhi = sin(phi);
    cosPhi = cos(phi);
    
    /* Convert into cubic bezier segments <= 90deg (from mozilla/svg; not checked) */
    segments = (int) ceil(fabs(dtheta/(M_PI/2.0)));
    delta = dtheta/segments;
    t = 8.0/3.0 * sin(delta/4.0) * sin(delta/4.0) / sin(delta/2.0);
    
    for (i = 0; i < segments; ++i) {
        double cosTheta1 = cos(theta1);
        double sinTheta1 = sin(theta1);
        double theta2 = theta1 + delta;
        double cosTheta2 = cos(theta2);
        double sinTheta2 = sin(theta2);
        
        /* a) calculate endpoint of the segment: */
        double xe = cosPhi * rx*cosTheta2 - sinPhi * ry*sinTheta2 + cx;
        double ye = sinPhi * rx*cosTheta2 + cosPhi * ry*sinTheta2 + cy;
    
        /* b) calculate gradients at start/end points of segment: */
        double dx1 = t * ( - cosPhi * rx*sinTheta1 - sinPhi * ry*cosTheta1);
        double dy1 = t * ( - sinPhi * rx*sinTheta1 + cosPhi * ry*cosTheta1);
        
        double dxe = t * ( cosPhi * rx*sinTheta2 + sinPhi * ry*cosTheta2);
        double dye = t * ( sinPhi * rx*sinTheta2 - cosPhi * ry*cosTheta2);
    
        /* c) draw the cubic bezier: */
        TkPathCurveTo(ctx, x1+dx1, y1+dy1, xe+dxe, ye+dye, xe, ye);

        /* do next segment */
        theta1 = theta2;
        x1 = (float) xe;
        y1 = (float) ye;
    }
}

/*-----------------------------------------------------------------------*/


