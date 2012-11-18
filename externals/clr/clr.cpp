// mono
extern "C" {
#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/class.h>
#include <mono/metadata/metadata.h>

// we need this one - it's in mono-devel
gpointer mono_delegate_to_ftnptr (MonoDelegate *delegate);
}

#ifdef _MSC_VER
#pragma warning(disable: 4091)
#endif
#include <m_pd.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h> // for _close
#define close _close
#else
#include <unistd.h>
#endif

#include <map>
#include <vector>
#include <list>


#define CALL __stdcall

// main class library
#define CORELIB "PureData"
#define DLLEXT "dll"

// symbol for inter-object messages
#define SYM_OBJECT "clr-object"

static t_symbol *sym_object;


struct PassedData
{
	MonoObject object;

	MonoObject *klass;	
	MonoObject *ext;
	MonoObject *obj;
};

struct AtomList
{
	AtomList(int c,t_atom *v): argc(c),argv(v) {}
	
	int argc;
    t_atom *argv;
};


// cached mono data
static MonoDomain *monodomain;
static PassedData *clr_pass;

// transforms a pointer (like MonoObject *) into a atom list
template <class T>
struct ObjectAtom
{
    // 64-bit safe...
    enum { bitshift = 22,size = sizeof(T)*8/bitshift+1 };

    t_atom msg[size];

    ObjectAtom() {}

    ObjectAtom(T o)
    {
        long ptr = (long)o;
        for(int i = 0; i < size; ++i) {
            SETFLOAT(msg+i,(int)(ptr&((1<<bitshift)-1)));
            ptr >>= bitshift;
        }
    }

    static bool check(int argc,const t_atom *argv)
    {
        if(argc != size) return false;
        for(int i = 0; i < size; ++i)
            if(argv[i].a_type != A_FLOAT) return false;
        return true;
    }

    static T ptr(const t_atom *argv)
    {
        long ret = 0;
        for(int i = size-1; i >= 0; --i)
            ret = (ret<<bitshift)+(int)argv[i].a_w.w_float;
        return (T)ret;
    }

    operator t_atom *() { return msg; }
};


enum Kind { k_bang, k_float, k_symbol, k_pointer, k_list, k_anything, k_object };


// this is the class structure
// holding the pd and mono class
struct t_clr_class
{
    t_class *pd_class;
    MonoObject *mono_class;
};

static t_class *proxy_class;

struct t_instance;

struct t_proxy
{
    t_object pd_obj; // myself
    t_instance *parent; // parent object
    int inlet;
};

typedef std::map<t_symbol *,t_clr_class *> ClrMap;
// this holds the class name to class structure association
static ClrMap clr_map;

typedef std::vector<t_outlet *> OutletArr;
typedef std::list<t_proxy *> ProxyList;

// this is the class to be setup (while we are in the CLR static Main method)
static t_clr_class *clr_setup_class = NULL;

// inlet index... must start with 0 every time a new object is made
static int clr_inlet;


// this is the class instance object structure
struct t_instance
{
    t_object pd_obj; // myself
	t_clr_class *pd_class;
    MonoObject *mono_obj;  // the mono class instance
    guint32 mono_handle;  // the mono class instance

	OutletArr *outlets;
    ProxyList *proxies;
};


typedef void (__stdcall NewClass)(t_clr_class *c,const t_symbol *sym);
typedef void (__stdcall NewInstance)(t_instance *p,AtomList l);
typedef void (__stdcall CallBang)();
typedef void (__stdcall CallFloat)(float f);
typedef void (__stdcall CallSymbol)(const t_symbol *sym);
typedef void (__stdcall CallPointer)(const t_gpointer *ptr);
typedef void (__stdcall CallList)(AtomList l);
typedef void (__stdcall CallAnything)(int inlet,const t_symbol *sym,AtomList l);
typedef void (__stdcall CallObject)(int inlet);

static NewClass *newclass = NULL;
static NewInstance *newinstance = NULL;
static CallBang *callbang = NULL;
static CallFloat *callfloat = NULL;
static CallSymbol *callsymbol = NULL;
static CallPointer *callpointer = NULL;
static CallList *calllist = NULL;
static CallAnything *callanything = NULL;
static CallObject *callobject = NULL;

