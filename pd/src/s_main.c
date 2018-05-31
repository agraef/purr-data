/* Copyright (c) 1997-1999 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef UNISTD
#include <unistd.h>
#endif
#ifdef MSW
#include <io.h>
#include <windows.h>
#include <winbase.h>
#endif
#ifdef _MSC_VER  /* This is only for Microsoft's compiler, not cygwin, e.g. */
#define snprintf sprintf_s
#endif

char *pd_version;
static const char pd_compiletime[] = __TIME__;
static const char pd_compiledate[] = __DATE__;

void pd_init(void);
int sys_argparse(int argc, char **argv);
void sys_findprogdir(char *progname);
int sys_startgui(const char *guipath);
int sys_rcfile(void);
int m_mainloop(void);
int m_batchmain(void);
void sys_addhelppath(char *p);
#ifdef USEAPI_ALSA
void alsa_adddev(char *name);
#endif

int sys_debuglevel;
int sys_verbose;
int sys_noloadbang;
int sys_nogui;
int sys_hipriority = -1;    /* -1 = don't care; 0 = no; 1 = yes */
int sys_guisetportnumber;   /* if started from the GUI, this is the port # */
int sys_nosleep = 0;    /* skip all "sleep" calls and spin instead */
int sys_console = 0;    /* default settings for the console is off */
int sys_k12_mode = 0;   /* by default k12 mode is off */
int sys_unique = 0;     /* by default off, prevents multiple instances
                           of pd-l2ork */
int sys_legacy = 0;     /* by default off, used to enable legacy features,
                           such as offsets in iemgui object positioning */
char *sys_guicmd;
t_symbol *sys_gui_preset; /* name of gui theme to be used */
t_symbol *sys_libdir;
t_symbol *sys_guidir;
static t_namelist *sys_openlist;
static t_namelist *sys_messagelist;
static int sys_version;
int sys_oldtclversion;      /* hack to warn g_rtext.c about old text sel */

int sys_nmidiout = -1;
int sys_nmidiin = -1;
int sys_midiindevlist[MAXMIDIINDEV] = {1};
int sys_midioutdevlist[MAXMIDIOUTDEV] = {1};

#ifdef __APPLE__
char sys_font[] = "Monaco"; /* tb: font name */
#else
char sys_font[] = "DejaVu Sans Mono"; /* tb: font name */
#endif
char sys_fontweight[] = "normal"; /* currently only used for iemguis */
static int sys_main_srate;
static int sys_main_advance;
static int sys_main_callback;
static int sys_main_blocksize;
static int sys_listplease;

int sys_externalschedlib;
char sys_externalschedlibname[MAXPDSTRING];
static int sys_batch;
int sys_extraflags;
char sys_extraflagsstring[MAXPDSTRING];
int sys_run_scheduler(const char *externalschedlibname,
    const char *sys_extraflagsstring);
int sys_noautopatch = 0;    /* temporary hack to defeat new 0.42 editing */

    /* here the "-1" counts signify that the corresponding vector hasn't been
    specified in command line arguments; sys_set_audio_settings will detect it
    and fill things in. */
static int sys_nsoundin = -1;
static int sys_nsoundout = -1;
static int sys_soundindevlist[MAXAUDIOINDEV];
static int sys_soundoutdevlist[MAXAUDIOOUTDEV];

static int sys_nchin = -1;
static int sys_nchout = -1;
static int sys_chinlist[MAXAUDIOINDEV];
static int sys_choutlist[MAXAUDIOOUTDEV];

t_sample* get_sys_soundout() { return sys_soundout; }
t_sample* get_sys_soundin() { return sys_soundin; }
int* get_sys_main_advance() { return &sys_main_advance; }
double* get_sys_time_per_dsp_tick() { return &sys_time_per_dsp_tick; }
int* get_sys_schedblocksize() { return &sys_schedblocksize; }
double* get_sys_time() { return &pd_this->pd_systime; }
t_float* get_sys_dacsr() { return &sys_dacsr; }
int* get_sys_sleepgrain() { return &sys_sleepgrain; }
int* get_sys_schedadvance() { return &sys_schedadvance; }

typedef struct _fontinfo
{
    int fi_fontsize;
    int fi_maxwidth;
    int fi_maxheight;
    int fi_hostfontsize;
    int fi_width;
    int fi_height;
} t_fontinfo;

static t_fontinfo sys_fontlist[] = { \
    {8, 6, 10, 1, 1, 1}, {10, 7, 13, 1, 1, 1}, {12, 9, 16, 1, 1, 1},
    {16, 10, 20, 1, 1, 1}, {24, 15, 30, 1, 1, 1}, {36, 25, 45, 1, 1, 1}};
