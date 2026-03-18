// test/unit/test_symbol.cpp
#include <gtest/gtest.h>
#include "symbol.hpp"
#include "ast.hpp"

using namespace minilang;

class SymbolTableTest : public ::testing::Test
{
protected:
    SymbolTable global;
    Position dummyPos{1, 1};

    void SetUp() override {}
};

TEST_F(SymbolTableTest, DeclareVariable)
{
    SymbolEntry* entry = global.declare("x", Type::INT, dummyPos);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->name, "x");
    EXPECT_EQ(entry->kind, SymbolKind::VARIABLE);
    EXPECT_EQ(entry->type, Type::INT);
    EXPECT_EQ(entry->declPos.line, 1);
    EXPECT_EQ(entry->declPos.column, 1);
    EXPECT_EQ(entry->scope, &global);

    SymbolEntry* dup = global.declare("x", Type::BOOL, dummyPos);
    EXPECT_EQ(dup, nullptr);
}

TEST_F(SymbolTableTest, LookupCurrentScope)
{
    global.declare("x", Type::INT, dummyPos);
    SymbolEntry* found = global.lookup("x");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "x");
    EXPECT_EQ(found->type, Type::INT);
}

TEST_F(SymbolTableTest, LookupNotFound)
{
    SymbolEntry* found = global.lookup("y");
    EXPECT_EQ(found, nullptr);
}

TEST_F(SymbolTableTest, NestedScopes)
{
    global.declare("a", Type::INT, dummyPos);

    global.enterScope();
    global.declare("b", Type::BOOL, dummyPos);

    SymbolEntry* a = global.lookup("a");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->name, "a");
    SymbolEntry* b = global.lookup("b");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->name, "b");

    global.exitScope();

    SymbolEntry* b_after = global.lookup("b");
    EXPECT_EQ(b_after, nullptr);
    a = global.lookup("a");
    ASSERT_NE(a, nullptr);
}

TEST_F(SymbolTableTest, DeclareFunction)
{
    SymbolEntry* func = global.declareFunction("abc", Type::INT, dummyPos);
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name, "abc");
    EXPECT_EQ(func->kind, SymbolKind::FUNCTION);
    EXPECT_EQ(func->type, Type::INT);

    func->paramTypes = {Type::INT, Type::BOOL};

    SymbolEntry* dup = global.declare("abc", Type::BOOL, dummyPos);
    EXPECT_EQ(dup, nullptr);
}

TEST_F(SymbolTableTest, LookupFunction)
{
    global.declareFunction("bar", Type::BOOL, dummyPos);
    SymbolEntry* found = global.lookup("bar");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->kind, SymbolKind::FUNCTION);
}

TEST_F(SymbolTableTest, Namespaces)
{
    SymbolTable* ns = global.getOrCreateNamespace("ns");
    ASSERT_NE(ns, nullptr);
    EXPECT_EQ(ns->getName(), "ns");

    ns->declare("x", Type::INT, dummyPos);

    std::vector<std::string> names = {"ns", "x"};
    SymbolEntry* found = global.lookupQualified(names);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "x");
    EXPECT_EQ(found->type, Type::INT);

    names = {"ns", "y"};
    found = global.lookupQualified(names);
    EXPECT_EQ(found, nullptr);

    names = {"bad", "x"};
    found = global.lookupQualified(names);
    EXPECT_EQ(found, nullptr);
}

TEST_F(SymbolTableTest, NestedNamespaces)
{
    SymbolTable* ns1 = global.getOrCreateNamespace("ns1");
    SymbolTable* ns2 = ns1->getOrCreateNamespace("ns2");
    ns2->declare("z", Type::BOOL, dummyPos);

    std::vector<std::string> names = {"ns1", "ns2", "z"};
    SymbolEntry* found = global.lookupQualified(names);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "z");
    EXPECT_EQ(found->type, Type::BOOL);
}

TEST_F(SymbolTableTest, QualifiedSingle)
{
    global.declare("alone", Type::INT, dummyPos);
    std::vector<std::string> names = {"alone"};
    SymbolEntry* found = global.lookupQualified(names);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "alone");
}
