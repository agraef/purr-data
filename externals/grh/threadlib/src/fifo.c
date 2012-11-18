/* 
 * fifo.c
 * this is the lockfree fifo implementation of pd_devel_0.39
 *
 * Copyright (c) 2004, Tim Blechmann
 * supported by vibrez.net
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt" in this distribution.  */


#include "threadlib.h"
#include "stddef.h"


#ifndef THREADLIB_LOCKFREE

/* we always have the implementation for posix systems with threadlocks */

#include "errno.h"

typedef struct _fifocell
{
  struct _fifocell* next;
  void* data;            /* pointer to our data */
} t_fifocell;

struct _fifo
{
  t_fifocell * head;
  t_fifocell * tail;
  pthread_mutex_t mutex;
};


t_fifo * threadlib_fifo_init()
{
  t_fifo* ret = (t_fifo*) getbytes(sizeof (t_fifo));
  t_fifocell * fifo_begin = (t_fifocell*) getbytes (sizeof (t_fifocell) );
	
  fifo_begin->data = NULL;
  fifo_begin->next = NULL;

  ret->head = fifo_begin;
  ret->tail = fifo_begin;
	
  pthread_mutex_init(&ret->mutex, NULL);
	
  pthread_mutex_unlock(&ret->mutex);

  return ret;
}

void threadlib_fifo_destroy(t_fifo* fifo)
{
  void * data;
	
  do
  {
    data = threadlib_fifo_get(fifo);
  }
  while (data != NULL);

  pthread_mutex_lock(&fifo->mutex);
  pthread_mutex_destroy(&fifo->mutex);
	
  freebytes(fifo, sizeof(t_fifo));
  return;
}

/* fifo_put and fifo_get are the only threadsafe functions!!! */
void threadlib_fifo_put(t_fifo* fifo, void* data)
{
  if (data != NULL)
  {
    t_fifocell * cell = (t_fifocell*) getbytes(sizeof(t_fifocell));
		
    cell->data = data;
    cell->next = NULL;
		
    pthread_mutex_lock(&fifo->mutex);
		
    fifo->tail->next = cell;
    fifo->tail = cell;
		
    pthread_mutex_unlock(&fifo->mutex);
  }
  return;
}


/* this fifo_get returns NULL if the fifo is empty 
 * or locked by another thread */
void* threadlib_fifo_get(t_fifo* fifo)
{
  t_fifocell * cell;
  void* data;
	
  if(pthread_mutex_trylock(&fifo->mutex) != EBUSY)
  {
    cell = fifo->head->next;

    if (cell != NULL)
    {
      fifo->head->next = cell->next;
      if(cell == fifo->tail)
	fifo->tail = fifo->head;
      data = cell->data;
			
      freebytes (cell, sizeof(t_fifocell));
    }
    else
      data = NULL;

    pthread_mutex_unlock(&fifo->mutex);
  }
  else
    data = NULL;
  return data;
}

#else /* THREADLIB_LOCKFREE */

/* 
   lockfree fifo adapted from the midishare: Copyright � Grame 1999
   Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
   grame@rd.grame.fr
*/



typedef struct _fifocell
{
  struct _fifocell* next;
  void* data;            /* pointer to our data */
} t_fifocell;

typedef struct _lifo
{
  unsigned long ic;       /* operation counter */
  t_fifocell* top;        /* stack pointer */
  unsigned long oc;       /* operation counter */
#ifdef __POWERPC__
	long 	unused [5];		/* lifo size must be at least 32 bytes */
							/* to avoid livelock in multiprocessor */
#endif
} t_lifo;

struct _fifo
{
  t_lifo in;
  t_lifo out;
};

/* platform dependent code */

#ifdef __SMP__
#define LOCK lock ;
#else
#define LOCK
#endif

#if defined(__GNUC__) && defined(__POWERPC__)

static void* lifo_pop(t_lifo* lifo)
{
  register void * data;
  register volatile long a, b;
  register long c=0;
  asm volatile (
      "# LFPOP					\n"
      "0:						\n"
      "	lwarx	%4, %1, %2	\n"         /* creates a reservation on lf    */
      "	cmpwi	%4, 0		\n"         /* test if the lifo is empty      */
      "	beq-	1f		\n"
      "	lwz		%5, 0(%4)	\n"         /* next cell in b                */
      "	sync            	\n"         /* synchronize instructions       */
      "	stwcx.	%5, %1, %2	\n"         /* if the reservation is not altered */
      /* modify lifo top                */
      "	bne-	0b  		\n"         /* otherwise: loop and try again  */
      "0:						\n"
      "	lwarx	%5, %1, %3	\n"         /* creates a reservation on lf->count */
      "	addi	%5, %5, -1	\n"         /* dec count                      */
      "	sync            	\n"         /* synchronize instructions       */
      "	stwcx.	%5, %1, %3	\n"         /* conditionnal store             */
      "	bne-	0b			\n"
      "1:						\n"
      "	mr		%0, %4		\n"
  :"=r" (data), "=r" (c)
  : "r" (&lifo->top), "r" (&lifo->oc), "r" (a), "r" (b), "1" (c)
  : "r0" 		/* prevents using r0 because of the ambiguity of 'addi' coding: */
      /* gcc version 2.95.3 20010315 (release - Linux-Mandrake 8.0 for PPC) */
      /* compiles the instruction "addi 0, 0, n" as li 0, n */
	       );
  return data;
}

