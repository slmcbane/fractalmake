/*
 * Driver program (running 'make' builds this and creates the 'fractalmake'
 * executable). Expects a single command line argument, the name of a 
 * configuration file to read. This driver expects all options to be 
 * specified (see the sample config "mandelbrot.cfg" for example and
 * documentation of options). After parsing the options executes the
 * setup that was specified and saves the image.
 */

#include "options.hpp"
#include "color_scale.hpp"
#include "fractals.hpp"

#include <complex>
#include <cstdlib>
#include <fstream>

using cmplx = std::complex<float>;

int main(int argc, char* argv[])
{
    if (argc != 2)
        throw std::exception();

    std::ifstream config(argv[1]);
    if (!config.is_open())
        throw std::exception();

    fractals::options::FractalOptions<cmplx> opts;
    try {
        opts = fractals::options::get_options<cmplx>(config);
    } catch (fractals::options::ParsingException exc) {
        std::cerr << "Exception caught during option parsing:\n"
            << exc.what() << "\n";
        std::exit(1);
    }

    ColorScale colorscale(opts.colors);

    auto point_checker = [=](const fractals::Domain<cmplx>& dom, 
        vector_slice<unsigned>& slice) -> void
    {
        auto dx = (dom.upper_right.real() - dom.lower_left.real()) /
            (dom.nacross - 1);
        auto dy = (dom.upper_right.imag() - dom.lower_left.imag()) /
            (dom.nup - 1);
        
        for (unsigned i = 0; i < dom.nup; ++i) {
            for (unsigned j = 0; j < dom.nacross; ++j) {
                cmplx c(dom.lower_left.real() + j*dx, 
                        dom.lower_left.imag() + i*dy);
                slice[i*dom.nacross + j] = opts.test_function(c);
            }
        }
    };

    auto result = fractals::make_fractal(opts.domain, point_checker, opts.numthreads);

    std::cout << "Saving image now... \n";

    save_fractal_img(result, opts.output == "-" ? stdout : 
                     fopen(opts.output.c_str(), "wb"), 
    [&] (unsigned iters, fractals::Color& clr)
    {
        clr = iters == 0 ? fractals::Color{0, 0, 0} : colorscale.color(iters);
    });
    return 0;
}
