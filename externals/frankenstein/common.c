/*

implementation of common functions used by the externals of the frank framework

Authors:
Davide Morelli http://www.davidemorelli.it
David Plans Casal http://www.studios.uea.ac.uk/people/staff/casal

*/
#include "common.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "m_pd.h" // to post errors and debug messages

t_duration int2duration(int n)
{
	t_duration dur;
	int curr, i, j, ok, currden;
	curr=0;
	ok=0;
	for (i=0; i<num_possible_denominators; i++)
	{
		currden = possible_denominators[i];
		for (j=0; j<currden; j++)
		{
			if (curr==n)
			{
				dur.numerator=j;
				dur.denominator=currden;
				j=currden;
				i=num_possible_denominators;
				ok=1;
			} else
				curr++;
		}
	}
	if (ok)
		return dur;
	else
	{
		dur.numerator=1;
		dur.denominator=1;
		return dur;
	}
}

unsigned short int duration2int(t_duration dur)
{
	unsigned short int curr, i, j;
	curr=0;
	for (i=0; i<num_possible_denominators; i++)
	{
		for (j=0; j<i; j++)
		{
			if ((dur.numerator==j) && (dur.denominator==possible_denominators[i]))
			{
				return curr;
			} else
				curr++;
		}
	}
	return curr;	
}

int possible_durations()
{
	int ris, i;
	ris = 0;
	for (i=0; i<num_possible_denominators; i++)
	{
		ris += possible_denominators[i]-1;
	}
	ris += 1;
	return ris;
}

t_duration float2duration(float fduration)
{
	float minDiff, currfduration, currDiff;
	int i, maxi;
	t_duration currDur, bestDur;

	bestDur.numerator=1;
	bestDur.denominator=1;
	minDiff = 1;
	maxi = possible_durations();
	for (i=0; i<maxi; i++)
	{
	//	post("i=%i", i);
		currDur = int2duration(i);
	//	post("currDur=%i/%i", currDur.numerator, currDur.denominator);
		currfduration = duration2float(currDur);
	//	post("currfduration=%f", currfduration);
		currDiff = (float) fabs(fduration - currfduration);
		if (currDiff<=minDiff)
		{
			minDiff = currDiff;
			bestDur = currDur;
		}
	}
	return bestDur;
}

float duration2float(t_duration duration)
{
	return (float) (((float)duration.numerator) / ((float)duration.denominator));
}

void setFirstBeat(t_rhythm_event **firstEvent, unsigned short int voice, float fstart, float fduration)
{
	t_duration res;
	t_rhythm_event *newElement;
	// convert from float to duration
	res = float2duration(fduration);
	// allocate a new element of the list
	newElement = malloc(sizeof(t_rhythm_event));
	// set the pointers
	newElement->previous = 0;
	newElement->next = 0;
	newElement->voice=voice;
	newElement->duration.numerator = res.numerator;
	newElement->duration.denominator = res.denominator;
	res = float2duration(fstart);
	newElement->start.numerator = res.numerator;
	newElement->start.denominator = res.denominator;
	*firstEvent = newElement;
	//post("DEBUG setFirstBeat: %i %i", res.numerator, res.denominator);
}

void concatenateBeat(t_rhythm_event *currentEvent, unsigned short int voice, float fstart, float fduration)
{
	t_duration res;
	t_rhythm_event *newElement, *lastElement;
	lastElement = currentEvent;
	while(lastElement->next)
		lastElement = lastElement->next;
	// convert from float to duration
	res = float2duration(fduration);
	// allocate a new element of the list
	newElement = (t_rhythm_event *) malloc(sizeof(t_rhythm_event));
	// set the pointers
	newElement->previous = lastElement;
	newElement->next = 0;
	lastElement->next = newElement;
	newElement->voice=voice;
	newElement->duration.numerator = res.numerator;
	newElement->duration.denominator = res.denominator;
	res = float2duration(fstart);
	newElement->start.numerator = res.numerator;
	newElement->start.denominator = res.denominator;
	//post("DEBUG concatenateBeat: %i %i", res.numerator, res.denominator);
}

