
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <sstream>

#ifdef _MSC_VER
#   pragma warning(disable: 4244)

#   ifndef _USE_MATH_DEFINES
#   define _USE_MATH_DEFINES
#   endif
#endif

/************************************************************************
  fft(int n, double xRe[], double xIm[], double yRe[], double yIm[])
 ------------------------------------------------------------------------
  NOTE : This is copyrighted material, Not public domain. See below.
 ------------------------------------------------------------------------
  Input/output:
      int n          transformation length.
      double xRe[]   real part of input sequence.
      double xIm[]   imaginary part of input sequence.
      double yRe[]   real part of output sequence.
      double yIm[]   imaginary part of output sequence.
 ------------------------------------------------------------------------
  Function:
      The procedure performs a fast discrete Fourier transform (FFT) of
      a complex sequence, x, of an arbitrary length, n. The output, y,
      is also a complex sequence of length n.

      y[k] = sum(x[m]*exp(-i*2*pi*k*m/n), m=0..(n-1)), k=0,...,(n-1)

      The largest prime factor of n must be less than or equal to the
      constant maxPrimeFactor defined below.
 ------------------------------------------------------------------------
  Author:
      Jens Joergen Nielsen            For non-commercial use only.
      Bakkehusene 54                  A $100 fee must be paid if used
      DK-2970 Hoersholm               commercially. Please contact.
      DENMARK

      E-mail : jjn@get2net.dk   All rights reserved. October 2000.
      Homepage : http://home.get2net.dk/jjn
 ------------------------------------------------------------------------
  Implementation notes:
      The general idea is to factor the length of the DFT, n, into
      factors that are efficiently handled by the routines.

      A number of short DFT's are implemented with a minimum of
      arithmetical operations and using (almost) straight line code
      resulting in very fast execution when the factors of n belong
      to this set. Especially radix-10 is optimized.

      Prime factors, that are not in the set of short DFT's are handled
      with direct evaluation of the DFP expression.

      Please report any problems to the author. 
      Suggestions and improvements are welcomed.
 ------------------------------------------------------------------------
  Benchmarks:                   
      The Microsoft Visual C++ compiler was used with the following 
      compile options:
      /nologo /Gs /G2 /W4 /AH /Ox /D "NDEBUG" /D "_DOS" /FR
      and the FFTBENCH test executed on a 50MHz 486DX :
      
      Length  Time [s]  Accuracy [dB]

         128   0.0054     -314.8   
         256   0.0116     -309.8   
         512   0.0251     -290.8   
        1024   0.0567     -313.6   
        2048   0.1203     -306.4   
        4096   0.2600     -291.8   
        8192   0.5800     -305.1   
         100   0.0040     -278.5   
         200   0.0099     -280.3   
         500   0.0256     -278.5   
        1000   0.0540     -278.5   
        2000   0.1294     -280.6   
        5000   0.3300     -278.4   
       10000   0.7133     -278.5   
 ------------------------------------------------------------------------
  The following procedures are used :
      factorize       :  factor the transformation length.
      transTableSetup :  setup table with sofar-, actual-, and remainRadix.
      permute         :  permutation allows in-place calculations.
      twiddleTransf   :  twiddle multiplications and DFT's for one stage.
      initTrig        :  initialise sine/cosine table.
      fft_4           :  length 4 DFT, a la Nussbaumer.
      fft_5           :  length 5 DFT, a la Nussbaumer.
      fft_10          :  length 10 DFT using prime factor FFT.
      fft_odd         :  length n DFT, n odd.
*************************************************************************/

/************************************************************************

	changes by Thomas Grill:

	- introduced REAL type for numbers
	- made functions static
	- threw fft_n functions out of twiddleTransf
	  if feasible, these will be inlined by the compiler
	- changed log prints (to post)

************************************************************************/

#define  maxPrimeFactor        8000  // all static data should fit into 256kB of cache
#define  maxPrimeFactorDiv2    (maxPrimeFactor+1)/2
#define  maxFactorCount        100

static const double  c3_1 = -1.5000000000000E+00;  /*  c3_1 = cos(2*pi/3)-1;          */
static const double  c3_2 =  8.6602540378444E-01;  /*  c3_2 = sin(2*pi/3);            */

