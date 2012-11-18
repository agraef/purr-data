%module tclpd
%include exception.i
%include cpointer.i

#define __attribute__(x)

/* functions that are in m_pd.h but don't exist in modern versions of pd */
%ignore pd_getfilename;
%ignore pd_getdirname;
%ignore pd_anything;
%ignore class_parentwidget;
%ignore sys_isreadablefile;
%ignore garray_get;
%ignore c_extern;
%ignore c_addmess;

/* functions that we can't auto-wrap, because they have varargs */
%ignore post;
%ignore class_new;

/* functions that we can't auto-wrap, because <insert reason here> */
%ignore glist_new;
%ignore canvas_zapallfortemplate;
%ignore canvas_fattenforscalars;
%ignore canvas_visforscalars;
%ignore canvas_clicksub;
%ignore text_xcoord;
%ignore text_ycoord;
%ignore canvas_getglistonsuper;
%ignore canvas_getfont;
%ignore canvas_setusedastemplate;
%ignore canvas_vistext;
%ignore rtext_remove;
%ignore canvas_recurapply;
%ignore gobj_properties;

/* end of ignore-list */

%include "m_pd.h"
%include "g_canvas.h"
%include "tcl_extras.h"

%{
    #include "m_pd.h"
    #include "tcl_extras.h"

    typedef t_atom t_atom_array;

    /* extern "C" SWIGEXPORT int Tclpd_SafeInit(Tcl_Interp *interp); */
    /* extern "C" { void tcl_setup() {tclpd_setup(void);} } */
%}

/* this does the trick of solving
 TypeError in method 'xyz', argument 4 of type 't_atom *' */
%name(outlet_list) EXTERN void outlet_list(t_outlet *x, t_symbol *s, int argc, t_atom_array *argv);
%name(outlet_anything) EXTERN void outlet_anything(t_outlet *x, t_symbol *s, int argc, t_atom_array *argv);

%pointer_class(t_float, t_float)
%pointer_class(t_symbol, t_symbol)

%typemap(in) t_atom * {
    t_atom *a = (t_atom*)getbytes(sizeof(t_atom));
    if(tcl_to_pd($input, a) == TCL_ERROR) {
#ifdef DEBUG
        post("Tcl SWIG: typemap(in) error");
#endif
        return TCL_ERROR;
    }
    $1 = a;
}

%typemap(freearg) t_atom * {
    freebytes($1, sizeof(t_atom));
}

%typemap(out) t_atom* {
    Tcl_Obj* res_obj;
    if(pd_to_tcl($1, &res_obj) == TCL_ERROR) {
#ifdef DEBUG
        post("Tcl SWIG: typemap(out) error");
#endif
        return TCL_ERROR;
    }
    Tcl_SetObjResult(tcl_for_pd, res_obj);
}

/* helper functions for t_atom arrays */
%inline %{
    t_atom_array *new_atom_array(int size) {
        return (t_atom_array*)getbytes(size*sizeof(t_atom));
    }

    void delete_atom_array(t_atom_array *a, int size) {
        freebytes(a, size*sizeof(t_atom));
    }

    t_atom* get_atom_array(t_atom_array *a, int index) {
        return &a[index];
    }

    void set_atom_array(t_atom_array *a, int index, t_atom *n) {
        memcpy(&a[index], n, sizeof(t_atom));
    }
%}


