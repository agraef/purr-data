/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>      /* for read/write to files */
#include "m_pd.h"
#include "g_canvas.h"
#include <math.h>
#include <ctype.h>

extern int glob_lmclick;

static void garray_select(t_gobj *z, t_glist *glist, int state);
static void garray_doredraw(t_gobj *client, t_glist *glist);

/* see also the "plot" object in g_scalar.c which deals with graphing
arrays which are fields in scalars.  Someday we should unify the
two, but how? */

    /* aux routine to bash leading '#' to '$' for dialogs in u_main.tk
    which can't send symbols starting with '$' (because the Pd message
    interpreter would change them!) */

t_symbol *sharptodollar(t_symbol *s)
{
    char buf[MAXPDSTRING], *s1, *s2;
    if (strlen(s->s_name) >= MAXPDSTRING)
        return (s);
    for (s1 = s->s_name, s2 = buf; ; s1++, s2++)
    {
        if (*s1 == '#' && *s1 && isdigit(s1[1]))
            *s2 = '$';
        else if (!(*s2 = *s1))
            break;
    }
    return (gensym(buf));
}

/* --------- "pure" arrays with scalars for elements. --------------- */

/* Pure arrays have no a priori graphical capabilities.
They are instantiated by "garrays" below or can be elements of other
scalars (g_scalar.c); their graphical behavior is defined accordingly. */

t_array *array_new(t_symbol *templatesym, t_gpointer *parent)
{
    t_array *x = (t_array *)getbytes(sizeof (*x));
    t_template *template;
    template = template_findbyname(templatesym);
    x->a_templatesym = templatesym;
    x->a_n = 1;
    x->a_elemsize = sizeof(t_word) * template->t_n;
    x->a_vec = (char *)getbytes(x->a_elemsize);
        /* note here we blithely copy a gpointer instead of "setting" a
        new one; this gpointer isn't accounted for and needn't be since
        we'll be deleted before the thing pointed to gets deleted anyway;
        see array_free. */
    x->a_gp = *parent;
    x->a_stub = gstub_new(0, x);
    word_init((t_word *)(x->a_vec), template, parent);
    return (x);
}

void array_resize(t_array *x, int n)
{
    //fprintf(stderr,"array_resize %d\n", n);
    int elemsize, oldn;
    t_template *template = template_findbyname(x->a_templatesym);
    if (n < 1)
        n = 1;
    oldn = x->a_n;
    elemsize = sizeof(t_word) * template->t_n;

    x->a_vec = (char *)resizebytes(x->a_vec, oldn * elemsize, n * elemsize);
    x->a_n = n;
    if (n > oldn)
    {
        char *cp = x->a_vec + elemsize * oldn;
        int i = n - oldn;
        for (; i--; cp += elemsize)
        {
            t_word *wp = (t_word *)cp;
            word_init(wp, template, &x->a_gp);
        }
    }
    x->a_valid = ++glist_valid;
}

void array_resize_and_redraw(t_array *array, t_glist *glist, int n)
{
    //fprintf(stderr,"array_resize_and_redraw\n");
    t_array *a2 = array;
    int vis = glist_isvisible(glist);
    while (a2->a_gp.gp_stub->gs_which == GP_ARRAY)
        a2 = a2->a_gp.gp_stub->gs_un.gs_array;
    if (vis)
        gobj_vis(a2->a_gp.gp_un.gp_gobj, glist, 0);
    array_resize(array, n);
    if (vis)
        gobj_vis(a2->a_gp.gp_un.gp_gobj, glist, 1);
}

void word_free(t_word *wp, t_template *template);

void array_free(t_array *x)
{
    int i;
    t_template *scalartemplate = template_findbyname(x->a_templatesym);
    gstub_cutoff(x->a_stub);
    for (i = 0; i < x->a_n; i++)
    {
        t_word *wp = (t_word *)(x->a_vec + x->a_elemsize * i);
        word_free(wp, scalartemplate);
    }
    freebytes(x->a_vec, x->a_elemsize * x->a_n);
    freebytes(x, sizeof *x);
}

/* --------------------- graphical arrays (garrays) ------------------- */

t_class *garray_class;
static int gcount = 0;

struct _garray
{
    t_gobj x_gobj;
    t_scalar *x_scalar;     /* scalar "containing" the array */
    t_glist *x_glist;       /* containing glist */
    t_symbol *x_name;       /* unexpanded name (possibly with leading '$') */
    t_symbol *x_realname;   /* expanded name (symbol we're bound to) */
    char x_usedindsp;       /* true if some DSP routine is using this */
    char x_saveit;          /* true if we should save this with parent */
    char x_joc;             /* true if we should "jump on click" in a graph */
    char x_hidename;        /* don't print name above graph */
    int x_style;            /* so much simpler to keep it here */
    t_symbol *x_send;       /* send_changed hook */
    t_symbol *x_fillcolor;       /* color for filled area of the are */
    t_symbol *x_outlinecolor;    /* color of the outline around the element */
};

t_pd *garray_arraytemplatecanvas;
t_pd *garray_floattemplatecanvas;
static char garray_arraytemplatefile[] = "\
#N canvas 0 0 458 153 10;\n\
#X obj 43 31 struct _float_array array z float float style\n\
float linewidth float color symbol fillcolor symbol outlinecolor;\n\
#X obj 43 70 plot z color linewidth 0 0 1 style fillcolor outlinecolor;\n\
";
static char garray_floattemplatefile[] = "\
#N canvas 0 0 458 153 10;\n\
#X obj 39 26 struct float float y;\n\
";

/* create invisible, built-in canvases to determine the templates for floats
and float-arrays. */

void garray_init( void)
{
    t_binbuf *b;
    if (garray_arraytemplatecanvas)
        return;
    b = binbuf_new();
    
    glob_setfilename(0, gensym("_float"), gensym("."));
    binbuf_text(b, garray_floattemplatefile, strlen(garray_floattemplatefile));
    binbuf_eval(b, 0, 0, 0);
    garray_floattemplatecanvas = s__X.s_thing;
    vmess(s__X.s_thing, gensym("pop"), "i", 0);
    
    glob_setfilename(0, gensym("_float_array"), gensym("."));
    binbuf_text(b, garray_arraytemplatefile, strlen(garray_arraytemplatefile));
    binbuf_eval(b, 0, 0, 0);
    garray_arraytemplatecanvas = s__X.s_thing;
    vmess(s__X.s_thing, gensym("pop"), "i", 0);

    glob_setfilename(0, &s_, &s_);
    binbuf_free(b);
}

/* create a new scalar attached to a symbol.  Used to make floating-point
arrays (the scalar will be of type "_float_array").  Currently this is
always called by graph_array() below; but when we make a more general way
to save and create arrays this might get called more directly. */

static t_garray *graph_scalar(t_glist *gl, t_symbol *s, t_symbol *templatesym,
    t_symbol *fill, t_symbol *outline, int saveit)
{
    t_garray *x;
    t_template *template;
    if (!(template = template_findbyname(templatesym)))
        return (0);
    x = (t_garray *)pd_new(garray_class);
    x->x_scalar = scalar_new(gl, templatesym);
    x->x_name = s;
    x->x_realname = canvas_realizedollar(gl, s);
    pd_bind(&x->x_gobj.g_pd, x->x_realname);
    x->x_usedindsp = 0;
    x->x_saveit = saveit;
    template_setsymbol(template, gensym("fillcolor"), x->x_scalar->sc_vec,
        fill, 1);
    template_setsymbol(template, gensym("outlinecolor"), x->x_scalar->sc_vec,
        outline, 1);
    glist_add(gl, &x->x_gobj);
    x->x_glist = gl;
    char buf[MAXPDSTRING];
    sprintf(buf, "%s_changed", x->x_realname->s_name);
    x->x_send = gensym(buf);
    return (x);
}

    /* get a garray's "array" structure. */
t_array *garray_getarray(t_garray *x)
{
    int zonset, ztype;
    t_symbol *zarraytype;
    t_scalar *sc = x->x_scalar;
    t_symbol *templatesym = sc->sc_template;
    t_template *template = template_findbyname(templatesym);
    if (!template)
    {
        error("array: couldn't find template %s", templatesym->s_name);
        return (0);
    }
    if (!template_find_field(template, gensym("z"), 
        &zonset, &ztype, &zarraytype))
    {
        error("array: template %s has no 'z' field", templatesym->s_name);
        return (0);
    }
    if (ztype != DT_ARRAY)
    {
        error("array: template %s, 'z' field is not an array",
            templatesym->s_name);
        return (0);
    }
    return (sc->sc_vec[zonset].w_array);
}

    /* get the "array" structure and furthermore check it's float */
static t_array *garray_getarray_floatonly(t_garray *x,
    int *yonsetp, int *elemsizep)
{
    t_array *a = garray_getarray(x);
    int yonset, type;
    t_symbol *arraytype;
    t_template *template = template_findbyname(a->a_templatesym);
    if (!template_find_field(template, gensym("y"), &yonset,
        &type, &arraytype) || type != DT_FLOAT)
            return (0);
    *yonsetp = yonset;
    *elemsizep = a->a_elemsize;
    return (a);
}

    /* get the array's name.  Return nonzero if it should be hidden */
