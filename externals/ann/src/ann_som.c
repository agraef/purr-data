/* ann_som :
   part of the ARTIFICIAL NEURAL NETWORK external for PURE DATA
   SELF-ORGANIZED MAP : instar learning-rule

   (l) 0201:forum::für::umläute:2001
   this software is licensed under the GNU General Public License
*/

#include "ann.h"
#include <math.h>
#ifdef NT
#define sqrtf sqrt
#endif

#if 1
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <sys/timeb.h>

#ifdef linux
#include <unistd.h>
#endif
#ifdef NT
#include <io.h>
#endif
#endif

/* ****************************************************************************** */
/* som : save and load messages... */

#define INSTAR  1
#define OUTSTAR 2
#define KOHONEN 0

/* learning-rule
 INSTAR : instar learning-rule
*/

#define TRAIN 0
#define TEST  1


typedef struct _som {
  t_object x_obj;
  t_outlet *left, *right;

  int rule;  /* INSTAR, OUTSTAR, KOHONEN */
  int mode;  /* TRAIN, TEST */

  t_symbol *filename;
  int defaultfilename; /* TRUE if filename is still "default.som" */

  int num_neurX, num_neurY; /* for 2dim-fields */
  int num_neurons; /* num_neurX * num_neurY */
  int num_sensors;

  t_float **weights; /* the neural network (pointer to neuron (neuron is a pointer to an array of weights)) */
  t_float **dist; /* squaredistances between neurons (for neighbourhood) (pointer to neuron (is a pointer to an array of distances))*/

  t_float *workingspace; /* a for comparing data*/

  double lr, lr_factor, lr_bias; /* learning rate: lr(n)=(lr(n-1)*lr_factor; LR=lr(n)+lr_bias */
  double nb, nb_factor, nb_bias; /* neighbourhood */

  /* something for reading/writing to files */
  t_canvas *x_canvas;
  t_symbol *x_dir;

} t_som;

static t_class *som_class;

/* -----------------  private functions -------------------- */

static void som_killsom(t_som *x)
{
  /* kill the weights-field */
  int i=x->num_neurons;

  while (i--) {
    freebytes(x->weights[i], sizeof(x->weights[i]));
    x->weights[i]=0;
  }
  freebytes(x->weights, sizeof(x->weights));
  x->weights = 0;

  /* kill the dist-field */
  i=x->num_neurons;

  while (i--) {
    freebytes(x->dist[i], sizeof(x->dist[i]));
    x->dist[i]=0;
  }
  freebytes(x->dist, sizeof(x->dist));
  x->dist = 0;

  /* kill the working-space */
  freebytes(x->workingspace, sizeof(x->workingspace));
  x->workingspace = 0;
}

static void som_makedist(t_som *x)
{
  int i, j;

  x->dist = (t_float **)getbytes(x->num_neurons * sizeof(t_float *));

  for (i=0; i<x->num_neurons; i++) {
    int X1 = (i%x->num_neurX), Y1 = (i/x->num_neurX);
    x->dist[i]=(t_float *)getbytes(x->num_neurons * sizeof(t_float));

    for (j=0; j<x->num_neurons; j++) {
      int X2 = (j%x->num_neurX), Y2 = (j/x->num_neurX);
      x->dist[i][j] = sqrt((X1-X2)*(X1-X2)+(Y1-Y2)*(Y1-Y2));
    }
  }
}

static int som_whosthewinner(t_som *x, t_float *senses)
{
  t_float min_dist = 0;
  int min_n = x->num_neurons-1;
  t_float *weight = x->weights[min_n];
  int n = x->num_sensors;

  while (n--) {
    t_float f = senses[n] - weight[n];
    min_dist += f*f;
  }

  n=x->num_neurons-1;
  while (n--) {
    int s = x->num_sensors;
  
    t_float dist = 0;
    weight = x->weights[n];

    while (s--) {
      t_float f;
      f = senses[s] - weight[s];
      dist += f*f;
    }
    if (dist<min_dist) {
      min_dist = dist;
      min_n    = n;
    }
  }

  return min_n;
}

