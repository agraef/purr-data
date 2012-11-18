/* 
zconf - zeroconf networking objects

Copyright (c)2006,2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3627 $
$LastChangedDate: 2008-08-23 14:10:17 -0400 (Sat, 23 Aug 2008) $
$LastChangedBy: thomas $
*/

#include "zconf.h"

#define ZCONF_VERSION "0.2.1"

namespace zconf {

// unescape a DNS-escaped string and make a symbol
// http://www.faqs.org/rfcs/rfc1035.html, section 5.1
std::string DNSEscape(const char *txt,bool escdot)
{
	std::string ret;
	for(const char *c = txt; *c; ++c) {
		// \TODO: here, the choice of characters to escape is tentative... look up which ones should be really escaped
		if((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || strchr("_-+",*c) || (!escdot && *c == '.'))
			ret += *c;
		else {
			ret += '\\';
			if(strchr(".\\/!?=*#:;,&%()<>",*c))
				ret += *c;
			else {
				int d = *c;
				ret += (char)(unsigned char)(d/100);
				ret += (char)(unsigned char)((d/10)%10);
				ret += (char)(unsigned char)(d%10);
			}
		}
	}
	return ret;
}

std::string DNSUnescape(const char *txt)
{
	std::string ret;
	const char *c = txt;
	bool esc = false;
	while(*c) {
		if(esc) {
			if(*c >= '0' && *c <= '9') {
				// decimal code
				int d = (*c++)-'0';
				d = d*10+(*c++)-'0';
				d = d*10+(*c++)-'0';
				ret += (char)(unsigned char)d;
			}
			else
				// escaped special char (like .)
				ret += *(c++);
			esc = false;
		}
		else if(*c == '\\') {
			esc = true;
			c++;
		}
		else
			ret += *(c++);
	}
	return ret;
}

////////////////////////////////////////////////

Worker::~Worker()
{
//    fprintf(stderr,"Destroy %p\n",this);
	if(client) 
		DNSServiceRefDeallocate(client);
}

bool Worker::Init()
{
//    fprintf(stderr,"Init %p\n",this);
	fd = DNSServiceRefSockFD(client);
	return fd >= 0;
}


char *Worker::conv_label2str(const domainlabel *const label, char *ptr)
{
	FLEXT_ASSERT(label != NULL);
	FLEXT_ASSERT(ptr   != NULL);
	
	const unsigned char *      src = label->c;      // Domain label we're reading.
	const unsigned char        len = *src++;        // Read length of this (non-null) label.
	const unsigned char *const end = src + len;     // Work out where the label ends.
	
	if (len > MAX_DOMAIN_LABEL) return(NULL);       // If illegal label, abort.
	while (src < end) {                             // While we have characters in the label.
		unsigned char c = *src++;
		if (c == '.' || c == '\\')                  // If character is a dot or the escape character
			*ptr++ = '\\';                          // Output escape character.
		else if (c <= ' ') {                        // If non-printing ascii, output decimal escape sequence.
			*ptr++ = '\\';
			*ptr++ = (char)  ('0' + (c / 100)     );
			*ptr++ = (char)  ('0' + (c /  10) % 10);
			c      = (unsigned char)('0' + (c      ) % 10);
		}
		*ptr++ = (char)c;                           // Copy the character.
	}
	*ptr = 0;                                       // Null-terminate the string
	return(ptr);                                    // and return.
}

char *Worker::conv_domain2str(const domainname *const name, char *ptr)
{
	FLEXT_ASSERT(name != NULL);
	FLEXT_ASSERT(ptr  != NULL);

	const unsigned char *src         = name->c;                     // Domain name we're reading.
	const unsigned char *const max   = name->c + MAX_DOMAIN_NAME;   // Maximum that's valid.

	if (*src == 0) *ptr++ = '.';                                    // Special case: For root, just write a dot.

	while (*src) {                                                  // While more characters in the domain name.
		if (src + 1 + *src >= max) return(NULL);
		ptr = conv_label2str((const domainlabel *)src, ptr);
		if (!ptr) return(NULL);
		src += 1 + *src;
		*ptr++ = '.';                                               // Write the dot after the label.
	}

	*ptr++ = 0;                                                     // Null-terminate the string
	return(ptr);                                                    // and return.
}

bool Worker::conv_type_domain(const void * rdata, uint16_t rdlen, char * type, char * domain)
{
	unsigned char *cursor;
	unsigned char *start;
	unsigned char *end;

	FLEXT_ASSERT(rdata  != NULL);
	FLEXT_ASSERT(rdlen  != 0);
	FLEXT_ASSERT(type   != NULL);
	FLEXT_ASSERT(domain != NULL);

	start = new unsigned char[rdlen];
	FLEXT_ASSERT(start != NULL);
	memcpy(start, rdata, rdlen);

	end = start + rdlen;
	cursor = start;
	if ((*cursor == 0) || (*cursor >= 64)) goto exitWithError;
	cursor += 1 + *cursor;                                       // Move to the start of the second DNS label.
	if (cursor >= end) goto exitWithError;
	if ((*cursor == 0) || (*cursor >= 64)) goto exitWithError;
	cursor += 1 + *cursor;                                       // Move to the start of the thrid DNS label.
	if (cursor >= end) goto exitWithError;
	
	/* Take everything from start of third DNS label until end of DNS name and call that the "domain". */
	if (conv_domain2str((const domainname *)cursor, domain) == NULL) goto exitWithError;
	*cursor = 0;                                                 // Set the length byte of the third label to zero.

	/* Take the first two DNS labels and call that the "type". */
	if (conv_domain2str((const domainname *)start, type) == NULL) goto exitWithError;
	delete[] start;
	return true;

exitWithError:
	delete[] start;
	return false;
}

////////////////////////////////////////////////

Symbol Worker::sym_error,Worker::sym_add,Worker::sym_remove;

Base::Workers *Base::newworkers = NULL;
flext::ThrCond Base::cond;

typedef std::set<Base *> ObjSet;
static ObjSet objects;

Base::Base() 
{
	AddInAnything("messages");
    objects.insert(this);
}

Base::~Base() 
{
    objects.erase(this);
	Install(NULL);
}

// can be called from a secondary thread
void Worker::OnError(DNSServiceErrorType error)
{
	const char *errtxt;
	switch(error) { 
		case kDNSServiceErr_NoError: errtxt = "NoError"; break; 
		case kDNSServiceErr_Unknown: errtxt = "Unknown"; break;
		case kDNSServiceErr_NoSuchName: errtxt = "NoSuchName"; break;
		case kDNSServiceErr_NoMemory: errtxt = "NoMemory"; break;
		case kDNSServiceErr_BadParam: errtxt = "BadParam"; break;
		case kDNSServiceErr_BadReference: errtxt = "BadReference"; break;
		case kDNSServiceErr_BadState: errtxt = "BadState"; break;
		case kDNSServiceErr_BadFlags: errtxt = "BadFlags"; break;
		case kDNSServiceErr_Unsupported: errtxt = "Unsupported"; break;
		case kDNSServiceErr_NotInitialized: errtxt = "NotInitialized"; break;
		case kDNSServiceErr_AlreadyRegistered: errtxt = "AlreadyRegistered"; break;
		case kDNSServiceErr_NameConflict: errtxt = "NameConflict"; break;
		case kDNSServiceErr_Invalid: errtxt = "Invalid"; break;
		case kDNSServiceErr_Firewall: errtxt = "Firewall"; break;
		case kDNSServiceErr_Incompatible: errtxt = "Incompatible"; break;
		case kDNSServiceErr_BadInterfaceIndex: errtxt = "BadInterfaceIndex"; break;
		case kDNSServiceErr_Refused: errtxt = "Refused"; break;
		case kDNSServiceErr_NoSuchRecord: errtxt = "NoSuchRecord"; break;
		case kDNSServiceErr_NoAuth: errtxt = "NoAuth"; break;
		case kDNSServiceErr_NoSuchKey: errtxt = "NoSuchKey"; break;
		case kDNSServiceErr_NATTraversal: errtxt = "NATTraversal"; break;
		case kDNSServiceErr_DoubleNAT: errtxt = "DoubleNAT"; break;
		case kDNSServiceErr_BadTime: errtxt = "BadTime"; break;
		default: errtxt = "?";
	};  

	t_atom at; 
	SetString(at,errtxt);
	Message(sym_error,1,&at);
}

void Base::Install(Worker *w)
{
    if(worker)
        worker->shouldexit = true;

    worker.reset(w);

    if(worker) {
        FLEXT_ASSERT(newworkers);
	    newworkers->Put(worker);
	    // wake up worker thread....
        cond.Signal();
    }
}

void Base::threadfun(thr_params *)
{
	FLEXT_ASSERT(newworkers);

    typedef std::set<WorkerPtr> WorkerSet;
    WorkerSet curworkers;

    for(;;) {
        // add new workers
        while(UNLIKELY(newworkers->Avail())) {
            // we ought to be the only reader!
            WorkerPtr w(newworkers->Get());
            if(LIKELY(w->Init()))
                curworkers.insert(w);
            // else abandon worker
        }

	    fd_set readfds;
	    int maxfds = -1;
	    FD_ZERO(&readfds);
        
        for(WorkerSet::iterator it = curworkers.begin(); it != curworkers.end(); ) {
		    WorkerPtr w(*it);
            WorkerSet::iterator it1 = it; ++it1;

            if(UNLIKELY(w->shouldexit))
                curworkers.erase(it);
            else {
                FLEXT_ASSERT(w->client && w->fd >= 0);

                FD_SET(w->fd,&readfds);
			    if(w->fd > maxfds) maxfds = w->fd;
            }

            it = it1;
	    }

        // all workers are in local myworkers now, none in curworkers

	    if(maxfds >= 0) {
		    timeval tv; 
		    tv.tv_sec = tv.tv_usec = 0; // don't block
		    int result = select(maxfds+1,&readfds,NULL,NULL,&tv);
		    if(result > 0) {
                for(WorkerSet::iterator it = curworkers.begin(); it != curworkers.end(); ++it) {
                    WorkerPtr w(*it);
                    FLEXT_ASSERT(w->client && w->fd >= 0);

                    // let's see if worker has been selected
				    if(/*w->fd >= 0 &&*/ FD_ISSET(w->fd,&readfds)) {
					    DNSServiceErrorType err = DNSServiceProcessResult(w->client);
                        if(UNLIKELY(err)) {
                            // selected and failed -> abandon worker and post error
						    post("DNSServiceProcessResult call failed: %i",err);

                            // delete failing worker on next round
                            w->shouldexit = true;
                        }
				    }
			    }
		    }
	    }

        cond.TimedWait(0.01);
    }
}

#ifdef PD_DEVEL_VERSION
t_int Base::idlefun(t_int* argv)
{
#else
static flext::Timer *idleclk = NULL;
void Base::idlefun(void *)
{
#endif
    for(ObjSet::const_iterator it = objects.begin(); it != objects.end(); ++it)
        (*it)->CbIdle();
#ifdef PD_DEVEL_VERSION
    return 2;
#endif
}

bool Base::CbIdle()
{
    if(worker) {
        // send waiting responses
        while(worker->messages.Avail())  // it's important that we are the only message reader...
            ToOutAnything(GetOutAttr(),worker->messages.Get());
    }
    return false;
}

void Base::Setup(t_classid)
{
	if(!newworkers) {
        Worker::sym_error = MakeSymbol("error");
		Worker::sym_add = MakeSymbol("add");
		Worker::sym_remove = MakeSymbol("remove");

		newworkers = new Workers;

#ifdef PD_DEVEL_VERSION
		sys_callback(idlefun,NULL,0);
#else
        idleclk = new flext::Timer;
        idleclk->SetCallback(idlefun);
        idleclk->Periodic(0.001);
#endif

        // start file helper thread
        LaunchThread(threadfun,NULL);
	}
}

////////////////////////////////////////////////

static void main()
{
	flext::post("---------------------------------------");
	flext::post("zconf - zeroconfig objects");
    flext::post("version " ZCONF_VERSION " (c)2006,2008 Thomas Grill");
#ifdef FLEXT_DEBUG
    flext::post("");
    flext::post("DEBUG BUILD - " __DATE__ " " __TIME__);
#endif
	flext::post("---------------------------------------");

	// call the objects' setup routines
	FLEXT_SETUP(Domains);
	FLEXT_SETUP(Browse);
	FLEXT_SETUP(Service);
	FLEXT_SETUP(Resolve);
	FLEXT_SETUP(Meta);
}

} // namespace

// setup the library
FLEXT_LIB_SETUP(zconf,zconf::main)

