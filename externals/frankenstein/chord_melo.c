/* 
BUGS:
- I had to comment out arrays re-display because it crashed my pd
- sometimes dest_played gets filled with junk

TODO:
- now rests can't be used! change chord_melo_seq_max_min
*/
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "m_pd.h"

#define BUFFER_LENGHT 16 // lunghezza dei buffers (quanti elementi nel pattern)
#define MAX_POPULATION 500
#define CHOIR 20
#define NOTES 128 // how many notes can I use ? MIDI !#define MAX_OCTAVES 3

#define DEF_PROB_CROSSOVER 0.9f
#define DEF_PROB_MUTATION 0.03f
#define REINSERT_SRC 2 // quanti reinserisco ad ogni ciclo usando il ritmo src
#define REINSERT_LAST 0 // quanti reinserisco ad ogni ciclo usando l'ultimo ritmo scelto

#define DEBUG 1 // messaggi di debug
#define DEBUG_VERBOSE 0 // messaggi di debug

static t_class *chord_melo_class;

// 1 gene of the genome
typedef struct _chord_melo_gene
{
	int chord_note; // 0-12
	int passing_note; // from -4 to +4
	int played; // 0 / 1
} chord_melo_gene;

// an interval between 2 genes
typedef struct _chord_melo_interval
{
	int direction; // -1/0/1 
	int chord_note; // 0-12
	int passing_note; // from -4 to +4
} chord_melo_interval;

typedef struct _chord_melo_critic
{
	double interval[12][9];
} chord_melo_critic;

// return the interval between 2 genes
chord_melo_interval chord_melo_Getchord_melo_interval(chord_melo_gene *g1, chord_melo_gene *g2)
{
	int note1, note2;
	chord_melo_interval res;
	if (g1->chord_note < g2->chord_note)
	{
		// ascending
		res.direction = 1;
	} else if (g1->chord_note > g2->chord_note)
	{
		// descending
		res.direction = -1;
	} else if (g1->passing_note == g2->passing_note)
	{
		// unison
		res.direction = 0;		
	} else
	{
		// passing note
		if (g1->passing_note < g2->passing_note)
		{
			// ascending
			res.direction = 1;
		} else
		{
			// descending
			res.direction = -1;
		}
	}
	res.chord_note = g1->chord_note - g2->chord_note;
	res.passing_note = g2->passing_note - g1->passing_note;
	return res;
}

// fills an array of intervals with the transition in a genome
void chord_melo_transitions(chord_melo_interval *res, chord_melo_gene *src)
{
	int i;
	//chord_melo_gene *last;
	//last = &src[0];
	for (i=1;i<BUFFER_LENGHT;i++)
	{
		res[i-1] = chord_melo_Getchord_melo_interval(&src[i-1], &src[i]);
		//last = &src[i];		
	}
}

// fills an array of integers to design the shape of a genome
// -1 = descending, 0=unisone, 1=ascending
void chord_melo_shape(int *res, chord_melo_gene *src)
{
	int i, tmp;
	chord_melo_gene *last;
	last = &src[0];
	for (i=1;i<BUFFER_LENGHT;i++)
	{
		tmp = chord_melo_Getchord_melo_interval(last, &src[i]).chord_note;
		if (tmp == 0)
		{
			tmp = chord_melo_Getchord_melo_interval(last, &src[i]).passing_note;
			if (abs(tmp) > 1)
				tmp = chord_melo_Getchord_melo_interval(last, &src[i]).direction;
		}
		//res[i-1] = chord_melo_Getchord_melo_interval(last, &src[i]).direction;
		res[i-1] = tmp;
		last = &src[i];
	}
	if (DEBUG_VERBOSE)
		post("shape: %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
		res[0], res[1],res[2],res[3],res[4],res[5],res[6],res[7],res[8],res[9],
		res[10],res[11],res[12],res[13],res[14]);

}

