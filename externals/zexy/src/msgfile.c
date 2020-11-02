/*
 * msgfile: an improved version of [textfile]
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


/*
    this is heavily based on code from [textfile],
    which is part of pd and written by Miller S. Puckette
    pd (and thus [textfile]) come with their own license
*/

#include "zexy.h"

#ifdef __WIN32__
# include <io.h>
#else
# include <unistd.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <string.h>




/* ****************************************************************************** */
/* msgfile : save and load messages... */

typedef enum {
  /* PD
   * separate items by whitespace (' ', '\t'), separate lines by ";"
   * looks like a Pd-file */
  FORMAT_PD = 0,
  /* FUDI
   * separate items by whitespace (' ', '\t'), separate lines by ";"
   * looks like a Pd-file, but uses our own parser (to better handle escaped linebreaks) */
  FORMAT_FUDI = 0,
  /* CR
   * separate items by whitespace (' ', '\t'), separate lines by linebreaks ("\n")
   * how you would expect a file to look like
   * (use Pd's parser that has some quirks with escaped linebreaks) */
  FORMAT_CR,
  /* TXT:
   * separate items by whitespace (' ', '\t'), separate lines by "\n"
   * how you would expect a file to look like
   * (uses our own parser that handles escaped '\n' better!) */
  FORMAT_TXT,
  /* CSV: separate items by ','; separate lines by " \n"
   * spreadsheet: each argument gets its own column
   * with proper escaping of everything */
  FORMAT_CSV,

  /* illegal format */
  FORMAT_ILLEGAL,
} t_msgfile_format;


typedef struct _msglist {
  int n;
  t_atom *thislist;

  void *next;
  void *previous;
} t_msglist;

typedef struct _msgfile {
  t_object x_obj;              /* everything */
  t_outlet *x_secondout;        /* "done" */

  t_msgfile_format format;

  t_msglist *start;

  t_msglist *current;         /* pointer to our list */
  t_msglist *previous; /* just in case we lost "current" */

  t_symbol *x_dir;
  t_canvas *x_canvas;

  char eol, separator;

} t_msgfile;

static t_class *msgfile_class=NULL;





/* ************************************************************************ */
/* forward declarations                                                     */

static void msgfile_end(t_msgfile *x);
static void msgfile_goto(t_msgfile *x, t_float f);

/* ************************************************************************ */
/* helper functions                                                         */

static t_msgfile_format symbol2format(t_msgfile*x, t_symbol*s)
{
  if (!s || gensym("")==s) {
    return x->format;
  }
  if (gensym("pd")==s) {
    return FORMAT_PD;
  }
  if (gensym("fudi")==s) {
    return FORMAT_FUDI;
  }
  if (gensym("cr")==s) {
    return FORMAT_CR;
  }
  if (gensym("txt")==s) {
    return FORMAT_TXT;
  }
  if (gensym("csv")==s) {
    return FORMAT_CSV;
  }
  pd_error(x, "msgfile: ignoring unknown format: '%s'", s->s_name);
  return x->format;
}

static int is_float(t_atom*a)
{
  return (a && A_FLOAT == a->a_type);
}

static int node_count(t_msgfile *x)
{
  t_msglist *dummy = x->start;
  unsigned int counter = 0;
  while (dummy && dummy->next) {
    counter++;
    dummy = dummy->next;
  }
  return counter;
}

static int node_wherearewe(t_msgfile *x)
{
  int counter = 0;
  t_msglist *cur = x->start;

  while (cur && cur->next && cur!=x->current) {
    counter++;
    cur = cur->next;
  }

  if(cur&&cur->thislist) {
    return counter;
  }
  return -1;
}

static void write_currentnode(t_msgfile *x, int ac, t_atom *av)
{
  /* append list to the current node list */

  t_msglist *cur=x->current;
  t_atom *ap=NULL;
  int newsize = 0;

  if(!cur || (ac && av && A_SYMBOL==av->a_type
              && gensym("")==atom_getsymbol(av))) {
    /* ignoring empty symbols! */
    return;
  }

  newsize = cur->n + ac;

  ap = (t_atom*)resizebytes(cur->thislist, cur->n * sizeof(t_atom),
                            newsize * sizeof(t_atom));
  if (ap) {
    cur->thislist = ap;
    memcpy(cur->thislist + cur->n, av, ac * sizeof(t_atom));

    cur->n = newsize;
  }
}

