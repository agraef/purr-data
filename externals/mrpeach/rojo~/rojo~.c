/* Martin Peach 20060827 start porting this to pd */
/* 20060828 move the used functions from ruidolib.c to this file */
/*
 *  ruido.c
 *  Copyright (C)  2004 josé manuel berenguer
 *  generador de números aleatorios para max/msp

 *  Este programa es Software Libre; usted puede redistribuirlo
 *  y/o modificarlo bajo los términos de la "GNU General Public
 *  License" como lo publica la "FSF Free Software Foundation",
 *  o (a su elección) de cualquier versión posterior.

 *  Este programa es distribuido con la esperanza de que le será
 *  útil, pero SIN NINGUNA GARANTIA; incluso sin la garantía
 *  implícita por el MERCADEO o EJERCICIO DE ALGUN PROPOSITO en
 *  particular. Vea la "GNU General Public License" para más
 *  detalles.

 *  Usted debe haber recibido una copia de la "GNU General Public
 *  License" junto con este programa, si no, escriba a la "FSF
 *  Free Software Foundation, Inc.", 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 */

#include "m_pd.h"
#include <math.h>

void *rojo_class;

typedef struct _rojo
{
    t_object x_obj;
    t_float  x_scale;
    t_float  x_alpha;
    double   x_u[97];
    double   x_c;
    double   x_cd;
    double   x_cm;
    int      x_i97;
    int      x_j97;
    int      x_test;
} t_rojo;

static t_int *perform0(t_int *w);
static void rojo_dsp(t_rojo *x, t_signal **sp);
static void rojo_assist(t_rojo *x, void *b, long m, long a, char *s);
static void *rojo_new(t_symbol *s, short ac, t_atom *av);
void rojo_tilde_setup(void);
static void rojo_RandomInitialise(t_rojo *x, int ij,int kl);
static double rojo_RandomUniform(t_rojo *x);
static double rojo_RandomGaussian(t_rojo *x, double mean, double stddev);
static double rojo_RuidoRojo(t_rojo *x, double c, double a, double media, double desviacion);

/* some defines from ruidolib.h: */
#ifndef ABS
#define ABS(x) (x < 0 ? -(x) : (x))
#endif
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif
/*------------------------------------------------------------------*/

