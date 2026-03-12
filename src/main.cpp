#include "grammar.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include "semantic.hpp"
#include "irgen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

#define LALR

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

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    std::string source = readFile(argv[1]);
    if (source.empty()) return 1;

    grammar::Grammar gr;
    gr.buildMiniLang();
    gr.computeFirst();
    gr.computeFollow();
    gr.prepareStatesLR1();

    #ifdef LALR
    gr.buildLALR();
    #else
    gr.buildLR1();
    #endif

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(gr, tokens);
    std::unique_ptr<Program> ast = parser.parse();

    if (!ast)
    {
        std::cerr << "Parsing failed" << std::endl;
        return 1;
    }

    SemanticAnalyzer analyzer;
    bool semanticOk = analyzer.analyze(ast.get());

    if (!semanticOk)
    {
        std::cerr << "Semantic errors found" << std::endl;
        return 1;
    }


    llvm::LLVMContext context;
    IRGenerator generator(context, "minilang_prog");
    if (!generator.generate(ast.get()))
    {
        std::cerr << "Code generation failed" << std::endl;
    }


    generator.printModule();

    return 0;
}
