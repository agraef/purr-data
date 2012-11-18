/* 
chords_memory:
an external that using graph
learns the played style 
and reproduces chords sequences 
see
http://en.wikipedia.org/wiki/Graph_%28mathematics%29
for an idea of what graphs are (but we'll use weights also)

idea:

------- NODES

each node of the graph is a chord
each node has a name like "Imin Imaj ... IImin IImaj .... etc"
We'll cover each possible grade and form, each grade can be
maj
min
dim
aug
maj7maj
maj7min
min7maj
min7min
dim7dim
dim7min
dim7maj
aug7min
aug7maj
(tot 13 forms)

pland adding more chord types here:



we have 12 grades:
I
I# 
II 
II# 
III 
IV 
IV# 
V 
V# 
VI 
VI# 
VII

for a total of 12x13=156 nodes

------- ARCS

each node is connected to any other node
each node has a weight
the weight is augmented each time the human plays this chord sequence
so probably I V will have high weight
and I VI#7 will be very light

this will be a table of 156x156 int (24336 elements)

what can i do with the graph?

i can do questions like:

simple questions like: 
starting from IIImin tell me a chord so i have a high weight (no novelty)
starting from IIImin tell me a chord so I have hight novelty (low weight)
(the graph simply must select the arc starting from IIImin with the desired weight)

i can build walks giving starting chord:
as before but more than 1 passage

i can build walks giving target chord:
- "build a walk from current chord to target chord with high novelty"
- "build the shortest walk from current chord to target chord with novelty less than ..."
- "build a 4 chords long walk from here to there with ... average novelty"
all these questions are usefull if there is a "form manager" that decides
the piece structure,for example it wants the 2nd theme to start in 4 measures 
and asks for a chord sequence with medium novelty ending on the dominant (V),
when the piece is ending it can ask for a coming back to the first tone..

once we have all the arcs building walks is simply a matter of apply search methods
like http://en.wikipedia.org/wiki/A%2A_search_algorithm
there are plenty of such algos, we must just copy them down.


*/
#include <stdlib.h>
#include <math.h>
// for random, we may want to use it when building walks
#include <time.h> 
// for file io
#include <stdio.h>
// for string manipulation
#include <string.h>
#include <ctype.h>

#include "m_pd.h"
#include "common.h"


#define DEBUG 1 // messaggi di debug
#define DEBUG_VERBOSE 0 // messaggi di debug

// is this system Little Endian? (thanks to Mathieu Bouchard)
// TODO: use this code in file writing/reading
// instead of forcing little endian!
//static inline int is_le() {int x=1; return ((char *)&x)[0];}

// ------------------------------- declaration of used types 

static t_class *chords_memory_class;


// data type for the steps of a walk
typedef struct _step
{
	// 1 if i must modulate to tonality before computing chord
	int modulate;
	// this chord
	chord_t chord;
	// new tonality to be applied before chord
	int tonality_note;
	int tonality_mode;
} step_t;

// struct defining this external's memory space
typedef struct _chords_memory
{
    t_object x_obj; // myself
	t_outlet *x_outchordname;         /* chord name, e.g. "Cmajor7" */
	t_outlet *x_outtonalityname;      /* chord name, e.g. "Cmajor7" */
	t_outlet *x_outchordssequence;    /* sequence of chords,a walk */
	t_outlet *x_outnovelty; /* the degree of novelty of last added chord */
	// the matrix : arcs of the graph
	// each tonality mode has his matrix
	// each matrix is in this form:
	// from which chord to which chord
	short int arcs[MODES_NUM][NODES_NUM][NODES_NUM];
	// modulations matrix
	// same as above
	// but for modulations
	short int modulations[MODES_NUM][NODES_NUM][MODULATIONS_NUM];
	// I use this to normalize weights 0-1
	short int maxweight[MODES_NUM];
	// to convert from absolute tones (C, D, E, ..) to relative ones (I, II, III) and back
	abs_note_t fundamental_note; // describes current tonality
	modes_t fundamental_mode; // describes current tonality
	// to save and load my status
	t_symbol *filename;
	// to normalize weights to 0-1
	chord_t last_chord;
	int last_chord_set;
	// to use for walks
	step_t *walk;
	int steps;
	int using_walk;
	int current_step;

} t_chords_memory;

//static inline int mod(int a, int b) {int c=a%b; c+=b&-(c&&(a<0)^(b<0)); return c;}

// bring this number in 0 12 range
int clean_note(int src)
{
	while (src>12)
		src-=12;
	while (src<0)
		src+=12;
	return src;
}
// or use this one:
#define MOD(x,y) (((x%y)+y)%y)


// ------------------------------- init functions

// initializes the graphs to 0 (everything is new)
void chords_memory_init_graph(t_chords_memory *x)
{
	int i, j, m;
	for (m=0; m<MODES_NUM; m++)
	{
		x->maxweight[m]=1;
		for (i=0; i<NODES_NUM; i++)
		{
			for (j=0; j<NODES_NUM; j++)
			{
				// initially every chord sequence is a novelty
				x->arcs[m][i][j]=0;
			}
		}
	}
	x->last_chord_set=0;
}