void freeBeats(t_rhythm_event *currentEvent)
{
	t_rhythm_event *prev;
	t_rhythm_event *next;

	if (currentEvent==0)
		return;

	// go to the first element of the list
	while(currentEvent->previous)
		currentEvent = currentEvent->previous;

	// now free each element
	next=currentEvent->next;
	while(currentEvent)
	{
		prev = currentEvent;
		currentEvent = currentEvent->next;
		free(prev);
	} 
	currentEvent = 0;

}

void add_t_rhythm_memory_arc(t_rhythm_memory_node *srcNode, unsigned short int dstNode)
{
	t_rhythm_memory_arc *newArc;
	t_rhythm_memory_arc *lastArc;

	// create a new arc
	newArc = (t_rhythm_memory_arc *) malloc(sizeof(t_rhythm_memory_arc));
	newArc->to_node_index = dstNode;
	// go to the last arc in the list
	// and add this arc as the last
	lastArc = srcNode->arcs;
	if (lastArc)
	{
		// this is not the first arc
		while(lastArc->next_arc)
			lastArc = lastArc->next_arc;
		lastArc->next_arc = newArc;
	} else
	{
		// this is the first arc
		srcNode->arcs = newArc;
	}
}

// initialize this representation, allocates memory for the pointers
void create_rhythm_memory_representation(t_rhythm_memory_representation **this_rep, unsigned short int id)
{
	int i;
	// allocate space for the data structure
	*this_rep = (t_rhythm_memory_representation *)malloc(sizeof(t_rhythm_memory_representation));
	// space for transitions
	(*this_rep)->transitions = (t_rhythm_memory_node *) malloc(sizeof(t_rhythm_memory_node) * possible_durations());
	// initialize transitions
	for (i=0; i<possible_durations(); i++)
	{
		(*this_rep)->transitions[i].first=0;
		(*this_rep)->transitions[i].weight=0;
		(*this_rep)->transitions[i].arcs=0;
	}
	(*this_rep)->rhythms = 0;
	// the naming variables
	(*this_rep)->id = id; // the main id
	(*this_rep)->last_sub_id = 0; // the sub id
	(*this_rep)->next = 0;
	(*this_rep)->max_weight = 0;
}

// add a new rhythm in the list of similar thythms related to one main rhythm
unsigned short int add_t_rhythm_memory_element(t_rhythm_memory_representation *this_rep, t_rhythm_event *new_rhythm)
{
	t_rhythm_memory_element *curr;
	t_rhythm_memory_element *newElement;
	t_rhythm_event *currEvent;
	t_rhythm_memory_arc *currArc, *newArc, *prevArc;
	unsigned short int last, sub_id;
	int i, arcFound;
	if (new_rhythm==0)
		return INVALID_RHYTHM;
	// creates a new element of the list of similar rhythms
	newElement = (t_rhythm_memory_element *) malloc(sizeof(t_rhythm_memory_element));
	newElement->rhythm = new_rhythm;
	newElement->next = 0;
	sub_id = this_rep->last_sub_id;
	newElement->id = sub_id;
	this_rep->last_sub_id++;
	// finds the last element and adds itself
	curr = this_rep->rhythms;
	if (curr)
	{
		while (curr->next)
			curr=curr->next;
		curr->next = newElement;

	} else
	{
		this_rep->rhythms = newElement;
	}
	// now update the transition table..
	currEvent = new_rhythm;
	// set the first event..
	i = duration2int(new_rhythm->start);
	this_rep->transitions[i].first++;
	last = 0;
	while (currEvent)
	{
		// get the duration and translate into an int
		i = duration2int(currEvent->start);
		if (last) // NB if last==0 then last is not set
		{
			// add an arc between last and i
			currArc = this_rep->transitions[last].arcs;
			arcFound=0;
			// is this arc rpesent?
			// also i need to get to the last element of the lsit
			while (currArc)
			{
				// for each arc in the list
				if (currArc->to_node_index == i)
				{
					// this arc is already present
					arcFound=1;
				}
				prevArc = currArc; // last valid arc
				currArc = currArc->next_arc;
			} 
			if (!arcFound)
			{
				// this arc was not present, add it!
				newArc = (t_rhythm_memory_arc *) malloc(sizeof(t_rhythm_memory_arc));
				newArc->next_arc = 0; 
				newArc->to_node_index = i; // set my destination
				if (this_rep->transitions[last].arcs)
				{
					// this is not the first arc
					// then prevArc is set and valid
					prevArc->next_arc = newArc;
				} else
				{
					// this is the first arc
					this_rep->transitions[last].arcs = newArc;
				}
			}
		}
		// increment the weight
		this_rep->transitions[i].weight++;
		if (this_rep->transitions[i].weight > this_rep->max_weight)
		{
			// a new champion!
			this_rep->max_weight = this_rep->transitions[i].weight;
			//post("DEBUG: max_weight=%i", this_rep->max_weight);
		}
		last = i;
		currEvent = currEvent->next;
	}
	return sub_id;
}

