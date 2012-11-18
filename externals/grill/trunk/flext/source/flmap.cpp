/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3657 $
$LastChangedDate: 2009-02-09 17:58:30 -0500 (Mon, 09 Feb 2009) $
$LastChangedBy: thomas $
*/

/*! \file flmap.cpp
    \brief flext container classes.
*/
 
#include "flext.h"
#include "flmap.h"

#include "flpushns.h"

TableAnyMap::~TableAnyMap() { clear(); }

void TableAnyMap::clear() 
{
    if(left) { _delmap(left); left = NULL; }
    if(right) { _delmap(right); right = NULL; }
    n = 0;
}


void *TableAnyMap::_set(int tsize,size_t k,void *t)
{
    FLEXT_ASSERT(n);

    if(n < tsize) {
        // fall through
    }
    else if(k < data[0].key)
        return _toleft(tsize,k,t);
    else if(k > data[tsize-1].key)
        return _toright(tsize,k,t);

    int ix = _tryix(k);
    if(ix >= n) {
        FLEXT_ASSERT(ix == n);
        // after last entry
        data[n++](k,t);
        return NULL;
    }

    size_t dk = data[ix].key;
    if(k == dk) {
        // update data in existing slot (same key)
        void *a = data[ix].value;
        data[ix] = t;
        return a;
    }
    else {
        // insert new slot by shifting the higher ones
        FLEXT_ASSERT(k < dk);
        void *a;
        if(n == tsize)
            a = _toright(tsize,data[tsize-1]);
        else {
            ++n;
            a = NULL;
        }

        Data *tg = data+ix;
        for(Data *d = data+n-1; d > tg; d--) d[0] = d[-1];
        (*tg)(k,t);
        return a;
    }
}

void *TableAnyMap::_find(int tsize,size_t k) const
{
    FLEXT_ASSERT(n);
    if(n < tsize) {
        // fall through
    }
    else if(k < data[0].key)
        return left?left->_find(tsize,k):NULL;
    else if(k > data[n-1].key)
        return right?right->_find(tsize,k):NULL;

    const int ix = _tryix(k);
    return ix < n && data[ix].key == k?data[ix].value:NULL;
}

#ifdef FLEXT_DEBUG
void TableAnyMap::_check(int tsize)
{
    FLEXT_ASSERT(n);

    size_t k = data[0].key;
    for(int i = 1; i < n; ++i) {
        size_t k2 = data[i].key;
        FLEXT_ASSERT(k < k2);
        k = k2;
    }

    if(left || right) FLEXT_ASSERT(n == tsize);

    if(left) { 
        FLEXT_ASSERT(flext::MemCheck(left)); 
        left->_check(tsize); 
    }
    if(right) { 
        FLEXT_ASSERT(flext::MemCheck(right)); 
        right->_check(tsize); 
    }
}
#endif

void *TableAnyMap::_remove(int tsize,size_t k)
{
    FLEXT_ASSERT(n);
    if(n < tsize) {
        // fall through
    }
    else if(k < data[0].key) {
        void *r = left?left->_remove(tsize,k):NULL;
        if(r) _eraseempty(left);
        return r;
    }
    else if(k > data[n-1].key) {
        void *r = right?right->_remove(tsize,k):NULL;
        if(r) _eraseempty(right);
        return r;
    }

    const int ix = _tryix(k);
    if(ix >= n || data[ix].key != k)
        return NULL;
    else {
        // found key in this map
        void *ret = data[ix].value;

        Data dt;
        bool fnd,ins = false;
        if(n >= tsize) {
            // if this table is full get fill-in elements from branches
            if(left) {
                // try to get biggest element from left branch
                left->_getbig(dt);
                _eraseempty(left);
                fnd = true,ins = true;
            }
            else if(right) {
                // try to get smallest element from right branch
                right->_getsmall(dt);
                _eraseempty(right);
                fnd = true;
            }
            else
                fnd = false;
        }
        else fnd = false;

        if(ins) {
            // insert smaller element from left
            for(int i = ix; i; --i) data[i] = data[i-1];
            data[0] = dt;
        }
        else {
            // shift elements
            for(int i = ix+1; i < n; ++i) data[i-1] = data[i];
            // insert bigger element from right or reduce table size
            if(fnd)
                data[n-1] = dt;
            else
                --n;
        }

        return ret;
    }
}

void TableAnyMap::_getbig(Data &dt)
{
    FLEXT_ASSERT(n);

    if(right) {
        right->_getbig(dt);
        _eraseempty(right);
    }
    else {
        dt = data[n-1];
        if(left) {
            for(int i = n-1; i; --i) data[i] = data[i-1];
            left->_getbig(data[0]);
            _eraseempty(left);
        }
        else
            --n;
    }
}

void TableAnyMap::_getsmall(Data &dt)
{
    FLEXT_ASSERT(n);

    if(left) {
        left->_getsmall(dt);
        _eraseempty(left);
    }
    else {
        dt = data[0];
        for(int i = 1; i < n; ++i) data[i-1] = data[i];
        if(right) {
            right->_getsmall(data[n-1]);
            _eraseempty(right);
        }
        else
            --n;
    }
}

void TableAnyMap::iterator::forward() 
{ 
    FLEXT_ASSERT(map || ix >= map->n);
	
	if(++ix >= map->n) {
		TableAnyMap *nmap;

		// we reached the end of the slots
		if(map->right) {
			// climb up one
			map = map->right;
			leftmost();
			ix = 0;
		}
		else {
			// fall back
			for(;;) {
				nmap = map->parent;
				if(!nmap) break; // no parent
				if(nmap->left == map) {
					// ok, we are in front of the slots now
					ix = 0;
					map = nmap;
					break;
				}
				else {
					FLEXT_ASSERT(nmap->right == map);
					ix = (map = nmap)->n;
				}
			}
		}
	}
}

#include "flpopns.h"

