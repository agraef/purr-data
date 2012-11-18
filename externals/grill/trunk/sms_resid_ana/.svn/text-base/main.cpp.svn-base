#define FLEXT_ATTRIBUTES 1

#include <flext.h>
#include <vector>
#include <cmath>

namespace {

class sms_resid_ana
    : public flext_dsp
{
    FLEXT_HEADER_S(sms_resid_ana,flext_dsp,setup)
    
public:
    sms_resid_ana()
        : frqmin(50),frqmax(15000)
        , autobang(false)
    {
        AddInSignal();
        AddOutList();
        
	    FLEXT_ADDTIMER(tmr,m_bang);
        
        ms_points(10);
    }
    
protected:

    virtual void CbSignal()
    {
        int n = Blocksize()/2;
        t_sample const *in = InSig(0);
        float nyfrq = Samplerate()/2;
        
        contour.clear();

        float lmin = log(frqmin);
        float lmax = log(frqmax);
        float lf = (lmax-lmin)/(points-1);
        float lfh = lf/2.;

        for(int i = 0; i < points; ++i) {
            float lfrq = lmin+lf*i;
            float llo = lfrq-lfh,lhi = lfrq+lfh;
            int blo = std::max((int)(exp(llo)/nyfrq*n+0.5),1);
            int bhi = std::min((int)(exp(lhi)/nyfrq*n+0.5),n);
            float s = 0;
            if(UNLIKELY(blo <= bhi)) {
                for(int b = blo; b <= bhi; ++b)
                    s += in[b];
                s /= bhi-blo+1;
            }
            contour.push_back(std::pair<float,float>(exp(lfrq),s));
        }
        
        if(autobang)
            tmr.Now();
    }

    void m_bang(void * = NULL)
    {
        AtomListStatic<32> lst(contour.size()*2);
        for(int i = 0; i < contour.size(); ++i) {
            SetFloat(lst[i*2+0],contour[i].first);
            SetFloat(lst[i*2+1],contour[i].second);
        }
        ToOutList(0,lst);
    }

    void ms_points(int p)
    {
        points = std::max(p,2);
        contour.reserve(points);
    }

    static void setup(t_classid c)
    {
        FLEXT_CADDMETHOD(c,0,m_bang);
        FLEXT_CADDATTR_VAR1(c,"fmin",frqmin);
        FLEXT_CADDATTR_VAR1(c,"fmax",frqmax);
        FLEXT_CADDATTR_VAR(c,"points",points,ms_points);
        FLEXT_CADDATTR_VAR1(c,"auto",autobang);
    }
    
private:

    Timer tmr;
    std::vector<std::pair<float,float> > contour;
    float frqmin,frqmax;
    int points;
    bool autobang;

    FLEXT_CALLBACK(m_bang)
    FLEXT_ATTRVAR_F(frqmin)
    FLEXT_ATTRVAR_F(frqmax)
    FLEXT_ATTRGET_I(points)
    FLEXT_CALLSET_I(ms_points)
    FLEXT_ATTRVAR_B(autobang)
    FLEXT_CALLBACK_T(m_bang)
};

FLEXT_NEW_DSP("sms_resid_ana~",sms_resid_ana)

} // namespace
