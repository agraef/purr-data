/* this external is based on nop~ which was part of zexy-1.x, see the copyright notice below: 

 * ZEXY is published under the GNU GeneralPublicLicense, that must be shipped with ZEXY.
 * if you are using Debian GNU/linux, the GNU-GPL can be found under /usr/share/common-licenses/GPL
 * if you still haven't found a copy of the GNU-GPL, have a look at http://www.gnu.org
 *
 * "pure data" has it's own license, that comes shipped with "pure data".
 *
 * there are ABSOLUTELY NO WARRANTIES for anything

 it does absolutely the same as z~ 64, but since it uses simd instructions, it's faster.
*/



/* ------------------------ block_delay~ ----------------------------- */
/* this will pass trough the signal unchanged except for a delay of 1 block */

#include "m_pd.h"

static t_class *block_delay_tilde_class;

typedef struct _block_delay
{
	t_object x_obj;
	t_float *buf;
	int n;
	int toggle;
} t_block_delay;


static t_int *block_delay_tilde_perfsimd(t_int *w)
{
	t_float *in  = (t_float *)w[1];
	t_float *out = (t_float *)w[2];
	t_block_delay *x = (t_block_delay *)w[3];
	int n = x->n;
	t_float *rp = x->buf + n * x->toggle, *wp = x->buf + n * (x->toggle ^= 1);

	copyvec_simd(wp, in, n);
	copyvec_simd(out, rp, n);

	return (w+4);
}

static t_int *block_delay_tilde_perform(t_int *w)
{
	t_float *in  = (t_float *)w[1];
	t_float *out = (t_float *)w[2];
	t_block_delay *x = (t_block_delay *)w[3];
	int n = x->n;
	t_float *rp = x->buf + n * x->toggle, *wp = x->buf + n * (x->toggle ^= 1);

	while (n--)	{
		*wp++ = *in++;
		*out++ = *rp++;
	}

	return (w+4);
}

static void block_delay_tilde_dsp(t_block_delay *x, t_signal **sp)
{
	if (x->n != sp[0]->s_n)
	{
		if (x->n)
			freealignedbytes(x->buf, x->n * 2 * sizeof(t_float));
		x->buf = (t_float *)getalignedbytes(sizeof(t_float) * 2 * (x->n = sp[0]->s_n));
	}
	if (simd_check2(sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec))
		dsp_add(block_delay_tilde_perfsimd, 3, sp[0]->s_vec, sp[1]->s_vec, x);
	else
		dsp_add(block_delay_tilde_perform, 3, sp[0]->s_vec, sp[1]->s_vec, x);
}

static void block_delay_tilde_free(t_block_delay *x)
{
	if (x->buf)
		freealignedbytes(x->buf, x->n * sizeof(t_float));
}


static void *block_delay_tilde_new(void)
{
	t_block_delay *x = (t_block_delay *)pd_new(block_delay_tilde_class);
	outlet_new(&x->x_obj, gensym("signal"));
	x->toggle = 0;
	x->n = 0;

	return (x);
}

void block_delay_tilde_setup(void)
{
	block_delay_tilde_class = class_new(gensym("block_delay~"), (t_newmethod)block_delay_tilde_new, (t_method)block_delay_tilde_free,
		sizeof(t_block_delay), 0, A_DEFFLOAT, 0);
	class_addmethod(block_delay_tilde_class, nullfn, gensym("signal"), 0);
	class_addmethod(block_delay_tilde_class, (t_method)block_delay_tilde_dsp, gensym("dsp"), 0);
}
