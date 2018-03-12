/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdlib.h>
#include <string.h>
#include "m_pd.h"
#include "m_imp.h"

#include "g_canvas.h"
#include <stdio.h>

    /* FIXME no out-of-memory testing yet! */

/*int canvas_check_duplicate = 0;

extern t_redundant_mem *rm_start;
extern t_redundant_mem *rm_end;
*/

t_pd *pd_new(t_class *c)
{
    t_pd *x = NULL;
    if (!c) 
        bug ("pd_new: apparently called before setup routine");

    x = (t_pd *)t_getbytes(c->c_size);
    *x = c;
    if (c->c_patchable)
    {
        ((t_object *)x)->ob_inlet = 0;
        ((t_object *)x)->ob_outlet = 0;
    }
    return (x);
}

void pd_free(t_pd *x)
{
    t_class *c = *x;
    if (c->c_freemethod) (*(t_gotfn)(c->c_freemethod))(x);
    if (c->c_patchable)
    {
        while (((t_object *)x)->ob_outlet)
            outlet_free(((t_object *)x)->ob_outlet);
        while (((t_object *)x)->ob_inlet)
            inlet_free(((t_object *)x)->ob_inlet);
        if (((t_object *)x)->ob_binbuf)
            binbuf_free(((t_object *)x)->ob_binbuf);
    }
    if (c->c_size) t_freebytes(x, c->c_size);
}

void gobj_save(t_gobj *x, t_binbuf *b)
{
    t_class *c = x->g_pd;
    if (c->c_savefn)
        (c->c_savefn)(x, b);
}

/* deal with several objects bound to the same symbol.  If more than one, we
actually bind a collection object to the symbol, which forwards messages sent
to the symbol. */

static t_class *bindlist_class;

typedef struct _bindelem
{
    t_pd *e_who;
    struct _bindelem *e_next;
    int e_delayed_free;
} t_bindelem;

typedef struct _bindlist
{
    t_pd b_pd;
    t_bindelem *b_list;
} t_bindlist;

static int change_bindlist_via_graph = 0;

static void bindlist_cleanup(t_bindlist *x)
{
    //fprintf(stderr,"bindlist_cleanup\n");
    t_bindelem *e, *e2;
    if (x->b_list->e_delayed_free == 1)
    {
        e = x->b_list;
        x->b_list = e->e_next;
        freebytes(e, sizeof(t_bindelem));
        //fprintf(stderr,"success B1a\n");
    }
    for (e = x->b_list; e2 = e->e_next; e = e2)
        if (e2->e_delayed_free == 1)
    {
        e->e_next = e2->e_next;
        freebytes(e2, sizeof(t_bindelem));
        //fprintf(stderr,"success B1b\n");
        break;
    }
    if (!x->b_list->e_next)
    {
        freebytes(x->b_list, sizeof(t_bindelem));
        pd_free(&x->b_pd);
        //fprintf(stderr,"success B2\n");
    }
}

static void bindlist_bang(t_bindlist *x)
{
    t_bindelem *e;
    int save = change_bindlist_via_graph;
    change_bindlist_via_graph = 1;
    for (e = x->b_list; e; e = e->e_next)
        if (e->e_who != NULL) pd_bang(e->e_who);
    if (change_bindlist_via_graph > 1)
        bindlist_cleanup(x);
    change_bindlist_via_graph = save;
}

static void bindlist_float(t_bindlist *x, t_float f)
{
    t_bindelem *e;
    int save = change_bindlist_via_graph;
    change_bindlist_via_graph = 1;
    for (e = x->b_list; e; e = e->e_next)
        if (e->e_who != NULL) pd_float(e->e_who, f);
    if (change_bindlist_via_graph > 1)
        bindlist_cleanup(x);
    change_bindlist_via_graph = save;
}

static void bindlist_symbol(t_bindlist *x, t_symbol *s)
{
    t_bindelem *e;
    int save = change_bindlist_via_graph;
    change_bindlist_via_graph = 1;
    for (e = x->b_list; e; e = e->e_next)
        if (e->e_who != NULL) pd_symbol(e->e_who, s);
    if (change_bindlist_via_graph > 1)
        bindlist_cleanup(x);
    change_bindlist_via_graph = save;
}

