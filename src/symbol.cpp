#include "symbol.hpp"
#include <cassert>
#include <iostream>

namespace minilang
{

    SymbolTable::SymbolTable(const std::string& name, SymbolTable* parent)
        : m_name(name), m_parent(parent)
    {
        m_scopes.emplace_back();
    }

    void SymbolTable::enterScope()
    {
        m_scopes.emplace_back();
    }

    void SymbolTable::exitScope()
    {
        assert(m_scopes.size() > 1);
        if (m_scopes.size() > 1) m_scopes.pop_back();
    }

    bool SymbolTable::declare(const std::string& name, Type type, const Position& pos)
    {
        auto& current = m_scopes.back();
        if (current.find(name) != current.end())
            return false;

        current[name] = SymbolEntry(name, type, pos);
        return true;
    }

    SymbolEntry* SymbolTable::lookupInThisTableOnly(const std::string& name)
    {
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        {
            auto found = it->find(name);
            if (found != it->end())
                return &found->second;
        }
        return nullptr;
    }

    SymbolEntry* SymbolTable::lookupQualified(const std::vector<std::string>& names)
    {
        if (names.empty()) return nullptr;
        if (names.size() == 1)
            return lookupInThisTableOnly(names[0]);

        auto it = m_namespaces.find(names[0]);
        if (it == m_namespaces.end())
            return nullptr;

        std::vector<std::string> rest(names.begin() + 1, names.end());
        return it->second->lookupQualified(rest);
    }

    SymbolTable* SymbolTable::getOrCreateNamespace(const std::string& name)
    {
        auto it = m_namespaces.find(name);
        if (it != m_namespaces.end())
            return it->second.get();
        auto ns = std::make_unique<SymbolTable>(name, this);
        auto ptr = ns.get();
        m_namespaces[name] = std::move(ns);
        return ptr;
    }



} // namespace minilang
