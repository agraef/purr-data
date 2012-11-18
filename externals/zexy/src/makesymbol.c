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


/* 
(l) 1210:forum::für::umläute:1999

"makesymbol" is something between "symbol" and "makefilename", 
   thus storing and creating (formatted) symbols...


   TODO:
    the use of only "%s" is really not very satisfying   
   LATER: 
   split the entire format string into subformats with only 
   one format-placeholder ("%[^%]*[diufFgGxXoscpn]") .
   split the entire format string into subformats with only one format-placeholder,
   store the types
   when a list comes in, evaluate each subformat (with prior type-conversion)

*/

#include "zexy.h"

#include <string.h>
#include <stdio.h>

#define MAXSTRINGARGS 10
#define MAXSTRINGLENG MAXPDSTRING

/* ----------------------- makesymbol --------------------- */

static t_class *makesymbol_class;

typedef struct _makesymbol
{
	t_object x_obj;
	t_symbol *x_sym;

	char* mask;
	char* buf;
} t_makesymbol;

static void reset_mask(t_makesymbol *x, t_symbol *s)
{
	if (*s->s_name) {
		x->mask = s->s_name;
		x->x_sym = s;
	} else {
		x->mask = "%s%s%s%s%s%s%s%s%s%s";
		x->x_sym = gensym("");
	}
}

typedef char cstring[MAXSTRINGLENG];

static t_symbol* list2symbol(char *masque, int argc, t_atom *argv)
{
  cstring*buf=(cstring*)getbytes(MAXSTRINGARGS*sizeof(cstring));
	cstring buffer;
	int i;

	for(i=0; i<MAXSTRINGARGS; i++)*buf[i]=0;

	for (i=0; i<argc; i++) {
		atom_string(argv+i, buf[i], MAXSTRINGLENG);
	}

	sprintf(buffer,
		masque,
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);

        freebytes(buf, MAXSTRINGARGS*sizeof(cstring));
	return (gensym(buffer));
}

static void makesymbol_list(t_makesymbol *x, t_symbol *s, int argc, t_atom *argv)
{
        ZEXY_USEVAR(s);
	x->x_sym = list2symbol(x->mask, argc, argv);
	outlet_symbol(x->x_obj.ob_outlet, x->x_sym);
}

static void makesymbol_bang(t_makesymbol *x)
{
	outlet_symbol(x->x_obj.ob_outlet, x->x_sym);
}


static void *makesymbol_new(t_symbol *s, int argc, t_atom *argv)
{
	t_makesymbol *x = (t_makesymbol *)pd_new(makesymbol_class);
        ZEXY_USEVAR(s);

	x->buf = (char *)getbytes(MAXSTRINGLENG * sizeof(char));

	x->mask  = x->buf;

	if (argc) {
		atom_string(argv, x->buf, MAXSTRINGLENG);
		x->x_sym = gensym(x->buf);
	} else {
		x->mask = "%s%s%s%s%s%s%s%s%s%s";
		x->x_sym = gensym("");
	}

	outlet_new(&x->x_obj, &s_symbol);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("symbol"), gensym("sym1"));

	return (x);
}

static void makesymbol_free(t_makesymbol *x)
{
	freebytes(x->buf, MAXSTRINGLENG*sizeof(char));
}


static void makesymbol_helper(t_makesymbol *x)
{
  ZEXY_USEVAR(x);
	post("\n%c makesymbol :: create a formatted symbol", HEARTSYMBOL);
	post("<list of anything>\t: glue up to 10 list-elements to 1 formatted symbol\n"
		"'bang'\t\t\t: re-output\n"
		"'help'\t\t\t: view this"
		"\ninlet2 : <format-string>: new format-string (symbol !)"
		"\noutlet : <symbol>\t: formatted concatenation");
	post("\ncreation:\"makesymbol [<format-string>]\": C-style format-string (%s only)", "%s");
}

void makesymbol_setup(void)
{
	makesymbol_class = class_new(gensym("makesymbol"),
	(t_newmethod)makesymbol_new, (t_method)makesymbol_free,
		sizeof(t_makesymbol), 0, A_GIMME, 0);

	class_addlist(makesymbol_class, makesymbol_list);
	class_addbang(makesymbol_class, makesymbol_bang);

	class_addmethod(makesymbol_class, (t_method)reset_mask, gensym("sym1"), A_SYMBOL, 0);

	class_addmethod(makesymbol_class, (t_method)makesymbol_helper, gensym("help"), 0);
  zexy_register("makesymbol");
}
