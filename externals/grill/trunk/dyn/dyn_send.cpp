/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

DYN_EXPORT int dyn_Send(int sched,dyn_id oid,int inlet,const t_symbol *sym,int argc,const t_atom *argv)
{
    ASSERT(oid);
    dyn_patchable *obj = oid->Patchable();

    if(obj) {
        /*
            It seems that we need connections (and proxy objects) for all inlets.
            Sending typed messages to the 0-inlet doesn't work for abstractions.
        */

        proxyin *px = obj->GetProxyIn(inlet);
        if(!px) {
            dyn_patcher *patcher = obj->owner;
            ASSERT(patcher);

            px = (proxyin *)NewPDObject(DYN_TYPE_OBJECT,patcher->glist(),sym_dynpxin);
            if(px) {
                px->init(obj);

//                    sys_lock();

//                    canvas_setcurrent(patcher->glist());
                //  connect to associated object
                if(obj_connect(&px->pdobj,0,(t_object *)obj->pdobj,inlet)) {
                    obj->AddProxyIn(inlet,px);
                }
                else {
                    glist_delete(patcher->glist(),(t_gobj *)px);
                    px = NULL;

                    // could not connect
                    post("Couldn't connect proxy object");
                }
//                    canvas_unsetcurrent(patcher->glist());
//                    sys_unlock();
            }
        }

        if(px) {
//                sys_lock();
            px->Message(sym,argc,argv);
//                sys_unlock();
            return DYN_ERROR_NONE;
        }
        else
            return DYN_ERROR_GENERAL;
    }
    else
        return DYN_ERROR_TYPEMISMATCH;
}

DYN_EXPORT int dyn_SendStr(int sched,dyn_id id,int inlet,const char *msg)
{
    t_binbuf *b = binbuf_new();
    binbuf_text(b,(char *)msg,strlen(msg));
    int argc = binbuf_getnatom(b);
    t_atom *argv = binbuf_getvec(b);
    int ret;
    if(argc) {
        t_symbol *sym;
        if(argv->a_type == A_SYMBOL) { sym = atom_getsymbol(argv); argc--,argv++; }
        else sym = &s_list;
        ret = dyn_Send(sched,id,inlet,sym,argc,argv);
    }
    else
        ret = DYN_ERROR_PARAMS;
    binbuf_free(b);
    return ret;
}
