/* 

deljoin - join a list with delimiter

Copyright (c) 2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define VERSION "0.1.5"

#ifdef __MWERKS__
#define STD std
#else
#define STD 
#endif


class deljoin:
	public flext_base
{
	FLEXT_HEADER_S(deljoin,flext_base,Setup)

public:
	deljoin(int argc,const t_atom *argv);

protected:
	void m_list(const t_symbol *s,int argc,const t_atom *argv);
    void m_del(const t_symbol *s,int argc,const t_atom *argv);	

	const t_symbol *delim;
	
private:
	static void Setup(t_classid c);

	static const t_symbol *sym__space;
	
	FLEXT_CALLBACK_A(m_list)
	FLEXT_CALLBACK_A(m_del)
    FLEXT_ATTRVAR_S(delim)
};

FLEXT_NEW_V("deljoin",deljoin)


const t_symbol *deljoin::sym__space = NULL;

void deljoin::Setup(t_classid c)
{
    sym__space = MakeSymbol(" ");

	FLEXT_CADDMETHOD(c,0,m_list);
	FLEXT_CADDMETHOD(c,1,m_del);
	FLEXT_CADDATTR_VAR1(c,"del",delim);
}

deljoin::deljoin(int argc,const t_atom *argv):
	delim(sym__)
{ 
	AddInAnything("Anything in - triggers output");
	AddInAnything("Set the Delimiter");
	AddOutSymbol("A symbol representing the joined list");

    m_del(sym_list,argc,argv);
}

void deljoin::m_del(const t_symbol *s,int argc,const t_atom *argv) 
{	
    delim = NULL;
    if(s == sym_symbol) {
        FLEXT_ASSERT(argc == 1 && IsSymbol(argv[0]));
        delim = GetSymbol(argv[0]);
    }
    else if(s == sym_list) {
        if(argc == 0)
            delim = sym__space;
        else if(argc >= 1) {
            if(IsSymbol(argv[0]))
                delim = GetSymbol(argv[0]);
            else if(IsFloat(argv[0]) || IsInt(argv[0]))
                delim = sym__;
        }
    }
    else if(s == sym_bang || s == sym_float || s == sym_int)
        delim = sym__;

    if(!delim) {
        post("%s - Argument must be a symbol, list or int/float/bang",thisName());
        delim = sym__;
    }
}
	
/** \brief convert incoming list to a concatenated string
	
	Handles symbols, integers and floats
*/
void deljoin::m_list(const t_symbol *s,int argc,const t_atom *argv)
{
    FLEXT_ASSERT(delim);

	char tmp[1024] = "",*t = tmp;
	const char *sdel = GetString(delim);
	int ldel = strlen(sdel);
	
	if(s && s != sym_list && s != sym_float && s != sym_int && s != sym_bang) {
		strcpy(t,GetString(s));		
		t += strlen(t);
	}
	
	for(int i = 0; i < argc; ++i) {
		if(t != tmp) {
			strcpy(t,sdel);		
			t += ldel;
		}
	
		const t_atom &a = argv[i];
		if(IsSymbol(a))
			strcpy(t,GetString(a));
        else if(CanbeFloat(a)) {
            const float f = GetAFloat(a);
            const int fi = (int)f;
            if(f == fi)
			    STD::sprintf(t,"%i",fi);
            else
    			STD::sprintf(t,"%f",f);
		}
//		else do nothing
			
		t += strlen(t);
	}
	
	ToSysString(0,tmp);
}
