

/* ----------------------- Experimental routines for jack -------------- */
#ifdef USEAPI_JACK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m_pd.h"
#include "s_stuff.h"
#include <jack/jack.h>
#include <regex.h>


#define MAX_CLIENTS 100
#define NUM_JACK_PORTS 128
#define BUF_JACK 4096
// ag 20241213: This was a hard-coded constant 100 in the code previously, but
// Pipewire seems to allow larger sizes. We now use jack_client_name_size()
// here if it is supported, the following is used as a fallback if not.
// See: https://github.com/pure-data/pure-data/commit/a20fce3c
#define CLIENT_NAME_SIZE_FALLBACK 256
static jack_nframes_t jack_out_max;
#define JACK_OUT_MAX  64
static jack_nframes_t jack_filled = 0;
static t_sample jack_outbuf[NUM_JACK_PORTS*BUF_JACK];
static t_sample jack_inbuf[NUM_JACK_PORTS*BUF_JACK];
static int jack_started = 0;
static int jack_default_client = -1;


static jack_port_t *input_port[NUM_JACK_PORTS];
static jack_port_t *output_port[NUM_JACK_PORTS];
static int outport_count = 0;
static jack_client_t *jack_client = NULL;
char *jack_client_names[MAX_CLIENTS];
static int jack_dio_error;


pthread_mutex_t jack_mutex;
pthread_cond_t jack_sem;

int jack_get_srate(void)
{
    if (jack_client) return jack_get_sample_rate (jack_client);
    else return 0;
}

static int process (jack_nframes_t nframes, void *arg)
{
    int j;
    jack_default_audio_sample_t *out, *in;
        
    if (nframes > JACK_OUT_MAX) jack_out_max = nframes;
    else jack_out_max = JACK_OUT_MAX;
    if (jack_filled >= nframes)
    {
        if (jack_filled != nframes) fprintf(stderr,"Partial read");
        /* hmm, how to find out whether 't_sample' and
           'jack_default_audio_sample_t' are actually the same type? */
        if(sizeof(t_sample)==sizeof(jack_default_audio_sample_t)) 
        {
            for (j = 0; j < sys_outchannels;  j++)
            {
                out = jack_port_get_buffer (output_port[j], nframes);
                memcpy(out, jack_outbuf + (j * BUF_JACK),
                    sizeof(jack_default_audio_sample_t) * nframes);
            }
            for (j = 0; j < sys_inchannels; j++)
            {
                in = jack_port_get_buffer( input_port[j], nframes);
                memcpy(jack_inbuf + (j * BUF_JACK), in,
                    sizeof(jack_default_audio_sample_t) * nframes);
            }
        } 
        else
        {
            unsigned int frame=0;
            t_sample*data;
            for (j = 0; j < sys_outchannels;  j++)
            {
                out = jack_port_get_buffer (output_port[j], nframes);
                data=jack_outbuf + (j * BUF_JACK);
                for(frame=0; frame<nframes; frame++)
                {
                    *out++=*data++;
                }
            }
            for (j = 0; j < sys_inchannels; j++)
            {
                in = jack_port_get_buffer( input_port[j], nframes);
                data=jack_inbuf+(j*BUF_JACK);
                for(frame=0; frame<nframes; frame++)
                {
                  *data++=*in++;
                }
            }
        }
        jack_filled -= nframes;
    }
    else
    { /* PD could not keep up ! */
        if (jack_started) jack_dio_error = 1;
        for (j = 0; j < outport_count;  j++)
        {
            out = jack_port_get_buffer (output_port[j], nframes);
            memset(out, 0, sizeof (jack_default_audio_sample_t) * nframes); 
        }
        memset(jack_outbuf,0,sizeof(jack_outbuf));
        jack_filled = 0;
    }
    pthread_cond_broadcast(&jack_sem);
    return 0;
}

static int jack_srate (jack_nframes_t srate, void *arg)
{
    sys_dacsr = srate;
    return 0;
}

static void
jack_shutdown (void *arg)
{
    verbose(1, "JACK-server shut down");
    sys_close_audio();
    jack_client = NULL;
}

static int jack_xrun(void* arg)
{
    verbose(1, "JACK-server xrun");
    jack_dio_error = 1;
    return 0;
}


