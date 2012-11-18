/*
 *  This file is part of the pv-lib
 *
 *  The pv-lib can be used by everyone as desired
 *
 *  (c) Eric Lyon and Christopher Penrose
 */

#include <stdio.h>

/* -------------------------------------
 modifications by Thomas Grill
*/

/* #include <fts/fts.h> */
#include <math.h>

#ifdef _MSC_VER
#pragma warning(disable: 4305)
#pragma warning(disable: 4244)
#pragma warning(disable: 4101)
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define PV_PI 3.141592653589793115997963468544185161590576171875
#define PV_2PI (2.*PV_PI)

/* ------------------------------------- */


#define FORWARD 1
#define INVERSE 0

typedef struct {
  float    min;
  float    max;
} Bound;

void init_rdft(int n, int *ip, float *w);
void pv_rdft(int n, int isgn, float *a, int *ip, float *w);
void fold( float *I, float *W, int Nw, float *O, int N, int n );
void overlapadd(float *I, int N, float *W, float *O, int Nw, int n );
void makehanning( float *H, float *A, float *S, int Nw, int N, int I, int osc, int odd );
void makewindows( float *H, float *A, float *S, int Nw, int N, int I, int osc );
void leanconvert( float *S, float *C, int N2 , int amp, int ph );
void leanunconvert( float *C, float *S, int N2 );
void pv_rfft( float *x, int N, int forward );
void pv_cfft( float *x, int NC, int forward );
void convert_new(float *S, float *C, int N2, float *lastphase,  float fundamental, float factor );
void convert(float *S, float *C, int N2, float *lastphase,  float fundamental, float factor );
void unconvert(float  *C, float *S, int N2, float *lastphase, float fundamental,  float factor );


/* ------------------------------------- */

#ifdef __cplusplus
}
#endif

/* ------------------------------------- */

