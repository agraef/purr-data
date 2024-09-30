/* Copyright (c) 1997- Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "config.h"

#include "m_pd.h"
#include "s_stuff.h"
#include <string.h>

#if defined HAVE_MALLOC_H || defined MSW
#include <malloc.h>
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

extern t_pd *newest;

#ifndef HAVE_ALLOCA     /* can work without alloca() but we never need it */
#define HAVE_ALLOCA 1
#endif

#define LIST_NGETBYTE 100 /* bigger that this we use alloc, not alloca */

/* the "list" object family.

    list append - append a list to another
    list prepend - prepend a list to another
    list split - first n elements to first outlet, rest to second outlet
    list trim - trim off "list" selector
    list length - output number of items in list
    list fromsymbol - "explode" a symbol into a list of character codes
    list cat - build a list by accumulating elements

Need to think more about:
    list foreach - spit out elements of a list one by one (also in reverse?)
    list array - get items from a named array as a list
    list reverse - permute elements of a list back to front
    list pack - synonym for 'pack'
    list unpack - synonym for 'unpack'

Probably don't need:
    list first - output first n elements.
    list last - output last n elements
    list nth - nth item in list, counting from zero
*/

/* -------------- utility functions: storage, copying  -------------- */

#if HAVE_ALLOCA
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)((n) < LIST_NGETBYTE ?  \
        alloca((n) * sizeof(t_atom)) : getbytes((n) * sizeof(t_atom))))
#define ATOMS_FREEA(x, n) ( \
    ((n) < LIST_NGETBYTE || (freebytes((x), (n) * sizeof(t_atom)), 0)))
#else
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)getbytes((n) * sizeof(t_atom)))
#define ATOMS_FREEA(x, n) (freebytes((x), (n) * sizeof(t_atom)))
#endif

void atoms_copy(int argc, t_atom *from, t_atom *to)
{
    int i;
    for (i = 0; i < argc; i++)
        to[i] = from[i];
}

/* ------------- fake class to divert inlets to ----------------- */

t_class *alist_class;

void alist_init(t_alist *x)
{
    x->l_pd = alist_class;
    x->l_n = x->l_npointer = 0;
    x->l_vec = 0;
}

void alist_clear(t_alist *x)
{
    int i;
    for (i = 0; i < x->l_n; i++)
    {
        if (x->l_vec[i].l_a.a_type == A_POINTER)
            gpointer_unset(x->l_vec[i].l_a.a_w.w_gpointer);
    }
    if (x->l_vec)
        freebytes(x->l_vec, x->l_n * sizeof(*x->l_vec));
}

static void alist_copyin(t_alist *x, t_symbol *s, int argc, t_atom *argv,
    int where)
{
    int i, j;
    for (i = 0, j = where; i < argc; i++, j++)
    {
        x->l_vec[j].l_a = argv[i];
        if (x->l_vec[j].l_a.a_type == A_POINTER)
        {
            x->l_npointer++;
            gpointer_copy(x->l_vec[j].l_a.a_w.w_gpointer, &x->l_vec[j].l_p);
            x->l_vec[j].l_a.a_w.w_gpointer = &x->l_vec[j].l_p;
        }
    }
}

    /* set contents to a list */
void alist_list(t_alist *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    alist_clear(x);
    if (!(x->l_vec = (t_listelem *)getbytes(argc * sizeof(*x->l_vec))))
    {
        x->l_n = 0;
        error("list_alloc: out of memory");
        return;
    }
    x->l_n = argc;
    x->l_npointer = 0;
    for (i = 0; i < argc; i++)
    {
        x->l_vec[i].l_a = argv[i];
        if (x->l_vec[i].l_a.a_type == A_POINTER)
        {
            x->l_npointer++;
            gpointer_copy(x->l_vec[i].l_a.a_w.w_gpointer, &x->l_vec[i].l_p);
            x->l_vec[i].l_a.a_w.w_gpointer = &x->l_vec[i].l_p;
        }
    }
}

    /* set contents to an arbitrary non-list message */
