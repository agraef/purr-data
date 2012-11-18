#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static t_class *oneoverf_class;
static t_class *chaosgame_class;


/* 
 * Implementation of 1/f noise sequences
 */

typedef struct _oneoverf {
  t_object x_obj;
//  t_int nrp;
  t_int nrbits;
  t_float bits;
  t_float output;
  t_float rans[32];
//  t_int count[16];
  t_int counter;
//  t_float avg[16];  
  t_outlet *note_out;//,*r_out,*n_out;
} t_oneoverf;

void oneoverf_bang(t_oneoverf *x)
{
if ((t_int) x->bits != x->nrbits) {
post("Changed number of bits");
x->nrbits = (t_int) x->bits;
x->counter = 0;	
}
int i;
outlet_float(x->note_out, x->output);
//outlet_float(x->r_out, x->nrp);
//outlet_float(x->n_out, x->nrp);
x->output = 0; // ((t_float) rand())/RAND_MAX/x->nrbits;
x->counter++;
for (i=x->nrbits-1;i>=0;i--) {
//  x->avg[i] = ((t_float) (x->avg[i]*(x->count[i]-1)+x->old))/x->count[i];
  
  if (x->counter%((int) pow(2,i)) == 0) {
  if (i == x->nrbits-1) x->counter = 0;
//  x->count[i] = 1;
//  x->avg[i] = 0;
  x->rans[i] = ((t_float) rand())/RAND_MAX/x->nrbits;
  post("Changing value of %d, counter: %d",i,x->counter);
  i = -1; // break
  }
}
for (i=0;i<x->nrbits;i++) {
  x->output += x->rans[i];
}
}

void *oneoverf_new(t_floatarg f1)
{
  t_oneoverf *x = (t_oneoverf *)pd_new(oneoverf_class);
  int i;
  x->bits = f1;
  x->nrbits = (t_int) f1;
  if (f1 < 1) x->nrbits = 1;
  else if (f1 > 32) x->nrbits = 32;
  x->output = 0;
  x->counter = 0;
  for (i=0;i<x->nrbits;i++) {
  x->rans[i] = (((t_float) rand())/RAND_MAX)/x->nrbits;
 // x->count[i] = 1;
//  x->avg[i] = 0;
  x->output += x->rans[i];
  post("Random generators: %d", x->nrbits);
  }
  x->note_out = outlet_new(&x->x_obj,&s_float);
    floatinlet_new(&x->x_obj, &x->bits);  
  //x->r_out = outlet_new(&x->x_obj,&s_float);
  //x->n_out = outlet_new(&x->x_obj,&s_float);
  return (void *)x;
}

/*
 * Implementation of the chaos game!
 */
typedef struct _chaosgame {
  t_object x_obj;
  t_float x,y;
  t_float px[3],py[3];
  t_outlet *x_out,*y_out;
} t_chaosgame;


void chaosgame_bang(t_chaosgame *x)
{	
int i;	
outlet_float(x->x_out, x->x);
outlet_float(x->y_out, x->y);
i = 3*rand()/32765;
x->x = (x->x + x->px[i])/2;
x->y = (x->y + x->py[i])/2;
}

void *chaosgame_new(t_floatarg f1)
{
  t_chaosgame *x = (t_chaosgame *)pd_new(chaosgame_class);
	x->px[0] = -1;
    x->py[0] = -1;
    x->px[1] = 0;
    x->py[1] = 1;
    x->px[2] = 1;
    x->py[2] = -1;

	x->x = rand()/RAND_MAX;
	x->y = rand()/RAND_MAX;
	
	x->x_out = outlet_new(&x->x_obj,&s_float);
  	x->y_out = outlet_new(&x->x_obj,&s_float);
  	return (void *)x;
}
