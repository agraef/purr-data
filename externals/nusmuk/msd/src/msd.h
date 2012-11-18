

/* 
 msd - mass spring damper model for Pure Data or Max/MSP

 Copyright (C) 2005  Nicolas Montgermont
 Written by Nicolas Montgermont 
 Optimized by Thomas Grill for Flext
 Based on pmpd by Cyrille Henry 
 Based on Pure Data by Miller Puckette and others

 Contact : Nicolas Montgermont, nicolas_montgermont @ yahoo dot fr
	   Cyrille Henry, Cyrille.Henry @ la-kitchen dot fr

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 Version 0.09 -- 12.09.2010
*/

// include flext header
#include <flext.h>
#include <flmap.h>
#include <math.h>
#include <string.h>
#include <vector>

// define constants
#define MSD_VERSION  0.09
#define PI  3.1415926535

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#ifdef _MSC_VER
#define NEWARR(type,var,size) type *var = new type[size]
#define DELARR(var) delete[] var
#else
#define NEWARR(type,var,size) type var[size]
#define DELARR(var) ((void)0)
#endif

// Maths functions
inline t_float sqr(t_float x) { return x*x; }

template<int N> class Link;

class t_buffer {
public:
	t_int nbr;
	const t_symbol *Id;
	flext::buffer *buf;
	bool tested;
	t_int size;

	t_buffer(t_int n,const t_symbol *id):nbr(n),Id(id) {
		buf = new flext::buffer(Id);
		tested=true;
		if(!buf->Ok()) {
   			post("error : table %s not found!", *Id); 
   			tested = false;
		}
	}
	
	~t_buffer() {
		if(buf) {
			delete buf;
			buf=NULL;
			Id=NULL;
		}
	}	
	
	inline void buffer_test() {
		if(!buf || !buf->Ok() ) {
			post("no %s table",*Id); 
  			tested = false;
			} 
		else if(!buf->Valid()) { 
  			post("no valid %s table",*Id); 
  			tested = false; 
 		}  
 		else { 
  			if(!buf || buf->Update()) { 
   			// buffer parameters have been updated 
   				if(buf->Valid()) { 
    				post("updated buffer %s reference", *Id); 
    				tested = true; 
   				} 
   				else { 
    				post("buffer %s has become invalid", *Id); 
    				tested = false; 
   				}
   			}  
  			else 
   				tested = true;
 		}	   
 	}
 	
 	inline t_float interp_buf(t_float indexf, t_float factor) {
		t_float size_buf=buf->Frames();
		t_float index_factor = indexf*(size_buf-1)/factor;
		if (index_factor > size_buf - 1)
			return buf->Data()[(int)size_buf - 1];
		else if (index_factor < 0)
			return buf->Data()[0];
		else {
			t_int index = floor(index_factor);
			t_float interp = index_factor - (float)index;
			if (index==index_factor)
				return buf->Data()[index];
			else 
				return (1-interp) * buf->Data()[index] + interp * buf->Data()[index+1];
		}
	}
};

template<int N> 
class LinkList	: public std::vector<Link<N> *> {
public:
	void insert(Link<N> *l) 
	{
		for(typename LinkList<N>::iterator it = this->begin(); it != this->end(); ++it)
			if(*it == l) return;
		// not found -> add
		push_back(l);
	}
	
	void erase(Link<N> *l)
	{
		for(typename LinkList<N>::iterator it = this->begin(); it != this->end(); ++it)
			if(*it == l) { 
				// found
				std::vector<Link<N> *>::erase(it); 
				return; 
			}
	}
};

template<int N>
class Mass {
public:
	t_int nbr;
	const t_symbol *Id;
	t_float M,invM;
	t_float speed[N];
	t_float pos[N];
	t_float pos2[N];
	t_float force[N];
	t_float out_force[N];
	LinkList<N> links;
	
	Mass(t_int n,const t_symbol *id,bool mob,t_float m,t_float p[N])
		: nbr(n),Id(id)
		, M(m)
	{
		if(mob) setMobile(); else setFixed();
	
		for(int i = 0; i < N; ++i) {
			pos[i] = pos2[i] = p[i];
			force[i] = speed[i] = 0;
		}
	}

	inline void setForce(int n,t_float f) { force[n] += f; }
	
	inline void setForce(t_float f[N]) 
	{ 
		for(int i = 0; i < N; ++i) setForce(i,f[i]); 
	}

	inline void setPos(int n,t_float p) { pos[n] = pos2[n] = p; }
	
	inline void setPos(t_float p[N]) 
	{ 
		for(int i = 0; i < N; ++i) setPos(i,p[i]);
	}
	
	inline bool getMobile() const { return invM != 0; }
	
	inline void setMobile() { invM = M?1/M:0.; }
	inline void setFixed() { invM = 0; }
	
	inline void compute(t_float limit[N][2])
	{
		for(int i = 0; i < N; ++i) {
			t_float pold = pos[i];
			t_float pnew;
			if(invM) // if mass is mobile
				pnew = force[i] * invM + 2*pold - pos2[i]; // x[n] =Fx[n]/M+2x[n]-x[n-1]
			else // if mass is fixed
				pnew = pos[i];
			
			// check limit
			if(pnew < limit[i][0]) pnew = limit[i][0]; else if(pnew > limit[i][1]) pnew = limit[i][1];
			speed[i] = (pos[i] = pnew) - (pos2[i] = pold);	// x[n-2] = x[n-1], x[n-1] = x[n],vx[n] = x[n] - x[n-1]

			// clear forces
			out_force[i] = force[i];
			force[i] = 0;						// Fx[n] = 0
		}
	}

	static inline t_float dist(const Mass &m1,const Mass &m2) 
	{
		if(N == 1) 
			return fabs(m1.pos[0]-m2.pos[0]);		// L[n] = |x1 - x2|
		else {
			t_float distance = 0;
			for(int i = 0; i < N; ++i) distance += sqr(m1.pos[i]-m2.pos[i]);
			return sqrt(distance);
		}
	}
};

template<int N>
class Link {
	
public:
	t_int nbr;
	const t_symbol *Id;
	Mass<N> *mass1,*mass2;
	t_float K1, D1, D2;
	t_float longueur, long_min, long_max;
	t_float distance_old;
	t_float puissance;
	t_int link_type; //0 : no, 1 : tangential, 2 : normal, 3 : table
	t_float tdirection1[N], tdirection2[N];
	t_float l_tab, l_tab2;
	t_int buffer_tested;
	t_buffer *k_buffer,*d_buffer;
	
	Link(t_int n,const t_symbol *id,Mass<N> *m1,Mass<N> *m2,t_float k1,t_float d1, t_int o=0, t_float tangent[N]=NULL,t_float pow=1, t_float lmin = 0,t_float lmax = 1e10,t_buffer *ktab=NULL, t_float ltab=1, t_buffer *dtab=NULL, t_float ltab2=1)
	: nbr(n),Id(id)
	, mass1(m1),mass2(m2)
	, K1(k1),D1(d1),D2(0),link_type(o),puissance(pow)
	, long_min(lmin),long_max(lmax)
	, l_tab(ltab), l_tab2(ltab2)
	, buffer_tested(1)
	, k_buffer(ktab),d_buffer(dtab)
	{
		for (int i=0; i<N; i++)	{
			tdirection1[i] = 0;
			tdirection2[i] = 0;
			}
		if (link_type == 1)	{			// TANGENTIAL LINK
			t_float norme = 0;
			for(int i = 0; i < N; ++i) 
				norme += sqr(tangent[i]);
			norme = sqrt(norme);
			for(int i = 0; i < N; ++i)
				tdirection1[i] = tangent[i]/norme;
			distance_old = 0;
			for(int i = 0; i < N; ++i)	
				distance_old += sqr((m1->pos[i]-m2->pos[i])*tdirection1[i]);
			distance_old  = sqrt(distance_old);
			longueur = distance_old;
		}
		else
			distance_old = longueur = Mass<N>::dist(*mass1,*mass2); // L[n-1]
		mass1->links.insert(this);
		mass2->links.insert(this);
	}
	
	~Link() {
		mass1->links.erase(this);
		mass2->links.erase(this);
	}

