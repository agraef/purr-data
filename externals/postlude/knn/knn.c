/*
 * Copyright (C) 2000 Ichiro Fujinaga and Karl MacMillan
 *
 * Minor updates and maintenance (2008) Jamie Bullock <jamie@postlude.co.uk>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* This is a simple KNN object for PureData */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include "m_pd.h"


#undef N_FEATURES
#define N_FEATURES 40 /* FIXME - this really needs to be in the file somehow */
#define MAX_N_CLASSES 100  /* number of instruments */
#define TRUE 1
#define FALSE 0

#define ABS(x) (((x) > 0) ? (x) : -(x))

typedef struct _nn {
   int index;			/* where in the feature db */
   float dist;			/* distance from the unknown */
} NN;

typedef struct _feature {
   unsigned long id;
   unsigned long symbol_id;
   float *feature;
} FEATURE;

static t_class *knn_class;	/* class pointer for PD */

typedef struct _knn {
   t_object x_obj;		/* object pointer for PD */
   t_outlet *out;		/* this outputs the instrument class 
				 * (a float) */
   t_outlet *loo;		/* this is used for debugging - it outputs a 
				 * feature vec */
   FEATURE *feature_db;		/* the database of features */
   FEATURE unknown;
   int db_size;		/* size of the features db */
   float mean[N_FEATURES];	/* for normalization */
   float stdev[N_FEATURES];	/* for normalization */
   int k;			/* number of k to use */
   NN *knn;			/* array of nearest neighbors of length k */
   float wt[N_FEATURES];	/* the feature weight vector */
   int normalize;		/* flag for normalization */
   int learn_mode;		/* flag for learn (i.e. add) mode */
   int learn_symbol_id;		/* the symbol id to add if in learn mode */
} t_knn;

static void *knn_new(t_floatarg f);
static int find_nn(t_knn * x, FEATURE * unknown, FEATURE * database,
		   int size, float *weights, int k);
static NN *k_nn_new(int k);
static int majority(t_knn * x);
static float k_nn(t_knn * x, int candidate, float dist);
static float calc_dist(FEATURE * unknown, FEATURE * known, float *weights,
		       float min_dist);
static void get_timbre_features(t_knn * x, char *fname);
static void normalize_database(t_knn * x, FEATURE * Features, int size);
static void normalize(t_knn * x, FEATURE * f);
static void add_feature(t_knn * x, float *features, int id);

static void *knn_new(t_floatarg f)
{
   int i;
   t_knn *x = (t_knn *) pd_new(knn_class);
   x->out = outlet_new(&x->x_obj, &s_float);
   x->loo = outlet_new(&x->x_obj, gensym("list"));

   /* create the NN for the correct size of k */
   if (f == 0)
      x->k = 1;	/* default */
   else
      x->k = (int) f;
   x->knn = k_nn_new(x->k);

   /* create space for the unknown feature */
   x->unknown.feature = (float *) calloc(sizeof(float), N_FEATURES);

   x->db_size = 0;
   x->feature_db = NULL;
   x->normalize = FALSE;
   x->learn_mode = FALSE;

   for (i = 0; i < N_FEATURES; i++) {
      x->wt[i] = 1;
      x->mean[i] = 0;
      x->stdev[i] = 1;
   }
   return (x);
}

static void knn_free(t_knn * x)
{
   free(x->knn);
   free(x->feature_db);
}

static void knn_list(t_knn * x, t_symbol * s, int argc, t_atom * argv)
{
   int i;
   int answer;
   int id;
   int add;
   t_atom *a;
   float f[N_FEATURES];  // Is this still used? 00/12/04 IF

   /* read in the features from the PD list */
   for (i = 0, a = argv; i < argc; i++, a++) {
      if (a->a_type == A_FLOAT)
	 f[i] = x->unknown.feature[i] = a->a_w.w_float;
      else {
	 f[i] = x->unknown.feature[i] = 0;
	 post("KNN: error - wrong type for list elem %d %d", i,
	      a->a_w.w_float);
      if (isinf(x->unknown.feature[i]))
	 f[i] = x->unknown.feature[i] = 0;
      }
   }

   if (x->learn_mode == TRUE) {
      add_feature(x, x->unknown.feature, x->learn_symbol_id);
   } else if (x->feature_db != NULL) {
      post("Looking for answer");
      if (x->normalize == TRUE)
	 normalize(x, &x->unknown);
      answer =
	  find_nn(x, &x->unknown, x->feature_db, x->db_size, x->wt, 1);
post("ANSWER: %d", answer);
      outlet_float(x->out, answer);
   } else
      post("No database loaded!");
}

