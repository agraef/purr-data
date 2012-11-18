/*************************************************************
 *
 *    streaming external for PD
 *
 * File: streamogg.h
 *
 * Description: Declaration of the streamer class for OGG/vorbis
 *
 * Author: Thomas Grill (t.grill@gmx.net)
 *
 *************************************************************/

#ifndef __STREAMOGG_H
#define __STREAMOGG_H

#include "stream.h"

extern "C" {
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
}


//! Class representing an OGG stream.
class StreamOGG:
	public Stream
{
public:
	StreamOGG();

	//! Get named tag of stream
	virtual std::string getTag(const char *tag) const;

	//! Get number of stream channels
	virtual int getChannels() const { return ov_inf?ov_inf->channels:0; }
	//! Get stream sample rate
	virtual float getSamplerate() const { return ov_inf?(float)ov_inf->rate:0; }
	//! Get nominal stream bit rate
	virtual float getBitrate() const { return ov_inf?(float)ov_inf->bitrate_nominal:0; }

protected:

	//! Reset encoder state (disconnect and clear FIFOs)
	virtual void Reset();

	//! Init decoder
	virtual bool WorkInit();
	//! Decode data to channel buffers
	virtual int DataRead(int frames);

	// OGG/Vorbis data
	OggVorbis_File ov_file;
	ov_callbacks callbacks;
	vorbis_info *ov_inf;
    vorbis_comment *ov_comm;

	// OGG callbacks
	static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource);
	static int close_func(void *datasource);
	static int seek_func(void *datasource, ogg_int64_t offset, int whence) { return -1; }
	static long tell_func(void *datasource) { return -1; }
};

#endif // __STREAMOGG_H
