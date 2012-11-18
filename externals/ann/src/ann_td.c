/*	ann_td : Time Delay Neural Networks for PD
	by Davide Morelli - info@davidemorelli.it - http://www.davidemorelli.it
	this software is simply an interface for FANN classes
	http://fann.sourceforge.net/
	FANN is obviously needed for compilation
	use 1.2 version only
	this software is licensed under the GNU General Public License
*/
#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include "fann.h"

#ifndef VERSION 
#define VERSION "0.2"
#endif

#ifndef __DATE__ 
#define __DATE__ ""
#endif

#define TRAIN 0
#define RUN  1

#define MAXINPUT 1024
#define MAXOUTPUT 256

static t_class *ann_td_class;

typedef struct _ann_td {
	t_object  x_obj;
	struct fann *ann;
	int mode; // 0 = training, 1 = running
	t_symbol *filename; // name of the file where this ann is saved
	t_symbol *filenametrain; // name of the file with training data
	float desired_error;
	unsigned int max_iterations;
	unsigned int iterations_between_reports;
	unsigned int frames;
	unsigned int num_input;
	t_float *inputs;
	unsigned int ins_frames_set;
	t_outlet *l_out, *f_out;
} t_ann_td;

static void ann_td_help(t_ann_td *x)
{
	post("");
	post("ann_td:time delay neural networks for PD");
	post("ann_td:Davide Morelli - info@davidemorelli.it - (c)2005");
	post("ann_td:create or load an ann, train it and run it passing a list with inputs to the inlet, nn will give a list of float as output");
	post("ann_td:main commands: create, filename, load, save, train-on-file, run");
	post("ann_td:see help-nn.pd for details on commands and usage");
	post("ann_td:this is an interface to FANN");

}

static void ann_td_deallocate_inputs(t_ann_td *x)
{
	if (x->inputs != 0)
	{
		freebytes(x->inputs, sizeof(x->inputs));
		x->inputs = 0;
	}
}

static void ann_td_allocate_inputs(t_ann_td *x)
{
	unsigned int i;
	ann_td_deallocate_inputs(x);
	// allocate space for inputs array
	x->inputs = (t_float *)getbytes((x->frames) * (x->num_input) * sizeof(t_float));
	for (i=0; i<(x->frames * x->num_input); i++) x->inputs[i]=0.f;
}

static void ann_td_createFann(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	unsigned int num_input = 2;
	unsigned int num_output = 1;
	unsigned int num_layers = 3;
	unsigned int num_neurons_hidden = 3;
	float connection_rate = 1;
	float learning_rate = (float)0.7;
	
	if (argc<3)
	{
		error("you must provide at least num_input, num_output amd frames number");
		return;
	}
	if (argc>0)
		num_input = atom_getint(argv++);

	if (argc>1)
		num_output = atom_getint(argv++);
	
	if (argc>2)
	{
		x->frames = atom_getint(argv++);
		x->ins_frames_set=1;
	}

	if (argc>3)
		num_layers = atom_getint(argv++);

	if (argc>4)
		num_neurons_hidden = atom_getint(argv++);

	if (argc>5)
		connection_rate = atom_getfloat(argv++);

	if (argc>6)
		learning_rate = atom_getfloat(argv++);

	if ((num_input * x->frames)>MAXINPUT)
	{
		error("too many inputs, maximum allowed is %f", MAXINPUT/x->frames);
		return;
	}

	if (num_output>MAXOUTPUT)
	{
		error("too many outputs, maximum allowed is MAXOUTPUT");
		return;
	}

	x->ann	= fann_create(connection_rate, learning_rate, num_layers,
		(num_input*x->frames), num_neurons_hidden, num_output);

	fann_set_activation_function_hidden(x->ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(x->ann, FANN_SIGMOID_SYMMETRIC);

	ann_td_allocate_inputs(x);

	if (x->ann == 0)
	{
		error("error creating the ann");
	} else
	{
		post("ann_td:created ann with:");
		post("num_input = %i", num_input);
		post("num_output = %i", num_output);
		post("frames = %i", x->frames);
		post("num_layers = %i", num_layers);
		post("num_neurons_hidden = %i", num_neurons_hidden);
		post("connection_rate = %f", connection_rate);
		post("learning_rate = %f", learning_rate);
	}
}

static void ann_td_print_status(t_ann_td *x)
{
		if (x->mode == TRAIN)
			post("ann_td:training");
		else
			post("ann_td:running");
}

static void ann_td_train(t_ann_td *x)
{
	x->mode=TRAIN;
	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}
	fann_reset_MSE(x->ann);
	ann_td_print_status(x);
}

static void ann_td_run(t_ann_td *x)
{
	x->mode=RUN;
	ann_td_print_status(x);
}

