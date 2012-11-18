// line3
// based on miller puckette line object (so licence / copyright comes from pure data)
// compatible with line, but with a 3d order polynome.
// there is continuity of the variation speed

/* 
This software is copyrighted by Miller Puckette and others.  The following
terms (the "Standard Improved BSD License") apply to all files associated with
the software unless explicitly disclaimed in individual files:

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above  
   copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided
   with the distribution.
3. The name of the author may not be used to endorse or promote
   products derived from this software without specific prior 
   written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

// Cyrille Henry 01 2005


#include "m_pd.h"

static t_class *line3_class;

typedef struct _line3
{
    t_object x_obj;
    t_clock *x_clock;
    double x_targettime;
    t_float x_targetval,setderiv, a, b;
    double x_prevtime;
    t_float x_setval;
    int x_gotinlet;
    t_float x_grain;
    double x_1overtimediff;
    double x_in1val;
} t_line3;

void line3_tick(t_line3 *x)
{
    double tmp, t;
    double timenow = clock_getsystime();
    double msectogo = - clock_gettimesince(x->x_targettime);
    if (msectogo < 1E-9)
    {
        outlet_float(x->x_obj.ob_outlet, x->x_targetval);
    }
    else
    {
	   t = (timenow - x->x_prevtime);

	   tmp = x->a * t * t * t + x->b * t * t + x->setderiv * t + x->x_setval;

       outlet_float(x->x_obj.ob_outlet, tmp);
       clock_delay(x->x_clock, (x->x_grain > msectogo ? msectogo : x->x_grain));
    }
}

void line3_float(t_line3 *x, t_float f)
{
    double timenow = clock_getsystime();
    if (x->x_gotinlet && x->x_in1val > 0)
    {
        if (timenow >= x->x_targettime) 
	{
		x->x_setval = x->x_targetval;
		x->setderiv = 0;	
	}
        else 
	{
		x->x_setval = x->a * (timenow - x->x_prevtime) * (timenow - x->x_prevtime) * (timenow - x->x_prevtime) + x->b * (timenow - x->x_prevtime) * (timenow - x->x_prevtime) + x->setderiv * (timenow - x->x_prevtime) + x->x_setval;

		x->setderiv = 3 * x->a * (timenow - x->x_prevtime) * (timenow - x->x_prevtime) + 2 * x->b * (timenow - x->x_prevtime) + x->setderiv;

	}	
        x->x_prevtime = timenow;
        x->x_targettime = clock_getsystimeafter(x->x_in1val);
        x->x_targetval = f;
        x->x_1overtimediff = 1./ (x->x_targettime - timenow);

	x->a = -2 * (x->x_targetval - x->x_setval) * x->x_1overtimediff;
	x->a += x->setderiv;
	x->a *= x->x_1overtimediff;
	x->a *= x->x_1overtimediff;

	x->b = 3 * (x->x_targetval - x->x_setval) * x->x_1overtimediff;
	x->b -= 2 * x->setderiv;
	x->b *= x->x_1overtimediff;

        line3_tick(x);
        x->x_gotinlet = 0;

        clock_delay(x->x_clock, (x->x_grain > x->x_in1val ? x->x_in1val : x->x_grain));
    }
    else
    {
        clock_unset(x->x_clock);
        x->x_targetval = x->x_setval = f;
        x->x_targettime = timenow;
        outlet_float(x->x_obj.ob_outlet, f);
    }
    x->x_gotinlet = 0;
}

void line3_ft1(t_line3 *x, t_floatarg g)
{
    x->x_in1val = g;
    x->x_gotinlet = 1;
}

void line3_stop(t_line3 *x)
{
    x->x_targetval = x->x_setval;
    clock_unset(x->x_clock);
}

void line3_set(t_line3 *x, t_floatarg f)
{
    clock_unset(x->x_clock);
    x->x_targetval = x->x_setval = f;
    x->setderiv = 0;
}

void line3_free(t_line3 *x)
{
    clock_free(x->x_clock);
}

void *line3_new(t_floatarg f, t_floatarg grain)
{
    t_line3 *x = (t_line3 *)pd_new(line3_class);
    x->x_targetval = x->x_setval = f;
    x->x_gotinlet = 0;
    x->setderiv = 0;
    x->x_1overtimediff = 1;
    x->x_clock = clock_new(x, (t_method)line3_tick);
    x->x_targettime = x->x_prevtime = clock_getsystime();
    if (grain <= 0) grain = 20;
    x->x_grain = grain;
    outlet_new(&x->x_obj, gensym("float"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    return (x);
}

void line3_setup(void)
{
    line3_class = class_new(gensym("line3"), (t_newmethod)line3_new,
        (t_method)line3_free, sizeof(t_line3), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(line3_class, (t_method)line3_ft1,
        gensym("ft1"), A_FLOAT, 0);
    class_addmethod(line3_class, (t_method)line3_stop,
        gensym("stop"), 0);
    class_addmethod(line3_class, (t_method)line3_set,
        gensym("set"), A_FLOAT, 0);
    class_addfloat(line3_class, (t_method)line3_float);
}
