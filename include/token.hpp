#ifndef MINILANG_TOKEN_HPP
#define MINILANG_TOKEN_HPP

#include "grammar.hpp"
#include <string>

namespace minilang
{

    struct Token
    {
        grammar::Term type;
        std::string lexeme;
        int line;
        int column;
        int intValue;
        std::string identifier;

        Token();
    };

    void printToken(const Token& tok);

} // namespace minilang

#endif // MINILANG_TOKEN_HPP
