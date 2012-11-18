#ifdef _REENTRANT
#undef _REENTRANT
#endif

#include <flext.h>
#include <cstdlib>
#include <blitz/array.h>

namespace {

using namespace blitz;

class dct
    : public flext_base
{
    FLEXT_HEADER_S(dct,flext_base,setup);
    
public:
    dct(int nc)
        : nceps(nc)
    {
        AddInAnything();
        AddOutList();
    }

protected:
    static void setup(t_classid c)
    {
        FLEXT_CADDMETHOD(c,0,m_list);
        FLEXT_CADDATTR_VAR1(c,"nceps",nceps);
    }
    
    void m_list(int argc,const t_atom *argv)
    {
        if(!nceps) return;
    
        int nfilt = argc;
        Array<t_float,1> energies(nfilt);
        for(int i = 0; i < nfilt; ++i)
            energies(i) = GetAFloat(argv[i]);

        if(nceps != dctmat.length(0) || nfilt != dctmat.length(1)) {
            dctmat.resize(nceps,nfilt);
            float nrm = 1./sqrt(nfilt);
            dctmat = cos((secondIndex()+0.5)*firstIndex()*M_PI/nfilt)*nrm;
//            dctmat(0,Range::all()) *= M_SQRT1_2;  // orthogonalize
        }
            
        AtomList ret(nceps);
        for(int i = 0; i < nceps; ++i) {
            float v = sum(energies*dctmat(i,Range::all()));
            SetFloat(ret[i],v);
        }
        ToOutList(0,ret);
    }
    
    Array<t_float,2> dctmat;
    int nceps;
    
private:
    FLEXT_CALLBACK_V(m_list)
    FLEXT_ATTRVAR_I(nceps)
};

FLEXT_NEW_1("dct",dct,int0)

} // namespace