static const double  u5   =  1.2566370614359E+00;  /*  u5   = 2*pi/5;                 */
static const double  c5_1 = -1.2500000000000E+00;  /*  c5_1 = (cos(u5)+cos(2*u5))/2-1;*/
static const double  c5_2 =  5.5901699437495E-01;  /*  c5_2 = (cos(u5)-cos(2*u5))/2;  */
static const double  c5_3 = -9.5105651629515E-01;  /*  c5_3 = -sin(u5);               */
static const double  c5_4 = -1.5388417685876E+00;  /*  c5_4 = -(sin(u5)+sin(2*u5));   */
static const double  c5_5 =  3.6327126400268E-01;  /*  c5_5 = (sin(u5)-sin(2*u5));    */
static const double  c8   =  7.0710678118655E-01;  /*  c8 = 1/sqrt(2);    */

static const double   pi = M_PI;

template<typename REAL>
class MixFFT
{
public:
    class PrimeFactorTooLargeException: public std::runtime_error 
    {
    public:
        PrimeFactorTooLargeException(int factor):
            std::runtime_error(errtext(factor))
        {}
    protected:
        std::string errtext(int factor) 
        {
            std::stringstream ss;
            ss << "FFT: Prime factor of FFT length is too large (" << factor << ") - aborted";
            return ss.str();
        }
    };
protected:
    int      groupOffset,dataOffset,blockOffset,adr;
    int      groupNo,dataNo,blockNo,twNo;
    double   omega;
    REAL tw_re,tw_im;
    REAL   twiddleRe[maxPrimeFactor], twiddleIm[maxPrimeFactor];
    REAL trigRe[maxPrimeFactor], trigIm[maxPrimeFactor];
    REAL zRe[maxPrimeFactor], zIm[maxPrimeFactor];
    REAL   vRe[maxPrimeFactorDiv2], vIm[maxPrimeFactorDiv2];
    REAL   wRe[maxPrimeFactorDiv2], wIm[maxPrimeFactorDiv2];

    void factorize(int n, int *nFact, int fact[])
    {
        int i,j,k;
        int nRadix;
        int radices[7];
        int factors[maxFactorCount];

        nRadix    =  6;  
        radices[1]=  2;
        radices[2]=  3;
        radices[3]=  4;
        radices[4]=  5;
        radices[5]=  8;
        radices[6]= 10;

        if (n==1)
        {
            j=1;
            factors[1]=1;
        }
        else j=0;
        i=nRadix;
        while ((n>1) && (i>0))
        {
          if ((n % radices[i]) == 0)
          {
            n=n / radices[i];
            j=j+1;
            factors[j]=radices[i];
          }
          else  i=i-1;
        }
        if (factors[j] == 2)   /*substitute factors 2*8 with 4*4 */
        {   
          i = j-1;
          while ((i>0) && (factors[i] != 8)) i--;
          if (i>0)
          {
            factors[j] = 4;
            factors[i] = 4;
          }
        }
        if (n>1)
        {
            for (k=2; k<sqrt((double)n)+1; k++)
                while ((n % k) == 0)
                {
                    n=n / k;
                    j=j+1;
                    factors[j]=k;
                }
            if (n>1)
            {
                j=j+1;
                factors[j]=n;
            }
        }               
        for (i=1; i<=j; i++)         
        {
          fact[i] = factors[j-i+1];
        }
        *nFact=j;
    }   /* factorize */

    /****************************************************************************
      After N is factored the parameters that control the stages are generated.
      For each stage we have:
        sofar   : the product of the radices so far.
        actual  : the radix handled in this stage.
        remain  : the product of the remaining radices.
     ****************************************************************************/

    bool transTableSetup(int sofar[], int actual[], int remain[],
                         int *nFact,
                         int *nPoints)
    {
        int i;

        factorize(*nPoints, nFact, actual);
        if (actual[1] > maxPrimeFactor)
            throw PrimeFactorTooLargeException(actual[1]);

        remain[0]=*nPoints;
        sofar[1]=1;
        remain[1]=*nPoints / actual[1];
        for (i=2; i<=*nFact; i++)
        {
            sofar[i]=sofar[i-1]*actual[i-1];
            remain[i]=remain[i-1] / actual[i];
        }
    	return true;
    }   /* transTableSetup */

