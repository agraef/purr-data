#include "MSPd.h"

/* comment */

static t_class *function_class;
#define OBJECT_NAME "function~"

typedef struct _function
{
    t_object x_obj;
    t_float x_f;
    t_symbol *wavename; // name of waveform buffer
    
    int b_frames;
    int b_nchans;
    t_word *b_samples;
    short normalize;
} t_function;

void function_setbuf(t_function *x, t_symbol *wavename);
void *function_new(t_symbol *msg, int argc, t_atom *argv);
void function_dsp(t_function *x, t_signal **sp);
void function_redraw(t_function *x);
void function_clear(t_function *x);
void function_addsyn(t_function *x, t_symbol *msg, int argc, t_atom *argv);
void function_aenv(t_function *x, t_symbol *msg, int argc, t_atom *argv);
void function_adenv(t_function *x, t_symbol *msg, int argc, t_atom *argv);
void function_normalize(t_function *x, t_floatarg f);
void function_adrenv(t_function *x, t_symbol *msg, int argc, t_atom *argv);
void function_rcos(t_function *x);
void function_gaussian(t_function *x);
void function_print(t_function *x);

void function_tilde_setup(void)
{
    function_class = class_new(gensym("function~"), (t_newmethod)function_new,
                               NO_FREE_FUNCTION,sizeof(t_function), 0,A_GIMME,0);
    CLASS_MAINSIGNALIN(function_class, t_function, x_f);
    class_addmethod(function_class,(t_method)function_dsp,gensym("dsp"),0);
    class_addmethod(function_class,(t_method)function_addsyn,gensym("addsyn"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_aenv,gensym("aenv"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_adenv,gensym("adenv"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_adrenv,gensym("adrenv"),A_GIMME,0);
    class_addmethod(function_class,(t_method)function_clear,gensym("clear"),0);
    class_addmethod(function_class,(t_method)function_normalize,gensym("normalize"),A_FLOAT,0);
    class_addmethod(function_class,(t_method)function_rcos,gensym("rcos"),0);
    class_addmethod(function_class,(t_method)function_gaussian,gensym("gaussian"),0);
    class_addmethod(function_class,(t_method)function_print,gensym("print"),0);
    potpourri_announce(OBJECT_NAME);
}

void function_rcos(t_function *x)
{
    int i;
    long b_frames;
    t_word *b_samples;
    function_setbuf(x, x->wavename);
    b_samples = x->b_samples;
    b_frames = x->b_frames;
    if(b_frames == 0){
        return;
    }
    for(i=0;i<b_frames;i++){
        x->b_samples[i].w_float = (t_float) (0.5 - 0.5 * cos(TWOPI * (t_float)i/(t_float)b_frames));
    }
    function_redraw(x);
    /* for(i=0;i<b_frames;i++){
        post("%f", x->b_samples[i].w_float);
    } */
}

/*
void function_rcos(t_function *x)
{
    int i;
    long b_frames;
    t_word *b_samples;
    t_symbol *wavename = x->wavename;
    int frames;
    t_garray *a;
    
    x->b_frames = 0;
    x->b_nchans = 1;
    // open array
    if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
        if (*wavename->s_name) pd_error(x, "function~: %s: no such array", wavename->s_name);
    }
    else if (!garray_getfloatwords(a, &frames, &x->b_samples)) {
        pd_error(x, "%s: bad template for function~", wavename->s_name);
    }
    else  {
        x->b_frames = frames;
        garray_usedindsp(a);
    }
    b_frames = x->b_frames;
    post("rcos generating %d frames", b_frames); // correct
    // put samples into array
    for(i=0;i<b_frames;i++){
        x->b_samples[i].w_float = (t_float) (0.5 - 0.5 * cos(TWOPI * (t_float)i/(t_float)b_frames));
    }
    if (!(a = (t_garray *)pd_findbyclass(x->wavename, garray_class))) {
        if (*x->wavename->s_name) pd_error(x, "function~: %s: no such array", x->wavename->s_name);
    }
    else  {
        garray_redraw(a);
    }
    // printed samples are correct
    for(i=0;i<b_frames;i++){
        post("%f", x->b_samples[i].w_float);
    }
}
*/
void function_print(t_function *x)
{
    int frames;
    t_garray *a;
    int i;
    x->b_frames = 0;
    x->b_nchans = 1;
    if (!(a = (t_garray *)pd_findbyclass(x->wavename, garray_class))) {
        if (*x->wavename->s_name) pd_error(x, "function~: %s: no such array", x->wavename->s_name);
    }
    else if (!garray_getfloatwords(a, &frames, &x->b_samples)) {
        pd_error(x, "%s: bad template for function~", x->wavename->s_name);
    }
    else  {
        x->b_frames = frames;
        garray_usedindsp(a);
        for(i = 0; i < frames; i++){
            post("%d: %f",i, x->b_samples[i].w_float);
        }
    }
}


void function_gaussian(t_function *x)
{
    int i;
    long b_frames;
    t_word *b_samples;
    t_float arg, xarg,in;
    
    if(!b_frames){
        post("* zero length function!");
        return;
    }
    function_setbuf(x, x->wavename);
    b_frames = x->b_frames;
    b_samples = x->b_samples;
    arg = 12.0 / (t_float)b_frames;
    xarg = 1.0;
    in = -6.0;
    
    for(i=0;i<b_frames;i++){
        b_samples[i].w_float = xarg * pow(2.71828, -(in*in)/2.0);
        in += arg;
    }
    function_redraw(x);
}

void function_normalize(t_function *x, t_floatarg f)
{
    x->normalize = (short)f;
}

void function_clear(t_function *x)
{
    int i;
    long b_frames = x->b_frames;
    t_word *b_samples = x->b_samples;
    
    function_setbuf(x, x->wavename);
    b_frames = x->b_frames;
    b_samples = x->b_samples;
    for(i=0;i<b_frames;i++)
        b_samples[i].w_float = 0.0;
    function_redraw(x);
}

void function_adrenv(t_function *x, t_symbol *msg, int argc, t_atom *argv)
{
    int i,j;
    int al, dl, sl, rl;
    long b_frames = x->b_frames;
    t_word *b_samples = x->b_samples;
    t_float downgain = 0.33;
    
    function_setbuf(x, x->wavename);
    al = (t_float) b_frames * atom_getfloatarg(0,argc,argv);
    dl = (t_float) b_frames * atom_getfloatarg(1,argc,argv);
    rl = (t_float) b_frames * atom_getfloatarg(2,argc,argv);
    downgain = atom_getfloatarg(3,argc,argv);
    if(downgain <= 0)
        downgain = 0.333;
    if(al+dl+rl >= b_frames){
        post("atk and dk and release are too long");
        return;
    }
    sl = b_frames - (al+dl+rl);
    
    for(i=0;i<al;i++){
        b_samples[i].w_float = (t_float)i/(t_float)al;
    }
    for(i=al, j=dl;i<al+dl;i++,j--){
        b_samples[i].w_float = downgain + (1.-downgain)*(t_float)j/(t_float)dl;
    }
    for(i=al+dl;i<al+dl+sl;i++){
        b_samples[i].w_float = downgain;
    }
    for(i=al+dl+sl,j=rl;i<b_frames;i++,j--){
        b_samples[i].w_float = downgain * (t_float)j/(t_float)rl;
    }
    function_redraw(x);
}

void function_adenv(t_function *x, t_symbol *msg, int argc, t_atom *argv)
{
    int i,j;
    int al, dl, rl;
    long b_frames = x->b_frames;
    t_word *b_samples = x->b_samples;
    t_float downgain = 0.33;
    
    function_setbuf(x, x->wavename);
    al = (t_float) b_frames * atom_getfloatarg(0,argc,argv);
    dl = (t_float) b_frames * atom_getfloatarg(1,argc,argv);
    downgain = atom_getfloatarg(2,argc,argv);
    if(downgain <= 0)
        downgain = 0.333;
    if(al+dl >= b_frames){
        post("atk and dk are too long");
        return;
    }
    rl = b_frames - (al+dl);
    
    for(i=0;i<al;i++){
        b_samples[i].w_float = (t_float)i/(t_float)al;
    }
    for(i=al, j=dl;i<al+dl;i++,j--){
        b_samples[i].w_float = downgain + (1.-downgain)*(t_float)j/(t_float)dl;
    }
    for(i=al+dl,j=rl;i<b_frames;i++,j--){
        b_samples[i].w_float = downgain * (t_float)j/(t_float)rl;
    }
    function_redraw(x);
}

void function_aenv(t_function *x, t_symbol *msg, int argc, t_atom *argv)
{
    int i,j;
    int al, dl;
    long b_frames = x->b_frames;
    t_word *b_samples = x->b_samples;
    t_float frac;
    frac = atom_getfloatarg(0,argc,argv);
    
    function_setbuf(x, x->wavename);
    if(frac <= 0 || frac >= 1){
        post("* attack time must range from 0.0 - 1.0, rather than %f",frac);
    }
    
    al = b_frames * frac;
    
    dl = b_frames - al;
    for(i=0;i<al;i++){
        b_samples[i].w_float = (t_float)i/(t_float)al;
    }
    for(i=al, j=dl;i<b_frames;i++,j--){
        b_samples[i].w_float = (t_float)j/(t_float)dl;
    }
    function_redraw(x);
}

void function_addsyn(t_function *x, t_symbol *msg, int argc, t_atom *argv)
{
    int i,j;
    long b_frames;
    t_word *b_samples;
    t_float amp;
    t_float maxamp, rescale;
    t_float theSample;
    
    /* for(i = 0; i < argc; i++){
        post("argument %d: %f",i, atom_getfloatarg(i,argc,argv));
    }*/
    
    function_setbuf(x, x->wavename);
    b_samples = x->b_samples;
    b_frames = x->b_frames;
    amp = atom_getfloatarg(0,argc,argv);
    // post("harmonic: 0, weight: %.12f", (float)amp);
    for(i=0;i<b_frames;i++){
        b_samples[i].w_float = amp;
    }
    for(j=1;j<argc;j++){
        amp = atom_getfloatarg(j,argc,argv);
        if(amp){
           // post("harmonic: %d, weight: %.12f", j, (t_float)amp);
            for(i=0;i<b_frames;i++){
                theSample = amp * sin(TWOPI * (t_float) j * (t_float)i/(t_float)b_frames);
                b_samples[i].w_float += theSample;
               // post("%d: %f", i, (t_float)b_samples[i].w_float);
            }
        }
    }
    
    if(x->normalize){
        maxamp = 0;
        for(i=0;i<b_frames;i++){
            if(maxamp < fabs(b_samples[i].w_float))
                maxamp = fabs(b_samples[i].w_float);
        }
        if(!maxamp){
            post("* zero maxamp!");
            return;
        }
        rescale = 1.0 / maxamp;
        for(i=0;i<b_frames;i++){
            b_samples[i].w_float *= rescale;
        }
    }
    
    /*
    for(i=0;i<b_frames;i++){
        post("%f", x->b_samples[i].w_float);
    }
    */
    function_redraw(x);
}


void *function_new(t_symbol *msg, int argc, t_atom *argv)
{
    
    t_function *x = (t_function *)pd_new(function_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->wavename = atom_getsymbolarg(0,argc,argv);
    x->normalize = 1;
//    post("float size: %d, t_float size: %d, t_word size %d", sizeof(float), sizeof(t_float), sizeof(t_word));
//    post("latest version");
    return x;
}

void function_setbuf(t_function *x, t_symbol *wavename)
{
    int frames;
    t_garray *a;
    
    x->b_frames = 0;
    x->b_nchans = 1;
    if (!(a = (t_garray *)pd_findbyclass(wavename, garray_class))) {
        if (*wavename->s_name) pd_error(x, "function~: %s: no such array", wavename->s_name);
    }
    else if (!garray_getfloatwords(a, &frames, &x->b_samples)) {
        pd_error(x, "%s: bad template for function~", wavename->s_name);
    }
    else  {
        x->b_frames = frames;
        garray_usedindsp(a);
        // post("%d frames in buffer", x->b_frames);
    }
}

void function_redraw(t_function *x)
{
    t_garray *a;
    if (!(a = (t_garray *)pd_findbyclass(x->wavename, garray_class))) {
        if (*x->wavename->s_name) pd_error(x, "function~: %s: no such array", x->wavename->s_name);
    }
    else  {
        garray_redraw(a);
    }
}



void function_dsp(t_function *x, t_signal **sp)
{
}

