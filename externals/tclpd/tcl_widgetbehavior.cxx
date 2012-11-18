#include "tcl_extras.h"
#include <string.h>

void tclpd_guiclass_motion(t_tcl* x, t_floatarg dx, t_floatarg dy) {
    Tcl_Obj* av[5]; InitArray(av, 5, NULL);
    int tmp[4], i, length;
    av[0] = x->self;
    Tcl_IncrRefCount(av[0]);
    av[1] = Tcl_NewStringObj("widgetbehavior", -1);
    Tcl_IncrRefCount(av[1]);
    av[2] = Tcl_NewStringObj("motion", -1);
    Tcl_IncrRefCount(av[2]);
    av[3] = Tcl_NewDoubleObj(dx);
    Tcl_IncrRefCount(av[3]);
    av[4] = Tcl_NewDoubleObj(dy);
    Tcl_IncrRefCount(av[4]);
    int result = Tcl_EvalObjv(tcl_for_pd, 5, av, 0);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    goto cleanup;
error:
cleanup:
    Tcl_DecrRefCount(av[0]);
    Tcl_DecrRefCount(av[1]);
    Tcl_DecrRefCount(av[2]);
    Tcl_DecrRefCount(av[3]);
    Tcl_DecrRefCount(av[4]);
}

void tclpd_guiclass_grab(t_tcl* x, t_glist* glist, int xpix, int ypix) {
    glist_grab(glist, &x->o.te_g, (t_glistmotionfn)tclpd_guiclass_motion, 0, \
        (t_floatarg)xpix, (t_floatarg)ypix);
}

int tclpd_guiclass_click(t_gobj* z, t_glist* glist, int xpix, int ypix, int shift, int alt, int dbl, int doit) {
    Tcl_Obj* av[9]; InitArray(av, 9, NULL);
    Tcl_Obj* o = NULL;
    int i = 0;
    t_tcl* x = (t_tcl*)z;
    av[0] = x->self;
    Tcl_IncrRefCount(av[0]);
    av[1] = Tcl_NewStringObj("widgetbehavior", -1);
    Tcl_IncrRefCount(av[1]);
    av[2] = Tcl_NewStringObj("click", -1);
    Tcl_IncrRefCount(av[2]);
    av[3] = Tcl_NewIntObj(xpix);
    Tcl_IncrRefCount(av[3]);
    av[4] = Tcl_NewIntObj(ypix);
    Tcl_IncrRefCount(av[4]);
    av[5] = Tcl_NewIntObj(shift);
    Tcl_IncrRefCount(av[5]);
    av[6] = Tcl_NewIntObj(alt);
    Tcl_IncrRefCount(av[6]);
    av[7] = Tcl_NewIntObj(dbl);
    Tcl_IncrRefCount(av[7]);
    av[8] = Tcl_NewIntObj(doit);
    Tcl_IncrRefCount(av[8]);
    int result = Tcl_EvalObjv(tcl_for_pd, 9, av, 0);
    if(result != TCL_OK) {
        goto error;
    }
    o = Tcl_GetObjResult(tcl_for_pd);
    Tcl_IncrRefCount(o);
    if(strlen(Tcl_GetStringFromObj(o, NULL)) > 0) {
        result = Tcl_GetIntFromObj(tcl_for_pd, o, &i);
        if(result != TCL_OK) {
            goto error;
        }
    }
    goto cleanup;

error:
    tclpd_interp_error(result);

cleanup:
    if(o) Tcl_DecrRefCount(o);
    Tcl_DecrRefCount(av[0]);
    Tcl_DecrRefCount(av[1]);
    Tcl_DecrRefCount(av[2]);
    Tcl_DecrRefCount(av[3]);
    Tcl_DecrRefCount(av[4]);
    Tcl_DecrRefCount(av[5]);
    Tcl_DecrRefCount(av[6]);
    Tcl_DecrRefCount(av[7]);
    Tcl_DecrRefCount(av[8]);

    // return value (BOOL) means 'object wants to be clicked' (g_editor.c:1270)
    return i;
}