static void som_createnewsom(t_som *x, int sens, int nx, int ny)
{ /* create a new SOM */
  int i, j;

  /* clean up the old SOM */
  som_killsom(x);


  /* make new SOM */
  x->num_neurons = nx * ny;
  x->num_neurX = nx;
  x->num_neurY = ny;
  x->num_sensors = sens;

  x->weights = (t_float **)getbytes(x->num_neurons * sizeof(t_float *));
  for (i=0; i<x->num_neurons; i++) {
    x->weights[i]=(t_float *)getbytes(x->num_sensors * sizeof(t_float));

    for (j=0; j<x->num_sensors; j++) x->weights[i][j] = 0;
  }

  /* make new dist */
  som_makedist(x);

  /* make new workingspace */
  x->workingspace = (t_float *)getbytes(x->num_sensors * sizeof(t_float));
  for (i=0; i<x->num_sensors; i++) x->workingspace[i]=0.f;
}

/* ----------------- public functions ---------------------- */

static void som_list(t_som *x, t_symbol *sl, int argc, t_atom *argv)
{ /* present the data */
  int i = x->num_sensors;
  //  t_float *data = (t_float *)getbytes(sizeof(t_float) * i);
  t_float *data = x->workingspace;
  t_float *dummy = data;
  int winner;

  t_float learningrate = x->lr+x->lr_bias, neighbourhood = x->nb+x->nb_bias;

  /* first: extract the data */
  /* check if there is enough input data; fill up with zeros if not; if there's plenty, maybe forget about the rest */
  if ((i = x->num_sensors - argc) > 0) {
    dummy = data + argc;
    while (i--) *dummy++ = 0;
    i = x->num_sensors;
  } else i = x->num_sensors;
  dummy = data;
  /* really get the data */
  while (i--) *dummy++ = atom_getfloat(argv++);

  /* second: get the winning neuron */
  winner = som_whosthewinner(x, data);

  if (x->mode == TRAIN) {
    /* third: learn something */
    /* update all the neurons that are within the neighbourhood */
    i=x->num_neurons;
    switch (x->rule) {
    case OUTSTAR:
      while (i--) {
	t_float dist = x->dist[winner][i];
	if (neighbourhood > dist) {
	  t_float factor = 1 - dist/neighbourhood;
	  t_float *weight=x->weights[i];
	  int s = x->num_sensors;

	  while (s--) weight[s] += learningrate*data[s]*(factor-weight[s]);
	}
      }
      break;
    case INSTAR:
      while (i--) {
	t_float dist = x->dist[winner][i];
	if (neighbourhood > dist) {
	  t_float factor = learningrate * (1 - dist/neighbourhood);
	  t_float *weight=x->weights[i];
	  int s = x->num_sensors;

	  while (s--) weight[s] += (data[s]-weight[s])*factor;
	}
      }
      break;
    default:
      /* KOHONEN rule */
      while (i--) {
	t_float dist = x->dist[winner][i];
	if (neighbourhood > dist) {
	  t_float *weight=x->weights[i];
	  int s = x->num_sensors;

	  while (s--) weight[s] += (data[s]-weight[s])*learningrate;
	}
      }
    }

    /* update learning-rate and neighbourhood */
    x->lr *= x->lr_factor;
    x->nb *= x->nb_factor;
  }

  /* finally: do the output thing */
  /* do the output thing */
  outlet_float(x->x_obj.ob_outlet, winner);

  //  freebytes(data, sizeof(t_float)*x->num_sensors);
}

static void som_bang(t_som *x)
{ /* re-trigger the last output */
  error("som_bang: nothing to do");
}

static void som_init(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* initialize the neuron-weights */
  int i, j;
  t_float f;

  switch (argc) {
  case 0:
  case 1:
    f = (argc)?atom_getfloat(argv):0;
    for (i=0; i<x->num_neurons; i++)
      for (j=0; j<x->num_sensors; j++)
	x->weights[i][j]=f;
    break;
  default:
    if (argc == x->num_sensors) {
      for (i=0; i<x->num_neurons; i++)
	for (j=0; j<x->num_sensors; j++)
	  x->weights[i][j]=atom_getfloat(&argv[j]);
    } else 
      error("som_init: you should pass a list of expected mean-values for each sensor to the SOM");
  }
}

