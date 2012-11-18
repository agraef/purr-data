/* stkdrone~ -- STK-based drone synthesis 
 * requires STK library
 * Copyleft 2001 Yves Degoyon.
 * Permission is granted to use this software for any purpose provided you
 * keep this copyright notice intact.
 *
 * THE AUTHOR AND HIS EXPLOITERS MAKE NO WARRANTY, EXPRESS OR IMPLIED,
 * IN CONNECTION WITH THIS SOFTWARE.
 *
*/

#include "m_pd.h"
#include "drone.h"

#define DEFAULT_FREQ 250.0
#define DEFAULT_PLUCK 0.25

typedef struct _stkdrone
{
    t_object x_obj;
    drone *x_stkdrone;
    t_int x_on;
    t_float x_freq;
    t_float x_pluck;
} t_stkdrone;

static t_class *stkdrone_class;

static void *stkdrone_new(void)
{
    t_stkdrone *x = (t_stkdrone *)pd_new(stkdrone_class);
    x->x_freq = DEFAULT_FREQ;
    x->x_pluck = DEFAULT_PLUCK;
    outlet_new(&x->x_obj, &s_signal);
    inlet_new( &x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("freq") );
    inlet_new( &x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("pluck") );
    if( (x->x_stkdrone = new drone( 50.0 )) == NULL ) {
       post( "stkdrone~: cannot build drone instrument from STK" );
       return NULL;
    } else {
       return (x);
    }
}

static void stkdrone_freq(t_stkdrone* x, t_float ffreq)
{
    if ( ffreq < 50.0 )
    {
       post("stkdrone~ : wrong frequency" );
       return;
    }
    x->x_stkdrone->setFreq( (MY_FLOAT) ffreq );
    x->x_freq = ffreq;
}

static void stkdrone_pluck(t_stkdrone* x, t_float fpluck)
{
    if ( fpluck < 0.05 || fpluck > 0.35 )
    {
       post("stkdrone~ : wrong pluck argument" );
       return;
    }
    x->x_stkdrone->pluck( (MY_FLOAT) fpluck );
    x->x_pluck = fpluck;
}

static void stkdrone_play(t_stkdrone* x)
{
    x->x_on = 1; // activate sound
    x->x_stkdrone->noteOn( x->x_freq, x->x_pluck ); // start sound
}

static void stkdrone_stop(t_stkdrone* x)
{
    x->x_on = 0; // deactivate sound
}

static t_int *stkdrone_perform(t_int *w)
{
    t_float *out = (t_float *)(w[1]);
    int n = (int)(w[2]);
    t_stkdrone* x = (t_stkdrone*)(w[3]);

    while ( n-- ) 
    {
       if ( x->x_on ) 
       {
         double dare;

         dare = (float)x->x_stkdrone->tick();
         // post( "synthesis : %f", dare );
         *out=dare;
       }
       else
       {
         *(out) = 0.0;
       }
       out++;
    }

    return (w+4);
}

static void stkdrone_dsp(t_stkdrone *x, t_signal **sp)
{
    dsp_add(stkdrone_perform, 3, sp[0]->s_vec, sp[0]->s_n, x);
}

extern "C" void stkdrone_tilde_setup(void)
{
    stkdrone_class = class_new(gensym("stkdrone~"), (t_newmethod)stkdrone_new, 0,
    	sizeof(t_stkdrone), 0, A_NULL);
    class_sethelpsymbol(stkdrone_class, gensym("help-stkdrone~.pd") );
    class_addmethod(stkdrone_class, (t_method)stkdrone_dsp, gensym("dsp"), A_NULL);
    class_addmethod(stkdrone_class, (t_method)stkdrone_play, gensym("play") , A_NULL);
    class_addmethod(stkdrone_class, (t_method)stkdrone_stop, gensym("stop") , A_NULL);
    class_addmethod(stkdrone_class, (t_method)stkdrone_freq, gensym("freq") , A_DEFFLOAT, A_NULL);
    class_addmethod(stkdrone_class, (t_method)stkdrone_pluck, gensym("pluck") , A_DEFFLOAT, A_NULL);
}
