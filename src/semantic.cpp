#include "semantic.hpp"
#include <iostream>

namespace minilang
{



    bool SemanticAnalyzer::analyze(Program* root)
    {
        if (!root) return false;
        root->accept(*this);
        return !m_hasError;
    }

    void SemanticAnalyzer::error(const Position& pos, const std::string& message)
    {
        std::cerr << "Error at " << pos.line << ":" << pos.column << ": " << message << std::endl;
        m_hasError = true;
    }



    // Visitor implementation


    void SemanticAnalyzer::visit(Program& node)
    {
        for (auto& decl : node.declarations)
            decl->accept(*this);
        for (auto& stmt : node.statements)
            stmt->accept(*this);
    }

    void SemanticAnalyzer::visit(Declaration& node)
    {
        if (node.initializer)
        {
            node.initializer->accept(*this);
            if (node.initializer->exprType != Type::ERROR)
            {
                Type declType = (node.type == Declaration::Type::INT) ? Type::INT : Type::BOOL;
                if (node.initializer->exprType != declType)
                {
                    std::string msg = "Type mismatch in initialization: expected ";
                    msg += (declType == Type::INT ? "int" : "bool");
                    msg += ", got ";
                    msg += (node.initializer->exprType == Type::INT ? "int" : "bool");
                    error(node.pos, msg);
                }
            }
        }

        Type declType = (node.type == Declaration::Type::INT) ? Type::INT : Type::BOOL;
        if (!m_currentTable->declare(node.name, declType, node.pos))
            error(node.pos, "Redeclaration of variable '" + node.name + "'");
    }

    void SemanticAnalyzer::visit(Assignment& node)
    {

        node.target->accept(*this);

        if (node.target->exprType == Type::ERROR)
        {
            if (node.expr) node.expr->accept(*this);
            return;
        }


        node.expr->accept(*this);
        Type exprType = node.expr->exprType;
        if (exprType == Type::ERROR) return;


        if (exprType != node.target->exprType)
        {
            std::string msg = "Type mismatch in assignment: variable '";
            std::string fullName;
            for (size_t i = 0; i < node.target->names.size(); ++i)
            {
                if (i > 0) fullName += "::";
                fullName += node.target->names[i];
            }
            msg += fullName + "' has type " + 
                (node.target->exprType == Type::INT ? "int" : "bool") +
                ", expression has type " + (exprType == Type::INT ? "int" : "bool");
            error(node.pos, msg);
        }
    }

    void SemanticAnalyzer::visit(IfStmt& node)
    {
        node.condition->accept(*this);
        Type condType = node.condition->exprType;
        if (condType != Type::ERROR && condType != Type::BOOL)
        {
            std::string msg = "If condition must be boolean, got ";
            msg += (condType == Type::INT ? "int" : "bool");
            error(node.condition->pos, msg);
        }

        if (node.thenBranch) node.thenBranch->accept(*this);
        if (node.elseBranch) node.elseBranch->accept(*this);
    }

    void SemanticAnalyzer::visit(WhileStmt& node)
    {
        node.condition->accept(*this);
        Type condType = node.condition->exprType;
        if (condType != Type::ERROR && condType != Type::BOOL)
        {
            std::string msg = "While condition must be boolean, got ";
            msg += (condType == Type::INT ? "int" : "bool");
            error(node.condition->pos, msg);
        }

        if (node.body) node.body->accept(*this);
    }

    void SemanticAnalyzer::visit(Block& node)
    {
        m_currentTable->enterScope();

        for (auto& decl : node.declarations)
            decl->accept(*this);
        for (auto& stmt : node.statements)
            stmt->accept(*this);

        m_currentTable->exitScope();
    }

    void SemanticAnalyzer::visit(BinaryOperation& node)
    {
        node.left->accept(*this);
        node.right->accept(*this);
        node.exprType = checkBinaryOp(node);
    }