// free the list of representations
void free_memory_representations(t_rhythm_memory_representation *this_rep)
{
	int i, maxi;
	t_rhythm_memory_representation *currRep, *oldRep;
	t_rhythm_memory_element *currElement, *tmpElement;
	t_rhythm_memory_arc *currArc, *tmpArc;
	currRep = this_rep;
	while(currRep)
	{
		// free the table
		maxi = possible_durations();
		for (i=0; i<maxi; i++)
		{
			currArc = currRep->transitions[i].arcs;
			while (currArc)
			{
				tmpArc = currArc;
				currArc = currArc->next_arc;
				free(tmpArc);
			}
		}
		free(currRep->transitions);

		// free the list of similar rhythms
		currElement = currRep->rhythms;
		while (currElement)
		{
			freeBeats(currElement->rhythm);
			tmpElement = currElement;
			currElement = currElement->next;
			free(tmpElement);
		}
		oldRep = currRep;
		currRep = currRep->next;
		free(oldRep);
	}

}

void create_array_beats(unsigned short int **this_array, t_rhythm_event *currentEvent)
{
	unsigned short int *new_array;
	//t_rhythm_event *curr_event;
	int i, maxi, startint;
	maxi = possible_durations();
	// allocate space for the nodes
	new_array = (unsigned short int *) malloc(sizeof(unsigned short int) * maxi);
	// set default values
	for (i=0; i<maxi; i++)
	{
			new_array[i] = 0;
	}
	// set the actual data
	//curr_event = currentEvent;
	while(currentEvent)
	{
		startint = duration2int(currentEvent->start);
		new_array[startint]=1;
		currentEvent = currentEvent->next;
	}
	*this_array = new_array;

}

