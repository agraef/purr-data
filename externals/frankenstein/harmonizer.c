/* 
harmonizer:
this external build voicing from a chord to another.
takes n voices (midi) as input
you can set the current chord
you can ask for next note of each voice to get to a target chord

usefull to create chorals

voicing is built using traditional GA (not co-evolving GA)

voicing rules are hardcoded and are:
- no parallel 8ths nor 5ths
- better no hidden 8ths nor 5ths
- better if uniform voice spacing (except for the bass)
- better if little intervals
- no all voices same direction
- no voices outside limits
- better complete chords

TODO:
would be nice to be able so set some rule at runtime 
or at least set the importance of rules in realtime..


*/
#include <stdlib.h>
#include <math.h>
#include <time.h>
// for string manipulation
#include <string.h>
#include <ctype.h>
#include "m_pd.h"
#include "common.h"

// to sort arrays
#include "sglib.h"

#define MAX_POPULATION 500

#define DEF_PROB_MUTATION 0.03f

#define VOICES 5

#define NOTES_RANGE 80 // this should be multiple of 16
#define LOWER_POSSIBLE_NOTE 24 // lower note possible, it should be a C
#define POSSIBLE_NOTES (NOTES_RANGE/12*5) // 5 is the max number of notes in a chord

// default values 
#define DEF_WIDENESS 3 // 3 octaves
#define DEF_CENTER_NOTE 72 // central C
// testing i noticed that we don't need more than 1 generation..
// this is because we create an initial population that is really good
// we may not need to crossover at all!
#define GENERATIONS 1

#define DEBUG 0 // messaggi di debug
#define DEBUG_VERBOSE 0 // messaggi di debug

static t_class *harmonizer_class;



// this defines a chord in a tonality
typedef struct _chord_abs
{
	chord_type_t mode;
	abs_note_t note;
} chord_abs_t;


typedef struct _harmonizer
{
    t_object x_obj; // myself
	// genotypes
	//int population[MAX_POPULATION][VOICES];
	//int current_voices[VOICES];
	int *population[MAX_POPULATION];
	int *current_voices;
	chord_abs_t current_chord;
	chord_abs_t target_chord;
	int target_notes[POSSIBLE_NOTES];
	t_outlet *l_out;
	int voices;
	float wideness;
	int center_note;
	float i_like_parallelism;
	float small_intervals;
	//TODO
//	int lower_octave;
//	int notes_range;
	
} t_harmonizer;

