/* 

*/
#include <stdlib.h>
#include <time.h>
#include "m_pd.h"

#define BUFFER_LENGHT 16 // lunghezza dei buffers (quanti elementi nel pattern)
#define MAX_POPULATION 100
#define CHOIR 20
#define NUM_STRUM 4 // quanti strumenti uso , max 8

#define DEF_PROB_CROSSOVER 0.9f
#define DEF_PROB_MUTATION 0.025f
#define REINSERT_SRC 1 // quanti reinserisco ad ogni ciclo usando il ritmo src
#define REINSERT_LAST 0 // quanti reinserisco ad ogni ciclo usando il ritmo src

#define DEBUG 0 // messaggi di debug

static t_class *GArhythm_class;


typedef struct _GArhythm
{
    t_object x_obj; // myself
    t_symbol *x_arrayname_src_strum1; // where i read the current pattern
    t_symbol *x_arrayname_src_strum2; // where i read the current pattern
    t_symbol *x_arrayname_src_strum3; // where i read the current pattern
    t_symbol *x_arrayname_src_strum4; // where i read the current pattern
	t_symbol *x_arrayname_dest_strum1; // where i put the computed pattern
	t_symbol *x_arrayname_dest_strum2; // where i put the computed pattern
    t_symbol *x_arrayname_dest_strum3; // where i put the computed pattern
	t_symbol *x_arrayname_dest_strum4; // where i put the computed pattern
	//t_float *buf_strum1; // buffer strum1o
	//t_float *buf_strum2; //  buffer alto
	// tutti gli indici vanno da 0 a 1;
	float indice_variazione; // quanto cambio dalla battuta precedente
	float indice_riempimento; // quanto voglio fitto il pattern risultante
	float indice_aderenza; // quanto simile al ritmo sorgente devo essere
	// la popolazione array di cromosomi
	char population[MAX_POPULATION][BUFFER_LENGHT];
	float prob_crossover;
	float prob_mutation;
	char last[BUFFER_LENGHT];
	int reinsert_src;
	int reinsert_last;
} t_GArhythm;

void GArhythm_init_pop(t_GArhythm *x)
{
	int i, j, tmp, k;
	double rnd;

	for (i=0; i<MAX_POPULATION; i++)
	{
		for (j=0; j<BUFFER_LENGHT; j++)
		{
			tmp = 0;
			for (k=0; k<NUM_STRUM; k++)
			{
				rnd = rand()/((double)RAND_MAX + 1);
				if (rnd > 0.5)
				{
					tmp =  tmp + (1<<k); // da 0 a MAX_POPULATION
				} 
			}
			x->population[i][j]=tmp;
		}
		if (DEBUG)
			post("inizializzo population[%i] = %i%i%i%i", 
				i, 
				x->population[i][0],
				x->population[i][1],
				x->population[i][2],
				x->population[i][3]
				); 
	}
	
}

void GArhythm_reinit_pop(t_GArhythm *x)
{
	int i, j, vecsize, ntot, tmp, me;
	float prob, variatore;
	t_garray *arysrc_strum1;
	t_garray *arysrc_strum2;
	t_garray *arysrc_strum3;
	t_garray *arysrc_strum4;
	t_float *vecsrc_strum1;
	t_float *vecsrc_strum2;
	t_float *vecsrc_strum3;
	t_float *vecsrc_strum4;

	// load tables

	if (!(arysrc_strum1 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum1, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum1->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum1, &vecsize, &vecsrc_strum1))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum1->s_name);
	} 
	else if (!(arysrc_strum2 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum2, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum2->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum2, &vecsize, &vecsrc_strum2))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum2->s_name);
	}
	else if (!(arysrc_strum3 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum3, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum3->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum3, &vecsize, &vecsrc_strum3))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum3->s_name);
	}
	else if (!(arysrc_strum4 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum4, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum4->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum4, &vecsize, &vecsrc_strum4))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum4->s_name);
	}
	
	for (i=0; i<MAX_POPULATION; i++)
		{
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				char c = 0x00;
				if (vecsrc_strum1[j])
					c = c | 0x01;
				if (vecsrc_strum2[j])
					c = c | (0x01 << 1);
				if (vecsrc_strum3[j])
					c = c | (0x01 << 2);
				if (vecsrc_strum4[j])
					c = c | (0x01 << 3);
				x->population[i][j]=c;
			}
		}
}

