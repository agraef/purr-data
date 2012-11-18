#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TRUE  1
#define FALSE 0
#define MAX_ARRAY_SIZE 256

//Already defined elsewhere ??
//typedef short boolean;

static t_class *eca_class;


typedef struct _eca {
  t_object  x_obj;
  t_int counter;
  boolean cells[256];
  t_int size;
  //t_int offset;
  t_outlet *on_cells;
  //t_outlet *firstcell;
  t_atom oncells[256];
  boolean ruletable[8];
  t_int rule;
  t_float inrule;
} t_eca;

void randomizeCells(t_eca *x) {
	int i;
	for (i=0;i<x->size;i++) {
	x->cells[i] = ((t_float) rand())/RAND_MAX*2;	
	}
}

void eca_activateMiddleCell(t_eca *x) {
	int i;
	post("activate middle cell");
	for (i=0;i<x->size;i++) {
	x->cells[i] = FALSE;	
	}
	x->cells[x->size/2] = TRUE;
}

void calculateCellStates(t_eca *x) {
	
	int i;
	
	boolean newstates[MAX_ARRAY_SIZE];
	newstates[0] = x->ruletable[x->cells[x->size-1]*4+x->cells[0]*2+x->cells[1]];
	newstates[x->size-1] = x->ruletable[x->cells[x->size-2]*4+x->cells[x->size-1]*2+x->cells[0]];
	for (i=1;i<x->size-1;i++) {
	newstates[i] = x->ruletable[x->cells[i-1]*4+x->cells[i]*2+x->cells[i+1]];
	}
	for (i=0;i<x->size;i++) {
	x->cells[i] = newstates[i];
	}
}

void eca_randomize(t_eca *x) {
post("Randomize called");	
randomizeCells(x);
}

void createRuleTable(t_eca *x,unsigned short decimalcode) {
	
	int i;
	post("Rule changed to %d",x->rule);
	//decimalcode to binary
	fast_d2b(decimalcode,x->ruletable);
	//for (i=0;i<8;i++) 
	//post("%d",x->ruletable[i]);
}

void eca_bang(t_eca *x)
{
t_atom *y;
int i;
int k = 0;
if ((t_int) x->inrule != x->rule) {
x->rule = (t_int) x->inrule/1;
createRuleTable(x,(unsigned short) x->rule);
}
//startpost("%d",x->counter);
for (i=0;i<x->size;i++) {
	if (x->cells[i]==TRUE)
	{
		//startpost("& $\\bullet$ ");
		SETFLOAT(&x->oncells[k],i);
//		SETFLOAT(&x->oncells[k],i+x->offset);
//		oncells[k] = y;
		k++;
	}
	//else startpost("& ");
	
	
}
//post("\\tabularnewline");
x->counter++;
//randomizeCells(x);
outlet_list(x->on_cells,&s_list, k, &x->oncells[0]);
calculateCellStates(x); // calculate for next step
//outlet_float(x->firstcell,x->cells[0]);
}



/*
 * take argument rule, size, init (0=random), (1=middlecell active)
 */
void *eca_new(t_floatarg rule,t_floatarg size,t_floatarg init)
{
  
  t_eca *x = (t_eca *)pd_new(eca_class);
  //inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("list"), gensym("randomize"));
  if (size != 0)
  x->size = size;
  else (size = 16);
  //x->offset = offset;
  
  if (init == 0) randomizeCells(x);
  else if (init == 1) eca_activateMiddleCell(x);
  x->rule = rule;	
  x->inrule = rule;
  x->counter = 1;
  createRuleTable(x,(unsigned short) rule);
  x->on_cells = outlet_new(&x->x_obj,&s_list);
  //x->firstcell = outlet_new(&x->x_obj,&s_float);
  floatinlet_new(&x->x_obj, &x->inrule);  
  return (void *)x;
}
