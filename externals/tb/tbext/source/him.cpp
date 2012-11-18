/* Copyright (c) 2004 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* him~ is a semi-classicical simulation of an hydrogen atom in a magnetic field*/
/*                                                                              */
/* him~ uses the flext C++ layer for Max/MSP and PD externals.                  */
/* get it at http://www.parasitaere-kapazitaeten.de/PD/ext                      */
/* thanks to Thomas Grill                                                       */
/*                                                                              */
/* him~ is based on code provided in the lecture "physik auf dem computer 1"    */
/* held by joerg main during the winter semester 2003/04 at the university      */
/* stuttgart ... many thanks to him and his assistant ralf habel                */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* See file LICENSE for further informations on licensing terms.                */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* coded while listening to: Elliott Sharp: The Velocity Of Hue                 */
/*                           Fred Frith: Traffic Continues                      */
/*                           Nmperign: Twisted Village                          */
/*                           Frank Lowe: Black Beings                           */
/*                                                                              */



#include <flext.h>
#include <cstdlib>
#include <cmath>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error upgrade your flext version!!!!!!
#endif

#define NUMB_EQ 4

class him: public flext_dsp
{
  FLEXT_HEADER(him,flext_dsp);

public: 
    him(int argc, t_atom *argv);

protected:
    virtual void m_signal (int n, float *const *in, float *const *out);
    virtual void m_dsp (int n, float *const *in, float *const *out);
    
    void set_mu(t_float);
    void set_muv(t_float);
    void set_nu(t_float);
    void set_nuv(t_float);
    void set_etilde(t_float);
    void set_dt(t_float);
    void set_regtime(bool);
    void state();
    void reset();

private:
    // contains DGL-System
    t_float deriv(t_float x[],int eq);

    // 4th order Runge Kutta update of the dynamical variables 
    void runge_kutta_4(t_float dt);

    //these are our data
    t_float data[4]; //mu, muv, nu, nuv (semi-parabolische koordinaten)
    t_float E;

    //and these our settings
    t_float dt;
    bool regtime; //if true "regularisierte zeit"

    bool xfade;
    t_float newE;
    t_float newdt;
    bool newsystem;
    bool newregtime;

    t_float * m_fader;

    //Callbacks
    FLEXT_CALLBACK_1(set_mu,t_float);
    FLEXT_CALLBACK_1(set_muv,t_float);
    FLEXT_CALLBACK_1(set_nu,t_float);
    FLEXT_CALLBACK_1(set_nuv,t_float);
    FLEXT_CALLBACK_1(set_etilde,t_float);
    FLEXT_CALLBACK_1(set_dt,t_float);
    FLEXT_CALLBACK_1(set_regtime,bool);
    FLEXT_CALLBACK(state);
    FLEXT_CALLBACK(reset);

    //reset mus / nus 
    void reset_nuv()
    {
	data[3]= 0.5*sqrt( - (4*data[1]*data[1]) - 
			   ( data[0]*data[0]*data[1]*data[1]*data[1]*data[1])
			   + (8*E*data[0]) - (8*E*data[2]) - 
			   (data[0]*data[0]*data[0]*data[0]*data[1]*data[1])
			   + 16);
     	if (fabs((data[3]))<1e-5)
 	    data[3]=0;
    }
    
    void reset_muv()
    {
	data[1]= 0.5*sqrt( - (4*data[3]*data[3]) - 
			   ( data[0]*data[0]*data[1]*data[1]*data[1]*data[1])
			   + (8*E*data[0]) - (8*E*data[2]) - 
			   (data[0]*data[0]*data[0]*data[0]*data[1]*data[1])
			   + 16);
 	if (fabs((data[1]))<1e-5)
 	    data[1]=0;
    }
    
};


FLEXT_LIB_DSP_V("him~",him)

