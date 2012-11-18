/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1233 $
$LastChangedDate: 2008-01-17 11:11:19 -0500 (Thu, 17 Jan 2008) $
$LastChangedBy: thomas $
*/

/*! \file fldefs_meththr.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_METHTHR_H
#define __FLEXT_DEFS_METHTHR_H


#ifdef FLEXT_THREADS


/*!	\defgroup FLEXT_D_THREAD Declare threaded method callbacks
	@{ 
*/

//! Set up a threaded method callback with no arguments
#define FLEXT_THREAD(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c) {  \
	thr_params *p = new thr_params; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	delete p; \
	if(ok) { \
		th->M_FUN(); \
		th->PopThread(); \
	} \
} 

//! Set up a threaded method callback for an anything argument
#define FLEXT_THREAD_A(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,t_symbol *s,int argc,t_atom *argv) {  \
	thr_params *p = new thr_params; p->set_any(s,argc,argv); \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	AtomAnything *args = p->var[0]._any; \
	delete p; \
	if(ok) { \
		th->M_FUN(args->Header(),args->Count(),args->Atoms()); \
		th->PopThread(); \
	} \
	delete args; \
} 

//! Set up a threaded method callback for a variable argument list
#define FLEXT_THREAD_V(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,int argc,t_atom *argv) {  \
	thr_params *p = new thr_params; p->set_list(argc,argv); \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	AtomList *args = p->var[0]._list; \
	delete p; \
	if(ok) { \
		th->M_FUN(args->Count(),args->Atoms()); \
		th->PopThread(); \
	} \
	delete args; \
} 

/*! \brief Set up a threaded method callback for an arbitrary data struct.
	\note Data is pure... no destructor is called upon delete
*/
#define FLEXT_THREAD_X(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,void *data) {  \
	thr_params *p = new thr_params; p->var[0]._ext = data; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	void *data = p->var[0]._ext; \
	delete p; \
	if(ok) { \
		th->M_FUN(data); \
		th->PopThread(); \
	} \
	/* delete (char *)data; */ \
} 

//! Set up a threaded method callback for a boolean argument
#define FLEXT_THREAD_B(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,int &arg1) {  \
	thr_params *p = new thr_params; p->var[0]._bool = arg1 != 0; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	bool b = p->var[0]._bool; \
	delete p; \
	if(ok) { \
		th->M_FUN(b); \
		th->PopThread(); \
	} \
} 

//! Set up a threaded method callback for 1 argument
#define FLEXT_THREAD_1(M_FUN,TP1) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1) {  \
	thr_params *p = new thr_params(1); \
	p->var[0]._ ## TP1 = arg1; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	const TP1 v1 = p->var[0]._ ## TP1; \
	delete p; \
	if(ok) { \
		th->M_FUN(v1); \
		th->PopThread(); \
	} \
} 

//! Set up a threaded method callback for 2 arguments
#define FLEXT_THREAD_2(M_FUN,TP1,TP2) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2) {  \
	thr_params *p = new thr_params(2); \
	p->var[0]._ ## TP1 = arg1; \
	p->var[1]._ ## TP2 = arg2; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	const TP1 v1 = p->var[0]._ ## TP1; \
	const TP1 v2 = p->var[1]._ ## TP2; \
	delete p; \
	if(ok) { \
		th->M_FUN(v1,v2); \
		th->PopThread(); \
	} \
} 

//! Set up a threaded method callback for 3 arguments
#define FLEXT_THREAD_3(M_FUN,TP1,TP2,TP3) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2,TP3 &arg3) {  \
	thr_params *p = new thr_params(3); \
	p->var[0]._ ## TP1 = arg1; \
	p->var[1]._ ## TP2 = arg2; \
	p->var[2]._ ## TP3 = arg3; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	const TP1 v1 = p->var[0]._ ## TP1; \
	const TP2 v2 = p->var[1]._ ## TP2; \
	const TP3 v3 = p->var[2]._ ## TP3; \
	delete p; \
	if(ok) { \
		th->M_FUN(v1,v2,v3); \
		th->PopThread(); \
	} \
} 

