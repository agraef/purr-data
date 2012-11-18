/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

dyn_patchable::dyn_patchable(dyn_id id,dyn_patcher *o,t_gobj *obj)
    : dyn_base(id)
    , pdobj(obj),owner(o)
{
    if(owner) owner->Add(this);
}

dyn_patchable::~dyn_patchable()
{
    if(owner) {
        // if no owner, it must be a "root" patcher, which is not connectable

        owner->Remove(this);

        t_glist *glist = owner->glist();

    //	canvas_setcurrent(glist); 

        // delete object itself
        glist_delete(glist,pdobj);

        // delete proxies
        for(Proxies::iterator iit = pxin.begin(); iit != pxin.end(); ++iit) {
            proxyin *px = (proxyin *)iit->second;
            glist_delete(glist,(t_gobj *)px);
        }

        for(Proxies::iterator oit = pxout.begin(); oit != pxout.end(); ++oit) {
            proxyout *px = (proxyout *)oit->second;
            glist_delete(glist,(t_gobj *)px);
        }

    //	canvas_unsetcurrent(glist); 

        // invalidate all associated connection objects
        // the connection objects will not be deleted here!
        Connections::iterator cit;
        for(cit = inconns.begin(); cit != inconns.end(); ++cit)
            Destroy(*cit);

        for(cit = outconns.begin(); cit != outconns.end(); ++cit)
            Destroy(*cit);
    }
    else {
        dyn_patcher *p = dynamic_cast<dyn_patcher *>(this);
        ASSERT(p);

        // delete "root"-patcher
        canvas_free(p->glist());
    }
}

void dyn_patchable::AddInlet(dyn_conn *o)
{
    ASSERT(inconns.find(o) == inconns.end());
    inconns.insert(o);
}

void dyn_patchable::RmvInlet(dyn_conn *o)
{
    ASSERT(inconns.find(o) != inconns.end());
    inconns.erase(o);
}

void dyn_patchable::AddOutlet(dyn_conn *o)
{
    ASSERT(outconns.find(o) == outconns.end());
    outconns.insert(o);
}

void dyn_patchable::RmvOutlet(dyn_conn *o)
{
    ASSERT(outconns.find(o) != outconns.end());
    outconns.erase(o);
}

void dyn_patchable::EnumInlet(int ix,dyn_enumfun fun,void *data)
{
    for(Connections::const_iterator it = inconns.begin(); it != inconns.end(); ++it) {
        const dyn_conn *c = *it;
        if(c->dlet == ix && !fun(c->ident,data)) break;
    }
}

void dyn_patchable::EnumOutlet(int ix,dyn_enumfun fun,void *data)
{
    for(Connections::const_iterator it = outconns.begin(); it != outconns.end(); ++it) {
        const dyn_conn *c = *it;
        if(c->slet == ix && !fun(c->ident,data)) break;
    }
}
