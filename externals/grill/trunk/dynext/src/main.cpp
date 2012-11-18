/* 

dyn~ - dynamical object management for PD

Copyright (c)2003-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  


-- this is all a bit hacky, but hey, it's PD! --

*/

#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#define DYN_VERSION "0.1.2"


#if FLEXT_SYS != FLEXT_SYS_PD
#error Sorry, dyn~ works for PD only!
#endif


#include "flinternal.h"
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable: 4091 4244)
#endif
#include "g_canvas.h"


class dyn:
	public flext_dsp
{
	FLEXT_HEADER_S(dyn,flext_dsp,setup)
public:
	dyn(int argc,const t_atom *argv);
	virtual ~dyn();

    void m_reset();
	void m_reload(); // refresh objects/abstractions
	void m_newobj(int argc,const t_atom *argv);
	void m_newmsg(int argc,const t_atom *argv);
	void m_newtext(int argc,const t_atom *argv);
	void m_del(const t_symbol *n);
	void m_connect(int argc,const t_atom *argv) { ConnDis(true,argc,argv); }
	void m_disconnect(int argc,const t_atom *argv) { ConnDis(false,argc,argv); }
	void m_send(int argc,const t_atom *argv);
    void ms_vis(bool vis) { canvas_vis(canvas,vis?1:0); }
    void mg_vis(bool &vis) const { vis = canvas && canvas->gl_editor; }

protected:

    virtual void CbClick() { ms_vis(true); }

    static const t_symbol *k_obj,*k_msg,*k_text;

	class Obj {
	public:
		Obj(t_glist *gl,t_gobj *o): glist(gl),object(o) {}

        t_glist *AsGlist() const 
        { 
            return pd_class(&object->g_pd) == canvas_class?(t_glist *)object:NULL; 
        }

        t_glist *glist;
		t_gobj *object;
	};
    
    class ObjMap
        :public TablePtrMap<const t_symbol *,Obj *>
    {
    public:
        virtual ~ObjMap() { clear(); }
            
        virtual void clear()
        {
            for(iterator it(*this); it; ++it) delete it.data();
            TablePtrMap<const t_symbol *,Obj *>::clear();
        }
    } root;

    typedef TablePtrMap<Obj *,const t_symbol *> GObjMap;

    class GLstMap
        :public TablePtrMap<t_glist *,GObjMap *>
    {
    public:
        virtual ~GLstMap() { clear(); }
            
        virtual void clear()
        {
            for(iterator it(*this); it; ++it) delete it.data();
            TablePtrMap<t_glist *,GObjMap *>::clear();
        }
    } groot;

    Obj *Find(const t_symbol *n) { return root.find(n); }
    t_glist *FindCanvas(const t_symbol *n);

    Obj *Remove(const t_symbol *n);
    bool Add(const t_symbol *n,t_glist *gl,t_gobj *o);

    t_gobj *New(const t_symbol *kind,int _argc_,const t_atom *_argv_,bool add = true);

	void ConnDis(bool conn,int argc,const t_atom *argv);

    void DoInit();
    void DoExit();
    void NewProxies();
    void DelProxies();

    virtual bool CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv);
	virtual bool CbDsp();


	// proxy object
	class proxy
	{ 
	public:
		t_object obj;
		dyn *th;

		void init(dyn *t)
        { 
	        th = t; 
        }
	};

	// proxy object
	class sigproxy
        : public proxy
	{ 
	public:
		t_sample defsig;
		int n;
		t_sample *vec;

		void init(dyn *t)
        { 
            proxy::init(t); 
	        defsig = 0;
	        n = 0,vec = NULL;
        }

        void set(int n,t_sample *vec)
        {
            this->n = n;
            this->vec = vec;
        }
	};

	// proxy for inbound messages
	class proxyin:
		public proxy
	{ 
	public:
		void init(dyn *t)
        { 
	        proxy::init(t);
	        outlet_new(&obj,&s_anything); 
        }

		void Message(const t_symbol *s,int argc,const t_atom *argv) 
		{
			outlet_anything(obj.te_outlet,(t_symbol *)s,argc,(t_atom *)argv);
		}

		static void px_method(proxyin *obj,const t_symbol *s,int argc,const t_atom *argv)
		{
			obj->Message(s,argc,argv);
		}
	};

	// proxy for inbound signals
	class sigproxyin:
		public sigproxy
	{ 
	public:
		void init(dyn *t)
        { 
	        sigproxy::init(t);
	        outlet_new(&obj,&s_signal); 
        }

		static void dsp(sigproxyin *x, t_signal **sp);
	};


	// proxy for outbound messages
	class proxyout:
		public proxy
	{ 
	public:
		void init(dyn *t,int o)
        { 
	        proxy::init(t);
	        outlet = o;
        }

		static void px_method(proxyout *obj,const t_symbol *s,int argc,const t_atom *argv)
		{
			obj->th->ToSysAnything(obj->outlet,s,argc,argv);
		}

    protected:
		int outlet;
	};

	// proxy for outbound messages
	class sigproxyout:
		public sigproxy
	{ 
	public:
		void init(dyn *t)
        { 
	        sigproxy::init(t);
	        outlet_new(&obj,&s_signal); 
        }

		static void dsp(sigproxyout *x, t_signal **sp);
	};

	static t_class *pxin_class,*pxout_class;
	static t_class *pxins_class,*pxouts_class;

    int s_inlets,m_inlets,s_outlets,m_outlets;
    sigproxyin **pxsin;
	proxyin **pxmin;
	sigproxyout **pxsout;
	proxyout **pxmout;

    static t_object *pxin_new() { return (t_object *)pd_new(pxin_class); }
    static t_object *pxins_new() { return (t_object *)pd_new(pxins_class); }
    static t_object *pxout_new() { return (t_object *)pd_new(pxout_class); }
    static t_object *pxouts_new() { return (t_object *)pd_new(pxouts_class); }

	t_canvas *canvas;
    bool stripext,canvasmsg,symreuse;

