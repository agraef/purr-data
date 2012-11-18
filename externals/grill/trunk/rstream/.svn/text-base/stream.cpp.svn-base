#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
        #include <ctype.h>
	#define Sleep(ms) usleep((ms)*1000)
#endif

#include "stream.h"

#include "resample.h"
#ifdef HAVE_SRC
#include "resamplesrc.h"
#endif

// default size of encoded data fifo
#define ENCSIZE 10000
// default size for encoding chunk
#define ENCCHUNK 500
// default ratio for fifo filling
#define ENCTHRESH 0.95f

// additional buffer frames for resampling algorithm
#define DECMORE 100


// relative thread priority (-2...0)
#define THRPRIOR 0

// default time grain to wait on error (ms)
#define WAITGRAIN 50
// default time until reconnecting (ms)
#define WAITRECONNECT 200


// explicit definition of report functions
extern "C" {
	extern void	post(char *fmt, ...);
	extern void error(char *fmt, ...);
}

Stream::Stream(Callback cb,void *dt):
	encoded(ENCSIZE),encchunk(ENCCHUNK),encthresh(ENCTHRESH),
	waittime(0),waitgrain(WAITGRAIN),waitreconnect(WAITRECONNECT),
	file(-1),
	exit(false),state(ST_IDLE),debug(0),
	bufch(0),bufs(NULL),decoded(NULL)
    , rs(NULL)
    ,callback(cb),data(dt)
{
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	pthread_create(&thrid,&attr,thr_func,this);
	pthread_attr_destroy(&attr);
}

Stream::~Stream()
{
	// cause thread to exit
	exit = true;
	pthread_cond_signal(&cond);
	if(pthread_join(thrid,NULL) != 0) error("join failed");

	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);

	Reset();

	if(decoded) delete[] decoded;
    if(rs) delete rs;
}

void Stream::Reset()
{
	if(file >= 0) { 
        Disconnect(file); 
        file = -1; 
		if(debug) post("connection closed");
    }
	encoded.Clear();

    if(rs) { delete rs; rs = NULL; }
}

void Stream::ResetHost()
{
	hostname = mountpt = ""; 
    port = -1;
}

bool Stream::doInit(const char *url)
{
	if(isInitializing()) {
		// this is not completely clean (should be within the lock) but otherwise the 
		// caller would have to wait for the thread lock

		post("Still initializing %s/%s:%i",hostname.c_str(),mountpt.c_str(),port);
		return false;
	}

	bool ok = true;

	pthread_mutex_lock(&mutex);

	// close open file
	Reset();

	// try to set host name, mount point, port number
	if(ok) {
		char *err = "Invalid URL";
		try { ok = SetURL(url); }
		catch(char *tx) { err = tx;	ok = false;	}
		catch(...) { ok = false; }

		if(!ok) { post(err); ResetHost(); }
	}

	if(ok) {
		state = ST_INIT;
		pthread_cond_signal(&cond);
	}

	pthread_mutex_unlock(&mutex);

	// let the thread worker do the rest

	return ok;
}

bool Stream::doExit()
{
	pthread_mutex_lock(&mutex);

	Reset();
	ResetHost();
	state = ST_IDLE;

	pthread_mutex_unlock(&mutex);

	return true;
}

