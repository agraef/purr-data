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

class DomainsWorker
	: public Worker
{
public:
	DomainsWorker(int i,bool reg)
        : interf(i),regdomains(reg)
	{}
	
protected:
	int interf;
    bool regdomains;

	virtual bool Init()
	{
        DNSServiceErrorType err = DNSServiceEnumerateDomains( 
            &client, 
            regdomains?kDNSServiceFlagsRegistrationDomains:kDNSServiceFlagsBrowseDomains, // flags
            interf < 0?kDNSServiceInterfaceIndexLocalOnly:kDNSServiceInterfaceIndexAny, 
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
	
private:

    static void DNSSD_API callback(
        DNSServiceRef client, 
        DNSServiceFlags flags, // kDNSServiceFlagsMoreComing + kDNSServiceFlagsAdd
        uint32_t ifIndex, 
        DNSServiceErrorType errorCode,
        const char *replyDomain,                             
        void *context)
    {
        DomainsWorker *w = (DomainsWorker *)context;
		if(LIKELY(errorCode == kDNSServiceErr_NoError))
			w->OnDomain(replyDomain,ifIndex,(flags & kDNSServiceFlagsAdd) != 0,(flags & kDNSServiceFlagsMoreComing) != 0);
		else
			w->OnError(errorCode);
    }

	// can be called from a secondary thread
    void OnDomain(const char *domain,int ifix,bool add,bool more)
    {
        t_atom at[3]; 
		SetString(at[0],DNSUnescape(domain).c_str());
		SetInt(at[1],ifix);
		SetBool(at[2],more);
		Message(add?sym_add:sym_remove,3,at);
    }
};

class Domains
	: public Base
{
	FLEXT_HEADER_S(Domains,Base,Setup)
public:

	Domains()
        : mode(0),interf(0)
	{		
		Update();
	}

    void ms_mode(int m) 
    {
        if(m < 0 || m > 2)
            post("%s - mode must be 0 (off), 1 (browse domains), 2 (registration domains)",thisName());
        else {
            mode = m;
            Update();
        }
    }

	void ms_interface(int i)
	{
		if(i != interf) {
			interf = i;
			Update();
		}
	}

protected:
    int mode;
	int interf;

	void Update()
	{
        Install(mode?new DomainsWorker(interf,mode == 2):NULL);
	}

    FLEXT_ATTRGET_I(mode)
    FLEXT_CALLSET_I(ms_mode)
	FLEXT_CALLSET_I(ms_interface)
	FLEXT_ATTRGET_I(interf)

	static void Setup(t_classid c)
	{
        FLEXT_CADDATTR_VAR(c,"mode",mode,ms_mode);
        FLEXT_CADDATTR_VAR(c,"interface",interf,ms_interface);
	}
};

FLEXT_LIB("zconf.domains, zconf",Domains)

} //namespace
