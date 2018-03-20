/*
disis.munger~ 1.4.3
a realtime multichannel granulator

a flext (cross-platform PD & Max/MSP) port of
the munger~ object from the PeRColate library (0.9 beta6)
http://www.music.columbia.edu/PeRColate/

Original PeRColate library by:

Dan Trueman http://www.music.princeton.edu/~dan/
R. Luke DuBois's http://www.lukedubois.com/

Flext port and additions by:
Ivica Ico Bukvic http://ico.bukvic.net
Ji-Sun Kim hideaway@vt.edu
http://disis.music.vt.edu

Released under GPL license
(whichever is the latest version--as of this release, version 2)
For more info on the GPL license please visit:
http://www.gnu.org/copyleft/gpl.html

For latest changes please see changelog
*/

#include "m_pd.h"
#include "ADSR.h" /* small C library ported from stk */
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

//version
#define MUNGER_MAJOR 1
#define MUNGER_MINOR 4
#define MUNGER_REV 3

/* MSVC doesn't know RANDOM(), while GCC's
   (at least on Linux) has rand() limit much higher */
#if !defined(__GNUC__) || defined(__MINGW32__)
#define RANDOM() (rand())
#else
#define RANDOM() (random()%32768)
#endif

#define ONE_OVER_HALFRAND 0.00006103516	/* constant = 1. / 16384.0 */
#define ONE_OVER_MAXRAND 0.000030517578	/* 1 / 32768 */
#define MINSPEED .001 /* minimum speed through buffer on playback */
#define ENVSIZE 32
#define ONE_OVER_ENVSIZE .0078125
#define MINSIZE 64 /* twice ENVSIZE. minimum grainsize in samples */
#define RAND01 (((long)RANDOM() * ONE_OVER_MAXRAND) /* RANDOM() numbers 0-1 */

/* RANDOM() numbers -1 to 1 */
#define RAND11 (((long)RANDOM() - 16384.) * ONE_OVER_HALFRAND)
#define WINLENGTH 1024

/* max number of transpositions for the "scale" message */
#define PITCHTABLESIZE 1000
#define RECORDRAMP 1000
#define RECORDRAMP_INV 0.001

/* these are arbitrary-- can we trust users not to
   do something silly and remove these? */
#define MAXCHANNELS 64
#define MAXVOICES 1000

/* useful define */
#ifndef TWOPI
#define TWOPI 6.28318530717958647692
#endif

static t_class *disis_munger_class;

typedef struct _disis_munger {
    t_object x_obj;
    t_float x_f;
    t_float x_srate;
    t_float x_one_over_srate;
    t_float x_srate_ms;
    t_float x_one_over_srate_ms;
    t_float x_initbuflen;
    int x_buflen;
    t_float x_maxdelay;
    int x_num_channels;
    int x_numvoices;
    t_symbol *x_munger_name;
    t_float x_maxsize;
    t_float x_minsize;
    t_float x_twothirdBufsize;
    t_float x_onethirdBufsize;
    int x_verbose;

    /* Ancillary inlet vars */
    t_float x_grate;
    t_float x_grate_var;
    t_float x_glen;
    t_float x_glen_var;
    t_float x_gpitch;
    t_float x_gpitch_var;
    t_float x_gpan_spread;

    /* Heap allocated based on number of voices */
    t_float *x_recordBuf;
    t_float *x_winTime;
    t_float *x_winRate;
    long *x_gvoiceSize;
    double *x_gvoiceSpeed;
    double *x_gvoiceCurrent;
    int *x_gvoiceDirection;
    int *x_gvoiceOn;
    long *x_gvoiceDone;
    t_float *x_gvoiceLPan;
    t_float *x_gvoiceRPan;
    t_float *x_gvoiceRamp;
    t_float *x_gvoiceOneOverRamp;
    t_float *x_gvoiceGain;
    t_stk_ADSR *x_gvoiceADSR;
    int *x_gvoiceADSRon;
    t_float *x_noteTransp;
    t_float *x_noteSize;
    t_float *x_notePan;
    t_float *x_noteGain;
    t_float *x_noteAttack;
    t_float *x_noteDecay;
    t_float *x_noteSustain;
    t_float *x_noteRelease;
    int *x_noteDirection;

    /* nvoices x nchannels */
    t_float **x_gvoiceSpat;
    t_float **x_notechannelGain;
    t_float **x_notechannelGainSpread;

    /* Heap allocated for signal vector x nchannels */
    t_float **x_out;
    /* Heap allocated x nchannels */
    t_float *x_outsamp;
    t_float *x_channelGain;
    t_float *x_channelGainSpread;

    /* Oh wow, there are more... */
    int x_graincounter;
    int x_countsamples;
    int x_voices;
    t_float x_gain;
    t_float x_randgain;
    t_float x_twelfth;
    t_float x_semitone;
    short x_smoothPitch;
    int x_scale_len;

    t_float x_tempgrate;
    long x_time;
    t_float x_position;
    t_float x_gimme;
    int x_power;
    short x_ambi;
    long x_maxvoices;

    short x_oneshot;
    int x_newnote;

    short x_doHanning;
    t_float x_winTable[WINLENGTH];
    t_float x_pitchTable[PITCHTABLESIZE];
    t_float x_rampLength;

    int x_recordOn;
    long x_recordCurrent;
    int x_recordRampVal;   /* ramp for when toggling record on and off */
    int x_rec_ramping;     /* -1 ramp down, 1 ramp up, 0 not ramping */

    t_symbol *x_arrayname;
    int x_arraylength;
    t_word *x_arrayvec;      /* vec to use if we want an external buffer */
    long x_l_chan;           /* is there any other choice? */
    int x_discretepan;       /* off by default */
} t_disis_munger;


static void float_2d_alloc(t_float ***fp, int nrow, int ncol)
{
    int i;
    *fp = t_getbytes(nrow * sizeof(t_float*));
    for (i = 0; i < nrow; i++)
        (*fp)[i] = t_getbytes(ncol * sizeof(t_float));
}

static void float_2d_free(t_float ***fp, int nrow, int ncol)
{
    int i;
    for (i = 0; i < nrow; i++)
        t_freebytes((*fp)[i], ncol * sizeof(t_float));
    t_freebytes(*fp, nrow * sizeof(t_float*));
}

static t_disis_munger *munger_alloc(t_disis_munger *x)
{
    /* Heap allocated based on number of voices */
    int nv = x->x_numvoices, nchan = x->x_num_channels;

    x->x_recordBuf = (t_float *)t_getbytes((x->x_buflen + 1) * sizeof(t_float));

    /* If recordBuf didn't get allocated, let's go ahead and bail. Otherwise
       we just assume all the ones below will succeed. */

    if (!x->x_recordBuf)
    {
        error("disis_munger~ %s: out of memory", x->x_munger_name->s_name);
        return 0;
    }
    x->x_winTime = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_winRate = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_gvoiceSize = (long *)t_getbytes(nv * sizeof(long));
    x->x_gvoiceSpeed = (double *)t_getbytes(nv * sizeof(double));
    x->x_gvoiceCurrent = (double *)t_getbytes(nv * sizeof(double));
    x->x_gvoiceDirection = (int *)t_getbytes(nv * sizeof(int));
    x->x_gvoiceOn = (int *)t_getbytes(nv * sizeof(int));
    x->x_gvoiceDone = (long *)t_getbytes(nv * sizeof(long));
    x->x_gvoiceLPan = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_gvoiceRPan = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_gvoiceRamp = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_gvoiceOneOverRamp = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_gvoiceGain = (t_float *)t_getbytes(nv * sizeof(t_float));

    /* This is its own type */
    x->x_gvoiceADSR = (t_stk_ADSR *)t_getbytes(nv * sizeof(t_stk_ADSR)); 
    x->x_gvoiceADSRon = (int *)t_getbytes(nv * sizeof(int));
    x->x_noteTransp = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_noteSize = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_notePan = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_noteGain = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_noteAttack = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_noteDecay = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_noteSustain = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_noteRelease = (t_float *)t_getbytes(nv * sizeof(t_float));
    x->x_noteDirection = (int *)t_getbytes(nv * sizeof(int));

    /* nvoices x nchannels */
    float_2d_alloc(&x->x_gvoiceSpat, nv, nchan);
    float_2d_alloc(&x->x_notechannelGain, nv, nchan);
    float_2d_alloc(&x->x_notechannelGainSpread, nv, nchan);

    /* Heap allocated for signal vector x nchannels */
    x->x_out = (t_float **)t_getbytes(nchan * sizeof(t_float*));
    /* Heap allocated x nchannels */
    x->x_outsamp = (t_float *)t_getbytes(nchan * sizeof(t_float));
    x->x_channelGain = (t_float *)t_getbytes(nchan * sizeof(t_float));
    x->x_channelGainSpread = (t_float *)t_getbytes(nchan * sizeof(t_float));
    return x;
}