int garray_getname(t_garray *x, t_symbol **namep)
{
    *namep = x->x_name;
    return (x->x_hidename);
}

    /* find out if array elements should "jump on click" in a graph */
int garray_joc(t_garray *x)
{
    return (x->x_joc);
}

    /* get a garray's containing glist */
t_glist *garray_getglist(t_garray *x)
{
    return (x->x_glist);
}

    /* get a garray's associated scalar */
t_scalar *garray_getscalar(t_garray *x)
{
    return (x->x_scalar);
}

    /* helper function for fittograph to see if the same GOP has multiple
        arrays in which case take length of the largest one */
static int garray_get_largest_array(t_garray *x)
{
    t_gobj *g;
    t_array *a;
    t_garray *tmp;

    // checks if there is a PLOTSTYLE_POLY vs others
    // longest array uses has_*, total_* gives total number
    // n_array gives you the number of arrays
    int has_poly = 0;
    int total_poly = 0;
    int has_other = 0;
    int total_other = 0;
    int n_array = 0;

    int length = 0;

    for (g = x->x_glist->gl_list; g; g = g->g_next)
    {
        if (pd_class(&g->g_pd) == garray_class)
        {
            n_array++;
            a = garray_getarray((t_garray *)g);
            //if ((t_garray *)g != x)
            //{
            //    an = (x->x_style == PLOTSTYLE_POINTS
            //        || x->x_style == PLOTSTYLE_BARS
            //        || a->a_n == 1 ? a->a_n : a->a_n-1);
            //    if (an > length)
            //        length = an;               
            //}
            //else
            //{
                a = garray_getarray((t_garray *)g);
                tmp = (t_garray *)g;

                // if we have new longest plot reset the
                // count for the longest plot style
                if (a->a_n > length)
                {
                    has_poly = 0;
                    has_other = 0;
                }

                // get the longest (or matching) plots style
                if (a->a_n >= length)
                {
                    if (tmp->x_style == PLOTSTYLE_POLY)
                    {
                        has_poly++;                      
                    }
                    else
                    {
                        has_other++;
                    }
                    length = a->a_n;
                }

                // adjust total number of plot styles
                if (tmp->x_style == PLOTSTYLE_POLY)
                {
                    total_poly++;                        
                }
                else
                {
                    total_other++;
                }
            //}
        }
    }
    //fprintf(stderr, "has_poly=%d has_other=%d | total_poly=%d total_other=%d | n_array=%d | pre-length=%d ",
    //        has_poly, has_other, total_poly, total_other, n_array, length);
    if (has_poly && !has_other)
    {
        if (total_other)
            length--;
    }
    if (!has_poly && has_other)
    {
        if (total_poly)
            length++;
    }
    //fprintf(stderr, "post-lenght=%d\n", length);
    return(length);
}

    /* reset the graph's coordinates
        to fit a new size and style for the garray */
    /* in pd-l2ork we use this for all situations, both one or more
        arrays to ensure that they are always drawn within boundaries. */
    /* flag options:
        -1 = don't resize or redraw, just send bounds message to scalars
         0 = bounds, and also resize
         1 = bounds, resize, and redraw */
void garray_fittograph(t_garray *x, int n, int flag)
{
    //fprintf(stderr,"garray_fittograph\n");
    // here we check for x->x_glist validity because when creating
    // a new array from the menu gl is null at the first garray_vis call
    if (!x->x_glist)
        return;
    int max_length = garray_get_largest_array(x);
    //fprintf(stderr,"garray_fittograph n=%d flag=%d | max_length=%d\n", n, flag, max_length);
    t_array *array = garray_getarray(x);
    t_garray *tmp;
    t_glist *gl = x->x_glist;
    //if (gl->gl_list == &x->x_gobj && !x->x_gobj.g_next)

    t_gobj *g;
    for (g = gl->gl_list; g; g = g->g_next)
    {
        if (pd_class(&g->g_pd) == garray_class)
        {
            //fprintf(stderr,"found array\n");
            tmp = (t_garray *)g;
            vmess(&gl->gl_pd, gensym("bounds"), "ffff",
                0., gl->gl_y1, (double)
                    (tmp->x_style == PLOTSTYLE_POINTS || tmp->x_style == PLOTSTYLE_BARS
                        || max_length == 1 ? max_length : max_length-1),
                        gl->gl_y2);
        }
        /* close any dialogs that might have the wrong info now... 
        TODO: make changes dynamic to avoid this as it causes Apply to
        close the properties which is annoying */
        //gfxstub_deleteforkey(gl);
    }
    if (flag >= 0)
    {
        if (flag == 1)
            array_resize_and_redraw(array, x->x_glist, n);
        else
            array_resize(array, n);
    }
}

/* handle "array" message to glists; call graph_scalar above with
an appropriate template; then set size and flags.  This is called
from the menu and in the file format for patches.  LATER replace this
by a more coherent (and general) invocation. */

t_garray *graph_array(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *fill;
    t_symbol *outline;
    int fflags;
    t_symbol *asym = gensym("#A");

    if (argc < 3) {pd_error(gl, "garray: not enough args"); return 0;}
    t_symbol *name = atom_getsymbolarg(0, argc--, argv++);
    int fsize = (int)atom_getfloatarg(0, argc--, argv++);
    t_symbol *templateargsym = atom_getsymbolarg(0, argc--, argv++);
    if (argc) fflags = (int)atom_getfloatarg(0, argc--, argv++);
    else fflags = 0;
    if (argc) fill = atom_getsymbolarg(0, argc--, argv++);
    else fill = gensym("black");
    if (argc) outline = atom_getsymbolarg(0, argc--, argv++);
    else outline = gensym("black");
    int n = fsize, zonset, ztype, saveit;
    t_symbol *zarraytype;
    t_garray *x;
    t_template *template, *ztemplate;
    t_symbol *templatesym;
    int flags = fflags;
    int filestyle = ((flags & 6) >> 1);
    //fprintf(stderr,"filestyle = %d\n", filestyle);
    int style = (filestyle == 0 ? PLOTSTYLE_POLY :
        (filestyle == 1 ? PLOTSTYLE_POINTS : filestyle));
    if (templateargsym != &s_float)
    {
        error("array %s: only 'float' type understood", templateargsym->s_name);
        return (0);
    }
    templatesym = gensym("pd-_float_array");
    template = template_findbyname(templatesym);
    if (!template)
    {
        error("array: couldn't find template %s", templatesym->s_name);
        return (0);
    }
    if (!template_find_field(template, gensym("z"), 
        &zonset, &ztype, &zarraytype))
    {
        error("array: template %s has no 'z' field", templatesym->s_name);
        return (0);
    }
    if (ztype != DT_ARRAY)
    {
        error("array: template %s, 'z' field is not an array",
            templatesym->s_name);
        return (0);
    }
    if (!(ztemplate = template_findbyname(zarraytype)))
    {
        error("array: no template of type %s", zarraytype->s_name);
        return (0);
    }
    saveit = ((flags & 1) != 0);
    x = graph_scalar(gl, name, templatesym, fill, outline, saveit);
    x->x_hidename = ((flags & 8) >> 3);
    x->x_joc = ((flags & 16) >> 4);
    x->x_fillcolor = fill;
    x->x_outlinecolor = outline;
    x->x_style = style;

    if (n <= 0)
        n = 100;
    array_resize(x->x_scalar->sc_vec[zonset].w_array, n);

    // let's make sure that the GOP glist has sane x values to ensure that
    // the entire array fits inside the box (this gets properly initialized
    // when creating from the menu but not when loading from the file)
    int gop_w = (x->x_style == PLOTSTYLE_POINTS || x->x_style == PLOTSTYLE_BARS
                    || n == 1 ? n : n-1);
    //fprintf(stderr,"graph_array glist->gl_x2=%g gop_w=%d n=%d\n", gl->gl_x2, gop_w, n);
    if (gl->gl_x2 != gop_w) gl->gl_x2 = gop_w;

    template_setfloat(template, gensym("style"), x->x_scalar->sc_vec,
        x->x_style, 1);
    template_setfloat(template, gensym("linewidth"), x->x_scalar->sc_vec, 
        ((x->x_style == PLOTSTYLE_POINTS) ? 2 : 1), 1);
    template_setsymbol(template, gensym("fillcolor"), x->x_scalar->sc_vec,
        fill, 1);
    template_setsymbol(template, gensym("outlinecolor"), x->x_scalar->sc_vec,
        outline, 1);

           /* bashily unbind #A -- this would create garbage if #A were
           multiply bound but we believe in this context it's at most
           bound to whichever textobj or array was created most recently */
    asym->s_thing = 0;
        /* and now bind #A to us to receive following messages in the
        saved file or copy buffer */
    pd_bind(&x->x_gobj.g_pd, asym);

    /*if (x2 = pd_findbyclass(gensym("#A"), garray_class))
        pd_unbind(x2, gensym("#A"));
    pd_bind(&x->x_gobj.g_pd, gensym("#A"));*/

    garray_redraw(x);

/* todo: need to test to see if this is necessary
   doesn't seem like it is...
    garray_fittograph(x, n, 1);
*/
    return (x);
}

    /* called from array menu item to create a new one */