// fills a 4 gene long array with a sequence
// starting - max/min - max/min - ending
void chord_melo_seq_max_min(chord_melo_gene *seq,  chord_melo_gene *src)
{
	// return a 4 gene seq: 
	// seq[0] = starting
	// seq[1] = min/max
	// seq[2] = min/max
	// seq[3] = end
	int i;
	int min_pos=0;
	int max_pos=0;
	int first_pos=0;
	int last_pos=0;
	chord_melo_interval interv;
	chord_melo_gene curr_min = src[0];
	chord_melo_gene curr_max = src[0];
	for (i=1;i<BUFFER_LENGHT;i++)
	{
		// look for first
		//TODO
		// look for last
		//TODO
		// look for min
		interv = chord_melo_Getchord_melo_interval(&curr_min, &src[i]);
		if (interv.direction < 0)
		{
			min_pos = i;
			curr_min = src[i];
		}
		// look for max
		interv = chord_melo_Getchord_melo_interval(&curr_max, &src[i]);
		if (interv.direction > 0)
		{
			max_pos = i;
			curr_max = src[i];
		}
	}
	seq[0] = src[0];
	seq[3] = src[BUFFER_LENGHT-1];
	if (min_pos<max_pos)
	{
		seq[1] = src[min_pos];
		seq[2] = src[max_pos];
	} else
	{
		seq[2] = src[min_pos];
		seq[1] = src[max_pos];
	}


}

// return the shape of the min/max sequence
void chord_melo_higher_shape(int *res, chord_melo_gene *src)
{
	int i, tmp;
	chord_melo_gene *last;
	chord_melo_gene seq[4];
	// i get the max min sequence
	chord_melo_seq_max_min(seq, src);
	last = &seq[0];
	for (i=1;i<4;i++)
	{
		tmp = chord_melo_Getchord_melo_interval(last, &src[i]).chord_note;
		if (tmp == 0)
		{
			tmp = chord_melo_Getchord_melo_interval(last, &src[i]).passing_note;
			if (abs(tmp) > 1)
				tmp = chord_melo_Getchord_melo_interval(last, &src[i]).direction;
		}
		//res[i-1] = chord_melo_Getchord_melo_interval(last, &src[i]).direction;
		res[i-1] = tmp;

		//res[i-1] = chord_melo_Getchord_melo_interval(last, &seq[i]).direction;
		last = &seq[i];
	}

	if (DEBUG_VERBOSE)
		post("higher shape: %i %i %i %i",
		res[0], res[1],res[2],res[3]);
}

typedef struct _chord_melo
{
    t_object x_obj; // myself
    t_symbol *x_arrayname_src_note; // where i read the current pattern
    t_symbol *x_arrayname_src_octave; // where i read the current pattern
    t_symbol *x_arrayname_src_passing; // where i read the current pattern
	t_symbol *x_arrayname_src_played; // where i read the current pattern
    t_symbol *x_arrayname_dest_note; // where i put the computed pattern
	t_symbol *x_arrayname_dest_octave; // where i put the computed pattern
	t_symbol *x_arrayname_dest_passing; // where i put the computed pattern
	t_symbol *x_arrayname_dest_played;
	// tutti gli indici vanno da 0 a 1;
	float indice_fitness1;
	float indice_fitness2;
	float indice_fitness3;
	// la popolazione array di cromosomi
	chord_melo_gene population[MAX_POPULATION][BUFFER_LENGHT];
	float prob_crossover;
	float prob_mutation;
	chord_melo_gene last[BUFFER_LENGHT];
	chord_melo_gene last_src[BUFFER_LENGHT];
	int init;
	t_float *vecsrc_note;
	t_float *vecsrc_octave;
	t_float *vecsrc_passing;
	t_float *vecsrc_played;
	t_float *vecdest_note;
	t_float *vecdest_octave;
	t_float *vecdest_passing;
	t_float *vecdest_played;
	int tables_loaded;

} t_chord_melo;


// i use pointers to return more than 1 object at a time
void chord_melo_gene2note(chord_melo_gene thisgene, 
									 unsigned int *octave, 
									 unsigned int *note, 
									 int *alteration, 
									 unsigned int *played)
{
	*octave = thisgene.chord_note / 3;
	*note = thisgene.chord_note % 3;
	*alteration = thisgene.passing_note;
	*played = thisgene.played;
}

// passing note as a value between 0 and 2 (with octave)
chord_melo_gene chord_melo_note2gene(unsigned int octave,
											  unsigned int note,
											  int alteration,
											  unsigned int played)
{
	chord_melo_gene ris;
	ris.chord_note = note + 3*octave;
	ris.passing_note = alteration;
	ris.played = played;
	return ris;
}

// passing note as a value 0-12 (without octave)
chord_melo_gene chord_melo_note2geneB(unsigned int note,
											  int alteration,
											  unsigned int played)
{
	chord_melo_gene ris;
	ris.chord_note = note;
	ris.passing_note = alteration;
	ris.played = played;
	return ris;
}

