#include <fluidsynth.h>

#include "m_pd.h"
 
static t_class *fluid_tilde_class;
 
typedef struct _fluid_tilde {
    t_object x_obj;
    fluid_synth_t *x_synth;
    fluid_settings_t *x_settings;
    t_outlet *x_out_left;
    t_outlet *x_out_right;
} t_fluid_tilde;
 
t_int *fluid_tilde_perform(t_int *w)
{
    t_fluid_tilde *x = (t_fluid_tilde *)(w[1]);
    t_sample *left = (t_sample *)(w[2]);
    t_sample *right = (t_sample *)(w[3]);
    int n = (int)(w[4]);

    //while (n--) *out++ = (*in1++)*(1-f_pan)+(*in2++)*f_pan;
    fluid_synth_write_float(x->x_synth, n, left, 0, 1, right, 0, 1);
 
    return (w+5);
}

static void fluid_tilde_dsp(t_fluid_tilde *x, t_signal **sp)
{
    dsp_add(fluid_tilde_perform, 4, x,
        sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void fluid_tilde_free(t_fluid_tilde *x)
{
    outlet_free(x->x_out_left);
    outlet_free(x->x_out_right);
}

static void fluid_help(void)
{
    const char * helptext =
        "_ __fluid~_ _  a soundfont external for Pd and Max/MSP \n"
        "_ argument: \"/path/to/soundfont.sf\" to load on object creation\n"
        "_ messages: \n"
        "load /path/to/soundfont.sf2  --- Loads a Soundfont \n"
        "note 0 0 0                   --- Play note. Arguments: \n"
        "                                 channel-# note-#  veloc-#\n"
        "n 0 0 0                      --- Play note, same as above\n"
        "0 0 0                        --- Play note, same as above\n"
        "control 0 0 0                --- set controller\n"
        "c       0 0 0                --- set controller, shortcut\n"
        "prog 0 0                     --- progam change, \n"
        "                                 args: channel-# prog-#\n"
        "p    0 0                     --- program change, shortcut\n"
    ;
    post("%s", helptext);
}

static void fluid_note(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_synth == NULL) return;
    if (argc == 3)
    {
        int chan, key, vel;
        chan = atom_getintarg(0, argc, argv);
        key = atom_getintarg(1, argc, argv);
        vel = atom_getintarg(2, argc, argv);
        fluid_synth_noteon(x->x_synth, chan - 1, key, vel);
    }
}

static void fluid_program_change(t_fluid_tilde *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (x->x_synth == NULL) return;
    if (argc == 2)
    {
        int chan, prog;
        chan = atom_getintarg(0, argc, argv);
        prog = atom_getintarg(1, argc, argv);
        fluid_synth_program_change(x->x_synth, chan - 1, prog);
    }
}

static void fluid_control_change(t_fluid_tilde *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (x->x_synth == NULL) return;
    if (argc == 3)
    {
        int chan, ctrl, val;
        chan = atom_getintarg(0, argc, argv);
        ctrl = atom_getintarg(1, argc, argv);
        val = atom_getintarg(2, argc, argv);
        fluid_synth_cc(x->x_synth, chan - 1, ctrl, val);
    }
}

static void fluid_pitch_bend(t_fluid_tilde *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (x->x_synth == NULL) return;
    if (argc == 2)
    {
        int chan, val;
        chan = atom_getintarg(0, argc, argv);
        val = atom_getintarg(1, argc, argv);
        fluid_synth_pitch_bend(x->x_synth, chan - 1, val);
    }
}

static void fluid_bank(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_synth == NULL) return;
    if (argc == 2)
    {
        int chan, bank;
        chan = atom_getintarg(0, argc, argv);
        bank = atom_getintarg(1, argc, argv);
        fluid_synth_bank_select(x->x_synth, chan - 1, bank);
    }
}

