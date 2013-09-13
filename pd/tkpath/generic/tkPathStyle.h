/*
 * tkPathStyle.h --
 *
 *	This file contains definitions for style objects used when drawing paths.
 *	Mostly used for option parsing.
 *
 * Copyright (c) 2007-2008  Mats Bengtsson
 *
 * $Id: tkPathStyle.h,v 1.5 2008/06/03 08:08:17 matben Exp $
 */

#include "tkIntPath.h"


int 		MatrixSetOption(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                    Tcl_Obj **value, char *recordPtr, int internalOffset, char *oldInternalPtr, int flags);
Tcl_Obj *	MatrixGetOption(ClientData clientData, Tk_Window tkwin, char *recordPtr, int internalOffset);
void		MatrixRestoreOption(ClientData clientData, Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);
void		MatrixFreeOption(ClientData clientData, Tk_Window tkwin, char *internalPtr);
int 		PathColorSetOption(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                    Tcl_Obj **value, char *recordPtr, int internalOffset, char *oldInternalPtr, int flags);
Tcl_Obj *	PathColorGetOption(ClientData clientData, Tk_Window tkwin, char *recordPtr, int internalOffset);
void		PathColorRestoreOption(ClientData clientData, Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);
void		PathColorFreeOption(ClientData clientData, Tk_Window tkwin, char *internalPtr);

MODULE_SCOPE int	Tk_PathDashOptionSetProc(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj **value,	
			    char *recordPtr, int internalOffset, char *oldInternalPtr, int flags);
MODULE_SCOPE Tcl_Obj *	Tk_PathDashOptionGetProc(ClientData clientData,
			    Tk_Window tkwin, char *recordPtr, int internalOffset);
MODULE_SCOPE void	Tk_PathDashOptionRestoreProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr, char *oldInternalPtr);
MODULE_SCOPE void	Tk_PathDashOptionFreeProc(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr);


MODULE_SCOPE Tk_PathDash *  TkPathDashNew(Tcl_Interp *interp, Tcl_Obj *dashObjPtr);
MODULE_SCOPE void	    TkPathDashFree(Tk_PathDash *dashPtr);

MODULE_SCOPE TkPathStyleInst *TkPathGetStyle(Tcl_Interp *interp, CONST char *name, 
				Tcl_HashTable *tablePtr, TkPathStyleChangedProc *changeProc, 
				ClientData clientData);
MODULE_SCOPE void	    TkPathFreeStyle(TkPathStyleInst *stylePtr);
MODULE_SCOPE void	    TkPathStyleChanged(Tk_PathStyle *masterPtr, int flags);


#define PATH_STYLE_CUSTOM_OPTION_MATRIX		\
    static Tk_ObjCustomOption matrixCO = {	\
        "matrix",				\
        MatrixSetOption,			\
        MatrixGetOption,			\
        MatrixRestoreOption,			\
        MatrixFreeOption,			\
        (ClientData) NULL			\
    };

#define PATH_STYLE_CUSTOM_OPTION_DASH		\
    static Tk_ObjCustomOption dashCO = {	\
        "dasharray",				\
        Tk_PathDashOptionSetProc,		\
        Tk_PathDashOptionGetProc,		\
        Tk_PathDashOptionRestoreProc,		\
        Tk_PathDashOptionFreeProc,		\
        (ClientData) NULL			\
    };
    
#define PATH_STYLE_CUSTOM_OPTION_PATHCOLOR	\
    static Tk_ObjCustomOption pathColorCO = {	\
        "pathcolor",				\
        PathColorSetOption,			\
        PathColorGetOption,			\
        PathColorRestoreOption,			\
        PathColorFreeOption,			\
        (ClientData) NULL			\
    };

#define PATH_STYLE_CUSTOM_OPTION_RECORDS	\
    PATH_STYLE_CUSTOM_OPTION_MATRIX 		\
    PATH_STYLE_CUSTOM_OPTION_DASH
    

/* 
 * These must be kept in sync with defines in X.h! 
 */

#define PATH_OPTION_STRING_TABLES_FILL		\
    static char *fillRuleST[] = {		\
	"evenodd", "nonzero", (char *) NULL	\
    };