// compares this rhythm to this representation
// and tells you how close it is to it
void compare_rhythm_vs_representation(t_rhythm_memory_representation *this_rep, 
						 t_rhythm_event *src_rhythm, // the src rhythm 
						 unsigned short int *sub_id, // the sub-id of the closest sub-rhythm 
						 float *root_closeness, // how much this rhythm is close to the root (1=identical, 0=nothing common)
						 float *sub_closeness // how much this rhythm is close to the closest sub-rhythm (1=identical, 0=nothing common)
						 )
{
	t_duration tmp_dur, this_dur;
	t_rhythm_event *curr_event;
	float this_weight_float, average_weight, strong_ratio;
	int i, max_i, int_from_dur, this_weight_int, beats, strong_ok, strong_no;
	unsigned short int *src_rhythm_array, *tmp_rhythm_array;
	unsigned short int best_subid, curr_subid;
	float best_closeness, curr_closeness;
	int sub_ok, sub_no, sub_count;
	int count_strong;
	t_rhythm_memory_element *possible_rhythms;

	// check that the return values have been allocated
	if ((sub_id==0)||(root_closeness==0)||(sub_closeness==0))
	{
		post("error in find_similar_rhythm(): return values not allocated");
		return;
	}

	max_i = possible_durations();
	// create an array of bools
	create_array_beats(&src_rhythm_array, src_rhythm);

	// look the main table for closeness to the main rhythm
	curr_event = src_rhythm;
	beats=0;
	strong_ok=0;
	strong_no=0;
	strong_ratio=0;
	/*
	average_weight = 0;
	while(curr_event)
	{
		int_from_dur = duration2int(curr_event->start);
		// get the weight of this beat
		this_weight_int = this_rep->transitions[int_from_dur].weight;
		this_weight_float = (float) ( ((float) this_weight_int) / ((float) this_rep->max_weight));
		average_weight += this_weight_float;
		beats++;
		curr_event = curr_event->next;
	}
	// this is the average weight of this rhythm in this representation
	if (beats==0)
	{
		average_weight = 0;
	} else
	{
		average_weight = (float) (average_weight / ((float) beats));
	}
	*/
	// look all the representation's rhythm 
	// looking for strong beats corrispondance
	count_strong = 0;
	for (i=0; i<max_i; i++)
	{
			this_weight_int = this_rep->transitions[i].weight;
			this_weight_float = (float) (((float) this_weight_int) / ((float) this_rep->max_weight));
			//post("DEBUG: transition %i this_weight_int=%i max_weight=%i this_weight_float=%f", i, this_weight_int, this_rep->max_weight, this_weight_float);
			if (this_weight_float > min_to_be_main_rhythm_beat)
			{
				// this is a main rhythm beat
				if (src_rhythm_array[i]>0)
				{
					// both playing
					strong_ok++;
					//post("DEBUG: beat %i, both playing", i);
				} else
				{
					// strong beat miss
					strong_no++;
					//post("DEBUG: beat %i, src not playing", i);
				}
				count_strong++;
			}	
	}

	// ratio of corresponding strong beats.. 
	// close to 0 = no corrispondance
	// close to 1 = corrispondance
	if (count_strong==0)
	{
		strong_ratio = 0;
	} else
	{
		strong_ratio = (float) ( ( ((float) strong_ok) / ((float)count_strong) ) -
			( ((float) strong_no) / ((float)count_strong) )) ;
		//post("DEBUG: strong_ratio=%f", strong_ratio);
	}
	// for each rhythm in the list
	// count the number of identical nodes
	// cound thenumber of different nodes
	best_subid = curr_subid = INVALID_RHYTHM;
	best_closeness = curr_closeness = 0;
	possible_rhythms = this_rep->rhythms;
	while(possible_rhythms)
	{
		// create the table of this rhythm
		create_array_beats(&tmp_rhythm_array, possible_rhythms->rhythm);
		sub_ok = sub_no = sub_count = 0;
		for (i=0; i<max_i; i++)
		{
			if (tmp_rhythm_array[i]>0  && src_rhythm_array[i]>0)
			{
				sub_ok++;
			} else if (tmp_rhythm_array[i]==0  && src_rhythm_array[i]==0)
			{
				// nothing important
			} else
			{
				sub_no++;
			}
			sub_count++;
		}
		if (sub_no == 0)
		{
			curr_closeness = 1;
		} else
		{
			curr_closeness = (float) (( ((float) sub_ok) - ((float) sub_no) ) / ((float) sub_count));
		}
		if (curr_closeness > best_closeness)
		{
			best_closeness = curr_closeness;
			best_subid = possible_rhythms->id;
		}
		possible_rhythms = possible_rhythms->next;
		free(tmp_rhythm_array);
	}

	// return the better matching rhythm id
	// and the closeness floats

	*sub_id = best_subid;
	*sub_closeness = best_closeness;
	*root_closeness = strong_ratio;

	// free allocated memory
	free(src_rhythm_array);
}

void find_rhythm_in_memory(t_rhythm_memory_representation *rep_list, 
						 t_rhythm_event *src_rhythm, // the src rhythm 
						 unsigned short int *id, // the id of the closest rhythm
						 unsigned short int *sub_id, // the sub-id of the closest sub-rhythm 
						 float *root_closeness, // how much this rhythm is close to the root (1=identical, 0=nothing common)
						 float *sub_closeness // how much this rhythm is close to the closest sub-rhythm (1=identical, 0=nothing common)
						 )
{
	unsigned short int best_id, curr_id, best_subid, curr_subid;
	float best_closeness, curr_closeness, best_sub_closeness, curr_sub_closeness;
	t_rhythm_memory_representation *this_rep;
	best_closeness = curr_closeness = best_sub_closeness = curr_sub_closeness = 0;
	best_id = curr_id = best_subid = curr_subid = INVALID_RHYTHM;
	// for each element of the rep_list
	this_rep = rep_list;
	while(this_rep)
	{
		// if max_weight maxweight == 0 then there are no rhythms and no table
		if (this_rep->max_weight)
		{
			compare_rhythm_vs_representation(this_rep, 
				src_rhythm, 
				&curr_subid, 
				&curr_closeness, 
				&curr_sub_closeness);
			if (curr_closeness > best_closeness)
			{
				best_closeness = curr_closeness;
				best_id = this_rep->id;
				best_sub_closeness = curr_sub_closeness;
				best_subid = curr_subid;
			}
		}
		this_rep = this_rep->next;
	}

	*id = best_id;
	*sub_id = best_subid;
	*root_closeness = best_closeness;
	*sub_closeness = best_sub_closeness;
	

}


