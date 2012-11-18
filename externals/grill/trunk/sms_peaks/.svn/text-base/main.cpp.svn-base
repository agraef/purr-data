/* 
sms_peaks - FFT peak analysis with interpolation

Copyright (c)2008-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt" in this distribution.  
*/

#define FLEXT_ATTRIBUTES 1
#define _USE_MATH_DEFINES

#include <flext.h>
#include <vector>
#include <cmath>

namespace {

class sms_peaks
    : public flext_dsp
{
    FLEXT_HEADER_S(sms_peaks,flext_dsp,setup)
    
public:
    sms_peaks(const t_symbol *bufname = NULL)
        : frqmin(50),frqmax(15000)
        , thrabspow(-60),thrrelpow(-60),thrrelpeak(-30),thrreldist(0)
        , autobang(false)
    {
        found.reserve(100);
    
        AddInSignal();
        AddOutList();
	    FLEXT_ADDTIMER(tmr,m_bang);

        if(bufname) {
            t_atom at; 
            SetSymbol(at,bufname);
            ms_buffer(1,&at);
        }
    }
    
protected:

    static float lin2db(float x) { return log10(x)*20; }
    static float db2lin(float x) { return pow(10,x/20); }

    virtual bool CbDsp()
    {
        slog.resize(Blocksize()/2);
        return true;
    }

    virtual void CbSignal()
    {
        int i,n = Blocksize()/2;
        t_sample const *in = InSig(0);
        float nyfrq = Samplerate()/2;
        
        found.clear();

        int mn = int(frqmin/nyfrq*n);
        int mx = int(ceil(frqmax/nyfrq*n));

        float sum = 0;
        for(i = mn; i < mx; ++i)
            sum += in[i];
        float thr = std::max(lin2db(sum)+thrrelpow,thrabspow);
        
        const float nf = nyfrq/n;
        const int imin = std::max(mn,1),imax = std::min(mx,n-1); 

        for(i = imin; i < imax; ++i)
            slog[i] = lin2db(in[i]);
        
        for(i = imin; i < imax; ++i) {
            t_sample s = slog[i];
            if(s >= thr) {
                const float in_p = slog[i-1],in_n = slog[i+1];
                if((s-in_p) >= thrreldist && (s-in_n) >= thrreldist) {
                    // interpolation
                    float p = (in_p-in_n)/(in_p+in_n-s*2)/2;
                    float f = (i+p)*nf;
                    s -= (in_p-in_n)*p/4;
                    found.push_back(std::pair<float,float>(f,s));
                }
            }
        }
        
        if(buf.Symbol()) {
            bool ok = buf.Valid();
            if(ok) {
                buf.Update();
                ok = buf.Ok();
            }

            if(ok) {
                int i = 0,cnt = std::min<int>((buf.Frames()-1)/2,found.size());
                flext::buffer::Element *bufdt = buf.Data();
                for(; i < cnt; ++i) {
                    bufdt[i*2+0] = found[i].first;
                    bufdt[i*2+1] = db2lin(found[i].second);
                }
                bufdt[i*2] = -1; // sentinel
            }
        }
        else if(autobang)
            tmr.Now();
    }

    void m_bang(void * = NULL)
    {
        AtomListStatic<32> lst(found.size()*2);
        int i,cnt;
        float g = -1000;
        for(i = 0; i < found.size(); ++i)
            g = std::max(g,found[i].second);
        g += thrrelpeak;
            
        for(i = cnt = 0; i < found.size(); ++i) 
            if(found[i].second >= g) {
                SetFloat(lst[cnt++],found[i].first);
                SetFloat(lst[cnt++],db2lin(found[i].second));
            }
        ToOutList(0,cnt,lst.Atoms());
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

    static void setup(t_classid c)
    {
        FLEXT_CADDATTR_VAR1(c,"fmin",frqmin);
        FLEXT_CADDATTR_VAR1(c,"fmax",frqmax);
        FLEXT_CADDATTR_VAR1(c,"abspow",thrabspow);
        FLEXT_CADDATTR_VAR1(c,"relpow",thrrelpow);
        FLEXT_CADDATTR_VAR1(c,"relpeak",thrrelpeak);
        FLEXT_CADDATTR_VAR1(c,"reldist",thrreldist);
        FLEXT_CADDMETHOD(c,0,m_bang);
        FLEXT_CADDATTR_VAR1(c,"auto",autobang);
        
        FLEXT_CADDATTR_VAR(c,"buffer",mg_buffer,ms_buffer);
    }
    
private:

    std::vector<t_sample> slog;
    std::vector<std::pair<float,float> > found;
    float frqmin,frqmax,thrabspow,thrrelpow,thrrelpeak,thrreldist;
        
    FLEXT_ATTRVAR_F(frqmin)
    FLEXT_ATTRVAR_F(frqmax)
    FLEXT_ATTRVAR_F(thrabspow)
    FLEXT_ATTRVAR_F(thrrelpow)
    FLEXT_ATTRVAR_F(thrrelpeak)
    FLEXT_ATTRVAR_F(thrreldist)

    Timer tmr;
    bool autobang;

    FLEXT_ATTRVAR_B(autobang)
    FLEXT_CALLBACK(m_bang)
    FLEXT_CALLBACK_T(m_bang)

    buffer buf;
    FLEXT_CALLVAR_V(mg_buffer,ms_buffer)
};

FLEXT_NEW_DSP_1("sms_peaks~",sms_peaks,t_symptr0)

} // namespace