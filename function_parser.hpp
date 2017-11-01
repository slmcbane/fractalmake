#pragma once

#include <complex>
#include <exception>
#include <functional>
#include <sstream>
#include <string>

namespace fractals {

namespace fn_parser {

template <typename cmplx>
using fn = std::function<cmplx(const cmplx&, const cmplx&)>;

namespace {
    
    void skip_whitespace(std::istringstream& stream)
    {
        while ((stream.peek() == ' ' || stream.peek() == '\t') 
               && !stream.eof())
            stream.get();
    }
    
}

class FunctionParser
{
public:
    FunctionParser(const std::string&);
    
    template <typename cmplx = std::complex<double>>
    fn<cmplx> get();

private:
    std::istringstream stream;

    template <typename cmplx>
    fn<cmplx> parse_expr();

    template <typename cmplx>
    fn<cmplx> parse_lvl0_term();

    template <typename cmplx>
    fn<cmplx> parse_lvl1_term();

    template <typename cmplx>
    fn<cmplx> parse_factor();

    template <typename cmplx>
    fn<cmplx> parse_number_or_fn();

    template <typename cmplx>
    fn<cmplx> parse_function();

    template <typename cmplx>
    fn<cmplx> parse_number();

    enum class operators {
        PLUS, MINUS, MUL, DIV, EXPONENT
    };

    enum class functions {
        ABS, EXP, SIN, COS, TAN, ASIN, ACOS, ATAN, SQRT, REAL, IMAG,
        CONSTANT
    };

    struct ParseException : std::exception 
    {};

    operators parse_plus_or_minus()
    {
        skip_whitespace(stream);
        if (stream.peek() == '+') {
            stream.get();
            return operators::PLUS;
        } else if (stream.peek() == '-') {
            stream.get();
            return operators::MINUS;
        } else {
            throw ParseException{};
        }
    }