void canvas_menuarray(t_glist *canvas)
{
    if (canvas_hasarray(canvas)) return;
    t_glist *x = (t_glist *)canvas;
    pd_vmess(&x->gl_pd, gensym("editmode"), "i", 1);
    //char cmdbuf[200];
    gcount++;
    gui_vmess("gui_array_new", "si",
        gfxstub_new2(&x->gl_pd, x),
        gcount);
    //sprintf(cmdbuf, "pdtk_array_dialog %%s array%d 100 3 1 .x%lx black black\n",
    //    ++gcount, (long unsigned int)canvas);
    //gfxstub_new(&x->gl_pd, x, cmdbuf);
}

    /* called from canvas_dialog to return array properties for the gui */
int garray_properties(t_garray *x, t_symbol **gfxstubp, t_symbol **namep,
    int *sizep, int *flagsp, t_symbol **fillp, t_symbol **outlinep)
{
    char namebuf[MAXPDSTRING];
    t_array *a = garray_getarray(x);
    if (!a)
        return 0;
    //gfxstub_deleteforkey(x);
        /* LATER fix this to escape '$'
        properly; right now we just detect a leading '$' and escape
        it.  There should be a systematic way of doing this. */
    /* still don't understand this filestyle business... */
    int filestyle = (x->x_style == 0 ? PLOTSTYLE_POLY :
        (x->x_style == 1 ? PLOTSTYLE_POINTS : x->x_style));
    int flags = x->x_saveit + 2 * filestyle + 8 * x->x_hidename + 16 * x->x_joc;

    if (x->x_name->s_name[0] == '$')
    {
        sprintf(namebuf, "\\%s", x->x_name->s_name);
    }
    else
    {
        sprintf(namebuf, "%s", x->x_name->s_name);
    }

    *gfxstubp = gensym(gfxstub_new2(&x->x_gobj.g_pd, x));
    *namep = gensym(namebuf);
    *sizep = a->a_n;
    *flagsp = flags;
    *fillp = x->x_fillcolor;
    *outlinep = x->x_outlinecolor;
    //sprintf(cmdbuf, ((x->x_name->s_name[0] == '$') ?
    //    "pdtk_array_dialog %%s \\%s %d %d 0 .x%lx %s %s\n" :
    //    "pdtk_array_dialog %%s %s %d %d 0 .x%lx %s %s\n"),
    //        x->x_name->s_name,
    //        a->a_n,
    //        x->x_saveit +  2 * filestyle + 8 * x->x_hidename + 16 * x->x_joc,
    //        (long unsigned int)glist_getcanvas(canvas),
    //        x->x_fillcolor->s_name,
    //        x->x_outlinecolor->s_name);
    //gfxstub_new(&x->x_gobj.g_pd, x, cmdbuf);
    return 1;
}

    /* this is called back from the dialog window to create a garray. 
    The otherflag requests that we find an existing graph to put it in. */
void glist_arraydialog(t_glist *parent, t_symbol *s, int argc, t_atom *argv)
//t_floatarg size, t_floatarg fflags, t_floatarg otherflag, float xdraw, float ydraw)
{
    t_atom at[6];
    if (argc !=8) {pd_error(parent,
        "arraydialog: wrong number of args"); return;}
    t_float size, fflags, otherflag, xdraw, ydraw;
    t_symbol *name = atom_getsymbolarg(0, argc, argv);
    size = atom_getfloatarg(1, argc, argv);
    fflags = atom_getfloatarg(2, argc, argv);
    otherflag = atom_getfloatarg(3, argc, argv);
    xdraw = atom_getfloatarg(4, argc, argv);
    ydraw = atom_getfloatarg(5, argc, argv);
    t_symbol *fillcolor = atom_getsymbolarg(6, argc, argv);
    t_symbol *outlinecolor = atom_getsymbolarg(7, argc, argv);

    t_glist *gl;
    t_garray *a;
    int flags = fflags;
    if (size < 1)
        size = 1;
    if (otherflag == 0 || (!(gl = glist_findgraph(parent))))
    {
        gl = glist_addglist(parent, &s_, 0, 1,
            (size > 1 ? size-1 : size), -1, xdraw+30, ydraw+30,
            xdraw+30+GLIST_DEFGRAPHWIDTH, ydraw+30+GLIST_DEFGRAPHHEIGHT);
        gl->gl_hidetext = 1;
    }
    //a = graph_array(gl, sharptodollar(name), &s_float, size, flags);
    SETSYMBOL(at, sharptodollar(name));
    SETFLOAT(at+1, size);
    SETSYMBOL(at+2, &s_float);
    SETFLOAT(at+3, flags);

    /* no idea what xdraw and ydraw are used for...
       graph_array doesn't seem to use them 
    SETFLOAT(at+4, xdraw);
    SETFLOAT(at+5, ydraw);
    */

    SETSYMBOL(at+4, fillcolor);
    SETSYMBOL(at+5, outlinecolor);
    a = graph_array(gl, gensym("array"), 8, at);
    
    //canvas_redraw(glist_getcanvas(parent));
    garray_fittograph(a, (int)size, -1);
    // we queue gui here because otherwise garray is not drawn properly
    // since things are still being created and the object has not yet
    // been associated with glist
    // sys_queuegui((t_gobj *)gl, glist_getcanvas(gl), graph_redraw);
    //fprintf(stderr,"glist_arraydialog done\n");
    glist_redraw(gl);
    canvas_getscroll(glist_getcanvas(parent));
    canvas_dirty(parent, 1);
}

extern void canvas_apply_setundo(t_canvas *x, t_gobj *y);

    /* this is called from the properties dialog window for an existing array */
void garray_arraydialog(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    //fprintf(stderr,"================garray_arraydialog\n");

    int deleteit = atom_getfloatarg(3, argc, argv);
    if (deleteit != 0)
    {
        //fprintf(stderr,"deleteit\n");
        //glist_select(x->x_glist, &x->x_gobj);
        //canvas_undo_add(x->x_glist, 3, "delete", canvas_undo_set_cut(x->x_glist, 2)); // 2 = UCUT_CLEAR (from g_editor.c)
//currently cannot be undo'd until we do a new kind of undo
        t_canvas *c = x->x_glist;
        int dspwas = canvas_suspend_dsp();
        glist_delete(x->x_glist, &x->x_gobj);
        canvas_resume_dsp(dspwas);
        canvas_redraw(glist_getcanvas(c));
    }
    else 
    {
        //need a new kind of undo
        //canvas_apply_setundo(glist_getcanvas(x->x_glist), (t_gobj *)x);

        if (argc !=8)
        {
            pd_error(x, "arraydialog: wrong number of args"); return;
        }
        t_symbol *name = atom_getsymbolarg(0, argc, argv);
        int fsize = atom_getfloatarg(1, argc, argv);
        int flags = atom_getfloatarg(2, argc, argv);
        t_symbol *fill = atom_getsymbolarg(6, argc, argv);
        t_symbol *outline = atom_getsymbolarg(7, argc, argv);
        int saveit = ((flags & 1) != 0);
        int filestyle = ((flags & 6) >> 1);
        int style = (filestyle == 0 ? PLOTSTYLE_POLY :
            (filestyle == 1 ? PLOTSTYLE_POINTS : filestyle));

        /* todo: revisit this filestyle business
        if (style < 2) style = !style;
        */

        int hidename = ((flags & 8) >> 3);
        int joc = ((flags & 16) >> 4);

        /*t_float stylewas = template_getfloat(
        template_findbyname(x->x_scalar->sc_template),
        gensym("style"), x->x_scalar->sc_vec, 1);*/

        long size;
        t_symbol *argname = sharptodollar(name);
        t_array *a = garray_getarray(x);
        t_template *scalartemplate;
        if (!a)
        {
            pd_error(x, "can't find array\n");
            return;
        }
        if (!(scalartemplate = template_findbyname(x->x_scalar->sc_template)))
        {
            error("array: no template of type %s",
                x->x_scalar->sc_template->s_name);
            return;
        }
        if (argname != x->x_name)
        {
            x->x_name = argname;
            pd_unbind(&x->x_gobj.g_pd, x->x_realname);
            x->x_realname = canvas_realizedollar(x->x_glist, argname);
            pd_bind(&x->x_gobj.g_pd, x->x_realname);
        }
            /* redraw the whole glist, just so the name change shows up
                there has to be a better way */
        /*if (x->x_glist->gl_havewindow)
        {
            canvas_redraw(glist_getcanvas(x->x_glist));
            //fprintf(stderr,"================REDRAW\n");
        }*/
        size = fsize;
        if (size < 1)
            size = 1;
        if (style != x->x_style)
        {
            x->x_style = style;
            //garray_fittograph(x, size, 1);
        }
        //fprintf(stderr,"style=%d %f\n", style, (t_float)x->x_style);
        template_setfloat(scalartemplate, gensym("style"),
            x->x_scalar->sc_vec, (t_float)x->x_style, 0);
        template_setfloat(scalartemplate, gensym("linewidth"),
            x->x_scalar->sc_vec, ((x->x_style == PLOTSTYLE_POINTS) ? 2 : 1), 1);
        template_setsymbol(scalartemplate, gensym("fillcolor"),
            x->x_scalar->sc_vec, fill, 0);
        template_setsymbol(scalartemplate, gensym("outlinecolor"),
            x->x_scalar->sc_vec, outline, 0);

        char buf[MAXPDSTRING];
        sprintf(buf, "%s_changed", x->x_realname->s_name);
        x->x_send = gensym(buf);

        garray_setsaveit(x, (saveit != 0));
        x->x_joc = joc;
        x->x_hidename = hidename;
        x->x_fillcolor = fill;
        x->x_outlinecolor = outline;
        x->x_style = style;
        if (size != a->a_n)
        {
            garray_resize_long(x, size);
        }
        else
        {
            /* So, this next function call is here _solely_ to get the
               the new array name to show up in the graph label if the user
               happened to change it. Let me emphasize-- in order to
               redraw array labels, we must call a function that redraws
               not only the _entire_ array and its graph, but also redraws
               the parent canvas in which the graph is displayed. There is
               no interface I can find to just say, "redraw the label".

               Worse, Pd redraws a single array at least 3 times, and maybe
               even the graph and the containing glist-- it's hard to tell
               because so much data is sent over the wire that I run out of
               buffer in my terminal window. These are a side-effect of
               garray_redraw, as well as the garray_resize branch above. (And 
               don't forget that the canvas dialog callback probably causes
               as many redraws as well.)

               Until the Pd codebase handles redrawing in a sane fashion,
               without depending on a vast array of side-effects, there's
               simply no way to effectively maintain it. */
            glist_redraw(x->x_glist);
            garray_redraw(x);
        }

        //fprintf(stderr,"garray_arraydialog garray_redraw done\n");
        garray_select((t_gobj *)x,glist_getcanvas(x->x_glist),1);
        canvas_dirty(x->x_glist, 1);
    }
}