void rojo_tilde_setup(void)
{
    rojo_class = class_new(gensym("rojo~"), (t_newmethod)rojo_new, 0,
        sizeof(t_rojo), CLASS_NOINLET, A_GIMME, 0);
    class_addmethod(rojo_class, (t_method)rojo_dsp, gensym("dsp"), 0);
}
/*------------------------------------------------------------------*/
static t_int *perform0(t_int *w)
{
    t_rojo  *x = (t_rojo *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    double  scale = x->x_scale;
    double  alpha = x->x_alpha;
    int     n = (int)(w[3]);

    if (alpha > 1.0) alpha = 1.0;/* MP 20060827 audio sticks if this is allowed */
    else if (alpha < -1.0) alpha = -1.0;/* MP 20060827 audio sticks if this is allowed */
    while (--n) *++out = scale * rojo_RuidoRojo(x, 0.0, alpha, 0.0, 1.0); 
    return (w+4);
}
/*------------------------------------------------------------------*/
static void rojo_dsp(t_rojo *x, t_signal **sp)
{
    dsp_add(perform0, 3, x, sp[0]->s_vec, sp[0]->s_n);
}
/*------------------------------------------------------------------*/
/* outlet 0:  red noise outlet       */
/*  inlet 0:  (float) scale (0, x)   */
/*  inlet 1:  (float), alpha (-1, 1) */
/*------------------------------------------------------------------*/
static void *rojo_new(t_symbol *s, short ac, t_atom *av)
{
    t_rojo *x = (t_rojo *)pd_new(rojo_class);
    outlet_new(&x->x_obj, gensym("signal"));
    floatinlet_new(&x->x_obj, &x->x_scale);
    floatinlet_new(&x->x_obj, &x->x_alpha);

    post("rojo~ x[n] = scale * (alpha * x[n-1] + z[n])", 0);
    if (ac == 2)
    {
        x->x_scale = (t_float) av[0].a_w.w_float; 
        x->x_alpha = (t_float) av[1].a_w.w_float; 
    }
    else
    {
        x->x_scale = (t_float)0.1; 
        x->x_alpha = (t_float)0.7;
    }
    post("scale %lf", x->x_scale);
    post("alpha %lf", x->x_alpha);
    x->x_test = FALSE;
    return (x);
}

/*
    Miscellaneous routines
*/

/*
   This Random Number Generator is based on the algorithm in a FORTRAN
   version published by George Marsaglia and Arif Zaman, Florida State
   University; ref.: see original comments below.
   At the fhw (Fachhochschule Wiesbaden, W.Germany), Dept. of Computer
   Science, we have written sources in further languages (C, Modula-2
   Turbo-Pascal(3.0, 5.0), Basic and Ada) to get exactly the same test
   results compared with the original FORTRAN version.
   April 1989
   Karl-L. Noell <NOELL@DWIFH1.BITNET>
      and  Helmut  Weber <WEBER@DWIFH1.BITNET>

   This random number generator originally appeared in "Toward a Universal
   Random Number Generator" by George Marsaglia and Arif Zaman.
   Florida State University Report: FSU-SCRI-87-50 (1987)
   It was later modified by F. James and published in "A Review of Pseudo-
   random Number Generators"
   THIS IS THE BEST KNOWN RANDOM NUMBER GENERATOR AVAILABLE.
   (However, a newly discovered technique can yield
   a period of 10^600. But that is still in the development stage.)
   It passes ALL of the tests for random number generators and has a period
   of 2^144, is completely portable (gives bit identical results on all
   machines with at least 24-bit mantissas in the floating point
   representation).
   The algorithm is a combination of a Fibonacci sequence (with lags of 97
   and 33, and operation "subtraction plus one, modulo one") and an
   "arithmetic sequence" (using subtraction).

   Use IJ = 1802 & KL = 9373 to test the random number generator. The
   subroutine RANMAR should be used to generate 20000 random numbers.
   Then display the next six random numbers generated multiplied by 4096*4096
   If the random number generator is working properly, the random numbers
   should be:
           6533892.0  14220222.0  7275067.0
           6172232.0  8354498.0   10633180.0
*/

/*
   This is the initialization routine for the random number generator.
   NOTE: The seed variables can have values between:    0 <= IJ <= 31328
                                                        0 <= KL <= 30081
   The random number sequences created by these two seeds are of sufficient
   length to complete an entire calculation with. For example, if sveral
   different groups are working on different parts of the same calculation,
   each group could be assigned its own IJ seed. This would leave each group
   with 30000 choices for the second seed. That is to say, this random
   number generator can create 900 million different subsequences -- with
   each subsequence having a length of approximately 10^30.
*/
static void rojo_RandomInitialise(t_rojo *x, int ij, int kl)
{
    double s, t;
    int    ii, i, j, k, l, jj, m;

    /*
      Handle the seed range errors:
       First random number seed must be between 0 and 31328.
       Second seed must have a value between 0 and 30081.
    */
    if (ij < 0 || ij > 31328 || kl < 0 || kl > 30081)
    {
        ij = 1802;
        kl = 9373;
    }

    i = (ij / 177) % 177 + 2;
    j = (ij % 177) + 2;
    k = (kl / 169) % 178 + 1;
    l = (kl % 169);

    for (ii=0; ii<97; ii++)
    {
        s = 0.0;
        t = 0.5;
        for (jj=0; jj<24; jj++)
        {
            m = (((i * j) % 179) * k) % 179;
            i = j;
            j = k;
            k = m;
            l = (53 * l + 1) % 169;
            if (((l * m % 64)) >= 32) s += t;
            t *= 0.5;
        }
        x->x_u[ii] = s;
    }

    x->x_c = 362436.0 / 16777216.0;
    x->x_cd = 7654321.0 / 16777216.0;
    x->x_cm = 16777213.0 / 16777216.0;
    x->x_i97 = 97;
    x->x_j97 = 33;
    x->x_test = TRUE;
}

/* 
   This is the random number generator proposed by George Marsaglia in
   Florida State University Report: FSU-SCRI-87-50
*/
static double rojo_RandomUniform(t_rojo *x)
{
    double uni;

    /* Make sure the initialisation routine has been called */
    if (!x->x_test) rojo_RandomInitialise(x, 1802,9373);

    uni = x->x_u[x->x_i97-1] - x->x_u[x->x_j97-1];
    if (uni <= 0.0) uni++;
    x->x_u[x->x_i97-1] = uni;
    x->x_i97--;
    if (x->x_i97 == 0) x->x_i97 = 97;
    x->x_j97--;
    if (x->x_j97 == 0) x->x_j97 = 97;
    x->x_c -= x->x_cd;
    if (x->x_c < 0.0) x->x_c += x->x_cm;
    uni -= x->x_c;
    if (uni < 0.0) uni++;

    return(uni);
}

/*
  ALGORITHM 712, COLLECTED ALGORITHMS FROM ACM.
  THIS WORK PUBLISHED IN TRANSACTIONS ON MATHEMATICAL SOFTWARE,
  VOL. 18, NO. 4, DECEMBER, 1992, PP. 434-435.
  The function returns a normally distributed pseudo-random number
  with a given mean and standard devaiation.  Calls are made to a
  function subprogram which must return independent random
  numbers uniform in the interval (0,1).
  The algorithm uses the ratio of uniforms method of A.J. Kinderman
  and J.F. Monahan augmented with quadratic bounding curves.
*/
static double rojo_RandomGaussian(t_rojo *x, double mean, double stddev)
{
    double  q, u, v, xx, y;

    /*  
      Generate P = (u,v) uniform in rect. enclosing acceptance region 
      Make sure that any random numbers <= 0 are rejected, since
      gaussian() requires uniforms > 0, but RandomUniform() delivers >= 0.
    */
    do
    {
        u = rojo_RandomUniform(x);
        v = rojo_RandomUniform(x);
        if (u <= 0.0 || v <= 0.0) 
        {
            u = 1.0;
            v = 1.0;
        }
        v = 1.7156 * (v - 0.5);

        /*  Evaluate the quadratic form */
        xx = u - 0.449871;
        y = ABS(v) + 0.386595;
        q = xx * xx + y * (0.19600 * y - 0.25472 * xx);

        /* Accept P if inside inner ellipse */
        if (q < 0.27597) break;

        /*  Reject P if outside outer ellipse, or outside acceptance region */
    } while ((q > 0.27846) || (v * v > -4.0 * log(u) * u * u));

    /*  Return ratio of P's coordinates as the normal deviate */
    return (mean + stddev * v / u);
}

/*-------------------------------------------------------------------------
    Esta calcula una muestra de una secuencia de ruido rojo obtenida por un proceso 
    autoregresivo de primer orden (AR(1)) del tipo x[n] = c + a x[n-1] + z[n]

    donce 
    c constante (casi siempre = 0 para su uso en max
    a es constante -1 > a < 1
    z[n] es la innovación. una muestra de un ruido gaussiano
    x[n] es la muestra actual
    x[n-1] es la anterior muestra obtenida
    media es la media del ruido gaussiano y desviacion es la desviación tipica del ruido gaussiano
*/
/*---------translated by mrpeach:
    Calculate a sample of a sequence of red noise obtained by a first-order
    autoregressive process (AR(1)) of type x[n] = c + a x[n-1] + z[n] 
    where:
    c = constant (always = 0 for maximum output)
    a = constant (-1 < a < 1)
    z[n] is a new sample of gaussian noise
    x[n] is the current sample
    x[n-1] is the previous sample obtained
    media is the mean and desviacion is the standard deviation of the gaussian noise
*/
static double rojo_RuidoRojo(t_rojo *x, double c, double a, double media, double desviacion)
{
    static double xx;

    xx = c + a * xx + rojo_RandomGaussian(x, media, desviacion);

    return xx;
}

/* end of rojo~.c */
