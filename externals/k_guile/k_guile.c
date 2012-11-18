/* --------------------------- k_guile  ----------------------------------- */
/*   ;; Kjetil S. Matheussen, 2004.                                             */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* ---------------------------------------------------------------------------- */



#include "libguile.h"

/* Need some more macros. */

#define POINTER_P(x) (((int) (x) & 3) == 0)
#define INTEGER_P(x) (! POINTER_P (x))

#define GET_INTEGER SCM_INUM 
#define MAKE_INTEGER  SCM_MAKINUM

#define MAKE_STRING(a) scm_mem2string(a,strlen(a))
#define EVAL(a) scm_eval_string(MAKE_STRING(a))

#define MAKE_SYM(a) gensym(SCM_SYMBOL_CHARS(a))

#define MAKE_POINTER(a) scm_ulong2num((unsigned long)a)
#define GET_POINTER(a) (void *)scm_num2ulong(a,0,"GET_POINTER()")

#define GET_X(a) ((t_k_guile *)GET_POINTER(a))

#define RU_ return SCM_UNSPECIFIED



#include "m_pd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdarg.h>


struct k_guile_workaround;

typedef struct k_guile
{
  t_object x_ob;

  int num_ins;
  int num_outs;

  struct k_guile_workaround **inlets;
  t_outlet **outlets;

  SCM inlet_func;
  SCM cleanup_func;

  char *filename;

  bool isinited;
} t_k_guile;

typedef struct k_guile_workaround{
  t_object x_ob;
  t_k_guile *x;
  t_inlet *inlet;
  int index;
  SCM func;
} t_k_guile_workaround;




#define KG_MAX(a,b) (((a)>(b))?(a):(b))
#define KG_MIN(a,b) (((a)<(b))?(a):(b))



static char *version = 
"k_guile v0.0.2, written by Kjetil S. Matheussen, k.s.matheussen@notam02.no";

static t_class *k_guile_class, *k_guile_workaroundclass;

static SCM pd_backtrace_run;
static SCM pd_backtrace_runx;
static SCM pd_backtrace_run1;
static SCM pd_backtrace_run2;
static SCM pd_backtrace_run3;
static SCM pd_backtrace_run4;
static SCM eval_string_func;



/*****************************************************************************************************
 *****************************************************************************************************
 *    Functions to evaluate large amount of scheme code from C.
 *****************************************************************************************************
 *****************************************************************************************************/

static char *evalstring=NULL;
static void eval2(char *string){
  char *new;
  if(evalstring==NULL){
    new=malloc(strlen(string)+1);
    sprintf(new,"%s",string);
  }else{
    new=malloc(strlen(evalstring)+strlen(string)+1);
    sprintf(new,"%s%s",evalstring,string);
    free(evalstring);
  }
  evalstring=new;
}
static void eval_file(FILE *file){
  char line[50000];
  for(;;){
    int c=fgetc(file);
    if(c==EOF) break;
    ungetc(c,file);
    fgets(line,49999,file);
    eval2(line);
  }
}
static SCM eval_do(void){
  //post(evalstring);
  SCM ret=EVAL(evalstring);
  free(evalstring);
  evalstring=NULL;
  return ret;
}





/*****************************************************************************************************
 *****************************************************************************************************
 *    Sending data to the guile side. Called either via bind or an inlet.
 *****************************************************************************************************
 *****************************************************************************************************/

static void k_guile_anything_do(t_k_guile *x,int index,SCM func,t_symbol *s, t_int argc, t_atom* argv){
  int lokke;
  SCM applyarg=SCM_EOL;

  for(lokke=argc-1;lokke>=0;lokke--){
    SCM to=SCM_BOOL_F;
    switch(argv[lokke].a_type){
    case A_NULL:
      to=SCM_EOL;
      break;
    case A_FLOAT:
      to=scm_make_real(atom_getfloatarg(lokke,argc,argv));
      break;
    case A_SYMBOL:
      to=scm_string_to_symbol(MAKE_STRING(atom_getsymbolarg(lokke,argc,argv)->s_name));
      break;
    default:
      post("Strange");
      break;
    }
    applyarg=scm_cons(to,applyarg);
  }

  if(index>=0){
    // Inlet
    scm_call_4(pd_backtrace_run3,x->inlet_func,MAKE_INTEGER(index),scm_string_to_symbol(MAKE_STRING(s->s_name)),applyarg);
  }else{
    // Binding
    if(s!=&s_float && s!=&s_list && s!=&s_symbol){
      applyarg=scm_cons(scm_string_to_symbol(MAKE_STRING(s->s_name)),applyarg);
    }
    if(s!=&s_list && GET_INTEGER(scm_length(applyarg))==1)
      applyarg=SCM_CAR(applyarg);
    scm_call_2(pd_backtrace_run1,func,applyarg);
  }
}