static void garray_free(t_garray *x)
{
    t_pd *x2;
    sys_unqueuegui(&x->x_gobj);
    gfxstub_deleteforkey(x);
    pd_unbind(&x->x_gobj.g_pd, x->x_realname);
        /* LATER find a way to get #A unbound earlier (at end of load?) */
    while (x2 = pd_findbyclass(gensym("#A"), garray_class))
        pd_unbind(x2, gensym("#A"));
    pd_free(&x->x_scalar->sc_gobj.g_pd);
}

/* ------------- code used by both array and plot widget functions ---- */

void array_redraw(t_array *a, t_glist *glist)
{
    while (a->a_gp.gp_stub->gs_which == GP_ARRAY)
        a = a->a_gp.gp_stub->gs_un.gs_array;
    t_scalar *sc = (t_scalar *)(a->a_gp.gp_un.gp_gobj);
    scalar_redraw(sc, glist);
}

    /* routine to get screen coordinates of a point in an array */
    /* glist_topixels parameter was added because of t_scalar
       and t_draw caching mechanism. For scalers, we're only
       converting to pixels in scalar_getrect. So in plot_getrect
       we need to get the coordinate without regard to the x/y
       offset (and scaling factor) of a gop window */
void array_getcoordinate(t_glist *glist,
    char *elem, int xonset, int yonset, int wonset, int indx,
    t_float basex, t_float basey, t_float xinc,
    t_fielddesc *xfielddesc, t_fielddesc *yfielddesc, t_fielddesc *wfielddesc,
    t_float *xp1, t_float *xp2, t_float *yp, t_float *wp,
    int glist_topixels)
{
    t_float xval, yval, ypix, wpix;
    if (xonset >= 0)
        xval = *(t_float *)(elem + xonset);
    else xval = indx * xinc;
    if (yonset >= 0)
        yval = *(t_float *)(elem + yonset);
    else yval = 0;
    /* This is from the old API. Now we group drawing instructions in
       a hierarchy so we can do glist_ytopixels math on the GUI side */
    //ypix = glist_ytopixels(glist, basey +
    //    fielddesc_cvttocoord(yfielddesc, yval));
    ypix = basey + fielddesc_cvttocoord(yfielddesc, yval);
    if (glist_topixels)
        ypix = glist_ytopixels(glist, ypix);
    if (wonset >= 0)
    {
            /* found "w" field which controls linewidth. */
        t_float wval = *(t_float *)(elem + wonset);
        /* ytopixels is now done GUI-side */
        //wpix = glist_ytopixels(glist, basey +
        //    fielddesc_cvttocoord(yfielddesc, yval) +
        //        fielddesc_cvttocoord(wfielddesc, wval)) - ypix;
        wpix = basey + fielddesc_cvttocoord(yfielddesc, yval) +
            fielddesc_cvttocoord(wfielddesc, wval);
        if (glist_topixels)
            wpix = glist_ytopixels(glist, wpix);
        wpix -= ypix;
        if (wpix < 0)
            wpix = -wpix;
    }
    else wpix = 1;
    if (glist_topixels)
    {
        *xp1 = glist_xtopixels(glist, basex +
            fielddesc_cvttocoord(xfielddesc, xval));
        *xp2 = glist_xtopixels(glist, basex +
            fielddesc_cvttocoord(xfielddesc, xval+1))-1;
    }
    else
    {
        *xp1 = basex + fielddesc_cvttocoord(xfielddesc, xval);
        *xp2 = basex + fielddesc_cvttocoord(xfielddesc, xval+1)-1;
    }
    *yp = ypix;
    // increased following on 20140830 to 8 (+4 and -4) and 
    // so that the smallest hitbox is always at least 8x8
    // check with all_about_arrays.pd inside custom scalars
    // in an array
    if (glist_topixels && *xp1 == *xp2)
    {
        *xp1 = *xp1 - 4;
        *xp2 = *xp2 + 4;
        if (wpix < 8) wpix = 8;
    }
    *wp = wpix;
}

extern int array_joc; /* from g_canvas.h */
static t_float array_motion_xcumulative;
static t_float array_motion_ycumulative;
static t_fielddesc *array_motion_xfield;
static t_fielddesc *array_motion_yfield;
static t_glist *array_motion_glist;
static t_scalar *array_motion_scalar;
static t_array *array_motion_array;
static t_word *array_motion_wp;
static t_template *array_motion_template;
static int array_motion_npoints;
static int array_motion_elemsize;
//static int array_motion_altkey;
static t_float array_motion_initx;
static t_float array_motion_xperpix;
static t_float array_motion_yperpix;
static int array_motion_lastx;
static int array_motion_fatten;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

    /* this is called when a mouse drag happens inside an array (either
       scalar or the whole array--this needs to be tested) */
static void array_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    array_motion_xcumulative += dx * array_motion_xperpix;
    array_motion_ycumulative += dy * array_motion_yperpix;
    //fprintf(stderr,"array_motion %f %f %f %f\n", array_motion_xcumulative, array_motion_ycumulative, dx, dy);

    // used to set up boundaries and update sends accordingly
    t_glist *graph = NULL;
    if (array_garray != NULL) graph = array_garray->x_glist;

    if (array_motion_xfield)
    {
            /* it's an x, y plot */
        int i;
        for (i = 0; i < array_motion_npoints; i++)
        {
            t_word *thisword = (t_word *)(((char *)array_motion_wp) +
                i * array_motion_elemsize);
            t_float xwas = fielddesc_getcoord(array_motion_xfield, 
                array_motion_template, thisword, 1);
            t_float ywas = (array_motion_yfield ?
                fielddesc_getcoord(array_motion_yfield, 
                    array_motion_template, thisword, 1) : 0);
            fielddesc_setcoord(array_motion_xfield,
                array_motion_template, thisword, xwas + dx, 1);
            if (array_motion_yfield)
            {
                if (array_motion_fatten)
                {
                    if (i == 0)
                    {
                        t_float newy = ywas + dy * array_motion_yperpix;
                        if (newy < 0)
                            newy = 0;
                        fielddesc_setcoord(array_motion_yfield,
                            array_motion_template, thisword, newy, 1);
                    }
                }
                else
                {
                    fielddesc_setcoord(array_motion_yfield,
                        array_motion_template, thisword,
                            ywas + dy * array_motion_yperpix, 1);
                }
            }
        }
    }
    else if (array_motion_yfield)
    {
            /* a y-only plot. */
        int thisx = array_motion_initx + array_motion_xcumulative + 0.5, x2;
        int increment, i, nchange;
        t_float newy = array_motion_ycumulative,
            oldy = fielddesc_getcoord(array_motion_yfield,
                array_motion_template,
                    (t_word *)(((char *)array_motion_wp) +
                        array_motion_elemsize * array_motion_lastx),
                            1);
        // here we block scalar from exceeding the array GOP edges
        // LATER: see if we need to do the same for an x plot
        if (graph)
        {
            if (graph->gl_y1 > graph->gl_y2)
            {
                if (newy > graph->gl_y1) newy = graph->gl_y1;
                if (newy < graph->gl_y2) newy = graph->gl_y2;
            }
            else
            {
                if (newy < graph->gl_y1) newy = graph->gl_y1;
                if (newy > graph->gl_y2) newy = graph->gl_y2;
            }
        }
        //fprintf(stderr, "y = %f\n", newy);
        t_float ydiff = newy - oldy;
        if (thisx < 0) thisx = 0;
        else if (thisx >= array_motion_npoints)
            thisx = array_motion_npoints - 1;
        increment = (thisx > array_motion_lastx ? -1 : 1);
        nchange = 1 + increment * (array_motion_lastx - thisx);

        for (i = 0, x2 = thisx; i < nchange; i++, x2 += increment)
        {
            fielddesc_setcoord(array_motion_yfield,
                array_motion_template,
                    (t_word *)(((char *)array_motion_wp) +
                        array_motion_elemsize * x2), newy, 1);
            if (nchange > 1)
                newy -= ydiff * (1./(nchange - 1));
         }
         array_motion_lastx = thisx;
    }
    //fprintf(stderr, "%f %f\n", graph->gl_y1, graph->gl_y2);

    if (array_motion_scalar)
        scalar_redraw(array_motion_scalar, array_motion_glist);
    if (array_motion_array)
        array_redraw(array_motion_array, array_motion_glist);

    /* send a bang to the associated send to reflect the change
       via mouse click/drag */
    if (graph && array_garray->x_send->s_thing)
        pd_bang(array_garray->x_send->s_thing);
}

