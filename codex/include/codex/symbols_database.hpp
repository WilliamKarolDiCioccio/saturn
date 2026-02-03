#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "symbol.hpp"

namespace codex
{

class SymbolsDatabase
{
   private:
    uint32_t m_nextID = 1;
    std::unordered_map<uint32_t, Symbol> m_symbols;
    std::unordered_map<std::string, std::vector<SymbolID>> m_byName;
    std::unordered_map<std::string, std::vector<SymbolID>> m_byQualified;

   public:
    SymbolsDatabase() = default;
    SymbolsDatabase(const SymbolsDatabase&) = delete;
    SymbolsDatabase& operator=(const SymbolsDatabase&) = delete;
    SymbolsDatabase(SymbolsDatabase&&) = default;
    SymbolsDatabase& operator=(SymbolsDatabase&&) = default;

   public:
    SymbolID addSymbol(Symbol symbol);
    const Symbol* findByID(SymbolID id) const;
    Symbol* mutableFindByID(SymbolID id);
    const Symbol* findByQualifiedName(const std::string& qn) const;
    std::vector<const Symbol*> findByName(const std::string& name) const;
    std::vector<const Symbol*> findByKind(SymbolKind kind) const;
    std::vector<const Symbol*> allSymbols() const;
    size_t size() const;
};

} // namespace codex
