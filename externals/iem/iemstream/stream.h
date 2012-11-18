/*************************************************************
 *
 *    streaming external for PD
 *
 * File: stream.h
 *
 * Description: Declaration of the streamer class
 *
 * Author: Thomas Grill (t.grill@gmx.net)
 *
 *************************************************************/

#ifndef __STREAM_H
#define __STREAM_H

#include <pthread.h>
 
#include <assert.h>
#define ASSERT assert

#include <string>

#include <samplerate.h>

#include "fifo.h"
#include "socket.h"


/*! Class representing an abstract stream.
	\note Must be inherited for a special stream format
*/
class Stream
{
public:
	Stream();
	virtual ~Stream();

	//! Return true if in initializing state
	bool isInitializing() const { return state == ST_INIT || state == ST_RECONNECT; }
	//! Return true if socket is valid
	bool isOk() const { return file >= 0; }

	// Get/set size of encoder FIFO
	int getStreamBufSize() const { return encoded.Size(); }
	bool setStreamBufSize(int sz) { return encoded.Resize(sz,true); }

	// Get/set size of encoder chunk (amount of data written from the stream at once)
	int getStreamBufChunk() const { return encchunk; }
	void setStreamBufChunk(int sz) { if(encchunk > 0) encchunk = sz; }

	// Get/set buffer size ratio under which refilling is triggered (0...1)
	float getStreamBufThresh() const { return encthresh; }
	void setStreamBufThresh(float r) { if(encthresh > 0 && encthresh <= 1) encthresh = r; }

	//! Get ratio of filling (0...1)
	float getFilling() const { return (float)encoded.Have()/encoded.Size(); }

	//! name of stream
	virtual std::string getTag(const char *tag) const { return ""; }
	//! number of stream channels
	virtual int getChannels() const = 0;
	//! stream sample rate
	virtual float getSamplerate() const = 0;
	//! nominal stream bit rate
	virtual float getBitrate() const = 0;

	const std::string &getHostname() const { return hostname; }
	const std::string &getMountpoint() const { return mountpt; }
	int getPort() const { return port; }

	/*! Initialize stream */
	bool doInit(const char *url);

	/*! Disconnect from stream */
	bool doExit();

	//! Get a number of sample frames
	int doGet(int ch,float *const *buf,int frames,float sr);

	// Debug flag
	volatile bool debug;

protected:

	//! Reset encoder state (disconnect and clear FIFOs)
	virtual void Reset();
	//! Reset URL
	void ResetHost();

	//! Init decoder
	virtual bool WorkInit() = 0;
	//! Decode data to channel buffers
	virtual int DataRead(int frames) = 0;

	//! Read stream data to encoder FIFO
	int ReadChunk(int chunk,bool unlock);
	//! Read stream data to buffer
	int ReadChunk(char *buf,int chunk,bool unlock);

	//! Set hostname, mountpt, port
	bool SetURL(const char *url);

	std::string hostname,mountpt;
	int port;

	// --- FIFO for encoded stream data -----------------------

	int encchunk;		//! Size of data chunk to get from socket at once
	float encthresh;	//! Ratio of fifo filling to keep up to
	Fifo<char> encoded; //! Fifo for encoded stream data

	// --- low-level socket stuff ------------------------------

	//! stream socket
	volatile SOCKET file;

	//! Connect to stream
	static SOCKET Connect(const char *hostname,const char *mountpoint,int portno);
	//! Disonnect from stream
	static void Disconnect(SOCKET fd);
	//! Read data from stream
	static int Read(SOCKET fd,char *buf,int size,int timeout = 1000);

	// --- threading stuff -------------------------------------

	//! status type
	enum state_t {
		ST_IDLE,  // nothing to do
		ST_INIT,  // shall connect
		ST_PROCESS,  // do decoding
		ST_WAIT,  // wait a bit
		ST_RECONNECT,  // try to reconnect
		ST_FINISHED  // waiting for shutdown
	};

	volatile bool exit;  //! exit flag
	volatile state_t state;  //! decoder state 
	pthread_mutex_t mutex;  //! thread mutex
	pthread_cond_t cond;	//! thread conditional
	pthread_t thrid;  //! worker thread ID

	int waitgrain,waitreconnect;

	static void *thr_func(void *th);
	void Work();

	// --- channel buffers --------------------------------------

	int bufch;
	float **bufs;
	Fifo<float> *decoded;

	// --- SRC stuff --------------------------------------------

	int src_channels;
	double src_factor;
	SRC_STATE **src_state;
};

#endif // __STREAM_H
