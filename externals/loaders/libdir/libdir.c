#include "m_pd.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* this object requires Pd 0.40.3 or later */

/* WARNING: KLUDGE!  */
/*
 * this struct is not publically defined (its in g_canvas.c) so I need to
 * include this here.  Its from Pd 0.47-0. */
struct _canvasenvironment
{
    t_symbol *ce_dir;      /* directory patch lives in */
    int ce_argc;           /* number of "$" arguments */
    t_atom *ce_argv;       /* array of "$" arguments */
    int ce_dollarzero;     /* value of "$0" */
    t_namelist *ce_path;   /* search path */
};


static char *version = "1.10";

/* This loader opens a directory with a -meta.pd file as a library.  In the
 * long run, the idea is that one folder will have all of objects files, all
 * of the related *-help.pd files, a file with meta data for the help system,
 * etc.  Then to install the lib, it would just be dropped into extra, or
 * anywhere in the global classpath.
 *
 * Ultimately, the meta file will be read for meta data, specifically for
 * the auto-generated Help system, but for other things too.  Right now,
 * its just used as a marker that a directory is meant to be a library.
 * Plus its much easier to implement it this way, I can use
 * open_via_path() instead of writing a new function.  The grand plan is
 * to have one directory hold the objects, help files, manuals,
 * etc. making it a self-contained library. <hans@at.or.at>
 */
/*
 * TODO
 * - get 'declare' messages from the meta-file, and send them to the current canvas
 */
static void libdir_get_fullname(char*dest, size_t size, const char*classname) {
  snprintf(dest, size-1, "%s/%s-meta", classname, classname);
  dest[size-1]=0;
}
static int libdir_add_to_path(const char*dirbuf, t_canvas*canvas) {
  if(sys_isabsolutepath(dirbuf)) { // only include actual full paths
    if (canvas) {
      t_canvasenvironment *canvasenvironment = canvas_getenv(canvas);
      canvasenvironment->ce_path = namelist_append(canvasenvironment->ce_path,
						 dirbuf, 0);
    } else {
        sys_searchpath = namelist_append(sys_searchpath, dirbuf, 0);
    }
    return 1;
  }
  return 0;
}

static int libdir_loader_legacy(t_canvas *canvas, char *classname)
{
    int fd = -1;
    char fullclassname[FILENAME_MAX], dirbuf[FILENAME_MAX];
    char *nameptr;

/* look for meta file (classname)/(classname)-meta.pd */
    libdir_get_fullname(fullclassname, FILENAME_MAX, classname);

    /* if this is being called from a canvas, then add the library path to the
     * canvas-local path */
    if(canvas)
      /* setting the canvas to NULL causes it to ignore any canvas-local path */
      fd = canvas_open(NULL, fullclassname, ".pd",
		       dirbuf, &nameptr, FILENAME_MAX, 0);
    else
      fd = open_via_path(".", fullclassname, ".pd",
			 dirbuf, &nameptr, FILENAME_MAX, 0);
    if(fd < 0)
    {
      return (0);
    }
    sys_close(fd);
#if 0
    if(!canvas) {
      char helppathname[FILENAME_MAX];
      strncpy(helppathname, sys_libdir->s_name, FILENAME_MAX-30);
      helppathname[FILENAME_MAX-30] = 0;
      strcat(helppathname, "/doc/5.reference/");
      strcat(helppathname, classname);
      sys_helppath = namelist_append(sys_helppath, helppathname, 0);
    }
#endif
    if(libdir_add_to_path(dirbuf, canvas))
      logpost(NULL, 3, "libdir_loader: added '%s' to the %s objectclass path",
	      classname, canvas?"canvas-local":"global");

    /* post("libdir_loader loaded fullclassname: '%s'\n", fullclassname); */
#if 0
    // AG: This is from https://github.com/pure-data/libdir/commit/2f7b873e.
    // Seems overly verbose, though, disasbled for now since Purr Data won't
    // filter console messages according to their verbosity level.
    logpost(NULL, 14, "Loaded libdir '%s' from '%s'", classname, dirbuf);
#endif

    return (1);
}

static int libdir_loader_pathwise(t_canvas *canvas, const char *classname, const char*path)
{
    int fd = -1;
    char fullclassname[FILENAME_MAX], dirbuf[FILENAME_MAX];
    char *nameptr;

    if(!path) {
      /* we already tried all paths, so skip this */
      return 0;
    }

    /* look for meta file (classname)/(classname)-meta.pd */
    libdir_get_fullname(fullclassname, FILENAME_MAX, classname);

    if ((fd = sys_trytoopenone(path, fullclassname, ".pd",
			       dirbuf, &nameptr, FILENAME_MAX, 0)) < 0) {
      return 0;
    }
    sys_close(fd);
    if(libdir_add_to_path(dirbuf, canvas))
      logpost(NULL, 3, "libdir_loader: added '%s' to the %s objectclass path",
	      classname, canvas?"canvas-local":"global");

    /* post("libdir_loader loaded fullclassname: '%s'\n", fullclassname); */
#if 0
    logpost(NULL, 14, "Loaded libdir '%s' from '%s'", classname, dirbuf);
#endif

    return (1);
}

static t_class *libdir_class;
static void*libdir_new(void)
{
  t_pd *x = pd_new(libdir_class);
  return (x);
}

void libdir_setup(void)
{
    int major, minor, bugfix;
    sys_getversion(&major, &minor, &bugfix);
    if (major>0 || minor >=47) {
      sys_register_loader(libdir_loader_pathwise);
    } else {
      sys_register_loader(libdir_loader_legacy);
    }
    logpost(NULL, 3, "libdir loader %s",version);
    logpost(NULL, 3, "\tcompiled on "__DATE__" at "__TIME__ " ");
    logpost(NULL, 3, "\tcompiled against Pd version %d.%d.%d.%s",
            PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION, PD_TEST_VERSION);
    libdir_class = class_new(gensym("libdir"), libdir_new, 0, sizeof(t_object), CLASS_NOINLET, 0);
}

void setup(void)
{
    libdir_setup();
}
