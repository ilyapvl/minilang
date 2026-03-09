#include "lexer.hpp"
#include <cctype>
#include <stdexcept>

namespace minilang
{

Lexer::Lexer(std::string source) : m_source(std::move(source)), m_pos(0), m_line(1), m_column(1) {}

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
    while (isspace(peek()))
    {
        advance();
    }
}

Token Lexer::makeToken(TokenType type, std::string lexeme)
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
    Token t = makeToken(TokenType::INTEGER, lexeme);
    t.intValue = std::stoi(lexeme);
    return t;
}

Token Lexer::makeIdentifier(std::string lexeme)
{
    if (lexeme == "int")   return makeToken(TokenType::INT, lexeme);
    if (lexeme == "bool")  return makeToken(TokenType::BOOL, lexeme);
    if (lexeme == "if")    return makeToken(TokenType::IF, lexeme);
    if (lexeme == "else")  return makeToken(TokenType::ELSE, lexeme);
    if (lexeme == "while") return makeToken(TokenType::WHILE, lexeme);
    if (lexeme == "true")  return makeToken(TokenType::TRUE, lexeme);
    if (lexeme == "false") return makeToken(TokenType::FALSE, lexeme);

    Token t = makeToken(TokenType::IDENTIFIER, lexeme);
    t.identifier = lexeme;
    return t;
}

Token Lexer::errorToken(const std::string& message)
{
    Token t;
    t.type = TokenType::ERROR;
    t.lexeme = message;
    t.line = m_line;
    t.column = m_column;
    return t;
}

Token Lexer::getNextToken()
{
    skipWhitespace();

    char c = peek();
    if (c == '\0') return makeToken(TokenType::END_OF_FILE, "");

    if (isdigit(c))
    {
        std::string num;
        while (isdigit(peek()))
        {
            num += advance();
        }
        return makeNumber(num);
    }

    if (isalpha(c) || c == '_')
    {
        std::string ident;
        while (isalnum(peek()) || peek() == '_')
        {
            ident += advance();
        }
        return makeIdentifier(ident);
    }

    switch (c)
    {
        case '+':
            advance();
            return makeToken(TokenType::PLUS, "+");

        case '-':
            advance();
            return makeToken(TokenType::MINUS, "-");

        case '*':
            advance();
            return makeToken(TokenType::STAR, "*");

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
                    {
                        return errorToken("Unterminated multi-line comment");
                    }
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
                return makeToken(TokenType::SLASH, "/");
            }

        case '%':
            advance();
            return makeToken(TokenType::PERCENT, "%");

        case '=':
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::EQ, "==");
            }
            else
            {
                return makeToken(TokenType::ASSIGN, "=");
            }

        case '!':
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::NE, "!=");
            }
            else
            {
                return makeToken(TokenType::NOT, "!");
            }

        case '<':
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::LE, "<=");
            }
            else
            {
                return makeToken(TokenType::LT, "<");
            }

        case '>':
            advance();
            if (peek() == '=')
            {
                advance();
                return makeToken(TokenType::GE, ">=");
            }
            else
            {
                return makeToken(TokenType::GT, ">");
            }

        case '&':
            advance();
            if (peek() == '&')
            {
                advance();
                return makeToken(TokenType::AND, "&&");
            }
            else
            {
                return errorToken("Unexpected character '&' (did you mean '&&'?)");
            }

        case '|':
            advance();
            if (peek() == '|')
            {
                advance();
                return makeToken(TokenType::OR, "||");
            }
            else
            {
                return errorToken("Unexpected character '|' (did you mean '||'?)");
            }

        case ';':
            advance();
            return makeToken(TokenType::SEMICOLON, ";");

        case '{':
            advance();
            return makeToken(TokenType::LBRACE, "{");

        case '}':
            advance();
            return makeToken(TokenType::RBRACE, "}");

        case '(':
            advance();
            return makeToken(TokenType::LPAREN, "(");

        case ')':
            advance();
            return makeToken(TokenType::RPAREN, ")");

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
        if (tok.type == TokenType::END_OF_FILE)
        {
            break;
        }
    }
    return tokens;
}

} // namespace minilang