static void* lifo_push(register t_lifo* lifo, register void* data)
{
  register volatile long t1;
  register long t2=0;
  asm volatile (
      "# LFPUSH \n"
      "0: 				      \n"
      "   lwarx   %0, %3, %1  \n"		
      "   stw	  %0, 0(%2)   \n"	
      "   sync  			  \n"	
      "   stwcx.  %2, %3, %1  \n"						   
      "   bne-    0b	      \n"  
      "0:				      \n"
      "   lwarx   %0, %3, %4  \n"		
      "   addi    %0, %0, 1	  \n"  
      "   sync  			  \n"  
      "   stwcx.  %0, %3, %4  \n"
      "   bne-    0b		  \n"
  : "=r" (t1)
  : "r" (&lifo->top), "r" (data), "r" (t2), "r" (&lifo->oc), "0" (t1)
  : "r0" 		/* prevents using r0 because of the ambiguity of 'addi' coding: */
      /* gcc version 2.95.3 20010315 (release - Linux-Mandrake 8.0 for PPC) */
      /* compiles the instruction "addi 0, 0, n" as li 0, n */
	       );
}

#elif defined(__Macintosh__) || defined(__MacOSX__)

static void* lifo_pop(t_lifo* lifo)
{
  register cell * data;
  register long a, b;
  asm {
    addi	lifo, lifo, 4
    loop:
    lwarx	a, 0, lifo       /* creates a reservation on lifo        */
    cmpwi	a, 0             /* test if the lifo is empty            */
    beq-	empty
    lwz		b, 0(a)          /* next cell in b                       */
    sync                         /* synchronize instructions             */
    stwcx.	b, 0, lifo       /* if the reservation is not altered    */
                                 /* modify lifo top                      */
    bne-	loop             /* otherwise: loop and try again        */

    addi	lifo, lifo, 4
    dec:
    lwarx	b, 0, lifo       /* creates a reservation on lifo->count */
    addi	b, b, -1         /* dec count                            */
    sync                         /* synchronize instructions             */
    stwcx.	b, 0, lifo       /* conditionnal store                   */
    bne-	dec
 
    empty:
    mr		data, a
  }
  return data;
}

static void lifo_push (register t_lifo * lifo, register void * data) 
{
  register long tmp;
  asm {
    addi	lifo, lifo, 4
    loop:
    lwarx	tmp, 0, lifo     /* creates a reservation on lifo        */
    stw		tmp, 0(data)     /* link the new cell to the lifo        */
    sync                         /* synchronize instructions             */
    stwcx.	data, 0, lifo    /* if the reservation is not altered    */
                                 /* modify lifo top                      */
    bne-	loop             /* otherwise: loop and try again        */

    addi	lifo, lifo, 4
    inc:
    lwarx	tmp, 0, lifo     /* creates a reservation on lifo->count */
    addi	tmp, tmp, 1      /* inc count                            */
    sync                         /* synchronize instructions             */
    stwcx.	tmp, 0, lifo     /* conditionnal store                   */
    bne-	inc 
  }
}



#elif defined(__GNUC__)  && (defined(_X86_) || defined(__i386__) || defined(__i586__) || defined(__i686__))

static void* lifo_pop(t_lifo* lifo)
{
  void * data = 0;
  __asm__ __volatile__ (
      "# LFPOP 					\n\t"
      "pushl	%%ebx				\n\t"
      "pushl	%%ecx				\n\t"
      "movl 	4(%%esi), %%edx		\n\t"
      "movl  	(%%esi), %%eax		\n\t"	
      "testl	%%eax, %%eax		\n\t"
      "jz		20f					\n"
      "10:\t"
      "movl 	(%%eax), %%ebx		\n\t"
      "movl	%%edx, %%ecx		\n\t"
      "incl	%%ecx				\n\t"
      LOCK "cmpxchg8b (%%esi)		\n\t"
      "jz		20f					\n\t"
      "testl	%%eax, %%eax		\n\t"
      "jnz	10b					\n"
      "20:\t"
      "popl	%%ecx				\n\t"
      "popl	%%ebx				\n\t"
  :"=a" (data)
  :"S" (&lifo->top)
  :"memory", "edx");
  return data;			 
}

