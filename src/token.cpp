#include "token.hpp"
#include <iostream>
#include <string>

namespace minilang
{

    using namespace grammar;

    Token::Token()
        : type(TERM_ERROR), line(0), column(0), intValue(0) {}

    static std::string termToString(Term t)
    {
        switch (t)
        {
            case TERM_EOF:          return "EOF";
            case TERM_INT:          return "int";
            case TERM_BOOL:         return "bool";
            case TERM_IF:           return "if";
            case TERM_ELSE:         return "else";
            case TERM_WHILE:        return "while";
            case TERM_TRUE:         return "true";
            case TERM_FALSE:        return "false";
            case TERM_IDENTIFIER:   return "identifier";
            case TERM_INTEGER:      return "integer";
            case TERM_PLUS:         return "+";
            case TERM_MINUS:        return "-";
            case TERM_STAR:         return "*";
            case TERM_SLASH:        return "/";
            case TERM_PERCENT:      return "%";
            case TERM_ASSIGN:       return "=";
            case TERM_EQ:           return "==";
            case TERM_NE:           return "!=";
            case TERM_LT:           return "<";
            case TERM_GT:           return ">";
            case TERM_LE:           return "<=";
            case TERM_GE:           return ">=";
            case TERM_AND:          return "&&";
            case TERM_OR:           return "||";
            case TERM_NOT:          return "!";
            case TERM_SEMICOLON:    return ";";
            case TERM_LBRACE:       return "{";
            case TERM_RBRACE:       return "}";
            case TERM_LPAREN:       return "(";
            case TERM_RPAREN:       return ")";
            case TERM_FUNC:         return "func";
            case TERM_RETURN:       return "return";
            case TERM_ARROW:        return "->";


            case TERM_ERROR:        return "error";
            default:                return "unknown";
        }
    }

    void printToken(const Token& tok)
    {
        std::cout << "[" << tok.line << ":" << tok.column << "] "
                << termToString(tok.type) << " '" << tok.lexeme << "'";
        if (tok.type == TERM_INTEGER) std::cout << " value=" << tok.intValue;
        if (tok.type == TERM_IDENTIFIER) std::cout << " name=" << tok.identifier;
        std::cout << std::endl;
    }

} // namespace minilang
