/* ********************************************** */
/* the ZEXY external                              */
/* ********************************************** */
/*                            forum::für::umläute */
/* ********************************************** */

/* the ZEXY external is a runtime-library for miller s. puckette's realtime-computermusic-software "pure data"
 * therefore you NEED "pure data" to make any use of the ZEXY external
 * (except if you want to use the code for other things)
 * download "pure data" at

 http://pd.iem.at
 ftp://iem.at/pd

 *
 * if you are looking for the latest release of the ZEXY-external you should have another look at

 http://puredata.info/community/projects/software/zexy/
 ftp://iem.at/pd/Externals/ZEXY

 *
 * ZEXY is published under the GNU GeneralPublicLicense, that must be shipped with ZEXY.
 * if you are using Debian GNU/linux, the GNU-GPL can be found under /usr/share/common-licenses/GPL
 * if you still haven't found a copy of the GNU-GPL, have a look at http://www.gnu.org
 *
 * "pure data" has it's own license, that comes shipped with "pure data".
 *
 * there are ABSOLUTELY NO WARRANTIES for anything
 */

#ifndef INCLUDE_ZEXY_H__
#define INCLUDE_ZEXY_H__

/* these pragmas are only used for MSVC, not MinGW or Cygwin */
#ifdef _MSC_VER
# pragma warning( disable : 4018 )
# pragma warning( disable : 4244 )
# pragma warning( disable : 4305 )
# pragma warning( disable : 4996)  /* deprecated functions */
#endif

#include "m_pd.h"


#ifndef VERSION
# define VERSION "(unknown version)"
#endif

#ifndef HEARTSYMBOL
# if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 43)
#  define HEARTSYMBOL "♡"
# else
#  define HEARTSYMBOL "@"
# endif
#endif

#include <math.h>

#ifdef __WIN32__
# define STATIC_INLINE

# ifndef sqrtf
#  define sqrtf sqrt
# endif /* sqrtf */
# ifndef fabsf
#  define fabsf fabs
# endif /* fabsf */

#else
# define STATIC_INLINE static
#endif

#ifdef __GNUC__
# define UNUSED(x) ZUNUSED_ ## x __attribute__((__unused__))
# define UNUSED_FUNCTION(x) __attribute__((__unused__)) ZUNUSEDFUN_ ## x
# define MAYBE_USED_FUNCTION(x) __attribute__((__unused__)) x
# if __GNUC__ >= 9
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
#else
# define UNUSED(x) ZUNUSED_ ## x
# define UNUSED_FUNCTION(x) ZUNUSEDFUN_ ## x
# define MAYBE_USED_FUNCTION(x) x
#endif

#define ZEXY_TYPE_EQUAL(type1, type2) (sizeof(type1) == sizeof(type2))

typedef struct _mypdlist {
  t_object x_obj;

  int x_n;
  t_atom *x_list;
} t_mypdlist;

#ifndef BUILD_DATE
# define BUILD_DATE  __DATE__
#endif

#if PD_FLOATSIZE == 32
# define Z_FABS(f) fabsf(f)
#else
# define Z_FABS(f) fabs(f)
#endif

/* marker for setup-functions to be called by zexy_setup() */
#define ZEXY_SETUP

/* convenience functions */
static int zexy_argparse(const char*argstring, int argc, t_atomtype*argv)
{
  const char*args = argstring;
  int i;
  for(i=0; i<argc; i++) {
    argv[i]=A_NULL;
  }
  for(i=0; i<argc && *args; i++, args++) {
    switch(*args) {
    case 'f':
      argv[i] = A_FLOAT;
      break;
    case 'F':
      argv[i] = A_DEFFLOAT;
      break;
    case 's':
      argv[i] = A_SYMBOL;
      break;
    case 'S':
      argv[i] = A_DEFSYM;
      break;
    case 'p':
      argv[i] = A_POINTER;
      break;
    case '!':
      argv[i] = A_CANT;
      break;
    case '*':
      argv[i] = A_GIMME;
      break;
    default:
      error("ZEXYERROR: unknown argument specifier '%s'", argstring);
      return -1;
    }
  }
  return i;
}

static t_class MAYBE_USED_FUNCTION(*zexy_classnew) (const char*name,
    t_newmethod newmethod, t_method freemethod, size_t size, int flags,
    const char*args)
{
  t_atomtype at[5];
  if(zexy_argparse(args, 5, at) < 0) {
    return 0;
  }
  return class_new(gensym(name), newmethod, freemethod, size, flags, at[0],
                   at[1], at[2], at[3], at[4], A_NULL);
}
#define zexy_new(name, ctor, dtor, memberstruct, flags, args) \
  zexy_classnew(name, (t_newmethod)ctor, (t_method)dtor, sizeof(memberstruct), flags, args)


static void MAYBE_USED_FUNCTION(zexy_addmethod) (t_class*c, t_method fn,
    const char*s, const char*args)
{
  /* wrapper around 'class_addmethod' that is a bit more terse... */
  t_atomtype at[5];
  if(zexy_argparse(args, 5, at) < 0) {
    return;
  }
  class_addmethod(c, fn, gensym(s), at[0], at[1], at[2], at[3], at[4],
                  A_NULL);
}

#ifndef ZEXY_LIBRARY
static void zexy_register(char*object)
{
  if(object!=0) {
    post("[%s] part of zexy-%s (compiled "BUILD_DATE")", object, VERSION);
    post("\tCopyright (c) 1999-2018 IOhannes m zmölnig, forum::für::umläute & IEM");
  }
}
#else
static void zexy_register(char*UNUSED(object))
{
}
#endif /* ZEXY_LIBRARY */

#endif /* INCLUDE_ZEXY_H__ */
