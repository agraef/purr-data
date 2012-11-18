#include <string.h>
#ifndef __pdoctave_command_c_
#define __pdoctave_command_c_
#include "pdoctave.h"  /* writing a command to octave */
#include "pdoctave_dataframe.h"
#include "m_pd.h"

static t_class *pdoctave_command_class;

typedef struct _PDOctaveCommand_ PDOctaveCommand;
struct _PDOctaveCommand_
{
   t_object x_obj;
   char oct_command[1024];
   int pos;
};

static void clearStringCommand (PDOctaveCommand *pdoctcmd_obj)
{
   pdoctcmd_obj->pos=0;
   pdoctcmd_obj->oct_command[0] = '\0';
}

static void addStringToCommand (PDOctaveCommand *pdoctcmd_obj, char *str)
{
   int n = strlen (str);
   strcpy(pdoctcmd_obj->oct_command+pdoctcmd_obj->pos, str);
   pdoctcmd_obj->pos += n;
}

static void addAtomToCommand (PDOctaveCommand *pdoctcmd_obj, t_atom *a)
{
   const unsigned int bufsize = 50;
   char str[bufsize];
   
   atom_string (a, str, bufsize);

   if (strcmp(str,"\\\\,") == 0) {
      strcpy(str,",\0");
   }
   else if (strcmp(str,"\\\\;") == 0) {
      strcpy(str,";\0");
   }

   addStringToCommand (pdoctcmd_obj, str);
}

static void removeEscapeSlashes (char *c)
{
   int pos = strlen(c);
   c = c+pos-1;
   
   while (--pos) {
      if (*--c == '\\') {
	 strcpy(c,c+1);
      }
   }
}
static void pDOctaveCommandBang (PDOctaveCommand *pdoctcmd_obj)
{
   //post("command: %s sent", pdoctcmd_obj->oct_command);
   writeToOctaveStdIN (pdoctcmd_obj->oct_command);
}


static void pDOctaveCommandAny (PDOctaveCommand *pdoctcmd_obj, t_symbol *s, int argc, t_atom *argv)
{
   clearStringCommand (pdoctcmd_obj);
   if (argc>0) 
      while (argc--) {
	 addAtomToCommand (pdoctcmd_obj, argv++);
	 addStringToCommand (pdoctcmd_obj, " \0");
      }

   addStringToCommand (pdoctcmd_obj, "\n");
   removeEscapeSlashes (pdoctcmd_obj->oct_command);
   pDOctaveCommandBang (pdoctcmd_obj);
}

static void *newPDOctaveCommand (t_symbol *s, int argc, t_atom *argv)
{
   PDOctaveCommand *pdoctcmd_obj = (PDOctaveCommand *)
      pd_new (pdoctave_command_class);

//   post("getpdoctaveinstances returned %d", getPDOctaveInstances());
   if (getPDOctaveInstances()<1) {
      error("Octave not running, insert a 'pdoctave' object!!");
      return 0;
   }
   pdoctcmd_obj->pos = 0;
   if (argc>0) 
      while (argc--) {
	 addAtomToCommand (pdoctcmd_obj, argv++);
	 addStringToCommand (pdoctcmd_obj, " \0");
      }

   addStringToCommand (pdoctcmd_obj, "\n");
   removeEscapeSlashes(pdoctcmd_obj->oct_command);

   return ((void *) pdoctcmd_obj);
}
void pdoctave_command_setup (void)
{
   pdoctave_command_class = class_new 
      (gensym("pdoctave_command"),
       (t_newmethod) newPDOctaveCommand,
       0,
       sizeof (PDOctaveCommand),
       CLASS_DEFAULT, A_GIMME, 0);
   class_addbang (pdoctave_command_class, (t_method) pDOctaveCommandBang);
   class_addanything (pdoctave_command_class, (t_method) pDOctaveCommandAny);
   post("pdoctave_command successfully loaded");
}
#endif

