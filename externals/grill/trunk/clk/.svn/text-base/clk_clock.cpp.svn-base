/* 
clk - syncable clocking objects

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

#include "clk_client.h"
#include <cmath>

namespace clk {

void Clock::Set(double x,double y,bool pre)
{ 
    if(UNLIKELY(pre && n)) {
        // there are statistics already... preserve existing factor

        n = -1;
        prex = x,prey = y;

        double f = Factor();
        reset();
        initfactor(x,y,f);
    }
    else {
        double t = Time();
        double cold = Get(t);

        if(UNLIKELY(n < 0)) {
            reset();
            add(prex,prey);
        }
        add(x,y);

        double cnew = Get(t);

        if(fabs(cnew-cold) >= precision) {
            for(Clients::iterator it = clients.begin(); it != clients.end(); ++it)
                (*it)->Update(cold,cnew);
        }
    }
}

void Clock::TryFree(Clock *clk)
{
    if(clk->clients.empty() && !clk->master) {
        FLEXT_ASSERT(clocks.find(clk->name) != clocks.end());
        clocks.erase(clk->name);
        delete clk;
    }
}

Clock *Clock::Register(const t_symbol *n,Client *c) 
{
    FLEXT_ASSERT(c);

    Clocks::iterator it = clocks.find(n);
    Clock *clk;
    if(it == clocks.end())
        clocks[n] = clk = new Clock(n);
    else {
        clk = it->second;
        FLEXT_ASSERT(clk->clients.find(c) == clk->clients.end());
    }
    clk->clients.insert(c);
    return clk;
}

void Clock::Unregister(Clock *clk,Client *c) 
{
    FLEXT_ASSERT(clk->clients.find(c) != clk->clients.end());
    clk->clients.erase(c);
    TryFree(clk);
}

Clock *Clock::Register(const t_symbol *n,Master *m)
{
    FLEXT_ASSERT(m);

    Clocks::iterator it = clocks.find(n);
    Clock *clk;
    if(it == clocks.end())
        clocks[n] = clk = new Clock(n,m);
    else {
        clk = it->second;
        if(clk->master) return NULL; // already occupied
        clk->master = m;
    }
    return clk;
}

void Clock::Unregister(Clock *clk,Master *m) 
{
    FLEXT_ASSERT(clk->master == m);
    clk->master = NULL;
    TryFree(clk);
}

Clock::Clocks Clock::clocks;

} // namespace