private:
	static void setup(t_classid c);

	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK(m_reload)
	FLEXT_CALLBACK_V(m_newobj)
	FLEXT_CALLBACK_V(m_newmsg)
	FLEXT_CALLBACK_V(m_newtext)
	FLEXT_CALLBACK_S(m_del)
	FLEXT_CALLBACK_V(m_connect)
	FLEXT_CALLBACK_V(m_disconnect)
	FLEXT_CALLBACK_V(m_send)
	FLEXT_CALLVAR_B(mg_vis,ms_vis)

	FLEXT_ATTRVAR_B(stripext)
	FLEXT_ATTRVAR_B(symreuse)
	FLEXT_ATTRVAR_B(canvasmsg)

    static const t_symbol *sym_dot,*sym_dynsin,*sym_dynsout,*sym_dynin,*sym_dynout,*sym_dyncanvas;
    static const t_symbol *sym_vis,*sym_loadbang,*sym_dsp,*sym_pop;
};

FLEXT_NEW_DSP_V("dyn~",dyn)


t_class *dyn::pxin_class = NULL,*dyn::pxout_class = NULL;
t_class *dyn::pxins_class = NULL,*dyn::pxouts_class = NULL;

const t_symbol *dyn::k_obj = NULL;
const t_symbol *dyn::k_msg = NULL;
const t_symbol *dyn::k_text = NULL;

const t_symbol *dyn::sym_dot = NULL;
const t_symbol *dyn::sym_dynsin = NULL;
const t_symbol *dyn::sym_dynsout = NULL;
const t_symbol *dyn::sym_dynin = NULL;
const t_symbol *dyn::sym_dynout = NULL;
const t_symbol *dyn::sym_dyncanvas = NULL;

const t_symbol *dyn::sym_vis = NULL;
const t_symbol *dyn::sym_loadbang = NULL;
const t_symbol *dyn::sym_dsp = NULL;
const t_symbol *dyn::sym_pop = NULL;


