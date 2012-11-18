#ifndef COMPOSER_COMMON_H_INCLUDED
#define COMPOSER_COMMON_H_INCLUDED

#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)

//get rid of the "deprecated conversion from string constant to char*'" warning
#define sys_gui(x) sys_gui(const_cast<char*>(x))
#define sys_vgui(format, args...) sys_vgui(const_cast<char*>(format), ## args)
#define gensym(x) gensym(const_cast<char*>(x))
#define binbuf_addv(b, format, args...) binbuf_addv(b, const_cast<char*>(format), ## args)
#define WRAP(v,w) (((v)<0)?(((w)-1)-((-(v)-1)%(w))):((v)%(w)))

#endif // COMPOSER_COMMON_H_INCLUDED