/* centered initialization: 
 * the "first" neuron will be set to all zeros
 * the "middle" neuron will be set to the given data
 * the "last" neuron will be set to teh double of the given data
 */
static void som_cinit(t_som *x, t_symbol *s, int argc, t_atom *argv){
  /* initialize the neuron-weights */
  int i, j;
  t_float f;
  t_float v = 1.0f;
  
  switch (argc) {
  case 0:
  case 1:
    f = (argc)?atom_getfloat(argv):0;
    for (i=0; i<x->num_neurons; i++){
      v=i*2.0/x->num_neurons;
      for (j=0; j<x->num_sensors; j++)
	x->weights[i][j]=f*v;
    }
    break;
  default:
    if (argc == x->num_sensors) {
      for (i=0; i<x->num_neurons; i++){
	v=i*2.0/x->num_neurons;
	for (j=0; j<x->num_sensors; j++)
	  x->weights[i][j]=v*atom_getfloat(&argv[j]);
      }
    } else 
      error("som_init: you should pass a list of expected mean-values for each sensor to the SOM");
  }
}

static void som_rinit(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* initialize the neuron-weights to time-seeded random values*/
  int i, j;
  float m,r;
  struct timeb mytime;

  ftime(&mytime); // get current time
  srand(mytime.time*1000+mytime.millitm); // Seed with time (in millisecs)

  switch (argc) {
  case 0:
    for (i=0; i<x->num_neurons; i++) {
      for (j=0; j<x->num_sensors; j++) {
	r = (float)rand()/RAND_MAX;
	x->weights[i][j]=r;
      }
    }
  case 1:
    m = atom_getfloat(argv);
    for (i=0; i<x->num_neurons; i++) {
      for (j=0; j<x->num_sensors; j++) {
	r = (float)rand()/RAND_MAX*m;
	x->weights[i][j]=r;
      }
    }
    break;
  default:
    if (argc > 1) {
      error("som_rinit: Pass a single float (random value multiplier).");
     }
  }
}

/* dump the weights of the queried neuron to the output */
static void som_dump(t_som *x, t_float nf){
  int n=nf;
  int i=x->num_sensors;
  t_atom*ap=0;
  if (n<0 || n>=x->num_neurons)return;
  ap=(t_atom*)getbytes(sizeof(t_atom)*x->num_sensors);
  while(i--)SETFLOAT(&ap[i], x->weights[n][i]);
  outlet_list(x->x_obj.ob_outlet, &s_list, x->num_sensors, ap);

  freebytes(ap, x->num_sensors*sizeof(t_atom));
}

static void som_makenewsom(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* create a new SOM */
  int sens, nx, ny;


  /* check whether there is sufficient data to create a new SOM */
  if ((argc != 2) && (argc !=3)) {
    error("som_new: wrong number of arguments (only 2 or 3 parameters are allowed)");
    return;
  }

  /* 3 arguments : #sensors #neurX #neurY :: 2D-field of neurons with neurX * neurY  items
     2 arguments : #sensors #neurXY       :: 2D-field of neurons with neurXY* neurXY items

     to create more-dimensional fields, we now have to manually adjust the SOM-file (change the distances...)
     LATER, we might do a function "ann_makedist"
  */

  sens = atom_getfloat(argv);
  if (sens <= 0) {
    error("some_new: number of sensors must be >= 1");
    return;
  }

  if (argc==3) {
    nx = atom_getint(argv+1);
    ny = atom_getint(argv+2);
    if ((nx<=0) || (ny<=0)) {
      error("some_new: number of neurons must be >= 1");
      return;
    }
  } else {
    nx = atom_getint(argv+1);
    if (nx<=0) {
      error("some_new: number of neurons must be >= 1");
      return;
    }
    ny = nx;
  }

  som_createnewsom(x, sens, nx, ny);
}

static void som_train(t_som *x)
{ /* set the mode to TRAIN */
  x->mode = TRAIN;
}
static void som_test(t_som *x)
{ /* set the mode to TEST */
  x->mode = TEST;
}

