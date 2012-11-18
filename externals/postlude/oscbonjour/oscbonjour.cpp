/*
    This file is part of Oscbonjour. 
	Copyright (c) 2005 Rémy Muller. 
	
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "m_pd.h"

#include "OscUdpZeroConfBrowser.h"
#include "OscUdpZeroConfResolver.h"
#include "OscUdpZeroConfService.h"

//#include "ZeroConf.h"

#include <vector>
#include <string>
#include <algorithm>

#define VERSION "0.1 alpha"

class Oscbonjour;


//------------------------------------------------------------------------------------------------------------
//    class
//------------------------------------------------------------------------------------------------------------
typedef struct oscbonjour 
{
	t_object x_obj;
    Oscbonjour *oscbonjour;
    t_outlet *out0,*out1,*out2;
} t_oscbonjour;

t_class *oscbonjour_class;
//static t_messlist *oscbonjour_class = NULL;


//------------------------------------------------------------------------------------------------------------
class Oscbonjour   :   public  OSCBrowseListener
                   ,   public  OSCResolveListener
                   ,   public  OSCRegisterListener
{
    t_oscbonjour *external;

    //ZeroConf zeroconf;
    OscUdpZeroConfBrowser   *browser;
    OscUdpZeroConfResolver  *resolver;
    OscUdpZeroConfService   *service;
    std::vector<std::string>     services;
    typedef std::vector<std::string>::iterator veciterator;

public:
    Oscbonjour(t_oscbonjour *external)
        :external(external)
        ,browser(0)
        ,resolver(0)
        ,service(0)
        
    {
    }
    virtual ~Oscbonjour()
    {
        if(browser) delete browser;
        if(resolver)delete resolver;
        if(service) delete service;
    }
    void Browse(const char *type, const char *domain)
    {
        services.clear();
        if(browser) delete browser;
        browser = 0;
        browser = new OscUdpZeroConfBrowser(this);
//        zeroconf.Browse(type,domain,this);
    }
    void Resolve(const char *name,const char *type,const char *domain)
    {
        if(resolver) delete resolver;
        resolver = 0;
        resolver = new OscUdpZeroConfResolver(name,type,domain,this);
//        zeroconf.Resolve(name,type,domain,this);
    }
    void Register(const char *name,t_int port)
    {
        if(service) delete service;
        service = 0;
        service = new OscUdpZeroConfService(name,port,this);
//        zeroconf.Register(name,"_osc._udp","local",port,this);
    }
    virtual void OnAddService(const char *name,const char *type,const char *domain)
    {
        std::string servname(name);
        veciterator it = std::find(services.begin(),services.end(),servname);
        if(it!=services.end()) return; // we already have it
        services.push_back(servname);

        t_atom at[1];
        SETSYMBOL(at,gensym(const_cast<char*>(servname.c_str())));

        if(external)
            outlet_anything(external->out2,gensym("append"),1,at);
    }
    virtual void OnRemoveService(const char *name,const char *type,const char *domain)
    {
        std::string servname(name);
        veciterator it = std::find(services.begin(),services.end(),servname);
        if(it==services.end()) return;      // we don't have it
        t_int index = it-services.begin();   // store the position
        services.erase(it);

        t_atom at[1];
        SETFLOAT(at,index);

        if(external)
            outlet_anything(external->out2,gensym("delete"),1,at);
    }
    virtual void OnResolveService(const char *fullName,const char *hostTarget,int port,const char *txtRecord)
    {
        if(external)
        {
            t_atom at[1];

            SETSYMBOL(at,gensym(const_cast<char*>(hostTarget)));
            outlet_anything(external->out0,gensym("host"),1,at);

            SETFLOAT(at,port);
            outlet_anything(external->out0,gensym("port"),1,at);
        }
    }
	virtual void OnRegisterService(const char *name)
	{
        t_atom at[1];
        SETSYMBOL(at,gensym(const_cast<char*>(name)));
		
        if(external)
            outlet_anything(external->out1,gensym("realname"),1,at);
	}

};

//------------------------------------------------------------------------------------------------------------
static void oscbonjour_version(t_oscbonjour *x, t_symbol *s, short ac, t_atom *at)
{
	post("oscbonjour (mDNS for Pure Data) version %s", VERSION);
}
//------------------------------------------------------------------------------------------------------------
//  user t_methods
//------------------------------------------------------------------------------------------------------------
static void oscbonjour_register(t_oscbonjour *x, t_symbol *s, short ac, t_atom *at)
{
    if(ac<2) return;
    if(at[0].a_type != A_SYMBOL || at[1].a_type != A_FLOAT) return;

    t_int port    =   (t_int)at[1].a_w.w_float; 
    x->oscbonjour->Register(at[0].a_w.w_symbol->s_name,port);
}
//------------------------------------------------------------------------------------------------------------
static void oscbonjour_browse(t_oscbonjour *x, t_symbol *s, short ac, t_atom *at)
{
    outlet_anything(x->out2,gensym("clear"),0,NULL);

    const char *type      =   "_osc._udp";
    const char *domain    =   "local";

    if(ac>0)
    {
        // 1st arg is type
        // 2nd is domain
    }
    x->oscbonjour->Browse(type,domain);
}
//------------------------------------------------------------------------------------------------------------
static void oscbonjour_resolve(t_oscbonjour *x, t_symbol *s, short ac, t_atom *at)
{
    if(ac<1) return;
    if(at[0].a_type != A_SYMBOL) return;

    const char *name      =   at[0].a_w.w_symbol->s_name;
    const char *type      =   "_osc._udp";
    const char *domain    =   "local";
    
    if(ac>1)
    {
        // 2nd argument is type
        // 3rd argument is domain
    }

    x->oscbonjour->Resolve(name,type,domain); 
}
//------------------------------------------------------------------------------------------------------------
static void oscbonjour_assist(t_oscbonjour *x, void *b, t_int msg, t_int a, char *dst)
{
	if (msg == 1) //inlet
    {
        sprintf(dst,"messages: browse, resolve, register");
	} 
    else if (msg == 2)  //outlet
    {
        switch(a)
        {
        case 0: sprintf(dst,"resolved host and port for service name");     break;
        case 1: sprintf(dst,"real registered service name");                break;
        case 2: sprintf(dst,"available services, connect to menu");             break;
        default: break;
        }
	}
}
//------------------------------------------------------------------------------------------------------------
static void *oscbonjour_new(t_symbol *s, t_int ac, t_atom *at)
{
	t_oscbonjour *x = (t_oscbonjour *)pd_new(oscbonjour_class);

    x->oscbonjour = new Oscbonjour(x);
    x->out2 = outlet_new(&x->x_obj,0L);
    x->out1 = outlet_new(&x->x_obj,0L);
    x->out0 = outlet_new(&x->x_obj,0L);

    if(ac>0)
    {
        if(at[0].a_type != A_SYMBOL) 
        {
            post("bad arguments");
            return x;
        }
        t_symbol *sym = at[0].a_w.w_symbol;
        std::string cmd(sym->s_name);

        at++;
        ac--;

        if      (cmd == "register") oscbonjour_register (x,sym,ac,at);
        else if (cmd == "browse")   oscbonjour_browse   (x,sym,ac,at);
        else if (cmd == "resolve")  oscbonjour_resolve  (x,sym,ac,at);
    }

	return x;
}
//------------------------------------------------------------------------------------------------------------
static void oscbonjour_free(t_oscbonjour *x)
{
    if(x->oscbonjour) delete x->oscbonjour;
}

//------------------------------------------------------------------------------------------------------------
// Entry Point
//------------------------------------------------------------------------------------------------------------
extern "C" void oscbonjour_setup(void)
{
    oscbonjour_class = class_new(gensym("oscbonjour"), (t_newmethod)oscbonjour_new, (t_method)oscbonjour_free, (short)sizeof(t_oscbonjour), 0L, A_GIMME, 0);

 class_addmethod(oscbonjour_class, (t_method)oscbonjour_register, gensym("register"), A_GIMME, 0);
 class_addmethod(oscbonjour_class, (t_method)oscbonjour_browse, gensym("browse"), A_GIMME, 0);
 class_addmethod(oscbonjour_class, (t_method)oscbonjour_resolve, gensym("resolve"), A_GIMME, 0);
 class_addmethod(oscbonjour_class, (t_method)oscbonjour_assist, gensym("assist"), A_CANT, 0);

    oscbonjour_version(0, 0, 0, 0);
    
}



