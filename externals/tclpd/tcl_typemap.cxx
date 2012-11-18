#include "tcl_extras.h"
#include <string.h>

int tcl_to_pd(Tcl_Obj *input, t_atom *output) {
    int llength;
    if(Tcl_ListObjLength(tcl_for_pd, input, &llength) == TCL_ERROR)
        return TCL_ERROR;
    if(llength != 2)
        /*SWIG_exception(SWIG_ValueError, "Bad t_atom: expeting a 2-elements list.");*/
        return TCL_ERROR;

    int i;
    Tcl_Obj* obj[2];
    for(i = 0; i < 2; i++) Tcl_ListObjIndex(tcl_for_pd, input, i, &obj[i]);
    char* argv0 = Tcl_GetStringFromObj(obj[0], 0);

    if(strcmp(argv0, "float") == 0) {
        double dbl;
        if(Tcl_GetDoubleFromObj(tcl_for_pd, obj[1], &dbl) == TCL_ERROR)
            return TCL_ERROR;
        SETFLOAT(output, dbl);
    } else if(strcmp(argv0, "symbol") == 0) {
        SETSYMBOL(output, gensym(Tcl_GetStringFromObj(obj[1], 0)));
    } else if(strcmp(argv0, "pointer") == 0) {
        // TODO:
        return TCL_ERROR;
    }
    return TCL_OK;
}

int pd_to_tcl(t_atom *input, Tcl_Obj **output) {
    Tcl_Obj* tcl_t_atom[2];
#ifdef DEBUG
    post("pd_to_tcl: atom type = %d (%s)",
        input->a_type, input->a_type == A_FLOAT ? "A_FLOAT" :
        input->a_type == A_SYMBOL ? "A_SYMBOL" :
        input->a_type == A_POINTER ? "A_POINTER" : "?");
#endif
    switch (input->a_type) {
        case A_FLOAT: {
            tcl_t_atom[0] = Tcl_NewStringObj("float", -1);
            tcl_t_atom[1] = Tcl_NewDoubleObj(input->a_w.w_float);
            break;
        }
        case A_SYMBOL: {
            tcl_t_atom[0] = Tcl_NewStringObj("symbol", -1);
            tcl_t_atom[1] = Tcl_NewStringObj(input->a_w.w_symbol->s_name, strlen(input->a_w.w_symbol->s_name));
            break;
        }
        case A_POINTER: {
            return TCL_ERROR;
            tcl_t_atom[0] = Tcl_NewStringObj("pointer", -1);
            tcl_t_atom[1] = Tcl_NewDoubleObj((long)input->a_w.w_gpointer);
            break;
        }
        default: {
            tcl_t_atom[0] = Tcl_NewStringObj("?", -1);
            tcl_t_atom[1] = Tcl_NewStringObj("", 0);
            break;
        }
    }
#ifdef DEBUG
    post("pd_to_tcl: atom value = \"%s\"", Tcl_GetStringFromObj(tcl_t_atom[1], 0));
#endif
    *output = Tcl_NewListObj(2, &tcl_t_atom[0]);
    Tcl_IncrRefCount(*output);
    return TCL_OK;
}
