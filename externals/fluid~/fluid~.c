// Original version by Frank Barknecht (fbar@footils.org) 2003
// Ported from Flext/C++ to plain C/pdlibbuilder by Jonathan Wilkes 2016
// SMMF mode and various other little improvements by Albert Gräf 2020
// Distributed under the GPLv2+, please check the LICENSE file for details.

#include <fluidsynth.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "m_pd.h"
 
static t_class *fluid_tilde_class;
 
typedef struct _fluid_tilde {
    t_object x_obj;
    fluid_synth_t *x_synth;
    fluid_settings_t *x_settings;
    t_outlet *x_out_left;
    t_outlet *x_out_right;
    t_canvas *x_canvas;
    int smmf_mode;
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
    if (x->x_synth) delete_fluid_synth(x->x_synth);
    if (x->x_settings) delete_fluid_settings(x->x_settings);
}

static void fluid_help(void)
{
    const char * helptext =
        "fluid~: a soundfont external for Pd and Max/MSP\n"
        "options:\n"
        "-smmf: enable SMMF mode (https://bitbucket.org/agraef/pd-smmf)\n"
        "any other symbol: soundfont file to load on object creation\n"
        "messages:\n"
        "load /path/to/soundfont.sf2  --- Loads a Soundfont\n"
        "note 0 0 0                   --- Play note, arguments:\n"
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

static void fluid_legacy_note(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
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

static void fluid_legacy_program_change(t_fluid_tilde *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (x->x_synth == NULL) return;
    if (argc == 2)
    {
        int chan, prog;
        chan = atom_getintarg(0, argc, argv);
        prog = atom_getintarg(1, argc, argv);
        fluid_synth_program_change(x->x_synth, chan - 1, prog - 1);
    }
}

static void fluid_legacy_control_change(t_fluid_tilde *x, t_symbol *s, int argc,
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

static void fluid_legacy_pitch_bend(t_fluid_tilde *x, t_symbol *s, int argc,
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

static void fluid_legacy_bank(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
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

static void fluid_legacy_gen(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
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

// Note that in all the SMMF methods we allow the channel to be omitted, in
// which case it defaults to 1. Also note that the channel argument *always*
// comes last, and that the argument order, being in 1-1 correspondence with
// the Pd MIDI objects, is a bit different from the legacy message format
// above which follows the MIDI standard instead.

// The system realtime messages start, stop, and cont are in SMMF, but not
// recognized by fluidsynth, so we don't support them here either. (MTS) sysex
// messages (which fluidsynth recognizes) are supported, however.

// Please check https://bitbucket.org/agraef/pd-smmf for details.

static void fluid_note(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->smmf_mode)
    {
        fluid_legacy_note(x, s, argc, argv); return;
    }
    if (x->x_synth == NULL) return;
    if (argc == 2 || argc == 3)
    {
        int key = atom_getintarg(0, argc, argv);
        int vel = atom_getintarg(1, argc, argv);
        int chan = argc>2 ? atom_getintarg(2, argc, argv) : 1;
        fluid_synth_noteon(x->x_synth, chan - 1, key, vel);
    }
}

static void fluid_ctl(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->smmf_mode || x->x_synth == NULL) return;
    if (argc == 2 || argc == 3)
    {
        int val = atom_getintarg(0, argc, argv);
        int ctrl = atom_getintarg(1, argc, argv);
        int chan = argc>2 ? atom_getintarg(2, argc, argv) : 1;
        fluid_synth_cc(x->x_synth, chan - 1, ctrl, val);
    }
}

static void fluid_pgm(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->smmf_mode || x->x_synth == NULL) return;
    if (argc == 1 || argc == 2)
    {
        int prog = atom_getintarg(0, argc, argv);
        int chan = argc>1 ? atom_getintarg(1, argc, argv) : 1;
        fluid_synth_program_change(x->x_synth, chan - 1, prog - 1);
    }
}

static void fluid_polytouch(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->smmf_mode || x->x_synth == NULL) return;
    if (argc == 2 || argc == 3)
    {
        int val = atom_getintarg(0, argc, argv);
        int key = atom_getintarg(1, argc, argv);
        int chan = argc>2 ? atom_getintarg(2, argc, argv) : 1;
        fluid_synth_key_pressure(x->x_synth, chan - 1, key, val);
    }
}

static void fluid_touch(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->smmf_mode || x->x_synth == NULL) return;
    if (argc == 1 || argc == 2)
    {
        int val = atom_getintarg(0, argc, argv);
        int chan = argc>1 ? atom_getintarg(1, argc, argv) : 1;
        fluid_synth_channel_pressure(x->x_synth, chan - 1, val);
    }
}

static void fluid_bend(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->smmf_mode)
    {
        fluid_legacy_pitch_bend(x, s, argc, argv); return;
    }
    if (x->x_synth == NULL) return;
    if (argc == 1 || argc == 2)
    {
        int val = atom_getintarg(0, argc, argv);
        int chan = argc>1 ? atom_getintarg(1, argc, argv) : 1;
        fluid_synth_pitch_bend(x->x_synth, chan - 1, val);
    }
}