void alist_anything(t_alist *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    alist_clear(x);
    if (!(x->l_vec = (t_listelem *)getbytes((argc+1) * sizeof(*x->l_vec))))
    {
        x->l_n = 0;
        error("list_alloc: out of memory");
        return;
    }
    x->l_n = argc+1;
    x->l_npointer = 0;
    SETSYMBOL(&x->l_vec[0].l_a, s);
    for (i = 0; i < argc; i++)
    {
        x->l_vec[i+1].l_a = argv[i];
        if (x->l_vec[i+1].l_a.a_type == A_POINTER)
        {
            x->l_npointer++;            
            gpointer_copy(x->l_vec[i+1].l_a.a_w.w_gpointer, &x->l_vec[i+1].l_p);
            x->l_vec[i+1].l_a.a_w.w_gpointer = &x->l_vec[i+1].l_p;
        }
    }
}

void alist_toatoms(t_alist *x, t_atom *to, int onset, int count)
{
    int i;
    for (i = 0; i < count; i++)
        to[i] = x->l_vec[onset + i].l_a;
}

void alist_clone(t_alist *x, t_alist *y, int onset, int count)
{
    int i;
    y->l_pd = alist_class;
    y->l_n = count;
    y->l_npointer = 0;
    if (!(y->l_vec = (t_listelem *)getbytes(y->l_n * sizeof(*y->l_vec))))
    {
        y->l_n = 0;
        error("list_alloc: out of memory");
    }
    else for (i = 0; i < count; i++)
    {
        y->l_vec[i].l_a = x->l_vec[onset + i].l_a;
        if (y->l_vec[i].l_a.a_type == A_POINTER)
        {
            gpointer_copy(y->l_vec[i].l_a.a_w.w_gpointer, &y->l_vec[i].l_p);
            y->l_vec[i].l_a.a_w.w_gpointer = &y->l_vec[i].l_p;
            y->l_npointer++;
        }
    }
}

void alist_setup(void)
{
    alist_class = class_new(gensym("list inlet"),
        0, 0, sizeof(t_alist), 0, 0);
    class_addlist(alist_class, alist_list);
    class_addanything(alist_class, alist_anything);
}

/* ------------- list append --------------------- */

t_class *list_append_class;

typedef struct _list_append
{
    t_object x_obj;
    t_alist x_alist;
} t_list_append;

static void *list_append_new(t_symbol *s, int argc, t_atom *argv)
{
    t_list_append *x = (t_list_append *)pd_new(list_append_class);
    alist_init(&x->x_alist);
    alist_list(&x->x_alist, 0, argc, argv);
    outlet_new(&x->x_obj, &s_list);
    inlet_new(&x->x_obj, &x->x_alist.l_pd, 0, 0);
    return (x);
}

static void list_append_list(t_list_append *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    int n, outc;
    n = x->x_alist.l_n;
    outc = n + argc;
    XL_ATOMS_ALLOCA(outv, outc);
    atoms_copy(argc, argv, outv);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, outv+argc, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv+argc, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    }
    XL_ATOMS_FREEA(outv, outc);
}

static void list_append_anything(t_list_append *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    int n, outc;
    n = x->x_alist.l_n;
    outc = n + argc + 1;
    XL_ATOMS_ALLOCA(outv, outc);
    SETSYMBOL(outv, s);
    atoms_copy(argc, argv, outv + 1);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, outv + 1 + argc, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv + 1 + argc, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    }
    XL_ATOMS_FREEA(outv, outc);
}

static void list_append_free(t_list_append *x)
{
    alist_clear(&x->x_alist);
}

static void list_append_setup(void)
{
    list_append_class = class_new(gensym("list append"),
        (t_newmethod)list_append_new, (t_method)list_append_free,
        sizeof(t_list_append), 0, A_GIMME, 0);
    class_addlist(list_append_class, list_append_list);
    class_addanything(list_append_class, list_append_anything);
    class_sethelpsymbol(list_append_class, &s_list);
}

/* ------------- list cat --------------------- */

t_class *list_cat_class;
t_class *list_cat_proxy_class;

typedef struct _list_cat_proxy
{
    t_pd l_pd;
    void *parent;
} t_list_cat_proxy;

typedef struct _list_cat
{
    t_object x_obj;
    t_alist x_alist;
    t_list_cat_proxy x_pxy;
} t_list_cat;

static void list_cat_clear(t_list_cat *x);

static void list_cat_proxy_init(t_list_cat_proxy *x, t_list_cat *p)
{
    x->l_pd = list_cat_proxy_class;
    x->parent = (void *)p;
}

static void list_cat_proxy_clear(t_list_cat_proxy *x)
{
    t_list_cat *p = (t_list_cat *)x->parent;
    list_cat_clear(p);
}

