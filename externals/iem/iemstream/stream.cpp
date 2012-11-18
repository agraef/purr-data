/*************************************************************
 *
 *    streaming external for PD
 *
 * File: stream.cpp
 *
 * Description: Implementation of the streamer class
 *
 * Author: Thomas Grill (t.grill@gmx.net)
 *
 *************************************************************/

#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
        #include <ctype.h>
	#define Sleep(ms) usleep((ms)*1000)
#endif

#include "stream.h"


// default size of encoded data fifo
#define ENCSIZE 10000
// default size for encoding chunk
#define ENCCHUNK 500
// default ratio for fifo filling
#define ENCTHRESH 0.95f

// additional buffer frames for resampling algorithm
#define DECMORE 100


// relative thread priority (-2...0)
#define THRPRIOR -1

// default time grain to wait on error (ms)
#define WAITGRAIN 100
// default time until reconnecting (ms)
#define WAITRECONNECT 3000


// explicit definition of report functions
extern "C" {
	extern void	post(char *fmt, ...);
	extern void error(char *fmt, ...);
}

Stream::Stream():
	encoded(ENCSIZE),encchunk(ENCCHUNK),encthresh(ENCTHRESH),
	waitgrain(WAITGRAIN),waitreconnect(WAITRECONNECT), //waitthread(WAITTHREAD),
	file(-1),
	exit(false),state(ST_IDLE),debug(false),
	bufch(0),bufs(NULL),decoded(NULL),
	src_channels(0),src_factor(1),src_state(NULL)
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
	if(pthread_join(thrid,NULL) != 0) post("join failed");

	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);

	Reset();

	if(bufs) delete[] bufs;
	if(decoded) delete[] decoded;
	if(src_state) {
		for(int i = 0; i < bufch; ++i) src_delete(src_state[i]);
		delete[] src_state;
	}
}

void Stream::Reset()
{
	if(file >= 0) { Disconnect(file); file = -1; }
	encoded.Clear();

	if(src_state) {
		for(int i = 0; i < bufch; ++i) {
			src_reset(src_state[i]);
			decoded[i].Clear();
		}
	}
}