static void ann_td_set_mode(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (argc<1)
	{
		error("usage: setmode 0/1: 0 for training, 1 for running");
	}
	else	
	{	
		x->mode = atom_getint(argv++);
		ann_td_print_status(x);
	}
}



static void ann_td_train_on_file(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}

	if (argc<1)
	{
		error("you must specify the filename with training data");
		return;
	} else
	{
		x->filenametrain = atom_gensym(argv);
	}

	//post("nn: starting training on file %s, please be patient and wait for my next message (it could take severeal minutes to complete training)", x->filenametrain->s_name);

	fann_train_on_file(x->ann, x->filenametrain->s_name, x->max_iterations,
		x->iterations_between_reports, x->desired_error);
	
	post("ann_td: finished training on file %s", x->filenametrain->s_name);
}

static void ann_td_set_desired_error(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	float desired_error = (float)0.001;
	if (0<argc)
	{
		desired_error = atom_getfloat(argv);
		x->desired_error = desired_error;
		post("ann_td:desired_error set to %f", x->desired_error);
	} else
	{
		error("you must pass me a float");
	}
}

static void ann_td_set_max_iterations(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	unsigned int max_iterations = 500000;
	if (argc>0)
	{
		max_iterations = atom_getint(argv);
		x->max_iterations = max_iterations;
		post("ann_td:max_iterations set to %i", x->max_iterations);
	} else
	{
		error("you must pass me an int");
	}
}

static void ann_td_set_iterations_between_reports(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	
	unsigned int iterations_between_reports = 1000;
	if (argc>0)
	{
		iterations_between_reports = atom_getint(argv);
		x->iterations_between_reports = iterations_between_reports;
		post("ann_td:iterations_between_reports set to %i", x->iterations_between_reports);
	} else
	{
		error("you must pass me an int");
	}

}


static void ann_td_scale_inputs(t_ann_td *x)
{
	unsigned int j;
	unsigned int k;

	for(j = (x->frames - 1); j>0; j--)
	{
		// scorro la lista all'indietro
		for (k=0; k < x->num_input; k++)
		{
			// scalo i valori dei frames
			x->inputs[(x->num_input) * j + k]=x->inputs[(x->num_input) * (j-1) + k];
		}
	}
}

// run the ann using floats in list passed to the inlet as input values
// and send result to outlet as list of float
static void ann_td_run_the_net(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	int i=0;
	unsigned j=0;
	//fann_type input[MAXINPUT];	
	fann_type *calc_out;
	t_atom lista[MAXOUTPUT];
	int quanti;
	float valoreTMP;

	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}

	if (x->ins_frames_set==0)
	{
		error("num_inputs and frames not set");
		return;
	}

	if (argc < (int) x->num_input)
	{
		error("insufficient inputs");
		return;
	}
	quanti = x->ann->num_output;

	ann_td_scale_inputs(x);

	// fill output array with zeros
	for (i=0; i<MAXOUTPUT; i++)
	{
		SETFLOAT(lista + i,0);
	}

	// fill input array with actual data sent to inlet
	for (j=0; j < x->num_input ;j++)
	{
		//input[j] = atom_getfloat(argv++);
		x->inputs[j] = atom_getfloat(argv++);
	}
	
	// run the ann
	//calc_out = fann_run(x->ann, input);
	calc_out = fann_run(x->ann, x->inputs);

	// fill the output array with result from ann
	for (i=0;i<quanti;i++)
	{
		valoreTMP = calc_out[i];
		//post("calc_out[%i]=%f", i, calc_out[i]);
		SETFLOAT(lista+i, valoreTMP);
	}

	// send output array to outlet
	outlet_anything(x->l_out,
                     gensym("list") ,
					 quanti, 
					 lista);
	
}

static void ann_td_train_on_the_fly(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	int i=0;
	unsigned int j=0;
	fann_type input_merged[MAXINPUT];	
	fann_type output[MAXOUTPUT];
	//fann_type *calcMSE;
	//t_atom lista[MAXOUTPUT];
	float mse;

	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}

	if ((x->num_input + x->ann->num_output) > (unsigned int) argc)
	{
		error("insufficient number of arguments passed, in training mode you must prive me a list with (num_input + num_output) floats");
		return;
	}

	// fill input array with zeros
	for (i=0; i<MAXINPUT; i++)
	{
		input_merged[i]=0;
	}
	// fill input array with zeros
	for (i=0; i<MAXOUTPUT; i++)
	{
		output[i]=0;
	}

	ann_td_scale_inputs(x);

	// fill input array with actual data sent to inlet
	for (j = 0; j < x->num_input; j++)
	{
		input_merged[j] = atom_getfloat(argv++);
	}
	for (j = x->num_input; j < (x->num_input * x->frames); j++)
	{
		input_merged[j] = x->inputs[j];
	}

	for (j = 0; j < (x->ann->num_output);j++)
	{
		output[j] = atom_getfloat(argv++);
	}
	
	//fann_reset_MSE(x->ann);

	fann_train(x->ann, input_merged, output);

	mse = fann_get_MSE(x->ann);
	
	outlet_float(x->f_out, mse);


}

