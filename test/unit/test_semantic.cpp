#include <gtest/gtest.h>
#include "semantic.hpp"
#include "ast.hpp"
#include <memory>
#include <sstream>

using namespace minilang;

class SemanticTest : public ::testing::Test
{
protected:
    std::unique_ptr<Program> ast;
    SemanticAnalyzer analyzer;
    Position dummyPos{1, 1};

    void SetUp() override
    {
        ast = std::make_unique<Program>(
            std::vector<std::unique_ptr<Declaration>>(),
            std::vector<std::unique_ptr<Statement>>(),
            dummyPos
        );
    }


    std::unique_ptr<Variable> makeVar(const std::string& name)
    {
        return std::make_unique<Variable>(name, dummyPos);
    }


    std::unique_ptr<QualifiedIdentifier> makeQualId(const std::vector<std::string>& names)
    {
        return std::make_unique<QualifiedIdentifier>(names, dummyPos);
    }


    std::unique_ptr<IntegerLiteral> makeInt(int value)
    {
        return std::make_unique<IntegerLiteral>(value, dummyPos);
    }


    std::unique_ptr<BooleanLiteral> makeBool(bool value)
    {
        return std::make_unique<BooleanLiteral>(value, dummyPos);
    }


    bool analyze()
    {
        return analyzer.analyze(ast.get());
    }


    Type getExprType(Expression* expr)
    {
        expr->accept(analyzer);
        return expr->exprType;
    }
};

// func main() -> int { int x = 5; x = 10; return x; }
TEST_F(SemanticTest, CorrectProgram)
{
    
    std::vector<std::unique_ptr<VarDecl>> params;
    std::vector<std::unique_ptr<Declaration>> bodyDecls;
    bodyDecls.push_back(std::make_unique<VarDecl>(Type::INT, "x", makeInt(5), dummyPos));
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    bodyStmts.push_back(std::make_unique<Assignment>(makeQualId({"x"}), makeInt(10), dummyPos));
    bodyStmts.push_back(std::make_unique<ReturnStmt>(makeVar("x"), dummyPos));
    auto body = std::make_unique<Block>(std::move(bodyDecls), std::move(bodyStmts), dummyPos);
    auto func = std::make_unique<FuncDecl>("main", Type::INT, std::move(params), std::move(body), dummyPos);

    ast->declarations.push_back(std::move(func));

    bool ok = analyze();
    EXPECT_TRUE(ok);
    EXPECT_FALSE(analyzer.hasError());
}




TEST_F(SemanticTest, RedeclarationError)
{
    ast->declarations.push_back(std::make_unique<VarDecl>(Type::INT, "x", nullptr, dummyPos));
    ast->declarations.push_back(std::make_unique<VarDecl>(Type::INT, "x", nullptr, dummyPos));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}


TEST_F(SemanticTest, UndeclaredVariable)
{

    std::vector<std::unique_ptr<VarDecl>> params;
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    bodyStmts.push_back(std::make_unique<Assignment>(makeQualId({"x"}), makeInt(5), dummyPos));
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Declaration>>(), std::move(bodyStmts), dummyPos);
    auto func = std::make_unique<FuncDecl>("main", Type::INT, std::move(params), std::move(body), dummyPos);
    ast->declarations.push_back(std::move(func));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}



TEST_F(SemanticTest, TypeMismatchAssignment)
{
    std::vector<std::unique_ptr<VarDecl>> params;
    std::vector<std::unique_ptr<Declaration>> bodyDecls;
    bodyDecls.push_back(std::make_unique<VarDecl>(Type::INT, "x", nullptr, dummyPos));
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    bodyStmts.push_back(std::make_unique<Assignment>(makeQualId({"x"}), makeBool(true), dummyPos));
    auto body = std::make_unique<Block>(std::move(bodyDecls), std::move(bodyStmts), dummyPos);
    auto func = std::make_unique<FuncDecl>("main", Type::INT, std::move(params), std::move(body), dummyPos);
    ast->declarations.push_back(std::move(func));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}

// int x; x = true + 5;
TEST_F(SemanticTest, ArithmeticWithBool)
{
    
    std::vector<std::unique_ptr<VarDecl>> params;
    std::vector<std::unique_ptr<Declaration>> bodyDecls;
    bodyDecls.push_back(std::make_unique<VarDecl>(Type::INT, "x", nullptr, dummyPos));
    auto binOp = std::make_unique<BinaryOperation>(
        BinaryOperation::Op::PLUS,
        makeBool(true),
        makeInt(5),
        dummyPos
    );
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    bodyStmts.push_back(std::make_unique<Assignment>(makeQualId({"x"}), std::move(binOp), dummyPos));
    auto body = std::make_unique<Block>(std::move(bodyDecls), std::move(bodyStmts), dummyPos);
    auto func = std::make_unique<FuncDecl>("main", Type::INT, std::move(params), std::move(body), dummyPos);
    ast->declarations.push_back(std::move(func));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}

