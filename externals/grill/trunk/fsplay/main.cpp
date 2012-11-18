/* 
fsplay~ - file and stream player

Copyright (c)2004-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.

$LastChangedRevision: 3740 $
$LastChangedDate: 2011-02-14 18:57:54 -0500 (Mon, 14 Feb 2011) $
$LastChangedBy: thomas $
*/

#include "fsplay.h"
//#include "pa_ringbuffer.h"
#include <flcontainers.h>
#include <string>
#include <list>
#include <set>
#include <vector>
#include <cmath>
#include <boost/shared_ptr.hpp>
#include "resample.hpp"

// just for now
#define WORKAROUND


#define FSPLAY_VERSION "0.1.2"

#define SIMDGRAIN 16

// default buffer settings
#define MINBUF 8000  // frames = samples per channel
#define MAXBUF 10000 // frames = samples per channel
#define SAFETY 2000 // minimum buffer at seek or file change, frames = samples per channel

// minimal buffer size - frames, hardcoded
#define MINBUFSIZE 64

// chunk to read from file at once
#define MINRDBUF (SIMDGRAIN)  // in samples (not frames) (must be > 16)
#define MAXRDBUF 65536  // in samples (not frames) (must be > 16)
#define DEFRDBUF 4096  // in samples (not frames) (must be > 16)

inline int RndSIMD(int n) { return (n+(SIMDGRAIN-1))&~(SIMDGRAIN-1); }


typedef std::list<fspformat::SetupHandler> SetupList;
typedef std::list<fspformat::NewHandler> NewList;

static const t_symbol *sym_loop,*sym_eof,*sym_underflow,*sym_file;


// must use a pointer because we can't be sure that std::list is contructed previous to the Add calls in the other files
static SetupList *setuphandlers = NULL;
static NewList *newhandlers = NULL;

bool fspformat::Add(SetupHandler setupfun)
{
    if(!setuphandlers) {
        setuphandlers = new SetupList;
        newhandlers = new NewList;
    }
    setuphandlers->push_back(setupfun);
    return true;
}

bool fspformat::Add(NewHandler newfun)
{
    newhandlers->push_back(newfun);
    return true;
}

void fspformat::Setup()
{
    if(setuphandlers)
        for(SetupList::const_iterator it = setuphandlers->begin(); it != setuphandlers->end(); ++it) {
            bool ret = (*it)();
#ifdef FLEXT_DEBUG
            if(!ret) post("Handler couldn't be set up");
#endif
        }
}

fspformat *fspformat::New(const std::string &filename)
{
    fspformat *ret = NULL;
    if(newhandlers)
        for(NewList::const_iterator it = newhandlers->begin(); !ret && it != newhandlers->end(); ++it)
            ret = (*it)(filename);
    return ret;
}

void fspformat::ThreadBegin() {}
void fspformat::ThreadEnd() {}

typedef boost::shared_ptr<fspformat> FormatPtr;

static void copy_samples(t_sample *outb,size_t outs,const t_sample *inb,size_t ins,size_t cnt)
{
    if(ins == 1 && outs == 1)
        flext::CopySamples(outb,inb,cnt);
    else {
        for(size_t i = 0; i < cnt; ++i)
            outb[i*outs] = inb[i*ins];
    }
}

////////////////// Resampler ////////////////////

////////////////// buffer class ////////////////////

class Buffer
    : public flext
{
public:
    Buffer(int c): cnt(c),data(static_cast<t_sample *>(NewAligned(sizeof(t_sample)*c))) {}
    ~Buffer() { FreeAligned(data); }

    int size() const { return cnt; }
    t_sample *get() const { return data; }

protected:
    const int cnt;
    t_sample *data;
};

////////////////// ring buffer ////////////////////

#if 0

template<class T>
class RingBuffer
{
public:
    RingBuffer(int frames)
    {
        data = new T[frames];
        PaUtil_InitializeRingBuffer(&rb,frames*sizeof(T),data);
    }

    ~RingBuffer()
    {
        delete[] data;
    }

    int WriteAvail() const 
    {
        return PaUtil_GetRingBufferWriteAvailable(&rb)/sizeof(T);
    }

    int Write(const T *d,int frames)
    {
        return PaUtil_WriteRingBuffer(&rb,d,frames)/sizeof(T);
    }

