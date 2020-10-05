/*
 * fwriteln: writes messages continuously into a file
 *
 * (c) 1999-2011 IOhannes m zmölnig, forum::für::umläute, institute of electronic music and acoustics (iem)
 * (c) 2007 Franz Zotter <zotter@iem.at>
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

#ifdef __WIN32__
# define snprintf _snprintf
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* fwriteln: writes messages continuously into a file that
 * doesn't necessarily need to fit into the RAM of your system
 *
 * Franz Zotter zotter@iem.at, 2007
 * Institute of Electronic Music and Acoustics
 *
 * parts of this externals were copied from Iohannes zmoelnig's
 * iemmatrix
 */

static t_class *fwriteln_class=NULL;

typedef struct fwriteln {
  t_object x_ob;
  FILE *x_file;
  char *x_filename;
  char *x_textbuf;
  char linebreak_chr[3];
  char format_string_afloats[MAXPDSTRING];
} t_fwriteln;


static void fwriteln_close (t_fwriteln *x)
{
  if(x->x_file) {
    sys_fclose(x->x_file);
  }
  x->x_file=0;
  if(x->x_filename) {
    free(x->x_filename);
  }
  x->x_filename=0;
  if(x->x_textbuf) {
    freebytes(x->x_textbuf, MAXPDSTRING + 1);
  }
  x->x_textbuf=0;
}

static void string_copy(const char* const from, char** to)
{
  if ((*to = malloc(strlen(from) + 1))) {
    strcpy(*to, from);
  }
}

static void fwriteln_open (t_fwriteln *x, t_symbol *s, t_symbol*type)
{
  char* filename;

  string_copy(s->s_name, &filename);

  sys_bashfilename (filename, filename);

  fwriteln_close (x);

  /*   if(0==type || type!=gensym("cr")) {
       pd_error(x, "unknown type '%s'", (type)?type->s_name:"");
       return;
     }*/

  if (type==gensym("cr")) {
    strcpy(x->linebreak_chr,"\n");
  } else {
    strcpy(x->linebreak_chr,";\n");
  }

  if (!(x->x_file=sys_fopen(filename, "w"))) {
    pd_error(x, "failed to open %128s",filename);
    free(filename);
    return;
  }
  string_copy(filename, &x->x_filename);
  free(filename);
  x->x_textbuf = (char *) getbytes (MAXPDSTRING + 1);
}

static void fwriteln_write (t_fwriteln *x, t_symbol *s, int argc,
                            t_atom *argv)
{
  char *text=x->x_textbuf;
  if (x->x_file) {
    int length=0;
    if ((s!=gensym("list"))||(argv->a_type==A_SYMBOL)) {
      snprintf(text,MAXPDSTRING,"%s ", s->s_name);
      text[MAXPDSTRING-1]=0;
      length=strlen(text);
      if (fwrite(text, length*sizeof(char),1,x->x_file) < 1) {
        pd_error(x, "failed to write %128s",x->x_filename);
        freebytes (text, MAXPDSTRING * sizeof(char));
        fwriteln_close(x);
        return;
      }
    }
    while (argc--) {
      switch (argv->a_type) {
      case A_FLOAT:
        snprintf(text,MAXPDSTRING,x->format_string_afloats,
                 atom_getfloat(argv));
        text[MAXPDSTRING-1]=0;
        length=strlen(text);
        if (fwrite(text, length*sizeof(char),1,x->x_file) < 1) {
          pd_error(x, "failed to write %128s",x->x_filename);
          freebytes (text, MAXPDSTRING * sizeof(char));
          fwriteln_close(x);
          return;
        }
        break;
      case A_SYMBOL:
        snprintf(text,MAXPDSTRING,"%s ", atom_getsymbol(argv)->s_name);
        text[MAXPDSTRING-1]=0;
        length=strlen(text);
        if (fwrite(text, length*sizeof(char),1,x->x_file) < 1) {
          pd_error(x, "failed to write %128s",x->x_filename);
          freebytes (text, MAXPDSTRING * sizeof(char));
          fwriteln_close(x);
          return;
        }
        break;
      case A_COMMA:
        snprintf(text,MAXPDSTRING,", ");
        length=strlen(text);
        if (fwrite(text, length*sizeof(char),1,x->x_file) < 1) {
          pd_error(x, "failed to write %128s",x->x_filename);
          freebytes (text, MAXPDSTRING * sizeof(char));
          fwriteln_close(x);
          return;
        }
        break;
      case A_SEMI:
        snprintf(text,MAXPDSTRING,"; ");
        length=strlen(text);
        if (fwrite(text, length*sizeof(char),1,x->x_file) < 1) {
          pd_error(x, "failed to write %128s",x->x_filename);
          freebytes (text, MAXPDSTRING * sizeof(char));
          fwriteln_close(x);
          return;
        }
        break;
      default:
        break;
      }
      argv++;
    }

    snprintf(text,MAXPDSTRING,"%s", x->linebreak_chr);
    length=strlen(text);
    if (fwrite(text, length*sizeof(char),1,x->x_file) < 1) {
      pd_error(x, "failed to write %128s",x->x_filename);
      freebytes (text, MAXPDSTRING * sizeof(char));
      fwriteln_close(x);
      return;
    }
  } else {
    pd_error(x, "no file opened for writing");
  }
}
static void fwriteln_free (t_fwriteln *x)
{
  fwriteln_close(x);
}