// create a new memory for rhythms
void rhythm_memory_create(t_rhythm_memory_representation **this_rep)
{
	create_rhythm_memory_representation(this_rep, 0);
}

// free the space 
void rhythm_memory_free(t_rhythm_memory_representation *rep_list)
{
	t_rhythm_memory_representation *curr, *nextRep;
	nextRep = rep_list;
	while (nextRep)
	{
		curr = nextRep;
		nextRep = curr->next;
		free_memory_representations(curr);
	}

}
// evaluate this rhythm and add it to the memory if is new
void rhythm_memory_evaluate(t_rhythm_memory_representation *rep_list, // the memory
							t_rhythm_event *src_rhythm, // the rhythm to evaluate
							// is it a new rhythm? (0 if no, 1 if new main rhythm, 2 if new subrhythm)
							unsigned short int *new_rhythm,
							// the id of the closest rhythm or the new id assigned
							unsigned short int *id, 
							// the sub-id of the closest sub-rhythm or the new sub-id assigned
							unsigned short int *sub_id,
							// how much this rhythm is close to the root (1=identical, 0=nothing common)
							float *root_closeness,
							// how much this rhythm is close to the closest sub-rhythm (1=identical, 0=nothing common)
							float *sub_closeness 
							)
{
	float root_closeness_found, sub_closeness_found;
	unsigned short int id_found, sub_id_found, new_id;
	t_rhythm_memory_representation *curr, *newRep, *lastRep;
	int found;

	// look for the closest main rhythm and subrhythm
	find_rhythm_in_memory(rep_list, 
						 src_rhythm, // the src rhythm 
						 &id_found, // the id of the closest rhythm
						 &sub_id_found, // the sub-id of the closest sub-rhythm 
						 &root_closeness_found, // how much this rhythm is close to the root (1=identical, 0=nothing common)
						 &sub_closeness_found // how much this rhythm is close to the closest sub-rhythm (1=identical, 0=nothing common)
						 );

	// decide if add to the memory or if return the closest

	if (root_closeness_found >= (float)min_to_be_same_rhythm)
	{
		// is close enough to be considered a rhythm
		post("DEBUG: rhythm found");
		*new_rhythm = 0;
		*id = id_found;
		*root_closeness = root_closeness_found;
		if (sub_closeness_found >= (float)min_to_be_same_subrhythm)
		{
			// this is a known subrhythm
			post("DEBUG: sub-rhythm found");
			*sub_id = sub_id_found;
			*sub_closeness = sub_closeness_found;
			//only in this case free the memory allocated for the rhythm
			freeBeats(src_rhythm);
		} else
		{
			// add this rhythm as a new subrhythm
			curr = rep_list;
			found = 0;
			while (curr && !found)
			{
				if (curr->id == id_found)
				{
					// i've found the rhythm
					found = 1;
				} else
				{
					curr = curr->next;
				}
			}

			sub_id_found = add_t_rhythm_memory_element(curr, src_rhythm);
			sub_closeness_found = 1;
			*sub_id = sub_id_found;
			*sub_closeness = sub_closeness_found;
			*new_rhythm = 2;
			//post("DEBUG: new subrhythm");
		}
	} else
	{
		// this is a completely new rhythm!
		// find the last id and representation
		curr = rep_list;
		new_id = 0;
		lastRep = 0;
		while (curr)
		{
			new_id = curr->id;
			lastRep = curr;
			curr = curr->next;
		}	
		// create a new representation with a new id
		new_id++;
		create_rhythm_memory_representation(&newRep, new_id);
		// link the representations
		lastRep->next = newRep;
		// add the rhythm as subrhythm
		sub_id_found = add_t_rhythm_memory_element(newRep, src_rhythm);
		sub_closeness_found = 1;
		*sub_id = sub_id_found;
		*sub_closeness = sub_closeness_found;
		*id = new_id;
		*root_closeness = 1;
		*new_rhythm = 1;
		//post("DEBUG: new rhythm");
	}

}
// return 0 if failed finding the rhythm, 1 if the rhythm was found
int rhythm_memory_get_rhythm(t_rhythm_memory_representation *rep_list, // the memory
							  t_rhythm_event **out_rhythm, // a pointer to the returned rhythm
							  // the id of the main rhythm wanted
							  unsigned short int id, 
							  // the sub-id of the sub-rhythm wanted
							  unsigned short int sub_id)
{
	// look for this id and subid in memory and return that subrhythm
	t_rhythm_memory_representation *curr;
	t_rhythm_memory_element *curr2;
	curr = rep_list;
	while (curr)
	{
		if (curr->id == id)
		{
			// this is the right main rhythm
			// now look for the correct subid
			curr2 = curr->rhythms;
			while (curr2)
			{
				if (curr2->id == sub_id)
				{
					// i've found the rhythm!
					*out_rhythm=curr2->rhythm;
					return 1;
				}
				curr2 = curr2->next;
			}

		}

		curr = curr->next;
	}
	// if i am here then i didn't find the rhythm
	return 0;
}


