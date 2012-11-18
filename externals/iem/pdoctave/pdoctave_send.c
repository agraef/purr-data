#include <string.h>
#ifndef __pdoctave_send_c_
#define __pdoctave_send_c_
#include "pdoctave.h"  /* writing a command to octave */
#include "pdoctave_dataframe.h"
#include "m_pd.h"

static t_class *pdoctave_send_class;

typedef struct _PDOctaveSend_ PDOctaveSend;
struct _PDOctaveSend_
{
   t_object x_obj;
   SharedDataFrame *sdf;
   char oct_command[150];
   char *oct_name;
   void *data;
};

static void *newPDOctaveSend (t_symbol *s, int argc, t_atom *argv)
{
   PDOctaveSend *pdoctsnd_obj = (PDOctaveSend *)
      pd_new (pdoctave_send_class);
   t_symbol *name;

   if (getPDOctaveInstances()<1) {
      error("Octave not running, insert a 'pdoctave' object!!");
      return 0;
   }
   if (argc>0)
      name = atom_getsymbol(argv);
   else
      name = gensym ("pdm1");

   if ((pdoctsnd_obj->sdf = newSharedDataFrame ())==0) {
	   error("pdoctave_send: failed to get shared memory");
	   return 0;
   }
   pdoctsnd_obj->data = 0;
   pdoctsnd_obj->oct_name = name->s_name;

   return ((void *) pdoctsnd_obj);
}

static void pDOctaveSendBang (PDOctaveSend *pdoctsnd_obj)
{
   char *cmd;
   strcpy(pdoctsnd_obj->oct_command, pdoctsnd_obj->oct_name);
   cmd = pdoctsnd_obj->oct_command + strlen(pdoctsnd_obj->oct_name);
   strcpy(cmd, "=read_shared_mem(");
   cmd += 17;
   sprintf(cmd, "%d", getSharedDataFrameId(pdoctsnd_obj->sdf));
   cmd = cmd+strlen(cmd);
   strcpy (cmd, ");\n");

   writeToOctaveStdIN (pdoctsnd_obj->oct_command);
   freeSharedData (pdoctsnd_obj->sdf, &(pdoctsnd_obj->data));
   //removeSharedData (pdoctsnd_obj->sdf, &(pdoctsnd_obj->data));
}

static void copyFloats (float *f, t_atom *a, int n)
{
   for (;n--;a++,f++)
      *f = atom_getfloat(a);
}

static DatTyp pdSelectorClassify (char *selector)
{
   DatTyp pdtyp;
   if (!strcmp (selector, "matrix"))
      pdtyp = MATRIX;
   else if (!strcmp (selector, "float"))
      pdtyp = FLOAT;
   else if (!strcmp (selector, "list"))
      pdtyp = LIST;
   else if (!strcmp (selector, "symbol"))
      pdtyp = SYMBOL;
   else
      pdtyp = UNKNOWN;
   return pdtyp;
}

static void pDOctaveSend (PDOctaveSend *pdoctsnd_obj, 
      t_symbol *s, int argc, t_atom *argv)
{
   DatTyp pdtyp;
   SharedDataFrame *sdf;

   char *selector = s->s_name;
   t_symbol *symptr;
   float *f;
   char *c;
   int count = argc;
   
   pdtyp = pdSelectorClassify (selector);

   sdf = pdoctsnd_obj->sdf;
   
   //sleepUntilWriteUnBlocked (sdf, STD_USLEEP_TIME);
   if ((sleepUntilWriteUnBlocked (sdf))==0) {
	   error("pdoctave_send: pd and octave scheduling error, restart pd");
	   return;
   }
   blockForWriting (sdf);
   
   if (pdoctsnd_obj->data)
      removeSharedData (sdf, &(pdoctsnd_obj->data));
  
   if (pdtyp != SYMBOL) {
      if (pdoctsnd_obj->data = newSharedData (sdf, argc, sizeof(float), pdtyp)) {
	 f = (float *) pdoctsnd_obj->data;
	 copyFloats (f, argv, argc); 
      }
      else {
	 post("pdoctave_send: allocation of shared memory size %d bytes failed!",
	       sizeof(float) * argc);
	 return;
      }
   }
   else {
      symptr = atom_getsymbol (argv);
      if (pdoctsnd_obj->data = newSharedData (sdf, strlen(symptr->s_name)+1, sizeof(char), pdtyp)) {
	 c = (char *) pdoctsnd_obj->data;
	 strcpy (c, symptr->s_name);
      }
      else {
	 post("pdoctave_send: allocation of shared memory size %d bytes failed!",
	       sizeof(char) * (strlen(symptr->s_name)+1));
	 return;
      }
   }
   blockForReading (sdf);
   freeSharedData (sdf, &(pdoctsnd_obj->data));
   pDOctaveSendBang (pdoctsnd_obj);
   if ((sleepUntilReadUnBlocked (sdf))==0) {
		   error("pdoctave_send: pd and octave scheduling error, restart pd");
		   return;
   }
   unBlockForWriting (sdf);
}

static void deletePDOctaveSend (PDOctaveSend *pdoctsnd_obj)
{
   removeSharedData (pdoctsnd_obj->sdf, &(pdoctsnd_obj->data));
   removeSharedDataFrame (&(pdoctsnd_obj->sdf));
}

void pdoctave_send_setup (void)
{
   pdoctave_send_class = class_new 
      (gensym("pdoctave_send"),
       (t_newmethod) newPDOctaveSend,
       (t_method) deletePDOctaveSend,
       sizeof (PDOctaveSend),
       CLASS_DEFAULT, A_GIMME, 0);
   class_addbang (pdoctave_send_class, (t_method) pDOctaveSendBang);
   class_addanything (pdoctave_send_class, (t_method) pDOctaveSend);
   post("pdoctave_send successfully loaded");
}
#endif

