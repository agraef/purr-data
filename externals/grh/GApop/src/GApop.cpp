//*************************************************************************
// GApop - external for PD and MAX/MSP
// 
// This is a genetic algorithm, see the PD help-file
// how to use it
// 
// Copyright (c) 2004 Georg Holzmann <grh@mur.at>
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "license.txt," in this distribution.  
// 
// You'll need flext by Thomas Grill to compile this external
//*************************************************************************



// IMPORTANT: enable attribute processing (specify before inclusion of flext headers!)
// For clarity, this is done here, but you'd better specify it as a compiler definition
// FLEXT_ATTRIBUTES must be 0 or 1, 
#define FLEXT_ATTRIBUTES 1

// includes
#include <flext.h>
#include <stdlib.h>
#include <ctime>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


// define the class that stands for a pd/Max object
class GApop:
	// inherit from basic flext class
	public flext_base
{
	// obligatory flext header (class name,base class name) featuring a setup function
	FLEXT_HEADER_S(GApop,flext_base,setup)
 
public:
	// constructor with a variable argument list
	GApop(int argc,const t_atom *argv);
  
  //destructor
  ~GApop();

protected:
	
	// 2 Arrays are saved here:
	// popbuf: the population itself
	// fitbuf: the fitness function
	const t_symbol *fitname, *popname;
	buffer *fitbuf, *popbuf;
	// size of the popbuf
	int buffsize;
	// the fitness order
	int *tempfit;
  float *tempfit1;

	// the other parameters:
	// pairs = number of pairs at crossover
	// mutprop = mutation probability
	// mutrange = mutation range
	int pairs;
	float mutprop, mutrange;


	// set new buffer for the population
	void m_set(int argc,const t_atom *argv); 

	// get population buffer name
	void mg_pop(AtomList &lst) const;
	// set population buffer name
	inline void ms_pop(const AtomList &lst) { m_set(lst.Count(),lst.Atoms()); }

	// get fitness function buffer name
	void mg_fit(AtomList &lst) const;
	// set fitness function buffer name
	inline void ms_fit(const AtomList &lst);


	// make the fitscaling, crossover and mutation
	void m_cross();

	// takes the incomig ints and gives out the specific individuum:
	// 0 ... fittest individuum
	// 1 ... next individuum
	// ...
	void m_trigger(int i);

	// cuts a number, if it's greater than 1 or smaller than 0
	float cutse(float nu);

	// get number of pairs for crossover
	void mg_pairs(int &p) { p = pairs; }
	// set number of pairs for crossover
	void ms_pairs(int &p) { pairs = p; }

	// get mutation porpability
	void mg_mutprop(float &mp) { mp = mutprop; }
	// set mutation porpability
	void ms_mutprop(float &mp) { mutprop = cutse(mp); }
	
	// get mutation range
	void mg_mutrange(float &mp) { mp = mutrange; }
	// set mutation range
	void ms_mutrange(float &mp) { mutrange = cutse(mp); }


	// update the array (set the actual frame length)
	inline void ms_frames() { if(Checkpopbuf()) popbuf->Frames(buffsize); }
	
	// check and eventually update fit buffer reference (return true if valid)		
	bool Checkfitbuf();
	// check and eventually update pop buffer reference (return true if valid)		
	bool Checkpopbuf();
	
	// gives out a random float in the given boundaries
	float ZZ(float b1, float b2);

	// override default flext help function	
	virtual void m_help();


private:
	static void setup(t_classid c);

	FLEXT_CALLBACK_V(m_set)  // wrapper for method m_set (with variable argument list)
	
	FLEXT_CALLVAR_V(mg_pop,ms_pop) // wrappers for attribute getter/setter (with variable argument list)	
	FLEXT_CALLVAR_V(mg_fit,ms_fit) // wrappers for attribute getter/setter (with variable argument list)	

	// callback for method "m_cross" (with no argument):
	FLEXT_CALLBACK(m_cross)	
	
	// callback for method "m_trigger" (with one int argument):
	FLEXT_CALLBACK_I(m_trigger)

	// the variables:
	FLEXT_CALLVAR_I(mg_pairs,ms_pairs)
	FLEXT_CALLVAR_F(mg_mutprop,ms_mutprop)	
	FLEXT_CALLVAR_F(mg_mutrange,ms_mutrange)

	FLEXT_CALLBACK(ms_frames) // callback for attribute setter ms_frames
};

// instantiate the class
FLEXT_NEW_V("GApop",GApop)


