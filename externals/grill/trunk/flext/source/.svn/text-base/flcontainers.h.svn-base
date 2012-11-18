/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file flcontainers.h
	\brief Lock-free container classes
*/

#ifndef __FLCONTAINERS_H
#define __FLCONTAINERS_H

#include "flprefix.h"

#include "lockfree/stack.hpp"
#include "lockfree/fifo.hpp"

#include "flpushns.h"

class LifoCell: public lockfree::stack_node {};

class Lifo
	: public lockfree::intrusive_stack<LifoCell>
{
public:
	inline void Push(LifoCell *cell) { this->push(cell); }
	inline LifoCell *Pop() { return this->pop(); }
	inline bool Avail() const { return !this->empty(); }
};

template <typename T>
class TypedLifo
    : public Lifo
{
public:
    inline void Push(T *c) { Lifo::Push(static_cast<T *>(c)); }
    inline T *Pop() { return static_cast<T *>(Lifo::Pop()); }
};

template <typename T>
class ValueLifoCell
	: public LifoCell 
{
public:
	ValueLifoCell(T v): value(v) {}
	T value;
};

template <typename T>
class ValueLifo
    : public TypedLifo<ValueLifoCell<T> >
{
public:
    inline void Push(T v) 
	{ 
		TypedLifo<ValueLifoCell<T> >::Push(new ValueLifoCell<T>(v)); 
	}

    inline T Pop() 
	{
		ValueLifoCell<T> *p = TypedLifo<ValueLifoCell<T> >::Pop(); 
		T v = p->value;
		delete p; 
		return v;
	}
};

template <typename T,int M = 2,int O = 1>
class PooledLifo
    : public TypedLifo<T>
{
public:
	PooledLifo(): sz(0),resz(0) {}

	void Push(T *c) { TypedLifo<T>::Push(c); ++sz; }
	T *Pop() { T *r = TypedLifo<T>::Pop(); if(r) --sz; return r; }

    T *New() 
	{ 
		T *n = reuse.Pop(); 
		if(n) {
			--resz;
			return n;
		}
		else
			return new T; 
	}

    inline void Free(T *p) 
	{ 
		if(resz < sz*M+O) { reuse.Push(p); ++resz; }
		else delete p; 
	}
private:
    TypedLifo<T> reuse;
	size_t sz,resz;
};


class FifoCell: public lockfree::fifo_node {};

class Fifo
	: public lockfree::intrusive_fifo<FifoCell>
{
public:
	inline void Put(FifoCell *cl) { this->enqueue(cl); }
	inline FifoCell *Get() { return this->dequeue(); }
    inline bool Avail() const { return !this->empty(); }
};


template <typename T>
class TypedFifo
    : public Fifo
{
public:
    inline void Put(T *c) { Fifo::Put(static_cast<T *>(c)); }
    inline T *Get() { return static_cast<T *>(Fifo::Get()); }
};


template <typename T>
class ValueFifoCell
	: public FifoCell 
{
public:
	ValueFifoCell(T v): value(v) {}
	T value;
};

template <typename T>
class ValueFifo
    : public TypedFifo<ValueFifoCell<T> >
{
public:
    inline void Put(T v) 
	{ 
		TypedFifo<ValueFifoCell<T> >::Put(new ValueFifoCell<T>(v)); 
	}

    inline T Get() 
	{
		ValueFifoCell<T> *p = TypedFifo<ValueFifoCell<T> >::Get(); 
		T v = p->value;
		delete p; 
		return v;
	}
};


template <typename T,int M = 2,int O = 1>
class PooledFifo
    : public TypedFifo<T>
{
public:
    ~PooledFifo() { T *n; while((n = reuse.Get()) != NULL) delete n; }

    inline T *New() { T *n = reuse.Get(); return n?n:new T; }
    inline void Free(T *p) { if(resz < sz*M+O) reuse.Put(p); else delete p; }
private:
    TypedFifo<T> reuse;
	size_t sz,resz;
};

#include "flpopns.h"

#endif

