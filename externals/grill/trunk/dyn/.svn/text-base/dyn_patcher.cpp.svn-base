/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

dyn_patcher::~dyn_patcher()
{
    // delete sub-objects
    while(!objs.empty()) {
        // objects delete themselves from the container!
        Destroy(*objs.begin()); 
    }
}

void dyn_patcher::Enumerate(dyn_enumfun fun,void *data)
{
    for(Objects::const_iterator it = objs.begin(); it != objs.end(); ++it)
        if(fun((*it)->ident,data)) break;
}
