#include "MSPd.h"

#define MAX_ATTACKS (512)
#define MAX_PATTERNS (1024)


#define OBJECT_NAME "dmach~"


static t_class *dmach_class;


typedef struct
{
    float trigger_point;
    float increment;
    float amplitude;
} t_attack;

typedef struct
{
    short active; // flag for if this drum slot is used in current pattern
    int attack_count; // number of attacks in this pattern
    int adex; // index to current attack
    t_attack *attacks; // array containing attack data
} t_drumline;

typedef struct
{
    float beats; // how many beats in this pattern
    float dsamps; //duration of pattern in samples
    t_drumline *drumlines;
} t_pattern;

typedef struct _dmach
{
    
    t_object x_obj;
    float x_f;
    
	short mute; // global mute
	float clocker; // global sample counter clock
	float tempo;
	float tempo_factor; // multiplier to get actual beat duration
	t_pattern *patterns; // contains all drum patterns
	short *stored_patterns;// which locations contain a pattern
	float *gtranspose;// transpose factor for each individual drum slot
	float *gains; // gain factor for each individual drum slot
	float *current_increment;// maintains increment for sustained output
	int this_pattern; // number of current pattern
	int next_pattern; // number of pattern to call at end of current pattern
	float global_gain;
	float global_transpose;
	float sr;
	int drum_count; // number of drum slots to instantiate
	int outlet_count; // number of outlets on object
	short virgin; // no patterns stored - turn off performance
	/* sequencer */
	short playsequence; // flag to play through a stored sequence once
	short loopsequence; // flag to loop repeatedly through sequence
	int *sequence; // contains the sequence of bars to play
	int sequence_length; // how many bars are stored in sequence
	int seqptr; // keep track of current sequencer position
	float zeroalias; // use this to send a coded "zero" message (i.e. bar number is zero)
	t_atom *listdata; // for list output
	void *listraw_outlet;// send a list
	short clickincr; //flag that click increment is on (i.e. no sample and hold)
	short *attackpattern; // holds full attack sequence including zeros (for list output)
	int attackpattern_count;// how many ticks in attack pattern
	t_attack *tmpatks; // hold local copy of new slot pattern
	short *connected; // list of vector connections
	short *muted; // state of each slot
	short instant_recall; // flag that pattern gets loaded immediately (loops only)
} t_dmach;

