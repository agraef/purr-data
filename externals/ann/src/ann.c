/* ...this is an externals for comouting Aritficial Neural Networks...
   thikn aboiut this
	
   0201:forum::für::umläute:2001
*/

#include "ann.h"

//#include "ann_som.c"
//#include "ann_mlp.c"
//#include "ann_td.c"

typedef struct ann 
{
  t_object t_ob;
} t_ann;

t_class *ann_class;

/* do a little help thing */

static void ann_help(void)
{
  post("\n\n...this is the ann external "VERSION"..\n");
  post("self-organized maps"
       "\n\tann_som"
       "");
  post("\n(l) forum::für::umläute 2001\n"
       "this software is under the GnuGPL that is provided with these files");
}

void *ann_new(void)
{
  t_ann *x = (t_ann *)pd_new(ann_class);
  return (void *)x;
}

void ann_som_setup(void);
void ann_mlp_setup(void);
void ann_td_setup(void);



/*
  waiting to be released in near future:
  ANN_SOM : self organized maps
  ANN_PERCEPTRON : perceptrons
  ANN_MLP : multilayer perceptrons
  
  waiting to be realeased sometimes
  ANN_RBF : radial basis functions
*/



void ann_setup(void) 
{
  ann_som_setup();
  ann_mlp_setup();
  ann_td_setup();

  
  /* ************************************** */
  
  post("\n\t................................");
  post("\t...artificial neural networks...");
  post("\t..........version "VERSION"..........");
  post("\t....forum::für::umläute 2001....");
  post("\t....send me a 'help' message....");
  post("\t................................\n");
  
  ann_class = class_new(gensym("ann"), ann_new, 0, sizeof(t_ann), 0, 0);
  class_addmethod(ann_class, ann_help, gensym("help"), 0);
}
