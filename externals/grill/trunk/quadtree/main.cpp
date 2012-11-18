#include <flext.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "ntree.hpp"

// anonymous namespace ... no exports
namespace {

static const int QPTS = 16;

class ntree_base
: public flext
{
public:
    virtual ~ntree_base() {}

    virtual void m_reset() = 0;
    
    virtual void mg_size(int &s) const = 0;
    virtual void mg_leaves(int &l) const = 0;
    virtual void mg_depth(int &d) const = 0;

    virtual void m_add(int argc,t_atom const *argv) = 0;

    virtual bool m_find(int argc,t_atom const *argv,std::list<AtomList *> &lst) const = 0;

    virtual void m_load(const t_symbol *path) = 0;
    virtual void m_save(const t_symbol *path) const = 0;
};


inline std::ostream &operator <<(std::ostream &s,const flext::AtomList &l)
{
    char buf[256];
    for(int i = 0; i < l.Count(); ++i) {
        flext::PrintAtom(l[i],buf,sizeof buf);
        if(i) s << ' ';
        s << buf;
    }
    return s;
}

inline std::istream &operator >>(std::istream &s,flext::AtomList &l)
{
    flext::AtomListStatic<256> lst(256);
    std::string str;
    getline(s,str);
    if(!s.eof() && !s.bad()) {
        int c = lst.Scan(str.c_str());
        l.Set(c,lst.Atoms(),0,true);
    }
    return s;
}



template <int D = 2>
class ntree_imp
: public ntree_base
{
protected:
    class Data:
        public DataPoint<double,D>
    {
    public:
        Data(double p[D],int argc = 0,const t_atom *argv = NULL): DataPoint<double,D>(p),object(argc,argv) {}
        Data(double p[D],const AtomList &o): DataPoint<double,D>(p),object(o) {}
        AtomList const object;
    };

public:
    virtual void m_reset() { tree.Clear(); }


    virtual void mg_size(int &s) const { s = tree.GetCount(); }

    virtual void mg_leaves(int &l) const { l = tree.GetLeaves(); }

    virtual void mg_depth(int &d) const { d = tree.GetDepth(); }


    virtual void m_add(int argc,t_atom const *argv) 
    {
        if(argc < D) return;
        double p[D];
        for(int d = 0; d < D; ++d) 
            p[d] = GetAFloat(argv[d]);
        tree.Add(new Data(p,argc-D,argv+D));
    }

    virtual bool m_find(int argc,t_atom const *argv,std::list<AtomList *> &lst) const
    {
        Vector<double,D> p; 
        
        if(argc < D) return false;
        for(int d = 0; d < D; ++d) p[d] = GetAFloat(argv[d]);
        argc -= D,argv += D;
        
        double maxdist = -1;
        if(argc)
            maxdist = GetAFloat(*argv++),--argc;
        
        int maxobjs = 0;
        if(argc)
            maxobjs = GetAInt(*argv++),--argc;
        
        LimitedList<double,D> l(maxobjs?maxobjs:1);
        tree.GetNeighbors(l,p,maxdist);
        for(typename LimitedList<double,D>::const_iterator it = l.begin(); it != l.end(); ++it) {
            DPnt<double,D> const &dp = *it;
            Data const *d = dynamic_cast<Data const *>(dp.Pt());
            FLEXT_ASSERT(d);
            AtomList *at = new AtomListStatic<8>(d->object.Count()+D+1);
            double dist = 0;
            for(int d = 0; d < D; ++d) {
                SetFloat((*at)[d],dp[d]);
                dist += sqr(dp[d]-p[d]);
            }
            SetFloat((*at)[D],sqrt(dist));
            at->Set(d->object.Count(),d->object.Atoms(),D+1);
            lst.push_back(at);
        }
        
        return maxobjs != 0; // termination flag
    }

    virtual void m_load(const t_symbol *path)
    {
        std::ifstream s(GetString(path));
        if(!s.bad()) {
            tree.Clear();
            
            for(;;) {
                double p[D];
                for(int d = 0; d < D; ++d) s >> p[d];
                AtomListStatic<16> lst;
                s >> lst;
                if(s.eof()) break;
                
                tree.Add(new Data(p,lst));
            }
        }
        
//        std::cerr << *this << std::endl;
    }
    
    virtual void m_save(const t_symbol *path) const
    {
        std::ofstream s(GetString(path));
        if(!s.bad())
            recdump(s,tree);
    }

private:

    NTree<double,D,QPTS> tree;

    template<typename T>
    static T sqr(T x) { return x*x; }

    template <typename Q>
    static void recdump(std::ostream &s,Q const &q)
    {
        if(q.self) {
            for(int i = 0; i < q.pts.size(); ++i) {
                Data const *d = dynamic_cast<Data const *>(q.pts[i]);
                FLEXT_ASSERT(d);
                s << *d << d->object << std::endl;
            }
        }
        else {
            for(int i = 0; i < q.sub.size(); ++i)
                if(q.sub[i])
                    recdump(s,*q.sub[i]);
        }
    }
};


class ntree
: public flext_base
{
    FLEXT_HEADER_S(ntree,flext_base,setup)

public:
    ntree(int d) 
    {
        AddInAnything();

        switch(d) {
        case 1:
            tree = new ntree_imp<1>;
            break;
        case 2:
            tree = new ntree_imp<2>;
            break;
        case 3:
            tree = new ntree_imp<3>;
            break;
        case 4:
            tree = new ntree_imp<4>;
            break;
        default:
            InitProblem();
        }
    }

    void m_reset() { tree->m_reset(); }
    
    void mg_size(int &s) { tree->mg_size(s); }
    void mg_leaves(int &l) { tree->mg_leaves(l); }
    void mg_depth(int &d) { tree->mg_depth(d); }

    void m_add(int argc,t_atom const *argv) { tree->m_add(argc,argv); }

    void m_find(int argc,t_atom const *argv) const
    {
        std::list<AtomList *> lst;
        bool term = tree->m_find(argc,argv,lst);
        for(std::list<AtomList *>::const_iterator it = lst.begin(); it != lst.end(); ++it) {
            AtomList *l = *it;
            ToOutAnything(GetOutAttr(),thisTag(),*l);
            delete l;
        }
        if(term) 
            // termination
            ToOutAnything(GetOutAttr(),thisTag(),0,NULL);
    }

    void m_load(const t_symbol *path) { tree->m_load(path); }
    void m_save(const t_symbol *path) const { tree->m_save(path); }

protected:

    ntree_base *tree;

    FLEXT_CALLBACK(m_reset)
    FLEXT_CALLBACK_V(m_add)
    FLEXT_CALLBACK_V(m_find)
    FLEXT_CALLBACK_S(m_load)
    FLEXT_CALLBACK_S(m_save)
    
    FLEXT_CALLGET_I(mg_size)
    FLEXT_CALLGET_I(mg_leaves)
    FLEXT_CALLGET_I(mg_depth)

    static void setup(t_classid c)
    {
        post("ntree, (c)2008 grrrr.org");
    
        FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
        FLEXT_CADDMETHOD_(c,0,"add",m_add);
        FLEXT_CADDMETHOD_(c,0,"find",m_find);
        FLEXT_CADDMETHOD_(c,0,"load",m_load);
        FLEXT_CADDMETHOD_(c,0,"save",m_save);
        
        FLEXT_CADDATTR_GET(c,"size",mg_size);
        FLEXT_CADDATTR_GET(c,"leaves",mg_leaves);
        FLEXT_CADDATTR_GET(c,"depth",mg_depth);
    }
};

FLEXT_NEW_1("ntree",ntree,int)

}  // namespace
