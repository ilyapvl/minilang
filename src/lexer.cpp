#include "lexer.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>

namespace minilang
{

    using namespace grammar;
        
    Lexer::Lexer(std::string source)
        : m_source(std::move(source)), m_pos(0), m_line(1), m_column(1) {}

    char Lexer::peek() const
    {
        if (m_pos >= m_source.size()) return '\0';
        return m_source[m_pos];
    }

    char Lexer::advance()
    {
        char c = m_source[m_pos++];
        if (c == '\n')
        {
            ++m_line;
            m_column = 1;
        }
        else
        {
            ++m_column;
        }
        return c;
    }

    void Lexer::skipWhitespace()
    {
        while (isspace(peek())) advance();
    }

    Token Lexer::makeToken(Term type, std::string lexeme)
    {
        Token t;
        t.type = type;
        t.lexeme = std::move(lexeme);
        t.line = m_line;
        t.column = m_column - static_cast<int>(t.lexeme.length());
        return t;
    }

    Token Lexer::makeNumber(std::string lexeme)
    {
        Token t = makeToken(TERM_INTEGER, lexeme);
        t.intValue = std::stoi(lexeme);
        return t;
    }

    Token Lexer::makeIdentifier(std::string lexeme)
    {
        // Проверка на ключевые слова
        if (lexeme == "int")   return makeToken(TERM_INT, lexeme);
        if (lexeme == "bool")  return makeToken(TERM_BOOL, lexeme);
        if (lexeme == "if")    return makeToken(TERM_IF, lexeme);
        if (lexeme == "else")  return makeToken(TERM_ELSE, lexeme);
        if (lexeme == "while") return makeToken(TERM_WHILE, lexeme);
        if (lexeme == "true")  return makeToken(TERM_TRUE, lexeme);
        if (lexeme == "false") return makeToken(TERM_FALSE, lexeme);

        Token t = makeToken(TERM_IDENTIFIER, lexeme);
        t.identifier = lexeme;
        return t;
    }

    Token Lexer::errorToken(const std::string& message)
    {
        Token t;
        t.type = TERM_ERROR;
        t.lexeme = message;
        t.line = m_line;
        t.column = m_column;
        return t;
    }

    Token Lexer::getNextToken()
    {
        skipWhitespace();

        char c = peek();
        if (c == '\0')
            return makeToken(TERM_EOF, "");

        // Числа
        if (isdigit(c))
        {
            std::string num;
            while (isdigit(peek()))
                num += advance();
            return makeNumber(num);
        }

        // Идентификаторы и ключевые слова
        if (isalpha(c) || c == '_')
        {
            std::string ident;
            while (isalnum(peek()) || peek() == '_')
                ident += advance();
            return makeIdentifier(ident);
        }

        // Операторы и пунктуация
        switch (c)
        {
            case '+':
                advance();
                return makeToken(TERM_PLUS, "+");
            case '-':
                advance();
                return makeToken(TERM_MINUS, "-");
            case '*':
                advance();
                return makeToken(TERM_STAR, "*");
            case '/':
                advance();
                if (peek() == '/')
                {
                    while (peek() != '\n' && peek() != '\0')
                        advance();
                    return getNextToken();
                }
                else if (peek() == '*')
                {
                    advance();
                    while (true)
                    {
                        if (peek() == '\0')
                            return errorToken("Unterminated multi-line comment");
                        if (peek() == '*' && m_pos + 1 < m_source.size() && m_source[m_pos + 1] == '/')
                        {
                            advance();
                            advance();
                            break;
                        }
                        advance();
                    }
                    return getNextToken();
                }
                else
                {
                    return makeToken(TERM_SLASH, "/");
                }
            case '%':
                advance();
                return makeToken(TERM_PERCENT, "%");
            case '=':
                advance();
                if (peek() == '=')
                {
                    advance();
                    return makeToken(TERM_EQ, "==");
                }
                else
                {
                    return makeToken(TERM_ASSIGN, "=");
                }
            case '!':
                advance();
                if (peek() == '=')
                {
                    advance();
                    return makeToken(TERM_NE, "!=");
                }
                else
                {
                    return makeToken(TERM_NOT, "!");
                }
            case '<':
                advance();
                if (peek() == '=')
                {
                    advance();
                    return makeToken(TERM_LE, "<=");
                }
                else
                {
                    return makeToken(TERM_LT, "<");
                }
            case '>':
                advance();
                if (peek() == '=')
                {
                    advance();
                    return makeToken(TERM_GE, ">=");
                }
                else
                {
                    return makeToken(TERM_GT, ">");
                }
            case '&':
                advance();
                if (peek() == '&')
                {
                    advance();
                    return makeToken(TERM_AND, "&&");
                }
                else
                {
                    return errorToken("Unexpected character '&'");
                }
            case '|':
                advance();
                if (peek() == '|')
                {
                    advance();
                    return makeToken(TERM_OR, "||");
                }
                else
                {
                    return errorToken("Unexpected character '|'");
                }
            case ';':
                advance();
                return makeToken(TERM_SEMICOLON, ";");
            case '{':
                advance();
                return makeToken(TERM_LBRACE, "{");
            case '}':
                advance();
                return makeToken(TERM_RBRACE, "}");
            case '(':
                advance();
                return makeToken(TERM_LPAREN, "(");
            case ')':
                advance();
                return makeToken(TERM_RPAREN, ")");
            default:
            {
                std::string msg = "Unexpected character: ";
                msg += c;
                advance();
                return errorToken(msg);
            }
        }
    }

    std::vector<Token> Lexer::tokenize()
    {
        std::vector<Token> tokens;
        while (true)
        {
            Token tok = getNextToken();
            tokens.push_back(tok);
            if (tok.type == TERM_EOF || tok.type == TERM_ERROR)
                break;
        }
        return tokens;
    }

} // namespace minilang