// ------------------- themes manipulation functions

// set the first note of a sequence
void setFirstNote(t_note_event **firstEvent, unsigned short int voice, float fstart, float fduration, t_note note)
{
	t_duration res;
	t_note_event *newElement;
	// convert from float to duration
	res = float2duration(fduration);
	// allocate a new element of the list
	newElement = malloc(sizeof(t_note_event));
	// set the pointers
	newElement->previous = 0;
	newElement->next = 0;
	newElement->voice=voice;
	newElement->note.chord = note.chord;
	newElement->note.diatonic = note.diatonic;
	newElement->note.interval = note.interval;
	newElement->duration.numerator = res.numerator;
	newElement->duration.denominator = res.denominator;
	res = float2duration(fstart);
	newElement->start.numerator = res.numerator;
	newElement->start.denominator = res.denominator;
	*firstEvent = newElement;
}

//adds a note at the end of this list
void concatenateNote(t_note_event *currentEvent, unsigned short int voice, float fstart, float fduration, t_note note)
{
	t_duration res;
	t_note_event *newElement, *lastElement;
	lastElement = currentEvent;
	while(lastElement->next)
		lastElement = lastElement->next;
	// convert from float to duration
	res = float2duration(fduration);
	// allocate a new element of the list
	newElement = (t_note_event *) malloc(sizeof(t_note_event));
	// set the pointers
	newElement->previous = lastElement;
	newElement->next = 0;
	lastElement->next = newElement;
	newElement->voice=voice;
	newElement->note.chord = note.chord;
	newElement->note.diatonic = note.diatonic;
	newElement->note.interval = note.interval;
	newElement->duration.numerator = res.numerator;
	newElement->duration.denominator = res.denominator;
	res = float2duration(fstart);
	newElement->start.numerator = res.numerator;
	newElement->start.denominator = res.denominator;
}

// used to free the memory allocated by this list
void freeNotes(t_note_event *currentEvent)
{
	t_note_event *prev;
	t_note_event *next;

	// go to the first element of the list
	while(currentEvent->previous)
		currentEvent = currentEvent->previous;

	// now free each element
	next=currentEvent->next;
	do
	{
		prev = currentEvent;
		next = currentEvent->next;
		free(currentEvent);
	} while(next);

}


// ------------- function for string manipulation (from string to chords)