static void lifo_push(t_lifo * lifo, void * data) 
{
  __asm__ __volatile__ (
      "# LFPUSH					\n\t"
      "pushl	%%ebx				\n\t"
      "pushl	%%ecx				\n\t"
      "movl 0(%%esi), %%eax		\n\t"
      "movl 4(%%esi), %%edx		\n"	
      "1:\t"
      "movl %%eax, %%ebx			\n\t"
      "incl %%ebx					\n\t"
      "movl %%edx, (%%ecx)		\n\t"
      LOCK "cmpxchg8b (%%esi)		\n\t"
      "jnz	1b					\n\t"
      "popl	%%ecx				\n\t"
      "popl	%%ebx				\n\t"
  :/* no output */
  :"S" (lifo), "c" (data)
  :"memory", "eax", "edx");
}

#elif defined(__GNUC__)  && defined(__x86_64__)

/* this will not work for all revisions of the amd64 architecture ... */

static void* lifo_pop(t_lifo* lifo)
{
  void * data = 0;
  __asm__ __volatile__ (
      "# LFPOP 					\n\t"
      "push	%%rbx				\n\t"
      "push	%%rcx				\n\t"
      "mov 	8(%%rdi), %%rdx		\n\t"
      "mov  	(%%rdi), %%rax		\n\t"	
      "test	%%rax, %%rax		\n\t"
      "jz		20f					\n"
      "10:\t"
      "mov 	(%%rax), %%rbx		\n\t"
      "mov	%%rdx, %%rcx		\n\t"
      "inc	%%rcx				\n\t"
      LOCK "cmpxchg16b (%%rdi)		\n\t"
      "jz		20f					\n\t"
      "test	%%rax, %%rax		\n\t"
      "jnz	10b					\n"
      "20:\t"
      "pop	%%rcx				\n\t"
      "pop	%%rbx				\n\t"
  :"=a" (data)
  :"D" (&lifo->top)
  :"memory", "rdx");
  return data;			 
}

static void lifo_push(t_lifo * lifo, void * data) 
{
  __asm__ __volatile__ (
      "# LFPUSH					\n\t"
      "push	%%rbx				\n\t"
      "push	%%rcx				\n\t"
      "mov 0(%%rdi), %%rax		\n\t"
      "mov 8(%%rdi), %%rdx		\n"	
      "1:\t"
      "mov %%rax, %%rbx			\n\t"
      "inc %%rbx					\n\t"
      "mov %%rdx, (%%rcx)		\n\t"
      LOCK "cmpxchg16b (%%rdi)		\n\t"
      "jnz	1b					\n\t"
      "pop	%%rcx				\n\t"
      "pop	%%rbx				\n\t"
  :/* no output */
  :"D" (lifo), "c" (data)
  :"memory", "rax", "rdx");
}

#elif defined(_WIN32) && defined(_MSC_VER)

static void* lifo_pop(t_lifo* lifo)
{
  __asm 
  {
        push	ebx
	push	ecx
	push	edx
	push	esi
	mov		esi, lifo
	add		esi, 4
	mov 	edx, dword ptr [esi+4]
	mov  	eax, dword ptr [esi]	
	test	eax, eax
	jz		_end
  _loop:
	mov		ebx, dword ptr [eax]
	mov		ecx, edx
	inc		ecx
	LOCK cmpxchg8b qword ptr [esi]
	jz		_end
	test	eax, eax
	jnz		_loop
  _end:
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
  }
}

static void lifo_push(t_lifo * lifo, void * data) 
{
  __asm 
  {
        push	eax
	push	ebx
	push	ecx
	push	edx
	push	esi
	mov		esi, lifo
	mov		eax, dword ptr [esi]
	mov		ecx, data
	mov		edx, dword ptr 4[esi]
  _loop:
        mov		ebx, eax
	inc		ebx
	mov		[ecx], edx
	LOCK cmpxchg8b qword ptr [esi]
        jnz		_loop
	pop		esi
	pop		edx
	pop		ecx
	pop		ebx
	pop		eax
  }
}
 
 
#else
#error lockfree fifos not available on this platform
#endif



static void lifo_init(t_lifo* lifo)
{
  lifo->ic = 0;
  lifo->top = NULL;
  lifo->oc = 0;
}

t_fifo* threadlib_fifo_init(void)
{
  t_fifo* ret = (t_fifo*) getbytes(sizeof(t_fifo));
	
  lifo_init(&ret->in);
  lifo_init(&ret->out);
	
  return ret;
}


void threadlib_fifo_destroy(t_fifo* fifo)
{
  void * data;
  do
  {
    data = threadlib_fifo_get(fifo);
  }
  while (data != NULL);

  freebytes(fifo, sizeof(t_fifo));
  return;
}

void threadlib_fifo_put(t_fifo* fifo, void* data)
{
  lifo_push(&fifo->in, data);
}

void* threadlib_fifo_get(t_fifo* fifo)
{
  void * data;
  t_lifo *out = &fifo->out;
	
  data = lifo_pop(out);
	
  if (!data)
  {
    void * tmp;
    t_lifo *in = &fifo->in;
    data = lifo_pop(in);

    if (data)
    {
      while((tmp = lifo_pop(in)))
      {
	lifo_push(out, data);
	data = tmp;
      }
    }
		
  }
  return data;
}

#endif
