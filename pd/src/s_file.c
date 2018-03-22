/* Copyright (c) 1997-2004 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/*
 * this file implements a mechanism for storing and retrieving preferences.
 * Should later be renamed "preferences.c" or something.
 *
 * In unix this is handled by the "~/.pd-l2ork/user.settings" file, in windows by
 * the registry, and in MacOS by the Preferences system.
 */

#include "config.h"

#include "m_pd.h"
#include "s_stuff.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_UNISTD_H
/* XXX Hack!  This should be done with a cleaner check. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef MSW
#include <windows.h>
#include <tchar.h>
#endif
#ifdef _MSC_VER  /* This is only for Microsoft's compiler, not cygwin, e.g. */
#define snprintf sprintf_s
#endif

int sys_defeatrt, sys_zoom, sys_browser_doc = 1, sys_browser_path, sys_browser_init;
t_symbol *sys_flags = &s_;
void sys_doflags( void);

#ifdef UNIX

#define USER_CONFIG_DIR ".purr-data"

static char *sys_prefbuf;

static void sys_initloadpreferences( void)
{
    char filenamebuf[FILENAME_MAX], *homedir = getenv("HOME");
    int fd, length;
    char user_prefs_file[FILENAME_MAX]; /* user prefs file */
        /* default prefs embedded in the package */
    char default_prefs_file[FILENAME_MAX];
    struct stat statbuf;

    snprintf(default_prefs_file, FILENAME_MAX, "%s/default.settings", 
             sys_libdir->s_name);

    if (homedir)
        snprintf(user_prefs_file, FILENAME_MAX, "%s/" USER_CONFIG_DIR "/user.settings", homedir);
    if (stat(user_prefs_file, &statbuf) == 0) 
        strncpy(filenamebuf, user_prefs_file, FILENAME_MAX);
    else if (stat(default_prefs_file, &statbuf) == 0)
        strncpy(filenamebuf, default_prefs_file, FILENAME_MAX);
    else
        return;
    filenamebuf[FILENAME_MAX-1] = 0;
    if ((fd = open(filenamebuf, 0)) < 0)
    {
        if (sys_verbose)
            perror(filenamebuf);
        return;
    }
    length = lseek(fd, 0, 2);
    if (length < 0)
    {
        if (sys_verbose)
            perror(filenamebuf);
        close(fd);
        return;
    }
    lseek(fd, 0, 0);
    if (!(sys_prefbuf = malloc(length + 2)))
    {
        error("couldn't allocate memory for preferences buffer");
        close(fd);
        return;
    }
    sys_prefbuf[0] = '\n';
    if (read(fd, sys_prefbuf+1, length) < length)
    {
        perror(filenamebuf);
        sys_prefbuf[0] = 0;
        close(fd);
        return;
    }
    sys_prefbuf[length+1] = 0;
    close(fd);
    if (sys_verbose)
        post("success reading preferences from: %s", filenamebuf);
}

static int sys_getpreference(const char *key, char *value, int size)
{
    char searchfor[80], *where, *whereend;
    if (!sys_prefbuf)
        return (0);
    sprintf(searchfor, "\n%s:", key);
    where = strstr(sys_prefbuf, searchfor);
    if (!where)
        return (0);
    where += strlen(searchfor);
    while (*where == ' ' || *where == '\t')
        where++;
    for (whereend = where; *whereend && *whereend != '\n'; whereend++)
        ;
    if (*whereend == '\n')
        whereend--;
    if (whereend > where + size - 1)
        whereend = where + size - 1;
    strncpy(value, where, whereend+1-where);
    value[whereend+1-where] = 0;
    return (1);
}

static void sys_doneloadpreferences( void)
{
    if (sys_prefbuf)
        free(sys_prefbuf);
}

static FILE *sys_prefsavefp;

static void sys_initsavepreferences( void)
{
    char filenamebuf[FILENAME_MAX], *homedir = getenv("HOME");
    struct stat statbuf;

    if (!homedir)
        return;
    snprintf(filenamebuf, FILENAME_MAX, "%s/" USER_CONFIG_DIR, homedir);
    filenamebuf[FILENAME_MAX-1] = 0;
    if (stat(filenamebuf, &statbuf) || !S_ISDIR(statbuf.st_mode)) {
      // user config dir doesn't exist yet, try to create it
      if (mkdir(filenamebuf, 0755)) {
        pd_error(0, "%s: %s",filenamebuf, strerror(errno));
        return;
      }
    }
    snprintf(filenamebuf, FILENAME_MAX, "%s/" USER_CONFIG_DIR "/user.settings", homedir);
    filenamebuf[FILENAME_MAX-1] = 0;
    if ((sys_prefsavefp = fopen(filenamebuf, "w")) == NULL)
    {
        //snprintf(errbuf, FILENAME_MAX, "%s: %s",filenamebuf, strerror(errno));
        pd_error(0, "%s: %s",filenamebuf, strerror(errno));
    }
}

