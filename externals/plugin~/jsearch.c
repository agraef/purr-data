/* search.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/* patched by Jarno Seppänen, jams@cs.tut.fi, for plugin~ */

/*****************************************************************************/

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*****************************************************************************/

#include "ladspa.h"
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
  LADSPA_Descriptor_Function fDescriptorFunction;
  long lDirLength;
  long iNeedSlash;
  struct dirent * psDirectoryEntry;
  void * pvPluginHandle;

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
      fDescriptorFunction
        = (LADSPA_Descriptor_Function)dlsym(pvPluginHandle,
                                            "ladspa_descriptor");
      if (dlerror() == NULL && fDescriptorFunction) {
        /* We've successfully found a ladspa_descriptor function. Pass
           it to the callback function. */
        fCallbackFunction(pcFilename,
                          pvPluginHandle,
                          fDescriptorFunction,
                          user_data);
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
  const char * pcStart;

  pcLADSPAPath = getenv("LADSPA_PATH");
  if (!pcLADSPAPath) {
    //fprintf(stderr, "Warning: no LADSPA_PATH, assuming /usr/lib/ladspa:/usr/local/lib/ladspa\n");
#ifdef __APPLE__
    pcLADSPAPath = "/sw/lib/ladspa:/usr/local/lib/ladspa";
#else
    pcLADSPAPath = "/usr/lib/ladspa:/usr/local/lib/ladspa";
#endif
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