static void munger_free(t_disis_munger *x)
{
    /* Heap allocated based on number of voices */
    int nv = x->x_numvoices, nchan = x->x_num_channels;

    if (x->x_recordBuf)
        t_freebytes(x->x_recordBuf, (x->x_buflen + 1) * sizeof(t_float));
    if (x->x_winTime)
        t_freebytes(x->x_winTime, nv * sizeof(t_float));
    if (x->x_winRate)
        t_freebytes(x->x_winRate, nv * sizeof(t_float));
    if (x->x_gvoiceSize)
        t_freebytes(x->x_gvoiceSize, nv * sizeof(long));
    if (x->x_gvoiceSpeed)
        t_freebytes(x->x_gvoiceSpeed, nv * sizeof(double));
    if (x->x_gvoiceCurrent)
        t_freebytes(x->x_gvoiceCurrent, nv * sizeof(double));
    if (x->x_gvoiceDirection)
        t_freebytes(x->x_gvoiceDirection, nv * sizeof(int));
    if (x->x_gvoiceOn)
        t_freebytes(x->x_gvoiceOn, nv * sizeof(int));
    if (x->x_gvoiceDone)
        t_freebytes(x->x_gvoiceDone, nv * sizeof(long));
    if (x->x_gvoiceLPan)
        t_freebytes(x->x_gvoiceLPan, nv * sizeof(t_float));
    if (x->x_gvoiceRPan)
        t_freebytes(x->x_gvoiceRPan, nv * sizeof(t_float));
    if (x->x_gvoiceRamp)
        t_freebytes(x->x_gvoiceRamp, nv * sizeof(t_float));
    if (x->x_gvoiceOneOverRamp)
        t_freebytes(x->x_gvoiceOneOverRamp, nv * sizeof(t_float));
    if (x->x_gvoiceGain)
        t_freebytes(x->x_gvoiceGain, nv * sizeof(t_float));

    if (x->x_gvoiceADSR)
        t_freebytes(x->x_gvoiceADSR, nv * sizeof(t_stk_ADSR)); 

    if (x->x_gvoiceADSRon)
        t_freebytes(x->x_gvoiceADSRon, nv * sizeof(int));
    if (x->x_noteTransp)
        t_freebytes(x->x_noteTransp, nv * sizeof(t_float));
    if (x->x_noteSize)
        t_freebytes(x->x_noteSize, nv * sizeof(t_float));
    if (x->x_notePan)
        t_freebytes(x->x_notePan, nv * sizeof(t_float));
    if (x->x_noteGain)
        t_freebytes(x->x_noteGain, nv * sizeof(t_float));
    if (x->x_noteAttack)
        t_freebytes(x->x_noteAttack, nv * sizeof(t_float));
    if (x->x_noteDecay)
        t_freebytes(x->x_noteDecay, nv * sizeof(t_float));
    if (x->x_noteSustain)
        t_freebytes(x->x_noteSustain, nv * sizeof(t_float));
    if (x->x_noteRelease)
        t_freebytes(x->x_noteRelease, nv * sizeof(t_float));
    if (x->x_noteDirection)
        t_freebytes(x->x_noteDirection, nv * sizeof(int));

    /* nvoices x nchannels */
    float_2d_free(&x->x_gvoiceSpat, nv, nchan);
    float_2d_free(&x->x_notechannelGain, nv, nchan);
    float_2d_free(&x->x_notechannelGainSpread, nv, nchan);

    /* Heap allocated for signal vector x nchannels */
    if (x->x_out)
        t_freebytes(x->x_out, nchan * sizeof(t_float*));
    /* Heap allocated x nchannels */
    if (x->x_outsamp)
        t_freebytes(x->x_outsamp, nchan * sizeof(t_float));
    if (x->x_channelGain)
        t_freebytes(x->x_channelGain, nchan * sizeof(t_float));
    if (x->x_channelGainSpread)
        t_freebytes(x->x_channelGainSpread, nchan * sizeof(t_float));
}

