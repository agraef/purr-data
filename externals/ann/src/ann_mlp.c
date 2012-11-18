/*	ann_mlp : Neural Networks for PD
	by Davide Morelli - info@davidemorelli.it - http://www.davidemorelli.it
	this software is simply an interface for FANN classes
	http://fann.sourceforge.net/
	FANN is obviously needed for compilation
	USE 1.2 VERSION ONLY
	this software is licensed under the GNU General Public License
*/

/*
  hacked by Georg Holzmann for some additional methods, bug fixes, ...
  2005, grh@mur.at
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

static t_class *ann_mlp_class;

typedef struct _ann_mlp {
	t_object  x_obj;
	struct fann *ann;
	int mode; // 0 = training, 1 = running
	t_symbol *filename; // name of the file where this ann is saved
	t_symbol *filenametrain; // name of the file with training data
	float desired_error;
	unsigned int max_iterations;
	unsigned int iterations_between_reports;
  fann_type *input;     // grh: storage for input
  t_atom *output;       // grh: storage for output (t_atom)
  fann_type *out_float; // grh: storage for output (fann_type)
  t_canvas *x_canvas;
	t_outlet *l_out, *f_out;
} t_ann_mlp;

// allocation
static void ann_mlp_allocate_storage(t_ann_mlp *x)
{
  unsigned int i;

  if(!x->ann)
    return;
  
  x->input = (fann_type *)getbytes(x->ann->num_input*sizeof(fann_type));
  x->output = (t_atom *)getbytes(x->ann->num_output*sizeof(t_atom));
  x->out_float = (fann_type *)getbytes(x->ann->num_output*sizeof(fann_type));
  
  // init storage with zeros
  for (i=0; i<x->ann->num_input; i++)
    x->input[i]=0;
  for (i=0; i<x->ann->num_output; i++)
  {
    SETFLOAT(x->output+i, 0);
    x->out_float[i]=0;
  }
}

// deallocation
static void ann_mlp_free(t_ann_mlp *x)
{
  if(!x->ann)
    return;
  
  freebytes(x->input, x->ann->num_input * sizeof(fann_type));
  freebytes(x->output, x->ann->num_output * sizeof(t_atom));
  freebytes(x->out_float, x->ann->num_output * sizeof(fann_type));
  fann_destroy(x->ann);
}

static void ann_mlp_help(t_ann_mlp *x)
{
	post("");
	post("ann_mlp: neural nets for PD");
	post("ann_mlp:Davide Morelli - info@davidemorelli.it - (c)2005");
	post("ann_mlp:create or load an ann, train it and run it passing a list with inputs to the inlet, nn will give a list of float as output");
	post("ann_mlp:main commands: create, filename, load, save, train-on-file, run");
	post("ann_mlp:see help-nn.pd for details on commands and usage");
	post("ann_mlp:this is an interface to FANN");

}

static void ann_mlp_createFann(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
	unsigned int num_input = 2;
	unsigned int num_output = 1;
	unsigned int num_layers = 3;
	unsigned int *neurons_per_layer = NULL;
  int activated=0;
  int i, count_args = 0;
	float connection_rate = 1;
	float learning_rate = (float)0.7;
  
  
  // okay, start parsing init args ...
  
	if (argc > count_args++)
		num_input = atom_getint(argv++);

	if (argc > count_args++)
		num_output = atom_getint(argv++);

	if (argc > count_args++)
  {
    int hidden=0;
    
    num_layers = atom_getint(argv++);
    hidden = num_layers-2;
    
    neurons_per_layer = (unsigned int *)getbytes(num_layers*sizeof(unsigned int));
    
    neurons_per_layer[0] = num_input;
    
    // make standard initialization (if there are too few init args)
    for (i=1; i<hidden+1; i++)
      neurons_per_layer[i] = 3;
    
    // now check init args
    for (i=1; i<hidden+1; i++)
    {
      if (argc > count_args++)
        neurons_per_layer[i] = atom_getint(argv++);
    }
    
    neurons_per_layer[num_layers-1] = num_output;
    
    activated=1;
  }
	
  if (argc > count_args++)
		connection_rate = atom_getfloat(argv++);

	if (argc > count_args++)
		learning_rate = atom_getfloat(argv++);

  // make one hidden layer as standard, if there were too few init args
  if(!activated)
  {
    neurons_per_layer = (unsigned int *)getbytes(3*sizeof(unsigned int));
    neurons_per_layer[0] = num_input;
    neurons_per_layer[1] = 3;
    neurons_per_layer[2] = num_output;
  }

  // ... end of parsing init args
  
  
  if(x->ann)
    ann_mlp_free(x);
  
  x->ann = fann_create_array(connection_rate, learning_rate, num_layers, neurons_per_layer);
  
  // deallocate helper array
  freebytes(neurons_per_layer, num_layers * sizeof(unsigned int));
  
	if(!x->ann)
  {
    error("error creating the ann");
    return;
  }
  
  ann_mlp_allocate_storage(x);
	fann_set_activation_function_hidden(x->ann, FANN_SIGMOID_SYMMETRIC);
	fann_set_activation_function_output(x->ann, FANN_SIGMOID_SYMMETRIC);
  
  // set error log to stdout, so that you see it in the pd console
  //fann_set_error_log((struct fann_error*)x->ann, stdout);
  // unfortunately this doesn't work ... but it should do in a similar way !!
  
	post("created ann with:");
	post("num_input = %i", num_input);
	post("num_output = %i", num_output);
	post("num_layers = %i", num_layers);
	post("connection_rate = %f", connection_rate);
	post("learning_rate = %f", learning_rate);
}

static void ann_mlp_print_status(t_ann_mlp *x)
{
		if (x->mode == TRAIN)
			post("nn:training");
		else
			post("nn:running");
}

static void ann_mlp_train(t_ann_mlp *x)
{
	x->mode=TRAIN;
	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}
	fann_reset_MSE(x->ann);
	ann_mlp_print_status(x);
}

static void ann_mlp_run(t_ann_mlp *x)
{
	x->mode=RUN;
	ann_mlp_print_status(x);
}

static void ann_mlp_set_mode(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (argc<1)
	{
		error("usage: setmode 0/1: 0 for training, 1 for running");
	}
	else	
	{	
		x->mode = atom_getint(argv++);
		ann_mlp_print_status(x);
	}
}

static void ann_mlp_train_on_file(t_ann_mlp *x, t_symbol *s)
{
  // make correct path
  char patcher_path[MAXPDSTRING];
  char filename[MAXPDSTRING];

	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}

  // make correct path
  canvas_makefilename(x->x_canvas, s->s_name, patcher_path, MAXPDSTRING);
  sys_bashfilename(patcher_path, filename);
  x->filenametrain = gensym(filename);

  if(!x->filenametrain)
    return;
  
	post("nn: starting training on file %s, please be patient and wait ... (it could take severeal minutes to complete training)", x->filenametrain->s_name);

	fann_train_on_file(x->ann, x->filenametrain->s_name, x->max_iterations,
		x->iterations_between_reports, x->desired_error);
	
	post("ann_mlp: finished training on file %s", x->filenametrain->s_name);
}

static void ann_mlp_set_desired_error(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
	float desired_error = (float)0.001;
	if (0<argc)
	{
		desired_error = atom_getfloat(argv);
		x->desired_error = desired_error;
		post("nn:desired_error set to %f", x->desired_error);
	} else
	{
		error("you must pass me a float");
	}
}

static void ann_mlp_set_max_iterations(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
	unsigned int max_iterations = 500000;
	if (argc>0)
	{
		max_iterations = atom_getint(argv);
		x->max_iterations = max_iterations;
		post("nn:max_iterations set to %i", x->max_iterations);
	} else
	{
		error("you must pass me an int");
	}
}

static void ann_mlp_set_iterations_between_reports(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
	
	unsigned int iterations_between_reports = 1000;
	if (argc>0)
	{
		iterations_between_reports = atom_getint(argv);
		x->iterations_between_reports = iterations_between_reports;
		post("nn:iterations_between_reports set to %i", x->iterations_between_reports);
	} else
	{
		error("you must pass me an int");
	}

}

// run the ann using floats in list passed to the inlet as input values
// and send result to outlet as list of float
static void ann_mlp_run_the_net(t_ann_mlp *x, t_symbol *sl, unsigned int argc, t_atom *argv)
{
	unsigned int i=0;	
	fann_type *calc_out;

	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}
  
  if(argc < x->ann->num_input)
  {
    error("ann_mlp: too few input values!!");
    return;
  }

	// fill input array with actual data sent to inlet
	for (i=0;i<x->ann->num_input;i++)
	{
		x->input[i] = atom_getfloat(argv++);
	}
	
	// run the ann
	calc_out = fann_run(x->ann, x->input);

	// fill the output array with result from ann
	for (i=0;i<x->ann->num_output;i++)
		SETFLOAT(x->output+i, calc_out[i]);

	// send output array to outlet
	outlet_anything(x->l_out, gensym("list"),
                  x->ann->num_output, x->output);
}

static void ann_mlp_train_on_the_fly(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
	int i=0;
  int quantiINs, quantiOUTs;
	float mse;

	if (x->ann == 0)
	{
		error("ann not initialized");
		return;
	}

	quantiINs = x->ann->num_input;
	quantiOUTs = x->ann->num_output;

	if ((quantiINs + quantiOUTs)>argc)
	{
		error("insufficient number of arguments passed, in training mode you must prive me a list with (num_input + num_output) floats");
		return;
	}

	// fill input array with actual data sent to inlet
	for (i=0;i<quantiINs;i++)
		x->input[i] = atom_getfloat(argv++);

	for (i=0;i<quantiOUTs;i++)
		x->out_float[i] = atom_getfloat(argv++);
	
	//fann_reset_MSE(x->ann);

	fann_train(x->ann, x->input, x->out_float);

	mse = fann_get_MSE(x->ann);
	
	outlet_float(x->f_out, mse);
}

static void ann_mlp_manage_list(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
	if (x->mode)
		ann_mlp_run_the_net(x, sl, argc, argv);
	else
	{
		ann_mlp_train_on_the_fly(x, sl, argc, argv);
	}
}

static void ann_mlp_set_filename(t_ann_mlp *x, t_symbol *s)
{
  // make correct path
  char patcher_path[MAXPDSTRING];
  char filename[MAXPDSTRING];
  
  if(!s)
    return;
  
  // make correct path
  canvas_makefilename(x->x_canvas, s->s_name, patcher_path, MAXPDSTRING);
  sys_bashfilename(patcher_path, filename);
  x->filename = gensym(filename);
}

static void ann_mlp_load_ann_from_file(t_ann_mlp *x, t_symbol *s)
{
  ann_mlp_set_filename(x,s);
  
  if(!x->filename)
  {
    error("ann: no filename !!!");
    return;
  }
  
  // deallocate storage
  if(x->ann)
    ann_mlp_free(x);
      
  x->ann = fann_create_from_file(x->filename->s_name);
  
  if (x->ann == 0)
    error("error opening %s", x->filename->s_name);
  else
    post("nn:ann loaded fom file %s", x->filename->s_name);
  
  // allocate storage
  ann_mlp_allocate_storage(x);
}

static void ann_mlp_save_ann_to_file(t_ann_mlp *x, t_symbol *s)
{
  ann_mlp_set_filename(x,s);
	
  if(!x->filename)
  {
    error("ann: no filename !!!");
    return;
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
static void ann_mlp_set_FANN_TRAIN_INCREMENTAL(t_ann_mlp *x)
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
static void ann_mlp_set_FANN_TRAIN_BATCH(t_ann_mlp *x)
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
static void ann_mlp_set_FANN_TRAIN_RPROP(t_ann_mlp *x)
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
static void ann_mlp_set_FANN_TRAIN_QUICKPROP(t_ann_mlp *x)
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

static void ann_mlp_set_activation_function_output(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
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
    if (strcmp(parametro->s_name, "FANN_GAUSSIAN")==0)
      funzione = FANN_GAUSSIAN;
    if (strcmp(parametro->s_name, "FANN_GAUSSIAN_STEPWISE")==0)
      funzione = FANN_GAUSSIAN_STEPWISE;
    if (strcmp(parametro->s_name, "FANN_ELLIOT")==0)
      funzione = FANN_ELLIOT;
    if (strcmp(parametro->s_name, "FANN_ELLIOT_SYMMETRIC")==0)
      funzione = FANN_ELLIOT_SYMMETRIC;
    
		fann_set_activation_function_output(x->ann, funzione);
	} else
	{
		error("you must specify the activation function");
	}
	post("nn:activation function set to %s (%i)", parametro->s_name, funzione);

}

static void ann_mlp_set_activation_function_hidden(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
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
    if (strcmp(parametro->s_name, "FANN_GAUSSIAN")==0)
      funzione = FANN_GAUSSIAN;
    if (strcmp(parametro->s_name, "FANN_GAUSSIAN_STEPWISE")==0)
      funzione = FANN_GAUSSIAN_STEPWISE;
    if (strcmp(parametro->s_name, "FANN_ELLIOT")==0)
      funzione = FANN_ELLIOT;
    if (strcmp(parametro->s_name, "FANN_ELLIOT_SYMMETRIC")==0)
      funzione = FANN_ELLIOT_SYMMETRIC;
    
		fann_set_activation_function_hidden(x->ann, funzione);
	} else
	{
		error("you must specify the activation function");
	}
	post("nn:activation function set to %s (%i)", parametro->s_name, funzione);

}

static void ann_mlp_randomize_weights(t_ann_mlp *x, t_symbol *sl, int argc, t_atom *argv)
{
  t_float min = -1;
  t_float max = 1;

  if(!x->ann)
  {
    post("ann_mlp: ann is not initialized");
    return;
  }
  
  if (argc>0)
		min = atom_getfloat(argv++);

	if (argc>1)
		max = atom_getfloat(argv++);
    
  fann_randomize_weights(x->ann, min, max);
}

static void ann_mlp_learnrate(t_ann_mlp *x, t_float f)
{
  int learnrate = 0;
  
  if(!x->ann)
  {
    post("ann_mlp: ann is not initialized");
    return;
  }
  
  learnrate = (f<0) ? 0 : f;
  fann_set_learning_rate(x->ann, learnrate);
}

static void ann_mlp_set_activation_steepness_hidden(t_ann_mlp *x, t_float f)
{
  if(!x->ann)
  {
    post("ann_mlp: ann is not initialized");
    return;
  }
  
  fann_set_activation_steepness_hidden(x->ann, f);
}

static void ann_mlp_set_activation_steepness_output(t_ann_mlp *x, t_float f)
{
  if(!x->ann)
  {
    post("ann_mlp: ann is not initialized");
    return;
  }
  
  fann_set_activation_steepness_output(x->ann, f);
}

void fann_set_activation_steepness_hidden(struct fann * ann, fann_type steepness);

static void ann_mlp_print_ann_details(t_ann_mlp *x)
{
	if (x->ann == 0)
	{
		post("ann_mlp:ann is not initialized");
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
			post("ann_mlp:filename not set");
		} else
		{
			post("filename=%s", x->filename->s_name);
		}
	}
}

static void ann_mlp_print_ann_print(t_ann_mlp *x)
{
  if(!x->ann)
  {
    post("ann_mlp: ann is not initialized");
    return;
  }
    
  fann_print_connections(x->ann);
  fann_print_parameters(x->ann);
}

static void *ann_mlp_new(t_symbol *s, int argc, t_atom *argv)
{
	t_ann_mlp *x = (t_ann_mlp *)pd_new(ann_mlp_class);
	x->l_out = outlet_new(&x->x_obj, &s_list);
	x->f_out = outlet_new(&x->x_obj, &s_float);

	x->desired_error = (float)0.001;
	x->max_iterations = 500000;
	x->iterations_between_reports = 1000;
	x->mode=RUN;
  x->x_canvas = canvas_getcurrent();
  x->filename = NULL;
  x->filenametrain = NULL;
  x->ann = NULL;
  x->input = NULL;
  x->output = NULL;
  x->out_float = NULL;

	if (argc>0) {
		x->filename = atom_gensym(argv);
		ann_mlp_load_ann_from_file(x, NULL);
	}

	return (void *)x;
}

void ann_mlp_setup(void) {
	post("");
	post("ann_mlp: multilayer perceptron for PD");
	post("version: "VERSION"");
	post("compiled: "__DATE__);
	post("author: Davide Morelli");
	post("contact: info@davidemorelli.it www.davidemorelli.it");

	ann_mlp_class = class_new(gensym("ann_mlp"),
		(t_newmethod)ann_mlp_new,
		(t_method)ann_mlp_free, sizeof(t_ann_mlp),
		CLASS_DEFAULT, A_GIMME, 0);

	// general..
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_help, gensym("help"), 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_createFann, gensym("create"), A_GIMME, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_train, gensym("train"), 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_run, gensym("run"), 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_mode, gensym("setmode"), A_GIMME, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_train_on_file, gensym("train-on-file"), A_DEFSYMBOL, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_manage_list, gensym("data"), A_GIMME, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_filename, gensym("filename"), A_DEFSYMBOL, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_load_ann_from_file, gensym("load"),A_DEFSYMBOL, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_save_ann_to_file, gensym("save"),A_DEFSYMBOL, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_print_ann_details, gensym("details"), 0);
  class_addmethod(ann_mlp_class, (t_method)ann_mlp_print_ann_print, gensym("print"), 0);
	
	// change training parameters
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_desired_error, gensym("desired_error"),A_GIMME, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_max_iterations, gensym("max_iterations"),A_GIMME, 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_iterations_between_reports, gensym("iterations_between_reports"),A_GIMME, 0);
  class_addmethod(ann_mlp_class, (t_method)ann_mlp_learnrate, gensym("learnrate"), A_FLOAT, 0);

	// change training  and activation algorithms
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_FANN_TRAIN_INCREMENTAL, gensym("FANN_TRAIN_INCREMENTAL"), 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_FANN_TRAIN_BATCH, gensym("FANN_TRAIN_BATCH"), 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_FANN_TRAIN_RPROP, gensym("FANN_TRAIN_RPROP"), 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_FANN_TRAIN_QUICKPROP, gensym("FANN_TRAIN_QUICKPROP"), 0);
	class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_activation_function_output, gensym("set_activation_function_output"),A_GIMME, 0);
  class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_activation_function_hidden, gensym("set_activation_function_hidden"),A_GIMME, 0);
  class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_activation_steepness_hidden, gensym("set_activation_steepness_hidden"), A_FLOAT, 0);
  class_addmethod(ann_mlp_class, (t_method)ann_mlp_set_activation_steepness_output, gensym("set_activation_steepness_output"), A_FLOAT, 0);
	
  // initialization:
  class_addmethod(ann_mlp_class, (t_method)ann_mlp_randomize_weights, gensym("randomize_weights"),A_GIMME, 0);
  
	// the most important one: running the ann
	class_addlist(ann_mlp_class, (t_method)ann_mlp_manage_list);


}
