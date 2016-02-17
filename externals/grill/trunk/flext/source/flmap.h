/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flmap.h
	\brief special map class (faster and less memory-consuming than std::map)   
*/

#ifndef __FLMAP_H
#define __FLMAP_H

#include "flprefix.h"

/*!	\defgroup FLEXT_SUPPORT Flext support classes
	@{
*/

#include "flpushns.h"

FLEXT_TEMPLATE
class FLEXT_SHARE TableAnyMap
{
public:

    virtual TableAnyMap *_newmap(TableAnyMap *parent) = 0;
    virtual void _delmap(TableAnyMap *map) = 0;

    struct Data {
        void operator()(size_t k,void *v) { key = k,value = v; }
        void operator =(void *v) { value = v; }

        size_t key;
        void *value;
    };

protected:
    // constructor and destructor are protected so that they can't be directly instantiated 

    TableAnyMap(TableAnyMap *p,Data *dt)
        : data(dt)
        , parent(p),left(0),right(0) 
        , n(0)
    {}

    virtual ~TableAnyMap();

public:

#if 0 // set 1 for asserting the map structure (very cpu-intensive!)
    void check(int tsize) { if(n) _check(tsize); }
#else
//    void check(int tsize) {}
#endif

    void *insert(int tsize,size_t k,void *t)
    {
        void *r;
        if(LIKELY(n)) 
            r = _set(tsize,k,t);
        else {
            data[n++](k,t);
            r = 0;
        }
//        check(tsize);
        return r;
    }

    void *find(int tsize,size_t k) const { return LIKELY(n)?_find(tsize,k):0; }

    void *remove(int tsize,size_t k) 
	{ 
		void *r = LIKELY(n)?_remove(tsize,k):0; 
//		check(tsize); 
		return r; 
	}

    virtual void clear();

    class FLEXT_SHARE iterator
    {
    public:
        iterator(): map(0) {}
        iterator(const TableAnyMap &m): map(&m),ix(0) { leftmost(); }
        iterator(const iterator &it): map(it.map),ix(it.ix) {}
    
        iterator &operator =(const iterator &it) { map = it.map,ix = it.ix; return *this; }

        operator bool() const { return map && ix < map->n; }

        // no checking here!
        void *data() const { return map->data[ix].value; }
        size_t key() const { return map->data[ix].key; }

        iterator &operator ++() { forward(); return *this; }  

    protected:
        void leftmost()
        {
            // search smallest branch (go left as far as possible)
            const TableAnyMap *nmap;
            while((nmap = map->left) != 0) map = nmap;
        }

        void forward();

		// pointers to map and index within
        const TableAnyMap *map;
        int ix;
    };

    void _init(size_t k,void *t) { data[0](k,t); n = 1; }

    void *_toleft(int tsize,size_t k,void *t)
    {
        if(left)
            return left->_set(tsize,k,t);
        else {
            (left = _newmap(this))->_init(k,t);
            return 0;
        }
    }

    void *_toright(int tsize,size_t k,void *t)
    {
        if(right)
            return right->_set(tsize,k,t);
        else {
            (right = _newmap(this))->_init(k,t);
            return 0;
        }
    }

    void *_toleft(int tsize,Data &v) { return _toleft(tsize,v.key,v.value); }
    void *_toright(int tsize,Data &v) { return _toright(tsize,v.key,v.value); }

    void *_set(int tsize,size_t k,void *t);
    void *_find(int tsize,size_t k) const;
    void *_remove(int tsize,size_t k);

#ifdef FLEXT_DEBUG
    void _check(int tsize);
#endif

    Data *data;
    TableAnyMap *parent,*left,*right;
    int n;

    //! return index of data item with key <= k
    //! \note index can point past the last item!
    unsigned int _tryix(size_t k) const
    {
        unsigned int ix = 0,b = n;
		while(ix != b) {
			const unsigned int c = (ix+b)>>1;
			const size_t dk = data[c].key;
			if(k == dk)
				return c;
			else if(k < dk)
				b = c;
			else if(ix < c)
				ix = c;
			else
				return b;
		}
        return ix;
    }

    void _eraseempty(TableAnyMap *&b)
    {
        if(!b->n) { 
            // remove empty branch
            _delmap(b); b = 0; 
        }
    }

    void _getsmall(Data &dt);
    void _getbig(Data &dt);

private:
    // hide, so that it can't be used.....
    explicit TableAnyMap(const TableAnyMap &): data(NULL) {}
    TableAnyMap &operator =(const TableAnyMap &) { return *this; }
};

template <typename K,typename T,int N = 8>
class TablePtrMap
    : 
#if (defined(_MSC_VER) && _MSC_VER < 1300) || defined(__BORLANDC__) || defined(__MWERKS__)
    public  // necessary for VC6
#endif
    FLEXT_TEMPINST(TableAnyMap)
{
public:
    TablePtrMap(): TableAnyMap(0,slots),count(0) {}
    virtual ~TablePtrMap() { clear(); }

    virtual void clear() { TableAnyMap::clear(); count = 0; }

    inline int size() const { return count; }

    inline T insert(K k,T t) 
    { 
        void *d = TableAnyMap::insert(N,*(size_t *)&k,(void *)t); 
        if(!d) ++count;
        return (T)d;
    }

    inline T find(K k) const { return (T)TableAnyMap::find(N,*(size_t *)&k); }

    inline T remove(K k) 
    { 
        void *d = TableAnyMap::remove(N,*(size_t *)&k); 
        if(LIKELY(d)) --count;
        return (T)d;
    }


    class iterator
        : TableAnyMap::iterator
    {
    public:
        iterator() {}
        iterator(const TablePtrMap &m): TableAnyMap::iterator(m) {}
        iterator(const iterator &it): TableAnyMap::iterator(it) {}

        // this ugly syntax (cast to parent class) is needed for MSVC6 

        inline iterator &operator =(const iterator &it) { ((TableAnyMap::iterator &)*this) = it; return *this; }

        inline operator bool() const { return (bool)((TableAnyMap::iterator &)*this); } 
        inline T data() const { return (T)(((TableAnyMap::iterator &)*this).data()); }
        inline K key() const { return (K)(((TableAnyMap::iterator &)*this).key()); }

        inline iterator &operator ++() { ++((TableAnyMap::iterator &)*this); return *this; }  
    };

protected:
    TablePtrMap(TableAnyMap *p): TableAnyMap(p,slots),count(0) {}

    virtual TableAnyMap *_newmap(TableAnyMap *parent) { return new TablePtrMap(parent); }
    virtual void _delmap(TableAnyMap *map) { delete (TablePtrMap *)map; }

    int count;
    Data slots[N];

private:
    explicit TablePtrMap(const TableAnyMap &p) {}
};
            
#include "flpopns.h"

//! @} // FLEXT_SUPPORT

#endif