// I build a table of possible notes
// are the notes (midi)that form target_chord
void build_possible_notes_table(t_harmonizer *x)
{
	int i, octave, basenote;
	int n1, n2, n3, n4, n5;
	n1=n2=n3=n4=0; // there always is the fundamental
	if (DEBUG_VERBOSE)
		post("build_possible_notes_table target_chord.mode=%i target_chord.note=%i", x->target_chord.mode, x->target_chord.note);
	switch (x->target_chord.mode)
	{
		case kMaj:		n2=4; n3=7; n4=0;n5=0;break;
		case kMin:		n2=3; n3=7; n4=0;n5=0;break;
		case kDim:		n2=3; n3=6; n4=0;n5=0;break;
		case kAug:		n2=4; n3=8; n4=0;n5=0;break;
		case kMaj7:		n2=4; n3=7; n4=11;n5=0;break;
		case kDom7:		n2=4; n3=7; n4=10;n5=0;break;
		case kMin7: 	n2=3; n3=7; n4=10;n5=0;break;
		case kHalfDim7:	n2=3; n3=6; n4=10;n5=0;break;
		case kDim7:		n2=3; n3=6; n4=9;n5=0;break;
		case kMinMaj7:	n2=4; n3=7; n4=11;n5=0;break;

		case kMaj7b9:	n2=4; n3=7; n4=11;n5=1;break;
		case kMaj9:		n2=4; n3=7; n4=11;n5=2;break;
		//case kMinMaj7:	n2=4; n3=7; n4=11;n5=3;break;
		//case kMaj7:		n2=4; n3=7; n4=11;n5=4;break;
		case kDom7s11:	n2=4; n3=7; n4=10;n5=6;break;
		case kDomb9:	n2=4; n3=7; n4=10;n5=1;break;
		case kMaj7s5:	n2=4; n3=8; n4=11;n5=0;break;
		case kMin9:		n2=3; n3=7; n4=2;n5=0;break;

		case kDom9:		n2=4; n3=7; n4=10;n5=2;break;

		case kM7b9s13:		n2=1; n3=10; n4=11;n5=4;break;
		case kMinMajb9:		n2=1; n3=3; n4=11;n5=7;break;
		case kDimMajb9:		n2=3; n3=6; n4=11;n5=1;break;

		case kMinMaj9:		n2=3; n3=7; n4=11;n5=2;break;
		case kHalfDimb9:	n2=3; n3=6; n4=1;n5=10;break;
		case kDim7b9:		n2=3; n3=6; n4=1;n5=9;break;
		case kMaj7s9:		n2=3; n3=4; n4=7;n5=11;break;
		case kDom7s9:		n2=3; n3=4; n4=7;n5=10;break;
		case kMaj11:		n2=4; n3=5; n4=7;n5=11;break;
		case kMaj7b5:		n2=4; n3=6; n4=11;n5=0;break;
		case kMaj7s13:		n2=4; n3=7; n4=11;n5=10;break;

		case kUnison:		n2=0; n3=0; n4=0;n5=0;break;


		case kDom7b5:		n2=4; n3=6; n4=10;n5=0;break;

		case kHalfDim9:		n2=3; n3=6; n4=10;n5=2;break;

		case kMaj9b5:		n2=4; n3=6; n4=11;n5=2;break;
		case kDom9b5:		n2=4; n3=6; n4=10;n5=2;break;
		case kDom9b13:		n2=4; n3=2; n4=8;n5=10;break;
		case kMin9s11:		n2=2; n3=3; n4=6;n5=7;break;
		case kmM9b11:		n2=3; n3=11; n4=2;n5=4;break;

		case kMaj7s5b9:		n2=2; n3=4; n4=8;n5=11;break;
		case kDom7b9:		n2=1; n3=4; n4=7;n5=10;break;
		case kMin7b9:		n2=1; n3=3; n4=7;n5=10;break;
		case kMinb9s11:		n2=1; n3=3; n4=7;n5=6;break;

		case kMaj7s11:		n2=4; n3=6; n4=7;n5=11;break;
		case kMs9s11 :		n2=3; n3=4; n4=6;n5=7;break;
		case kHDimb11 :		n2=3; n3=6; n4=10;n5=4;break;

		case kDom11 :		n2=4; n3=7; n4=10;n5=5;break;
		case kMin11 :		n2=3; n3=7; n4=5;n5=0;break;
		case kHalfDim11 :	n2=3; n3=6; n4=10;n5=5;break;
		case kDim11 :		n2=3; n3=6; n4=9;n5=5;break;
		case kMinMaj11 :	n2=3; n3=7; n4=11;n5=5;break;
		case kDimMaj11 :	n2=3; n3=6; n4=11;n5=5;break;
		case kMaj11b5 :		n2=4; n3=6; n4=5;n5=0;break;
		case kMaj11s5 :		n2=4; n3=5; n4=8;n5=0;break;
		case kMaj11b9 :		n2=4; n3=5; n4=7;n5=1;break;
		case kMaj11s9 :		n2=4; n3=5; n4=7;n5=3;break;
		case kMaj11b13 :	n2=4; n3=5; n4=7;n5=8;break;
		case kMaj11s13 :	n2=4; n3=5; n4=10;n5=11;break;
		case kM11b5b9 :		n2=4; n3=6; n4=1;n5=11;break;
		case kDom11b5 :		n2=4; n3=6; n4=10;n5=5;break;
		case kDom11b9 :		n2=4; n3=5; n4=10;n5=1;break;
		case kDom11s9 :		n2=4; n3=5; n4=10;n5=3;break;
		case kHalfDim11b9 :	n2=3; n3=6; n4=5;n5=1;break;
		case kMin7s11 :		n2=3; n3=7; n4=10;n5=6;break; // is it correct?
		case kDom13s11 :	n2=4; n3=10; n4=6;n5=9;break; // is it correct ?
		case kM7b913 :		n2=4; n3=10; n4=1;n5=9;break;
		case kMaj9s13 :		n2=2; n3=4; n4=11;n5=9;break;
		case kDom7b13 :		n2=4; n3=10; n4=9;n5=7;break;
		case kChrom :		n2=1; n3=2; n4=3;n5=4;break;

	}
	if (DEBUG_VERBOSE)
		post("build_possible_notes_table n2=%i n3=%i n4=%i", n2, n3, n4);

	basenote=0;
	switch (x->target_chord.note)
	{
		case C:		basenote=0;break;
		case Db:	basenote=1;break;
		case D:		basenote=2;break;
		case Eb:	basenote=3;break;
		case E:		basenote=4;break;
		case F:		basenote=5;break;
		case Gb:	basenote=6;break;
		case G:		basenote=7;break;
		case Ab:	basenote=8;break;
		case A:		basenote=9;break;
		case Bb:	basenote=10;break;
		case B:		basenote=11;break;
	}
	if (DEBUG_VERBOSE)
		post("build_possible_notes_table basenote=%i", basenote);
	i=0;
	octave=0;
	while (i<(POSSIBLE_NOTES-3))
	{
		x->target_notes[i++]=octave*12 + LOWER_POSSIBLE_NOTE + basenote + n1;
		x->target_notes[i++]=octave*12 + LOWER_POSSIBLE_NOTE + basenote + n2;
		x->target_notes[i++]=octave*12 + LOWER_POSSIBLE_NOTE + basenote + n3;
		x->target_notes[i++]=octave*12 + LOWER_POSSIBLE_NOTE + basenote + n4;
		x->target_notes[i++]=octave*12 + LOWER_POSSIBLE_NOTE + basenote + n5;
		octave++;
	}
	if (DEBUG_VERBOSE)
	{
		i=0;
		while (i<(POSSIBLE_NOTES))
		{
			post("x->target_notes[%i]=%i", i, x->target_notes[i++]);
		}
	}
}



