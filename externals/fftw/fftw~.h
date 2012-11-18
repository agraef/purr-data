#ifndef FFTW_TILDE_H_
#define FFTW_TILDE_H_

#include "fftw_config.h"

#include "m_pd.h"

#ifdef HAVE_LIBFFTW3F
# include "fftw3.h"
# include <string.h>
#endif


#define EXTERNAL_SETUP void

static void fftw_siginvert(t_sample * s, t_int n)
{
  while (n!=0)
    {
      --n;
      s[n]=-s[n];
    }
}
#endif /* FFTW_TILDE_H_ */
