/*##############################################################################
#                                                                              #
#   This file is part of PSO (Particle Swarm Optimizer)                        #
#   Copyright Ben Bogart 2003, based on code from Thomas Grill & Jim McKenzie  #
#                                                                              #
#   This program is free software; you can redistribute it and/or modify       #
#   it under the terms of the GNU General Public License as published by       #
#   the Free Software Foundation; either version 2 of the License, or          #
#   (at your option) any later version.                                        #
#                                                                              #
#   This program is distributed in the hope that it will be useful,            #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of             #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              #
#   GNU General Public License for more details.                               #
#                                                                              #
#   You should have received a copy of the GNU General Public License          #
#   along with this program; if not, write to the Free Software                #
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  #
#                                                                              #
##############################################################################*/

// include flext header
#include <flext.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif

class pso:
	// inherit from basic flext class
	public flext_base
{
	// obligatory flext header (class name,base class name)
	FLEXT_HEADER(pso,flext_base)
 
public:
	// constructor 
	pso(int Popsize, int Dimen, int HoodSize);

protected:

	// override default flext help function
        virtual void m_help();

	// flext deconstructor
	~pso();

	// PSO Functions
	void init();					// Initialize the PSO 
	void copy_positions();				// Copy the current positions to previous positions
	float minimize(int particle);			// The function to Minimize (sphere)
	void set_pop(int argc,t_atom *argv);  		// Set values to the population
	void set_particle(int argc,t_atom *argv);	// Set values for a single particle
        void set_target(int argc,t_atom *argv);         // Set the PSO target
	void rand_pop(float min, float max);		// Randomize the values of the population
	void optim_thresh(float threshold);		// Optimization Threshold
	void rand_vel();				// Randomize the velocities
	void iterate(); 				// Iterate the PSO
	void reset();					// Rerun the PSO init
	void output_positions();			// Send out the lists of Particles' positions

	// PSO Init Variables
	int popsize;
	int dimen;
	int hoodsize;
	int gbest;
	float Min;
	float Max;
	float optimization_threshold;

	// PSO Arrays
        float *current_position;
        float *velocity;
        float *previous_best_position;
        int *neighbors;
        float *previous_best;
        float *target;

private:
	// PSO Callbacks to functions
	
	FLEXT_CALLBACK_V(set_pop);
	FLEXT_CALLBACK_V(set_particle);
	FLEXT_CALLBACK_V(set_target);
	FLEXT_CALLBACK_FF(rand_pop); 
	FLEXT_CALLBACK_F(optim_thresh);
	FLEXT_CALLBACK(iterate);
	FLEXT_CALLBACK(reset);
	FLEXT_CALLBACK(m_help);
};

// instantiate the class
FLEXT_NEW_3("pso",pso, int,int,int)


pso::pso(int Popsize, int Dimen, int HoodSize)
{ 
	// Say hello
	post("--------------------------------------------");
	post("Particle Swarm Optimizer (PSO)");
	post("Copyright Ben Bogart 2003.");
	post("Based on code by Thomas Grill & Jim Kennedy.");
	post("--------------------------------------------");

	// default vars
	this->Min = 0;
        this->Max = 1;
	this->optimization_threshold = 5;

        // Copy Arguments to the class variables
        this->popsize = Popsize;
        this->dimen = Dimen;
        this->hoodsize = HoodSize;

        // Create particle Arrays (single dimensional!)
        this->current_position = new float[this->popsize*this->dimen];
        this->velocity = new float[this->popsize*this->dimen];
        this->previous_best_position = new float[this->popsize*this->dimen];
        this->neighbors = new int[this->popsize*this->hoodsize];
        this->previous_best = new float[this->popsize];

        // Create target array
        this->target = new float[this->dimen];

	// define inlets:
	// first inlet must always by of type anything (or signal for dsp objects)
	AddInAnything("Send 'help' message for details...");  // add one inlet for any message
	
	// define outlets:
	AddOutList("Particle positions.");
	AddOutFloat("Current minimization.");
	AddOutBang("Bang when minimization has been reached.");
	
	// register methods
	FLEXT_ADDMETHOD_(0,"set_pop",set_pop);
	FLEXT_ADDMETHOD_(0,"set_part", set_particle);
	FLEXT_ADDMETHOD_FF(0,"rand_pop",rand_pop);
	FLEXT_ADDMETHOD_F(0,"optim_thresh", optim_thresh);
	FLEXT_ADDBANG(0,iterate);
	FLEXT_ADDMETHOD_(0,"reset",reset);
	FLEXT_ADDMETHOD_(0,"help",m_help);
	FLEXT_ADDMETHOD_(0,"target",set_target);

	// init pso
	pso::init();
} 