    int ReadAvail() const 
    {
        return PaUtil_GetRingBufferReadAvailable(&rb)/sizeof(T);
    }

    int Read(T *d,int frames)
    {
        return PaUtil_ReadRingBuffer(&rb,d,frames)/sizeof(T);
    }

protected:
    PaUtilRingBuffer rb;
    T *data;
};

#else

class RingBuffer
    : public flext
{
public:
    RingBuffer(int ch,int bufsz = MAXBUF)
        : channels(ch)
        , buf(NULL)
        , rptr(0),wptr(0)
        , minbuf(MINBUF),maxbuf(std::max<int>(MINBUFSIZE,RndSIMD(bufsz)))
        , safety(SAFETY)
    {
        buf = new Buffer *[ch];
        for(int i = 0; i < ch; ++i) buf[i] = new Buffer(maxbuf);
    }

    ~RingBuffer()
    {
        for(int i = 0; i < channels; ++i) delete buf[i];
        delete[] buf;
    }

    /*! Reduce current buffer filling to safety size (for better latency)
    \note should only be used when file stream changes! (data discontinuity)
    \todo RETHINK THIS - maybe it's better to clear the buffer (rptr = wptr)
    */
    void ClearBuf()
    {
#if 1
        wptr = rptr;
#else
        int fill = Filling();
        if(fill > safety) {
            // really reduce
            fill = safety;
            wptr = rptr+fill;
            if(wptr >= maxbuf) wptr -= maxbuf;
        }
#endif
    }

    int Filling() const 
    {
        int fill = wptr-rptr;
        if(fill < 0) fill += maxbuf;
        return fill;
    }

    int minbuf,maxbuf,safety,channels;
    Buffer **buf;
    int rptr,wptr;
};

#endif


typedef boost::shared_ptr<RingBuffer> RingPtr;

static t_sample *transfer = NULL;


class Worker;

typedef boost::shared_ptr<Worker> WorkerPtr;

class Command
    : public FifoCell
{
public:
    virtual void Do() = 0;
    virtual ~Command() {}
protected:
    Command(WorkerPtr w): worker(w) {}
    WorkerPtr worker;
};

static flext::ThrCond cond;
typedef TypedFifo<Command> List;
static List list;

static void Signal(Command *c = NULL)
{
    if(c) list.Put(c);
    cond.Signal();
}


typedef std::vector<WorkerPtr> Workers;
typedef boost::shared_ptr<Workers> WorkersPtr;

static WorkersPtr workers(new Workers);

static void AddWorker(WorkerPtr w)
{
    // add worker
    WorkersPtr wp(workers);
    Workers *wnew = new Workers;
    for(Workers::const_iterator it = wp->begin(); it != wp->end(); ++it)
        wnew->push_back(*it);
    wnew->push_back(w);
    workers.reset(wnew);  // swap pointers
}

static void RmvWorker(WorkerPtr w)
{
    // remove worker
    WorkersPtr wp(workers);
    Workers *wnew = new Workers;
    for(Workers::iterator it = wp->begin(); it != wp->end(); ++it)
        if(it->get() != w.get()) wnew->push_back(*it);
    workers.reset(wnew);  // swap pointers
}