/*! Get sample frames
	\param buf		pointer array to channel buffers
	\param frames	number of maximum frames to get
	\return			frames put into buffers
*/
int Stream::doGet(int ch,float *const *buf,int frames,float sr)
{
	if(!(ch > 0 && frames >= 0 && sr > 0)) 
        return 0;

	if(isOk() && !isInitializing()) {
	    // signal thread worker
        doWakeup();

		// get frames
    	double ratio = sr/getSamplerate();
		int frneed = (int)(frames/ratio)+DECMORE;  // number of frames to read from decoder fifo

		if(decoded[0].Size() < frneed) {
			// fifos are too small -> resize them (while keeping their contents)
			for(int i = 0; i < bufch; ++i) decoded[i].Resize(frneed,true);
		}

		// how many frames do we need to get from decoder?
		int frread = frneed-decoded[0].Have();

		int ret = isWaiting()?0:DataRead(frread);

		if(ret <= 0) {
            if(ret == -2) {
                if(debug) post("need to reinit");
                state = ST_RECONNECT;
                doWakeup();
            }
            else if(ret < 0)
                if(debug) post("error decoding data");

			// clear output
			for(int c = 0; c < ch; ++c)
				memset(buf[c],0,frames*sizeof *buf[c]);
			return 0;
		}
		else {
			// how many channels do we really need for output?
			// this should be set elsewhere, because we can't change anyway!!! 
			// (SRC_STATE for dangling channels would be incorrect)
			int cmin = bufch;
			if(ch < cmin) cmin = ch;
            ASSERT(cmin);

			// write data to fifo
            int wrote = -1;
			for(int i = 0; i < cmin; ++i) {
				int w = decoded[i].Write(ret,bufs[i]);
                ASSERT(wrote < 0 || w == wrote);
                wrote = w;
			}
            ASSERT(wrote >= 0);
    		if(wrote < ret) post("fifo overflow");

            int got = 0;
            if(rs) {
                got = rs->Do(cmin,decoded,buf,frames,ratio);
                if(got < frames) {
				    schedWait();
				    if(debug) post("fifo underrun");
                }
            }
            else
                if(debug) error("Resampling not supported");

            if(got < frames) {
    	        // zero out remainder
                for(int c = 0; c < cmin; ++c)
    			    memset(buf[c]+got,0,(frames-got)*sizeof(*buf[c]));
            }
			// zero remaining channels
			for(int c = cmin; c < ch; ++c)
				memset(buf[c],0,frames*sizeof *buf[c]);

			return ret;
		}
	}
	else {
		for(int c = 0; c < ch; ++c)
			memset(buf[c],0,frames*sizeof *buf[c]);
		return 0;
	}
}

#define MAXZEROES 5

/*!
	\param chunk	amount of data to read
	\param unlock	unlock mutex
*/
int Stream::ReadChunk(int chunk,bool unlock)
{
	if(chunk <= 0 || file < 0) return 0;

	bool ok = true;
	char tmp[1024];
	int n = 0,errcnt = 0;
	while(ok) {
		int c = chunk-n;
		if(c <= 0) break; // read enough data
		if(c > sizeof tmp) c = sizeof tmp;
		SOCKET fd = file;

		if(unlock) pthread_mutex_unlock(&mutex);

		int ret = Read(fd, tmp, c);

		if(unlock) pthread_mutex_lock(&mutex);

		if(ret < 0 || (!ret && ++errcnt == MAXZEROES)) {
			if(debug) post("Receive error");
			ok = false;
		}
		else if(ret > 0) {
			if(debug >= 2) post("read %i bytes",ret);
			errcnt = 0;
			encoded.Write(ret,tmp);
			n += ret;
		}
	}
	return n;
}

/*!
	\param buf		data buffer
	\param chunk	amount of data to read
	\param unlock	unlock mutex
*/
int Stream::ReadChunk(char *buf,int chunk,bool unlock)
{
	if(chunk <= 0) return 0;

	bool ok = true;
	int n = 0,errcnt = 0;
	while(ok && file >= 0) {
		int c = chunk-n;
		if(c <= 0) break; // read enough data
		SOCKET fd = file;

		// file should NOT have changed in the meantime, but...
		assert(fd >= 0);

		if(unlock) pthread_mutex_unlock(&mutex);

		int ret = Read(fd, buf+n, c);

		// file might change here!

		if(unlock) pthread_mutex_lock(&mutex);

		if(ret < 0 || (!ret && ++errcnt == MAXZEROES)) {
			if(debug) post("Receive error");
			ok = false;
		}
		else if(ret > 0) {
			if(debug >= 2) post("read %i bytes",ret);
			errcnt = 0;
			n += ret;
		}
	}
	return n;
}

#define MAXINITTRIES 5

/*! static pthreads thread function */
void *Stream::thr_func(void *th) 
{ 
	((Stream *)th)->Work(); 
	return NULL; 
}

