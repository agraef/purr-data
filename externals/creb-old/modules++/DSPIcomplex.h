/*
 *   DSPIcomplex.h - Quick and dirty inline class for complex numbers 
 *   (mainly to compute filter poles/zeros, not to be used inside loops)
 *   Copyright (c) 2000 by Tom Schouten
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef DSPIcomplex_h
#define DSPIcomplex_h

#include <math.h>
#include <iostream>

class DSPIcomplex
{
    public:
        inline DSPIcomplex() {_r = _i = 0;}
        inline DSPIcomplex(const float &a, const float &b) {setCart(a, b);}
        inline DSPIcomplex(const float &phasor) {setAngle(phasor);}
        
        inline void setAngle(const float &angle) {_r = cos(angle); _i = sin(angle);}
        inline void setPolar(const float &phasor, const float &norm) 
        {_r = norm * cos(phasor); _i = norm * sin(phasor);}
        inline void setCart(const float &a, const float &b) {_r = a; _i = b;}
        
        inline const float& r() const {return _r;}
        inline const float& i() const {return _i;}
        
        inline float norm2() const {return _r*_r+_i*_i;}
        inline float norm() const {return sqrt(norm2());}
        inline void normalize() {float n = 1.0f / norm(); _r *= n; _i *= n;}
        
        inline DSPIcomplex conj() const {return DSPIcomplex(_r, -_i);}

        inline float angle() const {return atan2(_i, _r);}


        inline DSPIcomplex operator+ (const DSPIcomplex &a) const
        {
            return DSPIcomplex(_r + a.r(), _i + a.i());
        }
        inline DSPIcomplex operator+ (float f) const
        {
            return DSPIcomplex(_r + f, _i);
        }
        inline DSPIcomplex operator- (const DSPIcomplex &a) const
        {
            return DSPIcomplex(_r - a.r(), _i - a.i());
        }
        inline DSPIcomplex operator- (float f) const
        {
            return DSPIcomplex(_r - f, _i);
        }

        inline DSPIcomplex operator* (const DSPIcomplex &a) const 
        {
            return DSPIcomplex(_r * a.r() - _i * a.i(), _i * a.r() + _r * a.i());
        }
        inline DSPIcomplex operator* (float f) const
        {
            return DSPIcomplex(_r * f, _i * f);
        }
        inline DSPIcomplex operator/ (const DSPIcomplex &a) const 
        {
            float n_t = 1.0f / a.norm2();
            return DSPIcomplex(n_t * (_r * a.r() + _i * a.i()), n_t * (_i * a.r() - _r * a.i()));
        }
        inline DSPIcomplex operator/ (float f) const 
        {
            float n_t = 1.0f / f;
            return DSPIcomplex(n_t * _r, n_t * _i);
        }
        
        inline friend std::ostream& operator<< (std::ostream& o, DSPIcomplex& a)
        {
            return o << "(" << a.r() << "," << a.i() << ")";
        }

        inline friend DSPIcomplex operator+ (float f, DSPIcomplex& a)
        {
            return(DSPIcomplex(a.r() + f, a.i()));
        }
        
        inline friend DSPIcomplex operator- (float f, DSPIcomplex& a)
        {
            return(DSPIcomplex(f - a.r(), - a.i()));
        }
        
        inline friend DSPIcomplex operator/ (float f, DSPIcomplex& a)
        {
            return(DSPIcomplex(f,0) / a);
        }
        
        // ????
        inline friend DSPIcomplex operator* (float f, DSPIcomplex& a)
        {
            return(DSPIcomplex(f*a.r(), f*a.i()));
        }
        
        
        inline DSPIcomplex& operator *= (float f)
        {
            _r *= f;
            _i *= f;
            return *this;
        }

        inline DSPIcomplex& operator /= (float f)
        {
            _r /= f;
            _i /= f;
            return *this;
        }

        inline DSPIcomplex& operator *= (DSPIcomplex& a)
        {
            float r_t = _r * a.r() - _i * a.i();
                   _i = _r * a.i() + _i * a.r();
                   _r = r_t;
                   
            return *this;
        }

        inline DSPIcomplex& operator /= (DSPIcomplex& a)
        {
            float n_t = a.norm2();
            float r_t = n_t * (_r * a.r() + _i * a.i());
                   _i = n_t * (_i * a.r() - _r * a.i());
                   _r = r_t;
                   
            return *this;
        }

        
        float _r;
        float _i;
};


// COMPLEX LOG

inline DSPIcomplex dspilog(DSPIcomplex a) /* complex log */
{
    float r_t = log(a.norm());
    float i_t = a.angle();
    return DSPIcomplex(r_t, i_t);
}

// COMPLEX EXP

inline DSPIcomplex dspiexp(DSPIcomplex a) /* complex exp */
{
    return (DSPIcomplex(a.i()) * exp(a.r()));
}

// BILINEAR TRANSFORM analog -> digital

inline DSPIcomplex bilin_stoz(DSPIcomplex a)
{
    DSPIcomplex a2 = a * 0.5f;
    return((1.0f + a2)/(1.0f - a2));
}    

// BILINEAR TRANSFORM digital -> analog

inline DSPIcomplex bilin_ztos(DSPIcomplex a)
{
    return ((a - 1.0f) / (a + 1.0f))*2.0f;
}

// not really a complex function but a nice complement to the bilinear routines

inline float bilin_prewarp(float freq)
{
    return 2.0f * tan(M_PI * freq);
}    

#endif //DSPIcomplex_h