//0.43 values
//    {8, 6, 10, 0, 0, 0}, {10, 7, 13, 0, 0, 0}, {12, 9, 16, 0, 0, 0},
//    {16, 10, 20, 0, 0, 0}, {24, 15, 25, 0, 0, 0}, {36, 25, 45, 0, 0, 0}};
#define NFONT (sizeof(sys_fontlist)/sizeof(*sys_fontlist))

static t_fontinfo *sys_findfont(int fontsize)
{
    unsigned int i;
    t_fontinfo *fi;
    for (i = 0, fi = sys_fontlist; i < (NFONT-1); i++, fi++)
        if (fontsize < fi[1].fi_fontsize) return (fi);
    return (sys_fontlist + (NFONT-1));
}

int sys_nearestfontsize(int fontsize)
{
    return (sys_findfont(fontsize)->fi_fontsize);
}

int sys_hostfontsize(int fontsize)
{
    return (sys_findfont(fontsize)->fi_hostfontsize);
}

int sys_fontwidth(int fontsize)
{
    return (sys_findfont(fontsize)->fi_width);
}

int sys_fontheight(int fontsize)
{
    return (sys_findfont(fontsize)->fi_height);
}

int sys_defaultfont;
#define DEFAULTFONT 10

static void openit(const char *dirname, const char *filename)
{
    char dirbuf[FILENAME_MAX], *nameptr;
    int fd = open_via_path(dirname, filename, "", dirbuf, &nameptr,
        FILENAME_MAX, 0);
    if (fd >= 0)
    {
        close (fd);
        glob_evalfile(0, gensym(nameptr), gensym(dirbuf));
        gui_vmess("gui_process_open_arg", "s", filename);
    }
    else
        error("%s: can't open", filename);
}

/* this is called from the gui process.  The first argument is the cwd, and
succeeding args give the widths and heights of known fonts.  We wait until 
these are known to open files and send messages specified on the command line.
We ask the GUI to specify the "cwd" in case we don't have a local OS to get it
from; for instance we could be some kind of RT embedded system.  However, to
really make this make sense we would have to implement
open(), read(), etc, calls to be served somehow from the GUI too. */

void glob_initfromgui(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    char *cwd = atom_getsymbolarg(0, argc, argv)->s_name;
    t_namelist *nl;
    unsigned int i;
    int j;
    int nhostfont = (argc-2)/3;
    sys_oldtclversion = atom_getfloatarg(1, argc, argv);
    if (argc != 2 + 3 * nhostfont) bug("glob_initfromgui");
    for (i = 0; i < NFONT; i++)
    {
        int best = 0;
        int wantheight = sys_fontlist[i].fi_maxheight;
        int wantwidth = sys_fontlist[i].fi_maxwidth;
        for (j = 1; j < nhostfont; j++)
        {
            if (atom_getintarg(3 * j + 4, argc, argv) <= wantheight &&
                atom_getintarg(3 * j + 3, argc, argv) <= wantwidth)
                    best = j;
        }
            /* best is now the host font index for the desired font index i. */
        sys_fontlist[i].fi_hostfontsize = atom_getintarg(3 * best + 2, argc, argv);
        sys_fontlist[i].fi_width = atom_getintarg(3 * best + 3, argc, argv);
        sys_fontlist[i].fi_height = atom_getintarg(3 * best + 4, argc, argv);
        sys_fontlist[i].fi_maxwidth = sys_fontlist[i].fi_width;
        sys_fontlist[i].fi_maxheight = sys_fontlist[i].fi_height;
    }
#if 0
    for (i = 0; i < 6; i++)
        fprintf(stderr, "font (%d %d %d) -> (%d %d %d)\n",
            sys_fontlist[i].fi_fontsize,
            sys_fontlist[i].fi_maxwidth,
            sys_fontlist[i].fi_maxheight,
            sys_fontlist[i].fi_hostfontsize,
            sys_fontlist[i].fi_width,
            sys_fontlist[i].fi_height);
#endif
        /* load dynamic libraries specified with "-lib" args */
    for  (nl = sys_externlist; nl; nl = nl->nl_next)
        if (!sys_load_lib(0, nl->nl_string))
            post("%s: can't load library", nl->nl_string);
        /* open patches specified with "-open" args */
    for  (nl = sys_openlist; nl; nl = nl->nl_next)
        openit(cwd, nl->nl_string);
    namelist_free(sys_openlist);
    sys_openlist = 0;
        /* send messages specified with "-send" args */
    for  (nl = sys_messagelist; nl; nl = nl->nl_next)
    {
        t_binbuf *b = binbuf_new();
        binbuf_text(b, nl->nl_string, strlen(nl->nl_string));
        binbuf_eval(b, 0, 0, 0);
        binbuf_free(b);
    }
    namelist_free(sys_messagelist);
    sys_messagelist = 0;
}

