#pragma once

#include <memory>
#include <string>
#include <vector>

#include "include_graph.hpp"
#include "nodes.hpp"
#include "symbols_database.hpp"

namespace codex
{

struct AnalysisResult
{
    SymbolsDatabase database;
    IncludeGraph includeGraph;
};

class Analyzer
{
   public:
    AnalysisResult analyze(const std::vector<std::shared_ptr<SourceNode>>& sourceNodes);

   private:
    // Phase 1
    IncludeGraph buildIncludeGraph(const std::vector<std::shared_ptr<SourceNode>>& sourceNodes);

    // Phase 2
    void indexSymbols(const IncludeGraph& graph, const std::vector<size_t>& order,
                      SymbolsDatabase& db);
    void indexFile(const std::shared_ptr<SourceNode>& node, SymbolsDatabase& db);
    void walkChildren(const std::vector<std::shared_ptr<Node>>& children,
                      const std::string& nsPrefix, const std::vector<std::string>& usingNamespaces,
                      SymbolsDatabase& db, SymbolID parent, const std::filesystem::path& filePath);

    // Phase 3
    void linkCrossReferences(SymbolsDatabase& db);
    SymbolID resolveTypeName(const std::string& typeName, const std::string& currentNs,
                             const std::vector<std::string>& usingNs,
                             const SymbolsDatabase& db) const;

    // Helpers
    void walkStructMembers(const StructNode& structNode, const std::string& qualPrefix,
                           const std::vector<std::string>& usingNamespaces, SymbolsDatabase& db,
                           SymbolID parent, const std::filesystem::path& filePath);
    void walkClassMembers(const ClassNode& classNode, const std::string& qualPrefix,
                          const std::vector<std::string>& usingNamespaces, SymbolsDatabase& db,
                          SymbolID parent, const std::filesystem::path& filePath);
    void walkUnionMembers(const UnionNode& unionNode, const std::string& qualPrefix,
                          const std::vector<std::string>& usingNamespaces, SymbolsDatabase& db,
                          SymbolID parent, const std::filesystem::path& filePath);

    SymbolID registerNode(const std::shared_ptr<Node>& node, SymbolKind kind,
                          const std::string& name, const std::string& qualifiedName,
                          const std::string& signature, AccessSpecifier access,
                          const std::filesystem::path& filePath, SymbolsDatabase& db,
                          SymbolID parent);

    std::string buildSignature(const std::vector<FunctionParameter>& params) const;

    void collectTypeRefs(const Symbol& sym, std::vector<std::string>& outTypes) const;
    void collectTypeSignatureRefs(const TypeSignature& ts, std::vector<std::string>& out) const;
};

} // namespace codex