#if 0
// Print error message given by exception
static void error_exc(char* txt,char* cname,MonoObject* exc)
{
    MonoMethod* m = mono_method_desc_search_in_class(clr_desc_tostring,mono_get_exception_class());
    assert(m);
    m = mono_object_get_virtual_method(exc,m);
    assert(m);
    MonoString* str = (MonoString*)mono_runtime_invoke(m,exc,NULL,NULL);
    assert(str);
    error("CLR class %s: %s",txt,cname);
    error(mono_string_to_utf8(str));
}
#endif


static void clr_method_bang(t_instance *x) 
{
    assert(x);
	assert(callbang);
	clr_pass->ext = x->mono_obj;
	callbang();
}

static void clr_method_float(t_instance *x,t_float f) 
{
    assert(x);
	assert(callfloat);
	clr_pass->ext = x->mono_obj;
	callfloat(f);
}

static void clr_method_symbol(t_instance *x,t_symbol *s) 
{
    assert(x);
	assert(callsymbol);
	clr_pass->ext = x->mono_obj;
	callsymbol(s);
}

static void clr_method_pointer(t_instance *x,t_gpointer *p)
{
    assert(x);
	assert(callpointer);
	clr_pass->ext = x->mono_obj;
	callpointer(p);
}

static void clr_method_list(t_instance *x,t_symbol *,int argc,t_atom *argv)
{
    assert(x);
	assert(calllist);
	clr_pass->ext = x->mono_obj;
	calllist(AtomList(argc,argv));
}

static void call_anything(t_instance *x,int inlet,t_symbol *s,int argc,t_atom *argv)
{
    assert(x);
	clr_pass->ext = x->mono_obj;
	if(s == sym_object && ObjectAtom<guint32>::check(argc,argv)) {
		assert(callobject);
		guint32 hnd = ObjectAtom<guint32>::ptr(argv);
		clr_pass->obj = mono_gchandle_get_target(hnd);
		callobject(inlet);
	}
	else {
		assert(callanything);
		callanything(inlet,s,AtomList(argc,argv));
	}
}

static void clr_method_anything(t_instance *x,t_symbol *s,int argc,t_atom *argv) { call_anything(x,0,s,argc,argv); }
static void clr_method_proxy(t_proxy *x,t_symbol *s,int argc,t_atom *argv) { call_anything(x->parent,x->inlet,s,argc,argv); }

static void CALL PD_Post(MonoString *str)
{
	post("%s",mono_string_to_utf8(str));	
}

static void CALL PD_PostError(MonoString *str)
{
	error("%s",mono_string_to_utf8(str));	
}

static void CALL PD_PostVerbose(int lvl,MonoString *str)
{
	verbose(lvl,"%s",mono_string_to_utf8(str));	
}

static void *CALL PD_SymGen(MonoString *str)
{
    assert(str);
	return gensym(mono_string_to_utf8(str));	
}

static MonoString *CALL PD_SymEval(t_symbol *sym)
{
    assert(sym);
    return mono_string_new(monodomain,sym->s_name);
}


static void CALL PD_AddInletAlias(t_instance *obj,t_symbol *sel,t_symbol *to_sel)
{
    ++clr_inlet;
    assert(obj);
    t_inlet *in = inlet_new(&obj->pd_obj,&obj->pd_obj.ob_pd,sel,to_sel);
    assert(in);
}

static void CALL PD_AddInletFloat(t_instance *obj,float *f)
{
    ++clr_inlet;
    assert(obj);
    t_inlet *in = floatinlet_new(&obj->pd_obj,f);
    assert(in);
}

static void CALL PD_AddInletSymbol(t_instance *obj,t_symbol **s)
{
    ++clr_inlet;
    assert(obj);
    t_inlet *in = symbolinlet_new(&obj->pd_obj,s);
    assert(in);
}

/*
static void CALL PD_AddInletPointer(t_clr *obj,t_gpointer *p)
{
    ++clr_inlet;
    assert(obj);
    t_inlet *in = pointerinlet_new(&obj->pd_obj,p);
    assert(in);
}
*/