static void sys_afterargparse(void);

static void pd_makeversion(void)
{
    char foo[100];

    //snprintf(foo, sizeof(foo), "Pd-l2ork version %d.%d-%d%s\n",
    //    PD_MAJOR_VERSION, PD_MINOR_VERSION,
    //    PD_BUGFIX_VERSION, PD_TEST_VERSION);    

    snprintf(foo, sizeof(foo), "Pd-L2Ork version %s (%s)\n", PD_L2ORK_VERSION,
	     PD_BUILD_VERSION);

    pd_version = strdup(foo);
}

/* send the openlist to the GUI before closing a secondary
   instance of Pd. */
void glob_forward_files_from_secondary_instance(void)
{
        /* check if we are unique, otherwise, just focus existing
           instance, and if necessary open file inside it. This doesn't
           yet work with the new GUI because we need to set it up to
           allow multiple instances. */
    gui_start_vmess("gui_open_via_unique", "xi", pd_this, sys_unique);
    gui_start_array();
    if (sys_openlist)
    {
        // send the files to be opened to the GUI. We send them as an
        // array here so that we don't have to allocate anything here
        // (as the previous API did)
        t_namelist *nl;
        for (nl = sys_openlist; nl; nl = nl->nl_next)
        {
            gui_s(nl->nl_string);
        }
    }
    gui_end_array();
    gui_end_vmess();
}

extern void glob_recent_files(t_pd *dummy);
extern int sys_browser_doc, sys_browser_path, sys_browser_init;

/* this is called from main() in s_entry.c */
int sys_main(int argc, char **argv)
{
    int i, noprefs;
    t_namelist *nl;
    sys_externalschedlib = 0;
    sys_extraflags = 0;
    sys_gui_preset = gensym("default");
#ifdef PD_DEBUG
    fprintf(stderr, "Pd-L2Ork: COMPILED FOR DEBUGGING\n");
#endif
    pd_init();                                  /* start the message system */
    logpost(NULL, 2, "PD_FLOATSIZE = %lu bits", sizeof(t_float)*8);
    sys_findprogdir(argv[0]);                   /* set sys_progname, guipath */
    for (i = noprefs = 0; i < argc; i++)        /* prescan args for noprefs */
        if (!strcmp(argv[i], "-noprefs"))
            noprefs = 1;
    if (!noprefs)
        sys_loadpreferences();                  /* load default settings */
    sys_load_recent_files();                    /* load recent files table */
#ifndef MSW
    if (!noprefs)
        sys_rcfile();                           /* parse the startup file */
#endif
    if (sys_argparse(argc-1, argv+1))           /* parse cmd line */
        return (1);
    sys_afterargparse();                    /* post-argparse settings */
        /* build version string from defines in m_pd.h */
    pd_makeversion();
    if (sys_verbose || sys_version) fprintf(stderr, "%scompiled %s %s\n",
        pd_version, pd_compiletime, pd_compiledate);
    if (sys_version)    /* if we were just asked our version, exit here. */
        return (0);
    if (sys_startgui(sys_guidir->s_name))       /* start the gui */
        return(1);
        /* send the libdir to the GUI */
    gui_vmess("gui_set_lib_dir", "s", sys_libdir->s_name);
        /* send the name of the gui preset */
    gui_vmess("gui_set_gui_preset", "s", sys_gui_preset->s_name);
        /* send the recent files list */
    glob_recent_files(0);
        /* AG: send the browser config; this must come *after* gui_set_lib_dir
           so that the lib_dir is available when help indexing starts */
    gui_start_vmess("gui_set_browser_config", "iii",
                    sys_browser_doc, sys_browser_path, sys_browser_init);
    gui_start_array();
    for (nl = sys_helppath; nl; nl = nl->nl_next)
    {
        gui_s(nl->nl_string);
    }
    gui_end_array();
    gui_end_vmess();

    if (sys_externalschedlib)
        return (sys_run_scheduler(sys_externalschedlibname,
            sys_extraflagsstring));
    else if (sys_batch)
        return (m_batchmain());
    else
    {
        /* open audio and MIDI */
        sys_reopen_midi();
        sys_reopen_audio();

        if (sys_console) sys_vgui("pdtk_toggle_console 1\n");
        if (sys_k12_mode)
        {
            t_namelist *path = pd_extrapath;
            while (path->nl_next)
                path = path->nl_next;
            sys_vgui("pdtk_enable_k12_mode %s\n", path->nl_string);
        }
         /* run scheduler until it quits */
        return (m_mainloop());
    }
}