void chords_memory_init(t_chords_memory *x, t_floatarg f)
{
	chords_memory_init_graph(x);
}


// ------------- function for string manipulation (from string to chords)

// builds a string for this chord
// the string is in maxlib's chord format
void chords_memory_chord2string(t_chords_memory *x, char *string, chord_t chord)
{
	abs_note_t newnote;
	memset( string, '\0', sizeof(string) );
	newnote = clean_note(chord.note + x->fundamental_note);
	switch (newnote)
	{
		case C:	strcat(string, "C "); 			  break;
		case Db:	strcat(string, "Db "); 			  break;
		case D:	strcat(string, "D "); 			  break;
		case Eb:	strcat(string, "Eb "); 			  break;
		case E:	strcat(string, "E "); 			  break;
		case F:	strcat(string, "F "); 			  break;
		case Gb:	strcat(string, "Gb "); 			  break;
		case G:	strcat(string, "G "); 			  break;
		case Ab:	strcat(string, "Ab "); 			  break;
		case A:	strcat(string, "A "); 			  break;
		case Bb:	strcat(string, "Bb "); 			  break;
		case B:	strcat(string, "B "); 			  break;
	}
	switch (chord.mode)
	{
		case kUnison:	strcat(string, "unison"); 			  break;
		case kMaj:		strcat(string, "major"); 			  break;
		case kMin:		strcat(string, "minor"); 			  break;
		case kDim:		strcat(string, "diminished"); 		  break;
		case kAug:		strcat(string, "augmented"); 		  break;

		case kMaj7:		strcat(string, "major 7th"); 		  break;
		case kDom7:		strcat(string, "dominant 7th"); 		  break;
		case kMin7: 	strcat(string, "minor 7th"); 		  break;
		case kHalfDim7:	strcat(string, "half diminished 7th"); break;
		case kDim7:		strcat(string, "diminished 7th");	  break;
		case kMinMaj7:	strcat(string, "minor/major 7th");	  break;

		case kMaj7s5:	strcat(string, "major 7th #5");		  break;
		case kMaj7b5:	strcat(string, "major 7th b5");		  break;
		case kDom7s5:	strcat(string, "dominant 7th #5"); 	  break;
		case kDom7b5:	strcat(string, "dominant 7th b5"); 	  break;
		case kDomb9:	strcat(string, "dominant b9");		  break;

		case kMaj9:		strcat(string, "major 9th");			  break;
		case kDom9:		strcat(string, "dominant 9th");		  break;
		case kMin9:		strcat(string, "minor 9th");			  break;
		case kHalfDim9:	strcat(string, "half diminished 9th"); break;
		case kMinMaj9:	strcat(string, "minor major 9th");	  break;
		case kDimMaj9:	strcat(string, "diminished major 9th");break;
		case kMaj9b5:	strcat(string, "major 9th b5");		  break;
		case kDom9b5:	strcat(string, "dominant 9th b5");	  break;
		case kDom9b13:	strcat(string, "dominant 9th b13");	  break;
		case kMin9s11:	strcat(string, "minor 9th #11");		  break;
		case kmM9b11:	strcat(string, "minor/maj 9th b11");	  break;

		case kMaj7b9:	strcat(string, "major 7th b9");		  break;
		case kMaj7s5b9:	strcat(string, "major 7th #5 b9");	  break;
		case kDom7b9:	strcat(string, "dominant 7th b9");	  break;
		case kMin7b9:	strcat(string, "minor 7th b9");		  break;
		case kMinb9s11:	strcat(string, "minor b9 #11");		  break;
		case kHalfDimb9:strcat(string, "half diminished b9");  break;
		case kDim7b9:	strcat(string, "diminished b9");		  break;
		case kMinMajb9: strcat(string, "minor/major b9");	  break;
		case kDimMajb9:	strcat(string, "diminished M7 b9");	  break;

		case kMaj7s9:	strcat(string, "major 7th #9");		  break;
		case kDom7s9:	strcat(string, "dominant #9");		  break;
		case kMaj7s11:	strcat(string, "major 7th #11");		  break;
		case kMaj9s13:	strcat(string, "major 9th #13");		  break;
		case kMs9s11:	strcat(string, "major #9 #11");		  break;
		case kHDimb11:	strcat(string, "half diminished b11"); break;

		case kMaj11:	strcat(string, "major 11th");		  break;
		case kDom11:	strcat(string, "dominant 11th");		  break;
		case kMin11:	strcat(string, "minor 11th");		  break;
		case kHalfDim11:strcat(string, "half diminished 11th");break;
		case kDim11:	strcat(string, "diminished 11th");	  break;
		case kMinMaj11:	strcat(string, "minor/major 11th");	  break;
		case kDimMaj11: strcat(string, "diminished maj 11th"); break;

		case kMaj11b5:	strcat(string, "major 11th b5");		  break;
		case kMaj11s5:	strcat(string, "major 11th #5");		  break;
		case kMaj11b9:	strcat(string, "major 11th b9");		  break;
		case kMaj11s9:	strcat(string, "major 11th #9");		  break;
		case kMaj11b13:	strcat(string, "major 11th b13");	  break;
		case kMaj11s13:	strcat(string, "major 11th #13");	  break;
		case kM11b5b9:	strcat(string, "major 11th b5 b9");	  break;
		case kDom11b5:	strcat(string, "dominant 11th b5");	  break;
		case kDom11b9:	strcat(string, "dominant 11th b9");	  break;
		case kDom11s9:	strcat(string, "dominant 11th #9");	  break;
		case kHalfDim11b9:strcat(string, "half dim 11th b9");  break;
		case kDom7s11:	strcat(string, "dominant #11");		  break;
		case kMin7s11:	strcat(string, "minor 7th #11");		  break;

		case kDom13s11:	strcat(string, "dominant 13th #11");	  break;
		case kM7b913:	strcat(string, "major 7 b9 13");		  break;
		case kMaj7s13:	strcat(string, "major 7th #13");		  break;
		case kM7b9s13:	strcat(string, "major 7 b9 #13");	  break;
		case kDom7b13:	strcat(string, "dominant 7th b13");	  break;
		case kChrom:	strcat(string, "chromatic");			  break;

	}
}

