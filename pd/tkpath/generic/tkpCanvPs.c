/*
 * tkpCanvPs.c --
 *
 *	This module provides Postscript output support for canvases, including
 *	the "postscript" widget command plus a few utility functions used for
 *	generating Postscript.
 *
 *      NB: A number of duplicate functions have been cleaned out from this file.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#include "tkInt.h"
#include "tkIntPath.h"
#include "tkpCanvas.h"

/*
 * See tkCanvas.h for key data structures used to implement canvases.
 */

/*
 * The following definition is used in generating postscript for images and
 * windows.
 */

typedef struct TkColormapData {	/* Hold color information for a window */
    int separated;		/* Whether to use separate color bands */
    int color;			/* Whether window is color or black/white */
    int ncolors;		/* Number of color values stored */
    XColor *colors;		/* Pixel value -> RGB mappings */
    int red_mask, green_mask, blue_mask;	/* Masks and shifts for each */
    int red_shift, green_shift, blue_shift;	/* color band */
} TkColormapData;

/*
 * One of the following structures is created to keep track of Postscript
 * output being generated. It consists mostly of information provided on the
 * widget command line.
 */

typedef struct TkPostscriptInfo {
    int x, y, width, height;	/* Area to print, in canvas pixel
				 * coordinates. */
    int x2, y2;			/* x+width and y+height. */
    char *pageXString;		/* String value of "-pagex" option or NULL. */
    char *pageYString;		/* String value of "-pagey" option or NULL. */
    double pageX, pageY;	/* Postscript coordinates (in points)
				 * corresponding to pageXString and
				 * pageYString. Don't forget that y-values
				 * grow upwards for Postscript! */
    char *pageWidthString;	/* Printed width of output. */
    char *pageHeightString;	/* Printed height of output. */
    double scale;		/* Scale factor for conversion: each pixel
				 * maps into this many points. */
    Tk_Anchor pageAnchor;	/* How to anchor bbox on Postscript page. */
    int rotate;			/* Non-zero means output should be rotated on
				 * page (landscape mode). */
    char *fontVar;		/* If non-NULL, gives name of global variable
				 * containing font mapping information.
				 * Malloc'ed. */
    char *colorVar;		/* If non-NULL, give name of global variable
				 * containing color mapping information.
				 * Malloc'ed. */
    char *colorMode;		/* Mode for handling colors: "monochrome",
				 * "gray", or "color".  Malloc'ed. */
    int colorLevel;		/* Numeric value corresponding to colorMode: 0
				 * for mono, 1 for gray, 2 for color. */
    char *fileName;		/* Name of file in which to write Postscript;
				 * NULL means return Postscript info as
				 * result. Malloc'ed. */
    char *channelName;		/* If -channel is specified, the name of the
                                 * channel to use. */
    Tcl_Channel chan;		/* Open channel corresponding to fileName. */
    Tcl_HashTable fontTable;	/* Hash table containing names of all font
				 * families used in output. The hash table
				 * values are not used. */
    int prepass;		/* Non-zero means that we're currently in the
				 * pre-pass that collects font information, so
				 * the Postscript generated isn't relevant. */
    int prolog;			/* Non-zero means output should contain the
				 * file prolog.ps in the header. */
} TkPostscriptInfo;

/*
 * The table below provides a template that's used to process arguments to the
 * canvas "postscript" command and fill in TkPostscriptInfo structures.
 */

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_STRING, "-colormap", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, colorVar),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-colormode", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, colorMode),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-file", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, fileName),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-channel", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, channelName),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-fontmap", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, fontVar),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-height", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, height),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_ANCHOR, "-pageanchor", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, pageAnchor),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-pageheight", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, pageHeightString),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-pagewidth", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, pageWidthString),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-pagex", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, pageXString),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-pagey", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, pageYString),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-prolog", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, prolog),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-rotate", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, rotate),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-width", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, width),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-x", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, x),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-y", NULL, NULL,
	NULL, -1, Tk_Offset(TkPostscriptInfo, y),
	TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL,           
	NULL, 0, -1, 0, (ClientData) NULL, 0}
};