// returns next passing note from src in the wanted direction, both chromatic or diatonic
chord_melo_gene chord_melo_get_next_passing(chord_melo_gene *src, int direction, int chromatic)
{
	int new_note;
	int new_octave;
	int new_passing;
	int origine_nota;
	int origine_ottava;
	int origine_passaggio;
	chord_melo_gene res;
	origine_nota = src->chord_note % 3;
	origine_ottava = src->chord_note / 3;
	origine_passaggio = src->passing_note;
	if (chromatic)
	{
		if (direction < 0)
		{
			new_octave = origine_ottava;
			new_note = origine_nota;
			new_passing = origine_passaggio - 1;
		} else
		{
			new_octave = origine_ottava;
			new_note = origine_nota;
			new_passing = origine_passaggio + 1;
		}
	} else
	{
		// diatonic
		if (direction < 0)
		{
			new_octave = origine_ottava;
			new_note = origine_nota;
			new_passing = origine_passaggio - 2;
		} else
		{
			new_octave = origine_ottava;
			new_note = origine_nota;
			new_passing = origine_passaggio + 2;
		}
	}
	// check notes
	if ((new_passing < -4 )&&(new_note==0))
	{
		new_passing += 5;
		new_note = 2;
		new_octave -= 1;
	}
	if ((new_passing < -3 )&&(new_note>0))
	{
		new_passing += 4;
		new_note -= 1;
	}
	if ((new_passing > 4 )&&(new_note==2))
	{
		new_passing -= 5;
		new_note = 0;
		new_octave += 1;
	}
	if ((new_passing > 3 )&&(new_note<2))
	{
		new_passing -= 4;
		new_note += 1;
	}
	if (new_octave < 0)
		new_octave=0;

	if (new_octave > 2)
		new_octave=2;

//	if (new_octave > MAX_OCTAVES)
//		new_octave=MAX_OCTAVES;

	res = chord_melo_note2gene(new_octave, new_note, new_passing, 1);
	return res;
}

void chord_melo_fill_critic(chord_melo_critic *critic, chord_melo_gene *src)
{
	int i, j;
	chord_melo_interval intervalli[BUFFER_LENGHT-1];
	for (i=0; i<12; i++)
	{
		for (j=0; j<9; j++)
		{
			critic->interval[i][j]=0;
		}
	}
	chord_melo_transitions(intervalli, src);
	for (i=0; i<(BUFFER_LENGHT-1); i++)
	{
		if (critic->interval[intervalli[i].chord_note][intervalli[i].passing_note] < 0)
					critic->interval[intervalli[i].chord_note][intervalli[i].passing_note] = 0;
		critic->interval[intervalli[i].chord_note][intervalli[i].passing_note] = 
			critic->interval[intervalli[i].chord_note][intervalli[i].passing_note] +1;
	}
}

double chord_melo_Todd_fitness1(chord_melo_critic *woman, chord_melo_gene *man)
{
	
	int i, j, res;
	chord_melo_interval intervalli[BUFFER_LENGHT-1];
	chord_melo_transitions(intervalli, man);
	res = 0;
	for (i=0; i<(BUFFER_LENGHT-1); i++)
	{
		res += woman->interval[intervalli[i].chord_note][intervalli[i].passing_note];
	}
	
	return res;
}

// fitness functions over higher shape
double chord_melo_fitness1(chord_melo_gene *woman, chord_melo_gene *man)
{
	int i, res;
	int hi_shape1[4];
	int hi_shape2[4];
	chord_melo_higher_shape(hi_shape1, woman);
	chord_melo_higher_shape(hi_shape2, man);
	res = 0;
	for (i=0; i<4; i++)
	{
		if (hi_shape1[i] * hi_shape2[i]>=0)
		{
			// same direction
			res += 12 - abs(hi_shape2[i] - hi_shape1[i]);
		} else if (hi_shape1[i] * hi_shape2[i]==0)
		{
			// one voice moving, the other no
		} else
		{
			// opposite direction
			res -= 5;
		}
	}

	return res;
}

// fitness functions over punctual shape
double chord_melo_fitness2(chord_melo_gene *woman, chord_melo_gene *man)
{
	int i, res;
	int shape1[BUFFER_LENGHT-1];
	int shape2[BUFFER_LENGHT-1];
	chord_melo_shape(shape1, woman);
	chord_melo_shape(shape2, man);
	res = 0;
	for (i=0; i<4; i++)
	{
		if (shape1[i] * shape2[i]>=0)
		{
			// same direction
			res += 12 - abs(shape2[i] - shape1[i]);
		} else if (shape1[i] * shape2[i]==0)
		{
			// one voice moving, the other no
		} else
		{
			// opposite direction
			res -= 5;
		}
	}
	return res;
}


