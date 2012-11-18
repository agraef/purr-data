#include <cmath>

#ifdef _MSC_VER
#   pragma warning(disable: 4244)

#   ifndef _USE_MATH_DEFINES
#   define _USE_MATH_DEFINES
#   endif
#endif

/////////////////////////////////////////////////////////
// Sorensen in-place split-radix FFT for real values
// data: array of floats:
// re(0),re(1),re(2),...,re(size-1)
// 
// output:
// re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
// normalized by array length
//
// Source: 
// Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
// IEEE Trans. ASSP, ASSP-35, No. 6, June 1987

template<typename T>
void realfft_split(T *data,int n)
{

  int i,j,k,i5,i6,i7,i8,i0,id,i1,i2,i3,i4,n2,n4,n8;
  float t1,t2,t3,t4,t5,t6,a3,ss1,ss3,cc1,cc3,a,e;
  
  n4=n-1;
  
  //data shuffling
      for (i=0,j=0,n2=n/2; i<n4 ; i++){
	  if (i<j){
				t1=data[j];
				data[j]=data[i];
				data[i]=t1;
				}
	  k=n2;
	  while (k<=j){
				j-=k;
				k>>=1;	
				}
	  j+=k;
      }
	
/*----------------------*/
	
	//length two butterflies	
	i0=0;
	id=4;
   do{
       for (; i0<n4; i0+=id){ 
			i1=i0+1;
			t1=data[i0];
			data[i0]=t1+data[i1];
			data[i1]=t1-data[i1];
		}
	   id<<=1;
       i0=id-2;
       id<<=1;
    } while ( i0<n4 );

   /*----------------------*/
   //L shaped butterflies
n2=2;
for(k=n;k>2;k>>=1){  
	n2<<=1;
	n4=n2>>2;
	n8=n2>>3;
	e = 2*M_PI/(n2);
	i1=0;
	id=n2<<1;
	do{ 
	    for (; i1<n; i1+=id){
			i2=i1+n4;
			i3=i2+n4;
			i4=i3+n4;
			t1=data[i4]+data[i3];
			data[i4]-=data[i3];
			data[i3]=data[i1]-t1;
			data[i1]+=t1;
			if (n4!=1){
				i0=i1+n8;
				i2+=n8;
				i3+=n8;
				i4+=n8;
				t1=(data[i3]+data[i4])/M_SQRT2;
				t2=(data[i3]-data[i4])/M_SQRT2;
				data[i4]=data[i2]-t1;
				data[i3]=-data[i2]-t1;
				data[i2]=data[i0]-t2;
				data[i0]+=t2;
			}
	     }
		 id<<=1;
	     i1=id-n2;
	     id<<=1;
	  } while ( i1<n );
	a=e;
	for (j=2; j<=n8; j++){  
	      a3=3*a;
	      cc1=cos(a);
	      ss1=sin(a);
	      cc3=cos(a3);
	      ss3=sin(a3);
	      a=j*e;
	      i=0;
	      id=n2<<1;
	      do{
		   for (; i<n; i+=id){  
			  i1=i+j-1;
			  i2=i1+n4;
			  i3=i2+n4;
			  i4=i3+n4;
			  i5=i+n4-j+1;
			  i6=i5+n4;
			  i7=i6+n4;
			  i8=i7+n4;
			  t1=data[i3]*cc1+data[i7]*ss1;
			  t2=data[i7]*cc1-data[i3]*ss1;
			  t3=data[i4]*cc3+data[i8]*ss3;
			  t4=data[i8]*cc3-data[i4]*ss3;
			  t5=t1+t3;
			  t6=t2+t4;
			  t3=t1-t3;
			  t4=t2-t4;
			  t2=data[i6]+t6;
			  data[i3]=t6-data[i6];
			  data[i8]=t2;
			  t2=data[i2]-t3;
			  data[i7]=-data[i2]-t3;
			  data[i4]=t2;
			  t1=data[i1]+t5;
			  data[i6]=data[i1]-t5;
			  data[i1]=t1;
			  t1=data[i5]+t4;
			  data[i5]-=t4;
			  data[i2]=t1;
		     }
		   id<<=1;
		   i=id-n2;
		   id<<=1;
		 } while(i<n);
	   }
      }
}


/////////////////////////////////////////////////////////
// Sorensen in-place inverse split-radix FFT for real values
// data: array of doubles:
// re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
// 
// output:
// re(0),re(1),re(2),...,re(size-1)
// NOT normalized by array length
//
// Source: 
// Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
// IEEE Trans. ASSP, ASSP-35, No. 6, June 1987

