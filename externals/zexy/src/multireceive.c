/*
 * multireceive: receive messages for various receive-names
 *
 * (c) 1999-2011 IOhannes m zmölnig, forum::für::umläute, institute of electronic music and acoustics (iem)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#include "zexy.h"

#ifdef _WIN32
# include <sys/timeb.h>
#else
# include <sys/time.h>
#endif

/* -------------------------- multireceive ------------------------------ */

static t_class *multireceive_class=NULL;
static t_class *multireceive_proxy_class=NULL;

typedef struct _symlist {
  t_symbol*s;
  struct _symlist*next;
} t_symlist;

typedef struct _multireceive_proxy {
  t_object x_obj;
  struct _multireceive*x_parent;
} t_multireceive_proxy;

typedef struct _multireceive {
  t_object x_obj;
  t_multireceive_proxy*x_proxy;
  t_symlist*x_symlist;
  t_outlet *x_out;
} t_multireceive;


static unsigned long long unique(void)
{
  unsigned long long uid = 0;
#ifdef _WIN32
  struct _timeb tb;
  _ftime(&tb);
  uid=(((unsigned long long)tb.time)<<(4*sizeof(uid))) |
      (unsigned long long)(tb.millitm);
#else
  struct timeval tv;

  gettimeofday(&tv, 0);

  /* First get the seconds right */
  uid=((unsigned long)tv.tv_sec)<<(4*sizeof(uid)) | tv.tv_usec;
#endif
  return uid;
}


static void multireceive_any(t_multireceive_proxy *x, t_symbol*s, int argc,
                             t_atom*argv)
{
  outlet_anything(x->x_parent->x_out, s, argc, argv);
}

static void multireceive_add(t_multireceive *x, t_symbol*s)
{
  t_symlist*sl=x->x_symlist;
  t_symlist*element=NULL;

  if(sl) {
    while(sl->next) {
      if(s==sl->s) {
        // already bound to this symbol
        return;
      }
      sl=sl->next;
    }
  }

  element=(t_symlist*)getbytes(sizeof(t_symlist));
  element->s=s;
  element->next=NULL;
  pd_bind(&x->x_proxy->x_obj.ob_pd, s);

  if(sl) {
    sl->next=element;
  } else {
    x->x_symlist=element;
  }
}

static void multireceive_clear(t_multireceive *x)
{
  t_symlist*sl=x->x_symlist;
  t_symlist*current=NULL;
  while(sl) {
    current=sl;
    sl=sl->next;

    pd_unbind(&x->x_proxy->x_obj.ob_pd, current->s);

    current->s=NULL;
    current->next=NULL;
    freebytes(current, sizeof(t_symlist));
  }
  x->x_symlist=NULL;
}


static void multireceive_set(t_multireceive *x, t_symbol*UNUSED(s),
                             int argc,
                             t_atom*argv)
{
  multireceive_clear(x);

  while(argc-->0) {
    t_symbol*s=atom_getsymbol(argv);
    if(A_SYMBOL==argv->a_type) {
      multireceive_add(x, s);
    } else {
      verbose(1, "[multireceive]: ignoring non-symbol receive name");
    }
    argv++;
  }

}

static void multireceive_free(t_multireceive *x)
{
  multireceive_clear(x);
  pd_free((t_pd *)x->x_proxy);
  outlet_free(x->x_out);
  x->x_out=NULL;
}

static void *multireceive_new(t_symbol *UNUSED(s), int argc, t_atom *argv)
{
  t_multireceive *x = (t_multireceive *)pd_new(multireceive_class);
  x->x_proxy=(t_multireceive_proxy*)pd_new(multireceive_proxy_class);
  x->x_proxy->x_parent=x;
  x->x_symlist=NULL;
  x->x_out = outlet_new(&x->x_obj, 0);

  multireceive_set(x, 0, argc, argv);
  return (x);
}

ZEXY_SETUP void multireceive_setup(void)
{
  char uniqsym[MAXPDSTRING];
  unsigned long long uid=unique();

  multireceive_class = zexy_new("multireceive",
                                multireceive_new, multireceive_free, t_multireceive, 0, "*");
  zexy_addmethod(multireceive_class, (t_method)multireceive_set, "set", "*");

  zexy_addmethod(multireceive_class, (t_method)multireceive_add, "add", "s");
  snprintf(uniqsym, MAXPDSTRING-2, "multireceive proxy %0llx", uid);
  uniqsym[MAXPDSTRING-1]=0;
  multireceive_proxy_class = zexy_new(uniqsym,
                                      0, 0, t_multireceive_proxy, CLASS_PD | CLASS_NOINLET, "");

  class_addanything(multireceive_proxy_class, multireceive_any);

  zexy_register("multireceive");
}
