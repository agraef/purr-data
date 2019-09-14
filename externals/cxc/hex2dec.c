#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "m_pd.h"

static t_class *hex2dec_class;

typedef struct _hex2dec {
    t_object  x_obj;
} t_hex2dec;

static int hex2dec_strtoll(t_hex2dec *x, char *buf, long long int *val)
{
    char *endptr;
    errno = 0;

    /* No empty strings, please. This guards against things like
       [symbol 42( silently failing */
    if (!buf || *buf == '\0')
    {
        pd_error(x, "hex2dec: empty hex string detected");
        return 0;
    }

    *val = strtoll(buf, &endptr, 16);
    if (errno == ERANGE) {
        if (*val == LLONG_MIN)
            pd_error(x, "hex2dec: underflow detected");
        else if (*val == LLONG_MAX)
            pd_error(x, "hex2dec: overflow detected");
        else
            pd_error(x, "hex2dec: unknown range error");
        return 0;
    }
    else if (errno != 0)
    {
        pd_error(x, "hex2dec: unknown error");
        return 0;
    }
    else if (*endptr != '\0')
    {
        pd_error(x, "hex2dec: invalid input '%s'", buf);
        return 0;
    }
    return 1;
}

    /* Let's see how we do with static allocation for size 10 lists. This
       can be expanded later if people want to avoid heap allocations for
       larger messages. */
#define STATIC_SIZE 10
int warned_about_allocation;

static void hex2dec_list(t_hex2dec *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[STATIC_SIZE], *outvec;
    char buf[MAXPDSTRING];
    if (argc > STATIC_SIZE)
    {
        if (!warned_about_allocation)
        {
            post("warning: hex2dec: realtime unsafe memory allocation for "
                 "lists with more than %d elements", STATIC_SIZE);
            warned_about_allocation++;
        }
        outvec = (t_atom *)t_getbytes(argc * sizeof(t_atom));
    }
    else
        outvec = at;

    int i;
    for (i = 0; i < argc; i++)
    {
        if (argv[i].a_type == A_SYMBOL)
        {
            long long int val;
            if (hex2dec_strtoll(x, argv[i].a_w.w_symbol->s_name, &val))
                SETFLOAT(outvec + i, (t_float)val);
            else
                return; /* no output if we hit any errors */
        }
        else if (argv[i].a_type == A_FLOAT)
        {
            long long int val;
            sprintf(buf, "%lld", (long long int)argv[i].a_w.w_float);
            if (hex2dec_strtoll(x, buf, &val))
                SETFLOAT(outvec + i, (t_float)val);
            else
                return;
        }
        else
        {
            pd_error(x, "hex2dec: only symbol and float accepted");
            /* cleanup for large lists */
            if (argc > STATIC_SIZE)
                t_freebytes(outvec, argc * sizeof(t_atom));
            return;
        }
    }
    outlet_list(x->x_obj.ob_outlet, &s_list, argc, outvec);

    /* cleanup for large lists */
    if (argc > STATIC_SIZE)
        t_freebytes(outvec, argc * sizeof(t_atom));
}

/* We could accept anything. But then we'd have to allocate for larger
   messages, copy the selector to the first slot, then copy argv in the
   remaining slots. There are some macros to achieve this in x_list.c
   but they're specific to the list classes and therefore not public. */
static void hex2dec_anything(t_hex2dec *x, t_symbol *s, int argc, t_atom *argv)
{
    if (s && s != &s_)
        pd_error(x, "hex2dec: no method for '%s' (did you mean "
                    "'%s %s%s'?)",
        s->s_name, argc ? "list" : "symbol", s->s_name, argc ? " ..." : "");
    else if (s && s == &s_)
        pd_error(x, "hex2dec: no method for empty symbol");
    else
        pd_error(x, "hex2dec: only float, symbol, or list accepted");
}

static void *hex2dec_new(void) {
    t_hex2dec *x = (t_hex2dec *)pd_new(hex2dec_class);
    outlet_new(&x->x_obj, &s_float);
    return (void *)x;
}

void hex2dec_setup(void) {
    hex2dec_class = class_new(gensym("hex2dec"), (t_newmethod)hex2dec_new,
        0, sizeof(t_hex2dec), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addlist(hex2dec_class, hex2dec_list);
    class_addanything(hex2dec_class, hex2dec_anything);
}