void chord_melo_create_child(t_chord_melo *x, chord_melo_gene *woman, chord_melo_gene *man, chord_melo_gene *child)
{
		double rnd, rnd2;
		int split, i, j, tmp, direction;
		// crossover
		rnd = rand()/((double)RAND_MAX + 1);
		if (rnd < x->prob_crossover)
		{
			rnd = rand()/((double)RAND_MAX + 1);

				// vertical split
				//split =(int) ( rnd * BUFFER_LENGHT); // da 0 a MAX_POPULATION
				split = rand() % BUFFER_LENGHT;
				for (i=0; i< split; i++)
				{
					child[i] = woman[i];
				}
				for (i=split; i<BUFFER_LENGHT; i++)
				{
					child[i] = man[i];
				}
		}else
		{
			for (i=0; i< BUFFER_LENGHT; i++)
			{
				child[i] = woman[i];
			}
		}
		// mutation
		
		for (i=0; i< BUFFER_LENGHT; i++)
		{
			rnd = rand()/((double)RAND_MAX + 1);
			if (rnd < x->prob_mutation)
			{
				rnd = rand()/((double)RAND_MAX + 1);
				if (rnd < 0.4)
				{
					int prec=6;
					int tmp_oct;
					int tmp_note;
					int tmp_alt;
					int tmp_played;
					int interval;
					// new chord note
					int nuovanota = 0;
					int nuovaplayed=0;
					if (i>0)
					{
						chord_melo_gene2note(child[i-1], &tmp_oct, &tmp_note, &tmp_alt, &tmp_played);
						prec = 3*tmp_oct + tmp_note;
						rnd2 = rand()/((double)RAND_MAX + 1);
						if (rnd2 < 0.3)
							interval = 1;
						else if (rnd2 < 0.6)
							interval = 2;
						else if (rnd2 < 0.8)
							interval = 3;
						else if (rnd2 < 0.9)
							interval = 4;
//						else if (rnd2 < 0.95)
//							interval = 5;
//						else if (rnd2 < 0.99)
//							interval = 6;
//						else 
//							interval = 7;
						rnd2 = rand()/((double)RAND_MAX + 1);
						if (rnd2 < 0.5)
							interval *= -1;

						nuovanota = prec + interval;

					} else
					{
						rnd2 = rand()/((double)RAND_MAX + 1);
						nuovanota = rnd2 * 12;
					}
					child[i] = chord_melo_note2gene(nuovanota / 3, nuovanota % 3, 0, 1);
				} else if (rnd < 0.8)
				{
					// diatonic passing note
					if (i>0)
					{
						if (child[i-1].passing_note == 0)
						{
							// only diatonic passing note if i come from a chord note!
							rnd2 = rand()/((double)RAND_MAX + 1);
							if (rnd2 < 0.5)
								direction = -1;
							else
								direction = 1;
							child[i] = chord_melo_get_next_passing(&child[i-1], direction, 0);
							// resolve passing note
							if (i<(BUFFER_LENGHT-1))
								child[i+1] = chord_melo_get_next_passing(&child[i], direction, 0);
							// shall I go on?
							if ((i<(BUFFER_LENGHT-2)) && 
								((child[i-1].chord_note==2 && direction==1)||
								(child[i-1].chord_note==0 && direction==-1)))
							{
								child[i+2] = chord_melo_get_next_passing(&child[i+1], direction, 0);
							}
								
						}
					}
				} else
				{
					// chromatic passing note
						if (child[i-1].passing_note == 0)
						{
							// only diatonic passing note if i come from a chord note!
							rnd2 = rand()/((double)RAND_MAX + 1);
							if (rnd2 < 0.5)
								direction = -1;
							else
								direction = 1;
							child[i] = chord_melo_get_next_passing(&child[i-1], direction, 1);
							// resolve passing note
							if (i<(BUFFER_LENGHT-1))
								child[i+1] = chord_melo_get_next_passing(&child[i], direction, 1);
							// shall I go on?
							if ((i<(BUFFER_LENGHT-2)) && 
								((child[i-1].chord_note==2 && direction==1)||
								(child[i-1].chord_note==0 && direction==-1)))
							{
								child[i+2] = chord_melo_get_next_passing(&child[i+1], direction, 1);
							}
								
						}
				}
				
			}
		}
		

		


}



// -----------------  normal external code ...


