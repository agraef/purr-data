/*
 *  This file is part of the pv-lib
 *
 *  The pv-lib can be used by everyone as desired
 *
 *  (c) Eric Lyon and Christopher Penrose
 *
 */
  
#include <math.h>
#include "pv.h"

static void bitrv2(int n, int *ip, float *a);
static void cftsub(int n, float *a, float *w);
static void rftsub(int n, float *a, int nc, float *c);
static void makewt(int nw, int *ip, float *w);
static void makect(int nc, int *ip, float *c);

void init_rdft(int n, int *ip, float *w)
{

  int	nw,
	nc;

  nw = n >> 2;
  makewt(nw, ip, w);

  nc = n >> 2;
  makect(nc, ip, w + nw);

  return;
}


void pv_rdft(int n, int isgn, float *a, int *ip, float *w)
{

  int		j,
		nw,
		nc;

  float		xi;

  nw = ip[0];
  nc = ip[1];

  if (isgn < 0) {
    a[1] = 0.5 * (a[1] - a[0]);
    a[0] += a[1];

    for (j = 3; j <= n - 1; j += 2) {
      a[j] = -a[j];
    }

    if (n > 4) {
      rftsub(n, a, nc, w + nw);
      bitrv2(n, ip + 2, a);
    }

    cftsub(n, a, w);

    for (j = 1; j < n; j += 2) {
      a[j] = -a[j];
    }
  }

  else {

    if (n > 4) {
      bitrv2(n, ip + 2, a);
    }

    cftsub(n, a, w);

    if (n > 4) {
      rftsub(n, a, nc, w + nw);
    }

    xi = a[0] - a[1];
    a[0] += a[1];
    a[1] = xi;
  }
}


static void bitrv2(int n, int *ip, float *a)
{
  int j, j1, k, k1, l, m, m2;
  float xr, xi;

  ip[0] = 0;
  l = n;
  m = 1;

  while ((m << 2) < l) {
    l >>= 1;
    for (j = 0; j < m; j++) {
      ip[m + j] = ip[j] + l;
    }
    m <<= 1;
  }

  if ((m << 2) > l) {

    for (k = 1; k < m; k++) {

      for (j = 0; j <= k - 1; j++) {
	j1 = (j << 1) + ip[k];
	k1 = (k << 1) + ip[j];
	xr = a[j1];
	xi = a[j1 + 1];
	a[j1] = a[k1];
	a[j1 + 1] = a[k1 + 1];
	a[k1] = xr;
	a[k1 + 1] = xi;
      }
    }
  }

  else {
    m2 = m << 1;

    for (k = 1; k < m; k++) {

      for (j = 0; j < k; j++) {
	j1 = (j << 1) + ip[k];
	k1 = (k << 1) + ip[j];
	xr = a[j1];
	xi = a[j1 + 1];
	a[j1] = a[k1];
	a[j1 + 1] = a[k1 + 1];
	a[k1] = xr;
	a[k1 + 1] = xi;
	j1 += m2;
	k1 += m2;
	xr = a[j1];
	xi = a[j1 + 1];
	a[j1] = a[k1];
	a[j1 + 1] = a[k1 + 1];
	a[k1] = xr;
	a[k1 + 1] = xi;
      }
    }
  }
}


