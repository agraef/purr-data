/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2411 $
$LastChangedDate: 2007-12-11 10:44:38 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#ifndef __RPN_OPERAND_H
#define __RPN_OPERAND_H

#include "rpn.h"

class Operand
{
public:
    Operand(): vec(NULL),real(0) {}

    Operand(const Operand &o): vec(NULL) { operator =(o); }
    Operand(Real r): vec(NULL),real(r) {}
    Operand(const Vector &v): vec(new Vector(v)) {}
    Operand(int argc,const t_atom *argv): vec(NULL) { operator()(argc,argv); }

    ~Operand() { if(UNLIKELY(vec)) { delete vec; vec = NULL; } }

    Operand &operator =(const Operand &o)
    {
        if(LIKELY(o.Scalar())) {
            if(UNLIKELY(vec)) { delete vec; vec = NULL; } 
            real = o.real;
        }
        else
            if(vec)
                *vec = *o.vec;
            else
                vec = new Vector(*o.vec);
        return *this;
    }

    Operand &operator =(Real r)
    {
        if(UNLIKELY(vec)) { delete vec; vec = NULL; }
        real = r;
        return *this;
    }

    Operand &operator =(const Vector &v)
    {
        if(vec) 
            *vec = v;
        else
            vec = new Vector(v);
        return *this;
    }

    bool Scalar() const { return vec == NULL; }


    operator Real() const { FLEXT_ASSERT(Scalar()); return real; }

    operator Vector &() { FLEXT_ASSERT(!Scalar()); return *vec; }
    operator const Vector &() const { FLEXT_ASSERT(!Scalar()); return *vec; }


    Operand &operator()(int argc,const t_atom *argv) 
    { 
        if(UNLIKELY(!argc))
            *this = 0;
        else if(argc == 1)
            *this = flext::GetAFloat(*argv);
        else {
            if(vec)
                vec->resize(argc);
            else
                vec = new Vector(argc);
            for(int i = 0; i < argc; ++i) 
                (*vec)[i] = flext::GetAFloat(argv[i]);
        }
        return *this; 
    }


    float to_float() const
    {
        FLEXT_ASSERT(Scalar());
        return static_cast<float>(real);
    }

    double to_double() const
    {
        FLEXT_ASSERT(Scalar());
        return static_cast<double>(real);
    }

    void to_list(flext::AtomList &l,int offset = 0) const
    {
        FLEXT_ASSERT(!Scalar());
        int cnt = (int)vec->size();
        l(offset+cnt);
        for(int i = 0; i < cnt; ++i) flext::SetFloat(l[offset+i],static_cast<float>((*vec)[i]));
    }

private:

    Real real;
    Vector *vec;
};

typedef std::vector<Operand> Operands;

#endif