static void *list_cat_new( void)
{
    t_list_cat *x = (t_list_cat *)pd_new(list_cat_class);
    alist_init(&x->x_alist);
    outlet_new(&x->x_obj, &s_list);
    list_cat_proxy_init(&x->x_pxy, x);
    inlet_new(&x->x_obj, &x->x_pxy.l_pd, 0, 0);
    return (x);
}

static void list_cat_list(t_list_cat *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    int n, outc;
    n = x->x_alist.l_n;
    outc = n + argc;
    XL_ATOMS_ALLOCA(outv, outc);
    atoms_copy(argc, argv, outv + x->x_alist.l_n);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, outv, 0, n);
        alist_list(&x->x_alist, s, outc, outv);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv, 0, n);
        alist_list(&x->x_alist, s, outc, outv);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    }
    XL_ATOMS_FREEA(outv, outc);
}

static void list_cat_anything(t_list_cat *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    int n, outc;
    n = x->x_alist.l_n;
    outc = n + argc + 1;
    XL_ATOMS_ALLOCA(outv, outc);
    SETSYMBOL(outv + x->x_alist.l_n, s);
    atoms_copy(argc, argv, outv + x->x_alist.l_n + 1);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, outv, 0, n);
        alist_list(&x->x_alist, s, outc, outv);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv, 0, n);
        alist_list(&x->x_alist, s, outc, outv);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    }
    XL_ATOMS_FREEA(outv, outc);
}

static void list_cat_clear(t_list_cat *x)
{
    alist_clear(&x->x_alist);
    alist_init(&x->x_alist);
}

static void list_cat_free(t_list_cat *x)
{
    alist_clear(&x->x_alist);
}

static void list_cat_setup(void)
{
    list_cat_class = class_new(gensym("list cat"),
        (t_newmethod)list_cat_new, (t_method)list_cat_free,
        sizeof(t_list_cat), 0, 0);
    class_addlist(list_cat_class, list_cat_list);
    class_addanything(list_cat_class, list_cat_anything);
    class_sethelpsymbol(list_cat_class, &s_list);

    list_cat_proxy_class = class_new(gensym("list cat pxy"), 0, 0,
        sizeof(t_list_cat_proxy), 0, 0);
    class_addmethod(list_cat_proxy_class, (t_method)list_cat_proxy_clear,
        gensym("clear"), 0);
}

/* ------------- list prepend --------------------- */

t_class *list_prepend_class;

typedef struct _list_prepend
{
    t_object x_obj;
    t_alist x_alist;
} t_list_prepend;

static void *list_prepend_new(t_symbol *s, int argc, t_atom *argv)
{
    t_list_prepend *x = (t_list_prepend *)pd_new(list_prepend_class);
    alist_init(&x->x_alist);
    alist_list(&x->x_alist, 0, argc, argv);
    outlet_new(&x->x_obj, &s_list);
    inlet_new(&x->x_obj, &x->x_alist.l_pd, 0, 0);
    return (x);
}

static void list_prepend_list(t_list_prepend *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    int n, outc;
    n = x->x_alist.l_n;
    outc = n + argc;
    XL_ATOMS_ALLOCA(outv, outc);
    atoms_copy(argc, argv, outv + n);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, outv, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    }
    XL_ATOMS_FREEA(outv, outc);
}



static void list_prepend_anything(t_list_prepend *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    int n, outc;
    n = x->x_alist.l_n;
    outc = n + argc + 1;
    XL_ATOMS_ALLOCA(outv, outc);
    SETSYMBOL(outv + n, s);
    atoms_copy(argc, argv, outv + n + 1);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, outv, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv, 0, n);
        outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    }
    XL_ATOMS_FREEA(outv, outc);
}

static void list_prepend_free(t_list_prepend *x)
{
    alist_clear(&x->x_alist);
}

static void list_prepend_setup(void)
{
    list_prepend_class = class_new(gensym("list prepend"),
        (t_newmethod)list_prepend_new, (t_method)list_prepend_free,
        sizeof(t_list_prepend), 0, A_GIMME, 0);
    class_addlist(list_prepend_class, list_prepend_list);
    class_addanything(list_prepend_class, list_prepend_anything);
    class_sethelpsymbol(list_prepend_class, &s_list);


}

/* ------------- list store --------------------- */

t_class *list_store_class;