static void *munger_new(t_symbol *s, int argc, t_atom *argv)
{
    t_float maxdelay = 3000.;
    int nchan = 2, nvoices = 50, i, j;
    t_symbol *munger_name = gensym("default");

    t_disis_munger *x = (t_disis_munger *)pd_new(disis_munger_class);

    if (argc) /* 1st arg: maxdelay */
    {
        /* Special case-- the help file says that a single symbolic argument
           is allowed for backward-compatibility. So we cover that here... */
        if (argc == 1 && argv->a_type == A_SYMBOL)
            munger_name = atom_getsymbolarg(0, argc--, argv++);
        else
        {
            t_float tmp = atom_getfloatarg(0, argc--, argv++);
            /* keep default if less than 100 */
            if (tmp >= 100.) maxdelay = tmp;
        }
    }

    if (argc) /* 2nd arg: no of channels */
    {
        nchan = (int)atom_getintarg(0, argc--, argv++);
        if (nchan < 2) nchan = 2;
        if (nchan > MAXCHANNELS) nchan = MAXCHANNELS;
    }

    if (argc) /* 3rd arg: either max voices OR a symbolic name IF this
                 is the final arg */
    {
        if (argc == 1 && argv->a_type == A_SYMBOL)
            munger_name = atom_getsymbolarg(0, argc--, argv++);
        else
        {
            nvoices = atom_getfloatarg(0, argc--, argv++);
            if (nvoices < 0) nvoices = 0;
            if (nvoices > 1000) nvoices = 1000;
        }
    }

    if (argc) /* 4th arg: name */
    {
        munger_name = atom_getsymbolarg(0, argc--, argv++);
    }

    x->x_maxdelay = maxdelay;
    x->x_num_channels = nchan;
    x->x_numvoices = nvoices;
    x->x_munger_name = munger_name;
    x->x_srate = sys_getsr();
    x->x_one_over_srate = 1./x->x_srate;
    x->x_srate_ms = x->x_srate/1000.;
    x->x_one_over_srate_ms = 1./x->x_srate_ms;
    x->x_initbuflen = maxdelay + 50. * x->x_srate_ms;
    x->x_buflen = (int)x->x_initbuflen;
    x->x_maxsize = x->x_buflen / 3;
    x->x_twothirdBufsize = x->x_maxsize * 2;
    x->x_onethirdBufsize = x->x_maxsize;
    x->x_minsize = MINSIZE;

    /* allocate a ton of fields */
    x = munger_alloc(x);
    /* bail if we couldn't allocate... */
    if (!x) return 0;

    x->x_verbose = 1;

    floatinlet_new(&x->x_obj, &x->x_grate);
    floatinlet_new(&x->x_obj, &x->x_grate_var);
    floatinlet_new(&x->x_obj, &x->x_glen);
    floatinlet_new(&x->x_obj, &x->x_glen_var);
    floatinlet_new(&x->x_obj, &x->x_gpitch);
    floatinlet_new(&x->x_obj, &x->x_gpitch_var);
    floatinlet_new(&x->x_obj, &x->x_gpan_spread);

    for (i = 0; i < nchan; i++)
        outlet_new(&x->x_obj, &s_signal);

    x->x_voices = 10;
    x->x_gain = 1.1;
    x->x_randgain = 0.;
    x->x_twelfth = 1./12;
    x->x_semitone  = pow(2., 1./12.);
    x->x_smoothPitch = 1;
    x->x_scale_len = PITCHTABLESIZE;

    x->x_grate = 1.;
    x->x_tempgrate = 1.;
    x->x_grate_var = 0.;
    x->x_glen = 1.;
    x->x_glen_var = 0.;
    x->x_gpitch = 1.;
    x->x_gpitch_var = 0.;
    x->x_gpan_spread = 0.;
    x->x_time = 0;
    x->x_position = -1.;
    x->x_gimme = 0.;
    x->x_power = 1;
    x->x_ambi = 0;
    x->x_maxvoices = x->x_numvoices;

    x->x_oneshot = 0;
    x->x_newnote = 0;

    for (i = 0; i < nvoices; i++)
    {
        x->x_gvoiceSize[i] = 1000;
        x->x_gvoiceSpeed[i] = 1.;
        x->x_gvoiceCurrent[i] = 0.;
        x->x_gvoiceDirection[i] = 1;
        x->x_gvoiceOn[i] = 0;
        x->x_gvoiceDone[i] = 0;
        x->x_gvoiceRPan[i] = .5;
        x->x_gvoiceLPan[i] = .5;
        x->x_gvoiceGain[i] = 1.;
        x->x_gvoiceADSRon[i] = 0;

        /* init the stk_ADSR elements */
        stk_ADSR_init(&x->x_gvoiceADSR[i]);
        stk_ADSR_setSampleRate(&x->x_gvoiceADSR[i], sys_getsr());
        for (j = 0; j < x->x_num_channels; j++)
        {
            x->x_gvoiceSpat[i][j] = 0.;
            x->x_notechannelGain[i][j] = 0.;
            x->x_notechannelGainSpread[i][j] = 0.;
        }
        //note and oneshot inits
        x->x_noteTransp[i] = 0.;
        x->x_noteSize[i] = 100.;
        x->x_notePan[i] = 0.5;
        x->x_noteGain[i] = 1.;
        x->x_noteAttack[i] = 20.;
        x->x_noteDecay[i] = 50.;
        x->x_noteSustain[i] = 0.3;
        x->x_noteRelease[i] = 200.;
    }
    for (i = 0; i < x->x_num_channels; i++)
    {
        x->x_channelGain[i] = 0.;
        x->x_channelGainSpread[i] = 0.;
    }

    x->x_doHanning = 0; // init hanning window

    for (i = 0; i < WINLENGTH; i++)
    {
        x->x_winTable[i] = 0.5 + 0.5 * cos(TWOPI * i / WINLENGTH + .5 * TWOPI);
    }

    for (i = 0; i < PITCHTABLESIZE; i++)
    {
        x->x_pitchTable[i] = 0.;
    }

    x->x_rampLength = 256.;

    //sample buffer
    for (i = 0; i < x->x_initbuflen; i++)
        x->x_recordBuf[i] = 0;

    x->x_recordOn = 1;           //boolean
    x->x_recordCurrent = 0;
    x->x_recordRampVal = 0;
    x->x_rec_ramping = 0;

    x->x_arrayname = NULL;
    x->x_arrayvec = NULL;
    x->x_arraylength = 0;         // use internal buffer by default
    x->x_l_chan = 0;             //is there any other choice?
    x->x_discretepan = 0;        //off by default

    srand(54); //0.54?

    return x;
}

/* creates a size for a new grain
   actual number of samples PLAYED, regardless of pitch
   might be shorter for higher pitches and long grains,
   to avoid collisions with recordCurrent

   size given now in milliseconds!

   for oneshot notes, this will also scale the ADSR and
   make it smaller, if the grainSpeed is high
*/
t_float munger_newNoteSize(t_disis_munger *x, int whichOne, int newNote)
{
    t_float newsize, temp, temp2, pitchExponent;

    /* set grain pitch */
    pitchExponent = x->x_noteTransp[newNote];
    x->x_gvoiceSpeed[whichOne] =
        x->x_gpitch * pow(x->x_semitone, pitchExponent);

    if (x->x_gvoiceSpeed[whichOne] < MINSPEED)
        x->x_gvoiceSpeed[whichOne] = MINSPEED;
    newsize = x->x_srate_ms * (x->x_noteSize[newNote]);
    //if(newsize > x->x_maxsize) newsize = maxsize;
    if (newsize * x->x_gvoiceSpeed[whichOne] > x->x_maxsize)
    {
        temp2 = x->x_maxsize / x->x_gvoiceSpeed[whichOne]; //newsize
        temp = temp2 / newsize;
        x->x_noteAttack[newNote] *= temp;
        x->x_noteDecay[newNote] *= temp;
        x->x_noteRelease[newNote] *= temp;
        newsize = temp2;
    }
    //if(newsize < minsize) newsize = minsize;
    return newsize;
}

static int munger_newDirection(t_disis_munger *x)
{
    //-1 == always backwards
    //0  == backwards and forwards (default)
    //1  == only forwards
    int dir;
    if (x->x_ambi == 0)
    {
        dir = RANDOM()- 16384;
        if (dir < 0) dir = -1;
        else dir = 1;
    }
    else
    {
        if (x->x_ambi == -1) dir = -1;
	else dir = 1;
    }
    return dir;
}

