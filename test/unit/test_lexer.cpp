#include <gtest/gtest.h>
#include "lexer.hpp"
#include "token.hpp"
#include "grammar.hpp"

using namespace minilang;
using namespace grammar;

void checkToken(const Token& tok, Term expectedType,
                const std::string& expectedLexeme,
                int expectedLine, int expectedColumn)
{
    EXPECT_EQ(tok.type, expectedType);
    EXPECT_EQ(tok.lexeme, expectedLexeme);
    EXPECT_EQ(tok.line, expectedLine);
    EXPECT_EQ(tok.column, expectedColumn);
}

TEST(LexerTest, EmptyInput)
{
    Lexer lexer("");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TERM_EOF);
}

TEST(LexerTest, WhitespaceOnly)
{
    Lexer lexer("   \n\t  ");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TERM_EOF);
}

TEST(LexerTest, Keywords)
{
    Lexer lexer("int bool if else while true false func return");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 10);
    checkToken(tokens[0], TERM_INT, "int", 1, 1);
    checkToken(tokens[1], TERM_BOOL, "bool", 1, 5);
    checkToken(tokens[2], TERM_IF, "if", 1, 10);
    checkToken(tokens[3], TERM_ELSE, "else", 1, 13);
    checkToken(tokens[4], TERM_WHILE, "while", 1, 18);
    checkToken(tokens[5], TERM_TRUE, "true", 1, 24);
    checkToken(tokens[6], TERM_FALSE, "false", 1, 29);
    checkToken(tokens[7], TERM_FUNC, "func", 1, 35);
    checkToken(tokens[8], TERM_RETURN, "return", 1, 40);
}

TEST(LexerTest, OperatorsAndPunctuation)
{
    Lexer lexer("+ - * / % = == != < > <= >= && || ! ; { } ( ) @ :: -> ,");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 25);

    checkToken(tokens[0], TERM_PLUS, "+", 1, 1);
    checkToken(tokens[1], TERM_MINUS, "-", 1, 3);
    checkToken(tokens[2], TERM_STAR, "*", 1, 5);
    checkToken(tokens[3], TERM_SLASH, "/", 1, 7);
    checkToken(tokens[4], TERM_PERCENT, "%", 1, 9);
    checkToken(tokens[5], TERM_ASSIGN, "=", 1, 11);
    checkToken(tokens[6], TERM_EQ, "==", 1, 13);
    checkToken(tokens[7], TERM_NE, "!=", 1, 16);
    checkToken(tokens[8], TERM_LT, "<", 1, 19);
    checkToken(tokens[9], TERM_GT, ">", 1, 21);
    checkToken(tokens[10], TERM_LE, "<=", 1, 23);
    checkToken(tokens[11], TERM_GE, ">=", 1, 26);
    checkToken(tokens[12], TERM_AND, "&&", 1, 29);
    checkToken(tokens[13], TERM_OR, "||", 1, 32);
    checkToken(tokens[14], TERM_NOT, "!", 1, 35);
    checkToken(tokens[15], TERM_SEMICOLON, ";", 1, 37);
    checkToken(tokens[16], TERM_LBRACE, "{", 1, 39);
    checkToken(tokens[17], TERM_RBRACE, "}", 1, 41);
    checkToken(tokens[18], TERM_LPAREN, "(", 1, 43);
    checkToken(tokens[19], TERM_RPAREN, ")", 1, 45);
    checkToken(tokens[20], TERM_AT, "@", 1, 47);
    checkToken(tokens[21], TERM_SCOPE, "::", 1, 49);
    checkToken(tokens[22], TERM_ARROW, "->", 1, 52);
    checkToken(tokens[23], TERM_COMMA, ",", 1, 55);
}

TEST(LexerTest, Identifiers)
{
    Lexer lexer("abc def123 _qwe");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    checkToken(tokens[0], TERM_IDENTIFIER, "abc", 1, 1);
    EXPECT_EQ(tokens[0].identifier, "abc");
    checkToken(tokens[1], TERM_IDENTIFIER, "def123", 1, 5);
    EXPECT_EQ(tokens[1].identifier, "def123");
    checkToken(tokens[2], TERM_IDENTIFIER, "_qwe", 1, 12);
    EXPECT_EQ(tokens[2].identifier, "_qwe");
}