static void delete_currentnode(t_msgfile *x)
{
  if (x&&x->current) {
    t_msglist *dummy = x->current;
    t_msglist *nxt=0;
    t_msglist *prv=0;

    if(dummy) {
      nxt=dummy->next;
      prv=dummy->previous;

      if(dummy==x->start) {
        x->start=nxt;
      }

      freebytes(dummy->thislist, sizeof(dummy->thislist));
      dummy->thislist = 0;
      dummy->n = 0;
      dummy->next=0;
      dummy->previous=0;

      freebytes(dummy, sizeof(t_msglist));

      dummy=0;
    }

    if (nxt) {
      nxt->previous = prv;
    }
    if (prv) {
      prv->next     = nxt;
    }

    x->current = (nxt)?nxt:prv;
    if(x->current) {
      x->previous=x->current->previous;
    } else {
      x->previous=prv;
    }

  }
}

static void delete_emptynodes(t_msgfile *x)
{
  x->current=x->start;
  x->previous=0;
  if (!x->current) {
    return;
  }

  while (x->current && x->current->next) {
    if (!x->current->thislist) {
      delete_currentnode(x);
    } else {
      x->previous=x->current;
      x->current = x->current->next;
    }
  }
}

static void add_currentnode(t_msgfile *x)
{
  /* add (after the current node) a node at the current position (do not write the listbuf !!!) */
  t_msglist *newnode = (t_msglist *)getbytes(sizeof(t_msglist));
  t_msglist  *prv, *nxt, *cur=x->current;

  newnode->n = 0;
  newnode->thislist = 0;

  prv = cur;
  nxt = (cur)?cur->next:0;

  newnode->next = nxt;
  newnode->previous = prv;

  if (prv) {
    prv->next = newnode;
  }
  if (nxt) {
    nxt->previous = newnode;
  }

  x->current = newnode;
  x->previous=prv;

  if(!x->start) { /* it's the first line in the buffer */
    x->start=x->current;
  }
}
static void insert_currentnode(t_msgfile *x)
{
  /* insert (add before the current node) a node (do not write a the listbuf !!!) */
  t_msglist *newnode;
  t_msglist  *prv, *nxt, *cur = x->current;

  if (!(cur && cur->thislist)) {
    add_currentnode(x);
  } else {
    newnode = (t_msglist *)getbytes(sizeof(t_msglist));

    newnode->n = 0;
    newnode->thislist = 0;

    nxt = cur;
    prv = (cur)?cur->previous:0;

    newnode->next = nxt;
    newnode->previous = prv;

    if (prv) {
      prv->next = newnode;
    }
    if (nxt) {
      nxt->previous = newnode;
    }

    x->previous=prv;
    x->current = newnode;
    if(0==prv) {
      /* oh, we have a new start! */
      x->start = newnode;
    }
  }
}

/* delete from line "start" to line "stop"
 * if "stop" is negative, delete from "start" to the end
 */
static void delete_region(t_msgfile *x, int start, int stop)
{
  int n;
  int newwhere, oldwhere = node_wherearewe(x);

  /* get the number of lists in the buffer */
  int counter = node_count(x);

  if ((stop > counter) || (stop == -1)) {
    stop = counter;
  }
  if ((stop+1) && (start > stop)) {
    return;
  }
  if (stop == 0) {
    return;
  }

  newwhere = (oldwhere < start)?oldwhere:( (oldwhere < stop)?start:start+
             (oldwhere-stop));
  n = stop - start;

  msgfile_goto(x, start);

  while (n--) {
    delete_currentnode(x);
  }

  if (newwhere+1) {
    msgfile_goto(x, newwhere);
  } else {
    msgfile_end(x);
  }
}


static int atomcmp(t_atom *this, t_atom *that)
{
  if (this->a_type != that->a_type) {
    return 1;
  }

  switch (this->a_type) {
  case A_FLOAT:
    return !(atom_getfloat(this) == atom_getfloat(that));
    break;
  case A_SYMBOL:
    return strcmp(atom_getsymbol(this)->s_name, atom_getsymbol(that)->s_name);
    break;
  case A_POINTER:
    return !(this->a_w.w_gpointer == that->a_w.w_gpointer);
    break;
  default:
    return 0;
  }

  return 1;
}


static t_atomtype str2atom(const char*atombuf, t_atom*ap,
                           int force_symbol)
{
  if(!force_symbol) {
    double f = 0;
    unsigned int count=0;
    int x = sscanf(atombuf, "%lg%n", &f, &count);
    if(x && strlen(atombuf)==count) {
      SETFLOAT(ap, f);
      return A_FLOAT;
    }
  }
  SETSYMBOL(ap, gensym(atombuf));
  return A_SYMBOL;
}