// Handles inlet>0 and bindings
static void k_guile_anything(t_k_guile_workaround *x2,t_symbol *s, t_int argc, t_atom* argv){
  if(x2->index>=0){
    // Inlet
    k_guile_anything_do(x2->x,x2->index,0,s,argc,argv);
  }else{
    // Binding
    k_guile_anything_do(NULL,x2->index,x2->func,s,argc,argv);
  }
}

// Handles first inlet
static void k_guile_anything_first(t_k_guile *x,t_symbol *s, t_int argc, t_atom* argv){
  k_guile_anything_do(x,0,0,s,argc,argv);
}






/*****************************************************************************************************
 *****************************************************************************************************
 *    Initialization, called from the guile side.
 *****************************************************************************************************
 *****************************************************************************************************/

static SCM gpd_inlets(SCM instance,SCM num_ins){
  int lokke;
  t_k_guile *x=GET_X(instance);

  if(x->isinited==true) goto exit;

  x->num_ins=GET_INTEGER(num_ins);
  x->inlets=calloc(sizeof(t_k_guile_workaround*),x->num_ins);

  for(lokke=1;lokke<x->num_ins;lokke++){
    t_k_guile_workaround *x2;
    x2=(t_k_guile_workaround*)pd_new(k_guile_workaroundclass);
    x->inlets[lokke]=x2;
    x2->x=x;
    x2->index=lokke;
    x2->inlet=inlet_new(&x->x_ob,(t_pd*)x2,0,0);
  }

 exit:
  RU_;
}
static SCM gpd_outlets(SCM instance,SCM num_outs){
  int lokke;
  t_k_guile *x=GET_X(instance);

  if(x->isinited==true) goto exit;

  x->num_outs=GET_INTEGER(num_outs);
  x->outlets=calloc(sizeof(t_outlet*),x->num_outs);

  for(lokke=0;lokke<x->num_outs;lokke++){
    x->outlets[lokke] = outlet_new(&x->x_ob, gensym("anything"));
  }

 exit:
  RU_;
}

static SCM gpd_inited_p(SCM instance){
  t_k_guile *x=GET_X(instance);
  if(x->isinited==true) return SCM_BOOL_T;
  return SCM_BOOL_F;
}
static SCM gpd_get_num_inlets(SCM instance){
  t_k_guile *x=GET_X(instance);
  return MAKE_INTEGER(x->num_ins);
}
static SCM gpd_get_num_outlets(SCM instance){
  t_k_guile *x=GET_X(instance);
  return MAKE_INTEGER(x->num_outs);
}



/*****************************************************************************************************
 *****************************************************************************************************
 *    Binding and unbinding. Called from the guile side. 
 *****************************************************************************************************
 *****************************************************************************************************/

static SCM gpd_bind(SCM symname,SCM func){
  t_k_guile_workaround *x2;
  x2=(t_k_guile_workaround*)pd_new(k_guile_workaroundclass);
  x2->index=-1;
  x2->func=func;
  scm_protect_object(x2->func);
  pd_bind((t_pd *)x2, MAKE_SYM(symname));
  return MAKE_POINTER(x2);
}
static SCM gpd_unbind(SCM scm_x2,SCM symname){
  t_k_guile_workaround *x2=GET_POINTER(scm_x2);
  pd_unbind((t_pd *)x2,MAKE_SYM(symname));
  scm_unprotect_object(x2->func);
  pd_free((t_pd*)x2);
  RU_;
}






/*****************************************************************************************************
 *****************************************************************************************************
 *    Got data from the guile side. Distributing to outlets or receivers.
 *    The guile side is responsible for checking that the arguments are correct.
 *****************************************************************************************************
 *****************************************************************************************************/

#define GET_CLASS() (INTEGER_P(symbol)?(t_symbol*)GET_POINTER(symbol):MAKE_SYM(symbol))->s_thing
#define CLASS_INIT t_class **s=GET_CLASS();if(s==NULL) post("no receiver"); else 
#define GET_OUTLET() GET_X(instance)->outlets[GET_INTEGER(outlet)]


/* Number -> float */
static SCM gpd_outlet_number(SCM instance,SCM outlet,SCM val){
  outlet_float(GET_OUTLET(),scm_num2dbl(val,"gpd_outlet"));
  RU_;
}


static SCM gpd_send_number(SCM symbol,SCM val){
  CLASS_INIT
    pd_float(s,scm_num2dbl(val,"gpd_send_number"));
  RU_;
}