// helper function that returns a substring of str
// starting from start and ending in end
char* substring_r(char* buffer, char* str, int start, int end)
{
      int i, x = 0;
      for(i = start ; i <= end; i++)
            buffer[x++] = str[i];
      buffer[x] = '\0';
      return buffer;
}

// TODO: function to translate a string to chord
// used both from inlet and from textfile
chord_t chords_memory_string2chord(t_chords_memory *x, char *string)
{
	chord_t chord;
	int index1;
	int interval;
	abs_note_t absnote;
	char substr[32]; // is 32 ok?
	if (DEBUG)
		post("chords_memory_string2chord: string='%s'",string);
	// c strings ends with \0
	// so I set the substring to \0
	memset( substr, '\0', sizeof(substr) );
	// I assume the input is from maxlib's [chord]
	// i don't need the notes before ":"
	index1 = strcspn( string, ":");
	if (index1 == strlen(string))
	{
		// : not found
		// then the input was not from maxlib's [chord]
		// i hope they passed me the right string...
		strncpy( substr, string, strlen(string));
	} else
	{
		// I will work on the right substring split by ":"
		substring_r(substr, string, index1+1, strlen(string)-1);
		if (isspace(substr[0]))
		{
			// substring inizia con uno spazio, lo tolgo
			index1 = strlen(substr)-1;
			memmove(substr, substr+1, index1); 
			substr[index1] = '\0';
		}
	}
	// now in substr i *should* have a string like this
	// "C dominant 7th"
	if (DEBUG)
		post("chords_memory_string2chord: substr='%s'",substr);
	// now I need to understand how many semitones there are
	// between x->current_fundamental and this chord
	absnote = from_string_to_abs_tone(substr);
	interval = clean_note(absnote - x->fundamental_note);
	chord.note = interval;
	chord.mode=string2mode(substr); 
	if (DEBUG)
		post("chords_memory_string2chord: chord.note=%i chord.mode=%i",chord.note, chord.mode);
	return chord;
}

// ------------------------------- search functions