void GArhythm_init_buf(t_float *buf)
{
	int i;
	for (i=0; i<sizeof(buf); i++)
	{
		buf[i] = 0;
	}
}

void GArhythm_allocate_buffers(t_GArhythm *x)
{
//	x->buf_strum1 = (t_float *)getbytes(BUFFER_LENGHT * sizeof(t_float));
//	x->buf_strum2 = (t_float *)getbytes(BUFFER_LENGHT * sizeof(t_float));
//	GArhythm_init_buf(x->buf_strum1);
//	GArhythm_init_buf(x->buf_strum2);
	
}

void GArhythm_free(t_GArhythm *x)
{
//	freebytes(x->buf_strum1, sizeof(x->buf_strum1));
//	freebytes(x->buf_strum2, sizeof(x->buf_strum2));
}

// returns fitness: how similar are man and woman
static double GArhythm_evaluate_fitness1(char *woman, char *man)
{
	int res=0;
	int max = BUFFER_LENGHT*2;
	int i;
	/*
	// commenting here I allow beat swapped rhythms to be considered as identical
	for (i=0; i<BUFFER_LENGHT; i++)
	{
		if (woman[i] == man[i])
			res++;
	}
	*/
	for (i=0; i<BUFFER_LENGHT; i++)
	{
		if ((woman[i]!= 0) && (man[i] != 0))
			res++;
		if ((woman[i]== 0) && (man[i] == 0))
			res++;
	}
	return (float) ((float) res) / ((float) max);
}

// riempimento
static double GArhythm_evaluate_fitness2(char *woman, char *man)
{
	int i, j, max;
	double ris=0;
	max = BUFFER_LENGHT * NUM_STRUM;
	for (i=0; i<BUFFER_LENGHT; i++)
	{
		for (j=0; j<NUM_STRUM; j++)
		{
			if (man[i] & (0x01<<j))
				ris++;
		}
	}
	return (float) ((float) ris) / ((float) max);

}

// similarities TODO
static double GArhythm_evaluate_fitness3(char *woman, char *man)
{
	// TODO: DUMMY, not working!
	int i;
	short int global1[BUFFER_LENGHT];
	short int global2[BUFFER_LENGHT];
	for (i=0; i<BUFFER_LENGHT; i++)
	{
		if (woman[i] != 0x00)
			global1[i]=1;
		else
			global1[i]=0;
		if (man[i] != 0x00)
			global2[i]=1;
		else
			global2[i]=0;

	}
	// TODO
	return 0;
}

// penalyze too many consecutive beats
static double GArhythm_evaluate_fitness4(char *woman, char *man)
{
	int i, j, max, curr_consecutivi, max_consecutivi, tot_consecutivi;
	double ris=0;
	curr_consecutivi = max_consecutivi = tot_consecutivi = 0;
	max = (BUFFER_LENGHT-1) * NUM_STRUM;
	for (j=0; j<NUM_STRUM; j++)
	{
		i=1; 
		curr_consecutivi = 0;
		while(i<BUFFER_LENGHT)
		{
			if ((man[i] & (0x01<<j)) && (man[i-1] & (0x01<<j)))
			{
				// here is an event
				//is it the first?
				curr_consecutivi++;
				if (curr_consecutivi>max_consecutivi)
					max_consecutivi = curr_consecutivi;
			} else
			{
				tot_consecutivi += curr_consecutivi;
				curr_consecutivi = 0;
			}
			i++;
		}
	}
	ris = (float) (((float) ris) / ((float) max));
	return 1 - ris;

}

