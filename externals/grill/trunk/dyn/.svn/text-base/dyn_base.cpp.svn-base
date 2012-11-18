/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

dyn_base::dyn_base(dyn_id id)
    : ident(id) 
{
    ident->Callback(DYN_SIGNAL_NEW);
}

dyn_base::~dyn_base() 
{
    // ident should already have been anonymized...
    ASSERT(!ident->data);

    ident->Callback(DYN_SIGNAL_FREE);
}