static void *fwriteln_new(t_symbol* UNUSED(s), int argc, t_atom *argv)
{
#define MAXFMTSTRING 10
  int k;
  char float_format = 'g';
  char sign=0;
  char width_str[MAXFMTSTRING];
  char precision_str[MAXFMTSTRING];
  t_fwriteln *x = (t_fwriteln *)pd_new(fwriteln_class);
  memset(width_str, 0, MAXFMTSTRING);
  memset(precision_str, 0, MAXFMTSTRING);

  x->x_filename=0;
  x->x_file=0;
  x->x_textbuf=0;

  for (k=0; k<argc; k++) {
    t_symbol*S = atom_getsymbol(&argv[k]);
    if (gensym("p")==S) {
      if ((k+1>=argc)||(argv[k+1].a_type!=A_FLOAT)) {
        pd_error(x, "fwriteln: no value given for precision!");
      } else {
        int precision=(int)atom_getfloat(&argv[++k]);
        precision=(precision<0)?0:precision;
        precision=(precision>30)?30:precision;
        snprintf(precision_str, MAXFMTSTRING, ".%d",precision);
      }
    } else if (gensym("w")==S) {
      if ((k+1>=argc)||(argv[k+1].a_type!=A_FLOAT)) {
        pd_error(x, "fwriteln: no value given for width!");
      } else {
        int width=(int)atom_getfloat(&argv[++k]);
        width=(width<1)?1:width;
        width=(width>40)?40:width;
        snprintf(width_str, MAXFMTSTRING, "%d",width);
      }
    } else if (gensym("g")==S || gensym("f")==S || gensym("e")==S) {
      float_format = S->s_name[0];
    } else if (gensym("-")==S || gensym("+")==S) {
      sign = S->s_name[0];
    }
  }
  snprintf(x->format_string_afloats, MAXPDSTRING, "%%%c%s%s%c ", sign,
           width_str, precision_str, float_format);
  return (void *)x;
}


ZEXY_SETUP void fwriteln_setup(void)
{
  fwriteln_class = zexy_new("fwriteln",
                            fwriteln_new,  fwriteln_free, t_fwriteln, CLASS_DEFAULT, "*");
  zexy_addmethod(fwriteln_class, (t_method)fwriteln_open, "open", "sS");
  zexy_addmethod(fwriteln_class, (t_method)fwriteln_close, "close", "");
  class_addanything(fwriteln_class, (t_method)fwriteln_write);

  zexy_register("fwriteln");
}