static void sys_putpreference(const char *key, const char *value)
{
    if (sys_prefsavefp)
        fprintf(sys_prefsavefp, "%s: %s\n",
            key, value);
}

static void sys_donesavepreferences( void)
{
    if (sys_prefsavefp)
    {
        fclose(sys_prefsavefp);
        sys_prefsavefp = 0;
    }
}

#endif /* UNIX */

#ifdef MSW

static void sys_initloadpreferences( void)
{
}

static int sys_getpreference(const char *key, char *value, int size)
{
    HKEY hkey;
    DWORD bigsize = size;
    LONG err = RegOpenKeyEx(HKEY_CURRENT_USER,
        "Software\\Purr-Data", 0,  KEY_QUERY_VALUE, &hkey);
    if (err != ERROR_SUCCESS)
    {
        err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            "Software\\Purr-Data", 0,  KEY_QUERY_VALUE, &hkey);
        if (err != ERROR_SUCCESS)
        {
            return (0);
	}
    }
    err = RegQueryValueEx(hkey, key, 0, 0, value, &bigsize);
    if (err != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return (0);
    }
    RegCloseKey(hkey);
    return (1);
}

static void sys_doneloadpreferences( void)
{
}

static void sys_initsavepreferences( void)
{
}

static void sys_putpreference(const char *key, const char *value)
{
    HKEY hkey;
    LONG err = RegCreateKeyEx(HKEY_CURRENT_USER,
        "Software\\Purr-Data", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
        NULL, &hkey, NULL);
    if (err != ERROR_SUCCESS)
    {
        post("unable to create registry entry: %s: error %lx\n", key,
            (long unsigned int)err);
        return;
    }
    err = RegSetValueEx(hkey, key, 0, REG_EXPAND_SZ, value, strlen(value)+1);
    if (err != ERROR_SUCCESS)
        post("unable to set registry entry: %s\n", key);
    RegCloseKey(hkey);
}

static void sys_donesavepreferences( void)
{
}

#endif /* MSW */

#ifdef __APPLE__

// prefs file that is currently the one to save to
static char current_prefs[FILENAME_MAX] = "org.puredata.pd-l2ork"; 

static char *sys_prefbuf;

// Maximum number of bytes to be read on each iteration. Note that the buffer
// is resized in increments of BUFSZ until the entire prefs data has been
// read. For best performance, BUFSZ should be a sizeable fraction of the
// expected preference data size. The size 4096 matches PD's internal GUI
// socket size and thus should normally be enough to read the entire prefs
// data in one go.
#define BUFSZ 4096

// AG: We have to go to some lengths here since 'defaults read' doesn't
// properly deal with UTF-8 characters in the prefs data. 'plutil' does the
// trick, however, so we use that to read the entire prefs data at once from a
// pipe, converting it to JSON format which can then be translated to Pd's
// Unix preferences file format using sed. The result is stored in a character
// buffer for efficient access. From there we can retrieve the individual keys
// in the same fashion as on Unix. A welcome side effect is that loading the
// prefs is *much* faster now than with the previous method which invoked
// 'defaults read' on each individual key.

static int save_prefs_later = 0;

