//  $Id: branch_hints.hpp 1234 2008-01-17 16:12:12Z thomas $
//
//  branch hints
//  Copyright (C) 2007 Tim Blechmann
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

//  $Revision: 1234 $
//  $LastChangedRevision: 1234 $
//  $LastChangedDate: 2008-01-17 11:12:12 -0500 (Thu, 17 Jan 2008) $
//  $LastChangedBy: thomas $

#ifndef __LOCKFREE_BRANCH_HINTS_HPP
#define __LOCKFREE_BRANCH_HINTS_HPP

namespace lockfree
{
    /** \brief hint for the branch prediction */
    inline bool likely(bool expr)
    {
#ifdef __GNUC__
        return __builtin_expect(expr, true);
#else
        return expr;
#endif
    }

    /** \brief hint for the branch prediction */
    inline bool unlikely(bool expr)
    {
#ifdef __GNUC__
        return __builtin_expect(expr, false);
#else
        return expr;
#endif
    }

} // namespace

#endif /* __LOCKFREE_BRANCH_HINTS_HPP */
