#include <string.h>
#ifndef __pdoctave_get_c_
#define __pdoctave_get_c_
#include "pdoctave.h"  /* writing a command to octave */
#include "pdoctave_dataframe.h"
#include "m_pd.h"

static t_class *pdoctave_get_class;

typedef struct _PDOctaveGet_ PDOctaveGet;
struct _PDOctaveGet_
{
   t_object x_obj;
   t_outlet *outlet;
   SharedDataFrame *sdf;
   char oct_command[150];
   char *oct_name;
   void *data;
   int list_length;
   t_atom *list;
};

static void *newPDOctaveGet (t_symbol *s, int argc, t_atom *argv)
{
   PDOctaveGet *pdoctget_obj = (PDOctaveGet *)
      pd_new (pdoctave_get_class);
   t_symbol *name;

//   post("getpdoctaveinstances returned %d", getPDOctaveInstances());
   if (getPDOctaveInstances()<1) {
      error("Octave not running, insert a 'pdoctave' object!!");
      return 0;
   }
   if (argc>0)
      name = atom_getsymbol(argv);
   else
      name = gensym ("pdm1");

   if ((pdoctget_obj->sdf = newSharedDataFrame ())==0) {
	   error("pdoctave_get: failed to get shared memory");
	   return 0;
   }
   pdoctget_obj->data = 0;
   pdoctget_obj->oct_name = name->s_name;
   pdoctget_obj->outlet = outlet_new (&pdoctget_obj->x_obj, 0);
   return ((void *) pdoctget_obj);
}

static void pDOctaveGetCommand (PDOctaveGet *pdoctget_obj)
{
   char *cmd;
   cmd = pdoctget_obj->oct_command;
   strcpy(cmd, "try\n write_shared_mem(");
   cmd += strlen(cmd);
   strcpy(cmd, pdoctget_obj->oct_name);
   cmd += strlen (cmd);
   *cmd++ = ',';
   sprintf(cmd, "%d", getSharedDataFrameId(pdoctget_obj->sdf));
   cmd += strlen(cmd);
   strcpy (cmd, ");\n catch\n disp(\"undefined variable\"); write_shared_mem(");
   cmd += strlen(cmd);
   strcpy (cmd, "[],");
   cmd +=strlen(cmd);
   sprintf(cmd, "%d", getSharedDataFrameId(pdoctget_obj->sdf));
   cmd +=strlen(cmd);
   strcpy (cmd,");\n end\n\0");
   //post("pdoctave_get: %s",pdoctget_obj->oct_command);
   
   writeToOctaveStdIN (pdoctget_obj->oct_command);
}

static void copyFloats (float *f, t_atom *a, int n)
{
   for (;n--;a++,f++)
      SETFLOAT (a, *f);
}

static void outletListAllocation (PDOctaveGet *pdoctget_obj, int newsize)
{
   if (newsize != pdoctget_obj->list_length) {
      if (newsize > 0) {
	 if (pdoctget_obj->list_length > 0) {
	    pdoctget_obj->list = (t_atom *)
	       resizebytes (pdoctget_obj->list, 
		     sizeof(t_atom) * pdoctget_obj->list_length,
		     sizeof(t_atom) * newsize);
	    pdoctget_obj->list_length = newsize;
	 }
	 else {
	    pdoctget_obj->list = (t_atom *) getbytes (sizeof(t_atom) * newsize);
	    pdoctget_obj->list_length = newsize;
	 }
      }
      else if (newsize == 0) {
	 freebytes (pdoctget_obj->list, sizeof(t_atom) * pdoctget_obj->list_length);
	 pdoctget_obj->list = 0;
	 pdoctget_obj->list_length = 0;
      }
   }
}

static void pdoctaveConvertData (PDOctaveGet *pdoctget_obj)
{
   int size = getSharedDataVecLength(pdoctget_obj->sdf);
   t_symbol *s;
   switch (getSharedDataType (pdoctget_obj->sdf)) {
      case FLOAT:
      case MATRIX:
      case LIST:
	 outletListAllocation (pdoctget_obj, size);
	 copyFloats ((float *) pdoctget_obj->data, pdoctget_obj->list, size);
	 break;
      case SYMBOL:
	 size = 1;
	 outletListAllocation (pdoctget_obj, 1);
	 s = gensym((char*)pdoctget_obj->data);
	 SETSYMBOL (pdoctget_obj->list, s);
	 break;
      case UNKNOWN:
	 post("pdoctave_get: unknown return value");
   }
   removeSharedData (pdoctget_obj->sdf, &(pdoctget_obj->data));
}

static void pdoctaveOutletList (PDOctaveGet *pdoctget_obj)
{
   t_symbol *s;
   switch (getSharedDataType (pdoctget_obj->sdf)) {
      case FLOAT:
	 s = gensym("float");
	 break;
      case LIST:
	 s = gensym("list");
	 break;
      case MATRIX:
	 s = gensym("matrix");
	 break;
      case SYMBOL:
	 s = gensym("symbol");
	 break;
      case UNKNOWN:
	 post("pdoctave_get: unknown return value");
	 s = gensym("");
	 return;
   }
   outlet_anything (pdoctget_obj->outlet,
	 s, pdoctget_obj->list_length, pdoctget_obj->list);
}

static void pDOctaveGetBang (PDOctaveGet *pdoctget_obj)
{
   SharedDataFrame *sdf;

   sdf = pdoctget_obj->sdf;

   if((sleepUntilReadUnBlocked (sdf))==0) {
	   error("pdoctave_get: pd and octave scheduling error, restart pd!");
	   return;
   }
   blockForReading (sdf);
   
   // sending read command
   blockForWriting (sdf);
   pDOctaveGetCommand (pdoctget_obj);
   // waiting for results
   if((sleepUntilWriteUnBlocked (sdf))==0) {
	   error("pdoctave_get: pd and octave scheduling error, restart pd!");
	   unBlockForReading (sdf);
	   return;
   }
   
   if ((pdoctget_obj->data = getSharedData (sdf))==0) {
	   error("pdoctave_get: empty data package received");
	   unBlockForReading (sdf);
	   return;
   }
   
   // converting incoming data
   pdoctaveConvertData (pdoctget_obj);
   unBlockForReading (sdf);

   // outletting data
   pdoctaveOutletList (pdoctget_obj);
}


static void deletePDOctaveGet (PDOctaveGet *pdoctget_obj)
{
   removeSharedData (pdoctget_obj->sdf, &(pdoctget_obj->data));
   removeSharedDataFrame (&(pdoctget_obj->sdf));
}

void pdoctave_get_setup (void)
{
   pdoctave_get_class = class_new 
      (gensym("pdoctave_get"),
       (t_newmethod) newPDOctaveGet,
       (t_method) deletePDOctaveGet,
       sizeof (PDOctaveGet),
       CLASS_DEFAULT, A_GIMME, 0);
   class_addbang (pdoctave_get_class, (t_method) pDOctaveGetBang);
   post("pdoctave_get successfully loaded");
}
#endif

