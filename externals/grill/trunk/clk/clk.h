/* 
clk - syncable clocking objects

Copyright (c)2006-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3600 $
$LastChangedDate: 2008-04-13 08:14:54 -0400 (Sun, 13 Apr 2008) $
$LastChangedBy: thomas $
*/

#ifndef __CLK_H
#define __CLK_H

#define FLEXT_ATTRIBUTES 1

#include <flext.h>
#include <set>
#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>

#ifdef BIGNUM
#include <gmp.h>
#include <mpfr.h>
#endif

namespace clk {


class Master;
class Client;

#ifdef BIGNM
class BigNum
{
public:
    BigNum(double d = 0)
    {
        mpfr_init_set_d(&x,d,GMP_RNDN);
    }

/*
    BigNum &operator =(const BigNum &b)
    {
        mpfr_set(&x,&b.x,GMP_RNDN);
        return *this;
    }
*/
    BigNum &operator +=(const BigNum &b)
    {
        mpfr_add(&x,&x,&b.x,GMP_RNDN);
        return *this;
    }

    BigNum operator +(const BigNum &b)
    {
        return BigNum(*this);
    }

    operator double() const 
    {
        return mpfr_get_d(&x,GMP_RNDN);
    }

private:
    mpfr_t x;
};
#else
typedef double BigNum;
#endif


template <class T>
class SlidingAvg
{
public:
    SlidingAvg(int sz)
        : sum(0),pos(0),maxsz(sz)
    {
    }

    operator T() const 
    {
        return sum/val.size();
    }

    SlidingAvg &clear()
    {
        sum = 0;
        val.resize(0);
        pos = 0;
        return *this;
    }

    size_t size() const { return maxsz; }

    SlidingAvg &resize(size_t sz)
    {
        if(sz < val.size()) {
            // preserve values
            rotate((int)pos);
            pos = 0;
            val.resize(sz);
        }
        maxsz = sz;
        return recalc();
    }

    SlidingAvg &rotate(int k)
    {
        const int n = (int)size();
        if(k < 0 || k >= n) {
            k %= n;
            if (k < 0) k += n;
        }
        if (k == 0) return *this;

        int c = 0;
        for(int v = 0; c < n; v++) {
            int t = v, tp = v + k;
            T tmp = val[v];
            c++;
            while (tp != v) {
                val[t] = val[tp];
                t = tp;
                tp += k;
                if (tp >= n) tp -= n;
                c++;
            }
            val[t] = tmp;
        }

        return *this;
    }

    SlidingAvg &recalc()
    {
        sum = 0;
        for(typename std::vector<T>::const_iterator it = val.begin(); it != val.end(); ++it)
            sum += *it;
        return *this;
    }

    SlidingAvg operator +=(T v)
    {
        if(val.size() == maxsz) {
            sum += v-val[pos];
            val[pos] = v;
            if(++pos >= val.size()) pos = 0;
        }
        else {
            sum += v;
            val.push_back(v);
        }
        return *this;
    }

    T sum;
    std::vector<T> val;
    size_t pos,maxsz;
};

template <class T>
static std::ostream &operator <<(std::ostream &s,SlidingAvg<T> &sa)
{
    s << &sa << ":" << sa.val.size() << "(" << sa.maxsz << ") - " << sa.pos << "[";
    for(typename std::vector<T>::const_iterator it = sa.val.begin(); it != sa.val.end(); ++it)
        s << *it << ' ';
    return s << "]";
}



class Clock
{
public:
	void Set(double x,double y,bool pre = false);

    double Get(double x) const 
    { 
//        FLEXT_ASSERT(n > 1); 
        if(n <= 1)
            return 0;
        else
            return a+b*(BigNum(x)-x0); 
    } 

	double Current() const { return Get(Time()); }

	double Offset() const { return a; }
	double Factor() const { return b; }

