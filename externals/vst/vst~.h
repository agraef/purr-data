#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
#endif


typedef struct _dsp_args
{
	float num_samples;
	float sample_rate;
	int num_in;
	int num_out;
	t_float** inbufs;
	t_float** outbufs;
	VSTPlugin *plug;
} t_dsp_args;



typedef struct _vst_tilde
{
     t_object x_obj;     
	 VSTPlugin *plug;
	 t_inlet** audio_inlets;
	t_outlet** audio_outlets;
	int num_audio_inputs;
	int num_audio_outputs;
	t_outlet* control_outlet;
	t_dsp_args *d_args ;
	t_float x_vel;
	 float x_f;
} t_vst_tilde;

t_int *vst_tilde_perform_replace(t_int *w);
t_int *vst_tilde_perform_acc(t_int *w);
static void vst_tilde_dsp(t_vst_tilde *x, t_signal **sp);
static void vst_tilde_free(t_vst_tilde *x);
static void *vst_tilde_new( t_symbol *s, int argc, t_atom *argv);
static void vst_tilde_float(t_vst_tilde* x, t_float n);

 void vst_tilde_setup(void);

  static void vst_tilde_control (t_vst_tilde* x,  t_symbol* ctrl_name,t_float ctrl_value) ;
static void vst_tilde_print (t_vst_tilde* x, t_symbol *s, int ac, t_atom *av );
static void display_parameter(t_vst_tilde* x, int param, bool showparams );
static void vst_tilde_edit (t_vst_tilde* x);
static void vst_tilde_showparams(t_vst_tilde* x);
static void vst_tilde_noshowparams(t_vst_tilde* x);
static void vst_tilde_param (t_vst_tilde* x , t_float pnum , t_float val );
static void vst_tilde_reset (t_vst_tilde* x);
static void vst_tilde_pitchbend (t_vst_tilde* x,  t_float ctrl_value);
 static void vst_tilde_programchange (t_vst_tilde* x,  t_float ctrl_value);
static void vst_tilde_ctrlchange (t_vst_tilde* x,  t_float control ,t_float ctrl_value);
static void vst_tilde_program (t_vst_tilde* x,  t_float ctrl_value) ;

static void vst_tilde_midinote(t_vst_tilde* x , t_float note );

static void *vstnamecanvas_new(t_symbol *s);

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif


