/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3711 $
$LastChangedDate: 2009-08-13 10:35:20 -0400 (Thu, 13 Aug 2009) $
$LastChangedBy: thomas $
*/

/*! \file flqueue.cpp
    \brief Implementation of the flext message queuing functionality.

    \todo Let's see if queuing can be implemented for Max/MSP with defer_low

    if FLEXT_PDLOCK is defined, the new PD thread lock functions are used
*/

#include "flext.h"
#include "flinternal.h"
#include "flcontainers.h"
#include <cstring> // for memcpy

#include "flpushns.h"

#ifdef FLEXT_THREADS
//! Thread id of message queue thread
flext::thrid_t flext::thrmsgid;
#endif

static bool qustarted = false;

#ifdef FLEXT_SHARED
/*
    For the shared version it _should_ be possible to have only one queue for all externals.
    Yet I don't know how to do this cross-platform
*/
#define PERMANENTIDLE
#endif

static void Trigger();


class flext::MsgBundle;

class QueueFifo
    : public PooledFifo<flext::MsgBundle>
{
public:
    ~QueueFifo();
};

class Queue:
    public flext,
    public QueueFifo
{
public:
    inline bool Empty() const { return !Avail(); }
    
    inline void Push(MsgBundle *m); // defined after MsgBundle (gcc 3.3. won't take it otherwise...)
};

static Queue queue;


#define STATSIZE 8

class flext::MsgBundle:
    public flext,
    public FifoCell
{
public:
    static MsgBundle *New()
    {
        MsgBundle *m = queue.New();
        m->msg.Init();
        return m;
    }

    static void Free(MsgBundle *m)
    {       
        for(Msg *mi = m->msg.nxt; mi; ) {
            Msg *mn = mi->nxt;
            mi->Free();
            delete mi;
            mi = mn;
        }
        m->msg.Free();
        queue.Free(m);
    }

    bool BelongsTo(flext_base *t) const
    {
        return !msg.nxt && msg.BelongsTo(t);
    }

    void Idle(flext_base *t)
    {
        Get()->Idle(t);
    }

    void Idle(bool (*idlefun)(int argc,const t_atom *argv),int argc,const t_atom *argv)
    {
        Get()->Idle(idlefun,argc,argv);
    }

    inline MsgBundle &Add(flext_base *t,int o,const t_symbol *s,int ac,const t_atom *av)
    {
        Get()->Set(t,o,s,ac,av);
        return *this;
    }

    inline MsgBundle &Add(const t_symbol *r,const t_symbol *s,int ac,const t_atom *av)
    {
        Get()->Set(r,s,ac,av);
        return *this;
    }

    inline MsgBundle &Add(flext_base *th,int o) // bang
    { 
        return Add(th,o,sym_bang,0,NULL);
    }

    inline MsgBundle &Add(flext_base *th,int o,float dt) 
    { 
        t_atom at; 
        SetFloat(at,dt);
        return Add(th,o,sym_float,1,&at);
    }

    inline MsgBundle &Add(flext_base *th,int o,int dt) 
    { 
        t_atom at; 
        SetInt(at,dt);
        const t_symbol *sym;
#if FLEXT_SYS == FLEXT_SYS_PD
        sym = sym_float;
#elif FLEXT_SYS == FLEXT_SYS_MAX
        sym = sym_int;
#else
#error Not implemented!
#endif
        return Add(th,o,sym,1,&at);
    }

    inline MsgBundle &Add(flext_base *th,int o,const t_symbol *dt) 
    { 
        t_atom at; 
        SetSymbol(at,dt);
        return Add(th,o,sym_symbol,1,&at);
    }

    inline MsgBundle &Add(flext_base *th,int o,const t_atom &a) 
    { 
        const t_symbol *sym;
        if(IsSymbol(a))
            sym = sym_symbol;
        else if(IsFloat(a))
            sym = sym_float;
#if FLEXT_SYS == FLEXT_SYS_MAX
        else if(IsInt(a))
            sym = sym_int;
#endif
#if FLEXT_SYS == FLEXT_SYS_PD
        else if(IsPointer(a))
            sym = sym_pointer;
#endif
        else {
            error("atom type not supported");
            return *this;
        }
        return Add(th,o,sym,1,&a);
    }

    inline MsgBundle &Add(flext_base *th,int o,int argc,const t_atom *argv) 
    {
        return Add(th,o,sym_list,argc,argv);
    }

    // \note PD sys lock must already be held by caller
    inline bool Send() const
    {
        if(!msg.Ok()) return false; // Empty!

        const Msg *m = &msg;
        do {
            if(m->Send()) {
                // we should re-enqeue the message... it can't be a real bundle then, only a solo message
                FLEXT_ASSERT(!m->nxt);
                return true;
            }
            m = m->nxt;
        } while(m);
        return false;
    }

private:

    class Msg {
    public:
        inline bool Ok() const { return th || recv; }

        void Init()
        {
            th = NULL;
            recv = NULL;
            nxt = NULL;
            argc = 0;
        }

        void Free()
        {
            if(argc > STATSIZE) {
                FLEXT_ASSERT(argv);
                delete[] argv;
            }
        }

        //! Attention: works only for solo messages, not real bundles!!
        bool BelongsTo(flext_base *t) const
        {
            FLEXT_ASSERT(!nxt);
            return th == t; 
        }

        void Set(flext_base *t,int o,const t_symbol *s,int ac,const t_atom *av)
        {
            FLEXT_ASSERT(t);
            th = t;
            out = o;
            SetMsg(s,ac,av);
        }

        void Set(const t_symbol *r,const t_symbol *s,int ac,const t_atom *av)
        {
            FLEXT_ASSERT(r);
            th = NULL;
            recv = r;
            SetMsg(s,ac,av);
        }

        void Idle(flext_base *t)
        {
            FLEXT_ASSERT(t);
            th = t;
            SetMsg(NULL,0,NULL);
        }

        void Idle(bool (*idlefun)(int argc,const t_atom *argv),int argc,const t_atom *argv)
        {
            FLEXT_ASSERT(idlefun);
            th = NULL;
            fun = idlefun;
            SetMsg(NULL,argc,argv);
        }

        bool Send() const
        {
            if(LIKELY(sym)) {
                // messages
                if(th) {
                    if(UNLIKELY(out < 0))
                        // message to self
                        th->CbMethodHandler(-1-out,sym,argc,argc > STATSIZE?argv:argl); 
                    else
                        // message to outlet
                        th->ToSysAnything(out,sym,argc,argc > STATSIZE?argv:argl);
                }
                else
                    flext::SysForward(recv,sym,argc,argc > STATSIZE?argv:argl);
                return false;
            }
            else {
                // idle processing
                if(th)
                    // call virtual method
                    return th->CbIdle();
                else
                    // call static function
                    return (*fun)(argc,argc > STATSIZE?argv:argl);
            }
        }

        Msg *nxt;

    protected:
        flext_base *th;
        union {
            int out;
            const t_symbol *recv;
            bool (*fun)(int argc,const t_atom *argv);
        };
        const t_symbol *sym;
        int argc;
        union {
            t_atom *argv;
            t_atom argl[STATSIZE];
        };

        void SetMsg(const t_symbol *s,int cnt,const t_atom *lst)
        {
            sym = s;
            argc = cnt;
            if(UNLIKELY(cnt > STATSIZE)) {
                argv = new t_atom[cnt];
                flext::CopyAtoms(cnt,argv,lst);
            }
            else
                flext::CopyAtoms(cnt,argl,lst);
        }

    } msg;

    Msg *Get()
    {
        Msg *m = &msg;
        if(LIKELY(m->Ok())) {
            for(; m->nxt; m = m->nxt) {}
            m = m->nxt = new Msg;
            m->Init();
        }
        return m;
    }
};

QueueFifo::~QueueFifo() 
{ 
    flext::MsgBundle *n; 
    while((n = Get()) != NULL) delete n; 
}

inline void Queue::Push(MsgBundle *m)
{
    if(LIKELY(m)) {
        Put(m);
        Trigger();
    }
}

#if FLEXT_QMODE == 2
static flext::ThrCond qthrcond;
#elif FLEXT_QMODE == 0
//static t_qelem *qclk = NULL;
static t_clock *qclk = NULL;
#endif


#define CHUNK 10

