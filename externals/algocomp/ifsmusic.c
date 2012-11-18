#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static t_class *ifsmusic_class;

/*
 * An interated function system music example
 */
typedef struct _ifsmusic {
  t_object x_obj;
  t_int nr_notes;
  t_int nr_functions;
//  t_int melodylength;
  t_float T[16];
  t_float S[16];
  t_float P[16];
  t_int R[16];
  t_int count;
  t_float note[16];
  t_atom notesout[16];
  t_outlet *x_out;
} t_ifsmusic;




void nextSerie(t_ifsmusic *x) {
int i;
if (x->count == x->nr_notes) {	
t_float ra = rand();
t_float r = ra/RAND_MAX;
int k = 0;
t_float noten[16];
x->count = 0;
while (r>x->P[k]) k++;
post("Selected function %d, r=%f",k,r);
for (i=1;i<x->nr_notes;i++) {
x->note[i]  = ((x->note[i] - x->note[0])*x->S[k]) + x->note[0];
}

for (i=0;i<x->nr_notes;i++) {
x->note[i] = x->note[i] + x->T[k];
}
if (x->R[k] == -1) {
for (i=0;i<x->nr_notes;i++) 
noten[x->nr_notes-1-i]=x->note[i];
for (i=0;i<x->nr_notes;i++) 
x->note[i]=noten[i];
}
}
}

void ifsmusic_outputSerie(t_ifsmusic *x)
{
	int i;	
	for (i=0;i< x->nr_notes;i++) {
	//x->notesout[i] = x->note[i];
	SETFLOAT(&x->notesout[i],x->note[i]);
	}
	
outlet_list(x->x_out,&s_list, x->nr_notes, &x->notesout[0]);	
x->count = x->nr_notes;	
nextSerie(x);	
}

void ifsmusic_bang(t_ifsmusic *x)
{
outlet_float(x->x_out,x->note[x->count]);
x->count++;
nextSerie(x);
}

void ifsmusic_setNotes(t_ifsmusic *x,t_symbol *s, int argc, t_atom *argv) {
int i;
x->nr_notes = argc;
post("%d new notes set!",x->nr_notes);
for (i=0;i<x->nr_notes;i++) {
	x->note[i] = atom_getfloat(&argv[i]);
}	
}

void ifsmusic_setFunctions(t_ifsmusic *x,t_symbol *s, int argc, t_atom *argv) {
int i;
float p = 0;
x->nr_functions = argc/4;
post("%d new function set!",x->nr_functions);

for (i=0;i<x->nr_functions;i++) {
	x->S[i] = atom_getfloat(&argv[i*4]);
	x->T[i] = atom_getfloat(&argv[i*4+1]);
//	x->T2[i] = atom_getfloat(&argv[i*5+2]);
	x->P[i] = p+atom_getfloat(&argv[i*4+3]);
	x->R[i] = (t_int) atom_getfloat(&argv[i*4+2]);
	p = x->P[i];
	post("function %d: %f %f %d %f",i,x->S[i],x->T[i],x->R[i],x->P[i]);
}	

if (p != 1.0) {
	float scale = 1.0/p;
	// post("%d new function set. Scale probabilities by %f",x->nr_functions,scale);
	for (i=0;i<x->nr_functions;i++) {
	x->P[i] = scale*x->P[i];	
	// startpost("p for %d: %f; ",i,x->P[i]);
	}
}

}


void ifsmusic_initDummyNoteSequence(t_ifsmusic *x) {
  x->nr_notes = 3;
  x->note[0] = 66;
  x->note[1] = 70;
  x->note[2] = 63;
}

void ifsmusic_initDummyFunctions(t_ifsmusic *x) {
  int i; 
  x->T[0] = 0;
  x->T[1] = 0;
  x->T[2] = 0;
  x->S[0] = 1;
  x->S[1] = 1;
  x->S[2] = 1;
  x->R[0] = 1;
  x->R[1] = 1;
  x->R[2] = 1;  
  x->nr_functions = 3;
  for (i=0;i<x->nr_functions;i++) x->P[i] = (t_float) (i+1)/x->nr_functions;
}
	

void *ifsmusic_new(t_symbol *s, int argc, t_atom *argv)
{
  t_ifsmusic *x = (t_ifsmusic *)pd_new(ifsmusic_class);
  x->count = 0;
  x->x_out = outlet_new(&x->x_obj,&s_float);
  //x->mapped_out = outlet_new(&x->x_obj,&s_float);
  if (argc == 0) ifsmusic_initDummyNoteSequence(x);
  else {
  	int i;
  	x->nr_notes = argc;
  	for (i=0;i<argc;i++) 
  	x->note[i] = atom_getfloat(argv+i);
  }
  ifsmusic_initDummyFunctions(x);
  post("Ifsmusic initialised with sequence length: %d",x->nr_notes);
  return (void *)x;
}