float pso::minimize(int particle)
{
	float result=0;
	int d;

	for (d=0; d< this->dimen; d++)
	{
	        // Pythagoras - c^2=(a^2+b^2) - for each dimension!! 
		result += pow( this->current_position[particle*this->dimen+d]-(float)this->target[d],2);
	}

	return result;
}

void pso::copy_positions()
{
	int p,d;

	// For each particle and each dimension
        for (p=0; p< this->popsize; p++)
        {
          for (d=0; d< this->dimen; d++)
          {
                this->previous_best_position[p*this->dimen+d] = this->current_position[p*this->dimen+d];
          }
        }
}

// Deconstructor
pso::~pso()
{
	// Remove Arrays from memmory space
        delete[] this->current_position;
        delete[] this->velocity;
        delete[] this->previous_best_position;
        delete[] this->neighbors;
        delete[] this->previous_best;
        delete[] this->target;
}

// The population must be set before this function is run!
void pso::init()
{
	int p,n;

	// Do stuff for random # generator
	time_t timer = time(NULL);
	srand((unsigned)timer);

	// set initial population
	pso::rand_vel();
	pso::copy_positions();

	// For each particle
	for (p=0; p< this->popsize; p++)
	{
		// Who has the best solutions?
		this->previous_best[p]=pso::minimize(p);

		// Create neighborhood topology
		for (n=0; n< this->hoodsize; n++)
		{
       			/* Connect each neighbor to its previous and next particle */
        		this->neighbors[p*this->hoodsize+n]=p-1+n;

        		/* Make a ring so the first and last particles are connected */
        		if(this->neighbors[p*this->hoodsize+n] < 0)
        		{
          		  this->neighbors[p*this->hoodsize+n]=this->popsize-this->neighbors[p*this->hoodsize+n]-2;
        		} else if(this->neighbors[p*this->hoodsize+n] >= this->popsize)
        		{
          	 	  this->neighbors[p*this->hoodsize+n]=this->neighbors[p*this->hoodsize+n]-this->popsize;
        		} 
		}
	}

	// Set initial global best solution
	this->gbest=0;
}

void pso::set_pop(int argc,t_atom *argv)
{
        int i;

        // If the number of arguments matches the number of particles*dimensions
        if (argc == this->dimen*this->popsize )
        {
                for (i=0; i< this->dimen*this->popsize; i++)
                {
                        this->current_position[i] = GetFloat(argv[i]);
                }
                post("%s: Population Set.", thisName() );
        } else {
                post("%s: There must be %d arguments for each of the %d particles.\n     You entered %d/%d arguments.", thisName(), this->dimen, this->popsize, argc, this->dimen*this->popsize);
        }

	// Do init stuff again
	pso::copy_positions();

	// Send out new values
	pso::output_positions();
}

void pso::set_particle(int argc,t_atom *argv)
{
        int i,particle;

        // If the number of arguments matches the number of dimensions plus the index value
        if ( argc == this->dimen+1 )
        {
		// Which particle are we setting?
		// Check to make sure the particle <= this->popsize
		particle = GetAInt(argv[0]);

		if (particle < this->popsize && particle >= 0)
		{

                	// Set particle
			for (i=1; i< argc; i++)
			{
				this->current_position[particle*this->dimen+(i-1)] = GetAFloat(argv[i]);
			}

                	post("%s: Particle %d Set.", thisName(), particle);

		        // Do init stuff again
     			pso::copy_positions();

        		// Send out new values
        		pso::output_positions();

		} else {
			post("%s: The particle %d is not within the population limits of 0 to %d.", thisName(), particle, this->popsize-1);
		}
       	} else {
                post("%s: There must be %d arguments for each particle.\n     You entered %d arguments.", thisName(), this->dimen, argc);
        }
}

void pso::set_target(int argc,t_atom *argv)
{
	int i;

	// If the number of arguments matches the number of dimensions
	if (argc == this->dimen )
	{
		for (i=0; i<dimen; i++)
		{
			this->target[i] = GetAFloat(argv[i]);
		}
		post("%s: Target Set.", thisName() );
	} else {
		post("%s: There must be one argument for each of the %d dimensions.\n     You entered %d arguments.", thisName(), this->dimen, argc);
	}
}

void pso::optim_thresh(float threshold)
{
	this->optimization_threshold = threshold;
	post("%s: Optimization Threshold set to %f.", thisName(), (threshold/100000000) );
}

void pso::rand_pop(float min, float max)
{
    // set the class variables to the min and max values
    this->Min = min;
    this->Max = max;

    int p,d;

    for(p = 0; p < this->popsize; p++)
    {
        for(d = 0; d < this->dimen; d++)
	{
	    // set to random numbers min-max
            this->current_position[p*this->dimen+d] = ((float)rand()/RAND_MAX)*(max-min)+min;
	}
    }

    pso::copy_positions();

    post("%s: Population Randomized.", thisName() ); 

    // Output new positions
    pso::output_positions();
}

