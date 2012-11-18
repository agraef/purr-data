#define FLEXT_ATTRIBUTES 1

#include <flext.h>

// include stream class
#include "streamogg.h"


class rstream
    : public flext_dsp
{
    FLEXT_HEADER_S(rstream,flext_dsp,setup)

public:

    rstream(int c)
        : stream(NULL)
        , clkrate(0)
        , url(sym__)
        // default values
        , debug(0)
        , strbuf(10000),strchunk(500),strthresh(0.8)
        , waitgrain(50),waitreconnect(500)
    {
	    if(c == 0) 
            c = 2;
	    else
		    if(c < 1) throw "Channel count must be >= 1";

        AddInAnything();
        AddOutSignal(c);

        FLEXT_ADDTIMER(timer,CbTimer);
    }

    ~rstream()
    {
        m_disconnect();
    }

    void CbTimer(void *)
    {
        ToSysFloat(GetOutAttr(),stream?stream->getFilling():0);
    }

    virtual void CbSignal()
    {
        int c = CntOutSig(),rc;

        if(stream) {
            int sc = stream->getChannels();
            rc = c < sc?c:sc;
            if(rc) 
                stream->doGet(rc,OutSig(),Blocksize(),Samplerate());
            else
                stream->doWakeup();
        }
        else 
            rc = 0;

        // clear remaining channels
        while(rc < c) ZeroSamples(OutSig(rc++),Blocksize());
    }

    /*! Connect to stream
	    \param s	URL of stream
    */
    void m_connect(const t_symbol *s) 
    { 
        m_disconnect();
        FLEXT_ASSERT(!stream);

        url = s;
        stream = new StreamOGG;

        stream->debug = debug;
        stream->setStreamBufSize(strbuf);
        stream->setStreamBufChunk(strchunk);
        stream->setStreamBufThresh(strthresh);
        stream->setWaitGrain(waitgrain);
        stream->setWaitReconnect(waitreconnect);

        stream->doInit(GetString(s)); 
    }

    /*! Disconnect from stream */
    void m_disconnect() 
    { 
        if(stream) {
            // utmost care, as DSP thread accesses stream
            Stream *s = stream;
            stream = NULL;

            s->doExit(); 
            delete s;
            url = sym__;
        }
        else
            FLEXT_ASSERT(url == sym__);
    }

    void ms_url(const t_symbol *s) 
    {
        if(s == sym__)
            m_disconnect();
        else
            m_connect(s);
    }

    /*! Set/Clear debug mode 
	    \param f	0/1 ... debug mode off/on
    */
    void ms_debug(int d) { debug = d; if(stream) stream->debug = d; }

    /*! Set tick rate for buffer filling status
	    \param f	tick rate in ms
    */
    void ms_tick(float f)
    {
        if(f > 0) 
            timer.Periodic((clkrate = f)*0.001);
        else {
            clkrate = 0;
            timer.Reset();
        }
    }

    /*! Set decoder FIFO size */
    void ms_strbuf(int s) { strbuf = s; if(stream) stream->setStreamBufSize(s); }

    /*! Set decoder chunk size */
    void ms_strchunk(int s) { strchunk = s; if(stream) stream->setStreamBufChunk(s); }

    /*! Set decoder filling threshold ratio */
    void ms_strthresh(float f) { strthresh = f; if(stream) stream->setStreamBufThresh(f); }

    /*! Set wait grain (ms) */
    void ms_waitgrain(float w) { waitgrain = w; if(stream) stream->setWaitGrain(w); }

    /*! Set wait grain (ms) */
    void ms_waitreconnect(float w) { waitreconnect = w; if(stream) stream->setWaitReconnect(w); }

    /*! Output number of stream channels to dump outlet */
    void mg_channels(int &c) { c = stream?stream->getChannels():0; }

    /*! Output sample rate to dump outlet */
    void mg_srate(float &sr) { sr = stream?stream->getSamplerate():0; }

    /*! Output bitrate to dump outlet */
    void mg_brate(float &br) { br = stream?stream->getBitrate():0; }

    /*! Output a stream tag to dump outlet 
	    \param sym	name of tag to output
    */
    void m_tag(const t_symbol *sym)
    {
        if(!stream) return;
        t_atom a; SetString(a,stream->getTag(GetString(sym)).c_str());
        ToSysAnything(GetOutAttr(),sym_tag,1,&a);
    }

    void CbReport(int code,const char *txt)
    {
        post("%s: %i - %s",thisName(),code,txt);
    }

protected:

    FLEXT_CALLBACK_S(m_connect)
    FLEXT_CALLBACK(m_disconnect)
    FLEXT_CALLSET_S(ms_url)
    FLEXT_ATTRGET_S(url)
    FLEXT_CALLSET_I(ms_debug)
    FLEXT_ATTRGET_I(debug)
    FLEXT_CALLSET_F(ms_tick)
    FLEXT_ATTRGET_F(clkrate)
    FLEXT_CALLSET_I(ms_strbuf)
    FLEXT_ATTRGET_I(strbuf)
    FLEXT_CALLSET_I(ms_strchunk)
    FLEXT_ATTRGET_I(strchunk)
    FLEXT_CALLSET_F(ms_strthresh)
    FLEXT_ATTRGET_F(strthresh)
    FLEXT_CALLSET_F(ms_waitgrain)
    FLEXT_ATTRGET_F(waitgrain)
    FLEXT_CALLSET_F(ms_waitreconnect)
    FLEXT_ATTRGET_F(waitreconnect)
    FLEXT_CALLGET_I(mg_channels)
    FLEXT_CALLGET_F(mg_srate)
    FLEXT_CALLGET_F(mg_brate)
    FLEXT_CALLBACK_S(m_tag)

    FLEXT_CALLBACK_T(CbTimer)

    Timer timer;
    float clkrate;
    const t_symbol *url;
    int debug;
    int strbuf,strchunk;
    float strthresh;
    float waitgrain,waitreconnect;
    Stream *stream;

    static const t_symbol *sym_tag;

    static void callback(void *data,int code,const char *txt)
    {
        static_cast<rstream *>(data)->CbReport(code,txt);
    }

    static void setup(t_classid c)
    {
	    post("rstream~ (C)2006 Thomas Grill");

        FLEXT_CADDMETHOD_(c,0,"connect",m_connect);
        FLEXT_CADDMETHOD_(c,0,"disconnect",m_disconnect);
        FLEXT_CADDATTR_VAR(c,"url",url,ms_url);

        FLEXT_CADDATTR_VAR(c,"debug",debug,ms_debug);
        FLEXT_CADDATTR_VAR(c,"tick",clkrate,ms_tick);

        FLEXT_CADDATTR_VAR(c,"buffer",strbuf,ms_strbuf);
        FLEXT_CADDATTR_VAR(c,"chunk",strchunk,ms_strchunk);
        FLEXT_CADDATTR_VAR(c,"thresh",strthresh,ms_strthresh);

        FLEXT_CADDATTR_VAR(c,"waitgrain",waitgrain,ms_waitgrain);
        FLEXT_CADDATTR_VAR(c,"waitreconnect",waitreconnect,ms_waitreconnect);

        FLEXT_CADDATTR_GET(c,"channels",mg_channels);
        FLEXT_CADDATTR_GET(c,"srate",mg_srate);
        FLEXT_CADDATTR_GET(c,"brate",mg_brate);

        sym_tag = MakeSymbol("tag");
        FLEXT_CADDMETHOD_(c,0,sym_tag,m_tag);
    }
};

const t_symbol *rstream::sym_tag;

FLEXT_NEW_DSP_1("rstream~",rstream,int0)
