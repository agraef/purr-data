/* op~ by Martin Peach 201003123, based on the zexy binop externals: */
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

/* ----------------------------- op_tilde ----------------------------- */
#include "m_pd.h"

static t_class  *op_tilde_class;

static char *operator_names[] = {"<=", ">=", "!=", "==", "<", ">", "le", "ge", "ne", "eq", "lt", "gt", "nop"};
static int n_opnames = 13;

typedef enum
{
    le = 0,
    ge,
    ne,
    eq,
    lt,
    gt,
    nop,
    none
} op_op;

typedef struct _op_tilde
{
    t_object    x_obj;
    t_float     x_f;
    t_float     x_g;    	    /* inlet value */
    op_op       x_operator;
} t_op_tilde;


static void *op_tilde_new(t_symbol *s, int argc, t_atom *argv);
static t_int *op_tilde_perform(t_int *w);
static void op_tilde_help(t_op_tilde *x);
static void op_tilde_op(t_op_tilde *x, t_symbol *op);
static void op_tilde_dsp(t_op_tilde *x, t_signal **sp);
void op_tilde_setup(void);

static void *op_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_symbol    *argsym;

    if (argc > 1) post("op~: extra arguments ignored");
    if (argc == 0) post("op~: need to specify operator");
    argsym = atom_getsymbolarg(0, argc, argv);
//    post ("op~: %s", argsym->s_name);

    t_op_tilde *x = (t_op_tilde *)pd_new(op_tilde_class);

    x->x_operator = none;

    switch (argsym->s_name[0])
    {
        case '<':
            if (argsym->s_name[1] == 0) x->x_operator = lt;
            else if (argsym->s_name[1] == '=') x->x_operator = le;
            break;
        case '>':
            if (argsym->s_name[1] == 0) x->x_operator = gt;
            else if (argsym->s_name[1] == '=') x->x_operator = ge;
            break;
        case '=':
            if (argsym->s_name[1] == '=') x->x_operator = eq;
            break;
        case '!':
            if (argsym->s_name[1] == '=') x->x_operator = ne;
            break;
        case 'n':
            if
            (
                (argsym->s_name[1] == 'o')
                && (argsym->s_name[2] == 'p')
                && (argsym->s_name[3] == '\0')
            )
                x->x_operator = nop;
            break;
    }
    if (x->x_operator == none)
    {
        post("op~: unknown operator");
        x->x_operator = nop;
    }
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    x->x_g = 0;  
    return (x);
}

static t_int *op_tilde_perform(t_int *w)
{
    t_op_tilde *x = (t_op_tilde *)(w[1]);
    t_sample *in1 = (t_sample *)(w[2]);
    t_sample *in2 = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    int n = (int)(w[5]);

    switch (x->x_operator)
    {
        case le:
            while (n--) *out++ = *in1++ <= *in2++; 
            break;
        case ge:
            while (n--) *out++ = *in1++ >= *in2++; 
            break;
        case ne:
            while (n--) *out++ = *in1++ != *in2++; 
            break;
        case eq:
            while (n--) *out++ = *in1++ == *in2++; 
            break;
        case lt:
            while (n--) *out++ = *in1++ < *in2++; 
            break;
        case gt:
            while (n--) *out++ = *in1++ > *in2++; 
            break;
        case nop:
        default:
            while (n--) *out++ = 0; 
            break;
    }
    return (w+6);
}

static void op_tilde_help(t_op_tilde *x)
{
    int i;

    post("op~: available operators:");
    for (i = 0; i < n_opnames; ++i)
    {
        post ("%s", operator_names[i]);    
    }
}

static void op_tilde_op(t_op_tilde *x, t_symbol *op)
{
    int i, j = 0;
    
    while ((op->s_name[j] != '\0') && (j < 3)) ++j;
    if ((j < 1) || (j > 3))
    {
        post("op~: %s is not a valid operator (length %d)", op->s_name, j);
        return;
    }
    for (i = 0; i < n_opnames; ++i)
    {
        if (operator_names[i][0] != op->s_name[0]) continue;
        if ((j == 2) && (operator_names[i][1] != op->s_name[1])) continue;
        if ((j == 3) && (operator_names[i][2] != op->s_name[2])) continue;
        if ((j == 1) && (operator_names[i][1] != '\0')) continue;
        break;
    } 
    if (i == n_opnames)   
    {
        post("op~: %s is not a valid operator (match)", op->s_name);
        return;
    }
switch(i)
    {
        /*"0",  "1",  "2",  "3",  "4", "5", "6",  "7",  "8",  "9",  "10", "11", "12"*/
        /*"<=", ">=", "!=", "==", "<", ">", "le", "ge", "ne", "eq", "lt", "gt", "nop"*/
        case 0:
        case 6:
            /* "<=" "le" */
            x->x_operator = 0;
            break;
        case 1:
        case 7:
            /* ">=" "ge" */
            x->x_operator = 1;
            break;
        case 2:
        case 8:
            /* "!=" "ne" */
            x->x_operator = 2;
            break;
        case 3:
        case 9:
            /* "==" "eq" */
            x->x_operator = 3;
            break;
        case 4:
        case 10:
            /* "<" "lt" */
            x->x_operator = 4;
            break;
        case 5:
        case 11:
            /* ">" "gt" */
            x->x_operator = 5;
            break;
        default:
            /* "nop" */
            x->x_operator = 6;
            break;
    }
//    post("op~: operator %s", operator_names[i]);
}

static void op_tilde_dsp(t_op_tilde *x, t_signal **sp)
{
    t_sample    *in1 = sp[0]->s_vec;
    t_sample    *in2 = sp[1]->s_vec;
    t_sample    *out = sp[2]->s_vec;
    int         n = sp[0]->s_n;

    dsp_add(op_tilde_perform, 5, x, in1, in2, out, n);
}

void op_tilde_setup(void)
{
    op_tilde_class = class_new(gensym("op~"), (t_newmethod)op_tilde_new, 0,
        sizeof(t_op_tilde), 0, A_GIMME, 0);
    class_addmethod(op_tilde_class, (t_method)op_tilde_dsp, gensym("dsp"), 0);
    class_addmethod(op_tilde_class, (t_method)op_tilde_help, gensym("help"), 0);
    class_addmethod(op_tilde_class, (t_method)op_tilde_op, gensym("op"), A_DEFSYMBOL, 0);
    CLASS_MAINSIGNALIN(op_tilde_class, t_op_tilde, x_f);
}

/* fin op~.c */

