#include "irgen.hpp"
#include <iostream>
#include <cassert>

namespace minilang
{

    IRGenerator::IRGenerator(llvm::LLVMContext& context, const std::string& moduleName)
        : m_context(context)
        , m_builder(context)
        , m_mainFunction(nullptr)
        , m_entryBlock(nullptr)
        , m_collecting(false)
    {
        m_module = new llvm::Module(moduleName, context);
    }

 
    llvm::AllocaInst* IRGenerator::createAllocaInEntry(llvm::Type* type, const std::string& name)
    {

        llvm::IRBuilder<>::InsertPoint savedIP = m_builder.saveIP();

        m_builder.SetInsertPoint(m_entryBlock);

        auto alloca = m_builder.CreateAlloca(type, nullptr, name);

        m_builder.restoreIP(savedIP);
        return alloca;
    }

    bool IRGenerator::generate(Program* ast)
    {
        // main function: i32()
        llvm::FunctionType* mainType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(m_context), false);
        m_mainFunction = llvm::Function::Create(
            mainType, llvm::Function::ExternalLinkage, "main", m_module);

        llvm::BasicBlock* entry = llvm::BasicBlock::Create(m_context, "entry", m_mainFunction);
        m_builder.SetInsertPoint(entry);
        m_entryBlock = entry;

        // first pass: collect declarations and create allocas
        m_collecting = true;
        ast->accept(*this);
        m_collecting = false;

        m_builder.SetInsertPoint(entry);

        // second pass: generate IR
        ast->accept(*this);

