#ifndef MINILANG_AST_HPP
#define MINILANG_AST_HPP

#include <memory>
#include <string>
#include <vector>

namespace minilang
{

    enum class Type { INT, BOOL, ERROR };

    struct SymbolEntry;

    struct Position
    {
        int line;
        int column;
        Position(int l = 0, int c = 0) : line(l), column(c) {}
    };


    class Program;
    class Declaration;
    class VarDecl;
    class FuncDecl;
    class Assignment;
    class IfStmt;
    class WhileStmt;
    class Block;
    class BinaryOperation;
    class UnaryOperation;
    class IntegerLiteral;
    class BooleanLiteral;
    class Variable;
    class TypeInt;
    class TypeBool;
    class DeclList;
    class StmtList;
    class DeclOrStmtList;
    class NamespaceDecl;
    class QualifiedIdentifier;
    class CallExpr;
    class ReturnStmt;
    class ExpressionStmt;

    class Visitor
    {
    public:
        virtual ~Visitor() = default;
        virtual void visit(Program& node) = 0;
        virtual void visit(Assignment& node) = 0;

        virtual void visit(FuncDecl& node) = 0;
        virtual void visit(VarDecl& node) = 0;

        virtual void visit(IfStmt& node) = 0;
        virtual void visit(WhileStmt& node) = 0;
        virtual void visit(Block& node) = 0;

        virtual void visit(BinaryOperation& node) = 0;
        virtual void visit(UnaryOperation& node) = 0;

        virtual void visit(IntegerLiteral& node) = 0;
        virtual void visit(BooleanLiteral& node) = 0;
        virtual void visit(Variable& node) = 0;

        virtual void visit(TypeInt& node) = 0;
        virtual void visit(TypeBool& node) = 0;

        virtual void visit(DeclList& node) = 0;
        virtual void visit(StmtList& node) = 0;
        virtual void visit(DeclOrStmtList& node) = 0;

        virtual void visit(NamespaceDecl& node) = 0;
        virtual void visit(QualifiedIdentifier& node) = 0;

        virtual void visit(ReturnStmt& node) = 0;
        virtual void visit(CallExpr& node) = 0;
        virtual void visit(ExpressionStmt& node) = 0;
    };

    class Node
    {
    public:
        Position pos;
        virtual ~Node() = default;
        explicit Node(Position p) : pos(p) {}
        virtual void accept(Visitor& v) = 0;
    };

    class TypeNode : public Node
    {
    public:
        using Node::Node;
    };