int scalar_doclick(t_word *data, t_template *template, t_scalar *sc,
    t_array *ap, struct _glist *owner,
    t_float xloc, t_float yloc, int xpix, int ypix,
    int shift, int alt, int dbl, int doit);
    /* try clicking on an element of the array as a scalar (if clicking
    on the trace of the array failed) */
static int array_doclick_element(t_array *array, t_glist *glist,
    t_scalar *sc, t_array *ap,
    t_symbol *elemtemplatesym,
    t_float linewidth, t_float xloc, t_float xinc, t_float yloc,
    t_fielddesc *xfield, t_fielddesc *yfield, t_fielddesc *wfield,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //fprintf(stderr,"array_doclick_element linewidth%f xloc%f xinc%f yloc%f xpix%d ypix%d doit%d\n", linewidth, xloc, xinc, yloc, xpix, ypix, doit);
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    int elemsize, yonset, wonset, xonset, i, incr, hit;
    t_float xsum;

    if (elemtemplatesym == &s_float)
        return (0);
    if (array_getfields(elemtemplatesym, &elemtemplatecanvas,
        &elemtemplate, &elemsize, xfield, yfield, wfield,
            &xonset, &yonset, &wonset))
                return (0);
        /* if it has more than 2000 points, just check 300 of them. */
    if (array->a_n < 2000)
        incr = 1;
    else incr = array->a_n / 300;
    for (i = 0, xsum = 0; i < array->a_n; i += incr)
    {
        t_float usexloc, useyloc;
        if (xonset >= 0)
            usexloc = xloc + fielddesc_cvttocoord(xfield, 
                *(t_float *)(((char *)(array->a_vec) + elemsize * i) + xonset));
        else usexloc = xloc + xsum, xsum += xinc;
        useyloc = yloc + (yonset >= 0 ? fielddesc_cvttocoord(yfield,
            *(t_float *)(((char *)(array->a_vec) + elemsize * i) + yonset)) : 0);
        
        if (hit = scalar_doclick(
            (t_word *)((char *)(array->a_vec) + i * elemsize),
            elemtemplate, 0, array,
            glist, usexloc, useyloc,
            xpix, ypix, shift, alt, dbl, doit))
                return (hit);
    }
    return (0);
}

    /* LATER move this and others back into plot parentwidget code, so
    they can be static (look in g_canvas.h for candidates). */
int array_doclick(t_array *array, t_glist *glist, t_scalar *sc, t_array *ap,
    t_symbol *elemtemplatesym,
    t_float linewidth, t_float xloc, t_float xinc, t_float yloc, t_float scalarvis,
    t_fielddesc *xfield, t_fielddesc *yfield, t_fielddesc *wfield,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //fprintf(stderr,"array_doclick linewidth%f xloc%f xinc%f yloc%f xpix%d ypix%d doit%d\n", linewidth, xloc, xinc, yloc, xpix, ypix, doit);
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    int elemsize, yonset, wonset, xonset, i;

    if (!array_getfields(elemtemplatesym, &elemtemplatecanvas,
        &elemtemplate, &elemsize, xfield, yfield, wfield,
        &xonset, &yonset, &wonset))
    {
        t_float best = -1;
            /* if it has more than 2000 points, just check 1000 of them. */
        int incr = (array->a_n <= 2000 ? 1 : array->a_n / 1000);
        t_float pxpix1 = 0.0, pxpix2 = 0.0, pypix = 0.0, pwpix = 0.0,
          dy, dy2, dy3;
        for (i = 0; i < array->a_n; i += incr)
        {
            array_getcoordinate(glist, (char *)(array->a_vec) + i * elemsize,
                xonset, yonset, wonset, i, xloc, yloc, xinc,
                xfield, yfield, wfield, &pxpix1, &pxpix2, &pypix, &pwpix, 1);
            // increased following on 20140830 to 8 and updated
            // array_getcoordinate
            // so that the smallest hitbox is always at least 8x8--check with
            // all_about_arrays.pd inside custom scalars in an array
            if (pwpix < 8)
                pwpix = 8;

            if (xpix >= (int)pxpix1-pwpix && xpix <= (int)pxpix2+pwpix &&
                ((array_joc) ||
                 (ypix >= pypix-pwpix && ypix <= pypix+pwpix)))
            {
                best = i;
                break;
            }
        }
        //fprintf(stderr,"    best = %f\n", best);
        /* this is the arbitrary radius away from the actual object's
           center, originally 8 */
        if (best == -1 && (array_joc == 0))
        {
            //fprintf(stderr,"best > 8\n");
            if (scalarvis != 0)
            {
                //fprintf(stderr,"    array_doclick_element\n");
                return (array_doclick_element(array, glist, sc, ap,
                    elemtemplatesym, linewidth, xloc, xinc, yloc,
                        xfield, yfield, wfield,
                        xpix, ypix, shift, alt, dbl, doit));
            }
            else
            {
                //fprintf(stderr,"    return 0\n");
                return (0);
            }
        }
        dy = pypix - ypix;
        if (dy < 0) dy = -dy;
        if (wonset >= 0)
        {
            dy2 = (pypix + pwpix) - ypix;
            if (dy2 < 0) dy2 = -dy2;
            dy3 = (pypix - pwpix) - ypix;
            if (dy3 < 0) dy3 = -dy3;
            if (yonset < 0)
                dy = 100;
        }
        else dy2 = dy3 = 100;
#if 0 // this doesn't seem to be used anywhere -ag
        int dx;
        int hit = 0;
        if(array_joc)
        {
                hit = (xpix >= pxpix1) && (xpix < pxpix2);
        }
        else
            hit = dx + dy <= best || dx + dy2 <= best || dx + dy3 <= best;
#endif
        if (dy < dy2 && dy < dy3)
        {
            array_motion_fatten = 0;
            //fprintf(stderr,"A\n");
        }
        else if (dy2 < dy3)
        {
            array_motion_fatten = -1;
            //fprintf(stderr,"B\n");
        }
        else if (!array_joc)
        {
            array_motion_fatten = 1;
            //fprintf(stderr,"C\n");
        }
        if (doit || (glob_lmclick && array_joc))
        {
            char *elem = (char *)array->a_vec;
            array_motion_elemsize = elemsize;
            array_motion_glist = glist;
            array_motion_scalar = sc;
            array_motion_array = ap;
            array_motion_template = elemtemplate;
            array_motion_xperpix = glist_dpixtodx(glist, 1);
            array_motion_yperpix = glist_dpixtody(glist, 1);
            if (alt) /* delete a point */
            {
                if (array->a_n <= 1)
                    return (0);
                memmove((char *)(array->a_vec) + elemsize * i, 
                    (char *)(array->a_vec) + elemsize * (i+1),
                        (array->a_n - 1 - i) * elemsize);
                //array_resize_and_redraw(array, glist, array->a_n - 1);
                garray_resize(array_garray, array->a_n - 1);
                canvas_setcursor(glist_getcanvas(glist), 0);
                return (0);
            }
            else if (shift)
            {
                /* add a point (after the clicked-on one) */
                //fprintf(stderr,"add a point\n");
                //array_resize_and_redraw(array, glist, array->a_n + 1);
                elem = (char *)array->a_vec;
                memmove(elem + elemsize * (i+1), 
                    elem + elemsize * i,
                        (array->a_n - i - 1) * elemsize);
                i++;
                garray_resize(array_garray, array->a_n + 1);
                canvas_setcursor(glist_getcanvas(glist), 0);
            }
            if (xonset >= 0)
            {
                //fprintf(stderr, "   xonset >=0\n");
                array_motion_xfield = xfield;
                array_motion_xcumulative = 
                    fielddesc_getcoord(xfield, array_motion_template,
                        (t_word *)(elem + i * elemsize), 1);
                    array_motion_wp = (t_word *)(elem + i * elemsize);
                if (shift)
                    array_motion_npoints = array->a_n - i;
                else array_motion_npoints = 1;
            }
            else
            {
                //fprintf(stderr, "   !(xonset >=0)\n");
                array_motion_xfield = 0;
                array_motion_xcumulative = 0;
                array_motion_wp = (t_word *)elem;
                array_motion_npoints = array->a_n;

                array_motion_initx = i;
                array_motion_lastx = i;
                array_motion_xperpix *= (xinc == 0 ? 1 : 1./xinc);
            }
            if (array_motion_fatten)
            {
                //fprintf(stderr, "   motion_fatten\n");
                array_motion_yfield = wfield;
                array_motion_ycumulative = 
                    fielddesc_getcoord(wfield, array_motion_template,
                        (t_word *)(elem + i * elemsize), 1);
                array_motion_yperpix *= -array_motion_fatten;
            }
            else if (yonset >= 0)
            {
                //fprintf(stderr, "   yonset >=0\n");
                array_motion_yfield = yfield;
                array_motion_ycumulative =
                    fielddesc_getcoord(yfield, array_motion_template,
                        (t_word *)(elem + i * elemsize), 1);
                    /* *(t_float *)((elem + elemsize * i) + yonset); */
                // we do cursor detection based on the kind of a graph.
                // (e.g. 3-point PLOYSTYLE_POINTS creates a graph that has 4
                // delimiting points, while a 3-point PLOTSTYLE_POLY creates
                // a graph that has only 3 delimiting points)
                // This, therefore takes into account whether we should count
                // the center point of a bar or the starting point (poly) as
                // our reference.
                // TODO: see if we can reimplement Bezier curves
                if (array_garray != NULL &&
                    (array_garray->x_style == PLOTSTYLE_POLY ||
                     array_garray->x_style == PLOTSTYLE_BEZ))
                {
                    array_motion(0, xpix - pxpix1, ypix - pypix);
                }
                else
                    array_motion(0, (xpix - (pxpix1 + (pxpix2 - pxpix1)/2)),
                        ypix - pypix);
            }
            else
            {
                //fprintf(stderr, "   else 0\n");
                array_motion_yfield = 0;
                array_motion_ycumulative = 0;
            }
            //fprintf(stderr,"    glist_grab %d %d\n", xpix, ypix);
            glist_grab(glist, 0, array_motion, 0, xpix, ypix);
        }
        if (alt)
        {
            return (CURSOR_EDITMODE_DISCONNECT);
        }
        else if (shift)
        {
            return (CURSOR_RUNMODE_ADDPOINT);
        }
        else return (array_motion_fatten ?
            CURSOR_RUNMODE_THICKEN : CURSOR_RUNMODE_CLICKME);
    }
    return (0);
}

