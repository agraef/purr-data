/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#ifndef __DYN_DATA_H
#define __DYN_DATA_H

#include "dyn_proxy.h"

#include <map>
#include <set>


class dyn_patcher;
class dyn_conn;


class dyn_base
{
public:
    dyn_base(dyn_id id);

    dyn_id ident;

    friend void Destroy(dyn_base *b);
protected:
    virtual ~dyn_base();
};


typedef std::set<dyn_base *> Objects;
typedef std::set<dyn_conn *> Connections;
typedef std::map<int,proxy *> Proxies;


class dyn_patchable:
    public dyn_base
{
public:
    dyn_patchable(dyn_id id,dyn_patcher *o,t_gobj *obj);

    void AddProxyIn(int inlet,proxyin *o) { pxin[inlet] = o; }
    void RmvProxyIn(int inlet) { pxin.erase(inlet); }
    proxyin *GetProxyIn(int inlet) { Proxies::iterator it = pxin.find(inlet); return it != pxin.end()?(proxyin *)it->second:NULL; }
    void AddProxyOut(int outlet,proxyout *o) { pxout[outlet] = o; }
    void RmvProxyOut(int outlet) { pxout.erase(outlet); }
    proxyout *GetProxyOut(int outlet) { Proxies::iterator it = pxout.find(outlet); return it != pxout.end()?(proxyout *)it->second:NULL; }

    int GetInletCount() const { return obj_ninlets((t_object *)pdobj); }
    int GetOutletCount() const { return obj_noutlets((t_object *)pdobj); }
    int GetInletType(int ix) const { return obj_issignalinlet((t_object *)pdobj,ix)?DYN_INOUT_SIGNAL:DYN_INOUT_MESSAGE; }
    int GetOutletType(int ix) const { return obj_issignaloutlet((t_object *)pdobj,ix)?DYN_INOUT_SIGNAL:DYN_INOUT_MESSAGE; }

    void AddInlet(dyn_conn *o);
    void RmvInlet(dyn_conn *o);
    void AddOutlet(dyn_conn *o);
    void RmvOutlet(dyn_conn *o);

    void EnumInlet(int ix,dyn_enumfun fun,void *data);
    void EnumOutlet(int ix,dyn_enumfun fun,void *data);

    t_gobj *pdobj; // PD object
    dyn_patcher *owner; // patcher

protected:
    virtual ~dyn_patchable();

    Proxies pxin,pxout;
    Connections inconns,outconns;
};

class dyn_patcher:
    public dyn_patchable
{
public:
    dyn_patcher(dyn_id id,dyn_patcher *o,t_glist *obj): dyn_patchable(id,o,(t_gobj *)obj) {}
    virtual ~dyn_patcher();

    t_glist *glist() { return (t_glist *)pdobj; }

    void Add(dyn_base *o) { objs.insert(o); }
    void Remove(dyn_base *o) { objs.erase(o); }

    void Enumerate(dyn_enumfun fun,void *data);

protected:
    Objects objs;
};

class dyn_object:
    public dyn_patchable
{
public:
    dyn_object(dyn_id id,dyn_patcher *o,t_gobj *obj): dyn_patchable(id,o,obj) {}
};

class dyn_message:
    public dyn_patchable
{
public:
    dyn_message(dyn_id id,dyn_patcher *o,t_gobj *obj): dyn_patchable(id,o,obj) {}
};

class dyn_text:
    public dyn_patchable
{
public:
    dyn_text(dyn_id id,dyn_patcher *o,t_gobj *obj): dyn_patchable(id,o,obj) {}
};


class dyn_conn:
    public dyn_base
{
public:
    dyn_conn(dyn_id id,dyn_id s,int six,dyn_id d,int dix)
        : dyn_base(id)
        , src(s),slet(six),dst(d),dlet(dix) 
    {}

    dyn_id src,dst; // connected objects
    int slet,dlet;

protected:
    virtual ~dyn_conn();
};

class dyn_listen:
    public dyn_base
{
public:
    dyn_listen(dyn_id id,proxyout *p,dyn_listener cb,void *dt)
        : dyn_base(id),px(p) 
        , callback(cb),userdata(dt)
    {}

    void Callback(int outlet,const t_symbol *sym,int argc,const t_atom *argv) 
    { 
        if(callback) callback(ident,px->object->ident,outlet,sym,argc,argv,userdata); 
    }

    proxyout *px;
    dyn_listener *callback;
    void *userdata;
protected:
    virtual ~dyn_listen();
};


struct dyn_ident
{
    dyn_ident(int tp,dyn_callback cb,void *dt = NULL):
        type(tp),data(NULL),
        callback(cb),userdata(dt)
    {}

    ~dyn_ident()
    {
        // data should already have been cleared by the objects
        ASSERT(!data);
    }

    void Set(dyn_base *obj) { data = obj; }

    void Callback(int signal) { if(callback) callback(this,signal,userdata); }

    dyn_patcher *Patcher() { return dynamic_cast<dyn_patcher *>(data); }
    dyn_object *Object() { return dynamic_cast<dyn_object *>(data); }
    dyn_message *Message() { return dynamic_cast<dyn_message *>(data); }
    dyn_text *Text() { return dynamic_cast<dyn_text *>(data); }
    dyn_conn *Conn() { return dynamic_cast<dyn_conn *>(data); }
    dyn_patchable *Patchable() { return dynamic_cast<dyn_patchable *>(data); }
    dyn_listen *Listen() { return dynamic_cast<dyn_listen *>(data); }

    int type;
    dyn_base *data;

    dyn_callback *callback;
    void *userdata;
};

inline void Destroy(dyn_base *b)
{
    b->ident->data = NULL;
    delete b;
}

#endif