// tries to find out absolute tones names in this string
abs_note_t from_string_to_abs_tone(const char *substr)
{
	if (strstr(substr, "C"))
		return C;
	if (strstr(substr, "Db"))
		return Db;
	if (strstr(substr, "D"))
		return D;
	if (strstr(substr, "Eb"))
		return Eb;
	if (strstr(substr, "E"))
		return E;
	if (strstr(substr, "F"))
		return F;
	if (strstr(substr, "Gb"))
		return Gb;
	if (strstr(substr, "G"))
		return G;
	if (strstr(substr, "Ab"))
		return Ab;
	if (strstr(substr, "A"))
		return A;
	if (strstr(substr, "Bb"))
		return Bb;
	if (strstr(substr, "B"))
		return B;
	return C;
}
/*
chord_type_t from_string_to_type(const char *substr)
{
	if (strstr(substr, "minor/major 7th"))
		return kMinMaj7;
	if (strstr(substr, "major 7th"))
		return kMaj7;
	if (strstr(substr, "major"))
		return kMaj;
	if (strstr(substr, "minor 7th"))
		return kMin7;
	if (strstr(substr, "minor"))
		return kMin;
	if (strstr(substr, "half diminished 7th"))
		return kHalfDim7;
	if (strstr(substr, "diminished 7th"))
		return kDim7;
	if (strstr(substr, "diminished"))
		return kDim;
	if (strstr(substr, "augmented"))
		return kAug;
	if (strstr(substr, "dominant 7th"))
		return kDom7;
	// pland adding chords 30.11.05
	if (strstr(substr, "dominant b9"))
		return kDomb9;
	if (strstr(substr, "major 9th"))
		return kMaj9;
	if (strstr(substr, "dominant 9th"))
		return kDom9;
	if (strstr(substr, "minor 9th"))
		return kMin9;
	if (strstr(substr, "half diminished 9th"))
		return kHalfDim9;
	if (strstr(substr, "minor major 9th"))
		return kMinMaj9;
	if (strstr(substr, "diminished major 9th"))
		return kDimMaj9;
	// TODO: other chords
	// beware when adding new chords
	// put shorter names at end of this function!
	return C;
}
*/
// find the tonality mode in this string
modes_t from_string_to_mode(const char *substr)
{
	if (strstr(substr, "major"))
		return MAJOR;
	if (strstr(substr, "minor"))
		return MINOR;

	// TODO: other modes (doric, misolidian , custom, etc..
	return C;
}

// tries to find out absolute tones names in this string
abs_note_t string2note(const char *substr)
{
	if (strstr(substr, "C"))
		return C;
	if (strstr(substr, "Db"))
		return Db;
	if (strstr(substr, "D"))
		return D;
	if (strstr(substr, "Eb"))
		return Eb;
	if (strstr(substr, "E"))
		return E;
	if (strstr(substr, "F"))
		return F;
	if (strstr(substr, "Gb"))
		return Gb;
	if (strstr(substr, "G"))
		return G;
	if (strstr(substr, "Ab"))
		return Ab;
	if (strstr(substr, "A"))
		return A;
	if (strstr(substr, "Bb"))
		return Bb;
	if (strstr(substr, "B"))
		return B;
	return C;
}

