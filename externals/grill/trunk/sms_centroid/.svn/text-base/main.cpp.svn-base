#define FLEXT_ATTRIBUTES 1

#include <flext.h>
#include <vector>
#include <cmath>

class sms_centroid
    : public flext_dsp
{
    FLEXT_HEADER_S(sms_centroid,flext_dsp,setup)
    
public:
    sms_centroid()
        : centroid(0)
        , autobang(false)
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
        float df = Samplerate()/2/n;
        
        float a = 0,b = 0;
        for(int i = 1; i <= n; ++i) {
            float f = log(i*df);
            a += f*in[i];
            b += in[i];
        }
        
        centroid = exp(a/b);
        
        if(autobang)
            tmr.Now();
    }

    void m_bang(void * = NULL)
    {
        ToOutFloat(0,centroid);
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
    float centroid;
    bool autobang;
//    float frqmin,frqmax;

    FLEXT_CALLBACK(m_bang)
//    FLEXT_ATTRVAR_F(frqmin)
//    FLEXT_ATTRVAR_F(frqmax)
    FLEXT_CALLBACK_T(m_bang)
    FLEXT_ATTRVAR_B(autobang)
};

FLEXT_NEW_DSP("sms_centroid~",sms_centroid)
