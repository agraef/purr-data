#include "m_pd.h"
//#include "m_imp.h"
#include "g_canvas.h"
//#include "s_stuff.h"

#include <tcl.h>

/* PATH_MAX is not defined in limits.h on some platforms */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define TCLPD_VERSION "0.2.1"

#define InitArray(name, size, value) for(int zz=0; zz<(size); zz++) name[zz]=value

typedef struct _t_tcl {
    t_object o;
    t_glist* x_glist;
    Tcl_Obj* self;
    Tcl_Obj* classname;
    int ninlets;
} t_tcl;

typedef struct _t_proxyinlet {
    t_object obj;
    t_tcl* target;
    int ninlet;
    t_symbol* sel;
    int argc;
    t_atom* argv;
} t_proxyinlet;

/* tcl_proxyinlet.cxx */
extern t_class* proxyinlet_class;
void proxyinlet_init(t_proxyinlet* x);
void proxyinlet_clear(t_proxyinlet* x);
void proxyinlet_anything(t_proxyinlet* x, t_symbol* s, int argc, t_atom* argv);
void proxyinlet_trigger(t_proxyinlet* x);
t_atom* proxyinlet_get_atoms(t_proxyinlet* x);
void proxyinlet_clone(t_proxyinlet* x, t_proxyinlet* y);
void proxyinlet_setup(void);

/* tcl_wrap.cxx */
extern "C" int Tclpd_SafeInit(Tcl_Interp *interp);

/* tcl_typemap.cxx */
int pd_to_tcl(t_atom* input, Tcl_Obj** output);
int tcl_to_pd(Tcl_Obj* input, t_atom* output);

/* tcl_setup.cxx */
extern Tcl_Interp* tcl_for_pd;
extern "C" void tclpd_setup(void);
void tclpd_interp_error(int result);

/* tcl_class.cxx */
t_class* tclpd_class_new(const char* name, int flags);
t_class* tclpd_guiclass_new(const char* name, int flags);
t_tcl* tclpd_new(t_symbol* classsym, int ac, t_atom* at);
void tclpd_free (t_tcl* self);
void tclpd_anything(t_tcl* self, t_symbol* s, int ac, t_atom* at);
void tclpd_inlet_anything(t_tcl *self, int inlet, t_symbol *s, int ac, t_atom *at);
void tclpd_loadbang(t_tcl* x);
t_proxyinlet* tclpd_add_proxyinlet(t_tcl* x);
t_tcl* tclpd_get_instance(const char* objectSequentialId);
t_pd* tclpd_get_instance_pd(const char* objectSequentialId);
t_object* tclpd_get_object(const char* objectSequentialId);
t_pd* tclpd_get_object_pd(const char* objectSequentialId);
t_glist* tclpd_get_glist(const char* objectSequentialId);
t_symbol* null_symbol();
void poststring2(const char* s);
extern "C" void text_save(t_gobj *z, t_binbuf *b);
void tclpd_save(t_gobj* z, t_binbuf* b);
void tclpd_properties(t_gobj* z, t_glist* owner);

/* tcl_widgetbehavior.cxx */
void tclpd_guiclass_getrect(t_gobj* z, t_glist* owner, int* xp1, int* yp1, int* xp2, int* yp2);
void tclpd_guiclass_displace(t_gobj* z, t_glist* glist, int dx, int dy);
void tclpd_guiclass_select(t_gobj* z, t_glist* glist, int selected);
void tclpd_guiclass_activate(t_gobj* z, t_glist* glist, int state);
void tclpd_guiclass_delete(t_gobj* z, t_glist* glist);
void tclpd_guiclass_vis(t_gobj* z, t_glist* glist, int vis);
int tclpd_guiclass_click(t_gobj* z, t_glist* glist, int xpix, int ypix, int shift, int alt, int dbl, int doit);
void tclpd_guiclass_motion(t_tcl* x, t_floatarg dx, t_floatarg dy);
void tclpd_guiclass_grab(t_tcl* x, t_glist* glist, int xpix, int ypix);

/* tcl_loader.cxx */
extern "C" int tclpd_do_load_lib(t_canvas* canvas, char* objectname);
/* pd loader private stuff: */
typedef int (*loader_t)(t_canvas *canvas, char* classname);
extern "C" void sys_register_loader(loader_t loader);
extern "C" int sys_onloadlist(char* classname);
extern "C" void sys_putonloadlist(char* classname);
extern "C" void class_set_extern_dir(t_symbol* s);