static t_float munger_newNote(t_disis_munger *x, int whichVoice, int newNote)
{
    t_float newPosition;
    int i, temp;

    x->x_gvoiceSize[whichVoice] =
        (long)munger_newNoteSize(x, whichVoice, newNote);
    //x->x_gvoiceDirection[whichVoice] = munger_newDirection(x);
    x->x_gvoiceDirection[whichVoice] = x->x_noteDirection[newNote];

    if (x->x_num_channels == 2)
    {
        //x->x_gvoiceLPan[whichVoice] = ((t_float)rand() - 16384.) *
        //    ONE_OVER_MAXRAND * x->x_gpan_spread + 0.5;
        //x->x_gvoiceRPan[whichVoice] = 1. - x->x_gvoiceLPan[whichVoice];
        //make equal power panning....
        //x->x_gvoiceLPan[whichVoice] = powf(x->x_gvoiceLPan[whichVoice], 0.5);
        //x->x_gvoiceRPan[whichVoice] = powf(x->x_gvoiceRPan[whichVoice], 0.5);
        x->x_gvoiceRPan[whichVoice] = powf(x->x_notePan[newNote], 0.5);
        x->x_gvoiceLPan[whichVoice] = powf((1. - x->x_notePan[newNote]), 0.5);
    }
    else
    {
        if (x->x_notePan[newNote] == -1.)
        {
            for (i = 0; i < x->x_num_channels; i++)
            {
                x->x_notechannelGain[whichVoice][i] = 1.;
                x->x_notechannelGainSpread[whichVoice][i] = 0.;
            }
        }
        else
        {
            for (i = 0; i < x->x_num_channels; i++)
            {
                //initialize all to 0.
                x->x_notechannelGain[whichVoice][i] = 0.;
                x->x_notechannelGainSpread[whichVoice][i] = 0.;
            }
            temp = (int)x->x_notePan[newNote];
            if (temp >= x->x_num_channels) temp=0;
            //update the one we want
            x->x_notechannelGain[whichVoice][temp] = 1.;
        }
        for (i = 0; i < x->x_num_channels; i++)
        {
            x->x_gvoiceSpat[whichVoice][i] =
                x->x_notechannelGain[whichVoice][i] +
                ((long)(RANDOM()) - 16384.) * ONE_OVER_HALFRAND *
                x->x_notechannelGainSpread[whichVoice][i];
        }
    }

    x->x_gvoiceOn[whichVoice] = 1;
    x->x_gvoiceDone[whichVoice] = 0;
    x->x_gvoiceGain[whichVoice] = x->x_noteGain[newNote];

    x->x_gvoiceADSRon[whichVoice] = 1;
    //post("adsr %f %f %f %f",
    //x->x_noteAttack[newNote], x->x_noteDecay[newNote],
    //x->x_noteSustain[newNote], x->x_noteRelease[newNote]);

    stk_ADSR_setAllTimes(&x->x_gvoiceADSR[whichVoice],
        x->x_noteAttack[newNote] / 1000.,
        x->x_noteDecay[newNote] / 1000.,
        x->x_noteSustain[newNote],
        x->x_noteRelease[newNote] / 1000.);
    stk_ADSR_keyOn(&x->x_gvoiceADSR[whichVoice]);

    /*** set start point; tricky, cause of moving buffer,
         variable playback rates, backwards/forwards, etc.... ***/

    if (!x->x_arraylength)
    {
        // 1. RANDOM() positioning and moving buffer (default)
        if (x->x_position == -1. && x->x_recordOn == 1)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) /* going forward */
            {
                if (x->x_gvoiceSpeed[whichVoice] > 1.)
                    newPosition = x->x_recordCurrent - x->x_onethirdBufsize -
                        (long)(RANDOM()) * ONE_OVER_MAXRAND *
                        x->x_onethirdBufsize;
                else
                    newPosition = x->x_recordCurrent - (long)(RANDOM()) *
                        ONE_OVER_MAXRAND * x->x_onethirdBufsize; // was 2/3rds
            }
            else //going backwards
            {
                newPosition = x->x_recordCurrent - (long)(RANDOM()) *
                    ONE_OVER_MAXRAND * x->x_onethirdBufsize;
            }
        }

        // 2. fixed positioning and moving buffer
        else if (x->x_position >= 0. && x->x_recordOn == 1)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) /* going forward */
            {
                if (x->x_gvoiceSpeed[whichVoice] > 1.)
                {
                    //newPosition = x->x_recordCurrent - x->x_onethirdBufsize -
                    //    x->x_position * x->x_onethirdBufsize;
                    //this will follow more closely...
                    newPosition = x->x_recordCurrent -
                        x->x_gvoiceSize[whichVoice] *
                        x->x_gvoiceSpeed[whichVoice] - x->x_position *
                        x->x_onethirdBufsize;
                }
                else
                {
                    newPosition = x->x_recordCurrent - x->x_position *
                        x->x_onethirdBufsize; //was 2/3rds
                }
            }
            else //going backwards
            {
                newPosition = x->x_recordCurrent - x->x_position *
                    x->x_onethirdBufsize;
            }
        }

        // 3. RANDOM() positioning and fixed buffer
        else if (x->x_position == -1. && x->x_recordOn == 0)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) // going forward
            {
                newPosition = x->x_recordCurrent - x->x_onethirdBufsize -
                    (long)(RANDOM()) * ONE_OVER_MAXRAND * x->x_onethirdBufsize;
            }
            else //going backwards
                newPosition = x->x_recordCurrent - (long)(RANDOM()) *
                    ONE_OVER_MAXRAND * x->x_onethirdBufsize;
        }

        // 4. fixed positioning and fixed buffer
        else if (x->x_position >= 0. && x->x_recordOn == 0)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) // going forward
            {
                newPosition = x->x_recordCurrent - x->x_onethirdBufsize -
                    x->x_position * x->x_onethirdBufsize;
            }
            else //going backwards
                newPosition = x->x_recordCurrent - x->x_position *
                    x->x_onethirdBufsize;
        }
    }
    else
    {
        if (x->x_position == -1.)
        {
            newPosition = (long)(RANDOM()) * ONE_OVER_MAXRAND * x->x_arraylength;
        }
        else if (x->x_position >= 0.) newPosition = x->x_position *
            x->x_arraylength;
    }
    return newPosition;
}

static void munger_spat(t_disis_munger *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j;

    if (argc)
    {
        for (i = j = 0; i < (argc - 1); i += 2)
        {
            x->x_channelGain[j] = atom_getfloatarg(i, argc, argv);
            x->x_channelGainSpread[j] = atom_getfloatarg(i+1, argc, argv);
            if (x->x_verbose > 1)
                post("disis_munger~ %s: channel gain %d = %f, spread = %f",
                    x->x_munger_name->s_name, j, x->x_channelGain[j],
                    x->x_channelGainSpread[j]);
            j++;
        }
    }
}

static void munger_note(t_disis_munger *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_oneshot)
    {
        int i, temp;

        if(argc < 8)
        {
            post("disis_munger~ %s: need 8 args -- transposition, gain, pan, "
                 "attkT, decayT, susLevel, relT, direction [-1/1]",
                x->x_munger_name->s_name);
            return;
        }

        x->x_newnote++;

        if (x->x_newnote > x->x_voices)
        {
            if (x->x_verbose > 0)
                post("disis_munger~ %s: too many notes amadeus.",
                    x->x_munger_name->s_name);
            return;
        }

        x->x_noteTransp[x->x_newnote] = atom_getfloatarg(0, argc, argv);
        x->x_noteGain[x->x_newnote] = atom_getfloatarg(1, argc, argv);
        x->x_notePan[x->x_newnote] = atom_getfloatarg(2, argc, argv);
        x->x_noteAttack[x->x_newnote] = atom_getfloatarg(3, argc, argv);
        x->x_noteDecay[x->x_newnote] = atom_getfloatarg(4, argc, argv);
        x->x_noteSustain[x->x_newnote] = atom_getfloatarg(5, argc, argv);
        x->x_noteRelease[x->x_newnote] = atom_getfloatarg(6, argc, argv);
        x->x_noteDirection[x->x_newnote] = atom_getfloatarg(7, argc, argv);

        //Stk ADSR bug?
        if (x->x_noteSustain[x->x_newnote] <= 0.001)
            x->x_noteSustain[x->x_newnote] = 0.001;

        x->x_noteSize[x->x_newnote] = x->x_noteAttack[x->x_newnote] +
            x->x_noteDecay[x->x_newnote] + x->x_noteRelease[x->x_newnote];
    }
}

// turn oneshot mode on/off. in oneshot mode, the internal granular voice
// allocation method goes away
// so the munger will be silent, except when it receives "note" messages
static void munger_oneshot(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    int temp;

    if (argc)
    {
        temp = (int)atom_getintarg(0, argc, argv);
        x->x_oneshot = temp;
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting oneshot: %d",
                x->x_munger_name->s_name, temp);
    }
}

static void munger_clearbuffer(t_disis_munger *x)
{
    if (x->x_arraylength)
    {
        x->x_arrayname = NULL;
        x->x_arrayname = NULL;
        x->x_arraylength = 0;
        x->x_arrayvec = NULL;
        if (x->x_verbose > 1)
            post("disis_munger~ %s: external buffer deleted.",
                x->x_munger_name->s_name);
    }
}