    /****************************************************************************
      The sequence y is the permuted input sequence x so that the following
      transformations can be performed in-place, and the final result is the
      normal order.
     ****************************************************************************/

    template<typename T1,typename T2,typename T3,typename T4>
    void permute(int nPoint, int nFact,
                 int fact[], int remain[],
                 T1 xRe[], T2 xIm[],
                 T3 yRe[], T4 yIm[])

    {
        int i,j,k;
        int count[maxFactorCount]; 

        for (i=1; i<=nFact; i++) count[i]=0;
        k=0;
        for (i=0; i<=nPoint-2; i++)
        {
            yRe[i] = xRe[k];
            yIm[i] = xIm[k];
            j=1;
            k=k+remain[j];
            count[1] = count[1]+1;
            while (count[j] >= fact[j])
            {
                count[j]=0;
                k=k-remain[j-1]+remain[j+1];
                j=j+1;
                count[j]=count[j]+1;
            }
        }
        yRe[nPoint-1]=xRe[nPoint-1];
        yIm[nPoint-1]=xIm[nPoint-1];
    }   /* permute */


    /****************************************************************************
      Twiddle factor multiplications and transformations are performed on a
      group of data. The number of multiplications with 1 are reduced by skipping
      the twiddle multiplication of the first stage and of the first group of the
      following stages.
     ***************************************************************************/

    void initTrig(int radix)
    {
        int i;
        double w,xre,xim,xre1,xim1;

        w=2*pi/radix;
        trigRe[0]=1; trigIm[0]=0;
        xre1=xre=cos(w); 
        xim1=xim=-sin(w);
        trigRe[1]=xre; trigIm[1]=xim;
        for (i=2; i<radix; i++)
        {
            trigRe[i] = xre1 = xre*trigRe[i-1] - xim*trigIm[i-1];
            trigIm[i] = xim1 = xim*trigRe[i-1] + xre*trigIm[i-1];
    //        trigRe[i] = xre1 = xre*xre1 - xim*xim1;
    //        trigIm[i] = xim1 = xim*xre1 + xre*xim1;
        }
    }   /* initTrig */

    void fft_2(REAL aRe[], REAL aIm[])
    {
    	double gem;
    	gem=zRe[0] + zRe[1];
    	zRe[1]=zRe[0] -  zRe[1]; zRe[0]=gem;
    	gem=zIm[0] + zIm[1];
    	zIm[1]=zIm[0] - zIm[1]; zIm[0]=gem;
    }

    void fft_3(REAL aRe[], REAL aIm[])
    {
    	REAL t1_re,t1_im;
        REAL  m2_re,m2_im; 
        REAL  m1_re,m1_im; 
        REAL  s1_re,s1_im; 
    	t1_re=zRe[1] + zRe[2]; t1_im=zIm[1] + zIm[2];
    	zRe[0]=zRe[0] + t1_re; zIm[0]=zIm[0] + t1_im;
    	m1_re=c3_1*t1_re; m1_im=c3_1*t1_im;
    	m2_re=c3_2*(zIm[1] - zIm[2]); 
    	m2_im=c3_2*(zRe[2] -  zRe[1]);
    	s1_re=zRe[0] + m1_re; s1_im=zIm[0] + m1_im;
    	zRe[1]=s1_re + m2_re; zIm[1]=s1_im + m2_im;
    	zRe[2]=s1_re - m2_re; zIm[2]=s1_im - m2_im;
    }

