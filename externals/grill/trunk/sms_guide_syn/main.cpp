#define FLEXT_ATTRIBUTES 1
#define _USE_MATH_DEFINES

#include <flext.h>
#include <map>
#include <vector>
#include <cmath>
#include <cstdlib>

namespace {

#define ARRAYSIZE 512

template<typename T,typename A>
static inline T interpolate(A f,T frac)
{
    FLEXT_ASSERT(frac >= 0 && frac <= 1);

    const T f1 = frac*0.5f-0.5f;
    const T f3 = frac*3.0f-1.0f;
    
    const T amdf = (f[0]-f[3])*frac;
    const T cmb = f[2]-f[1];
    const T bma = f[1]-f[0];
    return f[1] + frac*( cmb - f1 * ( amdf+bma+cmb*f3 ) );
}

class sms_guide_syn
    : public flext_dsp
{
    FLEXT_HEADER_S(sms_guide_syn,flext_dsp,setup)
    
public:
    sms_guide_syn(const t_symbol *bufname = NULL)
        : deftime(0)
    {
        AddInSignal();
        AddOutSignal();
        
        rmv.reserve(100);
        
        if(bufname) {
            t_atom at; 
            SetSymbol(at,bufname);
            ms_buffer(1,&at);
        }
    }
    
protected:

    virtual void CbSignal()
    {
        int const n = Blocksize();
        
        if(buf.Symbol()) {
            bool ok = buf.Valid();
            if(ok) {
                buf.Update();
                ok = buf.Ok();
            }

            guides.clear();

            if(ok) {
                t_sample const *dt = buf.Data();
                int const len = buf.Frames();
                int time = std::max(int(deftime*Samplerate()+0.5),0);
                
                for(int i = 0; i < len; ++i,dt += 3) {
                    int index = dt[0];
                    if(index < 0) break;
                    float frq = dt[1];
                    float gain = dt[2];
                    addpartial(index,frq,gain,time);
                }
            }
        }
        
        t_sample *out = OutSig(0);
        ZeroSamples(out,n);
    
        const float fr = ARRAYSIZE/Samplerate();
    
        for(GuideMap::iterator it = guides.begin(); it != guides.end(); ++it) {
            Guide &gd = it->second;
            
            float f = gd.cfrq,g = gd.cgain;
            float ph = gd.phase,dph;
            int i = 0;
            
            if(UNLIKELY(gd.togo >= 0)) {
                if(gd.togo) {
                    // currently changing
                    int tg = std::min(gd.togo,n);
                    float df = (gd.tfrq-f)/gd.togo;
                    float dg = (gd.tgain-g)/gd.togo;
                    
                    if(!df) {
                        // optimized for constant frequency
                        
                        if(f < 0) f = Samplerate()-f;
                        
                        dph = f*fr;
                        if(UNLIKELY(dph >= ARRAYSIZE)) 
                            dph = fmod(dph,float(ARRAYSIZE));
                        
                        int pi = int(ph),dpi = int(dph);
                        float fr = ph-pi,dfr = dph-dpi;
                        
                        for(; i < tg; ++i,g += dg) {
                            FLEXT_ASSERT(pi < ARRAYSIZE);
                        
                            out[i] += interpolate(costable+pi,fr)*g;
                            fr += dfr;
                            if(UNLIKELY(fr >= 1))
                                fr -= 1,pi += 1;
                            pi += dpi;
                            if(UNLIKELY(pi >= ARRAYSIZE)) 
                                pi -= ARRAYSIZE;
                        }

                        ph = pi+fr;
                    }
                    else
                        // general case (varying gain and frequency)
                        for(; i < tg; ++i) {
                            int pi = int(ph);
                            out[i] += interpolate(costable+pi,ph-pi)*g;
                            ph += f*fr;
                            if(UNLIKELY(ph >= ARRAYSIZE)) 
                                ph = fmod(ph,float(ARRAYSIZE));
                            f += df,g += dg;
                        }

                    gd.togo -= tg;
                }
                
                if(!gd.togo) {
                    // change has finished -> normalize values
                    f = gd.cfrq = gd.tfrq;
                    g = gd.cgain = gd.tgain;
                    gd.togo = -1;
                    
                    if(UNLIKELY(!g)) 
                        // gain == 0... mark guide for removal
                        rmv.push_back(it->first);
                }
                else
                    // store current values
                    gd.cfrq = f,gd.cgain = g;
            }
            else {
                // frq and gain constant
                FLEXT_ASSERT(gd.cfrq == gd.tfrq);
                FLEXT_ASSERT(gd.cgain == gd.tgain);
            }

            if(g) {
                //////////////////////////////////////////
                // optimized sampling for constant f and g
                //////////////////////////////////////////
                
                // special case for negative frequencies (so that dph stays >= 0)
                if(f < 0) f = Samplerate()-f;
                
                dph = f*fr;
                if(UNLIKELY(dph >= ARRAYSIZE)) 
                    dph = fmod(dph,float(ARRAYSIZE));
                
                int pi = int(ph),dpi = int(dph);
                float fr = ph-pi,dfr = dph-dpi;
                
                for(; i < n; ++i) {
                    FLEXT_ASSERT(pi < ARRAYSIZE);
                
                    out[i] += interpolate(costable+pi,fr)*g;
                    fr += dfr;
                    if(UNLIKELY(fr >= 1))
                        fr -= 1,pi += 1;
                    pi += dpi;
                    if(UNLIKELY(pi >= ARRAYSIZE)) 
                        pi -= ARRAYSIZE;
                }

                gd.phase = pi+fr;
            }
            else
                gd.phase = fmod(ph+f*fr*n,float(ARRAYSIZE));
        }
        
        // remove finished guides
        while(rmv.size()) {
            int ix = rmv.back();
            rmv.pop_back();
            guides.erase(ix);
        }
    }

    void addpartial(int index,float frq,float gain,int time)
    {
        GuideMap::iterator it = guides.find(index);
        if(it == guides.end()) 
            // new guide
            guides[index] = Guide(frq,gain,time);
        else {
            // update guide
            Guide &g = it->second;
            g.tfrq = frq; g.tgain = gain; g.togo = time;
        }
    }

    void ms_buffer(int argc,const t_atom *argv)
    {
        t_symbol const *s = argc && IsSymbol(argv[0])?GetSymbol(argv[0]):NULL;
        buf.Set(s?s:sym__);
    }

    void ms_buffer(AtomList const &lst) { ms_buffer(lst.Count(),lst.Atoms()); }

    void mg_buffer(AtomList &lst)
    {
        if(buf.Ok()) {
            lst(1);
            SetSymbol(lst[0],buf.Symbol());
        }
    }

    void m_reset()
    {
        guides.clear();
    }

    void m_partial(int argc,t_atom const *argv)
    {
        if(argc >= 2 && CanbeInt(argv[0]) && CanbeInt(argv[1])) {
            int index = GetAInt(argv[0]);
            float frq = GetAFloat(argv[1]);
            float gain = argc > 2?GetAFloat(argv[2]):0;
            int time = std::max(int((argc > 3?GetAFloat(argv[3]):deftime)*Samplerate()+0.5),0);
            addpartial(index,frq,gain,time);
        }
    }

    void mg_partial(int ix) const
    {
        GuideMap::const_iterator it = guides.find(ix);
        if(it != guides.end()) {
            const Guide &gd = it->second;
            t_atom lst[6];
            SetInt(lst[0],ix);
            SetFloat(lst[1],gd.tfrq);
            SetFloat(lst[2],gd.tgain);
            SetFloat(lst[3],gd.cfrq);
            SetFloat(lst[4],gd.cgain);
            SetInt(lst[5],gd.togo);
            ToOutAnything(GetOutAttr(),sym_partial,6,lst);
        }
        else {
            // not found
            t_atom at; SetInt(at,ix);
            ToOutAnything(GetOutAttr(),sym_partial,1,&at);
        }
    }

    void mg_partials(int &p) const { p = guides.size(); }

    static void setup(t_classid c)
    {
        sym_partial = MakeSymbol("partial");
    
        FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
        FLEXT_CADDMETHOD(c,0,m_partial); // list method
        FLEXT_CADDMETHOD_(c,0,sym_partial,m_partial);

        FLEXT_CADDMETHOD_I(c,0,"getpartial",mg_partial);
        FLEXT_CADDATTR_GET(c,"partials",mg_partials);
        FLEXT_CADDATTR_VAR1(c,"time",deftime);
        
        for(int i = 0; i < ARRAYSIZE+3; ++i)
            costable[i] = cos(i*M_PI*2/ARRAYSIZE);
    }
    
private:

    static t_sample costable[ARRAYSIZE+3];
    static t_symbol const *sym_partial;

    struct Guide
    {
        Guide(float f = 0,float g = 0,int ts = 0)
            : cfrq(f),tfrq(f)
            , cgain(0),tgain(g)
            , togo(ts)
            , phase((float)rand()/RAND_MAX*ARRAYSIZE) 
        {}
    
        float cfrq,cgain;
        float tfrq,tgain;
        int togo;
        float phase;
    };

    float deftime;
    typedef std::map<int,Guide> GuideMap;
    GuideMap guides;
    typedef std::vector<int> RemoveVec;
    RemoveVec rmv;

    FLEXT_CALLBACK_I(mg_partial)
    FLEXT_CALLGET_I(mg_partials)
    
    buffer buf;
    FLEXT_CALLVAR_V(mg_buffer,ms_buffer)
    FLEXT_CALLBACK(m_reset)
    FLEXT_CALLBACK_V(m_partial)
    FLEXT_ATTRVAR_F(deftime)
};

t_sample sms_guide_syn::costable[ARRAYSIZE+3];
t_symbol const *sms_guide_syn::sym_partial;

FLEXT_NEW_DSP_1("sms_guide_syn~",sms_guide_syn,t_symptr0)

} // namespace