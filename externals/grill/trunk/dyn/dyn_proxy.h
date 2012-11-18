/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#ifndef __DYN_PROXY_H
#define __DYN_PROXY_H

#include "dyn.h"
#include "dyn_pd.h"

#include <set>

class dyn_patchable;
class dyn_listen;

// attention... no virtual table allowed!
class proxy
{ 
public:
	t_object pdobj; // must be first
    dyn_patchable *object;
	int n;
	t_sample *buf;
	t_sample defsig;

	void init(dyn_patchable *o) { object = o,n = 0,buf = NULL,defsig = 0; }

    static void px_exit(proxy *px);
};

// proxy for inbound messages
class proxyin:
	public proxy
{ 
public:
	t_outlet *outlet;

	void Message(const t_symbol *s,int argc,const t_atom *argv) 
	{
		typedmess((t_pd *)&pdobj,(t_symbol *)s,argc,(t_atom *)argv);
	}

	void init(dyn_patchable *obj,bool s = false);
    void exit() { outlet = NULL; }

	static void px_method(proxyin *obj,const t_symbol *s,int argc,const t_atom *argv);
    static void dsp(proxyin *x, t_signal **sp);
};

typedef std::set<dyn_listen *> Listeners;

// proxy for outbound messages
class proxyout:
	public proxy
{ 
public:
	int outlet;
    Listeners *listeners; // this is initialized in init

    static void Add(proxyout *px,dyn_listen *l) { px->listeners->insert(l); }
    static void Rmv(proxyout *px,dyn_listen *l);

	void init(dyn_patchable *obj,int o,bool s = false);
    void exit();

	static void px_method(proxyout *obj,const t_symbol *s,int argc,const t_atom *argv);
	static void dsp(proxyout *x, t_signal **sp);
};

#endif
