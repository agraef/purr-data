/*
* Selfsimilar melodies and rhythm.
*/

#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#define TRUE  1
#define FALSE 0
#define MAX_LEVELS 16


typedef short boolean;
static t_class *selfsimilar_class;
static t_class *selfsimilarrhythm_class;
/*
 * Self-similar music
 */
typedef struct _selfsimilar {
  t_object x_obj;
  t_int counter;
  t_int nr_notes;
  t_int transpose;
  t_int note_level[MAX_LEVELS]; // notes for all level 
  t_int melodypos[MAX_LEVELS]; // position in melody for each level
  short poschanged[MAX_LEVELS]; // position changed?
  t_int melody[16]; // melody
  t_int levels; // number of levels (MAX 16)
  t_outlet *note_out;
  t_outlet *note_at_level[MAX_LEVELS];
} t_selfsimilar;


boolean changeposition(int level,t_selfsimilar *x) {
	if (level == 0) {
	x->melodypos[level]++;
	x->poschanged[level]=TRUE;
	}
	else 
	if (changeposition((level-1),x) == TRUE) {
		x->melodypos[level]++;
		x->poschanged[level]=TRUE;
	}
	else {
     	x->poschanged[level]=FALSE;
		return FALSE;
	}
	if (x->melodypos[level] > x->nr_notes-1) {
	x->melodypos[level] = 0;
	x->poschanged[level]=TRUE;
	return TRUE;	
	}
	else {
		//x->poschanged[level]=FALSE;
		return FALSE;
	}
}

t_int createmelody(int level,t_selfsimilar *x,int currentupper) {
	t_int note = 0; // not used!!!
	if (level == 0) {
		note = x->melody[x->melodypos[level]]-x->melody[0];
		x->note_level[level] = note+currentupper;	
	}
	else {
		int current = x->melody[x->melodypos[level]]-x->melody[0]+currentupper;
		int y = createmelody((level-1),x,current+x->transpose);
		x->note_level[level] = current;
		//note = x->melody[x->melodypos[level]]+y-x->melody[0]+x->transpose;
	}
	return note;
}

void selfsimilar_bang(t_selfsimilar *x)
{
	int i = 0;
	t_int note = 0;
	for (i=0;i<x->levels;i++) {
	 	if (x->poschanged[i] == TRUE)
		outlet_float(x->note_at_level[i],x->note_level[i]);
	}
	//post("upper pos: %d",x->melodypos[x->levels-1]);
	changeposition((x->levels-1),x);
	x->note_level[x->levels-1] = x->melody[x->melodypos[x->levels-1]];
	note = createmelody(x->levels-2,x,x->note_level[x->levels-1]+x->transpose);
	note += x->melody[x->melodypos[x->levels-1]];
	//post("notes: %d %d %d",x->note_level[0],x->note_level[1],x->note_level[2]);
	
	//outlet_float(x->note_out,note);

}

void selfsimilar_initDummyNoteSequence(t_selfsimilar *x) {
  x->nr_notes = 4;
/*  x->melody[0] = 1;
  x->melody[1] = 2;
  x->melody[2] = 3;
  x->melody[3] = 4;
  x->melody[4] = 5;
*/
  
  x->melody[0] = 54;
  x->melody[1] = 57;
  x->melody[2] = 52;
  x->melody[3] = 61;
}