    Type SemanticAnalyzer::checkBinaryOp(BinaryOperation& node)
    {
        Type leftType = node.left->exprType;
        Type rightType = node.right->exprType;

        if (leftType == Type::ERROR || rightType == Type::ERROR)
            return Type::ERROR;

        using Op = BinaryOperation::Op;
        switch (node.op)
        {
            case Op::PLUS:
            case Op::MINUS:
            case Op::MUL:
            case Op::DIV:
            case Op::MOD:
                if (leftType != Type::INT || rightType != Type::INT)
                {
                    error(node.pos, "Arithmetic operation requires int operands");
                    return Type::ERROR;
                }
                return Type::INT;


            case Op::LT:
            case Op::GT:
            case Op::LE:
            case Op::GE:
            case Op::EQ:
            case Op::NE:
                if (leftType != Type::INT || rightType != Type::INT)
                {
                    error(node.pos, "Comparison requires int operands");
                    return Type::ERROR;
                }
                return Type::BOOL;


            case Op::AND:
            case Op::OR:
                if (leftType != Type::BOOL || rightType != Type::BOOL)
                {
                    error(node.pos, "Logical operation requires bool operands");
                    return Type::ERROR;
                }
                return Type::BOOL;

            default:
                return Type::ERROR;
        }
    }

    void SemanticAnalyzer::visit(UnaryOperation& node)
    {
        node.operand->accept(*this);
        node.exprType = checkUnaryOp(node);
    }

    Type SemanticAnalyzer::checkUnaryOp(UnaryOperation& node)
    {
        Type opType = node.operand->exprType;
        if (opType == Type::ERROR) return Type::ERROR;

        using Op = UnaryOperation::Op;
        switch (node.op)
        {
            case Op::NEG:
                if (opType != Type::INT)
                {
                    error(node.pos, "Unary minus requires int operand");
                    return Type::ERROR;
                }
                return Type::INT;
            case Op::NOT:
                if (opType != Type::BOOL)
                {
                    error(node.pos, "Logical not requires bool operand");
                    return Type::ERROR;
                }
                return Type::BOOL;
            default:
                return Type::ERROR;
        }
    }

    void SemanticAnalyzer::visit(IntegerLiteral& node)
    {
        node.exprType = Type::INT;
    }

    void SemanticAnalyzer::visit(BooleanLiteral& node)
    {
        node.exprType = (node.value ? Type::BOOL : Type::BOOL);
    }


    SemanticAnalyzer::SemanticAnalyzer()
        : m_currentTable(&m_globalTable), m_hasError(false) {}


    void SemanticAnalyzer::visit(NamespaceDecl& node)
    {
        SymbolTable* nsTable = m_currentTable->getOrCreateNamespace(node.name);

        m_tableStack.push_back(m_currentTable);
        m_currentTable = nsTable;

        for (auto& decl : node.declarations) decl->accept(*this);
        for (auto& stmt : node.statements) stmt->accept(*this);

        m_currentTable = m_tableStack.back();
        m_tableStack.pop_back();
    }

    void SemanticAnalyzer::visit(QualifiedIdentifier& node)
    {
        SymbolEntry* entry = m_currentTable->lookupQualified(node.names);
        if (!entry)
        {
            std::string fullName;
            for (size_t i = 0; i < node.names.size(); ++i)
            {
                if (i > 0) fullName += "::";
                fullName += node.names[i];
            }
            error(node.pos, "Undeclared identifier '" + fullName + "'");
            node.exprType = Type::ERROR;
            return;
        }
        node.exprType = entry->type;
    }

    void SemanticAnalyzer::visit(Variable& node)
    {
        SymbolEntry* entry = m_currentTable->lookupInThisTableOnly(node.name);
        if (!entry)
        {
            error(node.pos, "Undeclared variable '" + node.name + "'");
            node.exprType = Type::ERROR;
            return;
        }
        node.exprType = entry->type;
    }


    void SemanticAnalyzer::visit(TypeInt&) {}
    void SemanticAnalyzer::visit(TypeBool&) {}

    void SemanticAnalyzer::visit(DeclList& node)
    {
        for (auto& decl : node.declarations)
            decl->accept(*this);
    }

    void SemanticAnalyzer::visit(StmtList& node)
    {
        for (auto& stmt : node.statements)
            stmt->accept(*this);
    }

    void SemanticAnalyzer::visit(DeclOrStmtList& node)
    {
        for (auto& decl : node.declarations)
            decl->accept(*this);
        for (auto& stmt : node.statements)
            stmt->accept(*this);
    }

} // namespace minilang