static Tk_OptionTable optionTable = NULL;

/*
 * Forward declarations for functions defined later in this file:
 */

static int		GetPostscriptPoints(Tcl_Interp *interp,
			    char *string, double *doublePtr);

/*
 *--------------------------------------------------------------
 *
 * TkCanvPostscriptCmd --
 *
 *	This function is invoked to process the "postscript" options of the
 *	widget command for canvas widgets. See the user documentation for
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

    /* ARGSUSED */
int
TkCanvPostscriptCmd(
    TkPathCanvas *canvasPtr,	/* Information about canvas widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. Caller has already parsed
				 * this command enough to know that argv[1] is
				 * "postscript". */
{
    TkPostscriptInfo psInfo, *psInfoPtr = &psInfo;
    Tk_PostscriptInfo oldInfoPtr;
    int result;
    Tk_PathItem *itemPtr;
#define STRING_LENGTH 400
    char string[STRING_LENGTH+1];
    CONST char *p;
    time_t now;
    size_t length;
    Tk_Window tkwin = canvasPtr->tkwin;
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Tcl_DString buffer;
    char psenccmd[] = "::tk::ensure_psenc_is_loaded";
    int deltaX = 0, deltaY = 0;	/* Offset of lower-left corner of area to be
				 * marked up, measured in canvas units from
				 * the positioning point on the page (reflects
				 * anchor position). Initial values needed
				 * only to stop compiler warnings. */

    /*
     * Initialize the data structure describing Postscript generation, then
     * process all the arguments to fill the data structure in.
     */

    result = Tcl_EvalEx(interp,psenccmd,-1,TCL_EVAL_GLOBAL);
    if (result != TCL_OK) {
        return result;
    }
    oldInfoPtr = canvasPtr->psInfo;
    canvasPtr->psInfo = (Tk_PostscriptInfo) psInfoPtr;
    psInfo.x = canvasPtr->xOrigin;
    psInfo.y = canvasPtr->yOrigin;
    psInfo.width = -1;
    psInfo.height = -1;
    psInfo.pageXString = NULL;
    psInfo.pageYString = NULL;
    psInfo.pageX = 72*4.25;
    psInfo.pageY = 72*5.5;
    psInfo.pageWidthString = NULL;
    psInfo.pageHeightString = NULL;
    psInfo.scale = 1.0;
    psInfo.pageAnchor = TK_ANCHOR_CENTER;
    psInfo.rotate = 0;
    psInfo.fontVar = NULL;
    psInfo.colorVar = NULL;
    psInfo.colorMode = NULL;
    psInfo.colorLevel = 0;
    psInfo.fileName = NULL;
    psInfo.channelName = NULL;
    psInfo.chan = NULL;
    psInfo.prepass = 0;
    psInfo.prolog = 1;
    
    Tcl_InitHashTable(&psInfo.fontTable, TCL_STRING_KEYS);
    if (optionTable == NULL) {
	optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    } 
    if (Tk_InitOptions(interp, (char *) &psInfo, optionTable, tkwin) != TCL_OK) {
        goto cleanup;
    }
    if (Tk_SetOptions(interp, (char *) &psInfo, optionTable, 
	    objc-2, objv+2, tkwin, NULL, NULL) != TCL_OK) {
        goto cleanup;
    }

    if (psInfo.width == -1) {
	psInfo.width = Tk_Width(tkwin);
    }
    if (psInfo.height == -1) {
	psInfo.height = Tk_Height(tkwin);
    }
    psInfo.x2 = psInfo.x + psInfo.width;
    psInfo.y2 = psInfo.y + psInfo.height;

    if (psInfo.pageXString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageXString,
		&psInfo.pageX) != TCL_OK) {
	    goto cleanup;
	}
    }
    if (psInfo.pageYString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageYString,
		&psInfo.pageY) != TCL_OK) {
	    goto cleanup;
	}
    }
    if (psInfo.pageWidthString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageWidthString,
		&psInfo.scale) != TCL_OK) {
	    goto cleanup;
	}
	psInfo.scale /= psInfo.width;
    } else if (psInfo.pageHeightString != NULL) {
	if (GetPostscriptPoints(interp, psInfo.pageHeightString,
		&psInfo.scale) != TCL_OK) {
	    goto cleanup;
	}
	psInfo.scale /= psInfo.height;
    } else {
	psInfo.scale = (72.0/25.4)*WidthMMOfScreen(Tk_Screen(tkwin));
	psInfo.scale /= WidthOfScreen(Tk_Screen(tkwin));
    }
    switch (psInfo.pageAnchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_W:
    case TK_ANCHOR_SW:
	deltaX = 0;
	break;
    case TK_ANCHOR_N:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_S:
	deltaX = -psInfo.width/2;
	break;
    case TK_ANCHOR_NE:
    case TK_ANCHOR_E:
    case TK_ANCHOR_SE:
	deltaX = -psInfo.width;
	break;
    }
    switch (psInfo.pageAnchor) {
    case TK_ANCHOR_NW:
    case TK_ANCHOR_N:
    case TK_ANCHOR_NE:
	deltaY = - psInfo.height;
	break;
    case TK_ANCHOR_W:
    case TK_ANCHOR_CENTER:
    case TK_ANCHOR_E:
	deltaY = -psInfo.height/2;
	break;
    case TK_ANCHOR_SW:
    case TK_ANCHOR_S:
    case TK_ANCHOR_SE:
	deltaY = 0;
	break;
    }

    if (psInfo.colorMode == NULL) {
	psInfo.colorLevel = 2;
    } else {
	length = strlen(psInfo.colorMode);
	if (strncmp(psInfo.colorMode, "monochrome", length) == 0) {
	    psInfo.colorLevel = 0;
	} else if (strncmp(psInfo.colorMode, "gray", length) == 0) {
	    psInfo.colorLevel = 1;
	} else if (strncmp(psInfo.colorMode, "color", length) == 0) {
	    psInfo.colorLevel = 2;
	} else {
	    Tcl_AppendResult(interp, "bad color mode \"", psInfo.colorMode,
		    "\": must be monochrome, gray, or color", NULL);
	    goto cleanup;
	}
    }

    if (psInfo.fileName != NULL) {
        /*
         * Check that -file and -channel are not both specified.
         */

        if (psInfo.channelName != NULL) {
            Tcl_AppendResult(interp, "can't specify both -file",
                    " and -channel", NULL);
            result = TCL_ERROR;
            goto cleanup;
        }

        /*
         * Check that we are not in a safe interpreter. If we are, disallow
         * the -file specification.
         */

        if (Tcl_IsSafe(interp)) {
            Tcl_AppendResult(interp, "can't specify -file in a",
                    " safe interpreter", NULL);
            result = TCL_ERROR;
            goto cleanup;
        }

	p = Tcl_TranslateFileName(interp, psInfo.fileName, &buffer);
	if (p == NULL) {
	    goto cleanup;
	}
	psInfo.chan = Tcl_OpenFileChannel(interp, p, "w", 0666);
	Tcl_DStringFree(&buffer);
	if (psInfo.chan == NULL) {
	    goto cleanup;
	}
    }

    if (psInfo.channelName != NULL) {
        int mode;

        /*
         * Check that the channel is found in this interpreter and that it is
         * open for writing.
         */

        psInfo.chan = Tcl_GetChannel(interp, psInfo.channelName, &mode);
        if (psInfo.chan == (Tcl_Channel) NULL) {
            result = TCL_ERROR;
            goto cleanup;
        }
        if ((mode & TCL_WRITABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"", psInfo.channelName,
		    "\" wasn't opened for writing", NULL);
            result = TCL_ERROR;
            goto cleanup;
        }
    }

    /*
     * Make a pre-pass over all of the items, generating Postscript and then
     * throwing it away. The purpose of this pass is just to collect
     * information about all the fonts in use, so that we can output font
     * information in the proper form required by the Document Structuring
     * Conventions.
     */

    psInfo.prepass = 1;
    for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	if ((itemPtr->x1 >= psInfo.x2) || (itemPtr->x2 < psInfo.x)
		|| (itemPtr->y1 >= psInfo.y2) || (itemPtr->y2 < psInfo.y)) {
	    continue;
	}
	if (itemPtr->typePtr->postscriptProc == NULL) {
	    continue;
	}
	result = (*itemPtr->typePtr->postscriptProc)(interp,
		(Tk_PathCanvas) canvasPtr, itemPtr, 1);
	Tcl_ResetResult(interp);
	if (result != TCL_OK) {
	    /*
	     * An error just occurred. Just skip out of this loop. There's no
	     * need to report the error now; it can be reported later (errors
	     * can happen later that don't happen now, so we still have to
	     * check for errors later anyway).
	     */
	    break;
	}
    }
    psInfo.prepass = 0;

    /*
     * Generate the header and prolog for the Postscript.
     */

    if (psInfo.prolog) {
	Tcl_AppendResult(interp, "%!PS-Adobe-3.0 EPSF-3.0\n",
		"%%Creator: Tk Canvas Widget\n", NULL);
#ifdef HAVE_PW_GECOS
	if (!Tcl_IsSafe(interp)) {
	    struct passwd *pwPtr = getpwuid(getuid());	/* INTL: Native. */

	    Tcl_AppendResult(interp, "%%For: ",
		    (pwPtr != NULL) ? pwPtr->pw_gecos : "Unknown", "\n", NULL);
	    endpwent();
	}
#endif /* HAVE_PW_GECOS */
	Tcl_AppendResult(interp, "%%Title: Window ", Tk_PathName(tkwin), "\n",
		NULL);
	time(&now);
	Tcl_AppendResult(interp, "%%CreationDate: ",
		ctime(&now), NULL);		/* INTL: Native. */
	if (!psInfo.rotate) {
	    sprintf(string, "%d %d %d %d",
		    (int) (psInfo.pageX + psInfo.scale*deltaX),
		    (int) (psInfo.pageY + psInfo.scale*deltaY),
		    (int) (psInfo.pageX + psInfo.scale*(deltaX + psInfo.width)
			    + 1.0),
		    (int) (psInfo.pageY + psInfo.scale*(deltaY + psInfo.height)
			    + 1.0));
	} else {
	    sprintf(string, "%d %d %d %d",
		    (int) (psInfo.pageX - psInfo.scale*(deltaY+psInfo.height)),
		    (int) (psInfo.pageY + psInfo.scale*deltaX),
		    (int) (psInfo.pageX - psInfo.scale*deltaY + 1.0),
		    (int) (psInfo.pageY + psInfo.scale*(deltaX + psInfo.width)
			    + 1.0));
	}
	Tcl_AppendResult(interp, "%%BoundingBox: ", string, "\n", NULL);
	Tcl_AppendResult(interp, "%%Pages: 1\n",
		"%%DocumentData: Clean7Bit\n", NULL);
	Tcl_AppendResult(interp, "%%Orientation: ",
		psInfo.rotate ? "Landscape\n" : "Portrait\n", NULL);
	p = "%%DocumentNeededResources: font ";
	for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_AppendResult(interp, p,
		    Tcl_GetHashKey(&psInfo.fontTable, hPtr), "\n", NULL);
	    p = "%%+ font ";
	}
	Tcl_AppendResult(interp, "%%EndComments\n\n", NULL);

	/*
	 * Insert the prolog
	 */

	Tcl_AppendResult(interp, Tcl_GetVar(interp,"::tk::ps_preamable",
		TCL_GLOBAL_ONLY), NULL);

	if (psInfo.chan != NULL) {
	    Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	    Tcl_ResetResult(canvasPtr->interp);
	}

	/*
	 * Document setup:  set the color level and include fonts.
	 */

	sprintf(string, "/CL %d def\n", psInfo.colorLevel);
	Tcl_AppendResult(interp, "%%BeginSetup\n", string, NULL);
	for (hPtr = Tcl_FirstHashEntry(&psInfo.fontTable, &search);
		hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
	    Tcl_AppendResult(interp, "%%IncludeResource: font ",
		    Tcl_GetHashKey(&psInfo.fontTable, hPtr), "\n", NULL);
	}
	Tcl_AppendResult(interp, "%%EndSetup\n\n", NULL);

	/*
	 * Page setup: move to page positioning point, rotate if needed, set
	 * scale factor, offset for proper anchor position, and set clip
	 * region.
	 */

	Tcl_AppendResult(interp, "%%Page: 1 1\n", "save\n", NULL);
	sprintf(string, "%.1f %.1f translate\n", psInfo.pageX, psInfo.pageY);
	Tcl_AppendResult(interp, string, NULL);
	if (psInfo.rotate) {
	    Tcl_AppendResult(interp, "90 rotate\n", NULL);
	}
	sprintf(string, "%.4g %.4g scale\n", psInfo.scale, psInfo.scale);
	Tcl_AppendResult(interp, string, NULL);
	sprintf(string, "%d %d translate\n", deltaX - psInfo.x, deltaY);
	Tcl_AppendResult(interp, string, NULL);
	sprintf(string,
		"%d %.15g moveto %d %.15g lineto %d %.15g lineto %d %.15g",
		psInfo.x, Tk_PostscriptY((double)psInfo.y,
			(Tk_PostscriptInfo)psInfoPtr),
		psInfo.x2, Tk_PostscriptY((double)psInfo.y,
			(Tk_PostscriptInfo)psInfoPtr),
		psInfo.x2, Tk_PostscriptY((double)psInfo.y2,
			(Tk_PostscriptInfo)psInfoPtr),
		psInfo.x, Tk_PostscriptY((double)psInfo.y2,
			(Tk_PostscriptInfo)psInfoPtr));
	Tcl_AppendResult(interp, string,
		" lineto closepath clip newpath\n", NULL);
    }
    if (psInfo.chan != NULL) {
	Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	Tcl_ResetResult(canvasPtr->interp);
    }

    /*
     * Iterate through all the items, having each relevant one draw itself.
     * Quit if any of the items returns an error.
     */

    result = TCL_OK;
    for (itemPtr = canvasPtr->rootItemPtr; itemPtr != NULL;
	    itemPtr = TkPathCanvasItemIteratorNext(itemPtr)) {
	if ((itemPtr->x1 >= psInfo.x2) || (itemPtr->x2 < psInfo.x)
		|| (itemPtr->y1 >= psInfo.y2) || (itemPtr->y2 < psInfo.y)) {
	    continue;
	}
	if (itemPtr->typePtr->postscriptProc == NULL) {
	    continue;
	}
	if (itemPtr->state == TK_PATHSTATE_HIDDEN) {
	    continue;
	}
	Tcl_AppendResult(interp, "gsave\n", NULL);
	result = (*itemPtr->typePtr->postscriptProc)(interp,
		(Tk_PathCanvas) canvasPtr, itemPtr, 0);
	if (result != TCL_OK) {
	    char msg[64 + TCL_INTEGER_SPACE];

	    sprintf(msg, "\n    (generating Postscript for item %d)",
		    itemPtr->id);
	    Tcl_AddErrorInfo(interp, msg);
	    goto cleanup;
	}
	Tcl_AppendResult(interp, "grestore\n", NULL);
	if (psInfo.chan != NULL) {
	    Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	    Tcl_ResetResult(interp);
	}
    }

    /*
     * Output page-end information, such as commands to print the page and
     * document trailer stuff.
     */

    if (psInfo.prolog) {
	Tcl_AppendResult(interp, "restore showpage\n\n",
		"%%Trailer\nend\n%%EOF\n", NULL);
    }
    if (psInfo.chan != NULL) {
	Tcl_Write(psInfo.chan, Tcl_GetStringResult(interp), -1);
	Tcl_ResetResult(canvasPtr->interp);
    }

    /*
     * Clean up psInfo to release malloc'ed stuff.
     */

  cleanup:
    if ((psInfo.chan != NULL) && (psInfo.channelName == NULL)) {
	Tcl_Close(interp, psInfo.chan);
    }
    Tcl_DeleteHashTable(&psInfo.fontTable);
    canvasPtr->psInfo = (Tk_PostscriptInfo) oldInfoPtr;
    Tk_FreeConfigOptions((char *) &psInfo, optionTable, tkwin);
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * GetPostscriptPoints --
 *
 *	Given a string, returns the number of Postscript points corresponding
 *	to that string.
 *
 * Results:
 *	The return value is a standard Tcl return result. If TCL_OK is
 *	returned, then everything went well and the screen distance is stored
 *	at *doublePtr; otherwise TCL_ERROR is returned and an error message is
 *	left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetPostscriptPoints(
    Tcl_Interp *interp,		/* Use this for error reporting. */
    char *string,		/* String describing a screen distance. */
    double *doublePtr)		/* Place to store converted result. */
{
    char *end;
    double d;

    d = strtod(string, &end);
    if (end == string) {
	goto error;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) {
	end++;
    }
    switch (*end) {
    case 'c':
	d *= 72.0/2.54;
	end++;
	break;
    case 'i':
	d *= 72.0;
	end++;
	break;
    case 'm':
	d *= 72.0/25.4;
	end++;
	break;
    case 0:
	break;
    case 'p':
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

  error:
    Tcl_AppendResult(interp, "bad distance \"", string, "\"", NULL);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * TkImageGetColor --
 *
 *	This function converts a pixel value to three floating point numbers,
 *	representing the amount of red, green, and blue in that pixel on the
 *	screen. It makes use of colormap data passed as an argument, and
 *	should work for all Visual types.
 *
 *	This implementation is bogus on Windows because the colormap data is
 *	never filled in. Instead all postscript generated data coming through
 *	here is expected to be RGB color data. To handle lower bit-depth
 *	images properly, XQueryColors must be implemented for Windows.
 *
 * Results:
 *	Returns red, green, and blue color values in the range 0 to 1. There
 *	are no error returns.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

#ifdef WIN32
#include <windows.h>

/*
 * We could just define these instead of pulling in windows.h.
 #define GetRValue(rgb)	((BYTE)(rgb))
 #define GetGValue(rgb)	((BYTE)(((WORD)(rgb)) >> 8))
 #define GetBValue(rgb)	((BYTE)((rgb)>>16))
 */

#else /* !WIN32 */

#define GetRValue(rgb)	((rgb & cdata->red_mask) >> cdata->red_shift)
#define GetGValue(rgb)	((rgb & cdata->green_mask) >> cdata->green_shift)
#define GetBValue(rgb)	((rgb & cdata->blue_mask) >> cdata->blue_shift)

#endif /* WIN32 */

#if defined(WIN32) || defined(MAC_OSX_TK)
static void
TkImageGetColor(
    TkColormapData *cdata,	/* Colormap data */
    unsigned long pixel,	/* Pixel value to look up */
    double *red, double *green, double *blue)
				/* Color data to return */
{
    *red   = (double) GetRValue(pixel) / 255.0;
    *green = (double) GetGValue(pixel) / 255.0;
    *blue  = (double) GetBValue(pixel) / 255.0;
}
#else /* ! (WIN32 || MAC_OSX_TK) */
static void
TkImageGetColor(
    TkColormapData *cdata,	/* Colormap data */
    unsigned long pixel,	/* Pixel value to look up */
    double *red, double *green, double *blue)
				/* Color data to return */
{
    if (cdata->separated) {
	int r = GetRValue(pixel);
	int g = GetGValue(pixel);
	int b = GetBValue(pixel);

	*red   = cdata->colors[r].red / 65535.0;
	*green = cdata->colors[g].green / 65535.0;
	*blue  = cdata->colors[b].blue / 65535.0;
    } else {
	*red   = cdata->colors[pixel].red / 65535.0;
	*green = cdata->colors[pixel].green / 65535.0;
	*blue  = cdata->colors[pixel].blue / 65535.0;
    }
}
#endif /* WIN32 || MAC_OSX_TK */

/*
 *--------------------------------------------------------------
 *
 * TkPathPostscriptImage --
 *
 *	This function is called to output the contents of an image in
 *	Postscript, using a format appropriate for the current color mode
 *	(i.e. one bit per pixel in monochrome, one byte per pixel in gray, and
 *	three bytes per pixel in color).
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
TkPathPostscriptImage(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tk_PostscriptInfo psInfo,	/* postscript info */
    XImage *ximage,		/* Image to draw */
    int x, int y,		/* First pixel to output */
    int width, int height)	/* Width and height of area */
{
    TkPostscriptInfo *psInfoPtr = (TkPostscriptInfo *) psInfo;
    char buffer[256];
    int xx, yy, band, maxRows;
    double red, green, blue;
    int bytesPerLine=0, maxWidth=0;
    int level = psInfoPtr->colorLevel;
    Colormap cmap;
    int i, ncolors;
    Visual *visual;
    TkColormapData cdata;

    if (psInfoPtr->prepass) {
	return TCL_OK;
    }

    cmap = Tk_Colormap(tkwin);
    visual = Tk_Visual(tkwin);

    /*
     * Obtain information about the colormap, ie the mapping between pixel
     * values and RGB values. The code below should work for all Visual types.
     */

    ncolors = visual->map_entries;
    cdata.colors = (XColor *) ckalloc(sizeof(XColor) * ncolors);
    cdata.ncolors = ncolors;

#if defined(__cplusplus) || defined(c_plusplus)
    if (visual->c_class == DirectColor || visual->c_class == TrueColor) {
#else
	int class;		/* class of screen (monochrome, etc.) */
    if (visual->class == DirectColor || visual->class == TrueColor) {
#endif
	cdata.separated = 1;
	cdata.red_mask = visual->red_mask;
	cdata.green_mask = visual->green_mask;
	cdata.blue_mask = visual->blue_mask;
	cdata.red_shift = 0;
	cdata.green_shift = 0;
	cdata.blue_shift = 0;

	while ((0x0001 & (cdata.red_mask >> cdata.red_shift)) == 0) {
	    cdata.red_shift ++;
	}
	while ((0x0001 & (cdata.green_mask >> cdata.green_shift)) == 0) {
	    cdata.green_shift ++;
	}
	while ((0x0001 & (cdata.blue_mask >> cdata.blue_shift)) == 0) {
	    cdata.blue_shift ++;
	}

	for (i = 0; i < ncolors; i ++) {
	    cdata.colors[i].pixel =
		    ((i << cdata.red_shift) & cdata.red_mask) |
		    ((i << cdata.green_shift) & cdata.green_mask) |
		    ((i << cdata.blue_shift) & cdata.blue_mask);
	}
    } else {
	cdata.separated=0;
	for (i = 0; i < ncolors; i ++) {
	    cdata.colors[i].pixel = i;
	}
    }

#if defined(__cplusplus) || defined(c_plusplus)
    if (visual->c_class == StaticGray || visual->c_class == GrayScale) {
#else
    if (visual->class == StaticGray || visual->class == GrayScale) {
#endif
	cdata.color = 0;
    } else {
	cdata.color = 1;
    }

    XQueryColors(Tk_Display(tkwin), cmap, cdata.colors, ncolors);

    /*
     * Figure out which color level to use (possibly lower than the one
     * specified by the user). For example, if the user specifies color with
     * monochrome screen, use gray or monochrome mode instead.
     */

    if (!cdata.color && level == 2) {
	level = 1;
    }

    if (!cdata.color && cdata.ncolors == 2) {
	level = 0;
    }

    /*
     * Check that at least one row of the image can be represented with a
     * string less than 64 KB long (this is a limit in the Postscript
     * interpreter).
     */

    switch (level) {
    case 0: bytesPerLine = (width + 7) / 8;  maxWidth = 240000; break;
    case 1: bytesPerLine = width;	     maxWidth = 60000;  break;
    case 2: bytesPerLine = 3 * width;	     maxWidth = 20000;  break;
    }

    if (bytesPerLine > 60000) {
	Tcl_ResetResult(interp);
	sprintf(buffer,
		"Can't generate Postscript for images more than %d pixels wide",
		maxWidth);
	Tcl_AppendResult(interp, buffer, NULL);
	ckfree((char *) cdata.colors);
	return TCL_ERROR;
    }

    maxRows = 60000 / bytesPerLine;

    for (band = height-1; band >= 0; band -= maxRows) {
	int rows = (band >= maxRows) ? maxRows : band + 1;
	int lineLen = 0;

	switch (level) {
	case 0:
	    sprintf(buffer, "%d %d 1 matrix {\n<", width, rows);
	    Tcl_AppendResult(interp, buffer, NULL);
	    break;
	case 1:
	    sprintf(buffer, "%d %d 8 matrix {\n<", width, rows);
	    Tcl_AppendResult(interp, buffer, NULL);
	    break;
	case 2:
	    sprintf(buffer, "%d %d 8 matrix {\n<", width, rows);
	    Tcl_AppendResult(interp, buffer, NULL);
	    break;
	}
	for (yy = band; yy > band - rows; yy--) {
	    switch (level) {
	    case 0: {
		/*
		 * Generate data for image in monochrome mode. No attempt at
		 * dithering is made--instead, just set a threshold.
		 */

		unsigned char mask = 0x80;
		unsigned char data = 0x00;

		for (xx = x; xx< x+width; xx++) {
		    TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
			    &red, &green, &blue);
		    if (0.30 * red + 0.59 * green + 0.11 * blue > 0.5) {
			data |= mask;
		    }
		    mask >>= 1;
		    if (mask == 0) {
			sprintf(buffer, "%02X", data);
			Tcl_AppendResult(interp, buffer, NULL);
			lineLen += 2;
			if (lineLen > 60) {
			    lineLen = 0;
			    Tcl_AppendResult(interp, "\n", NULL);
			}
			mask=0x80;
			data=0x00;
		    }
		}
		if ((width % 8) != 0) {
		    sprintf(buffer, "%02X", data);
		    Tcl_AppendResult(interp, buffer, NULL);
		    mask=0x80;
		    data=0x00;
		}
		break;
	    }
	    case 1:
		/*
		 * Generate data in gray mode; in this case, take a weighted
		 * sum of the red, green, and blue values.
		 */

		for (xx = x; xx < x+width; xx ++) {
		    TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
			    &red, &green, &blue);
		    sprintf(buffer, "%02X", (int) floor(0.5 + 255.0 *
			    (0.30 * red + 0.59 * green + 0.11 * blue)));
		    Tcl_AppendResult(interp, buffer, NULL);
		    lineLen += 2;
		    if (lineLen > 60) {
			lineLen = 0;
			Tcl_AppendResult(interp, "\n", NULL);
		    }
		}
		break;
	    case 2:
		/*
		 * Finally, color mode. Here, just output the red, green, and
		 * blue values directly.
		 */

		for (xx = x; xx < x+width; xx++) {
		    TkImageGetColor(&cdata, XGetPixel(ximage, xx, yy),
			    &red, &green, &blue);
		    sprintf(buffer, "%02X%02X%02X",
			    (int) floor(0.5 + 255.0 * red),
			    (int) floor(0.5 + 255.0 * green),
			    (int) floor(0.5 + 255.0 * blue));
		    Tcl_AppendResult(interp, buffer, NULL);
		    lineLen += 6;
		    if (lineLen > 60) {
			lineLen = 0;
			Tcl_AppendResult(interp, "\n", NULL);
		    }
		}
		break;
	    }
	}
	switch (level) {
	case 0: case 1:
	    sprintf(buffer, ">\n} image\n"); break;
	case 2:
	    sprintf(buffer, ">\n} false 3 colorimage\n"); break;
	}
	Tcl_AppendResult(interp, buffer, NULL);
	sprintf(buffer, "0 %d translate\n", rows);
	Tcl_AppendResult(interp, buffer, NULL);
    }
    ckfree((char *) cdata.colors);
    return TCL_OK;
}





