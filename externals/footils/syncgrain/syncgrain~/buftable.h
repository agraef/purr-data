//#=====================================================================================
//#
//#       Filename:  buftable.h
//#
//#    Description:  SndObj-Table implemented with Pd/Max buffer as data
//#
//#        Version:  1.0
//#        Created:  01/09/03
//#       Revision:  none
//#
//#         Author:  Frank Barknecht  (fbar)
//#          Email:  fbar@footils.org
//#      Copyright:  Frank Barknecht , 2003
//#
//#        This program is free software; you can redistribute it and/or modify
//#    it under the terms of the GNU General Public License as published by
//#    the Free Software Foundation; either version 2 of the License, or
//#    (at your option) any later version.
//#
//#    This program is distributed in the hope that it will be useful,
//#    but WITHOUT ANY WARRANTY; without even the implied warranty of
//#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#    GNU General Public License for more details.
//#
//#    You should have received a copy of the GNU General Public License
//#    along with this program; if not, write to the Free Software
//#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//#
//#
//#=====================================================================================




#ifndef _BUFTABLE_H
#define _BUFTABLE_H

#include "Table.h"
#include "flext.h"
// #include "flsupport.h"


class buftable : public Table
{
	public:
		buftable();
		buftable(const t_symbol *s = NULL,bool delayed = false);
		~buftable();
		long GetLen() 
		{ 
			return buf->Frames() - 1; 
		}
		float* GetTable()
		{ 
			if ( Ok() )
			{
				return buf->Data(); 
			}
			else
				return NULL;
		}
		float Lookup(int pos);
		char* ErrorMessage();
		short MakeTable(); 
		bool set(const t_symbol *s = NULL,bool delayed = false);
		bool Ok()
		{
			return buf->Ok() && buf->Valid();
		}

	private:
		int   m_error;     // error code
		flext::buffer * buf;
};

#endif
