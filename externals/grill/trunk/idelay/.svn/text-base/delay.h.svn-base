/* 
idelay~ - interpolating delay

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#ifndef _IDELAY_DELAY_H
#define _IDELAY_DELAY_H

template <class T>
class DelayLine
{ 
public:
	DelayLine(): buf(NULL),len(0),wpos(0) {}
	DelayLine(int smps): buf(NULL),len(0),wpos(0) { SetLen(smps); }
	~DelayLine() { if(buf) delete[] buf; }

	void SetLen(int smps)
    {
        if(len != smps) {
            if(buf) delete[] buf;
            len = smps;
            buf = new T[dim = (len+4)]; // some more samples for interpolation
            memset(buf,0,dim*sizeof(T)); 
            wpos = 4; 
        }
    }

	void Put(T s)
    {
        if(UNLIKELY(!len)) return;
        buf[wpos] = s;
        if(UNLIKELY(wpos >= len)) buf[wpos-len] = s; // initialize interpolation security zone
        ++wpos;
        if(UNLIKELY(wpos == dim)) wpos -= len;
    }
	
	T Get(float delsmps)
    {
        if(UNLIKELY(!len)) return 0;
        if(delsmps <= 0) 
            return buf[wpos-1];
        else {
            if(UNLIKELY(delsmps > len-1)) delsmps = len-1;
            int idelsmps = (int)delsmps;

            if(idelsmps == 0) {
                const T *bp = buf+wpos;
                const float r = 1.f-delsmps;
                return (float)(r*(r-1)*bp[-3]/2.+(1-r*r)*bp[-2]+r*(r+1)*bp[-1]/2.);
            }
            else {
                const T *bp = buf+wpos-idelsmps;
                if(bp < (const T *)buf+4) bp += len;
                const float r = 1.f-(delsmps-idelsmps);
                return (float)(
                    ((2-r)*(r-1)*r*bp[-3])/6. + 
                    ((r-2)*(r*r-1)*bp[-2])/2. + 
                    ((2-r)*r*(r+1)*bp[-1])/2. + 
                    ((r*r-1)*r*bp[0])/6.
                );
            }
        }
    }

    void Clear()
    {
		memset(buf,0,dim*sizeof(*buf)); 
    }

	T *buf; 
	int dim,wpos,len;
};

#endif