// -----------------  normal external code ...

void harmonizer_init_pop(t_harmonizer *x)
{
	int i, j, tmp, tmp2, k, steps, note, insertpoint;
	double rnd;
	for (i=0; i<MAX_POPULATION; i++)
	{
		for (j=0; j<x->voices; j++)
		{
			/*
			// totally randome version
			rnd = rand()/((double)RAND_MAX + 1);
			tmp = rnd * POSSIBLE_NOTES;
			x->population[i][j] = x->target_notes[tmp];
			*/

			// not totally random: i start from currend chord's notes
			// and randomly go up or down
			insertpoint = 0;
			while ((insertpoint < POSSIBLE_NOTES) && (x->target_notes[insertpoint] < x->current_voices[j]))
				insertpoint++;
			if (insertpoint >= POSSIBLE_NOTES)
			{ 
				// i didn't find my insert point, possible?
				// i pick a random one
				rnd = rand()/((double)RAND_MAX + 1);
				tmp = rnd * POSSIBLE_NOTES;
				x->population[i][j] = x->target_notes[tmp];
			} else
			{
				// insert point found
				rnd = rand()/((double)RAND_MAX + 1);
				if (rnd < 0.5)
				{
					// i go up
					rnd = rand()/((double)RAND_MAX + 1);
					steps = rnd * 10; // how many steps (good notes) will I ignore?
					note = insertpoint + steps;
					if (note >= POSSIBLE_NOTES)
						note = POSSIBLE_NOTES-1;
					
				} else
				{
					// i go down
					rnd = rand()/((double)RAND_MAX + 1);
					steps = rnd * 10; // how many steps (good notes) will I ignore?
					note = insertpoint - steps;
					if (note < 0)
						note = 0;
				}
				// finally assign the note
				x->population[i][j] = x->target_notes[note];
			}
		}
	}
}

void harmonizer_allocate(t_harmonizer *x)
{
	int i;
	for (i=0; i<MAX_POPULATION; i++)
	{
		x->population[i] = malloc(sizeof(int)*x->voices);
	}
	x->current_voices = malloc(sizeof(int)*x->voices);
	
}

void harmonizer_free(t_harmonizer *x)
{
//	freebytes(x->buf_strum1, sizeof(x->buf_strum1));
//	freebytes(x->buf_strum2, sizeof(x->buf_strum2));
	
	int i;
	for (i=0; i<MAX_POPULATION; i++)
	{
		free(x->population[i]);
	}
	free(x->current_voices);
}

