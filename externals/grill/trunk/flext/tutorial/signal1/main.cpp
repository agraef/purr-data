// signal1~ - a flext tutorial external written by Frank Barknecht
// 
// This is a commented port of the pan~ example from the PD-Externals-Howto to
// illustrate the usage of flext. You can get the original code at
// http://iem.kug.ac.at/pd/externals-HOWTO/

#include <flext.h>
 
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1
#endif


// A flext dsp external ("tilde object") inherits from the class flext_dsp 
class signal1: 
	public flext_dsp
{
	// Each external that is written in C++ needs to use #defines 
	// from flbase.h
	// 
	// The define
	// 
	// FLEXT_HEADER(NEW_CLASS, PARENT_CLASS)
	// 
	// should be somewhere in your dsp file.
	// A good place is here:
	
	FLEXT_HEADER(signal1, flext_dsp)

	public:
		signal1():
			f_pan(0) // initialize f_pan
		{
			// The constructor of your class is responsible for
			// setting up inlets and outlets and for registering
			// inlet-methods:
			// The descriptions of the inlets and outlets are output
			// via the Max/MSP assist method (when mousing over them in edit mode).
			// PD will hopefully provide such a feature as well soon

			AddInSignal("left audio in");       // left audio in
			AddInSignal("right audio in");      // right audio in
			AddInFloat("panning parameter");    // 1 float in
			AddOutSignal("audio out");          // 1 audio out 
			
			// Now we need to bind the handler function to our
			// panning inlet, which is inlet 2 (counting all inlets
			// from 0).  We want the function "setPan" to get
			// called on incoming float messages:

			FLEXT_ADDMETHOD(2,setPan);
			
			// We're done constructing:
			post("-- pan~ with flext ---");
			
		} // end of constructor
		
	
	protected:
		// here we declare the virtual DSP function
		virtual void m_signal(int n, float *const *in, float *const *out);
	private:	
		float f_pan;  // holds our panning factor
		
		// Before we can use "setPan" as a handler, we must register this
		// function as a callback to PD or Max. This is done using the
		// FLEXT_CALLBACK* macros. There are several of them.
		//
		// FLEXT_CALLBACK_F is a shortcut, that registers a function
		// expecting one float arg (thus ending in "_F"). There are
		// other shortcuts that register other types of functions. Look
		// into flext.h. No semicolon at the end of line!!!
		FLEXT_CALLBACK_F(setPan)

		// Now setPan can get declared and defined here.
		void setPan(float f) 
		{ 
			// set our private panning factor "f_pan" to the inlet
			// value float "f" in the intervall [0,1]
			f_pan = (f<0) ? 0.0f : (f>1) ? 1.0f : f ;	
			
			// if you want to debug if this worked, comment out the
			// following line: 
			//post("Set panning to %.2f, maybe clipped from %.2f", f_pan,f);
		} // end setPan
}; // end of class declaration for signal1


// Before we can run our signal1-class in PD, the object has to be registered as a
// PD object. Otherwise it would be a simple C++-class, and what good would
// that be for?  Registering is made easy with the FLEXT_NEW_* macros defined
// in flext.h. For tilde objects without arguments call:

FLEXT_NEW_DSP("signal1~ pan~", signal1)
// T.Grill: there are two names for the object: signal1~ as main name and pan~ as its alias

// Now we define our DSP function. It gets this arguments:
// 
// int n: length of signal vector. Loop over this for your signal processing.
// float *const *in, float *const *out: 
//          These are arrays of the signals in the objects signal inlets rsp.
//          oulets. We come to that later inside the function.

void signal1::m_signal(int n, float *const *in, float *const *out)
{
	
	const float *ins1    =  in[0];
	const float *ins2    =  in[1];
	// As said above "in" holds a list of the signal vectors in all inlets.
	// After these two lines, ins1 holds the signal vector ofthe first
	// inlet, index 0, and ins2 holds the signal vector of the second
	// inlet, with index 1.
	
	float *outs          = out[0];
	// Now outs holds the signal vector at the one signal outlet we have.
	
	// We are now ready for the main signal loop
	while (n--)
	{
		
		// The "++" after the pointers outs, ins1 and ins2 walks us
		// through the signal vector with each n, of course. Before
		// each step we change the signal value in the outlet *outs
		// according to our panning factor "f_pan" and according to the
		// signals at the two signal inlets, *ins1 and *ins2 
		
		*outs++  = (*ins1++) * (1-f_pan) + (*ins2++) * f_pan;
	}
}  // end m_signal