class Worker
    : public flext
{
    friend class fsplay;

public:
    Worker(RingBuffer *rb) 
        : grain(DEFRDBUF)
        , run(false)
        , ringbuffer(rb)
        , resampler(new Resampler *[rb->channels])
    {
        for(int c = 0; c < ringbuffer->channels; ++c)
            resampler[c] = new AlgoResampler<CubicAlgo>;

        Reset();
    }

    ~Worker()
    {
        for(int c = 0; c < ringbuffer->channels; ++c)
            delete resampler[c];
        delete[] resampler;
    }

    bool Work()
    {
        if(eof) return false; // nothing to do

        RingPtr rbp(ringbuffer);
        RingBuffer *rb = rbp.get();
        FLEXT_ASSERT(rb);

        int want = rb->rptr-(SIMDGRAIN)-rb->wptr;
        if(want < 0) want += rb->maxbuf;
        if(!want) return false; // nothing to do

        FormatPtr fmt(format);
        if(UNLIKELY(!fmt)) return false; // format not set
        if(UNLIKELY(!fmt->Channels())) return false; // not an audio file....

        reported = false;

        // rptr can be increased by m_dsp in the meantime but that's ok
        int frames,written = 0;

        {
            // read from file

            const int rdframes = grain/rb->channels;
            frames = want > rdframes?rdframes:want;

            const cnt_t rf = fmt->Read(transfer,frames);

    //        printf("TIME %lf - WANTED %i, GOT %i\n",clock_getlogicaltime(),frames,rf);

            if(rf >= 0) {
                fpos += rf/fmt->Samplerate();
                frames = (int)rf;
            }
            else {
                frames = 0;

                // end of file
                if(loop) {
                    if(LIKELY(fmt->Seek(0))) {
                        fpos = 0;
                        ++loops;
                        Message(sym_loop,0,NULL);
                    }
                    else
                        // repositioning to 0 shouldn't make any problems
                        FLEXT_ASSERT(false);
                }
                else
                    // eof messaging is done in dsp function
                    eof = true;
            }
        }

        {
            // copy to ring buffer

            // cache some variables
            const int chns = rb->channels,stride = fmt->Channels();
            const int cm = std::min<int>(stride,chns);
            const t_sample *buf = transfer;

            FLEXT_ASSERT(frames >= 0);

            while(frames > 0) {
                const int dif = rb->maxbuf-rb->wptr;
                const int outcnt = std::min<int>(dif,want);
                if(!outcnt) break;

                const int xfer = std::min<int>(outcnt,frames);

                int c;
                for(c = 0; c < cm; ++c) {
                    Buffer *b = rb->buf[c];
                    copy_samples(b->get()+rb->wptr,1,buf+c,stride,xfer);
                }
                for(; c < chns; ++c) {
                    Buffer *b = rb->buf[c];
                    ZeroSamples(b->get()+rb->wptr,xfer);
                }

                buf += xfer*stride;

                if((rb->wptr += xfer) >= rb->maxbuf) 
                    rb->wptr -= rb->maxbuf; // wrap around

                frames -= xfer;
                FLEXT_ASSERT(frames >= 0);
                written += xfer;
                want -= xfer;
                FLEXT_ASSERT(want >= 0);
            }
        }
        return written < want; // need more?
    }

    void Message(AtomAnything &a) { messages.Put(a); }
    void Message(const t_symbol *s,int argc,const t_atom *argv) { messages.Put(AtomAnything(s,argc,argv)); }


    void Format(const std::string &fn)
    {
        fspformat *fmt = NULL;
        if(fn.size()) {
            fmt = fspformat::New(fn);
            eof = false;
        }

        if(fmt) {
            float sr = fmt->Samplerate();
            t_atom at[5];
            SetString(at[0],filename.c_str());
            SetInt(at[1],fmt->Channels());
            SetFloat(at[2],sr);          
            SetDouble(at+3,double(fmt->Frames())/sr);
            Message(sym_file,5,at);
        }
        else {
            t_atom at[2];
            SetString(at[0],filename.c_str());
            SetInt(at[1],0);
            Message(sym_file,2,at);
        }

        filename = fn; 
        format.reset(fmt);  // set only here - no multiple writer threads
        Reset();
    }

    void Seek(double pos)
    {
        FormatPtr fmt(format);
        if(fmt && fmt->Seek(pos))
            Reset(pos);
    }

    // called from PD thread
    void Fill(int chns,t_sample *const *outsigs,int n,t_sample outrate,const t_sample *speed,int spdstride)
    {
        FormatPtr fmt(format);
        
        if(LIKELY(fmt && run)) {
            RingPtr rb(ringbuffer);    
            bool uflow = false;

            t_sample ratio = fmt->Samplerate()/outrate;

            if(rb->Filling()-int(n*ratio) < rb->minbuf)
                Signal(); // signal worker thread ought to fill the buffer!

            int fn = 0;
            double adv = 0;

            for(;;) {
                int outneed = n-fn;
                if(!outneed) break;

                int inhave = rb->Filling();
                if(UNLIKELY(!inhave)) break;

                if(LIKELY(rb->rptr < rb->maxbuf))
                    // only read till end of ringbuffer
                    inhave = std::min<int>(inhave,rb->maxbuf-rb->rptr);
                else
                    // ringbuffer boundary
                    rb->rptr = 0;

                int c,incnt,outcnt;

                const int cm = std::min<int>(fmt->Channels(),chns);
                for(c = 0; c < cm; ++c) {
                    Buffer *b = rb->buf[c];
                    incnt = inhave,outcnt = outneed;
                    const t_sample *sp = speed+fn*spdstride;
                    resampler[c]->work(sp,spdstride,ratio,outsigs[c]+fn,1,outcnt,b->get()+rb->rptr,1,incnt,eof);
                    if(!c) // only for first channel
                        if(!spdstride)
                            adv += *sp * outcnt;
                        else
                            for(int i = 0; i < outcnt; ++i,sp += spdstride)
                                adv += *sp;
                }
                for(; c < chns; ++c) {
                    Buffer *b = rb->buf[c];
                    ZeroSamples(outsigs[c]+fn,outcnt);
                }

                rb->rptr += incnt;
                fn += outcnt;
            }

            if(UNLIKELY(fn < n))
                uflow = true;

            ppos += adv/outrate;
            if(fmt->Frames()) {
                double dur = fmt->Frames()/outrate;
                ppos = fmod(ppos,dur);
            }

            if(UNLIKELY(uflow))
                for(int c = 0; c < chns; ++c)
                    ZeroSamples(outsigs[c]+fn,n-fn);

            if(UNLIKELY(eof && !rb->Filling())) {
                // stop running
                run = false;
                Message(sym_eof,0,NULL);
            }

            if(UNLIKELY(uflow && !eof && !reported)) {
                Message(sym_underflow,0,NULL);
                reported = true;
            }
        }
        else {
            // not running
            for(int c = 0; c < chns; ++c)
                ZeroSamples(outsigs[c],n);
        }
    }

    std::string filename;
    FormatPtr format;
    RingPtr ringbuffer;
    ValueFifo<AtomAnything> messages;
    Resampler **resampler;

    double fpos;  // position in s - media side
    double ppos;  // position in s - DSP side
    int grain;
    int loops;
    bool run,eof,loop,reported;

    void Reset(double pos = 0)
    {
        ppos = fpos = pos;
        eof = false;
        reported = true;  // underflows need not be displayed at empty ringbuffer
        loops = 0;
        RingPtr(ringbuffer)->ClearBuf();
    }
};

