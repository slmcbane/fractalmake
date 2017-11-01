#pragma once

#include "fractals.hpp"
#include "function_parser.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fractals
{

namespace options
{

/*
 * Type used to return options from parsing an optfile.
 * Each of the options is paired with a bool indicating whether or not that
 * particular option was actually specified.
 */
template <typename cmplx>
struct FractalOptions
{
    Domain<cmplx> domain;
    std::string output;
    std::vector<std::pair<unsigned, Color>> colors;
    unsigned numthreads;
    std::function<unsigned(const cmplx&)> test_function;
};

template <typename cmplx>
FractalOptions<cmplx> get_options(std::istream&);

/*
 * These are the 6 types of tokens that will be extracted from an option file
 * by get_next_token. A keyword is any string not containing a punctuation 
 * character and beginning with a letter. A symbol is just a punctuation 
 * character. A string is any string of characters surrounded by "" (escapes 
 * using \ are not recognized; it will just be parsed into the string). floating
 * is a floating point number (optionally containing a sign, a decimal point, or
 * scientific notation). An integer is an integer constant; it may not contain
 * scientific notation. None of the options take negative integer values, so 
 * a negative sign will be ignored. eof is returned if the end of the file is
 * reached.
 */
enum class token_type
{
    keyword, symbol, string, floating, integer, eof
};

struct Token
{
    token_type type;
    std::string contents;
};

// Extract next token from the input stream.
Token get_next_token(std::istream& istream);

// Parse a domain based on complex number type cmplx from the input.
template <typename cmplx>
Domain<cmplx> parse_domain(std::istream& istream);

// Dummy types for switching inside parser.
struct parser_internal {};
struct parser_not_internal {};

// Parse a complex number from the input.
// A dummy type is used to switch between internal and not internal mode of
// operation.
template <typename cmplx>
cmplx parse_constant(std::istream& istream, parser_internal flag);

template <typename cmplx>
cmplx parse_constant(std::istream& istream, parser_not_internal flag);

// Parse a string from the input.
std::string parse_string(std::istream& istream);

// Parse an integer from the input.
unsigned parse_integer(std::istream& istream);

// Parse a list of colors from the input.
std::vector<std::pair<unsigned, Color>> parse_colorlist(std::istream& istream);

// Parse the function to test a point from the option file.
template <typename cmplx>
std::function<unsigned(const cmplx&)> parse_testfun(std::istream& istream);



class ParsingException : std::exception
{
private:
    std::string str_;
public:
    explicit ParsingException(const std::string& s) : str_(s) {}
    const char* what() const noexcept { return str_.c_str(); }
};

/*
 * This is the routine you should call to read an option file. It returns a
 * struct as specified above containing all the options specified in that 
 * file, or throws a ParsingException if the syntax was malformed.
 */
template <typename cmplx>
FractalOptions<cmplx> get_options(std::istream& istream)
{
    FractalOptions<cmplx> options;
    // colors domain num_threads output function
    std::array<bool, 5> got_options = { false };

    Token tok = get_next_token(istream);
    while (tok.type != token_type::eof) {
        if (tok.type != token_type::keyword)
            throw ParsingException("Got non-keyword where keyword was expected");
        
        Token tmp = get_next_token(istream);
        if (tmp.type != token_type::symbol || tmp.contents != ":")
            throw ParsingException("Missing ':' after keyword");
        
        if (tok.contents == "colors") {
            if (got_options[0])
                throw ParsingException("Multiple definition of 'colors'");
            options.colors = parse_colorlist(istream);
            got_options[0] = true;
        } else if (tok.contents == "domain") {
            if (got_options[1])
                throw ParsingException("Multiple definition of 'domain'");
            options.domain = parse_domain<cmplx>(istream);
            got_options[1] = true;
        } else if (tok.contents == "num_threads") {
            if (got_options[2])
                throw ParsingException("Multiple definition of 'num_threads'");
            options.numthreads = parse_integer(istream);
            got_options[2] = true;
        } else if (tok.contents == "output") {
            if (got_options[3])
                throw ParsingException("Multiple definition of 'output'");
            options.output = parse_string(istream);
            got_options[3] = true;
        } else if (tok.contents == "function") {
            if (got_options[4])
                throw ParsingException("Multiple definition of 'function'");
            options.test_function = parse_testfun<cmplx>(istream);
            got_options[4] = true;
        } else {
            throw ParsingException("Unrecognized option keyword");
        }
        tok = get_next_token(istream);
    }

    if (!std::all_of(got_options.begin(), got_options.end(), 
        [](bool b){return b;}))
        throw ParsingException("Some options not specified");

    return options;
}

template <typename cmplx>
Domain<cmplx> parse_domain(std::istream& istream)
{
    Token curr_token = get_next_token(istream);

    if (curr_token.type != token_type::symbol || curr_token.contents != "{")
        throw ParsingException("Error - open bracket not encountered after "
                               "\"domain:\"");
    
    cmplx lower_left = parse_constant<cmplx>(istream, parser_internal());
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Malformed domain expression - expected ','");
    cmplx upper_right = parse_constant<cmplx>(istream, parser_internal());
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Malformed domain expression - expected ','");
    unsigned nacross = parse_integer(istream);
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Malformed domain expression - expected ','");
    unsigned nup = parse_integer(istream);
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "}")
        throw ParsingException("Malformed domain expression - missing "
                               "close bracket '}'");
    Domain<cmplx> d = { lower_left, upper_right, nacross, nup };
    return d;
}

template <typename cmplx>
cmplx parse_constant(std::istream& istream, parser_not_internal flag)
{
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ":")
        throw ParsingException("Error - constant keyword not followed by ':'");
    
    return parse_constant<cmplx>(istream, parser_internal());
}