him::him(int argc, t_atom *argv)
{
    AddInAnything();
    AddOutSignal();
    AddOutSignal();
    AddOutSignal();
    AddOutSignal();
    AddOutSignal();
    AddOutSignal();
    FLEXT_ADDMETHOD_F(0,"mu",set_mu);
    FLEXT_ADDMETHOD_F(0,"muv",set_muv);
    FLEXT_ADDMETHOD_F(0,"nu",set_nu);
    FLEXT_ADDMETHOD_F(0,"nuv",set_nuv);
    FLEXT_ADDMETHOD_F(0,"e",set_etilde);
    FLEXT_ADDMETHOD_F(0,"dt",set_dt);
    FLEXT_ADDMETHOD_B(0,"regtime",set_regtime);
    FLEXT_ADDMETHOD_(0,"state",state);
    FLEXT_ADDMETHOD_(0,"reset",reset);
    
    
    //beginning values
    if (argc==1)
	E=atom_getfloat(argv);
    else
	E= -float(rand())/float(RAND_MAX);
    
    reset();

    state();

    //default mode
    regtime=true;
    dt=0.01;
} 

inline t_float him::deriv(t_float * x, int eq)
{
    t_float result;
    // set DGL-System here
    if (eq == 0) result =  x[1];
    if (eq == 1) result = 2*E*x[0]-0.25*x[0]*x[2]*x[2]*(2*x[0]*x[0]+x[2]*x[2]);
    if (eq == 2) result =  x[3];
    if (eq == 3) result = 2*E*x[2]-0.25*x[2]*x[0]*x[0]*(2*x[2]*x[2]+x[0]*x[0]);

  return result;
}

inline void him::runge_kutta_4(t_float dt)                          
{
    t_float k1[NUMB_EQ],k2[NUMB_EQ],k3[NUMB_EQ],k4[NUMB_EQ];
    t_float temp1[NUMB_EQ], temp2[NUMB_EQ], temp3[NUMB_EQ];
    
    for(int i=0;i<=NUMB_EQ-1;i++) // iterate over equations 
    {
	k1[i] = dt * deriv(data,i);
	temp1[i] = data[i] + 0.5*k1[i];	    	    
    }
    
    for(int i=0;i<=NUMB_EQ-1;i++)
    {
	k2[i] = dt * deriv(temp1,i);
	temp2[i] = data[i] + 0.5*k2[i];
    }
    
    for(int i=0;i<=NUMB_EQ-1;i++)
    {
	k3[i] = dt * deriv(temp2,i);
	temp3[i] = data[i] + k3[i];    
    }
    
    for(int i=0;i<=NUMB_EQ-1;i++)
    {
	k4[i] = dt * deriv(temp3,i);
	data[i] = data[i] + (k1[i] + (2.*(k2[i]+k3[i])) + k4[i])/6.;
    	
	// we don't want to experience denormals in the next step */
	if(fabs((data[i]))<1e-5)   
	    data[i]=0;
    }

    
    /*
      the system might become unstable ... in this case, we'll request a new system
    */    

    for(int i=0;i<=NUMB_EQ-1;i++)
    {
	if(data[i]>2)
	    {
		xfade = newsystem =  true;
		data[i] = 2;
	    }
	if(data[i]<-2)
	    {
		xfade = newsystem = true;
		data[i] = -2;
	    }
    }
}



