#define FLEXT_ATTRIBUTES 1

#include <flext.h>
#include <vector>
#include <cmath>

class sms_power
    : public flext_dsp
{
    FLEXT_HEADER_S(sms_power,flext_dsp,setup)
    
public:
    sms_power()
        : autobang(false)
    {
        AddInSignal();
        AddOutFloat();
        
	    FLEXT_ADDTIMER(tmr,m_bang);
    }
    
protected:

    virtual void CbSignal()
    {
        int n = Blocksize()/2;
        t_sample const *in = InSig(0);
        
        power = 0;
        for(int i = 1; i <= n; ++i)
            power += in[i];
        
        if(autobang)
            tmr.Now();
    }

    void m_bang(void * = NULL)
    {
        ToOutFloat(0,power);
    }

    static void setup(t_classid c)
    {
        FLEXT_CADDMETHOD(c,0,m_bang);
//        FLEXT_CADDATTR_VAR1(c,"fmin",frqmin);
//        FLEXT_CADDATTR_VAR1(c,"fmax",frqmax);
        FLEXT_CADDATTR_VAR1(c,"auto",autobang);
    }
    
private:

    Timer tmr;
    float power;
//    float frqmin,frqmax;
    bool autobang;

    FLEXT_CALLBACK(m_bang)
//    FLEXT_ATTRVAR_F(frqmin)
//    FLEXT_ATTRVAR_F(frqmax)
    FLEXT_ATTRVAR_B(autobang)
    FLEXT_CALLBACK_T(m_bang)
};

FLEXT_NEW_DSP("sms_power~",sms_power)