/* List -> list */
static t_atom *make_list(t_atom *atom,SCM val){
  int lokke;
  int length=GET_INTEGER(scm_length(val));

  for(lokke=0;lokke<length;lokke++){
    SCM el=scm_list_ref(val,MAKE_INTEGER(lokke));
    t_atom *to=&atom[lokke];
    if(SCM_INUMP(el)){
      SETFLOAT(to,(float)GET_INTEGER(el));
    }else{
      if(SCM_UNBNDP(el)){
	SETSYMBOL(to,gensym("undefined"));
      }else{
	if(SCM_STRINGP(el)){
	  SETSYMBOL(to,gensym(SCM_STRING_CHARS(el)));
	}else{
	  if(SCM_SYMBOLP(el)){
	    SETSYMBOL(to,MAKE_SYM(el));
	  }else{
	    if(scm_number_p(el)){
	      if(scm_real_p(el)){
		SETFLOAT(to,(float)scm_num2dbl(el,"gpd_outlet_or_send_list"));
	      }else{
		post("Illegal argument to gdp_outlet_or_send_list. Setting atom to 0.");
		SETFLOAT(to,0.0f);
	      }
	    }
	  }
	}
      }
    }
  }
  return atom;
}
static SCM gpd_outlet_list(SCM instance,SCM outlet,SCM val){
  int length=GET_INTEGER(scm_length(val));
  t_atom atom[length];
  outlet_list(GET_OUTLET(), &s_list,length,make_list(atom,val));
  RU_;
}
static SCM gpd_send_list(SCM symbol,SCM val){
  int length=GET_INTEGER(scm_length(val));
  t_atom atom[length];
  CLASS_INIT
    pd_list(s, &s_list,length,make_list(atom,val));
  RU_;
}

/* Symbol -> symbol */
static SCM gpd_outlet_symbol(SCM instance,SCM outlet,SCM val){
  outlet_symbol(GET_OUTLET(),MAKE_SYM(val));
  RU_;
}
static SCM gpd_send_symbol(SCM symbol,SCM val){
  CLASS_INIT
    pd_symbol(s,MAKE_SYM(val));
  RU_;
}

/* String -> symbol */
static SCM gpd_outlet_string(SCM instance,SCM outlet,SCM val){
  outlet_symbol(GET_OUTLET(),gensym(SCM_STRING_CHARS(val)));
  RU_;
}
static SCM gpd_send_string(SCM symbol,SCM val){
  CLASS_INIT
    pd_symbol(s,gensym(SCM_STRING_CHARS(val)));
  RU_;
}

/* Bang -> bang */
static SCM gpd_outlet_bang(SCM instance,SCM outlet){
  outlet_bang(GET_OUTLET());
  RU_;
}
static SCM gpd_send_bang(SCM symbol){
  CLASS_INIT
    pd_bang(s);
  RU_;
}

/* <- symbol */
static SCM gpd_get_symbol(SCM symname){
  return MAKE_POINTER(MAKE_SYM(symname));
}




/*****************************************************************************************************
 *****************************************************************************************************
 *    Setting up global guile functions.
 *****************************************************************************************************
 *****************************************************************************************************/

static void k_guile_init(void){
  char *command=
#include "global_scm.txt"
    ;

  scm_init_guile();
  scm_c_define_gsubr("pd-c-outlets",2,0,0,gpd_outlets);
  scm_c_define_gsubr("pd-c-inlets",2,0,0,gpd_inlets);
  scm_c_define_gsubr("pd-c-inited?",1,0,0,gpd_inited_p);
  scm_c_define_gsubr("pd-c-get-num-inlets",1,0,0,gpd_get_num_inlets);
  scm_c_define_gsubr("pd-c-get-num-outlets",1,0,0,gpd_get_num_outlets);
  scm_c_define_gsubr("pd-c-bind",2,0,0,gpd_bind);
  scm_c_define_gsubr("pd-c-unbind",2,0,0,gpd_unbind);
  scm_c_define_gsubr("pd-c-outlet-number",3,0,0,gpd_outlet_number);
  scm_c_define_gsubr("pd-c-outlet-list",3,0,0,gpd_outlet_list);
  scm_c_define_gsubr("pd-c-outlet-symbol",3,0,0,gpd_outlet_symbol);
  scm_c_define_gsubr("pd-c-outlet-string",3,0,0,gpd_outlet_string);
  scm_c_define_gsubr("pd-c-outlet-bang",2,0,0,gpd_outlet_bang);
  scm_c_define_gsubr("pd-c-send-number",2,0,0,gpd_send_number);
  scm_c_define_gsubr("pd-c-send-list",2,0,0,gpd_send_list);
  scm_c_define_gsubr("pd-c-send-symbol",2,0,0,gpd_send_symbol);
  scm_c_define_gsubr("pd-c-send-string",2,0,0,gpd_send_string);
  scm_c_define_gsubr("pd-c-send-bang",1,0,0,gpd_send_bang);
  scm_c_define_gsubr("pd-c-get-symbol",1,0,0,gpd_get_symbol);

  EVAL(command);

  pd_backtrace_run=EVAL("pd-backtrace-run");
  scm_permanent_object(pd_backtrace_run);

  pd_backtrace_runx=EVAL("pd-backtrace-runx");
  scm_permanent_object(pd_backtrace_runx);

  pd_backtrace_run1=EVAL("pd-backtrace-run1");
  scm_permanent_object(pd_backtrace_run1);

  pd_backtrace_run2=EVAL("pd-backtrace-run2");
  scm_permanent_object(pd_backtrace_run2);

  pd_backtrace_run3=EVAL("pd-backtrace-run3");
  scm_permanent_object(pd_backtrace_run3);

  pd_backtrace_run4=EVAL("pd-backtrace-run4");
  scm_permanent_object(pd_backtrace_run4);

  eval_string_func=EVAL("eval-string");
}





