#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define LYONPOTPOURRI_MSG "-[LyonPotpourri 3.0]-"
#define LYONPOTPOURRI_VERSION "3.0 (pre-release 3.1)"
#define LPP_COMPILE_DATE "12 May 2010"
#define lpp_version(objectname) post("%s: version %s compiled %s",objectname,LYONPOTPOURRI_VERSION,LPP_COMPILE_DATE);
// #define potpourri_announce(objname)  post("%s\t ( %s )",LYONPOTPOURRI_MSG,objname)

#define NO_FREE_FUNCTION 0

#ifndef PI
#define PI 3.1415926535898
#endif
#ifndef PIOVERTWO
#define PIOVERTWO 1.5707963268
#endif
#ifndef TWOPI
#define TWOPI 6.2831853072
#endif
// budget version of Max/MSP denorm function
#define FIX_DENORM_FLOAT(v)	(v=(fabs(v) < 0.000001 ? 0.f : (v)))


/*** MSP helper functions, thanks JKC! ***/

void atom_arg_getfloat(float *c, long idx, long ac, t_atom *av);
void atom_arg_getsym(t_symbol **c, long idx, long ac, t_atom *av);

void atom_arg_getfloat(float *c, long idx, long ac, t_atom *av) 
{
		if (c&&ac&&av&&(idx<ac)) {
			*c = atom_getfloat(av+idx);
		}
}

void atom_arg_getsym(t_symbol **c, long idx, long ac, t_atom *av)
{
	if (c&&ac&&av&&(idx<ac)) {
		*c = atom_getsymbol(av+idx);
	} 
}