static char** jack_get_clients(void)
{
    const char **jack_ports;
    int tmp_client_name_size = jack_client_name_size ? jack_client_name_size() : CLIENT_NAME_SIZE_FALLBACK;
    char* tmp_client_name = (char*)getbytes(tmp_client_name_size);
    int i,j;
    int num_clients = 0;
    regex_t port_regex;
    jack_ports = jack_get_ports( jack_client, "", "", 0 );
    regcomp( &port_regex, "^[^:]*", REG_EXTENDED );

    jack_client_names[0] = NULL;

    /* Build a list of clients from the list of ports */
    if (jack_ports != NULL)
    {
        for( i = 0; jack_ports[i] != NULL; i++ )
        {
            int client_seen;
            regmatch_t match_info;

            /* extract the client name from the port name, using a regex
             * that parses the clientname:portname syntax */
            regexec( &port_regex, jack_ports[i], 1, &match_info, 0 );
            memcpy( tmp_client_name, &jack_ports[i][match_info.rm_so],
                    match_info.rm_eo - match_info.rm_so );
            tmp_client_name[ match_info.rm_eo - match_info.rm_so ] = '\0';

            /* do we know about this port's client yet? */
            client_seen = 0;

            for( j = 0; j < num_clients; j++ )
                if( strcmp( tmp_client_name, jack_client_names[j] ) == 0 )
                    client_seen = 1;

            if( client_seen == 0 )
            {
                jack_client_names[num_clients] =
                    (char*)getbytes(strlen(tmp_client_name) + 1);

                // ag 20241213: We will auto-connect to 'system' if it exists,
                // to maintain backward compatibility if you're still running
                // Jack proper. Note that this client normally won't exist on
                // Pipewire systems running the Pipewire Jack module, in which
                // case we skip the auto-connection, so you'll have to connect
                // using a patchbay program like qjackctl.
                if( strcmp( "system", tmp_client_name ) == 0 )
                    jack_default_client = num_clients;
                /* put the new client at the end of the client list */
                strcpy( jack_client_names[ num_clients ], tmp_client_name );
                num_clients++;

            }
        }
    }

    /*   for (i=0;i<num_clients;i++) post("client: %s",jack_client_names[i]); */

    freebytes( tmp_client_name, tmp_client_name_size );
    free( jack_ports );
    return jack_client_names;
}

/*   
 *   Wire up all the ports of one client.
 *
 */

static int jack_connect_ports(char* client)
{
    char  regex_pattern[CLIENT_NAME_SIZE_FALLBACK]; /* its always the same, ... */
    int i;
    const char **jack_ports;

    if (strlen(client) > CLIENT_NAME_SIZE_FALLBACK-4)  return -1;

    sprintf( regex_pattern, "%s:.*", client );

    jack_ports = jack_get_ports( jack_client, regex_pattern,
                                 NULL, JackPortIsOutput);
    if (jack_ports) 
    {
        for (i = 0;jack_ports[i] != NULL && i < sys_inchannels; i++)
        {
            if (jack_connect(jack_client, jack_ports[i],
                    jack_port_name(input_port[i]))) 
            {
                error("JACK: cannot connect input ports %s -> %s",
                    jack_ports[i],jack_port_name (input_port[i]));
            }
        }
    }
  
    jack_ports = jack_get_ports( jack_client, regex_pattern,
                                 NULL, JackPortIsInput);
    if (jack_ports) 
    {
        for (i = 0;jack_ports[i] != NULL && i < sys_outchannels; i++)
        {
            if (jack_connect (jack_client,
                    jack_port_name(output_port[i]), jack_ports[i]))
            {
                error("JACK: cannot connect output ports %s -> %s",
                    jack_port_name (output_port[i]),jack_ports[i]);
            }
        }
    }
  
    free(jack_ports);
    return 0;
}


void jack_error(const char *desc)
{
    error("JACK error: %s", desc);
    return;
}


