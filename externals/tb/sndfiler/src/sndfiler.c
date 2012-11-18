/*
 *
 * threaded soundfiler for pd
 * Copyright (C) 2005, Tim Blechmann
 *           (C) 2005, Georg Holzmann <grh@mur.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "sndfiler.h"
#include "file_input.h"


/************ forward declarations **************/

#ifdef UNIX
/* real-time flag, true if priority boosted */
extern int sys_hipriority;
#endif

#ifdef USE_PD_MAIN
struct _garray
{
    t_gobj x_gobj;
    t_scalar *x_scalar;     /* scalar "containing" the array */
    t_glist *x_glist;       /* containing glist */
    t_symbol *x_name;       /* unexpanded name (possibly with leading '$') */
    t_symbol *x_realname;   /* expanded name (symbol we're bound to) */
    char x_usedindsp;       /* true if some DSP routine is using this */
    char x_saveit;          /* true if we should save this with parent */
    char x_listviewing;     /* true if list view window is open */
};
#endif

/* get a garray's "array" structure. */
t_array *h_garray_getarray(t_garray *x)
{
    int zonset, ztype;
    t_symbol *zarraytype;
    t_scalar *sc = x->x_scalar;
    t_symbol *templatesym = sc->sc_template;
    t_template *_template = template_findbyname(templatesym);
    if (!_template)
    {
        error("array: couldn't find template %s", templatesym->s_name);
        return (0);
    }
    if (!template_find_field(_template, gensym("z"), 
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


/************ sndfiler **************/

static t_class *sndfiler_class;

typedef struct _sndfiler
{
    t_object x_obj;
    t_canvas *x_canvas;
} t_sndfiler;

typedef struct _sfprocess
{
    void* padding;
    /* callback function */
    void (* process) (t_sndfiler *, int, t_atom *);
    t_sndfiler * x;   /* soundfiler */
    int argc;
    t_atom * argv;
} t_sfprocess;

/* this is the queue for all soundfiler objects */
typedef struct _sfqueue
{
    t_fifo* x_jobs;
    SEM_T sem;
} t_sfqueue;

typedef struct _syncdata
{
    t_garray** arrays;
    t_float** helper_arrays;
    int argc;
    t_int frames;
} t_syncdata;

static t_sfqueue sndfiler_queue; 
static pthread_t sf_thread_id; /* id of soundfiler thread */

static t_sndfiler *sndfiler_new(void)
{
    t_sndfiler *x = (t_sndfiler *)pd_new(sndfiler_class);
    x->x_canvas = canvas_getcurrent();
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

/* global soundfiler thread ... sleeping until signaled */
static void sndfiler_thread(void)
{
    for(;;)
    {
        t_sfprocess * me;
        SEM_WAIT(sndfiler_queue.sem);

        while (me = (t_sfprocess *)fifo_get(sndfiler_queue.x_jobs))
        {
            (me->process)(me->x, me->argc, me->argv);

            /* freeing the argument vector */
            freebytes(me->argv, sizeof(t_atom) * me->argc);
            freebytes(me, sizeof(t_sfprocess));
        }
    }
}

static void sndfiler_start_thread(void)
{
    pthread_attr_t sf_attr;
    struct sched_param sf_param;
    int status;

    //initialize queue
    sndfiler_queue.x_jobs = fifo_init();

	status = SEM_INIT(sndfiler_queue.sem);
    if(!status)
        error("Couldn't create sndfiler semaphore: %i",status);
	
    // initialize thread
    pthread_attr_init(&sf_attr);
    
    sf_param.sched_priority=sched_get_priority_min(SCHED_OTHER);
    pthread_attr_setschedparam(&sf_attr,&sf_param);

    /* 1mb of stack should be enough */
    pthread_attr_setstacksize(&sf_attr,1048576);
	
#ifdef UNIX
    if (sys_hipriority == 1/*  && getuid() == 0 */)
    {
        sf_param.sched_priority=sched_get_priority_min(SCHED_RR);
        pthread_attr_setschedpolicy(&sf_attr,SCHED_RR);
    }
#endif /* UNIX */

    //start thread
    status = pthread_create(&sf_thread_id, &sf_attr, 
        (void *) sndfiler_thread,NULL);
  
    if (status != 0)
        error("Couldn't create sndfiler thread: %d",status);
    else
        post("Global sndfiler thread launched, priority: %d", 
            sf_param.sched_priority);
}

static void sndfiler_read_cb(t_sndfiler * x, int argc, t_atom* argv);

/* syntax:
 * read soundfile array0..n
 * if the soundfile has less channels than arrays are given, these arrays are
 * set to zero
 * if there are too little arrays given, only the first n channels will be used
 * */
static void sndfiler_read(t_sndfiler * x, t_symbol *s, int argc, t_atom* argv)
{
    t_sfprocess * process = getbytes(sizeof(t_sfprocess));

    process->process = &sndfiler_read_cb;
    process->x = x;
    process->argc = argc;
    process->argv = (t_atom*) copybytes(argv, sizeof(t_atom) * argc);

    fifo_put(sndfiler_queue.x_jobs, process);

    SEM_SIGNAL(sndfiler_queue.sem);
}

static t_int sndfiler_synchonize(t_int * w);

static void sndfiler_read_cb(t_sndfiler * x, int argc, t_atom* argv)
{
    int i, j, lib;
    int channel_count;
    t_float** helper_arrays;
    int resize = 0;
    int seek = 0, arraysize = 0;

    t_symbol* file;
    t_garray ** arrays;

    // parse flags
    while (argc > 0 && argv->a_type == A_SYMBOL &&
        *argv->a_w.w_symbol->s_name == '-')
    {
        char *flag = argv->a_w.w_symbol->s_name + 1;
        if (!strcmp(flag, "resize"))
        {
            resize = 1;
            argc -= 1; argv += 1;
        }
        else if (!strcmp(flag, "skip"))
        {
            if (argc < 2 || argv[1].a_type != A_FLOAT)
                goto usage;
            else
                seek = argv[1].a_w.w_float;
            argc -= 2; argv += 2;
        }
        else goto usage;
    }
    
    if (argc < 2)
        goto usage;

    file = atom_getsymbolarg(0, argc, argv);

    channel_count = argc - 1;
    helper_arrays = getbytes(channel_count * sizeof(t_float*));

    arrays = getbytes(channel_count * sizeof(t_garray*));
    for (i = 0; i != channel_count; ++i)
    {
        t_float *dummy;
        int size;
        t_garray *array;
         
        if(!(array = (t_garray *)pd_findbyclass(
                                                atom_getsymbolarg(i+1, argc, argv), garray_class)))
        {
            pd_error(x, "%s: no such array", atom_getsymbolarg(i+1, 
                         argc, argv)->s_name);
            return;
        }
	
        if(garray_getfloatarray(array, &size, &dummy))
            arrays[i] = array;
        else
        {
            pd_error(x, "%s: bad template for sndfiler", atom_getsymbolarg(i+1, 
                         argc, argv)->s_name);
            return;
        }
	
        // in multichannel mode: check if arrays have different length
        if (arraysize && arraysize != size && !resize)
        {
            post("sndfiler: arrays have different lengths, resizing to last one ...");
        }
        arraysize = size;
    }

    lib = check_fileformat(file);
    if(lib == USE_LIBSNDFILE)
        arraysize = read_libsndfile(helper_arrays, channel_count, seek,
                                    resize, arraysize, file);
    else if(lib == USE_LIBVORBISFILE)
        arraysize = read_libvorbisfile(helper_arrays, channel_count, seek,
                                       resize, arraysize, file);
    else
    {
        pd_error(x, "Error opening file");
        return;
    }

    if(arraysize > 0)
    {
        t_int ** syncdata = getbytes(sizeof(t_int*) * 5);

        syncdata[0] = (t_int*)arrays;
        syncdata[1] = (t_int*)helper_arrays;
        syncdata[2] = (t_int*)channel_count;
        syncdata[3] = (t_int*)arraysize;
        syncdata[4] = (t_int*)x;

        sys_callback(sndfiler_synchonize, (t_int*)syncdata, 5);
        return;
    }
    else
    {
        pd_error(x, "Error opening file");
        return;
    }
    
 usage:
	pd_error(x, "usage: read [flags] filename array1 array2 ...");
    post("flags: -skip <n> -resize ");
}

static t_int sndfiler_synchonize(t_int * w)
{
    int i;
    t_garray** arrays = (t_garray**) w[0];
    t_float** helper_arrays = (t_float**) w[1];
    t_int channel_count = (t_int)w[2];
    t_int frames = (t_int)w[3];
    t_sndfiler* x = (t_sndfiler*)w[4];

    for (i = 0; i != channel_count; ++i)
    {
        t_garray * garray = arrays[i];
        t_array * array = h_garray_getarray(garray);
        t_glist * gl = garray->x_glist;;

        freealignedbytes(array->a_vec, array->a_n);
        array->a_vec = (char*)helper_arrays[i];
        array->a_n = frames;

        if (gl->gl_list == &garray->x_gobj && !garray->x_gobj.g_next)
        {
            vmess(&gl->gl_pd, gensym("bounds"), "ffff", 0., gl->gl_y1,
                (double)(frames > 1 ? frames-1 : 1), gl->gl_y2);

            /* close any dialogs that might have the wrong info now... */
            gfxstub_deleteforkey(gl);
        }
        else 
            garray_redraw(garray);
    }

    free(arrays);
    free(helper_arrays);

    canvas_update_dsp();

    outlet_float(x->x_obj.ob_outlet, frames);

    return 0;
}

static void sndfiler_t_resize(t_sndfiler * y, int argc, t_atom* argv);

/* syntax:
 * resize table size
 * */
static void sndfiler_resize(t_sndfiler * x, t_symbol *s, int argc, t_atom* argv)
{
    t_sfprocess * process = getbytes(sizeof(t_sfprocess));

    process->process = &sndfiler_t_resize;
    process->x = x;
    process->argc = argc;
    process->argv = (t_atom*) copybytes(argv, sizeof(t_atom) * argc);

    fifo_put(sndfiler_queue.x_jobs, process);

    SEM_SIGNAL(sndfiler_queue.sem);
}

static void sndfiler_t_resize(t_sndfiler *y, int argc, t_atom *argv)
{
    int was, elemsize;       /* array contains was elements of size elemsize */
    t_float * vec;           /* old array */ 
    t_glist *gl;
    int n;                   /* resize of n elements */
    char *nvec;              /* new array */ 

    t_garray * x = (t_garray *)pd_findbyclass(argv[0].a_w.w_symbol, garray_class);
    t_array *data = h_garray_getarray(x);

    if (!(x))
    {
        pd_error(y, "%s: no such table", argv[0].a_w.w_symbol->s_name);
        goto usage;
    }
    
    vec = (t_float*) data->a_vec;
    was = data->a_n;

    if ((argv+1)->a_type == A_FLOAT)
        n = (int) (argv+1)->a_w.w_float;
    else
        goto usage;

    if (n == was) return;

    if (n < 1) n = 1;
    elemsize = template_findbyname(data->a_templatesym)->t_n * sizeof(t_word);

#if (_POSIX_MEMLOCK - 0) >=  200112L
    munlockall();
#endif

    if (was > n)
        nvec = (char*)copybytes(data->a_vec, was * elemsize);
    else
    {
        nvec = getbytes(n * elemsize);
        memcpy (nvec, data->a_vec, was * elemsize);

        /* LATER should check t_resizebytes result */
        memset(nvec + was*elemsize, 0, (n - was) * elemsize);
    }
  
    if (!nvec)
    {
        pd_error(x, "array resize failed: out of memory");

#if (_POSIX_MEMLOCK - 0) >=  200112L
        mlockall(MCL_FUTURE);
#endif

        return;
    }

    /* TB: we'll have to be sure that no one is accessing the array */
    sys_lock();

#ifdef GARRAY_THREAD_LOCK
    garray_lock(x);
#endif

    data->a_vec = nvec;
    data->a_n = n;

#ifdef GARRAY_THREAD_LOCK
    garray_unlock(x);
#endif
    
    if (x->x_usedindsp) canvas_update_dsp();
    sys_unlock();

    /* if this is the only array in the graph,
       reset the graph's coordinates */
    gl = x->x_glist;
    if (gl->gl_list == &x->x_gobj && !x->x_gobj.g_next)
    {
        vmess(&gl->gl_pd, gensym("bounds"), "ffff",
            0., gl->gl_y1, (double)(n > 1 ? n-1 : 1), gl->gl_y2);
        /* close any dialogs that might have the wrong info now... */
        gfxstub_deleteforkey(gl);
    }
    else garray_redraw(x);

    freebytes (vec, was * elemsize);

#if (_POSIX_MEMLOCK - 0) >= 200112L
    mlockall(MCL_FUTURE);
#endif

    sys_lock();
    outlet_float(y->x_obj.ob_outlet, (float)atom_getintarg(1,argc,argv)); 
    sys_unlock();
  
    return;
  
 usage:
    pd_error(x, "usage: resize tablename size");
}

#ifdef _MSC_VER
__declspec(dllexport)
#endif
void sndfiler_setup(void)
{
    sndfiler_class = class_new(gensym("sndfiler"), 
        (t_newmethod)sndfiler_new, 0,
        sizeof(t_sndfiler), 0, 0);

    class_addmethod(sndfiler_class, (t_method)sndfiler_read,
        gensym("read"), A_GIMME, 0);
    class_addmethod(sndfiler_class, (t_method)sndfiler_resize,
        gensym("resize"), A_GIMME, 0);

#ifdef USE_PD_MAIN
    // needed to start thread callback system
    threadlib_setup();
#endif
  
    // starts helper thread
    sndfiler_start_thread();
}
