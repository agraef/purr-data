/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2410 $
$LastChangedDate: 2007-12-11 09:16:34 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#ifndef __RPN_OP_META_H
#define __RPN_OP_META_H

#include "rpn.h"
#include "rpn_opimp.h"

#include "rpn_stack.h"

namespace OpMeta {


class MetaDo
    : public Meta
{
protected:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory,Command *function) 
    {
        int ix = stack.Int<Stack::_Pop>();
        for(int i = 0; i < ix; ++i)
            function->Do(stack,inputs,memory);
    }
};

class MetaWhile
    : public Meta
{
protected:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory,Command *function)
    {
        for(;;) {
            Operand op = stack.Pop();
            if(UNLIKELY(!op.Scalar())) 
                throw Error::OpTypeErr();
            if((Real)op)
                function->Do(stack,inputs,memory);
        }
    }
};


} // namespace

#endif
