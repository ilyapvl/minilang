#ifndef MINILANG_IRGEN_HPP
#define MINILANG_IRGEN_HPP

#include "ast.hpp"
#include "symbol.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include <unordered_map>
#include <vector>

namespace minilang
{

    class IRGenerator : public Visitor
    {
    public:
        IRGenerator(llvm::LLVMContext& context, const std::string& moduleName);

        llvm::Module* getModule() { return m_module; }

        bool generate(Program* ast);
        void printModule();

        // Visitor implementation
        void visit(Program& node)               override;
        void visit(Assignment& node)            override;
        void visit(IfStmt& node)                override;
        void visit(WhileStmt& node)             override;
        void visit(Block& node)                 override;
        void visit(BinaryOperation& node)       override;
        void visit(UnaryOperation& node)        override;
        void visit(IntegerLiteral& node)        override;
        void visit(BooleanLiteral& node)        override;
        void visit(Variable& node)              override;
        void visit(TypeInt& node)               override;
        void visit(TypeBool& node)              override;
        void visit(DeclList& node)              override;
        void visit(StmtList& node)              override;
        void visit(DeclOrStmtList& node)        override;
        void visit(NamespaceDecl& node)         override;
        void visit(QualifiedIdentifier& node)   override;

        void visit(VarDecl& node)               override;
        void visit(FuncDecl& node)              override;
        void visit(CallExpr& node)              override;
        void visit(ReturnStmt& node)            override;

    private:
        bool m_collecting;

        llvm::LLVMContext& m_context;
        llvm::Module* m_module;
        llvm::IRBuilder<> m_builder;
        std::unordered_map<SymbolEntry*, llvm::AllocaInst*> m_allocaMap;
        std::vector<llvm::Value*> m_valueStack;

        llvm::BasicBlock* m_entryBlock;

        llvm::AllocaInst* createAllocaInEntry(llvm::Type* type, const std::string& name);

        llvm::Function* m_mainFunction;

        void pushValue(llvm::Value* v) { m_valueStack.push_back(v); }
        llvm::Value* popValue()
        {
            if (m_valueStack.empty()) return nullptr;
            llvm::Value* v = m_valueStack.back();
            m_valueStack.pop_back();
            return v;
        }
    };

} // namespace minilang

#endif // MINILANG_IRGEN_HPP
