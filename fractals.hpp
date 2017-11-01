#pragma once

#include "qdbmp.h"
#include "vector_slice.hpp"

#include <thread>
#include <functional>

namespace fractals
{

// Heuristic; determines how many pixels in the resulting image each thread
// will be responsible for.
constexpr unsigned points_per_thread = 100000;

/*
 * This struct represents a region in the complex plane; the two complex
 * numbers 'lower_left' and 'upper_right' describe the rectangle comprising
 * the domain, and 'nacross' and 'nup' specify the number of points in each
 * dimension (up is the imaginary dimension) that will be computed.
 */
template <typename cmplx>
struct Domain
{
    cmplx lower_left;
    cmplx upper_right;
    unsigned nacross;
    unsigned nup;

    Domain() = default;
    constexpr Domain(cmplx ll, cmplx ur, unsigned na, unsigned nu) : 
        lower_left(ll), upper_right(ur), nacross(na), nup(nu) {}
};

/*
 * A Fractal is a domain along with an array of values corresponding to
 * each point in the domain. Values are given in row-major order and 
 * increasing in both the real and imaginary directions (so left->right and
 * bottom->top).
 */
template <typename cmplx>
struct Fractal
{
    Domain<cmplx> dom;
    std::vector<unsigned> values;

    Fractal(const Domain<cmplx>& d) : dom(d)
    {
        values.resize(dom.nacross * dom.nup);
    }
};

// Acquire/release the mutex for domain decomposition.
void lock_decomposition_mutex();
void unlock_decomposition_mutex();

// Get the beginning of the next domain to assign work.
unsigned& get_domain_start();

/*
 * For internal use only. Given the input domain and a reference to the values
 * array of a target fractal object, finds the next domain that the thread 
 * will be responsible for computing. 'target' is filled with this data and 
 * 'output' becomes a vector_slice that is a window into the correct region in
 * 'vals'.
 */
template <typename cmplx>
bool decompose_domain(const Domain<cmplx>& dom, Domain<cmplx>& target,
                      std::vector<unsigned>& vals, 
                      vector_slice<unsigned>& output)
{
    unsigned& dom_start = get_domain_start();
    if (dom_start >= dom.nup)
        return true;

    const unsigned first_row = dom_start;
    dom_start += (points_per_thread / dom.nacross + 1);
    const unsigned last_row = dom_start < dom.nup ? dom_start : dom.nup;

    typename cmplx::value_type dy = 
        (dom.upper_right.imag() - dom.lower_left.imag()) /
        (dom.nup - 1);

    target.lower_left = dom.lower_left + cmplx(0.0, dy * first_row);
    target.upper_right = cmplx(dom.upper_right.real(), 
            dy*(last_row - 1) + dom.lower_left.imag());
    target.nacross = dom.nacross;
    target.nup = last_row - first_row;
    output = vector_slice<unsigned>(vals, first_row*dom.nacross);
    return false;
}

/*
 * For internal use only; given the overall domain and the overall array of
 * values, as well as a function that checks points in a domain, get a slice
 * of work to do and do it repeatedly until we're done.
 */
template <typename cmplx, typename Check>
void check_points_thread(const Domain<cmplx>& dom, std::vector<unsigned>& vals,
                         const Check& chk)
{
    while (true) {
        Domain<cmplx> this_dom;
        vector_slice<unsigned> my_slice;

        lock_decomposition_mutex();
        bool done = decompose_domain(dom, this_dom, vals, my_slice);
        unlock_decomposition_mutex();

        if (done) 
            break;
        chk(this_dom, my_slice);
    }
}

/*
 * This is the routine that a caller is intended to use.
 *
 * To use this routine, specify a Domain (described above) and a function
 * that checks points in a domain. This function should have the signature
 *     void chk(const Domain<cmplx>& d, vector_slice<unsigned>& slice);
 * 
 * and should fill values in the slice given in row major order increasing
 * in both the real and imaginary dimension (as described above in comments
 * on the Fractal format).
 *
 * The optional argument num_threads simply specifies how many threads are 
 * to be used by the routine. As long as the heuristic 'points_per_thread'
 * defined above is good work will be distributed quite evenly, resulting 
 * in a nearly-linear speedup up to the number of cores on your machine.
 */
template <typename cmplx, typename Check>
Fractal<cmplx> make_fractal(const Domain<cmplx>& dom, const Check& chk, 
                            unsigned num_threads = 1)
{
    using std::thread;
    std::vector<std::thread> threads;
    Fractal<cmplx> f(dom);

    auto check_points = [&] ()
    {
        check_points_thread(dom, f.values, chk);
    };

    for (unsigned tid = 0; tid < num_threads; ++tid) {
        threads.push_back(thread(check_points)); 
    }
    for (auto& thr: threads)
        thr.join();
    return f;
}

struct Color
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

/*
 * Save the specified fractal to a bitmap image with given name. The functional
 * given should map unsigned integers from 0 - whatever max iterations you're 
 * using into an RGB color scale and should have the signature
 *     void calc_color(unsigned iters, Color& clr);
 *
 * If the C library I'm using to write the bitmap encounters an error I simply
 * throw the description of the error as an exception and make no attempt to
 * recover.
 */
template <typename cmplx, typename F>
void save_fractal_img(const Fractal<cmplx>& frac, FILE* f,
                      const F& calc_color)
{
    unsigned width, height;
    width = frac.dom.nacross;
    height = frac.dom.nup;
    BMP* bmp = BMP_Create(width, height, 24);
    if (BMP_GetError() != BMP_OK) {
        BMP_Free(bmp);
        throw BMP_GetErrorDescription();
    }

    Color output;
    for (unsigned i = 0; i < height; ++i) {
        for (unsigned j = 0; j < width; ++j) {
            calc_color(frac.values[i*width + j], output);
            BMP_SetPixelRGB(bmp, j, height - i - 1, output.r, output.g, output.b);
        }
    }
    BMP_WriteFile(bmp, f);
    if (BMP_GetError() != BMP_OK) {
        BMP_Free(bmp);
        throw BMP_GetErrorDescription();
    }
    BMP_Free(bmp);
}

}