static const char*parse_csv(const char*src, char dst[MAXPDSTRING],
                            int *_eol, int*_quoted)
{
  size_t len = 0;
  int quoted = (src[0] == '"');
  *_eol = 0;
  *_quoted = quoted;
  if (quoted) {
    src++;
  }

  while(*src) {
    if (!quoted || '"' == src[0]) {
      switch (src[quoted]) {
      default:
        break;
      case '\n': /* EOL */
        *_eol = 1;
      /* fallthrough */
      case ',': /* EOC */
        if(len<MAXPDSTRING) {
          dst[len++]=0;
        }
        dst[MAXPDSTRING-1] = 0;
        return src+1+quoted;
      case '"': /* quote */
        if(quoted) {
          src++;
        }
        break;
      }
    }
    if(len<MAXPDSTRING) {
      dst[len++]=*src;
    }
    src++;
  }
  dst[MAXPDSTRING-1] = 0;
  return src;
}

static const char*parse_fudi(const char*src, char dst[MAXPDSTRING],
                             int *_eol, int*_quoted)
{
  size_t len = 0;
  *_quoted = 0;
  *_eol = 0;

  while(*src) {
    char c = *src++;
    char c2;
    switch (c) {
    case '\\': /* quoting */
      c2=c;
      c=*src++;
      switch(c) {
      case ',':
      case ';':
      case '\t':
      case ' ':
      case '\n':
      case '\r':
        break;
      default:
        if(len<MAXPDSTRING) {
          dst[len++]=c2;
        }
        break;
      }
      break;
    case ';':
      *_eol = 1;
    case '\n':
    case '\r': /* EOL/EOA */
    case '\t':
    case ' ':  /* EOA */
      goto atomend;
      break;
    }
    if(len<MAXPDSTRING) {
      dst[len++]=c;
    }
  }
atomend:
  if(len<MAXPDSTRING) {
    dst[len++]=0;
  }

  while(*src) {
    switch(*src) {
    /* skip remaining whitespace */
    case '\n':
    case '\r':
    case ' ':
    case '\t':
      break;
    default:
      return src;
    }
    src++;
  }
  return src;
}

static const char*parse_txt(const char*src, char dst[MAXPDSTRING],
                            int *_eol, int*_quoted)
{
  size_t len = 0;
  *_quoted = 0;
  *_eol = 0;

  while(*src) {
    char c = *src++;
    char c2;
    switch (c) {
    case '\\': /* quoting */
      c2=c;
      c=*src++;
      switch(c) {
      case ',':
      case ';':
      case '\t':
      case ' ':
      case '\n':
      case '\r':
        break;
      default:
        if(len<MAXPDSTRING) {
          dst[len++]=c2;
        }
        break;
      }
      break;
    case '\n':
    case '\r': /* EOL/EOA */
      *_eol = 1;
    case '\t':
    case ' ':  /* EOA */
      goto atomend;
      break;
    }
    if(len<MAXPDSTRING) {
      dst[len++]=c;
    }
  }
atomend:
  if(len<MAXPDSTRING) {
    dst[len++]=0;
  }

  while(*src) {
    switch(*src) {
    /* skip remaining whitespace */
    case '\n':
    case '\r':
      *_eol = 1;
    case ' ':
    case '\t':
      break;
    default:
      return src;
    }
    src++;
  }
  return src;
}

typedef const char*(*t_parsefn)(const char*src, char dst[MAXPDSTRING],
                                int *_eol, int*_symbol);

static void msgfile_str2parse(t_msgfile *x, const char*src,
                              t_parsefn parsefn)
{
  t_binbuf*bbuf=binbuf_new();
  char atombuf[MAXPDSTRING + 1];
  while(*src) {
    int issymbol = 0;
    int iseol = 0;
    src = parsefn(src, atombuf, &iseol, &issymbol);
    atombuf[MAXPDSTRING] = 0;
    if(*atombuf) {
      t_atom a;
      str2atom(atombuf, &a, issymbol);
      binbuf_add(bbuf, 1, &a);
    }
    if(iseol) {
      t_atom*argv = binbuf_getvec(bbuf);
      int argc =  binbuf_getnatom(bbuf);
      add_currentnode(x);
      write_currentnode(x, argc, argv);
      binbuf_clear(bbuf);
    }
  }

  do {
    t_atom*argv = binbuf_getvec(bbuf);
    int argc =  binbuf_getnatom(bbuf);
    if(argc) {
      add_currentnode(x);
      write_currentnode(x, argc, argv);
    }
  } while(0);
  binbuf_free(bbuf);
  delete_emptynodes(x);
}

static int nextsemi(t_atom*argv, int argc)
{
  int count = 0;
  for(count=0; count<argc; count++) {
    if (A_SEMI==argv[count].a_type) {
      return count + 1;
    }
  }
  return 0;
}