void chord_melo_init_pop(t_chord_melo *x)
{
	int i, j, tmp, tmp2, k;
	double rnd;
	for (i=0; i<MAX_POPULATION; i++)
	{
		for (j=0; j<BUFFER_LENGHT; j++)
		{
			//rnd = rand()/((double)RAND_MAX + 1);
			//tmp = rnd * NOTES;
			tmp = rand() % NOTES;
			//rnd = rand()/((double)RAND_MAX + 1);
			//tmp2 = rnd * 12;
			tmp2 = rand() % 12;
			x->population[i][j] = chord_melo_note2gene(tmp2 / 3, tmp2 % 3, 0, 1);

		}

	}
}


void chord_melo_init_pop2(t_chord_melo *x)
{
	int i, j, tmp, tmp2, k;
	double rnd;
	for (i=0; i<MAX_POPULATION; i++)
	{
		rnd = rand()/((double)RAND_MAX + 1);
		//if (rnd > 0.5)
		if (rnd > 1)
		{
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				//rnd = rand()/((double)RAND_MAX + 1);
				//tmp = rnd * NOTES;
				tmp = rand() % NOTES;
				//rnd = rand()/((double)RAND_MAX + 1);
				//tmp2 = rnd * 12;
				tmp2 = rand() % 12;
				x->population[i][j] = chord_melo_note2gene(tmp2 / 3, tmp2 % 3, 0, 1);

			}
		} else
		{
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				x->population[i][j] = x->last_src[j];
			}

		}

	}

	for (j=0; j<BUFFER_LENGHT; j++)
	{
		x->last[j] = x->last_src[j];
		if (DEBUG)
			post("init last[%] chord_note=%i passing_note=%i played=%i", j,
			x->last[j].chord_note, x->last[j].passing_note, x->last[j].played);
	//	x->last[j] = x->population[0][j];
	//	x->last[i] = chord_melo_note2gene(1,1,0,1);
	}

	x->init=1;

}


void chord_melo_init_buf(t_float *buf)
{
	int i;
	for (i=0; i<sizeof(buf); i++)
	{
		buf[i] = 0;
	}
}

void chord_melo_allocate_buffers(t_chord_melo *x)
{
//	x->buf_strum1 = (t_float *)getbytes(BUFFER_LENGHT * sizeof(t_float));
//	x->buf_strum2 = (t_float *)getbytes(BUFFER_LENGHT * sizeof(t_float));
//	chord_melo_init_buf(x->buf_strum1);
//	chord_melo_init_buf(x->buf_strum2);
	
}

void chord_melo_free(t_chord_melo *x)
{
//	freebytes(x->buf_strum1, sizeof(x->buf_strum1));
//	freebytes(x->buf_strum2, sizeof(x->buf_strum2));
}