    void fft_4(REAL aRe[], REAL aIm[])
    {
        REAL t1_re,t1_im, t2_re,t2_im;
        REAL m2_re,m2_im, m3_re,m3_im;

        t1_re=aRe[0] + aRe[2]; t1_im=aIm[0] + aIm[2];
        t2_re=aRe[1] + aRe[3]; t2_im=aIm[1] + aIm[3];

        m2_re=aRe[0] - aRe[2]; m2_im=aIm[0] - aIm[2];
        m3_re=aIm[1] - aIm[3]; m3_im=aRe[3] - aRe[1];

        aRe[0]=t1_re + t2_re; aIm[0]=t1_im + t2_im;
        aRe[2]=t1_re - t2_re; aIm[2]=t1_im - t2_im;
        aRe[1]=m2_re + m3_re; aIm[1]=m2_im + m3_im;
        aRe[3]=m2_re - m3_re; aIm[3]=m2_im - m3_im;
    }   /* fft_4 */


    void fft_5(REAL aRe[], REAL aIm[])
    {    
        REAL t1_re,t1_im, t2_re,t2_im, t3_re,t3_im;
        REAL t4_re,t4_im, t5_re,t5_im;
        REAL m2_re,m2_im, m3_re,m3_im, m4_re,m4_im;
        REAL m1_re,m1_im, m5_re,m5_im;
        REAL s1_re,s1_im, s2_re,s2_im, s3_re,s3_im;
        REAL s4_re,s4_im, s5_re,s5_im;

        t1_re=aRe[1] + aRe[4]; t1_im=aIm[1] + aIm[4];
        t2_re=aRe[2] + aRe[3]; t2_im=aIm[2] + aIm[3];
        t3_re=aRe[1] - aRe[4]; t3_im=aIm[1] - aIm[4];
        t4_re=aRe[3] - aRe[2]; t4_im=aIm[3] - aIm[2];
        t5_re=t1_re + t2_re; t5_im=t1_im + t2_im;
        aRe[0]=aRe[0] + t5_re; aIm[0]=aIm[0] + t5_im;
        m1_re=c5_1*t5_re; m1_im=c5_1*t5_im;
        m2_re=c5_2*(t1_re - t2_re); m2_im=c5_2*(t1_im - t2_im);

        m3_re=-c5_3*(t3_im + t4_im); m3_im=c5_3*(t3_re + t4_re);
        m4_re=-c5_4*t4_im; m4_im=c5_4*t4_re;
        m5_re=-c5_5*t3_im; m5_im=c5_5*t3_re;

        s3_re=m3_re - m4_re; s3_im=m3_im - m4_im;
        s5_re=m3_re + m5_re; s5_im=m3_im + m5_im;
        s1_re=aRe[0] + m1_re; s1_im=aIm[0] + m1_im;
        s2_re=s1_re + m2_re; s2_im=s1_im + m2_im;
        s4_re=s1_re - m2_re; s4_im=s1_im - m2_im;

        aRe[1]=s2_re + s3_re; aIm[1]=s2_im + s3_im;
        aRe[2]=s4_re + s5_re; aIm[2]=s4_im + s5_im;
        aRe[3]=s4_re - s5_re; aIm[3]=s4_im - s5_im;
        aRe[4]=s2_re - s3_re; aIm[4]=s2_im - s3_im;
    }   /* fft_5 */

    void fft_8()
    {
        REAL aRe[4], aIm[4], bRe[4], bIm[4], gem;

        aRe[0] = zRe[0];    bRe[0] = zRe[1];
        aRe[1] = zRe[2];    bRe[1] = zRe[3];
        aRe[2] = zRe[4];    bRe[2] = zRe[5];
        aRe[3] = zRe[6];    bRe[3] = zRe[7];

        aIm[0] = zIm[0];    bIm[0] = zIm[1];
        aIm[1] = zIm[2];    bIm[1] = zIm[3];
        aIm[2] = zIm[4];    bIm[2] = zIm[5];
        aIm[3] = zIm[6];    bIm[3] = zIm[7];

        fft_4(aRe, aIm); fft_4(bRe, bIm);

        gem    = c8*(bRe[1] + bIm[1]);
        bIm[1] = c8*(bIm[1] - bRe[1]);
        bRe[1] = gem;
        gem    = bIm[2];
        bIm[2] =-bRe[2];
        bRe[2] = gem;
        gem    = c8*(bIm[3] - bRe[3]);
        bIm[3] =-c8*(bRe[3] + bIm[3]);
        bRe[3] = gem;
    
        zRe[0] = aRe[0] + bRe[0]; zRe[4] = aRe[0] - bRe[0];
        zRe[1] = aRe[1] + bRe[1]; zRe[5] = aRe[1] - bRe[1];
        zRe[2] = aRe[2] + bRe[2]; zRe[6] = aRe[2] - bRe[2];
        zRe[3] = aRe[3] + bRe[3]; zRe[7] = aRe[3] - bRe[3];

        zIm[0] = aIm[0] + bIm[0]; zIm[4] = aIm[0] - bIm[0];
        zIm[1] = aIm[1] + bIm[1]; zIm[5] = aIm[1] - bIm[1];
        zIm[2] = aIm[2] + bIm[2]; zIm[6] = aIm[2] - bIm[2];
        zIm[3] = aIm[3] + bIm[3]; zIm[7] = aIm[3] - bIm[3];
    }   /* fft_8 */

