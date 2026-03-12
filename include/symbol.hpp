#ifndef MINILANG_SYMBOL_HPP
#define MINILANG_SYMBOL_HPP

#include "ast.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace minilang
{

    struct SymbolEntry // FIXME mirroring name and type fields in AST classes
    {
        std::string name;
        Type type;
        Position declPos;
        bool initialized;

        SymbolEntry() : name(""), type(Type::ERROR), declPos(), initialized(false) {}

        SymbolEntry(const std::string& n, Type t, const Position& p)
            : name(n), type(t), declPos(p), initialized(false) {}
    };

    class SymbolTable
    {
    public:
        SymbolTable(const std::string& name = "", SymbolTable* parent = nullptr);

        // scope management
        void enterScope();
        void exitScope();

        SymbolEntry* declare(const std::string& name, Type type, const Position& pos);

        // search qualified ::
        SymbolEntry* lookupInThisTableOnly(const std::string& name);
        SymbolEntry* lookupQualified(const std::vector<std::string>& names);


        SymbolTable* getOrCreateNamespace(const std::string& name);

        const std::string& getName() const { return m_name; }

    private:
        std::string m_name;
        SymbolTable* m_parent;
        std::vector<std::unordered_map<std::string, SymbolEntry>> m_scopes;
        std::unordered_map<std::string, std::unique_ptr<SymbolTable>> m_namespaces;        
    };

} // namespace minilang

#endif // MINILANG_SYMBOL_HPP