void dyn::setup(t_classid c)
{
	post("");
	post("dyn~ %s - dynamic object management, (C)2003-2006 Thomas Grill",DYN_VERSION);
	post("");

    sym_dynsin = MakeSymbol("dyn_in~");
    sym_dynsout = MakeSymbol("dyn_out~");
    sym_dynin = MakeSymbol("dyn_in");
    sym_dynout = MakeSymbol("dyn_out");

    sym_dot = MakeSymbol(".");
    sym_dyncanvas = MakeSymbol(" dyn~-canvas ");

	// set up proxy class for inbound messages
    pxin_class = class_new(const_cast<t_symbol *>(sym_dynin),(t_newmethod)pxin_new,NULL,sizeof(proxyin),0, A_NULL);
	add_anything(pxin_class,proxyin::px_method); 

	// set up proxy class for inbound signals
	pxins_class = class_new(const_cast<t_symbol *>(sym_dynsin),(t_newmethod)pxins_new,NULL,sizeof(sigproxyin),0, A_NULL);
    add_dsp(pxins_class,sigproxyin::dsp);
    CLASS_MAINSIGNALIN(pxins_class, sigproxyin, defsig);

	// set up proxy class for outbound messages
	pxout_class = class_new(const_cast<t_symbol *>(sym_dynout),(t_newmethod)pxout_new,NULL,sizeof(proxyout),0, A_NULL);
	add_anything(pxout_class,proxyout::px_method); 

	// set up proxy class for outbound signals
	pxouts_class = class_new(const_cast<t_symbol *>(sym_dynsout),(t_newmethod)pxouts_new,NULL,sizeof(sigproxyout),0, A_NULL);
	add_dsp(pxouts_class,sigproxyout::dsp);
    CLASS_MAINSIGNALIN(pxouts_class, sigproxyout, defsig);

	// set up dyn~
	FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
	FLEXT_CADDMETHOD_(c,0,"reload",m_reload);
	FLEXT_CADDMETHOD_(c,0,"newobj",m_newobj);
	FLEXT_CADDMETHOD_(c,0,"newmsg",m_newmsg);
	FLEXT_CADDMETHOD_(c,0,"newtext",m_newtext);
	FLEXT_CADDMETHOD_(c,0,"del",m_del);
	FLEXT_CADDMETHOD_(c,0,"conn",m_connect);
	FLEXT_CADDMETHOD_(c,0,"dis",m_disconnect);
	FLEXT_CADDMETHOD_(c,0,"send",m_send);
	FLEXT_CADDATTR_VAR(c,"vis",mg_vis,ms_vis);
    FLEXT_CADDATTR_VAR1(c,"stripext",stripext);
    FLEXT_CADDATTR_VAR1(c,"symreuse",symreuse);
    FLEXT_CADDATTR_VAR1(c,"canvasmsg",canvasmsg);

    // set up symbols
    k_obj = MakeSymbol("obj"); 
    k_msg = MakeSymbol("msg"); 
    k_text = MakeSymbol("text"); 

    sym_vis = MakeSymbol("vis");
    sym_loadbang = MakeSymbol("loadbang");
    sym_dsp = MakeSymbol("dsp");
    sym_pop = MakeSymbol("pop");
}


