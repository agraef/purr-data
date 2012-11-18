//#=====================================================================================
//#
//#       Filename:  buftable.cpp
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

#include "buftable.h"

buftable::buftable()
	: 
	buf(NULL)
{
	buf = new flext::buffer(NULL);
}

buftable::buftable(const t_symbol *s, bool delayed)
{
	buf = new flext::buffer(s, delayed);
}


buftable::~buftable()
{
	if ( buf )     delete buf;
}

float buftable::Lookup(int pos)
{
	if ( buf && pos < GetLen() - 1 && pos >= 0)
	{
		return buf->Data()[pos];
	}
	else 
	{
		return 0;
	}
}


char * buftable::ErrorMessage()
{
	char * err = "Not implemented";
	return err;
}

short buftable::MakeTable()
{
	return 0;
}

bool buftable::set(const t_symbol *s,bool delayed)
{
	buf->Set(s, delayed);
	return buf->Ok();
}

