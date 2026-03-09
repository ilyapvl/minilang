#include "token.hpp"
#include <iostream>

namespace minilang
{

Token::Token() : type(TokenType::ERROR), line(0), column(0), intValue(0) {}

void printToken(const Token& tok)
{
    std::cout << "[" << tok.line << ":" << tok.column << "] ";
    switch (tok.type)
    {
        case TokenType::INT:            std::cout << "INT        "; break;
        case TokenType::BOOL:           std::cout << "BOOL       "; break;
        case TokenType::IF:             std::cout << "IF         "; break;
        case TokenType::ELSE:           std::cout << "ELSE       "; break;
        case TokenType::WHILE:          std::cout << "WHILE      "; break;
        case TokenType::TRUE:           std::cout << "TRUE       "; break;
        case TokenType::FALSE:          std::cout << "FALSE      "; break;
        case TokenType::IDENTIFIER:     std::cout << "IDENTIFIER "; break;
        case TokenType::INTEGER:        std::cout << "INTEGER    "; break;
        case TokenType::PLUS:           std::cout << "PLUS       "; break;
        case TokenType::MINUS:          std::cout << "MINUS      "; break;
        case TokenType::STAR:           std::cout << "STAR       "; break;
        case TokenType::SLASH:          std::cout << "SLASH      "; break;
        case TokenType::PERCENT:        std::cout << "PERCENT    "; break;
        case TokenType::ASSIGN:         std::cout << "ASSIGN     "; break;
        case TokenType::EQ:             std::cout << "EQ         "; break;
        case TokenType::NE:             std::cout << "NE         "; break;
        case TokenType::LT:             std::cout << "LT         "; break;
        case TokenType::GT:             std::cout << "GT         "; break;
        case TokenType::LE:             std::cout << "LE         "; break;
        case TokenType::GE:             std::cout << "GE         "; break;
        case TokenType::AND:            std::cout << "AND        "; break;
        case TokenType::OR:             std::cout << "OR         "; break;
        case TokenType::NOT:            std::cout << "NOT        "; break;
        case TokenType::SEMICOLON:      std::cout << "SEMICOLON  "; break;
        case TokenType::LBRACE:         std::cout << "LBRACE     "; break;
        case TokenType::RBRACE:         std::cout << "RBRACE     "; break;
        case TokenType::LPAREN:         std::cout << "LPAREN     "; break;
        case TokenType::RPAREN:         std::cout << "RPAREN     "; break;
        case TokenType::END_OF_FILE:    std::cout << "EOF        "; break;
        case TokenType::ERROR:          std::cout << "ERROR      "; break;
    }
    std::cout << " '" << tok.lexeme << "'";

    if (tok.type == TokenType::INTEGER) std::cout << " value=" << tok.intValue;

    if (tok.type == TokenType::IDENTIFIER) std::cout << " name=" << tok.identifier;

    std::cout << std::endl;
}

} // namespace minilang
