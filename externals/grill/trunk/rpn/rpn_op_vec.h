/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2410 $
$LastChangedDate: 2007-12-11 09:16:34 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#ifndef __RPN_OP_VEC_H
#define __RPN_OP_VEC_H

#include "rpn.h"
#include "rpn_opimp.h"

#include "rpn_stack.h"


namespace OpVec {

template<typename TN>
class PackX
    : public Opcode
{
protected:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory) 
    {
        int cnt = stack.Int<TN>();
        int sz = (int)stack.size();
        if(UNLIKELY(cnt > (int)sz))
            throw Error::OpUnderflow();
        else {
            Vector v(cnt);
            for(int i = 0; i < cnt; ++i) {
                Operand &s = stack.Get();
                if(LIKELY(s.Scalar()))
                    v[i] = (VEl)(Real)s;
                else
                    v[i] = ((Vector &)s)[0];
                stack.pop_back();
            }
            stack.Push(v);
        }
    }
};

typedef PackX<Stack::_Pop> PackN;
typedef PackX<Stack::_Size> PackAll;


class Unpack
    : public Opcode
{
protected:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory) 
    {
        if(UNLIKELY(stack.size() == 0))
            throw Error::OpUnderflow();
        else {
            Operand o = stack.Get();
            if(!o.Scalar()) {
                stack.pop_back();
                Vector &v = o;
                size_t sz = v.size();
                for(size_t i = 0; i < sz; ++i) 
                    stack.Push(Operand(v[sz-1-i]));
            }
            // else nothing to do, scalar stays on stack!
        }
    }
};



class Sum
{
public:
    static void Do(Operand &x)
    {
        if(LIKELY(!x.Scalar())) 
            x = ((Vector &)x).sum();
    }
};

class Product
{
public:
    static void Do(Operand &x)
    {
        if(LIKELY(!x.Scalar())) {
            Real p = 1;
            Vector &v = x;
            size_t sz = v.size();
            for(size_t i = 0; i < sz; ++i) p *= v[i];
            x = p;
        }
    }
};

class Len
{
public:
    static void Do(Operand &x)
    {
        if(LIKELY(!x.Scalar())) 
            x = (Real)((Vector &)x).size();
        else
            x = 0;
    }
};

class Nth
{
public:
    static void Do(Operand &x,const Operand &y)
    {
        if(LIKELY(!x.Scalar())) {
            Vector &v = x;
            size_t sz = v.size();
            if(LIKELY(y.Scalar())) {
                size_t ix = (size_t)(Real)y;
                if(UNLIKELY(ix < 0 || ix >= sz)) 
                    throw Error::OpBounds();
                x = v[ix];
            }
            else {
                const Vector &w = y;
                size_t els = w.size();
                Vector el(els);
                for(size_t i = 0; i < els; ++i) {
                    size_t ix = (size_t)w[i];
                    if(UNLIKELY(ix < 0 || ix >= sz)) 
                        throw Error::OpBounds();
                    el[i] = v[ix];
                }
                x = el;
            }
        }
    }
};

class Reverse
{
public:
    static void Do(Operand &x)
    {
        if(LIKELY(!x.Scalar())) {
            Vector &v = x;
            size_t sz = v.size();
            for(size_t i = 0; i < sz/2; ++i) swap(v[i],v[sz-1-i]);
        }
    }
private:
    template<typename R> static void swap(R &a,R &b) { R t = a; a = b; b = t; }
};

} // namespace

#endif
