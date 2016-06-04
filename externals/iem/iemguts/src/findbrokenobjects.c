
/******************************************************
 *
 * findbrokenobjects - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2007:forum::für::umläute:2007
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/*
 * find broken objects (objects that could not be created)
 * these objects are of class 'text_class'

 objsrc = pd_checkobject(&src->g_pd);
 if (objsrc && pd_class(&src->g_pd) == text_class && objsrc->te_type == T_OBJECT) {
   // 'src' is a broken object
 }

TODO:
 - output more data: canvas and object-ID
 */

#include "iemguts.h"
#include "g_canvas.h"
#include "m_imp.h"

#include <string.h>

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- findbrokenobjects ---------------------------- */

static t_class *findbrokenobjects_class;

typedef struct _findbrokenobjects
{
  t_object  x_obj;
  t_outlet *x_out;
  t_canvas *x_parent;   // the canvas we are acting on

  int       x_verbose;
} t_findbrokenobjects;

extern t_class *text_class;
extern t_class *canvas_class;

static void print_obj(const char*prefix, t_object *obj) {
  int ntxt;
  char *txt;
  t_binbuf*bb=obj->te_binbuf;
  binbuf_gettext(bb, &txt, &ntxt);
  pd_error(obj, "%s%p\t%s", prefix, obj, txt);
}

static void findbrokenobjects_doit(t_canvas*cnv) {
  t_gobj*src;
  for (src = cnv->gl_list; src; src = src->g_next) { /* traverse all objects in canvas */
    t_object*obj=pd_checkobject(&src->g_pd);
    if (obj && (pd_class(&src->g_pd) == text_class && obj->te_type == T_OBJECT)) {
      // found broken object
      print_obj("broken:\t", obj);
    }
  }
}

static int fbo_has_ctor(t_atom*ab) {
  t_symbol*s=(ab&&pd_objectmaker)?atom_getsymbol(ab):0;
  if(!s)
    return 0;
  if(zgetfn(&pd_objectmaker, s))
    return 1;
  return 0;
}

static void fbo_iterate(t_findbrokenobjects*x, t_canvas*cnv, int verbose) {
  // iterate over all top-level canvases
  if(!(cnv && cnv->gl_name && cnv->gl_name->s_name))
    return;
  t_gobj*g=0;
  int count=0;

  for(g=cnv->gl_list;g;g=g->g_next) {
    // iterate over all objects on the canvas
    t_object*ob=pd_checkobject(&g->g_pd);
    t_class*cls=0;
    count++;

    if(!(ob && ob->te_type == T_OBJECT))
      continue;

    cls=pd_class(&g->g_pd);
    if (cls == canvas_class) {
      // this is just another abstraction, recurse into it
      fbo_iterate(x, ob, verbose);
    } else if (cls == text_class) {
      t_binbuf*bb=ob->te_binbuf;
      t_atom*argv=binbuf_getvec(bb);
      int argc=binbuf_getnatom(bb);
      /* broken object */
      if(verbose) {
	int ntxt;
	char *txt;
	binbuf_gettext(bb, &txt, &ntxt);
	pd_error(ob, "[%s] broken object!", txt);
	freebytes(txt, ntxt);
      }
      //post("%s %d", cnv->gl_name->s_name, count);
      if(argc && fbo_has_ctor(argv)) {
	outlet_anything(x->x_out, gensym("not-created"), argc, argv);
      } else {
	outlet_anything(x->x_out, gensym("not-found"), argc, argv);
      }
    }
  }
}

static void findbrokenobjects_iterate(t_findbrokenobjects *x, int verbose) {
  // find all broken objects in the current patch
  if(x->x_parent) {
    fbo_iterate(x, x->x_parent, verbose);
  } else {
    t_canvas *c;
    for (c = pd_getcanvaslist(); c; c = c->gl_next) {
      const char*name=c->gl_name->s_name;
      /* only allow names ending with '.pd'
       * (reject template canvases)
       */
      int len=strlen(name);
      const char*exclude_name;
      int exclude=1;
      for(exclude_name="etalpmet_"; *exclude_name && len; len--, exclude_name++) {
	if(*exclude_name != name[len-1]) {
	  exclude=0;
	  break;
	}
      }
      if(!exclude){
	fbo_iterate(x, c, verbose);
      }// else post("canvas: %s", name);
    }
  }
}

static void findbrokenobjects_bang(t_findbrokenobjects *x) {
  findbrokenobjects_iterate(x, x->x_verbose);
}
static void findbrokenobjects_verbose(t_findbrokenobjects *x, t_float f) {
  x->x_verbose=(int)f;
}

static void findbrokenobjects_free(t_findbrokenobjects *x)
{
  outlet_free(x->x_out);
}

static void *findbrokenobjects_new(t_symbol*s, int argc, t_atom*argv)
{
  t_findbrokenobjects *x = (t_findbrokenobjects *)pd_new(findbrokenobjects_class);
  x->x_parent=0;
  if(argc==1 && argv->a_type == A_FLOAT) {
    int depth=atom_getint(argv);
    t_glist *glist=(t_glist *)canvas_getcurrent();
    if(depth>=0) {
      t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
      while(depth && canvas) {
	canvas=canvas->gl_owner;
	depth--;
      }
      if(canvas)
	x->x_parent = canvas;
    }
  }
  x->x_verbose=0;

  x->x_out = outlet_new(&x->x_obj, 0);
  return (x);
}

static char fbo_file[];
static void fbo_persist(void) {
  static t_pd*fbo_canvas=0;
  if(fbo_canvas)
    return;


  t_binbuf *b = binbuf_new();
  glob_setfilename(0, gensym("_deken_workspace"), gensym("."));
  binbuf_text(b, fbo_file, strlen(fbo_file));
  binbuf_eval(b, &pd_canvasmaker, 0, 0);
  fbo_canvas = s__X.s_thing;
  vmess(s__X.s_thing, gensym("pop"), "i", 0);
  glob_setfilename(0, &s_, &s_);
  binbuf_free(b);
}

void findbrokenobjects_setup(void)
{
  iemguts_boilerplate("[findbrokenobjects]", 0);
  findbrokenobjects_class = class_new(gensym("findbrokenobjects"), (t_newmethod)findbrokenobjects_new,
				     (t_method)findbrokenobjects_free, sizeof(t_findbrokenobjects), 0,
				     A_GIMME, 0);
  class_addbang  (findbrokenobjects_class, (t_method)findbrokenobjects_bang);
  class_addmethod(findbrokenobjects_class, (t_method)findbrokenobjects_verbose, gensym("verbose"), A_FLOAT, 0);

  if(0)
    fbo_persist();
}
static char fbo_file[] = "\
canvas 0 0 300 200;\n\
#X obj 20 20 receive __deken_findbroken_objects;\n\
#X obj 20 60 findbrokenobjects;\n\
#X obj 20 80 list prepend plugin-dispatch deken;\n\
#X msg 20 40 unique;\n\
#X obj 20 100 list trim;\n\
#X obj 20 120 s pd;\n\
#X connect 0 0 3 0;\n\
#X connect 1 0 2 0;\n\
#X connect 2 0 4 0;\n\
#X connect 3 0 1 0;\n\
#X connect 4 0 5 0;\n\
";