static void munger_setbuffer(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (argc == 0)
    {
        // argument list is empty
        // clear existing buffer
        if (x->x_arraylength) munger_clearbuffer(x);
    }
    else if (argc == 1 && argv->a_type == A_SYMBOL)
    {
        // one symbol given as argument
        // clear existing buffer
        if (x->x_arraylength) munger_clearbuffer(x);
       	// save buffer munger_name
       	x->x_arrayname = atom_getsymbolarg(0, argc, argv);
        /* If our name is NULL then just clear out and return... */
        if (!x->x_arrayname)
        {
            munger_clearbuffer(x);
            return;
        }
       	// make new reference to system buffer object
        /* make a new reference to the array */
        t_garray *g = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
        if (!g)
        {
            if (*s->s_name) pd_error(x, "disis_munger~: %s: no such array",
                x->x_arrayname->s_name);
        }
        else if (!garray_getfloatwords(g, &x->x_arraylength, &x->x_arrayvec))
        {
            pd_error(x, "%s: bad template for disis_munger~'s array",
                x->x_arrayname->s_name);
        }
        else
        {
            if (x->x_verbose > 1)
                post("disis_munger~ %s: successfully associated with "
                     "the %s array.", x->x_munger_name->s_name,
                      x->x_arrayname->s_name);
            x->x_l_chan = 0;
            garray_usedindsp(g);
        }

    }
    else
    {
        // invalid argument list, leave buffer as is but issue error message
        // to console
       	if (x->x_verbose > 0)
            post("disis_munger~ %s: error: message argument must be a string.",
                x->x_munger_name->s_name);
        if (x->x_arraylength) munger_clearbuffer(x);
    }
}

static void munger_setverbose(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    int temp;

    if (argc)
    {
        temp = (int)atom_getintarg(0, argc, argv);
        if (temp < 0) temp = 0;
        if (temp > 3) temp = 3;
        x->x_verbose = temp;
        post("disis_munger~ %s: setting verbose: %d",
            x->x_munger_name->s_name, temp);
        if (x->x_verbose < 3)
        {
            x->x_graincounter = 0;
            x->x_countsamples = 0;
        }
    }
}

//grain funcs
static t_float munger_envelope(t_disis_munger *x, int whichone, t_float sample)
{
    long done = x->x_gvoiceDone[whichone];
    long tail = x->x_gvoiceSize[whichone] - x->x_gvoiceDone[whichone];

    if (done < x->x_gvoiceRamp[whichone])
        sample *= (done * x->x_gvoiceOneOverRamp[whichone]);
    else if (tail < x->x_gvoiceRamp[whichone])
        sample *= (tail * x->x_gvoiceOneOverRamp[whichone]);

    return sample;
}

//tries to find an available voice; return -1 if no voices available
static int munger_findVoice(t_disis_munger *x)
{
    int i = 0, foundOne = -1;
    while (foundOne < 0 && i < x->x_voices ) {
        if (!x->x_gvoiceOn[i])
            foundOne = i;
        i++;
    }
    return foundOne;
}

// creates a size for a new grain
// actual number of samples PLAYED, regardless of pitch
// might be shorter for higher pitches and long grains,
// to avoid collisions with recordCurrent
// size given now in milliseconds!
static t_float munger_newSize(t_disis_munger *x, int whichOne)
{
    float newsize, temp;
    int pitchChoice, pitchExponent;

    //set grain pitch
    if (x->x_smoothPitch == 1)
        x->x_gvoiceSpeed[whichOne] = x->x_gpitch + ((long)RANDOM() - 16384.) *
            ONE_OVER_HALFRAND * x->x_gpitch_var;
    else
    {
        //temp = (long)RANDOM() * ONE_OVER_MAXRAND * x->x_gpitch_var *
        //    (float)PITCHTABLESIZE;
        temp = (long)RANDOM() * ONE_OVER_MAXRAND * x->x_gpitch_var *
            (float)x->x_scale_len;
        pitchChoice = (int) temp;
        if (pitchChoice > PITCHTABLESIZE)
            pitchChoice = PITCHTABLESIZE;
        if (pitchChoice < 0)
            pitchChoice = 0;
        pitchExponent = (int)x->x_pitchTable[pitchChoice];
        x->x_gvoiceSpeed[whichOne] = x->x_gpitch *
            pow(x->x_semitone, pitchExponent);
    }

    if (x->x_gvoiceSpeed[whichOne] < MINSPEED)
        x->x_gvoiceSpeed[whichOne] = MINSPEED;
    newsize = x->x_srate_ms * (x->x_glen + ((long)RANDOM() - 16384.) *
        ONE_OVER_HALFRAND * x->x_glen_var);
    if (newsize > x->x_maxsize)
        newsize = x->x_maxsize;
    if (newsize * x->x_gvoiceSpeed[whichOne] > x->x_maxsize)
        newsize = x->x_maxsize / x->x_gvoiceSpeed[whichOne];
    if (newsize < x->x_minsize)
        newsize = x->x_minsize;
    return newsize;
}

// creates a new (RANDOM()) start position for a new grain,
// returns beginning start sample
// sets up size and direction
// max grain size is BUFLENGTH / 3, to avoid recording into grains
// while they are playing
static t_float munger_newSetup(t_disis_munger* x, int whichVoice)
{
    t_float newPosition;
    int i, tmpdiscretepan;

    x->x_gvoiceSize[whichVoice] = (long)munger_newSize(x, whichVoice);
    x->x_gvoiceDirection[whichVoice] = munger_newDirection(x);
    if (x->x_num_channels == 2)
    {
        x->x_gvoiceLPan[whichVoice] = ((long)(RANDOM()) - 16384.) *
            ONE_OVER_MAXRAND * x->x_gpan_spread + 0.5;
        x->x_gvoiceRPan[whichVoice] = 1. - x->x_gvoiceLPan[whichVoice];
        //make equal power panning....
        x->x_gvoiceLPan[whichVoice] = powf(x->x_gvoiceLPan[whichVoice], 0.5);
        x->x_gvoiceRPan[whichVoice] = powf(x->x_gvoiceRPan[whichVoice], 0.5);
    }
    else if (x->x_discretepan)
    {
        tmpdiscretepan = (int)((long)(RANDOM()) *
            ONE_OVER_MAXRAND * ((float)x->x_num_channels + 0.99));
        for (i = 0; i < x->x_num_channels; i++)
        {
            if (i == tmpdiscretepan)
                x->x_gvoiceSpat[whichVoice][i] = x->x_channelGain[i] +
                    ((long)(RANDOM()) - 16384.) *
                    ONE_OVER_HALFRAND * x->x_channelGainSpread[i];
            else x->x_gvoiceSpat[whichVoice][i] = 0.;
        }
    }
    else
    {
        for (i = 0; i < x->x_num_channels; i++)
            x->x_gvoiceSpat[whichVoice][i] = x->x_channelGain[i] +
                ((long)(RANDOM()) - 16384.) * ONE_OVER_HALFRAND *
                x->x_channelGainSpread[i];
    }
    x->x_gvoiceOn[whichVoice] = 1;
    x->x_gvoiceDone[whichVoice] = 0;
    x->x_gvoiceGain[whichVoice] = x->x_gain + ((long)(RANDOM()) - 16384.) *
        ONE_OVER_HALFRAND * x->x_randgain;
    x->x_gvoiceADSRon[whichVoice] = 0;

    if (x->x_gvoiceSize[whichVoice] < 2. * x->x_rampLength)
    {
        x->x_gvoiceRamp[whichVoice] = .5 * x->x_gvoiceSize[whichVoice];
        if (x->x_gvoiceRamp[whichVoice] <= 0.)
        {
            x->x_gvoiceRamp[whichVoice] = 1.;
        }
        x->x_gvoiceOneOverRamp[whichVoice] = 1. / x->x_gvoiceRamp[whichVoice];
    }
    else
    {
        x->x_gvoiceRamp[whichVoice] = x->x_rampLength;
        if (x->x_gvoiceRamp[whichVoice] <= 0.)
        {
            x->x_gvoiceRamp[whichVoice] = 1.;
        }
        x->x_gvoiceOneOverRamp[whichVoice] = 1. / x->x_gvoiceRamp[whichVoice];
    }

    /*** set start point; tricky, cause of moving buffer,
         variable playback rates, backwards/forwards, etc.... ***/

    if (!x->x_arraylength)
    {
	// 1. RANDOM() positioning and moving buffer (default)
	if (x->x_position == -1. && x->x_recordOn == 1)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) // going forward
            {
                if (x->x_gvoiceSpeed[whichVoice] > 1.)
                {
                    newPosition = x->x_recordCurrent - x->x_onethirdBufsize -
                        (long)RANDOM() * ONE_OVER_MAXRAND * x->x_onethirdBufsize;
                }
                else
                {
                    newPosition = x->x_recordCurrent - (long)RANDOM() *
                        ONE_OVER_MAXRAND * x->x_onethirdBufsize; //was 2/3rds
                }
            }
            else //going backwards
            {
                newPosition = x->x_recordCurrent - (long)RANDOM() *
                    ONE_OVER_MAXRAND * x->x_onethirdBufsize;
            }
	}

        // 2. fixed positioning and moving buffer
        else if (x->x_position >= 0. && x->x_recordOn == 1)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) // going forward
            {
                if (x->x_gvoiceSpeed[whichVoice] > 1.)
                {
                    //newPosition = x->x_recordCurrent -
                    //    x->x_onethirdBufsize - x->x_position *
                    //    x->x_onethirdBufsize;
                    //this will follow more closely...
                    newPosition = x->x_recordCurrent -
                        x->x_gvoiceSize[whichVoice] *
                        x->x_gvoiceSpeed[whichVoice] -
                        x->x_position * x->x_onethirdBufsize;
                }
                else
                {
                    newPosition = x->x_recordCurrent - x->x_position *
                        x->x_onethirdBufsize; //was 2/3rds
                }
            }
            else //going backwards
            {
                newPosition = x->x_recordCurrent - x->x_position *
                    x->x_onethirdBufsize;
            }
        }

        // 3. RANDOM() positioning and fixed buffer
        else if (x->x_position == -1. && x->x_recordOn == 0)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) // going forward
            {
                newPosition = x->x_recordCurrent - x->x_onethirdBufsize -
                    (long)RANDOM() * ONE_OVER_MAXRAND * x->x_onethirdBufsize;
            }
            else //going backwards
            {
                newPosition = x->x_recordCurrent - (long)RANDOM() *
                    ONE_OVER_MAXRAND * x->x_onethirdBufsize;
            }
	}

        // 4. fixed positioning and fixed buffer
        else if (x->x_position >= 0. && x->x_recordOn == 0)
        {
            if (x->x_gvoiceDirection[whichVoice] == 1) // going forward
            {
                newPosition = x->x_recordCurrent - x->x_onethirdBufsize -
                    x->x_position * x->x_onethirdBufsize;
            }
            else //going backwards
            {
                newPosition = x->x_recordCurrent - x->x_position *
                    x->x_onethirdBufsize;
            }
        }
    }
    else
    {
        if (x->x_position == -1.)
        {
            newPosition = (float)(RANDOM() * ONE_OVER_MAXRAND *
                (float)(x->x_arraylength));
        }
        else if (x->x_position >= 0.)
            newPosition = x->x_position * (float)x->x_arraylength;
    }
    return newPosition;
}

