#include "MSPd.h"

/* pointless comment */

#define F_LEN 65536


static t_class *flanjah_class;


#define OBJECT_NAME "flanjah~"

typedef struct _flanjah
{
    
    t_object x_obj;
    float x_f;
    //
    float *sinetab;
    float si_factor;
    float osc1_phs;
    float osc1_si;
    float si1;
    float osc2_phs;
    float osc2_si;
    float si2;
    //
    float speed1;
    float speed2;
    float feedback;
    float maxdel;
    float depth;
    //
    float *ddl1 ;
    int ddl1_len;
    int ddl1_phs;
    float *ddl2 ;
    int ddl2_len;
    int ddl2_phs;
    //
    float tap1;
    float tap2;
    //
    int feedback_connected;
    int speed1_connected;
    int speed2_connected;
    int depth_connected;
    short connected[8];
    int feedback_protect;
    short mute;
    float sr;
} t_flanjah;

t_int *flanjah_perform(t_int *w);

void flanjah_protect(t_flanjah *x, t_floatarg state);
void flanjah_mute(t_flanjah *x, t_floatarg state);
void flanjah_dsp(t_flanjah *x, t_signal **sp);
void *flanjah_new(t_symbol *s, int argc, t_atom *argv);
void flanjah_dsp_free( t_flanjah *x );
void flanjah_report( t_flanjah *x );
void flanjah_float(t_flanjah *x, double f);
void flanjah_init(t_flanjah *x,short initialized);

