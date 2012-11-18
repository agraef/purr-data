#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static t_class *ifs_class;


/*
 * An interated function system music example
 * a more general approach is needed.
 */
typedef struct _ifs {
  t_object  x_obj;
  t_int nr_functions;
  t_float R1,R2;
  t_float T1[6],T2[6],S1[6],S2[6];
  t_float A[6], B[6];
  t_float C[6], D[6], P[6];
  t_float x,y;
  t_outlet *x_out,*y_out, *i_out;
} t_ifs;


void ifs_bang(t_ifs *x)
{
float r = ((float) rand())/RAND_MAX;
int i = 0;
while (r > x->P[i])
i++;
//post("Applying function nr: %d",i);
//x->x = x->A[i]*x->x + x->B[i]*x->y + x->T1[i];
//x->y = x->C[i]*x->x + x->D[i]*x->y + x->T2[i];
x->x = x->S1[i]*x->x + x->T1[i];
x->y = x->S2[i]*x->y + x->T2[i];
outlet_float(x->x_out,x->x);
outlet_float(x->y_out,x->y);
//outlet_float(x->i_out,r);
}

void ifs_setFunctions(t_ifs *x,t_symbol *s, int argc, t_atom *argv) {
int i;
float p = 0;
x->nr_functions = argc/5;
post("%d new function set!",x->nr_functions);

for (i=0;i<x->nr_functions;i++) {
	x->S1[i] = atom_getfloat(&argv[i*5]);
	x->S2[i] = atom_getfloat(&argv[i*5+1]);
	x->T1[i] = atom_getfloat(&argv[i*5+2]);
	x->T2[i] = atom_getfloat(&argv[i*5+3]);
	x->P[i] = p+atom_getfloat(&argv[i*5+4]);
	p = x->P[i];
}	

if (p != 1.0) {
	float scale = 1.0/p;
	post("%d new function set. Scale probabilities by %f",x->nr_functions,scale);
	for (i=0;i<x->nr_functions;i++) {
	x->P[i] = scale*x->P[i];	
	startpost("p for %d: %f; ",i,x->P[i]);
	}
}

}

void *ifs_new(t_floatarg f1)
{
  t_ifs *x = (t_ifs *)pd_new(ifs_class);
  t_float p;
  t_float R1[6],R2[6],S1[6],S2[6];
  t_int i; 
  x->nr_functions = 3;
  p = 1.0/x->nr_functions;
  //R1[0]=R1[1]=R1[2]=R1[3]=R1[4]=R1[5]= 1;
  //R2[0]=R2[1]=R2[2]=R2[3]=R2[4]=R2[5]= 0;
  //S1[0]=S1[1]=S1[2]=S1[3]=S1[4]=S1[5]= 0.5;//0.33333;
  //S2[0]=S2[1]=S2[2]=S2[3]=S2[4]=S2[5]= 1;//0.33333;
  
  x->S1[0]=0.333;
  x->S2[0]=0.333;
  x->S1[1]=0.667;
  x->S2[1]=0.333;
  x->S1[2]=0.333;
  x->S2[2]=0.333;
  x->T1[0] = 0;
  x->T2[0] = 0;
  x->T1[1] = 0.333;
  x->T2[1] = 0.333;
  x->T1[2] = 0;
  x->T2[2] = 0.667;

  for (i = 0;i<x->nr_functions;i++) {
  x->P[i] = p*(i+1);
  post("prob %d: %f",i,x->P[i]);
  }
  x->x = 0;
  x->y = 0;
  x->x_out = outlet_new(&x->x_obj,&s_float);
  x->y_out = outlet_new(&x->x_obj,&s_float);
  //x->i_out = outlet_new(&x->x_obj,&s_float); 
  post("ifs initialized");
  return (void *)x;
}

