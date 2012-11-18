#include <octave/oct.h>

#include <unistd.h>
#include <string.h>
#include "pdoctave_dataframe.h"

Matrix writeFloatIntoOctMatrix (int n, int m, float *f)
{
   int i;
   int j;
   Matrix mtx = Matrix(n,m);
   for (j = 0; j < n; j++)
      for (i=0; i < m; i++)
	 mtx(j,i) = (double)*f++;
   return mtx;
}

double writeFloatIntoDouble (float f)
{
   return (double) f;
}

RowVector writeFloatIntoRowVector (int n, float *f)
{
   RowVector rv = RowVector(n);
   int i;
   for (i = 0; i<n; i++)
      rv(i) = (double) *f++;
   return rv;
}
	 
ColumnVector writeFloatIntoColumnVector (int n, float *f)
{
   ColumnVector cv = ColumnVector(n);
   int i;
   for (i = 0; i<n; i++)
      cv(i) = (double) *f++;
   return cv;
}

DEFUN_DLD (read_shared_mem, args, , "reading and returning a pd-value in octave")
{
   SharedDataFrame *sdf;
   void *data;
   octave_value convert_result;
   int shmem_id = args(0).int_value();
   float *f;
   std::string str;
   std::string quote_sign = "\"";
   
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
	   error("read_shared_mem: pd and octave scheduling error, restart pd!");
	   return octave_value();
   }
   if((sleepUntilReadBlocked (sdf))==0) {
	   error("read_shared_mem: pd and octave scheduling error, restart pd!");
	   return octave_value();
   }
   
   data = getSharedData (sdf);
   if (!data) {
     error("failed to attach data!\n");
     freeSharedDataFrame (&sdf);
     unBlockForReading (sdf);
     return octave_value();
   }

   f = (float*) data;
 
   switch (getSharedDataType (sdf)) {
      case FLOAT: 
	 convert_result = octave_value(writeFloatIntoDouble (*f));
	 break;
      case LIST:
	 convert_result = octave_value( 
	       writeFloatIntoRowVector (getSharedDataVecLength(sdf), f));
	 break;
      case SYMBOL:
	 str = (std::string) (char *) data;
	 convert_result = octave_value(quote_sign+str+quote_sign);
	 break;
      case MATRIX:
	 convert_result = octave_value(writeFloatIntoOctMatrix ((int)f[0],(int)f[1],f+2));
	 break;
      case UNKNOWN:
	 error("unknown pdoctave type");
	 convert_result = octave_value ();
   }
   unBlockForReading (sdf);
   removeSharedData (sdf, &data);
   freeSharedDataFrame (&sdf);
   return (convert_result);
}