/*! Thread worker - fill the fifo with socket data */
void Stream::Work()
{
	// lower thread priority
    if(THRPRIOR) {
	    struct sched_param parm;
	    int policy;
	    if(pthread_getschedparam(pthread_self(),&policy,&parm) >= 0) {
            int minprio = sched_get_priority_min(policy);

            if(debug) post("priority was %i (min = %i)",parm.sched_priority,minprio);

            parm.sched_priority += THRPRIOR;

    		if(parm.sched_priority < minprio) parm.sched_priority = minprio;
            pthread_setschedparam(pthread_self(),policy,&parm);
        }

	    if(pthread_getschedparam(pthread_self(),&policy,&parm) >= 0) {
            if(debug) post("priority set to %i",parm.sched_priority);
        }
    }

	while(!exit) {
		pthread_mutex_lock(&mutex);

		bool wait = true;

		if(!hostname.length() || !mountpt.length() || port < 0) 
            {}
		else if(state == ST_INIT || state == ST_RECONNECT) {
			// initialize!

            clearWait(); // we might have already been waiting
            wait = false; // don't wait after initialization

        	Reset(); // clear all

            bool ok = false;
            try {
			    file = Connect(hostname.c_str(),mountpt.c_str(),port);
                if(file >= 0) ok = true;

                if(ok && debug) post("connection to %s:%i/%s opened",hostname.c_str(),port,mountpt.c_str());
            }
            catch(char *txt) {
                if(debug) post("error opening stream: %s",txt);
                ok = false;
            }
            catch(...) {
                if(debug) post("unknown error opening stream");
                ok = false;
            }

			// initialize decoder
			if(ok) ok = WorkInit();

            if(ok) {
                // (re)allocate buffers and resampler

                if(decoded) delete[] decoded;
                if(rs) delete rs;

		        bufch = getChannels();
                decoded = new Fifo<float>[bufch];

        #ifdef HAVE_SRC
                rs = new ResampleSRC(bufch);
        #else
                rs = new Resample(bufch);
        #endif
                // if ok, starting filling buffer
				state = ST_FILL;
            }
            else {
				Reset();

				if(state == ST_INIT) 
                    state = ST_IDLE;
		        else if(state == ST_RECONNECT) {
			        if(debug) post("reconnecting");
                    schedWait();
		        }
                else
                    ASSERT(false); // should not happen
			}
		}
		else if(state == ST_FILL) {
            if(debug) post("try to fill buffer");

			// try to fill buffer
			int i,lim = (int)(encoded.Size()*encthresh);
			for(i = MAXINITTRIES; i > 0 && encoded.Have() < lim; ) {
				int n = ReadChunk(encoded.Free(),true);
				if(!n) --i;
			}

            if(i) {
                if(debug) post("filled buffer");
                // go on with normal processing
                state = ST_PROCESS;
				clearWait();
            }
            else
                // stay filling
                schedWait();
        }
        else if(isOk()) {
			SOCKET fd = file;
			int chunk = encoded.Free();
			if(chunk > encchunk) chunk = encchunk;
			
			if(chunk) {
				int n = ReadChunk(chunk,true);

				if(n == 0) {
					if(debug) post("error receiving data");
                    schedWait();
				}
				else
					// reset error state
					state = ST_PROCESS;
			}

			if(encoded.Have() < encoded.Size()*encthresh) 
				// immediately get the next chunk
				wait = false;
		}

		if(debug >= 2 && encoded.Free()) {
			post("fifo: sz/fill = %5i/%3.0f%%",encoded.Size(),(float)encoded.Have()/encoded.Size()*100);
		}

        if(waittime) {
            wait = false;
    		if(debug) post("waiting for data");
            Sleep(waitgrain);
            waittime += waitgrain;
			if(waittime > waitreconnect) {
				state = ST_RECONNECT;
			}
        }

		if(wait) pthread_cond_wait(&cond,&mutex);

		pthread_mutex_unlock(&mutex);
	}

	state = ST_FINISHED;
}

bool Stream::SetURL(const char *url)
{
	char *p = (char *)url;

	// strip prefixes
	if(!strncmp(p, "http://", 7)) p += 7;
	if(!strncmp(p, "ftp://", 6)) p += 6;

	char *hostptr = p; // points to host name

	char *pathptr = strchr(hostptr,'/');
	if(pathptr) 
		// / found -> skip /
		++pathptr;
	else
		// no / found!! ILLEGAL
		throw "URL path not found";

	// get port number
	int portno;
	char *portptr = strchr(hostptr,':');
	if(portptr && portptr < pathptr) {
		portptr++;
		int sl = (int)(pathptr-portptr-1);
		char *p0 = new char[sl+1];
		ASSERT(p0);
		strncpy(p0,portptr,sl);
		p0[sl] = 0;

		for(p = p0; *p && isdigit(*p); p++) ;
		*p = 0;

		// convert port from string to int
		portno = (int)strtol(p0, NULL, 10);
		delete[] p0;
	}
	else
        // default
		portno = 8000;

	// assign found things to function parameters
	hostname = std::string(hostptr,(portptr?portptr:pathptr)-1-hostptr);
	mountpt = pathptr;
	port = portno;

	return true;
}
