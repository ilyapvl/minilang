#include "grammar.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace minilang;

std::string readFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printAST(Node* node, int indent = 0)
{
    if (!node) return;
    for (int i = 0; i < indent; ++i) std::cout << "  ";

    if (auto prog = dynamic_cast<Program*>(node))
    {
        std::cout << "Program\n";
        for (auto& decl : prog->declarations) printAST(decl.get(), indent + 1);
        for (auto& stmt : prog->statements) printAST(stmt.get(), indent + 1);
    }
    else if (auto decl = dynamic_cast<Declaration*>(node))
    {
        std::cout << "Declaration " << (decl->type == Declaration::Type::INT ? "int" : "bool")
                  << " " << decl->name;
        if (decl->initializer)
        {
            std::cout << " = \n";
            printAST(decl->initializer.get(), indent + 1);
        }
        else
        {
            std::cout << "\n";
        }
    }
    else if (auto assign = dynamic_cast<Assignment*>(node))
    {
        std::cout << "Assignment " << assign->varName << " =\n";
        printAST(assign->expr.get(), indent + 1);
    }
    else if (auto ifStmt = dynamic_cast<IfStmt*>(node))
    {
        std::cout << "If\n";
        printAST(ifStmt->condition.get(), indent + 1);
        std::cout << std::string(indent + 1, ' ') << "then:\n";
        printAST(ifStmt->thenBranch.get(), indent + 2);
        if (ifStmt->elseBranch)
        {
            std::cout << std::string(indent + 1, ' ') << "else:\n";
            printAST(ifStmt->elseBranch.get(), indent + 2);
        }
    }
    else if (auto whileStmt = dynamic_cast<WhileStmt*>(node))
    {
        std::cout << "While\n";
        printAST(whileStmt->condition.get(), indent + 1);
        std::cout << std::string(indent + 1, ' ') << "body:\n";
        printAST(whileStmt->body.get(), indent + 2);
    }
    else if (auto block = dynamic_cast<Block*>(node))
    {
        std::cout << "Block\n";
        for (auto& decl : block->declarations) printAST(decl.get(), indent + 1);
        for (auto& stmt : block->statements) printAST(stmt.get(), indent + 1);
    }
    else if (auto bin = dynamic_cast<BinaryOperation*>(node))
    {
        std::cout << "BinaryOp ";
        switch (bin->op)
        {
            case BinaryOperation::Op::PLUS:  std::cout << "+"; break;
            case BinaryOperation::Op::MINUS: std::cout << "-"; break;
            case BinaryOperation::Op::MUL:   std::cout << "*"; break;
            case BinaryOperation::Op::DIV:   std::cout << "/"; break;
            case BinaryOperation::Op::MOD:   std::cout << "%"; break;
            case BinaryOperation::Op::AND:   std::cout << "&&"; break;
            case BinaryOperation::Op::OR:    std::cout << "||"; break;
            case BinaryOperation::Op::EQ:    std::cout << "=="; break;
            case BinaryOperation::Op::NE:    std::cout << "!="; break;
            case BinaryOperation::Op::LT:    std::cout << "<"; break;
            case BinaryOperation::Op::GT:    std::cout << ">"; break;
            case BinaryOperation::Op::LE:    std::cout << "<="; break;
            case BinaryOperation::Op::GE:    std::cout << ">="; break;
        }
        std::cout << "\n";
        printAST(bin->left.get(), indent + 1);
        printAST(bin->right.get(), indent + 1);
    }
    else if (auto un = dynamic_cast<UnaryOperation*>(node))
    {
        std::cout << "UnaryOp ";
        if (un->op == UnaryOperation::Op::NEG) std::cout << "-";
        else std::cout << "!";
        std::cout << "\n";
        printAST(un->operand.get(), indent + 1);
    }
    else if (auto lit = dynamic_cast<IntegerLiteral*>(node))
    {
        std::cout << "Integer " << lit->value << "\n";
    }
    else if (auto blit = dynamic_cast<BooleanLiteral*>(node))
    {
        std::cout << "Boolean " << (blit->value ? "true" : "false") << "\n";
    }
    else if (auto var = dynamic_cast<Variable*>(node))
    {
        std::cout << "Variable " << var->name << "\n";
    }
    else
    {
        std::cout << "Unknown node\n";
    }
}

int main(int argc, char* argv[])
{
    std::string source = readFile(argv[1]);
    if (source.empty()) return 1;

    grammar::Grammar gr;
    gr.buildMiniLang();
    gr.computeFirst();
    gr.computeFollow();
    gr.buildLR1();
    gr.buildLALR();

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(gr, tokens);
    std::unique_ptr<Program> ast = parser.parse();

    if (!ast)
    {
        std::cerr << "Parsing failed." << std::endl;
        return 1;
    }

    printAST(ast.get());

    return 0;
}
