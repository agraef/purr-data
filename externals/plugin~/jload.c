/* load.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/* patched by Jarno Seppänen, jams@cs.tut.fi, for plugin~ */

/*****************************************************************************/

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "ladspa.h"
#include "jutils.h"

/*****************************************************************************/

/* This function provides a wrapping of dlopen(). When the filename is
   not an absolute path (i.e. does not begin with / character), this
   routine will search the LADSPA_PATH for the file. */
static void *
dlopenLADSPA(const char * pcFilename, int iFlag) {

  char * pcBuffer;
  const char * pcEnd;
  const char * pcLADSPAPath;
  const char * pcStart;
  int iEndsInSO;
  int iNeedSlash;
  size_t iFilenameLength;
  void * pvResult;

  iFilenameLength = strlen(pcFilename);
  pvResult = NULL;

  /* First we just try calling dlopen(). This works if the user knows
     about dlopen() and has placed the file on the LD_LIBRARY path or
     has used an absolute directory. */
  pvResult = dlopen(pcFilename, iFlag);
  if (pvResult != NULL)
    return pvResult;

  /* If the filename is not absolute then we wish to check along the
     LADSPA_PATH path to see if we can find the file there. */
  if (pcFilename[0] != '/') {

    pcLADSPAPath = getenv("LADSPA_PATH");

    if (pcLADSPAPath) {

      pcStart = pcLADSPAPath;
      while (*pcStart != '\0') {
	pcEnd = pcStart;
	while (*pcEnd != ':' && *pcEnd != '\0')
	  pcEnd++;
	
	pcBuffer = malloc(iFilenameLength + 2 + (pcEnd - pcStart));
	if (pcEnd > pcStart)
	  strncpy(pcBuffer, pcStart, pcEnd - pcStart);
	iNeedSlash = 0;
	if (pcEnd > pcStart)
	  if (*(pcEnd - 1) != '/') {
	    iNeedSlash = 1;
	    pcBuffer[pcEnd - pcStart] = '/';
	  }
	strcpy(pcBuffer + iNeedSlash + (pcEnd - pcStart), pcFilename);
	
	pvResult = dlopen(pcBuffer, iFlag);
	
	free (pcBuffer);
	if (pvResult != NULL)
	  return pvResult;
	
	pcStart = pcEnd;
	if (*pcStart == ':')
	  pcStart++;
      }
    } else {
	fputs ("warning: You haven't specified the LADSPA_PATH environment variable and didn't specify an absolute path to the plug-in.\n"
	       "Please set the LADSPA_PATH variable to point to your LADSPA plug-in directories (eg. \"export LADSPA_PATH=/usr/local/lib/ladspa\").\n", stderr);
    }
  }

  /* As a last ditch effort, check if filename does not end with
     ".so". In this case, add this suffix and recurse. */
  iEndsInSO = 0;
  if (iFilenameLength > 3)
    iEndsInSO = (strcmp(pcFilename + iFilenameLength - 3, ".so") == 0);
  if (!iEndsInSO) {
    pcBuffer = malloc(iFilenameLength + 4);
    strcpy(pcBuffer, pcFilename);
    strcat(pcBuffer, ".so");
    pvResult = dlopenLADSPA(pcBuffer, iFlag);
    free(pcBuffer);
  }

  if (pvResult != NULL)
    return pvResult;

  /* If nothing has worked, then at least we can make sure we set the
     correct error message - and this should correspond to a call to
     dlopen() with the actual filename requested. The dlopen() manual
     page does not specify whether the first or last error message
     will be kept when multiple calls are made to dlopen(). We've
     covered the former case - now we can handle the latter by calling
     dlopen() again here. */
  return dlopen(pcFilename, iFlag);
}

/*****************************************************************************/

void *
loadLADSPAPluginLibrary(const char * pcPluginFilename) {

  void * pvPluginHandle;

  pvPluginHandle = dlopenLADSPA(pcPluginFilename, RTLD_NOW);
  if (!pvPluginHandle) {
    fprintf(stderr, 
	    "Failed to load plugin \"%s\": %s\n", 
	    pcPluginFilename,
	    dlerror());
#if 0
    exit(1);
#else
    return NULL;
#endif
  }

  return pvPluginHandle;
}

/*****************************************************************************/

void 
unloadLADSPAPluginLibrary(void * pvLADSPAPluginLibrary) {
  dlclose(pvLADSPAPluginLibrary);
}

/*****************************************************************************/

const LADSPA_Descriptor *
findLADSPAPluginDescriptor(void * pvLADSPAPluginLibrary,
			   const char * pcPluginLibraryFilename,
			   const char * pcPluginLabel) {

  const LADSPA_Descriptor * psDescriptor;
  LADSPA_Descriptor_Function pfDescriptorFunction;
  unsigned long lPluginIndex;

  dlerror();
  pfDescriptorFunction 
    = (LADSPA_Descriptor_Function)dlsym(pvLADSPAPluginLibrary,
					"ladspa_descriptor");
  if (!pfDescriptorFunction) {
    const char * pcError = dlerror();
    if (pcError) 
      fprintf(stderr,
	      "Unable to find ladspa_descriptor() function in plugin "
	      "library file \"%s\": %s.\n"
	      "Are you sure this is a LADSPA plugin file?\n", 
	      pcPluginLibraryFilename,
	      pcError);
#if 0
    exit(1);
#else
    return NULL;
#endif
  }

  for (lPluginIndex = 0;; lPluginIndex++) {
    psDescriptor = pfDescriptorFunction(lPluginIndex);
    if (psDescriptor == NULL) {
      fprintf(stderr,
	      "Unable to find label \"%s\" in plugin library file \"%s\".\n",
	      pcPluginLabel,
	      pcPluginLibraryFilename);
#if 0
      exit(1);
#else
      return NULL;
#endif
    }
    if (strcmp(psDescriptor->Label, pcPluginLabel) == 0)
      return psDescriptor;
  }
}

/*****************************************************************************/

/* EOF */