static void fluid_gen(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_synth == NULL) return;
    if (argc == 3)
    {
        int chan, param;
        float value;
        chan = atom_getintarg(0, argc, argv);
        param = atom_getintarg(1, argc, argv);
        value = atom_getintarg(2, argc, argv);
        fluid_synth_set_gen(x->x_synth, chan - 1, param, value);
    }
}

static void fluid_load(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_synth == NULL)
    {
        post("No fluidsynth");
        return;
    }
    if (argc >= 1 && argv->a_type == A_SYMBOL)
    {
        const char* filename = atom_getsymbolarg(0, argc, argv)->s_name;
        if (fluid_synth_sfload(x->x_synth, filename, 0) >= 0)
        {
            post("Loaded Soundfont: %s", filename);
            fluid_synth_program_reset(x->x_synth);
        }
    }
}

static void fluid_init(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_synth) delete_fluid_synth(x->x_synth);

    float sr = sys_getsr();

    x->x_settings = new_fluid_settings();

    if (x->x_settings == NULL)
    {
        post("fluid~: couldn't create synth settings\n");
    }
    else
    {
        // fluid_settings_setstr(settings, "audio.driver", "float");
        // settings:
        fluid_settings_setnum(x->x_settings, "synth.midi-channels", 16);
        fluid_settings_setnum(x->x_settings, "synth.polyphony", 256);
        fluid_settings_setnum(x->x_settings, "synth.gain", 0.600000);
        fluid_settings_setnum(x->x_settings, "synth.sample-rate", 44100.000000);
        fluid_settings_setstr(x->x_settings, "synth.chorus.active", "no");
        fluid_settings_setstr(x->x_settings, "synth.reverb.active", "no");
        fluid_settings_setstr(x->x_settings, "synth.ladspa.active", "no");

        if (sr != 0)
        {
            fluid_settings_setnum(x->x_settings, "synth.sample-rate", sr);
        }
        // Create fluidsynth instance:
        x->x_synth = new_fluid_synth(x->x_settings);
        if (x->x_synth == NULL )
        {
            post("fluid~: couldn't create synth\n");
        }
        // try to load argument as soundfont
        fluid_load(x, gensym("load"), argc, argv);
        //if (settings != NULL )
        //      delete_fluid_settings(settings);

        // We're done constructing:
        if (x->x_synth)
            post("-- fluid~ for Pd ---");
    }
}

static void *fluid_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fluid_tilde *x = (t_fluid_tilde *)pd_new(fluid_tilde_class);
    x->x_out_left = outlet_new(&x->x_obj, &s_signal);
    x->x_out_right = outlet_new(&x->x_obj, &s_signal);
    fluid_init(x, gensym("init"), argc, argv);
    return (void *)x;
}
 
void fluid_tilde_setup(void)
{
    fluid_tilde_class = class_new(gensym("fluid~"),
        (t_newmethod)fluid_tilde_new, 0, sizeof(t_fluid_tilde),
        CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_init, gensym("init"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_load, gensym("load"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_note, gensym("note"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_program_change,
        gensym("prog"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_control_change,
        gensym("control"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_pitch_bend,
        gensym("bend"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_bank, gensym("bank"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_gen, gensym("gen"),
        A_GIMME, 0);

    // list input calls fluid_note(...)
    class_addlist(fluid_tilde_class, (t_method)fluid_note);

    // some alias shortcuts:
    class_addmethod(fluid_tilde_class, (t_method)fluid_note, gensym("n"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_program_change,
        gensym("p"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_control_change,
        gensym("c"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_control_change,
        gensym("cc"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_pitch_bend, gensym("b"),
        A_GIMME, 0);

    // Simulate Flext's help message
    class_addmethod(fluid_tilde_class, (t_method)fluid_help, gensym("help"),
        0);

    class_addmethod(fluid_tilde_class,
        (t_method)fluid_tilde_dsp, gensym("dsp"), A_CANT, 0);
}