static void GArhythm_create_child(t_GArhythm *x, char *woman, char *man, char *child)
{
		double rnd;
		int split, i, j, tmp;
		// crossover
		rnd = rand()/((double)RAND_MAX + 1);
		if (rnd < x->prob_crossover)
		{
			split =(int) ( rnd * BUFFER_LENGHT); // da 0 a MAX_POPULATION
			for (i=0; i< split; i++)
			{
				child[i] = woman[i];
			}
			for (i=split; i<BUFFER_LENGHT; i++)
			{
				child[i] = man[i];
			}
			// TODO: vertical split (some instr from mammy, some from daddy)
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
			// per ogni battito
			for (j=0; j<NUM_STRUM; j++)
			{
				// per ogni strumento
				tmp = child[i] & (0x01<<j); // tmp > 0 se è presente il battito là
				rnd = rand()/((double)RAND_MAX + 1);
				if (rnd < x->prob_mutation)
				{
					if (DEBUG)
						post("mutazione al battito %i allo strumento %i", i, j);
					if (tmp)
					{
						child[i] = child[i] & (~(0x01<<j)); // tolgo il bit
					} else
					{	
						child[i] = child[i] | (0x01<<j); // aggiungo il bit						
					}
				}
			}
		}
		if (DEBUG)
			post("generato figlio %i %i %i %i tra %i %i %i %i e %i %i %i %i, split=%i", 
				child[0], child[1], child[2], child[3],
				woman[0], woman[1], woman[2], woman[3], 
				man[0], man[1], man[2], man[3],
				split);

}