static void som_rule(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* set the learning rule */
  int rule=-1;

  if (argc>1) {
    error("som_rule: only 1 argument may be specified");
    return;
  }
  if (argc == 0) {
    post("som_rule: you are currently training with the %s rule", (x->rule==INSTAR)?"INSTAR":(x->rule==OUTSTAR)?"OUTSTAR":"KOHONEN");
    return;
  }

  if (argv->a_type==A_FLOAT) rule=atom_getint(argv);
  else if (argv->a_type==A_SYMBOL) {
    char name=*atom_getsymbol(argv)->s_name;
    if (name=='I' || name=='i')  rule=INSTAR;
    else if (name=='O' || name=='O')  rule=OUTSTAR;
    else if (name=='K' || name=='k')  rule=KOHONEN;
  }

  switch (rule) {
  case KOHONEN:
  case INSTAR:
  case OUTSTAR:
    x->rule=rule;
    break;
  default:
    error("som_rule: you specified an invalid rule !");
  }
}


static void som_learn(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* set a new LEARNINGRATE */
  switch (argc) {
  case 3:
    x->lr_bias = atom_getfloat(&argv[2]);
  case 2:
    x->lr_factor = atom_getfloat(&argv[1]);
  case 1:
    x->lr = atom_getfloat(&argv[0]);
    break;
  default:
    error("som_learn: you should pass up to 4 learning-rate parameters");
  }
}
static void som_neighbour(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* set a new NEIGHBOURHOOD */
  switch (argc) {
  case 3:
    x->nb_bias = atom_getfloat(&argv[2]);
  case 2:
    x->nb_factor = atom_getfloat(&argv[1]);
  case 1:
    x->nb = atom_getfloat(&argv[0]);
    break;
  default:
    error("som_neighbour: you should pass up to 4 neighbourhood parameters");
  }
} 

static void som_read(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* read a som-file */

  int fd;
  char filnam[MAXPDSTRING];
  char buf[MAXPDSTRING], *bufptr;

  int neuronsX, neuronsY, sensors, rule=0;
  double lr[3], nb[3];
  t_float dummy;
  char *text=0;
  int i, j;
  t_float *fp;

  FILE *f=0;

  text = (char *)getbytes(MAXPDSTRING*sizeof(char));

  if (argc>0) {
    x->filename = atom_gensym(argv);
    x->defaultfilename = 0;
  }
  if (x->defaultfilename) error("som_read: reading from default file \"%s\"", x->filename->s_name);

  if ((fd = open_via_path(canvas_getdir(x->x_canvas)->s_name,
		  x->filename->s_name, "", buf, &bufptr, MAXPDSTRING, 0)) < 0) {
    error("%s: can't open", x->filename->s_name);
    return;
  }
  else
    close (fd);

 /* open */
  sys_bashfilename(x->filename->s_name, filnam);
  dummy = 0;

  while (f == 0) {
    if (!(f = fopen(filnam, "r"))) {
      error("msgfile_read: unable to open %s", filnam);
      return;
    }

    /* read */

    /* read header */
    if ( (dummy=fscanf(f,"SOM:\n%d",&sensors)) != 1) {
      error("som_read: error reading file\n");
      break;
    }
    if ( (dummy=fscanf(f,"%d",&neuronsX)) != 1) {
      error("som_read: error reading file\n");
      break;
    }
    if ( (dummy=fscanf(f,"%d",&neuronsY)) != 1) {
     error("som_read: error reading file\n");
      break;
    }
    fscanf(f,"%s",text);
    if (!strcmp("INSTAR", text)) rule = INSTAR;
    else if (!strcmp("OUTSTAR", text)) rule = OUTSTAR;
    else if (!strcmp("KOHONEN", text)) rule = KOHONEN;

    for (i=0; i<3; i++)
      if ( (fscanf(f,"%lf",&lr[i])) != 1) {
	error("som_read: error reading file\n");
	break;
      }
    for (i=0; i<3; i++)
      if ( (fscanf(f,"%lf",&nb[i])) != 1) {
	error("som_read: error reading file\n");
	break;
      }

    /* we now have a valid SOM-definition
       let's create a dummy SOM */

    som_createnewsom(x, sensors, neuronsX, neuronsY);
 
    x->rule = rule;

    x->lr=lr[0];
    x->lr_factor=lr[1];
    x->lr_bias=lr[2];

    x->nb=nb[0];
    x->nb_factor=nb[1];
    x->nb_bias=nb[2];

    /* read the weights */

    if ((fscanf(f,"\nweights:\n %f",&dummy)) != 1) {
      break;
    }
    
    i=0;
    while (i<x->num_neurons) {
      j = x->num_sensors;
      fp= x->weights[i];
      while (j--) {
	*fp++=dummy;
	if ((fscanf(f,"%f",&dummy)) != 1) {
	  break;
	}
      }
      j = x->num_sensors;
      i++;
    }

    /* finally read the distances */
    if ((fscanf(f,"\ndists:\n %f",&dummy)) != 1) {
      break;
    }
    
    i=0;
    while (i<x->num_neurons) {
      j = x->num_neurons;
      fp= x->dist[i];
      while (j--) {
	*fp++=dummy;
	if ((fscanf(f,"%f",&dummy)) != 1) {
	  break;
	}
      }
      j = x->num_sensors;
      i++;
    }

  }

  /* close file */
  if (f) fclose(f);


}

