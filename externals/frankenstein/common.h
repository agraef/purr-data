/*

header file for common functions used by the externals of the "frank framework".

the frank framework is a set of pd externals implementing AI agents for improvisation.

Authors:
Davide Morelli http://www.davidemorelli.it
David Plans Casal http://www.studios.uea.ac.uk/people/staff/casal

*/

// here i put common data structures and functions

// ------------------------------------------------ data structures


// --------- theme notation


typedef struct t_note t_note;
typedef struct t_duration t_duration;
typedef struct t_note_event t_note_event;

typedef struct t_note
{
	unsigned short int chord; // 0 if is not a chord (strong) note, 1 if it is
	unsigned short int diatonic; // 0 if is a note not belonging to this tonality
	int interval; // semitones from prefious note
};
typedef struct t_duration
{
	unsigned short int numerator; // like in music notation: in a 1/4 note the numerator is 1
	unsigned short int denominator; // like in music notation: in a 1/4 note the denominator is 4
};
struct t_note_event
{
	unsigned short int voice;
	t_note note;
	t_duration start; // moment of the noteon event
	t_duration duration; // duration of this event
	t_note_event *previous; // this is a list, link to the previous element
	t_note_event *next;  // this is a list, link to the next element
};


// --------- rhythm notation

// data structures

// this describes a rhythm
typedef struct t_rhythm_event t_rhythm_event;
struct t_rhythm_event
{
	unsigned short int voice;
	t_duration start; // moment of the noteon event
	t_duration duration; // duration of this event
	t_rhythm_event *previous; // this is a list, link to the previous element
	t_rhythm_event *next;  // this is a list, link to the next element
};

// rhythms memory graph

// list implementation
// this implements a graph that stores the memory of the current rhythm sub-elements
// list of links
// the actual implementation will be an array of nodes, each node 

// this describes a probability transition table
typedef struct t_rhythm_memory_arc t_rhythm_memory_arc;
typedef struct t_rhythm_memory_node t_rhythm_memory_node;
// graph node
struct t_rhythm_memory_node
{
	// start is implicit:
	// this is in an array
	// use int2duration and duration2int 
	// against the index in the array to get its value
	unsigned short int first; // how many times this has been the first event
	unsigned short int weight; // how many times this has been an event
	t_rhythm_memory_arc *arcs; // the list of arcs to other nodes
} ;
// graph arc: related to t_rhythm_memory_node
struct t_rhythm_memory_arc
{
	unsigned short int to_node_index; // the target of this link (arc)
	t_rhythm_memory_arc *next_arc; // next link in the list
} ;
// it will be arranged in a heap list.. ?

//#define num_possible_denominators 11
//static unsigned short int possible_denominators[] = {1,2,3,4,6,8,12,16,18,24,32};
#define num_possible_denominators 7
static unsigned short int possible_denominators[] = {1,2,3,4,6,8,12};

// the minimum percentage for a beat to be considered part of the main rhythm
#define min_to_be_main_rhythm_beat 0.7
// minimum value to be considered a subrhythm of this rhythm
#define min_to_be_same_rhythm 0.7
// minimum percentage to be considered this exact rhythm
#define min_to_be_same_subrhythm 0.9

// this defines a space for rhythms, variations, transitions and representations
typedef struct t_rhythm_memory_representation t_rhythm_memory_representation;
typedef struct t_rhythm_memory_element t_rhythm_memory_element;
typedef struct t_rhythm_memory_first_node t_rhythm_memory_first_node;
// element of a list of rhythms
struct t_rhythm_memory_element
{
	t_rhythm_event *rhythm; // this rhythm
	t_rhythm_memory_element *next; // next element of the list
	unsigned short int id; // its sub id
} ;
// a rhythm in memory, each rhythm is :
// - its probability transition table
// - similar rhythms played
// - each one has its main id and each different played rhythm its sub-id
struct t_rhythm_memory_representation
{
	t_rhythm_memory_node *transitions;
	unsigned short int max_weight;
	t_rhythm_memory_element *rhythms;
	unsigned short int id; // its main id
	unsigned short int last_sub_id; // last sub assigned
	// I can express a list of representations with this data structure
	t_rhythm_memory_representation *next;
} ;

// define a return value to express "rhythm not found in this representation"
#define INVALID_RHYTHM 65535