static void cftsub(int n, float *a, float *w)
{
  int j, j1, j2, j3, k, k1, ks, l, m;
  float wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
  float x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
    
  l = 2;

  while ((l << 1) < n) {
    m = l << 2;

    for (j = 0; j <= l - 2; j += 2) {
      j1 = j + l;
      j2 = j1 + l;
      j3 = j2 + l;
      x0r = a[j] + a[j1];
      x0i = a[j + 1] + a[j1 + 1];
      x1r = a[j] - a[j1];
      x1i = a[j + 1] - a[j1 + 1];
      x2r = a[j2] + a[j3];
      x2i = a[j2 + 1] + a[j3 + 1];
      x3r = a[j2] - a[j3];
      x3i = a[j2 + 1] - a[j3 + 1];
      a[j] = x0r + x2r;
      a[j + 1] = x0i + x2i;
      a[j2] = x0r - x2r;
      a[j2 + 1] = x0i - x2i;
      a[j1] = x1r - x3i;
      a[j1 + 1] = x1i + x3r;
      a[j3] = x1r + x3i;
      a[j3 + 1] = x1i - x3r;
    }

    if (m < n) {
      wk1r = w[2];

      for (j = m; j <= l + m - 2; j += 2) {
	j1 = j + l;
	j2 = j1 + l;
	j3 = j2 + l;
	x0r = a[j] + a[j1];
	x0i = a[j + 1] + a[j1 + 1];
	x1r = a[j] - a[j1];
	x1i = a[j + 1] - a[j1 + 1];
	x2r = a[j2] + a[j3];
	x2i = a[j2 + 1] + a[j3 + 1];
	x3r = a[j2] - a[j3];
	x3i = a[j2 + 1] - a[j3 + 1];
	a[j] = x0r + x2r;
	a[j + 1] = x0i + x2i;
	a[j2] = x2i - x0i;
	a[j2 + 1] = x0r - x2r;
	x0r = x1r - x3i;
	x0i = x1i + x3r;
	a[j1] = wk1r * (x0r - x0i);
	a[j1 + 1] = wk1r * (x0r + x0i);
	x0r = x3i + x1r;
	x0i = x3r - x1i;
	a[j3] = wk1r * (x0i - x0r);
	a[j3 + 1] = wk1r * (x0i + x0r);
      }

      k1 = 1;
      ks = -1;

      for (k = (m << 1); k <= n - m; k += m) {
	k1++;
	ks = -ks;
	wk1r = w[k1 << 1];
	wk1i = w[(k1 << 1) + 1];
	wk2r = ks * w[k1];
	wk2i = w[k1 + ks];
	wk3r = wk1r - 2 * wk2i * wk1i;
	wk3i = 2 * wk2i * wk1r - wk1i;

	for (j = k; j <= l + k - 2; j += 2) {
	  j1 = j + l;
	  j2 = j1 + l;
	  j3 = j2 + l;
	  x0r = a[j] + a[j1];
	  x0i = a[j + 1] + a[j1 + 1];
	  x1r = a[j] - a[j1];
	  x1i = a[j + 1] - a[j1 + 1];
	  x2r = a[j2] + a[j3];
	  x2i = a[j2 + 1] + a[j3 + 1];
	  x3r = a[j2] - a[j3];
	  x3i = a[j2 + 1] - a[j3 + 1];
	  a[j] = x0r + x2r;
	  a[j + 1] = x0i + x2i;
	  x0r -= x2r;
	  x0i -= x2i;
	  a[j2] = wk2r * x0r - wk2i * x0i;
	  a[j2 + 1] = wk2r * x0i + wk2i * x0r;
	  x0r = x1r - x3i;
	  x0i = x1i + x3r;
	  a[j1] = wk1r * x0r - wk1i * x0i;
	  a[j1 + 1] = wk1r * x0i + wk1i * x0r;
	  x0r = x1r + x3i;
	  x0i = x1i - x3r;
	  a[j3] = wk3r * x0r - wk3i * x0i;
	  a[j3 + 1] = wk3r * x0i + wk3i * x0r;
	}
      }
    }

    l = m;
  }

  if (l < n) {

    for (j = 0; j <= l - 2; j += 2) {
      j1 = j + l;
      x0r = a[j] - a[j1];
      x0i = a[j + 1] - a[j1 + 1];
      a[j] += a[j1];
      a[j + 1] += a[j1 + 1];
      a[j1] = x0r;
      a[j1 + 1] = x0i;
    }
  }
}


static void rftsub(int n, float *a, int nc, float *c)
{
  int j, k, kk, ks;
  float wkr, wki, xr, xi, yr, yi;

  ks = (nc << 2) / n;
  kk = 0;

  for (k = (n >> 1) - 2; k >= 2; k -= 2) {
    j = n - k;
    kk += ks;
    wkr = 0.5 - c[kk];
    wki = c[nc - kk];
    xr = a[k] - a[j];
    xi = a[k + 1] + a[j + 1];
    yr = wkr * xr - wki * xi;
    yi = wkr * xi + wki * xr;
    a[k] -= yr;
    a[k + 1] -= yi;
    a[j] += yr;
    a[j + 1] -= yi;
  }
}


static void makewt(int nw, int *ip, float *w)
{
    int nwh, j;
    float delta, x, y;

    ip[0] = nw;
    ip[1] = 1;
    if (nw > 2) {
        nwh = nw >> 1;
        delta = atan(1.0) / nwh;
        w[0] = 1;
        w[1] = 0;
        w[nwh] = cos(delta * nwh);
        w[nwh + 1] = w[nwh];
        for (j = 2; j <= nwh - 2; j += 2) {
            x = cos(delta * j);
            y = sin(delta * j);
            w[j] = x;
            w[j + 1] = y;
            w[nw - j] = y;
            w[nw - j + 1] = x;
        }
        bitrv2(nw, ip + 2, w);
    }
}


static void makect(int nc, int *ip, float *c)
{
    int nch, j;
    float delta;

    ip[1] = nc;
    if (nc > 1) {
        nch = nc >> 1;
        delta = atan(1.0) / nch;
        c[0] = 0.5;
        c[nch] = 0.5 * cos(delta * nch);
        for (j = 1; j <= nch - 1; j++) {
            c[j] = 0.5 * cos(delta * j);
            c[nc - j] = 0.5 * sin(delta * j);
        }
    }
}