////////////////// commands ////////////////////

class CommandNew
    : public Command
{
public:
    CommandNew(WorkerPtr w,std::string &nm): Command(w),filename(nm) {}
    virtual void Do() { worker->Format(filename); }
protected:
    std::string filename;
};

class CommandPos
    : public Command
{
public:
    CommandPos(WorkerPtr w,double p): Command(w),pos(p) {}

    virtual void Do() 
    { 
        worker->Seek(pos); 
    }
protected:
    double pos;
};

////////////////// flext object ////////////////////

class fsplay;

typedef std::set<fsplay *> ObjSet;
static ObjSet objects;

class fsplay:
    public flext_dsp
{
    FLEXT_HEADER_S(fsplay,flext_dsp,Setup)
public:
    fsplay(int ch,bool sig)
    {
        if(sig)
            AddInSignal();
        else
            AddInAnything();

        if(!ch) ch = 2;
        AddOutSignal(ch);

        // set only here - no multiple writer threads
        Worker *wp = new Worker(new RingBuffer(ch));
        worker.reset(wp);
        
        AddWorker(worker);
        objects.insert(this);
        
        if(!sig) {
            FLEXT_ADDATTR_VAR("speed",mg_speed,ms_speed);
            FLEXT_ADDMETHOD(0,ms_speed);
            speed.resize(1);
            speed[0] = 1; // init to 1
        }

#ifndef WORKAROUND
        AddIdle();
#endif
    }

    ~fsplay()
    {
        objects.erase(this);
        RmvWorker(worker);
    }

    void ms_file(const AtomList &args)
    {
        std::string filename;
        for(int i = 0; i < args.Count(); ++i)
            if(IsString(args[i])) {
                if(filename.size()) filename += " ";
#if 1
                filename += GetString(args[i]);
#else
                CnvFlnm(filename,GetString(args[i]));
#endif
            }
            // else: ignore atom

        Signal(new CommandNew(worker,filename));
    }

    void mg_file(AtomList &args)
    {
        if(worker->filename.size()) {
            args(1); SetString(args[0],worker->filename.c_str());
        }
        else 
            args(0);
    }
    
    void ms_pos(const AtomList &l)
    {
        double p = GetDouble(l);
        Signal(new CommandPos(worker,p));
    }
    
    void mg_pos(AtomList &l)
    {
        FormatPtr fmt(worker->format);
        SetDouble(l,fmt?worker->ppos:0);
    }

    void mg_fpos(AtomList &l)
    {
        FormatPtr fmt(worker->format);
        SetDouble(l,fmt?worker->fpos:0);
    }
    
    void mg_run(bool &b) { b = worker->run; }
    void ms_run(bool b) { worker->run = b; }

    void mg_loop(bool &b) { b = worker->loop; }
    void ms_loop(bool b) { worker->loop = b; }

    void mg_maxbuf(int &b) 
    { 
        b = RingPtr(worker->ringbuffer)->maxbuf; 
    }
    
    // \todo this should probably be a command as the ringbuffer might change in the meantime....
    void ms_maxbuf(int b) 
    { 
        RingPtr r(worker->ringbuffer);
        if(b != r->maxbuf)
            worker->ringbuffer.reset(new RingBuffer(CntOutSig(),b));  // set only here - no multiple writer threads
    }

    void mg_minbuf(int &b) { b = RingPtr(worker->ringbuffer)->minbuf; }

    // \todo this should probably be a command as the ringbuffer might change in the meantime....
    void ms_minbuf(int b) { RingPtr(worker->ringbuffer)->minbuf = RndSIMD(b); }

    void mg_safety(int &b) { b = RingPtr(worker->ringbuffer)->safety; }

    // \todo this should probably be a command as the ringbuffer might change in the meantime....
    void ms_safety(int b) { RingPtr(worker->ringbuffer)->safety = RndSIMD(b); }

    void mg_grain(int &b) { b = worker->grain; }

    void ms_grain(int b) 
    { 
        int grain = RndSIMD(b); 
        if(grain < MINRDBUF) grain = MINRDBUF;
        else if(grain > MAXRDBUF) grain = MAXRDBUF;
        worker->grain = grain;
    }

    void mg_frames(AtomList &fr) const 
    { 
        // have to convert int64__ to 2 floats.... 
        FormatPtr fmt(worker->format);
        if(fmt)
            SetDouble(fr,double(fmt->Frames()));
    }

    void mg_buffer(int &fr) const 
    { 
        fr = RingPtr(worker->ringbuffer)->Filling(); 
    }

    void mg_speed(float &s) const 
    {
        s = speed[0];
    }

    void ms_speed(float s)
    {
        if(s < 0) return;
        speed[0] = s;
    }

#ifndef WORKAROUND
protected:
#endif
    virtual bool CbIdle()
    {
        // send waiting responses
        while(worker->messages.Avail())  // it's important that we are the only message reader...
            ToOutAnything(GetOutAttr(),worker->messages.Get());
        return false;
    }

    virtual bool CbDsp()
    {
        if(CntInSig()) {
            const t_sample *sp = InSig(0);
            int i;
            for(i = CntOutSig()-1; i >= 0 && OutSig(i) != sp; --i) {}
            // if i >= 0, in and out signals point to the same memory region
            speed.resize(i >= 0?Blocksize():0);
        }
        return true;
    }

    virtual void CbSignal()
    {   
        int n = Blocksize();
        int nsigs = CntOutSig();
        t_sample *const *sigs = OutSig();
        t_sample *sp;
        if(CntInSig()) {
            if(speed.size())
                CopySamples(sp = &speed[0],InSig(0),n);
            else
                sp = InSig(0);
            worker->Fill(nsigs,sigs,n,Samplerate(),sp,1);
        }
        else
            worker->Fill(nsigs,sigs,n,Samplerate(),&speed[0],0);
    }

private:

    bool sec_units;
    WorkerPtr worker;   // will never change here....
    std::vector<t_sample> speed;

    static void threadfun(thr_params *);

    static void Setup(t_classid c);

    FLEXT_CALLVAR_V(mg_file,ms_file)
    FLEXT_CALLGET_V(mg_frames)
    FLEXT_CALLVAR_B(mg_run,ms_run)
    FLEXT_CALLVAR_B(mg_loop,ms_loop)
    FLEXT_CALLVAR_V(mg_pos,ms_pos)
    FLEXT_CALLGET_V(mg_fpos)
    FLEXT_CALLVAR_I(mg_minbuf,ms_minbuf)
    FLEXT_CALLVAR_I(mg_maxbuf,ms_maxbuf)
    FLEXT_CALLVAR_I(mg_safety,ms_safety)
    FLEXT_CALLVAR_I(mg_grain,ms_grain)
    FLEXT_CALLGET_I(mg_buffer)
    FLEXT_CALLVAR_F(mg_speed,ms_speed)
    FLEXT_CALLBACK_F(ms_speed)
};

