#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#ifndef __pdoctave_c__
#include "m_pd.h"
#include "pdoctave_send.h"
#include "pdoctave_get.h"
#include "pdoctave_command.h"
//#include "math.h"
//#include "d_mayer_fft.c"
//#include "/home/fzotter/downloads/pd-0.38-4/src/d_mayer_fft.c"
//#include "/home/pd/src/pd-0.37-1-alsamm/src/d_mayer_fft.c"

static t_class *pdoctave_class;
static int obj_instances = 0;
static int octave_pid;
static int fd[2];


/* PDOCTAVE is an object that starts an octave process and holds a pipe to its stdin.
 * other sub-routines can use the 'writeToOctaveStdIN' routine to write commands
 * to octave.
*/


typedef struct _PDOctave_ PDOctave;
struct _PDOctave_
{
   t_object x_obj;
};

int getPDOctaveInstances ()
{
   return obj_instances;
}

// writing commands to the octave-process
int writeToOctaveStdIN (const char *cmd)
{
   int bytes_written;
   if (obj_instances > 0) {
      bytes_written = write(fd[1], (void*)cmd, strlen(cmd));
      if (bytes_written == -1) {
	 post("error writing to pipe");
	 return -1;
      }
//      post("wrote %d bytes", bytes_written);
      return 0;
   }
   else
      return -1;
}

static void deletePDOctave (PDOctave *pdoctave_obj) 
{
   if (obj_instances-- == 1) {
      int err = 0;
      close(fd[1]);

      if ((waitpid(octave_pid, NULL, 0) == -1) && (errno != ECHILD)) {
	 err = 1;
      }
      if (err) {
	 post("error closing octave");
      }
   }
}

/*
static void *newPDOctave (t_symbol *s, int argc, t_atom *argv)
{
   PDOctave * pdoctave_obj;
   pdoctave_obj = (PDOctave *) pd_new (pdoctave_class);

   // creating only one pipe!
   if (!obj_instances++) {

      if (pipe(fd) == -1) {
	 post("Error creating pipe.");
      }
      
      if ((octave_pid = fork()) == 0) { 
         //child process
	 // execution of octave and pipe to octave stdin
	 close(fd[1]);
	 
	 if (dup2(fd[0], STDIN_FILENO) == -1) {
	    post("error duplicating filedescriptor to STDIN");
	    exit(1);
	    return; 
	 }
	 
	 close(fd[0]);
	 execlp("octave", "octave", "-i", NULL);
	 // this is only reached, when the octave process is
	 // dying before the pdoctave object
	 post("shell command ``octave'' could not be executed");
      } else if (octave_pid == -1) {
	 // Error handling
      } else {       
         //parent process
	 close(fd[0]);
	 // waiting for the child process having the octave pipe
	 // and process properly set up
	 sleep(1);
      }
   }
   return ((void *) pdoctave_obj);
} 
*/
static void openPDOctave ()
{
   if (!obj_instances++) {

      if (pipe(fd) == -1) {
	 post("Error creating pipe.");
      }
      
      if ((octave_pid = fork()) == 0) { 
         //child process
	 // execution of octave and pipe to octave stdin
	 close(fd[1]);
	 
	 if (dup2(fd[0], STDIN_FILENO) == -1) {
	    post("error duplicating filedescriptor to STDIN");
	    exit(1);
	    return; 
	 }
	 
	 close(fd[0]);
	 execlp("octave", "octave", "-i", NULL);
	 // this is only reached, when the octave process is
	 // dying before the pdoctave object
	 post("shell command ``octave'' could not be executed");
      } else if (octave_pid == -1) {
	 // Error handling
      } else {       
         //parent process
	 close(fd[0]);
	 // waiting for the child process having the octave pipe
	 // and process properly set up
	 sleep(1);
      }
   }
} 


void pdoctave_setup (void)
{
/*
     	pdoctave_class = class_new 
      (gensym("pdoctave"),
       (t_newmethod) newPDOctave,
       (t_method) deletePDOctave,
       sizeof (PDOctave),
       CLASS_NOINLET, 0);
   class_sethelpsymbol(pdoctave_class, gensym("pdoctave-help.pd"));
   */
   openPDOctave();
   post("pdoctave successfully loaded!");
   pdoctave_send_setup ();
   pdoctave_command_setup ();
   pdoctave_get_setup ();
}

#define __pdoctave_c_
#endif