    void fft_10()
    {
        REAL aRe[5], aIm[5], bRe[5], bIm[5];

        aRe[0] = zRe[0];    bRe[0] = zRe[5];
        aRe[1] = zRe[2];    bRe[1] = zRe[7];
        aRe[2] = zRe[4];    bRe[2] = zRe[9];
        aRe[3] = zRe[6];    bRe[3] = zRe[1];
        aRe[4] = zRe[8];    bRe[4] = zRe[3];

        aIm[0] = zIm[0];    bIm[0] = zIm[5];
        aIm[1] = zIm[2];    bIm[1] = zIm[7];
        aIm[2] = zIm[4];    bIm[2] = zIm[9];
        aIm[3] = zIm[6];    bIm[3] = zIm[1];
        aIm[4] = zIm[8];    bIm[4] = zIm[3];

        fft_5(aRe, aIm); fft_5(bRe, bIm);

        zRe[0] = aRe[0] + bRe[0]; zRe[5] = aRe[0] - bRe[0];
        zRe[6] = aRe[1] + bRe[1]; zRe[1] = aRe[1] - bRe[1];
        zRe[2] = aRe[2] + bRe[2]; zRe[7] = aRe[2] - bRe[2];
        zRe[8] = aRe[3] + bRe[3]; zRe[3] = aRe[3] - bRe[3];
        zRe[4] = aRe[4] + bRe[4]; zRe[9] = aRe[4] - bRe[4];

        zIm[0] = aIm[0] + bIm[0]; zIm[5] = aIm[0] - bIm[0];
        zIm[6] = aIm[1] + bIm[1]; zIm[1] = aIm[1] - bIm[1];
        zIm[2] = aIm[2] + bIm[2]; zIm[7] = aIm[2] - bIm[2];
        zIm[8] = aIm[3] + bIm[3]; zIm[3] = aIm[3] - bIm[3];
        zIm[4] = aIm[4] + bIm[4]; zIm[9] = aIm[4] - bIm[4];
    }   /* fft_10 */

    void fft_odd(int radix)
    {
        REAL  rere, reim, imre, imim;
        int     i,j,k,n,max;

        n = radix;
        max = (n + 1)/2;
        for (j=1; j < max; j++)
        {
          vRe[j] = zRe[j] + zRe[n-j];
          vIm[j] = zIm[j] - zIm[n-j];
          wRe[j] = zRe[j] - zRe[n-j];
          wIm[j] = zIm[j] + zIm[n-j];
        }

        for (j=1; j < max; j++)
        {
            zRe[j]=zRe[0]; 
            zIm[j]=zIm[0];
            zRe[n-j]=zRe[0]; 
            zIm[n-j]=zIm[0];
            k=j;
            for (i=1; i < max; i++)
            {
                rere = trigRe[k] * vRe[i];
                imim = trigIm[k] * vIm[i];
                reim = trigRe[k] * wIm[i];
                imre = trigIm[k] * wRe[i];
            
                zRe[n-j] += rere + imim;
                zIm[n-j] += reim - imre;
                zRe[j]   += rere - imim;
                zIm[j]   += reim + imre;

                k = k + j;
                if (k >= n)  k = k - n;
            }
        }
        for (j=1; j < max; j++)
        {
            zRe[0]=zRe[0] + vRe[j]; 
            zIm[0]=zIm[0] + wIm[j];
        }
    }   /* fft_odd */


