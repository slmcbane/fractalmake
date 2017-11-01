#pragma once

#include <cassert>
#include <vector>

#include <Eigen/Dense>

namespace {

using Eigen::MatrixXd;
using Eigen::VectorXd;

VectorXd find_spline_coeffs(const std::vector<double>& xs_, 
                            const std::vector<double>& ys_)
{
    assert(xs_.size() == ys_.size());
    unsigned nintervals = xs_.size() - 1;
    MatrixXd A = MatrixXd::Zero(nintervals*4, nintervals*4);
    VectorXd B = VectorXd::Zero(nintervals*4);

    A(0, 0) = 3*xs_[0]*xs_[0];
    A(0, 1) = 2*xs_[0];
    A(0, 2) = 1.0;
    A(1, 0) = xs_[0] * xs_[0] * xs_[0];
    A(1, 1) = xs_[0] * xs_[0];
    A(1, 2) = xs_[0];
    A(1, 3) = 1.0;
    B(1) = ys_[0];

    unsigned curr_row = 2;
    for (unsigned node = 1; node < nintervals; ++node) {
        double xn2 = xs_[node] * xs_[node];
        double xn3 = xn2 * xs_[node];
        unsigned j = (node - 1) * 4;
        A(curr_row, j) = xn3;
        A(curr_row, j+1) = xn2;
        A(curr_row, j+2) = xs_[node];
        A(curr_row, j+3) = 1.0;
        B(curr_row) = ys_[node];
        curr_row += 1;
        A(curr_row, j) = 3*xn2;
        A(curr_row, j+1) = 2*xs_[node];
        A(curr_row, j+2) = 1.0;
        A(curr_row, j+4) = -3*xn2;
        A(curr_row, j+5) = -2*xs_[node];
        A(curr_row, j+6) = -1.0;
        curr_row += 1;
        A(curr_row, j) = 6*xs_[node];
        A(curr_row, j+1) = 2.0;
        A(curr_row, j+4) = -6*xs_[node];
        A(curr_row, j+5) = -2.0;
        curr_row += 1;
        A(curr_row, j+4) = xn3;
        A(curr_row, j+5) = xn2;
        A(curr_row, j+6) = xs_[node];
        A(curr_row, j+7) = 1.0;
        B(curr_row) = ys_[node];
        curr_row += 1;
    }

    unsigned j = (nintervals - 1) * 4;
    A(curr_row, j) = xs_[nintervals] * xs_[nintervals] * xs_[nintervals];
    A(curr_row, j+1) = xs_[nintervals] * xs_[nintervals];
    A(curr_row, j+2) = xs_[nintervals];
    A(curr_row, j+3) = 1.0;
    B(curr_row++) = ys_[nintervals];
    A(curr_row, j) = 3*xs_[nintervals]*xs_[nintervals];
    A(curr_row, j+1) = 2 * xs_[nintervals];
    A(curr_row, j+2) = 1.0;

    return A.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(B);
}

}

class Spline
{
private:
    struct Coefficients {
        double a; double b;
        double c; double d;
    };
    std::vector<double> xs_;
    std::vector<double> ys_;
    std::vector<Coefficients> coeffs_;

    template <typename It>
    It get_element_position(double x, It start, It end) const
    {
        if (end == start) {
            return start;
        } else if (end - start == 1) {
            return *start > x ? start : end;
        } else {
            It middle = start + (end - start) / 2;
            if (*middle > x) {
                return get_element_position(x, start, middle);
            } else {
                return get_element_position(x, middle, end);
            }
        }
    }

public:
    typedef std::pair<double, double> Point;

    void add_point(Point p)
    {
        auto it = get_element_position(p.first, xs_.begin(), xs_.end());
        auto offset = it - xs_.begin();
        xs_.insert(it, p.first);
        ys_.insert(ys_.begin() + offset, p.second);
    }

    void reset()
    {
        *this = Spline();
    }

    void calculate()
    {
        assert(xs_.size() == ys_.size());
        // X is a vector of a, b, c, d for each interval in the spline
        // in that order.
        auto X = find_spline_coeffs(xs_, ys_);
        coeffs_.resize(xs_.size() - 1);
        for (unsigned i = 0; i < coeffs_.size()*4; i += 4)
            coeffs_[i/4] = Coefficients{ X(i), X(i + 1), X(i + 2), X(i + 3) };
    }

    double operator()(double x) const
    {
        auto it = get_element_position(x, xs_.begin(), xs_.end());
        assert(!(it == xs_.begin() || it == xs_.end()));
        const auto& coeffs = coeffs_[it - xs_.begin() - 1];
        return coeffs.a * x * x * x + coeffs.b * x * x + coeffs.c * x +
            coeffs.d;
    }
};