void flanjah_tilde_setup(void){
    flanjah_class = class_new(gensym("flanjah~"), (t_newmethod)flanjah_new,
                              (t_method)flanjah_dsp_free,sizeof(t_flanjah), 0,A_GIMME,0);
    CLASS_MAINSIGNALIN(flanjah_class, t_flanjah, x_f);
    class_addmethod(flanjah_class,(t_method)flanjah_dsp,gensym("dsp"),0);
    class_addmethod(flanjah_class,(t_method)flanjah_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(flanjah_class,(t_method)flanjah_protect,gensym("protect"),A_FLOAT,0);
    class_addmethod(flanjah_class,(t_method)flanjah_report,gensym("report"),0);
    potpourri_announce(OBJECT_NAME);
}

void flanjah_report( t_flanjah *x ){
    post("feedback: %f", x->feedback);
    post("depth: %f", x->depth);
    post("si1: %f", x->osc1_si);
    post("si2: %f", x->osc2_si);
    post("speed1: %f", x->speed1);
    post("speed2: %f", x->speed2);
    post("phase1: %f", x->osc1_phs);
    post("phase2: %f", x->osc2_phs);
}

void flanjah_dsp_free( t_flanjah *x ){
    free(x->sinetab);
    free(x->ddl1);
    free(x->ddl2);
}


t_int *flanjah_perform(t_int *w)
{
    t_flanjah *x = (t_flanjah *)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *feedback_vec = (t_float *)(w[3]);
    t_float *speed1_vec = (t_float *)(w[4]);
    t_float *speed2_vec = (t_float *)(w[5]);
    t_float *depth_vec = (t_float *)(w[6]);
    t_float *out1 = (t_float *)(w[7]);
    int n = (int) w[8];
    float fdelay1, fdelay2;
    int idelay1, idelay2;
    float insamp1;//, insamp2;
    float frac;
    int index1, index2;
    float m1, m2;
    //
    float osc2_phs = x->osc2_phs;
    int ddl2_len = x->ddl2_len;
    float osc2_si = x->osc2_si;
    float *ddl2 = x->ddl2;
    int ddl2_phs = x->ddl2_phs;
    float *ddl1 = x->ddl1;
    float *sinetab = x->sinetab;
    int ddl1_phs = x->ddl1_phs;
    int ddl1_len = x->ddl1_len;
    float osc1_phs = x->osc1_phs;
    float osc1_si = x->osc1_si;
    float tap1 = x->tap1;
    float tap2 = x->tap2;
    float feedback = x->feedback;
    int feedback_connected = x->feedback_connected;
    int speed1_connected = x->speed1_connected;
    int speed2_connected = x->speed2_connected;
    int depth_connected = x->depth_connected;
    float si_factor = x->si_factor;
    int feedback_protect = x->feedback_protect;
    float depth_factor = x->depth;
    /**********************/
    
    if( x->mute ){
        while( n-- ){
            *out1++ = 0.0;
        }
        return (w+9);
    }
    while( n-- ){
        // Pull Data off Signal buffers
        insamp1 = *in1++;
        if( feedback_connected ){
            feedback = *feedback_vec++;
            
        }
        if( feedback_protect ) {
            if( feedback > 0.425){
                feedback = 0.425;
            }
            if( feedback < -0.425 )
                feedback = -0.425;
        }
        
        if( speed1_connected ){
            osc1_si = *speed1_vec++ * si_factor;
        }
        if( speed2_connected ){
            osc2_si = *speed2_vec++ * si_factor;
        }
        if( depth_connected ){
            depth_factor = *depth_vec++;
        }
        
        if( depth_factor < .0001 ){
            depth_factor = .0001;
        }
        if( depth_factor > 1. ){
            depth_factor = 1.;
        }
        
        fdelay1 = sinetab[ (int) osc1_phs ] * (float) ddl1_len * depth_factor;
        fdelay2 = sinetab[ (int) osc2_phs ] * (float) ddl2_len * depth_factor;
        
        // DSP Proper
		
        idelay1 = fdelay1;
        osc1_phs += osc1_si;
        while( osc1_phs >= F_LEN )
            osc1_phs -= F_LEN;
        while( osc1_phs < 0 )
            osc1_phs += F_LEN;
        
        
        
        
        idelay2 = fdelay2;
        osc2_phs += osc2_si;
        while( osc2_phs >= F_LEN )
            osc2_phs -= F_LEN;
        while( osc1_phs < 0 )
            osc2_phs += F_LEN;
		
        ddl1[ ddl1_phs++ ] = insamp1 + feedback * (tap1+tap2);
        ddl1_phs = ddl1_phs % ddl1_len;
        // linear interpolated lookup
        index1 = (ddl1_phs + idelay1) % ddl1_len;
        index2 = (index1 + 1) % ddl1_len ;
        frac = fdelay1 - idelay1 ;
        m1 = 1. - frac;
        m2 = frac;
        tap1 = m1 * ddl1[ index1 ] + m2 * ddl1[ index2 ];
        
        ddl2[ ddl2_phs++ ] = tap1;
        ddl2_phs = ddl2_phs % ddl2_len;
        index1 = (ddl2_phs + idelay2) % ddl2_len;
        index2 = (index1 + 1) % ddl2_len ;
        frac = fdelay2 - idelay2 ;
        m1 = 1. - frac;
        m2 = frac;
        tap2 = m1 * ddl2[ index1 ] + m2 * ddl2[ index2 ];
        *out1++ = (insamp1+tap2) * 0.2;
        
        
        ///
        
    }
    x->ddl1_phs = ddl1_phs;
    x->osc1_phs = osc1_phs;
    x->ddl2_phs = ddl2_phs;
    x->osc2_phs = osc2_phs;
    x->tap1 = tap1;
    x->tap2 = tap2;
    // DSP CONFIG
    return (w+9);
}


// void lop_dsp(t_lop *x, t_signal **sp, short *count)
void flanjah_dsp(t_flanjah *x, t_signal **sp)
{

    x->feedback_connected = 1;
    x->speed1_connected = 1;
    x->speed2_connected = 1;
    x->depth_connected = 1;
    
    if(x->sr != sp[0]->s_sr){
        x->sr = sp[0]->s_sr;
        flanjah_init(x,1);
    }
    dsp_add(flanjah_perform, 8, x,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec,
            (t_int)sp[0]->s_n);
}

void flanjah_mute(t_flanjah *x, t_floatarg state)
{
    x->mute = state;
}

void flanjah_protect(t_flanjah *x, t_floatarg state)
{
    x->feedback_protect = state;
    //  post("feedback proctection set to %d",x->feedback_protect);
}

void flanjah_init(t_flanjah *x,short initialized)
{
    int i;
    if( x->maxdel < .0001 ){
        x->maxdel = .0001;
        error("below minimum of 0.01 ms");
    }
    if( x->maxdel > 360000. ){
        x->maxdel = 360000.;
        error("above maximum of 360 seconds");
    }
	
    x->si_factor = (float)F_LEN / x->sr;
    x->ddl1_len = x->maxdel * x->sr ;
    x->ddl1_phs = 0;
    x->ddl2_len = x->maxdel * x->sr ;
    x->ddl2_phs = 0;
    x->osc1_si = x->si_factor * x->speed1;
    x->osc1_phs = 0;
    x->osc2_si = x->si_factor * x->speed2;
    x->osc2_phs = 0;
	
    x->tap1 = x->tap2 = 0;
    if(!initialized){
        x->ddl1 = (float *) calloc(x->ddl1_len + 2, sizeof(float));
        x->ddl2 = (float *) calloc(x->ddl2_len + 2, sizeof(float));
        x->sinetab = (float *) calloc(F_LEN,sizeof(float));
        for( i = 0; i < F_LEN ; i++ ){
            x->sinetab[i] = 0.51 - 0.47 * cos( TWOPI * (float) i / (float) F_LEN);
        }
    } else {
        x->ddl1 = (float *) realloc(x->ddl1,(x->ddl1_len + 2) * sizeof(float));
        x->ddl2 = (float *) realloc(x->ddl2,(x->ddl2_len + 2) * sizeof(float));
    }
}

void *flanjah_new(t_symbol *s, int argc, t_atom *argv)
{

    t_flanjah *x = (t_flanjah *)pd_new(flanjah_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("signal"), gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    
    x->sr = sys_getsr();
    if(!x->sr){
        error("zero sampling rate - set to 44100");
        x->sr = 44100;
    }
    
    // SET DEFAULTS
    
    x->maxdel = .05; // in seconds
    x->feedback = 0.7;
    x->speed1 = 0.136;
    x->speed2 = 0.183;
    x->feedback_protect = 1;
    x->depth = 1.0;
    
    if( argc > 0 )
        x->maxdel = atom_getfloatarg(0,argc,argv)/1000.0;
    if( argc > 1 )
        x->feedback = atom_getfloatarg(1,argc,argv);
    if( argc > 2 )
        x->speed1 = atom_getfloatarg(2,argc,argv);
    if( argc > 3 )
        x->speed2 = atom_getfloatarg(3,argc,argv);
    if( argc > 4 )
        x->depth = atom_getfloatarg(4,argc,argv);	
    
    flanjah_init(x,0);
    return (x);
}