	// compute link forces
	inline void compute() 
	{
		t_float distance=0;
		t_float F;
		Mass<N> *m1 = mass1,*m2 = mass2; // cache locally
		if (m1->invM || m2->invM) { 
			if (link_type == 1) {
				for(int i = 0; i < N; ++i)	
					distance += sqr((m1->pos[i]-m2->pos[i])*tdirection1[i]);
				distance = sqrt(distance);
			}
			else if (link_type == 2) {
				for(int i = 0; i < N; ++i)	
					distance += sqr((m1->pos[i]-m2->pos[i])*(tdirection1[i] +tdirection2[i]));
				distance = sqrt(distance);
			}
			else 
				distance = Mass<N>::dist(*m1,*m2); 
					
			if (distance < long_min || distance > long_max || distance == 0) {
			// pas de forces
			}
			else {	// Lmin < L < Lmax
				// F[n] = k1 (L[n] - L[0])/L[n] + D1 (L[n] - L[n-1])/L[n]
				if (link_type == 3 ) {//&& buffer_tested) { // tabLink
					t_float k_temp = distance-longueur;
					if (k_buffer && k_buffer->tested) {
						k_temp = k_buffer->interp_buf(distance,l_tab);
					}	
					t_float d_temp = distance-distance_old;	
					if (d_buffer && d_buffer->tested) {
						if (distance>distance_old)
							d_temp = d_buffer->interp_buf(distance-distance_old,l_tab2);
						else
							d_temp = -d_buffer->interp_buf(distance_old - distance,l_tab2);
					}
					F = (K1*k_temp + D1*d_temp)/distance; 
				}
				else {
					if ((distance - longueur)>0)
						F  = (K1 * pow(distance - longueur,puissance) + D1 * (distance - distance_old))/distance ;
					else
						F  = (-K1 * pow(longueur - distance,puissance) + D1 * (distance - distance_old))/distance ;
				}
					if (link_type == 1 || (link_type == 2 && N == 2)) // tangential
						for(int i = 0; i < N; ++i) {
							const t_float Fn = F * (m1->pos[i] - m2->pos[i])*tdirection1[i]; // Fx = F * Lx[n]/L[n]
							m1->force[i] -= Fn + D2 * m1->speed[i]; 	//  Fx1[n] = -Fx, Fx1[n] = Fx1[n] - D2 * vx1[n-1]
							m2->force[i] += Fn - D2 * m2->speed[i]; 	// Fx2[n] = Fx, Fx2[n] = Fx2[n] - D2 * vx2[n-1]
						}
					else if (link_type == 2 && N == 3) // deprecated
						for(int i = 0; i < N; ++i) {
							const t_float Fn = F * (m1->pos[i] - m2->pos[i])*(tdirection1[i] +tdirection2[i]); // Fx = F * Lx[n]/L[n]
							m1->force[i] -= Fn + D2 * m1->speed[i]; 	//  Fx1[n] = -Fx, Fx1[n] = Fx1[n] - D2 * vx1[n-1]
							m2->force[i] += Fn - D2 * m2->speed[i]; 	// Fx2[n] = Fx, Fx2[n] = Fx2[n] - D2 * vx2[n-1]
						}
					else 	 // usual link 
						for(int i = 0; i < N; ++i) {
							const t_float Fn = F * (m1->pos[i] - m2->pos[i]); // Fx = F * Lx[n]/L[n]
							m1->force[i] -= Fn + D2 * m1->speed[i]; 	//  Fx1[n] = -Fx, Fx1[n] = Fx1[n] - D2 * vx1[n-1]
							m2->force[i] += Fn - D2 * m2->speed[i]; 	// Fx2[n] = Fx, Fx2[n] = Fx2[n] - D2 * vx2[n-1]
						}
				
			}
			
			distance_old = distance;				// L[n-1] = L[n]			
		}
	}
};



template <typename T>
inline T bitrev(T k) {
	T r = 0;
	for(int i = 0; i < sizeof(k)*8; ++i) r = (r<<1)|(k&1),k >>= 1;
	return r;
}

// use bit-reversed key to pseudo-balance the map tree
template <typename T>
class IndexMap : TablePtrMap<unsigned int,T,64> {
public:
	typedef TablePtrMap<unsigned int,T,64> Parent;
	
    virtual ~IndexMap() { reset(); }

	void reset() 
	{ 
		// delete all associated items
		for(typename Parent::iterator it(*this); it; ++it) delete it.data();
		Parent::clear(); 
	}
	
	inline int size() const { return Parent::size(); }

	inline T insert(unsigned int k,T v) { return Parent::insert(bitrev(k),v); }	

	inline T find(unsigned int k) { return Parent::find(bitrev(k)); }

	inline T remove(unsigned int k) { return Parent::remove(bitrev(k)); }
	
	class iterator
		: public Parent::iterator
	{
	public:
		iterator() {}
		iterator(IndexMap &m): Parent::iterator(m) {}
		inline unsigned int key() const { return bitrev(Parent::key()); }
	};
};

template <typename T>
class IDMap	: TablePtrMap<const t_symbol *,TablePtrMap<T,T,4> *,4> {
public:
	// that's the container holding the data items (masses, links) of one ID
	typedef TablePtrMap<T,T,4> Container;
	// that's the map for the key ID (symbol,int) relating to the data items
	typedef TablePtrMap<const t_symbol *,Container *,4> Parent;

	typedef typename Container::iterator iterator;

	IDMap() {}
	
	virtual ~IDMap() { reset(); }
	
	void reset() 
	{
        typename Parent::iterator it(*this);
        for(; it; ++it) delete it.data();
		Parent::clear();
	}
	
	void insert(T item)
	{
		Container *c = Parent::find(item->Id);
        if(!c)
            Parent::insert(item->Id,c = new Container);
        c->insert(item,item);
	}
	
	iterator find(const t_symbol *key)
	{
		Container *c = Parent::find(key);
		if(c)
			return iterator(*c);
        else
			return iterator();
	}
	
	void erase(T item)
	{
		Container *c = Parent::find(item->Id);
		if(c) c->remove(item);
	}
};


template<int N>
class msdN:	public flext_base {
	FLEXT_HEADER_S(msdN,flext_base,setup)	//class with setup
 
public:
	// constructor with no arguments
	msdN(int argc,t_atom *argv)		: id_mass(0),id_link(0) {
		for(int i = 0; i < N; ++i) limit[i][0] = -1.e10,limit[i][1] = 1.e10;
	
		// --- define inlets and outlets ---
		AddInAnything("bang, reset, etc."); 	// default inlet
		AddOutAnything("infos on masses");	// outlet for integer count
		AddOutAnything("control");		// outlet for bang
	}

	virtual ~msdN() { clear(); }

protected:

// --------------------------------------------------------------  PROTECTED VARIABLES 
// -----------------------------------------------------------------------------------

	typedef Mass<N> t_mass;
	typedef Link<N> t_link;

	IndexMap<t_link *> link;		// links	
	IDMap<t_link *> linkids;		// links by name
	IndexMap<t_mass *> mass;		// masses
	IDMap<t_mass *> massids;		// masses by name
	IndexMap<t_buffer *> buffers;		// buffers
	IDMap<t_buffer *> buffersids;		// buffers by name
		
	t_float limit[N][2];			// Limit values
	unsigned int id_mass, id_link, id_buffer, mouse_grab, nearest_mass, link_deleted, mass_deleted;

// ---------------------------------------------------------------  RESET 
// ----------------------------------------------------------------------
	void m_reset() 	{ 
		clear();
		ToOutAnything(1,S_Reset,0,NULL);
	}

// --------------------------------------------------------------  COMPUTE 
// -----------------------------------------------------------------------

	void m_bang()	{
		// test all buffers
		for (typename IndexMap<t_buffer *>::iterator bit(buffers); bit; ++bit) bit.data()->buffer_test();
		
		// update all links
		for (typename IndexMap<t_link *>::iterator lit(link); lit; ++lit) lit.data()->compute();

		// update all masses
		for (typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) mit.data()->compute(limit);
	}

// --------------------------------------------------------------  MASSES
// ----------------------------------------------------------------------

	// add a mass
	// Id, nbr, mobile, invM, speedX, posX, forceX
	void m_mass(int argc,t_atom *argv) 	{
		if(argc != 3+N) {
			error("mass : Id mobile mass X%s%s",N >= 2?" Y":"",N >= 3?" Z":"");
			return;
		}
		
		t_float pos[N];
		for(int i = 0; i < N; ++i) pos[i] = GetAFloat(argv[3+i]);

		t_mass *m = new t_mass(
			id_mass, // index
			GetSymbol(argv[0]), // ID
			GetABool(argv[1]), // mobile
			GetAFloat(argv[2]), // mass
			pos // pos
		);
		
		outmass(S_Mass,m);

		massids.insert(m);
		mass.insert(id_mass++,m);
	}