    functions get_function_name();
};

inline FunctionParser::FunctionParser(const std::string& str) : stream(str) {}

template <typename cmplx>
inline fn<cmplx> FunctionParser::get()
{
    return parse_expr<cmplx>();
}

template <typename cmplx>
inline fn<cmplx> FunctionParser::parse_expr()
{
    auto f = parse_lvl0_term<cmplx>();
    skip_whitespace(stream);
    
    while (stream.peek() == '+' || stream.peek() == '-') {
        auto op = stream.get() == '+' ? operators::PLUS : operators::MINUS;
        auto g = parse_lvl0_term<cmplx>();
        if (op == operators::PLUS) {
            f = [=](const cmplx& z, const cmplx& c) {
                return f(z, c) + g(z, c);
            };
        } else {
            f = [=](const cmplx& z, const cmplx& c) {
                return f(z, c) - g(z, c);
            };
        }
        skip_whitespace(stream);
    }
    return f;
}

template <typename cmplx>
inline fn<cmplx> FunctionParser::parse_lvl0_term()
{
    auto f = parse_lvl1_term<cmplx>();
    skip_whitespace(stream);

    while (stream.peek() == '*' || stream.peek() == '/') {
        auto op = stream.get() == '*' ? operators::MUL : operators::DIV;
        auto g = parse_lvl1_term<cmplx>();
        if (op == operators::MUL) {
            f = [=](const cmplx& z, const cmplx& c){ return f(z, c) * g(z, c); };
        } else {
            f = [=](const cmplx& z, const cmplx& c){ return f(z, c) / g(z, c); };
        }
        skip_whitespace(stream);
    }
    return f;
}

template <typename cmplx>
inline fn<cmplx> FunctionParser::parse_lvl1_term()
{
    auto f = parse_factor<cmplx>();
    skip_whitespace(stream);

    while (stream.peek() == '^') {
        stream.get();
        auto g = parse_factor<cmplx>();
        f = [=](const cmplx& z, const cmplx& c) { return std::pow(f(z, c), g(z, c)); };
        skip_whitespace(stream);
    }
    return f;
}

template <typename cmplx>
struct imaginary_unit
{
    constexpr cmplx operator()(const cmplx& z, const cmplx& c) const
    {
        return cmplx(0, 1);
    }
};

template <typename cmplx>
struct identity_fn_z
{
    constexpr cmplx operator()(const cmplx& z, const cmplx& c) const
    {
        return z;
    }
};

template <typename cmplx>
struct identity_fn_c
{
    constexpr cmplx operator()(const cmplx& z, const cmplx& c) const
    {
        return c;
    }
};

template <typename cmplx>
struct constant_fn
{
    const cmplx c;
    constant_fn(const cmplx& z) : c(z) {}
    cmplx operator()(const cmplx& a, const cmplx& b) const
    {
        return c;
    }
};

template <typename cmplx>
inline fn<cmplx> FunctionParser::parse_factor()
{
    skip_whitespace(stream);
    switch (stream.peek()) {
    case '+':
    case '-':
    {
        auto op = parse_plus_or_minus();
        auto f = parse_factor<cmplx>();
        if (op == operators::MINUS)
            f = [=](const cmplx& z, const cmplx& c){ return -f(z, c); };
        return f;
    }
    case 'I':
        stream.get();
        return imaginary_unit<cmplx>{};
    case 'z':
        stream.get();
        return identity_fn_z<cmplx>{};
    case '(':
        stream.get();
        return parse_expr<cmplx>();
    default:
        return parse_number_or_fn<cmplx>();
    }
}

template <typename cmplx>
inline fn<cmplx> FunctionParser::parse_number_or_fn()
{
    char c = stream.peek();
    if ((c >= '0' && c <= '9') || c == '.') {
        return parse_number<cmplx>();
    } else {
        return parse_function<cmplx>();
    }
}

template <typename cmplx>
inline fn<cmplx> FunctionParser::parse_number()
{
    typename cmplx::value_type x;
    stream >> x;
    return constant_fn<cmplx>(x);
}

inline FunctionParser::functions FunctionParser::get_function_name()
{
    char c = stream.get();
    switch (c) {
    case 'a':
        c = stream.get();
        switch (c) {
        case 'b':
            stream.get();
            return functions::ABS;
        case 's':
            stream.get(); stream.get();
            return functions::ASIN;
        case 'c':
            stream.get(); stream.get();
            return functions::ACOS;
        case 't':
            stream.get(); stream.get();
            return functions::ATAN;
        default:
            throw ParseException{};
        }
    case 'e':
        stream.get(); stream.get();
        return functions::EXP;
    case 's':
        if (stream.get() == 'i') {
            stream.get();
            return functions::SIN;
        } else {
            stream.get(); stream.get();
            return functions::SQRT;
        }
    case 'c':
        if (stream.peek() != 'o')
            return functions::CONSTANT;
        stream.get(); stream.get();
        return functions::COS;
    case 't':
        stream.get(); stream.get();
        return functions::TAN;
    case 'r':
        stream.get(); stream.get(); stream.get();
        return functions::REAL;
    case 'i':
        stream.get(); stream.get(); stream.get();
        return functions::IMAG;
    default:
        throw ParseException{};
    }
}

template <typename cmplx>
inline fn<cmplx> FunctionParser::parse_function()
{
    functions fn_name = get_function_name();
    if (fn_name == functions::CONSTANT)
        return identity_fn_c<cmplx>{};
    skip_whitespace(stream);
    if (stream.peek() != '(')
        throw ParseException{};
    stream.get();
    auto f = parse_expr<cmplx>();
    skip_whitespace(stream);
    if (stream.peek() != ')')
        throw ParseException{};
    stream.get();

    switch (fn_name) {
    case functions::ABS:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::abs(f(z, c)); };
    case functions::EXP:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::exp(f(z, c)); };
    case functions::SIN:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::sin(f(z, c)); };
    case functions::COS:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::cos(f(z, c)); };
    case functions::TAN:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::tan(f(z, c)); };
    case functions::ASIN:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::asin(f(z, c)); };
    case functions::ACOS:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::acos(f(z, c)); };
    case functions::ATAN:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::atan(f(z, c)); };
    case functions::SQRT:
        return [=](const cmplx& z, const cmplx& c) 
            { return std::sqrt(f(z, c)); };
    case functions::REAL:
        return [=](const cmplx& z, const cmplx& c) 
            { return cmplx(f(z, c).real()); };
    default:
        return [=](const cmplx& z, const cmplx& c) 
            { return cmplx(0, f(z, c).imag()); };
    }
}

}

}