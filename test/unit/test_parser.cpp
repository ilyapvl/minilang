#include <gtest/gtest.h>
#include "parser.hpp"
#include "grammar.hpp"
#include "token.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>

using namespace minilang;
using namespace grammar;


Token make_token(Term type, const std::string& lexeme, int line = 1, int col = 1)
{
    Token t;
    t.type = type;
    t.lexeme = lexeme;
    t.line = line;
    t.column = col;
    if (type == TERM_IDENTIFIER) t.identifier = lexeme;
    if (type == TERM_INTEGER) t.intValue = std::stoi(lexeme);
    return t;
}


class ParserTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g.buildMiniLang();
        g.computeFirst();
        g.computeFollow();
        g.prepareStatesLR1();
        g.buildLALR();
    }

    Grammar g;

    // AST is not empty
    void expectParseSuccess(const std::vector<Token>& tokens)
    {
        Parser parser(g, tokens);
        auto ast = parser.parse();
        EXPECT_NE(ast, nullptr);
    }


    void expectParseFailure(const std::vector<Token>& tokens)
    {
        Parser parser(g, tokens);
        auto ast = parser.parse();
        EXPECT_EQ(ast, nullptr);
    }
};


TEST_F(ParserTest, VarDeclNoInit)
{
    std::vector<Token> tokens = {
        make_token(TERM_INT, "int"),
        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto decl = dynamic_cast<VarDecl*>(ast->declarations[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->type, Type::INT);
    EXPECT_EQ(decl->name, "x");
    EXPECT_EQ(decl->initializer, nullptr);
}


TEST_F(ParserTest, VarDeclWithInit)
{
    std::vector<Token> tokens = {
        make_token(TERM_BOOL, "bool"),
        make_token(TERM_IDENTIFIER, "b"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_TRUE, "true"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto decl = dynamic_cast<VarDecl*>(ast->declarations[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->type, Type::BOOL);
    EXPECT_EQ(decl->name, "b");
    ASSERT_NE(decl->initializer, nullptr);
    auto lit = dynamic_cast<BooleanLiteral*>(decl->initializer.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, true);
}

TEST_F(ParserTest, Assignment)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "main"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),

        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_INTEGER, "42"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto assign = dynamic_cast<Assignment*>(func->body->statements[0].get());
    ASSERT_NE(assign, nullptr);
    ASSERT_NE(assign->target, nullptr);
    EXPECT_EQ(assign->target->names.size(), 1);
    EXPECT_EQ(assign->target->names[0], "x");
    ASSERT_NE(assign->expr, nullptr);
    auto lit = dynamic_cast<IntegerLiteral*>(assign->expr.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, 42);
}

// if (a) b = 1;
TEST_F(ParserTest, IfWithoutElse)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "main"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        make_token(TERM_IF, "if"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_IDENTIFIER, "a"),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_IDENTIFIER, "b"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_INTEGER, "1"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto ifstmt = dynamic_cast<IfStmt*>(func->body->statements[0].get());
    ASSERT_NE(ifstmt, nullptr);
    ASSERT_NE(ifstmt->condition, nullptr);

    
    auto condId = dynamic_cast<QualifiedIdentifier*>(ifstmt->condition.get());
    ASSERT_NE(condId, nullptr);
    EXPECT_EQ(condId->names.size(), 1);
    EXPECT_EQ(condId->names[0], "a");
    ASSERT_NE(ifstmt->thenBranch, nullptr);
    auto thenAssign = dynamic_cast<Assignment*>(ifstmt->thenBranch.get());
    ASSERT_NE(thenAssign, nullptr);
    EXPECT_EQ(thenAssign->target->names[0], "b");
    EXPECT_EQ(ifstmt->elseBranch, nullptr);
}

// if (a) b = 1; else c = 2;
TEST_F(ParserTest, IfWithElse)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "main"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        make_token(TERM_IF, "if"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_IDENTIFIER, "a"),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_IDENTIFIER, "b"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_INTEGER, "1"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_ELSE, "else"),
        make_token(TERM_IDENTIFIER, "c"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_INTEGER, "2"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto ifstmt = dynamic_cast<IfStmt*>(func->body->statements[0].get());
    ASSERT_NE(ifstmt, nullptr);
    // condition
    auto condId = dynamic_cast<QualifiedIdentifier*>(ifstmt->condition.get());
    ASSERT_NE(condId, nullptr);
    EXPECT_EQ(condId->names[0], "a");
    EXPECT_NE(ifstmt->thenBranch, nullptr);
    EXPECT_NE(ifstmt->elseBranch, nullptr);
    auto elseAssign = dynamic_cast<Assignment*>(ifstmt->elseBranch.get());
    ASSERT_NE(elseAssign, nullptr);
    EXPECT_EQ(elseAssign->target->names[0], "c");
}

// while (x < 10) x = x + 1;
TEST_F(ParserTest, WhileStmt)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "main"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        // function body
        make_token(TERM_WHILE, "while"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_LT, "<"),
        make_token(TERM_INTEGER, "10"),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_PLUS, "+"),
        make_token(TERM_INTEGER, "1"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto whileStmt = dynamic_cast<WhileStmt*>(func->body->statements[0].get());
    ASSERT_NE(whileStmt, nullptr);
    ASSERT_NE(whileStmt->condition, nullptr);
    auto binOp = dynamic_cast<BinaryOperation*>(whileStmt->condition.get());
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->op, BinaryOperation::Op::LT);
    ASSERT_NE(whileStmt->body, nullptr);
}