static void som_write(t_som *x, t_symbol *s, int argc, t_atom *argv)
{ /* write a som-file */
  char filnam[MAXPDSTRING];
  char buf[MAXPDSTRING];
  char *text=0;
  int textlen;

  FILE *f=0;

  int i;

  if (argc>0) {
    x->filename = atom_gensym(argv);
    x->defaultfilename = 0;
  }
  if (x->defaultfilename) error("som_write: writing to default file \"%s\"", x->filename->s_name);

  canvas_makefilename(x->x_canvas, x->filename->s_name, buf, MAXPDSTRING);
  sys_bashfilename(x->filename->s_name, filnam);

  while (f==0) {
    /* open file */
    if (!(f = fopen(filnam, "w"))) {
      error("msgfile : failed to open %s", filnam);
    } else {

      /* write header information */
      text=(char *)getbytes(sizeof(char)*MAXPDSTRING);
      sprintf(text, "SOM:\n%d %d %d %s\n%.15f %.15f %.15f\n%.15f %.15f %.15f\nweights:\n",
	      x->num_sensors, x->num_neurX, x->num_neurY, (x->rule==INSTAR)?"INSTAR":(x->rule==OUTSTAR)?"OUTSTAR":"KOHONEN",
	      x->lr, x->lr_factor, x->lr_bias,
	      x->nb, x->nb_factor, x->nb_bias);
      textlen = strlen(text);

      if (fwrite(text, textlen*sizeof(char), 1, f) < 1) {
	error("msgfile : failed to write %s", filnam); break;
      }
    
      /* write weights */
      for (i=0; i<x->num_neurons; i++) {
	int j=x->num_sensors;
	t_float *weight = x->weights[i];
	while (j--) {
	  sprintf(text, " %.15f", *weight++);
	  textlen=strlen(text);
	  if (fwrite(text, textlen*sizeof(char), 1, f) < 1) {
	    error("msgfile : failed to write %s", filnam); break;
	  }
	}
	if (fwrite("\n", sizeof(char), 1, f) < 1) {
	  error("msgfile : failed to write %s", filnam); break;
	}
      }

      /* write dists */
      if (fwrite("dists:\n", 7*sizeof(char), 1, f) < 1) {
	error("msgfile : failed to write %s", filnam); break;
      }
      for (i=0; i<x->num_neurons; i++) {
	int j=x->num_neurons;
	t_float *dist = x->dist[i];
	while (j--) {
	  sprintf(text, " %.15f", *dist++);
	  textlen=strlen(text);
	  if (fwrite(text, textlen*sizeof(char), 1, f) < 1) {
	    error("msgfile : failed to write %s", filnam); break;
	  }
	}
	if (fwrite("\n", sizeof(char), 1, f) < 1) {
	  error("msgfile : failed to write %s", filnam); break;
	}
      }   
    }
  }
  /* close file */
  if (f) fclose(f);

  freebytes(text, sizeof(text));
}