static void add_feature(t_knn * x, float *features, int id)
{
   int i;

   if (x->feature_db == NULL)
      x->feature_db = (FEATURE *) calloc(sizeof(FEATURE), 1);
   else
      x->feature_db = (FEATURE *) realloc(x->feature_db, sizeof(FEATURE) *
					  (x->db_size + 1));
   assert(x->feature_db != NULL);

   x->feature_db[x->db_size].feature =
       (float *) calloc(sizeof(float), N_FEATURES);
   x->feature_db[x->db_size].symbol_id = id;
   for (i = 0; i < N_FEATURES; i++)
      x->feature_db[x->db_size].feature[i] = features[i];
   x->db_size++;
   if (x->learn_mode == TRUE)
      post("Added entry for instrument %d - db now has %d entries.",
	   id, x->db_size);
}

static void knn_loo(t_knn * x, float f)
{
   int i;
   t_atom at[N_FEATURES];

   if ((int) f >= 0 && (int) f <= x->db_size - 1) {
      for (i = 0; i < N_FEATURES; i++) {
	 SETFLOAT(&at[i], x->feature_db[(int) f].feature[i]);
      }
      outlet_list(x->loo, 0, N_FEATURES, at);
      outlet_float(x->out, x->feature_db[(int) f].symbol_id);

      post("KNN: output instrument %d", x->feature_db[(int) f].symbol_id);
   } else
      post("Index out of range");
}

static void knn_set_normal(t_knn * x, float f)
{
   if ((int) f == FALSE)
      x->normalize = FALSE;
   else if ((int) f == TRUE)
      x->normalize = TRUE;
   post("set normalize %d", x->normalize);
}

static void knn_set_learn(t_knn * x, float mode, float symbol)
{
   if ((int) mode == TRUE) {
      x->learn_mode = TRUE;
      x->learn_symbol_id = (int) symbol;
      post("Now in learn mode for instrument %d", (int) symbol);
   } else {
      x->learn_mode = FALSE;
      post("Learn mode deactivated");
   }
}

static void knn_free_db(t_knn * x)
{
   int i;
   for (i = 0; i < x->db_size; i++) {
      free(x->feature_db[i].feature);
   }
   free(x->feature_db);
   x->db_size = 0;
   x->feature_db = NULL;
   post("Database cleared");
}

static void knn_save(t_knn * x, t_symbol * s)
{
   FILE *fp;
   int i, j;

   fp = fopen(s->s_name, "w");
   if (fp == NULL) {
      post("Error saving file %s", strerror(errno));
      return;
   }

   for (i = 0; i < x->db_size; i++) {
      fprintf(fp, "%i", (int) x->feature_db[i].symbol_id);

      for (j = 0; j < N_FEATURES; j++) {
	 fprintf(fp, " %g", x->feature_db[i].feature[j]);
      }
      fprintf(fp, "\n");
   }
   fclose(fp);
   post("File saved to %s", s->s_name);

}

static void knn_read(t_knn * x, t_symbol * s)
{
   if (x->feature_db != NULL)
      knn_free_db(x);
   get_timbre_features(x, s->s_name);
}

static void knn_read_wt(t_knn * x, t_symbol * s)
{
   FILE *fp;
   int ret, count = 0;

   fp = fopen(s->s_name, "r");
   if (fp == NULL) {
      post("Error reading weights %s", strerror(errno));
      return;
   }
   printf("Reading weights from %s . . . ", s->s_name);
   while (1) {
      ret = fscanf(fp, "%g", &x->wt[count]);
      if (ret != 1)
	 break;
      count++;
      if (count > N_FEATURES) {
	 post("too many weights!");
	 return;
      }
   }
   fclose(fp); /* JB-070505 - close the file handle */
   printf("done\n");
}

void knn_setup(void)
{
   knn_class = class_new(gensym("knn"), (t_newmethod) knn_new,
			 (t_method) knn_free, sizeof(t_knn), 0, A_DEFFLOAT,
			 0);
   class_addmethod(knn_class, (t_method) knn_loo, gensym("loo"), A_FLOAT,
		   0);
   class_addmethod(knn_class, (t_method) knn_set_normal, gensym("normal"),
		   A_FLOAT, 0);
   class_addmethod(knn_class, (t_method) knn_set_learn, gensym("learn"),
		   A_FLOAT, A_DEFFLOAT, 0);
   class_addmethod(knn_class, (t_method) knn_save, gensym("save"),
		   A_SYMBOL, 0);
   class_addmethod(knn_class, (t_method) knn_read, gensym("read"),
		   A_SYMBOL, 0);
   class_addmethod(knn_class, (t_method) knn_free_db, gensym("clear"), 0);
   class_addmethod(knn_class, (t_method) knn_read_wt,
		   gensym("readweights"), A_SYMBOL, 0);
   class_addlist(knn_class, knn_list);
}

