/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

DYN_EXPORT int dyn_EnumObjects(dyn_id id,dyn_enumfun fun,void *data)
{
    dyn_patcher *p;
    if(id && (p = id->Patcher())) {
        p->Enumerate(fun,data);
        return DYN_ERROR_NONE;
    }
    else
        return DYN_ERROR_NOTFOUND;
}

DYN_EXPORT int dyn_NewPatcher(int sched,dyn_id *sid,dyn_callback cb,dyn_id psid /*,const char *name*/)
{
    try {
        ASSERT(sid);
        *sid = NewObject(DYN_TYPE_PATCHER,cb,psid,NULL);
        return *sid?DYN_ERROR_NONE:DYN_ERROR_GENERAL;
    }
    catch(int err) { return err; }
}

DYN_EXPORT int dyn_NewObject(int sched,dyn_id *oid,dyn_callback cb,dyn_id sid,const t_symbol *obj,int argc,const t_atom *argv)
{
    try {
        ASSERT(oid);
        *oid = NewObject(DYN_TYPE_OBJECT,cb,sid,obj,argc,argv);      
        return *oid?DYN_ERROR_NONE:DYN_ERROR_GENERAL;
    }
    catch(int err) { return err; }
}

DYN_EXPORT int dyn_NewObjectStr(int sched,dyn_id *oid,dyn_callback cb,dyn_id sid,const char *args)
{
    t_binbuf *b = binbuf_new();
    binbuf_text(b,(char *)args,strlen(args));
    int argc = binbuf_getnatom(b);
    t_atom *argv = binbuf_getvec(b);
    int ret;
    if(argc) {
        t_symbol *sym;
        if(argv->a_type == A_SYMBOL) { sym = atom_getsymbol(argv); argc--,argv++; }
        else sym = &s_list;
        ret = dyn_NewObject(sched,oid,cb,sid,sym,argc,argv);
    }
    else
        ret = DYN_ERROR_PARAMS;
    binbuf_free(b);
    return ret;
}

DYN_EXPORT int dyn_NewMessage(int sched,dyn_id *oid,dyn_callback cb,dyn_id sid,int argc,t_atom *argv)
{
    try {
        ASSERT(oid);
        *oid = NewObject(DYN_TYPE_MESSAGE,cb,sid,NULL,argc,argv);
        return *oid?DYN_ERROR_NONE:DYN_ERROR_GENERAL;
    }
    catch(int err) { return err; }
}

DYN_EXPORT int dyn_NewMessageStr(int sched,dyn_id *oid,dyn_callback cb,dyn_id sid,const char *msg)
{
    t_binbuf *b = binbuf_new();
    binbuf_text(b,(char *)msg,strlen(msg));
    int argc = binbuf_getnatom(b);
    t_atom *argv = binbuf_getvec(b);
    int ret = dyn_NewMessage(sched,oid,cb,sid,argc,argv);
    binbuf_free(b);
    return ret;
}

DYN_EXPORT int dyn_NewConnection(int sched,dyn_id *cid,dyn_callback cb,dyn_id soid,int outlet,dyn_id doid,int inlet)
{
    ASSERT(cid);
    ASSERT(soid && inlet >= 0);
    ASSERT(doid && outlet >= 0);

    dyn_patchable *sobj = soid->Patchable();
    dyn_patchable *dobj = doid->Patchable();

    int ret = DYN_ERROR_NONE;
    dyn_id conn = DYN_ID_NONE;

    if(!sobj || !dobj) {
	    ret = DYN_ERROR_TYPEMISMATCH;
        goto error;
    }

    if(sobj->owner != dobj->owner) {
	    ret = DYN_ERROR_CONN_PATCHER;
        goto error;
    }

	{
		dyn_patcher *patcher = sobj->owner;
		ASSERT(patcher);
	
	//    sys_lock();
	
		canvas_setcurrent(patcher->glist());
		bool ok = 
			!canvas_isconnected(patcher->glist(),(t_text *)sobj->pdobj,outlet,(t_text *)dobj->pdobj,inlet) &&
			obj_connect((t_object *)sobj->pdobj, outlet,(t_object *)dobj->pdobj,inlet);
		canvas_unsetcurrent(patcher->glist());
	//    sys_unlock();
	
		if(!ok) {
			ret = DYN_ERROR_CONN_GENERAL;
			goto error;
		}
	}

    conn = new dyn_ident(DYN_TYPE_CONN,cb);
    conn->Set(new dyn_conn(conn,soid,outlet,doid,inlet));

    // add connection to object lists
    sobj->AddOutlet(conn->Conn());
    dobj->AddInlet(conn->Conn());

    *cid = conn;
    return ret;

error:
    if(conn != DYN_ID_NONE) delete conn;
    return ret;
}