/*
There must be a separate canvas for the dynamically created objects as some mechanisms in PD
(like copy/cut/paste) get confused otherwise.
On the other hand it seems to be possible to create objects without a canvas. 
They won't receive DSP processing, though, hence it's only possible for message objects.
Problems arise when an object is not yet loaded... the canvas environment is then needed to
load it.. if there is no canvas, PD currently crashes.


How to create the canvas:
1) via direct call to canvas_new()
2) a message to pd_canvasmaker
	con: does not return a pointer for the created canvas

There are two possibilities for the canvas
1) make a sub canvas to the one where dyn~ resides:
	pro: no problems with environment (abstractions are found and loaded correctly)
2) make a root canvas:
	pro: it will be in the canvas list per default, hence DSP is processed
	con: canvas environment must be created manually 
			(is normally done by pd_canvasmaker if there is a directory set, which is again done somewhere else)

Enabling DSP on the subcanvas
1) send it a "dsp" message (see rabin~ by K.Czaja)... but, which signal vector should be taken?
	-> answer: NONE!  (just send NULL)
2) add it to the list of _root_ canvases (these will be DSP-processed per default)
	(for this the canvas_addtolist and canvas_takefromlist functions are used)
	however, it's not clear if this can lead to problems since it is no root-canvas!

In all cases the 1)s have been chosen as the cleaner solution
*/

dyn::dyn(int argc,const t_atom *argv):
	canvas(NULL),
	pxsin(NULL),pxmin(NULL),pxsout(NULL),pxmout(NULL),
    stripext(false),symreuse(true),canvasmsg(false)
{
	if(argc < 4) { 
		post("%s - Syntax: dyn~ sig-ins msg-ins sig-outs msg-outs",thisName());
		InitProblem(); 
		return; 
	}

	s_inlets = GetAInt(argv[0]);
	m_inlets = GetAInt(argv[1]);
	s_outlets = GetAInt(argv[2]);
	m_outlets = GetAInt(argv[3]);

	// --- make a sub-canvas for dyn~ ------

	t_atom arg[6];
	SetInt(arg[0],0);	// xpos
	SetInt(arg[1],0);	// ypos
	SetInt(arg[2],700);	// xwidth 
	SetInt(arg[3],520);	// xwidth 
	SetSymbol(arg[4],sym_dyncanvas);	// canvas name
	SetInt(arg[5],0);	// visible

	canvas = canvas_new(NULL, NULL, 6, arg);
    // pop canvas (must do that...)
    SetInt(arg[0],0);
    pd_typedmess((t_pd *)canvas,(t_symbol *)sym_pop,1,arg);

    DoInit();

	AddInSignal("Messages (newobj,newmsg,newtext,del,conn,dis)");
	AddInSignal(s_inlets);
	AddInAnything(m_inlets);
	AddOutSignal(s_outlets);
	AddOutAnything(m_outlets);
}

dyn::~dyn()
{
	DoExit();

	if(canvas) pd_free((t_pd *)canvas);
}

void dyn::DoInit()
{
    // add to list of canvases
    groot.insert(canvas,new GObjMap);

    NewProxies();
}

void dyn::DoExit()
{
    // delete proxies
    DelProxies();
    // remove all objects
    if(canvas) glist_clear(canvas);
    // remove all names
    groot.clear();
    root.clear();
}

void dyn::NewProxies()
{
	// --- create inlet proxies ------
    int i;
	if(s_inlets) pxsin = new sigproxyin *[s_inlets];
	if(m_inlets) pxmin = new proxyin *[m_inlets];
	for(i = 0; i < s_inlets+m_inlets; ++i) {
        t_atom lst[5];
        SetInt(lst[0],i*100);
        SetInt(lst[1],10);
        SetSymbol(lst[2],sym_dot);
        SetSymbol(lst[3],sym__);

        try {
            if(i < s_inlets) {
                SetSymbol(lst[4],sym_dynsin);
                pxsin[i] = (sigproxyin *)New(k_obj,5,lst,false);
		        if(pxsin[i]) pxsin[i]->init(this);
            }
            else {
                int j = i-s_inlets;
                SetSymbol(lst[4],sym_dynin);
                pxmin[j] = (proxyin *)New(k_obj,5,lst,false);
		        if(pxmin[j]) pxmin[j]->init(this);
            }
        }
        catch(...) {
            error("%s - Error creating inlet proxy",thisName());
        }
    }

	// --- create outlet proxies ------

	if(s_outlets) pxsout = new sigproxyout *[s_outlets];
	if(m_outlets) pxmout = new proxyout *[m_outlets];
	for(i = 0; i < s_outlets+m_outlets; ++i) {
        t_atom lst[5];
        SetInt(lst[0],i*100);
        SetInt(lst[1],500);
        SetSymbol(lst[2],sym_dot);
        SetSymbol(lst[3],sym__);

        try {
            if(i < s_outlets) {
                SetSymbol(lst[4],sym_dynsout);
                pxsout[i] = (sigproxyout *)New(k_obj,5,lst,false);
                if(pxsout[i]) pxsout[i]->init(this);
            }
            else {
                int j = i-s_outlets;
                SetSymbol(lst[4],sym_dynout);
                pxmout[j] = (proxyout *)New(k_obj,5,lst,false);
                if(pxmout[j]) pxmout[j]->init(this,i);
            }
        }
        catch(...) {
            error("%s - Error creating outlet proxy",thisName());
        }
    }
}