void tclpd_guiclass_getrect(t_gobj* z, t_glist* owner, int* xp1, int* yp1, int* xp2, int* yp2) {
    Tcl_Obj* av[5]; InitArray(av, 5, NULL);
    Tcl_Obj* o;
    Tcl_Obj* theList = NULL;
    int tmp[4], i, length;
    t_tcl* x = (t_tcl*)z;
    av[0] = x->self;
    Tcl_IncrRefCount(av[0]);
    av[1] = Tcl_NewStringObj("widgetbehavior", -1);
    Tcl_IncrRefCount(av[1]);
    av[2] = Tcl_NewStringObj("getrect", -1);
    Tcl_IncrRefCount(av[2]);
    av[3] = Tcl_NewIntObj(text_xpix(&x->o, owner));
    Tcl_IncrRefCount(av[3]);
    av[4] = Tcl_NewIntObj(text_ypix(&x->o, owner));
    Tcl_IncrRefCount(av[4]);
    int result = Tcl_EvalObjv(tcl_for_pd, 5, av, 0);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    theList = Tcl_GetObjResult(tcl_for_pd);
    Tcl_IncrRefCount(theList);
    length = 0;
    //result = Tcl_ListObjGetElements(tcl_for_pd, theList, @, @);
    result = Tcl_ListObjLength(tcl_for_pd, theList, &length);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    if(length != 4) {
        pd_error(x, "widgetbehavior getrect: must return a list of 4 integers");
        goto error;
    }
    o = NULL;
    for(i = 0; i < 4; i++) {
        result = Tcl_ListObjIndex(tcl_for_pd, theList, i, &o);
        if(result != TCL_OK) {
            tclpd_interp_error(result);
            goto error;
        }
        result = Tcl_GetIntFromObj(tcl_for_pd, o, &tmp[i]);
        if(result != TCL_OK) {
            tclpd_interp_error(result);
            goto error;
        }
    }
    *xp1 = tmp[0]; *yp1 = tmp[1]; *xp2 = tmp[2]; *yp2 = tmp[3];
    goto cleanup;
error:
cleanup:
    if(theList) Tcl_DecrRefCount(theList);
    Tcl_DecrRefCount(av[0]);
    Tcl_DecrRefCount(av[1]);
    Tcl_DecrRefCount(av[2]);
    Tcl_DecrRefCount(av[3]);
    Tcl_DecrRefCount(av[4]);
}

void tclpd_guiclass_displace(t_gobj* z, t_glist* glist, int dx, int dy) {
    Tcl_Obj* av[5]; InitArray(av, 5, NULL);
    Tcl_Obj* theList = NULL;
    Tcl_Obj* o;
    int length, i, tmp[2];
    t_tcl* x = (t_tcl*)z;
    av[0] = x->self;
    Tcl_IncrRefCount(av[0]);
    av[1] = Tcl_NewStringObj("widgetbehavior", -1);
    Tcl_IncrRefCount(av[1]);
    av[2] = Tcl_NewStringObj("displace", -1);
    Tcl_IncrRefCount(av[2]);
    av[3] = Tcl_NewIntObj(dx);
    Tcl_IncrRefCount(av[3]);
    av[4] = Tcl_NewIntObj(dy);
    Tcl_IncrRefCount(av[4]);
    int result = Tcl_EvalObjv(tcl_for_pd, 5, av, 0);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    theList = Tcl_GetObjResult(tcl_for_pd);
    Tcl_IncrRefCount(theList);
    length = 0;
    //result = Tcl_ListObjGetElements(tcl_for_pd, theList, @, @);
    result = Tcl_ListObjLength(tcl_for_pd, theList, &length);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    if(length != 2) {
        pd_error(x, "widgetbehavior displace: must return a list of 2 integers");
        goto error;
    }
    o = NULL;
    for(i = 0; i < 2; i++) {
        result = Tcl_ListObjIndex(tcl_for_pd, theList, i, &o);
        if(result != TCL_OK) {
            tclpd_interp_error(result);
            goto error;
        }
        result = Tcl_GetIntFromObj(tcl_for_pd, o, &tmp[i]);
        if(result != TCL_OK) {
            tclpd_interp_error(result);
            goto error;
        }
    }
    x->o.te_xpix = tmp[0];
    x->o.te_ypix = tmp[1];
    canvas_fixlinesfor(glist_getcanvas(glist), (t_text*)x);
    goto cleanup;
error:
cleanup:
    if(theList) Tcl_DecrRefCount(theList);
    Tcl_DecrRefCount(av[0]);
    Tcl_DecrRefCount(av[1]);
    Tcl_DecrRefCount(av[2]);
    Tcl_DecrRefCount(av[3]);
    Tcl_DecrRefCount(av[4]);
}