static void munger_clear(t_disis_munger *x)
{
    long i, len = (long)x->x_initbuflen;
    for (i = 0; i < len; i++)
        x->x_recordBuf[i] = 0;
}

static void munger_setramp(t_disis_munger *x, t_floatarg f)
{
    x->x_doHanning = 0;
    x->x_rampLength = f <= 0. ? 1. : f;
    if (x->x_verbose > 1)
        post("disis_munger~ %s: setting ramp to: %f ms",
            x->x_munger_name->s_name,
            (x->x_rampLength * x->x_one_over_srate_ms));
}

static void munger_scale(t_disis_munger *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    if (x->x_verbose > 1)
        post("disis_munger~ %s: loading scale from input list",
            x->x_munger_name->s_name);
    x->x_smoothPitch = 0;

    for (i = 0; i < PITCHTABLESIZE; i++)
        x->x_pitchTable[i] = 0.;
    if (argc > PITCHTABLESIZE)
        argc = PITCHTABLESIZE;
    for (i = 0; i < argc; i++)
    {
        x->x_pitchTable[i] = atom_getfloatarg(i, argc, argv);
    }
    x->x_scale_len = argc;

    i = 0;
    //wrap input list through all of pitchTable
    for (j = argc; j < PITCHTABLESIZE; j++) {
        x->x_pitchTable[j] = x->x_pitchTable[i++];
        if (i >= argc) i = 0;
    }
}

static void munger_bufsize(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
        t_float temp;
        if (argc)
        {
            temp = x->x_srate * atom_getfloatarg(0, argc, argv);
            if (temp < 3. * (float)MINSIZE)
            {
                temp = 3. * (float)MINSIZE;
                if (x->x_verbose > 0)
                    post("disis_munger~ %s error: delaylength too small!",
                        x->x_munger_name->s_name);
            }
            if (temp > x->x_initbuflen)
            {
                temp = x->x_initbuflen;
                if (x->x_verbose > 0)
                    post("disis_munger~ %s error: delaylength too large!",
                        x->x_munger_name->s_name);
            }
            x->x_maxsize = temp / 3.;
            x->x_twothirdBufsize = x->x_maxsize * 2.;
            x->x_onethirdBufsize = x->x_maxsize;
            if (x->x_verbose > 1)
                post("disis_munger~ %s: setting delaylength to: %f seconds",
                    x->x_munger_name->s_name, temp / x->x_srate);
        }
}

static void munger_bufsize_ms(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    t_float temp;
    if (argc)
    {
        temp = x->x_srate_ms * atom_getfloatarg(0, argc, argv);
        if (temp < 3. * (float)MINSIZE)
        {
            temp = 3. * (float)MINSIZE;
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: delaylength_ms too small!",
                    x->x_munger_name->s_name);
        }
        if (temp > x->x_initbuflen)
        {
            temp = x->x_initbuflen;
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: delaylength_ms too large!",
                    x->x_munger_name->s_name);
        }
        x->x_maxsize = temp / 3.;
        x->x_twothirdBufsize = x->x_maxsize * 2.;
        x->x_onethirdBufsize = x->x_maxsize;
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting delaylength to: %d milliseconds",
                x->x_munger_name->s_name, (long)(temp / x->x_srate_ms));
    }
}

static void munger_setminsize(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    t_float temp;

    if (argc)
    {
        temp = x->x_srate_ms * atom_getfloatarg(0, argc, argv);
        if (temp < (float)MINSIZE)
        {
            temp = (float)MINSIZE;
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: minsize too small!",
                    x->x_munger_name->s_name);
        }
        if (temp >= x->x_initbuflen)
        {
            temp = (float)MINSIZE;
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: minsize too large!",
                    x->x_munger_name->s_name);
        }
        x->x_minsize = temp;
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting min grain size to: %f ms",
                x->x_munger_name->s_name, (x->x_minsize / x->x_srate_ms));
    }
}

static void munger_discretepan(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    int temp;

    if (argc)
    {
        temp = (int)atom_getintarg(0, argc, argv);
        if (temp < 0)
        {
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: discretepan can be only 0 or 1!",
                    x->x_munger_name->s_name);
            temp = 0;
        }
        if (temp > 1)
        {
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: error: discretepan can be only 0 "
                     "or 1!", x->x_munger_name->s_name);
            temp = 1;
        }
        x->x_discretepan = temp;
    }
}

static void munger_setvoices(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    int temp;

    if (argc)
    {
        temp = atom_getintarg(0, argc, argv);
        if (temp < 0)
        {
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: voices has to be between "
                     "0 and maxvoices (currently %d)!",
                    x->x_munger_name->s_name, x->x_maxvoices);
            temp = 0;
        }
        if (temp > x->x_maxvoices)
        {
            if (x->x_verbose > 0)
            {
                post("disis_munger~ %s error: voices has to be between "
                     "0 and maxvoices (currently %d)!",
                    x->x_munger_name->s_name, x->x_maxvoices);
            }
            temp = x->x_maxvoices;
        }
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting voices to: %d ",
                x->x_munger_name->s_name, x->x_voices);
        x->x_voices = temp;
    }
}