void pso::rand_vel()
{ 
    int p,d;
    for(p = 0; p < this->popsize; p++)
    {
        for(d = 0; d < this->dimen; d++)
        {
	    // set to random numbers 0-1
            this->velocity[p*this->dimen+d] = ((float)rand()/RAND_MAX);

	    // Set a random number of the velocities to negative
	    if ( ((float)rand()/RAND_MAX) < 0.5 )
		this->velocity[p*this->dimen+d] = this->velocity[p*this->dimen+d]*-1;
        }
    }
}

void pso::iterate()
{
	int p,d,n,g,neighbor,index_p,index_g;
	float current_minima,rand1,rand2;
	
        // Create constants
        float khi=0.729;
        float HalfPhi=2.05;

	// For each particle
	for (p=0; p< this->popsize; p++)
	{
	  // Who has the best solution?
	  current_minima = pso::minimize(p);

	  // Set initial neighbor with which to compare (first neighbor)
	  // Changed this to fix segfault (from popsize to hoodsize)
	  g=neighbors[p*this->hoodsize];

	  // For each neighbor
	  for (n=0; n< this->hoodsize; n++)
	  {
	    neighbor=this->neighbors[p*this->hoodsize+n];
	    if (this->previous_best[neighbor] < this->previous_best[g])
	    {

		// If the current neighbor has a better solution then make g (the best neighbor) the current neighbor
		g=neighbor; 
	    }
	    if (current_minima < this->previous_best[p])
	    {
		if (current_minima < this->previous_best[this->gbest])
		{
		  this->gbest = p;
		}
		for (d=0; d< this->dimen; d++)
		{
		  this->previous_best_position[p*this->dimen+d] = this->current_position[p*this->dimen+d];
		}
		this->previous_best[p]=current_minima;
	    }

	    for (d=0; d< this->dimen; d++)
	    {
		rand1 = (float)rand()/RAND_MAX;
		rand2 = (float)rand()/RAND_MAX;

		index_p = p*this->dimen+d;
		index_g = g*this->dimen+d;

		if ( (index_p < dimen*popsize && index_p >= 0) && (index_g < dimen*popsize && index_g >= 0) )
		{
			this->velocity[index_p]=khi*(this->velocity[index_p] + rand1*HalfPhi*(this->previous_best_position[index_p] - this->current_position[index_p]) + rand2*HalfPhi*(this->previous_best_position[index_g] - this->current_position[index_p]));
            		this->current_position[index_p]=this->current_position[index_p]+this->velocity[index_p];
		}
	    }
	  }
	}

	// Have we minimized yet?
	// For Small systems (Nell) use 0.0000005
	// This constant 0.0000005 is user specified add a method for it? Will be send enough digits of percision?
	if (this->previous_best[this->gbest] <= (this->optimization_threshold/10000000))
	{
		// Send bang out the last outlet (numOutlets-1)
		ToOutBang( CntOut()-1 );
	}

	// Output current minimization
	ToOutFloat(1, this->previous_best[this->gbest]);

	// Output the current position of the particles
	pso::output_positions();
}

void pso::output_positions()
{
	int d,p;
	t_atom atom;
	t_atom p_atom;
	AtomList list;

	// Construct a list of particle positions for each dimension
	for (p=0; p< this->popsize; p++)
	{
		// Clear the list first
		list.Clear();
	
		// start the lists with the index of the particle
		SetInt(p_atom, p );
		list.Append(p_atom);

		for (d=0; d< this->dimen; d++)
		{
			SetFloat(atom, this->current_position[p*this->dimen+d]);
			list.Append(atom);
		}

		// Send out list
		ToOutList(0, list);
	}
}

// Population must be randomized or set FIRST!
void pso::reset()
{
	// Reinitialize the PSO
	pso::init();
	post("%s: Reset.", thisName());
}

void pso::m_help()
{
	post("%s Help:", thisName());
	post("Arguments: [Population Size] [Number of Dimensions] [Neighborhood Size]");
	post("");

	post("PSO Help Method:");
	post("  'help'\t\t\t- This message.");
	post("");

	post("PSO Setup Methods:");
	post("  'target [list]'\t\t- Specify the optimization target with list of values for each dimension.");
	post("  'rand_pop [min] [max]'\t- Randomize the population with values ranging from min to max.");
	post("  'set_pop [list]'\t\t- Set population with list of values for each particle for each dimension.");
	post("  'set_part [particle] [list]'\t- Set particle with values for each dimension.");
	post("  'optim_thresh [int]'\t\t- Set the threshold at which minimization is considered reached. (0-10)");
	post("");

	post("PSO Run-Time Methods:");
	post("  'reset'\t\t\t- Reset the PSO.");
	post("  bang\t\t\t\t- Calculate one interation of the PSO.");
}
