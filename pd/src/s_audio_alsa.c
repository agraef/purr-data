/* Copyright (c) 1997-2003 Guenter Geiger, Miller Puckette, Larry Troxler,
* Winfried Ritsch, Karl MacMillan, and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* this file inputs and outputs audio using the ALSA API available on linux. */

/* support for ALSA pcmv2 api by Karl MacMillan<karlmac@peabody.jhu.edu> */ 
/* support for ALSA MMAP noninterleaved by Winfried Ritsch, IEM */

#include <alsa/asoundlib.h>

#include "m_pd.h"
#include "s_stuff.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include "s_audio_alsa.h"
#include <endian.h>

/* Defines */
#define DEBUG(x) x
#define DEBUG2(x) {x;}

/* needed for alsa 0.9 compatibility: */
#if (SND_LIB_MAJOR < 1)
#define ALSAAPI9
#endif

static void alsa_checkiosync( void);
static void alsa_numbertoname(int iodev, char *devname, int nchar);
static int alsa_jittermax;
#define ALSA_DEFJITTERMAX 3

    /* don't assume we can turn all 31 bits when doing float-to-fix; 
    otherwise some audio drivers (e.g. Midiman/ALSA) wrap around. */
#define FMAX 0x7ffff000
#define CLIP32(x) (((x)>FMAX)?FMAX:((x) < -FMAX)?-FMAX:(x))

static char *alsa_snd_buf;
static int alsa_snd_bufsize;
static int alsa_buf_samps;
static snd_pcm_status_t *alsa_status; 
static int alsa_usemmap;

t_alsa_dev alsa_indev[ALSA_MAXDEV];
t_alsa_dev alsa_outdev[ALSA_MAXDEV];
int alsa_nindev;
int alsa_noutdev;

static void check_error(int err, const char *why)
{
    if (err < 0)
        fprintf(stderr, "%s: %s\n", why, snd_strerror(err));
}