// setup function of the GApop
void GApop::setup(t_classid c)
{
	// register methods and attributes
	
	FLEXT_CADDMETHOD_(c,0,"set",m_set);  // register method "set" for inlet 0
	FLEXT_CADDMETHOD_(c,0,"cross",m_cross);  // register method "cross" for inlet 0
	// register a method to the default inlet (0)
	FLEXT_CADDMETHOD(c,0,m_trigger);

	FLEXT_CADDATTR_VAR(c,"popbuf",mg_pop,ms_pop);  // register attribute "popbuf" 
	FLEXT_CADDATTR_VAR(c,"fitbuf",mg_fit,ms_fit);  // register attribute "fitbuf" 

	FLEXT_CADDATTR_VAR(c,"pairs",mg_pairs,ms_pairs);  // register attribute for pairs
	FLEXT_CADDATTR_VAR(c,"mutprop",mg_mutprop,ms_mutprop);  // register attribute for mutprop
	FLEXT_CADDATTR_VAR(c,"mutrange",mg_mutrange,ms_mutrange);  // register attribute for mutrange 

	FLEXT_CADDMETHOD_(c,0,"update",ms_frames); // register method "update" for inlet 0

	// write to the console:
	post("\nGApop - by Georg Holzmann <grh@gmx.at>, 2004");
	post("(send me a help - message !!!)");
}


void GApop::m_help()
{
	// post a help message
	// thisName() returns a char * for the object name
	post("\nGApop, Vers.0.0.1 - a genetic algorithm object");
	post("compiled with flext on %s",__DATE__);
	post("1 - set all parameters:");
	post("popbuf    contains the population (array with numbers");
	post("          between 0 and 1)");
	post("fitbuf    contains the fitness function (numbers between");
	post("          0 and 1, size should be 101: 0 = fitness(0),");
	post("          1 = fitness(0.01), ..., 100 = fitness(1) )");
	post("pairs     number of pairs for the crossover");
	post("mutprop   mutation probability (between 0 and 1)");
	post("mutrange  mutation range in percent (between 0 and 1)");
	post("2 - get the data:");
	post("cross     makes fitscaling, crossover and mutation");
	post("numbers in inlet 0 get the values: 0 means the value");
	post("          of the fittest, 1 the value of the next...");
	post("have fun - Georg Holzmann <grh@mur.at>\n");
}


// constructor of GApop
GApop::GApop(int argc,const t_atom *argv)
{ 
	// reset random numbers
	srand(static_cast<int>(time(NULL)));
	
	// set the variables
	fitbuf=NULL; fitname=NULL;
	popbuf=NULL; popname=NULL;
  tempfit=NULL;
  tempfit1=NULL;
	buffsize=0;
	pairs = 0;
	mutprop = 0; mutrange = 0;


	// define inlets:
	// first inlet must always be of type anything (or signal for dsp objects)
	AddInAnything("message inlet");  // add one inlet for any message
	
	// peek outlet
	AddOutFloat("parameter outlet");
	
	// set buffer according to creation arguments
	if(argc == 1 && IsSymbol(argv[0]))
	 m_set(argc,argv);
}

GApop::~GApop()
{
  if(popbuf) delete popbuf;
  if(fitbuf) delete fitbuf;
  if(tempfit) delete[] tempfit;
  if(tempfit1) delete[] tempfit1;
  
}


// gives out a random float in the given boundaries
float GApop::ZZ(float b1, float b2)
{
	const int faktor = 10000;

	int min, max;
	
	if(b1<b2)
	{
		min = int(b1*faktor);
		max = int(b2*faktor);
	} else
	{
		max = int(b1*faktor);
		min = int(b2*faktor);
	}	

	return (float(min + rand()%(max-min+1))/faktor);
}


// check and eventually update pop buffer reference (return true if valid)		
bool GApop::Checkpopbuf()
{
	if(!popbuf || !popbuf->Valid()) {
		post("%s (%s) - no valid population buffer defined",thisName(),GetString(thisTag()));
		// return zero length
		return false;
	} 
	else {
		if(popbuf->Update()) {
			// buffer parameters have been updated
			if(popbuf->Valid()) {
				post("%s (%s) - updated population buffer reference",thisName(),GetString(thisTag()));
				return true;
			}
			else {
				post("%s (%s) - population buffer has become invalid",thisName(),GetString(thisTag()));
				return false;
			}
		}
		else
			return true;		
	}
}


// cuts a number, if it's greater than 1 or smaller than 0
float GApop::cutse(float nu)
{
	if(nu>1) { return 1;}
	else
	{
		if(nu<0) { return 0;}
	else
	{
		return nu;
	}
	}
}


// and now the same for the fitness buffer		
bool GApop::Checkfitbuf()
{
	if(!fitbuf || !fitbuf->Valid()) {
		post("%s (%s) - no valid fitness buffer defined",thisName(),GetString(thisTag()));
		// return zero length
		return false;
	} 
	else {
		if(fitbuf->Update()) {
			// buffer parameters have been updated
			if(fitbuf->Valid()) {
				post("%s (%s) - updated fitness buffer reference",thisName(),GetString(thisTag()));
				return true;
			}
			else {
				post("%s (%s) - fitness buffer has become invalid",thisName(),GetString(thisTag()));
				return false;
			}
		}
		else
			return true;		
	}
}