void him::m_signal(int n, t_float *const *in, t_float *const *out)
{
    t_float * out0 = out[0];
    t_float * out1 = out[1];
    t_float * out2 = out[2];
    t_float * out3 = out[3];
    t_float * out4 = out[4];
    t_float * out5 = out[5];

    
    if (regtime)
    {
	for (int i=0;i!=n;++i)
	{
	    runge_kutta_4(dt);
	    (*(out0)++)=data[0];
	    (*(out1)++)=data[1];
	    (*(out2)++)=data[2];
	    (*(out3)++)=data[3];
	    (*(out4)++)=data[0]*data[2];
	    (*(out5)++)=(data[0]*data[0]-data[2]*data[2])*0.5;
	}
    }
    else
    {
	for (int i=0;i!=n;++i)
	{
	    runge_kutta_4(dt/(2*sqrt(data[0]*data[0]+data[2]*data[2])));
	    (*(out0)++)=data[0];
	    (*(out1)++)=data[1];
	    (*(out2)++)=data[2];
	    (*(out3)++)=data[3];
	    (*(out4)++)=data[0]*data[2];
	    (*(out5)++)=(data[0]*data[0]-data[2]*data[2])*0.5;
	}
    }
    
    if (xfade)
    {
	/* fading */
	out0 = out[0];
	out1 = out[1];
	out2 = out[2];
	out3 = out[3];
	out4 = out[4];
	out5 = out[5];
	
	t_float * fader = m_fader + n - 1;
	for (int i=0;i!=n;++i)
	{
	    (*(out0)++) *= *fader;
	    (*(out1)++) *= *fader;
	    (*(out2)++) *= *fader;
	    (*(out3)++) *= *fader;
	    (*(out4)++) *= *fader;
	    (*(out5)++) *= *fader--;
	}

	if (newsystem)
	{
	    reset();
	    newsystem = false;
	}
	
	E = newE;
	dt = newdt;
	regtime = newregtime;

	out0 = out[0];
	out1 = out[1];
	out2 = out[2];
	out3 = out[3];
	out4 = out[4];
	out5 = out[5];
    
	fader = m_fader;
	if (regtime)
	{
	    for (int i=0;i!=n;++i)
	    {
		runge_kutta_4(dt);
		(*(out0)++)+= data[0]* *fader;
		(*(out1)++)+= data[1]* *fader;
		(*(out2)++)+= data[2]* *fader;
		(*(out3)++)+= data[3]* *fader;
		(*(out4)++)+= data[0]*data[2]* *fader;
		(*(out5)++)+= (data[0]*data[0]-data[2]*data[2])*0.5* *fader++;
	    }
	}
	else
	{
	    for (int i=0;i!=n;++i)
	    {
		runge_kutta_4(dt/(2*sqrt(data[0]*data[0]+data[2]*data[2])));
		(*(out0)++)+= data[0]* *fader;
		(*(out1)++)+= data[1]* *fader;
		(*(out2)++)+= data[2]* *fader;
		(*(out3)++)+= data[3]* *fader;
		(*(out4)++)+= data[0]*data[2]* *fader;
		(*(out5)++)+= (data[0]*data[0]-data[2]*data[2])*0.5* *fader++;
	    }
	}
	
	xfade = false;
    }
    
}    

void him::m_dsp(int n, t_float *const *in, t_float *const *out)
{
    m_fader = new t_float[n];
    t_float on = 1.f/(n-1);
    
    t_float value = 0;

    t_float* localfader = m_fader;

    while (n--)
    {
	*localfader = value;
	value += on;
	++localfader;
    }
    xfade = false;
}


void him::set_mu(t_float f)
{
    data[0]=f;
    reset_nuv();
}

void him::set_muv(t_float f)
{
    data[1]=f;
    reset_nuv();
}

void him::set_nu(t_float f)
{
    data[3]=f;
    reset_nuv();
}

void him::set_nuv(t_float f)
{
    data[3]=f;
    reset_muv();
}

void him::set_etilde(t_float f)
{
    newE=f;
    xfade = true;
    //reset_nuv();
}

void him::set_dt(t_float f)
{
    newdt=f;
    xfade = true;
}

void him::set_regtime(bool b)
{
    newregtime=b;
    xfade = true;
}


void him::state()
{
    post("mu %f",data[0]);
    post("mus %f",data[1]);
    post("nu %f",data[2]);
    post("nus %f",data[3]);
    post("etilde %f",E);
}

void him::reset()
{
    data[0]=float(rand())/float(RAND_MAX);
    data[1]=float(rand())/float(RAND_MAX);
    data[2]=float(rand())/float(RAND_MAX);
    reset_nuv();
}
