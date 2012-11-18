#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_LAYERS 3
#define MAX_UNITS 3


static t_class *mlp_class;


typedef struct _mlp {
	t_object  x_obj;
	int nr_inputunits;
	int nr_hiddenunits;
	int nr_outunits;
	int nr_layers;
	t_float weights[2][MAX_UNITS][MAX_UNITS];
	t_float output[3][MAX_UNITS];
	t_float bias[2][MAX_UNITS];
	t_float input[MAX_UNITS];
	t_float target[MAX_UNITS];
	float eta;
	t_outlet *out,*error_out;
} 	t_mlp;


void calculateOutput(t_mlp *x) {
	int i,k,l;
	for (k=0;k<x->nr_hiddenunits;k++) {
			x->output[1][k] = x->bias[0][k];
			for (l=0;l<x->nr_inputunits;l++) {
			x->output[1][k] += x->output[0][l]*x->weights[0][l][k];
			}
		x->output[1][k] = 1.0/(1.0-exp(-x->output[1][k]));
	}
	for (k=0;k<x->nr_outunits;k++) {
			x->output[2][k] = x->bias[1][k];
			for (l=0;l<x->nr_hiddenunits;l++) {
			x->output[2][k] += x->output[1][l]*x->weights[1][l][k];
			}
		x->output[2][k] = 1.0/(1.0-exp(-x->output[2][k]));	
	}
}

void measureError(t_mlp *x) {
//post("Measuring Error");
int i,k,j;
float Error = 0.0;
float SumDOW[MAX_UNITS];
float DeltaH[MAX_UNITS];
float DeltaO[MAX_UNITS];
float DeltaWeightIH[MAX_UNITS][MAX_UNITS];
float DeltaWeightHO[MAX_UNITS][MAX_UNITS];
  
  for(k=0 ;k<x->nr_outunits;k++) {
  	Error +=0.5*(x->target[k]-x->output[2][k])*(x->target[k]-x->output[2][k]);
 	DeltaO[k] = (x->target[k]-x->output[2][k]);//*x->output[2][k] * (1 - x->output[2][k]);
	}
	
  post("Target: %f Error: %f Delta: %f", x->target[0], Error, DeltaO[0]);
  outlet_float(x->error_out, Error);
  for(j=0;j<x->nr_hiddenunits;j++) {         /* 'back-propagate' errors to hidden layer */
      SumDOW[j] = 0.0 ;
      for(k=0 ; k<=x->nr_outunits; k++ ) {
            SumDOW[j] += x->weights[1][j][k]*DeltaO[k] ;
      }
      DeltaH[j] = SumDOW[j] * x->output[1][j]* (1.0 - x->output[1][j]);
  }
   for(j = 0 ;j<x->nr_hiddenunits; j++) {         /* update weights WeightIH */
      //DeltaWeightIH[0][j] = + x->alpha*DeltaWeightIH[0][j];
      x->bias[0][j] += x->eta*DeltaH[j];
      for(i = 0; i < x->nr_inputunits ; i++ ) {
            DeltaWeightIH[i][j] = x->eta * x->output[0][i] * DeltaH[j];// + x->alpha * DeltaWeightIH[i][j];
            x->weights[0][i][j] += DeltaWeightIH[i][j];
      }
	}
	for( k = 0; k < x->nr_outunits; k++) {         /* update weights WeightHO */
      x->bias[1][k] += x->eta*DeltaO[k];
//      DeltaWeightHO[0][k] = x->eta * DeltaO[k] + x->alpha * DeltaWeightHO[0][k] ;
//      x->weights[1][0][k] += DeltaWeightHO[0][k];
      for(j = 0; j < x->nr_hiddenunits; j++ ) {
            DeltaWeightHO[j][k] = x->eta * x->output[1][j] * DeltaO[k];// + x->alpha * DeltaWeightHO[j][k] ;
            x->weights[1][j][k] += DeltaWeightHO[j][k];
      }
}
}


void mlp_bang(t_mlp *x)
{
	calculateOutput(x);
	outlet_float(x->out, x->output[2][0]);
}

void mlp_inputed(t_mlp *x,t_symbol *s, int argc, t_atom *argv) {
int i;
post("inputed called");
for (i=0;i<argc;i++) {
	if (i > x->nr_inputunits) break;
	x->output[0][i] = atom_getfloat(&argv[i]);
}
mlp_bang(x);
}

void mlp_setTarget(t_mlp *x,t_symbol *s, int argc, t_atom *argv) {
int i;
//post("target called: %d",argc);
for (i=0;i<argc-1;i++) {
	if (i > x->nr_inputunits) break;
	x->output[0][i] = atom_getfloat(&argv[i]);
}
post("set targetto: %f",atom_getfloat(&argv[argc-1]));
x->target[0] = atom_getfloat(&argv[argc-1]);
mlp_bang(x);
measureError(x);
}


void initializeWeights(t_mlp *x) {
	int i,k,l;
	for (k=0;k<x->nr_hiddenunits;k++) {
			x->bias[0][k] =  0;//((t_float) rand())/RAND_MAX;
			for (i=0;i<x->nr_inputunits;i++) 
				x->weights[0][i][k] = ((t_float) rand())/RAND_MAX;
			for (l=0;l<x->nr_outunits;l++) 
				x->weights[1][k][l] = ((t_float) rand())/RAND_MAX;
	}
	for (l=0;l<x->nr_outunits;l++) 
		x->bias[1][l] =  0;//((t_float) rand())/RAND_MAX;
	
}


void mlp_train(t_mlp *x) {
post("train called");	
}
	
void *mlp_new(void)
{
  int i;
  t_mlp *x = (t_mlp *)pd_new(mlp_class);
  x->eta = 0.1;
  x->nr_layers = 3;
  x->nr_hiddenunits = 4;
  x->nr_inputunits = 8;
  x->nr_outunits = 1;
  //for (i=0;i<x->nr_inputunits;i++)
  //x->output[0][i] = ((t_float) rand())/RAND_MAX*5;
  initializeWeights(x);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("list"), gensym("target"));
  x->out = outlet_new(&x->x_obj,&s_float);
  x->error_out = outlet_new(&x->x_obj,&s_float);
  return (void *)x;
}	
	
