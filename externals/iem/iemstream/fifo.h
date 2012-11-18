/*************************************************************
 *
 *    streaming external for PD
 *
 * File: fifo.h
 *
 * Description: Implementation of a FIFO template class
 *
 * Author: Thomas Grill (t.grill@gmx.net)
 *
 *************************************************************/

#ifndef _FIFO_H_
#define _FIFO_H_

#include <memory.h>

#include <assert.h>
#define ASSERT assert


/*! FIFO class
	\note not thread-safe
*/
template<class T>
class Fifo
{
public:
	Fifo(int n = 0):  size(n), arr(new T[n]),rd(0),wr(0),have(0) {}
	~Fifo()	{ if(arr) delete[] arr;	}

	//! Clear fifo
	void Clear() { rd = wr = have = 0; }

	//! Get total fifo size
	int Size() const { return size; }
	//! Get number of items in fifo
	int Have() const { return have; }
	//! Get free fifo size
	int Free() const { return size-have; }

	//! Get pointer to beginning of contained data
	T *ReadPtr() const { return arr+rd; }
	//! Get number of items from read pos to end of contiguous space
	int ReadSamples() const { return size-rd; }
	//! Get pointer to beginning of free space
	T *WritePtr() const { return arr+wr; }
	//! Get number of items from write pos to end of contiguous space
	int WriteSamples() const { return size-wr; }

	/*! Resize FIFO
		\param nsz	new FIFO size
		\param keep	keep data?
		\return true if all data could be kept
	*/
	bool Resize(int nsz,bool keep)
	{
		if(keep) {
			bool all = true;
			T *narr = new T[nsz];
			// try to keep newest data
			int s = Have()-nsz;
			if(s > 0) {
				// skip items that don't fit into new buffer
				all = false;
				int r = Read(s,NULL);
				ASSERT(r == s);
			}
			s = Have();
			int r = Read(s,narr);
			ASSERT(r == s);
			
			delete[] arr;
			arr = narr; size = nsz;
			Clear();
			return all;
		}
		else {
			delete[] arr;
			arr = new T[size = nsz];
			Clear();
			return true;
		}
	}

	/*! Write data to fifo 
		\param n	number of items to write
		\param buf	memory buffer containing items
		\return		number of written items
	*/
	int Write(int n,const T *buf)
	{
		if(n > Free()) n = Free();

		if(wr+n >= size) {
			// wrap-over
			int ch1 = size-wr;
			memcpy(arr+wr,buf,ch1*sizeof(T));
			wr = n-ch1;
			memcpy(arr,buf+ch1,wr*sizeof(T));
		}
		else {
			memcpy(arr+wr,buf,n*sizeof(T));
			wr += n;
		}

		have += n;
		return n;
	}

	/*! Read data from fifo 
		\param n	number of items to read
		\param buf	memory buffer to store items (NULL to simply skip items)
		\return		number of read items
	*/
	int Read(int n,T *buf)
	{
		if(n > Have()) n = Have();
		
		if(rd+n >= size) {
			// wrap-over
			int ch1 = size-rd;
			if(buf) memcpy(buf,arr+rd,ch1*sizeof(T));
			rd = n-ch1;
			if(buf) memcpy(buf+ch1,arr,rd*sizeof(T));
		}
		else {
			if(buf) memcpy(buf,arr+rd,n*sizeof(T));
			rd += n;
		}

		have -= n;
		return n;
	}

protected:
	int size;
	int rd,wr,have;
	mutable T *arr;
};

#endif