chord_type_t string2mode(const char *substr)
{
	// beware when adding new chords
	// put shorter names at end of this function!
	if (strstr(substr, "unison"))
		return kUnison;
	if (strstr(substr, "major 7 b9 13"))
		return kM7b913;
	if (strstr(substr, "major 7th #13"))
		return kMaj7s13;
	if (strstr(substr, "major 7 b9 #13"))
		return kM7b9s13;
	if (strstr(substr, "dominant 7th b13"))
		return kDom7b13;
	if (strstr(substr, "major 7th #5"))
		return kMaj7s5;
	if (strstr(substr, "major 7th b5"))
		return kMaj7b5;
	if (strstr(substr, "dominant 7th #5"))
		return kDom7s5;
	if (strstr(substr, "dominant 7th b5"))
		return kDom7b5;
	if (strstr(substr, "half diminished 9th"))
		return kHalfDim9;
	if (strstr(substr, "minor major 9th"))
		return kMinMaj9;
	if (strstr(substr, "diminished major 9th"))
		return kDimMaj9;
	if (strstr(substr, "major 9th b5"))
		return kMaj9b5;
	if (strstr(substr, "dominant 9th b5"))
		return kDom9b5;
	if (strstr(substr, "dominant 9th b13"))
		return kDom9b13;
	if (strstr(substr, "minor 9th #11"))
		return kMin9s11;
	if (strstr(substr, "minor/maj 9th b11"))
		return kmM9b11;
	if (strstr(substr, "major 7th b9"))
		return kMaj7b9;
	if (strstr(substr, "major 7th #5 b9"))
		return kMaj7s5b9;
	if (strstr(substr, "dominant 7th b9"))
		return kDom7b9;
	if (strstr(substr, "minor 7th b9"))
		return kMin7b9;
	if (strstr(substr, "minor b9 #11"))
		return kMinb9s11;
	if (strstr(substr, "half diminished b9"))
		return kHalfDimb9;
	if (strstr(substr, "diminished b9"))
		return kDim7b9;
	if (strstr(substr, "minor/major b9"))
		return kMinMajb9;
	if (strstr(substr, "diminished M7 b9"))
		return kDimMajb9;
	if (strstr(substr, "major 7th #9"))
		return kMaj7s9;
	if (strstr(substr, "dominant #9"))
		return kDom7s9;
	if (strstr(substr, "major 7th #11"))
		return kMaj7s11;
	if (strstr(substr, "major 9th #13"))
		return kMaj9s13;
	if (strstr(substr, "major #9 #11"))
		return kMs9s11;
	if (strstr(substr, "half diminished b11"))
		return kHDimb11;
	if (strstr(substr, "major 11th"))
		return kMaj11;
	if (strstr(substr, "dominant 11th"))
		return kDom11;
	if (strstr(substr, "minor 11th"))
		return kMin11;
	if (strstr(substr, "half diminished 11th"))
		return kHalfDim11;
	if (strstr(substr, "diminished 11th"))
		return kDim11;
	if (strstr(substr, "minor/major 11th"))
		return kMinMaj11;
	if (strstr(substr, "diminished maj 11th"))
		return kDimMaj11;
	if (strstr(substr, "major 11th b5"))
		return kMaj11b5;
	if (strstr(substr, "major 11th #5"))
		return kMaj11s5;
	if (strstr(substr, "major 11th b9"))
		return kMaj11b9;
	if (strstr(substr, "major 11th #9"))
		return kMaj11s9;
	if (strstr(substr, "major 11th b13"))
		return kMaj11b13;
	if (strstr(substr, "major 11th #13"))
		return kMaj11s13;
	if (strstr(substr, "major 11th b5 b9"))
		return kM11b5b9;
	if (strstr(substr, "dominant 11th b5"))
		return kDom11b5;
	if (strstr(substr, "dominant 11th b9"))
		return kDom11b9;
	if (strstr(substr, "dominant 11th #9"))
		return kDom11s9;
	if (strstr(substr, "half dim 11th b9"))
		return kHalfDim11b9;
	if (strstr(substr, "dominant #11"))
		return kDom7s11;
	if (strstr(substr, "minor 7th #11"))
		return kMin7s11;
	if (strstr(substr, "dominant 13th #11"))
		return kDom13s11;
	if (strstr(substr, "chromatic"))
		return kChrom;
	if (strstr(substr, "dominant b9"))
		return kDomb9;
	if (strstr(substr, "major 9th"))
		return kMaj9;
	if (strstr(substr, "dominant 9th"))
		return kDom9;
	if (strstr(substr, "minor 9th"))
		return kMin9;
	if (strstr(substr, "minor/major 7th"))
		return kMinMaj7;
	if (strstr(substr, "major 7th"))
		return kMaj7;
	if (strstr(substr, "half diminished 7th"))
		return kHalfDim7;
	if (strstr(substr, "diminished 7th"))
		return kDim7;
	if (strstr(substr, "major"))
		return kMaj;
	if (strstr(substr, "minor 7th"))
		return kMin7;
	if (strstr(substr, "minor"))
		return kMin;
	if (strstr(substr, "diminished"))
		return kDim;
	if (strstr(substr, "augmented"))
		return kAug;
	if (strstr(substr, "dominant 7th"))
		return kDom7;

	return kUnison;
}