static void GArhythm_bang(t_GArhythm *x) {

	int i, j, vecsize, ntot, tmp, me;
	float prob, variatore;
	t_garray *arysrc_strum1;
	t_garray *arysrc_strum2;
	t_garray *arysrc_strum3;
	t_garray *arysrc_strum4;
	t_garray *arydest_strum1;
	t_garray *arydest_strum2;
	t_garray *arydest_strum3;
	t_garray *arydest_strum4;
	t_float *vecsrc_strum1;
	t_float *vecsrc_strum2;
	t_float *vecsrc_strum3;
	t_float *vecsrc_strum4;
	t_float *vecdest_strum1;
	t_float *vecdest_strum2;
	t_float *vecdest_strum3;
	t_float *vecdest_strum4;
	double rnd;
	int winner;
	double winner_fitness;

	char figli[MAX_POPULATION][BUFFER_LENGHT];

	// load tables

	if (!(arysrc_strum1 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum1, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum1->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum1, &vecsize, &vecsrc_strum1))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum1->s_name);
	} 
	else if (!(arysrc_strum2 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum2, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum2->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum2, &vecsize, &vecsrc_strum2))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum2->s_name);
	}
	else if (!(arysrc_strum3 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum3, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum3->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum3, &vecsize, &vecsrc_strum3))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum3->s_name);
	}
	else if (!(arysrc_strum4 = (t_garray *)pd_findbyclass(x->x_arrayname_src_strum4, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_src_strum4->s_name);
	}
    else if (!garray_getfloatarray(arysrc_strum4, &vecsize, &vecsrc_strum4))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_src_strum4->s_name);
	}
	  else 	if (!(arydest_strum1 = (t_garray *)pd_findbyclass(x->x_arrayname_dest_strum1, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_strum1->s_name);
	}
    else if (!garray_getfloatarray(arydest_strum1, &vecsize, &vecdest_strum1))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_strum1->s_name);
	}
	 else 	if (!(arydest_strum2 = (t_garray *)pd_findbyclass(x->x_arrayname_dest_strum2, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_strum2->s_name);
	}
    else if (!garray_getfloatarray(arydest_strum2, &vecsize, &vecdest_strum2))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_strum2->s_name);
	}
	 else 	if (!(arydest_strum3 = (t_garray *)pd_findbyclass(x->x_arrayname_dest_strum3, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_strum3->s_name);
	}
    else if (!garray_getfloatarray(arydest_strum3, &vecsize, &vecdest_strum3))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_strum3->s_name);
	}
	 else 	if (!(arydest_strum4 = (t_garray *)pd_findbyclass(x->x_arrayname_dest_strum4, garray_class)))
	{
        pd_error(x, "%s: no such array", x->x_arrayname_dest_strum4->s_name);
	}
    else if (!garray_getfloatarray(arydest_strum4, &vecsize, &vecdest_strum4))
	{
		pd_error(x, "%s: bad template for tabwrite", x->x_arrayname_dest_strum4->s_name);
	}
	else // I got arrays and data
	{
		// vecdest_strum2 e _strum1 contengono i valori in float degli array
		if (DEBUG)
			post("--------- starting process");
	
		// uccido a caso REINSERT_SRC elementi e inserisco il ritmo src al loro posto
		for (i=0; i<x->reinsert_src; i++)
		{
			rnd = rand()/((double)RAND_MAX + 1);
			me = (int) (rnd * MAX_POPULATION);
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				char c = 0x00;
				if (vecsrc_strum1[j])
					c = c | 0x01;
				if (vecsrc_strum2[j])
					c = c | (0x01 << 1);
				if (vecsrc_strum3[j])
					c = c | (0x01 << 2);
				if (vecsrc_strum4[j])
					c = c | (0x01 << 3);
				x->population[me][j]=c;
			}
		}
		// uccido a caso REINSERT_LAST elementi e inserisco il last al loro posto
		for (i=0; i<x->reinsert_last; i++)
		{
			rnd = rand()/((double)RAND_MAX + 1);
			me = (int) (rnd * MAX_POPULATION);
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				x->population[me][j]=x->last[j];
			}
		}

		// metà sono donne, prese a caso
		for (i=0; i<(MAX_POPULATION/2); i++)
		{
			int winner=CHOIR;
			int winner_value=0;
			int men[CHOIR];
			char figlio[BUFFER_LENGHT];
			double fitness1[CHOIR];
			double fitness2[CHOIR];
			double fitness3[CHOIR];
			double fitnessTOT[CHOIR];
			rnd = rand()/((double)RAND_MAX + 1);
			me =(int) ( rnd * MAX_POPULATION); // da 0 a MAX_POPULATION
			// me è la donna che valuta gli uomini

			if (DEBUG)
				post("woman %i = %i %i %i %i", me, x->population[me][0], x->population[me][1], x->population[me][2], x->population[me][3]);

			for (j=0; j<CHOIR; j++)
			{
				rnd = rand()/((double)RAND_MAX + 1);
				tmp =(int) ( rnd * MAX_POPULATION); // da 0 a MAX_POPULATION
				// tmp è questo uomo
				men[j] = tmp;
				fitness1[j]=GArhythm_evaluate_fitness1(x->population[me], x->population[tmp]);
				fitness2[j]=GArhythm_evaluate_fitness2(x->population[me], x->population[tmp]);
				fitness3[j]=GArhythm_evaluate_fitness3(x->population[me], x->population[tmp]);
				fitnessTOT[j]=fitness1[j] * (x->indice_aderenza) 
					+ fitness2[j] * (x->indice_riempimento) 
					+ (1 - fitness2[j]) * (1-(x->indice_riempimento)) 
					+ fitness3[j] * (x->indice_variazione)
					+ GArhythm_evaluate_fitness4(x->population[me], x->population[tmp]);
				if (winner_value <= fitnessTOT[j])
				{
					winner = tmp;
					winner_value = fitnessTOT[j];
				}
			}
			// winner è il maschio migliore nel coro
			if (DEBUG)
				post("ho scelto il maschio %i", winner);
			// genero un figlio
			GArhythm_create_child(x, x->population[me], x->population[winner], figlio);
			for (j=0; j<BUFFER_LENGHT; j++)
			{
				figli[i][j] = figlio[j];
			}
		}

		// uccido a caso metà popolazione e ci metto i nuovi nati
		for (i=0; i<(MAX_POPULATION/2); i++)
		{
			rnd = rand()/((double)RAND_MAX + 1);
			me =(int) ( rnd * MAX_POPULATION); // da 0 a MAX_POPULATION
			// me è chi deve morire

			for (j=0; j<BUFFER_LENGHT; j++)
			{
				x->population[me][j] = figli[i][j];
			}
		}

		// prendo il più adatto rispetto all'ultimo ritmo suonato
		winner = 0;
		winner_fitness = 0;
		for(i=0; i<BUFFER_LENGHT; i++)
		{
			double tmp1, tmp2, tmp3, tmpTOT;
			tmp1 = GArhythm_evaluate_fitness1(x->last, x->population[i]);
			tmp2 = GArhythm_evaluate_fitness2(x->last, x->population[i]);
			tmp3 = GArhythm_evaluate_fitness3(x->last, x->population[i]);
			tmpTOT = tmp1 * (x->indice_aderenza) 
					+ tmp2 * (x->indice_riempimento) 
					+ (1-tmp2) * (1-(x->indice_riempimento)) 
					+ tmp3 * (x->indice_variazione)
					+ GArhythm_evaluate_fitness4(x->last, x->population[i]);
			if (tmpTOT >= winner_fitness)
			{
				winner_fitness = tmpTOT;
				winner = i;
			}
		}
			
		for (i=0; i<BUFFER_LENGHT; i++)
		{
			// copio il vincitor ein x->last
			x->last[i] = x->population[winner][i];
			// scrivo i buffer in uscita
			vecdest_strum1[i]=((x->population[winner][i] & (0x01<<0)) ? 1 : 0);				
			vecdest_strum2[i]=((x->population[winner][i] & (0x01<<1)) ? 1 : 0);				
			vecdest_strum3[i]=((x->population[winner][i] & (0x01<<2)) ? 1 : 0);				
			vecdest_strum4[i]=((x->population[winner][i] & (0x01<<3)) ? 1 : 0);				
		}

		// redraw the arrays
		//garray_redraw(arysrc);
		garray_redraw(arydest_strum1);
		garray_redraw(arydest_strum2);
		garray_redraw(arydest_strum3);
		garray_redraw(arydest_strum4);


	}
}
/*
static void GArhythm_src(t_GArhythm *x, t_symbol *s) {
    x->x_arrayname_src = s;
}
*/

