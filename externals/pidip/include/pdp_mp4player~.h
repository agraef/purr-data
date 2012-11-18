
#ifndef __PDP_MP4PLAYER__
#define __PDP_MP4PLAYER__

struct pdp_mp4player_struct;
typedef struct pdp_mp4player_struct t_pdp_mp4player;

#include "pdp.h"
#include "yuv.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "pdp_mp4playersession.h"
#include "pdp_mp4playermedia.h"
#include "pdp_mp4videosync.h"
#include "pdp_mp4audiosync.h"
#include "media_utils.h"
#include "codec_plugin_private.h"
#include "our_config_file.h"
#include "player_util.h"
#include <rtp/debug.h>
#include <libhttp/http.h>


/* mpeg4ip includes taken from the source tree ( not exported ) */
#include <mp4.h>
#undef  DECLARE_CONFIG_VARIABLES
#include "config_set.h"

#undef CONFIG_BOOL
#define CONFIG_BOOL(var, name, defval) \
 { &(var), (name), CONFIG_TYPE_BOOL, (defval), (defval) }
#undef CONFIG_FLOAT
#define CONFIG_FLOAT(var, name, defval) \
  { &(var), (name), CONFIG_TYPE_FLOAT,(float) (defval), (float) (defval) }
#undef CONFIG_INT
#define CONFIG_INT(var, name, defval) \
   { &(var), (name), CONFIG_TYPE_INTEGER,(config_integer_t) (defval), (config_integer_t)(defval) }
#undef CONFIG_STRING
#define CONFIG_STRING(var, name, defval) \
    { &(var), (name), CONFIG_TYPE_STRING, (defval), (defval) }

#include "pdp_mp4config.h"

#undef   DECLARE_CONFIG_VARIABLES
#ifndef debug_message
#define debug_message post
#endif
#include "rtp_transmitter.h"
#include "pdp_mp4videosource.h"
#include "pdp_mp4audiosource.h"

#define DEFAULT_CHANNELS 2
#define MIN_PRIORITY -20
#define DEFAULT_PRIORITY 0
#define MAX_PRIORITY 20

#define VIDEO_BUFFER_SIZE (1024*1024)
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)
#define MIN_AUDIO_SIZE (64 * 1024)
#define AUDIO_PACKET_SIZE (2*1152)

typedef struct pdp_mp4player_struct
{
    t_object x_obj;
    t_float x_f;

    t_int x_packet;
    t_int x_dropped;

    t_pdp *x_header;
    short int *x_data;
    t_int x_vwidth;
    t_int x_vheight;
    t_int x_vsize;

    t_outlet *x_pdp_out;           // output decoded pdp packets
    t_outlet *x_outlet_left;       // left audio output
    t_outlet *x_outlet_right;      // right audio output
    t_outlet *x_outlet_streaming;  // indicates the action of streaming
    t_outlet *x_outlet_nbframes;   // number of frames emitted
    t_outlet *x_outlet_framerate;  // real framerate

    char  *x_url;
    t_int x_rtpovertcp;     // flag to bypass certain firewalls (tcp mode)
    t_int x_streaming;      // streaming flag
    t_int x_nbframes;       // number of frames emitted
    t_int x_framerate;      // framerate
    t_int x_samplerate;     // audio sample rate
    t_int x_audiochannels;  // audio channels
    t_int x_audioon;        // enough audio data to start playing
    t_int x_blocksize;      // audio block size
    struct timeval x_starttime; // streaming starting time
    t_int x_cursec;         // current second
    t_int x_secondcount;    // number of frames received in the current second
    pthread_t x_decodechild;// stream decoding thread
    t_int x_priority;       // priority of decoding thread
    t_int x_newpicture;     // flag indicating a new picture

    short int *x_datav;     // video data from mpeg4hippies

      /* audio structures */
    t_int x_audio;           // flag to activate the decoding of audio
    short x_audio_in[4*MAX_AUDIO_PACKET_SIZE]; /* buffer for resampled PCM audio */
    t_int x_audioin_position; // writing position for incoming audio

      /* mpeg4hippies structures */
    CPlayerSession      *x_psession;
    CMsgQueue           x_queue;
    SDL_sem             *x_psem;
    t_int x_decodingstate;    // internal decoding state

} t_pdp_mp4player;

#endif