	// add a force to mass(es) named Id or No
	void m_force(int argc,t_atom *argv,int n)	{
		if(argc != 2) {
			error("%s - %s Syntax : Id/Nomass value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float f = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_mass *>::iterator it;
            for(it = massids.find(GetSymbol(argv[0])); it; ++it) {
                t_mass *m = it.data();
				m->setForce(n,f);
            }
		}
		else {
			t_mass *m = mass.find(GetAInt(argv[0]));
			if(m) 
				m->setForce(n,f);
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}

	inline void m_forceX(int argc,t_atom *argv) { m_force(argc,argv,0); }
	inline void m_forceY(int argc,t_atom *argv) { m_force(argc,argv,1); }
	inline void m_forceZ(int argc,t_atom *argv) { m_force(argc,argv,2); }
	inline void m_forceN(int argc,t_atom *argv) {
		t_atom arglist[2];

		if(argc != 3) {
			error("%s - %s Syntax : N Id/Nomass value",thisName(),GetString(thisTag()));
			return;
		}

		if (IsSymbol(argv[1]))
			SetSymbol(arglist[0],GetSymbol(argv[1]));
		else
			SetInt(arglist[0],GetAInt(argv[1]));
		SetFloat(arglist[1],GetFloat(argv[2]));
		m_force(argc-1,arglist,GetAInt(argv[0])-1);
	 }

	// displace mass(es) named Id or No to a certain position
	void m_pos(int argc,t_atom *argv,int n)	{
		if(argc != 2) {
			error("%s - %s Syntax : Id/Nomass value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float p = GetAFloat(argv[1]);
		if(p > limit[n][1] || p < limit[n][0])  return;
		
		if(IsSymbol(argv[0])) {
			typename IDMap<t_mass *>::iterator it;
			for(it = massids.find(GetSymbol(argv[0])); it; ++it)
				it.data()->setPos(n,p);
		}
		else {
			t_mass *m = mass.find(GetAInt(argv[0]));
			if(m) 
				m->setPos(n,p);
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}

	inline void m_posX(int argc,t_atom *argv) { m_pos(argc,argv,0); }
	inline void m_posY(int argc,t_atom *argv) { m_pos(argc,argv,1); }
	inline void m_posZ(int argc,t_atom *argv) { m_pos(argc,argv,2); }
	inline void m_posN(int argc,t_atom *argv) {
		t_atom arglist[2];

		if(argc != 3) {
			error("%s - %s Syntax : N Id/Nomass value",thisName(),GetString(thisTag()));
			return;
		}

		if (IsSymbol(argv[1]))
			SetSymbol(arglist[0],GetSymbol(argv[1]));
		else
			SetInt(arglist[0],GetAInt(argv[1]));
		SetFloat(arglist[1],GetFloat(argv[2]));
		m_pos(argc-1,arglist,GetAInt(argv[0])-1);
	 }

	// set mass to mobile
	void m_set_mobile(int argc,t_atom *argv,bool mob = true) 	{
		if (argc != 1) {
			error("%s - %s Syntax : Id/Nomass",thisName(),GetString(thisTag()));
			return;
		}
		if(IsSymbol(argv[0])) {
			typename IDMap<t_mass *>::iterator it;
			if(mob)
				for(it = massids.find(GetSymbol(argv[0])); it; ++it)
					it.data()->setMobile();
			else
				for(it = massids.find(GetSymbol(argv[0])); it; ++it)
					it.data()->setFixed();
		}
		else {	
			t_mass *m = mass.find(GetAInt(argv[0]));
			if(m) 
				if(mob) m->setMobile(); 
				else m->setFixed();
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}

	// set mass No to fixed
	inline void m_set_fixe(int argc,t_atom *argv) { m_set_mobile(argc,argv,false); }

	// Delete mass
	void m_delete_mass(int argc,t_atom *argv)	{
		if (argc != 1) {
			error("%s - %s Syntax : Nomass",thisName(),GetString(thisTag()));
			return;
		}
		

                t_mass *m = mass.find(GetAInt(argv[0]));
                if(m) {
                        // Delete all associated links

            while(!m->links.empty())
                deletelink(m->links.front());

                        outmass(S_Mass_deleted,m);
                        massids.erase(m);
                        mass.remove(m->nbr); 

			delete m;
			mass_deleted = 1;
		}
		else
			error("%s - %s : Index not found",thisName(),GetString(thisTag()));
	}

	// set X,Y,Z min/max
	void m_limit(int argc,t_atom *argv,int n,int i) 	{
		if (argc != 1)
			error("%s - %s Syntax : Value",thisName(),GetString(thisTag()));
		else
			limit[n][i] = GetAFloat(argv[0]);
	}

	inline void m_Xmin(int argc,t_atom *argv) { m_limit(argc,argv,0,0); }
	inline void m_Ymin(int argc,t_atom *argv) { m_limit(argc,argv,1,0); }
	inline void m_Zmin(int argc,t_atom *argv) { m_limit(argc,argv,2,0); }
	inline void m_Nmin(int argc,t_atom *argv) {
		t_atom arglist[1];

		if(argc != 2) {
			error("%s - %s Syntax : N value",thisName(),GetString(thisTag()));
			return;
		}

		SetFloat(arglist[0],GetFloat(argv[1]));
		m_limit(argc-1,arglist,GetAInt(argv[0])-1,0);
	 }

	inline void m_Xmax(int argc,t_atom *argv) { m_limit(argc,argv,0,1); }
	inline void m_Ymax(int argc,t_atom *argv) { m_limit(argc,argv,1,1); }
	inline void m_Zmax(int argc,t_atom *argv) { m_limit(argc,argv,2,1); }
	inline void m_Nmax(int argc,t_atom *argv) {
		t_atom arglist[1];

		if(argc != 2) {
			error("%s - %s Syntax : N value",thisName(),GetString(thisTag()));
			return;
		}

		SetFloat(arglist[0],GetFloat(argv[1]));
		m_limit(argc-1,arglist,GetAInt(argv[0])-1,1);
	 }

	// set Id of mass(s) named Id or number No
	void m_setMassId(int argc,t_atom *argv) 	{
		if (argc != 2) {
			error("%s - %s Syntax : OldId/NoMass NewId",thisName(),GetString(thisTag()));
			return;
		}

		const t_symbol *id = GetSymbol(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_mass *>::iterator it;
			for(it = massids.find(GetSymbol(argv[0])); it; ++it)
				it.data()->Id = id;
		}
		else	{
			t_mass *m = mass.find(GetAInt(argv[0]));
			if(m)
				m->Id = id;
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}
	
	// set mass of mass(s) named Id or number No
	void m_setM(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : Id/NoLink Value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float ma = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_mass *>::iterator it;
			//typename IDMap<t_link *>::iterator it;
			for(it = massids.find(GetSymbol(argv[0])); it; ++it) {
				it.data()->M = ma;
				it.data()->invM = ma?1/ma:0.; 
			}
		}
		else	{
			t_mass *m = mass.find(GetAInt(argv[0]));
			if(m) {
				m->M = ma;
				m->invM = ma?1/ma:0.;
			}
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}

	// grab nearest mass
	void m_grab_mass(int argc,t_atom *argv) 	{
	// grab nearest mass X Y
		t_mass **mi;
		t_float aux, distance;
		t_atom aux2[2];
 		bool mobil;

		// if click
		if (GetInt(argv[2])==1 && mass.size()>0)	{

			if (argc != 3)
				error("grabMass : X Y click");
			// first time we grab this mass?Find nearest mass
			if (mouse_grab == 0)	{
				t_mass *m = mass.find(0);
				aux = sqr(m->pos[0]-GetFloat(argv[0])) + sqr(m->pos[1]-GetFloat(argv[1]));
				nearest_mass = 0;
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {	
					distance = sqr(mit.data()->pos[0]-GetFloat(argv[0])) + sqr(mit.data()->pos[1]-GetFloat(argv[1]));
					if (distance<aux)	{
						aux = distance;
						nearest_mass = mit.data()->nbr;
					}
				}
			}
			
			// Set fixed if mobile
			mobil = mass.find(nearest_mass)->invM;
			SetInt(aux2[0],nearest_mass);
			if (mobil != 0)
				m_set_fixe(1,aux2);

			// Set XY
			SetFloat(aux2[1],GetFloat(argv[0]));
			m_posX(2,aux2);
			SetFloat(aux2[1],GetFloat(argv[1]));
			m_posY(2,aux2);

			// Set mobile
			if(mobil != 0)
				m_set_mobile(1,aux2);		
			
			// Current grabbing on
			mouse_grab = 1;
		}
		else
			// Grabing off
			mouse_grab = 0;
	}

// --------------------------------------------------------------  LINKS 
// ---------------------------------------------------------------------

	// add a link
	// Id, *mass1, *mass2, K1, D1, D2, (Lmin,Lmax)
	void m_link(int argc,t_atom *argv) 	{
		if (argc < 5 || argc > 8) {
			error("%s - %s Syntax : Id No/Idmass1 No/Idmass2 K D1 (pow Lmin Lmax)",thisName(),GetString(thisTag()));
			return;
		}
		if (IsSymbol(argv[1]) && IsSymbol(argv[2]))	{		// ID & ID
			typename IDMap<t_mass *>::iterator it1,it2,it;
			it1 = massids.find(GetSymbol(argv[1]));
			it2 = massids.find(GetSymbol(argv[2]));
			for(; it1; ++it1) {
				for(it = it2; it; ++it) {
					t_link *l = new t_link(
						id_link,
						GetSymbol(argv[0]), // ID
						it1.data(),it.data(), // pointer to mass1, mass2
						GetAFloat(argv[3]), // K1
						GetAFloat(argv[4]), // D1
						0,NULL,
						argc >= 6?GetFloat(argv[5]):1, // power
						argc >= 7?GetFloat(argv[6]):0,
						argc >= 8?GetFloat(argv[7]):1e10
					);
					linkids.insert(l);
					link.insert(id_link++,l);
					outlink(S_iLink,l);
				}
			}
		}
		else if (IsSymbol(argv[1])==0 && IsSymbol(argv[2]))	{	// No & ID
			typename IDMap<t_mass *>::iterator it2,it;
	 		t_mass *mass1 = mass.find(GetAInt(argv[1]));
			it2 = massids.find(GetSymbol(argv[2]));
			for(it = it2; it; ++it) {
				t_link *l = new t_link(
					id_link,
					GetSymbol(argv[0]), // ID
					mass1,it.data(), // pointer to mass1, mass2
					GetAFloat(argv[3]), // K1
					GetAFloat(argv[4]), // D1
					0,NULL,
					argc >= 6?GetFloat(argv[5]):1, // power
					argc >= 7?GetFloat(argv[6]):0,
					argc >= 8?GetFloat(argv[7]):1e10
				);
				linkids.insert(l);
				link.insert(id_link++,l);
				outlink(S_iLink,l);
			}
		}
		else if (IsSymbol(argv[1]) && IsSymbol(argv[2])==0)	{	// ID & No
			typename IDMap<t_mass *>::iterator it1,it;
			it1 = massids.find(GetSymbol(argv[1]));
	 		t_mass *mass2 = mass.find(GetAInt(argv[2]));
			for(it = it1; it; ++it) {
				t_link *l = new t_link(
					id_link,
					GetSymbol(argv[0]), // ID
					it.data(),mass2, // pointer to mass1, mass2
					GetAFloat(argv[3]), // K1
					GetAFloat(argv[4]), // D1
					0,NULL,
					argc >= 6?GetFloat(argv[5]):1, // power
					argc >= 7?GetFloat(argv[6]):0,
					argc >= 8?GetFloat(argv[7]):1e10
				);
				linkids.insert(l);
				link.insert(id_link++,l);
				outlink(S_iLink,l);
			}
		}
		else	{										// No & No
	 		t_mass *mass1 = mass.find(GetAInt(argv[1]));
			t_mass *mass2 = mass.find(GetAInt(argv[2]));
 	
   			if(!mass1 || !mass2) {
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				return;
			}
	
			t_link *l = new t_link(
				id_link,
				GetSymbol(argv[0]), // ID
				mass1,mass2, // pointer to mass1, mass2
				GetAFloat(argv[3]), // K1
				GetAFloat(argv[4]), // D1
				0,NULL,
				argc >= 6?GetFloat(argv[5]):1, // power
				argc >= 7?GetFloat(argv[6]):0,	// Lmin
				argc >= 8?GetFloat(argv[7]):1e10// Lmax
			);

			linkids.insert(l);
			link.insert(id_link++,l);
			outlink(S_Link,l);
		}
	}

	// add interactor link (for compatibility)
	// Id, Id masses1, Id masses2, K1, D1, D2, (Lmin, Lmax)
	void m_ilink(int argc,t_atom *argv) {m_link(argc,argv);	}

	// add a tangential link
	// Id, *mass1, *mass2, K1, D1, D2, (Lmin,Lmax)
	void m_tlink(int argc,t_atom *argv) {
		if (argc < 5+N || argc > 8+N) {
			error("%s - %s Syntax : Id Nomass1 Nomass2 K D1 xa%s%s (pow Lmin Lmax)",thisName(),GetString(thisTag()),N >= 2?" ya":"",N >= 3?" za":"");
			return;
		}
		t_float tangent[N];
		for(int i = 0; i < N; ++i) tangent[i] = GetAFloat(argv[5+i]);

		if (IsSymbol(argv[1]) && IsSymbol(argv[2]))	{		// ID & ID
			typename IDMap<t_mass *>::iterator it1,it2,it;
			it1 = massids.find(GetSymbol(argv[1]));
			it2 = massids.find(GetSymbol(argv[2]));
			for(; it1; ++it1) {
				for(it = it2; it; ++it) {
					t_link *l = new t_link(
						id_link,
						GetSymbol(argv[0]), // ID
						it1.data(),it.data(), // pointer to mass1, mass2
						GetAFloat(argv[3]), // K1
						GetAFloat(argv[4]), // D1
						1,					// tangential
						tangent,
						argc >= 6+N?GetFloat(argv[5+N]):1, // power
						argc >= 7+N?GetFloat(argv[6+N]):0, // Lmin
						argc >= 8+N?GetFloat(argv[7+N]):1e10 // Lmax
					);
					linkids.insert(l);
					link.insert(id_link++,l);
					outlink(S_iLink,l);
				}
			}
		}
		else if (IsSymbol(argv[1])==0 && IsSymbol(argv[2]))	{	// No & ID
			typename IDMap<t_mass *>::iterator it2,it;
	 		t_mass *mass1 = mass.find(GetAInt(argv[1]));
			it2 = massids.find(GetSymbol(argv[2]));
			for(it = it2; it; ++it) {
				t_link *l = new t_link(
					id_link,
					GetSymbol(argv[0]), // ID
					mass1,it.data(), // pointer to mass1, mass2
					GetAFloat(argv[3]), // K1
					GetAFloat(argv[4]), // D1
					1,					// tangential
					tangent,
					argc >= 6+N?GetFloat(argv[5+N]):1, // power
					argc >= 7+N?GetFloat(argv[6+N]):0, // Lmin
					argc >= 8+N?GetFloat(argv[7+N]):1e10 // Lmax
				);
				linkids.insert(l);
				link.insert(id_link++,l);
				outlink(S_tLink,l);
			}
		}
		else if (IsSymbol(argv[1]) && IsSymbol(argv[2])==0)	{	// ID & No
			typename IDMap<t_mass *>::iterator it1,it;
			it1 = massids.find(GetSymbol(argv[1]));
	 		t_mass *mass2 = mass.find(GetAInt(argv[2]));
			for(it = it1; it; ++it) {
				t_link *l = new t_link(
					id_link,
					GetSymbol(argv[0]), // ID
					it.data(),mass2, // pointer to mass1, mass2
					GetAFloat(argv[3]), // K1
					GetAFloat(argv[4]), // D1
					1,
					tangent,					// tangential
					argc >= 6+N?GetFloat(argv[5+N]):1, // power
					argc >= 7+N?GetFloat(argv[6+N]):0, // Lmin
					argc >= 8+N?GetFloat(argv[7+N]):1e10 // Lmax
				);
				linkids.insert(l);
				link.insert(id_link++,l);
				outlink(S_tLink,l);
			}
		}
		else	{										// No & No
			t_mass *mass1 = mass.find(GetAInt(argv[1]));
			t_mass *mass2 = mass.find(GetAInt(argv[2]));
 
   			if(!mass1 || !mass2) {
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				return;
			}
			t_link *l = new t_link(
				id_link,
				GetSymbol(argv[0]), // ID
				mass1,mass2, // pointer to mass1, mass2
				GetAFloat(argv[3]), // K1
				GetAFloat(argv[4]), // D1
				1,					// tangential
				tangent,					// tangential
				argc >= 6+N?GetFloat(argv[5+N]):1, // power
				argc >= 7+N?GetFloat(argv[6+N]):0, // Lmin
				argc >= 8+N?GetFloat(argv[7+N]):1e10 // Lmax
			);
			linkids.insert(l);
			link.insert(id_link++,l);
			outlink(S_tLink,l);
		}
	}

	// add a tab link
	// Id, *mass1, *mass2, k_tabname/K1, d_tabname/D1, l_tab
	void m_tablink(int argc,t_atom *argv) {
		if (argc != 7) {
			error("%s - %s Syntax : Id No/Idmass1 No/Idmass2 ktab lk dtab ld",thisName(),GetString(thisTag()));
			return;
		}
			// Searching exisiting buffers or create one.
			t_buffer *bk,*bd;

			if (IsSymbol(argv[3])) { // K tab
				typename IDMap<t_buffer *>::iterator itk;
				itk = buffersids.find(GetSymbol(argv[3]));
				if(!itk) {
					bk = new t_buffer(id_buffer,GetSymbol(argv[3]));
					buffersids.insert(bk);
					buffers.insert(id_buffer++,bk);
				}
				else
					bk = itk.data();
			}
			if (IsSymbol(argv[5])) { // D tab
				typename IDMap<t_buffer *>::iterator itd;
				itd = buffersids.find(GetSymbol(argv[5]));
				if(!itd) {
					bd = new t_buffer(id_buffer,GetSymbol(argv[5]));
					buffersids.insert(bd);
					buffers.insert(id_buffer++,bd);
				}
				else
					bd = itd.data();
			}
			
		if (IsSymbol(argv[1]) && IsSymbol(argv[2]))	{		// ID & ID
			typename IDMap<t_mass *>::iterator it1,it2,it;
			it1 = massids.find(GetSymbol(argv[1]));
			it2 = massids.find(GetSymbol(argv[2]));
			for(; it1; ++it1) {
				for(it = it2; it; ++it) {						
					t_link *l = new t_link(
						id_link,
						GetSymbol(argv[0]), // ID
						it1.data(),it.data(), // pointer to mass1, mass2
						IsSymbol(argv[3])?1:GetAFloat(argv[3]), // K1 = 1 if buffer
						IsSymbol(argv[5])?1:GetAFloat(argv[5]), // D1
						3,NULL,
						1, // power
						0, // Lmin
						1e10, // Lmax
						IsSymbol(argv[3])?bk:NULL,	// k_tabname 	
						GetAFloat(argv[4]), // l_tab
						IsSymbol(argv[5])?bd:NULL, // d_tabname
						GetAFloat(argv[6]) // l_tab2
					);
					linkids.insert(l);
					link.insert(id_link++,l);
					outlink(S_tabLink,l);
				}
			}
		}
		else if (IsSymbol(argv[1])==0 && IsSymbol(argv[2]))	{	// No & ID
			typename IDMap<t_mass *>::iterator it2,it;
	 		t_mass *mass1 = mass.find(GetAInt(argv[1]));
			it2 = massids.find(GetSymbol(argv[2]));
			for(it = it2; it; ++it) {
				t_link *l = new t_link(
						id_link,
						GetSymbol(argv[0]), // ID
						mass1,it.data(), // pointer to mass1, mass2
						IsSymbol(argv[3])?1:GetAFloat(argv[3]), // K1 = 1 if buffer
						IsSymbol(argv[5])?1:GetAFloat(argv[5]), // D1
						3,NULL,
						1, // power
						0, // Lmin
						1e10, // Lmax
						IsSymbol(argv[3])?bk:NULL,	// k_tabname 	
						GetAFloat(argv[4]), // l_tab
						IsSymbol(argv[5])?bd:NULL, // d_tabname
						GetAFloat(argv[6]) // l_tab2
				);
				linkids.insert(l);
				link.insert(id_link++,l);
				outlink(S_tabLink,l);
			}
		}
		else if (IsSymbol(argv[1]) && IsSymbol(argv[2])==0)	{	// ID & No
			typename IDMap<t_mass *>::iterator it1,it;
			it1 = massids.find(GetSymbol(argv[1]));
	 		t_mass *mass2 = mass.find(GetAInt(argv[2]));
			for(it = it1; it; ++it) {
				t_link *l = new t_link(
						id_link,
						GetSymbol(argv[0]), // ID
						it1.data(),mass2, // pointer to mass1, mass2
						IsSymbol(argv[3])?1:GetAFloat(argv[3]), // K1 = 1 if buffer
						IsSymbol(argv[5])?1:GetAFloat(argv[5]), // D1
						3,NULL,
						1, // power
						0, // Lmin
						1e10, // Lmax
						IsSymbol(argv[3])?bk:NULL,	// k_tabname 	
						GetAFloat(argv[4]), // l_tab
						IsSymbol(argv[5])?bd:NULL, // d_tabname
						GetAFloat(argv[6]) // l_tab2
				);
				linkids.insert(l);
				link.insert(id_link++,l);
				outlink(S_tabLink,l);
			}
		}
		else	{										// No & No
	 		t_mass *mass1 = mass.find(GetAInt(argv[1]));
			t_mass *mass2 = mass.find(GetAInt(argv[2]));
 	
   			if(!mass1 || !mass2) {
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				return;
			}
					
			t_link *l = new t_link(
						id_link,
						GetSymbol(argv[0]), // ID
						mass1,mass2, // pointer to mass1, mass2
						IsSymbol(argv[3])?1:GetAFloat(argv[3]), // K1 = 1 if buffer
						IsSymbol(argv[5])?1:GetAFloat(argv[5]), // D1
						3,NULL,
						1, // power
						0, // Lmin
						1e10, // Lmax
						IsSymbol(argv[3])?bk:NULL,	// k_tabname 	
						GetAFloat(argv[4]), // l_tab
						IsSymbol(argv[5])?bd:NULL, // d_tabname
						GetAFloat(argv[6]) // l_tab2
			);

			linkids.insert(l);
			link.insert(id_link++,l);
			outlink(S_tabLink,l);
		}
	}

	// add a normal link
	// Id, *mass1, *mass2, K1, D1, D2, (Lmin,Lmax)
	void m_nlink(int argc,t_atom *argv) {
	// deprecated
	}

	// set Id of link(s) named Id or number No
	void m_setLinkId(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : OldId/NoLink NewId",thisName(),GetString(thisTag()));
			return;
		}

		const t_symbol *id = GetSymbol(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_link *>::iterator it;
			for(it = linkids.find(GetSymbol(argv[0])); it; ++it)
				it.data()->Id = id;
		}
		else	{
			t_link *l = link.find(GetAInt(argv[0]));
			if(l)
				l->Id = id;
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}

	// set rigidity of link(s) named Id or number No
	void m_setK(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : Id/NoLink Value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float k1 = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_link *>::iterator it;
			for(it = linkids.find(GetSymbol(argv[0])); it; ++it)
				it.data()->K1 = k1;
		}
		else	{
			t_link *l = link.find(GetAInt(argv[0]));
			if(l)
				l->K1 = k1;
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}

	// set damping of link(s) named Id or number No
	void m_setD(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : Id/NoLink Value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float d1 = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_link *>::iterator it;
			for(it = linkids.find(GetSymbol(argv[0])); it; ++it)
				it.data()->D1 = d1;
		}
		else	{
			t_link *l = link.find(GetAInt(argv[0]));
			if(l)
				l->D1 = d1;
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}
	
	// set max lenght of link(s) named Id or number No
	void m_setLmax(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : Id/NoLink Value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float lon = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_link *>::iterator it;
			for(it = linkids.find(GetSymbol(argv[0])); it; ++it) {
				it.data()->long_max = lon;
			}
		}
		else	{
			t_link *l = link.find(GetAInt(argv[0]));
			if(l) {
				l->long_max = lon;
			}
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}

	// set min lenght of link(s) named Id or number No
	void m_setLmin(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : Id/NoLink Value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float lon = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_link *>::iterator it;
			for(it = linkids.find(GetSymbol(argv[0])); it; ++it) {
				it.data()->long_min = lon;
			}
		}
		else	{
			t_link *l = link.find(GetAInt(argv[0]));
			if(l) {
				l->long_min = lon;
			}
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}
	
	// set initial lenght of link(s) named Id or number No
	void m_setL(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : Id/NoLink Value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float lon = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_link *>::iterator it;
			for(it = linkids.find(GetSymbol(argv[0])); it; ++it) {
				it.data()->longueur = lon;
				it.data()->distance_old = lon;
			}
		}
		else	{
			t_link *l = link.find(GetAInt(argv[0]));
			if(l) {
				l->longueur = lon;
				l->distance_old = lon;
			}
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}
	
	// set l_tab of tablink(s) named Id or number No
	void m_setLtab(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : Id/NoLink Value",thisName(),GetString(thisTag()));
			return;
		}

		const t_float lon = GetAFloat(argv[1]);

		if(IsSymbol(argv[0])) {
			typename IDMap<t_link *>::iterator it;
			for(it = linkids.find(GetSymbol(argv[0])); it; ++it) {
				if(it.data()->link_type==3)
					it.data()->l_tab = lon;
				else
					error("%s - %s : set Ltab is working only on tablink",thisName(),GetString(thisTag()));
			}
		}
		else	{
			t_link *l = link.find(GetAInt(argv[0]));
			if(l) {
				if(l->link_type==3)
					l->l_tab = lon;
				else
					error("%s - %s : set Ltab is working only on tablink",thisName(),GetString(thisTag()));
			
			}
			else
				error("%s - %s : Index not found",thisName(),GetString(thisTag()));
		}
	}
	
	// set damping of link(s) named Id
	void m_setD2(int argc,t_atom *argv) {
		if (argc != 2) {
			error("%s - %s Syntax : IdLink Value",thisName(),GetString(thisTag()));
			return;
		}

		t_float d2 = GetAFloat(argv[1]);
		typename IDMap<t_link *>::iterator it;
		for(it = linkids.find(GetSymbol(argv[0])); it; ++it)
			it.data()->D2 = d2;
	}

	// Delete link
	void m_delete_link(int argc,t_atom *argv) {
		if (argc != 1) {
			error("%s - %s Syntax : NtLink",thisName(),GetString(thisTag()));
			return;
		}
		
		t_link *l = link.find(GetAInt(argv[0]));
		if(l)	{
			deletelink(l);
			link_deleted = 1;
		}
		else {
			error("%s - %s : Index not found",thisName(),GetString(thisTag()));
			return;
		}
	}


// --------------------------------------------------------------  GET 
// -------------------------------------------------------------------

	// get attributes
	void m_get(int argc,t_atom *argv) {
		if(argc == 0) {
			return;
		}
	
		t_atom sortie[1+2*N];
		t_float mean[N] ,std[N], nombre;
		const t_symbol *auxtype = GetSymbol(argv[0]);


		if (argc == 1) {
			if (auxtype == S_massesPos)	{	// get all masses positions
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {	
					SetInt(sortie[0],mit.data()->nbr);
					for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->pos[i]);
					ToOutAnything(0,S_massesPos,1+N,sortie);
				}
			}
			else if (auxtype == S_massesPosName)	{	// get all masses positions output Id
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {	
					SetSymbol(sortie[0],mit.data()->Id);
					for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->pos[i]);
					ToOutAnything(0,S_massesPosName,1+N,sortie);
				}
			}
			else if (auxtype == S_massesPosMean)	{	// get all masses positions mean
				for(int i = 0; i<N; ++i) 
					mean[i] = 0;
				nombre = 0;
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {
					++nombre;
					for(int i = 0; i < N; ++i)	
						mean[i] += mit.data()->pos[i];
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0+i],mean[i]/nombre);
				ToOutAnything(0,S_massesPosMean,0+N,sortie);
			}
			else if (auxtype == S_massesPosStd)	{	// get all masses positions std
				for(int i = 0; i<N; ++i) {
					mean[i] = 0;
					std[i] = 0;
				}
				nombre = 0;
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {
					++nombre;
					for(int i = 0; i < N; ++i) {	
						mean[i] += mit.data()->pos[i];
						std[i] += sqr(mit.data()->pos[i]) ;
					} 
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0+i],sqrt(std[i]/nombre-sqr(mean[i]/nombre)));
				ToOutAnything(0,S_massesPosStd,0+N,sortie);
			}
			else if (auxtype == S_massesForces)	{ // get all masses forces
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {	
					SetInt(sortie[0],mit.data()->nbr);	
					for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->out_force[i]);
					ToOutAnything(0,S_massesForces,1+N,sortie);
				}
			}
			else if (auxtype == S_massesForcesName)	{ // get all masses forces
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {	
					SetSymbol(sortie[0],mit.data()->Id);	
					for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->out_force[i]);
					ToOutAnything(0,S_massesForcesName,1+N,sortie);
				}
			}
			else if (auxtype == S_massesForcesMean)	{	// get all masses forces mean
				for(int i = 0; i<N; ++i) 
					mean[i] = 0;
				nombre = 0;
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {
					++nombre;
					for(int i = 0; i < N; ++i)	
						mean[i] += mit.data()->out_force[i];
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0+i],mean[i]/nombre);
				ToOutAnything(0,S_massesForcesMean,0+N,sortie);
			}
			else if (auxtype == S_massesForcesStd)	{	// get all masses forces std
				for(int i = 0; i<N; ++i) {
					mean[i] = 0;
					std[i] = 0;
				}
				nombre = 0;
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {
					++nombre;
					for(int i = 0; i < N; ++i) {	
						mean[i] += mit.data()->out_force[i];
						std[i] += sqr(mit.data()->out_force[i]) ;
					} 
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0+i],sqrt(std[i]/nombre-sqr(mean[i]/nombre)));
				ToOutAnything(0,S_massesForcesStd,0+N,sortie);
			}
			else if (auxtype == S_linksPos) {		// get all links positions
				for(typename IndexMap<t_link *>::iterator lit(link); lit; ++lit) {	
					SetInt(sortie[0],lit.data()->nbr);	
					for(int i = 0; i < N; ++i) {
						SetFloat(sortie[1+i],lit.data()->mass1->pos[i]);
						SetFloat(sortie[1+N+i],lit.data()->mass2->pos[i]);
					}
					ToOutAnything(0,S_linksPos,1+2*N,sortie);
				}
			}
			else if (auxtype == S_linksPosName) {		// get all links positions
				for(typename IndexMap<t_link *>::iterator lit(link); lit; ++lit) {	
					SetSymbol(sortie[0],lit.data()->Id);	
					for(int i = 0; i < N; ++i) {
						SetFloat(sortie[1+i],lit.data()->mass1->pos[i]);
						SetFloat(sortie[1+N+i],lit.data()->mass2->pos[i]);
					}
					ToOutAnything(0,S_linksPosName,1+2*N,sortie);
				}
			}
			else if (auxtype == S_linksLenghts) {		// get all links lenghts
				for(typename IndexMap<t_link *>::iterator lit(link); lit; ++lit) {	
					SetInt(sortie[0],lit.data()->nbr);	
					SetFloat(sortie[1],lit.data()->distance_old);
					ToOutAnything(0,S_linksLenghts,2,sortie);
				}
			}
			else if (auxtype == S_linksLenghtsMean)	{	// get all links lenghts mean
				for(int i = 0; i<N; ++i) 
					mean[i] = 0;
				nombre = 0;
				for(typename IndexMap<t_link *>::iterator lit(link); lit; ++lit) {
					++nombre;
					mean[0] += lit.data()->distance_old;
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0],mean[0]/nombre);
				ToOutAnything(0,S_linksLenghtsMean,1,sortie);
			}
			else if (auxtype == S_linksLenghtsStd)	{	// get all links lenghts std
				for(int i = 0; i<N; ++i) {
					mean[i] = 0;
					std[i] = 0;
				}
				nombre = 0;
				for(typename IndexMap<t_link *>::iterator lit(link); lit; ++lit) {
					++nombre;
					mean[0] += lit.data()->distance_old;
					std[0] += sqr(lit.data()->distance_old) ;
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0],sqrt(std[0]/nombre-sqr(mean[0]/nombre)));
				ToOutAnything(0,S_linksLenghtsStd,1,sortie);
			}
			else if (auxtype == S_massesSpeeds) {		// get all masses speeds
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {	
					SetInt(sortie[0],mit.data()->nbr);	
					for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->speed[i]);
					ToOutAnything(0,S_massesSpeeds,1+N,sortie);
				}
			}
			else if (auxtype == S_massesSpeedsName) {		// get all masses speeds
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {	
					SetSymbol(sortie[0],mit.data()->Id);	
					for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->speed[i]);
					ToOutAnything(0,S_massesSpeedsName,1+N,sortie);
				}
			}
			else if (auxtype == S_massesSpeedsMean)	{	// get all masses forces mean
				for(int i = 0; i<N; ++i) 
					mean[i] = 0;
				nombre = 0;
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {
					++nombre;
					for(int i = 0; i < N; ++i)	
						mean[i] += mit.data()->speed[i];
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0+i],mean[i]/nombre);
				ToOutAnything(0,S_massesSpeedsMean,0+N,sortie);
			}
			else if (auxtype == S_massesSpeedsStd)	{	// get all masses forces std
				for(int i = 0; i<N; ++i) {
					mean[i] = 0;
					std[i] = 0;
				}
				nombre = 0;
				for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit) {
					++nombre;
					for(int i = 0; i < N; ++i) {	
						mean[i] += mit.data()->speed[i];
						std[i] += sqr(mit.data()->speed[i]) ;
					} 
				}
				for(int i = 0; i < N; ++i)	
					SetFloat(sortie[0+i],sqrt(std[i]/nombre-sqr(mean[i]/nombre)));
				ToOutAnything(0,S_massesSpeedsStd,0+N,sortie);
			}
			else 
				error("%s - %s : Syntax error",thisName(),GetString(thisTag()));
			return;
		}
		
		// more than 1 args
		if (auxtype == S_massesPos) 	// get mass positions
		{
			for(int j = 1; j<argc; j++) {
				if(IsSymbol(argv[j])) {
					typename IDMap<t_mass *>::iterator mit;
					for(mit = massids.find(GetSymbol(argv[j])); mit; ++mit) {
						SetSymbol(sortie[0],mit.data()->Id);
						for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->pos[i]);
						ToOutAnything(0,S_massesPosId,1+N,sortie);
					}
				}
				else {
					t_mass *m = mass.find(GetAInt(argv[j]));
					if(m) {
						SetInt(sortie[0],m->nbr);
						for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],m->pos[i]);
						ToOutAnything(0,S_massesPosNo,1+N,sortie);
					}
