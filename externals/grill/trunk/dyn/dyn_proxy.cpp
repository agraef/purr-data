/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

// proxy

void proxy::px_exit(proxy *px) 
{ 
    if(px->buf) FreeAligned(px->buf); 
}


// proxyin

void proxyin::init(dyn_patchable *obj,bool s)
{
    proxy::init(obj);
    outlet = outlet_new(&pdobj,s?&s_signal:&s_anything); 
}

void proxyin::px_method(proxyin *th,const t_symbol *s,int argc,const t_atom *argv)
{
    // send to connected object
	outlet_anything(th->outlet,(t_symbol *)s,argc,(t_atom *)argv);
}

void proxyin::dsp(proxyin *x, t_signal **sp) 
{
	int n = sp[0]->s_n;
	if(n != x->n) {
		// if vector size has changed make new buffer
		if(x->buf) FreeAligned(x->buf);
		x->buf = (t_sample *)NewAligned(sizeof(t_sample)*(x->n = n));
	}
	dsp_add_copy(x->buf,sp[0]->s_vec,n);
}


// proxyout

void proxyout::init(dyn_patchable *obj,int o,bool s)
{
    proxy::init(obj);
    listeners = new Listeners;
    outlet = o;
    if(s) outlet_new(&pdobj,&s_signal); 
}

void proxyout::exit()
{
    // invalidate all listeners
    for(Listeners::const_iterator it = listeners->begin(); it != listeners->end(); ++it)
        Destroy(*it);
    // delete container
    delete listeners;
}

void proxyout::Rmv(proxyout *px,dyn_listen *l) 
{ 
    px->listeners->erase(l);

    // if there are no more listeners we can delete the object!
    if(px->listeners->empty()) {
        dyn_patcher *p = px->object->owner;

        // remove reference
        px->object->RmvProxyOut(px->outlet);

        // delete PD object
        glist_delete(p->glist(),(t_gobj *)px);
    }
}

void proxyout::px_method(proxyout *th,const t_symbol *s,int argc,const t_atom *argv)
{
    // call attached responders
    for(Listeners::const_iterator it = th->listeners->begin(); it != th->listeners->end(); ++it)
        (*it)->Callback(th->outlet,s,argc,argv);
}

void proxyout::dsp(proxyout *x, t_signal **sp)
{
	int n = sp[0]->s_n;
	if(n != x->n) {
		// if vector size has changed make new buffer
		if(x->buf) FreeAligned(x->buf);
		x->buf = (t_sample *)NewAligned(sizeof(t_sample)*(x->n = n));
	}
	dsp_add_copy(sp[0]->s_vec,x->buf,n);
}