//! Set up a threaded method callback for 4 arguments
#define FLEXT_THREAD_4(M_FUN,TP1,TP2,TP3,TP4) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2,TP3 &arg3,TP4 &arg4) {  \
	thr_params *p = new thr_params(4); \
	p->var[0]._ ## TP1 = arg1; \
	p->var[1]._ ## TP2 = arg2; \
	p->var[2]._ ## TP3 = arg3; \
	p->var[3]._ ## TP4 = arg4; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	const TP1 v1 = p->var[0]._ ## TP1; \
	const TP2 v2 = p->var[1]._ ## TP2; \
	const TP3 v3 = p->var[2]._ ## TP3; \
	const TP4 v4 = p->var[3]._ ## TP4; \
	delete p; \
	if(ok) { \
		th->M_FUN(v1,v2,v3,v4); \
		th->PopThread(); \
	} \
} 

//! Set up a threaded method callback for 5 arguments
#define FLEXT_THREAD_5(M_FUN,TP1,TP2,TP3,TP4,TP5) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2,TP3 &arg3,TP4 &arg4,TP5 &arg5) {  \
	thr_params *p = new thr_params(5); \
	p->var[0]._ ## TP1 = arg1; \
	p->var[1]._ ## TP2 = arg2; \
	p->var[2]._ ## TP3 = arg3; \
	p->var[3]._ ## TP4 = arg4; \
	p->var[4]._ ## TP5 = arg5; \
	return c->StartThread(FLEXT_THR_PRE(M_FUN),p,#M_FUN); \
} \
static void FLEXT_THR_PRE(M_FUN)(thr_params *p) {  \
	thisType *th = FLEXT_CAST<thisType *>(p->cl); \
	bool ok = th->PushThread(); \
	const TP1 v1 = p->var[0]._ ## TP1; \
	const TP2 v2 = p->var[1]._ ## TP2; \
	const TP3 v3 = p->var[2]._ ## TP3; \
	const TP4 v4 = p->var[3]._ ## TP4; \
	const TP5 v5 = p->var[4]._ ## TP5; \
	delete p; \
	if(ok) { \
		th->M_FUN(v1,v2,v3,v4,v5); \
		th->PopThread(); \
	} \
} 


//!	Shortcuts

//! Set up a threaded method callback for 1 float argument
#define FLEXT_THREAD_F(M_FUN) \
\
FLEXT_THREAD_1(M_FUN,float)

//! Set up a threaded method callback for 2 float arguments
#define FLEXT_THREAD_FF(M_FUN) \
\
FLEXT_THREAD_2(M_FUN,float,float)

//! Set up a threaded method callback for 3 float arguments
#define FLEXT_THREAD_FFF(M_FUN) \
\
FLEXT_THREAD_3(M_FUN,float,float,float)

//! Set up a threaded method callback for 1 integer argument
#define FLEXT_THREAD_I(M_FUN) \
\
FLEXT_THREAD_1(M_FUN,int)

//! Set up a threaded method callback for 2 integer arguments
#define FLEXT_THREAD_II(M_FUN) \
\
FLEXT_THREAD_2(M_FUN,int,int)

//! Set up a threaded method callback for 3 integer arguments
#define FLEXT_THREAD_III(M_FUN) \
\
FLEXT_THREAD_3(M_FUN,int,int,int)

//! Set up a threaded method callback for 1 symbol argument
#define FLEXT_THREAD_S(M_FUN) \
\
FLEXT_THREAD_1(M_FUN,t_symptr)

// deprecated
#define FLEXT_THREAD_G FLEXT_THREAD_V

//! @} FLEXT_D_THREAD


#endif // FLEXT_THREADS


#endif
