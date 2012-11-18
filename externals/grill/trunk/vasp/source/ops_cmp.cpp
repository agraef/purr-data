/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_cmp.h"
#include "opdefs.h"
#include "util.h"

// --------------------------------------------------------------

VASP_BINARY("vasp.<",lwr,true,VASP_ARG_R(0),"set destination to 1 if source < argument, 0 otherwise")
VASP_BINARY("vasp.>",gtr,true,VASP_ARG_R(0),"set destination to 1 if source > argument, 0 otherwise")
VASP_BINARY("vasp.a<",alwr,true,VASP_ARG_R(0),"set destination to 1 if abs(source) < abs(argument), 0 otherwise")
VASP_BINARY("vasp.a>",agtr,true,VASP_ARG_R(0),"set destination to 1 if abs(source) > abs(argument), 0 otherwise")
VASP_BINARY("vasp.<=",leq,true,VASP_ARG_R(0),"set destination to 1 if source <= argument, 0 otherwise")
VASP_BINARY("vasp.>=",geq,true,VASP_ARG_R(0),"set destination to 1 if source >= argument, 0 otherwise")
VASP_BINARY("vasp.a<=",aleq,true,VASP_ARG_R(0),"set destination to 1 if abs(source) <= abs(argument), 0 otherwise")
VASP_BINARY("vasp.a>=",ageq,true,VASP_ARG_R(0),"set destination to 1 if abs(source) >= abs(argument), 0 otherwise")
VASP_BINARY("vasp.==",equ,true,VASP_ARG_R(0),"set destination to 1 if source == argument, 0 otherwise")
VASP_BINARY("vasp.!=",neq,true,VASP_ARG_R(0),"set destination to 1 if source != argument, 0 otherwise")


// --------------------------------------------------------------

VASP_BINARY("vasp.min",min,true,VASP_ARG_R(0),"assigns the minimum of the comparison with a value or vasp")
VASP_BINARY("vasp.max",max,true,VASP_ARG_R(0),"assigns the maximum of the comparison with a value or vasp")

VASP_BINARY("vasp.rmin",rmin,true,VASP_ARG_R(0),"assigns the minimum of the radius comparison with a complex value or vasp")
VASP_BINARY("vasp.rmax",rmax,true,VASP_ARG_R(0),"assigns the maximum of the radius comparison with a complex value or vasp")


// --------------------------------------------------------------

VASP_UNARY("vasp.minmax",minmax,true,"compare two vectors, assign the lower values to the first and the higher to the second one") 