// internal function
// find the better chord starting from chord1
// using the desired weight which is a value between 0 and 1
// so i have to normalize weights to 0-1 interval
// i use maxweight to do that
// TODO: add random, don't simply select the best but make a list with candidates
//  and select randomly
chord_t chords_memory_find_better(t_chords_memory *x, chord_t chord1, float desired_weight)
{
	// chords are integers
	// to know what this integer means do that:
	// int tone = chord1 / TONES_NUM
	// int type = chord1 % TYPES_NUM
	// the use a switch(tone) and a switch(type) 
	// to know what kind of chord is this
	int chord1int, chord2int, i, best_index;
	float best_value;
	float tmp;
	double rnd;
	chord_t chord2;
	chord1int = chord1.note*TYPES_NUM + chord1.mode;

	//rnd = rand()/((double)RAND_MAX + 1);
	//best_index = rnd * NODES_NUM;
	tmp = 0;
	//best_value = fabs(((float) x->arcs[chord1int][best_index]) / ((float) x->maxweight) - desired_weight);
	//if (DEBUG_VERBOSE)
	//	post("chords_memory_find_better: initial %i best value = %f",best_index, best_value);
	best_index = x->fundamental_mode; // fallback is I
	best_value = 2; // higher than everyone
	for (i=0; i<NODES_NUM; i++)
	{
		// i don't want it if has never been used
		// i wouldn't know where to go then
		// and i'd end up wondering blind
		if (x->arcs[x->fundamental_mode][chord1int][i]>0)
		{
			tmp = fabs(((float)x->arcs[x->fundamental_mode][chord1int][i]) / ((float)x->maxweight[x->fundamental_mode]) - desired_weight);
				if (DEBUG_VERBOSE)
					post("chords_memory_find_better: x->arcs[%i][%i][%i]=%i x->maxweight[%i]=%i desired_weight=%f tmp=%f",x->fundamental_mode,chord1int,i, x->arcs[x->fundamental_mode][chord1int][i], x->fundamental_mode, x->maxweight[x->fundamental_mode], desired_weight, tmp);

			if (tmp < best_value)
			{
				if (DEBUG_VERBOSE)
				{
					post("chords_memory_find_better: new best with value = %f", tmp);
					post("chords_memory_find_better: x->arcs[%i][chord1int][%i]=%i x->maxweight[%i]=%i desired_weight=%f",x->fundamental_mode,i, x->arcs[x->fundamental_mode][chord1int][i], x->fundamental_mode, x->maxweight[x->fundamental_mode], desired_weight);
				}

				best_value = tmp;
				best_index = i;
			}
			if (tmp == best_value)
			{
				rnd = rand()/((double)RAND_MAX + 1);
				if (rnd < 0.5)
				{
					best_value = tmp;
					best_index = i;
					if (DEBUG_VERBOSE)
					{
						post("chords_memory_find_better: new best with value = %f", tmp);
						post("chords_memory_find_better: x->arcs[%i][chord1int][%i]=%i x->maxweight[%i]=%i desired_weight=%f",x->fundamental_mode,i, x->arcs[x->fundamental_mode][chord1int][i],x->fundamental_mode,x->maxweight[x->fundamental_mode], desired_weight);
					}
				}
			}
		}
	}
	// now in best_index I have the best chord
	// i build the chord back from the integer
	chord2.mode = best_index % TYPES_NUM;
	chord2.note = best_index / TYPES_NUM;
	if (DEBUG)
		post("chords_memory_find_better: chord.note=%i chord.mode=%i",chord2.note, chord2.mode);
	return chord2;
}

// data structures to be used by build_walk

// to sort arrays
#include "sglib.h"

// the data set we will work on it a matrix of candidates steps
// this define each possibile chord and/or modulation
// that can be chosen at each step
typedef struct _possible_step
{
	int chordInt;
	int cost; // NB integers!
	int modulation; // 1 if needs modulations before new chord
	int tonalityInt;
} possible_step_t;

typedef struct _possible_step2
{
	int index;
	int cost; // NB integers!
} possible_step2_t;

// a row of our data set
// each row is a step in the walk
typedef struct _step_row
{
	possible_step_t cell[NODES_NUM+MODULATIONS_NUM];
	int curr_cell;
} step_row_t;

// this is the real searching function
// implementing a modified version of the 
// depth limited search
// the difference is that we don't accept solutions
// in less than the wanted number of steps

// sglib comparator for an array of possible_step_t
#define POSSIBLE_STEP_COMPARATOR(e1, e2) (e1.cost - e2.cost)
#define MY_ARRAY_ELEMENTS_EXCHANGER(type, a, i, j) {type tmp;tmp=a[i];a[i]=a[j];a[j]=tmp;}

