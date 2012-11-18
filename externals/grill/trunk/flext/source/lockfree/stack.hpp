//  $Id: stack.hpp 1218 2008-01-05 22:02:14Z thomas $
//
//  lock-free stack
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

//  $Revision: 1218 $
//  $LastChangedRevision: 1218 $
//  $LastChangedDate: 2008-01-05 17:02:14 -0500 (Sat, 05 Jan 2008) $
//  $LastChangedBy: thomas $

#ifndef __LOCKFREE_STACK_HPP
#define __LOCKFREE_STACK_HPP

#include "cas.hpp"
#include "atomic_ptr.hpp"
#include "branch_hints.hpp"

#include <memory>  // for std::allocator

#if HAVE_BOOST
#   include <boost/type_traits.hpp>
#   include <boost/static_assert.hpp>
#else
#   define BOOST_STATIC_ASSERT(x)
#endif

namespace lockfree
{
    //! nodes for the intrusive_stack must be derived from that
    class stack_node 
    {
        template <class T> friend class intrusive_stack;

    public:
        stack_node(): next(NULL) {}
    
    private:
        atomic_ptr<stack_node> next;
    };

    //! intrusive lock-free stack implementation with T being the node type (inherited from stack_node)
    template <typename T>
    class intrusive_stack 
    {
        BOOST_STATIC_ASSERT((boost::is_base_of<stack_node,T>::value));

    public:
        intrusive_stack(): head(NULL) {}

        ~intrusive_stack()
        {
            assert(empty());
        }

        bool empty() const { return !head.getPtr(); }

        void push(T *node) 
        {
            assert(!node->next.getPtr());
            while(unlikely(!head.CAS(node->next = head,node)));
        }

        T *pop() 
        {
            for(;;) {
                atomic_ptr<stack_node> current(head);
                T *node = static_cast<T *>(current.getPtr());
                if(!node || likely(head.CAS(current,node->next.getPtr()))) {
                    if(node) node->next.setPtr(NULL);
                    return node;
                }
            }
        }
  
    private:
        atomic_ptr<stack_node> head;
    };


    //! node type used by non-intrusive stack
    template <typename T>
    class stack_value_node
        : public stack_node
    {
    public:
        stack_value_node(T const &v): value(v) {}   
        T value;
    };


    //! non-intrusive lock-free stack
    template <typename T,class Alloc = std::allocator<T> >
    class stack
        : intrusive_stack<stack_value_node<T> >
    {
    public:
        ~stack()
        {
            // delete remaining elements
            stack_value_node<T> * node;
            while((node = intrusive_stack<stack_value_node<T> >::pop()) != NULL)
                free(node);
        }

        void push(T const &v) 
        {
            intrusive_stack<stack_value_node<T> >::push(alloc(v));
        }

        bool pop(T &v) 
        {
            stack_value_node<T> *node = intrusive_stack<stack_value_node<T> >::pop();
            if(!node)
                return false;
            v = node->value;
            free(node);
            return true;
        }
        
    private:

        inline stack_value_node<T> *alloc(const T &k)
        {
            stack_value_node<T> *node = allocator.allocate(1);
            allocator.construct(node,k);
            return node;
        }

        inline void free(stack_value_node<T> *n)
        {
            assert(n);
            allocator.destroy(n);
            allocator.deallocate(n,1);
        }

        typename Alloc::template rebind<stack_value_node<T> >::other allocator;
    };

} // namespace

#endif
