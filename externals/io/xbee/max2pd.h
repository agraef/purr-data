/*
 * this header aims to make it easy to port Max objects to Pd
 */

/* name changes */
#define SETSYM SETSYMBOL

/* Pd doesn't have longs */
#define SETLONG SETFLOAT

/* allocate memory */
#define sysmem_newptr(size) getbytes(128)
#define sysmem_freeptr(ptr) freebytes(ptr, 128)


#define atom_getlong(atom) atom_getfloatarg(0, 1, atom)
#define atom_getsym(atom) atom_getsymbolarg(0, 1, atom)
#define object_alloc(obj_class) pd_new(obj_class)
#define object_free(obj) pd_free((t_pd*)obj)
