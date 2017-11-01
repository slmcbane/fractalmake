#include "options.hpp"

#include <cassert>
#include <cctype>

namespace fractals {
namespace options {

namespace {

void skip_to_next_line(std::istream& istream)
{
    int next = istream.get();
    assert(next == '#');
    while (next != '\n' && next != EOF) {
        next = istream.get();
    }
}

void skip_whitespace(std::istream& istream)
{
    while (true) {
        int next = istream.peek();
        if (next == '#') {
            skip_to_next_line(istream);
            continue;
        } else if (next == EOF || (next != ' ' && next != '\n' 
                                   && next != '\t' && next != '\r')) {
            break;
        }
        next = istream.get();
    }
}

Token get_numeric_token(std::istream& istream)
{
    bool got_scientific_specifier = false;
    bool got_decimal = false;
    token_type type = token_type::integer;
    std::string contents;
    
    int val = istream.get();
    contents.push_back(val);
    if (val == '-' || val == '.')
        type = token_type::floating;
    if (val == '.')
        got_decimal = true;
        
    val = istream.peek();
    while (isdigit(val) || val == 'e' || val == 'E' || val == '.' || val == '-'
           || val == '+') {
        if (val == '.') {
            if (got_decimal) throw "Bad number format - multiple decimal points";
            got_decimal = true;
            type = token_type::floating;
        } else if (val == 'E' || val == 'e') {
            if (got_scientific_specifier) throw "Bad number format - multiple "
                                                "occurrences of 'E'";
            got_scientific_specifier = true;
            type = token_type::floating;
        } else if (val == '-' || val == '+') {
            if (contents[contents.size() - 1] != 'E' && 
                contents[contents.size() - 1] != 'e')
                throw "Bad number format - sign somewhere besides the beginning "
                      "or immediately after 'E'";
        }
        contents.push_back(istream.get());
        val = istream.peek();
    }
    
    return Token{type, contents};
}

Token get_word(std::istream& istream)
{
    std::string word;
    int val = istream.peek();
    while (isalpha(val) || val == '_') {
        word.push_back(istream.get());
        val = istream.peek();
    }
    return Token{token_type::keyword, word};
}

Token get_string(std::istream& istream)
{
    std::string str;
    int val = istream.get();
    assert(val == '"');
    
    val = istream.peek();
    while (val != '"') {
        if (val == EOF)
            throw "Error - reached EOF while reading string.";
        str.push_back(istream.get());
        val = istream.peek();
    }
    val = istream.get();
    return Token{token_type::string, str};
}

Color parse_color(std::istream& istream)
{
    Color c; int i;
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "{")
        throw "Error - expected color specification to begin with '{'";
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::integer)
        throw "Error - non-integer value encountered in color specification";
    std::istringstream(curr_token.contents) >> i;
    if (i > 255 || i < 0) 
        throw "Error - color values must be in the range [0, 255]";
    c.r = i;
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw "Error - missing ',' separator in color specification.";
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::integer)
        throw "Error - non-integer value encountered in color specification";
    std::istringstream(curr_token.contents) >> i;
    if (i > 255 || i < 0) 
        throw "Error - color values must be in the range [0, 255]";
    c.g = i;
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw "Error - missing ',' separator in color specification.";
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::integer)
        throw "Error - non-integer value encountered in color specification";
    std::istringstream(curr_token.contents) >> i;
    if (i > 255 || i < 0) 
        throw "Error - color values must be in the range [0, 255]";
    c.b = i;
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "}")
        throw "Error - missing closing '}' in color specification";
        
    return c;
}

std::pair<unsigned, Color> parse_color_pair(std::istream& istream)
{
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "{")
        throw "Error - expected color pair to begin with '{'";
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::integer)
        throw "Error - expected integer as first part of color pair";
    
    std::pair<unsigned, Color> cpair;
    std::istringstream(curr_token.contents) >> cpair.first;
    
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != ",")
        throw "Error - missing comma separator in color pair";
    
    cpair.second = parse_color(istream);
    curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "}")
        throw "Error - missing closing '}' in color pair.";
    
    return cpair;
}

} /* end anon namespace */

Token get_next_token(std::istream& istream)
{
    skip_whitespace(istream);
    if (istream.peek() == EOF) {
        return Token{token_type::eof, ""};
    } else if (isdigit(istream.peek()) || istream.peek() == '+' || 
               istream.peek() == '-' || istream.peek() == '.') {
        return get_numeric_token(istream);
    } else if (isalpha(istream.peek()) || istream.peek() == '_') {
        return get_word(istream);
    } else if (istream.peek() == '"') {
        return get_string(istream);
    } else {
        std::string s;
        s.push_back(istream.get());
        return Token{token_type::symbol, s};
    }
}

std::vector<std::pair<unsigned, Color>> parse_colorlist(std::istream& istream)
{
    std::vector<std::pair<unsigned, Color>> colors;
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::symbol || curr_token.contents != "{")
        throw ParsingException("color list should begin with a '{'");
        
    while (true) {
        colors.push_back(parse_color_pair(istream));
        curr_token = get_next_token(istream);
        if (curr_token.type != token_type::symbol)
            throw ParsingException("Error - malformed color list");
            
        if (curr_token.contents != "}") {
            if (curr_token.contents != ",") {
                throw ParsingException("unexpected symbol in color list");
            }
        } else {
            break;
        }
    }
    return colors;
}

std::string parse_string(std::istream& istream)
{
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::string)
        throw "Error - output option given is not a string";
    return curr_token.contents;
}

unsigned parse_integer(std::istream& istream)
{
    Token curr_token = get_next_token(istream);
    if (curr_token.type != token_type::integer)
        throw "Error - got non-integer data where we expected an integer";
    unsigned n;
    std::istringstream(curr_token.contents) >> n;
    return n;
}

} /* namespace options */

} /* namespace fractals */
