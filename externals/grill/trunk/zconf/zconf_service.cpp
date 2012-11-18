/* 
zconf - zeroconf networking objects

Copyright (c)2006,2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3540 $
$LastChangedDate: 2007-12-12 11:10:55 -0500 (Wed, 12 Dec 2007) $
$LastChangedBy: thomas $
*/

#include "zconf.h"
#include <map>

namespace zconf {

static Symbol sym_service,sym_txtrecord;

class ServiceWorker
	: public Worker
{
public:
	ServiceWorker(Symbol n,Symbol t,Symbol d,int p,int i,const std::string &txt)
        : name(n),type(t),domain(d),interf(i),port(p),txtrec(txt)
	{}
	
protected:
	typedef union { unsigned char b[2]; unsigned short NotAnInteger; } Opaque16;

	virtual bool Init()
	{
		uint16_t PortAsNumber	= port;
		Opaque16 registerPort   = { { PortAsNumber >> 8, PortAsNumber & 0xFF } };
		int txtlen = (int)txtrec.length();

		DNSServiceErrorType err = DNSServiceRegister(
			&client, 
			0, // flags: default renaming behaviour 
            interf < 0?kDNSServiceInterfaceIndexLocalOnly:kDNSServiceInterfaceIndexAny, 
			name?GetString(name):NULL,
			GetString(type),
			domain?GetString(domain):NULL,
			NULL, // host
			registerPort.NotAnInteger,
			txtlen, txtlen?txtrec.c_str():NULL,
			(DNSServiceRegisterReply)&callback, this
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
	
	Symbol name,type,domain,text;
    int interf,port;
	std::string txtrec;

private:
    static void DNSSD_API callback(
        DNSServiceRef       sdRef, 
        DNSServiceFlags     flags, 
        DNSServiceErrorType errorCode, 
        const char          *name, 
        const char          *regtype, 
        const char          *domain, 
        void                *context ) 
	{
        // do something with the values that have been registered
        ServiceWorker *w = (ServiceWorker *)context;
		
		if(LIKELY(errorCode == kDNSServiceErr_NoError))
			w->OnRegister(name,regtype,domain);
		else
			w->OnError(errorCode);
	}

	void OnRegister(const char *name,const char *type,const char *domain)
    {
		t_atom at[3];
		SetString(at[0],name);
		SetString(at[1],type);
		SetString(at[2],DNSUnescape(domain).c_str());
		Message(sym_service,3,at);
    }
};

class Service
	: public Base
{
	FLEXT_HEADER_S(Service,Base,Setup)
public:

	Service(int argc,const t_atom *argv)
		: name(NULL),type(NULL),domain(NULL),interf(0),port(0)
	{		
		if(argc >= 1) {
			if(IsSymbol(*argv)) 
				type = GetSymbol(*argv);
			else
				throw "type must be a symbol";
			--argc,++argv;
		}
		if(argc >= 1) {
			if(CanbeInt(*argv)) 
				port = GetAInt(*argv);
			else
				throw "port must be a int";
			--argc,++argv;
		}
		if(argc >= 1) {
			if(IsSymbol(*argv)) 
				name = GetSymbol(*argv);
			else
				throw "name must be a symbol";
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

	void ms_name(const AtomList &args)
	{
		Symbol n;
		if(!args.Count())
			n = NULL;
		else if(args.Count() == 1 && IsSymbol(args[0]))
			n = GetSymbol(args[0]);
		else {
			post("%s - name [symbol]",thisName());
			return;
		}

		if(n != name) {
			name = n;
			Update();
		}
	}

	void mg_name(AtomList &args) const { if(name) { args(1); SetSymbol(args[0],name); } }

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

	void ms_port(int p)
	{
		if(p != port) {
			port = p;
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
	typedef std::map<Symbol,std::string> Textrecords;

public:
	void ms_txtrecord(int argc,const t_atom *argv)
	{
		bool upd = false;
		if(!argc) {
			if(txtrec.size()) {
				txtrec.clear();
				upd = true;
			}
		}
		else if(IsSymbol(*argv)) {
			Symbol key = GetSymbol(*argv++); --argc;
			if(!argc) {
				Textrecords::iterator it = txtrec.find(key);
				if(it != txtrec.end()) {
					txtrec.erase(it);
					upd = true;
				}
			}
			else {
				std::string txt;
				while(argc) {
					if(IsString(*argv)) {
						txt += GetString(*argv++);
						--argc;
						if(argc) txt += ' ';
					}
					else if(CanbeFloat(*argv)) {
						char num[32];
						sprintf(num,"%g",GetAFloat(*argv++));
						--argc;
						txt += num;
						if(argc) txt += ' ';
					}
					else
						++argv,--argc;
				}
				txtrec[key] = txt;
				upd = true;
			}
		}
		else
			post("%s %s - textrecord key must be a symbol",thisName(),GetString(thisTag()));
			
		if(upd) Update();
	}

	void mg_txtrecord(int argc,const t_atom *argv) 
	{
		if(!argc) {
			// dump all textrecord entries
			for(Textrecords::const_iterator it = txtrec.begin(); it != txtrec.end(); ++it)
				dumprec(it);
			ToQueueAnything(GetOutAttr(),sym_txtrecord,0,NULL);
		}
		else if(argc == 1 && IsSymbol(*argv)) {
			Symbol s = GetSymbol(*argv);
			Textrecords::const_iterator it = txtrec.find(s);
			if(it != txtrec.end()) 
				dumprec(it);
			else
				post("%s %s - textrecord %s not found",thisName(),GetString(thisTag()),GetString(s));
		}
		else
			post("%s %s - textrecord key must be a symbol (or empty)",thisName(),GetString(thisTag()));
	}

protected:

	void dumprec(Textrecords::const_iterator it)
	{
		t_atom at[2];
		SetSymbol(at[0],it->first);
		SetString(at[1],it->second.c_str());
		ToQueueAnything(GetOutAttr(),sym_txtrecord,2,at);
	}
	
	std::string makerec()
	{
		std::string ret;
		for(Textrecords::const_iterator it = txtrec.begin(); it != txtrec.end(); ++it) {
			const char *k = GetString(it->first);
			size_t len = strlen(k)+1+it->second.length();
			if(ret.length() > 255) {
				post("txtrecord %s too long!",k);
				continue;
			}
			ret += (char)(unsigned char)len;
			ret += k;
			ret += '=';
			ret += it->second;
		}
		return ret;
	}

	Symbol name,type,domain;
    int interf,port;
	Textrecords txtrec;
	
	virtual void Update()
	{
        Install(type?new ServiceWorker(name,type,domain,port,interf,makerec()):NULL);
	}

	FLEXT_CALLVAR_V(mg_name,ms_name)
	FLEXT_CALLVAR_V(mg_type,ms_type)
	FLEXT_CALLVAR_V(mg_domain,ms_domain)
	FLEXT_CALLSET_I(ms_port)
	FLEXT_ATTRGET_I(port)
	FLEXT_CALLSET_I(ms_interface)
	FLEXT_ATTRGET_I(interf)
	FLEXT_CALLBACK_V(ms_txtrecord)
	FLEXT_CALLBACK_V(mg_txtrecord)
	
	static void Setup(t_classid c)
	{
		sym_service = MakeSymbol("service");
		sym_txtrecord = MakeSymbol("txtrecord");
	
		FLEXT_CADDATTR_VAR(c,"name",mg_name,ms_name);
		FLEXT_CADDATTR_VAR(c,"port",port,ms_port);
		FLEXT_CADDATTR_VAR(c,"type",mg_type,ms_type);
		FLEXT_CADDATTR_VAR(c,"domain",mg_domain,ms_domain);
		FLEXT_CADDATTR_VAR(c,"interface",interf,ms_interface);
		FLEXT_CADDMETHOD_(c,0,sym_txtrecord,ms_txtrecord);
		FLEXT_CADDMETHOD_(c,0,"gettxtrecord",mg_txtrecord);
	}
};

FLEXT_LIB_V("zconf.service, zconf",Service)

} // namespace
