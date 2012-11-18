/******************************************************
 *
 * zexy - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1999:forum::für::umläute:2004
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

/* print the incoming message as raw as possible */

#include "zexy.h"
#include <stdio.h>

static t_class *rawprint_class;

typedef struct _rawprint {
  t_object  x_obj;
  t_symbol*label;
} t_rawprint;

static void rawprint_any(t_rawprint *x, t_symbol*s, int argc, t_atom*argv)
{
  char buf[MAXPDSTRING];
  if(x->label) {
    startpost("%s: ", x->label->s_name);
  }
  startpost("\"%s\"", s->s_name);
  while(argc--) {
    switch(argv->a_type) {
    case A_FLOAT:
      snprintf(buf, MAXPDSTRING-1, "%f", atom_getfloat(argv));
      break;
    case A_SYMBOL:
      snprintf(buf, MAXPDSTRING-1, "'%s'", atom_getsymbol(argv)->s_name);
      break;
    case A_POINTER:
      snprintf(buf, MAXPDSTRING-1, "pointer[0x%X]", argv->a_w.w_gpointer);
      break;
    case A_SEMI:
      snprintf(buf, MAXPDSTRING-1, "SEMI");
      break;
    case A_COMMA:
      snprintf(buf, MAXPDSTRING-1, "COMMA");
      break;
    case A_DEFFLOAT:
      snprintf(buf, MAXPDSTRING-1, "DEFFLOAT[%f]", atom_getfloat(argv));
      break;
    case A_DEFSYM:
      snprintf(buf, MAXPDSTRING-1, "DEFSYM['%s']", atom_getsymbol(argv)->s_name);
      break;
    case A_DOLLAR:
      snprintf(buf, MAXPDSTRING-1, "DOLLAR['%s']", atom_getsymbol(argv)->s_name);
      break;
    case A_DOLLSYM:
      snprintf(buf, MAXPDSTRING-1, "DOLLSYM['%s']", atom_getsymbol(argv)->s_name);
      break;
    case A_GIMME:
      snprintf(buf, MAXPDSTRING-1, "GIMME");
      break;
    case A_CANT: // we _really_ cannot do CANT
      snprintf(buf, MAXPDSTRING-1, "CANT");
      break;
    default:
      snprintf(buf, MAXPDSTRING-1, "unknown[%d]", argv->a_type);
    }
    buf[MAXPDSTRING-1]=0;
    
    startpost(" %s", buf);
    argv++;
  }
  endpost();

}

static void *rawprint_new(t_symbol*s)
{
  t_rawprint *x = (t_rawprint *)pd_new(rawprint_class);
  x->label=NULL;
  if(s&&gensym("")!=s)
    x->label=s;

  return (x);
}

void rawprint_setup(void) {
  rawprint_class = class_new(gensym("rawprint"),
                             (t_newmethod)rawprint_new,
                             0, sizeof(t_rawprint),
                             CLASS_DEFAULT, A_DEFSYMBOL, 0);

  class_addanything(rawprint_class, rawprint_any);
  zexy_register("rawprint");
}