typedef struct _list_store
{
    t_object x_obj;
    t_alist x_alist;
    t_outlet *x_out1;
    t_outlet *x_out2;
} t_list_store;

static void *list_store_new(t_symbol *s, int argc, t_atom *argv)
{
    t_list_store *x = (t_list_store *)pd_new(list_store_class);
    alist_init(&x->x_alist);
    alist_list(&x->x_alist, 0, argc, argv);
    x->x_out1 = outlet_new(&x->x_obj, &s_list);
    x->x_out2 = outlet_new(&x->x_obj, &s_bang);
    inlet_new(&x->x_obj, &x->x_alist.l_pd, 0, 0);
    return (x);
}

static void list_store_send(t_list_store *x, t_symbol *s)
{
    t_atom *vec;
    int n = x->x_alist.l_n;
    if (!s->s_thing)
    {
        pd_error(x, "%s: no such object", s->s_name);
        return;
    }
    ATOMS_ALLOCA(vec, n);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, vec, 0, n);
        pd_list(s->s_thing, gensym("list"), n, vec);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, vec, 0, n);
        pd_list(s->s_thing, gensym("list"), n, vec);
    }
    ATOMS_FREEA(vec, n);
}

static void list_store_list(t_list_store *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    int n, outc;
    n = x->x_alist.l_n;
    outc = n + argc;
    ATOMS_ALLOCA(outv, outc);
    atoms_copy(argc, argv, outv);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, 0, n);
        alist_toatoms(&y, outv+argc, 0, n);
        outlet_list(x->x_out1, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv+argc, 0, n);
        outlet_list(x->x_out1, &s_list, outc, outv);
    }
    ATOMS_FREEA(outv, outc);
}

/* function to restore gpointers after the list has moved in memory */
static void alist_restore_gpointers(t_alist *x, int offset, int count)
{
    t_listelem *vec = x->l_vec + offset;
    while (count--)
    {
        if (vec->l_a.a_type == A_POINTER)
            vec->l_a.a_w.w_gpointer = &vec->l_p;
        vec++;
    }
}

static void list_store_doinsert(t_list_store *x, t_symbol *s,
    int argc, t_atom *argv, int index)
{
    t_listelem *oldptr = x->x_alist.l_vec;
        /* try to allocate more memory */
    if (!(x->x_alist.l_vec = (t_listelem *)resizebytes(x->x_alist.l_vec,
        (x->x_alist.l_n) * sizeof(*x->x_alist.l_vec),
        (x->x_alist.l_n + argc) * sizeof(*x->x_alist.l_vec))))
    {
        x->x_alist.l_n = 0;
        pd_error(0, "list: out of memory");
        return;
    }
        /* fix gpointers in case resizebytes() has moved the alist in memory */
    if (x->x_alist.l_vec != oldptr && x->x_alist.l_npointer)
        alist_restore_gpointers(&x->x_alist, 0, x->x_alist.l_n);
        /* shift existing elements after 'index' to the right */
    if (index < x->x_alist.l_n)
    {
        memmove(x->x_alist.l_vec + index + argc, x->x_alist.l_vec + index,
            (x->x_alist.l_n - index) * sizeof(*x->x_alist.l_vec));
            /* fix gpointers because of memmove() */
        if (x->x_alist.l_npointer)
            alist_restore_gpointers(&x->x_alist, index + argc, x->x_alist.l_n - index);
    }
        /* finally copy new elements */
    alist_copyin(&x->x_alist, s, argc, argv, index);
    x->x_alist.l_n += argc;
}

static void list_store_insert(t_list_store *x, t_symbol *s,
    int argc, t_atom *argv)
{
    if (argc > 1)
    {
        int index = atom_getfloat(argv);
        if (index < 0)
        {
            pd_error(x, "list_store_insert: index %d out of range", index);
            return;
        } else if (index > x->x_alist.l_n)
            index = x->x_alist.l_n;
        list_store_doinsert(x, s, --argc, ++argv, index);
    }
}

static void list_store_append(t_list_store *x, t_symbol *s,
    int argc, t_atom *argv)
{
    list_store_doinsert(x, s, argc, argv, x->x_alist.l_n);
}

static void list_store_prepend(t_list_store *x, t_symbol *s,
    int argc, t_atom *argv)
{
    list_store_doinsert(x, s, argc, argv, 0);
}