#define PATH_OPTION_STRING_TABLES_STROKE	\
    static char *lineCapST[] = {		\
	"notlast", "butt", "round", "projecting", (char *) NULL	\
    };						\
    static char *lineJoinST[] = {		\
	"miter", "round", "bevel", (char *) NULL \
    };


#define PATH_OPTION_SPEC_STYLENAME(typeName)				\
    {TK_OPTION_STRING, "-style", NULL, NULL,				\
        "", Tk_Offset(typeName, styleObj), -1, TK_OPTION_NULL_OK, 0, 0}

/*
 * This assumes that we have a Tk_PathStyle struct element named 'style'.
 */

#define PATH_OPTION_SPEC_STYLE_FILL(typeName, theColor)			\
    {TK_OPTION_STRING, "-fill", NULL, NULL,				\
	theColor, Tk_Offset(typeName, style.fillObj), -1,		\
	TK_OPTION_NULL_OK, 0, PATH_STYLE_OPTION_FILL},			\
    {TK_OPTION_DOUBLE, "-fillopacity", NULL, NULL,			\
        "1.0", -1, Tk_Offset(typeName, style.fillOpacity), 0, 0,        \
        PATH_STYLE_OPTION_FILL_OPACITY},                                \
    {TK_OPTION_STRING_TABLE, "-fillrule", NULL, NULL,			\
        "nonzero", -1, Tk_Offset(typeName, style.fillRule),             \
        0, (ClientData) fillRuleST, PATH_STYLE_OPTION_FILL_RULE}

#define PATH_OPTION_SPEC_STYLE_MATRIX(typeName)                         \
    {TK_OPTION_CUSTOM, "-matrix", NULL, NULL,				\
	NULL, -1, Tk_Offset(typeName, style.matrixPtr),			\
	TK_OPTION_NULL_OK, (ClientData) &matrixCO, PATH_STYLE_OPTION_MATRIX}

#define PATH_OPTION_SPEC_STYLE_STROKE(typeName, theColor)		\
    {TK_OPTION_COLOR, "-stroke", NULL, NULL,				\
        theColor, -1, Tk_Offset(typeName, style.strokeColor),		\
        TK_OPTION_NULL_OK, 0, PATH_STYLE_OPTION_STROKE},		\
    {TK_OPTION_CUSTOM, "-strokedasharray", NULL, NULL,			\
	NULL, -1, Tk_Offset(typeName, style.dashPtr),			\
	0, (ClientData) &dashCO,					\
        PATH_STYLE_OPTION_STROKE_DASHARRAY},				\
    {TK_OPTION_STRING_TABLE, "-strokelinecap", NULL, NULL,		\
        "butt", -1, Tk_Offset(typeName, style.capStyle),		\
        0, (ClientData) lineCapST, PATH_STYLE_OPTION_STROKE_LINECAP},	\
    {TK_OPTION_STRING_TABLE, "-strokelinejoin", NULL, NULL,		\
        "round", -1, Tk_Offset(typeName, style.joinStyle),		\
        0, (ClientData) lineJoinST, PATH_STYLE_OPTION_STROKE_LINEJOIN}, \
    {TK_OPTION_DOUBLE, "-strokemiterlimit", NULL, NULL,			\
        "4.0", -1, Tk_Offset(typeName, style.miterLimit), 0, 0,		\
        PATH_STYLE_OPTION_STROKE_MITERLIMIT},                           \
    {TK_OPTION_DOUBLE, "-strokeopacity", NULL, NULL,			\
        "1.0", -1, Tk_Offset(typeName, style.strokeOpacity), 0, 0,	\
        PATH_STYLE_OPTION_STROKE_OPACITY},				\
    {TK_OPTION_DOUBLE, "-strokewidth", NULL, NULL,			\
        "1.0", -1, Tk_Offset(typeName, style.strokeWidth), 0, 0,    	\
        PATH_STYLE_OPTION_STROKE_WIDTH}
        
#define PATH_OPTION_SPEC_END						\
	{TK_OPTION_END, NULL, NULL, NULL,				\
		NULL, 0, -1, 0, (ClientData) NULL, 0}




