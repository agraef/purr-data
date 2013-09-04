/*
 * path.c --
 *
 *	This file is main for the tkpath package.
 *  SVG counterpart. See http://www.w3.org/TR/SVG11/.
 *
 * Copyright (c) 2005-2008  Mats Bengtsson
 *
 * $Id$
 */

#include "tkp.h"
#include "tkIntPath.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* Keep patch level release numbers odd and set even only on release. */
#define TKPATH_VERSION    "0.3"
#define TKPATH_PATCHLEVEL "0.3.2"
#define TKPATH_REQUIRE    "8.5"

int gAntiAlias = 1;
int gSurfaceCopyPremultiplyAlpha = 1;
int gDepixelize = 1;
Tcl_Interp *gInterp = NULL;

extern int 	PixelAlignObjCmd(ClientData clientData, Tcl_Interp* interp,
                    int objc, Tcl_Obj* CONST objv[]);
extern int	SurfaceInit(Tcl_Interp *interp);


#ifdef _WIN32
    BOOL APIENTRY
    DllMain( HINSTANCE hInst,   /* Library instance handle. */
             DWORD     reason,  /* Reason this function is being called. */
             LPVOID    reserved /* Not used. */ )
    {
        return TRUE;
    }
#endif


/*
 *----------------------------------------------------------------------
 *
 * Tkpath_Init --
 *
 *		Initializer for the tkpath package.
 *
 * Results:
 *		A standard Tcl result.
 *
 * Side Effects:
 *   	Tcl commands created
 *
 *----------------------------------------------------------------------
 */
#ifdef __cplusplus
extern "C" {
#endif
#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
    __declspec(dllexport)
#endif

int Tkpath_Init(Tcl_Interp *interp)		/* Tcl interpreter. */
{
        
#if defined(USE_TCL_STUBS)
    if (Tcl_InitStubs(interp, TKPATH_REQUIRE, 0) == NULL) {
	return TCL_ERROR;
    }
#endif
    if (Tcl_PkgRequire(interp, "Tcl", TKPATH_REQUIRE, 0) == NULL) {
	return TCL_ERROR;
    }
#if defined(USE_TK_STUBS)
    if (Tk_InitStubs(interp, TKPATH_REQUIRE, 0) == NULL) {
	return TCL_ERROR;
    }
#endif
    if (Tcl_PkgRequire(interp, "Tk", TKPATH_REQUIRE, 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_CreateNamespace(interp, "::tkp", NULL, NULL) == NULL) {
	Tcl_ResetResult(interp);
    }
    Tcl_CreateObjCommand(interp, "::tkp::canvas", Tk_PathCanvasObjCmd,
	    (ClientData) Tk_MainWindow(interp), NULL);

    gInterp = interp;

    /*
     * Link the ::tkp::antialias variable to control antialiasing. 
     */
    if (Tcl_LinkVar(interp, "::tkp::antialias",
            (char *) &gAntiAlias, TCL_LINK_BOOLEAN) != TCL_OK) {
        Tcl_ResetResult(interp);
    }
    
    /*
     * With gSurfaceCopyPremultiplyAlpha true we ignore the "premultiply alpha"
     * and use RGB as is. Else we need to divide each RGB with alpha
     * to get "true" values.
     */
    if (Tcl_LinkVar(interp, "::tkp::premultiplyalpha",
            (char *) &gSurfaceCopyPremultiplyAlpha, TCL_LINK_BOOLEAN) != TCL_OK) {
        Tcl_ResetResult(interp);
    }    
    if (Tcl_LinkVar(interp, "::tkp::depixelize",
            (char *) &gDepixelize, TCL_LINK_BOOLEAN) != TCL_OK) {
        Tcl_ResetResult(interp);
    }    
    Tcl_CreateObjCommand(interp, "::tkp::pixelalign",
            PixelAlignObjCmd, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    /*
     * Make separate gradient objects, similar to SVG.
     */
    PathGradientInit(interp);
    SurfaceInit(interp);

    /*
     * Style object.
     */
    PathStyleInit(interp);

    return Tcl_PkgProvide(interp, "tkpath", TKPATH_PATCHLEVEL);
}

/*
 *----------------------------------------------------------------------
 *
 * Tkpath_SafeInit --
 *
 *		This is just to provide a "safe" entry point (that is not safe!).
 *
 * Results:
 *		A standard Tcl result.
 *
 * Side Effects:
 *   	Tcl commands created
 *
 *----------------------------------------------------------------------
 */
#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
    __declspec(dllexport)
#endif

int Tkpath_SafeInit(Tcl_Interp *interp)	    { return Tkpath_Init(interp); }
int Tkpath_Unload(Tcl_Interp *interp)	    { return TCL_ERROR; }
int Tkpath_SafeUnload(Tcl_Interp *interp)   { return Tkpath_Unload( interp ); }
#ifdef __cplusplus
}
#endif
/*--------------------------------------------------------------------------------*/
