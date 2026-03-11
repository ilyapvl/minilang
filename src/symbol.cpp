#include "symbol.hpp"

namespace minilang
{

    void SymbolTable::enterScope()
    {
        scopes.emplace_back();
    }

    void SymbolTable::exitScope()
    {
        if (!scopes.empty()) scopes.pop_back();
    }

    bool SymbolTable::declare(const std::string& name, Type type, const Position& pos)
    {
        if (scopes.empty())
        {
            // should have at least global scope. if not, create one implicitly
            enterScope();
        }

        // redeclaration check
        auto& current = scopes.back();
        auto it = current.find(name);

        if (it != current.end())
            return false;

        SymbolEntry entry{name, type, pos, false};
        current[name] = entry;
        return true;
    }

    SymbolEntry* SymbolTable::lookup(const std::string& name)
    {
        // search from innermost to outermost
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        {
            auto found = it->find(name);
            if (found != it->end())
                return &found->second;
        }
        return nullptr;
    }

    SymbolEntry* SymbolTable::lookupCurrentScope(const std::string& name)
    {
        if (scopes.empty())
            return nullptr;
        auto& current = scopes.back();
        auto it = current.find(name);
        if (it != current.end())
            return &it->second;
        return nullptr;
    }

} // namespace minilang