static void munger_maxvoices(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    int temp;

    if (argc)
    {
        temp = (int)atom_getintarg(0, argc, argv);
        if (temp < 0)
        {
            if (x->x_verbose > 0)
                post("disis_munger~ %s error: maxvoices cannot be less than 0!",
                    x->x_munger_name->s_name);
            temp = 0;
        }
        if (temp > x->x_numvoices) temp = x->x_numvoices;
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting max voices to: %d ",
                x->x_munger_name->s_name, x->x_maxvoices);
        x->x_maxvoices = temp;
    }
}

static void munger_setpower(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (argc)
    {
        x->x_power = (int)atom_getintarg(0, argc, argv);
        post("disis_munger~ %s: setting power: %d",
            x->x_munger_name->s_name, x->x_power);
    }
}

static void munger_record(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    int temp;

    if (argc)
    {
        temp = (int)atom_getintarg(0, argc, argv);
        if (!temp)
        {
            x->x_recordRampVal = RECORDRAMP;
            x->x_rec_ramping = -1;
        }
        else
        {
            x->x_recordOn = 1;
            x->x_recordRampVal = 0;
            x->x_rec_ramping = 1;
        }
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting record: %d",
                x->x_munger_name->s_name, temp);
    }
}

static void munger_ambidirectional(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (argc)
    {
        x->x_ambi = atom_getfloatarg(0, argc, argv);
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting ambidirectional: %d",
                x->x_munger_name->s_name, x->x_ambi);
    }
}

static void munger_gain(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (argc)
    {
        x->x_gain = atom_getfloatarg(0, argc, argv);
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting gain to: %f ",
                x->x_munger_name->s_name, x->x_gain);
    }
}

static void munger_setposition(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    float temp;

    if (argc)
    {
        temp = atom_getfloatarg(0, argc, argv);
        if (temp > 1.) temp = 1.;
        if (temp < 0.) temp = -1.;
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting position to: %f ",
                x->x_munger_name->s_name, temp);
        x->x_position = temp;
    }
}

static void munger_randgain(t_disis_munger *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (argc)
    {
        x->x_randgain = atom_getfloatarg(0, argc, argv);
        if (x->x_verbose > 1)
            post("disis_munger~ %s: setting rand_gain to: %f ",
                x->x_munger_name->s_name, x->x_randgain);
    }
}

static void munger_sethanning(t_disis_munger *x)
{
    x->x_doHanning = 1;
    if (x->x_verbose > 1)
        post("disis_munger~ %s: hanning window is busted",
            x->x_munger_name->s_name);
}

static void munger_tempered(t_disis_munger *x)
{
    int i;
    if (x->x_verbose > 1)
        post("disis_munger~ %s: doing tempered scale",
            x->x_munger_name->s_name);
    x->x_smoothPitch = 0;
    x->x_scale_len = 100;
    for (i=0; i < x->x_scale_len - 1; i += 2)
    {
        x->x_pitchTable[i] = 0.5 * i;
        x->x_pitchTable[i+1] = -0.5 * i;
    }
    x->x_scale_len = PITCHTABLESIZE;
}

static void munger_smooth(t_disis_munger *x)
{
    x->x_smoothPitch = 1;
    if (x->x_verbose > 1)
        post("disis_munger~ %s: doing smooth scale", x->x_munger_name->s_name);
}