template<typename T>
void irealfft_split(T *data,int n){

  int i,j,k,i5,i6,i7,i8,i0,id,i1,i2,i3,i4,n2,n4,n8,n1;
  float t1,t2,t3,t4,t5,a3,ss1,ss3,cc1,cc3,a,e;
  
n1=n-1;
n2=n<<1;
for(k=n;k>2;k>>=1){  
	id=n2;
	n2>>=1;
	n4=n2>>2;
	n8=n2>>3;
	e = 2*M_PI/(n2);
	i1=0;
	do{ 
	    for (; i1<n; i1+=id){
			i2=i1+n4;
			i3=i2+n4;
			i4=i3+n4;
			t1=data[i1]-data[i3];
			data[i1]+=data[i3];
			data[i2]*=2;
			data[i3]=t1-2*data[i4];
			data[i4]=t1+2*data[i4];
			if (n4!=1){
				i0=i1+n8;
				i2+=n8;
				i3+=n8;
				i4+=n8;
				t1=(data[i2]-data[i0])/M_SQRT2;
				t2=(data[i4]+data[i3])/M_SQRT2;
				data[i0]+=data[i2];
				data[i2]=data[i4]-data[i3];
				data[i3]=2*(-t2-t1);
				data[i4]=2*(-t2+t1);
			}
	     }
		 id<<=1;
	     i1=id-n2;
	     id<<=1;
	  } while ( i1<n1 );
	a=e;
	for (j=2; j<=n8; j++){  
	      a3=3*a;
	      cc1=cos(a);
	      ss1=sin(a);
	      cc3=cos(a3);
	      ss3=sin(a3);
	      a=j*e;
	      i=0;
	      id=n2<<1;
	      do{
		   for (; i<n; i+=id){  
			  i1=i+j-1;
			  i2=i1+n4;
			  i3=i2+n4;
			  i4=i3+n4;
			  i5=i+n4-j+1;
			  i6=i5+n4;
			  i7=i6+n4;
			  i8=i7+n4;
			  t1=data[i1]-data[i6];
			  data[i1]+=data[i6];
			  t2=data[i5]-data[i2];
			  data[i5]+=data[i2];
			  t3=data[i8]+data[i3];
			  data[i6]=data[i8]-data[i3];
			  t4=data[i4]+data[i7];
			  data[i2]=data[i4]-data[i7];
			  t5=t1-t4;
			  t1+=t4;
			  t4=t2-t3;
			  t2+=t3;
			  data[i3]=t5*cc1+t4*ss1;
			  data[i7]=-t4*cc1+t5*ss1;
			  data[i4]=t1*cc3-t2*ss3;
			  data[i8]=t2*cc3+t1*ss3;
			  }
		   id<<=1;
		   i=id-n2;
		   id<<=1;
		 } while(i<n1);
	   }
	}	

   /*----------------------*/
	i0=0;
	id=4;
   do{
       for (; i0<n1; i0+=id){ 
			i1=i0+1;
			t1=data[i0];
			data[i0]=t1+data[i1];
			data[i1]=t1-data[i1];
		}
	   id<<=1;
       i0=id-2;
       id<<=1;
    } while ( i0<n1 );

/*----------------------*/

//data shuffling
      for (i=0,j=0,n2=n/2; i<n1 ; i++){
	  if (i<j){
				t1=data[j];
				data[j]=data[i];
				data[i]=t1;
				}
	  k=n2;
	  while (k<=j){
				j-=k;
				k>>=1;	
				}
	  j+=k;
      }	
}


#if 0
/////////////////////////////////////////////////////////
// Sorensen in-place radix-2 FFT for real values
// data: array of floats:
// re(0),re(1),re(2),...,re(size-1)
// 
// output:
// re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
// normalized by array length
//
// Source: 
// Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
// IEEE Trans. ASSP, ASSP-35, No. 6, June 1987

void realfft_radix2(float *data,int n){

    float  xt,a,e, t1, t2, cc, ss;
    int  i, j, k, n1, n2, n3, n4, i1, i2, i3, i4;

	n4=n-1;
  //data shuffling
      for (i=0,j=0,n2=n/2; i<n4 ; i++){
	  if (i<j){
				xt=data[j];
				data[j]=data[i];
				data[i]=xt;
				}
	  k=n2;
	  while (k<=j){
				j-=k;
				k>>=1;	
				}
	  j+=k;
      }
	
/* -------------------- */
    for (i=0; i<n; i += 2)  
      {
	 xt = data[i];
	 data[i] = xt + data[i+1];
	 data[i+1] = xt - data[i+1];
      }
/* ------------------------ */
    n2 = 1;
    for (k=n;k>2;k>>=1){ 
		n4 = n2;
		n2 = n4 << 1;
		n1 = n2 << 1;
		e = 2*PI/(n1);
		for (i=0; i<n; i+=n1){  
			xt = data[i];
			data[i] = xt + data[i+n2];
			data[i+n2] = xt-data[i+n2];
			data[i+n4+n2] = -data[i+n4+n2];
			a = e;
			n3=n4-1;
			for (j = 1; j <=n3; j++){
				i1 = i+j;
				i2 = i - j + n2;
				i3 = i1 + n2;
				i4 = i - j + n1;
				cc = cos(a);
				ss = sin(a);
				a += e;
				t1 = data[i3] * cc + data[i4] * ss;
				t2 = data[i3] * ss - data[i4] * cc;
				data[i4] = data[i2] - t2;
				data[i3] = -data[i2] - t2;
				data[i2] = data[i1] - t1;
				data[i1] += t1;
		  }
	  }
  }
}
#endif