// chords data structure
// tells you how many durations there // how can a chord be?
#define TYPES_NUM 69 // keep me updated
typedef enum {
	// this enumerator is from maxlib chord 
 kUnison = 0,
kMaj = 1, 
kMin = 2, 
kDim = 3, 
kAug = 4, 
kMaj7 = 5, 
kDom7 = 6, 
kMin7 = 7, 
kHalfDim7 = 8, 
kDim7 = 9, 
kMinMaj7 = 10, 
kMaj7s5 = 11,
kMaj7b5 = 12,
kDom7s5 = 13,
kDom7b5 = 14,
kDomb9 = 15, 
kMaj9 = 16,
kDom9 = 17,
kMin9 = 18,
kHalfDim9 = 19,
kMinMaj9 = 20,
kDimMaj9 = 21,
kMaj9b5 = 22,
kDom9b5 = 23,
kDom9b13 = 24,
kMin9s11 = 25,
kmM9b11 = 26,
kMaj7b9 = 27,
kMaj7s5b9 = 28,
kDom7b9 = 29,
kMin7b9 = 30,
kMinb9s11 = 31,
kHalfDimb9 = 32,
kDim7b9 = 33,
kMinMajb9 = 34, 
kDimMajb9 =35,
kMaj7s9 = 36,
kDom7s9 = 37,
kMaj7s11 = 38,
kMs9s11 = 39,
kHDimb11 = 40,
kMaj11 = 41,
kDom11 = 42,
kMin11 = 43,
kHalfDim11 = 44,  
kDim11 = 45,
kMinMaj11 =46, 
kDimMaj11 =47,
kMaj11b5 = 48,
kMaj11s5 = 49,
kMaj11b9 = 50,
kMaj11s9 = 51,
kMaj11b13 = 52,
kMaj11s13 = 53,
kM11b5b9 = 54,
kDom11b5 = 55,
kDom11b9 = 56,
kDom11s9 = 57,
kHalfDim11b9 = 58,
kDom7s11 = 59,
kMin7s11 = 60,
kDom13s11 = 61,
kM7b913 = 62,
kMaj7s13 = 63,
kMaj9s13 = 64,
kM7b9s13 = 65,
kDom7b13 = 66,
kChrom = 67,
kNone = 68
			} chord_type_t;

// how many tones do we have in our octave?
#define TONES_NUM 12 // keep me updated
typedef enum {
			I=0,
			Id=1,
			II=2,
			IId=3,
			III=4,
			IV=5,
			IVd=6,
			V=7,
			Vd=8,
			VI=9,
			VId=10,
			VII=11			
			} chord_tone_t;

// how many nodes does this graph have?
// for now TYPES_NUM*TONES_NUM
// when we introduce modulation
// we'll have more
#define NODES_NUM TYPES_NUM*TONES_NUM

// this defines a chord in a tonality
typedef struct _chord
{
	chord_type_t mode;
	chord_tone_t note;
} chord_t;

// enumeration of absolute notes 
// i'll need this when parsing strings like "C major"
typedef enum {
			C=0,
			Db=1,
			D=2,
			Eb=3,
			E=4,
			F=5,
			Gb=6,
			G=7,
			Ab=8,
			A=9,
			Bb=10,
			B=11			
			} abs_note_t;

// enumeration of modes
// i'll start with minor and major only
// but we could add phrigian, doric, misolidian ,e tc...
#define MODES_NUM 2
typedef enum {
		MAJOR=0,
		MINOR=1	} modes_t;

#define MODULATIONS_NUM MODES_NUM*TONES_NUM



// ------------------------------------------------ functions

// ----------- rhythm manipolation functions

// converts from integer to duration: used to know this table index
// what corresponds in terms of duration
t_duration int2duration(int n);
// converts from duration to integer: used to know this duration
// what corresponds in terms table index
unsigned short int duration2int(t_duration dur);

int possible_durations();

// converts from float (0-1) to duration. it performs quantization
t_duration float2duration(float fduration);

// converts from numerator/denominator to a float (0-1)
float duration2float(t_duration duration);

// --- rhythms creation and manupulation functions

// set the first beat of a sequence
// this also creates a new rhythm
void setFirstBeat(t_rhythm_event **firstEvent, unsigned short int voice, float fstart, float fduration);

//adds a beat at the end of this list
void concatenateBeat(t_rhythm_event *currentEvent, unsigned short int voice, float fstart, float fduration);

// used to free the memory allocated by this list
void freeBeats(t_rhythm_event *currentEvent);


// --- memory representation of rhythms

// create an array of nodes (without arcs?) to express the beats in this rhythm (the noteon moments)
void create_array_beats(unsigned short int **this_array, t_rhythm_event *currentEvent);

// add an arc to this node
void add_t_rhythm_memory_arc(t_rhythm_memory_node *srcNode, unsigned short int dstNode);

// create and initialize this representation, allocate memory for the pointers
// I must pass its id also
void create_rhythm_memory_representation(t_rhythm_memory_representation **this_rep, unsigned short int id);