    class TypeInt : public TypeNode
    {
    public:
        TypeInt(Position p) : TypeNode(p) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class TypeBool : public TypeNode
    {
    public:
        TypeBool(Position p) : TypeNode(p) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class Expression : public Node
    {
    public:
        Type exprType = Type::ERROR;
        using Node::Node;
    };

    class BinaryOperation : public Expression
    {
    public:
        enum class Op { PLUS, MINUS, MUL, DIV, MOD, AND, OR, EQ, NE, LT, GT, LE, GE };
        Op op;
        std::unique_ptr<Expression> left;
        std::unique_ptr<Expression> right;

        BinaryOperation(Op o, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r, Position p)
            : Expression(p), op(o), left(std::move(l)), right(std::move(r)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class UnaryOperation : public Expression
    {
    public:
        enum class Op { NEG, NOT };
        Op op;
        std::unique_ptr<Expression> operand;

        UnaryOperation(Op o, std::unique_ptr<Expression> e, Position p)
            : Expression(p), op(o), operand(std::move(e)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class IntegerLiteral : public Expression
    {
    public:
        int value;
        IntegerLiteral(int v, Position p) : Expression(p), value(v) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class BooleanLiteral : public Expression
    {
    public:
        bool value;
        BooleanLiteral(bool v, Position p) : Expression(p), value(v) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class Variable : public Expression
    {
    public:
        std::string name;
        SymbolEntry* symbol;

        Variable(std::string n, Position p) : Expression(p), name(std::move(n)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class Declaration : public Node
    {
    public:
        using Node::Node;
        virtual ~Declaration() = default;
    };

    class VarDecl : public Declaration
    {
    public:
        Type type;
        std::string name;
        std::unique_ptr<Expression> initializer;
        SymbolEntry* symbol;

        VarDecl(Type t, std::string n, std::unique_ptr<Expression> init, Position p)
            : Declaration(p), type(t), name(std::move(n)), initializer(std::move(init)), symbol(nullptr) {}

        void accept(Visitor& v) override { v.visit(*this); }
    };

    class FuncDecl : public Declaration
    {
    public:
        std::string name;
        Type returnType;
        std::unique_ptr<Block> body;
        SymbolEntry* symbol;

        FuncDecl(std::string n, Type ret, std::unique_ptr<Block> b, Position p)
            : Declaration(p), name(std::move(n)), returnType(ret), body(std::move(b)), symbol(nullptr) {}

        void accept(Visitor& v) override { v.visit(*this); }
    };

    class Statement : public Node
    {
    public:
        using Node::Node;
    };

    class Assignment : public Statement
    {
    public:
        std::unique_ptr<QualifiedIdentifier> target;
        std::unique_ptr<Expression> expr;

        Assignment(std::unique_ptr<QualifiedIdentifier> t, std::unique_ptr<Expression> e, Position p)
            : Statement(p), target(std::move(t)), expr(std::move(e)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class IfStmt : public Statement
    {
    public:
        std::unique_ptr<Expression> condition;
        std::unique_ptr<Statement> thenBranch;
        std::unique_ptr<Statement> elseBranch;

        IfStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> thenStmt,
            std::unique_ptr<Statement> elseStmt, Position p)
            : Statement(p), condition(std::move(cond)), thenBranch(std::move(thenStmt)),
            elseBranch(std::move(elseStmt)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class WhileStmt : public Statement
    {
    public:
        std::unique_ptr<Expression> condition;
        std::unique_ptr<Statement> body;

        WhileStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> b, Position p)
            : Statement(p), condition(std::move(cond)), body(std::move(b)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class NamespaceDecl : public Statement
    {
    public:
        std::string name;
        std::vector<std::unique_ptr<Declaration>> declarations;
        std::vector<std::unique_ptr<Statement>> statements;

        NamespaceDecl(std::string n,
                    std::vector<std::unique_ptr<Declaration>> decls,
                    std::vector<std::unique_ptr<Statement>> stmts,
                    Position p)
            : Statement(p), name(std::move(n)),
            declarations(std::move(decls)), statements(std::move(stmts)) {}

        void accept(Visitor& v) override { v.visit(*this); }
    };

    class ReturnStmt : public Statement
    {
    public:
        std::unique_ptr<Expression> value;

        ReturnStmt(std::unique_ptr<Expression> val, Position p)
            : Statement(p), value(std::move(val)) {}

        void accept(Visitor& v) override { v.visit(*this); }
    };

    class QualifiedIdentifier : public Expression
    {
    public:
        std::vector<std::string> names;
        SymbolEntry* symbol;

        QualifiedIdentifier(std::vector<std::string> n, Position p)
            : Expression(p), names(std::move(n)) {}

        void accept(Visitor& v) override { v.visit(*this); }

        
        std::string toString() const
        {
            std::string result;
            for (size_t i = 0; i < names.size(); ++i)
            {
                if (i > 0) result += "::";
                result += names[i];
            }
            return result;
        }
    };

    class Block : public Statement
    {
    public:
        std::vector<std::unique_ptr<Declaration>> declarations;
        std::vector<std::unique_ptr<Statement>> statements;

        Block(std::vector<std::unique_ptr<Declaration>> decls,
            std::vector<std::unique_ptr<Statement>> stmts, Position p)
            : Statement(p), declarations(std::move(decls)), statements(std::move(stmts)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class DeclList : public Node
    {
    public:
        std::vector<std::unique_ptr<Declaration>> declarations;
        DeclList(std::vector<std::unique_ptr<Declaration>> d, Position p)
            : Node(p), declarations(std::move(d)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class StmtList : public Node
    {
    public:
        std::vector<std::unique_ptr<Statement>> statements;
        StmtList(std::vector<std::unique_ptr<Statement>> s, Position p)
            : Node(p), statements(std::move(s)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class DeclOrStmtList : public Node
    {
    public:
        std::vector<std::unique_ptr<Declaration>> declarations;
        std::vector<std::unique_ptr<Statement>> statements;
        DeclOrStmtList(std::vector<std::unique_ptr<Declaration>> d,
                    std::vector<std::unique_ptr<Statement>> s, Position p)
            : Node(p), declarations(std::move(d)), statements(std::move(s)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

    class CallExpr : public Expression
    {
    public:
        std::unique_ptr<QualifiedIdentifier> callee;
        SymbolEntry* symbol;
        // std::vector<std::unique_ptr<Expression>> arguments;

        CallExpr(std::unique_ptr<QualifiedIdentifier> c, Position p)
            : Expression(p), callee(std::move(c)), symbol(nullptr) {}

        void accept(Visitor& v) override { v.visit(*this); }
    };

    class ExpressionStmt : public Statement
    {
    public:
        std::unique_ptr<Expression> expr;

        ExpressionStmt(std::unique_ptr<Expression> e, Position p)
            : Statement(p), expr(std::move(e)) {}

        void accept(Visitor& v) override { v.visit(*this); }
    };

    class Program : public Node
    {
    public:
        std::vector<std::unique_ptr<Declaration>> declarations;
        std::vector<std::unique_ptr<Statement>> statements;

        Program(std::vector<std::unique_ptr<Declaration>> decls,
                std::vector<std::unique_ptr<Statement>> stmts, Position p)
            : Node(p), declarations(std::move(decls)), statements(std::move(stmts)) {}
        void accept(Visitor& v) override { v.visit(*this); }
    };

} // namespace minilang

#endif // MINILANG_AST_HPP