static void msgfile_addbinbuf(t_msgfile *x, t_binbuf*bbuf)
{
  t_atom*argv = binbuf_getvec(bbuf);
  int argc =  binbuf_getnatom(bbuf);

  while(argc>0) {
    int next = nextsemi(argv, argc);
    if(next>1) {
      add_currentnode(x);
      write_currentnode(x, next-1, argv);
    } else if (next<1) {
      add_currentnode(x);
      write_currentnode(x, argc, argv);
      break;
    }
    argv+=next;
    argc-=next;
  }
  delete_emptynodes(x);
}

static char* escape_pd(const char*src, char*dst)
{
  /* ',' -> '\,'; ' ' -> '\ ' */
  char*dptr = dst;
  while(*src) {
    switch(*src) {
    default:
      break;
#if 0
    /* Pd already escapes these for us... */
    case ',':
    case ';':
#endif
    case ' ':
    case '\n':
    case '\r':
    case '\t':
      *dptr++ = '\\';
      break;
    case 0:
      *dptr++ = 0;
      return dst;
    }
    *dptr++=*src++;
  }
  *dptr++ = 0;
  return dst;
}
static char* escape_csv(const char*src, char*dst)
{
  /* if there are special characters in the string, quote everything */
  int needsquotes = 0;
  const char*sptr;
  char*dptr = dst;
  for(sptr = src; *sptr; sptr++) {
    switch(*sptr) {
    default:
      break;
    case ',':
    case '"':
    case '\n':
      needsquotes = 1;
      break;
    }
    if(needsquotes) {
      break;
    }
  }
  if (needsquotes) {
    *dptr++ = '"';
  }

  for(sptr = src; *sptr; sptr++) {
    switch(*sptr) {
    default:
      break;

    /* unescape "\," and "\;" */
    case '\\':
      switch(sptr[1]) {
      default:
        break;
      case ',':
      case ';':
      case '\\':
        sptr++;
        break;
      }
      break;
    }

    /* escape quotes */
    switch(*sptr) {
    default:
      break;
    case '"':
      *dptr++ = '"';
      break;
    }
    *dptr++=*sptr;
  }

  if (needsquotes) {
    *dptr++ = '"';
  }
  *dptr++ = 0;
  return dst;
}

typedef char*(*t_escapefn)(const char*src, char*dst);


/* ************************************************************************ */
/* object methods                                                           */

static void msgfile_rewind(t_msgfile *x)
{
  //  while (x->current && x->current->previous) x->current = x->current->previous;
  x->current = x->start;
  x->previous=0;
}
static void msgfile_end(t_msgfile *x)
{
  if (!x->current) {
    return;
  }
  while (x->current->next) {
    x->previous= x->current;
    x->current = x->current->next;
  }

}
static void msgfile_goto(t_msgfile *x, t_float f)
{
  int i = f;

  if (i<0) {
    return;
  }
  if (!x->current) {
    return;
  }
  msgfile_rewind(x);

  while (i-- && x->current->next) {
    x->previous= x->current;
    x->current = x->current->next;
  }
}
static void msgfile_skip(t_msgfile *x, t_float f)
{
  int i;
  int counter = 0;

  t_msglist *dummy = x->start;

  if (!f) {
    return;
  }
  if (!x->current) {
    return;
  }

  while (dummy->next && dummy!=x->current) {
    counter++;
    dummy=dummy->next;
  }

  i = counter + f;
  if (i<0) {
    i=0;
  }

  msgfile_goto(x, i);
}

static void msgfile_clear(t_msgfile *x)
{
  /* find the beginning */
  msgfile_rewind(x);

  while (x->current) {
    delete_currentnode(x);
  }
}

static int atom2rangeint(t_atom*a, int range)
{
  t_float f = atom_getfloat(a);
  if (f>range) {
    return range;
  }
  if (f<-range) {
    return -range;
  }
  return (unsigned int)f;
}
static void msgfile_delete(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                           t_atom *av)
{
  int count = node_count(x);
  int pos = atom2rangeint(av+0, count);
  if (!is_float(av)) {
    pd_error(x, "[msgfile] illegal deletion index %s",
             atom_getsymbol(av)->s_name);
    return;
  }
  if (count<1) {
    return;
  }
  if (ac==1) {
    int oldwhere = node_wherearewe(x);

    if (pos<0) {
      return;
    }
    if (oldwhere > pos) {
      oldwhere--;
    }
    msgfile_goto(x, pos);
    delete_currentnode(x);
    msgfile_goto(x, oldwhere);
  } else if (ac==2) {
    int pos1 = pos;
    int pos2 = atom2rangeint(av+1, count);
    if (!is_float(av+1)) {
      pd_error(x, "[msgfile] illegal deletion range %s",
               atom_getsymbol(av+1)->s_name);
      return;
    }

    if ((pos1 < pos2) || (pos2 == -1)) {
      if (pos2+1) {
        delete_region(x, pos1, pos2+1);
      } else {
        delete_region(x, pos1, -1);
      }
    } else {
      delete_region(x, pos1+1, -1);
      delete_region(x, 0, pos2);
    }
  } else {
    delete_currentnode(x);
  }
}