void dyn::DelProxies()
{
    int i;
    if(pxsin) {
	    for(i = 0; i < s_inlets; ++i) glist_delete(canvas,(t_gobj *)pxsin[i]);
	    delete[] pxsin; pxsin = NULL;
    }
    if(pxmin) {
	    for(i = 0; i < m_inlets; ++i) glist_delete(canvas,(t_gobj *)pxmin[i]);
	    delete[] pxmin; pxmin = NULL;
    }
    if(pxsout) {
	    for(i = 0; i < s_outlets; ++i) glist_delete(canvas,(t_gobj *)pxsout[i]);
	    delete[] pxsout; pxsout = NULL;
    }
    if(pxmout) {
	    for(i = 0; i < m_outlets; ++i) glist_delete(canvas,(t_gobj *)pxmout[i]);
	    delete[] pxmout; pxmout = NULL;
    }
}

t_glist *dyn::FindCanvas(const t_symbol *n)
{
    if(n == sym_dot) 
        return canvas;
    else {
        Obj *o = Find(n);
        t_glist *gl = o->AsGlist();
        return gl && groot.find(gl)?(t_glist *)o->object:NULL;
    }
}

static t_gobj *GetLast(t_glist *gl)
{
    t_gobj *go = gl->gl_list;
    if(go)
        while(go->g_next) 
            go = go->g_next;
    return go;
}

bool dyn::Add(const t_symbol *n,t_glist *gl,t_gobj *o) 
{ 
    // remove previous name entry
    Obj *prv = Remove(n);
    if(prv) delete prv;

    // get canvas map
    GObjMap *gm = groot.find(gl);
    // if none existing create one
    if(!gm) return false;

    // insert object to canvas map
    Obj *obj = new Obj(gl,o);
    gm->insert(obj,n);
    // insert object to object map
    root.insert(n,obj); 

    t_glist *nl = obj->AsGlist();
    if(nl) {
        FLEXT_ASSERT(!groot.find(nl));
        groot.insert(nl,new GObjMap);
    }

    return true;
}

dyn::Obj *dyn::Remove(const t_symbol *n)
{
    // see if there's already an object of the same name
    Obj *prv = root.remove(n);
    if(prv) {
        t_glist *pl = prv->glist;
        // get canvas map
        GObjMap *gm = groot.find(pl);
        FLEXT_ASSERT(gm);
        // remove object from canvas map
        gm->remove(prv);

        // non-NULL if object itself is a glist
        t_glist *gl = prv->AsGlist();
        if(gl) {
            GObjMap *gm = groot.remove(gl);
            // if it's a loaded abstraction it need not be in our list
            if(gm) {
                // remove all objects in canvas map
                for(GObjMap::iterator it(*gm); it; ++it) {
                    Obj *r = Remove(it.data());
                    FLEXT_ASSERT(r);
                    delete r;
                }
                // delete canvas map
                delete gm;
            }
        }
    }
    return prv;
}