static void bindlist_pointer(t_bindlist *x, t_gpointer *gp)
{
    t_bindelem *e;
    int save = change_bindlist_via_graph;
    change_bindlist_via_graph = 1;
    for (e = x->b_list; e; e = e->e_next)
        if (e->e_who != NULL) pd_pointer(e->e_who, gp);
    if (change_bindlist_via_graph > 1)
        bindlist_cleanup(x);
    change_bindlist_via_graph = save;
}

static void bindlist_list(t_bindlist *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_bindelem *e;
    int save = change_bindlist_via_graph;
    change_bindlist_via_graph = 1;
    for (e = x->b_list; e; e = e->e_next)
        if (e->e_who != NULL) pd_list(e->e_who, s, argc, argv);
    if (change_bindlist_via_graph > 1)
        bindlist_cleanup(x);
    change_bindlist_via_graph = save;
}

static void bindlist_anything(t_bindlist *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_bindelem *e;
    int save = change_bindlist_via_graph;
    change_bindlist_via_graph = 1;
    for (e = x->b_list; e; e = e->e_next)
        if (e->e_who != NULL) pd_typedmess(e->e_who, s, argc, argv);
    if (change_bindlist_via_graph > 1)
        bindlist_cleanup(x);
    change_bindlist_via_graph = save;
}

void m_pd_setup(void)
{
    bindlist_class = class_new(gensym("bindlist"), 0, 0,
        sizeof(t_bindlist), CLASS_PD, 0);
    class_addbang(bindlist_class, bindlist_bang);
    class_addfloat(bindlist_class, (t_method)bindlist_float);
    class_addsymbol(bindlist_class, bindlist_symbol);
    class_addpointer(bindlist_class, bindlist_pointer);
    class_addlist(bindlist_class, bindlist_list);
    class_addanything(bindlist_class, bindlist_anything);
}

void pd_bind(t_pd *x, t_symbol *s)
{
    //fprintf(stderr,"pd_bind %s\n", s->s_name);
    if (s->s_thing)
    {
        if (*s->s_thing == bindlist_class)
        {
            //fprintf(stderr,"    pd_bind option 1A %lx\n", (t_int)x);
            t_bindlist *b = (t_bindlist *)s->s_thing;
            t_bindelem *e = (t_bindelem *)getbytes(sizeof(t_bindelem));
            e->e_next = b->b_list;
            e->e_who = x;
            e->e_delayed_free = 0;
            b->b_list = e;
        }
        else
        {
            //fprintf(stderr,"    pd_bind option 1B %lx\n", (t_int)x);
            t_bindlist *b = (t_bindlist *)pd_new(bindlist_class);
            t_bindelem *e1 = (t_bindelem *)getbytes(sizeof(t_bindelem));
            t_bindelem *e2 = (t_bindelem *)getbytes(sizeof(t_bindelem));
            b->b_list = e1;
            e1->e_who = x;
            e1->e_next = e2;
            e1->e_delayed_free = 0;
            e2->e_who = s->s_thing;
            e2->e_next = 0;
            e2->e_delayed_free = 0;
            s->s_thing = &b->b_pd;
        }
    }
    else {
        //fprintf(stderr,"pd_bind option 2 %lx\n", (t_int)x);
        s->s_thing = x;
    }
}

