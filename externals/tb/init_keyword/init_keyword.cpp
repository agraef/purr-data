// 
//  
//  init_keyword
//  Copyright (C) 2005  Tim Blechmann
//  
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#include <flext/flext.h>
#include <cstring>
#include <map>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error update flext!!!!!!
#endif

class init_keyword
	: public flext_base
{
	FLEXT_HEADER(init_keyword,flext_base);
	
public: 
	init_keyword(int argc,t_atom *argv)
	{
		getargs(argc, argv);

		AddInBang();
		
		for (int i = 0; i != arguments.Count(); ++i)
		{
			AddOutAnything();
		}
		FLEXT_ADDBANG(0,m_bang);
	}

	void m_bang(void)
	{
		int i = arguments.Count();
		while (i--)
		{
			if (IsFloat(arguments[i]))
				ToOutFloat(i, GetFloat(arguments[i]));
			else if(IsSymbol(arguments[i]))
				ToOutSymbol(i, GetSymbol(arguments[i]));
			else
				FLEXT_ASSERT(false); /* we should never get here */ 
		}
	}

protected:
	
private:
	void getargs(int& argc,t_atom* &argv)
	{
		AtomList keywords (argc, argv);
		
		AtomList canvasargs;
		GetCanvasArgs(canvasargs);

		std::map<const t_symbol*, t_atom> argmap;
		
		for (int i = 0; i != canvasargs.Count(); ++i)
		{
			t_atom atom;
			CopyAtom(&atom, &canvasargs[i]);
			if (IsSymbol(atom))
			{
				const char* symbol = GetString(atom);
				char keyword[80];
				keyword[0] = 0;

				strncat(keyword, symbol, strcspn(symbol, "="));
 				
				char* value = strchr(symbol, '=');
				
				if (value)
				{
					/* try to convert to float */
					char * endptr;
					float fvalue = strtof(value+1, &endptr);
					t_atom value_atom;

					if (*endptr)
						SetString(value_atom, value+1);
					else
 						SetFloat(value_atom, fvalue);
					
					argmap[MakeSymbol(keyword)] = value_atom;
				}
			}
		}		
		
		for (int i = 0; i!= keywords.Count(); ++i)
		{	
			if (IsSymbol(keywords[i]))
			{
				const t_symbol *  keyword = GetSymbol(keywords[i]);
				t_atom value = argmap[keyword];
				arguments.Append(value);
			}
			else if (IsFloat(keywords[i]))
				arguments.Append(canvasargs[(int)i]);
			
		};
	}

    AtomList arguments;
	FLEXT_CALLBACK(m_bang);
};

FLEXT_NEW_V("init_keyword", init_keyword)