static void munger_poststate(t_disis_munger *x)
{
    post (">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    post ("***CURRENT disis_munger~ %s PARAMETER VALUES***:",
         x->x_munger_name->s_name);
    post ("all times in milliseconds");
    post ("version: %d.%d.%d", (int)MUNGER_MAJOR, (int)MUNGER_MINOR, (int)MUNGER_REV);
    post ("grain spacing = %f", x->x_grate);
    post ("grain spacing variation = %f", x->x_grate_var);
    post ("grain length = %f", x->x_glen);
    post ("grain length variation = %f", x->x_glen_var);
    post ("grain transposition multiplier = %f", x->x_gpitch);
    post ("grain transposition multiplier variation = %f", x->x_gpitch_var);
    post ("panning spread = %f", x->x_gpan_spread);
    post ("grain gain = %f", x->x_gain);
    post ("grain gain variation = %f", x->x_randgain);
    post ("playback position (-1 = RANDOM()) = %f", x->x_position);
    post ("grain playback direction (0 = both) = %d", x->x_ambi);
    post ("buffer length = %f", x->x_buflen * x->x_one_over_srate_ms);
    post ("max grain size = %f", x->x_maxsize * x->x_one_over_srate_ms);
    post ("min grain size = %f", x->x_minsize * x->x_one_over_srate_ms);
    post ("max number of voices = %ld", x->x_maxvoices);
    post ("current number of voices = %d", x->x_voices);
    post ("grain envelope (ramp) length = %f",
        x->x_rampLength * x->x_one_over_srate_ms);
    post ("recording? = %d", x->x_recordOn);
    post ("power = %d", x->x_power);
    post ("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}

//buffer funcs
static void munger_recordSamp(t_disis_munger *x, t_float sample)
{
    if (x->x_recordCurrent >= x->x_buflen) x->x_recordCurrent = 0;

    if (x->x_recordOn)
    {
        if (x->x_rec_ramping == 0)
            x->x_recordBuf[x->x_recordCurrent++] = sample; //add sample
        else //ramp up or down if turning on/off recording
        {
            x->x_recordBuf[x->x_recordCurrent++] =
                sample * RECORDRAMP_INV * (float)x->x_recordRampVal;
            x->x_recordRampVal += x->x_rec_ramping;
            if (x->x_recordRampVal <= 0)
            {
                x->x_rec_ramping = 0;
                x->x_recordOn = 0;
            }
            if (x->x_recordRampVal >= RECORDRAMP) x->x_rec_ramping = 0;
        }
    }
}

static t_float munger_getSamp(t_disis_munger *x, double where)
{
    double alpha, om_alpha, output;
    long first;

    while (where < 0.) where += x->x_buflen;
    while (where >= x->x_buflen) where -= x->x_buflen;

    first = (long)where;

    alpha = where - first;
    om_alpha = 1. - alpha;

    output = x->x_recordBuf[first++] * om_alpha;
    if (first <  x->x_buflen)
    {
        output += x->x_recordBuf[first] * alpha;
    }
    else
    {
        output += x->x_recordBuf[0] * alpha;
    }
    return (float)output;
}

static t_float munger_getExternalSamp(t_disis_munger *x, double where)
{
    double alpha, om_alpha, output;
    long first;

    t_float *tab;
    double frames, sampNum, nc;

    if (!x->x_arraylength)
    {
        post("disis_munger~ %s: error: external buffer mysteriously AWOL, "
             "reverting to internal buffer...", x->x_munger_name->s_name);
        return 0.;
    }

    tab = (t_float *)x->x_arrayvec;
    frames = (double)x->x_arraylength;
    /* Hm... isn't this always 1 for a garray? Setting it to that for now... */
    nc = (double)1; //== buffer~ framesize...

    if (where < 0.) where = 0.;
    else if (where >= frames) where = 0.;

    sampNum = where * nc;

    first = (long)sampNum;

    alpha = sampNum - first;
    om_alpha = 1. - alpha;

    output = tab[first] * om_alpha;
    first += (long)nc;
    output += tab[first] * alpha;

    return (t_float)output;
}

static t_int *munger_perform(t_int *w)
{
    /* Let's do it this way:
       (0: our function pointer)
       (1: our pd)
       2: insig
       3: n
       The outsigs will be pointed to by x->x_out
    */

    t_disis_munger *x = (t_disis_munger *)(w[1]);

// Hm... shouldn't we be using t_sample here as well as in x_out?
    t_float samp;
    int newvoice, i, j, n, sample_number;

    n = (int)(w[3]);
    /* ins1 holds the signal vector ofthe first inlet, index 0 */
    t_sample *ins1 = (t_sample *)(w[2]);

    /* pointer for incrementing the outlet samps */
    t_float **out = x->x_out;
    if (x->x_gpan_spread > 1.) x->x_gpan_spread = 1.;
    if (x->x_gpan_spread < 0.) x->x_gpan_spread = 0.;

    if (!x->x_power)
    {
        for (i = 0; i < x->x_num_channels; i++)
            for (j = 0; j < x->x_num_channels; j++)
                out[i][j] = 0.;
    }
    else
    {
        for (sample_number = 0; sample_number < n; sample_number++)
        {
            if (x->x_verbose > 2)
            {
                x->x_countsamples++;
                if (x->x_countsamples >= ((int)x->x_srate - 1))
                {
                    post("disis_munger~ %s: outputting %d grains per second",
                        x->x_munger_name->s_name, x->x_graincounter);
                    x->x_graincounter = 0;
                    x->x_countsamples = 0;
                }
            }
            for (i = 0; i < x->x_num_channels; i++) x->x_outsamp[i] = 0.;
            //record a sample
            munger_recordSamp(x, *ins1++);

            //grab a note if requested; works in oneshot mode or otherwise
            if (x->x_oneshot)
            {
                while (x->x_newnote > 0)
                {
                    newvoice = munger_findVoice(x);
                    if (newvoice >= 0)
                    {
                        x->x_gvoiceCurrent[newvoice] =
                            munger_newNote(x, newvoice, x->x_newnote);
                    }
                    x->x_newnote--;
                }
            }
            else
            {
                /* find a voice if it's time (high resolution).
                   ignore if in "oneshot" mode */
                if (x->x_time++ >= (long)x->x_gimme)
                {
                    x->x_time = 0;
                    newvoice = munger_findVoice(x);
                    if (newvoice >= 0)
                    {
                        x->x_gvoiceCurrent[newvoice] =
                            munger_newSetup(x, newvoice);
                    }
                    x->x_tempgrate = x->x_grate + ((long)RANDOM() - 16384.) *
                        ONE_OVER_HALFRAND * x->x_grate_var;
                    /* grate is actually time-distance between grains */
                    x->x_gimme = x->x_srate_ms * x->x_tempgrate;
                }
            }
            x->x_time++;
            //mix 'em, pan 'em
            for (i = 0; i < x->x_maxvoices; i++)
            {
                if (x->x_gvoiceOn[i])
                {
                    //get a sample, envelope it
                    if (x->x_arraylength)
                        samp = munger_getExternalSamp(x, x->x_gvoiceCurrent[i]);
                    else
                        samp = munger_getSamp(x, x->x_gvoiceCurrent[i]);
                    if (!x->x_gvoiceADSRon[i])
                        samp = munger_envelope(x, i, samp) * x->x_gvoiceGain[i];
                    else
                        samp = samp * stk_ADSR_tick(&x->x_gvoiceADSR[i]) *
                            x->x_gvoiceGain[i]; //ADSR_ADRtick->computeSample()
                    //pan it
                    if (x->x_num_channels == 2)
                    {
                        x->x_outsamp[0] += samp * x->x_gvoiceLPan[i];
                        x->x_outsamp[1] += samp * x->x_gvoiceRPan[i];
                    }
                    else // multichannel subroutine
                    {
                        for (j = 0; j < x->x_num_channels; j++)
                        {
                            x->x_outsamp[j] += samp * x->x_gvoiceSpat[i][j];
                        }
                    }

                    //see if grain is done after jumping to next sample point
                    x->x_gvoiceCurrent[i] += (double)x->x_gvoiceDirection[i] *
                        (double)x->x_gvoiceSpeed[i];
                    if (!x->x_gvoiceADSRon[i])
                    {
                        if (x->x_gvoiceDone[i] >= x->x_gvoiceSize[i] ||
                            ++x->x_gvoiceDone[i] >= x->x_gvoiceSize[i])
                        {
                            x->x_gvoiceOn[i] = 0;
                        }
                    }
                    else
                    {
                        if (stk_ADSR_getState(&x->x_gvoiceADSR[i]) == SUSTAIN)
                        {
                            stk_ADSR_keyOff(&x->x_gvoiceADSR[i]);
                        }
                        if (++x->x_gvoiceDone[i] >= x->x_gvoiceSize[i] ||
                            stk_ADSR_getState(&x->x_gvoiceADSR[i]) == IDLE)
                        {
                            x->x_gvoiceOn[i] = 0;
                        }
                        if (!x->x_gvoiceOn[i])
                            x->x_graincounter++;
                    }
                }
            }
            for (i = 0; i < x->x_num_channels; i++)
            {
                out[i][sample_number] = x->x_outsamp[i];
            }
        }
    }
    return (w+4);
}

static void munger_dsp(t_disis_munger *x, t_signal **sp)
{
    t_float old_srate;

    // Go ahead and set the array based on the array name.
    pd_vmess(&x->x_obj.te_pd, gensym("buffer"), "s", x->x_arrayname);

    old_srate = x->x_srate;
    x->x_srate = sys_getsr();
    if (x->x_srate != old_srate)
    {
        x->x_one_over_srate = 1. / x->x_srate;
        x->x_srate_ms = x->x_srate / 1000;
        x->x_one_over_srate_ms = 1. / x->x_srate_ms;
    }
    /* Let's do it this way:
       (0: our function pointer)
       (1: our pd)
       2: insig
       3: n
       The outsigs will be pointed to by x->x_out
    */
    int i;
    for (i = 0; i < x->x_num_channels; i++)
    {
        x->x_out[i] = (t_float *)(sp[i + 1]->s_vec);
    }
    dsp_add(munger_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

void disis_munger_tilde_setup(void)
{
    disis_munger_class = class_new(gensym("disis_munger~"),
        (t_newmethod)munger_new, (t_method)munger_free,
        sizeof(t_disis_munger),
        CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(disis_munger_class, t_disis_munger, x_f);
    class_addmethod(disis_munger_class, (t_method)munger_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(disis_munger_class, (t_method)munger_setverbose,
        gensym("verbose"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_setramp,
        gensym("ramptime"), A_DEFFLOAT, 0);
    class_addmethod(disis_munger_class, (t_method)munger_scale,
        gensym("scale"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_bufsize,
        gensym("delaylength"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_bufsize_ms,
        gensym("delaylength_ms"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_setminsize,
        gensym("minsize"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_setpower,
        gensym("power"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_record,
        gensym("record"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_ambidirectional,
        gensym("ambidirectional"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_smooth,
        gensym("smooth"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_tempered,
        gensym("tempered"), 0);
    class_addmethod(disis_munger_class, (t_method)munger_sethanning,
        gensym("hanning"), 0);
    class_addmethod(disis_munger_class, (t_method)munger_randgain,
        gensym("rand_gain"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_setposition,
        gensym("position"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_gain,
        gensym("gain"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_setvoices,
        gensym("voices"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_maxvoices,
        gensym("maxvoices"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_clear,
        gensym("clear"), 0);
    class_addmethod(disis_munger_class, (t_method)munger_poststate,
        gensym("state"), 0);
    class_addmethod(disis_munger_class, (t_method)munger_setbuffer,
        gensym("buffer"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_spat,
        gensym("spatialize"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_note,
        gensym("note"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_oneshot,
        gensym("oneshot"), A_GIMME, 0);
    class_addmethod(disis_munger_class, (t_method)munger_discretepan,
        gensym("discretepan"), A_GIMME, 0);
}