void dmach_store(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void *dmach_new(t_symbol *s, int argc, t_atom *argv);
t_int *dmach_perform(t_int *w);
void dmach_mute(t_dmach *x, t_floatarg toggle);
void dmach_dsp(t_dmach *x, t_signal **sp);
void dmach_dsp_free(t_dmach *x);
void dmach_init_pattern(t_dmach *x, int pnum);
void dmach_show(t_dmach *x, t_floatarg fn);
void dmach_tempo(t_dmach *x, t_floatarg new_tempo);
void dmach_recall(t_dmach *x, t_floatarg pnf);
void dmach_transpose(t_dmach *x, t_floatarg slotf, t_floatarg new_transpose_factor);
void dmach_gain(t_dmach *x, t_floatarg slotf, t_floatarg new_gain_factor);
void dmach_arm(t_dmach *x, t_floatarg pnf);
void dmach_playsequence(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_slotamps(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_printraw(t_dmach *x, t_floatarg fn);
void dmach_readraw(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_slotincrs(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_loopsequence(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_muteslot(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_slotampsfull(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_nosequence(t_dmach *x);
void dmach_copypattern(t_dmach *x, t_floatarg pn1, t_floatarg pn2);
void dmach_listraw(t_dmach *x, t_symbol *s, int argc, t_atom *argv);
void dmach_clickincr(t_dmach *x, t_floatarg toggle);
void dmach_instant_recall(t_dmach *x, t_floatarg toggle);




void dmach_tilde_setup(void){
	dmach_class = class_new(gensym("dmach~"), (t_newmethod)dmach_new,
							(t_method)dmach_dsp_free,sizeof(t_dmach), 0,A_GIMME,0);
	CLASS_MAINSIGNALIN(dmach_class, t_dmach, x_f);
	class_addmethod(dmach_class,(t_method)dmach_dsp,gensym("dsp"),0);
	class_addmethod(dmach_class,(t_method)dmach_mute,gensym("mute"),A_FLOAT,0);
	class_addmethod(dmach_class,(t_method)dmach_show,gensym("show"),A_FLOAT,0);
	class_addmethod(dmach_class,(t_method)dmach_tempo,gensym("tempo"),A_FLOAT,0);
	class_addmethod(dmach_class,(t_method)dmach_store,gensym("store"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_recall,gensym("recall"),A_FLOAT,0);
	class_addmethod(dmach_class,(t_method)dmach_transpose,gensym("transpose"),A_FLOAT,A_FLOAT, 0);
	class_addmethod(dmach_class,(t_method)dmach_gain,gensym("gain"),A_FLOAT,A_FLOAT, 0);
	class_addmethod(dmach_class,(t_method)dmach_printraw,gensym("printraw"),A_FLOAT,0);
	class_addmethod(dmach_class,(t_method)dmach_arm,gensym("arm"),A_FLOAT,0);
	class_addmethod(dmach_class,(t_method)dmach_readraw,gensym("readraw"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_listraw,gensym("listraw"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_playsequence,gensym("playsequence"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_slotamps,gensym("slotamps"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_slotampsfull,gensym("slotampsfull"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_slotincrs,gensym("slotincrs"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_loopsequence,gensym("loopsequence"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_muteslot,gensym("muteslot"),A_GIMME,0);
	class_addmethod(dmach_class,(t_method)dmach_nosequence,gensym("nosequence"),0);
	class_addmethod(dmach_class,(t_method)dmach_copypattern,gensym("copypattern"),A_FLOAT,A_FLOAT, 0);
	class_addmethod(dmach_class,(t_method)dmach_clickincr,gensym("clickincr"),A_FLOAT,0);
	class_addmethod(dmach_class,(t_method)dmach_instant_recall,gensym("instant_recall"),A_FLOAT,0);
	
 	potpourri_announce(OBJECT_NAME);
}


void dmach_muteslot(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
	int slot;
	int drum_count = x->drum_count;
	short mutestate;
	
	if(argc < 2){
		post("muteslot: pattern number, slot number");
		return;
	}
	
	slot = (int)atom_getfloatarg(0,argc,argv);
	mutestate = (short)atom_getfloatarg(1,argc,argv);
	
	
	if(slot < 0 || slot > drum_count - 1){
		error("muteslot: illegal slot index: %d",slot);
		return;
	}
	x->muted[slot] = mutestate;
}

void dmach_nosequence(t_dmach *x)
{
	x->playsequence = 0;
	x->loopsequence = 0;
}

void dmach_playsequence(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
	int pnum;
	
	if(argc < 1){
		error("%s: zero length sequence",OBJECT_NAME);
		return;
	}
	/* need safety check here */
	for(i = 0; i < argc; i++){
		pnum = (int) atom_getfloatarg(i,argc,argv);
		if(! x->stored_patterns[pnum]){
			error("%d is not currently stored",pnum);
			return;
		}
	}
    
	for(i = 0; i < argc; i++){
		x->sequence[i] = (int) atom_getfloatarg(i,argc,argv);
	}
    
	//
	
	
	if(x->instant_recall){
		x->this_pattern = x->sequence[0];
		x->clocker = x->patterns[x->this_pattern].dsamps;
	} else {
		x->next_pattern = x->sequence[0];
	}
	
	x->mute = 0;
	x->playsequence = 1;
	x->loopsequence = 0;
	x->sequence_length = argc;
	x->seqptr = 0;
}
void dmach_loopsequence(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
	int pnum;
	
	if(argc < 1){
		error("%s: zero length sequence",OBJECT_NAME);
		return;
	}
	
	for(i = 0; i < argc; i++){
		pnum = (int) atom_getfloatarg(i,argc,argv);
		if(! x->stored_patterns[pnum]){
			error("%d is not currently stored",pnum);
			return;
		}
	}
	
	for(i = 0; i < argc; i++){
		x->sequence[i] = (int) atom_getfloatarg(i,argc,argv);
	}
    
	if(x->instant_recall){
		x->this_pattern = x->sequence[0];
		x->clocker = x->patterns[x->this_pattern].dsamps;
	} else {
		x->next_pattern = x->sequence[0];
	}
	
	x->mute = 0;
	x->playsequence = 1;
	x->loopsequence = 1;
	x->sequence_length = argc;
	x->seqptr = 0;
}

void dmach_gain(t_dmach *x, t_floatarg slotf, t_floatarg new_gain_factor)
{
	int slot = slotf;
    //	float ratio;
    //	float gain_factor;
    //	short *stored_patterns = x->stored_patterns;
	float *gains = x->gains;
    //	t_pattern *p = x->patterns;
	int drum_count = x->drum_count;
    //	int i,j,k;
	
	if(slot < 0 || slot > drum_count - 1){
		error("illegal slot index: %d",slot);
		return;
	}
	/* if(new_gain_factor <= 0){
     error("illegal gain factor %f", new_gain_factor);
     return;
     } */
    //	gain_factor = gains[slot];
    //	ratio = new_gain_factor / gain_factor;
	gains[slot] = new_gain_factor;
}

void dmach_transpose(t_dmach *x, t_floatarg slotf, t_floatarg new_transpose_factor)
{
	int slot = slotf;
	float *gtranspose = x->gtranspose;
	int drum_count = x->drum_count;
	
	if(slot < 0 || slot > drum_count - 1){
		error("%s: transpose given illegal slot index: %d",OBJECT_NAME, slot);
		return;
	}
	if(new_transpose_factor == 0){
		error("illegal transpose factor %f", new_transpose_factor);
		return;
	}
	gtranspose[slot] = new_transpose_factor;
	
}


void dmach_recall(t_dmach *x, t_floatarg pnf)
{
	int pnum = pnf;
	
	/* post("requested recall of %d, ignored",pnum);
     return;*/
	if(pnum < 0){
		error("requested index is less than zero");
		return;
	}
	if(pnum >= MAX_PATTERNS){
		error("requested index is greater than the maximum of %d",MAX_PATTERNS-1);
		return;
	}
	if(! x->stored_patterns[pnum]){
		error("%d is not currently stored",pnum);
		return;
	}
    //	x->this_pattern = x->next_pattern = pnum;
	x->mute = 0;
	x->next_pattern = pnum;
}

void dmach_arm(t_dmach *x, t_floatarg pnf)
{
	int pnum = pnf;
	int i;
	t_pattern *p = x->patterns;
    
	if(pnum < 0){
		error("requested index is less than zero");
		return;
	}
	if(pnum > MAX_PATTERNS){
		error("%s: requested index is greater than the maximum of %d",OBJECT_NAME,MAX_PATTERNS-1);
		return;
	}
	if(! x->stored_patterns[pnum]){
		error("%s: %d is not currently stored",OBJECT_NAME,pnum);
		return;
	}
	x->mute = 1;
	x->clocker = 0;
	x->next_pattern = x->this_pattern = pnum;
	for(i = 0; i < x->drum_count; i++){ /* reset pointers */
		p[x->this_pattern].drumlines[i].adex = 0;
	}
}


void dmach_tempo(t_dmach *x, t_floatarg new_tempo)
{
	float ratio;
	int i, j, k;
	short *stored_patterns = x->stored_patterns;
	t_pattern *p = x->patterns;
	int drum_count = x->drum_count;
	float sr = x->sr;
	float tempo_factor = x->tempo_factor;
	if(new_tempo <= 0.0){
		error("tempo must be greater than zero, but was %f",new_tempo);
		return;
	}
	ratio = x->tempo / new_tempo;
	x->clocker *= ratio;
	x->tempo = new_tempo;
    tempo_factor = (60.0/new_tempo);
    
	for(i = 0; i < MAX_PATTERNS; i++){
		if(stored_patterns[i]){
            p[i].dsamps = p[i].beats * tempo_factor * sr;
			for(j = 0; j < drum_count; j++){
			    if(p[i].drumlines[j].active){
                    for(k = 0; k < p[i].drumlines[j].attack_count; k++){
                        p[i].drumlines[j].attacks[k].trigger_point *= ratio;
                    }
			    }
			}
		}
	}
	x->tempo_factor = tempo_factor;
}
void dmach_show(t_dmach *x, t_floatarg fn)
{
	int i,j;
	int pnum = (int) fn;
    t_pattern *p = x->patterns;
    t_attack *ptr;
    int drum_count = x->drum_count;
    
    if(pnum < 0 || pnum > MAX_PATTERNS-1){
    	error("illegal pattern number: %d",pnum);
    	return;
    }
    
	if(! x->stored_patterns[pnum]){
		error("%d is not currently stored",pnum);
		return;
	}
	post("showing pattern %d",pnum);
    /* need to check if pattern is valid */
    
    for(j = 0; j < drum_count; j++){
        if(p[pnum].drumlines[j].active){
            post("*** drum line for slot %d ***",j);
            ptr = p[pnum].drumlines[j].attacks;
            post("there are %d attacks",p[pnum].drumlines[j].attack_count);
            for(i = 0; i < p[pnum].drumlines[j].attack_count; i++){
                post("amp: %f, transp: %f, trigger: %f",
                     ptr->amplitude, ptr->increment, ptr->trigger_point);
                ptr++;
            }
        }
    }
}

void dmach_printraw(t_dmach *x, t_floatarg fn)
{
	int i,j;
	int pnum = (int) fn;
    t_pattern *p = x->patterns;
    t_attack *ptr;
    int drum_count = x->drum_count;
    float normalized_trigger;
    float tempo_factor = x->tempo_factor;
    float sr = x->sr;
    
    if(pnum < 0 || pnum > MAX_PATTERNS-1){
    	error("illegal pattern number: %d",pnum);
    	return;
    }
    
	if(! x->stored_patterns[pnum]){
		error("%d is not currently stored",pnum);
		return;
	}
    if(!tempo_factor){
        error("tempo factor is zero!");
        return;
    }
    
	post("readraw %d %f",pnum, p[pnum].beats);
    for(j = 0; j < drum_count; j++){
        if(p[pnum].drumlines[j].active){
            ptr = p[pnum].drumlines[j].attacks;
            post("%d %d",j, p[pnum].drumlines[j].attack_count);
            
            for(i = 0; i < p[pnum].drumlines[j].attack_count; i++){
                /* scale attack times to factor out sample rate and tempo */
                normalized_trigger = ptr->trigger_point / (tempo_factor * sr);
                post("%f %f %f",
                     ptr->amplitude, ptr->increment, normalized_trigger);
                ptr++;
            }
        }
    }
}

void dmach_listraw(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
	int i,j;
	int pnum;
    t_pattern *p = x->patterns;
    t_attack *ptr;
    int drum_count = x->drum_count;
    float normalized_trigger;
    float tempo_factor = x->tempo_factor;
    float sr = x->sr;
    int ldex = 0;
    t_atom *listdata = x->listdata;
    
    if(argc < 1){
        pnum = x->this_pattern;
    } else {
  		pnum = (int) atom_getfloatarg(0,argc,argv);
    }
    if(pnum < 0 || pnum > MAX_PATTERNS-1){
        error("illegal pattern number: %d",pnum);
        return;
    }
    
	if(! x->stored_patterns[pnum]){
		error("%d is not currently stored",pnum);
		return;
	}
    if(!tempo_factor){
        error("tempo factor is zero!");
        return;
    }
    
    /* note: format of MACROS requires that ldex be incremented outside of
     call. Also note that traditional indexing A[x] cannot be used; instead must
     use A+x format. */

    SETSYMBOL(listdata + ldex, gensym("readraw")); ++ldex;
    
    SETFLOAT(listdata + ldex, (float)pnum);  ++ldex;
	SETFLOAT(listdata + ldex, p[pnum].beats);  ++ldex;
    
    for(j = 0; j < drum_count; j++){
        if(p[pnum].drumlines[j].active){
            ptr = p[pnum].drumlines[j].attacks;
            SETFLOAT(listdata + ldex, (float)j); ++ldex;
            SETFLOAT(listdata + ldex, (float)(p[pnum].drumlines[j].attack_count)); ++ldex;
            
            for(i = 0; i < p[pnum].drumlines[j].attack_count; i++){
                normalized_trigger = ptr->trigger_point / (tempo_factor * sr);
                SETFLOAT(listdata + ldex, ptr->amplitude); ++ldex;
                SETFLOAT(listdata + ldex, ptr->increment); ++ldex;
                SETFLOAT(listdata + ldex, normalized_trigger); ++ldex;
                ptr++;
            }
        }
    }
    outlet_list(x->listraw_outlet,0,ldex,listdata);
}

void dmach_copypattern(t_dmach *x, t_floatarg pn1, t_floatarg pn2)
{
	int i,j;
	int pnum_from = (int) pn1;
	int pnum_to = (int) pn2;
    t_pattern *p = x->patterns;
    t_attack *ptr_from, *ptr_to;
    int drum_count = x->drum_count;
    
    if(pnum_from < 0 || pnum_from > MAX_PATTERNS-1){
    	error("illegal source pattern number: %d",pnum_from);
    	return;
    }
    if(pnum_to < 0 || pnum_to > MAX_PATTERNS-1){
    	error("illegal dest pattern number: %d",pnum_to);
    	return;
    }
    if(pnum_from == pnum_to){
        error("source and dest patterns are the same");
        return;
    }
	if(! x->stored_patterns[pnum_from]){
		error("%d is not currently stored",pnum_from);
		return;
	}
	dmach_init_pattern(x,pnum_to);
    //	post("readraw %d %f %f",pnum, p[pnum].beats, p[pnum].dsamps);
    
	p[pnum_to].beats = p[pnum_from].beats;
	p[pnum_to].dsamps = p[pnum_from].dsamps;
	
    for(j = 0; j < drum_count; j++){
        p[pnum_to].drumlines[j].active = p[pnum_from].drumlines[j].active;
        if(p[pnum_from].drumlines[j].active){
            ptr_from = p[pnum_from].drumlines[j].attacks;
            ptr_to = p[pnum_to].drumlines[j].attacks;
            p[pnum_to].drumlines[j].attack_count = p[pnum_from].drumlines[j].attack_count;
            for(i = 0; i < p[pnum_from].drumlines[j].attack_count; i++){
                ptr_to->amplitude = ptr_from->amplitude;
                ptr_to->increment = ptr_from->increment;
                ptr_to->trigger_point = ptr_from->trigger_point;
                ptr_from++;
                ptr_to++;
            }
        }
    }
    x->stored_patterns[pnum_to] = 1;// assert that a legal pattern is now stored there
}

void dmach_readraw(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
	int pnum;
	int pdex = 0;
	int slot;
    t_pattern *p = x->patterns;
    t_attack *ptr;
    //  int drum_count = x->drum_count;
	float tempo_factor = x->tempo_factor;
	float sr = x->sr;
	short mutein;
	
	mutein = x->mute;
	x->mute = 1;
	pnum = (int) atom_getfloatarg(pdex++,argc,argv);
    
    
    if(pnum < 0 || pnum > MAX_PATTERNS-1){
    	error("%s: illegal pattern number: %d",OBJECT_NAME,pnum);
    	return;
    }
    
	if(! x->stored_patterns[pnum]){
        x->stored_patterns[pnum] = 1; // means there's something there now
        dmach_init_pattern(x,pnum);
        post("readraw: loading pattern %d",pnum);
        
	} else {
		post("readraw: reloading pattern %d",pnum);
	}
	p[pnum].beats = atom_getfloatarg(pdex++,argc,argv);
    //	p[pnum].dsamps = atom_getfloatarg(pdex++,argc,argv);
	p[pnum].dsamps = p[pnum].beats * tempo_factor * sr;
    //	post("dsamps calculated to be %f", p[pnum].dsamps);
	while(pdex < argc){
		slot = (int) atom_getfloatarg(pdex++,argc,argv);
		p[pnum].drumlines[slot].active = 1;
		p[pnum].drumlines[slot].attack_count = (int) atom_getfloatarg(pdex++,argc,argv);
		p[pnum].drumlines[slot].adex = 0;
		ptr = p[pnum].drumlines[slot].attacks;
		for(i = 0; i < p[pnum].drumlines[slot].attack_count; i++){
            ptr->amplitude = atom_getfloatarg(pdex++,argc,argv);
            ptr->increment = atom_getfloatarg(pdex++,argc,argv);
            ptr->trigger_point = (sr * tempo_factor) * atom_getfloatarg(pdex++,argc,argv);
			ptr++;
		}
		
	}
	x->this_pattern = x->next_pattern = pnum;
	x->virgin = 0;
	x->mute = mutein;
}

void dmach_slotamps(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
	int pdex,i;
	int slot = 0;
	float beatseg;
    //	float tmpbeats;
	float subdiv;
	float beat_samps;
	float tempo_factor;
	int attack_count;
	int local_attacks;
	float trigger_point;
	float val;
	int pnum;
	t_pattern *p = x->patterns;
	float tempo = x->tempo;
	float sr = x->sr;
	t_attack *tmpatks = x->tmpatks;
	
    
	
	pdex = 0;
	pnum = atom_getfloatarg(pdex++,argc,argv);
    /*
     post("skipping slotamps for %d",pnum);
     return;*/
    
	if(pnum < 0 || pnum > MAX_PATTERNS - 1){
		error("%s: invalid pattern number: %d",OBJECT_NAME,pnum);
		return;
	}
	
	if(!x->stored_patterns[pnum]){
		error("%s: no pattern found at location : %d",OBJECT_NAME,pnum);
		return;
	}
    
	slot = (int) atom_getfloatarg(pdex++,argc,argv);
	if(slot < 0 || slot >= x->drum_count){
		post("%s: %d is an illegal slot",OBJECT_NAME,slot);
		return;
	}
    //	post("filling slotamps %d for %d",slot, pnum);
	if(tempo <= 0){
		tempo = 60;
		error("zero tempo found");
	}
	tempo_factor = (60.0/tempo);
	
	beatseg = p[pnum].beats; // less general but we're going for ease here
	subdiv = atom_getfloatarg(pdex++,argc,argv);
	beat_samps = (beatseg/subdiv) * tempo_factor * sr;
    trigger_point = 0;
    attack_count = 0;
	/* read attack cycle and store any non-zero attacks */
	local_attacks = 0;
	// clean me
	memset((void *)tmpatks, 0, MAX_ATTACKS * sizeof(t_attack) );
	
	for(i = 0; i < subdiv; i++){
		val = atom_getfloatarg(pdex++,argc,argv);
		if(val){
			tmpatks[local_attacks].amplitude = val;
			// force to integer sample point
			tmpatks[local_attacks].trigger_point = (int)trigger_point;
			++local_attacks;
		}
		trigger_point += beat_samps;
	}
    
	/* initialize with increment of 1.0 for each non-zero amplitude attack. */
	for(i = 0; i < local_attacks; i++){
		if(tmpatks[i].amplitude) {
			tmpatks[i].increment = 1.0;
		} else {
			tmpatks[i].increment = 0.0;
		}
	}
	p[pnum].drumlines[slot].active = 0;
	memcpy((void *)p[pnum].drumlines[slot].attacks,(void *)tmpatks,
           MAX_ATTACKS * sizeof(t_attack));
	p[pnum].drumlines[slot].attack_count = local_attacks;
	p[pnum].drumlines[slot].adex = 0;
	p[pnum].drumlines[slot].active = 1;
	x->this_pattern = x->next_pattern = pnum; // set pattern to what we're working on
    
    
}



/* more general version */
void dmach_slotampsfull(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
	int pdex,i;
	int slot = 0;
	float beatseg;
	float tmpbeats;
	float subdiv;
	float beat_samps;
	float tempo_factor;
	int attack_count;
	int local_attacks;
	float trigger_point;
	float val;
	int pnum;
	t_pattern *p = x->patterns;
	float tempo = x->tempo;
	float sr = x->sr;
	
	if(argc > MAX_ATTACKS + 1){
		post("%s: %d is too long an atk message",OBJECT_NAME,argc);
		return;
	}
	pdex = 0;
	pnum = atom_getfloatarg(pdex++,argc,argv);
	
	if(pnum < 0 || pnum > MAX_PATTERNS - 1){
		error("%s: invalid pattern number: %d",OBJECT_NAME,pnum);
		return;
	}
	
	if(!x->stored_patterns[pnum]){
		error("%s: no pattern found at location : %d",OBJECT_NAME,pnum);
		return;
	}
	
	x->this_pattern = x->next_pattern = pnum; // set current pattern to what we're working on
	
	slot = (int) atom_getfloatarg(pdex++,argc,argv);
	p[pnum].drumlines[slot].active = 1;
	tempo_factor = (60.0/tempo);
	tmpbeats = p[pnum].beats;
	
	
    trigger_point = 0;
    attack_count = 0;
    while(tmpbeats > 0){
		local_attacks = 0;
		beatseg = atom_getfloatarg(pdex++,argc,argv);
		subdiv = atom_getfloatarg(pdex++,argc,argv);
		beat_samps = (beatseg/subdiv) * tempo_factor * sr;
		/* read attack cycle and store any non-zero attacks */
		local_attacks = 0;
		for(i = 0; i < subdiv; i++){
			val = atom_getfloatarg(pdex++,argc,argv);
			if(val){
				p[pnum].drumlines[slot].attacks[attack_count + local_attacks].amplitude = val;
				/* force to integer sample point (couldn't get round() to work) */
				p[pnum].drumlines[slot].attacks[attack_count + local_attacks].trigger_point = (int)trigger_point;
				++local_attacks;
			}
			trigger_point += beat_samps;
		}
        for(i = 0; i < local_attacks; i++){
			p[pnum].drumlines[slot].attacks[i + attack_count].increment = 1.0;
        }
        tmpbeats -= beatseg;
        attack_count += local_attacks;
	}
	p[pnum].drumlines[slot].attack_count = attack_count;
    
}

void dmach_slotincrs(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
    int pdex,i;
    int slot = 0;
    int local_attacks;
	int pnum;
    t_pattern *p = x->patterns;

	if(argc > MAX_ATTACKS + 1){
		post("%s: %d is too long a slotincrs message",OBJECT_NAME,argc);
		return;
	}
    pdex = 0;
	pnum = (int) atom_getfloatarg(pdex++,argc,argv);
    slot = (int) atom_getfloatarg(pdex++,argc,argv);
	if(slot < 0 || slot >= x->drum_count){
		post("%s: %d is an illegal slot",OBJECT_NAME,slot);
		return;
	}
    local_attacks = p[pnum].drumlines[slot].attack_count;
    if(argc != local_attacks + 2){
        // post("rejected slotincrs with %d local atks, argc = %d",local_attacks, argc - 2);
        return;
    }
    //  post("%d local atks, argc = %d",local_attacks, argc);
    
    
    if(pnum < 0 || pnum >= MAX_PATTERNS){
        error("%s: slotincrs sent invalid pattern number: %d",OBJECT_NAME,pnum);
        return;
    }
    
    if(!x->stored_patterns[pnum]){
        error("%s: slotincrs: no pattern found at location : %d",OBJECT_NAME,pnum);
		return;
    }
	
    
    
    
    
    //  post("filling slotincr for slot %d pnum %d",slot, pnum);
    
    
    
    for(i = 0; i < local_attacks; i++){
		p[pnum].drumlines[slot].attacks[i].increment = atom_getfloatarg(pdex++,argc,argv);
    }
    x->this_pattern = x->next_pattern = pnum; // set current pattern to what we're working on
    
    //  x->mute = mutein;
}

void dmach_store(t_dmach *x, t_symbol *s, int argc, t_atom *argv)
{
    int pdex,i;
    int slot = 0;
    float beatseg;
    float tmpbeats;
    float subdiv;
    float beat_samps;
    float tempo_factor = x->tempo_factor;
    int attack_count;
    int local_attacks;
    float trigger_point;
    float val;
	int pnum;
    t_pattern *p = x->patterns;
    float tempo = x->tempo;
    float sr = x->sr;
    
    
    
    pnum = atom_getfloatarg(0,argc,argv);
    if(pnum < 0 || pnum > MAX_PATTERNS - 1){
        error("invalid pattern number: %d",pnum);
        return;
    }
    //  post("%d arguments to \"store\" at pattern %d",argc,pnum);
    
    dmach_init_pattern(x,pnum);
    
    p[pnum].beats = atom_getfloatarg(1,argc,argv);
    if(p[pnum].beats <= 0){
        post("illegal beats at pnum %d: %f",pnum,p[pnum].beats);
        p[pnum].beats = 4;
    }
    if(tempo <= 0){
        error("zero tempo in store msg");
        tempo = 60;
    }
    tempo_factor = (60.0/tempo);
    p[pnum].dsamps = p[pnum].beats * tempo_factor * sr;
    pdex = 2;
    
    //  post("%f beats %f samps in this pattern",p[pnum].beats,p[pnum].dsamps );
    while(pdex < argc){
        slot = atom_getfloatarg(pdex++,argc,argv);
        p[pnum].drumlines[slot].active = 1;
        tmpbeats = p[pnum].beats;
        
        trigger_point = 0;
        attack_count = 0;
        while(tmpbeats > 0){
            local_attacks = 0;
            beatseg = atom_getfloatarg(pdex++,argc,argv);
            subdiv = atom_getfloatarg(pdex++,argc,argv);
            beat_samps = (beatseg/subdiv) * tempo_factor * sr;
            
            /* read attack cycle and store any non-zero attacks */
            for(i = 0; i < subdiv; i++){
				val = atom_getfloatarg(pdex++,argc,argv);
				if(val){
                    p[pnum].drumlines[slot].attacks[attack_count + local_attacks].amplitude = val;
                    /* force to integer sample point (couldn't get round() to work) */
                    p[pnum].drumlines[slot].attacks[attack_count + local_attacks].trigger_point = (int)trigger_point;
                    ++local_attacks;
				}
				trigger_point += beat_samps;
            }
            /* we now know number of attacks and read that many transpose factors */
            
            for(i = 0; i < local_attacks; i++){
				p[pnum].drumlines[slot].attacks[i + attack_count].increment = atom_getfloatarg(pdex++,argc,argv);
            }
            tmpbeats -= beatseg;
            attack_count += local_attacks;
        }
        p[pnum].drumlines[slot].attack_count = attack_count;
        //	post("%d attacks in slot %d for pattern %d",attack_count, slot, pnum);
    }
    // new - set internal pointer to start of array
    p[pnum].drumlines[slot].adex = 0;
    // set current pattern to this (to avoid crash if pnum 0 is uninitialized)
    x->this_pattern = x->next_pattern = pnum;
    x->virgin = 0; // now at least one pattern is stored
    x->stored_patterns[pnum] = 1; // means there's something there now
    x->tempo_factor = tempo_factor;//restore this value
    //  post("pattern stored at %d with %f beats",pnum,p[pnum].beats);
}


void dmach_init_pattern(t_dmach *x, int pnum)
{
    int i;
    int drum_count = x->drum_count;
    t_pattern *p = x->patterns;
    if(pnum < 0 || pnum >= MAX_PATTERNS){
        error("invalid pattern number: %d",pnum);
        return;
    }
    
    
    
    if( x->stored_patterns[pnum] ){
        // post("replacing pattern stored at %d",pnum);
    }
    
    if(p[pnum].drumlines == NULL){
        // post("initializing drumline memory at location %d",pnum);
        p[pnum].drumlines = (t_drumline *)malloc(drum_count * sizeof(t_drumline));
    }
    
    
    for(i = 0; i < drum_count; i++){
        p[pnum].drumlines[i].attacks = (t_attack *)calloc(MAX_ATTACKS, sizeof(t_attack));
        p[pnum].drumlines[i].adex = 0;
        p[pnum].drumlines[i].active = 0;
        p[pnum].drumlines[i].attack_count = 0;
    }
    
    
}

void dmach_dsp_free( t_dmach *x )
{
    /* need some freeing action here! */
	free(x->patterns);
	free(x->stored_patterns);
	free(x->current_increment);
	free(x->gtranspose);
	free(x->gains);
	free(x->sequence);
	free(x->listdata);
	free(x->connected);
	free(x->tmpatks);
	free(x->muted);
}

void dmach_mute(t_dmach *x, t_floatarg toggle)
{
	x->mute = (short)toggle;
}

void dmach_instant_recall(t_dmach *x, t_floatarg toggle)
{
	x->instant_recall = (short)toggle;
}

void dmach_clickincr(t_dmach *x, t_floatarg toggle)
{
	x->clickincr = (short)toggle;
}



void *dmach_new(t_symbol *s, int argc, t_atom *argv)
{
  	int i;
	t_dmach *x;
	x = (t_dmach *)pd_new(dmach_class);
	
	if(argc >= 1)
		x->tempo = atom_getfloatarg(0,argc,argv);
	else
		x->tempo = 120;
	if(argc >= 2)
		x->drum_count = atom_getfloatarg(1,argc,argv);
	else
		x->drum_count = 8;
	
	x->outlet_count = x->drum_count * 2 + 1; // one extra for pattern start click

    for(i = 0; i < x->outlet_count; i++){
        outlet_new(&x->x_obj, gensym("signal"));
    }
	x->listraw_outlet = outlet_new(&x->x_obj, gensym("list"));
	x->patterns = (t_pattern *) malloc(MAX_PATTERNS * sizeof(t_pattern));
	x->stored_patterns = (short *) malloc(MAX_PATTERNS * sizeof(short));
	x->current_increment = (float *) malloc(x->drum_count * sizeof(float)); // for sample + hold of increment
	x->gtranspose = (float *) malloc(x->drum_count * sizeof(float));
	x->gains = (float *) malloc(x->drum_count * sizeof(float));
	x->sequence = (int *) malloc(1024 * sizeof(int));
	x->listdata = (t_atom *) malloc(1024 * sizeof(t_atom));
	x->connected = (short *) malloc(1024 * sizeof(short));
	x->tmpatks = (t_attack *)calloc(MAX_ATTACKS, sizeof(t_attack));
	x->muted = (short *)calloc(x->drum_count, sizeof(short)); // by default mute is off on each slot
    
	x->seqptr = 0;
	x->sequence_length = 0;
	x->playsequence = 0;
	x->loopsequence = 0;
	x->zeroalias = -1.0;
	x->clickincr = 0;
	x->instant_recall = 0;
	
	
	if(x->tempo <= 0 || x->tempo > 6666)
		x->tempo = 60.0;
    
    //	post("initial tempo is %f",x->tempo);
	for(i = 0; i < MAX_PATTERNS; i++){
		x->patterns[i].drumlines = NULL;
		x->stored_patterns[i] = 0;
	}
	x->this_pattern = x->next_pattern = 0;
	x->mute = 0;
    
	x->clocker = 0;
	x->sr = sys_getsr();
	x->tempo_factor = 60.0 / x->tempo ;
    
	for(i = 0; i < x->drum_count; i++){
		x->gains[i] = 1.0;
		x->gtranspose[i] = 1.0;
		x->current_increment[i] = 1.0;
	}
	// safety - init memory for all drum patterns
	for(i = 0; i < MAX_PATTERNS; i++){
		dmach_init_pattern(x,i);
	}
	x->virgin = 1;
    return (x);
}



t_int *dmach_perform(t_int *w)
{
    int i,j;
    t_float *trig_outlet, *incr_outlet;
    
	t_dmach *x = (t_dmach *) w[1];
	int outlet_count = x->outlet_count;
    //	t_float *in_sync = (t_float *) w[2]; // input for sync trigger (not implemented yet)
	t_float *sync = (t_float *) w[outlet_count + 2];
	int n = (int) w[outlet_count + 3];
    
	int this_pattern = x->this_pattern;
	int next_pattern = x->next_pattern;
	t_pattern *p = x->patterns;
	float *current_increment = x->current_increment;
	float clocker = x->clocker;
	float dsamps = p[this_pattern].dsamps;
	int drum_count = x->drum_count;
	int adex;
	/* sequence stuff */
	int *sequence = x->sequence;
	short playsequence = x->playsequence;
	short loopsequence = x->loopsequence;
	short clickincr = x->clickincr;
	int seqptr = x->seqptr;
	int sequence_length = x->sequence_length;
	short *connected = x->connected;
	short *muted = x->muted;
	float *gtranspose = x->gtranspose;
	float *gains = x->gains;
	
	/* clean pnum click outlet */
	memset((void *)sync, 0, n * sizeof(float));
	
	if( x->mute || x->virgin ){
		for(i = 0; i < drum_count; i++){
			if(connected[i * 2 + 1]){
				// post("cleaning outlet pair %d", i);
				trig_outlet = (t_float *) w[i * 2 + 3];
				memset((void *)trig_outlet, 0, n * sizeof(float));
			}
		}
		return w + (outlet_count + 4);
	}
	
	/* pre-clean all connected trigger vectors */
	for(i = 0; i < drum_count; i++){
		if(connected[i * 2 + 1]){
			trig_outlet = (t_float *) w[i * 2 + 3];
			memset((void *)trig_outlet, 0, n * sizeof(float));
		}
	}
	
	for(j = 0; j < n; j++){
		if(clocker >= dsamps){ // ready for next pattern now
			clocker -= dsamps;
 			/* this is the pattern sequencer */
 			if(playsequence){
  				if (seqptr >= sequence_length){
					if(loopsequence){
						seqptr = 0;
					} else {
						/* this is fine, the playthrough has concluded and we now mute external */
						x->mute = 1;
						goto escape;
					}
 				}
 				this_pattern = sequence[seqptr++];
                
 			}
 			/* we do this if we're not pattern sequencing: */
			else if(next_pattern != this_pattern){
				this_pattern = next_pattern;
			}
			
			for(i = 0; i < drum_count; i++){ /* reset pointers */
				p[this_pattern].drumlines[i].adex = 0;
			}
			/* send current bar number if in sequence. Note kludge for if
             bar is 0. We cannot send a 0 click so we alias it to something else.
             */
			if(playsequence){
				if(this_pattern){
					sync[j] = this_pattern;
				} else {
					sync[j] = x->zeroalias;
				}
			} else{
				sync[j] = 1; /* send a sync click */
			}
		}
		else {
			sync[j] = 0;
		}
		for(i = 0; i < drum_count; i++){
			// float locamp;
			if(p[this_pattern].drumlines[i].active && ! muted[i]){
				trig_outlet = (t_float *) w[i * 2 + 3];
				incr_outlet = (t_float *) w[i * 2 + 4];
				
				adex = p[this_pattern].drumlines[i].adex; // overflow danger ???
                
				if((int)clocker == (int)p[this_pattern].drumlines[i].attacks[adex].trigger_point){
					current_increment[i] =
                    p[this_pattern].drumlines[i].attacks[adex].increment * gtranspose[i];
					/* put sync click into sample j of the output vector */
                    
					trig_outlet[j] =
                    p[this_pattern].drumlines[i].attacks[adex].amplitude * gains[i];
					++adex;
					p[this_pattern].drumlines[i].adex = adex;
					incr_outlet[j] = current_increment[i];
					/*
                     post("t: sample %d, slot %d, amp: %f, incr: %f",
                     (int)clocker, i, locamp, current_increment[i]); */
				}
				else {
					trig_outlet[j] = 0; // not sure we need these assignments ??
					if(clickincr){
						incr_outlet[j] = 0;
					} else {
						incr_outlet[j] = current_increment[i]; // samp + hold the increment to outlet
					}
				}
				
			} 
		}
		++clocker;
	}
	
escape:
	
	x->clocker = clocker;
	x->this_pattern = this_pattern;
	x->seqptr = seqptr;
	
	return w + (outlet_count + 4);
}		

void dmach_dsp(t_dmach *x, t_signal **sp)
{
	long i;
	t_int **sigvec;
	int pointer_count;
	
    
	if(x->virgin){
		post("%s: no patterns are stored",OBJECT_NAME);
        //		return;
	}
	if(x->sr != sp[0]->s_sr){
		x->sr = sp[0]->s_sr;
        /* still need to adjust every stored pattern dsamps variable */
	}
	
	pointer_count = x->outlet_count + 3; // all outlets, 1 inlet, object and s_n
	
    //	post("pointer count %d", pointer_count);
    
    
    // copy vector to be safe

	for(i = 0; i < pointer_count - 2; i++){
		x->connected[i] = 1;
	}

    
	sigvec  = (t_int **) calloc(pointer_count, sizeof(t_int *));	
	for(i = 0; i < pointer_count; i++){
		sigvec[i] = (t_int *) calloc(sizeof(t_int),1);
	}
	sigvec[0] = (t_int *)x; // first pointer is to the object
    
	sigvec[pointer_count - 1] = (t_int *)sp[0]->s_n; // last pointer is to vector size (N)
	
	for(i = 1; i < pointer_count - 1; i++){ // now attach the inlet and all outlets
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
	}	

	dsp_addv(dmach_perform, pointer_count, (t_int *) sigvec); 
	free(sigvec);
}

