/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

dyn_conn::~dyn_conn()
{
    dyn_patchable *sobj = NULL,*dobj = NULL;
    if(src != DYN_ID_NONE && (sobj = src->Patchable()))
        sobj->RmvOutlet(this);

    if(dst != DYN_ID_NONE && (dobj = dst->Patchable()))
        dobj->RmvInlet(this);

    if(sobj || dobj) {
        src = dst = DYN_ID_NONE; 
        ident->Callback(DYN_SIGNAL_DISCONN);
    }

    if(sobj && dobj) {
//        sys_lock();
        obj_disconnect((t_object *)sobj->pdobj,slet,(t_object *)dobj->pdobj,dlet);
//        sys_unlock();
    }
}

/*
void dyn_conn::Invalidate() 
{ 
    if(src != DYN_ID_NONE || dst != DYN_ID_NONE) {
        src = dst = DYN_ID_NONE; 
        ident->Callback(DYN_SIGNAL_DISCONN);
    }
}
*/

