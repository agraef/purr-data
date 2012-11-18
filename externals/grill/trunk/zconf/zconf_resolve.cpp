/* 
zconf - zeroconf networking objects

Copyright (c)2006,2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3741 $
$LastChangedDate: 2011-02-14 19:01:25 -0500 (Mon, 14 Feb 2011) $
$LastChangedBy: thomas $
*/

#include "zconf.h"

namespace zconf {

static Symbol sym_resolve,sym_txtrecord;

class ResolveWorker
	: public Worker
{
public:
	ResolveWorker(Symbol n,Symbol t,Symbol d,int i)
        : name(n),type(t),domain(d),interf(i)
	{}
	
protected:
	virtual bool Init()
	{
		DNSServiceErrorType err = DNSServiceResolve(
            &client,
			0, // default renaming behaviour 
            interf < 0?kDNSServiceInterfaceIndexLocalOnly:kDNSServiceInterfaceIndexAny, 
			GetString(name),
			GetString(type),
			domain?GetString(domain):"local",
			callback, this
		);

//        post("resolve install err==%i",err);

		if(LIKELY(err == kDNSServiceErr_NoError)) {
			FLEXT_ASSERT(client);
			return Worker::Init();
		}
		else {
			OnError(err);
			return false;
		}
	} 
	
	Symbol name,type,domain;
    int interf;

private:
    static void DNSSD_API callback(
        DNSServiceRef client, 
        DNSServiceFlags flags, 
        uint32_t ifIndex, 
        DNSServiceErrorType errorCode,
	    const char *fullname, 
        const char *hosttarget, 
        uint16_t opaqueport, 
        uint16_t txtLen,
        const unsigned char *txtRecord,
        void *context)
	{
//        post("Resolve callback");

        ResolveWorker *w = (ResolveWorker *)context;

		if(LIKELY(errorCode == kDNSServiceErr_NoError)) {
//            post("Resolve ok");

			union { uint16_t s; unsigned char b[2]; } oport = { opaqueport };
			uint16_t port = ((uint16_t)oport.b[0]) << 8 | oport.b[1];
		
			char temp[256],*t,*t1;
			strcpy(temp,fullname);
			
			t = getdot(t1 = temp);
			FLEXT_ASSERT(t); // after service name           
            *t = 0;
			const char *srvname = t1; // service name

			t = getdot(t1 = t+1);
			FLEXT_ASSERT(t); // middle dot in type
			t = getdot(t+1);
			FLEXT_ASSERT(t); // after type
			*t = 0;
			const char *type = t1; // type

			const char *domain = t+1; // domain

//            post("Resolve name %s",hosttarget);
            const hostent *he = gethostbyname(hosttarget);
            if(he && he->h_length == 4) {
                const unsigned char *addr = (unsigned char *)he->h_addr_list[0];
                char ipaddr[16];
                sprintf(ipaddr,"%03i.%03i.%03i.%03i",addr[0],addr[1],addr[2],addr[3]);
//                post("Resolve %s %i",ipaddr,port);
                w->OnResolve(srvname,hosttarget,ipaddr,type,domain,port,ifIndex,txtLen,(char *)txtRecord);
            }
		}
		else
			w->OnError(errorCode);
		
		// remove immediately
//		w->shouldexit = true;
    }
	
	// can be called from a secondary thread
    void OnResolve(const char *srvname,const char *hostname,const char *ipaddr,const char *type,const char *domain,int port,int ifix,int txtLen,const char *txtRecord)
    {
        bool hastxtrec = txtRecord && txtLen && *txtRecord;
		t_atom at[8];
        SetString(at[0],DNSUnescape(srvname).c_str()); // host name
        SetString(at[1],type); // type
        SetString(at[2],DNSUnescape(domain).c_str()); // domain
		SetInt(at[3],ifix);
        SetString(at[4],DNSUnescape(hostname).c_str()); // host name
        SetString(at[5],ipaddr); // ip address
		SetInt(at[6],port);
        SetBool(at[7],hastxtrec);
		Message(sym_resolve,8,at);
        if(hastxtrec) {
            for(int i = 0; i < txtLen; ++i) {
                char txt[256];
                int l = (txtRecord)[i];
                memcpy(txt,txtRecord+i+1,l);
                txt[l] = 0;
                char *ass = strchr(txt,'=');
                if(ass) { 
                    *ass = 0;
                    SetString(at[1],ass+1);
                }
                SetString(at[0],txt);
                Message(sym_txtrecord,ass?2:1,at);
                i += l;
            }
    		Message(sym_txtrecord,0,NULL);
        }
    }

    static char *getdot(char *txt)
    {
        bool escaped = false;      
        for(char *t = txt; *t; ++t) {
            if(*t == '\\')
                escaped = !escaped;
			else if(escaped) {
				if(isdigit(*t)) {
					// three digits if escaping
					if(!isdigit(*++t)) return NULL;
					if(!isdigit(*++t)) return NULL;
				}

				escaped = false;
			}
			else if(*t == '.')
                return t;
        }
        return NULL;
    }
};


class Resolve
	: public Base
{
	FLEXT_HEADER_S(Resolve,Base,Setup)
public:

	Resolve() {}

	void m_resolve(int argc,const t_atom *argv)
	{
        if(argc == 0)
            Install(NULL);
        else if(argc < 2 || !IsSymbol(argv[0]) || !IsSymbol(argv[1])) {
			post("%s - %s: type (like _ssh._tcp or _osc._udp) and servicename must be given",thisName(),GetString(thisTag()));
		}
        else {
		    Symbol type = GetSymbol(argv[0]);
		    Symbol name = GetASymbol(argv[1]);
            Symbol domain = argc >= 3?GetASymbol(argv[2]):NULL;
            int interf = argc >= 4?GetAInt(argv[3]):0;

		    Install(new ResolveWorker(name,type,domain,interf));
        }
	}

protected:

	FLEXT_CALLBACK_V(m_resolve)

	static void Setup(t_classid c)
	{
		sym_resolve = MakeSymbol("resolve");
		sym_txtrecord = MakeSymbol("txtrecord");
	
		FLEXT_CADDMETHOD_(c,0,sym_resolve,m_resolve);
	}
};

FLEXT_LIB("zconf.resolve, zconf",Resolve)

} // namespace