DYN_EXPORT int dyn_Free(int sched,dyn_id oid)
{
    try {
        DelObject(oid);
        return DYN_ERROR_NONE;
    }
    catch(int err) { return err; }
}

// NOT IMPLEMENTED
DYN_EXPORT int dyn_Reset()
{
    return DYN_ERROR_GENERAL;
}


DYN_EXPORT int dyn_SetData(dyn_id oid,void *data)
{
    if(oid) {
        oid->userdata = data;
        return DYN_ERROR_NONE;
    }
    else
        return DYN_ERROR_NOTFOUND;
}

DYN_EXPORT int dyn_GetData(dyn_id oid,void **data)
{
    ASSERT(data);
    if(oid) {
        *data = oid->userdata;
        return DYN_ERROR_NONE;
    }
    else
        return DYN_ERROR_NOTFOUND;
}


DYN_EXPORT int dyn_GetType(dyn_id oid)
{
    if(oid)
        return oid->type;
    else
        return DYN_ERROR_NOTFOUND;
}


DYN_EXPORT int dyn_GetInletCount(dyn_id id)
{
    dyn_patchable *p;
    if(id && (p = id->Patchable()))
        return p->GetInletCount();
    else
        return DYN_ERROR_NOTFOUND;
}

DYN_EXPORT int dyn_GetOutletCount(dyn_id id)
{
    dyn_patchable *p;
    if(id && (p = id->Patchable()))
        return p->GetOutletCount();
    else
        return DYN_ERROR_NOTFOUND;
}

DYN_EXPORT int dyn_GetInletType(dyn_id id,int inlet)
{
    dyn_patchable *p;
    if(id && (p = id->Patchable()))
        return p->GetInletType(inlet);
    else
        return DYN_ERROR_NOTFOUND;
}

DYN_EXPORT int dyn_GetOutletType(dyn_id id,int outlet)
{
    dyn_patchable *p;
    if(id && (p = id->Patchable()))
        return p->GetOutletType(outlet);
    else
        return DYN_ERROR_NOTFOUND;
}


DYN_EXPORT int dyn_GetConnectionSource(dyn_id cid,dyn_id *soid,int *outlet)
{
    ASSERT(soid && outlet);
    dyn_conn *c;
    if(cid && (c = cid->Conn())) {
        *soid = c->src;
        *outlet = c->slet;
        return DYN_ERROR_NONE;
    }
    else
        return DYN_ERROR_NOTFOUND;
}

DYN_EXPORT int dyn_GetConnectionDrain(dyn_id cid,dyn_id *doid,int *inlet)
{
    ASSERT(doid && inlet);
    dyn_conn *c;
    if(cid && (c = cid->Conn())) {
        *doid = c->dst;
        *inlet = c->dlet;
        return DYN_ERROR_NONE;
    }
    else
        return DYN_ERROR_NOTFOUND;
}


DYN_EXPORT int dyn_EnumInletConnections(dyn_id id,int inlet,dyn_enumfun fun,void *data)
{
    dyn_patchable *p;
    if(id && (p = id->Patchable())) {
        p->EnumInlet(inlet,fun,data);
        return DYN_ERROR_NONE;
    }
    else
        return DYN_ERROR_NOTFOUND;
}

DYN_EXPORT int dyn_EnumOutletConnections(dyn_id id,int outlet,dyn_enumfun fun,void *data)
{
    dyn_patchable *p;
    if(id && (p = id->Patchable())) {
        p->EnumOutlet(outlet,fun,data);
        return DYN_ERROR_NONE;
    }
    else
        return DYN_ERROR_NOTFOUND;
}