t_gobj *dyn::New(const t_symbol *kind,int _argc_,const t_atom *_argv_,bool add)
{
    t_gobj *newest = NULL;
    const char *err = NULL;
    const t_symbol *name = NULL,*canv = NULL;
    t_glist *glist = NULL;

    AtomListStatic<16> args;

	if(_argc_ >= 4 && CanbeInt(_argv_[0]) && CanbeInt(_argv_[1]) && IsSymbol(_argv_[2]) && IsSymbol(_argv_[3])) {
        canv = GetSymbol(_argv_[2]);
        name = GetSymbol(_argv_[3]);

        args(_argc_-2);
		SetInt(args[0],GetAInt(_argv_[0]));
		SetInt(args[1],GetAInt(_argv_[1]));
        for(int i = 0; i < _argc_; ++i) SetAtom(args[i+2],_argv_[i+4]);
	}
	else if(_argc_ >= 3 && IsSymbol(_argv_[0]) && IsSymbol(_argv_[1])) {
        canv = GetSymbol(_argv_[0]);
        name = GetSymbol(_argv_[1]);

        args(_argc_);
		// random position if not given
		SetInt(args[0],rand()%600);
		SetInt(args[1],50+rand()%400);
        for(int i = 0; i < _argc_-2; ++i) SetAtom(args[i+2],_argv_[i+2]);
	}

	if(args.Count()) {
        if(name == sym_dot)
			err = ". cannot be redefined";
        else if(!symreuse && root.find(name))
			err = "Name already in use";
        else if(!canv || !(glist = FindCanvas(canv)))
			err = "Canvas could not be found";
        else {
            // convert abstraction filenames
            if(stripext && kind == k_obj && args.Count() >= 3 && IsSymbol(args[2])) {
                const char *c = GetString(args[2]);
                int l = strlen(c);
                // check end of string for .pd file extension
                if(l >= 4 && !memcmp(c+l-3,".pd",4)) {
                    // found -> get rid of it
                    char tmp[64],*t = tmp;
                    if(l > sizeof tmp-1) t = new char[l+1];
                    memcpy(tmp,c,l-3); tmp[l-3] = 0;
                    SetString(args[2],tmp);
                    if(tmp != t) delete[] t;
                }
            }

            // set selected canvas as current
			canvas_setcurrent(glist); 

            t_gobj *last = GetLast(glist);
            pd_typedmess((t_pd *)glist,(t_symbol *)kind,args.Count(),args.Atoms());
            newest = GetLast(glist);

            if(kind == k_obj) {
                t_object *o = (t_object *)pd_newest();

                if(!o) {
                    // PD creates a text object when the intended object could not be created
                    t_gobj *trash = GetLast(glist);

                    // Test for newly created object....
                    if(trash && last != trash) {
                        // Delete it!
                        glist_delete(glist,trash);
                    }
                    newest = NULL;
                }
                else
                    newest = &o->te_g;
            }

			// look for latest created object
			if(newest) {
				// add to database
                if(add) {
                    bool ok = Add(name,glist,newest);
                    FLEXT_ASSERT(ok);
                }

				// send loadbang (if it is an abstraction)
				if(pd_class(&newest->g_pd) == canvas_class) {
					// hide the sub-canvas
					pd_vmess((t_pd *)newest,const_cast<t_symbol *>(sym_vis),"i",0);

                    // loadbang the abstraction
					pd_vmess((t_pd *)newest,const_cast<t_symbol *>(sym_loadbang),"");
                }

				// restart dsp - that's necessary because ToCanvas is called manually
				canvas_update_dsp();
			}
			else
				if(!err) err = "Could not create object";

			// pop the current canvas 
			canvas_unsetcurrent(glist); 
		}
	}
	else 
        if(!err) err = "new name object [args]";

    if(err) throw err;

    return newest;
}

void dyn::m_reset() 
{ 
    int dsp = canvas_suspend_dsp(); 
    DoExit(); 
    DoInit(); 
    canvas_resume_dsp(dsp); 
}