static void sys_initloadpreferences(void)
{
    char cmdbuf[MAXPDSTRING], *buf;
    FILE *fp;
    size_t sz, n = 0;
    int res;
    char default_prefs[FILENAME_MAX];  // default prefs embedded in the package
    char embedded_prefs[FILENAME_MAX]; // overrides others for standalone app
    char user_prefs[FILENAME_MAX];     // user preferences
    char embedded_prefs_file[FILENAME_MAX];
    char user_prefs_file[FILENAME_MAX];
    const char *prefs, *homedir = getenv("HOME");
    struct stat statbuf;
    // On the Mac, we first look for an embedded prefs file, then for a user
    // prefs file, and fall back to the defaults in the package if none of
    // these exist. Note that Pd-l2ork can't create standalone apps à la
    // Pd-extended right now, but we might want to support them in the future,
    // so we handle the embedded prefs case anyway.
    snprintf(default_prefs, FILENAME_MAX,
	     "%s/../org.puredata.pd-l2ork.default",
             sys_libdir->s_name);
    snprintf(embedded_prefs, FILENAME_MAX,
	     "%s/../org.puredata.pd-l2ork",
             sys_libdir->s_name);
    snprintf(user_prefs, FILENAME_MAX,
             "%s/Library/Preferences/org.puredata.pd-l2ork", homedir);
    snprintf(embedded_prefs_file, FILENAME_MAX, "%s.plist", embedded_prefs);
    snprintf(user_prefs_file, FILENAME_MAX, "%s.plist", user_prefs);
    if (stat(embedded_prefs_file, &statbuf) == 0) {
      // Read from and write to the embedded prefs (standalone app).
      prefs = embedded_prefs;
      strncpy(current_prefs, embedded_prefs, FILENAME_MAX);
    } else if (stat(user_prefs_file, &statbuf) == 0) {
      // Read from and write to the user prefs.
      prefs = current_prefs;
      strncpy(current_prefs, user_prefs, FILENAME_MAX);
    } else {
      // Read from the package defaults and write to the user prefs.
      prefs = default_prefs;
      strncpy(current_prefs, user_prefs, FILENAME_MAX);
      // AG: Remember to save the prefs later after we loaded them (see below).
      save_prefs_later = 1;
    }
    // This looks complicated, but is rather straightforward. The individual
    // stages of the pipe are:
    // 1. plutil -convert json -r -o -: grab our defaults and convert to JSON
    // 2. sed: a few edits remove the extra JSON bits (curly braces, string
    //    quotes, unwanted whitespace and character escapes) and produce
    //    Pd-L2Ork's Unix prefs format, i.e.:
    // JSON                        -->            Unix prefs
    // {
    //   "nloadlib" : "33",                       nloadlib: 33
    //   "loadlib1" : "libdir",                   loadlib1: libdir
    //   "path1" : "\/System\/Library\/Fonts"     path1: /System/Library/Fonts
    // }
    snprintf(cmdbuf, MAXPDSTRING,
        "plutil -convert json -r -o - \"%s.plist\" "
        "| sed -E "
          "-e 's/[{}]//g' "
          "-e 's/^ *\"(([^\"]|\\\\.)*)\" *: *\"(([^\"]|\\\\.)*)\".*/\\1: \\3/' "
          "-e 's/\\\\(.)/\\1/g'",
        prefs);
    // open the pipe
    fp = popen(cmdbuf, "r");
    if (!fp) {
      // if opening the pipe failed for some reason, bail out now
      if (sys_verbose)
        perror(current_prefs);
      error("%s: %s", current_prefs, strerror(errno));
      return;
    }
    // Initialize the buffer. Note that we have to reserve one extra byte for
    // the terminating NUL character. The buf variable always points to the
    // current chunk of memory to be written into.
    sys_prefbuf = buf = malloc((sz = BUFSZ)+1);
    while (buf && (n = fread(buf, 1, BUFSZ, fp)) > 0) {
      char *newbuf;
      size_t oldsz = sz;
      // terminating NUL byte, to be safe
      buf[n] = 0;
      // if the byte count is short, then all data has been read; bail out
      if (n < BUFSZ) break;
      // more data may follow, enlarge the buffer in BUFSZ increments
      sz += BUFSZ;
      if ((newbuf = realloc(sys_prefbuf, sz+1))) {
        // memory allocation succeeded, prepare the new buffer for the next read
        sys_prefbuf = newbuf;
        // adjust the current buffer pointer
        buf = newbuf + oldsz;
      } else {
        // memory allocation failed, bail out
        buf = NULL;
      }
    }
    // close the pipe
    res = pclose(fp);
    if (res)
      post("%s: pclose returned exit status %d", current_prefs, WEXITSTATUS(res));
    // check for memory allocation errors
    if (!buf) {
      error("couldn't allocate memory for preferences buffer");
      return;
    }
    // When we come here, n is the length of the last chunk we read into buf.
    // Add the terminating NUL byte there.
    buf[n] = 0;
    if (sys_verbose)
      post("success reading preferences from: %s", current_prefs);
    //post("%s: read %d bytes of preferences data", current_prefs, strlen(sys_prefbuf));
}