        // add ret 0 if current block is not terminated
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), 0));
        }

        // verification
        bool hasError = llvm::verifyFunction(*m_mainFunction, &llvm::errs());
        if (hasError)
        {
            std::cerr << "Function verification failed" << std::endl;
            return false;
        }
        
        hasError = llvm::verifyModule(*m_module, &llvm::errs());
        if (hasError)
        {
            std::cerr << "Module verification failed" << std::endl;
            return false;
        }

        return true;
    }

    void IRGenerator::printModule()
    {
        m_module->print(llvm::outs(), nullptr);
    }

    // Visitor implementation

    void IRGenerator::visit(Program& node)
    {
        if (m_collecting)
        {
            for (auto& decl : node.declarations) decl->accept(*this);
            for (auto& stmt : node.statements) stmt->accept(*this);
            return;
        }

        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(Declaration& node)
    {
        if (m_collecting)
        {

            llvm::Type* llvmType = nullptr;
            if (node.type == Declaration::Type::INT)
            {
                llvmType = llvm::Type::getInt32Ty(m_context);
            }
            else if (node.type == Declaration::Type::BOOL)
            {
                llvmType = llvm::Type::getInt1Ty(m_context);
            }
            else
            {
                std::cerr << "Unknown declaration type" << std::endl;
                return;
            }

            
            llvm::AllocaInst* alloca = createAllocaInEntry(llvmType, node.name);


            if (node.symbol)
            {
                m_allocaMap[node.symbol] = alloca;
            }
            else
            {
                std::cerr << "Declaration without symbol entry" << std::endl;
            }
            return;
        }

        // second pass for initialization
        if (node.initializer)
        {
            node.initializer->accept(*this);
            llvm::Value* initVal = popValue();
            if (initVal)
            {
                auto it = m_allocaMap.find(node.symbol);
                if (it != m_allocaMap.end())
                {
                    m_builder.CreateStore(initVal, it->second);
                }
            }
        }
    }

    void IRGenerator::visit(Assignment& node)
    {
        if (m_collecting)
        {
            if (node.target) node.target->accept(*this);
            if (node.expr) node.expr->accept(*this);
            return;
        }

        node.expr->accept(*this);
        llvm::Value* val = popValue();
        if (!val) return;

        auto it = m_allocaMap.find(node.target->symbol);
        if (it == m_allocaMap.end())
        {
            std::cerr << "Assignment to unknown variable: " << node.target->toString() << std::endl;
            return;
        }
        m_builder.CreateStore(val, it->second);
    }

    void IRGenerator::visit(IfStmt& node)
    {
        if (m_collecting)
        {
            if (node.condition) node.condition->accept(*this);
            if (node.thenBranch) node.thenBranch->accept(*this);
            if (node.elseBranch) node.elseBranch->accept(*this);
            return;
        }

        node.condition->accept(*this);
        llvm::Value* cond = popValue();
        if (!cond) return;

        llvm::Function* func = m_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(m_context, "then", func);
        llvm::BasicBlock* elseBB = nullptr;
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(m_context, "ifmerge", func);

        if (node.elseBranch)
        {
            elseBB = llvm::BasicBlock::Create(m_context, "else", func);
            m_builder.CreateCondBr(cond, thenBB, elseBB);
        }
        else
        {
            m_builder.CreateCondBr(cond, thenBB, mergeBB);
        }

        m_builder.SetInsertPoint(thenBB);
        node.thenBranch->accept(*this);
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(mergeBB);
        }

        if (elseBB)
        {
            m_builder.SetInsertPoint(elseBB);
            node.elseBranch->accept(*this);
            if (!m_builder.GetInsertBlock()->getTerminator())
            {
                m_builder.CreateBr(mergeBB);
            }
        }

        m_builder.SetInsertPoint(mergeBB);
    }

    void IRGenerator::visit(WhileStmt& node)
    {
        if (m_collecting)
        {
            if (node.condition) node.condition->accept(*this);
            if (node.body) node.body->accept(*this);
            return;
        }

        llvm::Function* func = m_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* condBB = llvm::BasicBlock::Create(m_context, "whilecond", func);
        llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(m_context, "whilebody", func);
        llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(m_context, "whileafter", func);

        m_builder.CreateBr(condBB);

        m_builder.SetInsertPoint(condBB);
        node.condition->accept(*this);
        llvm::Value* cond = popValue();
        if (!cond) cond = llvm::ConstantInt::getBool(m_context, false);
        m_builder.CreateCondBr(cond, bodyBB, afterBB);

        m_builder.SetInsertPoint(bodyBB);
        node.body->accept(*this);
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(condBB);
        }

        m_builder.SetInsertPoint(afterBB);
    }

    void IRGenerator::visit(Block& node)
    {
        if (m_collecting)
        {
            for (auto& decl : node.declarations) decl->accept(*this);
            for (auto& stmt : node.statements) stmt->accept(*this);
            return;
        }

        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(BinaryOperation& node)
    {
        if (m_collecting)
        {
            if (node.left) node.left->accept(*this);
            if (node.right) node.right->accept(*this);
            return;
        }
        using Op = BinaryOperation::Op;


        if (node.op == Op::AND || node.op == Op::OR)
        {
            node.left->accept(*this);
            llvm::Value* leftVal = popValue();
            node.right->accept(*this);
            llvm::Value* rightVal = popValue();
            if (!leftVal || !rightVal)
            {
                pushValue(llvm::ConstantInt::getBool(m_context, false));
                return;
            }
            llvm::Value* result = nullptr;
            if (node.op == Op::AND)
            {
                result = m_builder.CreateAnd(leftVal, rightVal, "andtmp");
            }
            else
            {
                result = m_builder.CreateOr(leftVal, rightVal, "ortmp");
            }
            pushValue(result);
            return;
        }

        // binary operations
        node.left->accept(*this);
        node.right->accept(*this);

        llvm::Value* rightVal = popValue();
        llvm::Value* leftVal = popValue();

        if (!leftVal || !rightVal)
        {
            pushValue(nullptr);
            return;
        }

        llvm::Value* result = nullptr;

        switch (node.op)
        {
            case Op::PLUS:
                result = m_builder.CreateAdd(leftVal, rightVal, "addtmp");
                break;
            case Op::MINUS:
                result = m_builder.CreateSub(leftVal, rightVal, "subtmp");
                break;
            case Op::MUL:
                result = m_builder.CreateMul(leftVal, rightVal, "multmp");
                break;
            case Op::DIV:
                result = m_builder.CreateSDiv(leftVal, rightVal, "divtmp");
                break;
            case Op::MOD:
                result = m_builder.CreateSRem(leftVal, rightVal, "modtmp");
                break;
            case Op::EQ:
                result = m_builder.CreateICmpEQ(leftVal, rightVal, "eqtmp");
                break;
            case Op::NE:
                result = m_builder.CreateICmpNE(leftVal, rightVal, "netmp");
                break;
            case Op::LT:
                result = m_builder.CreateICmpSLT(leftVal, rightVal, "lttmp");
                break;
            case Op::GT:
                result = m_builder.CreateICmpSGT(leftVal, rightVal, "gttmp");
                break;
            case Op::LE:
                result = m_builder.CreateICmpSLE(leftVal, rightVal, "letmp");
                break;
            case Op::GE:
                result = m_builder.CreateICmpSGE(leftVal, rightVal, "getmp");
                break;
            default:
                break;
        }

        pushValue(result);
    }

    void IRGenerator::visit(UnaryOperation& node)
    {
        if (m_collecting)
        {
            if (node.operand) node.operand->accept(*this);
            return;
        }
        node.operand->accept(*this);
        llvm::Value* operand = popValue();
        if (!operand)
        {
            pushValue(nullptr);
            return;
        }

        llvm::Value* result = nullptr;
        if (node.op == UnaryOperation::Op::NEG)
        {
            result = m_builder.CreateNeg(operand, "negtmp");
        }
        else if (node.op == UnaryOperation::Op::NOT)
        {
            result = m_builder.CreateNot(operand, "nottmp");
        }

        pushValue(result);
    }

    void IRGenerator::visit(IntegerLiteral& node)
    {
        if (m_collecting) return;
        llvm::Value* val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_context), node.value);
        pushValue(val);
    }

    void IRGenerator::visit(BooleanLiteral& node)
    {
        if (m_collecting) return;
        llvm::Value* val = llvm::ConstantInt::getBool(m_context, node.value);
        pushValue(val);
    }

    void IRGenerator::visit(Variable& node)
    {
        if (m_collecting) return;
        auto it = m_allocaMap.find(node.symbol);
        if (it == m_allocaMap.end())
        {
            std::cerr << "Variable '" << node.name << "' has no alloca!" << std::endl;
            pushValue(nullptr);
            return;
        }
        llvm::AllocaInst* alloca = it->second;
        llvm::Value* val = m_builder.CreateLoad(alloca->getAllocatedType(), alloca, node.name);
        pushValue(val);
    }

    void IRGenerator::visit(QualifiedIdentifier& node)
    {
        if (m_collecting) return;
        auto it = m_allocaMap.find(node.symbol);
        if (it == m_allocaMap.end())
        {
            std::cerr << "Qualified identifier '" << node.toString() << "' has no alloca!" << std::endl;
            pushValue(nullptr);
            return;
        }
        llvm::AllocaInst* alloca = it->second;
        llvm::Value* val = m_builder.CreateLoad(alloca->getAllocatedType(), alloca, node.toString());
        pushValue(val);
    }

    void IRGenerator::visit(TypeInt&) {}
    void IRGenerator::visit(TypeBool&) {}

    void IRGenerator::visit(DeclList& node)
    {
        if (m_collecting)
        {
            for (auto& decl : node.declarations) decl->accept(*this);
            return;
        }
        for (auto& decl : node.declarations) decl->accept(*this);
    }

    void IRGenerator::visit(StmtList& node)
    {
        if (m_collecting)
        {
            for (auto& stmt : node.statements) stmt->accept(*this);
            return;
        }
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(DeclOrStmtList& node)
    {
        if (m_collecting)
        {
            for (auto& decl : node.declarations) decl->accept(*this);
            for (auto& stmt : node.statements) stmt->accept(*this);
            return;
        }
        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

    void IRGenerator::visit(NamespaceDecl& node)
    {
        if (m_collecting)
        {
            for (auto& decl : node.declarations) decl->accept(*this);
            for (auto& stmt : node.statements) stmt->accept(*this);
            return;
        }
        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);
    }

} // namespace minilang
