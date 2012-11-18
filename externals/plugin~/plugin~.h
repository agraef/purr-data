#include "ladspa.h"
#include "m_pd.h"

typedef struct
{
    const LADSPA_Descriptor*	type;
    LADSPA_Handle*	instance;

    /* Memory to pass async control data to/from the plugin */
    float*		control_input_values;
    int *               control_input_ports; /* port indexes */
    float*		control_output_values;
    int *               control_output_ports; /* port indexes */

    /* Used for monitoring changes in the values */
    float*		prev_control_output_values;
    int			prev_control_output_values_invalid;

    /* Pointers to signal memory for out-of-place processing */
    float**		outofplace_audio_outputs;
    float**		actual_audio_outputs;	/* real audio outputs for out-of-place */

    unsigned long	num_samples;
    unsigned long	sample_rate;

} Plugin_Tilde_Ladspa;

typedef struct
{
    /* Pd's way of object-oriented programming */
    t_object		x_obj;

    /* Access to LADSPA/VST plugins */
    void*		plugin_library;
    const char*		plugin_library_filename; /* only for diagnostics */
    union {

	Plugin_Tilde_Ladspa	ladspa;

    }			plugin;

    /* Plugin information */
    unsigned		num_audio_inputs;
    unsigned		num_audio_outputs;
    unsigned		num_control_inputs;
    unsigned		num_control_outputs;

    /* Pointers to our Pd in- and outlets */
    t_inlet**		audio_inlets;
    t_outlet**		audio_outlets;
    t_outlet*		control_outlet;

    /* Pd's way of passing parameters to the DSP routine */
    t_int*		dsp_vec;
    unsigned		dsp_vec_length;
    unsigned dsp_active;

} Pd_Plugin_Tilde;

/* Object construction and destruction */
void		plugin_tilde_setup (void);
static void*	plugin_tilde_new (t_symbol* s_name, t_symbol* s_lib_name);
static void	plugin_tilde_free (Pd_Plugin_Tilde* x);

/* DSP callbacks */
static void	plugin_tilde_dsp (Pd_Plugin_Tilde* x, t_signal** sp);
static t_int*	plugin_tilde_perform (t_int* w);

/* Plugin callback for sending control output messages */
void	plugin_tilde_emit_control_output (Pd_Plugin_Tilde* x,
					  const char* name,
					  float new_value,
					  int output_port_index);

/* First inlet message callback for "control" messages */
static void	plugin_tilde_control (Pd_Plugin_Tilde* x,
				      t_symbol* ctrl_name,
				      t_float ctrl_value);

/* First inlet message callback for "control" messages */
static void	plugin_tilde_list (Pd_Plugin_Tilde* x);
static void	plugin_tilde_info (Pd_Plugin_Tilde* x);
static void	plugin_tilde_plug (Pd_Plugin_Tilde* x,  t_symbol* plug_name);
static void	plugin_tilde_active (Pd_Plugin_Tilde* x,  t_float active);
/* First inlet message callback for "reset" messages */
static void	plugin_tilde_reset (Pd_Plugin_Tilde* x);

static unsigned	plugin_tilde_get_parm_number (Pd_Plugin_Tilde* x,
					      const char* str);

/* internal API to wrap the different plug-in interfaces */
static const char*	plugin_tilde_search_plugin (Pd_Plugin_Tilde* x,
						    const char* name);
static int	plugin_tilde_open_plugin (Pd_Plugin_Tilde* x,
					  const char* name,
					  const char* lib_name,
					  unsigned long sample_rate);
static void	plugin_tilde_close_plugin (Pd_Plugin_Tilde* x);
static void	plugin_tilde_apply_plugin (Pd_Plugin_Tilde* x);

static void	plugin_tilde_connect_audio (Pd_Plugin_Tilde* x,
					    float** audio_inputs,
					    float** audio_outputs,
					    unsigned long num_samples);
static void	plugin_tilde_set_control_input_by_name (Pd_Plugin_Tilde* x,
						const char* name,
						float value);
static void	plugin_tilde_set_control_input_by_index (Pd_Plugin_Tilde* x,
						unsigned index_,
						float value);

/* subroutines to wrap the LADSPA interface */
const char*	plugin_tilde_ladspa_search_plugin (Pd_Plugin_Tilde* x,
						   const char* name);
int	plugin_tilde_ladspa_open_plugin (Pd_Plugin_Tilde* x,
					 const char* name,
					 const char* lib_name,
					 unsigned long sample_rate);
void	plugin_tilde_ladspa_close_plugin (Pd_Plugin_Tilde* x);
void	plugin_tilde_ladspa_apply_plugin (Pd_Plugin_Tilde* x);

void	plugin_tilde_ladspa_reset (Pd_Plugin_Tilde* x);

void	plugin_tilde_ladspa_connect_audio (Pd_Plugin_Tilde* x,
					   float** audio_inputs,
					   float** audio_outputs,
					   unsigned long num_samples);
void	plugin_tilde_ladspa_set_control_input_by_name (Pd_Plugin_Tilde* x,
					       const char* name,
					       float value);
void	plugin_tilde_ladspa_set_control_input_by_index (Pd_Plugin_Tilde* x,
					       unsigned index_,
					       float value);
/*float	plugin_tilde_ladspa_get_control_input (Pd_Plugin_Tilde* x,
					       const char* name);*/
/* Control output is handled with plugin_tilde_emit_control_output() callback */

/* Local subroutines */
static void plugin_tilde_ladspa_describe (const char* full_filename,
							    void* plugin_handle,
							    LADSPA_Descriptor_Function descriptor_function,
							    void* user_data);
static void	plugin_tilde_ladspa_search_plugin_callback (const char* full_filename,
							    void* plugin_handle,
							    LADSPA_Descriptor_Function descriptor_function,
							    void* user_data);
static void	plugin_tilde_ladspa_count_ports (Pd_Plugin_Tilde* x);
static void	plugin_tilde_ladspa_connect_control_ports (Pd_Plugin_Tilde* x);

static int	plugin_tilde_ladspa_alloc_outofplace_memory (Pd_Plugin_Tilde* x, unsigned long buflen);
static void	plugin_tilde_ladspa_free_outofplace_memory (Pd_Plugin_Tilde* x);
static int	plugin_tilde_ladspa_alloc_control_memory (Pd_Plugin_Tilde* x);
static void	plugin_tilde_ladspa_free_control_memory (Pd_Plugin_Tilde* x);