// if (5 == true) {}
TEST_F(SemanticTest, CompareIntBool)
{
    
    std::vector<std::unique_ptr<VarDecl>> params;
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    auto cond = std::make_unique<BinaryOperation>(
        BinaryOperation::Op::EQ,
        makeInt(5),
        makeBool(true),
        dummyPos
    );
    auto thenStmt = std::make_unique<Block>(
        std::vector<std::unique_ptr<Declaration>>(),
        std::vector<std::unique_ptr<Statement>>(),
        dummyPos
    );
    auto ifStmt = std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), nullptr, dummyPos);
    bodyStmts.push_back(std::move(ifStmt));
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Declaration>>(), std::move(bodyStmts), dummyPos);
    auto func = std::make_unique<FuncDecl>("main", Type::INT, std::move(params), std::move(body), dummyPos);
    ast->declarations.push_back(std::move(func));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}

// if (5 && true) {}
TEST_F(SemanticTest, LogicalWithInt)
{

    std::vector<std::unique_ptr<VarDecl>> params;
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    auto cond = std::make_unique<BinaryOperation>(
        BinaryOperation::Op::AND,
        makeInt(5),
        makeBool(true),
        dummyPos
    );
    auto thenStmt = std::make_unique<Block>(
        std::vector<std::unique_ptr<Declaration>>(),
        std::vector<std::unique_ptr<Statement>>(),
        dummyPos
    );
    auto ifStmt = std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), nullptr, dummyPos);
    bodyStmts.push_back(std::move(ifStmt));
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Declaration>>(), std::move(bodyStmts), dummyPos);
    auto func = std::make_unique<FuncDecl>("main", Type::INT, std::move(params), std::move(body), dummyPos);
    ast->declarations.push_back(std::move(func));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}

// func foo(a int) -> int { return a; }
// func main() -> int { return foo(1, 2); }
TEST_F(SemanticTest, FunctionCallArgCountMismatch)
{
    
    std::vector<std::unique_ptr<VarDecl>> fooParams;
    fooParams.push_back(std::make_unique<VarDecl>(Type::INT, "a", nullptr, dummyPos));
    auto fooBody = std::make_unique<Block>(
        std::vector<std::unique_ptr<Declaration>>(),
        std::vector<std::unique_ptr<Statement>>(),
        dummyPos
    );
    auto foo = std::make_unique<FuncDecl>("foo", Type::INT, std::move(fooParams), std::move(fooBody), dummyPos);

    std::vector<std::unique_ptr<VarDecl>> mainParams;
    std::vector<std::unique_ptr<Expression>> args;
    args.push_back(makeInt(1));
    args.push_back(makeInt(2));
    auto call = std::make_unique<CallExpr>(makeQualId({"foo"}), std::move(args), dummyPos);
    std::vector<std::unique_ptr<Statement>> mainStmts;
    mainStmts.push_back(std::make_unique<ReturnStmt>(std::move(call), dummyPos));
    auto mainBody = std::make_unique<Block>(
        std::vector<std::unique_ptr<Declaration>>(),
        std::move(mainStmts),
        dummyPos
    );
    auto main = std::make_unique<FuncDecl>("main", Type::INT, std::move(mainParams), std::move(mainBody), dummyPos);

    ast->declarations.push_back(std::move(foo));
    ast->declarations.push_back(std::move(main));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}

// func foo(a int) -> int { return a; }
// func main() -> int { return foo(true); }
TEST_F(SemanticTest, FunctionCallArgTypeMismatch)
{

    std::vector<std::unique_ptr<VarDecl>> fooParams;
    fooParams.push_back(std::make_unique<VarDecl>(Type::INT, "a", nullptr, dummyPos));
    auto fooBody = std::make_unique<Block>(
        std::vector<std::unique_ptr<Declaration>>(),
        std::vector<std::unique_ptr<Statement>>(),
        dummyPos
    );
    auto foo = std::make_unique<FuncDecl>("foo", Type::INT, std::move(fooParams), std::move(fooBody), dummyPos);

    std::vector<std::unique_ptr<VarDecl>> mainParams;
    std::vector<std::unique_ptr<Expression>> args;
    args.push_back(makeBool(true));
    auto call = std::make_unique<CallExpr>(makeQualId({"foo"}), std::move(args), dummyPos);
    std::vector<std::unique_ptr<Statement>> mainStmts;
    mainStmts.push_back(std::make_unique<ReturnStmt>(std::move(call), dummyPos));
    auto mainBody = std::make_unique<Block>(
        std::vector<std::unique_ptr<Declaration>>(),
        std::move(mainStmts),
        dummyPos
    );
    auto main = std::make_unique<FuncDecl>("main", Type::INT, std::move(mainParams), std::move(mainBody), dummyPos);

    ast->declarations.push_back(std::move(foo));
    ast->declarations.push_back(std::move(main));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}


// func main() -> int { return true; }
TEST_F(SemanticTest, ReturnTypeMismatch)
{

    std::vector<std::unique_ptr<VarDecl>> params;
    std::vector<std::unique_ptr<Statement>> bodyStmts;
    bodyStmts.push_back(std::make_unique<ReturnStmt>(makeBool(true), dummyPos));
    auto body = std::make_unique<Block>(std::vector<std::unique_ptr<Declaration>>(), std::move(bodyStmts), dummyPos);
    auto func = std::make_unique<FuncDecl>("main", Type::INT, std::move(params), std::move(body), dummyPos);
    ast->declarations.push_back(std::move(func));

    bool ok = analyze();
    EXPECT_FALSE(ok);
    EXPECT_TRUE(analyzer.hasError());
}