// { int x; x = 5; }
TEST_F(ParserTest, Block)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "main"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        // function body contains block
        make_token(TERM_LBRACE, "{"),
        make_token(TERM_INT, "int"),
        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_INTEGER, "5"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto block = dynamic_cast<Block*>(func->body->statements[0].get());
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(block->declarations.size(), 1);
    EXPECT_EQ(block->statements.size(), 1);
}

// func foo() -> int { return 0; }
TEST_F(ParserTest, FunctionNoParams)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "foo"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        make_token(TERM_RETURN, "return"),
        make_token(TERM_INTEGER, "0"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "foo");
    EXPECT_EQ(func->returnType, Type::INT);
    EXPECT_TRUE(func->parameters.empty());
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto ret = dynamic_cast<ReturnStmt*>(func->body->statements[0].get());
    ASSERT_NE(ret, nullptr);
    auto lit = dynamic_cast<IntegerLiteral*>(ret->value.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, 0);
}

// func add(a int, b int) -> int { return a + b; }
TEST_F(ParserTest, FunctionWithParams)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "add"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_INT, "int"),
        make_token(TERM_IDENTIFIER, "a"),
        make_token(TERM_COMMA, ","),
        make_token(TERM_INT, "int"),
        make_token(TERM_IDENTIFIER, "b"),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        make_token(TERM_RETURN, "return"),
        make_token(TERM_IDENTIFIER, "a"),
        make_token(TERM_PLUS, "+"),
        make_token(TERM_IDENTIFIER, "b"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "add");
    ASSERT_EQ(func->parameters.size(), 2);
    EXPECT_EQ(func->parameters[0]->name, "a");
    EXPECT_EQ(func->parameters[1]->name, "b");
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto ret = dynamic_cast<ReturnStmt*>(func->body->statements[0].get());
    ASSERT_NE(ret, nullptr);
    auto bin = dynamic_cast<BinaryOperation*>(ret->value.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, BinaryOperation::Op::PLUS);
}


TEST_F(ParserTest, CallExpr)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "main"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        // function body
        make_token(TERM_IDENTIFIER, "foo"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_INTEGER, "1"),
        make_token(TERM_COMMA, ","),
        make_token(TERM_TRUE, "true"),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto exprStmt = dynamic_cast<ExpressionStmt*>(func->body->statements[0].get());
    ASSERT_NE(exprStmt, nullptr);
    auto call = dynamic_cast<CallExpr*>(exprStmt->expr.get());
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->callee->names[0], "foo");
    ASSERT_EQ(call->arguments.size(), 2);
    EXPECT_NE(dynamic_cast<IntegerLiteral*>(call->arguments[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<BooleanLiteral*>(call->arguments[1].get()), nullptr);
}

// a::b::c = 0;
TEST_F(ParserTest, QualifiedIdentifier)
{
    std::vector<Token> tokens = {
        make_token(TERM_FUNC, "func"),
        make_token(TERM_IDENTIFIER, "main"),
        make_token(TERM_LPAREN, "("),
        make_token(TERM_RPAREN, ")"),
        make_token(TERM_ARROW, "->"),
        make_token(TERM_INT, "int"),
        make_token(TERM_LBRACE, "{"),
        // function body
        make_token(TERM_IDENTIFIER, "a"),
        make_token(TERM_SCOPE, "::"),
        make_token(TERM_IDENTIFIER, "b"),
        make_token(TERM_SCOPE, "::"),
        make_token(TERM_IDENTIFIER, "c"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_INTEGER, "0"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RBRACE, "}"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);
    auto func = dynamic_cast<FuncDecl*>(ast->declarations[0].get());
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->body, nullptr);
    ASSERT_EQ(func->body->statements.size(), 1);
    auto assign = dynamic_cast<Assignment*>(func->body->statements[0].get());
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->target->names, std::vector<std::string>({"a", "b", "c"}));
}

TEST_F(ParserTest, MissingSemicolon)
{
    std::vector<Token> tokens = {
        make_token(TERM_INT, "int"),
        make_token(TERM_IDENTIFIER, "x"),
        // missing TERM_SEMICOLON
        make_token(TERM_EOF, "")
    };
    expectParseFailure(tokens);
}

TEST_F(ParserTest, UnexpectedToken)
{
    std::vector<Token> tokens = {
        make_token(TERM_INT, "int"),
        make_token(TERM_IDENTIFIER, "x"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_RPAREN, ")"), // extra closing parenthesis
        make_token(TERM_EOF, "")
    };
    expectParseFailure(tokens);
}

TEST_F(ParserTest, MultipleDecls)
{
    std::vector<Token> tokens = {
        make_token(TERM_INT, "int"),
        make_token(TERM_IDENTIFIER, "a"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_BOOL, "bool"),
        make_token(TERM_IDENTIFIER, "b"),
        make_token(TERM_ASSIGN, "="),
        make_token(TERM_TRUE, "true"),
        make_token(TERM_SEMICOLON, ";"),
        make_token(TERM_EOF, "")
    };
    Parser parser(g, tokens);
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->declarations.size(), 2);
    EXPECT_EQ(ast->statements.size(), 0);
}