static void GArhythm_variazione_set(t_GArhythm *x, t_floatarg f)
{
  x->indice_variazione = f;
 }

static void GArhythm_aderenza_set(t_GArhythm *x, t_floatarg f)
{
  x->indice_aderenza = f;
}

static void GArhythm_riempimento_set(t_GArhythm *x, t_floatarg f)
{
  x->indice_riempimento = f;
}

static void GArhythm_crossover_set(t_GArhythm *x, t_floatarg f)
{
  x->prob_crossover = f;
}

static void GArhythm_mutation_set(t_GArhythm *x, t_floatarg f)
{
  x->prob_mutation = f;
}

static void GArhythm_reinsert_src_set(t_GArhythm *x, t_floatarg f)
{
	if (f>=0)
		x->reinsert_src = (int) f;
}

static void GArhythm_reinsert_last_set(t_GArhythm *x, t_floatarg f)
{
	if (f>=0)
		x->reinsert_last = (int) f;
}

static void GArhythm_prob_crossover_set(t_GArhythm *x, t_floatarg f)
{
	if (f<=1 && f>=0)
		x->prob_crossover = f;
}

static void GArhythm_prob_mutation_set(t_GArhythm *x, t_floatarg f)
{
	if (f<=1 && f>=0) 
		x->prob_mutation = f;
}