static void list_store_delete(t_list_store *x, t_floatarg f1, t_floatarg f2)
{
    int i, max, index = (int)f1, n = (int)f2;
    t_listelem *oldptr = x->x_alist.l_vec;
    if (index < 0 || index >= x->x_alist.l_n)
    {
        pd_error(x, "list_store_delete: index %d out of range", index);
        return;
    }
    max = x->x_alist.l_n - index;
    if (!n)
        n = 1; /* default */
    else if (n < 0 || n > max)
        n = max; /* till the end of the list */

        /* unset pointers for elements which are to be deleted */
    if (x->x_alist.l_npointer)
    {
        t_listelem *vec = x->x_alist.l_vec + index;
        for (i = 0; i < n; i++)
        {
            if (vec[i].l_a.a_type == A_POINTER)
            {
                gpointer_unset(vec[i].l_a.a_w.w_gpointer);
                x->x_alist.l_npointer--;
            }
        }
    }
        /* shift elements (after the deleted elements) to the left */
    memmove(x->x_alist.l_vec + index, x->x_alist.l_vec + index + n,
        (x->x_alist.l_n - index - n) * sizeof(*x->x_alist.l_vec));
        /* shrink memory */
    if (!(x->x_alist.l_vec = (t_listelem *)resizebytes(x->x_alist.l_vec,
        (x->x_alist.l_n) * sizeof(*x->x_alist.l_vec),
        (x->x_alist.l_n - n) * sizeof(*x->x_alist.l_vec))))
    {
        x->x_alist.l_n = 0;
        pd_error(0, "list: out of memory");
        return;
    }
    if (x->x_alist.l_npointer)
    {
            /* fix all gpointers in case resizebytes() has moved the alist in memory */
        if (x->x_alist.l_vec != oldptr)
            alist_restore_gpointers(&x->x_alist, 0, x->x_alist.l_n - n);
        else /* only fix gpointers after index (because of of memmove()) */
            alist_restore_gpointers(&x->x_alist, index, x->x_alist.l_n - index - n);
    }
    x->x_alist.l_n -= n;
}

static void list_store_get(t_list_store *x, t_floatarg f1, t_floatarg f2)
{
    t_atom *outv;
    int onset = f1, outc = f2;
    if (!outc)
        outc = 1; /* default */
    else if (outc < 0)
    {
        outc = x->x_alist.l_n - onset; /* till the end of the list */
        if (outc <= 0) /* onset out of range */
        {
            outlet_bang(x->x_out2);
            return;
        }
    }
    if (onset < 0 || (onset + outc > x->x_alist.l_n))
    {
        outlet_bang(x->x_out2);
        return;
    }
    ATOMS_ALLOCA(outv, outc);
    if (x->x_alist.l_npointer)
    {
        t_alist y;
        alist_clone(&x->x_alist, &y, onset, outc);
        alist_toatoms(&y, outv, 0, outc);
        outlet_list(x->x_out1, &s_list, outc, outv);
        alist_clear(&y);
    }
    else
    {
        alist_toatoms(&x->x_alist, outv, onset, outc);
        outlet_list(x->x_out1, &s_list, outc, outv);
    }
    ATOMS_FREEA(outv, outc);
}

static void list_store_set(t_list_store *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 1)
    {
        int n, max, onset = atom_getfloat(argv);
        if (onset < 0 || onset >= x->x_alist.l_n)
        {
            pd_error(x, "list_store_set: index %d out of range", onset);
            return;
        }
        argc--; argv++;
        max = x->x_alist.l_n - onset;
        n = (argc > max) ? max : argc;
        alist_copyin(&x->x_alist, s, n, argv, onset);
    }
}

static void list_store_free(t_list_store *x)
{
    alist_clear(&x->x_alist);
}

static void list_store_setup(void)
{
    list_store_class = class_new(gensym("list store"),
        (t_newmethod)list_store_new, (t_method)list_store_free,
        sizeof(t_list_store), 0, A_GIMME, 0);
    class_addlist(list_store_class, list_store_list);
    class_addmethod(list_store_class, (t_method)list_store_send,
        gensym("send"), A_SYMBOL, 0);
    class_addmethod(list_store_class, (t_method)list_store_append,
        gensym("append"), A_GIMME, 0);
    class_addmethod(list_store_class, (t_method)list_store_prepend,
        gensym("prepend"), A_GIMME, 0);
    class_addmethod(list_store_class, (t_method)list_store_insert,
        gensym("insert"), A_GIMME, 0);
    class_addmethod(list_store_class, (t_method)list_store_delete,
        gensym("delete"), A_FLOAT, A_DEFFLOAT, 0);
    class_addmethod(list_store_class, (t_method)list_store_get,
        gensym("get"), A_FLOAT, A_DEFFLOAT, 0);
    class_addmethod(list_store_class, (t_method)list_store_set,
        gensym("set"), A_GIMME, 0);
    class_sethelpsymbol(list_store_class, &s_list);
}