static int sys_getpreference(const char *key, char *value, int size)
{
    char searchfor[80], *where, *whereend;
    if (!sys_prefbuf)
        return (0);
    sprintf(searchfor, "\n%s:", key);
    where = strstr(sys_prefbuf, searchfor);
    if (!where)
        return (0);
    where += strlen(searchfor);
    while (*where == ' ' || *where == '\t')
        where++;
    for (whereend = where; *whereend && *whereend != '\n'; whereend++)
        ;
    if (*whereend == '\n')
        whereend--;
    if (whereend > where + size - 1)
        whereend = where + size - 1;
    strncpy(value, where, whereend+1-where);
    value[whereend+1-where] = 0;
    return (1);
}

static void sys_doneloadpreferences( void)
{
    if (sys_prefbuf)
        free(sys_prefbuf);
    sys_prefbuf = NULL;
    if (save_prefs_later) {
      // AG: We need to save the default prefs to the user prefs at this point
      // in order to avoid losing them, in case the recent file list is written
      // without first saving the defaults (fixes #339).
      extern void glob_savepreferences(t_pd *dummy);
      glob_savepreferences(NULL);
      save_prefs_later = 0;
    }
}

// AG: We use a similar approach here to import the data into the defaults
// storage in one go. To these ends, a temporary plist file in xml format is
// created which is then submitted to 'defaults import'. This is *much* faster
// than the previous implementation which invoked the shell to run 'defaults
// write' for each individual key.

#define save_prefs_template "/tmp/pd-l2ork.defaults.plist.XXXXXX"

static FILE *save_fp;
static char save_prefs[] = save_prefs_template;

