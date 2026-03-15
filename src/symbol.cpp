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

    SymbolEntry* SymbolTable::declare(const std::string& name, Type type, const Position& pos)
    {
        auto& current = m_scopes.back();
        if (current.find(name) != current.end())
            return nullptr;

        auto it = current.emplace(name, SymbolEntry(name, SymbolKind::VARIABLE, type, pos, this)).first;
        return &it->second;
    }

    SymbolEntry* SymbolTable::lookup(const std::string& name)
    {
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        {
            auto found = it->find(name);
            if (found != it->end())
                return &found->second;
        }
        if (m_parent)
            return m_parent->lookup(name);
        return nullptr;
    }

    SymbolEntry* SymbolTable::lookupQualified(const std::vector<std::string>& names)
    {
        if (names.empty()) return nullptr;
        if (names.size() == 1)
            return lookup(names[0]);

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


    SymbolEntry* SymbolTable::declareFunction(const std::string& name, Type returnType, const Position& pos)
    {
        auto& current = m_scopes.back();
        if (current.find(name) != current.end())
            return nullptr;

        auto it = current.emplace(name, SymbolEntry(name, SymbolKind::FUNCTION, returnType, pos, this)).first;
        return &it->second;
    }

    std::string SymbolTable::getQualifiedName() const
    {
        if (m_parent && !m_parent->getName().empty())
            return m_parent->getQualifiedName() + "::" + m_name;
        else
            return m_name;
    }


} // namespace minilang
