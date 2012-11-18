/* 
dyn_py - Python interface to the dyn library

Copyright (c)2004-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#define DYNPY_VERSION_MAJOR 0
#define DYNPY_VERSION_MINOR 2

#define DYNPY_VERSION (DYNPY_VERSION_MAJOR*100+DYNPY_VERSION_MINOR)


#ifdef _MSC_VER
#pragma warning(disable:4267)
#endif

#include <CXX/Objects.hxx>
#include <CXX/Extensions.hxx>

#include <dyn.h>
#include <pysymbol.h>

using namespace Py;


static Object dynpatcher(const Tuple &a,dyn_id ident = DYN_ID_NONE);
static Object dynobject(const Tuple &a,dyn_id ident = DYN_ID_NONE);
static Object dynmessage(const Tuple &a,dyn_id ident = DYN_ID_NONE); 


/*! Convert a Python tuple to PD atoms
    \note we don't need to check for $-arguments here... symbols will do
*/
static int tuple2atoms(const Tuple &s,int offs,const t_symbol *&hdr,t_atom *lst,int sz,bool listonly = false)
{
    hdr = listonly?flext::sym_list:NULL;
    int i = 0,len = s.length();
    for(int it = offs; it < len && i < sz; ++it) {
        const Object &o = s[it];
        if(o.isNumeric()) { 
            if(!hdr) hdr = flext::sym_list;
            float f = (float)Float(o);
            int fi = (int)f;
            if(fi == f)
                flext::SetInt(lst[i++],fi);
            else
                flext::SetFloat(lst[i++],f);
        }
        else if(pySymbol_Check(o.ptr())) {
            const t_symbol *s = pySymbol_AS_SYMBOL(o.ptr());
            if(!hdr) hdr = s;
            else flext::SetSymbol(lst[i++],s);
        }
        else if(o.isString()) { 
            const t_symbol *s = flext::MakeSymbol(o.as_string().c_str());
            if(!hdr) hdr = s;
            else flext::SetSymbol(lst[i++],s);
        }
        else {
            hdr = NULL;
            return 0;
        }
    }

    if(hdr == flext::sym_list && i == 1) { 
        if(flext::IsFloat(*lst)) hdr = flext::sym_float;
        else if(flext::IsInt(*lst)) hdr = flext::sym_int;
        else if(flext::IsSymbol(*lst)) hdr = flext::sym_symbol;
    }

    return i;
}

static Tuple atoms2tuple(const t_symbol *sym,int argc,const t_atom *argv)
{
    bool hd = sym != flext::sym_list && sym != flext::sym_float;
    Tuple args(argc+hd?1:0);
    int o = 0;
    if(hd) args[o++] = String(flext::GetString(sym));
    for(int i = 0; i < argc; ++i,++o) {
        if(flext::IsFloat(argv[i]))
            args[o] = Float(flext::GetFloat(argv[i]));
        else if(flext::IsInt(argv[i]))
            args[o] = Int(flext::GetInt(argv[i]));
        else if(flext::IsSymbol(argv[i]))
            args[o] = asObject(pySymbol_FromSymbol(flext::GetSymbol(argv[i])));
        else
            args[o] = Nothing();
    }
    return args;
}

typedef std::map<dyn_id,Callable *> ListenMap;
// all listeners
static ListenMap listen;


static void callback(dyn_id ident,int signal,void *data) 
{
    if(dyn_GetType(ident) == DYN_TYPE_LISTENER && signal == DYN_SIGNAL_FREE) {
        // clear listener data
        ListenMap::iterator it = listen.find(ident);
        if(it != listen.end()) {
            delete it->second;
            listen.erase(it);
        }
    }

#ifdef FLEXT_DEBUG
//    flext::post("dyn callback - signal %i, object %x",signal,ident);
#endif
}


