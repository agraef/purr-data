/* 
flext tutorial - buffer 1 

Copyright (c) 2003-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of a simple object doing some basic buffer operation
*/


// IMPORTANT: enable attribute processing (specify before inclusion of flext headers!)
// For clarity, this is done here, but you'd better specify it as a compiler definition
// FLEXT_ATTRIBUTES must be 0 or 1, 
#define FLEXT_ATTRIBUTES 1


// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


// define the class that stands for a pd/Max object

class buffer1:
	// inherit from basic flext class
	public flext_base
{
	// obligatory flext header (class name,base class name) featuring a setup function
	FLEXT_HEADER_S(buffer1,flext_base,setup)
 
public:
	// constructor with a variable argument list
	buffer1(int argc,const t_atom *argv);

protected:
	const t_symbol *bufname;
	buffer *buf;

	// set new buffer (or none if name omitted)
	void m_set(int argc,const t_atom *argv);  

	// get buffer name
	void mg_buf(AtomList &lst) const;
	// set buffer name (simply reuse m_set method)
	inline void ms_buf(const AtomList &lst) { m_set(lst.Count(),lst.Atoms()); }

	// get buffer channels
	inline void mg_chns(int &chns) { chns = Check()?buf->Channels():0; }

	// get buffer length in frames
	inline void mg_frames(int &frames) { frames = Check()?buf->Frames():0; }
	// set buffer length in frames
	inline void ms_frames(int frames) { if(Check()) buf->Frames(frames); }

	// get sample (index channel)
	void m_peek(int argc,const t_atom *argv);
	// set sample (index value channel)
	void m_poke(int argc,const t_atom *argv);

	// delete eventual existing buffer		
	void Clear();
	
	// check and eventually update buffer reference (return true if valid)		
	bool Check();
	
private:
	static void setup(t_classid c);

	FLEXT_CALLBACK_V(m_set)  // wrapper for method m_set (with variable argument list)
	FLEXT_CALLBACK_V(m_peek)  // wrapper for method m_peek (with variable argument list)
	FLEXT_CALLBACK_V(m_poke)  // wrapper for method m_poke (with variable argument list)
	
	FLEXT_CALLVAR_V(mg_buf,ms_buf) // wrappers for attribute getter/setter mg_buffer/ms_buffer (with variable argument list)	
	FLEXT_CALLGET_I(mg_chns) // wrappers for attribute getter mg_chns (with integer arguments)	
	FLEXT_CALLVAR_I(mg_frames,ms_frames) // wrappers for attribute getter/setter mg_frames/ms_frames (with integer arguments)	
};

// instantiate the class
FLEXT_NEW_V("buffer1",buffer1)


void buffer1::setup(t_classid c)
{
	// register methods and attributes
	
	FLEXT_CADDMETHOD_(c,0,"set",m_set);  // register method "set" for inlet 0
	FLEXT_CADDMETHOD_(c,0,"peek",m_peek);  // register method "peek" for inlet 0
	FLEXT_CADDMETHOD_(c,0,"poke",m_poke);  // register method "poke" for inlet 0

	FLEXT_CADDATTR_VAR(c,"buffer",mg_buf,ms_buf);  // register attribute "buffer" 
	FLEXT_CADDATTR_GET(c,"channels",mg_chns);  // register attribute "channels" 
	FLEXT_CADDATTR_VAR(c,"frames",mg_frames,ms_frames);  // register attribute "frames" 
}


buffer1::buffer1(int argc,const t_atom *argv):
	// clear buffer
	buf(NULL),bufname(NULL)
{ 
	// define inlets:
	// first inlet must always be of type anything (or signal for dsp objects)
	AddInAnything("message inlet");  // add one inlet for any message
	
	// peek outlet
	AddOutFloat("peek value outlet");
	
	// set buffer according to creation arguments
	m_set(argc,argv);
}	
 

void buffer1::Clear() 
{
	if(buf) { 
		delete buf; 
		buf = NULL; bufname = NULL;
	}
}

bool buffer1::Check() 
{
	if(!buf || !buf->Valid()) {
		post("%s (%s) - no valid buffer defined",thisName(),GetString(thisTag()));
		// return zero length
		return false;
	} 
	else {
		if(buf->Update()) {
			// buffer parameters have been updated
			if(buf->Valid()) {
				post("%s (%s) - updated buffer reference",thisName(),GetString(thisTag()));
				return true;
			}
			else {
				post("%s (%s) - buffer has become invalid",thisName(),GetString(thisTag()));
				return false;
			}
		}
		else
			return true;		
	}
}

void buffer1::m_set(int argc,const t_atom *argv)
{
	if(argc == 0) {
		// argument list is empty

		// clear existing buffer
		Clear();		
	}
	else if(argc == 1 && IsSymbol(argv[0])) {
		// one symbol given as argument
		
		// clear existing buffer
		Clear();
		// save buffer name
		bufname = GetSymbol(argv[0]);
		// make new reference to system buffer object
		buf = new buffer(bufname);
		
		if(!buf->Ok()) {
			post("%s (%s) - warning: buffer is currently not valid!",thisName(),GetString(thisTag()));
		}
	}
	else {
		// invalid argument list, leave buffer as is but issue error message to console
		post("%s (%s) - message argument must be a symbol (or left blank)",thisName(),GetString(thisTag()));
	}
}

void buffer1::mg_buf(AtomList &lst) const
{
	if(buf) {
		// buffer exists: return buffer name
		lst(1); SetSymbol(lst[0],bufname);
	}
	else 
		// no buffer: set empty list
		lst(0);
}


void buffer1::m_poke(int argc,const t_atom *argv)
{
	// if buffer is invalid bail out
	if(!Check()) return;
	
	bool ok = true;
	int ix,chn = 0;
	float val;

	if(argc == 3) {
		if(CanbeInt(argv[2]))
			// get channel index
			chn = GetAInt(argv[2]);
		else
			ok = false;
	}

	if(ok && (argc == 2 || argc == 3) && CanbeInt(argv[0]) && CanbeFloat(argv[1]))	{
		// get frame index
		ix = GetAInt(argv[0]);
		// get value
		val = GetAFloat(argv[1]);
	}
	else
		ok = false;
		
	if(ok) {
		// correct syntax, set sample and trigger display
		(*buf)[ix] = val;
		buf->Dirty(true);
	}
	else
		post("%s (%s) - syntax error - use \"poke index value [channel]\"",thisName(),GetString(thisTag()));
}

void buffer1::m_peek(int argc,const t_atom *argv)
{
	// if buffer is invalid bail out
	if(!Check()) return;
	
	bool ok = true;
	int ix,chn = 0;

	if(argc == 2) {
		if(CanbeInt(argv[1]))
			// get channel index
			chn = GetAInt(argv[1]);
		else
			ok = false;
	}

	if(ok && (argc == 1 || argc == 2) && CanbeInt(argv[0])) {
		// get frame index
		ix = GetAInt(argv[0]);
	}
	else
		ok = false;
		
	if(ok)
		// correct syntax, output value 
		ToOutFloat(0,(*buf)[ix]);
	else
		post("%s (%s) - syntax error - use \"peek index [channel]\"",thisName(),GetString(thisTag()));
}