    template<typename T3,typename T4>
    void twiddleTransf(int sofarRadix, int radix, int remainRadix,
                        T3 yRe[], T4 yIm[])

    {   /* twiddleTransf */ 
        double cosw, sinw, gem;

        initTrig(radix);
        omega = 2*pi/(double)(sofarRadix*radix);
        cosw =  cos(omega);
        sinw = -sin(omega);
        tw_re = 1.0;
        tw_im = 0;
        dataOffset=0;
        groupOffset=dataOffset;
        adr=groupOffset;
        for (dataNo=0; dataNo<sofarRadix; dataNo++)
        {
            if (sofarRadix>1)
            {
                twiddleRe[0] = 1.0; 
                twiddleIm[0] = 0.0;
                twiddleRe[1] = tw_re;
                twiddleIm[1] = tw_im;
                for (twNo=2; twNo<radix; twNo++)
                {
                    twiddleRe[twNo]=tw_re*twiddleRe[twNo-1]
                                   - tw_im*twiddleIm[twNo-1];
                    twiddleIm[twNo]=tw_im*twiddleRe[twNo-1]
                                   + tw_re*twiddleIm[twNo-1];
                }
                gem   = cosw*tw_re - sinw*tw_im;
                tw_im = sinw*tw_re + cosw*tw_im;
                tw_re = gem;                      
            }
            for (groupNo=0; groupNo<remainRadix; groupNo++)
            {
                if ((sofarRadix>1) && (dataNo > 0))
                {
                    zRe[0]=yRe[adr];
                    zIm[0]=yIm[adr];
                    blockNo=1;
                    do {
                        adr = adr + sofarRadix;
                        zRe[blockNo]=  twiddleRe[blockNo] * yRe[adr]
                                     - twiddleIm[blockNo] * yIm[adr];
                        zIm[blockNo]=  twiddleRe[blockNo] * yIm[adr]
                                     + twiddleIm[blockNo] * yRe[adr]; 
                    
                        blockNo++;
                    } while (blockNo < radix);
                }
                else
                    for (blockNo=0; blockNo<radix; blockNo++)
                    {
                       zRe[blockNo]=yRe[adr];
                       zIm[blockNo]=yIm[adr];
                       adr=adr+sofarRadix;
                    }
                switch(radix) {
    			// T.Grill - replaced the inlined code by their function counterparts
    			  case  2  : fft_2(zRe,zIm); break;
    			  case  3  : fft_3(zRe,zIm); break;
                  case  4  : fft_4(zRe,zIm); break;
                  case  5  : fft_5(zRe,zIm); break;
                  case  8  : fft_8(); break;
                  case 10  : fft_10(); break;
                  default  : fft_odd(radix); break;
                }
                adr=groupOffset;
                for (blockNo=0; blockNo<radix; blockNo++)
                {
                    yRe[adr]=zRe[blockNo]; yIm[adr]=zIm[blockNo];
                    adr=adr+sofarRadix;
                }
                groupOffset=groupOffset+sofarRadix*radix;
                adr=groupOffset;
            }
            dataOffset=dataOffset+1;
            groupOffset=dataOffset;
            adr=groupOffset;
        }
    }   /* twiddleTransf */

public:
    int   sofarRadix[maxFactorCount], 
          actualRadix[maxFactorCount], 
          remainRadix[maxFactorCount];
    int   n,nFactor;

    MixFFT(int _n):
        n(_n)
    {
        transTableSetup(sofarRadix, actualRadix, remainRadix, &nFactor, &n);
    }   /* fft */

    template<typename T1,typename T2,typename T3,typename T4>
    void operator ()(T1 *xRe, T2 *xIm,T3 *yRe,T4 *yIm)
    {
        permute(n, nFactor, actualRadix, remainRadix, xRe, xIm, yRe, yIm);

        for(int count=1; count<=nFactor; count++)
            twiddleTransf(sofarRadix[count], actualRadix[count], remainRadix[count], yRe, yIm);
    }

}; // class MixFFT
