/* 

absattr - patcher attributes

Copyright (c) 2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#undef FLEXT_ATTRIBUTES

#define VERSION "0.2.3"

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 501)
#error You need at least flext version 0.5.1
#endif

#include <map>
#include <set>

class absattr
    : public flext_base
{
	FLEXT_HEADER_S(absattr,flext_base,Setup)

public:
    absattr(int argc,const t_atom *argv)
          : parent(0),prior(0)
          , loadbang(true),echo(true)
    {
        AddInAnything("bang/get/set");
        AddInAnything("external attribute messages");
        AddOutAnything("arguments");
        AddOutAnything("attributes");
        AddOutAnything("external attribute outlet");

		// process default values (only attributes can have default values)
		Process(argc,argv,false);

		// process canvas arguments
        AtomListStatic<20> args;
        GetCanvasArgs(args);
        Process(args.Count(),args.Atoms(),true);

        // add to loadbang registry
        Loadbangs::iterator it = loadbangs.find(parent);
		if(it != loadbangs.end())
			it->second.obj.insert(this);
		else {
			Loadbang &lb = loadbangs[parent];
			lb.lasttime = -1;
			lb.obj.insert(this);
		}
    }

    ~absattr()
    {
        // remove from loadbang registry
        Loadbangs::iterator it = loadbangs.find(parent);
        if(it != loadbangs.end()) {
            Objects &o = it->second.obj;
			Objects::iterator oit;
            for(oit = o.begin(); oit != o.end(); ++oit) {
                if(*oit == this) {
                    // found
                    o.erase(oit);
                    break;
                }
            }
            FLEXT_ASSERT(oit != o.end());
            if(o.empty()) loadbangs.erase(it);
        }
        else
            error("%s - not found in loadbang registry (parent=%i)",thisName(),parent);
    }

    //! dump parameters
    void m_bang() { BangAttr(0); }

    void m_bangx() { BangAttr(1); }

    void m_dump() { DumpAttr(0); }

    void m_get(const t_symbol *s) { OutAttr(0,s); }

    void m_dumpx() { DumpAttr(1); }

    void m_getx(const t_symbol *s) { OutAttr(1,s); }

    void m_set(int argc,const t_atom *argv)
    {
        if(!argc || !IsSymbol(*argv))
            post("%s - attribute must be given as first argument",thisName());
        else {
            const t_symbol *attr = GetSymbol(*argv);
            --argc,++argv;
            SetAttr(attr,argc,argv);
            if(echo)
                OutAttr(0,attr); // tell abstraction about the changed value
        }
    }

protected:

    typedef std::map<const t_symbol *,AtomList> AttrMap;

    int parent;  // don't change after inserting into registry
    float prior; // don't change after inserting into registry
    bool loadbang;
    bool echo;

    AtomList args;
    AttrMap attrs;  

    class Compare
    {
    public:
        bool operator()(const absattr *a,const absattr *b) const { return a->prior == b->prior?a < b:a->prior < b->prior; }
    };

    typedef std::set<absattr *,Compare> Objects;

    struct Loadbang 
    {
        double lasttime;
        Objects obj;
    };

    typedef std::map<int,Loadbang> Loadbangs;

    static Loadbangs loadbangs;

    virtual void CbLoadbang() 
    { 
        if(parent) {      
            // we are a sub-abstraction, sharing a parent with others

            // all loadbangs have the same logical time
    	    double time = GetTime();

            Loadbangs::iterator it = loadbangs.find(parent);
            if(it != loadbangs.end()) {
                Loadbang &lb = it->second;
                // found
                if(lb.lasttime < time) {
                    // bang all objects with the same parent in the prioritized order
                    for(Objects::iterator oit = lb.obj.begin(); oit != lb.obj.end(); ++oit) {
                        absattr *o = *oit;
                        FLEXT_ASSERT(o);
                        if(o->loadbang) o->m_bang();
                    }

                    // set timestamp
                    lb.lasttime = time;
                }
            }
            else
                error("%s - not found in database",thisName());
        }
        else {
            // loadbang only this
            if(loadbang) m_bang();
        }
    }

    void BangAttr(int ix)
    {
        if(ix == 0)
            ToSysList(ix,args);

        for(AttrMap::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
            const AtomList &lst = it->second;
            ToSysAnything(1+ix,it->first,lst.Count(),lst.Atoms());
        }
    }

    void DumpAttr(int ix)
    {
        int cnt = attrs.size();
        AtomListStatic<20> lst(cnt);

        AttrMap::const_iterator it = attrs.begin();
        for(int i = 0; it != attrs.end(); ++it,++i)
            SetSymbol(lst[i],it->first);

        ToSysAnything(1+ix,sym_attributes,lst.Count(),lst.Atoms());
    }

    void OutAttr(int ix,const t_symbol *s)
    {
        AttrMap::const_iterator it = attrs.find(s);
        if(it != attrs.end()) {
            const AtomList &lst = it->second;
            ToSysAnything(1+ix,s,lst.Count(),lst.Atoms());
        }
        else
            post("%s - attribute %s not found",thisName(),GetString(s));
    }

    void SetAttr(const t_symbol *attr,int argc,const t_atom *argv)
    {
        if(argc) {
            AtomList &lst = attrs[attr]; 
            lst.Set(argc,argv,0,true);
        }
        else
            attrs.erase(attr);
    }

    static bool IsAttr(const t_atom &at) { return IsSymbol(at) && *GetString(at) == '@'; }

    void Process(int argc,const t_atom *argv,bool ext)
    {
        int cnt = CheckAttrib(argc,argv);

        args.Set(cnt,argv,0,true);
        argc -= cnt,argv += cnt;

        while(argc) {
            FLEXT_ASSERT(IsAttr(*argv));
            const t_symbol *attr = MakeSymbol(GetString(*argv)+1);
            --argc,++argv;

            cnt = CheckAttrib(argc,argv);
            if(cnt) {
                if(attr == sym_loadbang) {
                    if(ext) {
                        if(cnt == 2 && CanbeInt(argv[0]) && CanbeFloat(argv[1])) {
                            parent = GetAInt(argv[0]);
                            prior = GetAFloat(argv[1]);
                        }
                        else
                            post("%s - expected: @loadbang [parent-id ($0)] priority",thisName());
                    }
                    else {
                        if(cnt == 1 && CanbeBool(*argv))
                            loadbang = GetABool(*argv);
                        else
                            post("%s - expected: @loadbang 0/1",thisName());
                    }
                }
                else {
                    AtomList &lst = attrs[attr];
                    lst.Set(cnt,argv,0,true);
                }
                argc -= cnt,argv += cnt;
            }
            else
                attrs.erase(attr);
        }
    }

    void ms_echo(bool e) { echo = e; }

private:

    static const t_symbol *sym_attributes;
    static const t_symbol *sym_loadbang;

    FLEXT_CALLBACK(m_bang);
    FLEXT_CALLBACK_S(m_get);
    FLEXT_CALLBACK(m_dump);
    FLEXT_CALLBACK(m_bangx);
    FLEXT_CALLBACK_S(m_getx);
    FLEXT_CALLBACK(m_dumpx);
    FLEXT_CALLBACK_V(m_set);
    FLEXT_CALLBACK_B(ms_echo);

	static void Setup(t_classid cl)
    {
	    post("absattr " VERSION ", (C)2006-2008 Thomas Grill");
#ifdef FLEXT_DEBUG
        post("--- DEBUG VERSION ---");
#endif

        sym_attributes = MakeSymbol("attributes");
        sym_loadbang = MakeSymbol("loadbang");

        FLEXT_CADDMETHOD(cl,0,m_bang);
        FLEXT_CADDMETHOD_(cl,0,"echo",ms_echo);
        FLEXT_CADDMETHOD_(cl,0,"get",m_get);
        FLEXT_CADDMETHOD_(cl,0,"getattributes",m_dump);
        FLEXT_CADDMETHOD_(cl,0,"set",m_set);
        FLEXT_CADDMETHOD(cl,1,m_bangx);
        FLEXT_CADDMETHOD_(cl,1,"get",m_getx);
        FLEXT_CADDMETHOD_(cl,1,"getattributes",m_dumpx);
        FLEXT_CADDMETHOD_(cl,1,"set",m_set);
    }
};

const t_symbol *absattr::sym_attributes;
const t_symbol *absattr::sym_loadbang;

absattr::Loadbangs absattr::loadbangs;

FLEXT_NEW_V("absattr",absattr)