static void msgfile_add(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                        t_atom *av)
{
  msgfile_end(x);
  add_currentnode(x);
  write_currentnode(x, ac, av);
}
static void msgfile_add2(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                         t_atom *av)
{
  msgfile_end(x);
  if (x->current) {
    if(x->current->previous) {
      x->current = x->current->previous;
    }
  } else {
    add_currentnode(x);
  }
  write_currentnode(x, ac, av);
  if (x->current && x->current->next) {
    x->previous= x->current;
    x->current = x->current->next;
  }
}
static void msgfile_append(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                           t_atom *av)
{
  add_currentnode(x);
  write_currentnode(x, ac, av);
}
static void msgfile_append2(t_msgfile *x, t_symbol *s, int ac, t_atom *av)
{
  if(!x->current) {
    add_currentnode(x);
  }

  if (x->current->thislist) {
    write_currentnode(x, ac, av);
  } else {
    msgfile_append(x, s, ac, av);
  }
}
static void msgfile_insert(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                           t_atom *av)
{
  t_msglist *cur = x->current;
  insert_currentnode(x);
  write_currentnode(x, ac, av);
  x->current = cur;
}
static void msgfile_insert2(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                            t_atom *av)
{
  t_msglist *cur = x->current;
  if ((x->current) && (x->current->previous)) {
    x->current = x->current->previous;
  }
  write_currentnode(x, ac, av);
  x->current = cur;
}

static void msgfile_set(t_msgfile *x, t_symbol *s, int ac, t_atom *av)
{
  msgfile_clear(x);
  msgfile_add(x, s, ac, av);
}

static void msgfile_replace(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                            t_atom *av)
{
  if(x->current) {
    if(x->current->thislist) {
      freebytes(x->current->thislist, sizeof(x->current->thislist));
    }
    x->current->thislist = 0;
    x->current->n = 0;
  } else {
    add_currentnode(x);
  }
  write_currentnode(x, ac, av);
}

static void msgfile_flush(t_msgfile *x)
{
  t_msglist *cur = x->start;
  while (cur && cur->thislist) {
    outlet_list(x->x_obj.ob_outlet, gensym("list"), cur->n, cur->thislist);
    cur = cur->next;
  }
}
static void msgfile_this(t_msgfile *x)
{
  if ((x->current) && (x->current->thislist)) {
    outlet_list(x->x_obj.ob_outlet, gensym("list"), x->current->n,
                x->current->thislist);
  } else {
    outlet_bang(x->x_secondout);
  }
}
static void msgfile_next(t_msgfile *x)
{
  if ((x->current) && (x->current->next)) {
    t_msglist *next = x->current->next;
    if (next->thislist) {
      outlet_list(x->x_obj.ob_outlet, gensym("list"), next->n, next->thislist);
    } else {
      outlet_bang(x->x_secondout);
    }
  } else {
    outlet_bang(x->x_secondout);
  }
}
static void msgfile_prev(t_msgfile *x)
{
  t_msglist*prev=0;

  if ((x->current) && (x->current->previous)) {
    prev = x->current->previous;
  } else if (x->previous) {
    prev = x->previous;
  }
  if(prev) {
    if (prev->thislist) {
      outlet_list(x->x_obj.ob_outlet, gensym("list"), prev->n, prev->thislist);
    } else {
      outlet_bang(x->x_secondout);
    }

  } else {
    outlet_bang(x->x_secondout);
  }
}

static void msgfile_bang(t_msgfile *x)
{
  if ((x->current) && (x->current->thislist)) {
    t_msglist*cur=x->current;
    x->current=cur->next;
    x->previous=cur;
    outlet_list(x->x_obj.ob_outlet, gensym("list"), cur->n, cur->thislist);
  } else {
    outlet_bang(x->x_secondout);
  }
}

static void msgfile_find(t_msgfile *x, t_symbol *UNUSED(s), int ac,
                         t_atom *av)
{
  t_msglist *found = 0;
  t_msglist *cur=x->current;

  while (cur) {
    int n = cur->n;
    int equal = 1;
    t_atom *that = av;
    t_atom *this = cur->thislist;

    if(0==this) {
      cur=cur->next;
      continue;
    }

    if (ac < n) {
      n = ac;
    }

    while (n-->0) {
      if ( (strcmp("*", atom_getsymbol(that)->s_name) && atomcmp(that, this)) ) {
        equal = 0;
      }

      that++;
      this++;
    }

    if (equal) {
      found = cur;
      break;
    }

    cur=cur->next;
  }

  if(found) {
    x->current = found;
    x->previous= found->previous;
    outlet_float(x->x_secondout, node_wherearewe(x));
    if(found->n && found->thislist) {
      outlet_list(x->x_obj.ob_outlet, gensym("list"), found->n,
                  found->thislist);
    }
  } else {
    outlet_bang(x->x_secondout);
  }
}

static void msgfile_where(t_msgfile *x)
{
  if (x->current && x->current->thislist) {
    outlet_float(x->x_secondout, node_wherearewe(x));
  } else {
    outlet_bang(x->x_secondout);
  }
}


static void msgfile_sort(t_msgfile *x, t_symbol *s0, t_symbol*s1,
                         t_symbol*r)
{
  pd_error(x, "sorting not implemented yet: '%s', '%s' -> '%s'", s0->s_name,
           s1->s_name, r->s_name);


#if 0
  int step = argc, n;
  t_atom *atombuf = (t_atom *)getbytes(sizeof(t_atom) * argc);
  t_float *buf;
  t_int   *idx;

  int i, loops = 1;

  sort_buffer(x, argc, argv);
  buf = x->buffer;
  idx = x->indices;

  while (step > 1) {
    step = (step % 2)?(step+1)/2:step/2;

    i = loops;
    loops += 2;

    while(i--) { /* there might be some optimization in here */
      for (n=0; n<(argc-step); n++) {
        if (buf[n] > buf[n+step]) {
          t_int   i_tmp = idx[n];
          t_float f_tmp = buf[n];
          buf[n]        = buf[n+step];
          buf[n+step]   = f_tmp;
          idx[n]        = idx[n+step];
          idx[n+step]   = i_tmp;
        }
      }
    }
  }
#endif

}


/* ********************************** */
/* file I/O                           */

static void msgfile_read2(t_msgfile *x, t_symbol *filename,
                          t_symbol *sformat)
{
  int rmode = 0;

  int fd=0;
  FILE*fil=NULL;
  long readlength, length;
  char filnam[MAXPDSTRING];
  char buf[MAXPDSTRING], *bufptr, *readbuf;
  const char*dirname=canvas_getdir(x->x_canvas)->s_name;
  t_parsefn parsefn = 0;

  t_msgfile_format format = symbol2format(x, sformat);

#ifdef __WIN32__
  rmode |= O_BINARY;
#endif

  switch(format) {
  case FORMAT_CSV:
    parsefn = parse_csv;
    break;
  case FORMAT_TXT:
    parsefn = parse_txt;
    break;
  case FORMAT_FUDI:
    parsefn = parse_fudi;
    break;
  default:
    parsefn = 0;
  }

  if( !parsefn ) {
    /* use Pd's own parser
     * this gives somewhat weird results with escaped LF,
     * but is consistent with how [textfile] reads the data
     */
    t_binbuf*bbuf = binbuf_new();
    binbuf_read_via_canvas(bbuf, filename->s_name, x->x_canvas,
                           (FORMAT_CR == format));
    msgfile_addbinbuf(x, bbuf);
    binbuf_free(bbuf);
    return;
  }

  /* cannot use Pd's binbuf parser, so we do our own thing */

  fd = open_via_path(dirname,
                     filename->s_name, "", buf, &bufptr, MAXPDSTRING, 0);

  if (fd < 0) {
    /* open via path failed, fall back */
    fd=sys_open(filename->s_name, rmode);
    if(fd < 0) {
      pd_error(x, "can't open in %s/%s",  dirname, filename->s_name);
      return;
    } else {
      sys_close(fd);
      snprintf(filnam, MAXPDSTRING, "%s", filename->s_name);
    }
  } else {
    sys_close(fd);
    if(snprintf(filnam, MAXPDSTRING, "%s/%s", buf, bufptr) < 0) {
      pd_error(x, "can't create in '%s/%s'",  buf, bufptr);
      return;
    }
  }
  filnam[MAXPDSTRING-1]=0;

  fil=sys_fopen(filnam, "rb");
  if(fil==NULL) {
    pd_error(x, "could not open '%s'", filnam);
    return;
  }
  fseek(fil, 0, SEEK_END);
  length=ftell(fil);
  fseek(fil, 0, SEEK_SET);

  if (!(readbuf = t_getbytes(length+1))) {
    pd_error(x, "msgfile_read: could not reserve %ld bytes to read into",
             length);
    sys_fclose(fil);
    return;
  }

  /* read */
  if ((readlength = fread(readbuf, sizeof(char), length, fil)) < length) {
    pd_error(x, "msgfile_read: unable to read %s: %ld of %ld", filnam,
             readlength, length);
    sys_fclose(fil);
    t_freebytes(readbuf, length+1);
    return;
  }
  /* close */
  sys_fclose(fil);

  /* we overallocated readbuf by 1, so we can store a terminating 0 */
  readbuf[length] = 0;

  msgfile_str2parse(x, readbuf, parsefn);

  t_freebytes(readbuf, length+1);
}
static void msgfile_read(t_msgfile *x, t_symbol *filename,
                         t_symbol *format)
{
  msgfile_clear(x);
  msgfile_read2(x, filename, format);
}