// add a new rhythm in the list of similar rhythms related to one main rhythm
// the sub id is auto-generated and returned
unsigned short int add_t_rhythm_memory_element(t_rhythm_memory_representation *this_rep, t_rhythm_event *new_rhythm);

// free the list of representations
void free_memory_representations(t_rhythm_memory_representation *this_rep);

// compares this rhythm to this representation
// and tells you how close it is to it
// I return values using pointers
// the unsigned short and the 2 floats should be already allocated
void compare_rhythm_vs_representation(t_rhythm_memory_representation *this_rep, 
						 t_rhythm_event *src_rhythm, // the src rhythm 
						 unsigned short int *sub_id, // the sub-id of the closest sub-rhythm 
						 float *root_closeness, // how much this rhythm is close to the root (1=identical, 0=nothing common)
						 float *sub_closeness // how much this rhythm is close to the closest sub-rhythm (1=identical, 0=nothing common)
						 );

// same as before but search all available representations
void find_rhythm_in_memory(t_rhythm_memory_representation *rep_list, 
						 t_rhythm_event *src_rhythm, // the src rhythm 
						 unsigned short int *id, // the id of the closest rhythm
						 unsigned short int *sub_id, // the sub-id of the closest sub-rhythm 
						 float *root_closeness, // how much this rhythm is close to the root (1=identical, 0=nothing common)
						 float *sub_closeness // how much this rhythm is close to the closest sub-rhythm (1=identical, 0=nothing common)
						 );

// the following are the functions that externals should use
/* usage:

	// first of all declare a pointer for the memory
	t_rhythm_memory_representation *rhythms_memory;
	// initialize it
	rhythm_memory_create(&this_rep);
	// then each time you get a rhythm let the memory evaluate it and
	// tell you if is a new rhythm or a old one
	float root_closeness, sub_closeness;
	unsigned short int id, subid, is_it_a_new_rhythm;
	rhythm_memory_evaluate(rhythms_memory, rhythm, &is_it_a_new_rhythm,
							&id, &subid, &root_closeness, &sub_closeness);
	if (is_it_a_new_rhythm==1)
	{
		// it was a completely new rhythm
		// id tells us new id assigned
		// and subid tells us the new sub id assigned
	}
	if (is_it_a_new_rhythm==2)
	{
		// it was a new sub-rhythm of a known rhythm
		// id tells us rht root rhythm id
		// and subid tells us the new sub id assigned
	}
	if (is_it_a_new_rhythm==0)
	{
		// it was a known rhythm and subrhythm
		// id and subid tell us the identificator
	}
	// i can also use root_closeness and sub_closeness and is_it_a_new_rhythm
	// to know how much novelty there was in this rhythm

	// i can ask the memory to give me back a specific rhythm
	t_rhythm_event *wanted_rhythm
	int rhythm_found = rhythm_memory_get_rhythm(rhythms_memory,
							  wanted_rhythm,
							  id, // the id of the main rhythm wanted
							  sub_id // the sub-id of the sub-rhythm wanted
							  );
	if (rhythm_found == 0)
	{
		// that rhythm was not present!
	}

	// when i am ready I should free the memory
	rhythm_memory_free(rhythms_memory);
*/


// create a new memory for rhythms
void rhythm_memory_create(t_rhythm_memory_representation **this_rep);
// free the space 
void rhythm_memory_free(t_rhythm_memory_representation *rep_list);
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
							);
// return 0 if failed finding the rhythm, 1 if the rhythm was found
int rhythm_memory_get_rhythm(t_rhythm_memory_representation *rep_list, // the memory
							  t_rhythm_event **out_rhythm, // a pointer to the returned rhythm
							  // the id of the main rhythm wanted
							  unsigned short int id, 
							  // the sub-id of the sub-rhythm wanted
							  unsigned short int sub_id);

// -------- notes manipulation functions

// set the first beat of a sequence
void setFirstNote(t_note_event **firstEvent, unsigned short int voice, float fstart, float fduration, t_note note);

//adds a beat at the end of this list
void concatenateNote(t_note_event *currentEvent, unsigned short int voice, float fstart, float fduration, t_note note);

// used to free the memory allocated by this list
void freeNotes(t_note_event *currentEvent);




// ------------- function for string manipulation (from string to chords)

// tries to find out absolute tones names in this string
abs_note_t from_string_to_abs_tone(const char *substr);
chord_type_t from_string_to_type(const char *substr);
modes_t from_string_to_mode(const char *substr);
chord_type_t string2mode(const char *substr);
abs_note_t string2note(const char *substr);
