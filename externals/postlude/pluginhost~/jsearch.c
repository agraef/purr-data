/* search.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/* patched by Jarno Seppänen, jams@cs.tut.fi, for plugin~ */

/* patched by Jamie Bullock, jamie@postlude.co.uk, for dssi~ */

/*****************************************************************************/

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*****************************************************************************/

#include "dssi.h"
#include "jutils.h"

/*****************************************************************************/

/* Search just the one directory. */
    static void
LADSPADirectoryPluginSearch (const char * pcDirectory, 
        LADSPAPluginSearchCallbackFunction fCallbackFunction,
        void* user_data)
{
    char * pcFilename;
    DIR * psDirectory;
    DSSI_Descriptor_Function fDescriptorFunction;
    long lDirLength;
    long iNeedSlash;
    struct dirent * psDirectoryEntry;
    void * pvPluginHandle;
    bool is_DSSI = false;

    lDirLength = strlen(pcDirectory);
    if (!lDirLength)
        return;
    if (pcDirectory[lDirLength - 1] == '/')
        iNeedSlash = 0;
    else
        iNeedSlash = 1;

    psDirectory = opendir(pcDirectory);
    if (!psDirectory)
        return;

    while (1) {

        psDirectoryEntry = readdir(psDirectory);
        if (!psDirectoryEntry) {
            closedir(psDirectory);
            return;
        }

        pcFilename = malloc(lDirLength
                + strlen(psDirectoryEntry->d_name)
                + 1 + iNeedSlash);
        strcpy(pcFilename, pcDirectory);
        if (iNeedSlash)
            strcat(pcFilename, "/");
        strcat(pcFilename, psDirectoryEntry->d_name);

        pvPluginHandle = dlopen(pcFilename, RTLD_LAZY);
        if (pvPluginHandle) {
            /* This is a file and the file is a shared library! */

            dlerror();
            if((fDescriptorFunction = 
			(DSSI_Descriptor_Function)dlsym(pvPluginHandle,
                        "ladspa_descriptor"))) {
                is_DSSI = false;
	    } else if ((fDescriptorFunction = 
		    (DSSI_Descriptor_Function)dlsym(pvPluginHandle,
                        "dssi_descriptor"))) {
                is_DSSI = true;
	    }

            if (dlerror() == NULL && fDescriptorFunction) {
                /* We've successfully found a ladspa_descriptor function. Pass
                   it to the callback function. */
                fCallbackFunction(pcFilename,
                        pvPluginHandle,
                        fDescriptorFunction,
                        user_data,
                        is_DSSI);
                dlclose (pvPluginHandle);
            }
            else {
                /* It was a library, but not a LADSPA one. Unload it. */
                dlclose(pcFilename);
            }
        }
    }
}

/*****************************************************************************/

    void 
LADSPAPluginSearch(LADSPAPluginSearchCallbackFunction fCallbackFunction,
        void* user_data)
{

    char * pcBuffer;
    const char * pcEnd;
    const char * pcLADSPAPath;
    char *pluginPath;
    const char * pcStart;


    pcLADSPAPath = NULL;

    if(getenv("LADSPA_PATH") && getenv("DSSI_PATH")){ 
        pluginPath = malloc(sizeof(char) * 
                (strlen(getenv("LADSPA_PATH")) + 1) + 
                sizeof(char) * strlen(getenv("DSSI_PATH")));
        sprintf(pluginPath, "%s:%s", 
                getenv("LADSPA_PATH"), getenv("DSSI_PATH"));
        pcLADSPAPath = pluginPath;
        free(pluginPath);
    }
    if (pcLADSPAPath == NULL) {
        fprintf(stderr, "Warning: no LADSPA_PATH and DSSI_PATH, assuming /usr/lib/ladspa:/usr/local/lib/ladspa:/usr/lib/dssi:/usr/local/lib/dssi\n");
        pcLADSPAPath = 
            "/usr/lib/ladspa:/usr/local/lib/ladspa:/usr/lib/dssi:/usr/local/lib/dssi";
    }

    pcStart = pcLADSPAPath;
    while (*pcStart != '\0') {
        pcEnd = pcStart;
        while (*pcEnd != ':' && *pcEnd != '\0')
            pcEnd++;

        pcBuffer = malloc(1 + pcEnd - pcStart);
        if (pcEnd > pcStart)
            strncpy(pcBuffer, pcStart, pcEnd - pcStart);
        pcBuffer[pcEnd - pcStart] = '\0';

        LADSPADirectoryPluginSearch(pcBuffer, fCallbackFunction, user_data);

        pcStart = pcEnd;
        if (*pcStart == ':')
            pcStart++;
    }
}


/*****************************************************************************/

/* EOF */