#if FLEXT_QMODE == 1
static bool QWork(bool syslock,flext_base *flushobj = NULL)
{
    // Since qcnt can only be increased from any other function than QWork
    // qc will be a minimum guaranteed number of present queue elements.
    // On the other hand, if new queue elements are added by the methods called
    // in the loop, these will be sent in the next tick to avoid recursion overflow.
    flext::MsgBundle *q;
    if((q = queue.Get()) == NULL) 
        return false;
    else if(q->Send()) {
        if(!flushobj || !q->BelongsTo(flushobj))
            queue.Push(q);  // remember messages to be processed again
        else
            flext::MsgBundle::Free(q);
        return true;
    }
    else {
        flext::MsgBundle::Free(q);
        return false;
    }
}
#else
static bool QWork(bool syslock,flext_base *flushobj = NULL)
{
    Queue newmsgs;
    flext::MsgBundle *q;

#if 0
    static int counter = 0;
    fprintf(stderr,"QWORK %i\n",counter++);
#endif

    for(;;) {
        // Since qcnt can only be increased from any other function than QWork
        // qc will be a minimum guaranteed number of present queue elements.
        // On the other hand, if new queue elements are added by the methods called
        // in the loop, these will be sent in the next tick to avoid recursion overflow.
        if(!queue.Avail()) break;

    #if FLEXT_QMODE == 2
        if(syslock) flext::Lock();
    #endif

        while((q = queue.Get()) != NULL) {
            if(q->Send())
                newmsgs.Push(q);  // remember messages to be processed again
            else
                flext::MsgBundle::Free(q);
        }

    #if FLEXT_QMODE == 2
        if(syslock) flext::Unlock();
    #endif

    }

    // enqueue messages that have to be processed again
    while((q = newmsgs.Get()) != NULL)
        if(!flushobj || !q->BelongsTo(flushobj))
            queue.Push(q);
        else
            flext::MsgBundle::Free(q);

    return queue.Avail();
}
#endif

