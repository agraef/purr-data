/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2410 $
$LastChangedDate: 2007-12-11 09:16:34 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#ifndef __RPN_OP_INPUT_H
#define __RPN_OP_INPUT_H

#include "rpn.h"
#include "rpn_opimp.h"

#include "rpn_stack.h"

namespace OpInput {

class VariableI
    : public Opcode
{
protected:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory) 
    {
        size_t ix = stack.Int<Stack::_Pop>();
        if(UNLIKELY(ix < 0 || ix >= inputs.size()))
            throw Error::OpBounds();
        else
            stack.Push(inputs[ix]);
    }
};

class Variables
    : public Opcode
{
protected:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory) 
    {
        size_t sz = inputs.size();
        for(size_t i = 0; i < sz; ++i) stack.Push(inputs[sz-1-i]);
    }
};

} // namespace

#endif