	void Factor(double f)
    {
        double t = Time();
        double c = Get(t);
//        printf("CURRENT1 %lf->%lf\n",t,c);
        reset();
        // simulate existing data
        initfactor(t,c,f);
//        printf("CURRENT2 %lf->%lf\n",t,Get(t));
    }

    float Precision() const { return precision; }
    void Precision(float f) { precision = f; }

    float Weight() const { return weight; }
    void Weight(float w) { weight = w; }

    bool Logical() const { return logical; }
    void Logical(bool l) { logical = l; }

    const t_symbol *const name;


    static Clock *Register(const t_symbol *n,Client *c);
    static void Unregister(Clock *clk,Client *c);

    static Clock *Register(const t_symbol *n,Master *m);
    static void Unregister(Clock *clk,Master *m);

    typedef std::set<Client *> Clients;

    const Master *GetMaster() const { return master; }
    const Clients &GetClients() const { return clients; }


    double Time() const
    { 
        return logical?flext::GetTime():(flext::GetOSTime()-calibrate); 
    }

private:

    bool logical;
    int n;
    double prex,prey;

    BigNum x1,y1;
    BigNum x0,a,b;
    float weight;
    double calibrate;

    Clock(const t_symbol *n,Master *m = NULL)
        : name(n),master(m)
        , logical(true)
        , precision(1.e-10f)
        , weight(0.5)
    { 
        reset(); 
    }

    ~Clock() 
    { 
        FLEXT_ASSERT(!master);
        FLEXT_ASSERT(clients.empty()); 
    }

	void reset()
	{
        n = 0;
		x0 = a = b = 0;
        calibrate = flext::GetOSTime()-flext::GetTime();
	}

    void add(double x,double y)
    { 
        const BigNum _x = x,_y = y;

        if(LIKELY(n)) {
            if(LIKELY(_x != x1)) {
                const BigNum _a = (_y+y1)/2;
                const BigNum _b = (_y-y1)/(_x-x1);
                const BigNum _x0 = (_x+x1)/2;
                if(LIKELY(n > 1)) {
                    double w = weight,wi = 1-w;
                    x0 = x0*wi+_x0*w;                           
                    b = b*wi+_b*w;                           
                    a = a*wi+_a*w;                           
                }
                else
                    x0 = _x0,b = _b,a = _a;
            }
        }

        x1 = _x,y1 = _y;
        ++n;

        //        fprintf(stderr,"%i: %lf %lf %lf %lf %lf, %lf -> %lf %lf\n",n,(double)s,(double)sx,(double)sy,(double)sxx,(double)sxy,d,a,b);
    }

    void initfactor(double x,double y,double f)
    {
        add(x-0.5,y-f/2);
        add(x+0.5,y+f/2);
    }

    Master *master;
    float precision;

    Clients clients;

    typedef std::map<const t_symbol *,Clock *> Clocks;
    static Clocks clocks;

    static void TryFree(Clock *clk);
};



class ExcSyntax: public std::runtime_error { public: ExcSyntax(): runtime_error("Syntax error") {} };
class ExcExisting: public std::runtime_error { public: ExcExisting(): runtime_error("Name already existing") {} };


class Parent
    : public flext
{
protected:

    Parent(): clock(NULL) {}

	void mg_timebase(float &t) const 
    { 
        if(LIKELY(clock)) { 
            float f = static_cast<float>(clock->Factor()); 
            t = LIKELY(f)?1.f/f:0; 
        } 
        else 
            t = 0; 
    }

	void ms_timebase(float t)
    { 
        if(LIKELY(clock) && LIKELY(t))
            clock->Factor(1.f/t);
    }

	void mg_precision(float &p) const 
    { 
        p = LIKELY(clock)?clock->Precision():0;
    }

	void ms_precision(float p)
    {
        if(LIKELY(clock)) clock->Precision(p);
    }

	Clock *clock;
};

} // namespace

#endif