int jack_open_audio(int inchans, int outchans, int rate)
{
    int j;
    char port_name[80] = "";
    int client_iterator = 0;
    int new_jack = 0;
    int srate;
    jack_status_t status;

    jack_dio_error = 0;
    
    if ((inchans == 0) && (outchans == 0)) return 0;

    if (outchans > NUM_JACK_PORTS)
    {
        error("JACK: %d output ports not supported, setting to %d",
            outchans, NUM_JACK_PORTS);
        outchans = NUM_JACK_PORTS;
    }

    if (inchans > NUM_JACK_PORTS)
    {
        error("JACK: %d input ports not supported, setting to %d",
            inchans, NUM_JACK_PORTS);
        inchans = NUM_JACK_PORTS;
    }

    /* try to become a client of the JACK server */
    /* if no JACK server exists, start a default one
       (jack_client_open() does that for us... */
    if (!jack_client)
    {
        do {
            sprintf(port_name,"pure_data_%d",client_iterator);
            client_iterator++;
            /* do not try to start the jack-server...
               seems to make problems... */
            jack_client = jack_client_open(port_name,
                JackNoStartServer, &status, NULL);
            if (status & JackServerFailed)
            {
                error("JACK: unable to connect to JACK server");
                jack_client=NULL;
                break;
            }
        } while (status & JackNameNotUnique);

        if(status)
        {
            if (status & JackServerStarted)
            {
                post("JACK: started JACK server?");
            }
            else
            {
                post("JACK: jack returned status %d", status);
            }
        }
      
        if (!jack_client)
        { // jack spits out enough messages already, do not warn
            sys_inchannels = sys_outchannels = 0;
            return 1;
        }
      
        jack_get_clients();

        /* tell the JACK server to call `process()' whenever
           there is work to be done.
        */
      
        jack_set_process_callback (jack_client, process, 0);
      
        // jack_set_error_function (jack_error);
      
#ifdef JACK_XRUN
        jack_set_xrun_callback (jack_client, jack_xrun, NULL);
#endif
      
        /* tell the JACK server to call `srate()' whenever
           the sample rate of the system changes.
        */
      
        jack_set_sample_rate_callback (jack_client, jack_srate, 0);
      
      
        /* tell the JACK server to call `jack_shutdown()' if
           it ever shuts down, either entirely, or if it
           just decides to stop calling us.
        */
      
        jack_on_shutdown (jack_client, jack_shutdown, 0);
      
        for (j = 0; j < NUM_JACK_PORTS; j++)
        {
             input_port[j]=NULL;
             output_port[j] = NULL;
        }

        new_jack = 1;
    }

    /* display the current sample rate. once the client is activated
       (see below), you should rely on your own sample rate
       callback (see above) for this value.
    */
    
    srate = jack_get_sample_rate (jack_client);
    sys_dacsr = srate;
            
    /* create the ports */
    
    for (j = 0; j < inchans; j++)
    {
        sprintf(port_name, "input%d", j);
        if (!input_port[j])
            input_port[j] = jack_port_register(jack_client, port_name,
                JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        if (!input_port[j])
        {
            error("JACK: can only register %d input ports "
                  "(instead of requested %d)",
                j, inchans);
            sys_inchannels = inchans = j;
            break;
        }
    }

    for (j = 0; j < outchans; j++)
    {
        sprintf(port_name, "output%d", j);
        if (!output_port[j])
            output_port[j] = jack_port_register(jack_client, port_name,
                JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (!output_port[j])
        {
            error("JACK: can only register %d output ports "
                  "(instead of requested %d)",
                j, outchans);
            sys_outchannels = outchans = j;
            break;
        }
    } 
    outport_count = outchans;

    /* tell the JACK server that we are ready to roll */

    if (new_jack)
    {
        if (jack_activate (jack_client))
        {
            error("JACK: cannot activate client");
            sys_inchannels = sys_outchannels = 0;
            return 1;
        }
        
        memset(jack_outbuf,0,sizeof(jack_outbuf));

        if (jack_default_client >= 0)
            jack_connect_ports(jack_client_names[jack_default_client]);

        pthread_mutex_init(&jack_mutex,NULL);
        pthread_cond_init(&jack_sem,NULL);
    }
    return 0;
}

void jack_close_audio(void) 

{
    jack_started = 0;
    pthread_cond_broadcast(&jack_sem);
}

int jack_send_dacs(void)

{
    t_sample * fp;
    int j;
    int rtnval =  SENDDACS_YES;
    int timenow;
    int timeref = sys_getrealtime();
            
    if (!jack_client) return SENDDACS_NO;

    if (!sys_inchannels && !sys_outchannels) return (SENDDACS_NO); 

    if (jack_dio_error)
    {
        sys_log_error(ERR_RESYNC);
        jack_dio_error = 0;
    }
    if (jack_filled >= jack_out_max)
        pthread_cond_wait(&jack_sem,&jack_mutex);
      
    jack_started = 1;

    fp = sys_soundout;
    for (j = 0; j < sys_outchannels; j++)
    {
            memcpy(jack_outbuf + (j * BUF_JACK) + jack_filled,fp,
                DEFDACBLKSIZE*sizeof(t_sample));
            fp += DEFDACBLKSIZE;  
    }
    fp = sys_soundin;
    for (j = 0; j < sys_inchannels; j++)
    {
        memcpy(fp, jack_inbuf + (j * BUF_JACK) + jack_filled,
            DEFDACBLKSIZE*sizeof(t_sample));
        fp += DEFDACBLKSIZE;
    }

    if ((timenow = sys_getrealtime()) - timeref > 0.002)
    {
        rtnval = SENDDACS_SLEPT;
    }

    memset(sys_soundout,0,DEFDACBLKSIZE*sizeof(t_sample)*sys_outchannels);
    jack_filled += DEFDACBLKSIZE;
    return rtnval;
}

void jack_getdevs(char *indevlist, int *nindevs,
    char *outdevlist, int *noutdevs, int *canmulti, 
    int maxndev, int devdescsize)
{
    int i, ndev;
    *canmulti = 0;  /* supports multiple devices */
    ndev = 1;
    for (i = 0; i < ndev; i++)
    {
        sprintf(indevlist + i * devdescsize, "JACK");
        sprintf(outdevlist + i * devdescsize, "JACK");
    }
    *nindevs = *noutdevs = ndev;
}

void jack_listdevs( void)
{
    error("device listing not implemented for jack yet");
}

#endif /* JACK */