static char *(usagemessage[]) = {
"Usage: pd-l2ork [-flags <value>] [file1 file2 ... filen]\n",
"\nAudio configuration flags:\n",
"-r <n>           -- specify sample rate\n",
"-audioindev ...  -- audio in devices; e.g., \"1,3\" for first and third\n",
"-audiooutdev ... -- audio out devices (same)\n",
"-audiodev ...    -- specify input and output together\n",
"-inchannels ...  -- audio input channels (by device, like \"2\" or \"16,8\")\n",
"-outchannels ... -- number of audio out channels (same)\n",
"-channels ...    -- specify both input and output channels\n",
"-audiobuf <n>    -- specify size of audio buffer in msec\n",
"-blocksize <n>   -- specify audio I/O block size in sample frames\n",
"-sleepgrain <n>  -- specify number of milliseconds to sleep when idle\n",
"-nodac           -- suppress audio output\n",
"-noadc           -- suppress audio input\n",
"-noaudio         -- suppress audio input and output (-nosound is synonym) \n",
"-listdev         -- list audio and MIDI devices\n",

#ifdef USEAPI_OSS
"-oss             -- use OSS audio API\n",
#endif

#ifdef USEAPI_ALSA
"-alsa            -- use ALSA audio API\n",
"-alsaadd <name>  -- add an ALSA device name to list\n",
#endif

#ifdef USEAPI_JACK
"-jack            -- use JACK audio API\n",
#endif

#ifdef USEAPI_PORTAUDIO
#ifdef MSW
"-asio            -- use ASIO audio driver (via Portaudio)\n",
"-pa              -- synonym for -asio\n",
#else
"-pa              -- use Portaudio API\n",
#endif
#endif

#ifdef USEAPI_MMIO
"-mmio            -- use MMIO audio API (default for Windows)\n",
#endif
"      (default audio API for this platform:  ", API_DEFSTRING, ")\n",

"\nMIDI configuration flags:\n",
"-midiindev ...   -- midi in device list; e.g., \"1,3\" for first and third\n",
"-midioutdev ...  -- midi out device list, same format\n",
"-mididev ...     -- specify -midioutdev and -midiindev together\n",
"-nomidiin        -- suppress MIDI input\n",
"-nomidiout       -- suppress MIDI output\n",
"-nomidi          -- suppress MIDI input and output\n",
#ifdef USEAPI_ALSA
"-alsamidi        -- use ALSA midi API\n",
#endif


"\nOther flags:\n",
"-path <path>     -- add to file search path\n",
"-nostdpath       -- don't search standard (\"extra\") directory\n",
"-stdpath         -- search standard directory (true by default)\n",
"-helppath <path> -- add to help file search path\n",
"-open <file>     -- open file(s) on startup\n",
"-lib <file>      -- load object library(s)\n",
"-font-size <n>     -- specify default font size in points\n",
"-font-face <name>  -- specify default font (default: Bitstream Vera Sans Mono)\n",
"-font-weight <name>-- specify default font weight (normal or bold)\n",
"-verbose         -- extra printout on startup and when searching for files\n",
"-version         -- don't run Pd; just print out which version it is \n",
"-d <n>           -- specify debug type:\n",
"                    1=Pd->GUI\n",
"                    2=GUI->Pd\n",
"                    3=Pd->GUI and GUI->Pd\n",
"                    5=Pd->GUI with Pd linenumbers\n",
"                    7=Pd->GUI and GUI->Pd with Pd linenumbers\n", 
"-noloadbang      -- suppress all loadbangs\n",
"-stderr          -- send printout to standard error instead of GUI\n",
"-nogui           -- suppress starting the GUI\n",
"-guiport <n>     -- connect to pre-existing GUI over port <n>\n",
"-guicmd \"cmd...\" -- start alternatve GUI program (e.g., remote via ssh)\n",
"-send \"msg...\"   -- send a message at startup, after patches are loaded\n",
"-noprefs         -- suppress loading preferences on startup\n",
"-console         -- open the console along with the pd window\n",
#ifdef UNISTD
"-rt or -realtime -- use real-time priority\n",
"-nrt             -- don't use real-time priority\n",
#endif
"-nosleep         -- spin, don't sleep (may lower latency on multi-CPUs)\n",
"-schedlib <file> -- plug in external scheduler\n",
"-extraflags <s>  -- string argument to send schedlib\n",
"-batch           -- run off-line as a batch process\n",
"-autopatch       -- enable auto-patching new from selected objects\n",
"-k12             -- enable K-12 education mode (requires L2Ork K12 lib)\n",
"-unique          -- enable multiple instances (disabled by default)\n",
"-legacy          -- enable legacy features (disabled by default)\n", 
"\n",
};