/* ------------- list split --------------------- */

t_class *list_split_class;

typedef struct _list_split
{
    t_object x_obj;
    t_float x_f;
    t_outlet *x_out1;
    t_outlet *x_out2;
    t_outlet *x_out3;
} t_list_split;

static void *list_split_new(t_floatarg f)
{
    t_list_split *x = (t_list_split *)pd_new(list_split_class);
    x->x_out1 = outlet_new(&x->x_obj, &s_list);
    x->x_out2 = outlet_new(&x->x_obj, &s_list);
    x->x_out3 = outlet_new(&x->x_obj, &s_list);
    floatinlet_new(&x->x_obj, &x->x_f);
    x->x_f = f;
    return (x);
}

static void list_split_list(t_list_split *x, t_symbol *s,
    int argc, t_atom *argv)
{
    int n = x->x_f;
    if (n < 0)
        n = 0;
    if (argc >= n)
    {
        outlet_list(x->x_out2, &s_list, argc-n, argv+n);
        outlet_list(x->x_out1, &s_list, n, argv);
    }
    else outlet_list(x->x_out3, &s_list, argc, argv);
}

static void list_split_anything(t_list_split *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    XL_ATOMS_ALLOCA(outv, argc+1);
    SETSYMBOL(outv, s);
    atoms_copy(argc, argv, outv + 1);
    list_split_list(x, &s_list, argc+1, outv);
    XL_ATOMS_FREEA(outv, argc+1);
}

static void list_split_setup(void)
{
    list_split_class = class_new(gensym("list split"),
        (t_newmethod)list_split_new, 0,
        sizeof(t_list_split), 0, A_DEFFLOAT, 0);
    class_addlist(list_split_class, list_split_list);
    class_addanything(list_split_class, list_split_anything);
    class_sethelpsymbol(list_split_class, &s_list);
}

/* ------------- list trim --------------------- */

t_class *list_trim_class;

typedef struct _list_trim
{
    t_object x_obj;
} t_list_trim;

static void *list_trim_new( void)
{
    t_list_trim *x = (t_list_trim *)pd_new(list_trim_class);
    outlet_new(&x->x_obj, &s_list);
    return (x);
}

static void list_trim_list(t_list_trim *x, t_symbol *s,
    int argc, t_atom *argv)
{
    if (argc < 1 || argv[0].a_type != A_SYMBOL)
        outlet_list(x->x_obj.ob_outlet, &s_list, argc, argv);
    else outlet_anything(x->x_obj.ob_outlet, argv[0].a_w.w_symbol,
        argc-1, argv+1);
}

