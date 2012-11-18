/* 
xsample - extended sample objects for Max/MSP and pd (pure data)

Copyright (c) 2001-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 39 $
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __INTER_H
#define __INTER_H

TMPLDEF void xinter::st_play0(const Element *,const int ,const int ,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs,bool looped)
{
	// stopped/invalid buffer -> output zero
	for(int ci = 0; ci < outchns; ++ci) ZeroSamples(outvecs[ci],n);
}

TMPLDEF void xinter::st_play1(const Element *bdt,const int smin,const int smax,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs,bool looped)
{
	SIGCHNS(BCHNS,inchns,OCHNS,outchns);

	// position info are frame units
	const t_sample *pos = invecs[0];
	t_sample *const *sig = outvecs;
	
	// no interpolation
	// ----------------

    if(UNLIKELY(smin == smax)) {
        // zero loop length -> assume that smin is a valid sample position...

        int ci;
        for(ci = 0; ci < OCHNS; ++ci) SetSamples(sig[ci],n,bdt[smin*BCHNS]);
	    // clear rest of output channels (if buffer has less channels)
	    for(; ci < outchns; ++ci) ZeroSamples(sig[ci],n);
    }
    else if(OCHNS == 1) {
        t_sample *sig0 = sig[0];
	    for(int i = 0; i < n; ++i) {	
		    register long oint = CASTINT<long>(*(pos++));

            // for xplay oint can be out of bounds -> check
		    if(LIKELY(oint >= smin))
			    if(LIKELY(oint < smax)) {
				    // normal
				    *(sig0++) = bdt[oint*BCHNS];
			    }
			    else {
				    // position > last sample ... take only last sample
				    *(sig0++) = bdt[(smax-1)*BCHNS];
			    }
		    else {
			    // position < 0 ... take only 0th sample
			    *(sig0++) = bdt[smin*BCHNS];
		    }
	    }
    }
    else {
	    for(int i = 0,si = 0; i < n; ++i,++si) {	
		    register long oint = CASTINT<long>(*(pos++));
		    register const Element *fp;

            // for xplay oint can be out of bounds -> check
		    if(LIKELY(oint >= smin))
			    if(LIKELY(oint < smax)) {
				    // normal
				    fp = bdt+oint*BCHNS;
			    }
			    else {
				    // position > last sample ... take only last sample
				    fp = bdt+(smax-1)*BCHNS;
			    }
		    else {
			    // position < 0 ... take only 0th sample
			    fp = bdt+smin*BCHNS;
		    }

            for(int ci = 0; ci < OCHNS; ++ci)
			    sig[ci][si] = fp[ci];
        }

	    // clear rest of output channels (if buffer has less channels)
	    for(int ci = OCHNS; ci < outchns; ++ci) ZeroSamples(sig[ci],n);
    }
}

TMPLDEF void xinter::st_play2(const Element *bdt,const int smin,const int smax,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs,bool looped)
{
	const int plen = smax-smin;
	if(UNLIKELY(plen < 2)) {
		st_play1 TMPLCALL (bdt,smin,smax,n,inchns,outchns,invecs,outvecs,looped);
		return;
	}

	SIGCHNS(BCHNS,inchns,OCHNS,outchns);

	// position info are frame units
	const t_sample *pos = invecs[0];
	t_sample *const *sig = outvecs;
	
	// linear interpolation
	// --------------------

	const int maxo = smax-1;  // last sample in buffer

    if(OCHNS == 1) {
        t_sample *sig0 = sig[0];
	    for(int i = 0; i < n; ++i) {	
		    const float o = *(pos++);
		    register long oint = CASTINT<long>(o);
			const float frac = o-oint;
			t_sample fp0,fp1;

		    if(LIKELY(oint >= smin))
			    if(LIKELY(oint < maxo)) {
				    // normal interpolation
			        fp0 = bdt[oint*BCHNS];
			        fp1 = bdt[(oint+1)*BCHNS];
			    }
			    else {
				    // position is past last sample
                    if(looped) {
        				oint = smin+(oint-smin)%plen;
                        fp0 = bdt[oint*BCHNS];
                        fp1 = oint >= maxo?t_sample(bdt[smin]):fp0;
                    }
                    else
    	                fp0 = fp1 = bdt[maxo*BCHNS]; 
                }
		    else {
			    // position is before first sample
                if(looped) {
        			oint = smax-(smin-oint)%plen;
                    fp0 = bdt[oint*BCHNS]; 
                    fp1 = oint >= maxo?t_sample(bdt[smin]):fp0;
                }
                else
		            fp0 = fp1 = bdt[smin*BCHNS]; 
		    }

            *(sig0++) = fp0+frac*(fp1-fp0);
	    }
    }
    else {
	    for(int i = 0,si = 0; i < n; ++i,++si) {	
		    const float o = *(pos++);
		    register long oint = CASTINT<long>(o);
			const Element *fp0,*fp1;
			const float frac = o-oint;

		    if(LIKELY(oint >= smin))
			    if(LIKELY(oint < maxo)) {
				    // normal interpolation
			        fp0 = bdt+oint*BCHNS;
			        fp1 = fp0+BCHNS;
			    }
			    else {
				    // position is past last sample
                    if(looped) {
        				oint = smin+(oint-smin)%plen;
                        fp0 = bdt+oint*BCHNS;
                        fp1 = oint >= maxo?bdt+smin:fp0;
                    }
                    else
		                fp0 = fp1 = bdt+maxo*BCHNS;
			    }
		    else {
			    // position is before first sample
                if(looped) {
        			oint = smax-(smin-oint)%plen;
                    fp0 = bdt+oint*BCHNS;
                    fp1 = oint >= maxo?bdt+smin:fp0;
                }
                else
		            fp0 = fp1 = bdt+smin*BCHNS;
		    }

			for(int ci = 0; ci < OCHNS; ++ci) 
				sig[ci][si] = fp0[ci]+frac*(fp1[ci]-fp0[ci]);
	    }

    	// clear rest of output channels (if buffer has less channels)
	    for(int ci = OCHNS; ci < outchns; ++ci) ZeroSamples(sig[ci],n);
    }
}

TMPLDEF void xinter::st_play4(const Element *bdt,const int smin,const int smax,const int n,const int inchns,const int outchns,t_sample *const *invecs,t_sample *const *outvecs,bool looped)
{
	const int plen = smax-smin; //curlen;
	if(UNLIKELY(plen < 4)) {
		if(plen < 2) st_play1 TMPLCALL (bdt,smin,smax,n,inchns,outchns,invecs,outvecs,looped);
		else st_play2 TMPLCALL (bdt,smin,smax,n,inchns,outchns,invecs,outvecs,looped);
		return;
	}

	SIGCHNS(BCHNS,inchns,OCHNS,outchns);

	// position info are frame units
	const t_sample *pos = invecs[0];
	t_sample *const *sig = outvecs;
	
	// 4-point interpolation
	// ---------------------
	const int maxo = smax-1; // last sample in play region

    if(OCHNS == 1) {
        t_sample *sig0 = sig[0];
	    for(int i = 0; i < n; ++i) {	
		    float o = pos[i];
		    register long oint = CASTINT<long>(o);
		    register t_sample fa,fb,fc,fd;
		    const float frac = o-oint;
            register const Element *ptr = bdt+oint*BCHNS;

            if(LIKELY(oint > smin)) {
			    if(LIKELY(oint < maxo-2)) {
				    // normal case
				    fa = ptr[-BCHNS];
				    fb = ptr[0];
				    fc = ptr[BCHNS];
				    fd = ptr[BCHNS*2];
                }
			    else {
				    // not enough space at the end

                    if(looped) {
                        // normalize position
                        oint = smin+(oint-smin)%plen;
                        goto looped1;
                    }
                    else {
                        // last sample is outside in any case
                        fd = bdt[maxo*BCHNS];

                        if(oint-1 >= maxo)
                            // if first is outside, all are outside
                            fa = fb = fc = fd;
                        else {
                            fa = ptr[-BCHNS];
                            if(oint >= maxo) 
                                fb = fc = fd;
                            else {
                                fb = ptr[0];
                                fc = oint+1 < maxo?ptr[BCHNS]:fd;
                            }
                        }
                    }
                }
            }
		    else {
			    // not enough space at the beginning

                if(looped) {
                    // normalize position
                    oint = smax-(smin-oint)%plen;
looped1:
                    ptr = bdt+oint*BCHNS;

                    // inside in any case
                    fb = ptr[0];

                    if(oint < maxo-1) {
                        fa = oint > smin?ptr[-BCHNS]:bdt[maxo*BCHNS];
                        fc = ptr[BCHNS];
                        fd = ptr[BCHNS*2];
                    }
                    else {
                        fa = ptr[-BCHNS];
                        fc = oint < maxo?ptr[BCHNS]:ptr[(1-plen)*BCHNS];
                        fd = ptr[(2-plen)*BCHNS];
                    }
                }
                else {
                    // first sample is outside in any case
                    fa = bdt[smin*BCHNS];

                    if(oint+2 < smin)
                        // if last is outside, all are outside
                        fb = fc = fd = fa;
                    else {
                        fd = ptr[BCHNS*2];
                        if(oint+1 < smin) 
                            fb = fc = fa;
                        else {
                            fc = ptr[BCHNS];
                            fb = oint < smin?fa:ptr[0];
                        }
                    }
                }
		    }
    		
		    const float f1 = frac*0.5f-0.5f;
		    const float f3 = frac*3.0f-1.0f;
    		
			const float amdf = (fa-fd)*frac;
			const float cmb = fc-fb;
			const float bma = fb-fa;
			sig0[i] = fb + frac*( cmb - f1 * ( amdf+bma+cmb*f3 ) );
	    }
    }
    else {
	    for(int i = 0,si = 0; i < n; ++i,++si) {	
		    float o = *(pos++);
		    register long oint = CASTINT<long>(o);
		    const float frac = o-oint;
            register const Element *ptr = bdt+oint*BCHNS;
		    register const Element *fa,*fb,*fc,*fd;

		    if(LIKELY(oint > smin))
			    if(LIKELY(oint < maxo-2)) {
				    // normal case
    				fb = ptr;
				    fa = fb-BCHNS;
				    fc = fb+BCHNS;
				    fd = fc+BCHNS;
			    }
			    else {
				    // not enough space at the end

                    if(looped) {
                        // normalize position
                        oint = smin+(oint-smin)%plen;
                        goto looped2;
                    }
                    else {
                        // last sample is outside in any case
                        fd = bdt+maxo*BCHNS;

                        if(oint-1 >= maxo)
                            // if first is outside, all are outside
                            fa = fb = fc = fd;
                        else {
                            fa = ptr-BCHNS;
                            if(oint >= maxo) 
                                fb = fc = fd;
                            else {
                                fb = ptr;
                                fc = oint+1 < maxo?ptr+BCHNS:fd;
                            }
                        }
                    }
			    }
		    else {
			    // not enough space at the beginning

                if(looped) {
                    // normalize position
                    oint = smax-(smin-oint)%plen;
looped2:
                    // inside in any case
                    fb = bdt+oint*BCHNS;

                    if(oint < maxo-1) {
                        fa = oint > smin?fb-BCHNS:bdt+maxo*BCHNS;
                        fc = fb+BCHNS;
                        fd = fc+BCHNS;
                    }
                    else {
                        fa = fb-BCHNS;
                        fc = oint < maxo?fb+BCHNS:bdt+(oint-plen+1)*BCHNS;
                        fd = bdt+(oint-plen+2)*BCHNS;
                    }
                }
                else {
                    // first sample is outside in any case
                    fa = bdt+smin*BCHNS;

                    if(oint+2 < smin)
                        // if last is outside, all are outside
                        fb = fc = fd = fa;
                    else {
                        fd = ptr+BCHNS*2;
                        if(oint+1 < smin) 
                            fb = fc = fa;
                        else {
                            fc = ptr+BCHNS;
                            fb = oint < smin?fa:ptr;
                        }
                    }
                }
		    }
    		
		    const float f1 = 0.5f*(frac-1.0f);
		    const float f3 = frac*3.0f-1.0f;
    		
		    for(int ci = 0; ci < OCHNS; ++ci) {
			    const float amdf = (fa[ci]-fd[ci])*frac;
			    const float cmb = fc[ci]-fb[ci];
			    const float bma = fb[ci]-fa[ci];
			    sig[ci][si] = fb[ci] + frac*( cmb - f1 * ( amdf+bma+cmb*f3 ) );
		    }
	    }

	    // clear rest of output channels (if buffer has less channels)
	    for(int ci = OCHNS; ci < outchns; ++ci) ZeroSamples(sig[ci],n);
    }
}


TMPLDEF inline void xinter::s_play0(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	st_play0 TMPLCALL (buf.Data(),curmin,curmax,n,buf.Channels(),outchns,invecs,outvecs,loopmode == xsl_loop);
}

TMPLDEF inline void xinter::s_play1(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	st_play1 TMPLCALL (buf.Data(),curmin,curmax,n,buf.Channels(),outchns,invecs,outvecs,loopmode == xsl_loop);
}

TMPLDEF inline void xinter::s_play2(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	st_play2 TMPLCALL (buf.Data(),curmin,curmax,n,buf.Channels(),outchns,invecs,outvecs,loopmode == xsl_loop);
}

TMPLDEF inline void xinter::s_play4(int n,t_sample *const *invecs,t_sample *const *outvecs)
{
	st_play4 TMPLCALL (buf.Data(),curmin,curmax,n,buf.Channels(),outchns,invecs,outvecs,loopmode == xsl_loop);
}

#endif
