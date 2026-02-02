#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <tuple>

#include <tree_sitter/api.h>

#include "nodes.hpp"

namespace codex
{

class Parser
{
   private:
    TSParser* m_parser;
    std::shared_ptr<Source> m_source;
    std::shared_ptr<CommentNode> m_leadingComment;
    std::shared_ptr<TemplateNode> m_templateDeclaration;

   public:
    Parser();
    ~Parser();

   public:
    std::shared_ptr<SourceNode> parse(const std::shared_ptr<Source>& _source);
    void reset();

   private:
    std::shared_ptr<Node> dispatch(TSNode _node);

    // Dependent nodes (requiring to be linked to other nodes and leaving outisde the main tree)
    std::shared_ptr<CommentNode> parseComment(const TSNode& _node);
    std::shared_ptr<TemplateNode> parseTemplateDeclaration(const TSNode& _node);

    // Dependent nodes consumption helpers
    std::shared_ptr<CommentNode> getLeadingComment();
    std::shared_ptr<TemplateNode> getTemplateDeclaration();
    void clearLeadingComment() { m_leadingComment = nullptr; }
    void clearTemplateDeclaration() { m_templateDeclaration = nullptr; }

    // Independent nodes
    std::shared_ptr<IncludeNode> parseInclude(const TSNode& _node);
    std::shared_ptr<ObjectLikeMacroNode> parseObjectLikeMacro(const TSNode& _node);
    std::shared_ptr<FunctionLikeMacroNode> parseFunctionLikeMacro(const TSNode& _node);
    std::shared_ptr<NamespaceNode> parseNamespace(const TSNode& _node);
    std::shared_ptr<NamespaceAliasNode> parseNamespaceAlias(const TSNode& _node);
    std::shared_ptr<UsingNamespaceNode> parseUsingNamespace(const TSNode& _node);
    std::shared_ptr<TypedefNode> parseTypedef(const TSNode& _node);
    std::shared_ptr<TypeAliasNode> parseTypeAlias(const TSNode& _node);
    std::shared_ptr<EnumNode> parseEnum(const TSNode& _node);
    std::shared_ptr<ConceptNode> parseConcept(const TSNode& _node);
    std::shared_ptr<VariableNode> parseVariable(const TSNode& _node);
    std::shared_ptr<FunctionNode> parseFunction(const TSNode& _node);
    std::shared_ptr<OperatorNode> parseOperator(const TSNode& _node);
    std::shared_ptr<UnionNode> parseUnion(const TSNode& _node);
    std::shared_ptr<Node> parseFriend(const TSNode& _node);
    std::shared_ptr<StructNode> parseStruct(const TSNode& _node);
    std::shared_ptr<ClassNode> parseClass(const TSNode& _node);

    // Custom AST helpers
    std::shared_ptr<Node> parseAmbiguousDeclaration(const TSNode& _node);
    std::shared_ptr<Node> parseAmbiguousDefinition(const TSNode& _node);

    void parseInitDeclarator(const TSNode& _node, std::shared_ptr<VariableNode>& _varNode);
    void parseFunctionDeclarator(const TSNode& _node, std::shared_ptr<FunctionNode>& _fn);
    void parseOperatorDeclarator(const TSNode& _node, std::shared_ptr<OperatorNode>& _op);
    void parseMemberList(const TSNode& _listNode, const std::string& _parentName,
                         std::vector<std::shared_ptr<Node>>& _memberVars,
                         std::vector<std::shared_ptr<Node>>& _staticMemberVars,
                         std::vector<std::shared_ptr<Node>>& _memberFuncs,
                         std::vector<std::shared_ptr<Node>>& _staticMemberFuncs,
                         std::vector<std::shared_ptr<Node>>& _ctors,
                         std::vector<std::shared_ptr<Node>>& _dtors,
                         std::vector<std::shared_ptr<Node>>& _ops,
                         std::vector<std::shared_ptr<Node>>& _friends,
                         std::vector<std::shared_ptr<Node>>& _nestedTypes);
    void parseClassMemberList(
        const TSNode& _listNode, const std::string& _parentName,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _memberVars,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _staticMemberVars,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _memberFuncs,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _staticMemberFuncs,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _ctors,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _dtors,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _ops,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _friends,
        std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _nestedTypes);

    std::vector<TemplateParameter> parseTemplateParameterList(const TSNode& _node);

    TypeSignature parseTypeSignature(const TSNode& _node);
    std::vector<GenericParameter> parseGenericParametersList(const TSNode& _node);
    std::vector<TemplateArgument> parseTemplateArgumentsList(const TSNode& _node);
    std::vector<std::pair<AccessSpecifier, std::string>> parseBaseClassesList(
        const TSNode& _node, AccessSpecifier _defaultAccess = AccessSpecifier::Public);
};

} // namespace codex
