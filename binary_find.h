#ifndef BINARY_FIND_H
#define BINARY_FIND_H

#include <algorithm>

template<class Iter, class T>
Iter binary_find(Iter begin, Iter end, T val)
{
    // Finds the lower bound in at most log(last - first) + 1 comparisons
    Iter i = std::lower_bound(begin, end, val);

    if (i != end && !(val < *i))
        return i; // found
    else
        return end; // not found
}

#endif // BINARY_FIND_H
