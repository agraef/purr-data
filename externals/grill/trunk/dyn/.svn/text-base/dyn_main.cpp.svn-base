/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"

t_class *pxin_class,*pxout_class;
t_class *pxins_class,*pxouts_class;

static t_object *pxin_new() { return (t_object *)pd_new(pxin_class); }
static t_object *pxins_new() { return (t_object *)pd_new(pxins_class); }
static t_object *pxout_new() { return (t_object *)pd_new(pxout_class); }
static t_object *pxouts_new() { return (t_object *)pd_new(pxouts_class); }


const t_symbol *sym_dyncanvas = gensym(" dyn canvas ");
const t_symbol *sym_dynsxin = gensym(" dyn in~ ");
const t_symbol *sym_dynsxout = gensym(" dyn out~ ");
const t_symbol *sym_dynpxin = gensym(" dyn in ");
const t_symbol *sym_dynpxout = gensym(" dyn out ");

static const t_symbol *sym_dsp = gensym("dsp");


static bool dyn_init()
{
	// set up proxy class for inbound messages
    pxin_class = class_new(const_cast<t_symbol *>(sym_dynpxin),(t_newmethod)pxin_new,(t_method)proxy::px_exit,sizeof(proxyin),0, A_NULL);
	class_addanything(pxin_class,proxyin::px_method); 

	// set up proxy class for inbound signals
	pxins_class = class_new(const_cast<t_symbol *>(sym_dynsxin),(t_newmethod)pxins_new,(t_method)proxy::px_exit,sizeof(proxyin),0, A_NULL);
    class_addmethod(pxins_class,(t_method)proxyin::dsp,const_cast<t_symbol *>(sym_dsp),A_NULL);
    CLASS_MAINSIGNALIN(pxins_class,proxyin,defsig);

	// set up proxy class for outbound messages
	pxout_class = class_new(const_cast<t_symbol *>(sym_dynpxout),(t_newmethod)pxout_new,(t_method)proxy::px_exit,sizeof(proxyout),0, A_NULL);
	class_addanything(pxout_class,proxyout::px_method); 

	// set up proxy class for outbound signals
	pxouts_class = class_new(const_cast<t_symbol *>(sym_dynsxout),(t_newmethod)pxouts_new,(t_method)proxy::px_exit,sizeof(proxyout),0, A_NULL);
    class_addmethod(pxouts_class,(t_method)proxyout::dsp,const_cast<t_symbol *>(sym_dsp),A_NULL);
    CLASS_MAINSIGNALIN(pxouts_class,proxyout,defsig);

    return true;
}

// static variables are hopefully initialized in order of appearance
// dyn_init depends on the symbols above
static bool init = dyn_init();


DYN_EXPORT int dyn_Version()
{
    return DYN_VERSION;
}

// \todo Implement
DYN_EXPORT int dyn_Lock()
{
    return DYN_ERROR_GENERAL;
}

// \todo Implement
DYN_EXPORT int dyn_Unlock()
{
    return DYN_ERROR_GENERAL;
}

// \todo Implement
DYN_EXPORT int dyn_Pending()
{
    return 0;
}

// \todo Implement
DYN_EXPORT int dyn_Finish()
{
    return DYN_ERROR_GENERAL;
}

