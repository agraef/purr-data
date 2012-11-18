/*************************************************************
 *
 *    streaming external for PD
 *
 * File: streamogg.cpp
 *
 * Description: Implementation of the streamer class for OGG/vorbis
 *
 * Author: Thomas Grill (t.grill@gmx.net)
 *
 *************************************************************/

#include "streamogg.h"


// explicit definition of report functions
extern "C" {
	extern void	post(char *fmt, ...);
	extern void error(char *fmt, ...);
}


StreamOGG::StreamOGG():
	ov_inf(NULL),ov_comm(NULL)
{
	// set OGG callbacks
	callbacks.read_func = read_func;
	callbacks.seek_func = seek_func;
	callbacks.close_func = close_func;
	callbacks.tell_func = tell_func;
}

void StreamOGG::Reset()
{
	Stream::Reset();
	ov_inf = NULL;
	ov_comm = NULL;
}

std::string StreamOGG::getTag(const char *tag) const
{
	const char *c = NULL;
	if(ov_comm) c = vorbis_comment_query(ov_comm,const_cast<char *>(tag),0);
	return c?c:"";
}


#define MAXINITTRIES 5

bool StreamOGG::WorkInit()
{
	bool ok = true;

	// read in enough data to represent the OGG header
	char hdrbuf[8500];
	int i,need = sizeof hdrbuf;
	for(i = MAXINITTRIES; i > 0 && need > 0; ) {
		int n = ReadChunk(hdrbuf,need,true);
		if(n) 
			need -= n;
		else {
			--i;
			if(debug) post("Try to init again (%i)....",i);
		}
	}
	if(!i) ok = false;
		
	// got n bytes into fifo

	if(ok && ov_open_callbacks(this,&ov_file,hdrbuf,sizeof hdrbuf,callbacks) < 0 ) {
		post("Header format error");
		ok = false; 
	}
	else {
		ov_inf = ov_info(&ov_file,-1);
		ov_comm = ov_comment(&ov_file,-1);
	}

	return ok;
}

int StreamOGG::DataRead(int frames)
{
	int current_section;
	return ov_read_float(&ov_file,&bufs,frames,&current_section);
}

size_t StreamOGG::read_func(void *ptr, size_t size, size_t nmemb, void *datasource) 
{
	StreamOGG *str = (StreamOGG *)datasource;

	if(!str->isInitializing()) pthread_mutex_lock(&str->mutex);

	int ret = str->encoded.Read(size*nmemb,(char *)ptr);

	if(str->debug) post("read %i of %i bytes from fifo",ret,size*nmemb);

	if(!str->isInitializing()) pthread_mutex_unlock(&str->mutex);

	return ret;
}

int StreamOGG::close_func(void *datasource) 
{
	post("Closing stream");

	StreamOGG *str = (StreamOGG *)datasource;
	pthread_mutex_lock(&str->mutex);
	str->Reset();
	pthread_mutex_unlock(&str->mutex);
	return 0;
}

