//  $Id: atomic_int.hpp 3666 2009-03-04 23:00:05Z thomas $
//
//  Copyright (C) 2007 Tim Blechmann & Thomas Grill
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

//  $Revision: 3666 $
//  $LastChangedRevision: 3666 $
//  $LastChangedDate: 2009-03-04 18:00:05 -0500 (Wed, 04 Mar 2009) $
//  $LastChangedBy: thomas $

#ifndef __LOCKFREE_ATOMIC_INT_HPP
#define __LOCKFREE_ATOMIC_INT_HPP

#include "prefix.hpp"

namespace lockfree
{

#if defined(__GNUC__) && ( (__GNUC__ > 4) || ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 1)) )

template <typename T>
class atomic_int
{
public:
    explicit atomic_int(T v = 0):
        value(v)
    {
    }

    operator T(void) const
    {
        return __sync_fetch_and_add(&value, 0);
    }

    void operator =(T v)
    {
        value = v;
        __sync_synchronize();
    }

    T operator +=(T v)
    {
        return __sync_add_and_fetch(&value, v);
    }

    T operator -=(T v)
    {
        return __sync_sub_and_fetch(&value, v);
    }

    /* prefix operator */
    T operator ++(void)
    {
        return __sync_add_and_fetch(&value, 1);
    }

    /* prefix operator */
    T operator --(void)
    {
        return __sync_sub_and_fetch(&value, 1);
    }

    /* postfix operator */
    T operator ++(int)
    {
        return __sync_fetch_and_add(&value, 1);
    }

    /* postfix operator */
    T operator --(int)
    {
        return __sync_fetch_and_sub(&value, 1);
    }

private:
    mutable T value;
};

#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)

template <typename T>
class atomic_int
{
public:
    explicit atomic_int(T v = 0):
        value(v)
    {
    }

    operator T(void) const
    {
        return __gnu_cxx::__exchange_and_add(&value, 0);
    }

    void operator =(T v)
    {
        value = v;
    }

    T operator +=(T v)
    {
        return __gnu_cxx::__exchange_and_add(&value, v) + v;
    }

    T operator -=(T v)
    {
        return __gnu_cxx::__exchange_and_add(&value, -v) - v;
    }

    /* prefix operator */
    T operator ++(void)
    {
        return operator+=(1);
    }

    /* prefix operator */
    T operator --(void)
    {
        return operator-=(1);
    }

    /* postfix operator */
    T operator ++(int)
    {
        return __gnu_cxx::__exchange_and_add(&value, 1);
    }

    /* postfix operator */
    T operator --(int)
    {
        return __gnu_cxx::__exchange_and_add(&value, -1);
    }

private:
    mutable _Atomic_word value;
};

#else /* emulate via CAS */

template <typename T>
class atomic_int
{
public:
    explicit atomic_int(T v = 0)
    {
        *this = v;
    }

    operator T(void) const
    {
        memory_barrier();
        return value;
    }

    void operator =(T v)
    {
        value = v;
        memory_barrier();
    }

    /* prefix operator */
    T operator ++()
    {
        return *this += 1;
    }

    /* prefix operator */
    T operator --()
    {
        return *this -= 1;
    }

    T operator +=(T v)
    {
        for(;;)
        {
            T newv = value+v;
            if(likely(CAS(&value,value,newv)))
                return newv;
        }
    }

    T operator -=(T v)
    {
        for(;;)
        {
            T newv = value-v;
            if(likely(CAS(&value,value,newv)))
                return newv;
        }
    }

    /* postfix operator */
    T operator ++(int)
    {
        for(;;)
        {
            T oldv = value;
            if(likely(CAS(&value,oldv,oldv+1)))
                return oldv;
        }
    }

    /* postfix operator */
    T operator --(int)
    {
        for(;;)
        {
            T oldv = value;
            if(likely(CAS(&value,oldv,oldv-1)))
                return oldv;
        }
    }

private:
    T value;
};


#endif

} // namespace lockfree

#endif /* __LOCKFREE_ATOMIC_INT_HPP */
