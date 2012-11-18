/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2410 $
$LastChangedDate: 2007-12-11 09:16:34 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#ifndef __RPN_OP_DEBUG_H
#define __RPN_OP_DEBUG_H

#include "rpn.h"
#include "rpn_opimp.h"

#include "rpn_stack.h"


namespace OpDebug {

template<typename TI,typename TN>
class PostX
    : public Opcode
{
protected:
    virtual void Do(Stack &stack,const Inputs &inputs,Memory &memory) 
    {
        int ix = stack.Int<TI>();
        int cnt = stack.Int<TN>();
        for(int i = 0; i < cnt; ++i) {
            const Operand &o = stack.Get(i);
            if(o.Scalar())
                post("%lf",(Real)o);
            else 
                post("[]");
        }
    }
};

typedef PostX<Stack::_Pop,Stack::_Pop> PostIN;
typedef PostX<Stack::_Pop,Stack::_1> PostI;
typedef PostX<Stack::_0,Stack::_Pop> PostN;
typedef PostX<Stack::_0,Stack::_1> Post;
typedef PostX<Stack::_0,Stack::_Size> PostAll;

}

#endif
