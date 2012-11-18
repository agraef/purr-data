/* 
zconf - zeroconf networking objects

Copyright (c)2006,2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3538 $
$LastChangedDate: 2007-11-30 09:25:15 -0500 (Fri, 30 Nov 2007) $
$LastChangedBy: thomas $
*/

#include "zconf.h"

namespace zconf {

class BrowseWorker
	: public Worker
{
public:
	BrowseWorker(Symbol t,Symbol d,int i)
        : type(t),domain(d),interf(i)
	{}
	
protected:
	virtual bool Init()
	{
		DNSServiceErrorType err = DNSServiceBrowse(
            &client, 
			0, // default renaming behaviour
            interf < 0?kDNSServiceInterfaceIndexLocalOnly:kDNSServiceInterfaceIndexAny, 
			GetString(type), 
			domain?GetString(domain):NULL, 
			&callback, this
        );

		if(LIKELY(err == kDNSServiceErr_NoError)) {
			FLEXT_ASSERT(client);
			return Worker::Init();
		}
		else {
			OnError(err);
			return false;
		}
	} 
	
	Symbol type,domain;
    int interf;

private:
    static void DNSSD_API callback(
        DNSServiceRef client, 
        DNSServiceFlags flags, // kDNSServiceFlagsMoreComing + kDNSServiceFlagsAdd
        uint32_t ifIndex, 
        DNSServiceErrorType errorCode,
        const char *replyName, 
        const char *replyType, 
        const char *replyDomain,                             
        void *context)
    {
        BrowseWorker *w = (BrowseWorker *)context;
		if(LIKELY(errorCode == kDNSServiceErr_NoError))
			w->OnBrowse(replyName,replyType,replyDomain,ifIndex,(flags & kDNSServiceFlagsAdd) != 0,(flags & kDNSServiceFlagsMoreComing) != 0);
		else
			w->OnError(errorCode);
    }

	// can be called from a secondary thread
    void OnBrowse(const char *name,const char *type,const char *domain,int ifix,bool add,bool more)
    {
        t_atom at[5]; 
		SetString(at[0],DNSUnescape(name).c_str());
		SetString(at[1],type);
		SetString(at[2],DNSUnescape(domain).c_str());
		SetInt(at[3],ifix);
		SetBool(at[4],more);
		Message(add?sym_add:sym_remove,5,at);
    }
};

class Browse
	: public Base
{
	FLEXT_HEADER_S(Browse,Base,Setup)
public:

	Browse(int argc,const t_atom *argv)
		: type(NULL),domain(NULL),interf(0)
	{
		if(argc >= 1) {
			if(IsSymbol(*argv)) 
				type = GetSymbol(*argv);
			else
				throw "type must be a symbol";
			--argc,++argv;
		}
		if(argc >= 1) {
			if(IsSymbol(*argv)) 
				domain = GetSymbol(*argv);
			else
				throw "domain must be a symbol";
			--argc,++argv;
		}
		if(argc >= 1) {
			if(CanbeInt(*argv)) 
				interf = GetAInt(*argv);
			else
				throw "interface must be an int";
			--argc,++argv;
		}
		Update();
	}

	void ms_type(const AtomList &args)
	{
		Symbol t;
		if(!args.Count())
			t = NULL;
		else if(args.Count() == 1 && IsSymbol(args[0]))
			t = GetSymbol(args[0]);
		else {
			post("%s - type [symbol]",thisName());
			return;
		}

		if(t != type) {
			type = t;
			Update();
		}
	}

	void mg_type(AtomList &args) const { if(type) { args(1); SetSymbol(args[0],type); } }

	void ms_domain(const AtomList &args)
	{
		Symbol d;
		if(!args.Count())
			d = NULL;
		else if(args.Count() == 1 && IsSymbol(args[0]))
			d = GetSymbol(args[0]);
		else {
			post("%s - domain [symbol]",thisName());
			return;
		}

		if(d != domain) {
			domain = d;
			Update();
		}
	}
	
	void mg_domain(AtomList &args) const { if(domain) { args(1); SetSymbol(args[0],domain); } }

	void ms_interface(int i)
	{
		if(i != interf) {
			interf = i;
			Update();
		}
	}

protected:
	Symbol type,domain;
    int interf;
	
	virtual void Update()
	{
        Install(type?new BrowseWorker(type,domain,interf):NULL);
	}

	FLEXT_CALLVAR_V(mg_type,ms_type)
	FLEXT_CALLVAR_V(mg_domain,ms_domain)
	FLEXT_CALLSET_I(ms_interface)
	FLEXT_ATTRGET_I(interf)
	
	static void Setup(t_classid c)
	{
		FLEXT_CADDATTR_VAR(c,"type",mg_type,ms_type);
		FLEXT_CADDATTR_VAR(c,"domain",mg_domain,ms_domain);
		FLEXT_CADDATTR_VAR(c,"interface",interf,ms_interface);
	}
};

FLEXT_LIB_V("zconf.browse, zconf",Browse)

} //namespace

