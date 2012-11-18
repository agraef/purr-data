/* 
clk - syncable clocking objects

Copyright (c)2006-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#include "clk_client.h"
#include "clk_master.h"
#include <cstdio>

namespace clk {

Client::Client(int argc,const t_atom *argv)
    : offset(0),factor(1)
{
    ms_name(argc,argv);
}

Client::~Client()
{
	ms_name();
}

void Client::ms_name(int argc,const t_atom *argv)
{
    const t_symbol *n;
    if(!argc)
        n = NULL;
    else if(CanbeInt(*argv)) {
        // that's mainly for missing $-args which are set as 0
        char tmp[20];
        sprintf(tmp,"%i",GetAInt(*argv));
        n = MakeSymbol(tmp);
    }
    else
        n = GetSymbol(*argv);

    double current;
    if(LIKELY(clock)) {
        if(clock->name == n) return;

        current = clock->Current();

        Clock::Unregister(clock,this);
        clock = NULL;
    }
    else if(!n)
        return;
    else
        current = 0;

	if(LIKELY(n)) {
        clock = Clock::Register(n,this);
        offset += current-clock->Current();
	}
}

ClientExt::ClientExt(int argc,const t_atom *argv)
    : Client(argc,argv)
    , dblprec(false)
    , t3mode(false)
{
    AddInSignal();
	AddOutAnything(); // for sync'd timebase
	AddOutAnything(); // for logical time and correction

	setcnv();
}

void ClientExt::m_get(double offs) 
{ 
    if(!clock) return;

	ToSysFloat(1,static_cast<float>(offs*1000.));
    double co = Current(offs);
	if(dblprec)
		ToSysDouble(0,co);
	else
		ToSysFloat(0,static_cast<float>(co)); 
} 

void ClientExt::Forward(const t_symbol *sym,int argc,const t_atom *argv)
{
    if(LIKELY(clock)) {
        Master *master = const_cast<Master *>(clock->GetMaster());
        if(master)
            dynamic_cast<MasterExt *>(master)->Message(sym,argc,argv);
    }
}

bool ClientExt::CbDsp() { setcnv(); return false; }

void ClientExt::Setup(t_classid c)
{
    sym_message = MakeSymbol("message");

	FLEXT_CADDATTR_VAR(c,"name",mg_name,ms_name);

	FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
	FLEXT_CADDMETHOD_(c,0,sym_message,m_message);

	FLEXT_CADDATTR_VAR(c,"offset",mg_offset,ms_offset);
	FLEXT_CADDATTR_VAR(c,"factor",mg_factor,ms_factor);

	FLEXT_CADDATTR_GET(c,"timebase",mg_timebase);

	FLEXT_CADDATTR_VAR1(c,"dblprec",dblprec);
	FLEXT_CADDATTR_VAR1(c,"t3mode",t3mode);
}

const t_symbol *ClientExt::sym_message;

} // namespace

