//  $Id: fifo.hpp 1218 2008-01-05 22:02:14Z thomas $
//
//  lock-free fifo queue from
//  Michael, M. M. and Scott, M. L.,
//  "simple, fast and practical non-blocking and blocking concurrent queue algorithms"
//
//  intrusive implementation for c++
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

#ifndef __LOCKFREE_FIFO_HPP
#define __LOCKFREE_FIFO_HPP

#include "cas.hpp"
#include "atomic_ptr.hpp"
#include "branch_hints.hpp"

#ifdef HAVE_BOOST
#   include <boost/type_traits.hpp>
#   include <boost/static_assert.hpp>
#else /* HAVE_BOOST */
#   ifdef BOOST_STATIC_ASSERT
#      undef BOOST_STATIC_ASSERT
#   endif
#   define BOOST_STATIC_ASSERT(x)
#endif /* HAVE_BOOST */

#include <memory>

namespace lockfree
{
    struct intrusive_fifo_node;

    typedef atomic_ptr<intrusive_fifo_node> intrusive_fifo_ptr_t;

    struct intrusive_fifo_node
    {
        intrusive_fifo_ptr_t next;
        struct fifo_node * data;
    };

    struct fifo_node
    {
        intrusive_fifo_node *volatile node;

    protected:
        fifo_node(void)
        {
            node = new intrusive_fifo_node();
        }

        ~fifo_node(void)
        {
            delete node;
        }

        template <class T> friend class intrusive_fifo;
    };

    template <typename T>
    class intrusive_fifo
    {
        BOOST_STATIC_ASSERT((boost::is_base_of<fifo_node,T>::value));

    public:
        intrusive_fifo(void)
        {
            /* dummy pointer for head/tail */
            intrusive_fifo_node * dummy = new intrusive_fifo_node();
            dummy->next(NULL,0);
            head_(dummy,0);
            tail_(dummy,0);
        }

        ~intrusive_fifo(void)
        {
            /* client must have freed all members already */
            assert (empty());
            delete head_.getPtr();
        }

        bool empty() const
        {
            return head_.getPtr() == tail_.getPtr() || (!tail_.getPtr());
        }

        void enqueue(T * instance)
        {
            /* volatile */ intrusive_fifo_node * node = static_cast<fifo_node*>(instance)->node;
            node->next.setPtr(NULL);
            node->data = static_cast<fifo_node*>(instance);

            for (;;)
            {
                intrusive_fifo_ptr_t tail(tail_);
                memory_barrier();

                intrusive_fifo_ptr_t next(tail.getPtr()->next);
                memory_barrier();

                if (likely(tail == tail_))
                {
                    if (next.getPtr() == 0)
                    {
                        if (tail.getPtr()->next.CAS(next,node))
                        {
                            tail_.CAS(tail,node);
                            return;
                        }
                    }
                    else
                        tail_.CAS(tail,next);
                }
            }
        }

        T* dequeue (void)
        {
            T * ret;
            for (;;)
            {
                intrusive_fifo_ptr_t head(head_);
                memory_barrier();

                intrusive_fifo_ptr_t tail(tail_);
                /* volatile */ intrusive_fifo_node * next = head.getPtr()->next.getPtr();
                memory_barrier();

                if (likely(head == head_))
                {
                    if (head.getPtr() == tail.getPtr())
                    {
                        if (next == 0)
                            return 0;
                        tail_.CAS(tail,next);
                    }
                    else
                    {
                        ret = static_cast<T*>(next->data);
                        if (head_.CAS(head,next))
                        {
                            ret->node = head.getPtr();
                            return ret;
                        }
                    }
                }
            }
        }

    private:
        intrusive_fifo_ptr_t head_,tail_;
    };

    template <typename T>
    class fifo_value_node:
        public fifo_node
    {
    public:
        fifo_value_node(T const & v): value(v) {}

        T value;
    };

    template <typename T, class Alloc = std::allocator<T> >
    class fifo
        : intrusive_fifo<fifo_value_node<T> >
    {
    public:
        ~fifo()
        {
            fifo_value_node<T> * node;
            while((node = intrusive_fifo<fifo_value_node<T> >::dequeue()) != NULL)
                free(node);
        }

        void enqueue(T const & v)
        {
            intrusive_fifo<fifo_value_node<T> >::enqueue(alloc(v));
        }

        bool dequeue (T & v)
        {
            fifo_value_node<T> * node = intrusive_fifo<fifo_value_node<T> >::dequeue();
            if(!node)
                return false;

            v = node->value;
            free(node);
            return true;
        }

    private:

#if 0
        inline fifo_value_node<T> *alloc(const T &k)
        {
            fifo_value_node<T> *node = allocator.allocate(1);
            allocator.construct(node,k);
            return node;
        }

        inline void free(fifo_value_node<T> *n)
        {
            assert(n);
            allocator.destroy(n);
            allocator.deallocate(n,1);
        }
#else
        // hmmm... static keyword brings 10% speedup...

        static inline fifo_value_node<T> *alloc(const T &k)
        {
            return new fifo_value_node<T>(k);
        }

        static inline void free(fifo_value_node<T> *n)
        {
            assert(n);
            delete n;
        }
#endif

        typename Alloc::template rebind<fifo_value_node<T> >::other allocator;
    };
}

#endif /* __LOCKFREE_FIFO_HPP */