// Maximum size of sysex data (excluding the f0 and f7 bytes) that we can
// handle. The size below should be plenty to handle any kind of MTS message,
// which at the time of this writing is the only kind of sysex message
// recognized by fluidsynth.
#define MAXSYSEXSIZE 1024

static void fluid_sysex(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!x->smmf_mode || x->x_synth == NULL) return;
    if (argc > 0)
    {
        char buf[MAXSYSEXSIZE];
        int len = 0;
        while (len < MAXSYSEXSIZE && len < argc) {
            buf[len] = atom_getintarg(len, argc, argv);
            len++;
        }
        // TODO: In order to handle bulk dump requests in the future, we will
        // have to pick up fluidsynth's response here and output that to a
        // control outlet (which doesn't exist at present).
        fluid_synth_sysex(x->x_synth, buf, len, NULL, NULL, NULL, 0);
    }
}

static void fluid_load(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_synth == NULL)
    {
        pd_error(x, "fluid~: no fluidsynth");
        return;
    }
    if (argc >= 1 && argv->a_type == A_SYMBOL)
    {
        const char* filename = atom_getsymbolarg(0, argc, argv)->s_name;
        const char* ext = strrchr(filename, '.');
        if (ext && !strchr(ext, '/'))
          // extension already supplied, no default extension
          ext = "";
        else
          ext = ".sf2";
        char realdir[MAXPDSTRING], *realname = NULL;
        int fd = canvas_open(x->x_canvas, filename, ext, realdir,
                             &realname, MAXPDSTRING, 0);
        if (fd < 0) {
          pd_error(x, "fluid~: can't find soundfont %s", filename);
          return;
        }
        // Save the current working directory.
        char buf[MAXPDSTRING], *cwd = getcwd(buf, MAXPDSTRING);
        sys_close(fd);
        if (chdir(realdir)) {}
        if (fluid_synth_sfload(x->x_synth, realname, 0) >= 0)
        {
            post("fluid~: loaded soundfont %s", realname);
            fluid_synth_program_reset(x->x_synth);
        }
        // Restore the working directory.
        cwd && chdir(cwd);
    }
}

static void fluid_init(t_fluid_tilde *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_synth) delete_fluid_synth(x->x_synth);
    if (x->x_settings) delete_fluid_settings(x->x_settings);

    float sr = sys_getsr();

    // Some drivers (e.g. ALSA) are very chatty and will print a lot of
    // log messages to the terminal while fluidsynth is being
    // initialized. The only way to get rid of those seems to temporarily
    // redirect stderr, so here goes.
    int saved_stderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_RDWR);
    dup2(devnull, STDERR_FILENO);

    x->x_settings = new_fluid_settings();

    if (x->x_settings == NULL)
    {
        pd_error(x, "fluid~: couldn't create synth settings\n");
    }
    else
    {
        // ag: fluidsynth defaults are: 0.2, 16, 256, 44100.0, 1, 1, 0. The
        // default gain is absurdly low, so we use a larger but still moderate
        // value (range is 0-10). I find 1.0 to be a good default, YMMV.
        // (Original fluid~ had 0.6 here, but I found that too low with most
        // soundfonts.)
        fluid_settings_setnum(x->x_settings, "synth.gain", 1.0);
#if 0
        // These are the original defaults from fluid~ of old, but most of
        // these are the defaults anyway, or weren't working with fluidsynth
        // 2, so lets just get rid of them. We really want to keep things as
        // close to the defaults as possible, so that fluid~ sounds *exactly*
        // like the stand-alone program, in order to not confuse the user.
        fluid_settings_setint(x->x_settings, "synth.midi-channels", 16);
        fluid_settings_setint(x->x_settings, "synth.polyphony", 256);
        fluid_settings_setnum(x->x_settings, "synth.sample-rate", 44100.0);
        fluid_settings_setint(x->x_settings, "synth.chorus.active", 0);
        fluid_settings_setint(x->x_settings, "synth.reverb.active", 0);
        fluid_settings_setint(x->x_settings, "synth.ladspa.active", 0);
#endif

        if (sr != 0)
        {
            fluid_settings_setnum(x->x_settings, "synth.sample-rate", sr);
        }
        // Create fluidsynth instance:
        x->x_synth = new_fluid_synth(x->x_settings);
        if (x->x_synth == NULL )
        {
            pd_error(x, "fluid~: couldn't create synth");
        }
        // check for SMMF mode
        const char* arg = atom_getsymbolarg(0, argc, argv)->s_name;
        if (strcmp(arg, "-smmf") == 0)
        {
            x->smmf_mode = 1; argc--; argv++;
        }
        // try to load argument as soundfont
        fluid_load(x, gensym("load"), argc, argv);

        // We're done constructing:
        if (x->x_synth)
            post("-- fluid~ for Pd%s --", x->smmf_mode?" (SMMF mode)":"");
    }
    // Restore stderr.
    dup2(saved_stderr, STDERR_FILENO);
}