// recursive function
// returns 0 if solutions was not found
// 1 if a solution was found
// the way i implement this changes the search function
// actually is a greedy one:
// i always select the lower cost
// and take the first solution
// this algo is:
// COMPLETE: YES
// OPTIMAL: NO
// complexity (where s=steps, n=nodes:
// best case: n*s
// worst case: n^s
// this will surely need threads!
int chords_memory_build_walk_recursive(t_chords_memory *x,
							 int chord_from_int, 
							 int tonality_from_int, 
							 int chord_to_int, 
							 int tonality_to_int, 
							 int this_step, 
							 int wanted_steps, 
							 float desired_weight,
							 step_row_t *step_matrix)
{

	int i, cost_tmp, ret;
	int this_tonality_note;
	int this_tonality_mode;
	float cost_float;
	//test
	possible_step2_t *ordered_list;
	ordered_list = malloc(sizeof(possible_step2_t)*(NODES_NUM+MODULATIONS_NUM));

	this_tonality_note = tonality_from_int / MODES_NUM;
	this_tonality_mode = tonality_from_int % MODES_NUM;

	if (DEBUG)
		post("chords_memory_build_walk_recursive: recursive function called, this_step=%i,this_tonality_mode=%i", 
		this_step,this_tonality_mode);

	// first of all, I write all costs in step_matrix[this_step];
	// chords first
	for (i=0; i<NODES_NUM; i++)
	{
		step_matrix[this_step].cell[i].chordInt = i;
		step_matrix[this_step].cell[i].modulation = 0;
		// evaluate this cost
		if (x->arcs[this_tonality_mode][chord_from_int][i]>0)
		{
			cost_float = fabs(((float)x->arcs[this_tonality_mode][chord_from_int][i]) / ((float)x->maxweight[this_tonality_mode]) - desired_weight);
		} else
		{
			cost_float = 2; // never used this chord, so very costly
		}
		cost_tmp = cost_float * 1000;
		step_matrix[this_step].cell[i].cost = cost_tmp;

		//test
		ordered_list[i].index=i;
		ordered_list[i].cost=cost_tmp;

		if (DEBUG_VERBOSE)
			post("%i: cost_float=%f cost_tmp=%i chordInt=%i", 
			i,cost_float,step_matrix[this_step].cell[i].cost, step_matrix[this_step].cell[i].chordInt); 
	}
	// then modulations
	for (i=NODES_NUM; i<MODULATIONS_NUM; i++)
	{
		step_matrix[this_step].cell[i].tonalityInt = i-NODES_NUM;
		step_matrix[this_step].cell[i].modulation = 1;
		// step_matrix[this_step].cell[i].chordInt = ?? TODO
		//step_matrix[this_step].cell[i].cost = ?? TODO
		step_matrix[this_step].cell[i].cost = 2;
		// HOW DO I MANAGE MODULATIONS ?!?!?!

	}
	step_matrix[this_step].curr_cell=0;

	if (DEBUG_VERBOSE)
		post("sorting...");
	// now order the costs using sglib
	// as i am greedy i follow the cheaper path first
//	SGLIB_ARRAY_QUICK_SORT (possible_step_t, step_matrix[this_step].cell, NODES_NUM+MODULATIONS_NUM, POSSIBLE_STEP_COMPARATOR, MY_ARRAY_ELEMENTS_EXCHANGER)
	SGLIB_ARRAY_QUICK_SORT (possible_step2_t, ordered_list, NODES_NUM+MODULATIONS_NUM, POSSIBLE_STEP_COMPARATOR, MY_ARRAY_ELEMENTS_EXCHANGER)

	// this function is called recursively
	//base case is then this_step==wanted_steps-1

//	for (step_matrix[this_step].curr_cell=0; 
//		step_matrix[this_step].curr_cell<(NODES_NUM+MODULATIONS_NUM); 
//		step_matrix[this_step].curr_cell++)
	for (i=0; i<(NODES_NUM+MODULATIONS_NUM); i++)
	{
		step_matrix[this_step].curr_cell = ordered_list[i].index;
		// i don't want solutions that use never heard chords...
		if (ordered_list[i].cost < 2000)
		{
			if (DEBUG_VERBOSE)
			{
				post("step_matrix[%i].curr_cell=%i chodInt=%i cost=%i", 
					this_step,step_matrix[this_step].curr_cell,
					step_matrix[this_step].cell[step_matrix[this_step].curr_cell].chordInt,
					step_matrix[this_step].cell[step_matrix[this_step].curr_cell].cost
					); 
			}
			if (this_step==wanted_steps-1)
			{
				// base case
				// look for the solution
				if ((step_matrix[this_step].cell[step_matrix[this_step].curr_cell].chordInt == chord_to_int) &&
					(step_matrix[this_step].cell[step_matrix[this_step].curr_cell].tonalityInt == tonality_to_int))
				{
					// i just found a solution!
					if (DEBUG)
						post("chords_memory_build_walk_recursive: solution found! chord_note=%i chord_to_int=%i",
						chord_to_int/TYPES_NUM,chord_to_int);
					free(ordered_list);
					return 1; // as i am greedy (and lazy) i return immediatly
				}
			} else
			{
				// recurive case
				ret = chords_memory_build_walk_recursive(x, 
					step_matrix[this_step].cell[step_matrix[this_step].curr_cell].chordInt, 
					step_matrix[this_step].cell[step_matrix[this_step].curr_cell].tonalityInt, 
					chord_to_int,  
					tonality_to_int, 
					this_step+1, 
					wanted_steps, 
					desired_weight, 
					step_matrix);
				if 	(ret == 1)
				{
					// solution found below me!
					if (DEBUG)
						post("chords_memory_build_walk_recursive: solution found! chordInt=%i cost=%i",
						step_matrix[this_step].cell[step_matrix[this_step].curr_cell].chordInt,
						step_matrix[this_step].cell[step_matrix[this_step].curr_cell].cost);
					free(ordered_list);
					return 1;
				}

			}
		}
	}
	free(ordered_list);
	return 0;
}