#if 0 // this doesn't seem to be used anywhere -ag
static void array_getrect(t_array *array, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    //fprintf(stderr,"array getrect %d %d\n", glist_istoplevel(glist), (array_joc != 0 ? 1 : 0));
    t_float x1 = 0x7fffffff, y1 = 0x7fffffff, x2 = -0x7fffffff, y2 = -0x7fffffff;
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    int elemsize, yonset, wonset, xonset, i;
    
    if (!array_getfields(array->a_templatesym, &elemtemplatecanvas,
        &elemtemplate, &elemsize, 0, 0, 0, &xonset, &yonset, &wonset))
    {
        int incr;
            /* if it has more than 2000 points, just check 300 of them. */
        if (array->a_n < 2000)
            incr = 1;
        else incr = array->a_n / 300;
        for (i = 0; i < array->a_n; i += incr)
        {
            t_float pxpix1, pxpix2, pypix, pwpix;
            array_getcoordinate(glist, (char *)(array->a_vec) +
                i * elemsize,
                xonset, yonset, wonset, i, 0, 0, 1,
                0, 0, 0,
                &pxpix1, &pxpix2, &pypix, &pwpix, 1);
            if (pwpix < 2)
                pwpix = 2;
            if (pxpix1 < x1)
                x1 = pxpix1;
            if (pxpix2 > x2)
                x2 = pxpix2;
            if (pypix - pwpix < y1)
                y1 = pypix - pwpix;
            if (pypix + pwpix > y2)
                y2 = pypix + pwpix;
        }
    }
    //fprintf(stderr,"array_getrect %f %f %f %f\n", x1, y1, x2, y2);
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}
#endif

/* -------------------- widget behavior for garray ------------ */

static void garray_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    //fprintf(stderr,"garray_getrect\n");
    t_garray *x = (t_garray *)z;
    gobj_getrect(&x->x_scalar->sc_gobj, glist, xp1, yp1, xp2, yp2);
}

static void garray_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    /* refuse */
}

static void garray_select(t_gobj *z, t_glist *glist, int state)
{
    //fprintf(stderr,">>>>>>>>>>>>garray_select %d\n", state);
    t_garray *x = (t_garray *)z;
    /* There's no replacement for the following command in the
       new GUI, but it looks like it's not needed anymore. */
    //sys_vgui("pdtk_select_all_gop_widgets .x%lx %lx %d\n",
    //    glist_getcanvas(glist), x->x_glist, state);

    extern void scalar_select(t_gobj *z, t_glist *owner, int state);
    scalar_select((t_gobj *)x->x_scalar, glist, state);
}

static void garray_activate(t_gobj *z, t_glist *glist, int state)
{
}

static void garray_delete(t_gobj *z, t_glist *glist)
{
    /* nothing to do */
}

extern int do_not_redraw;

static void garray_vis(t_gobj *z, t_glist *glist, int vis)
{
    //fprintf(stderr,"garray_vis %d\n", vis);
    t_garray *x = (t_garray *)z;

    if (vis) {
        t_array *a = garray_getarray(x);
        int ne = a->a_n;
        int n = (x->x_style == PLOTSTYLE_POINTS ||
            x->x_style == PLOTSTYLE_BARS || ne == 1 ? ne : ne-1);
        //fprintf(stderr,"garray_vis glist->gl_x2=%g n=%d a->a_n=%d\n", glist->gl_x2, n, a->a_n);
        //if (glist->gl_x2 != n) {
        glist->gl_x2 = n;
        do_not_redraw = 1;
        garray_fittograph(x, n, -1);
        do_not_redraw = 0;
    }
    gobj_vis(&x->x_scalar->sc_gobj, glist, vis);
    //if (((t_glist *)z)->gl_isgraph)
    //    fprintf(stderr,"garray_vis am_graph\n");
}

static int garray_click(t_gobj *z, t_glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_garray *x = (t_garray *)z;
    array_garray = x;
    return (gobj_click(&x->x_scalar->sc_gobj, glist,
        xpix, ypix, shift, alt, dbl, doit));
}

#define ARRAYWRITECHUNKSIZE 1000

void garray_savecontentsto(t_garray *x, t_binbuf *b)
{
    if (x->x_saveit)
    {
        t_array *array = garray_getarray(x);
        int n = array->a_n, n2 = 0;
        if (n > 200000)
            post("warning: I'm saving an array with %d points!\n", n);
        while (n2 < n)
        {
            int chunk = n - n2, i;
            if (chunk > ARRAYWRITECHUNKSIZE)
                chunk = ARRAYWRITECHUNKSIZE;
            binbuf_addv(b, "si", gensym("#A"), n2);
            for (i = 0; i < chunk; i++)
                binbuf_addv(b, "f", ((t_word *)(array->a_vec))[n2+i].w_float);
            binbuf_addv(b, ";");
            n2 += chunk;
        }
    }
}

