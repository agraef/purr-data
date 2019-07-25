#include "MSPd.h"


static t_class *sigseq_class;

#define MAX_VEC 2048
#define MAX_SEQ 1024
#define internal_clock 1
#define EXTERNAL_CLOCK 2

#define OBJECT_NAME "sigseq~"
typedef struct _sigseq
{
    
    t_object x_obj;
    float x_f;
    
    // Variables Here
    float *sequence;
    float *trigger_vec; // stores vector with trigger click
    int seq_len;
    int seq_ptr;// position in sequence
    short bang_ptr;
    float tempo;
    int beat_subdiv;
    int tsamps;
    int counter;
    float val;
    void *mybang;
    void *m_outlet; // NEW
    void *m_clock; // NEW
    float flat_gain;
    float last_val;
    short retro_state;
    short rand_state;
    // ADSR
    float a;
    float d;
    float s;
    float r;
    int ebreak1;
    int ebreak2;
    int ebreak3;
    int asamps;
    int dsamps;
    int ssamps;
    int rsamps;
    float egain;
    int do_envelope;
    int bang_on;
    short mute;
    int rval;
    int method; //synthesis method to use
    float sr;
} t_sigseq;

void *sigseq_new(t_symbol *s, int argc, t_atom *argv);

t_int *sigseq_perform(t_int *w);
t_int *sigseq_perform_clickin(t_int *w);
void sigseq_dsp(t_sigseq *x, t_signal **sp);
void sigseq_list (t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_adsr (t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_adsrgate (t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_banggate (t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_tempo(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_retro(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_rand(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_mute(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv);
void sigseq_tick(t_sigseq *x);
void sigseq_report(t_sigseq *x);
void sigseq_readfile(t_sigseq *x, t_symbol *filename);
void sigseq_internal_clock(t_sigseq *x, t_floatarg toggle);
void sigseq_external_clock(t_sigseq *x, t_floatarg toggle);
void sigseq_gozero(t_sigseq *x);
void sigseq_free(t_sigseq *x);
void sigseq_init(t_sigseq *x,short initialized);


void sigseq_tilde_setup(void){
    sigseq_class = class_new(gensym("sigseq~"), (t_newmethod)sigseq_new,
                             (t_method)sigseq_free ,sizeof(t_sigseq), 0,A_GIMME,0);
    CLASS_MAINSIGNALIN(sigseq_class, t_sigseq, x_f);
    class_addmethod(sigseq_class,(t_method)sigseq_dsp,gensym("dsp"),0);
    class_addmethod(sigseq_class,(t_method)sigseq_mute,gensym("mute"),A_FLOAT,0);
    class_addmethod(sigseq_class,(t_method)sigseq_list,gensym("list"),A_GIMME,0);
    class_addmethod(sigseq_class,(t_method)sigseq_adsr,gensym("adsr"),A_GIMME,0);
    class_addmethod(sigseq_class,(t_method)sigseq_adsrgate,gensym("adsrgate"),A_GIMME,0);
    class_addmethod(sigseq_class,(t_method)sigseq_banggate,gensym("banggate"),A_GIMME,0);
    class_addmethod(sigseq_class,(t_method)sigseq_tempo,gensym("tempo"),A_GIMME,0);
    class_addmethod(sigseq_class,(t_method)sigseq_retro,gensym("retro"),A_GIMME,0);
    class_addmethod(sigseq_class,(t_method)sigseq_rand,gensym("rand"),A_GIMME,0);
    class_addmethod(sigseq_class,(t_method)sigseq_report,gensym("report"),0);
    class_addmethod(sigseq_class,(t_method)sigseq_gozero,gensym("gozero"),0);
    class_addmethod(sigseq_class,(t_method)sigseq_internal_clock,gensym("internal_clock"),A_FLOAT,0);
    class_addmethod(sigseq_class,(t_method)sigseq_external_clock,gensym("external_clock"),A_FLOAT,0);
    potpourri_announce(OBJECT_NAME);
}

void sigseq_gozero(t_sigseq *x)
{
    if(x->seq_len <= 0)
        return;
    x->seq_ptr = x->seq_len - 1;
}

void sigseq_internal_clock(t_sigseq *x, t_floatarg toggle)
{
    if(toggle){
        x->method = internal_clock;
    } else {
        x->method = EXTERNAL_CLOCK;
    }
}

void sigseq_external_clock(t_sigseq *x, t_floatarg toggle)
{
    if(toggle){
        x->method = EXTERNAL_CLOCK;
    } else {
        x->method = internal_clock;
    }
    post("method is %d",x->method);
}

void sigseq_report(t_sigseq *x)
{
    int i;
    post("randstate: %d", x->rand_state);
    post("rval: %d", x->rval);
    post("seqpt: %d", x->seq_ptr);
    post("manual rnd pos: %d", x->rval % x->seq_len);
    
    for(i=0;i<x->seq_len;i++){
        post("%f",x->sequence[i]);
    }
}

void sigseq_readfile(t_sigseq *x, t_symbol *filename)
{
    FILE *fp;
    float data;
    post("requested path: %s", filename->s_name);
    fp = fopen(filename->s_name, "r");
    if( fp == NULL ){
        post("could not open file!");
        return;
    }
    while( fscanf(fp, "%f", &data) != EOF ){
        post("%f",data);
    }
    fclose(fp);
}

void sigseq_mute(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    x->mute = atom_getfloatarg(0,argc,argv);
}


void sigseq_rand(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    x->rand_state = atom_getfloatarg(0,argc,argv);	
}

void sigseq_retro(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    x->retro_state = atom_getfloatarg(0,argc,argv);
	
}

void sigseq_adsrgate(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    
    x->do_envelope = atom_getfloatarg(0,argc,argv);
}

void sigseq_banggate(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    
    x->bang_on = atom_getfloatarg(0,argc,argv);
}

void sigseq_tempo(t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    float beatdur;
	
    x->tempo = atom_getfloatarg(0,argc,argv);
    if(!x->tempo)
        x->tempo = 120;
   	
    beatdur = (60. / x->tempo ) / (float) x->beat_subdiv ;
    x->tsamps = x->sr * beatdur;
   	
    x->asamps = x->sr * x->a;
    x->dsamps = x->sr * x->d;
    x->rsamps = x->sr * x->r;
    x->ssamps = x->tsamps - (x->asamps+x->dsamps+x->rsamps);
    x->ebreak1 = x->asamps;
    x->ebreak2 = x->asamps+x->dsamps;
    x->ebreak3 = x->asamps+x->dsamps+x->ssamps;
    if( x->ssamps < 0 ){
        x->ssamps = 0;
        // post("adsr: Warning: zero duration sustain");
    }
}
void *sigseq_new(t_symbol *s, int argc, t_atom *argv)
{
    t_sigseq *x = (t_sigseq *)pd_new(sigseq_class);
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->mybang = outlet_new(&x->x_obj, gensym("bang"));
    x->m_clock = clock_new(x,(void *)sigseq_tick);
    
    srand(clock());
    x->tempo = atom_getfloatarg(0,argc,argv);
    x->beat_subdiv = atom_getfloatarg(1,argc,argv);
    x->flat_gain = atom_getfloatarg(2,argc,argv);
    if( x->tempo <= 0 )
        x->tempo = 120.;
    if(x->beat_subdiv <= 0)
        x->beat_subdiv = 2;
    if(x->flat_gain<= 0)
        x->flat_gain = 0.5;
    
    x->sr = sys_getsr();
    if(!x->sr){
        x->sr = 44100;
        error("zero sampling rate - set to 44100");
    }
    sigseq_init(x,0);
	
    return (x);
}

void sigseq_init(t_sigseq *x,short initialized)
{
    float beatdur;
    int asamp, dsamp, ssamp, rsamp;
    //  int i;
    
    if(!initialized){
        x->sequence = (float *) t_getbytes(MAX_SEQ * sizeof(float));
        x->trigger_vec = (float *) t_getbytes(MAX_VEC * sizeof(float));
        x->seq_len = 3;
        x->seq_ptr = 0;
        x->bang_ptr = 0;
        x->sequence[0] = 313;
        x->sequence[1] = 511;
        x->sequence[2] = 71;
        x->method = internal_clock;
        x->a = .005;
        x->d = .01;
        x->r = .2;
        x->egain = .707;
        x->do_envelope = 1;
        x->bang_on = 0;
        x->retro_state = 0;
        x->rand_state = 0;
        x->mute = 0;
    }
    
    beatdur = (60. / x->tempo ) / (float) x->beat_subdiv;
    x->tsamps = x->sr * beatdur;
    x->counter = x->tsamps ;
    x->last_val = 666.6661;
    x->val = x->sequence[0];
    asamp = x->sr * x->a;
    dsamp = x->sr * x->d;
    rsamp = x->sr * x->r;
    ssamp = x->tsamps - (asamp+dsamp+rsamp);
    if( ssamp < 0 ){
        ssamp = 0;
    }
    x->ebreak1 = asamp;
    x->ebreak2 = asamp+dsamp;
    x->ebreak3 = asamp+dsamp+ssamp;
    x->asamps = asamp;
    x->dsamps = dsamp;
    x->ssamps = ssamp;
    x->rsamps = rsamp;
}

void sigseq_tick(t_sigseq *x)
{
    if(x->seq_ptr) //weird that we need this
        x->seq_ptr = 0;
    //  post("bang: val %f s0 %f pt %d",x->val,x->sequence[0],x->seq_ptr);
    outlet_bang(x->mybang);
}

void sigseq_list (t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    short i;
    
    if( argc < 1 ){
        // post("null list ignored");
        return;
    }
    x->seq_len = 0;
    for( i = 0; i < argc; i++ ){
        x->sequence[i] = atom_getfloatarg(i,argc,argv);
        ++(x->seq_len);
    }
    x->seq_ptr = 0;
    x->val = x->sequence[ 0 ];
    x->counter = 0 ;
    //  sigseq_tick(x);
    return;
}

void sigseq_adsr (t_sigseq *x, t_symbol *msg, short argc, t_atom *argv)
{
    //  short i;
    
    if( argc != 4 ){
        error("sigseq~: bad arguments for adsr");
        return;
    }
    x->a = atom_getfloatarg(0,argc,argv) * .001;
    x->d = atom_getfloatarg(1,argc,argv) * .001;
    x->r = atom_getfloatarg(2,argc,argv) * .001;
    x->egain = atom_getfloatarg(3,argc,argv);
    
    x->asamps = x->sr * x->a;
    x->dsamps = x->sr * x->d;
    x->rsamps = x->sr * x->r;
    x->ssamps = x->tsamps - (x->asamps+x->dsamps+x->rsamps);
    x->ebreak1 = x->asamps;
    x->ebreak2 = x->asamps+x->dsamps;
    x->ebreak3 = x->asamps+x->dsamps+x->ssamps;
    if( x->ssamps < 0 ){
        x->ssamps = 0;
        // post("adsr: Warning: zero duration sustain");
    }
    //  	post("A %d D %d S %d R %d gain %f",x->asamps,x->dsamps,x->ssamps,x->rsamps,x->egain);
    return;
}
t_int *sigseq_perform(t_int *w)
{
    
    t_sigseq *x = (t_sigseq *) (w[1]);
    //  t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    t_float *adsr = (t_float *)(w[4]);
    int n = (int) w[5];
    /*********************************************/
    float *sequence = x->sequence;
    int seq_len = x->seq_len;
    int seq_ptr = x->seq_ptr;
    int tsamps = x->tsamps;
    int counter = x->counter;
    float val = x->val;
    float last_val = x->last_val;
    int ebreak1 = x->ebreak1;
    int ebreak2 = x->ebreak2;
    int ebreak3 = x->ebreak3;
    float egain = x->egain;
    float env_val;
    float flat_gain = x->flat_gain;
    int do_envelope = x->do_envelope;
    int asamps = x->asamps;
    int dsamps = x->dsamps;
    //  int ssamps = x->ssamps;
    int rsamps = x->rsamps;
    int bang_on = x->bang_on;
    short bang_ptr = x->bang_ptr;
    short retro_state = x->retro_state;
    short rand_state = x->rand_state;
    float etmp;
    //  short bang_me_now = 0 ;
    float trand;
    /*********************************************/
    if(x->mute){
        while (n--) {
            *out++ = 0.0;
        }
        return (w+6);
    } else {
        
        while(n--) {
            if(counter >= tsamps){
                counter = 0;
                bang_ptr = (bang_ptr + 1) % seq_len ;
                if (rand_state) {
                    trand = (float) ( rand() % 32768 / 32768.0) * (float) seq_len;
                    x->rval = trand ;
                    seq_ptr = x->rval % seq_len;
                    
                }
                else if (retro_state) {
                    seq_ptr = (seq_ptr - 1) % seq_len ;
                    if( seq_ptr < 0) {
                        seq_ptr = seq_len - 1;
                    }
                    //
                }
                else {
                    seq_ptr = (seq_ptr + 1) % seq_len ;
                }
                if ( seq_ptr >= seq_len || seq_ptr < 0) {
                    seq_ptr = 1;
                }
                val = sequence[ seq_ptr ];
                if( bang_ptr == 0 && bang_on) {
                    clock_delay(x->m_clock,0);
                }
            }
            *out++ = val;
            if( do_envelope ) {
                if( counter < ebreak1 ){
                    env_val = (float) counter / (float) asamps;
                } else if (counter < ebreak2) {
                    etmp = (float) (counter - ebreak1) / (float) dsamps;
                    env_val = (1.0 - etmp) + (egain * etmp);
                } else if (counter < ebreak3) {
                    env_val = egain;
                } else {
                    env_val = ((float)(tsamps-counter)/(float)rsamps) * egain ;
                }
                *adsr++ = env_val;
            } else {
                *adsr++ = flat_gain;
            }
            counter++;
        }
        if( last_val != val) {
            last_val = val;
        }
        
        x->seq_ptr = seq_ptr;
        x->bang_ptr = bang_ptr;
        x->counter = counter;
        x->val = val;
        x->last_val = last_val;
        
    }
    return (w+6);
}


t_int *sigseq_perform_clickin(t_int *w)
{
    
    t_sigseq *x = (t_sigseq *) (w[1]);
    t_float *trigger = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    t_float *adsr = (t_float *)(w[4]);
    int n = (int) w[5];
    /*********************************************/
    float *sequence = x->sequence;
    int seq_len = x->seq_len;
    int seq_ptr = x->seq_ptr;
    int tsamps = x->tsamps;
    int counter = x->counter;
    //  float val = x->val;
    float last_val = x->last_val;
    int ebreak1 = x->ebreak1;
    int ebreak2 = x->ebreak2;
    int ebreak3 = x->ebreak3;
    float egain = x->egain;
    float env_val;
    float flat_gain = x->flat_gain;
    int do_envelope = x->do_envelope;
    int asamps = x->asamps;
    int dsamps = x->dsamps;
    //  int ssamps = x->ssamps;
    int rsamps = x->rsamps;
    int bang_on = x->bang_on;
    short bang_ptr = x->bang_ptr;
    short retro_state = x->retro_state;
    short rand_state = x->rand_state;
    float *trigger_vec = x->trigger_vec;
    float etmp;
    //  short bang_me_now = 0 ;
    float trand;
    int i;
    /*********************************************/
    if(x->mute){
        while (n--) {
            *out++ = 0.0;
        }
        return (w+6);
    }
 	
    for(i = 0; i < n; i++){
        trigger_vec[i] = trigger[i];
    }
    for(i = 0; i < n; i++) {
        if(trigger_vec[i]){
            counter = 0;
            //     bang_ptr = (bang_ptr + 1) % seq_len ;
            
            if (rand_state) {
                trand = (float) ( rand() % 32768 / 32768.0) * (float) seq_len;
                x->rval = trand ;
                x->seq_ptr = x->rval % seq_len;	
                
            }
            else if (retro_state) {
                x->seq_ptr = (seq_ptr - 1) % seq_len ;
                if( x->seq_ptr < 0) {
                    x->seq_ptr = seq_len - 1;
                }
            } 
            else {
                x->seq_ptr = (x->seq_ptr + 1) % seq_len ;
            }
            if ( x->seq_ptr >= seq_len || x->seq_ptr < 0) {
                x->seq_ptr = 1;
            }
            x->val = sequence[x->seq_ptr];			
            if(x->seq_ptr == 0 && bang_on) {
                clock_delay(x->m_clock,0);
            }
        }
        out[i] = x->val;
        if( do_envelope ) {
            if( counter < ebreak1 ){
                env_val = (float) counter / (float) asamps;
            } else if (counter < ebreak2) {
                etmp = (float) (counter - ebreak1) / (float) dsamps;
                env_val = (1.0 - etmp) + (egain * etmp);
            } else if (counter < ebreak3) {
                env_val = egain;
            } else if(counter < tsamps) {
                env_val = ((float)(tsamps-counter)/(float)rsamps) * egain ;
            } else {
                env_val = 0;
            }
            adsr[i] = env_val;
        } else {
            adsr[i] = flat_gain;
        }
        counter++;
    }
    if( last_val != x->val) {
        last_val = x->val;		
    }
    
    x->bang_ptr = bang_ptr;
    x->counter = counter;
    
    x->last_val = last_val;
	
    return (w+6);
}		

void sigseq_free(t_sigseq *x)
{
    
    t_freebytes(x->sequence, MAX_SEQ * sizeof(float));
    t_freebytes(x->trigger_vec, MAX_VEC * sizeof(float));
}

void sigseq_dsp(t_sigseq *x, t_signal **sp)
{
    if(!sp[0]->s_sr)
        return;
    if(x->sr != sp[0]->s_sr){
        x->sr = sp[0]->s_sr;
        sigseq_init(x,1);
    }
    if(x->method == EXTERNAL_CLOCK){
        dsp_add(sigseq_perform_clickin, 5, x, 
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
    } else {
        dsp_add(sigseq_perform, 5, x, 
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
    }
}