void Stream::ResetHost()
{
	hostname = mountpt = ""; port = -1;
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
	ASSERT(ch > 0 && frames >= 0 && sr > 0);

	if(isOk() && !isInitializing()) {
		// signal thread worker
		pthread_cond_signal(&cond);

		// check/(re)allocate buffers

		int strch = getChannels();
		if(bufs && bufch < strch) { 
			delete[] decoded;
			for(int i = 0; i < bufch; ++i) src_delete(src_state[i]);
			delete[] src_state;
			delete[] bufs;	bufs = NULL; 
		}

		if(!bufs) {
			if(bufch < strch) bufch = strch;
			bufs = new float *[bufch];
			decoded = new Fifo<float>[bufch];

			src_state = new SRC_STATE *[bufch];
			for(int i = 0; i < bufch; ++i) {
				int error;
				src_state[i] = src_new(SRC_ZERO_ORDER_HOLD,1,&error);
				if(!src_state[i]) post("src init error %i",error);
			}
		}

		// get frames

		float ratio = sr/getSamplerate();
		int frneed = (int)(frames/ratio)+DECMORE;  // number of frames to read from decoder fifo

		if(decoded[0].Size() < frneed) {
			// fifos are too small -> resize them (while keeping their contents)
			for(int i = 0; i < bufch; ++i) decoded[i].Resize(frneed,true);
		}

		// how many frames do we need to get from decoder?
		int frread = frneed-decoded[0].Have();

		int ret = state == ST_WAIT?0:DataRead(frread);

		if(ret < 0) {
			if(debug) post("read error");
			// clear output
			for(int c = 0; c < ch; ++c)
				memset(buf[c],0,frames*sizeof *buf[c]);
			return 0;
		}
		else {
			// how many channels do we really need for output?
			// this should be set elsewhere, because we can't change anyway!!! 
			// (SRC_STATE for dangling channels would be incorrect)
			int cmin = strch;
			if(ch < cmin) cmin = ch;

			// write data to fifo
			for(int i = 0; i < cmin; ++i) {
				int wr = decoded[i].Write(ret,bufs[i]);
				if(wr < ret) post("fifo overflow");
			}

//			state = ST_PROCESS;

			if(ratio == 1) {
				// no resampling necessary

				// hopefully all channel fifos advance uniformly.....
				for(int i = 0; i < cmin; ++i) {

					for(int got = 0; got < frames; ) {
						int cnt = frames-got;

						if(decoded[i].Have()) {
							got += decoded[i].Read(cnt,buf[i]+got);
						}
						else {
							state = ST_WAIT;
							if(debug) post("fifo underrun");

							// Buffer underrun!! -> zero output buffer
							memset(buf[i]+got,0,cnt*sizeof(*buf[i]));
							got += cnt;
						}
					}
				}
			}
			else 
			{
				SRC_DATA src_data;
				src_data.src_ratio = ratio;
				src_data.end_of_input = 0;

				// hopefully all channel fifos advance uniformly.....
				for(int i = 0; i < cmin; ++i) {
					src_set_ratio(src_state[i],ratio);

					for(int got = 0; got < frames; ) {
						src_data.data_out = buf[i]+got;
						src_data.output_frames = frames-got;

						if(decoded[i].Have()) {
							src_data.data_in = decoded[i].ReadPtr();
							src_data.input_frames = decoded[i].ReadSamples();

							int err = src_process(src_state[i],&src_data);
							if(err) post("src_process error %i",err);

							// advance buffer
							decoded[i].Read(src_data.input_frames_used,NULL);
						}
						else {
							state = ST_WAIT;
							if(debug) post("fifo underrun");

							// Buffer underrun!! -> zero output buffer
							memset(src_data.data_out,0,src_data.output_frames*sizeof(*src_data.data_out));
							src_data.output_frames_gen = src_data.output_frames;
						}
						got += src_data.output_frames_gen;
					}
				}
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
	if(chunk <= 0) return 0;

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
			if(debug) post("read %i bytes",ret);
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
	while(ok) {
		int c = chunk-n;
		if(c <= 0) break; // read enough data
		SOCKET fd = file;

		if(unlock) pthread_mutex_unlock(&mutex);

		int ret = Read(fd, buf+n, c);

		if(unlock) pthread_mutex_lock(&mutex);

		if(ret < 0 || (!ret && ++errcnt == MAXZEROES)) {
			if(debug) post("Receive error");
			ok = false;
		}
		else if(ret > 0) {
			if(debug) post("read %i bytes",ret);
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
	int waittime = 0;

	// lower thread priority
    {
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

		if(!hostname.length() || !mountpt.length() || port < 0) {}
		else
		if(state == ST_INIT || state == ST_RECONNECT) {
			// initialize!

			bool ok = true;

			try {
				file = Connect( hostname.c_str(),mountpt.c_str(),port);
			}
			catch(char *str) {
				if(state != ST_RECONNECT) post(str);
				ok = false;
			}
			catch(...) {
				post("Unknown error while connecting");
				ok = false;
			}

			// initialize decoder
			if(ok) ok = WorkInit();

			// try to fill buffer
			if(ok) {
				int i,lim = (int)(encoded.Size()*encthresh);
				for(i = MAXINITTRIES; i > 0 && encoded.Have() < lim; ) {
					int n = ReadChunk(encoded.Free(),true);
					if(!n) --i;
				}
				if(!i) ok = false;
			}

			if(!ok) {
				Reset();

				if(state == ST_INIT) state = ST_IDLE;
				// if reconnecting keep on doing that...
			}
			else {
				state = ST_PROCESS;
				waittime = 0;
			}
		}
		else if(isOk()) {
			SOCKET fd = file;
			int chunk = encoded.Free();
			if(chunk > encchunk) chunk = encchunk;
			
			if(chunk) {
				int n = ReadChunk(chunk,true);

				if(n == 0) {
					if(debug) post("error receiving data");
					state = ST_WAIT;
				}
				else
					// reset error state
					state = ST_PROCESS;
			}

			if(encoded.Have() < encoded.Size()*encthresh) 
				// immediately get the next chunk
				wait = false;
		}

		if(debug && encoded.Free()) {
			post("fifo: sz/fill = %5i/%3.0f%%",encoded.Size(),(float)encoded.Have()/encoded.Size()*100);
		}

		if(state == ST_WAIT) {
			if(debug) post("Wait for data");
			Sleep(waitgrain);
			waittime += waitgrain;
			if(waittime > waitreconnect) {
				if(debug) post("do reconnect");
				state = ST_RECONNECT;
			}
			wait = false;
		}
		else if(state == ST_RECONNECT) {
			if(debug) post("Reconnecting again");
			Sleep(waitgrain);
			wait = false;
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
		portno = 8000;

	// assign found things to function parameters
	hostname = std::string(hostptr,(portptr?portptr:pathptr)-1-hostptr);
	mountpt = pathptr;
	port = portno;

	return true;
}