void garray_save(t_gobj *z, t_binbuf *b)
{
    int filestyle;
    t_garray *x = (t_garray *)z;
    t_array *array = garray_getarray(x);
    t_template *scalartemplate;
    if (x->x_scalar->sc_template != gensym("pd-_float_array"))
    {
            /* LATER "save" the scalar as such */ 
        pd_error(x, "can't save arrays of type %s yet", 
            x->x_scalar->sc_template->s_name);
        return;
    }
    if (!(scalartemplate = template_findbyname(x->x_scalar->sc_template)))
    {
        error("array: no template of type %s",
            x->x_scalar->sc_template->s_name);
        return;
    }
    /* style = template_getfloat(scalartemplate, gensym("style"),
            x->x_scalar->sc_vec, 0); */
    filestyle = (x->x_style == PLOTSTYLE_POINTS ? 1 : 
        (x->x_style == PLOTSTYLE_POLY ? 0 : x->x_style)); 

    binbuf_addv(b, "sssisiss;", gensym("#X"), gensym("array"),
        x->x_name, array->a_n, &s_float,
        x->x_saveit + 2 * filestyle + 8*x->x_hidename +
        16 * x->x_joc, x->x_fillcolor, x->x_outlinecolor);
    if (x->x_saveit)
    {
        int n = array->a_n, n2 = 0;
        if (n > 200000)
            post("warning: I'm saving an array with %d points!\n", n);
        while (n2 < n)
        {
            int chunk = n - n2, i;
            if (chunk > ARRAYWRITECHUNKSIZE)
                chunk = ARRAYWRITECHUNKSIZE;
            binbuf_addv(b, "si", gensym("#A"), n2);
            for (i = 0; i < chunk; i++)
                binbuf_addv(b, "f", ((t_word *)(array->a_vec))[n2+i].w_float);
            binbuf_addv(b, ";");
            n2 += chunk;
        }
    }
}

t_widgetbehavior garray_widgetbehavior =
{
    garray_getrect,
    garray_displace,
    garray_select,
    garray_activate,
    garray_delete,
    garray_vis,
    garray_click,
    NULL,
};

/* ----------------------- public functions -------------------- */

void garray_usedindsp(t_garray *x)
{
    x->x_usedindsp = 1;
}

static void garray_doredraw(t_gobj *client, t_glist *glist)
{
    //fprintf(stderr,"garray_doredraw\n");
    t_garray *x = (t_garray *)client;
    if (glist_isvisible(x->x_glist))
    {
        garray_vis(&x->x_gobj, x->x_glist, 0);
        garray_vis(&x->x_gobj, x->x_glist, 1);

        /* we do this to reposition objects back where they belong */
        if (!glist_istoplevel(glist))
        {
            canvas_restore_original_position(glist_getcanvas(glist),
                (t_gobj *)glist, 0, -1);
        }
        //fprintf(stderr,"check if we need to reselect %lx %lx %lx\n",
        //    glist_getcanvas(glist), (t_gobj *)glist, glist->gl_owner);
        int selected = 0;
        /* Unfortunately I forget to comment this. I can't remember why I
           use glist instead of glist->gl_owner (quick bugfix maybe?) */
        //t_glist *sel = glist->gl_owner;
        t_glist *sel = glist;
        while (sel && sel != glist_getcanvas(glist))
        {
            if (glist_isselected(glist_getcanvas(glist), (t_gobj *)sel))
            {
                selected = 1;
                break;
            }
            sel = sel->gl_owner;
        }
        if (selected)
        {
            //fprintf(stderr,"garray_doredraw isselected\n");
            /* I haven't found a case where this is actually needed...
               perhaps with nested GOPs or something? Anyhow if there's
               a regression this commented-out call might be a place
               to investigate... */
            //sys_vgui("pdtk_select_all_gop_widgets .x%lx %lx %d\n",
            //    glist_getcanvas(glist), glist, 1);
            // a giant kludge-- we really just need gop items
            // to be children of their gop <group>
            t_scalar *sc = x->x_scalar;
            char tagbuf[MAXPDSTRING];
            sprintf(tagbuf, "scalar%lx", (long unsigned int)sc->sc_vec);
            gui_vmess("gui_gobj_select", "xs",
                glist_getcanvas(glist), tagbuf);
        }
    }
}

void garray_redraw(t_garray *x)
{
    //fprintf(stderr,"garray_redraw\n");
    if (glist_isvisible(x->x_glist))
        /* enqueueing redraw ensures that the array is drawn after its values
           have been instantiated
           1-24-2015 Ico: reenabled this method to optimize redrawing as per
           Gilberto's report on l2ork-dev list.
           TODO: when adding new array to the newest array it is not always
           drawn, or tabwrite does not write to the right array--see Gilberto's
           example available from:
           http://disis.music.vt.edu/pipermail/l2ork-dev/2015-January/000676.htm
        */
        sys_queuegui(&x->x_gobj, x->x_glist, garray_doredraw);

        /* 1-24-2015 Ico: this however causes painfully slow and inefficient red
           when we use tabwrite which writes one point per array and requests
           redraw after each point is changed. Thus it is deprecated in favor of
           of the approach above */
        //garray_doredraw(&x->x_gobj, x->x_glist);
}

   /* This function gets the template of an array; if we can't figure
   out what template an array's elements belong to we're in grave trouble
   when it's time to free or resize it.  */
t_template *garray_template(t_garray *x)
{
    t_array *array = garray_getarray(x);
    t_template *template = 
        (array ? template_findbyname(array->a_templatesym) : 0);
    if (!template)
        bug("garray_template");
    return (template);
}

int garray_npoints(t_garray *x) /* get the length */
{
    t_array *array = garray_getarray(x);
    return (array->a_n);
}

char *garray_vec(t_garray *x) /* get the contents */
{
    t_array *array = garray_getarray(x);
    return ((char *)(array->a_vec));
}

    /* routine that checks if we're just an array of floats and if
    so returns the goods */

int garray_getfloatwords(t_garray *x, int *size, t_word **vec)
{
    int yonset, elemsize;
    t_array *a = garray_getarray_floatonly(x, &yonset, &elemsize);
    if (!a)
    {
        error("%s: needs floating-point 'y' field", x->x_realname->s_name);
        return (0);
    }
    else if (elemsize != sizeof(t_word))
    {
        error("%s: has more than one field", x->x_realname->s_name);
        return (0);
    }
    *size = garray_npoints(x);
    *vec =  (t_word *)garray_vec(x);
    return (1);
}
    /* older, non-64-bit safe version, supplied for older externs */

int garray_getfloatarray(t_garray *x, int *size, t_float **vec)
{
    if (sizeof(t_word) != sizeof(t_float))
    {
        static int warned;
        if (!warned)
            post(
 "warning: extern using garray_getfloatarray() won't work in 64-bit version");
        warned = 1;
    }
    return (garray_getfloatwords(x, size, (t_word **)vec));
}

    /* set the "saveit" flag */
void garray_setsaveit(t_garray *x, int saveit)
{
    if (x->x_saveit && !saveit)
        post("warning: array %s: clearing save-in-patch flag",
            x->x_name->s_name);
    x->x_saveit = saveit;
}

/*------------------- Pd messages ------------------------ */
static void garray_const(t_garray *x, t_floatarg g)
{
    int yonset, i, elemsize;
    t_array *array = garray_getarray_floatonly(x, &yonset, &elemsize);
    if (!array)
        error("%s: needs floating-point 'y' field", x->x_realname->s_name);
    else for (i = 0; i < array->a_n; i++)
        *((t_float *)((char *)array->a_vec
            + elemsize * i) + yonset) = g;
    garray_redraw(x);
}

    /* sum of Fourier components; called from routines below */
static void garray_dofo(t_garray *x, long npoints, t_float dcval,
    int nsin, t_float *vsin, int sineflag)
{
    double phase, phaseincr, fj;
    int yonset, i, j, elemsize;
    t_array *array = garray_getarray_floatonly(x, &yonset, &elemsize);
    if (!array)
    {
        error("%s: needs floating-point 'y' field", x->x_realname->s_name);
        return;
    }
    if (npoints == 0)
        npoints = 512;  /* dunno what a good default would be... */
    if (npoints != (1 << ilog2(npoints)))
        post("%s: rounding to %d points", array->a_templatesym->s_name,
            (npoints = (1<<ilog2(npoints))));
    garray_resize_long(x, npoints + 3);
    phaseincr = 2. * 3.14159 / npoints;
    for (i = 0, phase = -phaseincr; i < array->a_n; i++, phase += phaseincr)
    {
        double sum = dcval;
        if (sineflag)
            for (j = 0, fj = phase; j < nsin; j++, fj += phase)
                sum += vsin[j] * sin(fj);
        else
            for (j = 0, fj = 0; j < nsin; j++, fj += phase)
                sum += vsin[j] * cos(fj);
        *((t_float *)((array->a_vec + elemsize * i)) + yonset)
            = sum;
    }
    garray_redraw(x);
}