void pd_unbind(t_pd *x, t_symbol *s)
{
    //fprintf(stderr,"pd_unbind %s\n", s->s_name);
    if (s->s_thing == x) {
        //fprintf(stderr,"    pd_unbind option A %lx\n", (t_int)x);
        s->s_thing = 0;
    }
    else if (s->s_thing && *s->s_thing == bindlist_class)
    {
        /* bindlists always have at least two elements... if the number
           goes down to one, get rid of the bindlist and bind the symbol
           straight to the remaining element. */

        /* in pd-l2ork, we however also check whether changes to the bindlist
           occur via graph (through code execution, e.g. dynamic change of
           receives) and if so, we do not deallocate memory until the entire
           bindlist_<datatype> function is complete with its execution, after
           which we call bindlist_cleanup(). we control the execution via
           static int variable change_bindlist_via_graph */

        //fprintf(stderr,"    pd_unbind option B %lx\n", (t_int)x);

        t_bindlist *b = (t_bindlist *)s->s_thing;
        t_bindelem *e, *e2;
        if ((e = b->b_list)->e_who == x)
        {
            if (change_bindlist_via_graph)
            {
                change_bindlist_via_graph++;
                e->e_delayed_free = 1;
            }
            else
            {
                b->b_list = e->e_next;
                freebytes(e, sizeof(t_bindelem));
            }
            //fprintf(stderr,"    success B1a %d\n", e->e_delayed_free);
        }
        else for (e = b->b_list; e2 = e->e_next; e = e2)
        {
            if (e2->e_who == x)
            {
                if (change_bindlist_via_graph)
                {
                    change_bindlist_via_graph++;
                    e2->e_delayed_free = 1;
                }
                else
                {
                    e->e_next = e2->e_next;
                    freebytes(e2, sizeof(t_bindelem));
                }
                //fprintf(stderr,"    success B1b %d\n", e->e_delayed_free);
                break;
            }
        }

        int count_valid = 0;
        t_bindelem *e1 = NULL;
        for (e = b->b_list; e; e = e->e_next)
        {
            if (e->e_who != NULL && !e->e_delayed_free)
            {
                count_valid++;
                e1 = e;
            }

        }
        if (count_valid == 1)
        {
            s->s_thing = e1->e_who;
            if (!change_bindlist_via_graph)
            {
                freebytes(b->b_list, sizeof(t_bindelem));
                pd_free(&b->b_pd);
            }
            //fprintf(stderr,"success B2\n");
        }
    }
    else pd_error(x, "%s: couldn't unbind", s->s_name);
}

void zz(void) {}

t_pd *pd_findbyclass(t_symbol *s, t_class *c)
{
    t_pd *x = 0;
    
    //fprintf(stderr,"pd_findbyclass\n");
    if (!s->s_thing) return (0);
    if (*s->s_thing == c) return (s->s_thing);
    if (*s->s_thing == bindlist_class)
    {
        t_bindlist *b = (t_bindlist *)s->s_thing;
        t_bindelem *e;
        int warned = 0;
        for (e = b->b_list; e; e = e->e_next)
        {
            //if (e->e_who != NULL && *e->e_who == c)
            //fprintf(stderr, "(e_who == c)?%d || e->e_delayed_free=%d\n", (*e->e_who == c ? 1 : 0), e->e_delayed_free);
            if (e->e_delayed_free != 1 && *e->e_who == c)
            {
                //fprintf(stderr,"...found %lx", e);
                if (x && !warned)
                {
                    zz();
                    post("warning: %s: multiply defined", s->s_name);
                    warned = 1;
                }
                x = e->e_who;
            }
        }
    }
    //fprintf(stderr,"====\n");
    return x;
}

/* stack for maintaining bindings for the #X symbol during nestable loads.
*/

typedef struct _gstack
{
    t_pd *g_what;
    t_symbol *g_loadingabstraction;
    struct _gstack *g_next;
} t_gstack;

static t_gstack *gstack_head = 0;
static t_pd *lastpopped;
static t_symbol *pd_loadingabstraction;

int pd_setloadingabstraction(t_symbol *sym)
{
    t_gstack *foo = gstack_head;
    for (foo = gstack_head; foo; foo = foo->g_next)
        if (foo->g_loadingabstraction == sym)
            return (1);
    pd_loadingabstraction = sym;
    return (0);
}

void pd_pushsym(t_pd *x)
{
    t_gstack *y = (t_gstack *)t_getbytes(sizeof(*y));
    y->g_what = s__X.s_thing;
    y->g_next = gstack_head;
    y->g_loadingabstraction = pd_loadingabstraction;
    pd_loadingabstraction = 0;
    gstack_head = y;
    s__X.s_thing = x;
}

extern int abort_when_pasting_from_external_buffer;

