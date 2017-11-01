#pragma once

#include <stdexcept>
#include <vector>

/*
 * This is a utility class that I use in the fractal code to represent a 
 * "window" into a vector. Very simply just holds a pointer to the vector
 * and an offset at which we start reading/writing from the vector. No bounds
 * checking is done since I wanted minimal overhead.
 */
template <class Vec>
class Slice
{
private:
    Vec* vec_;
    unsigned start_;
public:

    Slice(Vec& v, unsigned begin) : vec_(&v), start_(begin)
    {
        if (!(begin < vec_->size())) {
            throw(std::out_of_range("The beginning of the range specified is "
                                    "greater than the size of given vector."));
        }
    }

    Slice() : vec_(nullptr), start_(0) {}
    
    using value_type = typename Vec::value_type;

    value_type& operator[](unsigned i)
    { 
        return (*vec_)[i + start_]; 
    }
    const value_type& operator[](unsigned i) const
    { 
        return (*vec_)[i + start_]; 
    }
};

template <typename T>
using vector_slice = Slice<std::vector<T>>;

template <typename T>
using const_vector_slice = Slice<const std::vector<T>>;

