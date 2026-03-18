#include <gtest/gtest.h>
#include "grammar.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "irgen.hpp"
#include "codegen.hpp"
#include <llvm/Support/raw_ostream.h>
#include <sstream>
#include <fstream>
#include <cstdio>

using namespace minilang;

class CompilerTest : public ::testing::Test
{
protected:
    grammar::Grammar g;
    llvm::LLVMContext context;
    std::string outputFile = "test_output.s";

    void SetUp() override
    {
        g.buildMiniLang();
        g.computeFirst();
        g.computeFollow();
        g.prepareStatesLR1();
        g.buildLALR();
    }

    void TearDown() override
    {
        std::remove(outputFile.c_str());
    }

    bool compile(const std::string& source)
    {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        for (const auto& tok : tokens)
        {
            if (tok.type == grammar::TERM_ERROR)
            {
                ADD_FAILURE() << "Lexer error: " << tok.lexeme;
                return false;
            }
        }

        Parser parser(g, tokens);
        std::unique_ptr<Program> ast = parser.parse();

        if (!ast)
        {
            return false;
        }

        SemanticAnalyzer analyzer;
        bool semanticOk = analyzer.analyze(ast.get());
        if (!semanticOk)
        {
            return false;
        }

        IRGenerator generator(context, "minilang_test");
        bool irOk = generator.generate(ast.get());
        if (!irOk)
        {
            return false;
        }

        optimizeModule(generator.getModule());
        bool asmOk = generateARMCode(generator.getModule(), outputFile);
        if (!asmOk)
        {
            return false;
        }

        std::ifstream f(outputFile);
        bool fileExists = f.good();
        f.close();
        if (!fileExists)
        {
            return false;
        }

        return true;
    }
};

TEST_F(CompilerTest, OnlyMain)
{
    std::string source = R"(
        func main() -> int
        {
            return 0;
        }
    )";
    EXPECT_TRUE(compile(source));
}

TEST_F(CompilerTest, Arithmetic)
{
    std::string source = R"(
        func main() -> int
        {
            int a = 5;
            int b = 10;
            int c = a + b * 2;
            return c;
        }
    )";
    EXPECT_TRUE(compile(source));
}

TEST_F(CompilerTest, IfStatement)
{
    std::string source = R"(
        func main() -> int
        {
            int x = 5;
            int y = 0;
            if (x > 3)
            {
                y = 10;
            }
                
            else
            {
                y = 20;
            }
            return y;
        }
    )";
    EXPECT_TRUE(compile(source));
}

TEST_F(CompilerTest, WhileLoop)
{
    std::string source = R"(
        func main() -> int
        {
            int sum = 0;
            int i = 1;
            while (i <= 10)
            {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
    )";
    EXPECT_TRUE(compile(source));
}

TEST_F(CompilerTest, FunctionCall)
{
    std::string source = R"(
        func add(int a, int b) -> int
        {
            return a + b;
        }
        func main() -> int
        {
            return add(5, 7);
        }
    )";
    EXPECT_TRUE(compile(source));
}

TEST_F(CompilerTest, RecursiveFactorial)
{
    std::string source = R"(
        func fact(int n) -> int
        {
            if (n <= 1) return 1;
            return n * fact(n - 1);
        }
        func main() -> int
        {
            return fact(5);
        }
    )";
    EXPECT_TRUE(compile(source));
}

TEST_F(CompilerTest, Namespace)
{
    std::string source = R"(
        @math
        {
            func add(int a, int b) -> int
            {
                return a + b;
            }
        }
        func main() -> int
        {
            return math::add(3, 4);
        }
    )";
    EXPECT_TRUE(compile(source));
}

TEST_F(CompilerTest, SyntaxError)
{
    std::string source = R"(
        func main() -> int
        {
            int x = 5
            return x;
        }
    )"; // missing semicolon
    EXPECT_FALSE(compile(source));
}

TEST_F(CompilerTest, SemanticError)
{
    std::string source = R"(
        func main() -> int
        {
            int x = true;  // type mismatch
            return x;
        }
    )";
    EXPECT_FALSE(compile(source));
}

TEST_F(CompilerTest, UndeclaredVariable)
{
    std::string source = R"(
        func main() -> int
        {
            x = 10;  // x not declared
            return x;
        }
    )";
    EXPECT_FALSE(compile(source));
}

TEST_F(CompilerTest, WrongArgumentCount)
{
    std::string source = R"(
        func foo(int a) -> int
        {
            return a;
        }
        func main() -> int
        {
            return foo(1, 2);  // too many args
        }
    )";
    EXPECT_FALSE(compile(source));
}