void pd_popsym(t_pd *x)
{
    if (!gstack_head || s__X.s_thing != x)
    {
        abort_when_pasting_from_external_buffer = 1;
        bug("gstack_pop");
    }
    else
    {
        t_gstack *headwas = gstack_head;
        s__X.s_thing = headwas->g_what;
        gstack_head = headwas->g_next;
        t_freebytes(headwas, sizeof(*headwas));
        lastpopped = x;
    }
}

void pd_doloadbang(void)
{
    if (lastpopped)
    {
        pd_vmess(lastpopped, gensym("loadbang"), "f", LB_LOAD);
    }
    lastpopped = 0;
}

void pd_bang(t_pd *x)
{
    (*(*x)->c_bangmethod)(x);
}

void pd_float(t_pd *x, t_float f)
{
    (*(*x)->c_floatmethod)(x, f);
}

void pd_pointer(t_pd *x, t_gpointer *gp)
{
    (*(*x)->c_pointermethod)(x, gp);
}

void pd_symbol(t_pd *x, t_symbol *s)
{
    (*(*x)->c_symbolmethod)(x, s);
}

void pd_blob(t_pd *x, t_blob *st) /* MP20061226 blob type */
{
    /*post("pd_blob: st %p length %lu (*x)->c_blobmethod %p", st, st->s_length, (*x)->c_blobmethod);*/
    (*(*x)->c_blobmethod)(x, st);
}

void pd_list(t_pd *x, t_symbol *s, int argc, t_atom *argv)
{
    (*(*x)->c_listmethod)(x, &s_list, argc, argv);
}

void mess_init(void);
void obj_init(void);
void conf_init(void);
void glob_init(void);
void garray_init(void);

t_pdinstance *pd_this;

static t_symbol *midi_gensym(const char *prefix, const char *name)
{
    char buf[80];
    strncpy(buf, prefix, 79);
    buf[79] = 0;
    buf[79] = 0;
    strncat(buf, name, 79 - strlen(buf));
    return (gensym(buf));
}

static t_pdinstance *pdinstance_donew(int useprefix)
{
    t_pdinstance *x = (t_pdinstance *)getbytes(sizeof(t_pdinstance));
    char midiprefix[80];
    if (useprefix)
        sprintf(midiprefix, "%p", x);
    else midiprefix[0] = 0;
    x->pd_systime = 0;
    x->pd_clock_setlist = 0;
    x->pd_dspchain = 0;
    x->pd_dspchainsize = 0;
    x->pd_canvaslist = 0;
    x->pd_dspstate = 0;
    x->pd_dspstate_user = 0;
    x->pd_midiin_sym = midi_gensym(midiprefix, "#midiin");
    x->pd_sysexin_sym = midi_gensym(midiprefix, "#sysexin");
    x->pd_notein_sym = midi_gensym(midiprefix, "#notein");
    x->pd_ctlin_sym = midi_gensym(midiprefix, "#ctlin");
    x->pd_pgmin_sym = midi_gensym(midiprefix, "#pgmin");
    x->pd_bendin_sym = midi_gensym(midiprefix, "#bendin");
    x->pd_touchin_sym = midi_gensym(midiprefix, "#touchin");
    x->pd_polytouchin_sym = midi_gensym(midiprefix, "#polytouchin");
    x->pd_midirealtimein_sym = midi_gensym(midiprefix, "#midirealtimein");
    return (x);
}

EXTERN t_pdinstance *pdinstance_new(void)
{
    return (pdinstance_donew(1));
}

void pd_init(void)
{
    if (!pd_this)
        pd_this = pdinstance_donew(0);
    mess_init();
    obj_init();
    conf_init();
    glob_init();
    garray_init();
}

EXTERN void pd_setinstance(t_pdinstance *x)
{
    pd_this = x;
}

EXTERN void pdinstance_free(t_pdinstance *x)
{
    /* placeholder - LATER free symtab, dsp chain, classes and canvases */
}

EXTERN t_canvas *pd_getcanvaslist(void)
{
    return (pd_this->pd_canvaslist);
}

EXTERN int pd_getdspstate(void)
{
    return (pd_this->pd_dspstate);
}
