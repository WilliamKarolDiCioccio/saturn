#include <codex/symbols_database.hpp>

namespace codex
{

SymbolID SymbolsDatabase::addSymbol(Symbol symbol)
{
    SymbolID id{m_nextID++};
    symbol.id = id;

    const auto& name = symbol.name;
    const auto& qualifiedName = symbol.qualifiedName;

    m_byName[name].push_back(id);
    m_byQualified[qualifiedName].push_back(id);
    m_symbols.emplace(id.value, std::move(symbol));

    return id;
}

const Symbol* SymbolsDatabase::findByID(SymbolID id) const
{
    auto it = m_symbols.find(id.value);
    if (it == m_symbols.end()) return nullptr;
    return &it->second;
}

Symbol* SymbolsDatabase::mutableFindByID(SymbolID id)
{
    auto it = m_symbols.find(id.value);
    if (it == m_symbols.end()) return nullptr;
    return &it->second;
}

const Symbol* SymbolsDatabase::findByQualifiedName(const std::string& qn) const
{
    auto it = m_byQualified.find(qn);
    if (it == m_byQualified.end() || it->second.empty()) return nullptr;
    return findByID(it->second.front());
}

std::vector<const Symbol*> SymbolsDatabase::findByName(const std::string& name) const
{
    std::vector<const Symbol*> result;
    auto it = m_byName.find(name);
    if (it == m_byName.end()) return result;
    for (const auto& id : it->second)
    {
        if (auto* sym = findByID(id)) result.push_back(sym);
    }
    return result;
}

std::vector<const Symbol*> SymbolsDatabase::findByKind(SymbolKind kind) const
{
    std::vector<const Symbol*> result;
    for (const auto& [_, sym] : m_symbols)
    {
        if (sym.symbolKind == kind) result.push_back(&sym);
    }
    return result;
}

std::vector<const Symbol*> SymbolsDatabase::allSymbols() const
{
    std::vector<const Symbol*> result;
    result.reserve(m_symbols.size());
    for (const auto& [_, sym] : m_symbols)
    {
        result.push_back(&sym);
    }
    return result;
}

size_t SymbolsDatabase::size() const { return m_symbols.size(); }

} // namespace codex