// here i evaluate this voicing
int fitness(t_harmonizer *x, int *candidate)
{
	int i, j, tmp, res, last, avgHI, avgLOW, min, max, distance;
	float wideness, ftmp;
	short int chord_notes[4];
	short int chord_notes_ok[4];
	//short int transitions[VOICES];
	short int *transitions;
	//short int directions[VOICES];
	short int *directions;
	// intervals between voices
	// for parallel and hidden 5ths
	// voices spacing etc..
	//short int intervals[VOICES][VOICES]; 
	short int **intervals; 
	//short int notes[VOICES];
	short int *notes;
	res=50; // starting fitness

	if (DEBUG_VERBOSE)
		post("evaluating fitness of %i %i %i %i", candidate[0], candidate[1], candidate[2], candidate[3]);

	// allocate arrays
	transitions = malloc(sizeof(short int)*x->voices);
	directions = malloc(sizeof(short int)*x->voices);
	notes = malloc(sizeof(short int)*x->voices);
	intervals = malloc(sizeof(short int *) * x->voices);
	for (i=0; i<x->voices; i++)
	{
		intervals[i] = malloc(sizeof(short int) * x->voices);
	}

 	// shared objects
	for (i=0; i<x->voices; i++)
	{
		notes[i]=candidate[i];
		transitions[i] = candidate[i] - x->current_voices[i];
		if (transitions[i]!=0)
			directions[i] = transitions[i]/abs(transitions[i]);
		else
			directions[i] = 0;
		if (DEBUG_VERBOSE)
			post("directions[%i]=%i", i, directions[i]);

	}
	for (i=0; i<x->voices; i++)
	{
		for (j=i+1; j<x->voices; j++)
		{
			intervals[i][j] = (candidate[i]-candidate[j])%12 ;
			if (DEBUG_VERBOSE)
				post("intervals[%i][%i]=%i", i, j, intervals[i][j]);
		}
	}
	SGLIB_ARRAY_SINGLE_QUICK_SORT(short int, notes, x->voices, SGLIB_NUMERIC_COMPARATOR)

	// all same direction? 
	if ( directions[0]==directions[1] &&
	directions[1]==directions[2] &&
	directions[2]==directions[3])
	{
		res += 10 * x->i_like_parallelism;
		if (DEBUG_VERBOSE)
			post("same direction!");
	}
	
	// parallel 5ths or octaves? (if yes return 0)
	// how?
	// hidden 8ths nor 5ths ?
	for (i=0; i<x->voices; i++)
	{
		for (j=i+1; j<x->voices; j++)
		{
			if (intervals[i][j]==7 || intervals[i][j]==0)
			{
				// hidden or parallel 5th,octave or unison
				// bad!
				if (directions[i]==directions[j])
				{
					res += 10 * x->i_like_parallelism;
					if (DEBUG_VERBOSE)
						post("hidden or parallel consonance!");
				}
			}
		}
	}

	// is voice spacing uniform ?(except for the bass)
	// TODO: use notes[]
	// are voices average centered?
	tmp=0;
	for (i=1; i<x->voices; i++)
	{
		tmp+=notes[i];
		if (DEBUG_VERBOSE)
			post("average note is %i at passage %i", tmp, i);
	}
	// this is the average note
	tmp = tmp/(x->voices-1);
	if (DEBUG_VERBOSE)
		post("average note is %i after division by (x->voices-1)", tmp);
//	tmp = abs((LOWER_POSSIBLE_NOTE + NOTES_RANGE)*2/3 - tmp); // how much average is far from 72
	tmp = abs(x->center_note - tmp); // how much average is far from desired center note
	res += 30; 
	res -= tmp;
	
	if (DEBUG_VERBOSE)
		post("average note is %i far from 2/3 of notes range", tmp);

	tmp=0;
	/*
	// are voices average centered?
	for (i=0; i<VOICES; i++)
	{
		tmp+=notes[i];
	}
	// this is the average note
	tmp = tmp/VOICES;
	tmp = abs(72-tmp); // how much average is far from 72
	res += 30; 
	res -= tmp;
	*/

	// are intervals small?
	//res+=50;
	if (DEBUG_VERBOSE)
		post("res before transitions %i", res);
	for (i=0; i<x->voices; i++)
	{
		if (DEBUG_VERBOSE)
			post("transitions[%i] = %i",i, transitions[i]);
		res-=abs(transitions[i]) * x->small_intervals;
		// give an incentive for semitones etc..
		if (transitions[i]==0)
			res += 5;
		if (abs(transitions[i]==1))
			res += 5 * x->small_intervals;
		if (abs(transitions[i]==2))
			res += 5 * x->small_intervals;
		if (abs(transitions[i]==3))
			res += 2 * x->small_intervals;
		if (abs(transitions[i]==4))
			res += 2 * x->small_intervals;
		if (abs(transitions[i]==5))
			res += 1 * x->small_intervals;
		if (abs(transitions[i]==6))
			res += 1 * x->small_intervals;
		if (abs(transitions[i]>11))
			res -= 2 * x->small_intervals;
		if (abs(transitions[i]>15))
			res -= 5 * x->small_intervals;

	}
	if (DEBUG_VERBOSE)
		post("res after transitions %i", res);

	// TODO: too many near limits?
	
	// is a complete chord?
	// does this voicing have all 5 notes?
	// first build a table for comparision
	for (i=0; i<4; i++)
	{
		chord_notes[i] = (x->target_notes[i]) % 12;
		chord_notes_ok[i] = 0;
	}
	for (i=0; i<x->voices; i++)
	{
		tmp = notes[i] % 12;
		for (j=0; j<4; j++)
		{
			if (chord_notes[j] == tmp)
				chord_notes_ok[j]++;
		}
	}
	// now in chord_notes_ok i have the number of times each note is present
	if (chord_notes_ok[0] == 0)
	{
		// no fundamental! this is bad!!
		res -= 10;
	}
	if ((chord_notes_ok[0] != 0) &&
		(chord_notes_ok[2] != 0) &&
		(chord_notes_ok[3] != 0) && 
		(chord_notes_ok[4] != 0))
	{
		// complete chord! this is good
		res += 10;
	}
	for (j=0; j<4; j++)
	{
		res -= 2^chord_notes_ok[j];
	}
	res += 2*x->voices;

	// penalize too many basses
	tmp = 0;
	for (i=0; i<x->voices; i++)
	{
		if (notes[i]<48)
			tmp++;
	}
	switch (tmp)
	{
	case 0: res -= 5; break;
	case 1: res += 10; break;
	case 2: res -= 10; break;
	case 3: res -= 20; break;
	case 4: res -= 30; break;
	}

	// now wideness	
	min = notes[0];
	max = notes[x->voices-1];
	distance = max - min;
	wideness = (float) (((float)distance) / ((float)12));
	ftmp = fabs(wideness - x->wideness);
	res -= ftmp * 5;
	
	if (DEBUG_VERBOSE)
		post("fitness is %i", res);

		// free memory
	free(transitions);
	free(directions); 
	free(notes);
	for (i=0; i<x->voices; i++)
	{
		free(intervals[i]);
	}
	free(intervals);

	return res;
}