static void sys_parsedevlist(int *np, int *vecp, int max, char *str)
{
    int n = 0;
    while (n < max)
    {
        if (!*str) break;
        else
        {
            char *endp;
            vecp[n] = strtol(str, &endp, 10);
            if (endp == str)
                break;
            n++;
            if (!endp)
                break;
            str = endp + 1;
        }
    }
    *np = n;
}

/*
static int sys_getmultidevchannels(int n, int *devlist)
{
    int sum = 0;
    if (n<0)return(-1);
    if (n==0)return 0;
    while(n--)sum+=*devlist++;
    return sum;
}
*/

    /* this routine tries to figure out where to find the auxilliary files
    Pd will need to run.  This is either done by looking at the command line
    invokation for Pd, or if that fails, by consulting the variable
    INSTALL_PREFIX.  In MSW, we don't try to use INSTALL_PREFIX. */
void sys_findprogdir(char *progname)
{
    char *execdir = pd_getdirname()->s_name, *lastslash;
    char sbuf[FILENAME_MAX], sbuf2[FILENAME_MAX], appbuf[FILENAME_MAX];
    strncpy(sbuf, execdir, FILENAME_MAX-1);
#ifndef MSW
    struct stat statbuf;
#endif
    lastslash = strrchr(sbuf, '/');
    if (lastslash)
    {
            // bash last slash to zero so that sbuf is directory pd was in,
            //    e.g., ~/pd/bin
        *lastslash = 0;
            // go back to the parent from there, e.g., ~/pd
        lastslash = strrchr(sbuf, '/');
        if (lastslash)
        {
            strncpy(sbuf2, sbuf, lastslash-sbuf);
            sbuf2[lastslash-sbuf] = 0;
        }
        else strcpy(sbuf2, "..");
    }
    else
    {
            /* no slashes found.  Try INSTALL_PREFIX. */
#ifdef INSTALL_PREFIX
        strncpy(sbuf2, INSTALL_PREFIX, FILENAME_MAX-1);
#else
        strcpy(sbuf2, ".");
#endif
    }
        /* now we believe sbuf2 holds the parent directory of the directory
        pd was found in.  We now want to infer the "lib" directory and the
        "gui" directory.  In "simple" unix installations, the layout is
            .../bin/pd
            .../bin/pd-gui
            .../doc
        and in "complicated" unix installations, it's:
            .../bin/pd
            .../lib/pd-l2ork/bin/pd-gui
            .../lib/pd-l2ork/doc
        To decide which, we stat .../lib/pd-l2ork; if that exists, we assume it's
        the complicated layout.  In MSW, it's the "simple" layout, but
        the gui program is straight wish80:
            .../bin/pd
            .../bin/wish80.exe
            .../doc
        */
#ifdef MSW
    sys_libdir = gensym(sbuf2);
    sys_guidir = &s_;   /* in MSW the guipath just depends on the libdir */
#else
    strncpy(sbuf, sbuf2, FILENAME_MAX-30);
    sbuf[FILENAME_MAX-30] = 0;
    strcat(sbuf2, "/lib/pd-l2ork");
    if (stat(sbuf2, &statbuf) >= 0)
    {
            /* complicated layout: lib dir is the one we just stat-ed above */
        sys_libdir = gensym(sbuf2);
            /* gui lives in .../lib/pd-l2ork/bin */
        strncpy(sbuf2, sbuf, FILENAME_MAX-30);
        sbuf[FILENAME_MAX-30] = 0;
        strcat(sbuf2, "/lib/pd-l2ork/bin");
        sys_guidir = gensym(sbuf2);
    }
    else
    {
            /* simple layout: lib dir is the parent */
            /* gui lives in .../bin */
        strncpy(sbuf2, sbuf, FILENAME_MAX-30);
        strncpy(appbuf, sbuf, FILENAME_MAX-30);
        sbuf[FILENAME_MAX-30] = 0;
        sys_libdir = gensym(sbuf);
        strcat(sbuf2, "/bin");
        /* special case-- with the OSX app bundle the guidir is actually
           in app.nw instead of app.nw/bin. So we check for a package.json
           there and then set it accordingly. */
        strcat(appbuf, "/package.json");
        if (stat(appbuf, &statbuf) >= 0)
            sys_guidir = gensym(sbuf);
        else
            sys_guidir = gensym(sbuf2);
    }
#endif
}

#ifdef MSW
static int sys_mmio = 1;
#else
static int sys_mmio = 0;
#endif