void tclpd_guiclass_select(t_gobj* z, t_glist* glist, int selected) {
    Tcl_Obj* av[4]; InitArray(av, 4, NULL);
    t_tcl* x = (t_tcl*)z;
    av[0] = x->self;
    Tcl_IncrRefCount(av[0]);
    av[1] = Tcl_NewStringObj("widgetbehavior", -1);
    Tcl_IncrRefCount(av[1]);
    av[2] = Tcl_NewStringObj("select", -1);
    Tcl_IncrRefCount(av[2]);
    av[3] = Tcl_NewIntObj(selected);
    Tcl_IncrRefCount(av[3]);
    int result = Tcl_EvalObjv(tcl_for_pd, 4, av, 0);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    goto cleanup;
error:
cleanup:
    Tcl_DecrRefCount(av[0]);
    Tcl_DecrRefCount(av[1]);
    Tcl_DecrRefCount(av[2]);
    Tcl_DecrRefCount(av[3]);
}

void tclpd_guiclass_activate(t_gobj* z, t_glist* glist, int state) {
    Tcl_Obj* av[4]; InitArray(av, 4, NULL);
    t_tcl* x = (t_tcl*)z;
    av[0] = x->self;
    Tcl_IncrRefCount(av[0]);
    av[1] = Tcl_NewStringObj("widgetbehavior", -1);
    Tcl_IncrRefCount(av[1]);
    av[2] = Tcl_NewStringObj("activate", -1);
    Tcl_IncrRefCount(av[2]);
    av[3] = Tcl_NewIntObj(state);
    Tcl_IncrRefCount(av[3]);
    int result = Tcl_EvalObjv(tcl_for_pd, 4, av, 0);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    goto cleanup;
error:
cleanup:
    Tcl_DecrRefCount(av[0]);
    Tcl_DecrRefCount(av[1]);
    Tcl_DecrRefCount(av[2]);
    Tcl_DecrRefCount(av[3]);
}

void tclpd_guiclass_delete(t_gobj* z, t_glist* glist) {
    /* will this be ever need to be accessed in Tcl land? */
    canvas_deletelinesfor(glist_getcanvas(glist), (t_text*)z);
}

void tclpd_guiclass_vis(t_gobj* z, t_glist* glist, int vis) {
    Tcl_Obj* av[7]; InitArray(av, 7, NULL);
    t_tcl* x = (t_tcl*)z;
    av[0] = x->self;
    Tcl_IncrRefCount(av[0]);
    av[1] = Tcl_NewStringObj("widgetbehavior", -1);
    Tcl_IncrRefCount(av[1]);
    av[2] = Tcl_NewStringObj("vis", -1);
    Tcl_IncrRefCount(av[2]);
    char buf[32];
    snprintf(buf, 32, ".x%lx.c", glist_getcanvas(glist));
    av[3] = Tcl_NewStringObj(buf, -1);
    Tcl_IncrRefCount(av[3]);
    av[4] = Tcl_NewIntObj(text_xpix(&x->o, glist));
    Tcl_IncrRefCount(av[4]);
    av[5] = Tcl_NewIntObj(text_ypix(&x->o, glist));
    Tcl_IncrRefCount(av[5]);
    av[6] = Tcl_NewIntObj(vis);
    Tcl_IncrRefCount(av[6]);
    int result = Tcl_EvalObjv(tcl_for_pd, 7, av, 0);
    if(result != TCL_OK) {
        tclpd_interp_error(result);
        goto error;
    }
    goto cleanup;
error:
cleanup:
    Tcl_DecrRefCount(av[0]);
    Tcl_DecrRefCount(av[1]);
    Tcl_DecrRefCount(av[2]);
    Tcl_DecrRefCount(av[3]);
    Tcl_DecrRefCount(av[4]);
    Tcl_DecrRefCount(av[5]);
    Tcl_DecrRefCount(av[6]);
}