TEST(LexerTest, IntegerLiterals)
{
    Lexer lexer("11 0 12345");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    checkToken(tokens[0], TERM_INTEGER, "11", 1, 1);
    EXPECT_EQ(tokens[0].intValue, 11);
    checkToken(tokens[1], TERM_INTEGER, "0", 1, 4);
    EXPECT_EQ(tokens[1].intValue, 0);
    checkToken(tokens[2], TERM_INTEGER, "12345", 1, 6);
    EXPECT_EQ(tokens[2].intValue, 12345);
}

TEST(LexerTest, MixedTokens)
{
    Lexer lexer("if (x <= 10) { y = 2 * x; }");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 15);
    EXPECT_EQ(tokens[0].type, TERM_IF);
    EXPECT_EQ(tokens[1].type, TERM_LPAREN);
    EXPECT_EQ(tokens[2].type, TERM_IDENTIFIER);
    EXPECT_EQ(tokens[2].identifier, "x");
    EXPECT_EQ(tokens[3].type, TERM_LE);
    EXPECT_EQ(tokens[4].type, TERM_INTEGER);
    EXPECT_EQ(tokens[4].intValue, 10);
    EXPECT_EQ(tokens[5].type, TERM_RPAREN);
    EXPECT_EQ(tokens[6].type, TERM_LBRACE);
    EXPECT_EQ(tokens[7].type, TERM_IDENTIFIER);
    EXPECT_EQ(tokens[7].identifier, "y");
    EXPECT_EQ(tokens[8].type, TERM_ASSIGN);
    EXPECT_EQ(tokens[9].type, TERM_INTEGER);
    EXPECT_EQ(tokens[9].intValue, 2);
    EXPECT_EQ(tokens[10].type, TERM_STAR);
    EXPECT_EQ(tokens[11].type, TERM_IDENTIFIER);
    EXPECT_EQ(tokens[11].identifier, "x");
    EXPECT_EQ(tokens[12].type, TERM_SEMICOLON);
    EXPECT_EQ(tokens[13].type, TERM_RBRACE);
}

TEST(LexerTest, SingleLineComment)
{
    Lexer lexer("int x; // comment\nreturn x;");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TERM_INT);
    EXPECT_EQ(tokens[1].type, TERM_IDENTIFIER);
    EXPECT_EQ(tokens[1].identifier, "x");
    EXPECT_EQ(tokens[2].type, TERM_SEMICOLON);
    EXPECT_EQ(tokens[3].type, TERM_RETURN);
    EXPECT_EQ(tokens[4].type, TERM_IDENTIFIER);
    EXPECT_EQ(tokens[4].identifier, "x");
    EXPECT_EQ(tokens[5].type, TERM_SEMICOLON);
}

TEST(LexerTest, MultiLineComment)
{
    Lexer lexer("int /* some\nwords */ x;");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TERM_INT);
    EXPECT_EQ(tokens[1].type, TERM_IDENTIFIER);
    EXPECT_EQ(tokens[1].identifier, "x");
    EXPECT_EQ(tokens[2].type, TERM_SEMICOLON);
}

TEST(LexerTest, UnterminatedComment)
{
    Lexer lexer("int /* comment not closed");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[1].type, TERM_ERROR);
    EXPECT_EQ(tokens[1].lexeme, "Unterminated multi-line comment");
}

TEST(LexerTest, InvalidCharacter)
{
    Lexer lexer("&");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type, TERM_ERROR);
    EXPECT_EQ(tokens[0].lexeme, "Unexpected character '&'");
}

TEST(LexerTest, InvalidColon)
{
    Lexer lexer(":");
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type, TERM_ERROR);
    EXPECT_EQ(tokens[0].lexeme, "Unexpected character ':' (maybe you meant '::')");
}

TEST(LexerTest, Positions)
{
    Lexer lexer("int\n  x = 1;");
    auto tokens = lexer.tokenize();

    ASSERT_GE(tokens.size(), 5);
    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);
    EXPECT_EQ(tokens[1].line, 2);
    EXPECT_EQ(tokens[1].column, 3);
    EXPECT_EQ(tokens[2].line, 2);
    EXPECT_EQ(tokens[2].column, 5);
    EXPECT_EQ(tokens[3].line, 2);
    EXPECT_EQ(tokens[3].column, 7);
    EXPECT_EQ(tokens[4].line, 2);
    EXPECT_EQ(tokens[4].column, 8);
}