void dyn::m_reload()
{
	post("%s - reload: not implemented yet",thisName());
}

void dyn::m_newobj(int argc,const t_atom *argv)
{
    try { New(k_obj,argc,argv); }
    catch(const char *err) {
		post("%s - %s",thisName(),err);
    }
    catch(...) {
		post("%s - unknown error",thisName());
    }
}

void dyn::m_newmsg(int argc,const t_atom *argv)
{
    try { New(k_msg,argc,argv); }
    catch(const char *err) {
		post("%s - %s",thisName(),err);
    }
    catch(...) {
		post("%s - unknown error",thisName());
    }
}

void dyn::m_newtext(int argc,const t_atom *argv)
{
    try { New(k_text,argc,argv); }
    catch(const char *err) {
		post("%s - %s",thisName(),err);
    }
    catch(...) {
		post("%s - unknown error",thisName());
    }
}

void dyn::m_del(const t_symbol *n)
{
    Obj *obj = Remove(n);
    if(obj) {
        glist_delete(obj->glist,obj->object);
        delete obj;
    }
    else
		post("%s - del: object not found",thisName());
}

void dyn::ConnDis(bool conn,int argc,const t_atom *argv)
{
	const t_symbol *s_n = NULL,*d_n = NULL;
	int s_x,d_x;

	if(argc == 4 && IsSymbol(argv[0]) && CanbeInt(argv[1]) && IsSymbol(argv[2]) && CanbeInt(argv[3])) {
		s_n = GetSymbol(argv[0]);
		s_x = GetAInt(argv[1]);
		d_n = GetSymbol(argv[2]);
		d_x = GetAInt(argv[3]);
	}
	else if(argc == 3 && CanbeInt(argv[0]) && IsSymbol(argv[1]) && CanbeInt(argv[2])) {
		s_n = NULL;
		s_x = GetAInt(argv[0]);
		d_n = GetSymbol(argv[1]);
		d_x = GetAInt(argv[2]);
	}
	else if(argc == 3 && IsSymbol(argv[0]) && CanbeInt(argv[1]) && CanbeInt(argv[2])) {
		s_n = GetSymbol(argv[0]);
		s_x = GetAInt(argv[1]);
		d_n = NULL;
		d_x = GetAInt(argv[2]);
	}
	else if(argc == 2 && CanbeInt(argv[0]) && CanbeInt(argv[1])) {
		// direct connection from proxy-in to proxy-out
		s_n = NULL;
		s_x = GetAInt(argv[0]);
		d_n = NULL;
		d_x = GetAInt(argv[1]);
	}
	else {
		post("%s - connect: [src-name] src-slot [dst-name] dst-slot",thisName());
		return;
	}

	t_text *s_obj,*d_obj;
    t_glist *s_cnv,*d_cnv;
	if(s_n) {
		Obj *s_o = Find(s_n);
		if(!s_o) { 
			post("%s - connect: source \"%s\" not found",thisName(),GetString(s_n));
			return;
		}
		s_obj = (t_text *)s_o->object;
        s_cnv = s_o->glist;
	}
	else if(s_x < 0 && s_x >= s_inlets+m_inlets) {
		post("%s - connect: inlet %i out of range (0..%i)",thisName(),s_x,s_inlets+m_inlets-1);
		return;
	}
	else {
        s_obj = s_x < s_inlets?&pxsin[s_x]->obj:&pxmin[s_x-s_inlets]->obj;
        s_cnv = canvas;
		s_x = 0; // always 0 for proxy
	}

	if(d_n) {
		Obj *d_o = Find(d_n);
		if(!d_o) { 
			post("%s - connect: destination \"%s\" not found",thisName(),GetString(d_n));
			return;
		}
		d_obj = (t_text *)d_o->object;
        d_cnv = d_o->glist;
	}
	else if(d_x < 0 && d_x >= s_outlets+m_outlets) {
		post("%s - connect: outlet %i out of range (0..%i)",thisName(),d_x,s_outlets+m_outlets-1);
		return;
	}
	else  {
        d_obj = d_x < s_outlets?&pxsout[d_x]->obj:&pxmout[d_x-s_outlets]->obj;
        d_cnv = canvas;
		d_x = 0; // always 0 for proxy
	}

    if(s_cnv != d_cnv) {
        post("%s - connect: objects \"%s\" and \"%s\" are not on same canvas",thisName(),GetString(s_n),GetString(d_n));
        return;
    }

#ifndef NO_VIS
	int s_oix = canvas_getindex(s_cnv,&s_obj->te_g);
	int d_oix = canvas_getindex(d_cnv,&d_obj->te_g);
#endif

    if(conn) {
		if(!canvas_isconnected(s_cnv,(t_text *)s_obj,s_x,(t_text *)d_obj,d_x)) {
#ifdef NO_VIS
			if(!obj_connect(s_obj, s_x, d_obj, d_x))
				post("%s - connect: connection could not be made",thisName());
#else
			canvas_connect(s_cnv,s_oix,s_x,d_oix,d_x);
#endif
		}
	}
	else {
#ifdef NO_VIS
		obj_disconnect(s_obj, s_x, d_obj, d_x);
#else
		canvas_disconnect(s_cnv,s_oix,s_x,d_oix,d_x);
#endif
	}
}