// TODO: add ending tonality !!!!
void chords_memory_build_walk(t_chords_memory *x, chord_t starting_chord, chord_t ending_chord, float desired_weight, int steps)
{
	int i,j,s,n, ret;
	int chord_from_int, chord_to_int, tonality_from_int, tonality_to_int;
	step_row_t *step_matrix;
	// for output
	t_atom *chords_sequence;
	char stringTMP[25];
	// alloc arrays
	if (x->using_walk)
		free(x->walk);
	x->walk = malloc(sizeof(step_t)*steps);
	step_matrix = malloc(sizeof(step_row_t)*steps);
	chords_sequence = malloc(sizeof(t_atom)*steps);

	// for each step:
	// fill this step with costs
	// order the list
	// take the first
	// use it to fill the next level until you get to the desired step
	// if you don't find a solution then step back
	// select the second alternative and again
	chord_from_int = starting_chord.note * TYPES_NUM + starting_chord.mode ;
	tonality_from_int = x->fundamental_note * MODES_NUM + x->fundamental_mode;
	chord_to_int = ending_chord.note * TYPES_NUM + ending_chord.mode ;
	tonality_to_int = tonality_from_int; // TODO !!!!!

	if (DEBUG)
		post("chords_memory_build_walk: calling recursive function");

	ret = chords_memory_build_walk_recursive(x, chord_from_int, tonality_from_int, chord_to_int,  tonality_to_int, 
		0, steps, desired_weight, step_matrix);
	if (ret==0)
	{
		// no solution found
		// what do i do now?!?!
		if (DEBUG)
			post("no solution found");
		return;
	}
	//  copy the solution to the x->walk
	//  and set all needed vars
	for (i=0; i<steps; i++)
	{
		int chordIntTMP = step_matrix[i].cell[step_matrix[i].curr_cell].chordInt;
		int tonalityIntTMP = step_matrix[i].cell[step_matrix[i].curr_cell].tonalityInt;
		int modulationTMP = step_matrix[i].cell[step_matrix[i].curr_cell].modulation;
		// i copy this chord/modulation to the walk
		x->walk[i].modulate = modulationTMP;
		x->walk[i].chord.note = chordIntTMP / TYPES_NUM;
		x->walk[i].chord.mode = chordIntTMP % TYPES_NUM;
		x->walk[i].tonality_note = tonalityIntTMP / MODULATIONS_NUM;
		x->walk[i].tonality_mode = tonalityIntTMP % MODULATIONS_NUM;
		// build a list to send out to the 
		// right outlet
		chords_memory_chord2string(x,stringTMP,x->walk[i].chord);
		if (DEBUG)
			post("chord: %s", stringTMP);

		//outlet_symbol(x->x_outtonalityname, gensym(stringTMP));

		SETSYMBOL(chords_sequence+1, gensym(stringTMP));
		//SETSYMBOL(chords_sequence+1, stringTMP);
	}

	// send the solution list to outlet x_outchordssequence
	outlet_list(x->x_outchordssequence,
                     gensym("list") ,
					 steps, 
					 chords_sequence);

	// TODO: set other vars for walk
	
	// free arrays
	free(step_matrix);
	free(chords_sequence);
	// don't free walk, will be freed by someone else
}


// ------------------------------- from now on there are functions directly called by pd

// when you send a |next x( message...
// function actually invoked when sending a "next x" message to the external
void chords_memory_next(t_chords_memory *x, t_floatarg f)
{
	float desired_weight;
	chord_t best;
	char string[32];
	desired_weight = f;
	best = chords_memory_find_better(x, x->last_chord, desired_weight);
	chords_memory_chord2string(x, string, best);
	outlet_symbol(x->x_outchordname, gensym(string));
}

// when you send a search message
void chords_memory_search(t_chords_memory *x, t_symbol *sl, int argc, t_atom *argv)
{
	int steps;
	chord_t chord2;
	float desired_weight;
	// parse the list
	// you need chord_dest, steps and desired_weight
	// (later desired tonality also)

	if (argc<4)
	{
		error("usage: search chord-note chord-type steps weight");
		return;
	}
	
	chord2.note = atom_getint(argv);
	chord2.mode= atom_getint(argv+1);
	steps = atom_getint(argv+2);
	desired_weight = atom_getfloat(argv+3);

	chords_memory_build_walk(x, x->last_chord, chord2, desired_weight, steps);

}


// ------------------------------- functions to set and add chords/tonality

// sets the current chord
void chords_memory_set_chord(t_chords_memory *x, t_symbol *s) {
	if (! x->last_chord_set)
	{
		x->last_chord_set=1;
	}
	x->last_chord = chords_memory_string2chord(x, s->s_name);
	if (DEBUG)
		post("chords_memory_set_chord: chord.note=%i chord.mode=%i",x->last_chord.note, x->last_chord.mode);
}

// add a chord sequence to the graph
float chords_memory_add(t_chords_memory *x, chord_t chord1, chord_t chord2)
{
	// chords are integers
	// to know what this integer means do that:
	// int tone = chord1 / TONES_NUM
	// int type = chord1 % TYPES_NUM
	// the use a switch(tone) and a switch(type) 
	// to know what kind of chord is this

	int chord1int, chord2int;
	chord1int = chord1.note*TYPES_NUM + chord1.mode;
	chord2int = chord2.note*TYPES_NUM + chord2.mode;
	// now that i've translated chords in integers i can add
	// 1 to its wheight (a bit less new)
	x->arcs[x->fundamental_mode][chord1int][chord2int] = x->arcs[x->fundamental_mode][chord1int][chord2int] + 1;
	// is this the new maxweight?
	if (x->arcs[x->fundamental_mode][chord1int][chord2int] > x->maxweight[x->fundamental_mode])
	{
		x->maxweight[x->fundamental_mode] = x->arcs[x->fundamental_mode][chord1int][chord2int];
		if (DEBUG)
			post("x->maxweight[%i] = %i",x->fundamental_mode, x->maxweight[x->fundamental_mode]);
	}
	return (float) (((float) x->arcs[x->fundamental_mode][chord1int][chord2int]) / ((float) x->maxweight[x->fundamental_mode]) );

}