static void CALL PD_AddInletProxyTyped(t_instance *obj,t_symbol *type)
{
    assert(obj);
    t_proxy *p = (t_proxy *)pd_new(proxy_class);
    p->parent = obj;
    p->inlet = ++clr_inlet;
    if(!obj->proxies) obj->proxies = new ProxyList;
    obj->proxies->push_back(p);
    t_inlet *in = inlet_new(&obj->pd_obj,&p->pd_obj.ob_pd,type,type);
    assert(in);
}

static void CALL PD_AddInletProxy(t_instance *obj) { PD_AddInletProxyTyped(obj,NULL); }


static void CALL PD_AddOutlet(t_instance *obj,t_symbol *type)
{
    assert(obj);
    t_outlet *out = outlet_new(&obj->pd_obj,type);
    assert(out);
    if(!obj->outlets) obj->outlets = new OutletArr;
    obj->outlets->push_back(out);
}

static void CALL PD_OutletBang(t_instance *obj,int n)
{
    assert(obj);
    assert(obj->outlets);
    assert(n >= 0 && n < (int)obj->outlets->size());
    outlet_bang((*obj->outlets)[n]);
}

static void CALL PD_OutletFloat(t_instance *obj,int n,float f)
{
    assert(obj);
    assert(obj->outlets);
    assert(n >= 0 && n < (int)obj->outlets->size());
    outlet_float((*obj->outlets)[n],f);
}

static void CALL PD_OutletSymbol(t_instance *obj,int n,t_symbol *s)
{
    assert(obj);
    assert(obj->outlets);
    assert(n >= 0 && n < (int)obj->outlets->size());
    outlet_symbol((*obj->outlets)[n],s);
}

static void CALL PD_OutletPointer(t_instance *obj,int n,t_gpointer *p)
{
    assert(obj);
    assert(obj->outlets);
    assert(n >= 0 && n < (int)obj->outlets->size());
    outlet_pointer((*obj->outlets)[n],p);
}

static void CALL PD_OutletAtom(t_instance *obj,int n,t_atom l)
{
    assert(obj);
    assert(obj->outlets);
    assert(n >= 0 && n < (int)obj->outlets->size());
    t_outlet* out = (*obj->outlets)[n];
    switch(l.a_type) {
        case A_FLOAT: outlet_float(out,l.a_w.w_float); break;
        case A_SYMBOL: outlet_symbol(out,l.a_w.w_symbol); break;
        case A_POINTER: outlet_pointer(out,l.a_w.w_gpointer); break;
        default: assert(false);
    }
}

static void CALL PD_OutletAnything(t_instance *obj,int n,t_symbol *s,MonoArray *l)
{
    assert(obj);
    assert(obj->outlets);
    assert(n >= 0 && n < (int)obj->outlets->size());
//    assert(mono_object_get_class(&l->obj) == clr_atom);
    outlet_anything((*obj->outlets)[n],s,mono_array_length(l),mono_array_addr(l,t_atom,0));
}


static void CALL PD_OutletObject(t_instance *obj,int n,MonoObject *o)
{
    assert(obj);
    assert(obj->outlets);
    assert(n >= 0 && n < (int)obj->outlets->size());
	guint32 hnd = mono_gchandle_new(o,TRUE);
    ObjectAtom<guint32> oa(hnd);
    outlet_anything((*obj->outlets)[n],sym_object,oa.size,oa);
	mono_gchandle_free(hnd);
}


static void CALL PD_SendAtom(t_symbol *dst,t_atom a)
{
    void *cl = dst->s_thing;
    if(cl) pd_forwardmess((t_class **)cl,1,&a);
}

static void CALL PD_SendAnything(t_symbol *dst,t_symbol *s,MonoArray *l)
{
    void *cl = dst->s_thing;
//    assert(mono_object_get_class(&l->obj) == clr_atom);
    if(cl) pd_typedmess((t_class **)cl,s,mono_array_length(l),mono_array_addr(l,t_atom,0));
}

static void CALL PD_SendObject(t_symbol *dst,MonoObject *o)
{
    void *cl = dst->s_thing;
//    assert(mono_object_get_class(&l->obj) == clr_atom);
    if(cl) {
		guint32 hnd = mono_gchandle_new(o,TRUE);
        ObjectAtom<guint32> oa(hnd);
        pd_typedmess((t_class **)cl,sym_object,oa.size,oa);
		mono_gchandle_free(hnd);
    }
}