void new_genotype(t_harmonizer *x, int *mammy, int *daddy, int *kid)
{
	int i, split;
	double rnd;
	// crossover
	rnd = rand()/((double)RAND_MAX + 1);
	split = rnd * x->voices;
	for (i=0; i<split; i++)
	{
		kid[i]=mammy[i];
	}
	for (i=split; i<x->voices; i++)
	{
		kid[i]=daddy[i];
	}

	//  mutation
	for (i=0; i<x->voices; i++)
	{
		rnd = rand()/((double)RAND_MAX + 1);
		if (rnd < DEF_PROB_MUTATION)
		{
			rnd = rand()/((double)RAND_MAX + 1) * POSSIBLE_NOTES;
			kid[i]=x->target_notes[(int)rnd];
		}
	}
}

typedef struct fitness_list_element_t 
{
	int index;
	int fitness;
} fitness_list_element;

#define FITNESS_LIST_COMPARATOR(e1, e2) (e1.fitness - e2.fitness)

void generate_voicing(t_harmonizer *x)
{
	fitness_list_element fitness_evaluations[MAX_POPULATION];
	int i, generation, mum, dad, winner;
	double rnd;
	//t_atom lista[VOICES];
	t_atom *lista;
	int *winner_notes;

	lista = malloc(sizeof(t_atom)*x->voices);
	winner_notes = malloc(sizeof(int)*x->voices);

	// inizialize tables of notes
	build_possible_notes_table(x);
	// inizialize population
	harmonizer_init_pop(x);
	// GA code
	for (generation=0; generation<GENERATIONS; generation++)
	{
		// i compute all the fitness
		for (i=0; i<MAX_POPULATION; i++)
		{
			fitness_evaluations[i].index=i;
			fitness_evaluations[i].fitness = fitness(x, x->population[i]);
		}
		// i sort the array
		SGLIB_ARRAY_SINGLE_QUICK_SORT(fitness_list_element, fitness_evaluations, MAX_POPULATION, FITNESS_LIST_COMPARATOR)

		// i kill half population
		// and use the survivors to create new genotypes
		for (i=0; i<(MAX_POPULATION/2); i++)
		{
			// create a new genotype
			// parents chosen randomly
			rnd = rand()/((double)RAND_MAX + 1);
			mum = MAX_POPULATION/2 + rnd*MAX_POPULATION/2;
			rnd = rand()/((double)RAND_MAX + 1);
			dad = MAX_POPULATION/2 + rnd*MAX_POPULATION/2;
			new_genotype(x, x->population[mum], x->population[dad], x->population[i]);
		}
		// repeat the process
	}
	// finally look for the winner
	// i compute all the fitness
	for (i=0; i<MAX_POPULATION; i++)
	{
		fitness_evaluations[i].index=i;
		fitness_evaluations[i].fitness = fitness(x, x->population[i]);
	}
	// i sort the array
	SGLIB_ARRAY_SINGLE_QUICK_SORT(fitness_list_element, fitness_evaluations, MAX_POPULATION, FITNESS_LIST_COMPARATOR)
	
	winner = fitness_evaluations[MAX_POPULATION-1].index;

	if (DEBUG)
		post("winner fitness = %i", fitness_evaluations[MAX_POPULATION-1].fitness);

	for (i=0;i<x->voices;i++)
	{
		winner_notes[i] = x->population[winner][i];
	}
	SGLIB_ARRAY_SINGLE_QUICK_SORT(int, winner_notes, x->voices, SGLIB_NUMERIC_COMPARATOR)

	for (i=0;i<x->voices;i++)
	{
		SETFLOAT(lista+i, winner_notes[i]);
	}

	// send output array to outlet
	outlet_anything(x->l_out,
                     gensym("list") ,
					 x->voices, 
					 lista);
	free(lista);
	free(winner_notes);
}