//					else
//						error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				}
			}
		}
		else if (auxtype == S_massesForces)	// get mass forces
		{
			for(int j = 1; j<argc; j++) {
				if(IsSymbol(argv[j])) {
					typename IDMap<t_mass *>::iterator mit;
					for(mit = massids.find(GetSymbol(argv[j])); mit; ++mit) {
						SetSymbol(sortie[0],mit.data()->Id);
						for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->out_force[i]);
						ToOutAnything(0,S_massesForcesId,1+N,sortie);
					}
				}
				else {
					t_mass *m = mass.find(GetAInt(argv[j]));
					if(m) {
						SetInt(sortie[0],m->nbr);
						for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],m->out_force[i]);
						ToOutAnything(0,S_massesForcesNo,1+N,sortie);
					}
//					else
//						error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				}
			}
		}
		else if (auxtype == S_linksPos)		// get links positions
		{
			for(int j = 1; j<argc; j++) {
				if(IsSymbol(argv[j])) {
					typename IDMap<t_link *>::iterator lit;
					for(lit = linkids.find(GetSymbol(argv[j])); lit; ++lit) {
						SetSymbol(sortie[0],lit.data()->Id);
						for(int i = 0; i < N; ++i) {
							SetFloat(sortie[1+i],lit.data()->mass1->pos[i]);
							SetFloat(sortie[1+N+i],lit.data()->mass2->pos[i]);
						}
						ToOutAnything(0,S_linksPosId,1+2*N,sortie);
					}
				}
				else {
					t_link *l = link.find(GetAInt(argv[j]));
					if(l) {
						SetInt(sortie[0],l->nbr);
						for(int i = 0; i < N; ++i) {
							SetFloat(sortie[1+i],l->mass1->pos[i]);
							SetFloat(sortie[1+N+i],l->mass2->pos[i]);
						}
						ToOutAnything(0,S_linksPosNo,1+2*N,sortie);
					}
//					else
//						error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				}
			}
		}
		else if (auxtype == S_linksLenghts)		// get links lenghts
		{
			for(int j = 1; j<argc; j++) {
				if(IsSymbol(argv[j])) {
					typename IDMap<t_link *>::iterator lit;
					for(lit = linkids.find(GetSymbol(argv[j])); lit; ++lit) {
						SetSymbol(sortie[0],lit.data()->Id);
						SetFloat(sortie[1],lit.data()->distance_old);
						ToOutAnything(0,S_linksLenghtsId,2,sortie);
					}
				}
				else {
					t_link *l = link.find(GetAInt(argv[j]));
					if(l) {
						SetInt(sortie[0],l->nbr);
						SetFloat(sortie[1],l->distance_old);
						ToOutAnything(0,S_linksLenghtsNo,2,sortie);
					}
//					else
//						error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				}
			}
		}
		else 			 		// get mass speeds
		{
			for(int j = 1; j<argc; j++) {
				if(IsSymbol(argv[j])) {
					typename IDMap<t_mass *>::iterator mit;
					for(mit = massids.find(GetSymbol(argv[j])); mit; ++mit) {
						SetSymbol(sortie[0],mit.data()->Id);
						for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],mit.data()->speed[i]);
						ToOutAnything(0,S_massesSpeedsId,1+N,sortie);
					}
				}
				else {
					t_mass *m = mass.find(GetAInt(argv[j]));
					if(m) {
						SetInt(sortie[0],m->nbr);
						for(int i = 0; i < N; ++i) SetFloat(sortie[1+i],m->speed[i]);
						ToOutAnything(0,S_massesSpeedsNo,1+N,sortie);
					}
//					else
//						error("%s - %s : Index not found",thisName(),GetString(thisTag()));
				}
			}
		}
	}

	// List of masses positions on first outlet
	void m_mass_dumpl() {
		if (mass_deleted ==0)	{
			int sz = mass.size();
			NEWARR(t_atom,sortie,sz*N);
			t_atom *s = sortie;
			for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit)
				for(int i = 0; i < N; ++i) SetFloat(s[mit.data()->nbr*N+i],mit.data()->pos[i]);
			ToOutAnything(0, S_massesPosL, sz*N, sortie);
			DELARR(sortie);
		}
		else
			error("%s - %s : Message Forbidden when deletion is used",thisName(),GetString(thisTag()));
	}

	// List of masses x positions on first outlet
	void m_mass_dump_xl() {	
		if (mass_deleted ==0)	{
			int sz = mass.size();
			NEWARR(t_atom,sortie,sz);
			t_atom *s = sortie;
			for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit)
				SetFloat(s[mit.data()->nbr],mit.data()->pos[0]);
			ToOutAnything(0, S_massesPosXL, sz, sortie);
			DELARR(sortie);
		}
		else
			error("%s - %s : Message Forbidden when deletion is used",thisName(),GetString(thisTag()));
	}

	// List of masses y positions on first outlet
	void m_mass_dump_yl() {	
		if (mass_deleted ==0)	{
			int sz = mass.size();
			NEWARR(t_atom,sortie,sz);
			t_atom *s = sortie;
			for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit)
				SetFloat(s[mit.data()->nbr],mit.data()->pos[1]);
			ToOutAnything(0, S_massesPosYL, sz, sortie);
			DELARR(sortie);
		}
		else
			error("%s - %s : Message Forbidden when deletion is used",thisName(),GetString(thisTag()));
	}

	// List of masses z positions on first outlet
	void m_mass_dump_zl() {	
		if (mass_deleted ==0)	{
			int sz = mass.size();
			NEWARR(t_atom,sortie,sz);
			t_atom *s = sortie;
			for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit)
				SetFloat(s[mit.data()->nbr],mit.data()->pos[2]);
			ToOutAnything(0, S_massesPosZL, sz, sortie);
			DELARR(sortie);
		}
		else
			error("%s - %s : Message Forbidden when deletion is used",thisName(),GetString(thisTag()));
	}

	// List of masses forces on first outlet
	void m_force_dumpl() {	
		if (mass_deleted ==0)	{
			int sz = mass.size();
			NEWARR(t_atom,sortie,sz*N);
			t_atom *s = sortie;
			for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit)
				for(int i = 0; i < N; ++i) SetFloat(s[mit.data()->nbr*N+i],mit.data()->out_force[i]);
			ToOutAnything(0, S_massesForcesL, sz*N, sortie);
			DELARR(sortie);
		}
		else
			error("%s - %s : Message Forbidden when deletion is used",thisName(),GetString(thisTag()));
	}

	// List of masses and links infos on second outlet
	void m_info_dumpl() {	
		for(typename IndexMap<t_mass *>::iterator mit(mass); mit; ++mit)
			outmass(S_Mass,mit.data());

		for(typename IndexMap<t_link *>::iterator lit(link); lit; ++lit)
			outlink(S_Link,lit.data());
	}


