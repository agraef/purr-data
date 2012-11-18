/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

DYN_EXPORT int dyn_Listen(int sched,dyn_id *id,dyn_id oid,int outlet,dyn_listener cb,void *data)
{
    ASSERT(id);

    if(oid == DYN_ID_NONE) return DYN_ERROR_NOTFOUND;

    dyn_patchable *obj = oid->Patchable();
    if(obj) {
        proxyout *px = obj->GetProxyOut(outlet);
        if(!px) {
            dyn_patcher *patcher = obj->owner;
            ASSERT(patcher);

            // put proxy on the same canvas as this object for it can be connected to it!
            px = (proxyout *)NewPDObject(DYN_TYPE_OBJECT,patcher->glist(),sym_dynpxout);
            if(px) {
                px->init(obj,outlet);

//                sys_lock();

//                canvas_setcurrent(patcher->glist());
                //  connect to associated object
                if(obj_connect((t_object *)obj->pdobj,outlet,&px->pdobj,0)) {
                    obj->AddProxyOut(outlet,px);
                }
                else {
                    // delete object;
                    glist_delete(patcher->glist(),(t_gobj *)px);
                    px = NULL;

                    // could not connect
                    post("Couldn't connect proxy object");
                }
//                canvas_unsetcurrent(patcher->glist());

//                sys_unlock();
            }
        }

        if(px) {
            dyn_ident *nid = new dyn_ident(DYN_TYPE_LISTENER,oid->callback,data);
            dyn_listen *l = new dyn_listen(nid,px,cb,data);
            proxyout::Add(px,l);
            nid->Set(l);
            *id = nid;
            return DYN_ERROR_NONE;
        }
        else
            return DYN_ERROR_GENERAL;
    }
    else
        return DYN_ERROR_TYPEMISMATCH;
}

dyn_listen::~dyn_listen()
{
    proxyout::Rmv(px,this);
}