// if i want another voicing i can send a bang
static void harmonizer_bang(t_harmonizer *x) {
	generate_voicing(x);
}

// called when i send a list (with midi values)
void set_current_voices(t_harmonizer *x, t_symbol *sl, int argc, t_atom *argv)
{
	int i=0;
	int *input_voices;
	
	if (argc<x->voices)
	{
		error("insufficient notes sent!");
		return;
	}
	input_voices = malloc(sizeof(int)*x->voices);
	// fill input array with actual data sent to inlet
	for (i=0;i<x->voices;i++)
	{
		input_voices[i] = atom_getint(argv++);
	}
	SGLIB_ARRAY_SINGLE_QUICK_SORT(int, input_voices, x->voices, SGLIB_NUMERIC_COMPARATOR)
	for (i=0;i<x->voices;i++)
	{
		x->current_voices[i] = input_voices[i];
	}
	
	generate_voicing(x);
	free(input_voices);


}
// set current chord
void set_current(t_harmonizer *x, t_symbol *s) {
	x->current_chord.mode = string2mode(s->s_name);
	x->current_chord.note = string2note(s->s_name);
	if (DEBUG)
		post("harmonizer: set_current %s",s->s_name); 
}

// set target chord
void set_target(t_harmonizer *x, t_symbol *s) {
	x->target_chord.mode = string2mode(s->s_name);
	x->target_chord.note = string2note(s->s_name);
	if (DEBUG)
		post("harmonizer: set_target %s",s->s_name); 
}

//how any octaves should this chord be?
void set_wideness(t_harmonizer *x, t_floatarg f)
{
	if (f>=0)
		x->wideness = f;
}

