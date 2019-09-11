/* THIS IS A TOTALLY HACKED HEADER - NO LONGER ANY GOOD FOR CMIX */

#define MAXSECTS 20
#define RESON_NO_SCL (0.)
#define START 3
#define STARTM1 2 /* for start of comb memory in a array */
#define NCOMBS  6 /* for reverb */
#define NALPASSES 2 /* for reverb */

typedef struct {
  float ps0;
  float ps1;
  float ps2;
  float ps3;
  float c0;
  float c1;
  float c2;
  float c3;
} LSTRUCT ;

typedef struct {
  int len;
  float *func;
  float amp;
  float phs;
  float si;
} CMIXOSC ;

typedef struct {
  float *arr;
  float lpt;
  float rvbt;
  int len;
  int status;
} CMIXCOMB ;

typedef struct {
  float cf;
  float bw;
  float scl;
  float q[5];
} CMIXRESON ;

typedef struct {
  float a;
  float d;
  float s;
  float r;
  float v1;
  float v2;
  float v3;
  float v4;
  float v5;
  float *func;
  int len;
} CMIXADSR ;

