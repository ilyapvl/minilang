#ifndef MINILANG_TOKEN_HPP
#define MINILANG_TOKEN_HPP

#include <string>

namespace minilang
{

enum class TokenType
{

    INT, BOOL, IF, ELSE, WHILE, TRUE, FALSE,

    IDENTIFIER,
 
    INTEGER,
  
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN,
    EQ, NE, LT, GT, LE, GE,
    AND, OR, NOT,

    SEMICOLON, LBRACE, RBRACE, LPAREN, RPAREN,
 
    END_OF_FILE, ERROR
};



struct Token
{
    TokenType type;
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