static MonoString *CALL PD_SearchPath(MonoString *file)
{
	char *filestr = mono_string_to_utf8(file);
    char dirbuf[MAXPDSTRING],*nameptr;
    // search for classname.dll in the PD path
    int fd;
	if ((fd = open_via_path("",filestr,"",dirbuf, &nameptr, MAXPDSTRING, 1)) >= 0) {
    // found
		if(dirbuf != nameptr) { 
			// fix for the fact that open_via_path doesn't return a path, when it's found in .
            strcat(dirbuf,"/");
			strcat(dirbuf,filestr);

//            close(fd);  // strange - we get an assertion failure here...
		}
		return mono_string_new(monodomain,dirbuf);
	}
	else
		return mono_string_new(monodomain,"");
}


void *clr_new(t_symbol *classname, int argc, t_atom *argv)
{
	// find class name in map
    ClrMap::iterator it = clr_map.find(classname);
    if(it == clr_map.end()) {
        error("CLR - class %s not found",classname->s_name);
        return NULL;
    }

    t_clr_class *clss = it->second;

    // make instance
    t_instance *x = (t_instance *)pd_new(clss->pd_class);
	x->pd_class = clss;
    x->outlets = NULL;
    x->proxies = NULL;

    clr_inlet = 0;

	assert(newinstance);

	clr_pass->klass = clss->mono_class;
	newinstance(x,AtomList(argc,argv));
	x->mono_obj = clr_pass->ext;
#ifdef _DEBUG
	clr_pass->klass = NULL;
#endif

	if(x->mono_obj) {
		x->mono_handle = mono_gchandle_new(x->mono_obj,TRUE);
		return x;
	}
	else {
        pd_free((t_pd *)x);
        error("CLR - class %s could not be instantiated",classname->s_name);
        return NULL;
	}
}

void clr_free(t_instance *obj)
{
	mono_gchandle_free(obj->mono_handle);

    if(obj->outlets) delete obj->outlets;

    if(obj->proxies) {
        for(ProxyList::iterator it = obj->proxies->begin(); it != obj->proxies->end(); ++it) pd_free((t_pd *)*it);
        delete obj->proxies;
    }
}

static void CALL PD_Register(MonoDelegate *d_class,MonoDelegate *d_new,MonoDelegate *d_bang,MonoDelegate *d_float,MonoDelegate *d_symbol,MonoDelegate *d_pointer,MonoDelegate *d_list,MonoDelegate *d_anything,MonoDelegate *d_object)
{
	newclass = (NewClass *)mono_delegate_to_ftnptr(d_class);
	newinstance = (NewInstance *)mono_delegate_to_ftnptr(d_new);
	callbang = (CallBang *)mono_delegate_to_ftnptr(d_bang);
	callfloat = (CallFloat *)mono_delegate_to_ftnptr(d_float);
	callsymbol = (CallSymbol *)mono_delegate_to_ftnptr(d_symbol);
	callpointer = (CallPointer *)mono_delegate_to_ftnptr(d_pointer);
	calllist = (CallList *)mono_delegate_to_ftnptr(d_list);
	callanything = (CallAnything *)mono_delegate_to_ftnptr(d_anything);
	callobject = (CallObject *)mono_delegate_to_ftnptr(d_object);
}

static bool CALL PD_RegisterClass(t_clr_class *c,t_symbol *classsym,int classflags,int methodflags)
{
	assert(c && !c->pd_class);
    c->pd_class = class_new(classsym,(t_newmethod)clr_new,(t_method)clr_free, sizeof(t_instance), classflags, A_GIMME, A_NULL);
	if(c->pd_class) {
		if(methodflags&0x01) class_addbang(c->pd_class,clr_method_bang);
		if(methodflags&0x02) class_addfloat(c->pd_class,clr_method_float);
		if(methodflags&0x04) class_addsymbol(c->pd_class,clr_method_symbol);
		if(methodflags&0x08) class_addpointer(c->pd_class,clr_method_pointer);
		if(methodflags&0x10) class_addlist(c->pd_class,clr_method_list);
		if(methodflags&0x20) class_addanything(c->pd_class,clr_method_anything);
		return true;
	}
	else
		return false;
}


