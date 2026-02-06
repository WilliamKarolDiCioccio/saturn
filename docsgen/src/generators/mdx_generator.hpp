#pragma once

#include <vector>

#include <codex/analyzer.hpp>

#include "../models/mdx_file.hpp"
#include "../models/parsed_comment.hpp"
#include "cross_linker.hpp"

namespace docsgen
{

class MDXGenerator
{
   private:
    const codex::SymbolsDatabase* m_db = nullptr;
    CrossLinker m_linker;
    bool m_verbose = false;

   public:
    void setVerbose(bool verbose) { m_verbose = verbose; }
    std::vector<MDXFile> generate(const codex::AnalysisResult& result);

   private:
    bool isPublicAPI(const codex::Symbol& sym) const;

    ParsedComment parseNodeComment(const std::shared_ptr<codex::Node>& node) const;
    std::string formatSeeAlsoFromComment(const ParsedComment& parsed) const;

    MDXFile generateClassDoc(const codex::Symbol& sym);
    MDXFile generateStructDoc(const codex::Symbol& sym);
    MDXFile generateEnumDoc(const codex::Symbol& sym);
    MDXFile generateFunctionDoc(const codex::Symbol& sym);
    MDXFile generateTypeAliasDoc(const codex::Symbol& sym);
    MDXFile generateConceptDoc(const codex::Symbol& sym);
    MDXFile generateOperatorDoc(const codex::Symbol& sym);

    std::string generateInheritanceSection(const codex::Symbol& sym);
    std::string generateTemplateSection(
        const std::shared_ptr<codex::Node>& templateDecl,
        const std::unordered_map<std::string, std::string>& tparamDescs = {});
    std::string generateConstructorsSection(
        const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& ctors);
    std::string generateDestructorsSection(
        const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& dtors);
    std::string generateMethodsSection(
        const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>&
            methods);
    std::string generateOperatorsSection(
        const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& ops);
    std::string generateMemberVarsSection(
        const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& vars);
    std::string generateSeeAlsoSection(const codex::Symbol& sym, const ParsedComment& parsed = {});

    // Struct members (no access specifier pairs)
    std::string generateStructConstructorsSection(
        const std::vector<std::shared_ptr<codex::Node>>& ctors);
    std::string generateStructDestructorsSection(
        const std::vector<std::shared_ptr<codex::Node>>& dtors);
    std::string generateStructMethodsSection(
        const std::vector<std::shared_ptr<codex::Node>>& methods);
    std::string generateStructOperatorsSection(
        const std::vector<std::shared_ptr<codex::Node>>& ops);
    std::string generateStructMemberVarsSection(
        const std::vector<std::shared_ptr<codex::Node>>& vars);

    std::string getComment(const std::shared_ptr<codex::Node>& node) const;
};

} // namespace docsgen