static void list_trim_anything(t_list_trim *x, t_symbol *s,
    int argc, t_atom *argv)
{
    outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void list_trim_setup(void)
{
    list_trim_class = class_new(gensym("list trim"),
        (t_newmethod)list_trim_new, 0,
        sizeof(t_list_trim), 0, 0);
    class_addlist(list_trim_class, list_trim_list);
    class_addanything(list_trim_class, list_trim_anything);
    class_sethelpsymbol(list_trim_class, &s_list);
}

/* ------------- list length --------------------- */

t_class *list_length_class;

typedef struct _list_length
{
    t_object x_obj;
} t_list_length;

static void *list_length_new( void)
{
    t_list_length *x = (t_list_length *)pd_new(list_length_class);
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

static void list_length_list(t_list_length *x, t_symbol *s,
    int argc, t_atom *argv)
{
    outlet_float(x->x_obj.ob_outlet, (t_float)argc);
}

static void list_length_anything(t_list_length *x, t_symbol *s,
    int argc, t_atom *argv)
{
    outlet_float(x->x_obj.ob_outlet, (t_float)argc+1);
}

static void list_length_setup(void)
{
    list_length_class = class_new(gensym("list length"),
        (t_newmethod)list_length_new, 0,
        sizeof(t_list_length), 0, 0);
    class_addlist(list_length_class, list_length_list);
    class_addanything(list_length_class, list_length_anything);
    class_sethelpsymbol(list_length_class, &s_list);
}

/* ------------- list fromsymbol --------------------- */

t_class *list_fromsymbol_class;

typedef struct _list_fromsymbol
{
    t_object x_obj;
} t_list_fromsymbol;

static void *list_fromsymbol_new( void)
{
    t_list_fromsymbol *x = (t_list_fromsymbol *)pd_new(list_fromsymbol_class);
    outlet_new(&x->x_obj, &s_list);
    return (x);
}

static void list_fromsymbol_symbol(t_list_fromsymbol *x, t_symbol *s)
{
    t_atom *outv;
    int n, outc = strlen(s->s_name);
    ATOMS_ALLOCA(outv, outc);
    for (n = 0; n < outc; n++)
        SETFLOAT(outv + n, (unsigned char)s->s_name[n]);
    outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    ATOMS_FREEA(outv, outc);
}

static void list_fromsymbol_setup(void)
{
    list_fromsymbol_class = class_new(gensym("list fromsymbol"),
        (t_newmethod)list_fromsymbol_new, 0, sizeof(t_list_fromsymbol), 0, 0);
    class_addsymbol(list_fromsymbol_class, list_fromsymbol_symbol);
    class_sethelpsymbol(list_fromsymbol_class, &s_list);
}

/* ------------- list tosymbol --------------------- */

t_class *list_tosymbol_class;

typedef struct _list_tosymbol
{
    t_object x_obj;
} t_list_tosymbol;

static void *list_tosymbol_new( void)
{
    t_list_tosymbol *x = (t_list_tosymbol *)pd_new(list_tosymbol_class);
    outlet_new(&x->x_obj, &s_symbol);
    return (x);
}

static void list_tosymbol_list(t_list_tosymbol *x, t_symbol *s,
    int argc, t_atom *argv)
{
    int i;
#if HAVE_ALLOCA
    char *str = alloca(argc + 1);
#else
    char *str = getbytes(argc + 1);
#endif
    for (i = 0; i < argc; i++)
        str[i] = (char)atom_getfloatarg(i, argc, argv);
    str[argc] = 0;
    outlet_symbol(x->x_obj.ob_outlet, gensym(str));
#if HAVE_ALLOCA
#else
    freebytes(str, argc+1);
#endif
}

static void list_tosymbol_setup(void)
{
    list_tosymbol_class = class_new(gensym("list tosymbol"),
        (t_newmethod)list_tosymbol_new, 0, sizeof(t_list_tosymbol), 0, 0);
    class_addlist(list_tosymbol_class, list_tosymbol_list);
    class_sethelpsymbol(list_tosymbol_class, &s_list);
}

/* ------------- list ------------------- */

void *list_new(t_pd *dummy, t_symbol *s, int argc, t_atom *argv)
{
    if (!argc || argv[0].a_type != A_SYMBOL)
        newest = list_append_new(s, argc, argv);
    else
    {
        t_symbol *s2 = argv[0].a_w.w_symbol;
        if (s2 == gensym("append"))
            newest = list_append_new(s, argc-1, argv+1);
        else if (s2 == gensym("cat"))
            newest = list_cat_new();
        else if (s2 == gensym("prepend"))
            newest = list_prepend_new(s, argc-1, argv+1);
        else if (s2 == gensym("split"))
            newest = list_split_new(atom_getfloatarg(1, argc, argv));
        else if (s2 == gensym("trim"))
            newest = list_trim_new();
        else if (s2 == gensym("length"))
            newest = list_length_new();
        else if (s2 == gensym("fromsymbol"))
            newest = list_fromsymbol_new();
        else if (s2 == gensym("tosymbol"))
            newest = list_tosymbol_new();
        else if (s2 == gensym("store"))
            newest = list_store_new(s, argc-1, argv+1);
        else 
        {
            error("list %s: unknown function", s2->s_name);
            newest = 0;
        }
    }
    return (newest);
}

void x_list_setup(void)
{
    alist_setup();
    list_append_setup();
    list_cat_setup();
    list_prepend_setup();
    list_store_setup();
    list_split_setup();
    list_trim_setup();
    list_length_setup();
    list_fromsymbol_setup();
    list_tosymbol_setup();
    class_addcreator((t_newmethod)list_new, &s_list, A_GIMME, 0);
}