static void som_help(t_som *x)
{
  post("\nann_som\t:: self orgranized map");
  post("<f1> <f2> <f3>... <fn>\t: train/test som with data"
       "\nlearn\t\t:... "

       "\nhelp\t\t: show this help");
  post("creation: \"ann_som <som-file>\": <som-file> defines a file to be loeaded as a som");
}


static void som_print(t_som *x)
{
  char c = (x->defaultfilename)?'\0':'\"';
  post("\nann_som\t:: self orgranized map");
  post("rule=%s\tmode=%s", (x->rule==INSTAR)?"INSTAR":(x->rule==OUTSTAR)?"OUTSTAR":"KOHONEN", (x->mode==TEST)?"TEST":"TRAIN");
  post("file = %c%s%c", c, x->filename->s_name,c );
  post("neurons = %d*%d = %d\tsensors=%d", x->num_neurX, x->num_neurY, x->num_neurons, x->num_sensors);
  post("learning-rate : lr=%.15f\tlr_x=%.15f\tlr_o=%.15f", x->lr, x->lr_factor, x->lr_bias);
  post("neighbourhood : nb=%.15f\tnb_x=%.15f\tnb_o=%.15f\n", x->nb, x->nb_factor, x->nb_bias);

}
static void som_free(t_som *x)
{
  som_killsom(x);
}

static void *som_new(t_symbol *s, int argc, t_atom *argv)
{
    t_som *x = (t_som *)pd_new(som_class);

    outlet_new(&x->x_obj, 0);

    x->rule = INSTAR;
    x->mode = TRAIN;

    x->filename = gensym("default.som");
    x->defaultfilename = 1;

    x->num_neurX   = 0;
    x->num_neurY   = 0;
    x->num_neurons = 0;

    x->num_sensors = 0;

    x->weights     = 0;
    x->dist        = 0;

    x->lr          = 1;
    x->lr_factor   = 0.999999999;
    x->lr_bias     = 0;

    x->nb          = 10;
    x->nb_factor   = 0.999999999;
    x->nb_bias     = 0.999999999;

    x->x_canvas = canvas_getcurrent();


    if ((argc==0) || (argv->a_type == A_SYMBOL)) {
      /* load the som-file */
      if (argc != 0) x->defaultfilename = 0;
      som_read(x, s, argc, argv);
    } else {
      /* create a new som */
      som_makenewsom(x, s, argc, argv);
    }

    return (x);
}

static void som_setup(void)
{
  som_class = class_new(gensym("ann_som"), (t_newmethod)som_new,
			    (t_method)som_free, sizeof(t_som), 0, A_GIMME, 0);

  class_addlist(som_class, som_list);
  class_addbang(som_class, som_bang);

  class_addmethod(som_class, (t_method)som_makenewsom, gensym("new"), A_GIMME, 0);
  class_addmethod(som_class, (t_method)som_init, gensym("init"), A_GIMME, 0);
  class_addmethod(som_class, (t_method)som_cinit, gensym("cinit"), A_GIMME, 0);
  class_addmethod(som_class, (t_method)som_rinit, gensym("rinit"), A_GIMME, 0);

  class_addmethod(som_class, (t_method)som_learn, gensym("learn"), A_GIMME, 0);
  class_addmethod(som_class, (t_method)som_neighbour, gensym("neighbour"), A_GIMME, 0);
 
  class_addmethod(som_class, (t_method)som_train, gensym("train"), 0);
  class_addmethod(som_class, (t_method)som_test, gensym("test"), 0);
  class_addmethod(som_class, (t_method)som_rule, gensym("rule"), A_GIMME, 0);

  class_addmethod(som_class, (t_method)som_read, gensym("read"), A_GIMME, 0);
  class_addmethod(som_class, (t_method)som_write, gensym("write"), A_GIMME, 0);

  class_addmethod(som_class, (t_method)som_dump, gensym("dump"), A_FLOAT, 0);

  class_addmethod(som_class, (t_method)som_print, gensym("print"), 0);
  class_addmethod(som_class, (t_method)som_help, gensym("help"), 0);

}

void ann_som_setup(void)
{
  som_setup();
}