template <typename cmplx>
cmplx parse_constant(std::istream& istream, parser_internal flag)
{
    typename cmplx::value_type x, y;
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "{")
        throw ParsingException("Error - missing open '{' for complex "
                               "constant declaration");
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::floating &&
        curr_token.type != token_type::integer)
        throw ParsingException("Error - non-numeric data in constant "
                               "declaration");
    
    std::istringstream(curr_token.contents) >> x;
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Error - missing ',' separator in "
                               "constant declaration");
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::floating &&
        curr_token.type != token_type::integer)
        throw ParsingException("Error - non-numeric data in constant "
                               "declaration");
    
    std::istringstream(curr_token.contents) >> y;
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "}")
        throw ParsingException("Error - missing closing '}' in "
                               "constant declaration");
    return cmplx(x, y);
}

// These two function objects are instantiated to provide the two types
// of test functions we allow.
template <typename cmplx>
class ztestfun
{
private:
    const cmplx constant;
    const typename cmplx::value_type escape;
    const unsigned max_iters;
    const fn_parser::fn<cmplx> func;
public:
    ztestfun(const cmplx& c, const typename cmplx::value_type& e, unsigned m,
             const fn_parser::fn<cmplx>& f) : constant(c), escape(e), 
             max_iters(m), func(f) {}
    
    unsigned operator()(const cmplx& z)
    {
        unsigned iters = 0;
        cmplx test = z;
        while (abs(test) < escape && iters < max_iters) {
            test = func(test, constant);
            iters += 1;
        }
        return iters == max_iters ? 0 : iters;
    }
};

template <typename cmplx>
class ctestfun
{
private:
    const cmplx constant;
    const typename cmplx::value_type escape;
    const unsigned max_iters;
    const fn_parser::fn<cmplx> func;
public:
    ctestfun(const cmplx& c, const typename cmplx::value_type& e, unsigned m,
             const fn_parser::fn<cmplx>& f) : constant(c), escape(e), 
             max_iters(m), func(f) {}
    
    unsigned operator()(const cmplx& c)
    {
        unsigned iters = 0;
        cmplx test = constant;
        while (abs(test) < escape && iters < max_iters) {
            test = func(test, c);
            iters += 1;
        }
        return iters == max_iters ? 0 : iters;
    }
};

template <typename cmplx>
std::function<unsigned(const cmplx&)> parse_testfun(std::istream& istream)
{
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "{")
        throw ParsingException("Missing open '{' in function definition");
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::string)
        throw ParsingException("Expected string giving function definition");
    
    auto f = fn_parser::FunctionParser(curr_token.contents).get<cmplx>();
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Missing delimiting ',' in function definition");
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::keyword || 
        curr_token.contents != "max_iterations")
        throw ParsingException("Expected 'max_iterations' specification next");
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ":")
        throw ParsingException("Missing ':' delimiter after 'max_iterations'");
    unsigned maxiters = parse_integer(istream);
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Missing ',' delimiter");

    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::keyword ||
        curr_token.contents != "escape_tol")
        throw ParsingException("Expected 'escape_tol' specification next");
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ":")
        throw ParsingException("Missing ':' delimiter after 'escape_tol'");
    typename cmplx::value_type esc;
    istream >> esc;
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Missing ',' delimiter");

    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::keyword || 
        curr_token.contents != "constant")
        throw ParsingException("Expected 'constant' specification next");
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ":")
        throw ParsingException("Missing ':' delimiter after 'constant'");
    cmplx constant = parse_constant<cmplx>(istream, parser_internal());
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw ParsingException("Missing ',' delimiter");

    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::keyword || 
        curr_token.contents != "point")
        throw ParsingException("Expected 'point' specification next");
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ":")
        throw ParsingException("Missing ':' delimiter after 'point'");
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::keyword || 
        (curr_token.contents != "c" && curr_token.contents != "z"))
        throw ParsingException("Bad point specification - expect 'z' or 'c'");
    
    std::function<unsigned(const cmplx&)> testfun;
    if (curr_token.contents == "c")
        testfun = ctestfun<cmplx>(constant, esc, maxiters, f);
    else 
        testfun = ztestfun<cmplx>(constant, esc, maxiters, f);
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "}")
        throw ParsingException("Missing closing '}' in function definition");
    return testfun;
}

} /* namespace options */

} /* namespace fractals */
