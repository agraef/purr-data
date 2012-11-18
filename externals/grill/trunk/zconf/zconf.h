/* 
zconf - zeroconf networking objects

Copyright (c)2006,2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3627 $
$LastChangedDate: 2008-08-23 14:10:17 -0400 (Sat, 23 Aug 2008) $
$LastChangedBy: thomas $
*/

#ifndef __ZCONF_H
#define __ZCONF_H

#define FLEXT_ATTRIBUTES 1

#include <flext.h>
#include <flcontainers.h>

#if FLEXT_OS == FLEXT_OS_WIN
	#include <stdlib.h>
    #include <winsock.h>
#else
	#include <unistd.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <net/if.h>
#endif

#include <dns_sd.h>

#include <vector>
#include <string>
#include <set>
#include <boost/shared_ptr.hpp>


namespace zconf {

#define MAX_DOMAIN_LABEL 63
#define MAX_DOMAIN_NAME 255

typedef const t_symbol *Symbol;

std::string DNSEscape(const char *txt,bool escdot = true);
std::string DNSUnescape(const char *txt);

class Worker
	: public flext
{
	friend class Base;

public:
	virtual ~Worker();

protected:
	Worker(): client(0),fd(-1),shouldexit(false) {}
	
    void Message(AtomAnything &msg) { messages.Put(msg); }
    void Message(const t_symbol *sym,int argc,const t_atom *argv) { AtomAnything msg(sym,argc,argv); Message(msg); }

    void OnError(DNSServiceErrorType error);

	// to be called from worker thread (does the actual work)
	virtual bool Init();	
	
	DNSServiceRef client;
	int fd;
    bool shouldexit;

	typedef struct { unsigned char c[ 64]; } domainlabel;      // One label: length byte and up to 63 characters.
	typedef struct { unsigned char c[256]; } domainname;       // Up to 255 bytes of length-prefixed domainlabels.

	static char *conv_label2str(const domainlabel *label, char *ptr);
	static char *conv_domain2str(const domainname *name, char *ptr);
	static bool conv_type_domain(const void *rdata, uint16_t rdlen, char *type, char *domain);

	static Symbol sym_error,sym_add,sym_remove;

    typedef ValueFifo<AtomAnything> Messages;
    Messages messages;
};

typedef boost::shared_ptr<Worker> WorkerPtr;


class Base
	: public flext_base
{
	FLEXT_HEADER_S(Base,flext_base,Setup)

	friend class Worker;

public:
	Base();
	virtual ~Base();
	
protected:
	void Install(Worker *w);

private:
	WorkerPtr worker;

    typedef ValueFifo<WorkerPtr> Workers;
	static Workers *newworkers;

#ifdef PD_DEVEL_VERSION
	static t_int idlefun(t_int *data);
#else
    static void idlefun(void *);
#endif

    static void threadfun(thr_params *);

	static void Setup(t_classid);

    static ThrCond cond;

    virtual bool CbIdle();
};

} // namespace

#endif