// --------------------------------------------------------------  SETUP
// ---------------------------------------------------------------------

private:

	void clear() {
		buffersids.reset();
		buffers.reset();
		
		linkids.reset();
		link.reset();

		massids.reset();
		mass.reset();
		// Reset state variables 		
		id_mass = id_link = id_buffer = mouse_grab = mass_deleted = link_deleted = 0;
	}
	
	void deletelink(t_link *l) {
		outlink(S_Link_deleted,l);
		linkids.erase(l);
		link.remove(l->nbr);
		delete l;
	}
	
	void outmass(const t_symbol *s,const t_mass *m) {
		t_atom sortie[4+N];
		SetInt((sortie[0]),m->nbr);
		SetSymbol((sortie[1]),m->Id);
		SetBool((sortie[2]),m->getMobile());
		SetFloat((sortie[3]),m->M);
		for(int i = 0; i < N; ++i) SetFloat((sortie[4+i]),m->pos[i]);
		ToOutAnything(1,s,4+N,sortie);
	}
	
	void outlink(const t_symbol *s,const t_link *l) {
		t_atom sortie[15];
		int size=6;
		SetInt((sortie[0]),l->nbr);
		SetSymbol((sortie[1]),l->Id);
		SetInt((sortie[2]),l->mass1->nbr);
		SetInt((sortie[3]),l->mass2->nbr);
		l->k_buffer?SetSymbol(sortie[4],l->k_buffer->Id):SetFloat((sortie[4]),l->K1);
		l->d_buffer?SetSymbol(sortie[5],l->d_buffer->Id):SetFloat((sortie[5]),l->D1);
		if (l->link_type == 1 ||(l->link_type == 2 && N ==2))	{
			for (int i=0; i<N; i++)
				SetFloat((sortie[6+i]),l->tdirection1[i]);
//			ToOutAnything(1,s,6+N,sortie);
			size = 6+N;
		}
		else if (l->link_type == 2 && N==3)	{
			for (int i=0; i<N; i++)	{
				SetFloat((sortie[6+i]),l->tdirection1[i]);
				SetFloat((sortie[6+i+N]),l->tdirection2[i]);	
			}			
//			ToOutAnything(1,s,6+2*N,sortie);
			size = 6+2*N;
		}

		if(l->long_max != 1e10)	{
			SetFloat((sortie[size]),l->puissance);
			size++;
			SetFloat((sortie[size]),l->long_min);
			size++;
			SetFloat((sortie[size]),l->long_max);
			size++;
		}
		else if(l->long_min != 0) {
			SetFloat((sortie[size]),l->puissance);
			size++;
			SetFloat((sortie[size]),l->long_min);
			size++;
		}
		else if(l->puissance != 1)	{
			SetFloat((sortie[size]),l->puissance);
			size++;
		}
		ToOutAnything(1,s,size,sortie);
	}
	

	// Static symbols
	const static t_symbol *S_Reset;
	const static t_symbol *S_Mass;
	const static t_symbol *S_Link;
	const static t_symbol *S_iLink;
	const static t_symbol *S_tLink;
	const static t_symbol *S_nLink;
	const static t_symbol *S_tabLink;
	const static t_symbol *S_Mass_deleted;
	const static t_symbol *S_Link_deleted;
	const static t_symbol *S_massesPos;
	const static t_symbol *S_massesPosName;
	const static t_symbol *S_massesPosMean;
	const static t_symbol *S_massesPosStd;
	const static t_symbol *S_massesPosNo;
	const static t_symbol *S_massesPosId;
	const static t_symbol *S_linksPos;
	const static t_symbol *S_linksPosName;
	const static t_symbol *S_linksPosNo;
	const static t_symbol *S_linksPosId;
	const static t_symbol *S_linksLenghts;
	const static t_symbol *S_linksLenghtsMean;
	const static t_symbol *S_linksLenghtsStd;
	const static t_symbol *S_linksLenghtsNo;
	const static t_symbol *S_linksLenghtsId;
	const static t_symbol *S_massesForces;
	const static t_symbol *S_massesForcesName;
	const static t_symbol *S_massesForcesMean;
	const static t_symbol *S_massesForcesStd;
	const static t_symbol *S_massesForcesNo;
	const static t_symbol *S_massesForcesId;
	const static t_symbol *S_massesSpeeds;
	const static t_symbol *S_massesSpeedsName;
	const static t_symbol *S_massesSpeedsMean;
	const static t_symbol *S_massesSpeedsStd;
	const static t_symbol *S_massesSpeedsNo;
	const static t_symbol *S_massesSpeedsId;
	const static t_symbol *S_massesPosL;
	const static t_symbol *S_massesPosXL;
	const static t_symbol *S_massesPosYL;
	const static t_symbol *S_massesPosZL;
	const static t_symbol *S_massesForcesL;

	static void setup(t_classid c) {
		S_Reset = MakeSymbol("Reset");
		S_Mass = MakeSymbol("Mass");
		S_Link = MakeSymbol("Link");
		S_iLink = MakeSymbol("iLink");
		S_tLink = MakeSymbol("tLink");
		S_nLink = MakeSymbol("nLink");
		S_tabLink = MakeSymbol("tabLink");
		S_Mass_deleted = MakeSymbol("Mass deleted");
		S_Link_deleted = MakeSymbol("Link deleted");
		S_massesPos = MakeSymbol("massesPos");
		S_massesPosName = MakeSymbol("massesPosName");
		S_massesPosMean = MakeSymbol("massesPosMean");
		S_massesPosStd = MakeSymbol("massesPosStd");
		S_massesPosNo = MakeSymbol("massesPosNo");
		S_massesPosId = MakeSymbol("massesPosId");
		S_linksPos = MakeSymbol("linksPos");
		S_linksPosName = MakeSymbol("linksPosName");
		S_linksPosNo = MakeSymbol("linksPosNo");
		S_linksPosId = MakeSymbol("linksPosId");
		S_linksLenghts = MakeSymbol("linksLenghts");
		S_linksLenghtsMean = MakeSymbol("linksLenghtsMean");
		S_linksLenghtsStd = MakeSymbol("linksLenghtsStd");
		S_linksLenghtsNo = MakeSymbol("linksLenghtsNo");
		S_linksLenghtsId = MakeSymbol("linksLenghtsId");
		S_massesForces = MakeSymbol("massesForces");
		S_massesForcesName = MakeSymbol("massesForcesName");
		S_massesForcesMean = MakeSymbol("massesForcesMean");
		S_massesForcesStd = MakeSymbol("massesForcesStd");
		S_massesForcesNo = MakeSymbol("massesForcesNo");
		S_massesForcesId = MakeSymbol("massesForcesId");
		S_massesSpeeds = MakeSymbol("massesSpeeds");
		S_massesSpeedsName = MakeSymbol("massesSpeedsName");
		S_massesSpeedsMean = MakeSymbol("massesSpeedsMean");
		S_massesSpeedsStd = MakeSymbol("massesSpeedsStd");
		S_massesSpeedsNo = MakeSymbol("massesSpeedsNo");
		S_massesSpeedsId = MakeSymbol("massesSpeedsId");
		S_massesPosL = MakeSymbol("massesPosL");
		S_massesPosXL = MakeSymbol("massesPosXL");
		S_massesPosYL = MakeSymbol("massesPosYL");
		S_massesPosZL = MakeSymbol("massesPosZL");
		S_massesForcesL = MakeSymbol("massesForcesL");

		// --- set up methods (class scope) ---

		// register a bang method to the default inlet (0)
		FLEXT_CADDBANG(c,0,m_bang);

		// set up tagged methods for the default inlet (0)
		// the underscore _ after CADDMETHOD indicates that a message tag is used
		// no, variable list or anything and all single arguments are recognized automatically, ...
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);

		FLEXT_CADDMETHOD_(c,0,"forceX",m_forceX);
		FLEXT_CADDMETHOD_(c,0,"posX",m_posX);
		FLEXT_CADDMETHOD_(c,0,"Xmax",m_Xmax);
		FLEXT_CADDMETHOD_(c,0,"Xmin",m_Xmin);
		FLEXT_CADDMETHOD_(c,0,"forceN",m_forceN);
		FLEXT_CADDMETHOD_(c,0,"posN",m_posN);
		FLEXT_CADDMETHOD_(c,0,"Nmax",m_Nmax);
		FLEXT_CADDMETHOD_(c,0,"Nmin",m_Nmin);
		FLEXT_CADDMETHOD_(c,0,"massesPosL",m_mass_dumpl);
		FLEXT_CADDMETHOD_(c,0,"massesPosXL",m_mass_dump_xl);
		if(N >= 2) {
			FLEXT_CADDMETHOD_(c,0,"forceY",m_forceY);
			FLEXT_CADDMETHOD_(c,0,"posY",m_posY);
			FLEXT_CADDMETHOD_(c,0,"Ymax",m_Ymax);
			FLEXT_CADDMETHOD_(c,0,"Ymin",m_Ymin);
			FLEXT_CADDMETHOD_(c,0,"massesPosYL",m_mass_dump_yl);
			FLEXT_CADDMETHOD_(c,0,"grabMass",m_grab_mass);
		}
		if(N >= 3) {
			FLEXT_CADDMETHOD_(c,0,"forceZ",m_forceZ);
			FLEXT_CADDMETHOD_(c,0,"posZ",m_posZ);
			FLEXT_CADDMETHOD_(c,0,"Zmax",m_Zmax);
			FLEXT_CADDMETHOD_(c,0,"Zmin",m_Zmin);
			FLEXT_CADDMETHOD_(c,0,"massesPosZL",m_mass_dump_zl);
		}
		
		FLEXT_CADDMETHOD_(c,0,"setMobile",m_set_mobile);
		FLEXT_CADDMETHOD_(c,0,"setFixed",m_set_fixe);
		FLEXT_CADDMETHOD_(c,0,"setMassId",m_setMassId);
		FLEXT_CADDMETHOD_(c,0,"setLinkId",m_setLinkId);
		FLEXT_CADDMETHOD_(c,0,"setK",m_setK);
		FLEXT_CADDMETHOD_(c,0,"setD",m_setD);
		FLEXT_CADDMETHOD_(c,0,"setL",m_setL);
		FLEXT_CADDMETHOD_(c,0,"setLtab",m_setLtab);
		FLEXT_CADDMETHOD_(c,0,"setLMin",m_setLmin);
		FLEXT_CADDMETHOD_(c,0,"setLMax",m_setLmax);
		FLEXT_CADDMETHOD_(c,0,"setM",m_setM);
		FLEXT_CADDMETHOD_(c,0,"setD2",m_setD2);
		FLEXT_CADDMETHOD_(c,0,"mass",m_mass);
		FLEXT_CADDMETHOD_(c,0,"link",m_link);
		FLEXT_CADDMETHOD_(c,0,"iLink",m_ilink);
		FLEXT_CADDMETHOD_(c,0,"tLink",m_tlink);
		FLEXT_CADDMETHOD_(c,0,"nLink",m_nlink);
		FLEXT_CADDMETHOD_(c,0,"tabLink",m_tablink);
		FLEXT_CADDMETHOD_(c,0,"get",m_get);
		FLEXT_CADDMETHOD_(c,0,"deleteLink",m_delete_link);
		FLEXT_CADDMETHOD_(c,0,"deleteMass",m_delete_mass);
		FLEXT_CADDMETHOD_(c,0,"infosL",m_info_dumpl);
		FLEXT_CADDMETHOD_(c,0,"massesForcesL",m_force_dumpl);
	}

	// for every registered method a callback has to be declared
	FLEXT_CALLBACK(m_bang)
	FLEXT_CALLBACK(m_mass_dumpl)
	FLEXT_CALLBACK(m_mass_dump_xl)
	FLEXT_CALLBACK(m_mass_dump_yl)
	FLEXT_CALLBACK(m_mass_dump_zl)
	FLEXT_CALLBACK(m_info_dumpl)
	FLEXT_CALLBACK(m_force_dumpl)
	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK_V(m_set_mobile)
	FLEXT_CALLBACK_V(m_set_fixe)
	FLEXT_CALLBACK_V(m_mass)
	FLEXT_CALLBACK_V(m_link)
	FLEXT_CALLBACK_V(m_ilink)
	FLEXT_CALLBACK_V(m_tlink)
	FLEXT_CALLBACK_V(m_nlink)
	FLEXT_CALLBACK_V(m_tablink)
	FLEXT_CALLBACK_V(m_Xmax)
	FLEXT_CALLBACK_V(m_Xmin)
	FLEXT_CALLBACK_V(m_forceX)
	FLEXT_CALLBACK_V(m_posX)
	FLEXT_CALLBACK_V(m_Ymax)
	FLEXT_CALLBACK_V(m_Ymin)
	FLEXT_CALLBACK_V(m_forceY)
	FLEXT_CALLBACK_V(m_posY)
	FLEXT_CALLBACK_V(m_Zmax)
	FLEXT_CALLBACK_V(m_Zmin)
	FLEXT_CALLBACK_V(m_forceZ)
	FLEXT_CALLBACK_V(m_posZ)
	FLEXT_CALLBACK_V(m_Nmax)
	FLEXT_CALLBACK_V(m_Nmin)
	FLEXT_CALLBACK_V(m_forceN)
	FLEXT_CALLBACK_V(m_posN)
	FLEXT_CALLBACK_V(m_setMassId)
	FLEXT_CALLBACK_V(m_setLinkId)
	FLEXT_CALLBACK_V(m_setK)
	FLEXT_CALLBACK_V(m_setD)
	FLEXT_CALLBACK_V(m_setL)
	FLEXT_CALLBACK_V(m_setLtab)
	FLEXT_CALLBACK_V(m_setLmin)
	FLEXT_CALLBACK_V(m_setLmax)
	FLEXT_CALLBACK_V(m_setM)
	FLEXT_CALLBACK_V(m_setD2)
	FLEXT_CALLBACK_V(m_get)
	FLEXT_CALLBACK_V(m_delete_link)
	FLEXT_CALLBACK_V(m_delete_mass)
	FLEXT_CALLBACK_V(m_grab_mass)
};
// -------------------------------------------------------------- STATIC VARIABLES
// -------------------------------------------------------------------------------

