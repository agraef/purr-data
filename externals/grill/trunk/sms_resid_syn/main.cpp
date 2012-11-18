#define FLEXT_ATTRIBUTES 1

#include <flext.h>
#include <vector>
#include <cmath>

namespace {

class sms_resid_syn
    : public flext_dsp
{
    FLEXT_HEADER_S(sms_resid_syn,flext_dsp,setup)
    
public:
    sms_resid_syn()
        : frqmin(50),frqmax(15000)
    {
        AddInAnything();
        AddOutSignal();
    }
    
protected:

    virtual void CbSignal()
    {
        int const n = Blocksize()/2;
        t_sample *out = OutSig(0);
        float const nyfrq = Samplerate()/2;

        int csz = contour.size();
        if(UNLIKELY(!csz))
            ZeroSamples(out,n+1);
        else {
            out[0] = 0;
            int cix = -1;
            float const bhop = nyfrq/n;
            for(int b = 1; b <= n; ++b) {
                float frq = b*bhop;
                while(cix < csz-1 && contour[cix+1].first < frq) 
                    ++cix;
                    
                if(UNLIKELY(cix < 0))
                    out[b] = contour[0].second;
                else if(UNLIKELY(cix >= csz-1))
                    out[b] = contour[csz-1].second;
                else {
                    // linear interpolation in log domain
                    float lmin = log2(contour[cix].first);
                    float lmax = log2(contour[cix+1].first);
                    float lfrq = log2(frq);
                    float x1 = contour[cix].second;
                    float x2 = contour[cix+1].second;
                    float f = (lfrq-lmin)/(lmax-lmin);
                    out[b] = x1+f*(x2-x1);
                }
            }
        }
        
        // clear remainder of the signal vector
        ZeroSamples(out+n+1,n-1);
   }

    void m_list(int argc,t_atom const *argv)
    {
        int p = argc/2;
        contour.reserve(p);
        contour.clear();
        for(int i = 0; i < p; ++i) {
            float f = GetAFloat(argv[i*2+0]);
            // check for monotonic increase
            if(!contour.size() || contour[contour.size()-1].first < f) {
                float g = GetAFloat(argv[i*2+1]);
                contour.push_back(std::pair<float,float>(f,g));
            }
        }
    }

    void ms_contour(AtomList const &lst) { m_list(lst.Count(),lst.Atoms()); }

    void mg_contour(AtomList &lst) 
    { 
        lst(contour.size()*2);
        for(int i = 0; i < contour.size(); ++i) {
            SetFloat(lst[i*2+0],contour[i].first);
            SetFloat(lst[i*2+1],contour[i].second);
        }
    }

    static void setup(t_classid c)
    {
        FLEXT_CADDMETHOD(c,0,m_list);
        FLEXT_CADDATTR_VAR1(c,"fmin",frqmin);
        FLEXT_CADDATTR_VAR1(c,"fmax",frqmax);
        FLEXT_CADDATTR_VAR(c,"contour",mg_contour,ms_contour);
    }
    
private:

    std::vector<std::pair<float,float> > contour;
    float frqmin,frqmax;

    FLEXT_CALLBACK_V(m_list)
    FLEXT_ATTRVAR_F(frqmin)
    FLEXT_ATTRVAR_F(frqmax)
    FLEXT_CALLVAR_V(mg_contour,ms_contour)
};

FLEXT_NEW_DSP("sms_resid_syn~",sms_resid_syn)

} // namespace
