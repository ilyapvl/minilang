#ifndef MINILANG_SYMBOL_HPP
#define MINILANG_SYMBOL_HPP

#include "ast.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace minilang
{

    struct SymbolEntry
    {
        std::string name;
        Type type;
        Position declPos;
        bool initialized;
    };

    class SymbolTable
    {
    public:
        SymbolTable() = default;

        // scope management
        void enterScope();
        void exitScope();

        // true if successful, false if redeclared in current scope
        bool declare(const std::string& name, Type type, const Position& pos);

        // finds in current or enclosing scopes
        SymbolEntry* lookup(const std::string& name);

        // lookup only in current scope
        SymbolEntry* lookupCurrentScope(const std::string& name);


        size_t currentScopeLevel() const { return scopes.size(); }

    private:
        // each scope is a map from name to entry
        using Scope = std::unordered_map<std::string, SymbolEntry>;
        std::vector<Scope> scopes;
    };

} // namespace minilang

#endif // MINILANG_SYMBOL_HPP
