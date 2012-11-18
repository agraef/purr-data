/* 
clk - syncable clocking objects

Copyright (c)2006-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#include "clk_master.h"
#include "clk_client.h"
#include <cstdio>

namespace clk {

Master::Master(int argc,const t_atom *argv)
    : pre(true)
{
    const t_symbol *name;

    if(UNLIKELY(argc != 1))
        throw ExcSyntax();

    // that's mainly for missing $-args which are set as 0
    if(CanbeInt(*argv)) {
        char tmp[20];
        sprintf(tmp,"%i",GetAInt(*argv));
        name = MakeSymbol(tmp);
    }
    else
        name = GetSymbol(*argv);

    clock = Clock::Register(name,this);

    if(UNLIKELY(!clock)) 
        throw ExcExisting();
}

Master::~Master()
{
    Clock::Unregister(clock,this);
}


MasterExt::MasterExt(int argc,const t_atom *argv)
    : Master(argc,argv)
{
    AddInAnything();
}

void MasterExt::m_reset() 
{ 
    Forward(sym_reset,0,NULL);
    reset(); 
}

void MasterExt::Forward(const t_symbol *sym,int argc,const t_atom *argv)
{
    if(LIKELY(clock)) {
        const Clock::Clients &clients = clock->GetClients();
        for(Clock::Clients::const_iterator it = clients.begin(); it != clients.end(); ++it)
            dynamic_cast<ClientExt *>(*it)->Message(sym,argc,argv);
    }
}

void MasterExt::Setup(t_classid c)
{
    sym_message = MakeSymbol("message");
    sym_reset = MakeSymbol("reset");

	FLEXT_CADDMETHOD_(c,0,sym_message,m_message);

    FLEXT_CADDATTR_VAR(c,"timebase",mg_timebase,ms_timebase);
	FLEXT_CADDATTR_VAR(c,"precision",mg_precision,ms_precision);
    FLEXT_CADDATTR_VAR(c,"weight",mg_weight,ms_weight);
}

const t_symbol *MasterExt::sym_message,*MasterExt::sym_reset;

} // namespace

