#ifndef MINILANG_LEXER_HPP
#define MINILANG_LEXER_HPP

#include "token.hpp"
#include <string>
#include <vector>

namespace minilang
{

    class Lexer
    {
    public:
        explicit Lexer(std::string source);
        std::vector<Token> tokenize();

    private:
        std::string m_source;
        size_t m_pos;
        int m_line;
        int m_column;

        char peek() const;
        char advance();
        void skipWhitespace();
        Token makeToken(grammar::Term type, std::string lexeme);
        Token makeNumber(std::string lexeme);
        Token makeIdentifier(std::string lexeme);
        Token errorToken(const std::string& message);
        Token getNextToken();
    };

} // namespace minilang


#endif // MINILANG_LEXER_HPP