// function invoked when a new chord is added at the graph
// the external remembers the previous played chord
void chords_memory_add_chord(t_chords_memory *x, t_symbol *s) {
    chord_t chord1;
	float ret = 0;
	chord1 = chords_memory_string2chord(x, s->s_name);
	if (x->last_chord_set)
	{
		ret = chords_memory_add(x, x->last_chord, chord1);
	} 
	else
	{
		x->last_chord_set=1;
	}
	x->last_chord = chord1;
	if (DEBUG)
		post("chord added: %s", s->s_name); 
	outlet_float(x->x_outnovelty, ret);
}

// sets the current tonality
void chords_memory_set_tonality(t_chords_memory *x, t_symbol *s) {
	chord_t c;
	int old_tonality;
	int interval;
	old_tonality = x->fundamental_note;
	//x->fundamental_note = (x->fundamental_note + from_string_to_abs_tone(s->s_name)) % 12;
	x->fundamental_note = from_string_to_abs_tone(s->s_name);
	x->fundamental_mode = from_string_to_mode(s->s_name);
	// when i set the tonality i always
	// go on the I grade
	if (! x->last_chord_set)
	{
		x->last_chord_set=1;
	}
	interval = x->fundamental_note - old_tonality;
	x->last_chord.note = clean_note(x->last_chord.note - interval);
	outlet_symbol(x->x_outtonalityname, gensym(s->s_name));
	if (DEBUG)
	{
		post("chords_memory_set_tonality: new tonality note=%i mode=%i",x->fundamental_note,x->fundamental_mode);
		post("chords_memory_set_tonality: chord.note=%i chord.mode=%i",x->last_chord.note, x->last_chord.mode);
	}
}

// adds this modulation to memory
// code similar to chords_memory_add
// but adds a modulation instead of a chord
void chords_memory_add_modulation(t_chords_memory *x, t_symbol *s) {
	chord_t c;
	int old_tonality;
	int newnote,newmode,newabsnote,chord1int;
	short int modulationInt;
	old_tonality = x->fundamental_note;
	//x->fundamental_note = (x->fundamental_note + from_string_to_abs_tone(s->s_name)) % 12;
	newabsnote = from_string_to_abs_tone(s->s_name);
	newmode = from_string_to_mode(s->s_name);
	// when i set the tonality i always
	// go on the I grade
	if (! x->last_chord_set)
	{
		x->last_chord_set=1;
	}
	newnote = clean_note(newabsnote - old_tonality);
	modulationInt = newnote*MODES_NUM + newmode;
	chord1int = x->last_chord.note*TYPES_NUM + x->last_chord.mode;
	x->modulations[x->fundamental_mode][chord1int][modulationInt] = x->modulations[x->fundamental_mode][chord1int][modulationInt] + 1;
	// is this the new maxweight?
	if (x->modulations[x->fundamental_mode][chord1int][modulationInt] > x->maxweight[x->fundamental_mode])
	{
		x->maxweight[x->fundamental_mode] = x->modulations[x->fundamental_mode][chord1int][modulationInt];
	}
	if (DEBUG)
	{
		post("chords_memory_add_modulation: new tonality note=%i mode=%i",newnote,newmode);
	}
	chords_memory_set_tonality(x, gensym(s->s_name));
}

// ---------------------- file I/O

// function to read graph from a file
void chords_memory_read_from_file(t_chords_memory *x, t_symbol *s)
{
	FILE *fp;
	int i, j, m;
	unsigned char tmp[2];
	if ((fp=fopen(s->s_name, "r+b"))==NULL)
	{
		post("error: can't open file %s", s->s_name);
		return;
	}
	
	for (m=0; m<MODES_NUM; m++)
	{
		// i read maxweight
		x->maxweight[m] = (fgetc(fp)<<16)|(fgetc(fp));
		// i read the matrix
		for (i=0; i<NODES_NUM; i++)
		{
			for (j=0; j<NODES_NUM; j++)
			{
				// this should avoids problems little/big endian
				// i force little endian (most significant byte first)
				tmp[0] = fgetc(fp);
				tmp[1] = fgetc(fp);
				x->arcs[m][i][j] = (tmp[0]<<8)|(tmp[1]);
				if (DEBUG_VERBOSE)
				{
					if (x->arcs[m][i][j]>0)
					{
						post("x->arcs[%i][%i][%i] = %i",m,i,j,x->arcs[m][i][j]);
					}
				}
			}
			for (j=0; j<MODES_NUM; j++)
			{
				// this should avoids problems little/big endian
				// i force little endian (most significant byte first)
				tmp[0] = fgetc(fp);
				tmp[1] = fgetc(fp);
				x->modulations[m][i][j] = (tmp[0]<<8)|(tmp[1]);
				if (DEBUG_VERBOSE)
				{
					if (x->modulations[m][i][j]>0)
					{
						post("x->modulations[%i][%i][%i] = %i",m,i,j,x->modulations[m][i][j]);
					}
				}
			}
		}
		if (DEBUG)
			post("x->maxweight[%i] = %i",m, x->maxweight[m]);
	}
	if (DEBUG)
		post("graph read from file %s",s->s_name);
	fclose(fp);

}