class dynIdent
    : public PythonExtension<dynIdent>
{
public:
    dyn_id ident;

    dynIdent(dyn_id id = DYN_ID_NONE): ident(id) {}

    static void init_type()
	{
    	behaviors().name("ID");
        behaviors().doc("documentation for dyn::Ident" );

	    add_varargs_method("Patcher", &dynIdent::f_patcher, "Create a new sub patcher");
	    add_varargs_method("Object", &dynIdent::f_object, "Create a new dyn object");
	    add_varargs_method("Message", &dynIdent::f_message, "Create a new dyn message object");

	    add_varargs_method("Free", &dynIdent::f_free, "Free the dyn object");

	    add_varargs_method("Inlets", &dynIdent::f_inlets, "Get inlet count of a dyn object");
	    add_varargs_method("Outlets", &dynIdent::f_outlets, "Get outlet count of a dyn object");

	    add_varargs_method("InletType", &dynIdent::f_inlettype, "Get type of an inlet");
	    add_varargs_method("OutletType", &dynIdent::f_outlettype, "Get type of an outlet");

	    add_varargs_method("Connect", &dynIdent::f_connect, "Create a connection between two dyn objects");

	    add_varargs_method("Send", &dynIdent::f_send, "Send a message to a dyn object");
	    add_varargs_method("Listen", &dynIdent::f_listen, "Create a listener to messages of a dyn object");
    }

private:

    void Check() const
    { 
        if(ident == DYN_ID_NONE) throw RuntimeError("Dyn object not initialized"); 
    }    

    Object f_patcher(const Tuple &a) { Check(); return dynpatcher(a,ident); }
	Object f_object(const Tuple &a) { Check(); return dynobject(a,ident); } 
	Object f_message(const Tuple &a) { Check(); return dynmessage(a,ident); }

    Object f_free(const Tuple &a) 
    { 
        // don't Check() object to free...
        if(ident != DYN_ID_NONE) {
            if(dyn_Free(DYN_SCHED_AUTO,ident) != DYN_ID_NONE)
                throw RuntimeError("Could not free dyn object");

            ident = DYN_ID_NONE;
        }
        return Nothing();
    }

    Object f_inlets(const Tuple &a) 
    { 
        Check();
        if(a.length()) throw RuntimeError("No arguments allowed");
        int ret = dyn_GetInletCount(ident);
        return ret >= 0?Int(ret):Nothing();
    }

    Object f_outlets(const Tuple &a) 
    { 
        Check();
        if(a.length()) throw RuntimeError("No arguments allowed");
        int ret = dyn_GetOutletCount(ident);
        return ret >= 0?Int(ret):Nothing();
    }

    Object f_inlettype(const Tuple &a) 
    { 
        Check();
        int ret = dyn_GetInletType(ident,Int(a[0]));
        if(ret >= 0)
            return Int(ret == DYN_INOUT_SIGNAL);
        else
            throw RuntimeError("Error querying inlet type");
    }

    Object f_outlettype(const Tuple &a) 
    { 
        Check();
        int ret = dyn_GetOutletType(ident,Int(a[0]));
        if(ret >= 0)
            return Int(ret == DYN_INOUT_SIGNAL);
        else
            throw RuntimeError("Error querying outlet type");
    }

	Object f_connect(const Tuple &a) 
    {
        if(a.length() == 3) {
            dyn_id obj;
            ExtensionObject<dynIdent> dst(a[1]);
            if(dyn_NewConnection(DYN_SCHED_AUTO,&obj,callback,ident,Int(a[0]),dst.extensionObject()->ident,Int(a[2])) == DYN_ID_NONE)
                return asObject(new dynIdent(obj));
            else
                throw RuntimeError("Error connecting dyn objects");
        }
        else
            throw RuntimeError("Syntax: Connect(src-outlet,dst-ID,dst-inlet)");
    }

    Object f_send(const Tuple &a) 
    {
        Check();

        int err = DYN_ERROR_GENERAL;
        if(a.length() == 2 && a[1].isString()) {
            std::string msg(a[1].as_string());
            err = dyn_SendStr(DYN_SCHED_AUTO,ident,Int(a[0]),msg.c_str());
        }
        else if(a.length() >= 1) {
            t_atom lst[256],*argv = lst;
            t_symbol *sym;
            int argc = tuple2atoms(a,1,sym,lst,256);
            if(argc || sym)
                err = dyn_Send(DYN_SCHED_AUTO,ident,Int(a[0]),sym,argc,argv);
        }
        
        if(err == DYN_ERROR_NONE)
            return Nothing();
        else
            throw RuntimeError("Could not send to dyn object");
    }

    Object f_listen(const Tuple &a) 
    {
        Check();
        if(a.length() == 2) {
            dyn_id obj;
            Callable *func = new Callable(a[1]);
            if(dyn_Listen(DYN_SCHED_AUTO,&obj,ident,Int(a[0]),listener,func) == DYN_ID_NONE) {
                listen[obj] = func;
                return asObject(new dynIdent(obj));
            }
            else {
                delete func;
                throw RuntimeError("Error creating dyn listener");
            }
        }
        else
            throw RuntimeError("Syntax: Listen(inlet,function)");
    }

    static void listener(dyn_id id,dyn_id oid,int outlet,const t_symbol *sym,int argc,const t_atom *argv,void *data)
    {
    #ifdef FLEXT_DEBUG
//        flext::post("dyn listener - object %x, outlet %i, sym %s",oid,outlet,sym->s_name);
    #endif

        Callable *func = (Callable *)data;

        // convert PD atoms to Python tuple
        Tuple args(atoms2tuple(sym,argc,argv));

        Tuple tpl(3);
        tpl[0] = asObject(new dynIdent(oid));
        tpl[1] = Int(outlet);
        tpl[2] = args.length() == 1?args[0]:args;

        try {
            func->apply(tpl); // call method
        }
        catch(Exception e) {
            PyErr_Print();
            e.clear();
        }
        catch(...) {
            PyErr_Clear();
        }
    }
};


