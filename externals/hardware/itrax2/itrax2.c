/* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.

itrax2 written by Thomas Musil (c) IEM KUG Graz Austria 2000 - 2004 */

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "iemlib.h"
#include "isense.h"

typedef struct _itrax2
{
	t_object						x_obj;
	float								x_poll;
	ISD_TRACKER_HANDLE	x_handle;
	void								*x_out2;
	void								*x_out3;
	void								*x_clock;
} t_itrax2;

static t_class *itrax2_class;

static void itrax2_tick(t_itrax2 *x)
{
	if(x->x_handle > 0)
	{
		ISD_TRACKER_DATA_TYPE data;

		clock_delay(x->x_clock, x->x_poll);
		ISD_GetData(x->x_handle, &data);
		outlet_float(x->x_out3, (float)(data.Station[0].Orientation[2]));
		outlet_float(x->x_out2, (float)(data.Station[0].Orientation[1]));
		outlet_float(x->x_obj.ob_outlet, (float)(data.Station[0].Orientation[0]));
	}
	else
		clock_unset(x->x_clock);
}

static void itrax2_init(t_itrax2 *x)
{
	x->x_handle = ISD_OpenTracker((Hwnd)NULL, 0, FALSE, FALSE);
	if(x->x_handle <= 0)
		post("Tracker not found");
	else
		post("Intertrax2 dedected, OK");
}

static void itrax2_reset(t_itrax2 *x)
{
	ISD_ResetHeading(x->x_handle, 1);
}

static void itrax2_bang(t_itrax2 *x)
{
	clock_delay(x->x_clock, x->x_poll);
}

static void itrax2_start(t_itrax2 *x)
{
	itrax2_bang(x);
}

static void itrax2_stop(t_itrax2 *x)
{
	clock_unset(x->x_clock);
}

static void itrax2_float(t_itrax2 *x, t_float cmd)
{
	if(cmd == 0.0)
		itrax2_stop(x);
	else
		itrax2_bang(x);
}

static void itrax2_ft1(t_itrax2 *x, t_float polltime_ms)
{
	if(polltime_ms < 8.0)
	{
		polltime_ms = 8.0;
		post("serial polling-time clipped to 8 ms");
	}
	x->x_poll = polltime_ms;
}

static void *itrax2_new(t_floatarg polltime_ms)
{
	t_itrax2 *x = (t_itrax2 *)pd_new(itrax2_class);

	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
	outlet_new(&x->x_obj, &s_float);
	x->x_out2 = outlet_new(&x->x_obj, &s_float);
	x->x_out3 = outlet_new(&x->x_obj, &s_float);
	x->x_clock = clock_new(x, (t_method)itrax2_tick);
	itrax2_ft1(x, polltime_ms);
	x->x_handle = 0;
	return(x);
}

static void itrax2_ff(t_itrax2 *x)
{
	clock_free(x->x_clock);
	if(x->x_handle > 0)
		ISD_CloseTracker(x->x_handle);
}

void itrax2_setup(void)
{
	itrax2_class = class_new(gensym("itrax2"), (t_newmethod)itrax2_new,
		(t_method)itrax2_ff, sizeof(t_itrax2), 0, A_DEFFLOAT, 0);
	class_addbang(itrax2_class, itrax2_bang);
	class_addfloat(itrax2_class, itrax2_float);
	class_addmethod(itrax2_class, (t_method)itrax2_start, gensym("start"), 0);
	class_addmethod(itrax2_class, (t_method)itrax2_stop, gensym("stop"), 0);
	class_addmethod(itrax2_class, (t_method)itrax2_init, gensym("init"), 0);
	class_addmethod(itrax2_class, (t_method)itrax2_reset, gensym("reset"), 0);
	class_addmethod(itrax2_class, (t_method)itrax2_ft1, gensym("ft1"), A_FLOAT, 0);
	class_sethelpsymbol(itrax2_class, gensym("iemhelp/help-itrax2"));

	post("itrax2 (R-1.15) library loaded!");
}