void chord_melo_get_tables(t_chord_melo *x) {

	t_garray *arysrc_note;
	t_garray *arysrc_octave;
	t_garray *arysrc_passing;
	t_garray *arysrc_played;
	t_garray *arydest_note;
	t_garray *arydest_octave;
	t_garray *arydest_passing;
	t_garray *arydest_played;

	int vecsize;
	
		// load tables

	if (!(arysrc_note = (t_garray *)pd_findbyclass(x->x_arrayname_src_note, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_note->s_name);
	}
    else if (!garray_getfloatarray(arysrc_note, &vecsize, &(x->vecsrc_note)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_note->s_name);
	}
		else if (!(arysrc_octave = (t_garray *)pd_findbyclass(x->x_arrayname_src_octave, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_octave->s_name);
	}
    else if (!garray_getfloatarray(arysrc_octave, &vecsize, &(x->vecsrc_octave)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_octave->s_name);
	}
		else if (!(arysrc_passing = (t_garray *)pd_findbyclass(x->x_arrayname_src_passing, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_passing->s_name);
	}
    else if (!garray_getfloatarray(arysrc_passing, &vecsize, &(x->vecsrc_passing)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_passing->s_name);
	}
	else if (!(arysrc_played = (t_garray *)pd_findbyclass(x->x_arrayname_src_played, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_played->s_name);
	}
    else if (!garray_getfloatarray(arysrc_played, &vecsize, &(x->vecsrc_played)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_played->s_name);
	}
	  else 	if (!(arydest_note = (t_garray *)pd_findbyclass(x->x_arrayname_dest_note, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_note->s_name);
	}
    else if (!garray_getfloatarray(arydest_note, &vecsize, &(x->vecdest_note)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_note->s_name);
	} 
	else 	if (!(arydest_octave = (t_garray *)pd_findbyclass(x->x_arrayname_dest_octave, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_octave->s_name);
	}
    else if (!garray_getfloatarray(arydest_octave, &vecsize, &(x->vecdest_octave)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_octave->s_name);
	} 
	else 	if (!(arydest_passing = (t_garray *)pd_findbyclass(x->x_arrayname_dest_passing, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_passing->s_name);
	}
    else if (!garray_getfloatarray(arydest_passing, &vecsize, &(x->vecdest_passing)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_note->s_name);
	}
	 else 	if (!(arydest_played = (t_garray *)pd_findbyclass(x->x_arrayname_dest_played, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_played->s_name);
	}
    else if (!garray_getfloatarray(arydest_played, &vecsize, &(x->vecdest_played)))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_played->s_name);
	}
	else // I got arrays and data
	{
		// tutto ok
		x->tables_loaded=1;
	}
}

static void chord_melo_bang(t_chord_melo *x) {

	int i, j, vecsize, ntot, tmp, me;
	float prob, variatore;
	
	double rnd;
	int winner;
	double winner_fitness;
	double average_fitness;
	chord_melo_gene src_genome[BUFFER_LENGHT];
	chord_melo_gene figli[MAX_POPULATION][BUFFER_LENGHT];

	//chord_melo_critic src_critic;
	chord_melo_critic last_critic;

	if (x->tables_loaded == 0)
	{
		chord_melo_get_tables(x);
	}
	else // I got arrays and data
	{
		if (DEBUG)
			post("--------- starting process");

		if (DEBUG)
			post("building genome for the src melody:");

		srand((unsigned int)time((time_t *)NULL));

		// get src's genome
		for (i=0; i<BUFFER_LENGHT; i++)
		{
			src_genome[i] = chord_melo_note2gene(x->vecsrc_octave[i],
				x->vecsrc_note[i], x->vecsrc_passing[i], x->vecsrc_played[i]);
			x->last_src[i] = src_genome[i];
			//post("src melody: vecsrc_octave[i]=%f,vecsrc_note[i]=%f,vecsrc_passing[i]=%f", 
			//	vecsrc_octave[i], vecsrc_note[i], vecsrc_passing[i]);

		}
	//return;

		if (x->init==0)
			return;

		// uccido a caso REINSERT_SRC elementi e inserisco il ritmo src al loro posto
		for (i=0; i<REINSERT_SRC; i++)
		{
			//rnd = rand()/((double)RAND_MAX + 1);
			//me = (int) (rnd * MAX_POPULATION);
			me = rand() % MAX_POPULATION;
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				x->population[me][j]=src_genome[j];
			}
		}
		
		//return;

		// uccido a caso REINSERT_LAST elementi e inserisco il last al loro posto
		for (i=0; i<REINSERT_LAST; i++)
		{
			//rnd = rand()/((double)RAND_MAX + 1);
			//me = (int) (rnd * MAX_POPULATION);
			me = rand() % MAX_POPULATION;
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				x->population[me][j]=x->last[j];
			}
		}

		// metà sono donne, prese a caso
		for (i=0; i<(MAX_POPULATION/2); i++)
		{
			//chord_melo_critic this_chord_melo_critic;
			int winner=0;
			double winner_value=0;
			int men[CHOIR];
			chord_melo_gene figlio[BUFFER_LENGHT];
			double fitness1[CHOIR];
			double fitness2[CHOIR];
			double fitness3[CHOIR];
			double fitnessTOT[CHOIR];
			double fitnessTodd1[CHOIR];
			chord_melo_critic woman_critic;
			//rnd = rand()/((double)RAND_MAX + 1);
			//me =(int) ( rnd * MAX_POPULATION); // da 0 a MAX_POPULATION
			me = rand() % MAX_POPULATION;
			// me è la donna che valuta gli uomini
			
			if (DEBUG_VERBOSE)
				post("woman %i = %i %i %i %i", me, x->population[me][0], x->population[me][1], x->population[me][2], x->population[me][3]);

			chord_melo_fill_critic(&woman_critic, x->population[me]);
			
			for (j=0; j<CHOIR; j++)
			{
				//rnd = rand()/((double)RAND_MAX + 1);
				//tmp =(int) ( rnd * MAX_POPULATION); // da 0 a MAX_POPULATION
				tmp = rand() % MAX_POPULATION;
				// tmp è questo uomo
				men[j] = tmp;
				if (DEBUG_VERBOSE)
					post("i will evaluate man %i", tmp);
				fitness1[j]=chord_melo_fitness1(x->population[me], x->population[tmp]);
				fitness2[j]=chord_melo_fitness2(x->population[me], x->population[tmp]);
				fitnessTodd1[j]=chord_melo_Todd_fitness1(&woman_critic, x->population[tmp]);
				fitnessTOT[j]=fitness1[j] * (x->indice_fitness1)
					+ fitness2[j] * (x->indice_fitness2)
					+ fitnessTodd1[j] * (x->indice_fitness3);
				if (DEBUG_VERBOSE)
					post("man %i has fitness %i", tmp, fitnessTOT[j]);
				if (winner_value <= fitnessTOT[j])
				{
					winner = tmp;
					winner_value = fitnessTOT[j];
				}
			}
			// winner è il maschio migliore nel coro
			if (DEBUG_VERBOSE)
				post("ho scelto il maschio %i con fitness %i", winner, winner_value);
			// chord_melo_genero un figlio
			chord_melo_create_child(x, x->population[me], x->population[winner], figlio);
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				figli[i][j] = figlio[j];
			}
		}

		// uccido a caso metà popolazione e ci metto i nuovi nati
		for (i=0; i<(MAX_POPULATION/2); i++)
		{
			//rnd = rand()/((double)RAND_MAX + 1);
			//me =(int) ( rnd * MAX_POPULATION); // da 0 a MAX_POPULATION
			me = rand() % MAX_POPULATION;
			// me è chi deve morire

			for (j=0; j<BUFFER_LENGHT; j++)
			{
				x->population[me][j] = figli[i][j];
			}
		}

		// prendo il più adatto rispetto all'ultimo ritmo suonato
		winner = 0;
		winner_fitness = 0;
		average_fitness = 0;
		
		//chord_melo_fill_critic(&src_critic, src_genome);
		chord_melo_fill_critic(&last_critic, x->last);

		for(i=0; i<MAX_POPULATION; i++)
		{
			double tmp1, tmp2, tmp3, tmpTOT, fitnessTodd1;
			//tmp1 = chord_melo_fitness1(src_genome, x->population[i]);
			//tmp2 = chord_melo_fitness2(src_genome, x->population[i]);
			//fitnessTodd1=chord_melo_Todd_fitness1(&src_critic, x->population[i]);
			tmp1 = chord_melo_fitness1(x->last, x->population[i]);
			tmp2 = chord_melo_fitness2(x->last, x->population[i]);
			fitnessTodd1=chord_melo_Todd_fitness1(&last_critic, x->population[i]);
			tmpTOT = tmp1 * (x->indice_fitness1) + 
				tmp2 * (x->indice_fitness2) + 
				fitnessTodd1 * (x->indice_fitness3);

			//post("%i fitness = %i", i, tmpTOT);
			
			if (tmpTOT >= winner_fitness)
			{
				winner_fitness = tmpTOT;
				winner = i;
			}
			average_fitness += tmpTOT;
		}
		average_fitness = average_fitness / MAX_POPULATION;

		if (DEBUG)
			post("winner is number %i with fitness=%d, average fitness = %d", winner, winner_fitness, average_fitness); 
			
		for (i=0; i<BUFFER_LENGHT; i++)
		{
			int note, played, octave, passing;
			// copio il vincitore in x->last
			x->last[i] = x->population[winner][i];
			// scrivo i buffer in uscita
			chord_melo_gene2note(x->population[winner][i],
				&octave, &note, &passing, &played);
			x->vecdest_note[i]=note;				
			x->vecdest_octave[i]=octave;				
			x->vecdest_passing[i]=passing;				
			x->vecdest_played[i]=(played > 0 ? 1 : 0);
			if (DEBUG)
				post("winner[%i] chord_note=%i, octave=%i, passing=%i, played=%i",
				i, note, octave, passing, played);
		}

		//return;

		// redraw the arrays
/*		garray_redraw(arydest_note);
		garray_redraw(arydest_octave);
		garray_redraw(arydest_passing);
		garray_redraw(arydest_played);
*/

	}
}

void chord_melo_src_note(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_src_note = s;
}

void chord_melo_src_octave(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_src_octave = s;
}

void chord_melo_src_passing(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_src_passing = s;
}

void chord_melo_src_played(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_src_played = s;
}

void chord_melo_dest_note(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_dest_note = s;
}

void chord_melo_dest_octave(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_dest_octave = s;
}

void chord_melo_dest_passing(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_dest_passing = s;
}

void chord_melo_dest_played(t_chord_melo *x, t_symbol *s) {
    x->x_arrayname_dest_played = s;
}


void chord_melo_fitness1_set(t_chord_melo *x, t_floatarg f)
{
  x->indice_fitness1 = f;
 }

void chord_melo_fitness2_set(t_chord_melo *x, t_floatarg f)
{
  x->indice_fitness2 = f;
}

void chord_melo_fitness3_set(t_chord_melo *x, t_floatarg f)
{
  x->indice_fitness3 = f;
}

void chord_melo_crossover_set(t_chord_melo *x, t_floatarg f)
{
  x->prob_crossover = f;
}

void chord_melo_mutation_set(t_chord_melo *x, t_floatarg f)
{
  x->prob_mutation = f;
}

/*
void chord_melo_init(t_chord_melo *x, t_symbol *s) {
	chord_melo_init_pop2(x);
}
*/

void *chord_melo_new(t_symbol *s, int argc, t_atom *argv)
{
	int i;
	time_t a;
    t_chord_melo *x = (t_chord_melo *)pd_new(chord_melo_class);
	chord_melo_allocate_buffers(x);
	chord_melo_init_pop(x);
	// inizializzo gli indici
	x->indice_fitness1=0;
	x->indice_fitness2=0;
	x->indice_fitness3=0;
	x->prob_crossover = DEF_PROB_CROSSOVER;
	x->prob_mutation = DEF_PROB_MUTATION;
	x->init=0;
	x->tables_loaded=0;
	for (i=0; i<BUFFER_LENGHT; i++)
	{
		x->last[i] = chord_melo_note2gene(1,0,0,1);
	}
	srand(time(&a));

	if (argc>0) 
	{
		x->x_arrayname_src_note = atom_getsymbolarg(0, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_src_note=%s",x->x_arrayname_src_note->s_name); 
	} 
	if (argc>1) 
	{
		x->x_arrayname_src_octave = atom_getsymbolarg(1, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_src_octave=%s",x->x_arrayname_src_octave->s_name); 
	} 
	if (argc>2) 
	{
		x->x_arrayname_src_passing = atom_getsymbolarg(2, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_src_passing=%s",x->x_arrayname_src_passing->s_name); 
	}
	if (argc>3) 
	{
		x->x_arrayname_src_played = atom_getsymbolarg(3, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_src_played=%s",x->x_arrayname_src_played->s_name); 
	} 
	if (argc>4) 
	{
		x->x_arrayname_dest_note = atom_getsymbolarg(4, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_dest_note=%s",x->x_arrayname_dest_note->s_name); 
	}
	if (argc>5) 
	{
		x->x_arrayname_dest_octave = atom_getsymbolarg(5, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_dest_octave=%s",x->x_arrayname_dest_octave->s_name); 
	}
	if (argc>6) 
	{
		x->x_arrayname_dest_passing = atom_getsymbolarg(6, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_dest_passing=%s",x->x_arrayname_dest_passing->s_name); 
	}
	if (argc>7) 
	{
		x->x_arrayname_dest_played = atom_getsymbolarg(7, argc, argv);
		if (DEBUG)
			post("x->x_arrayname_dest_played=%s",x->x_arrayname_dest_played->s_name); 
	}
    return (x);
}

void chord_melo_setup(void)
{
    chord_melo_class = class_new(gensym("chord_melo"), (t_newmethod)chord_melo_new,
        (t_method)chord_melo_free, sizeof(t_chord_melo), CLASS_DEFAULT, A_GIMME, 0);
    class_addbang(chord_melo_class, (t_method)chord_melo_bang);
    class_addmethod(chord_melo_class, (t_method)chord_melo_src_note, gensym("src_note"),A_SYMBOL, 0);
    class_addmethod(chord_melo_class, (t_method)chord_melo_src_octave, gensym("src_octave"),A_SYMBOL, 0);
    class_addmethod(chord_melo_class, (t_method)chord_melo_src_passing, gensym("src_passing"),A_SYMBOL, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_src_played, gensym("src_played"),A_SYMBOL, 0);
    class_addmethod(chord_melo_class, (t_method)chord_melo_dest_note, gensym("dest_note"),A_SYMBOL, 0);
    class_addmethod(chord_melo_class, (t_method)chord_melo_dest_octave, gensym("dest_octave"),A_SYMBOL, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_dest_passing, gensym("dest_passing"),A_SYMBOL, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_dest_played, gensym("dest_played"),A_SYMBOL, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_fitness1_set, gensym("fitness1"), A_DEFFLOAT, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_fitness2_set, gensym("fitness2"), A_DEFFLOAT, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_fitness3_set, gensym("fitness3"), A_DEFFLOAT, 0);
    class_addmethod(chord_melo_class, (t_method)chord_melo_init_pop2, gensym("init"),A_SYMBOL, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_mutation_set, gensym("mutation"), A_DEFFLOAT, 0);
	class_addmethod(chord_melo_class, (t_method)chord_melo_crossover_set, gensym("crossover"), A_DEFFLOAT, 0);

}
