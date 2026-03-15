#ifndef MINILANG_SEMANTIC_HPP
#define MINILANG_SEMANTIC_HPP

#include "ast.hpp"
#include "symbol.hpp"
#include <string>
#include <vector>

namespace minilang
{

    class SemanticAnalyzer : public Visitor
    {
    public:
        SemanticAnalyzer();
        bool analyze(Program* root);
        bool hasError() const { return m_hasError; }

        // Visitor implementation
        void visit(Program& node)           override;


        void visit(VarDecl& node)           override;
        void visit(FuncDecl& node)          override;

        void visit(Assignment& node)        override;
        void visit(IfStmt& node)            override;
        void visit(WhileStmt& node)         override;
        void visit(Block& node)             override;

        void visit(BinaryOperation& node)   override;
        void visit(UnaryOperation& node)    override;

        void visit(IntegerLiteral& node)    override;
        void visit(BooleanLiteral& node)    override;

        void visit(Variable& node)          override;
        void visit(TypeInt& node)           override;
        void visit(TypeBool& node)          override;

        void visit(DeclList& node)          override;
        void visit(StmtList& node)          override;
        void visit(DeclOrStmtList& node)    override;

        void visit(NamespaceDecl& node)         override;
        void visit(QualifiedIdentifier& node)   override;

        void visit(CallExpr& node)              override;
        void visit(ReturnStmt& node)            override;
        void visit(ExpressionStmt& node)        override;

        void visit(ParameterList& node)         override;
        void visit(ArgumentList& node)          override;





    private:
        SymbolTable m_globalTable;
        SymbolTable* m_currentTable;
        std::vector<SymbolTable*> m_tableStack;
        std::vector<FuncDecl*> m_functionStack;
        bool m_hasError;

        void error(const Position& pos, const std::string& message);
        Type checkBinaryOp(BinaryOperation& node);
        Type checkUnaryOp(UnaryOperation& node);
    };

} // namespace minilang

#endif // MINILANG_SEMANTIC_HPP
