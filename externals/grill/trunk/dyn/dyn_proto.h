/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#ifndef __DYN_PROTO_H
#define __DYN_PROTO_H

#include "dyn_data.h"

// \todo really align..
inline void *NewAligned(int n) { return new char[n]; }
inline void FreeAligned(void *b) { delete[] (char *)b; }

extern t_class *pxin_class,*pxout_class;
extern t_class *pxins_class,*pxouts_class;

extern const t_symbol *sym_dyncanvas;
extern const t_symbol *sym_dynsxin,*sym_dynsxout,*sym_dynpxin,*sym_dynpxout;


void *NewPDObject(int type,t_glist *glist,const t_symbol *hdr,int argc = 0,const t_atom *argv = NULL);
dyn_ident *NewObject(int type,dyn_callback cb,dyn_ident *owner,const t_symbol *hdr,int argc = 0,const t_atom *argv = NULL);
void DelObject(dyn_ident *obj);

#endif
