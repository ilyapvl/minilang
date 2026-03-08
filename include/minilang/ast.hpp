#ifndef MINILANG_AST_HPP
#define MINILANG_AST_HPP

#include <memory>
#include <string>
#include <vector>

namespace minilang
{

struct Position
{
    int line;
    int column;

    Position(int l = 0, int c = 0) : line(l), column(c) {}
};


class Node
{
public:
    Position pos;

    virtual ~Node() = default;

    explicit Node(Position p) : pos(p) {}
};


class Expression : public Node
{
public:
    using Node::Node;
};


class BinaryOperation : public Expression
{
public:
    enum class Op
    {
        PLUS,   // +
        MINUS,  // -
        MUL,    // *
        DIV,    // /
        MOD,    // %
        AND,    // &&
        OR,     // ||
        EQ,     // ==
        NE,     // !=
        LT,     // <
        GT,     // >
        LE,     // <=
        GE      // >=
    };

    Op op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryOperation(
        Op o,
        std::unique_ptr<Expression> l,
        std::unique_ptr<Expression> r,
        Position p) : Expression(p), op(o), left(std::move(l)), right(std::move(r)) {}
};


class UnaryOperation : public Expression
{
public:
    enum class Op
    {
        NEG, 
        NOT    
    };

    Op op;
    std::unique_ptr<Expression> operand;

    UnaryOperation(
        Op o,
        std::unique_ptr<Expression> e,
        Position p
    ) : Expression(p), op(o), operand(std::move(e)) {}
};

class IntegerLiteral : public Expression
{
public:
    int value;

    IntegerLiteral(int v, Position p) : Expression(p), value(v) {}
};

class BooleanLiteral : public Expression
{
public:
    bool value;

    BooleanLiteral(bool v, Position p): Expression(p), value(v) {}
};


class Variable : public Expression
{
public:
    std::string name;

    Variable(std::string n, Position p) : Expression(p), name(std::move(n)) {}
};

class Declaration : public Node
{
public:
    enum class Type
    {
        INT,
        BOOL
    };

    Type type;
    std::string name;
    std::unique_ptr<Expression> initializer;  

    Declaration(
        Type t,
        std::string n,
        std::unique_ptr<Expression> init,
        Position p
    ) : Node(p), type(t), name(std::move(n)), initializer(std::move(init)) {}
};

class Statement : public Node
{
public:
    using Node::Node;
};

class Assignment : public Statement
{
public:
    std::string varName;
    std::unique_ptr<Expression> expr;

    Assignment(
        std::string var,
        std::unique_ptr<Expression> e,
        Position p
    ) : Statement(p), varName(std::move(var)), expr(std::move(e)) {}
};

class IfStmt : public Statement
{
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch; 

    IfStmt(
        std::unique_ptr<Expression> cond,
        std::unique_ptr<Statement> thenStmt,
        std::unique_ptr<Statement> elseStmt,
        Position p
    ) : Statement(p), condition(std::move(cond)), thenBranch(std::move(thenStmt)), elseBranch(std::move(elseStmt)) {}
};


class WhileStmt : public Statement
{
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    WhileStmt(
        std::unique_ptr<Expression> cond,
        std::unique_ptr<Statement> b,
        Position p
    ) : Statement(p), condition(std::move(cond)), body(std::move(b)) {}
};

class Block : public Statement
{
public:
    std::vector<std::unique_ptr<Declaration>> declarations;
    std::vector<std::unique_ptr<Statement>> statements;

    Block(
        std::vector<std::unique_ptr<Declaration>> decls,
        std::vector<std::unique_ptr<Statement>> stmts,
        Position p
    ) : Statement(p), declarations(std::move(decls)), statements(std::move(stmts)) {}
};


class Program : public Node
{
public:
    std::vector<std::unique_ptr<Declaration>> declarations;
    std::vector<std::unique_ptr<Statement>> statements;

    Program(
        std::vector<std::unique_ptr<Declaration>> decls,
        std::vector<std::unique_ptr<Statement>> stmts,
        Position p
    ) : Node(p), declarations(std::move(decls)), statements(std::move(stmts)) {}
};

} // namespace minilang

#endif // MINILANG_AST_HPP
