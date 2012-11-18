#include <octave/oct.h>

#include <string.h>
DEFUN_DLD (test, args, , "reading and returning a pd-value in octave")
{
   puts("eval(a+b)\n");
   return octave_value ();
}


