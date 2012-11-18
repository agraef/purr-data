#include <flext.h>
#include <math.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 200)
#error You need at least flext version 0.2.0 
#endif

class shabby:
	public flext_dsp
{
	//    header(class, parent class)
	FLEXT_HEADER(shabby, flext_dsp)

	public:
		//shabby(float polycount) // later...
		shabby() 
		{
			// pc = static_cast<int>( polycount );
			const int pc = 12;
			AddInSignal(pc);         // audio ins
			AddOutSignal();          // audio out
			SetupInOut();            // set up inlets and outlets
		}
	
	protected:
		virtual void m_signal(int n, float *const *in, float *const *out);
	private:	
		// int pc;  // how many chebychev polynoms needed?
		float  t2(const float x);
		float  t3(const float x);
		float  t4(const float x);
		float  t5(const float x);
		float  t6(const float x);
		float  t7(const float x);
		float  t8(const float x);
		float  t9(const float x);
		float t10(const float x);
		float t11(const float x);
		// float computePoly (const float x, const int order);
};


FLEXT_NEW_TILDE("shabby~", shabby)

// later...
//FLEXT_NEW_TILDE_1("shabby~", shabby, float)

//float shabby::computePoly(const float x, const int order)
//{
//	float tmp = 0.0;
//	switch (order)
//	{
//		case 0:
//			return 1;
//			break;
//		case 1:
//			return x;
//			break;
//		default:
//			tmp = 2*x*shabby::computePoly(x, order - 1) 
//			      - shabby::computePoly(x, order - 2);
//			return tmp;
//	}
//}



float shabby::t2(const float x)
{
	//return shabby::computePoly(x, 2);
	return (2*pow(x,2) - 1);
}

float shabby::t3(const float x)
{
	//return shabby::computePoly(x, 3);
	return (4*pow(x,3) - 3*x);
}

float shabby::t4(const float x)
{
	//return shabby::computePoly(x, 4);
	return (8*pow(x,4) - 8*pow(x,2) + 1);
}

float shabby::t5(const float x)
{
	//return shabby::computePoly(x, 5);
	return (16*pow(x,5) - 20*pow(x,3)  + 5*x);
}

float shabby::t6(const float x)
{
	//return shabby::computePoly(x, 6);
	return (32*pow(x,6) - 48*pow(x,4) +18*pow(x,2) - 1);
}

float shabby::t7(const float x)
{
	return (64*pow(x,7) - 112*pow(x,5) + 56*pow(x,3)  - 7*x);
}

float shabby::t8(const float x)
{
	return (128*pow(x,8) - 256*pow(x,6) + 160*pow(x,4) - 32*pow(x,2) + 1);
}

float shabby::t9(const float x)
{
	return (256*pow(x,9) - 576*pow(x,7) + 432*pow(x,5) - 120*pow(x,3)  + 9*x);
}

float shabby::t10(const float x)
{
	return (512*pow(x,10) - 1280*pow(x,8) + 1120*pow(x,6) - 400*pow(x,4) + 50*pow(x,2) - 1);
}

float shabby::t11(const float x)
{
	return (1024*pow(x,11) - 2816*pow(x,9) + 2816*pow(x,7) -1232*pow(x,5) + 220*pow(x,3)  - 11*x);
}


void shabby::m_signal(int n, float *const *in, float *const *out)
{
	const float *ins0    = in[0];
	const float *ins1    = in[1];
	const float *ins2    = in[2];
	const float *ins3    = in[3];
	const float *ins4    = in[4];
	const float *ins5    = in[5];
	const float *ins6    = in[6];
	const float *ins7    = in[7];
	const float *ins8    = in[8];
	const float *ins9    = in[9];
	const float *ins10   = in[10];
	const float *ins11   = in[11];
	
	float tout = 0.0;
	
	float *outs = out[0];

	while (n--)
	{
		float x     = (*ins0++);
		
		tout  = (*ins1++) * x;
		tout += (*ins2++) * shabby::t2(x);
		tout += (*ins3++) * shabby::t3(x);
		tout += (*ins4++) * shabby::t4(x);
		tout += (*ins5++) * shabby::t5(x);
		tout += (*ins6++) * shabby::t6(x);
		tout += (*ins7++) * shabby::t7(x);
		tout += (*ins8++) * shabby::t8(x);
		tout += (*ins9++) * shabby::t9(x);
		tout += (*ins10++) * shabby::t10(x);
		tout += (*ins11++) * shabby::t11(x);
		
		*outs++ = tout;
	}
}
