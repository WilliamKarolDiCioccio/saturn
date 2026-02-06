#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <codex/nodes.hpp>
#include <codex/symbol.hpp>

namespace docsgen
{

class CrossLinker;

class MDXFormatter
{
   public:
    static std::string formatComment(const std::string& rawComment);
    static std::string extractBrief(const std::string& rawComment);
    static std::string escapeMDX(const std::string& text);
    static std::string escapeInlineMDX(const std::string& line);

    static std::string formatFunctionSignature(const codex::FunctionNode& fn);
    static std::string formatConstructorSignature(const codex::ConstructorNode& ctor);
    static std::string formatDestructorSignature(const codex::DestructorNode& dtor);
    static std::string formatOperatorSignature(const codex::OperatorNode& op);
    static std::string formatVariableSignature(const codex::VariableNode& var);
    static std::string formatEnumSignature(const codex::EnumNode& en);
    static std::string formatTypeAliasSignature(const codex::TypeAliasNode& alias);
    static std::string formatConceptSignature(const codex::ConceptNode& _concept);

    // 3-column tables (backward compat)
    static std::string formatParametersTable(const std::vector<codex::FunctionParameter>& params,
                                             const CrossLinker& linker);
    static std::string formatTemplateParamsTable(
        const std::vector<codex::TemplateParameter>& params);

    // 4-column tables with descriptions
    static std::string formatParametersTable(
        const std::vector<codex::FunctionParameter>& params, const CrossLinker& linker,
        const std::unordered_map<std::string, std::string>& paramDescs);
    static std::string formatTemplateParamsTable(
        const std::vector<codex::TemplateParameter>& params,
        const std::unordered_map<std::string, std::string>& tparamDescs);

    static std::string formatEnumValuesTable(
        const std::vector<std::shared_ptr<codex::Node>>& enumerators);

    static std::string formatNotesSection(const std::vector<std::string>& notes);
    static std::string formatReturnWithDescription(const std::string& linkifiedType,
                                                   const std::string& returnDesc);

    static std::string formatSourceLocation(const codex::SourceLocation& loc);

   private:
    static std::string formatModifiers(const codex::FunctionNode& fn);
    static std::string formatModifiers(const codex::ConstructorNode& ctor);
    static std::string formatModifiers(const codex::DestructorNode& dtor);
    static std::string formatModifiers(const codex::OperatorNode& op);
    static std::string formatModifiers(const codex::VariableNode& var);

    static std::string formatTemplateDecl(const std::shared_ptr<codex::Node>& templateDecl);
};

} // namespace docsgen