template<int N> const t_symbol *msdN<N>::S_Reset;
template<int N> const t_symbol *msdN<N>::S_Mass;
template<int N> const t_symbol *msdN<N>::S_Link;
template<int N> const t_symbol *msdN<N>::S_iLink;
template<int N> const t_symbol *msdN<N>::S_tLink;
template<int N> const t_symbol *msdN<N>::S_nLink;
template<int N> const t_symbol *msdN<N>::S_tabLink;
template<int N> const t_symbol *msdN<N>::S_Mass_deleted;
template<int N> const t_symbol *msdN<N>::S_Link_deleted;
template<int N> const t_symbol *msdN<N>::S_massesPos;
template<int N> const t_symbol *msdN<N>::S_massesPosName;
template<int N> const t_symbol *msdN<N>::S_massesPosNo;
template<int N> const t_symbol *msdN<N>::S_massesPosId;
template<int N> const t_symbol *msdN<N>::S_linksPos;
template<int N> const t_symbol *msdN<N>::S_linksPosName;
template<int N> const t_symbol *msdN<N>::S_linksPosNo;
template<int N> const t_symbol *msdN<N>::S_linksPosId;
template<int N> const t_symbol *msdN<N>::S_linksLenghts;
template<int N> const t_symbol *msdN<N>::S_linksLenghtsMean;
template<int N> const t_symbol *msdN<N>::S_linksLenghtsStd;
template<int N> const t_symbol *msdN<N>::S_linksLenghtsNo;
template<int N> const t_symbol *msdN<N>::S_linksLenghtsId;
template<int N> const t_symbol *msdN<N>::S_massesForces;
template<int N> const t_symbol *msdN<N>::S_massesForcesName;
template<int N> const t_symbol *msdN<N>::S_massesForcesMean;
template<int N> const t_symbol *msdN<N>::S_massesForcesStd;
template<int N> const t_symbol *msdN<N>::S_massesForcesNo;
template<int N> const t_symbol *msdN<N>::S_massesForcesId;
template<int N> const t_symbol *msdN<N>::S_massesSpeeds;
template<int N> const t_symbol *msdN<N>::S_massesSpeedsName;
template<int N> const t_symbol *msdN<N>::S_massesSpeedsMean;
template<int N> const t_symbol *msdN<N>::S_massesSpeedsStd;
template<int N> const t_symbol *msdN<N>::S_massesSpeedsNo;
template<int N> const t_symbol *msdN<N>::S_massesSpeedsId;
template<int N> const t_symbol *msdN<N>::S_massesPosL;
template<int N> const t_symbol *msdN<N>::S_massesPosXL;
template<int N> const t_symbol *msdN<N>::S_massesPosYL;
template<int N> const t_symbol *msdN<N>::S_massesPosZL;
template<int N> const t_symbol *msdN<N>::S_massesPosStd;
template<int N> const t_symbol *msdN<N>::S_massesPosMean;
template<int N> const t_symbol *msdN<N>::S_massesForcesL;

#define MSD(NAME,CLASS,N) \
typedef msdN<N> CLASS; \
template<> FLEXT_NEW_V(NAME,CLASS)
