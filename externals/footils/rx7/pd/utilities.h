#ifndef __UTILITIES_H
#define __UTILITIES_H
#include <vector>

template <class T> void freeVector(std::vector<T>& v)
{
	std::vector<T> empty;
	v.swap(empty);
}

#endif
