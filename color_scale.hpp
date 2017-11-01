#pragma once

#include "fractals.hpp"
#include "spline.hpp"

class ColorScale
{
private:
    Spline rspline;
    Spline gspline;
    Spline bspline;

    unsigned char get_val(double i, const Spline& s) const
    {
        double v = s(i);
        if (v < 0.0)
            return 0;
        else if (v > 255.0)
            return 255;
        else 
            return (unsigned char)(v);
    }
public:
    ColorScale(const std::vector<std::pair<unsigned, fractals::Color>>& points)
    {
        for (const auto& point: points) {
            rspline.add_point(std::make_pair(
                        double(point.first), double(point.second.r)));
            gspline.add_point(std::make_pair(
                        double(point.first), double(point.second.g)));
            bspline.add_point(std::make_pair(
                        double(point.first), double(point.second.b)));
        }
        rspline.calculate();
        gspline.calculate();
        bspline.calculate();
    }

    unsigned char r(unsigned i) const
    {
        return get_val(double(i), rspline);
    }

    unsigned char g(unsigned i) const
    {
        return get_val(double(i), gspline);
    }

    unsigned char b(unsigned i) const
    {
        return get_val(double(i), bspline);
    }
    
    fractals::Color color(unsigned i) const
    {
        return fractals::Color{r(i), g(i), b(i)};
    }
};