// function to write graph to a file (for later loading)
void chords_memory_write_to_file(t_chords_memory *x, t_symbol *s)
{
	FILE *fp;
	int i, j, m, tmp;
	fp=fopen(s->s_name, "w+b");

	for (m=0; m<MODES_NUM; m++)
	{
		// i write down maxweight
		fputc(x->maxweight[m]>>8, fp);
		tmp = x->maxweight[m]<<8;
		tmp = tmp >> 8;
		fputc(tmp, fp);
		// i write down the matrix
		for (i=0; i<NODES_NUM; i++)
		{
			for (j=0; j<NODES_NUM; j++)
			{
				// this should avoids problems little/big endian
				// i force little endian (most significant byte first)
				fputc(x->arcs[m][i][j]>>8, fp);
				tmp = x->arcs[m][i][j]<<8;
				tmp = tmp >> 8;
				fputc(tmp, fp);
			}
			for (j=0; j<MODES_NUM; j++)
			{
				// this should avoids problems little/big endian
				// i force little endian (most significant byte first)
				fputc(x->modulations[m][i][j]>>8, fp);
				tmp = x->modulations[m][i][j]<<8;
				tmp = tmp >> 8;
				fputc(tmp, fp);
			}
		}
	}
	if (DEBUG)
		post("graph wrote to file %s",s->s_name);
	fclose(fp);
}

// TODO: function that reads chords from a textfile and trains the graph

// TODO: recursive function that builds a walk from chord1 to chord2 using desired novelty

// set filename
void chords_memory_set_filename(t_chords_memory *x, t_symbol *s) {
    x->filename = s;
}



void *chords_memory_new(t_symbol *s, int argc, t_atom *argv)
{
	int i;
	time_t a;
    t_chords_memory *x = (t_chords_memory *)pd_new(chords_memory_class);
	x->x_outchordname = outlet_new(&x->x_obj, gensym("symbol"));
	x->x_outtonalityname = outlet_new(&x->x_obj, gensym("symbol"));
	x->x_outchordssequence = outlet_new(&x->x_obj, gensym("list"));
	x->x_outnovelty = outlet_new(&x->x_obj, gensym("float"));
	srand(time(&a));
	chords_memory_init_graph(x);
	x->fundamental_note = C;
	x->fundamental_mode = MAJOR;
	 // example parameter
	if (argc>0) 
	{
		x->filename = atom_getsymbolarg(0, argc, argv);
		chords_memory_read_from_file(x, x->filename); 
	} 

    return (x);
}

// here I free allocated memory if any
void chords_memory_free(t_chords_memory *x)
{
//	free(x->current_fundamental);
}

void chords_memory_setup(void)
{
    chords_memory_class = class_new(gensym("chords_memory"), (t_newmethod)chords_memory_new,
        (t_method)chords_memory_free, sizeof(t_chords_memory), CLASS_DEFAULT, A_GIMME, 0);
    // file I/O
	class_addmethod(chords_memory_class, (t_method)chords_memory_write_to_file, gensym("write"),A_SYMBOL, 0);
    class_addmethod(chords_memory_class, (t_method)chords_memory_read_from_file, gensym("read"),A_SYMBOL, 0);
 	// ask for the best choice form here
	class_addmethod(chords_memory_class, (t_method)chords_memory_next, gensym("next"), A_DEFFLOAT, 0);
    // ask for a path from here to desired destination
	class_addmethod(chords_memory_class, (t_method)chords_memory_search, gensym("search"), A_GIMME, 0);
    // add or set chord
	class_addmethod(chords_memory_class, (t_method)chords_memory_add_chord, gensym("add"),A_SYMBOL, 0);
    class_addmethod(chords_memory_class, (t_method)chords_memory_set_chord, gensym("set"),A_SYMBOL, 0);
	// change current tonality
	class_addmethod(chords_memory_class, (t_method)chords_memory_set_tonality, gensym("tonality"),A_SYMBOL, 0);
	class_addmethod(chords_memory_class, (t_method)chords_memory_add_modulation, gensym("modulation"),A_SYMBOL, 0);
 	// reinit memory
	class_addmethod(chords_memory_class, (t_method)chords_memory_init, gensym("init"), A_DEFFLOAT, 0);

}
