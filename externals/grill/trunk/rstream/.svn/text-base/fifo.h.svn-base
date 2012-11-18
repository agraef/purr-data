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
	Fifo(int n = 0,int c = 1):  channels(c),size(n), arr(new T[n*c]),rd(0),wr(0),have(0) {}
	~Fifo()	{ if(arr) delete[] arr;	}

	//! Clear fifo
	void Clear() { rd = wr = have = 0; }

	//! Get channel count
	int Channels() const { return channels; }
	//! Get total fifo size
	int Size() const { return size; }
	//! Get number of items in fifo
	int Have() const { return have; }
	//! Get free fifo size
	int Free() const { return size-have; }

	//! Get pointer to beginning of contained data
	T *ReadPtr() const { return arr+rd*channels; }
	//! Get number of items from read pos to end of contiguous space
	int ReadSamples() const { return size-rd; }
	//! Get pointer to beginning of free space
	T *WritePtr() const { return arr+wr*channels; }
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
			T *narr = new T[nsz*channels];
			// try to keep newest data
			int s = Have()-nsz;
			if(s > 0) {
				// skip items that don't fit into new buffer
				all = false;
				int r = Read(s);
				ASSERT(r == s);
			}
			s = Have();
			int r = Read(s,narr);
			ASSERT(r == s);
			
			delete[] arr;

			arr = narr; 
            size = nsz;
            rd = 0;
            wr = nsz == r?0:r;
            have = r;
            
            return all;
		}
		else {
			delete[] arr;
			arr = new T[(size = nsz)*channels];
            rd = wr = have = 0;
			return false;
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
        if(!n) return 0;

		if(wr+n >= size) {
			// wrap-over
			int ch1 = size-wr;
            int wr1 = n-ch1;
            const int chunk = channels*sizeof(T);
			memcpy(arr+wr*channels,buf,ch1*chunk);
			memcpy(arr,buf+ch1*channels,wr1*chunk);
			wr = wr1;
		}
		else {
			memcpy(arr+wr*channels,buf,n*channels*sizeof(T));
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
	int Read(int n,T *buf = 0)
	{
		if(n > Have()) n = Have();
        if(!n) return 0;
		
		if(rd+n >= size) {
			// wrap-over
			int ch1 = size-rd;
            int rd1 = n-ch1;
            if(buf) {
                const int chunk = channels*sizeof(T);
                memcpy(buf,arr+rd*channels,ch1*chunk);
			    memcpy(buf+ch1*channels,arr,rd1*chunk);
            }
			rd = rd1;
		}
		else {
			if(buf) memcpy(buf,arr+rd*channels,n*channels*sizeof(T));
			rd += n;
		}

		have -= n;
		return n;
	}

	/*! Write data to fifo 
		\param n	number of items to write
		\param buf	memory buffer containing items
		\return		number of written items
	*/
	int Write(int n,const T *const *buf)
	{
		if(n > Free()) n = Free();
        if(!n) return 0;

		if(wr+n >= size) {
			// wrap-over
			int ch1 = size-wr;
            int wr1 = n-ch1;
            for(int i = 0; i < channels; ++i) {
                WriteCopy(arr+i,wr,buf[i],ch1);
                WriteCopy(arr+i,0,buf[i]+ch1,wr1);
            }
			wr = wr1;
		}
		else {
            for(int i = 0; i < channels; ++i)
	            WriteCopy(arr+i,wr,buf[i],n);
			wr += n;
		}

		have += n;
		return n;
	}

	/*! Read data from fifo 
		\param n	number of items to read
		\param buf	memory buffer to store items
		\return		number of read items
	*/
	int Read(int n,T *const *buf)
	{
		if(n > Have()) n = Have();
        if(!n) return 0;

		if(rd+n >= size) {
			// wrap-over
			int ch1 = size-rd;
            int rd1 = n-ch1;
            for(int i = 0; i < channels; ++i) {
                ReadCopy(buf[i],arr+i,rd,ch1);
                ReadCopy(buf[i]+ch1,arr+i,0,rd1);
            }
			rd = rd1;
		}
		else {
            for(int i = 0; i < channels; ++i)
                ReadCopy(buf[i],arr+i,rd,n);
			rd += n;
		}

		have -= n;
		return n;
	}

protected:
    inline void WriteCopy(T *dst,int dix,const T *src,int n)
    {
        const int c = channels;
        const T *d = dst+dix*c;
        if(c == 1)
			memcpy(d,src,n*c*sizeof(T));
        else
            for(int i = 0; i < n; d += c) *d = src[i++];
    }

    inline void ReadCopy(T *dst,const T *src,int six,int n)
    {
        const int c = channels;
        const T *s = src+six*c;
        if(c == 1)
			memcpy(dst,s,n*c*sizeof(T));
        else
            for(int i = 0; i < n; s += c) dst[i++] = *s;
    }

    const int channels;
	int size;
	int rd,wr,have;
	mutable T *arr;
};

#endif