static void msgfile_write(t_msgfile *x, t_symbol *filename,
                          t_symbol *sformat)
{
  char buf[MAXPDSTRING];
  t_msglist *cur = x->start;

  char filnam[MAXPDSTRING];
  char separator, eol;
  t_msgfile_format format = symbol2format(x, sformat);
  int errcount = 0;
  t_escapefn escapefn = escape_pd;

  FILE *f=0;

  switch (format) {
  case FORMAT_TXT:
  case FORMAT_CR:
    separator = ' ';
    eol = 0;
    break;
  case FORMAT_CSV:
    separator = ',';
    eol = 0;
    escapefn = escape_csv;
    break;
  default:
    separator = ' ';
    eol = ';';
    break;
  }


  /* open */
  canvas_makefilename(x->x_canvas, filename->s_name,
                      buf, MAXPDSTRING);
  sys_bashfilename(buf, filnam);
  f = sys_fopen(filnam, "w");
  if (!f) {
    pd_error(x, "msgfile : failed to open %s", filnam);
    return;
  }

  for(cur = x->start; cur; cur=cur->next) {
    int i;
    for(i=0; i<cur->n; i++) {
      t_atom*a = cur->thislist + i;
      switch(a->a_type) {
      case A_FLOAT:
        errcount += (fprintf(f, "%g", atom_getfloat(a)) < 1);
        break;
      case A_POINTER:
        errcount += (fprintf(f, "%p", a->a_w.w_gpointer) < 1);
        break;
      default: {
        int mylen = 0;
        char mytext[MAXPDSTRING];
        char mytext2[MAXPDSTRING*2];
        atom_string(a, mytext, MAXPDSTRING);
        escapefn(mytext, mytext2);
        mylen = strnlen(mytext2, MAXPDSTRING);
        errcount += (fwrite(mytext2, mylen, sizeof(char), f) < 1);
      }
      }
      if(i + 1 < cur->n) {
        errcount += (fwrite(&separator, sizeof(char), 1, f) < 1);
      }
    }
    if(eol) {
      errcount += (fwrite(&eol, sizeof(char), 1, f) < 1);
    }
    errcount += (fwrite("\n", sizeof(char), 1, f) < 1);
  }

  if (errcount > 0) {
    pd_error(x, "msgfile : failed to write '%s': % d errors", filnam,
             errcount);
  }
  /* close */
  sys_fclose(f);
}

/* ********************************** */
/* misc                               */

static void msgfile_print(t_msgfile *x)
{
  t_msglist *cur = x->start;
  int j=0;
  post("--------- msgfile contents: -----------");

  while (cur) {
    t_msglist *dum=cur;
    int i;
    startpost("line %d:", j);
    j++;
    for (i = 0; i < dum->n; i++) {
      t_atom *a = dum->thislist + i;
      postatom(1, a);
    }
    endpost();
    cur = cur->next;
  }
}