static int alsaio_canmmap(t_alsa_dev *dev)
{
    snd_pcm_hw_params_t *hw_params;
    int err1, err2;
    
    snd_pcm_hw_params_alloca(&hw_params);

    err1 = snd_pcm_hw_params_any(dev->a_handle, hw_params);
    if (err1 < 0) {
      check_error(err1,"Broken configuration: no configurations available"); 
      return (0);
    }
    err1 = snd_pcm_hw_params_set_access(dev->a_handle,
        hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err1 < 0)
    {
        err2 = snd_pcm_hw_params_set_access(dev->a_handle,
            hw_params, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
    }
    else err2 = -1;
#if 0
    post("err 1 %d (%s), err2 %d (%s)", err1, snd_strerror(err1),
         err2, snd_strerror(err2));
#endif
    return ((err1 < 0) && (err2 >= 0));
}

static int alsaio_setup(t_alsa_dev *dev, int out, int *channels, int *rate,
    int nfrags, int frag_size)
{
    int bufsizeforthis, err;
    snd_pcm_hw_params_t* hw_params;
    unsigned int tmp_uint;
    snd_pcm_uframes_t tmp_snd_pcm_uframes;

    if (sys_verbose)
    {
        if (out)
            post("configuring sound output...");
        else post("configuring sound input...");
    }

        /* set hardware parameters... */
    snd_pcm_hw_params_alloca(&hw_params);

        /* get the default params */
    err = snd_pcm_hw_params_any(dev->a_handle, hw_params);
    check_error(err, "snd_pcm_hw_params_any");

        /* try to set interleaved access */
    err = snd_pcm_hw_params_set_access(dev->a_handle,
        hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0)
        return (-1);
    check_error(err, "snd_pcm_hw_params_set_access");
#if 0       /* enable this to print out which formats are available */
    {
        int i;
        for (i = 0; i <= SND_PCM_FORMAT_LAST; i++)
            fprintf(stderr, "%d -> %d\n",
                i, snd_pcm_hw_params_test_format(dev->a_handle, hw_params, i));
    }
#endif
        /* Try to set 32 bit format first */
    err = snd_pcm_hw_params_set_format(dev->a_handle,
        hw_params, SND_PCM_FORMAT_S32);
    if (err < 0)
    {
        /* fprintf(stderr,
            "PD-ALSA: 32 bit format not available - trying 24\n"); */
        err = snd_pcm_hw_params_set_format(dev->a_handle, hw_params,
            SND_PCM_FORMAT_S24_3LE);
        if (err < 0)
        {
            /* fprintf(stderr,
                "PD-ALSA: 32/24 bit format not available - using 16\n"); */
            err = snd_pcm_hw_params_set_format(dev->a_handle, hw_params,
                SND_PCM_FORMAT_S16);
            check_error(err, "snd_pcm_hw_params_set_format");
            dev->a_sampwidth = 2;
        }
        else dev->a_sampwidth = 3;
    }
    else dev->a_sampwidth = 4;

    if (sys_verbose)
        post("Sample width set to %d bytes", dev->a_sampwidth);

        /* set the subformat */
    err = snd_pcm_hw_params_set_subformat(dev->a_handle,
        hw_params, SND_PCM_SUBFORMAT_STD);
    check_error(err, "snd_pcm_hw_params_set_subformat");
    
        /* set the number of channels */
    tmp_uint = *channels;
    err = snd_pcm_hw_params_set_channels_min(dev->a_handle,
        hw_params, &tmp_uint);
    check_error(err, "snd_pcm_hw_params_set_channels");
    if (tmp_uint != (unsigned)*channels)
        post("ALSA: set %s channels to %d", (out?"output":"input"), tmp_uint);
    *channels = tmp_uint;
    dev->a_channels = *channels;

        /* set the sampling rate */
    err = snd_pcm_hw_params_set_rate_min(dev->a_handle, hw_params, 
        (unsigned int *)rate, 0);
    check_error(err, "snd_pcm_hw_params_set_rate_min");
#if 0
    err = snd_pcm_hw_params_get_rate(hw_params, &subunitdir);
    post("input sample rate %d", err);
#endif

    /* post("frag size %d, nfrags %d", frag_size, nfrags); */
        /* set "period size" */
#ifdef ALSAAPI9
    err = snd_pcm_hw_params_set_period_size_near(dev->a_handle,
        hw_params, (snd_pcm_uframes_t)frag_size, 0);
#else
    tmp_snd_pcm_uframes = frag_size;
    err = snd_pcm_hw_params_set_period_size_near(dev->a_handle,
        hw_params, &tmp_snd_pcm_uframes, 0);
#endif
    check_error(err, "snd_pcm_hw_params_set_period_size_near");

        /* set the buffer size */
#ifdef ALSAAPI9
    err = snd_pcm_hw_params_set_buffer_size_near(dev->a_handle,
        hw_params, nfrags * frag_size);
#else
    tmp_snd_pcm_uframes = nfrags * frag_size;
    err = snd_pcm_hw_params_set_buffer_size_near(dev->a_handle,
        hw_params, &tmp_snd_pcm_uframes);
#endif
    check_error(err, "snd_pcm_hw_params_set_buffer_size_near");

    err = snd_pcm_hw_params(dev->a_handle, hw_params);
    check_error(err, "snd_pcm_hw_params");

        /* set up the buffer */
    bufsizeforthis = DEFDACBLKSIZE * dev->a_sampwidth * *channels;
    if (alsa_snd_buf)
    {
        if (alsa_snd_bufsize < bufsizeforthis)
        {
            if (!(alsa_snd_buf = realloc(alsa_snd_buf, bufsizeforthis)))
            {
                post("out of memory");
                return (0);
            }
            memset(alsa_snd_buf, 0, bufsizeforthis);
            alsa_snd_bufsize = bufsizeforthis;
        }
    }
    else
    {
        if (!(alsa_snd_buf = (void *)malloc(bufsizeforthis)))
        {
            post("out of memory");
            return (0);
        }
        memset(alsa_snd_buf, 0, bufsizeforthis);
        alsa_snd_bufsize = bufsizeforthis;
    }
    return (1);
}


    /* return 0 on success */
int alsa_open_audio(int naudioindev, int *audioindev, int nchindev,
    int *chindev, int naudiooutdev, int *audiooutdev, int nchoutdev,
    int *choutdev, int rate, int blocksize)
{
    int err, inchans = 0, outchans = 0;
    char devname[512];
    int frag_size = (blocksize ? blocksize : ALSA_DEFFRAGSIZE);
    int nfrags, i, iodev, dev2;

    nfrags = sys_schedadvance * (float)rate / (1e6 * frag_size);
        /* save our belief as to ALSA's buffer size for later */
    alsa_buf_samps = nfrags * frag_size;
    alsa_nindev = alsa_noutdev = 0;
    alsa_jittermax = ALSA_DEFJITTERMAX;

    if (sys_verbose)
        post("audio buffer set to %d", (int)(0.001 * sys_schedadvance));

    for (iodev = 0; iodev < naudioindev; iodev++)
    {
        alsa_numbertoname(audioindev[iodev], devname, 512);
        err = snd_pcm_open(&alsa_indev[alsa_nindev].a_handle, devname,
            SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
        check_error(err, "snd_pcm_open (input)");
        if (err < 0)
            continue;
        alsa_indev[alsa_nindev].a_devno = audioindev[iodev];
        snd_pcm_nonblock(alsa_indev[alsa_nindev].a_handle, 1);
        if (sys_verbose)
            post("opened input device name %s", devname);
        alsa_nindev++;
    }
    for (iodev = 0; iodev < naudiooutdev; iodev++)
    {
        alsa_numbertoname(audiooutdev[iodev], devname, 512);
        err = snd_pcm_open(&alsa_outdev[alsa_noutdev].a_handle, devname,
            SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
        check_error(err, "snd_pcm_open (output)");
        if (err < 0)
            continue;
        alsa_outdev[alsa_noutdev].a_devno = audiooutdev[iodev];
        snd_pcm_nonblock(alsa_outdev[alsa_noutdev].a_handle, 1);
        alsa_noutdev++;
    }
    if (!alsa_nindev && !alsa_noutdev)
        goto blewit;

        /* If all the open devices support mmap_noninterleaved, let's call
        Wini's code in s_audio_alsamm.c */
    alsa_usemmap = 1;
    for (iodev = 0; iodev < alsa_nindev; iodev++)
        if (!alsaio_canmmap(&alsa_indev[iodev]))
            alsa_usemmap = 0;
    for (iodev = 0; iodev < alsa_noutdev; iodev++)
        if (!alsaio_canmmap(&alsa_outdev[iodev]))
            alsa_usemmap = 0;
    if (alsa_usemmap)
    {
        post("using mmap audio interface");
        if (alsamm_open_audio(rate, blocksize))
            goto blewit;
        else return (0);
    }
    for (iodev = 0; iodev < alsa_nindev; iodev++)
    {
        int channels = chindev[iodev];
        if (alsaio_setup(&alsa_indev[iodev], 0, &channels, &rate,
            nfrags, frag_size) < 0)
                goto blewit;
        inchans += channels;
    }
    for (iodev = 0; iodev < alsa_noutdev; iodev++)
    {
        int channels = choutdev[iodev];
        if (alsaio_setup(&alsa_outdev[iodev], 1, &channels, &rate,
            nfrags, frag_size) < 0)
                goto blewit;
        outchans += channels;
    }
    if (!inchans && !outchans)
        goto blewit;

    for (iodev = 0; iodev < alsa_nindev; iodev++)
        snd_pcm_prepare(alsa_indev[iodev].a_handle);
    for (iodev = 0; iodev < alsa_noutdev; iodev++)
        snd_pcm_prepare(alsa_outdev[iodev].a_handle);

        /* if duplex we can link the channels so they start together */
    for (iodev = 0; iodev < alsa_nindev; iodev++)
        for (dev2 = 0; dev2 < alsa_noutdev; dev2++)
    {
        if (alsa_indev[iodev].a_devno == alsa_outdev[iodev].a_devno)
        {
            snd_pcm_link(alsa_indev[iodev].a_handle,
                alsa_outdev[iodev].a_handle);
        }
    }

        /* allocate the status variables */
    if (!alsa_status)
    {
        err = snd_pcm_status_malloc(&alsa_status);
        check_error(err, "snd_pcm_status_malloc");
    }

        /* fill the buffer with silence and prime the output FIFOs.  This
        should automatically start the output devices. */
    memset(alsa_snd_buf, 0, alsa_snd_bufsize);

    if (outchans)
    {
        i = (frag_size * nfrags)/DEFDACBLKSIZE + 1;
        while (i--)
        {
            for (iodev = 0; iodev < alsa_noutdev; iodev++)
                snd_pcm_writei(alsa_outdev[iodev].a_handle, alsa_snd_buf,
                    DEFDACBLKSIZE);
        }
    }
    if (inchans)
    {
            /* some of the ADC devices might already have been started by
            starting the outputs above, but others might still need it. */
        for (iodev = 0; iodev < alsa_nindev; iodev++)
            if (snd_pcm_state(alsa_indev[iodev].a_handle)
                != SND_PCM_STATE_RUNNING)
                    if ((err = snd_pcm_start(alsa_indev[iodev].a_handle)) < 0)
                        check_error(err, "input start failed");
    }
    return (0);
blewit:
    sys_inchannels = 0;
    sys_outchannels = 0;
    alsa_close_audio();
    return (1);
}

void alsa_close_audio(void)
{
    int err, iodev;
    if (alsa_usemmap)
    {
        alsamm_close_audio();
        return;
    }
    for (iodev = 0; iodev < alsa_nindev; iodev++)
    {
        err = snd_pcm_close(alsa_indev[iodev].a_handle);
        check_error(err, "snd_pcm_close (input)");
    }
    for (iodev = 0; iodev < alsa_noutdev; iodev++)
    {
        err = snd_pcm_close(alsa_outdev[iodev].a_handle);
        check_error(err, "snd_pcm_close (output)");
    }
    alsa_nindev = alsa_noutdev = 0;
}

int alsa_send_dacs(void)
{
    static double timenow;
#ifdef DEBUG_ALSA_XFER
    static int xferno = 0;
    static int callno = 0;
    double timelast = timenow;
#endif
    t_sample *fp1, *fp2;
    int i, j, k, iodev, result, ch; 
    int chansintogo, chansouttogo;
    unsigned int transfersize;

    if (alsa_usemmap)
        return (alsamm_send_dacs());

    if (!alsa_nindev && !alsa_noutdev)
        return (SENDDACS_NO);

    chansintogo = sys_inchannels;
    chansouttogo = sys_outchannels;
    transfersize = DEFDACBLKSIZE;

    timenow = sys_getrealtime();

#ifdef DEBUG_ALSA_XFER
    if (timenow - timelast > 0.050)
        fprintf(stderr, "(%d)",
            (int)(1000 * (timenow - timelast))), fflush(stderr);
    callno++;
#endif

    if (alsa_nindev > 0 && alsa_noutdev > 0)
        alsa_checkiosync();     /* check I/O are in sync and data not late */

    for (iodev = 0; iodev < alsa_nindev; iodev++)
    {
        snd_pcm_status(alsa_indev[iodev].a_handle, alsa_status);
        if (snd_pcm_status_get_avail(alsa_status) < transfersize)
            return SENDDACS_NO;
    }
    for (iodev = 0; iodev < alsa_noutdev; iodev++)
    {
        snd_pcm_status(alsa_outdev[iodev].a_handle, alsa_status);
        if (snd_pcm_status_get_avail(alsa_status) < transfersize)
            return SENDDACS_NO;
    }
    /* do output */
    for (iodev = 0, fp1 = sys_soundout, ch = 0; iodev < alsa_noutdev; iodev++)
    {
        int thisdevchans = alsa_outdev[iodev].a_channels;
        int chans = (chansouttogo < thisdevchans ? chansouttogo : thisdevchans);
        chansouttogo -= chans;

        if (alsa_outdev[iodev].a_sampwidth == 4)
        {
            for (i = 0; i < chans; i++, ch++, fp1 += DEFDACBLKSIZE)
                for (j = i, k = DEFDACBLKSIZE, fp2 = fp1; k--;
                     j += thisdevchans, fp2++)
            {
                t_sample s1 = *fp2 * INT32_MAX;
                ((t_alsa_sample32 *)alsa_snd_buf)[j] = CLIP32(s1);
            } 
            for (; i < thisdevchans; i++, ch++)
                for (j = i, k = DEFDACBLKSIZE; k--; j += thisdevchans)
                    ((t_alsa_sample32 *)alsa_snd_buf)[j] = 0;
        }
        else if (alsa_outdev[iodev].a_sampwidth == 3)
        {
            for (i = 0; i < chans; i++, ch++, fp1 += DEFDACBLKSIZE)
                for (j = i, k = DEFDACBLKSIZE, fp2 = fp1; k--;
                     j += thisdevchans, fp2++)
            {
                int s = *fp2 * 8388352.;
                if (s > 8388351)
                    s = 8388351;
                else if (s < -8388351)
                    s = -8388351;
#if BYTE_ORDER == LITTLE_ENDIAN                                             
                ((char *)(alsa_snd_buf))[3*j] = (s & 255);
                ((char *)(alsa_snd_buf))[3*j+1] = ((s>>8) & 255);
                ((char *)(alsa_snd_buf))[3*j+2] = ((s>>16) & 255);
#else
                fprintf(stderr("big endian 24-bit not supported");
#endif
            }
            for (; i < thisdevchans; i++, ch++)
                for (j = i, k = DEFDACBLKSIZE; k--; j += thisdevchans)
                    ((char *)(alsa_snd_buf))[3*j] = 
                    ((char *)(alsa_snd_buf))[3*j+1] = 
                    ((char *)(alsa_snd_buf))[3*j+2] = 0;
        }
        else        /* 16 bit samples */
        {
            for (i = 0; i < chans; i++, ch++, fp1 += DEFDACBLKSIZE)
                for (j = ch, k = DEFDACBLKSIZE, fp2 = fp1; k--;
                     j += thisdevchans, fp2++)
            {
                int s = *fp2 * 32767.;
                if (s > 32767)
                    s = 32767;
                else if (s < -32767)
                    s = -32767;
                ((t_alsa_sample16 *)alsa_snd_buf)[j] = s;
            }
            for (; i < thisdevchans; i++, ch++)
                for (j = ch, k = DEFDACBLKSIZE; k--; j += thisdevchans)
                    ((t_alsa_sample16 *)alsa_snd_buf)[j] = 0;
        }
        result = snd_pcm_writei(alsa_outdev[iodev].a_handle, alsa_snd_buf,
            transfersize);

        if (result != (int)transfersize)
        {
    #ifdef DEBUG_ALSA_XFER
            if (result >= 0 || errno == EAGAIN)
                fprintf(stderr, "ALSA: write returned %d of %d\n",
                        result, transfersize);
            else fprintf(stderr, "ALSA: write: %s\n",
                         snd_strerror(errno));
            fprintf(stderr,
                    "inputcount %d, outputcount %d, outbufsize %d\n",
                    inputcount, outputcount, 
                    (ALSA_EXTRABUFFER + sys_advance_samples)
                    * alsa_outdev[iodev].a_sampwidth * outchannels);
    #endif
            sys_log_error(ERR_DACSLEPT);
            return (SENDDACS_NO);
        }

        /* zero out the output buffer */
        memset(sys_soundout, 0, DEFDACBLKSIZE * sizeof(*sys_soundout) *
               sys_outchannels);
        if (sys_getrealtime() - timenow > 0.002)
        {
    #ifdef DEBUG_ALSA_XFER
            fprintf(stderr, "output %d took %d msec\n",
                    callno, (int)(1000 * (timenow - timelast))), fflush(stderr);
    #endif
            timenow = sys_getrealtime();
            sys_log_error(ERR_DACSLEPT);
        }
    }

            /* do input */
    for (iodev = 0, fp1 = sys_soundin, ch = 0; iodev < alsa_nindev; iodev++)
    {
        int thisdevchans = alsa_indev[iodev].a_channels;
        int chans = (chansintogo < thisdevchans ? chansintogo : thisdevchans);
        chansouttogo -= chans;
        result = snd_pcm_readi(alsa_indev[iodev].a_handle, alsa_snd_buf,
            transfersize);
        if (result < (int)transfersize)
        {
#ifdef DEBUG_ALSA_XFER
            if (result < 0)
                fprintf(stderr,
                        "snd_pcm_read %d %d: %s\n", 
                        callno, xferno, snd_strerror(errno));
            else fprintf(stderr,
                         "snd_pcm_read %d %d returned only %d\n", 
                         callno, xferno, result);
            fprintf(stderr,
                    "inputcount %d, outputcount %d, inbufsize %d\n",
                    inputcount, outputcount, 
                    (ALSA_EXTRABUFFER + sys_advance_samples)
                    * alsa_indev[iodev].a_sampwidth * inchannels);
#endif
            sys_log_error(ERR_ADCSLEPT);
            return (SENDDACS_NO);
        }
        if (alsa_indev[iodev].a_sampwidth == 4)
        {
            for (i = 0; i < chans; i++, ch++, fp1 += DEFDACBLKSIZE)
            {
                for (j = ch, k = DEFDACBLKSIZE, fp2 = fp1; k--;
                     j += thisdevchans, fp2++)
                    *fp2 = (t_sample) ((t_alsa_sample32 *)alsa_snd_buf)[j]
                        * (1./ INT32_MAX);
            }
        }
        else if (alsa_indev[iodev].a_sampwidth == 3)
        {
#if BYTE_ORDER == LITTLE_ENDIAN                                             
            for (i = 0; i < chans; i++, ch++, fp1 += DEFDACBLKSIZE)
            {
                for (j = ch, k = DEFDACBLKSIZE, fp2 = fp1; k--;
                     j += thisdevchans, fp2++)
                    *fp2 = ((t_sample) (
                        (((unsigned char *)alsa_snd_buf)[3*j] << 8)
                        | (((unsigned char *)alsa_snd_buf)[3*j+1] << 16)
                        | (((unsigned char *)alsa_snd_buf)[3*j+2] << 24)))
                        * (1./ INT32_MAX);
            }
#else
                fprintf(stderr("big endian 24-bit not supported");
#endif
        }
        else
        {
            for (i = 0; i < chans; i++, ch++, fp1 += DEFDACBLKSIZE)
            {
                for (j = ch, k = DEFDACBLKSIZE, fp2 = fp1; k--;
                    j += thisdevchans, fp2++)
                        *fp2 = (t_sample) ((t_alsa_sample16 *)alsa_snd_buf)[j]
                            * 3.051850e-05;
            }
        }
    }
#ifdef DEBUG_ALSA_XFER
    xferno++;
#endif
    if (sys_getrealtime() - timenow > 0.002)
    {
#ifdef DEBUG_ALSA_XFER
        fprintf(stderr, "routine took %d msec\n",
            (int)(1000 * (sys_getrealtime() - timenow)));
#endif
        sys_log_error(ERR_ADCSLEPT);
    }
    return SENDDACS_YES;
}

void alsa_printstate( void)
{
    int result, iodev = 0;
    snd_pcm_sframes_t indelay = 0, outdelay = 0;
    if (sys_audioapi != API_ALSA)
    {
        error("restart-audio: implemented for ALSA only.");
        return;
    }
    if (sys_inchannels)
    {
        result = snd_pcm_delay(alsa_indev[iodev].a_handle, &indelay);
        if (result < 0)
            post("snd_pcm_delay 1 failed");
        else post("in delay %d", indelay);
    }
    if (sys_outchannels)
    {
        result = snd_pcm_delay(alsa_outdev[iodev].a_handle, &outdelay);
        if (result < 0)
            post("snd_pcm_delay 2 failed");
        else post("out delay %d", outdelay);
    }
    post("sum %d (%d mod 64)\n", indelay + outdelay, (indelay+outdelay)%64);

    post("buf samples %d", alsa_buf_samps);
}


void alsa_resync( void)
{
    int i, result, iodev = 0;
    if (sys_audioapi != API_ALSA)
    {
        error("restart-audio: implemented for ALSA only.");
        return;
    }
    memset(alsa_snd_buf, 0,
        alsa_indev[iodev].a_sampwidth *
            DEFDACBLKSIZE * sys_outchannels);
    for (i = 0; i < 1000000; i++)
    {
        result = snd_pcm_writei(alsa_outdev[iodev].a_handle, alsa_snd_buf,
            DEFDACBLKSIZE);
        if (result != (int)DEFDACBLKSIZE)
            break;
    }
    post("%d written", i);
}

void alsa_putzeros(int iodev, int n)
{
    int i;
    memset(alsa_snd_buf, 0,
        alsa_outdev[iodev].a_sampwidth * DEFDACBLKSIZE *
            alsa_outdev[iodev].a_channels);
    for (i = 0; i < n; i++)
    {
        snd_pcm_writei(alsa_outdev[iodev].a_handle, alsa_snd_buf,
            DEFDACBLKSIZE);
    }
    /* post ("putzeros %d", n); */
}

void alsa_getzeros(int iodev, int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        snd_pcm_readi(alsa_indev[iodev].a_handle, alsa_snd_buf,
            DEFDACBLKSIZE);
    }
    /* post ("getzeros %d", n); */
}

    /* call this only if both input and output are open */
static void alsa_checkiosync( void)
{
    int result, giveup = 1000, alreadylogged = 0, iodev = 0;
    snd_pcm_sframes_t minphase, maxphase, thisphase, outdelay;

    while (1)
    {
        if (giveup-- <= 0)
        {
            post("tried but couldn't sync A/D/A");
            alsa_jittermax += 1;
            return;
        }
        minphase = 0x7fffffff;
        maxphase = -0x7fffffff;
        for (iodev = 0; iodev < alsa_noutdev; iodev++)
        {
            result = snd_pcm_delay(alsa_outdev[iodev].a_handle, &outdelay);
            if (result < 0)
            {
                snd_pcm_prepare(alsa_outdev[iodev].a_handle);
                result = snd_pcm_delay(alsa_outdev[iodev].a_handle, &outdelay);
            }
            if (result < 0)
            {
                post("output snd_pcm_delay failed: %s", snd_strerror(result));
                if (snd_pcm_status(alsa_outdev[iodev].a_handle,
                    alsa_status) < 0)
                        post("output snd_pcm_status failed");
                else post("astate %d",
                     snd_pcm_status_get_state(alsa_status));
                return;
            }
            thisphase = alsa_buf_samps - outdelay;
            if (thisphase < minphase)
                minphase = thisphase;
            if (thisphase > maxphase)
                maxphase = thisphase;
            if (outdelay < 0)
                sys_log_error(ERR_DATALATE), alreadylogged = 1;
        }
        for (iodev = 0; iodev < alsa_nindev; iodev++)
        {
            result = snd_pcm_delay(alsa_indev[iodev].a_handle, &thisphase);
            if (result < 0)
            {
                snd_pcm_prepare(alsa_indev[iodev].a_handle);
                result = snd_pcm_delay(alsa_indev[iodev].a_handle, &thisphase);
            }
            if (result < 0)
            {
                post("input snd_pcm_delay failed: %s", snd_strerror(result));
                if (snd_pcm_status(alsa_indev[iodev].a_handle,
                    alsa_status) < 0)
                    post("input snd_pcm_status failed");
                else post("astate %d",
                     snd_pcm_status_get_state(alsa_status));
                return;
            }
            if (thisphase < minphase)
                minphase = thisphase;
            if (thisphase > maxphase)
                maxphase = thisphase;
        }
            /* the "correct" position is for all the phases to be exactly
            equal; but since we only make corrections DEFDACBLKSIZE samples
            at a time, we just ask that the spread be not more than 3/4
            of a block.  */
        if (maxphase <= minphase + (alsa_jittermax * (DEFDACBLKSIZE / 4)))
                break;

        if (!alreadylogged)
            sys_log_error(ERR_RESYNC), alreadylogged = 1;
        for (iodev = 0; iodev < alsa_noutdev; iodev++)
        {
            result = snd_pcm_delay(alsa_outdev[iodev].a_handle, &outdelay);
            if (result < 0)
                break;
            thisphase = alsa_buf_samps - outdelay;
            if (thisphase > minphase + DEFDACBLKSIZE)
            {
                alsa_putzeros(iodev, 1);
#if DEBUGSYNC
                fprintf(stderr, "putz %d %d\n", (int)thisphase, (int)minphase);
#endif
            }
        }
        for (iodev = 0; iodev < alsa_nindev; iodev++)
        {
            result = snd_pcm_delay(alsa_indev[iodev].a_handle, &thisphase);
            if (result < 0)
                break;
            if (thisphase > minphase + DEFDACBLKSIZE)
            {
                alsa_getzeros(iodev, 1);
#if DEBUGSYNC
                fprintf(stderr, "getz %d %d\n", (int)thisphase, (int)minphase);
#endif
            }
        }
    }
#if DEBUGSYNC
    if (alreadylogged)
        fprintf(stderr, "done\n");
#endif
}

static int alsa_nnames = 0;
static char **alsa_names = 0;

void alsa_adddev(char *name)
{
    if (alsa_nnames)
        alsa_names = (char **)t_resizebytes(alsa_names,
            alsa_nnames * sizeof(char *),
            (alsa_nnames+1) * sizeof(char *));
    else alsa_names = (char **)t_getbytes(sizeof(char *));
    alsa_names[alsa_nnames] = gensym(name)->s_name;
    alsa_nnames++;
}

static void alsa_numbertoname(int devno, char *devname, int nchar)
{
    int ndev = 0, cardno = -1;
    while (!snd_card_next(&cardno) && cardno >= 0)
        ndev++;
    if (devno < 2*ndev)
    {
        if (devno & 1)
            snprintf(devname, nchar, "plughw:%d", devno/2);
        else snprintf(devname, nchar, "hw:%d", devno/2);
    }
    else if (devno <2*ndev + alsa_nnames)
        snprintf(devname, nchar, "%s", alsa_names[devno - 2*ndev]);
    else snprintf(devname, nchar, "???");
}

    /* For each hardware card found, we list two devices, the "hard" and
    "plug" one.  The card scan is derived from portaudio code. */
void alsa_getdevs(char *indevlist, int *nindevs,
    char *outdevlist, int *noutdevs, int *canmulti, 
        int maxndev, int devdescsize)
{
    int ndev = 0, cardno = -1;
    *canmulti = 2;  /* supports multiple devices */

    /* First list devices from the -alsa-adddev command line arg */
    for (ndev = 0; ndev < alsa_nnames; ndev++)
    {
        if (ndev >= maxndev)
            break;
        snprintf(indevlist + ndev * devdescsize, devdescsize, "%s",
            alsa_names[ndev]);
        snprintf(outdevlist + ndev * devdescsize, devdescsize, "%s",
            alsa_names[ndev]);
    }

    /* Then query hardware devices from ALSA */
    while (!snd_card_next(&cardno) && cardno >= 0)
    {
        snd_ctl_t *ctl;
        snd_ctl_card_info_t *info = NULL;
        char devname[80];
        const char *desc;
        if (ndev + 2  > maxndev)
            break;
        sprintf(devname, "hw:%d", cardno);
        // fprintf(stderr, "\ntry %s...\n", devname);
        if (snd_ctl_open(&ctl, devname, 0) >= 0)
        {
            snd_ctl_card_info_malloc(&info);
            snd_ctl_card_info(ctl, info);
            desc = snd_ctl_card_info_get_name(info);
        }
        else
        {
            fprintf(stderr, "ALSA card scan error\n");
            desc = "???";
        }
        /* fprintf(stderr, "name: %s\n", snd_ctl_card_info_get_name(info)); */
        sprintf(indevlist + ndev * devdescsize, "%s (hardware)", desc);
        sprintf(indevlist + (ndev + 1) * devdescsize, "%s (plug-in)", desc);
        sprintf(outdevlist + ndev * devdescsize, "%s (hardware)", desc);
        sprintf(outdevlist + (ndev + 1) * devdescsize, "%s (plug-in)", desc);
        ndev += 2;
        if (info)
        {
            snd_ctl_card_info_free(info);
            info = NULL;
        }
    }

    *nindevs = *noutdevs = ndev;
}