static void GArhythm_help(t_GArhythm *x)
{
	post("");
	post("");
	post("GArhythm");
	post("");
	post("a rhythm generator/variatior that uses co-evolving Genetic Algorithms");
	post("at the moment it only works with 16 step measure, 1 measure rhythms, it needs 4 arrays as input rhythms and outputs its rhythms on 4 arrays");
	post("");

	post("global usage hints");
	post("you must provide 8 arguments: the first 4 are the names of arrays with src rhythms, the second 4 are names of arrays where GArhythm will put its output");
	post("send a bang each time you want a new population (and a new rhythm) to be evaluated");
	post("");
	post("available commands");
	post("reinit: initialize the population with the content of the src arrays");
	post("variazione float: sets the index of wanted variation between the last proposed rhythm and the next one (from 0 to 1)");
	post("aderenza float: sets the index of wanted closeness between the current src rhythm and proposed one (from 0 to 1)");
	post("riempimento float: set 0 if you want sparse rhythms, 1 if you want a rhythm full of events");
	post("reinsert_src int: how many times the src rhythms will be randomly copied in the population before breeding");
	post("reinsert_last int: how many times the last rhythms will be randomly copied in the population before breeding");
	post("prob_crossover float: sets the crossover probability. default is %f", DEF_PROB_CROSSOVER);
	post("prob_mutation float: sets the mutation probability, default is %f", DEF_PROB_MUTATION);

}


static void *GArhythm_new(t_symbol *s, int argc, t_atom *argv)
{
    t_GArhythm *x = (t_GArhythm *)pd_new(GArhythm_class);
	GArhythm_allocate_buffers(x);
	GArhythm_init_pop(x);
	// inizializzo gli indici
	x->indice_variazione=0;
	x->indice_riempimento=0;
	x->indice_aderenza=0;
	x->prob_crossover = DEF_PROB_CROSSOVER;
	x->prob_mutation = DEF_PROB_MUTATION;
	x->reinsert_src=REINSERT_SRC;
	x->reinsert_last=REINSERT_LAST;

	srand( (unsigned)time( NULL ) );
	
	if (argc>0) 
	{
		x->x_arrayname_src_strum1 = atom_getsymbolarg(0, argc, argv);
	} 
	if (argc>1) 
	{
		x->x_arrayname_src_strum2 = atom_getsymbolarg(1, argc, argv);
	} 
	if (argc>2) 
	{
		x->x_arrayname_src_strum3 = atom_getsymbolarg(2, argc, argv);
	} 
	if (argc>3) 
	{
		x->x_arrayname_src_strum4 = atom_getsymbolarg(3, argc, argv);
	} 
	if (argc>4) 
	{
		x->x_arrayname_dest_strum1 = atom_getsymbolarg(4, argc, argv);
	}
	if (argc>5) 
	{
		x->x_arrayname_dest_strum2 = atom_getsymbolarg(5, argc, argv);
	}
	if (argc>6) 
	{
		x->x_arrayname_dest_strum3 = atom_getsymbolarg(6, argc, argv);
	}
	if (argc>7) 
	{
		x->x_arrayname_dest_strum4 = atom_getsymbolarg(7, argc, argv);
	}

    return (x);
}

void GArhythm_setup(void)
{
    GArhythm_class = class_new(gensym("GArhythm"), (t_newmethod)GArhythm_new,
        (t_method)GArhythm_free, sizeof(t_GArhythm), CLASS_DEFAULT, A_GIMME, 0);
    class_addbang(GArhythm_class, (t_method)GArhythm_bang);
//    class_addmethod(GArhythm_class, (t_method)GArhythm_src, gensym("src"),A_SYMBOL, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_variazione_set, gensym("variazione"), A_DEFFLOAT, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_riempimento_set, gensym("riempimento"), A_DEFFLOAT, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_aderenza_set, gensym("aderenza"), A_DEFFLOAT, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_reinit_pop, gensym("reinit"), 0, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_reinsert_src_set, gensym("reinsert_src"), A_DEFFLOAT, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_reinsert_last_set, gensym("reinsert_last"), A_DEFFLOAT, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_prob_crossover_set, gensym("prob_crossover"), A_DEFFLOAT, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_prob_mutation_set, gensym("prob_mutation"), A_DEFFLOAT, 0);
	class_addmethod(GArhythm_class, (t_method)GArhythm_help, gensym("help"), 0, 0);
}
