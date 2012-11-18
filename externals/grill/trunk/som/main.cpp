#ifdef _REENTRANT
#undef _REENTRANT
#endif

#include <flext.h>
#include "som.hpp"
#include <fstream>

namespace {

using namespace blitz;

class som
    : public flext_base
{
    FLEXT_HEADER_S(som,flext_base,setup);
    
public:
    som()
        : nodes(NULL)
        , fast(0),refine(true)
    {
        AddInAnything();
    }
    
    ~som()
    {
        delete nodes;
    }

protected:
    static void setup(t_classid c)
    {
        FLEXT_CADDMETHOD(c,0,m_find);
        FLEXT_CADDMETHOD_(c,0,"load",m_load);
        FLEXT_CADDATTR_GET(c,"dim",mg_dim);
        FLEXT_CADDATTR_VAR1(c,"fast",fast);
        FLEXT_CADDATTR_VAR1(c,"refine",refine);
//        FLEXT_CADDATTR_VAR(c,"dim",mg_dim,ms_dim);
//        FLEXT_CADDATTR_VAR(c,"min",mg_min,ms_min);
//        FLEXT_CADDATTR_VAR(c,"max",mg_max,ms_max);
    }
    
    void m_load(const t_symbol *f)
    {
        const char *fname = GetString(f);
        std::ifstream file(fname);
        int cols,rows,n,i; bool c;
        file >> rows >> cols >> n >> c;
        if(cols && rows && n) {
            Array<t_float,1> mins(n),maxs(n);
            for(i = 0; i < n; ++i)
                file >> mins(i) >> maxs(i);
            
            delete nodes;
            if(c)
                nodes = new SOM<t_float,true>(cols,rows,n,mins,maxs);
            else
                nodes = new SOM<t_float,false>(cols,rows,n,mins,maxs);
    
            Array<t_float,1> v(n);
            for(int y = 0; y < rows; ++y)
                for(int x = 0; x < cols; ++x) {
                    for(int k = 0; k < n; ++k) file >> v(k);
                    (*nodes)(y,x,v);
                }
        }
    }
    
    void mg_dim(AtomList &l) const
    {
        if(!nodes) return;
        l(3);
        const TinyVector<int,3> &dim = nodes->length();
        SetInt(l[0],dim(0));
        SetInt(l[1],dim(1));
        SetInt(l[2],dim(2));
    }
    
/*
    void ms_dim(const AtomList &l)
    {
        if(l.Count() != 3) return;
        delete nodes;
        nodes = new SOM(GetAInt(l[0]),GetAInt(l[1]),GetAInt(l[2]));
    }
    
    void mg_min(AtomList &l) const
    {
        l(mins.length(0));
        for(int i = 0; i < l.Count(); ++i)
            SetFloat(l[i],mins(i));
    }
    
    void ms_min(const AtomList &l)
    {
        if(l.Count() != mins.length(0)) return;
        for(int i = 0; i < l.Count(); ++i)
            mins(i) = GetFloat(l[i]);
    }
    
    void mg_max(AtomList &l) const
    {
        l(maxs.length(0));
        for(int i = 0; i < l.Count(); ++i)
            SetFloat(l[i],maxs(i));
    }
    
    void ms_max(const AtomList &l)
    {
        if(l.Count() != maxs.length(0)) return;
        for(int i = 0; i < l.Count(); ++i)
            maxs(i) = GetFloat(l[i]);
    }
*/
    void m_find(int argc,t_atom *argv) const
    {
        if(!nodes) return;
        
        TinyVector<int,3> dim(nodes->length());
        if(argc != dim(2)) return;
        
        Array<float,1> v(argc);
        for(int i = 0; i < argc; ++i)
            v(i) = GetFloat(argv[i]);
            
        float x,y;
        float d = nodes->find(v,x,y,fast,refine);
        t_atom ret[3];
        SetFloat(ret[0],x/dim(0));
        SetFloat(ret[1],y/dim(1));
        SetFloat(ret[2],d);
        ToOutList(GetOutAttr(),3,ret);
    }
    
private:

    SOMBase<t_float> *nodes;
    float fast;
    bool refine;
    
    FLEXT_CALLBACK_V(m_find)
    FLEXT_CALLBACK_S(m_load)
    FLEXT_CALLGET_V(mg_dim)
    FLEXT_ATTRVAR_F(fast)
    FLEXT_ATTRVAR_B(refine)
//    FLEXT_CALLSET_V(ms_dim)
//    FLEXT_CALLVAR_V(mg_min,ms_min)
//    FLEXT_CALLVAR_V(mg_max,ms_max)
};

FLEXT_NEW("som",som)

} // namespace
