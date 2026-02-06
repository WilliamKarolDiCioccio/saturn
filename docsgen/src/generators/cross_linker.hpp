#pragma once

#include <string>
#include <unordered_map>

#include <codex/symbols_database.hpp>

namespace docsgen
{

class CrossLinker
{
   private:
    std::unordered_map<std::string, std::string> m_qualifiedToFilename;

   public:
    void buildIndex(const codex::SymbolsDatabase& db);
    std::string linkify(const std::string& typeName) const;
    std::string linkifyTypeSignature(const codex::TypeSignature& ts) const;

    static std::string qualifiedToFilename(const std::string& qualifiedName);
    static std::string sanitizeOperator(const std::string& op);

   private:
    std::string linkifyBaseType(const std::string& baseType) const;
};

} // namespace docsgen