FLEXT_NEW_DSP_2("fsplay~",fsplay,int0,bool0)


#ifdef WORKAROUND
#ifdef PD_DEVEL_VERSION
static t_int idlefun(t_int* argv)
{
#else
static t_clock *idleclk = NULL;
#if FLEXT_SYS == FLEXT_SYS_PD
static void idlefun()
#elif FLEXT_SYS == FLEXT_SYS_MAX
static void *idlefun(void *,...)
#endif
{
#endif
    for(ObjSet::const_iterator it = objects.begin(); it != objects.end(); ++it)
        (*it)->CbIdle();
#ifdef PD_DEVEL_VERSION
    return 2;
#else
    clock_delay(idleclk,1);
#endif
#if FLEXT_SYS == FLEXT_SYS_MAX
    return NULL;
#endif
}
#endif

void fsplay::Setup(t_classid c)
{
    // cache some symbols
    sym_loop = MakeSymbol("loop");
    sym_eof = MakeSymbol("eof");
    sym_underflow = MakeSymbol("underflow");
    sym_file = MakeSymbol("file");

    transfer = (t_sample *)NewAligned(MAXRDBUF*sizeof(float));

    // start file helper thread
    LaunchThread(threadfun,NULL);

    // add methods and attributes
    FLEXT_CADDATTR_VAR(c,sym_file,mg_file,ms_file);
    FLEXT_CADDATTR_GET(c,"frames",mg_frames);
    FLEXT_CADDATTR_VAR(c,"run",mg_run,ms_run);
    FLEXT_CADDATTR_VAR(c,sym_loop,mg_loop,ms_loop);
    FLEXT_CADDATTR_GET(c,"fpos",mg_fpos);
    FLEXT_CADDATTR_VAR(c,"pos",mg_pos,ms_pos);
    FLEXT_CADDATTR_VAR(c,"minbuf",mg_minbuf,ms_minbuf);
    FLEXT_CADDATTR_VAR(c,"maxbuf",mg_maxbuf,ms_maxbuf);
    FLEXT_CADDATTR_VAR(c,"safety",mg_safety,ms_safety);
    FLEXT_CADDATTR_VAR(c,"grain",mg_grain,ms_grain);
    FLEXT_CADDATTR_GET(c,"buffer",mg_buffer);

    post("fsplay~ - file and stream player");
    post("Version " FSPLAY_VERSION " - (c)2004-2011 Thomas Grill");
#ifdef FLEXT_DEBUG
    post("Debug version, compiled on " __DATE__ " " __TIME__);
#endif
    post("");

#ifdef WORKAROUND
#ifdef PD_DEVEL_VERSION
    sys_callback(idlefun,NULL,0);
#else
    idleclk = (t_clock *)clock_new(NULL,idlefun);
    clock_delay(idleclk,1);
#endif
#endif

    fspformat::Setup();
}

void fsplay::threadfun(thr_params *)
{
//    RelPriority(+1);

    for(;;) {
        Command *c = list.Get();
        if(c) {
            // execute explicit command
            c->Do();
            delete c;
        }
        else {
#ifdef FLEXT_DEBUG
            double tm1 = flext::GetOSTime();
#endif

            // no command... now work through all the workers
            bool more = false;
            WorkersPtr w(workers);
            for(Workers::const_iterator it = w->begin(); it != w->end(); ++it)
                // protect worker and do the work
                more |= WorkerPtr(*it)->Work();

#if 0 //def FLEXT_DEBUG
            double tm2 = flext::GetOSTime();
            printf("WORK TIME: %lf for %lf\n",tm1,tm2-tm1);
#endif

            if(!more) 
                cond.Wait();  // nothing to read -> wait
        }
    }
}

void CnvFlnm(std::string &dst,const char *src)
{
#if FLEXT_SYS == FLEXT_SYS_PD && FLEXT_OS == FLEXT_OS_WIN
    const char *s;
    for(s = src; *s; ++s)
        dst += (*s != '/'?*s:'\\');
#else
    dst += src;
#endif
}