int sys_argparse(int argc, char **argv)
{
    while ((argc > 0) && **argv == '-')
    {
        if (!strcmp(*argv, "-r") && argc > 1 &&
            sscanf(argv[1], "%d", &sys_main_srate) >= 1)
        {
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-inchannels") && (argc > 1))
        {
            sys_parsedevlist(&sys_nchin,
                sys_chinlist, MAXAUDIOINDEV, argv[1]);

          if (!sys_nchin)
              goto usage;

          argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-outchannels") && (argc > 1))
        {
            sys_parsedevlist(&sys_nchout, sys_choutlist,
                MAXAUDIOOUTDEV, argv[1]);

          if (!sys_nchout)
            goto usage;

          argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-channels") && (argc > 1))
        {
            sys_parsedevlist(&sys_nchin, sys_chinlist,MAXAUDIOINDEV,
                argv[1]);
            sys_parsedevlist(&sys_nchout, sys_choutlist,MAXAUDIOOUTDEV,
                argv[1]);

            if (!sys_nchout)
              goto usage;

            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-soundbuf") ||
                 !strcmp(*argv, "-audiobuf") && (argc > 1))
        {
            sys_main_advance = atoi(argv[1]);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-callback"))
        {
            sys_main_callback = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-blocksize"))
        {
            sys_setblocksize(atoi(argv[1]));
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-sleepgrain") && (argc > 1))
        {
            sys_sleepgrain = 1000 * atof(argv[1]);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-nodac"))
        {
            sys_nsoundout=0;
            sys_nchout = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-noadc"))
        {
            sys_nsoundin=0;
            sys_nchin = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-nosound") || !strcmp(*argv, "-noaudio"))
        {
            sys_nsoundin=sys_nsoundout = 0;
            sys_nchin = sys_nchout = 0;
            argc--; argv++;
        }
#ifdef USEAPI_OSS
        else if (!strcmp(*argv, "-oss"))
        {
            sys_set_audio_api(API_OSS);
            argc--; argv++;
        }
#endif
#ifdef USEAPI_ALSA
        else if (!strcmp(*argv, "-alsa"))
        {
            sys_set_audio_api(API_ALSA);
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-alsaadd") && (argc > 1))
        {
            if (argc > 1)
                alsa_adddev(argv[1]);
            else goto usage;
            argc -= 2; argv +=2;
        }
        else if (!strcmp(*argv, "-alsamidi"))
        {
          sys_set_midi_api(API_ALSA);
            argc--; argv++;
        }
#endif
#ifdef USEAPI_JACK
        else if (!strcmp(*argv, "-jack"))
        {
            sys_set_audio_api(API_JACK);
            argc--; argv++;
        }
#endif
#ifdef USEAPI_PORTAUDIO
        else if (!strcmp(*argv, "-pa") || !strcmp(*argv, "-portaudio")
#ifdef MSW
            || !strcmp(*argv, "-asio")
#endif
            )
        {
            sys_set_audio_api(API_PORTAUDIO);
            sys_mmio = 0;
            argc--; argv++;
        }
#endif
#ifdef USEAPI_MMIO
        else if (!strcmp(*argv, "-mmio"))
        {
            sys_set_audio_api(API_MMIO);
            sys_mmio = 1;
            argc--; argv++;
        }
#endif
        else if (!strcmp(*argv, "-nomidiin"))
        {
            sys_nmidiin = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-nomidiout"))
        {
            sys_nmidiout = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-nomidi"))
        {
            sys_nmidiin = sys_nmidiout = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-midiindev"))
        {
            sys_parsedevlist(&sys_nmidiin, sys_midiindevlist, MAXMIDIINDEV,
                argv[1]);
            if (!sys_nmidiin)
                goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-midioutdev") && (argc > 1))
        {
            sys_parsedevlist(&sys_nmidiout, sys_midioutdevlist, MAXMIDIOUTDEV,
                argv[1]);
            if (!sys_nmidiout)
                goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-mididev") && (argc > 1))
        {
            sys_parsedevlist(&sys_nmidiin, sys_midiindevlist, MAXMIDIINDEV,
                argv[1]);
            sys_parsedevlist(&sys_nmidiout, sys_midioutdevlist, MAXMIDIOUTDEV,
                argv[1]);
            if (!sys_nmidiout)
                goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-path") && (argc > 1))
        {
            sys_searchpath = namelist_append_files(sys_searchpath, argv[1]);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-nostdpath"))
        {
            sys_usestdpath = 0;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-stdpath"))
        {
            sys_usestdpath = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-helppath"))
        {
            sys_helppath = namelist_append_files(sys_helppath, argv[1]);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-open") && argc > 1)
        {
            sys_openlist = namelist_append_files(sys_openlist, argv[1]);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-lib") && argc > 1)
        {
            sys_externlist = namelist_append_files(sys_externlist, argv[1]);
            argc -= 2; argv += 2;
        }
        else if ((!strcmp(*argv, "-font-size") || !strcmp(*argv, "-font"))
            && argc > 1)
        {
            sys_defaultfont = sys_nearestfontsize(atoi(argv[1]));
            argc -= 2;
            argv += 2;
        }
        else if ((!strcmp(*argv, "-font-face") || !strcmp(*argv, "-typeface"))
            && argc > 1)
        {
            strncpy(sys_font,*(argv+1),sizeof(sys_font)-1);
            sys_font[sizeof(sys_font)-1] = 0;
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-font-weight") && argc > 1)
        {
            strncpy(sys_fontweight,*(argv+1),sizeof(sys_fontweight)-1);
            sys_fontweight[sizeof(sys_fontweight)-1] = 0;
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-verbose"))
        {
            sys_verbose++;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-version"))
        {
            sys_version = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-d") && argc > 1 &&
            sscanf(argv[1], "%d", &sys_debuglevel) >= 1)
        {
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-noloadbang"))
        {
            sys_noloadbang = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-nogui"))
        {
            sys_printtostderr = sys_nogui = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-console"))
        {
            sys_console = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-k12"))
        {
            sys_k12_mode = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-unique"))
        {
            sys_unique = 1;
            argc -= 1;
            argv += 1;
        }
        else if (!strcmp(*argv, "-legacy"))
        {
            sys_legacy = 1;
            argc -= 1;
            argv += 1;
        }
        else if (!strcmp(*argv, "-guiport") && argc > 1 &&
            sscanf(argv[1], "%d", &sys_guisetportnumber) >= 1)
        {
            argc -= 2;
            argv += 2;
        }
        else if (!strcmp(*argv, "-stderr"))
        {
            sys_printtostderr = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-guicmd") && argc > 1)
        {
            sys_guicmd = argv[1];
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-send") && argc > 1)
        {
            sys_messagelist = namelist_append(sys_messagelist, argv[1], 1);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-listdev"))
        {
            sys_listplease = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-schedlib") && argc > 1)
        {
            sys_externalschedlib = 1;
            strncpy(sys_externalschedlibname, argv[1],
                sizeof(sys_externalschedlibname) - 1);
            argv += 2;
            argc -= 2;
        }
        else if (!strcmp(*argv, "-extraflags") && argc > 1)
        {
            sys_extraflags = 1;
            strncpy(sys_extraflagsstring, argv[1],
                sizeof(sys_extraflagsstring) - 1);
            argv += 2;
            argc -= 2;
        }
        else if (!strcmp(*argv, "-batch"))
        {
            sys_batch = 1;
            sys_printtostderr = sys_nogui = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-noautopatch"))
        {
            sys_noautopatch = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-autopatch"))
        {
            sys_noautopatch = 0;
            argc--; argv++;
        }
#ifdef UNISTD
        else if (!strcmp(*argv, "-rt") || !strcmp(*argv, "-realtime"))
        {
            sys_hipriority = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-nrt"))
        {
            sys_hipriority = 0;
            argc--; argv++;
        }
#endif
        else if (!strcmp(*argv, "-nosleep"))
        {
            sys_nosleep = 1;
            argc--; argv++;
        }
        else if (!strcmp(*argv, "-soundindev") ||
            !strcmp(*argv, "-audioindev"))
        {
            sys_parsedevlist(&sys_nsoundin, sys_soundindevlist,
                MAXAUDIOINDEV, argv[1]);
            if (!sys_nsoundin)
                goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-soundoutdev") ||
            !strcmp(*argv, "-audiooutdev"))
        {
            sys_parsedevlist(&sys_nsoundout, sys_soundoutdevlist,
                MAXAUDIOOUTDEV, argv[1]);
            if (!sys_nsoundout)
                goto usage;
            argc -= 2; argv += 2;
        }
        else if ((!strcmp(*argv, "-sounddev") || !strcmp(*argv, "-audiodev"))
                 && (argc > 1))
        {
            sys_parsedevlist(&sys_nsoundin, sys_soundindevlist,
                MAXAUDIOINDEV, argv[1]);
            sys_parsedevlist(&sys_nsoundout, sys_soundoutdevlist,
                MAXAUDIOOUTDEV, argv[1]);
            if (!sys_nsoundout)
                goto usage;
            argc -= 2; argv += 2;
        }
        else if (!strcmp(*argv, "-noprefs")) /* did this earlier */
            argc--, argv++;
        else
        {
            unsigned int i;
        usage:
            for (i = 0; i < sizeof(usagemessage)/sizeof(*usagemessage); i++)
                fprintf(stderr, "%s", usagemessage[i]);
            return (1);
        }
    }
    if (sys_listplease && !sys_printtostderr)
    {
        // if we asked to list devices and are not using stderr output
        // open console to facilitate understanding where devices have 
        // been listed
        sys_console = 1;
    }
    if (!sys_defaultfont)
        sys_defaultfont = DEFAULTFONT;
    for (; argc > 0; argc--, argv++) 
        sys_openlist = namelist_append_files(sys_openlist, *argv);


    return (0);
}

int sys_getblksize(void)
{
    return (DEFDACBLKSIZE);
}

    /* stuff to do, once, after calling sys_argparse() -- which may itself
    be called more than once (first from "settings, second from .pdrc, then
    from command-line arguments */
static void sys_afterargparse(void)
{
    char sbuf[MAXPDSTRING];
    int i;
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int nchindev, nchoutdev, rate, advance, callback, blocksize;
    int nmidiindev = 0, midiindev[MAXMIDIINDEV];
    int nmidioutdev = 0, midioutdev[MAXMIDIOUTDEV];
            /* add "extra" library to path */
    strncpy(sbuf, sys_libdir->s_name, MAXPDSTRING-30);
    sbuf[MAXPDSTRING-30] = 0;
    strcat(sbuf, "/extra");
    sys_setextrapath(sbuf);
            /* add "doc/5.reference" library to helppath */
    strncpy(sbuf, sys_libdir->s_name, MAXPDSTRING-30);
    sbuf[MAXPDSTRING-30] = 0;
    strcat(sbuf, "/doc/5.reference");
    sys_helppath = namelist_append_files(sys_helppath, sbuf);
        /* correct to make audio and MIDI device lists zero based.  On
        MMIO, however, "1" really means the second device (the first one
        is "mapper" which is was not included when the command args were
        set up, so we leave it that way for compatibility. */
    if (!sys_mmio)
    {
        for (i = 0; i < sys_nsoundin; i++)
            sys_soundindevlist[i]--;
        for (i = 0; i < sys_nsoundout; i++)
            sys_soundoutdevlist[i]--;
    }
    for (i = 0; i < sys_nmidiin; i++)
        sys_midiindevlist[i]--;
    for (i = 0; i < sys_nmidiout; i++)
        sys_midioutdevlist[i]--;
    if (sys_listplease)
        sys_listdevs();
        
            /* get the current audio parameters.  These are set
            by the preferences mechanism (sys_loadpreferences()) or
            else are the default.  Overwrite them with any results
            of argument parsing, and store them again. */
    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance,
            &callback, &blocksize);
    if (sys_nchin >= 0)
    {
        nchindev = sys_nchin;
        for (i = 0; i < nchindev; i++)
            chindev[i] = sys_chinlist[i];
    }
    else nchindev = naudioindev;
    if (sys_nsoundin >= 0)
    {
        naudioindev = sys_nsoundin;
        for (i = 0; i < naudioindev; i++)
            audioindev[i] = sys_soundindevlist[i];
    }
    
    if (sys_nchout >= 0)
    {
        nchoutdev = sys_nchout;
        for (i = 0; i < nchoutdev; i++)
            choutdev[i] = sys_choutlist[i];
    }
    else nchoutdev = naudiooutdev;
    if (sys_nsoundout >= 0)
    {
        naudiooutdev = sys_nsoundout;
        for (i = 0; i < naudiooutdev; i++)
            audiooutdev[i] = sys_soundoutdevlist[i];
    }
    sys_get_midi_params(&nmidiindev, midiindev, &nmidioutdev, midioutdev);
    if (sys_nmidiin >= 0)
    {
        post("sys_nmidiin %d, nmidiindev %d", sys_nmidiin, nmidiindev);
        nmidiindev = sys_nmidiin;
        for (i = 0; i < nmidiindev; i++)
            midiindev[i] = sys_midiindevlist[i];
    }
    if (sys_nmidiout >= 0)
    {
        nmidioutdev = sys_nmidiout;
        for (i = 0; i < nmidioutdev; i++)
            midioutdev[i] = sys_midioutdevlist[i];
    }
    if (sys_main_advance)
        advance = sys_main_advance;
    if (sys_main_srate)
        rate = sys_main_srate;
    if (sys_main_callback)
        callback = sys_main_callback;
    if (sys_main_blocksize)
        blocksize = sys_main_blocksize;
    sys_set_audio_settings(naudioindev, audioindev, nchindev, chindev,
        naudiooutdev, audiooutdev, nchoutdev, choutdev, rate, advance, 
        callback, blocksize);
    sys_open_midi(nmidiindev, midiindev, nmidioutdev, midioutdev, 0);
}

//static void sys_addreferencepath(void)
//{
//    char sbuf[MAXPDSTRING];
//}
