/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2410 $
$LastChangedDate: 2007-12-11 09:16:34 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#ifndef __RPN_OPIMP_H
#define __RPN_OPIMP_H

#include "rpn.h"
#include "rpn_opcode.h"

template <typename OP>
class OpNonary
    : public Opcode
{
public:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory) 
    { 
        stack.Push(OP::Do()); 
    }
};

template <typename OP>
class OpUnary
    : public Opcode
{
public:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory)
    {
        OP::Do(stack.Get());
    }
};

template <typename OP>
class OpBinary
    : public Opcode
{
public:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory)
    {
        Operand x = stack.Pop();
        OP::Do(stack.Get(),x);
    }
};


template <typename OP>
class NonaryImp
{
public:
    static Operand Do() 
    { 
        return OP::template operate<Real>(); 
    }
};

template <typename OP>
class UnaryImp
{
public:
    static void Do(Operand &x)
    {
        if(x.Scalar()) 
            x = OP::operate((Real)x);
        else {
            Vector &v = x;
            size_t sz = v.size();
            for(size_t i = 0; i < sz; ++i) v[i] = OP::operate(v[i]);
        }
    }
};

template <typename OP>
class BinaryImp
{
public:
    static void Do(Operand &x,const Operand &y)
    {
        if(x.Scalar()) 
            if(y.Scalar())
                x = OP::operate((Real)x,(Real)y);
            else {
                Real r = x;
                const Vector &v = y;
                size_t sz = v.size();
                Vector tmp(sz);
                for(size_t i = 0; i < sz; ++i) tmp[i] = (VEl)OP::operate(r,(Real)v[i]);
                x = tmp;
            }
        else
            if(y.Scalar()) {
                Vector &v = x;
                Real r = y;
                size_t sz = v.size();
                for(size_t i = 0; i < sz; ++i) v[i] = (VEl)OP::operate((Real)v[i],r);
            }
            else {
                Vector &v = x;
                const Vector &w = y;
                size_t sz = v.size() < w.size()?v.size():w.size();
                if(v.size() == sz) {
                    for(size_t i = 0; i < sz; ++i) v[i] = OP::operate(v[i],w[i]);
                }
                else {
                    Vector tmp(sz);
                    for(size_t i = 0; i < sz; ++i) tmp[i] = OP::operate(v[i],w[i]);
                    x = tmp;
                }
            }
    }
};

template<typename OP> class ImpNonary: public OpNonary<NonaryImp<OP> > {};
template<typename OP> class ImpUnary: public OpUnary<UnaryImp<OP> > {};
template<typename OP> class ImpBinary: public OpBinary<BinaryImp<OP> > {};

#endif