// set new buffer for the population
void GApop::m_set(int argc,const t_atom *argv)
{
		if(argc == 1 && IsSymbol(argv[0])) {
		// one symbol given as argument
		
		// clear existing buffer
      if(popbuf) delete popbuf;
      if(tempfit) delete[] tempfit;
      if(tempfit1) delete[] tempfit1;
    
		// save buffer name
		popname = GetSymbol(argv[0]);
		// make new reference to system buffer object
		popbuf = new buffer(popname);
		buffsize = popbuf->Frames();
    
    // make new tempfit buffers
    tempfit = new int[buffsize];
    tempfit1 = new float[buffsize];

		if(!popbuf->Ok()) {
			post("%s (%s) - warning: population buffer is currently not valid!",thisName(),GetString(thisTag()));
		}
	}
	else {
		// invalid argument list, leave buffer as is but issue error message to console
		post("%s (%s) - message argument of popbuf must be a symbol",thisName(),GetString(thisTag()));
	}
}


// get population buffer name
void GApop::mg_pop(AtomList &lst) const
{
	if(popbuf) {
		// buffer exists: return buffer name
		lst(1); SetSymbol(lst[0],popname);
	}
	else 
		// no buffer: set empty list
		lst(0);
}


// get fitness function buffer name
void GApop::mg_fit(AtomList &lst) const
{
	if(fitbuf) {
		// buffer exists: return buffer name
		lst(1); SetSymbol(lst[0],fitname);
	}
	else 
		// no buffer: set empty list
		lst(0);
}

// set fitness function buffer name
void GApop::ms_fit(const AtomList &lst)
{
		if(lst.Count() == 1 && IsSymbol(*lst.Atoms())) {
		// one symbol given as argument
		
		// clear existing buffer
		delete fitbuf;
		// save buffer name
		fitname = GetSymbol(lst[0]);
		// make new reference to system buffer object
		fitbuf = new buffer(fitname);
		
		if(!fitbuf->Ok()) {
			post("%s (%s) - warning: fitness buffer is currently not valid!",thisName(),GetString(thisTag()));
		}
	}
	else {
		// invalid argument list, leave buffer as is but issue error message to console
		post("%s (%s) - message argument of fitbuf must be a symbol",thisName(),GetString(thisTag()));
	}
}


// make the fitscaling, crossover and mutation
void GApop::m_cross()
{
  if(!Checkpopbuf() || !Checkfitbuf())
    return;
  
  if(pairs>=(buffsize/2))
  {
    post("GApop - pairs must be smaller then (buffsize/2)-1!");
    post("  currently %d pairs with a buffsize of %d!", pairs, buffsize);
    return; 
  }
	

  // 1. step:
	// every parameter get's a fitness from the
	// given fitness function
	// this fitness is saved into the temporary array tempfit1[]

	// write the fitness
	for(int i=0; i < buffsize; i++)
		tempfit1[i] = cutse(fitbuf->Data()[int(popbuf->Data()[i]*100+0.5)]);


	// 2. step:
	// now the fitness order of the parameters are written
	// into the array tempfit[]
	
	for(int j=0; j < buffsize; j++)
	{
		int fitti=0;
		float fittw=0;
		
		// get max and set it to 0
		for(int k = 0; k < buffsize; k++)
		{
			if(fittw<tempfit1[k])
			{
				fittw = tempfit1[k];
				fitti = k;
			}
		}
		
		// write the order to the tempfit array
		tempfit[j] = fitti;
		tempfit1[fitti] = 0;
	}


	// 3. step:
	// the crossover: every pair generates 2 children and replace the
	// individuums with the lowest fitness
	// the number of pairs are given (int pairs)
	
	for(int ii=pairs; ii>0; ii--)
	{
		// the first children
		popbuf->Data()[tempfit[buffsize-ii*2+1]] = 
			cutse(ZZ(popbuf->Data()[tempfit[ii*2-1]],popbuf->Data()[tempfit[ii*2-2]]));
		// the second children
		popbuf->Data()[tempfit[buffsize-ii*2]] = 
			cutse(ZZ(popbuf->Data()[tempfit[ii*2-1]],popbuf->Data()[tempfit[ii*2-2]]));
	}


	// 4. step:
	// the last step is the mutation:
	// made with the parameter mutation probability (float mutprop)
	// and mutation range (float mutrange)

	for(int jj=0; jj<buffsize; jj++)
	{
	
		if(rand()%101 < int(mutprop*100))
		{
			popbuf->Data()[jj] = cutse(popbuf->Data()[jj] + (ZZ(0,2*mutrange)-mutrange));
		}
	}
}


// takes the incomig ints and gives out the specific individuum:
// 0 ... fittest individuum
// 1 ... next fit individuum
// ...
void GApop::m_trigger(int i)
{
	// if buffer is invalid bail out
	if(!Checkpopbuf()) return;
	
	// make the boundaries for i:
	if(i<0) i=0;
	if(i>buffsize) i=buffsize;
	
	// correct syntax, output value
	ToOutFloat(0,popbuf->Data()[tempfit[i]]);
}
