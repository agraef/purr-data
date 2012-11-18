#include <octave/oct.h>

#include <unistd.h>
#include <string.h>
#include "pdoctave_dataframe.h"

DatTyp classifyOctPDType (octave_value res)
{
   DatTyp pdtyp;
   if (res.is_real_scalar())
      pdtyp = FLOAT;
   else if (res.is_real_matrix()) 
      pdtyp = MATRIX;
   else if (res.is_string())
      pdtyp = SYMBOL;
   else
      pdtyp = UNKNOWN;
}

void writeOctMatrixIntoFloat (Matrix mtx, float *f)
{
   int n = mtx.rows();
   int m = mtx.columns();
   int i;
   int j;

   *f++ = n;
   *f++ = m;

   for (j = 0; j < m; j++)
      for (i=0; i < n; i++)
	 *f++ = (float) mtx(i,j);
}

void writeOctScalarIntoFloat (double d, float *f)
{
   *f = (float) d;
}
void writeOctStringIntoString (char *dst, char *src) 
{
   strcpy (dst, src);
}


DEFUN_DLD (write_shared_mem, args, , "returning an octave value to pd-value")
{
   SharedDataFrame *sdf;
   int size;
   void *data;
   DatTyp pdtype;
   int shmem_id = args(1).int_value();
   
   if (shmem_id == -1) {
      error("failed to get valid id\n");
      return octave_value();
   }
   sdf = getSharedDataFrame (shmem_id);
   
   if (!sdf) {
      error("failed to attach memory!\n");
      return octave_value();
   }

   if((sleepUntilWriteBlocked (sdf))==0) {
	   error("write_shared_mem: pd and octave scheduling error, restart pd!");
	   return octave_value();
   }
   if (args(0).is_empty()) {
	   unBlockForWriting (sdf);
	   removeSharedDataFrame (&sdf);
	   return octave_value();
   }
   else if (args(0).is_string()) {
      pdtype = SYMBOL;
      size = args(0).string_value().size();
      if (data = newSharedData (sdf, size, sizeof(char),pdtype)) {
	 writeOctStringIntoString( (char*)data, (char*)args(0).string_value().c_str() );
      }
      else {
	 error("failed to get new data memory!");
	 unBlockForWriting (sdf);
	 removeSharedDataFrame (&sdf);
	 return octave_value();
      }

   }
   else if (args(0).is_real_matrix()) {
      pdtype = MATRIX;
      size = args(0).columns() * args(0).rows()+2;
      if (data = newSharedData (sdf, size, sizeof(float),pdtype)) {
	 writeOctMatrixIntoFloat (args(0).matrix_value(), (float *) data);
      }
      else {
	 error("failed to get new data memory!");
	 unBlockForWriting (sdf);
	 removeSharedDataFrame (&sdf);
	 return octave_value();
      }
   }
   else if (args(0).is_real_scalar()) {
      pdtype = FLOAT;
      if (data = newSharedData (sdf, 1, sizeof(float), pdtype)) {
	 writeOctScalarIntoFloat(args(0).scalar_value(), (float *) data);
      }
      else {
	 error("failed to get new data memory!");
	 unBlockForWriting (sdf);
	 removeSharedDataFrame (&sdf);
	 return octave_value();
      }
   }
   else 
      std::cout << " no mehtod for argument conversion" << std::endl;

   unBlockForWriting (sdf);

   freeSharedData (sdf, &data);
   freeSharedDataFrame (&sdf);

   return octave_value();
}


