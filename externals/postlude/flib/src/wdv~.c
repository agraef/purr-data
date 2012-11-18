/* flib - PD library for feature extraction 
Copyright (C) 2005  Jamie Bullock

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/



/* calculates the wavelet dispersion vector from the output of dwt from creb*/

/* First argument gives the DSP block size, second argument gives the amount of data reduction in the output vector as a percentage */

#include "flib.h"

#define ISPOWEROFTWO(n) (!((n-1) & n))


static t_class *wdv_class;

typedef struct _coeff {
	t_int scale;
	t_float val;
} t_coeff;

static void divisi(t_float *, t_coeff **, int , int );

typedef struct _wdv {
  t_object  x_obj;
  t_float f;
  t_coeff **ranktab;
  t_int **histo, *vector;
  t_int N, M, scales, vecsize, offset;
  t_outlet *out_vec;
} t_wdv;


static t_int compare(t_coeff *x, t_coeff *y){ /*sorts in descending order */
	if( x->val > y->val)
		return -1;
	else if (x->val < y->val)
		return 1;
	else
		return 0;
}

static t_int bitcount(t_int x){
	t_int i=0;
	while(!(x & 01)){
			i++;
			x >>= 1;
		}
	return i;
}

static void divisi(t_float *arrayin, t_coeff **arrayout, int N, int scales){
	
	t_int row, elements, nelements,  n, p, i, j, col;
	t_float scaling;
	row = N / 2;
	col = scales; 
	nelements = elements = row;
	n = N - 1;

	scaling = arrayin[0] / scales; /* hmmm - include this or not ? */

	for(i = 0; i < row; i++)
		arrayout[i][0].val = scaling; 
	/* Copy the scaling function to all rows */		

	while(col--){
		row = 0;
		for(i = 0; i < elements; i++, n--){
			p = nelements / elements;
			for(j = 0; j < p; j++)
				arrayout[row++][col].val = 
					arrayin[n] / (t_float)(scales - col);
			/* divide by reverse scale no. to avg over the block */
		}
		
	elements /= 2;
	}
	
}

static t_int *wdv_perform(t_int *w)
{
  t_sample *in = (t_sample *)(w[1]);
  t_int N = (t_int)(w[2]);
  t_wdv *x = (t_wdv *)(w[3]);
  t_int i, j, k, n, scale;
  t_atom atom_list_out[x->vecsize];
 
  
  /* ensure our blocksize matches block~ size */
  
  if(N != x->N){ 	  
	  post("First argument must equal DSP block size!");
	  return (w+4);
  }


  /* Zero histogram and populate scale entries  */
  
  for (i = 0; i < x->scales; i++)
	  memset(&x->histo[i][0], 0, x->scales * sizeof(t_int));

 
  for(i = 0; i < x->M; i++){
	  for(j = 0, scale = x->scales; j < x->scales; j++, scale--)
		  x->ranktab[i][j].scale = scale;
  }

  
  /* Tabulate data and average scales accross columns */
  
  divisi(&in[0], &x->ranktab[0], x->N, x->scales); 
 

  /*  Calulate the rank value for each scale in place. Array index gives rank */
	  
  for(i = 0; i < x->M; i++)
	qsort(&x->ranktab[i][0], x->scales, 
			sizeof(t_coeff), (void *)compare);
  


  /* Create rank/scale matrix */
  
  for(i = 0; i < x->scales; i++){
	  for(j = 0; j < x->M; j++)
		  x->histo[x->ranktab[j][i].scale - 1][i]++;
  }
  
  
  /* Generate output vector */
  
  k = x->scales - x->offset;
  
  for(n = 0, i = x->offset; i < k; i++){
	  for(j = 0; j < x->scales; j++)
		  x->vector[n++] = x->histo[i][j];
  }
  
  for(i = 0; i < x->vecsize; i++)
	  SETFLOAT(atom_list_out+i, x->vector[i]);
  
	  outlet_list(x->out_vec, &s_list, x->vecsize, atom_list_out); 
	  
	  return (w+4);
}

static void wdv_dsp(t_wdv *x, t_signal **sp)
{
  dsp_add(wdv_perform, 3,
          sp[0]->s_vec, sp[0]->s_n, x);
}

static void *wdv_new(t_symbol *s, t_int argc, t_atom *argv)
{
  t_wdv *x = (t_wdv *)pd_new(wdv_class);
  x->out_vec = outlet_new(&x->x_obj, &s_list);

  t_int i, j, m, scale, rowsize, compression;
  
  x->N = (t_int)atom_getfloatarg(0, argc, argv); 
  x->M = x->N * .5f;
  x->scales = bitcount(x->N) + 1;
  compression = (t_int)atom_getfloatarg(1, argc, argv);
  x->offset = rintf((100 - compression) * x->scales * .01 * .5);
  x->vecsize = (x->scales - x->offset * 2) * x->scales;
  rowsize = scale = x->scales;
  m = x->M;
  
  if(!ISPOWEROFTWO(x->N))
	  post("invalid blocksize, must be a power of two");
  else
	  post("blocksize = %d, scales = %d, vectorsize = %d, offset = %d", 
			  x->N, x->scales, x->vecsize, x->offset);

  x->vector = (t_int *)getbytes(x->vecsize * sizeof(t_int));
  
  x->ranktab = (t_coeff **)getbytes(x->M * sizeof(t_coeff *));
  
  while(m--)
	  x->ranktab[m] = (t_coeff *)getbytes(x->scales * sizeof(t_coeff));
  
  x->histo = (t_int **)getbytes(x->scales * sizeof(t_int *));

  while(scale--)
	  x->histo[scale] = (t_int *)getbytes(x->scales * sizeof(t_int));

 
  return (void *)x;
}

static void wdv_tilde_free(t_wdv *x){
	t_int i;

	for(i = 0; i < x->M; i++)
		free(x->ranktab[i]);
	free(x->ranktab);
	
	for(i = 0; i < x->scales; i++)
		free(x->histo[i]);
	free(x->histo);

	free(x->vector);

/*	freebytes(x->buf, x->blocks * sizeof(t_coeff)); */
}


void wdv_tilde_setup(void) {
  wdv_class = class_new(gensym("wdv~"),
        (t_newmethod)wdv_new,
        0, sizeof(t_wdv),
        CLASS_DEFAULT, A_GIMME, 0); 
  
  class_addmethod(wdv_class, (t_method)wdv_dsp, gensym("dsp"), 0); 
  CLASS_MAINSIGNALIN(wdv_class, t_wdv,f); 
  class_sethelpsymbol(wdv_class, gensym("help-flib"));
}