// which note should the center note have ?
void set_center_note(t_harmonizer *x, t_floatarg f)
{
	if ((f>=LOWER_POSSIBLE_NOTE)&&(f<120))
		x->center_note = (int) f;
}

// which note should the center note have ?
void set_i_like_parallelism(t_harmonizer *x, t_floatarg f)
{
	float newval = f;
	if (newval<-1)
		newval = -1;
	if (newval>1)
		newval = 1;
	x->i_like_parallelism = newval;
}

// which note should the center note have ?
void set_small_intervals(t_harmonizer *x, t_floatarg f)
{
	float newval = f;
	if (newval<-1)
		newval = -1;
	if (newval>1)
		newval = 1;
	x->small_intervals = newval;
}

void set_voices(t_harmonizer *x, t_floatarg f)
{
	int newval = (int)  f;
	if (newval<1)
	{
		error("number of voices must be > 0 !");
		return;
	}
	x->voices = newval;
	harmonizer_free(x);
	harmonizer_allocate(x);
}

void print_help(t_harmonizer *x)
{
	post("");
	post("harmonizer is an external that builds voicing");
	post("takes chords name and outputs a list of midi notes");
	post("available commands:");
	post("current symbol: sets the current chord name (which chordwe are in)");
	post("target symbol: sets the target chord name (which chord we want to go to)");
	post("voices: sets the number of voices");
	post("wideness float: now many octaves wide should the next chord be? must be > than 0");
	post("set_center_note int: sets the desired center chord note, min 24 max 100");
	post("i_like_parallelism float: do I want parallelism? from -1 (I don't want them) to 1 (I like them), 0 means I don't care, default = -1");
	post("small_intervals float: do I want small intervals? from -1 (I don't want them) to 1 (I like them), 0 means I don't care, default = 1");
	post("this externalis part of the frank framework");
	post("authors: davide morelli, david casals");

}

void *harmonizer_new(t_symbol *s, int argc, t_atom *argv)
{
	int i;
	time_t a;
    t_harmonizer *x = (t_harmonizer *)pd_new(harmonizer_class);
	x->l_out = outlet_new(&x->x_obj, gensym("list"));
/*
	for (i=0; i<BUFFER_LENGHT; i++)
	{
		x->last[i] = harmonizer_note2gene(1,0,0,1);
	}
	*/
	srand(time(&a));
	x->center_note = DEF_CENTER_NOTE;
	x->wideness = DEF_WIDENESS;
	x->i_like_parallelism = -1; // by default we don't like them!
	x->small_intervals = 1; //by default we want small intervals
	x->voices = VOICES;
	if (argc>0) 
	{
		x->voices = atom_getintarg(0, argc, argv);
	}
	harmonizer_allocate(x);
    return (x);
}

void harmonizer_setup(void)
{
    harmonizer_class = class_new(gensym("harmonizer"), (t_newmethod)harmonizer_new,
        (t_method)harmonizer_free, sizeof(t_harmonizer), CLASS_DEFAULT, A_GIMME, 0);
    class_addbang(harmonizer_class, (t_method)harmonizer_bang);
	class_addmethod(harmonizer_class, (t_method)print_help, gensym("help"),0, 0);
	class_addmethod(harmonizer_class, (t_method)set_current, gensym("current"),A_SYMBOL, 0);
	class_addmethod(harmonizer_class, (t_method)set_target, gensym("target"),A_SYMBOL, 0);
//	class_addmethod(harmonizer_class, (t_method)harmonizer_fitness1_set, gensym("fitness1"), A_DEFFLOAT, 0);
	class_addlist(harmonizer_class, (t_method)set_current_voices);
	class_addmethod(harmonizer_class, (t_method)set_wideness, gensym("wideness"), A_DEFFLOAT, 0);
	class_addmethod(harmonizer_class, (t_method)set_center_note, gensym("center_note"), A_DEFFLOAT, 0);
	class_addmethod(harmonizer_class, (t_method)set_i_like_parallelism, gensym("i_like_parallelism"), A_DEFFLOAT, 0);
	class_addmethod(harmonizer_class, (t_method)set_small_intervals, gensym("small_intervals"), A_DEFFLOAT, 0);
	// set number of voices
	class_addmethod(harmonizer_class, (t_method)set_voices, gensym("voices"), A_DEFFLOAT, 0);
	

}
