#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TRUE  1
#define FALSE 0
#define MAX_ARRAY_SIZE 256
//typedef short boolean;

static t_class *genetic_class;

typedef struct _individual {
	boolean genes[MAX_ARRAY_SIZE];
	float fitness;
	t_int length;
} 	t_individual;

typedef struct _genetic {
  t_object  x_obj;
  float cumulativeprobability;
  t_int individuallength;
  t_individual population[MAX_ARRAY_SIZE];
  t_int populationsize;
  t_individual targetindividual;
  t_atom out[MAX_ARRAY_SIZE];
  t_int chunksize;
  t_outlet *outlist;
} t_genetic;

void randomizeIndividuals(t_genetic *x) {
	int i,k;
	for (k=0;k<x->populationsize;k++) {
	for (i=0;i<x->individuallength;i++) {
	x->population[k].genes[i] = ((t_float) rand())/RAND_MAX*2;	
	}
	x->population[k].length = x->individuallength;
	}
//	x->population[0].genes[0] = 1;
//	x->population[0].genes[1] = 0;
}

void printTarget(t_genetic *x) {
	unsigned int out[MAX_ARRAY_SIZE];
	int i;	
	for (i=0;i< x->targetindividual.length/x->chunksize;i++) {
	fast_b2short(&out[i],&x->targetindividual.genes[i*x->chunksize],x->chunksize);	
	SETFLOAT(&x->out[i],out[i]);
	}
	outlet_list(x->outlist,&s_list, i, &x->out[0]);
}


void randomTargetIndividual(t_genetic *x) {
	int i;
	for (i=0;i<x->individuallength;i++) {
	x->targetindividual.genes[i] = ((t_float) rand())/RAND_MAX*2;	
	}
	x->targetindividual.length = x->individuallength;
	//printTarget(&x->targetindividual,x);
}

/*float evaluateFitness(t_genetic *x,int index) {
	int fitness = 0;
	int i;
//	post("Fitness eval");
	for (i=0;i<x->individuallength;i++) {
	//if (x->targetindividual.genes[i] == x->population[index].genes[i])
	//if (x->population[index].genes[i] == TRUE)
	fitness += x->population[0].genes[0];
	//fitness++;
	}
	return fitness/x->individuallength;
	//return 1;
}
*/
void startReproduction(t_genetic *x) {
	int selectedIndividuals[MAX_ARRAY_SIZE];
	int count = 0;
	int i;
	float r;
	t_individual childs[MAX_ARRAY_SIZE];
	int childindex = 0;
	for (count =0; count < x->populationsize/4; count++) {
		float sum = 0;
		i = 0;
		r = ((t_float) rand())/RAND_MAX * x->cumulativeprobability;
		while (r > sum) {
				sum += x->population[i].fitness;
				i++;
		}
		selectedIndividuals[count] = i;
	}
	post("parents: %d",count);
	i=0;
	while (i<count) {
	int k = 0;
	for (k=0;k<8;k++) {
	int l;
	r = ((t_float) rand())/RAND_MAX * x->individuallength;
	for (l=0;l<x->individuallength;l++) {
	if (l<r) childs[childindex].genes[l] = x->population[selectedIndividuals[i]].genes[l];
	else childs[childindex].genes[l] = x->population[selectedIndividuals[i+1]].genes[l];
	}
	// mutation
	r = ((t_float) rand())/RAND_MAX * x->individuallength;
	childs[childindex].genes[(int)r] = 1-childs[childindex].genes[(int)r];
	childindex++;
	}
	i+=2;
	}
	for (i=0;i<childindex;i++) {
	x->population[i] = childs[i];	
	}
	post("New childs: %d",childindex);
	
}


int evaluatePopulation(t_genetic *x) {
	int i,k;
	int f;
	int highestfitnessindex = 0;
	float highestfitness = 0;
	float mean;
	x->cumulativeprobability = 0;
	for (k=0; k < x->populationsize; k++) {
		float fitness = 0;
		for (i=0;i<x->individuallength;i++) {
		//x->population[k].genes[i] = 0;
		if (x->targetindividual.genes[i] == x->population[k].genes[i])
		fitness++;
//		fitness += x->population[k].genes[i];
		}
		x->population[k].fitness = fitness/x->individuallength;
		x->population[k].fitness = (fitness/x->individuallength)*(fitness/x->individuallength);

		if (x->population[k].fitness > highestfitness) {
		// post("fitness of %d: %f",k,fitness);
		highestfitness = x->population[k].fitness;
		highestfitnessindex = k;
		}
		x->cumulativeprobability+=x->population[k].fitness;
	}
	//recalculate 
	mean = x->cumulativeprobability/x->populationsize;
	x->cumulativeprobability = 0;
	for (k=0;k < x->populationsize; k++) {
		if (x->population[k].fitness < mean)
		x->population[k].fitness = 0;
		else x->cumulativeprobability += x->population[k].fitness;	
	}
	post("Highest fitness: %f",highestfitness);
	return highestfitnessindex;
}


void genetic_setTarget(t_genetic *x,t_symbol *s, int argc, t_atom *argv) {
int i;
post("target called: %d",argc);
	for (i=0;i< x->individuallength/x->chunksize;i++) {
		if (i >= argc) break;
		fast_d2bl(atom_getint(&argv[i]),&x->targetindividual.genes[i*x->chunksize],x->chunksize);

	}
	printTarget(x);
}

void genetic_randomize(t_genetic *x) {
post("Randomize called");	
randomizeIndividuals(x);
}

void genetic_bang(t_genetic *x)
{
	int selected = 1;
	unsigned int out[MAX_ARRAY_SIZE];
	int i;	
	selected = evaluatePopulation(x);
	//post("Selected individual: %d",selected);
	for (i=0;i< x->individuallength/x->chunksize;i++) {
		//out[i] = ((t_float) rand())/RAND_MAX*2;
		fast_b2short(&out[i],&x->population[selected].genes[i*x->chunksize],x->chunksize);
		//fast_b2short8(&out[i],&x->population[selected].genes[i*8]);	
		SETFLOAT(&x->out[i],out[i]);
	} 
	//	fast_b2d(&out,&x->population[0].genes[0]);
//	fast_b2short(&out,&x->population[0].genes[0]);
//	fast_b2d(&out,&test[0]);
//	out = 100;
//	SETFLOAT(&x->out[1],out);
	outlet_list(x->outlist,&s_list, i, &x->out[0]);
	startReproduction(x);
}


/*
 * take argument rule, size, and offset
 */
void *genetic_new(t_float popsize,t_float indsize, t_float chunksize)
{
  t_genetic *x = (t_genetic *)pd_new(genetic_class);
  //inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("list"), gensym("randomize"));
  if ((popsize > 0) && (popsize <= 256))
  x->populationsize = popsize;
  else x->populationsize = 256;
  if ((indsize > 0) && (indsize <= 256))
  x->individuallength = indsize;
  else x->individuallength = 256;
  if ((chunksize <= 16) && (chunksize > 0))
  x->chunksize = (int) chunksize;
  else x->chunksize = 8;
  randomizeIndividuals(x);
  randomTargetIndividual(x);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd,gensym("list"), gensym("target"));
  x->outlist = outlet_new(&x->x_obj,&s_list);
  return (void *)x;
}
