/* stksitar~ -- STK-based sitar synthesis
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
#include "sitar.h"

#define DEFAULT_FREQ 250.0
#define DEFAULT_PLUCK 0.25

typedef struct _stksitar
{
    t_object x_obj;
    sitar *x_stksitar;
    t_int x_on;
    t_float x_freq;
    t_float x_pluck;
} t_stksitar;

static t_class *stksitar_class;

static void *stksitar_new(void)
{
    t_stksitar *x = (t_stksitar *)pd_new(stksitar_class);
    x->x_freq = DEFAULT_FREQ;
    x->x_pluck = DEFAULT_PLUCK;
    outlet_new(&x->x_obj, &s_signal);
    inlet_new( &x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("freq") );
    inlet_new( &x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("pluck") );
    if( (x->x_stksitar = new sitar( 50.0 )) == NULL ) {
       post( "stksitar~: cannot build sitar instrument from STK" );
       return NULL;
    } else {
       return (x);
    }
}

static void stksitar_freq(t_stksitar* x, t_float ffreq)
{
    if ( ffreq < 50.0 )
    {
       post("stksitar~ : wrong frequency" );
       return;
    }
    x->x_stksitar->setFreq( (StkFloat) ffreq );
    x->x_freq = ffreq;
}

static void stksitar_pluck(t_stksitar* x, t_float fpluck)
{
    if ( fpluck < 0.05 || fpluck > 0.35 )
    {
       post("stksitar~ : wrong pluck argument" );
       return;
    }
    x->x_stksitar->pluck( (StkFloat) fpluck );
    x->x_pluck = fpluck;
}

static void stksitar_play(t_stksitar* x)
{
    x->x_on = 1; // activate sound
    x->x_stksitar->noteOn( x->x_freq, x->x_pluck ); // start sound
}

static void stksitar_stop(t_stksitar* x)
{
    x->x_on = 0; // deactivate sound
}

static t_int *stksitar_perform(t_int *w)
{
    t_float *out = (t_float *)(w[1]);
    int n = (int)(w[2]);
    t_stksitar* x = (t_stksitar*)(w[3]);

    while ( n-- ) 
    {
       if ( x->x_on ) 
       {
         double dare;

         dare = (float)x->x_stksitar->tick();
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

static void stksitar_dsp(t_stksitar *x, t_signal **sp)
{
    dsp_add(stksitar_perform, 3, sp[0]->s_vec, sp[0]->s_n, x);
}

extern "C" void stksitar_tilde_setup(void)
{
    stksitar_class = class_new(gensym("stksitar~"), (t_newmethod)stksitar_new, 0,
    	sizeof(t_stksitar), 0, A_NULL);
    class_sethelpsymbol(stksitar_class, gensym("help-stksitar~.pd"));
    class_addmethod(stksitar_class, (t_method)stksitar_dsp, gensym("dsp"), A_NULL);
    class_addmethod(stksitar_class, (t_method)stksitar_play, gensym("play") , A_NULL);
    class_addmethod(stksitar_class, (t_method)stksitar_stop, gensym("stop") , A_NULL);
    class_addmethod(stksitar_class, (t_method)stksitar_freq, gensym("freq") , A_DEFFLOAT, A_NULL);
    class_addmethod(stksitar_class, (t_method)stksitar_pluck, gensym("pluck") , A_DEFFLOAT, A_NULL);
}