static void ann_td_manage_list(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (x->mode)
		ann_td_run_the_net(x, sl, argc, argv);
	else
	{
		ann_td_train_on_the_fly(x, sl, argc, argv);
	}
}

static void ann_td_set_filename(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (argc>0) {
    x->filename = atom_gensym(argv);
	} else
	{
		error("you must specify the filename");
	}
	post("nn:filename set to %s", x->filename->s_name);
}

static void ann_td_load_ann_from_file(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (x->ins_frames_set==0)
	{
		error("set num_input and frames with [inputs_frames int int(");
		error("I won't load without num_input and frames set");
		return;
	}
	if (argc>0) {
    x->filename = atom_gensym(argv);
	}
	x->ann = fann_create_from_file(x->filename->s_name);
	if (x->ann == 0)
		error("error opening %s", x->filename->s_name);
	else
		post("nn:ann loaded fom file %s", x->filename->s_name);
	
	ann_td_allocate_inputs(x);
}

static void ann_td_save_ann_to_file(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (argc>0) {
    x->filename = atom_gensym(argv);
	}
	if (x->ann == 0)
	{
		error("ann is not initialized");
	} else
	{
		fann_save(x->ann, x->filename->s_name);
		post("nn:ann saved in file %s", x->filename->s_name);
	}
}

// functions for training algo:
static void ann_td_set_FANN_TRAIN_INCREMENTAL(t_ann_td *x)
{
	if (x->ann == 0)
	{
		error("ann is not initialized");
	} else
	{
		fann_set_training_algorithm(x->ann, FANN_TRAIN_INCREMENTAL);
		post("nn:training algorithm set to FANN_TRAIN_INCREMENTAL");
	}
}
static void ann_td_set_FANN_TRAIN_BATCH(t_ann_td *x)
{
	if (x->ann == 0)
	{
		error("ann is not initialized");
	} else
	{
		fann_set_training_algorithm(x->ann, FANN_TRAIN_BATCH);
		post("nn:training algorithm set to FANN_TRAIN_BATCH");
	}
}
static void ann_td_set_FANN_TRAIN_RPROP(t_ann_td *x)
{
	if (x->ann == 0)
	{
		error("ann is not initialized");
	} else
	{
		fann_set_training_algorithm(x->ann, FANN_TRAIN_RPROP);
		post("nn:training algorithm set to FANN_TRAIN_RPROP");
	}
}
static void ann_td_set_FANN_TRAIN_QUICKPROP(t_ann_td *x)
{
	if (x->ann == 0)
	{
		error("ann is not initialized");
	} else
	{
		fann_set_training_algorithm(x->ann, FANN_TRAIN_QUICKPROP);
		post("nn:training algorithm set to FANN_TRAIN_QUICKPROP");
	}
}

static void ann_td_set_activation_function_output(t_ann_td *x, t_symbol *sl, int argc, t_atom *argv)
{
	t_symbol *parametro = 0;
	int funzione = 0;

	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}

	if (argc>0) {
		parametro = atom_gensym(argv);
		if (strcmp(parametro->s_name, "FANN_THRESHOLD")==0)
			funzione = FANN_THRESHOLD;
		if (strcmp(parametro->s_name, "FANN_THRESHOLD_SYMMETRIC")==0)
			funzione = FANN_THRESHOLD_SYMMETRIC;
		if (strcmp(parametro->s_name, "FANN_LINEAR")==0)
			funzione = FANN_LINEAR;
		if (strcmp(parametro->s_name, "FANN_SIGMOID")==0)
			funzione = FANN_SIGMOID;
		if (strcmp(parametro->s_name, "FANN_SIGMOID_STEPWISE")==0)
			funzione = FANN_SIGMOID_STEPWISE;
		if (strcmp(parametro->s_name, "FANN_SIGMOID_SYMMETRIC")==0)
			funzione = FANN_SIGMOID_SYMMETRIC;
		if (strcmp(parametro->s_name, "FANN_SIGMOID_SYMMETRIC_STEPWISE")==0)
			funzione = FANN_SIGMOID_SYMMETRIC_STEPWISE;
		fann_set_activation_function_output(x->ann, funzione);
	} else
	{
		error("you must specify the activation function");
	}
	post("nn:activation function set to %s (%i)", parametro->s_name, funzione);

}