static void sys_initsavepreferences( void)
{
  strcpy(save_prefs, save_prefs_template);
  int fd = mkstemp(save_prefs);
  if (fd < 0) {
    error("save preferences: %s", strerror(errno));
    return;
  }
  save_fp = fdopen(fd, "w");
  if (!save_fp) {
    error("save preferences: %s", strerror(errno));
    return;
  }
  fprintf(save_fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n");
}

static void sys_putpreference(const char *key, const char *value)
{
  if (!save_fp) return;
  fprintf(save_fp,
          "<key>%s</key>\n<string>%s</string>\n",
          key, value);
}

static void sys_donesavepreferences( void)
{
  if (!save_fp) return;
  fprintf(save_fp, "</dict>\n</plist>\n");
  fclose(save_fp);
  save_fp = 0;
  char cmdbuf[MAXPDSTRING];
  snprintf(cmdbuf, MAXPDSTRING,
           "defaults import '%s' '%s' 2> /dev/null",
           current_prefs, save_prefs);
  system(cmdbuf);
  unlink(save_prefs);
}

#endif /* __APPLE__ */


#ifdef _WIN32
static int check_exists(const char*path)
{
    char pathbuf[MAXPDSTRING];
    wchar_t ucs2path[MAXPDSTRING];
    sys_bashfilename(path, pathbuf);
    u8_utf8toucs2(ucs2path, MAXPDSTRING, pathbuf, MAXPDSTRING-1);
    return (0 ==  _waccess(ucs2path, 0));
}
#else
#include <unistd.h>
static int check_exists(const char*path)
{
    char pathbuf[MAXPDSTRING];
    sys_bashfilename(path, pathbuf);
    return (0 == access(pathbuf, 0));
}
#endif

extern void sys_expandpathelems(const char *name, char *result);

void sys_loadpreferences( void)
{
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int nmidiindev, midiindev[MAXMIDIINDEV];
    int nmidioutdev, midioutdev[MAXMIDIOUTDEV];
    int i, rate = 0, advance = -1, callback = 0, blocksize = 0,
        api, maxi;
    char prefbuf[MAXPDSTRING], keybuf[80];

    sys_initloadpreferences();
        /* load audio preferences */
    if (sys_getpreference("audioapi", prefbuf, MAXPDSTRING)
        && sscanf(prefbuf, "%d", &api) > 0)
            sys_set_audio_api(api);
            /* JMZ/MB: brackets for initializing */
    if (sys_getpreference("noaudioin", prefbuf, MAXPDSTRING) &&
        (!strcmp(prefbuf, ".") || !strcmp(prefbuf, "True")))
            naudioindev = 0;
    else
    {
        /* AG: naudioin key */
        /* The idea here is to keep track of the actual number of devices, so
           that we don't read stale device entries which were removed long ago
           but are still present in the defaults/registry on Mac and Windows.
           (This bug is in all other Pd versions out there. It only affects
           Mac and Windows, since the config information on Linux is kept in a
           file which is overwritten each time the config information is
           saved, so there are never any stale device entries in it.)
           Our code is fully compatible with config info from other Pd
           versions, i.e., we fall back to the default behavior of looking for
           up to MAXAUDIOINDEV devices if the naudioin key isn't set, and the
           naudioin key will just be ignored if our config happens to be read
           by other Pd versions. Audio outputs and MIDI inputs/outputs are
           handled in exactly the same fashion below. */
        int n;
        if (!(sys_getpreference("naudioin", prefbuf, MAXPDSTRING)
              && sscanf(prefbuf, "%d", &n) > 0))
            n = MAXAUDIOINDEV;
        for (i = 0, naudioindev = 0; i < n; i++)
        {
            sprintf(keybuf, "audioindev%d", i+1);
            if (!sys_getpreference(keybuf, prefbuf, MAXPDSTRING))
                break;
            if (sscanf(prefbuf, "%d %d", &audioindev[i], &chindev[i]) < 2)
                break;
            naudioindev++;
        }
            /* if no preferences at all, set -1 for default behavior */
        if (naudioindev == 0)
            naudioindev = -1;
    }
        /* JMZ/MB: brackets for initializing */
    if (sys_getpreference("noaudioout", prefbuf, MAXPDSTRING) &&
        (!strcmp(prefbuf, ".") || !strcmp(prefbuf, "True")))
            naudiooutdev = 0;
    else
    {
        /* AG: naudioout key */
        int n;
        if (!(sys_getpreference("naudioout", prefbuf, MAXPDSTRING)
              && sscanf(prefbuf, "%d", &n) > 0))
            n = MAXAUDIOOUTDEV;
        for (i = 0, naudiooutdev = 0; i < n; i++)
        {
            sprintf(keybuf, "audiooutdev%d", i+1);
            if (!sys_getpreference(keybuf, prefbuf, MAXPDSTRING))
                break;
            if (sscanf(prefbuf, "%d %d", &audiooutdev[i], &choutdev[i]) < 2)
                break;
            naudiooutdev++;
        }
        if (naudiooutdev == 0)
            naudiooutdev = -1;
    }
    if (sys_getpreference("rate", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &rate);
    if (sys_getpreference("audiobuf", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &advance);
    if (sys_getpreference("callback", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &callback);
    if (sys_getpreference("blocksize", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &blocksize);
    sys_set_audio_settings(naudioindev, audioindev, naudioindev, chindev,
        naudiooutdev, audiooutdev, naudiooutdev, choutdev, rate, advance,
        callback, blocksize);
        
        /* load MIDI preferences */
    if (sys_getpreference("midiapi", prefbuf, MAXPDSTRING)
        && sscanf(prefbuf, "%d", &api) > 0)
            sys_set_midi_api(api);
        /* JMZ/MB: brackets for initializing */
    if (sys_getpreference("nomidiin", prefbuf, MAXPDSTRING) &&
        (!strcmp(prefbuf, ".") || !strcmp(prefbuf, "True")))
            nmidiindev = 0;
    else
    {
        /* AG: nmidiin key */
        int n;
        if (!(sys_getpreference("nmidiin", prefbuf, MAXPDSTRING)
              && sscanf(prefbuf, "%d", &n) > 0))
            n = MAXMIDIINDEV;
        for (i = 0, nmidiindev = 0; i < n; i++)
        {
            sprintf(keybuf, "midiindev%d", i+1);
            if (!sys_getpreference(keybuf, prefbuf, MAXPDSTRING))
                break;
            if (sscanf(prefbuf, "%d", &midiindev[i]) < 1)
                break;
            nmidiindev++;
        }
    }
        /* JMZ/MB: brackets for initializing */
    if (sys_getpreference("nomidiout", prefbuf, MAXPDSTRING) &&
        (!strcmp(prefbuf, ".") || !strcmp(prefbuf, "True")))
            nmidioutdev = 0;
    else
    {
        /* AG: nmidiout key */
        int n;
        if (!(sys_getpreference("nmidiout", prefbuf, MAXPDSTRING)
              && sscanf(prefbuf, "%d", &n) > 0))
            n = MAXMIDIOUTDEV;
        for (i = 0, nmidioutdev = 0; i < n; i++)
        {
            sprintf(keybuf, "midioutdev%d", i+1);
            if (!sys_getpreference(keybuf, prefbuf, MAXPDSTRING))
                break;
            if (sscanf(prefbuf, "%d", &midioutdev[i]) < 1)
                break;
            nmidioutdev++;
        }
    }
    sys_open_midi(nmidiindev, midiindev, nmidioutdev, midioutdev, 0);

        /* search path */
    if (sys_getpreference("npath", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &maxi);
    else maxi = 0x7fffffff;
    for (i = 0; i<maxi; i++)
    {
        sprintf(keybuf, "path%d", i+1);
        if (!sys_getpreference(keybuf, prefbuf, MAXPDSTRING))
            break;
        else {
            // AG: need to expand ~ et al here
            char final_name[FILENAME_MAX];
            sys_expandpathelems(prefbuf, final_name);
            // AG: ignore non-existent paths
            if (!check_exists(final_name))
                continue;
	}
        sys_searchpath = namelist_append_files(sys_searchpath, prefbuf);
    }
    if (sys_getpreference("standardpath", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &sys_usestdpath);
    if (sys_getpreference("verbose", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &sys_verbose);

        /* startup settings */
    if (sys_getpreference("nloadlib", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &maxi);
    else maxi = 0x7fffffff;
    for (i = 0; i<maxi; i++)
    {
        sprintf(keybuf, "loadlib%d", i+1);
        if (!sys_getpreference(keybuf, prefbuf, MAXPDSTRING))
            break;
        sys_externlist = namelist_append_files(sys_externlist, prefbuf);
    }
    if (sys_getpreference("defeatrt", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &sys_defeatrt);
    if (sys_getpreference("savezoom", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &sys_zoom);
    if (sys_getpreference("browser_doc", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &sys_browser_doc);
    if (sys_getpreference("browser_path", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &sys_browser_path);
    if (sys_getpreference("browser_init", prefbuf, MAXPDSTRING))
        sscanf(prefbuf, "%d", &sys_browser_init);
    if (sys_getpreference("guipreset", prefbuf, MAXPDSTRING))
    {
        char preset_buf[MAXPDSTRING];
        sscanf(prefbuf, "%s", preset_buf);
        sys_gui_preset = gensym(preset_buf);
    }
    if (sys_getpreference("flags", prefbuf, MAXPDSTRING))
    {
        if (strcmp(prefbuf, "."))
            sys_flags = gensym(prefbuf);
    }
    sys_doneloadpreferences();
    sys_doflags();

    if (sys_defeatrt)
        sys_hipriority = 0;
    else
#ifdef UNIX
        sys_hipriority = 1; //!geteuid();
#else
#ifdef MSW
        sys_hipriority = 0;
#else
        sys_hipriority = 1;
#endif
#endif
}

void glob_savepreferences(t_pd *dummy)
{
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int i, rate, advance, callback, blocksize;
    char buf1[MAXPDSTRING], buf2[MAXPDSTRING];
    int nmidiindev, midiindev[MAXMIDIINDEV];
    int nmidioutdev, midioutdev[MAXMIDIOUTDEV];

    sys_initsavepreferences();


        /* audio settings */
    sprintf(buf1, "%d", sys_audioapi);
    sys_putpreference("audioapi", buf1);

    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance, &callback,
            &blocksize);

    sys_putpreference("noaudioin", (naudioindev <= 0 ? "True" : "False"));
    /* AG: additional naudioin key, see the comments in sys_loadpreferences
       above for explanation */
    sprintf(buf1, "%d", naudioindev);
    sys_putpreference("naudioin", buf1);
    for (i = 0; i < naudioindev; i++)
    {
        sprintf(buf1, "audioindev%d", i+1);
        sprintf(buf2, "%d %d", audioindev[i], chindev[i]);
        sys_putpreference(buf1, buf2);
    }
    sys_putpreference("noaudioout", (naudiooutdev <= 0 ? "True" : "False"));
    /* AG: naudioout key */
    sprintf(buf1, "%d", naudiooutdev);
    sys_putpreference("naudioout", buf1);
    for (i = 0; i < naudiooutdev; i++)
    {
        sprintf(buf1, "audiooutdev%d", i+1);
        sprintf(buf2, "%d %d", audiooutdev[i], choutdev[i]);
        sys_putpreference(buf1, buf2);
   }

    sprintf(buf1, "%d", advance);
    sys_putpreference("audiobuf", buf1);

    sprintf(buf1, "%d", rate);
    sys_putpreference("rate", buf1);

    sprintf(buf1, "%d", callback);
    sys_putpreference("callback", buf1);

    sprintf(buf1, "%d", blocksize);
    sys_putpreference("blocksize", buf1);

        /* MIDI settings */
    sprintf(buf1, "%d", sys_midiapi);
    sys_putpreference("midiapi", buf1);

    sys_get_midi_params(&nmidiindev, midiindev, &nmidioutdev, midioutdev);
    sys_putpreference("nomidiin", (nmidiindev <= 0 ? "True" : "False"));
    /* AG: nmidiin */
    sprintf(buf1, "%d", nmidiindev);
    sys_putpreference("nmidiin", buf1);
    for (i = 0; i < nmidiindev; i++)
    {
        sprintf(buf1, "midiindev%d", i+1);
        sprintf(buf2, "%d", midiindev[i]);
        sys_putpreference(buf1, buf2);
    }
    sys_putpreference("nomidiout", (nmidioutdev <= 0 ? "True" : "False"));
    /* AG: nmidiout */
    sprintf(buf1, "%d", nmidioutdev);
    sys_putpreference("nmidiout", buf1);
    for (i = 0; i < nmidioutdev; i++)
    {
        sprintf(buf1, "midioutdev%d", i+1);
        sprintf(buf2, "%d", midioutdev[i]);
        sys_putpreference(buf1, buf2);
    }
        /* file search path */

    for (i = 0; 1; i++)
    {
        char *pathelem = namelist_get(sys_searchpath, i);
        if (!pathelem)
            break;
        sprintf(buf1, "path%d", i+1);
        sys_putpreference(buf1, pathelem);
    }
    sprintf(buf1, "%d", i);
    sys_putpreference("npath", buf1);
    sprintf(buf1, "%d", sys_usestdpath);
    sys_putpreference("standardpath", buf1);
    sprintf(buf1, "%d", sys_verbose);
    sys_putpreference("verbose", buf1);
    
        /* startup */
    for (i = 0; 1; i++)
    {
        char *pathelem = namelist_get(sys_externlist, i);
        if (!pathelem)
            break;
        sprintf(buf1, "loadlib%d", i+1);
        sys_putpreference(buf1, pathelem);
    }
    sprintf(buf1, "%d", i);
    sys_putpreference("nloadlib", buf1);
    sprintf(buf1, "%d", sys_defeatrt);
    sys_putpreference("defeatrt", buf1);
    sprintf(buf1, "%d", sys_zoom);
    sys_putpreference("savezoom", buf1);
    sprintf(buf1, "%d", sys_browser_doc);
    sys_putpreference("browser_doc", buf1);
    sprintf(buf1, "%d", sys_browser_path);
    sys_putpreference("browser_path", buf1);
    sprintf(buf1, "%d", sys_browser_init);
    sys_putpreference("browser_init", buf1);
    sys_putpreference("guipreset", sys_gui_preset->s_name);
    sys_putpreference("flags", 
        (sys_flags ? sys_flags->s_name : ""));
    sys_donesavepreferences();
    
}

/* AG: Recent files table */

int sys_n_recent_files = 0;
char *sys_recent_files[MAX_RECENT_FILES];

void sys_add_recent_file(const char *s)
{
  int i;
  // only add the file if it actually exists
  if (!check_exists(s)) return;
  for (i = 0; i < sys_n_recent_files && strcmp(sys_recent_files[i], s); i++) ;
  if (i < sys_n_recent_files) {
    // already got an existing entry, move it to the front
    char *t = sys_recent_files[i];
    memmove(sys_recent_files+1, sys_recent_files, i*sizeof(char*));
    sys_recent_files[0] = t;
  } else {
    char *t = strdup(s);
    if (!t) return;
    if (sys_n_recent_files == MAX_RECENT_FILES) {
      // kick out the oldest entry to make room for a new one
      free(sys_recent_files[--sys_n_recent_files]);
    }
    // add a new entry at the beginning of the table
    memmove(sys_recent_files+1, sys_recent_files,
            sys_n_recent_files*sizeof(char*));
    sys_recent_files[0] = t;
    sys_n_recent_files++;
  }
}

void sys_save_recent_files(void)
{
  int i;
#ifdef UNIX
  // UNIX/Linux: save in recent_files file
  FILE *fp;
  char filenamebuf[FILENAME_MAX], *homedir = getenv("HOME");
  struct stat statbuf;
  if (!homedir) return;
  snprintf(filenamebuf, FILENAME_MAX, "%s/" USER_CONFIG_DIR, homedir);
  filenamebuf[FILENAME_MAX-1] = 0;
  if (stat(filenamebuf, &statbuf) || !S_ISDIR(statbuf.st_mode)) {
    // user config dir doesn't exist yet, try to create it
    if (mkdir(filenamebuf, 0755)) {
      pd_error(0, "%s: %s",filenamebuf, strerror(errno));
      return;
    }
  }
  snprintf(filenamebuf, FILENAME_MAX, "%s/" USER_CONFIG_DIR "/recent_files", homedir);
  filenamebuf[FILENAME_MAX-1] = 0;
  if ((fp = fopen(filenamebuf, "w")) == NULL) {
    pd_error(0, "%s: %s",filenamebuf, strerror(errno));
    return;
  }
  for (i = 0; i < sys_n_recent_files; i++) {
    fprintf(fp, "%s\n", sys_recent_files[i]);
  }
  fclose(fp);
#else
  // Mac/Windows (use the defaults/registry)
  char buf[MAXPDSTRING];
  sys_initsavepreferences();
  for (i = 0; i < sys_n_recent_files; i++) {
    sprintf(buf, "recent%d", i+1);
    sys_putpreference(buf, sys_recent_files[i]);
  }
  sprintf(buf, "%d", i);
  sys_putpreference("nrecent", buf);
  sys_donesavepreferences();
#endif
}

void sys_load_recent_files(void)
{
#ifdef UNIX
  // UNIX/Linux: load from recent_files file
  FILE *fp;
  char filenamebuf[FILENAME_MAX], *homedir = getenv("HOME");
  if (!homedir) return;
  snprintf(filenamebuf, FILENAME_MAX, "%s/" USER_CONFIG_DIR "/recent_files", homedir);
  filenamebuf[FILENAME_MAX-1] = 0;
  if ((fp = fopen(filenamebuf, "r")) == NULL) return;
  for (sys_n_recent_files = 0; sys_n_recent_files < MAX_RECENT_FILES &&
         fgets(filenamebuf, FILENAME_MAX, fp); ) {
    char *s;
    int l = strlen(filenamebuf);
    if (l > 0 && filenamebuf[l-1] == '\n') filenamebuf[--l] = 0;
    // only add files which actually exist
    if (l == 0 || !check_exists(filenamebuf)) continue;
    s = strdup(filenamebuf);
    if (s) sys_recent_files[sys_n_recent_files++] = s;
  }
  fclose(fp);
#else
  // Mac/Windows (use the defaults/registry)
  char prefbuf[MAXPDSTRING], keybuf[80];
  int i, maxi = MAX_RECENT_FILES;
  sys_initloadpreferences();
  if (sys_getpreference("nrecent", prefbuf, MAXPDSTRING))
    sscanf(prefbuf, "%d", &maxi);
  for (i = 0; i < maxi; i++) {
    int l;
    char *s;
    sprintf(keybuf, "recent%d", i+1);
    if (!sys_getpreference(keybuf, prefbuf, MAXPDSTRING))
      break;
    l = strlen(prefbuf);
    if (l == 0 || !check_exists(prefbuf)) continue;
    s = strdup(prefbuf);
    if (s) sys_recent_files[sys_n_recent_files++] = s;
  }
  sys_doneloadpreferences();
#endif
}

void sys_clear_recent_files(void)
{
  int i;
  for (i = 0; i < sys_n_recent_files; i++) {
    free(sys_recent_files[i]);
  }
  sys_n_recent_files = 0;
}

// send the recent files list back to the gui so that the Recent Files menu
// can be updated accordingly
void glob_recent_files(t_pd *dummy)
{
    int i;
    gui_start_vmess("gui_recent_files", "x", dummy);
    gui_start_array();
    for (i = 0; i < sys_n_recent_files; i++)
    {
        gui_s(sys_recent_files[i]);
    }
    gui_end_array();
    gui_end_vmess();
}

// add an entry to the recent files list, save the list and update the gui
void glob_add_recent_file(t_pd *dummy, t_symbol *s)
{
    sys_add_recent_file(s->s_name);
    sys_save_recent_files();
    glob_recent_files(dummy);
}

// clear the recent files list, save the list and update the gui
void glob_clear_recent_files(t_pd *dummy)
{
    sys_clear_recent_files();
    sys_save_recent_files();
    glob_recent_files(dummy);
}
