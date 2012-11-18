/******************************************************
 *
 * zexy - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1999:forum::für::umläute:2005
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/


/* ...this is a very ZEXY external ...
   so have fun
*/

#include "zexy.h"
#include "z_zexy.h"

#include <stdio.h>
#include <stdarg.h>


#ifdef __WIN32__
# define vsnprintf _vsnprintf
#endif

/* do a little help thing */

typedef struct zexy 
{
  t_object t_ob;
} t_zexy;

t_class *zexy_class;

static void zexy_help(void)
{
  endpost(); endpost(); 
  post("...this is the zexy %c external "VERSION"...", HEARTSYMBOL);
  endpost();
  post("%c handling signals", HEARTSYMBOL);
#if 0
  post("streamout~\t:: stream signals via a LAN : (%c) gige 1999");
  post("streamin~\t:: catch signals from a LAN : based on gige");
#endif
  post("sfplay\t\t:: play back a (multichannel) soundfile : (c) ritsch 1999");
  post("sfrecord\t:: record a (multichannel) soundfile : based on ritsch");

  endpost(); post("%c generating signals", HEARTSYMBOL);
  post("noish~\t\t:: generate bandlimited noise");
  post("noisi~\t\t:: generate bandlimited noise");
  post("dirac~\t\t:: generate a dirac-pulse");
  post("step~\t\t:: generate a unity-step");
  post("dfreq~\t\t:: detect frequency by counting zero-crossings : (c) ritsch 1998");

  endpost(); post("%c manipulating signals", HEARTSYMBOL);
  post("limiter~\t:: limit/compress one or more signals");
  post("nop~\t\t:: pass through a signal (delay 1 block)");
  post("z~\t\t:: samplewise delay");
  post("swap~\t\t:: byte-swap a signal");
  post("quantize~\t:: quantize a signal");

  endpost(); post("%c binary operations on signals", HEARTSYMBOL);
  post("abs~, sgn~, >~, <~, ==~, &&~, ||~");

  endpost(); post("%c multary operations on signals", HEARTSYMBOL);
  post("multiline~\t:: multiple line~ multiplication");
  post("multiplex~\t:: multiplex 1 inlet~ to 1-of-various outlet~s");
  post("demultiplex~\t:: demultiplex 1-of-various inlet~s to 1 outlet~");

  endpost(); post("%c investigating signals in message-domain", HEARTSYMBOL);
  post("pack~\t\t:: convert a signal into a list of floats");
  post("unpack~\t\t:: convert packages of floats into a signal");

  post("sigzero~\t:: indicates whether a signal is zero throughout the block");
  post("avg~\t\t:: outputs average of a signal as float");
  post("tavg~\t\t:: outputs average of a signal between two bangs");
  post("envrms~\t\t:: an env~-object that ouputs rms instead of db");
  post("pdf~\t\t:: power density function");
       
  endpost(); post("%c basic message objects", HEARTSYMBOL);
  post("nop\t\t:: a no-operation");
  post("lister\t\t:: stores lists");
  post("any2list\t\t:: converts \"anything\" to lists");
  post("list2int\t:: cast each float of a list to integer");
  post("atoi\t\t:: convert ascii to integer");
  post("list2symbol\t:: convert a list into a single symbol");
  post("symbol2list\t:: split a symbol into a list");
  post("strcmp\t\t:: compare 2 lists as if they where strings");
  post("repack\t\t:: (re)packs atoms to packages of a given size");
  post("packel\t\t:: element of a package");
  post("length\t\t:: length of a package");
  post("niagara\t\t:: divide a package into 2 sub-packages");
  post("glue\t\t:: append a list to another");
  post("repeat\t\t:: repeat a message");
  post("segregate\t:: sort inputs by type");
  post(".\t\t:: scalar multiplication of vectors (lists of floats)");

  endpost(); post("%c advanced message objects", HEARTSYMBOL);
  post("tabread4\t:: 4-point interpolating table-read object");
  post("tabdump\t\t:: dump the table as a list");
  post("tabset\t\t:: set a table with a list");
  post("mavg\t\t:: a variable moving average filter");
  post("mean\t\t:: get the arithmetic mean of a vector");
  post("minmax\t\t:: get the minimum and the maximum of a vector");
  post("makesymbol\t:: creates (formatted) symbols");
  post("date\t\t:: get the current system date");
  post("time\t\t:: get the current system time");
  post("index\t\t:: convert symbols to indices");
  post("drip\t\t:: converts a package to a sequence of atoms");
  post("sort\t\t:: shell-sort a package of floats");
  post("demux\t\t:: demultiplex the input to a specified output");
  post("msgfile\t\t:: store and handles lists of lists");
  post("lp\t\t:: write to the (parallel) port");
  post("wrap\t\t:: wrap a floating number between 2 limits");
  post("urn\t\t:: unique random numbers");
  post("operating_system\t:: information on the OS");

  endpost(); post("\n(l) forum::für::umläute except where indicated");
  post("this software is released under the GnuGPL that is provided with these files");
  endpost();
}

static void *zexy_new(void)
{
  t_zexy *x = (t_zexy *)pd_new(zexy_class);
  return (x);
}

void zexy_setup(void) 
{
  int i;

  startpost("\n\t");
  for (i=0; i<28; i++) startpost("%c", HEARTSYMBOL);
  endpost();
  post("\t%c the zexy external  "VERSION" %c", HEARTSYMBOL, HEARTSYMBOL);
  post("\t%c (l)  forum::für::umläute %c", HEARTSYMBOL, HEARTSYMBOL);
  post("\t%c       iem   @  kug       %c", HEARTSYMBOL, HEARTSYMBOL);
  post("\t%c  compiled:  "__DATE__"  %c", HEARTSYMBOL, HEARTSYMBOL);
  post("\t%c send me a 'help' message %c", HEARTSYMBOL, HEARTSYMBOL);
  startpost("\t");
  for (i=0; i<28; i++) startpost("%c", HEARTSYMBOL);
  endpost(); endpost();
  
  zexy_class = class_new(gensym("zexy"), zexy_new, 0, sizeof(t_zexy), 0, 0);
  class_addmethod(zexy_class, zexy_help, gensym("help"), 0);

  zexy_register("zexy");

  /* ************************************** */
  z_zexy_setup();
}

#ifndef __WIN32__
void verbose(int level, const char *fmt, ...){
  char buf[MAXPDSTRING];
  va_list ap;
 
  va_start(ap, fmt);
  vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
  va_end(ap);

  post("zverbose(%d): %s", level, buf);
}
#endif
