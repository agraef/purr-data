/*
* Main library file
*/

#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "helpers.c"
#include "chaotic.c"
#include "fractal.c"
#include "ifs.c"
#include "ifsmusic.c"
#include "selfsimilar.c"
#include "logistictilde.c"
#include "gauss_tilde.c"
#include "utils.c"
#include "elementaryca.c"
#include "genetic.c"
#include "neural.c"
#include "distribute.c"


void algocomp_setup(void) {
  logistic_class = class_new(gensym("logistic"),
        (t_newmethod)logistic_new,
        0, sizeof(t_logistic),
        CLASS_DEFAULT, A_DEFFLOAT,0);
  		class_addbang(logistic_class, logistic_bang);
  henon_class = class_new(gensym("henon"),
        (t_newmethod)henon_new,
        0, sizeof(t_henon),
        CLASS_DEFAULT, A_DEFFLOAT,A_DEFFLOAT,0);
  		class_addbang(henon_class, henon_bang);	
  lorenz_class = class_new(gensym("lorenz"),
        (t_newmethod)lorenz_new,
        0, sizeof(t_lorenz),
        CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT,A_DEFFLOAT,A_DEFFLOAT,0);
  		class_addbang(lorenz_class, lorenz_bang);
   chaosgame_class = class_new(gensym("chaosgame"),
        (t_newmethod)chaosgame_new,
        0, sizeof(t_chaosgame),
        CLASS_DEFAULT, A_DEFFLOAT,0);
  		class_addbang(chaosgame_class, chaosgame_bang);
   selfsimilar_class = class_new(gensym("selfsimilar"),
        (t_newmethod)selfsimilar_new,
        0, sizeof(t_selfsimilar),
        CLASS_DEFAULT, A_GIMME,0);
  		class_addbang(selfsimilar_class, selfsimilar_bang);
   selfsimilarrhythm_class = class_new(gensym("selfsimilarrhythm"),
        (t_newmethod)selfsimilarrhythm_new,
        0, sizeof(t_selfsimilarrhythm),
        CLASS_DEFAULT, A_DEFFLOAT,0);
 		class_addbang(selfsimilarrhythm_class, selfsimilarrhythm_bang);	
  ifs_class = class_new(gensym("ifs"),
        (t_newmethod)ifs_new,
        0, sizeof(t_ifs),
        CLASS_DEFAULT, 0);
  		class_addbang(ifs_class, ifs_bang);
  		class_addmethod(ifs_class,(t_method)ifs_setFunctions,gensym("list"), A_GIMME,0);
  distribute_class = class_new(gensym("distribute"),
        (t_newmethod)distribute_new,
        0, sizeof(t_distribute),
        CLASS_DEFAULT,  A_DEFFLOAT, 0);
  		class_addmethod(distribute_class,(t_method)distribute_float,gensym("float"), A_FLOAT,0);
  		
  		
  ifsmusic_class = class_new(gensym("ifsmusic"),
        (t_newmethod)ifsmusic_new,
        0, sizeof(t_ifsmusic),
        CLASS_DEFAULT, A_GIMME, 0);
  		class_addbang(ifsmusic_class, ifsmusic_bang);
  		class_addmethod(ifsmusic_class,(t_method)ifsmusic_setFunctions,gensym("list"), A_GIMME,0);	
  		class_addmethod(ifsmusic_class,(t_method)ifsmusic_setNotes,gensym("notes"), A_GIMME,0);	
   		class_addmethod(ifsmusic_class,(t_method)ifsmusic_outputSerie,gensym("serie"), 0); 		
  eca_class = class_new(gensym("eca"),
        (t_newmethod)eca_new,
        0, sizeof(t_eca),
        CLASS_DEFAULT,A_DEFFLOAT, A_DEFFLOAT,A_DEFFLOAT,0);
  		class_addbang(eca_class, eca_bang);
  		class_addmethod(eca_class,(t_method)eca_randomize,gensym("randomize"), 0);
  		class_addmethod(eca_class,(t_method)eca_activateMiddleCell,gensym("init"), 0);	
  genetic_class = class_new(gensym("genetic"),
        (t_newmethod)genetic_new,
        0, sizeof(t_genetic),
        CLASS_DEFAULT,A_DEFFLOAT, A_DEFFLOAT,A_DEFFLOAT,0);
  		class_addbang(genetic_class, genetic_bang);
  		class_addmethod(genetic_class,(t_method)genetic_randomize,gensym("randomize"), 0);
  		class_addmethod(genetic_class,(t_method)genetic_setTarget,gensym("target"), A_GIMME,0);				
	//void class_addlist(t_class *c, t_method fn);
  mlp_class = class_new(gensym("mlp"),
        (t_newmethod)mlp_new,
        0, sizeof(t_mlp),
        CLASS_DEFAULT,0);
  		class_addbang(mlp_class, mlp_bang);
  		class_addmethod(mlp_class,(t_method)mlp_inputed,gensym("list"), A_GIMME, 0);  		
  		class_addmethod(mlp_class,(t_method)mlp_train,gensym("train"), 0);
  		class_addmethod(mlp_class,(t_method)mlp_setTarget,gensym("target"), A_GIMME,0);
  oneoverf_class = class_new(gensym("oneoverf"),
        (t_newmethod)oneoverf_new,
        0, sizeof(t_oneoverf),
        CLASS_DEFAULT,A_DEFFLOAT,  0);
  		class_addbang(oneoverf_class, oneoverf_bang);
	logistic_tilde_class = class_new(gensym("logistic~"),
		(t_newmethod)logistic_tilde_new,
		0, sizeof(t_logistic_tilde),
		CLASS_DEFAULT,
		A_DEFFLOAT, 
		0);
		class_addmethod(logistic_tilde_class, 
		(t_method)logistic_tilde_dsp, gensym("dsp"), 0);
	// CLASS_MAINSIGNALIN(logistic_tilde_class, t_logistic_tilde, f);	
	gauss_tilde_class = class_new(gensym("gauss~"), 
		(t_newmethod)gauss_tilde_new, 0,
    	sizeof(t_gauss_tilde), 
    	0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(gauss_tilde_class, (t_method)gauss_tilde_dsp, 
    gensym("dsp"), 0);
    // class_addbang(rand_gauss_class, rand_gauss_bang);
	map_class = class_new(gensym("map"),
		(t_newmethod)map_new,
		0, sizeof(t_map),
		CLASS_DEFAULT,
		A_DEFFLOAT, A_DEFFLOAT,A_DEFFLOAT,A_DEFFLOAT,0);
		
	class_addfloat(map_class, map_float);
	class_addmethod(map_class,(t_method)map_list,gensym("list"), A_GIMME, 0);
	
	post("AlgoComp library loaded - v_0.20060116");	
	
}