class dynModule 
    : public flext
    , public ExtensionModule<dynModule>
{
public:
	dynModule()
		: ExtensionModule<dynModule>("dyn")
	{
        flext::Setup();

		dynIdent::init_type();

        add_varargs_method("Patcher", &dynModule::f_patcher, "Create a new dyn patcher");
	    add_varargs_method("Object", &dynModule::f_object, "Create a new dyn object");
	    add_varargs_method("Message", &dynModule::f_message, "Create a new dyn message object");
	    add_varargs_method("Version", &dynModule::f_version, "Return version number");
	    add_varargs_method("DynVersion", &dynModule::f_dynversion, "Return dyn version number");

        // must be last
        initialize( "documentation for the dyn module" );
    }

protected:

	Object f_dynversion(const Tuple &a) 
    {
        return Int(DYN_VERSION);
    }

	Object f_version(const Tuple &a) 
    {
        return Int(DYNPY_VERSION);
    }

    Object f_patcher(const Tuple &a) { return dynpatcher(a); }
    Object f_object(const Tuple &a) { return dynobject(a); }
    Object f_message(const Tuple &a) { return dynmessage(a); }
};

extern "C" 
#ifdef _MSC_VER
_declspec(dllexport)
#endif
void initdyn()
{
#if defined(PY_WIN32_DELAYLOAD_PYTHON_DLL)
	Py::InitialisePythonIndirectInterface();
#endif
	static dynModule* module = new dynModule;
}

static Object dynpatcher(const Tuple &a,dyn_id ident) 
{
    if(!a.length()) {
        dyn_id obj;
        if(dyn_NewPatcher(DYN_SCHED_AUTO,&obj,callback,ident) == DYN_ERROR_NONE)
            return asObject(new dynIdent(obj));
        else
            throw RuntimeError("Error creating dyn patcher");
    }
    else
        throw RuntimeError("Syntax: Patcher()");            
}

static Object dynobject(const Tuple &a,dyn_id ident) 
{
    dyn_id obj;

    int err = DYN_ERROR_GENERAL;
    if(a.length() == 1 && a[0].isString())
        err = dyn_NewObjectStr(DYN_SCHED_AUTO,&obj,callback,ident,a[0].as_string().c_str());
    else {
        t_atom lst[256],*argv = lst;
        t_symbol *sym;
        int argc = tuple2atoms(a,0,sym,lst,256);
        if(argc || sym)
            err = dyn_NewObject(DYN_SCHED_AUTO,&obj,callback,ident,sym,argc,argv);
        else
            throw RuntimeError("Invalid arguments for dyn object");
    }
    
    if(err == DYN_ERROR_NONE)
        return asObject(new dynIdent(obj));
    else
        throw RuntimeError("Error creating dyn object");            
}

static Object dynmessage(const Tuple &a,dyn_id ident) 
{
    dyn_id obj;

    int err = DYN_ERROR_GENERAL;
    if(a.length() == 1 && a[0].isString())
        err = dyn_NewMessageStr(DYN_SCHED_AUTO,&obj,callback,ident,a[0].as_string().c_str());
    else {
        t_atom argv[256];
        t_symbol *hdr;
        int argc = tuple2atoms(a,0,hdr,argv,256,true);
        if(argc)
            err = dyn_NewMessage(DYN_SCHED_AUTO,&obj,callback,ident,argc,argv);
        else
            throw RuntimeError("Invalid arguments for dyn message");
    }

    if(err == DYN_ERROR_NONE)
        return asObject(new dynIdent(obj));
    else
        throw RuntimeError("Error creating dyn message");
}