static void *fluid_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_fluid_tilde *x = (t_fluid_tilde *)pd_new(fluid_tilde_class);
    x->x_synth = NULL; x->x_settings = NULL;
    x->x_out_left = outlet_new(&x->x_obj, &s_signal);
    x->x_out_right = outlet_new(&x->x_obj, &s_signal);
    x->smmf_mode = 0;
    x->x_canvas = canvas_getcurrent();
    fluid_init(x, gensym("init"), argc, argv);
    return (void *)x;
}

static void fluid_log_cb(int level, const char *message, void *data)
{
  post("fluid~ [%d]: %s", level, message);
}

void fluid_tilde_setup(void)
{
    fluid_tilde_class = class_new(gensym("fluid~"),
        (t_newmethod)fluid_tilde_new, (t_method)fluid_tilde_free,
        sizeof(t_fluid_tilde),
        CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_init, gensym("init"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_load, gensym("load"),
        A_GIMME, 0);

    // "legacy" methods. These are 100% backwards-compatible, and are all
    // enabled by default. NOTE: When in SMMF mode (-smmf), the "note" and
    // "bend" messages actually invoke the corresponding SMMF methods below,
    // while all other legacy methods still work (in particular, the "note"
    // and "bend" shortcuts are still available).
#if 0
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_note, gensym("note"),
        A_GIMME, 0);
#endif
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_program_change,
        gensym("prog"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_control_change,
        gensym("control"), A_GIMME, 0);
#if 0
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_pitch_bend,
        gensym("bend"), A_GIMME, 0);
#endif
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_bank, gensym("bank"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_gen, gensym("gen"),
        A_GIMME, 0);

    // list input calls fluid_legacy_note(...)
    class_addlist(fluid_tilde_class, (t_method)fluid_legacy_note);

    // some alias shortcuts:
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_note, gensym("n"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_program_change,
        gensym("p"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_control_change,
        gensym("c"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_control_change,
        gensym("cc"), A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_legacy_pitch_bend, gensym("b"),
        A_GIMME, 0);

    // SMMF methods (new interface methods for MIDI, enabled with -smmf)
    // NOTE: When in the default legacy mode, fluid_note and fluid_bend
    // actually invoke the corresponding legacy methods above.
    class_addmethod(fluid_tilde_class, (t_method)fluid_note, gensym("note"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_ctl, gensym("ctl"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_pgm, gensym("pgm"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_polytouch, gensym("polytouch"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_touch, gensym("touch"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_bend, gensym("bend"),
        A_GIMME, 0);
    class_addmethod(fluid_tilde_class, (t_method)fluid_sysex, gensym("sysex"),
        A_GIMME, 0);

    // Simulate Flext's help message
    class_addmethod(fluid_tilde_class, (t_method)fluid_help, gensym("help"),
        0);

    class_addmethod(fluid_tilde_class,
        (t_method)fluid_tilde_dsp, gensym("dsp"), A_CANT, 0);

    // Set up logging. We don't want to have too much noise here, and we also
    // want to see the important stuff in the Pd console rather than the
    // terminal.
    fluid_set_log_function(FLUID_PANIC, fluid_log_cb, NULL);
    fluid_set_log_function(FLUID_ERR, fluid_log_cb, NULL);
    fluid_set_log_function(FLUID_WARN, fluid_log_cb, NULL);
    fluid_set_log_function(FLUID_DBG, NULL, NULL);
}