static void msgfile_help(t_msgfile *UNUSED(x))
{
  post("\n"HEARTSYMBOL " msgfile\t:: handle and store files of lists");
  post("goto <n>\t: goto line <n>"
       "\nrewind\t\t: goto the beginning of the file"
       "\nend\t\t: goto the end of the file"
       "\nskip <n>\t: move relatively to current position"
       "\nbang\t\t: output current line and move forward"
       "\nprev\t\t: output previous line"
       "\nthis\t\t: output this line"
       "\nnext\t\t: output next line"
       "\nflush\t\t: output all lines");
  post("set <list>\t: clear the buffer and add <list>"
       "\nadd <list>\t: add <list> at the end of the file"
       "\nadd2 <list>\t: append <list> to the last line of the file"
       "\nappend <list>\t: append <list> at the current position"
       "\nappend2 <list>\t: append <list> to the current line"
       "\ninsert <list>\t: insert <list> at the current position"
       "\ninsert2 <list>\t: append <list> to position [current-1]"
       "\nreplace <list>\t: replace current line by <list>"
       "\ndelete [<pos> [<pos2>]]\t: delete lines or regions"
       "\nclear\t\t: delete the whole buffer");
  post("where\t\t: output current position"
       "\nfind <list>\t: search for <list>"
       "\nread <file> [<format>]\t: read <file> as <format>"
       "\nwrite <file> [<format>]\t: write <file> as <format>"
       "\n\t\t: valid <formats> are\t: pd, cr, fudi, txt, csv"
       "\n\nprint\t\t: show buffer (for debugging)"
       "\nhelp\t\t: show this help");
  post("creation: \"msgfile [<format>]\": <format> defines fileaccess-mode(default is 'pd')");
}
static void msgfile_free(t_msgfile *x)
{
  msgfile_clear(x);
  freebytes(x->current, sizeof(t_msglist));
}

static void *msgfile_new(t_symbol *UNUSED(s), int argc, t_atom *argv)
{
  t_msgfile *x = (t_msgfile *)pd_new(msgfile_class);

  /* an empty node indicates the end of our listbuffer */
  x->current = 0;
  x->start   = 0;
  x->previous= 0;

  x->format=FORMAT_PD; /* that's the default */

  if ((argc==1) && (argv->a_type == A_SYMBOL)) {
    x->format = symbol2format(x, atom_getsymbol(argv));
  }

  outlet_new(&x->x_obj, gensym("list"));
  x->x_secondout = outlet_new(&x->x_obj, gensym("float"));
  x->x_canvas = canvas_getcurrent();

  x->eol=' ';
  x->separator=',';

  return (x);
}

ZEXY_SETUP void msgfile_setup(void)
{
  msgfile_class = zexy_new("msgfile",
                           msgfile_new, msgfile_free, t_msgfile, 0, "*");
  zexy_addmethod(msgfile_class, (t_method)msgfile_goto, "goto", "F");
  zexy_addmethod(msgfile_class, (t_method)msgfile_rewind, "rewind", "");
  zexy_addmethod(msgfile_class, (t_method)msgfile_rewind, "begin", "");
  zexy_addmethod(msgfile_class, (t_method)msgfile_end, "end", "");
  zexy_addmethod(msgfile_class, (t_method)msgfile_next, "next", "F");
  zexy_addmethod(msgfile_class, (t_method)msgfile_prev, "prev", "F");

  zexy_addmethod(msgfile_class, (t_method)msgfile_skip, "skip", "F");

  zexy_addmethod(msgfile_class, (t_method)msgfile_set, "set", "*");

  zexy_addmethod(msgfile_class, (t_method)msgfile_clear, "clear", "");
  zexy_addmethod(msgfile_class, (t_method)msgfile_delete, "delete", "*");

  zexy_addmethod(msgfile_class, (t_method)msgfile_add, "add", "*");
  zexy_addmethod(msgfile_class, (t_method)msgfile_add2, "add2", "*");
  zexy_addmethod(msgfile_class, (t_method)msgfile_append, "append", "*");
  zexy_addmethod(msgfile_class, (t_method)msgfile_append2, "append2", "*");
  zexy_addmethod(msgfile_class, (t_method)msgfile_insert, "insert", "*");
  zexy_addmethod(msgfile_class, (t_method)msgfile_insert2, "insert2", "*");

  zexy_addmethod(msgfile_class, (t_method)msgfile_replace, "replace", "*");

  zexy_addmethod(msgfile_class, (t_method)msgfile_find, "find", "*");

  zexy_addmethod(msgfile_class, (t_method)msgfile_read, "read", "sS");
  zexy_addmethod(msgfile_class, (t_method)msgfile_read2, "read2", "sS");
  zexy_addmethod(msgfile_class, (t_method)msgfile_write, "write", "sS");
  zexy_addmethod(msgfile_class, (t_method)msgfile_print, "print", "");
  zexy_addmethod(msgfile_class, (t_method)msgfile_flush, "flush", "");

  class_addbang(msgfile_class, msgfile_bang);
  zexy_addmethod(msgfile_class, (t_method)msgfile_this, "this", "");
  zexy_addmethod(msgfile_class, (t_method)msgfile_where, "where", "");


  zexy_addmethod(msgfile_class, (t_method)msgfile_sort, "sort", "sss");

  zexy_addmethod(msgfile_class, (t_method)msgfile_help, "help", "");

  zexy_register("msgfile");
}