/*****************************************************************************************************
 *****************************************************************************************************
 *    Starting and stopping new guile script
 *****************************************************************************************************
 *****************************************************************************************************/

static bool k_guile_load(t_k_guile *x,char *filename){
  SCM evalret;
  bool ret=false;
  
  FILE *file=fopen(filename,"r");
  if(file==NULL){
    post("file \"%s\" not found.\n",filename);
    return false;
  }


  // Let the file live in its own name-space (or something like that).
  eval2("(define (pd-instance-func pd-instance)");
  eval2(
#include "local_scm.txt"
);
  eval_file(file);
  eval2("  (cons pd-inlet-func pd-cleanup-func))");
  eval2("1");

  if(1!=GET_INTEGER(eval_do())){
    post("Failed.");
    goto exit;
  }

  evalret=scm_call_2(pd_backtrace_run1,EVAL("pd-instance-func"),MAKE_POINTER(x));
  if(INTEGER_P(evalret)){
    post("Failed.");
    goto exit;
  }
  x->inlet_func=SCM_CAR(evalret);
  x->cleanup_func=SCM_CDR(evalret);
  scm_gc_protect_object(x->inlet_func);
  scm_gc_protect_object(x->cleanup_func);

  ret=true;

 exit:
  fclose(file);

  return ret;
}

static void *k_guile_new(t_symbol *s){
  int lokke;
  t_k_guile *x = (t_k_guile *)pd_new(k_guile_class);
  x->filename=s->s_name;
  x->isinited=false;

  if(k_guile_load(x,x->filename)==true){
    x->isinited=true;
    return x;
  }

  return NULL;
}

static void k_guile_free(t_k_guile *x){
  int lokke;
  scm_call_1(pd_backtrace_run,x->cleanup_func);
  for(lokke=1;lokke<x->num_ins;lokke++){
    inlet_free(x->inlets[lokke]->inlet);
    pd_free((t_pd*)x->inlets[lokke]);
  }
  for(lokke=0;lokke<x->num_outs;lokke++){
    outlet_free(x->outlets[lokke]);
  }
  scm_gc_unprotect_object(x->inlet_func);
  scm_gc_unprotect_object(x->cleanup_func);

  free(x->inlets);
  free(x->outlets);
}


static void k_guile_reload(t_k_guile *x){
  scm_call_1(pd_backtrace_run,x->cleanup_func);
  scm_gc_unprotect_object(x->inlet_func);
  scm_gc_unprotect_object(x->cleanup_func);
  k_guile_load(x,x->filename);
}

static void k_guile_eval(t_k_guile *x,t_symbol *s){
  scm_call_2(pd_backtrace_run1,eval_string_func,MAKE_STRING(s->s_name));
}

//static void k_guile_evalfile(t_k_guile *x,t_symbol *s){
//}



/*****************************************************************************************************
 *****************************************************************************************************
 *    Das setup
 *****************************************************************************************************
 *****************************************************************************************************/
void k_guile_setup(void){

  k_guile_init();

  k_guile_class = class_new(gensym("k_guile"), (t_newmethod)k_guile_new,
			   (t_method)k_guile_free, sizeof(t_k_guile), 0, A_DEFSYM, 0);

  class_addanything(k_guile_class, (t_method)k_guile_anything_first);
  class_addmethod(k_guile_class, (t_method)k_guile_reload, gensym("reload"), 0);
  class_addmethod(k_guile_class, (t_method)k_guile_eval, gensym("eval"), A_DEFSYM,0);
  //class_addmethod(k_guile_class, (t_method)k_guile_evalfile, gensym("evalfile"), A_DEFSYM,0);
  class_sethelpsymbol(k_guile_class, gensym("help-k_guile.pd"));


  /* This trick(?) is taken from the flext source. (I don't understand what happens...) */
  k_guile_workaroundclass=class_new(gensym("indexworkaround"),NULL,NULL,sizeof(t_k_guile_workaround),CLASS_PD|CLASS_NOINLET, A_NULL);
  class_addanything(k_guile_workaroundclass,k_guile_anything);

 
  post(version);
}

