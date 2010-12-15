EXTERN void magicGlass_bind(t_magicGlass *x, t_object *obj, int outno);
EXTERN void magicGlass_unbind(t_magicGlass *x);
EXTERN void magicGlass_bang(t_magicGlass *x); 
EXTERN void magicGlass_float(t_magicGlass *x, t_float f); 
EXTERN void magicGlass_symbol(t_magicGlass *x, t_symbol *sym);
EXTERN void magicGlass_anything(t_magicGlass *x, t_symbol *sym, int argc, t_atom *argv);
EXTERN void magicGlass_list(t_magicGlass *x, t_symbol *sym, int argc, t_atom *argv);
EXTERN void magicGlass_setCanvas(t_magicGlass *x, int c);
EXTERN void magicGlass_show(t_magicGlass *x);
EXTERN void magicGlass_hide(t_magicGlass *x);
EXTERN void magicGlass_moveText(t_magicGlass *x, int pX, int pY);
EXTERN int magicGlass_bound(t_magicGlass *x);
EXTERN int magicGlass_isOn(t_magicGlass *x);
EXTERN void magicGlass_setOn(t_magicGlass *x, int i);
EXTERN void magicGlass_setDsp(t_magicGlass *x, int i);
EXTERN void *magicGlass_new(int c);
EXTERN void magicGlass_free(t_magicGlass *x);
EXTERN void magicGlass_setup(void);