void *selfsimilar_new(t_symbol *s, int argc, t_atom *argv)
{
  int i = 0;
  int y;
  t_selfsimilar *x = (t_selfsimilar *)pd_new(selfsimilar_class);
  
  
  if (argc > 2) {
  x->levels = atom_getfloat(&argv[0]); 
  x->transpose = atom_getfloat(&argv[1]); 
  x->nr_notes = argc-2;
  for (i=2;i<argc;i++) 
  	x->melody[i-2] = atom_getfloat(&argv[i]);
  }
  else {
  	selfsimilar_initDummyNoteSequence(x);
    if (argc < 2) {
    x->transpose = 12;
    if (argc == 0) x->levels = 3;
    else x->levels= atom_getfloat(&argv[0]);  }
    else x->transpose = atom_getfloat(&argv[1]);
  } 
  if (x->levels > MAX_LEVELS) x->levels = MAX_LEVELS; // levels between 2 and 10
  if (x->levels < 2) x->levels = 2;
  //x->note_out = outlet_new(&x->x_obj,&s_float);
  for (i=0;i<x->levels;i++) {
  	x->melodypos[i] = 0; // position of each level
  	x->poschanged[i] = TRUE; // notes on each level should start playing
	x->note_at_level[i] = outlet_new(&x->x_obj,&s_float); // connect outlet for each level
	x->note_level[i] = x->melody[0] + x->transpose*(x->levels-i-1);
  }
  return (void *)x;
}

typedef struct _selfsimilarrhythm {
  t_object x_obj;
  t_int counter;
  t_float time_level[MAX_LEVELS]; // times for all level 
  t_int melodypos[10]; // position in melody for each level
  short poschanged[MAX_LEVELS]; // position changed?
  t_float melody[5]; // melody
  t_int levels; // number of levels (MAX 16)
  t_outlet *time_out;
  t_outlet *time_at_level[MAX_LEVELS];
} t_selfsimilarrhythm;


boolean changepositiontime(int level,t_selfsimilarrhythm *x) {
	if (level == 0) {
	x->melodypos[level]++;
	x->poschanged[level]=TRUE;
	}
	else 
	if (changepositiontime((level-1),x) == TRUE) {
		x->melodypos[level]++;
		x->poschanged[level]=TRUE;
	}
	else {
     	x->poschanged[level]=FALSE;
		return FALSE;
	}
	if (x->melodypos[level] > 4) {
	x->melodypos[level] = 0;
	x->poschanged[level]=TRUE;
	return TRUE;	
	}
	else {
		x->poschanged[level]=FALSE;
		return FALSE;
	}
}


t_float createrhytm(int level,t_selfsimilarrhythm *x,int currentupper) {
	t_float time;
	if (level == 0) {
		time = x->melody[x->melodypos[level]];
		x->time_level[level] = time;	
	}
	else {
		float current = x->melody[x->melodypos[level]]*currentupper/1000;
		float y = createrhytm((level-1),x,current);
		x->time_level[level] = current;
		time = x->melody[x->melodypos[level]]*y;
	}
	return time;
}

void selfsimilarrhythm_bang(t_selfsimilarrhythm *x)
{
	int i = 0;
	t_float time = 0;
	time = createrhytm((x->levels-1),x,x->melody[x->melodypos[x->levels-1]]);
	//time += x->melody[x->melodypos[x->levels-1]];
	changepositiontime((x->levels-1),x);
	outlet_float(x->time_out,time);

//	for (i=0;i<x->levels;i++) {
//	 if (x->poschanged[i] == TRUE)
//	 outlet_float(x->time_at_level[i],x->time_level[i]);
//	}
	
}

void *selfsimilarrhythm_new(t_floatarg f1)
{
  int i = 0;
  int y;
  t_selfsimilarrhythm *x = (t_selfsimilarrhythm *)pd_new(selfsimilarrhythm_class);
  x->levels = f1;
  if (x->levels > 16) x->levels = 16; // levels between 2 and 10
  if (x->levels < 2) x->levels = 2;
  x->melody[0] = 1000;
  x->melody[1] = 250;
  x->melody[2] = 250;
  x->melody[3] = 500;
  x->time_out = outlet_new(&x->x_obj,&s_float);
  for (i=0;i<x->levels;i++) {
  	x->melodypos[i] = 0; // position of each level
  	x->poschanged[i] = TRUE; // times on each level should start playing
//	x->time_at_level[i] = outlet_new(&x->x_obj,&s_float); // connect outlet for each level
  }
  return (void *)x;
}
