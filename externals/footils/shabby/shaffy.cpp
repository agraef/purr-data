#include <flext.h>
#include <math.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 202)
#error You need at least flext version 0.2.2 
#endif
static const int pc = 9;       // only 9 inlets for max compatibility

class shaffy:
	public flext_dsp
{
	//    header(class, parent class)
	FLEXT_HEADER(shaffy, flext_dsp)

	public:
		shaffy() 
		{
			AddInSignal(1);         // audio ins
			AddInFloat(pc);         // float ins
			AddOutSignal();         // audio out
			SetupInOut();           // set up inlets and outlets

			FLEXT_ADDMETHOD(1,setF1);
			FLEXT_ADDMETHOD(2,setF2);
			FLEXT_ADDMETHOD(3,setF3);
			FLEXT_ADDMETHOD(4,setF4);
			FLEXT_ADDMETHOD(5,setF5);
			FLEXT_ADDMETHOD(6,setF6);
			FLEXT_ADDMETHOD(7,setF7);
			FLEXT_ADDMETHOD(8,setF8);
			FLEXT_ADDMETHOD(9,setF9);
			
		}
		
	
	protected:
		virtual void m_signal(int n, float *const *in, float *const *out);
	private:	
		FLEXT_CALLBACK_F(setF1)
		FLEXT_CALLBACK_F(setF2)
		FLEXT_CALLBACK_F(setF3)
		FLEXT_CALLBACK_F(setF4)
		FLEXT_CALLBACK_F(setF5)
		FLEXT_CALLBACK_F(setF6)
		FLEXT_CALLBACK_F(setF7)
		FLEXT_CALLBACK_F(setF8)
		FLEXT_CALLBACK_F(setF9)
		
		float total(const float x);
			
		float factors[pc];
		float coef[pc+1];
		void  recalcCoef();

		void setF1(float f) { factors[0] = f; recalcCoef();}
		void setF2(float f) { factors[1] = f; recalcCoef();}
		void setF3(float f) { factors[2] = f; recalcCoef();}
		void setF4(float f) { factors[3] = f; recalcCoef();}
		void setF5(float f) { factors[4] = f; recalcCoef();}
		void setF6(float f) { factors[5] = f; recalcCoef();}
		void setF7(float f) { factors[6] = f; recalcCoef();}
		void setF8(float f) { factors[7] = f; recalcCoef();}
		void setF9(float f) { factors[8] = f; recalcCoef();}
};


FLEXT_NEW_TILDE("shaffy~", shaffy)

// later...
//FLEXT_NEW_TILDE_1("shaffy~", shaffy, float)

void shaffy::recalcCoef()
{
	coef[0] = factors[0] - factors[3]*3 + factors[4]*5 - factors[6]*7 + factors[8]*9  ;
	coef[1] = factors[1]*2 - 8*factors[3]  + 18*factors[5] - 32*factors[7]  ;
	coef[2] = factors[2]*4 - 20*factors[4] + 56*factors[6]  - 120*factors[8] ;
	coef[3]=  8*factors[3]  - 48*factors[5] + 160*factors[7] ;
	coef[4] = 16*factors[4] - 112*factors[6] + 432*factors[8] ;
	coef[5] = 32*factors[5] - 256*factors[7] ;
	coef[6] = 64*factors[6] - 576*factors[8];
	coef[7] = factors[7] * 128;
	coef[8] = factors[8] * 256;
	coef[9] = factors[3] - factors[1]  - factors[5]	 + factors[7];
}


float shaffy::total(const float x)
{
	return
	x         * coef[0] +
	pow(x,2)  * coef[1] +
	pow(x,3)  * coef[2] +
	pow(x,4)  * coef[3] + 
	pow(x,5)  * coef[4] + 
	pow(x,6)  * coef[5] + 
	pow(x,7)  * coef[6] +
	pow(x,8)  * coef[7] +
	pow(x,9)  * coef[8] + coef[9]
	;
}


void shaffy::m_signal(int n, float *const *in, float *const *out)
{
	const float *ins0    =  in[0];
	float *outs          = out[0];

	while (n--)
	{
		*outs++ = shaffy::total(*ins0++);
	}
}