static int find_nn(t_knn * x, FEATURE * unknown, FEATURE * database,
		   int size, float *weights, int k)
{
   int i, min = 1;
   float dist = 0, min_dist = 9e100;	// A BIG number

   for (i = 0; i < x->k; i++) {
      x->knn[i].index = 0;
      x->knn[i].dist = 9e10;
   }

   for (i = 0; i < x->db_size; i++) {

      dist = calc_dist(unknown, &database[i], weights, min_dist);
      if (dist < min_dist) {
	 min_dist = dist;
	 min = i;
	 min_dist = k_nn(x, min, min_dist);
      }
   }

   min = majority(x);

   return min;
}

static NN *k_nn_new(int k)
{
   int i;
   NN *nn;

   nn = (NN *) malloc(sizeof(NN) * k);
   assert(nn != NULL);
   for (i = 0; i < k; i++) {
      nn[i].index = -1;
      nn[i].dist = FLT_MAX;
   }
   return (nn);
}


static int majority(t_knn * x)
{
/* returns class number. NB: no tie breaker */
   int i, max_i = 0, max = 0;
   int classes[MAX_N_CLASSES];

   for (i = 0; i < MAX_N_CLASSES; i++)
      classes[i] = 0;

   for (i = 0; i < x->k; i++)
      classes[x->feature_db[x->knn[i].index].symbol_id] += 1;

   for (i = 0; i < MAX_N_CLASSES; i++) {
      if (classes[i] > max) {
	 max = classes[i];
	 max_i = i;
      }
   }
   return (max_i);
}

static float k_nn(t_knn * x, int candidate, float dist)
{
   int i, insert;

   for (i = 0; i < x->k; i++) {
      if (dist < x->knn[i].dist)
	 break;
   }
   if (i < x->k) {
      insert = i;

      for (i = x->k - 1; i > insert; i--)
	 x->knn[i] = x->knn[i - 1];

      x->knn[insert].index = candidate;
      x->knn[insert].dist = dist;
   }
   return (x->knn[x->k - 1].dist);
}

static float calc_dist(FEATURE * unknown, FEATURE * known, float *weights,
		       float min_dist)
{
   float dist = 0;
   int i;

   for (i = 0; i < N_FEATURES && dist < min_dist; i++) {
      /* Euclidean w/o the sqrt */
      dist += (weights[i]) *
	  (unknown->feature[i] - known->feature[i]) *
	  (unknown->feature[i] - known->feature[i]);

   }
   return ((float) dist);
}

static void normalize_database(t_knn * x, FEATURE * Features, int size)
{

   int i, k;
   float sum, sum2;

   for (k = 0; k < N_FEATURES; k++) {
      sum = 0;
      sum2 = 0;
      for (i = 0; i < size; i++) {
	 sum += Features[i].feature[k];
	 sum2 += Features[i].feature[k] * Features[i].feature[k];
      }
      x->mean[k] = sum / size;
      x->stdev[k] = sqrt((size * sum2 - sum * sum) / (size * (size - 1)));

      if (x->stdev[k] < 0.00001)
	 x->stdev[k] = 0.00001;
      if (x->normalize == TRUE) {
	 for (i = 0; i < size; i++)
	    Features[i].feature[k] =
		(Features[i].feature[k] - x->mean[k]) / x->stdev[k];
      }
   }
}

static void normalize(t_knn * x, FEATURE * f)
{
   int i;

   for (i = 0; i < N_FEATURES; i++)
      f->feature[i] = (f->feature[i] - x->mean[i]) / x->stdev[i];
}


void get_timbre_features(t_knn * x, char *fname)
{
   FILE *fp;
   int i, j, ret, instr = 0;
   float val;
   static char last_name[20], name[20];
   float features[N_FEATURES];
   int id = 0;
   int lastpitch = 0;

   printf("Reading %s . . . ", fname);

   fp = fopen(fname, "r");
   if (fp == NULL) {
      post("\nError opening file - %s", strerror(errno));
      return;
   }
   while (1) {
      ret = fscanf(fp, "%d", &instr);

      if (ret != 1)
	 break;

      for (i = 0; i < N_FEATURES; i++) {
	 fscanf(fp, "%g ", &features[i]);
         if (isinf(features[i]))
           features[i] = 0.0;
      }

      add_feature(x, features, instr);
   }
   printf("done.\nDatabase contains %d entries.\n", x->db_size);
   fclose(fp);
   normalize_database(x, x->feature_db, x->db_size);
}