static int classloader(char *dirname, char *classname, char *altname)
{
	if(!newclass) {
		post("CLR - Entry point not set");
		return 0;
	}

	t_symbol *classsym = gensym(classname);

    t_clr_class *classdef = (t_clr_class *)getbytes(sizeof(t_clr_class));
    // set all struct members to 0
    memset(classdef,0,sizeof(t_clr_class));

	newclass(classdef,classsym);
	classdef->mono_class = clr_pass->klass;

	if(!classdef->mono_class)
	{
		freebytes(classdef,sizeof(t_clr_class));
		return 0;
	}

	mono_gchandle_new(classdef->mono_class,TRUE); // we don't remember the handle... won't be freed later

    // put into map
    clr_map[classsym] = classdef;

    verbose(1,"CLR - Loaded class %s OK",classname);

    return 1;
}

extern "C"
#ifdef _MSC_VER
__declspec(dllexport) 
#endif
void clr_setup(void)
{
#ifdef _WIN32
    // set mono paths
    const char *monopath = getenv("MONO_PATH");
    if(!monopath) {
        error("CLR - Please set the MONO_PATH environment variable to the folder of your MONO installation - CLR not loaded!");
        return;
    }
    
    char tlib[256],tconf[256];
    strcpy(tlib,monopath);
    strcat(tlib,"/lib");
    strcpy(tconf,monopath);
    strcat(tconf,"/etc");
    mono_set_dirs(tlib,tconf);
#endif

    // try to find PureData.dll in the PD path
    char dirbuf[MAXPDSTRING],*nameptr;
    // search in the PD path
    int fd;
    if ((fd = open_via_path("",CORELIB,"." DLLEXT,dirbuf,&nameptr,MAXPDSTRING,1)) >= 0) {
        if(dirbuf != nameptr) // fix for the fact that open_via_path doesn't return a path, when it's found in .
            strcat(dirbuf,"/" CORELIB "." DLLEXT);
//            close(fd);  // strange - we get an assertion failure here...
    }
	else {
        error("CLR - " CORELIB "." DLLEXT " not found in path");
		return;
	}

    try { 
        monodomain = mono_jit_init(dirbuf); 
    }
    catch(...) {
        monodomain = NULL;
    }


	if(monodomain) {
        // look for PureData.dll
        MonoAssembly *assembly = mono_domain_assembly_open (monodomain,dirbuf);
	    if(!assembly) {
		    error("CLR - assembly %s not found!",dirbuf);
		    return;
	    }

	    MonoImage *image = mono_assembly_get_image(assembly);
        assert(image);

	    // add mono to C hooks

        mono_add_internal_call("PureData.Internal::SymGen(string)", (const void *)PD_SymGen);
        mono_add_internal_call("PureData.Internal::SymEval(PureData.Symbol)", (const void *)PD_SymEval);

        mono_add_internal_call("PureData.Public::Post(string)",(const void *)PD_Post);
        mono_add_internal_call("PureData.Public::PostError(string)",(const void *)PD_PostError);
        mono_add_internal_call("PureData.Public::PostVerbose(int,string)",(const void *)PD_PostVerbose);
        mono_add_internal_call("PureData.Public::SearchPath(string)",(const void *)PD_SearchPath);

        mono_add_internal_call("PureData.Internal::AddInlet(PureData.ExternalPtr,PureData.Symbol,PureData.Symbol)", (const void *)PD_AddInletAlias);
        mono_add_internal_call("PureData.Internal::AddInlet(PureData.ExternalPtr,single&)", (const void *)PD_AddInletFloat);
        mono_add_internal_call("PureData.Internal::AddInlet(PureData.ExternalPtr,PureData.Symbol&)", (const void *)PD_AddInletSymbol);
//        mono_add_internal_call("PureData.Internal::AddInlet(PureData.ExternalPtr,PureData.Pointer&)", (const void *)PD_AddInletPointer);
//        mono_add_internal_call("PureData.Internal::AddInlet(PureData.ExternalPtr,PureData.Symbol)", (const void *)PD_AddInletTyped);
        mono_add_internal_call("PureData.Internal::AddInlet(PureData.ExternalPtr)", (const void *)PD_AddInletProxy);

        mono_add_internal_call("PureData.Internal::AddOutlet(PureData.ExternalPtr,PureData.Symbol)", (const void *)PD_AddOutlet);

        mono_add_internal_call("PureData.Internal::Outlet(PureData.ExternalPtr,int)", (const void *)PD_OutletBang);
        mono_add_internal_call("PureData.Internal::Outlet(PureData.ExternalPtr,int,single)", (const void *)PD_OutletFloat);
        mono_add_internal_call("PureData.Internal::Outlet(PureData.ExternalPtr,int,PureData.Symbol)", (const void *)PD_OutletSymbol);
        mono_add_internal_call("PureData.Internal::Outlet(PureData.ExternalPtr,int,PureData.Pointer)", (const void *)PD_OutletPointer);
        mono_add_internal_call("PureData.Internal::Outlet(PureData.ExternalPtr,int,PureData.Atom)", (const void *)PD_OutletAtom);
        mono_add_internal_call("PureData.Internal::Outlet(PureData.ExternalPtr,int,PureData.Symbol,PureData.Atom[])", (const void *)PD_OutletAnything);
        mono_add_internal_call("PureData.Internal::OutletEx(PureData.ExternalPtr,int,object)", (const void *)PD_OutletObject);

//        mono_add_internal_call("PureData.Internal::Bind(PureData.ExternalPtr,PureData.Symbol)", (const void *)PD_Bind);
//        mono_add_internal_call("PureData.Internal::Unbind(PureData.ExternalPtr,PureData.Symbol)", (const void *)PD_Unbind);

        mono_add_internal_call("PureData.Class::Register(PureData.Class/DelegateClass,PureData.Class/DelegateNew,PureData.Class/DelegateBang,PureData.Class/DelegateFloat,PureData.Class/DelegateSymbol,PureData.Class/DelegatePointer,PureData.Class/DelegateList,PureData.Class/DelegateAnything,PureData.Class/DelegateObject)", (const void *)PD_Register);
        mono_add_internal_call("PureData.Class::RegisterClass(PureData.ClassPtr,PureData.Symbol,PureData.Public/ClassType,PureData.Class/MethodFlags)", (const void *)PD_RegisterClass);
//        mono_add_internal_call("PureData.Class::AddMethodIntern(PureData.ClassPtr,int,PureData.Symbol,PureData.Class/Kind,System.Delegate)", (const void *)PD_AddMethodIntern);

        mono_add_internal_call("PureData.External::Send(PureData.Symbol,PureData.Atom)", (const void *)PD_SendAtom);
        mono_add_internal_call("PureData.External::Send(PureData.Symbol,PureData.Symbol,PureData.Atom[])", (const void *)PD_SendAnything);
        mono_add_internal_call("PureData.External::SendEx(PureData.Symbol,object)", (const void *)PD_SendObject);


		///////////////////////////////////////////////////////////

        // make proxy class
        proxy_class = class_new(gensym("clr proxy"),NULL,NULL,sizeof(t_proxy),CLASS_PD|CLASS_NOINLET,A_NULL);
        class_addanything(proxy_class,clr_method_proxy);

        // symbol for Mono object handling
        sym_object = gensym(SYM_OBJECT);

        // install loader hook
        sys_register_loader(classloader);

		///////////////////////////////////////////////////////////

		// call static Class::Setup function
        MonoClass *clr_class = mono_class_from_name(image,"PureData","Class");
        assert(clr_class);
        MonoMethodDesc *desc = mono_method_desc_new("::Setup()",FALSE);
        MonoMethod *setup = mono_method_desc_search_in_class(desc,clr_class);

        MonoObject *exc;
        mono_runtime_invoke(setup,NULL,NULL,&exc);
        if(exc)
            post("CLR - setup function failed");
        else {
            // ready!
	        post("CLR extension - (c)2006 Davide Morelli, Thomas Grill");
        }

		// necessary data should have been set by Setup
		MonoClass *clr_internal = mono_class_from_name(image,"PureData","Internal");
        assert(clr_internal);
		MonoVTable *clr_internal_vt = mono_class_vtable(monodomain,clr_internal);
		MonoClassField *fld = mono_class_get_field_from_name (clr_internal,"pass");
		assert(fld);
		mono_field_static_get_value(clr_internal_vt,fld,&clr_pass);
		assert(clr_pass);
    }
    else
        error("clr: mono domain couldn't be initialized!");
}