static void ann_td_print_ann_details(t_ann_td *x)
{
	if (x->ann == 0)
	{
		post("ann_td:ann is not initialized");
	} else
	{
		post("follows a description of the current ann:");
		post("num_input=%i", x->ann->num_input);
		post("num_output=%i", x->ann->num_output);
		post("learning_rate=%f", x->ann->learning_rate);
		post("connection_rate=%f", x->ann->connection_rate);
		post("total_neurons=%i", x->ann->total_neurons);
		post("total_connections=%i", x->ann->total_connections);
		post("last error=%i", x->ann->errstr);
		if (x->filename == 0)
		{
			post("filename not set");
		} else
		{
			post("filename=%s", x->filename->s_name);
		}
	}
}

static void ann_td_set_num_input_frames(t_ann_td *x, t_floatarg ins, t_floatarg frames)
{
	x->num_input = ins;
	x->frames = frames;
	x->ins_frames_set=1;
}

static void *ann_td_new(t_symbol *s, int argc, t_atom *argv)
{
	t_ann_td *x = (t_ann_td *)pd_new(ann_td_class);
	x->l_out = outlet_new(&x->x_obj, &s_list);
	x->f_out = outlet_new(&x->x_obj, &s_float);

	x->desired_error = (float)0.001;
	x->max_iterations = 500000;
	x->iterations_between_reports = 1000;
	x->mode=RUN;
	x->ins_frames_set=0;

	if (argc<2)
	{
		error("2 arguments needed: num_input and frames. filename optional");
		return (void *)x;
	}

	if (argc>0) {
		x->num_input = atom_getint(argv++);
	}

	if (argc>1) {
		x->frames = atom_getint(argv++);
		x->ins_frames_set=1;
		ann_td_allocate_inputs(x);
	}

	if (argc>2) {
		x->filename = atom_gensym(argv);
		ann_td_load_ann_from_file(x, NULL , 0, NULL);
	}

	return (void *)x;
}

// free resources
static void ann_td_free(t_ann_td *x)
{
  struct fann *ann = x->ann;
  fann_destroy(ann);
  ann_td_deallocate_inputs(x);
  // TODO: free other resources!
}

void ann_td_setup(void) {
	post("");
	post("ann_td: time delay neural nets for PD");
	post("version: "VERSION"");
	post("compiled: "__DATE__);
	post("author: Davide Morelli");
	post("contact: info@davidemorelli.it www.davidemorelli.it");

	ann_td_class = class_new(gensym("ann_td"),
		(t_newmethod)ann_td_new,
		(t_method)ann_td_free, sizeof(t_ann_td),
		CLASS_DEFAULT, A_GIMME, 0);

	// general..
	class_addmethod(ann_td_class, (t_method)ann_td_help, gensym("help"), 0);
	class_addmethod(ann_td_class, (t_method)ann_td_createFann, gensym("create"), A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_train, gensym("train"), 0);
	class_addmethod(ann_td_class, (t_method)ann_td_run, gensym("run"), 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_mode, gensym("setmode"), A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_train_on_file, gensym("train-on-file"), A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_manage_list, gensym("data"), A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_filename, gensym("filename"), A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_load_ann_from_file, gensym("load"),A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_save_ann_to_file, gensym("save"),A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_print_ann_details, gensym("details"), 0);
	
	// change training parameters
	class_addmethod(ann_td_class, (t_method)ann_td_set_desired_error, gensym("desired_error"),A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_max_iterations, gensym("max_iterations"),A_GIMME, 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_iterations_between_reports, gensym("iterations_between_reports"),A_GIMME, 0);

	// change training  and activation algorithms
	class_addmethod(ann_td_class, (t_method)ann_td_set_FANN_TRAIN_INCREMENTAL, gensym("FANN_TRAIN_INCREMENTAL"), 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_FANN_TRAIN_BATCH, gensym("FANN_TRAIN_BATCH"), 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_FANN_TRAIN_RPROP, gensym("FANN_TRAIN_RPROP"), 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_FANN_TRAIN_QUICKPROP, gensym("FANN_TRAIN_QUICKPROP"), 0);
	class_addmethod(ann_td_class, (t_method)ann_td_set_activation_function_output, gensym("set_activation_function_output"),A_GIMME, 0);
	
	class_addmethod(ann_td_class, (t_method)ann_td_set_num_input_frames, gensym("inputs_frames"),A_DEFFLOAT, A_DEFFLOAT, 0);
	
	// the most important one: running the ann
	class_addlist(ann_td_class, (t_method)ann_td_manage_list);


}