bool dyn::CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv)
{
	if(n == 0) 
		// messages into inlet 0 are for dyn~
		return flext_base::m_method_(n,s,argc,argv);
	else {
		// all other messages are forwarded to proxies (and connected objects)
        if(n >= s_inlets)
    		pxmin[n-s_inlets-1]->Message(s,argc,argv);
        else
            post("%s - message to signal inlet",thisName());
		return true;
	}
}


void dyn::m_send(int argc,const t_atom *argv)
{
	if(argc < 2 || !IsSymbol(argv[0])) 
		post("%s - Syntax: send name message [args]",thisName());
	else {
		Obj *o = Find(GetSymbol(argv[0]));
		if(!o)
			post("%s - send: object \"%s\" not found",thisName(),GetString(argv[0]));
		else if(!canvasmsg && o->AsGlist())
			post("%s - send: object \"%s\" is an abstraction, please create proxy",thisName(),GetString(argv[0]));
        else if(IsSymbol(argv[1]))
            // has a tag symbol
            pd_typedmess((t_pd *)o->object,(t_symbol *)GetSymbol(argv[1]),argc-2,(t_atom *)argv+2);
        else
            // assume it's a list
			pd_forwardmess((t_pd *)o->object,argc-1,(t_atom *)argv+1);
	}
}

void dyn::sigproxyin::dsp(sigproxyin *x,t_signal **sp)
{
    FLEXT_ASSERT(x->vec && x->n);
	int n = sp[0]->s_n;
	if(n != x->n) {
        post("dyn~ proxyin - blocksize doesn't match!");
	}
    else
	    dsp_add_copy(x->vec,sp[0]->s_vec,n);
}

void dyn::sigproxyout::dsp(sigproxyout *x,t_signal **sp)
{
    FLEXT_ASSERT(x->vec && x->n);
	int n = sp[0]->s_n;
	if(n != x->n) {
        post("dyn~ proxyout - blocksize doesn't match!");
	}
    else
	    dsp_add_copy(sp[0]->s_vec,x->vec,n);
}

bool dyn::CbDsp()
{
    int n = Blocksize();
    t_sample *const *in = InSig(),*const *out = OutSig();
	int i;
	for(i = 0; i < s_inlets; ++i) pxsin[i]->set(n,in[i+1]);
	for(i = 0; i < s_outlets; ++i) pxsout[i]->set(n,out[i]);

    // add sub canvas to dsp list (no signal vector to borrow from .. set it to NULL)
    mess1((t_pd *)canvas,const_cast<t_symbol *>(sym_dsp),NULL);
    return false;
}