static void garray_sinesum(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{    
    t_float *svec;
    long npoints;
    int i;
    if (argc < 2)
    {
        error("sinesum: %s: need number of points and partial strengths",
            x->x_realname->s_name);
        return;
    }

    npoints = atom_getfloatarg(0, argc, argv);
    argv++, argc--;
    
    svec = (t_float *)t_getbytes(sizeof(t_float) * argc);
    if (!svec) return;
    
    for (i = 0; i < argc; i++)
        svec[i] = atom_getfloatarg(i, argc, argv);
    garray_dofo(x, npoints, 0, argc, svec, 1);
    t_freebytes(svec, sizeof(t_float) * argc);
}

static void garray_cosinesum(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float *svec;
    long npoints;
    int i;
    if (argc < 2)
    {
        error("sinesum: %s: need number of points and partial strengths",
            x->x_realname->s_name);
        return;
    }

    npoints = atom_getfloatarg(0, argc, argv);
    argv++, argc--;
    
    svec = (t_float *)t_getbytes(sizeof(t_float) * argc);
    if (!svec) return;

    for (i = 0; i < argc; i++)
        svec[i] = atom_getfloatarg(i, argc, argv);
    garray_dofo(x, npoints, 0, argc, svec, 0);
    t_freebytes(svec, sizeof(t_float) * argc);
}

static void garray_normalize(t_garray *x, t_float f)
{
    int i;
    double maxv, renormer;
    int yonset, elemsize;
    t_array *array = garray_getarray_floatonly(x, &yonset, &elemsize);
    if (!array)
    {
        error("%s: needs floating-point 'y' field", x->x_realname->s_name);
        return;
    }

    if (f <= 0)
        f = 1;

    for (i = 0, maxv = 0; i < array->a_n; i++)
    {
        double v = *((t_float *)(array->a_vec + elemsize * i)
            + yonset);
        if (v > maxv)
            maxv = v;
        if (-v > maxv)
            maxv = -v;
    }
    if (maxv > 0)
    {
        renormer = f / maxv;
        for (i = 0; i < array->a_n; i++)
            *((t_float *)(array->a_vec + elemsize * i) + yonset)
                *= renormer;
    }
    garray_redraw(x);
}

    /* list -- the first value is an index; subsequent values are put in
    the "y" slot of the array.  This generalizes Max's "table", sort of. */
static void garray_list(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    int yonset, elemsize;
    t_array *array = garray_getarray_floatonly(x, &yonset, &elemsize);
    if (!array)
    {
        error("%s: needs floating-point 'y' field", x->x_realname->s_name);
        return;
    }
    if (argc < 2) return;
    else
    {
        int firstindex = atom_getfloat(argv);
        argc--;
        argv++;
            /* drop negative x values */
        if (firstindex < 0)
        {
            argc += firstindex;
            argv -= firstindex;
            firstindex = 0;
            if (argc <= 0) return;
        }
        if (argc + firstindex > array->a_n)
        {
            argc = array->a_n - firstindex;
            if (argc <= 0) return;
        }
        for (i = 0; i < argc; i++)
            *((t_float *)(array->a_vec + elemsize * (i + firstindex)) + yonset)
                = atom_getfloat(argv + i);
    }
    garray_redraw(x);
}

    /* forward a "bounds" message to the owning graph */
static void garray_bounds(t_garray *x, t_floatarg x1, t_floatarg y1,
    t_floatarg x2, t_floatarg y2)
{
    vmess(&x->x_glist->gl_pd, gensym("bounds"), "ffff", x1, y1, x2, y2);
}

    /* same for "xticks", etc */
static void garray_xticks(t_garray *x,
    t_floatarg point, t_floatarg inc, t_floatarg f)
{
    vmess(&x->x_glist->gl_pd, gensym("xticks"), "fff", point, inc, f);
}

static void garray_yticks(t_garray *x,
    t_floatarg point, t_floatarg inc, t_floatarg f)
{
    vmess(&x->x_glist->gl_pd, gensym("yticks"), "fff", point, inc, f);
}

static void garray_xlabel(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    typedmess(&x->x_glist->gl_pd, s, argc, argv);
}

static void garray_ylabel(t_garray *x, t_symbol *s, int argc, t_atom *argv)
{
    typedmess(&x->x_glist->gl_pd, s, argc, argv);
}
    /* change the name of a garray. */
static void garray_rename(t_garray *x, t_symbol *s)
{
    pd_unbind(&x->x_gobj.g_pd, x->x_realname);

    x->x_name = s;
    x->x_realname = canvas_realizedollar(x->x_glist, x->x_name);

    pd_bind(&x->x_gobj.g_pd, x->x_realname);

    char buf[MAXPDSTRING];
    sprintf(buf, "%s_changed", x->x_realname->s_name);
    x->x_send = gensym(buf);

    //garray_redraw(x);
    glist_redraw(x->x_glist);
}

static void garray_read(t_garray *x, t_symbol *filename)
{
    int nelem, filedesc, i;
    FILE *fd;
    char buf[FILENAME_MAX], *bufptr;
    int yonset, elemsize;
    t_array *array = garray_getarray_floatonly(x, &yonset, &elemsize);
    if (!array)
    {
        error("%s: needs floating-point 'y' field", x->x_realname->s_name);
        return;
    }
    nelem = array->a_n;
    if ((filedesc = canvas_open(glist_getcanvas(x->x_glist),
            filename->s_name, "", buf, &bufptr, FILENAME_MAX, 0)) < 0 
                || !(fd = fdopen(filedesc, "r")))
    {
        error("%s: can't open", filename->s_name);
        return;
    }
    for (i = 0; i < nelem; i++)
    {
        float f;
        if (!fscanf(fd, "%f", &f))
        {
            post("%s: read %d elements into table of size %d",
                filename->s_name, i, nelem);
            break;
        }
        else *((t_float *)(array->a_vec + elemsize * i) + yonset) = f;
    }
    while (i < nelem)
        *((t_float *)(array->a_vec +
            elemsize * i) + yonset) = 0, i++;
    fclose(fd);
    garray_redraw(x);
}

static void garray_write(t_garray *x, t_symbol *filename)
{
    FILE *fd;
    char buf[MAXPDSTRING];
    int yonset, elemsize, i;
    t_array *array = garray_getarray_floatonly(x, &yonset, &elemsize);
    if (!array)
    {
        error("%s: needs floating-point 'y' field", x->x_realname->s_name);
        return;
    }
    canvas_makefilename(glist_getcanvas(x->x_glist), filename->s_name,
        buf, MAXPDSTRING);
    sys_bashfilename(buf, buf);
    if (!(fd = fopen(buf, "w")))
    {
        error("%s: can't create", buf);
        return;
    }
    for (i = 0; i < array->a_n; i++)
    {
        if (fprintf(fd, "%g\n",
            *(t_float *)(((array->a_vec + sizeof(t_word) * i)) + yonset)) < 1)
        {
            post("%s: write error", filename->s_name);
            break;
        }
    }
    fclose(fd);
}


    /* this should be renamed and moved... */
int garray_ambigendian(void)
{
    unsigned short s = 1;
    unsigned char c = *(char *)(&s);
    return (c==0);
}

void garray_resize_long(t_garray *x, long n)
{
    t_array *array = garray_getarray(x);
    if (n < 1)
        n = 1;
    //fprintf(stderr,"garray_resize %d\n", n);
    array_resize(array, n);
    garray_fittograph(x, n, 1);/*template_getfloat(
        template_findbyname(x->x_scalar->sc_template),
            gensym("style"), x->x_scalar->sc_vec, 1));*/
    //array_resize_and_redraw(array, x->x_glist, n);
    if (x->x_usedindsp)
        canvas_update_dsp();
}

    /* float version to use as Pd method */
void garray_resize(t_garray *x, t_floatarg f)
{
    garray_resize_long(x, f);
}

static void garray_print(t_garray *x)
{
    t_array *array = garray_getarray(x);
    post("garray %s: template %s, length %d",
        x->x_realname->s_name, array->a_templatesym->s_name, array->a_n);
}

void g_array_setup(void)
{
    garray_class = class_new(gensym("array"), 0, (t_method)garray_free,
        sizeof(t_garray), CLASS_GOBJ, 0);
    class_setwidget(garray_class, &garray_widgetbehavior);
    class_addmethod(garray_class, (t_method)garray_const, gensym("const"),
        A_DEFFLOAT, A_NULL);
    class_addlist(garray_class, garray_list);
    class_addmethod(garray_class, (t_method)garray_bounds, gensym("bounds"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(garray_class, (t_method)garray_xticks, gensym("xticks"),
        A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(garray_class, (t_method)garray_xlabel, gensym("xlabel"),
        A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_yticks, gensym("yticks"),
        A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(garray_class, (t_method)garray_ylabel, gensym("ylabel"),
        A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_rename, gensym("rename"),
        A_SYMBOL, 0);
    class_addmethod(garray_class, (t_method)garray_read, gensym("read"),
        A_SYMBOL, A_NULL);
    class_addmethod(garray_class, (t_method)garray_write, gensym("write"),
        A_SYMBOL, A_NULL);
    class_addmethod(garray_class, (t_method)garray_resize, gensym("resize"),
        A_FLOAT, A_NULL);
    class_addmethod(garray_class, (t_method)garray_print, gensym("print"),
        A_NULL);
    class_addmethod(garray_class, (t_method)garray_sinesum, gensym("sinesum"),
        A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_cosinesum,
        gensym("cosinesum"), A_GIMME, 0);
    class_addmethod(garray_class, (t_method)garray_normalize,
        gensym("normalize"), A_DEFFLOAT, 0);
    class_addmethod(garray_class, (t_method)garray_arraydialog,
        gensym("arraydialog"), A_GIMME, 0);
    class_setsavefn(garray_class, garray_save);
}
