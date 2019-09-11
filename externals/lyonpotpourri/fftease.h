#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


/*
The new improved fftease.h
*/

#define getbytes t_getbytes
#define freebytes t_freebytes
#define resizebytes t_resizebytes

#define FFTEASE_ANNOUNCEMENT "- a member of FFTease 2.52"

#ifndef PI
#define PI 3.141592653589793115997963468544185161590576171875
#endif

#ifndef TWOPI
#define TWOPI 6.28318530717958623199592693708837032318115234375
#endif

#define MAX_N (16384)
#define MAX_N2 (MAX_N/2)
#define MAX_Nw (MAX_N * 4)

void convert(float *S, float *C, int N2, float *lastphase, float fundamental, float factor );
void unconvert( float *C, float *S, int N2, float *lastphase, float fundamental,  float factor );
void rfft( float *x, int N, int forward );
void cfft( float *x, int NC, int forward );
void bitreverse( float *x, int N );
void fold( float *I, float *W, int Nw, float *O, int N, int n );
void init_rdft(int n, int *ip, float *w);
void rdft(int n, int isgn, float *a, int *ip, float *w);
void bitrv2(int n, int *ip, float *a);
void cftsub(int n, float *a, float *w);
void rftsub(int n, float *a, int nc, float *c);
void makewt(int nw, int *ip, float *w);
void makect(int nc, int *ip, float *c);
void leanconvert( float *S, float *C, int N2 );
void leanunconvert( float *C, float *S, int N2 );
void makewindows( float *H, float *A, float *S, int Nw, int N, int I );
void makehamming( float *H, float *A, float *S, int Nw, int N, int I,int odd );
void makehanning( float *H, float *A, float *S, int Nw, int N, int I,int odd );
void overlapadd( float *I, int N, float *W, float *O, int Nw, int n );
void bloscbank( float *S, float *O, int D, float iD, float *lf, float *la,
	float *bindex, float *tab, int len, float synt, int lo, int hi );

float randf( float min, float max );
int randi( int min, int max );
int power_of_two(int test);


void freebytes2(void *fatso, size_t nbytes);
void *getbytes2(size_t nbytes);
void *resizebytes2(void *old, size_t oldsize, size_t newsize);
void limit_fftsize(int *N, int *Nw, char *OBJECT_NAME);

/* THE FUNCTIONS */
