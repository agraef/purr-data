#include "m_pd.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include <string.h>

/* WARNING: KLUDGE!  */
/*
 * this struct is not publically defined (its in g_canvas.c) so I need to
 * include this here.  Its from Pd 0.41-test03 2006-11-19. */
struct _canvasenvironment
{
    t_symbol *ce_dir;      /* directory patch lives in */
    int ce_argc;           /* number of "$" arguments */
    t_atom *ce_argv;       /* array of "$" arguments */
    int ce_dollarzero;     /* value of "$0" */
    t_namelist *ce_path;   /* search path */
};

static char *version = "1.3";

t_int import_instance_count;

#define DEBUG(x)
//#define DEBUG(x) x 

/*------------------------------------------------------------------------------
 *  CLASS DEF
 */
static t_class *import_class;

typedef struct _import
{
	t_object            x_obj;
	t_canvas            *x_canvas;
	t_namelist          *x_top;
	t_namelist          *x_current;
	char                x_classpath_root[MAXPDSTRING];
	t_outlet            *x_data_outlet;
	t_outlet            *x_status_outlet;
} t_import;

static int load_library(t_import *x, char *library_name)
{
	DEBUG(post("load_library"););
    if (!sys_load_lib(x->x_canvas, library_name)) return 0;
	return 1;
}

static void load_arguments(t_import *x, int argc, t_atom *argv)
{
    t_symbol *library_name;
    
    while (argc--) {
        switch (argv->a_type) {
        case A_FLOAT:
            post("[import] ERROR: floats not supported: %f",
				 atom_getfloat(argv));
            break;
        case A_SYMBOL:
            library_name = atom_getsymbol(argv);
            if (!load_library(x,library_name->s_name))
				post("[import]: ERROR: can't load library in %s", 
					 library_name->s_name);
			else
				post("[import] loaded library: %s",library_name->s_name);
            break;
        default:
            post("[import] ERROR: Unsupported atom type");
        }
        argv++;
    }
}


static void import_output(t_import* x)
{
	DEBUG(post("import_output"););
	char buffer[MAXPDSTRING];

/* TODO: think about using x->x_current->nl_next so that if [import] is at
 * the end of its list, and another element gets added to the local
 * namespace, [import] will output the new element on the next bang. */
	if(x->x_current)
	{
		post("current string: %s",  x->x_current->nl_string);
		strncpy(buffer, x->x_current->nl_string, MAXPDSTRING);
		post("current string buffer: %s",  buffer);
		outlet_symbol( x->x_data_outlet, gensym(buffer) );
		x->x_current = x->x_current->nl_next;
	}
	else 
		outlet_bang(x->x_status_outlet);
}


static void import_reset(t_import* x) 
{
/* on >= 0.40, this class uses the patch-local paths, on older versions
 * before that existed, this class uses the global classpath */
#if (PD_MINOR_VERSION >= 40)
	x->x_top = canvas_getenv(x->x_canvas)->ce_path;
#else
	x->x_top = sys_searchpath;
#endif /* (PD_MINOR_VERSION >= 40) */
	x->x_current = x->x_top;
}


static void *import_new(t_symbol *s, int argc, t_atom *argv)
{
    t_import *x = (t_import *)pd_new(import_class);
	t_symbol *currentdir;

	if(import_instance_count == 0) 
	{
		post("[import] %s",version);  
		post("\twritten by Hans-Christoph Steiner <hans@eds.org>");
		post("\tcompiled on "__DATE__" at "__TIME__ " ");
		post("\tcompiled against Pd version %d.%d.%d", PD_MAJOR_VERSION, 
			 PD_MINOR_VERSION, PD_BUGFIX_VERSION);
	}
	import_instance_count++;

	strncpy(x->x_classpath_root, sys_libdir->s_name, MAXPDSTRING - 7);
	strcat(x->x_classpath_root, "/extra");

	x->x_data_outlet = outlet_new(&x->x_obj, &s_symbol);
	x->x_status_outlet = outlet_new(&x->x_obj, 0);

	x->x_canvas = canvas_getcurrent();
	load_arguments(x,argc,argv);
	import_reset(x);
	
	return (x);
}


static void import_free(t_import *x)
{
  /* TODO: look into freeing the namelist.  It probably does not need to
   * happen, since this class is just copying the pointer of an existing
   * namelist that is handled elsewhere. */

/* TODO: perhaps this should remove any libs that this instance had added to
 * the namespace */
}


void import_setup(void)
{
    import_class = class_new(gensym("import"), (t_newmethod)import_new,
                             (t_method)import_free,
                             sizeof(t_import), 
                             CLASS_DEFAULT, 
                             A_GIMME, 
                             0);
	/* add inlet atom methods */
	class_addbang(import_class,(t_method) import_output);
	
	/* add inlet selector methods */
	class_addmethod(import_class,(t_method) import_reset,
					gensym("reset"), 0);
}