#if FLEXT_QMODE == 0
#if FLEXT_SYS == FLEXT_SYS_JMAX
static void QTick(fts_object_t *c,int winlet, fts_symbol_t s, int ac, const fts_atom_t *at)
{
#else
static void QTick(flext_base *c)
{
#endif
    QWork(false);
}

#elif FLEXT_QMODE == 1
#   ifndef PERMANENTIDLE
        static bool qtickactive = false;
#   endif
static t_int QTick(t_int *)
{
#ifndef PERMANENTIDLE
    qtickactive = false;
#endif

    if(QWork(false))
        return 1;
    else {
#       ifdef PERMANENTIDLE
            // will be run in the next idle cycle
            return 2;
#       else
            // won't be run again
            // for non-shared externals assume that there's rarely a message waiting
            // so it's better to delete the callback meanwhile
            return 0; 
#       endif
    }
}
#endif

/*
It would be sufficient to only flush messages belonging to object th
But then the order of sent messages is not as intended
*/
void flext_base::QFlush(flext_base *th)
{
    FLEXT_ASSERT(!IsThreadRegistered());
    while(!queue.Empty()) QWork(false,th);
}

static void Trigger()
{
#if FLEXT_SYS == FLEXT_SYS_PD
#   if FLEXT_QMODE == 2
        // wake up worker thread
        qthrcond.Signal();
#   elif FLEXT_QMODE == 1 && !defined(PERMANENTIDLE)
        if(!qtickactive) {
            sys_callback(QTick,NULL,0);
            qtickactive = true;
        }
#   elif FLEXT_QMODE == 0
#   ifdef FLEXT_THREADS
        bool sys = flext::IsThread(flext::GetSysThreadId());
#   else
        bool sys = true;
#   endif
        if(!sys) flext::Lock();
        clock_delay(qclk,0);
        if(!sys) flext::Unlock();
#   endif
#elif FLEXT_SYS == FLEXT_SYS_MAX
#   if FLEXT_QMODE == 0
//        qelem_front(qclk);
        clock_delay(qclk,0);
#   endif
#else
#   error Not implemented
#endif
}

#if FLEXT_QMODE == 2
void flext_base::QWorker(thr_params *)
{
    thrmsgid = GetThreadId();
    qustarted = true;
    for(;;) {
        // we need a timed wait so that idle processing can take place
        qthrcond.TimedWait(0.001);
        QWork(true);
    }
}
#endif

void flext_base::StartQueue()
{
    if(qustarted) return;
#if FLEXT_QMODE == 1
#   ifdef PERMANENTIDLE
        sys_callback(QTick,NULL,0);
        qustarted = true;
#   endif
#elif FLEXT_QMODE == 2
    LaunchThread(QWorker,NULL);
    // very unelegant... but waiting should be ok, since happens only on loading
    while(!qustarted) Sleep(0.001);
#elif FLEXT_QMODE == 0 && (FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX)
//    qclk = (t_qelem *)(qelem_new(NULL,(t_method)QTick));
    qclk = (t_clock *)(clock_new(NULL,(t_method)QTick));
    qustarted = true;
#else
#   error Not implemented!
#endif
}

flext::MsgBundle *flext::MsgNew() 
{ 
    return MsgBundle::New(); 
}

void flext::MsgFree(MsgBundle *m) 
{ 
    MsgBundle::Free(m); 
}

void flext::ToSysMsg(MsgBundle *m)
{
    m->Send();
    queue.Free(m);
}

void flext::ToQueueMsg(MsgBundle *m)
{
    queue.Push(m);
}



void flext_base::ToQueueBang(int o) const 
{
    MsgBundle *m = MsgBundle::New();
    m->Add(const_cast<flext_base *>(this),o);
    queue.Push(m);
}

void flext_base::ToQueueFloat(int o,float f) const
{
    MsgBundle *m = MsgBundle::New();
    m->Add(const_cast<flext_base *>(this),o,f);
    queue.Push(m);
}

void flext_base::ToQueueInt(int o,int f) const
{
    MsgBundle *m = MsgBundle::New();
    m->Add(const_cast<flext_base *>(this),o,f);
    queue.Push(m);
}

void flext_base::ToQueueSymbol(int o,const t_symbol *s) const
{
    MsgBundle *m = MsgBundle::New();
    m->Add(const_cast<flext_base *>(this),o,s);
    queue.Push(m);
}

void flext_base::ToQueueAtom(int o,const t_atom &at) const
{
    MsgBundle *m = MsgBundle::New();
    m->Add(const_cast<flext_base *>(this),o,at);
    queue.Push(m);
}

void flext_base::ToQueueList(int o,int argc,const t_atom *argv) const
{
    MsgBundle *m = MsgBundle::New();
    m->Add(const_cast<flext_base *>(this),o,argc,argv);
    queue.Push(m);
}

void flext_base::ToQueueAnything(int o,const t_symbol *s,int argc,const t_atom *argv) const
{
    MsgBundle *m = MsgBundle::New();
    m->Add(const_cast<flext_base *>(this),o,s,argc,argv);
    queue.Push(m);
}


void flext_base::MsgAddBang(MsgBundle *m,int n) const 
{ 
    m->Add(const_cast<flext_base *>(this),n);
}

void flext_base::MsgAddFloat(MsgBundle *m,int n,float f) const
{
    m->Add(const_cast<flext_base *>(this),n,f);
}

void flext_base::MsgAddInt(MsgBundle *m,int n,int f) const
{
    m->Add(const_cast<flext_base *>(this),n,f);
}

void flext_base::MsgAddSymbol(MsgBundle *m,int n,const t_symbol *s) const
{
    m->Add(const_cast<flext_base *>(this),n,s);
}

void flext_base::MsgAddAtom(MsgBundle *m,int n,const t_atom &at) const
{
    m->Add(const_cast<flext_base *>(this),n,at);
}

void flext_base::MsgAddList(MsgBundle *m,int n,int argc,const t_atom *argv) const
{
    m->Add(const_cast<flext_base *>(this),n,argc,argv);
}

void flext_base::MsgAddAnything(MsgBundle *m,int n,const t_symbol *s,int argc,const t_atom *argv) const
{
    m->Add(const_cast<flext_base *>(this),n,s,argc,argv);
}




bool flext::SysForward(const t_symbol *recv,const t_symbol *s,int argc,const t_atom *argv)
{
    void *cl = recv->s_thing;
    if(UNLIKELY(!cl)) return false;
    
#if FLEXT_SYS == FLEXT_SYS_PD
    pd_typedmess((t_class **)cl,(t_symbol *)s,argc,(t_atom *)argv);
#elif FLEXT_SYS == FLEXT_SYS_MAX
    typedmess(recv->s_thing,(t_symbol *)s,argc,(t_atom *)argv);
#else
#   error Not implemented
#endif
    return true;
}

bool flext::QueueForward(const t_symbol *recv,const t_symbol *s,int argc,const t_atom *argv)
{
    MsgBundle *m = MsgBundle::New();
    m->Add(recv,s,argc,argv);
    // send over queue
    queue.Push(m);
    return true;
}

bool flext::MsgForward(MsgBundle *m,const t_symbol *recv,const t_symbol *s,int argc,const t_atom *argv)
{
    m->Add(recv,s,argc,argv);
    return true;
}

void flext_base::AddIdle()
{
    MsgBundle *m = MsgBundle::New();
    m->Idle(const_cast<flext_base *>(this));
    // send over queue
    queue.Push(m);
}

void flext_base::AddIdle(bool (*idlefun)(int argc,const t_atom *argv),int argc,const t_atom *argv)
{
    MsgBundle *m = MsgBundle::New();
    m->Idle(idlefun,argc,argv);
    // send over queue
    queue.Push(m);
}

#include "flpopns.h"

