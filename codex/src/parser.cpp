#include "codex/parser.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <future>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <tree_sitter/api.h>

#include "codex/nodes.hpp"

#include "tree_sitter_cpp.hpp"

namespace codex
{

Parser::Parser()
{
    m_parser = ts_parser_new();

    if (!m_parser)
    {
        throw std::runtime_error("Failed to create TSParser");
    }

    ts_parser_set_language(m_parser, tree_sitter_cpp());
}

Parser::~Parser() { ts_parser_delete(m_parser); }

/////////////////////////////////////////////////////////////////////////////////////////////
// Public API, Lifecycle Management
/////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<SourceNode> Parser::parse(const std::shared_ptr<Source>& _source)
{
    m_source = _source;

    TSTree* tree = ts_parser_parse_string(m_parser, nullptr, m_source->content.c_str(),
                                          static_cast<uint32_t>(m_source->content.size()));

    TSNode root = ts_tree_root_node(tree);
    TSPoint rootEnd = ts_node_end_point(root);
    auto srcNode = std::make_shared<SourceNode>(0, 0, static_cast<int>(rootEnd.row),
                                                static_cast<int>(rootEnd.column));

    srcNode->source = m_source;

    uint32_t childCount = ts_node_child_count(root);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(root, i);
        auto childNode = dispatch(child);

        if (childNode) srcNode->children.emplace_back(childNode);
    }

    ts_tree_delete(tree);

    return srcNode;
}

void Parser::reset()
{
    ts_parser_reset(m_parser);
    m_source = nullptr;
    clearLeadingComment();
    clearTemplateDeclaration();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Custom AST helpers
/////////////////////////////////////////////////////////////////////////////////////////////

auto toConstructorNode = [](const std::shared_ptr<FunctionNode>& func,
                            const std::string& parentName) -> std::shared_ptr<ConstructorNode>
{
    auto ctor = std::make_shared<ConstructorNode>(func->startLine, func->startColumn, func->endLine,
                                                  func->endColumn);

    ctor->name = func->name;

    ctor->isExplicit = func->isExplicit;
    ctor->isNoexcept = func->isNoexcept;
    ctor->isConstexpr = func->isConstexpr;
    ctor->isInline = func->isInline;

    ctor->isDefaulted = false;
    ctor->isDeleted = false;

    ctor->attributes = func->attributes;
    ctor->parameters = func->parameters;
    ctor->templateDecl = func->templateDecl;
    ctor->templateArgs = func->templateArgs;

    ctor->comment = func->comment;

    // Detect copy/move constructor
    // Copy: single param of (const ClassName&) or (ClassName const&)
    // Move: single param of (ClassName&&)
    if (func->parameters.size() == 1)
    {
        const auto& param = func->parameters[0];
        const auto& sig = param.typeSignature;

        // Check if the base type matches the class name
        if (sig.baseType == parentName)
        {
            if (sig.isLValueRef && sig.isConst && !sig.isRValueRef)
            {
                ctor->isCopyConstructor = true;
            }
            else if (sig.isRValueRef && !sig.isLValueRef)
            {
                ctor->isMoveConstructor = true;
            }
        }
    }

    return ctor;
};

auto toDestructorNode =
    [](const std::shared_ptr<FunctionNode>& func) -> std::shared_ptr<DestructorNode>
{
    auto dtor = std::make_shared<DestructorNode>(func->startLine, func->startColumn, func->endLine,
                                                 func->endColumn);

    dtor->name = func->name;

    dtor->isVirtual = func->isVirtual;
    dtor->isPureVirtual = func->isPureVirtual;
    dtor->isNoexcept = func->isNoexcept;
    dtor->isConstexpr = func->isConstexpr;
    dtor->isInline = func->isInline;

    dtor->isDefaulted = false;
    dtor->isDeleted = false;

    dtor->attributes = func->attributes;
    dtor->comment = func->comment;
    return dtor;
};

auto determineKind = [](const TSNode& _node) -> NodeKind
{
    uint32_t childCount = ts_node_child_count(_node);

    for (uint32_t j = 0; j < childCount; ++j)
    {
        TSNode child = ts_node_child(_node, j);
        std::string type = ts_node_type(child);

        if (type == "operator_name" || type == "operator_cast") return NodeKind::Operator;
    }

    return NodeKind::Function;
};

auto isDeclaratorWrapper = [](const std::string& _nodeType) -> bool
{
    return _nodeType == "reference_declarator" || _nodeType == "pointer_declarator" ||
           _nodeType == "array_declarator";
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Core Parsing Logic
/////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Node> Parser::dispatch(TSNode _node)
{
    std::string type = ts_node_type(_node);

    if (type == "comment")
    {
        m_leadingComment = parseComment(_node);
    }
    else if (type == "template_declaration")
    {
        m_templateDeclaration = parseTemplateDeclaration(_node);

        uint32_t childCount = ts_node_child_count(_node);

        for (uint32_t i = 0; i < childCount; ++i)
        {
            auto child = dispatch(ts_node_child(_node, i));
            if (child) return child;
        }
    }
    else if (type == "namespace_definition")
    {
        auto ns = parseNamespace(_node);

        uint32_t childCount = ts_node_child_count(_node);

        for (uint32_t i = 0; i < childCount; ++i)
        {
            TSNode child = ts_node_child(_node, i);
            std::string childType = ts_node_type(child);

            if (childType == "declaration_list")
            {
                uint32_t subChildCount = ts_node_child_count(child);

                for (uint32_t j = 0; j < subChildCount; ++j)
                {
                    auto subChild = dispatch(ts_node_child(child, j));
                    if (subChild) ns->children.emplace_back(subChild);
                }
            }
        }

        return ns;
    }
    else if (type == "preproc_include")
    {
        return parseInclude(_node);
    }
    else if (type == "preproc_def")
    {
        return parseObjectLikeMacro(_node);
    }
    else if (type == "preproc_function_def")
    {
        return parseFunctionLikeMacro(_node);
    }
    else if (type == "namespace_alias_definition")
    {
        return parseNamespaceAlias(_node);
    }
    else if (type == "type_definition")
    {
        return parseTypedef(_node);
    }
    else if (type == "alias_declaration")
    {
        return parseTypeAlias(_node);
    }
    else if (type == "using_declaration")
    {
        return parseUsingNamespace(_node);
    }
    else if (type == "enum_specifier")
    {
        return parseEnum(_node);
    }
    // might be a function, an operator or a variable
    else if (type == "declaration" || type == "field_declaration")
    {
        return parseAmbiguousDeclaration(_node);
    }
    // might be a function or an operator
    else if (type == "function_definition")
    {
        return parseAmbiguousDefinition(_node);
    }
    else if (type == "concept_definition")
    {
        return parseConcept(_node);
    }
    // These also need to be parsed inside parseAmbiguousDeclaration for attributed types as they
    // will be wrapped in a "declaration" node
    else if (type == "union_specifier")
    {
        return parseUnion(_node);
    }
    else if (type == "struct_specifier")
    {
        return parseStruct(_node);
    }
    else if (type == "class_specifier")
    {
        return parseClass(_node);
    }

    return nullptr;
}

std::shared_ptr<Node> Parser::parseAmbiguousDeclaration(const TSNode& _node)
{
    NodeKind kind = NodeKind::Variable;
    const uint32_t childCount = ts_node_child_count(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        // Check if this is a direct function declarator
        if (childType == "function_declarator")
        {
            kind = determineKind(child);
        }
        // Check if function declarator is nested within pointer/reference/array declarator
        else if (isDeclaratorWrapper(childType))
        {
            if (tryFindNestedFunctionDeclarator(child, kind))
            {
                // kind has been updated by reference
            }
        }
        // Handle attributed declarations (struct/class/union with attributes)
        else if (childType == "attribute_declaration")
        {
            auto [attributedNode, consumed] = parseAttributedTypeDeclaration(child);
            if (attributedNode) return attributedNode;
            i += consumed - 1; // Skip consumed siblings (loop will increment i)
        }
    }

    switch (kind)
    {
        case NodeKind::Function:
            return parseFunction(_node);
        case NodeKind::Operator:
            return parseOperator(_node);
        case NodeKind::Variable:
            return parseVariable(_node);
        default:
            return std::make_shared<Node>((NodeKind)0, 0, 0, 0, 0);
    }
}

// Helper: Search for nested function declarator and update kind if found
bool Parser::tryFindNestedFunctionDeclarator(const TSNode& declaratorNode, NodeKind& outKind)
{
    const uint32_t subChildCount = ts_node_child_count(declaratorNode);
    for (uint32_t j = 0; j < subChildCount; ++j)
    {
        TSNode subChild = ts_node_child(declaratorNode, j);
        if (ts_node_type(subChild) == std::string("function_declarator"))
        {
            outKind = determineKind(subChild);
            return true;
        }
    }
    return false;
}

// Helper: Parse attributed type declarations (struct/class/union with attributes)
// Returns the parsed node and the number of consumed siblings (attributes + type specifier)
std::pair<std::shared_ptr<Node>, uint32_t> Parser::parseAttributedTypeDeclaration(
    TSNode attributeNode)
{
    // Collect all consecutive attribute declarations
    std::vector<std::string> attributes;
    TSNode currNode = attributeNode;
    std::string currNodeType = ts_node_type(currNode);
    uint32_t consumed = 0;

    while (currNodeType == "attribute_declaration")
    {
        attributes.push_back(getNodeText(currNode, m_source->content));
        currNode = ts_node_next_sibling(currNode);
        currNodeType = ts_node_type(currNode);
        ++consumed;
    }

    // Parse the type declaration and attach attributes
    if (currNodeType == "union_specifier")
    {
        std::shared_ptr<UnionNode> unionNode = parseUnion(currNode);
        unionNode->attributes = std::move(attributes);
        return {unionNode, consumed + 1};
    }
    else if (currNodeType == "struct_specifier")
    {
        std::shared_ptr<StructNode> structNode = parseStruct(currNode);
        structNode->attributes = std::move(attributes);
        return {structNode, consumed + 1};
    }
    else if (currNodeType == "class_specifier")
    {
        std::shared_ptr<ClassNode> classNode = parseClass(currNode);
        classNode->attributes = std::move(attributes);
        return {classNode, consumed + 1};
    }

    return {nullptr, consumed};
}

std::shared_ptr<Node> Parser::parseAmbiguousDefinition(const TSNode& _node)
{
    NodeKind kind = NodeKind::Function;

    uint32_t childCount = ts_node_child_count(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "function_declarator")
        {
            kind = determineKind(child);
        }
        else if (childType == "reference_declarator" || childType == "pointer_declarator" ||
                 childType == "array_declarator")
        {
            uint32_t subChildCount = ts_node_child_count(child);

            for (uint32_t j = 0; j < subChildCount; ++j)
            {
                TSNode subChild = ts_node_child(child, j);
                std::string subChildType = ts_node_type(subChild);

                if (subChildType == "function_declarator")
                {
                    kind = determineKind(subChild);
                    break;
                }
            }
        }
    }

    switch (kind)
    {
        case NodeKind::Operator:
            return parseOperator(_node);
        default:
            return parseFunction(_node);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Preprocessor & Global Scopes
/////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<IncludeNode> Parser::parseInclude(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    uint32_t childCount = ts_node_child_count(_node);

    auto incNode = std::make_shared<IncludeNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    incNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);
        std::string text = getNodeText(child, m_source->content);

        if (type == "system_lib_string")
        {
            incNode->path = text.substr(1, text.size() - 2); // remove < >
            incNode->isSystem = true;
        }
        else if (type == "string_literal")
        {
            incNode->path = text.substr(1, text.size() - 2); // remove " "
            incNode->isSystem = false;
        }
    }

    return incNode;
}

std::shared_ptr<ObjectLikeMacroNode> Parser::parseObjectLikeMacro(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto objMacroNode =
        std::make_shared<ObjectLikeMacroNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    objMacroNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "identifier" && objMacroNode->name.empty())
        {
            objMacroNode->name = getNodeText(child, m_source->content);
        }
        else if (childType == "preproc_arg")
        {
            objMacroNode->body = getNodeText(child, m_source->content);
        }
    }

    return objMacroNode;
}

std::shared_ptr<FunctionLikeMacroNode> Parser::parseFunctionLikeMacro(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto fnMacroNode =
        std::make_shared<FunctionLikeMacroNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    fnMacroNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "identifier" && fnMacroNode->name.empty())
        {
            fnMacroNode->name = getNodeText(child, m_source->content);
        }
        else if (childType == "preproc_params")
        {
            const uint32_t paramCount = ts_node_child_count(child);

            for (uint32_t j = 0; j < paramCount; ++j)
            {
                TSNode param = ts_node_child(child, j);
                std::string paramType = ts_node_type(param);

                MacroParameter mp;

                if (paramType == "identifier")
                {
                    mp.name = getNodeText(param, m_source->content);
                }
                else if (paramType == "...")
                {
                    mp.name = "...";
                    mp.isVariadic = true;
                }

                if (!mp.name.empty()) fnMacroNode->parameters.emplace_back(mp);
            }
        }
        else if (childType == "preproc_arg")
        {
            fnMacroNode->body = getNodeText(child, m_source->content);
        }
    }

    return fnMacroNode;
}

std::shared_ptr<NamespaceNode> Parser::parseNamespace(const TSNode& _node)
{
    auto collectNamespaceParts = [&](const TSNode& node, auto&& self,
                                     std::vector<std::string>& out) -> void
    {
        const uint32_t cnt = ts_node_child_count(node);

        for (uint32_t i = 0; i < cnt; ++i)
        {
            TSNode child = ts_node_child(node, i);
            std::string type = ts_node_type(child);

            if (type == "namespace_identifier")
            {
                out.emplace_back(getNodeText(child, m_source->content));
            }
            else if (type == "nested_namespace_specifier")
            {
                self(child, self, out);
            }
        }
    };

    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto nsNode = std::make_shared<NamespaceNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    nsNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "comment")
        {
            m_leadingComment = parseComment(_node);
        }
        else if (type == "namespace_identifier")
        {
            // e.g. "mynamespace"
            nsNode->name = getNodeText(child, m_source->content);
            nsNode->isAnonymous = false;
        }
        else if (type == "nested_namespace_specifier")
        {
            // e.g. "outer::inner"
            nsNode->isNested = true;
            nsNode->isAnonymous = false;

            std::vector<std::string> parts;
            collectNamespaceParts(child, collectNamespaceParts, parts);
            nsNode->name = fmt::format("{}", fmt::join(parts, "::"));
        }
    }

    return nsNode;
}

std::shared_ptr<NamespaceAliasNode> Parser::parseNamespaceAlias(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto nsAliasNode =
        std::make_shared<NamespaceAliasNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    nsAliasNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "namespace_identifier")
        {
            if (nsAliasNode->aliasName.empty())
            {
                nsAliasNode->aliasName = getNodeText(child, m_source->content); // first is alias
            }
            else
            {
                nsAliasNode->targetNamespace =
                    getNodeText(child, m_source->content); // second is target
            }
        }
    }

    return nsAliasNode;
}

std::shared_ptr<UsingNamespaceNode> Parser::parseUsingNamespace(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto usingNsNode =
        std::make_shared<UsingNamespaceNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    usingNsNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "namespace" || childType == "identifier")
        {
            usingNsNode->name = getNodeText(child, m_source->content);
        }
    }

    return usingNsNode;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Type & Data Structures
/////////////////////////////////////////////////////////////////////////////////////////////

TypeSignature Parser::parseTypeSignature(const TSNode& _node)
{
    TypeSignature sig;

    auto fullText = getNodeText(_node, m_source->content);

    const uint32_t childCount = ts_node_child_count(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);
        std::string childText = getNodeText(child, m_source->content);

        if (childType == "primitive_type" || childType == "type_identifier")
        {
            sig.baseType += childText;
        }
        else if (childType == "qualified_identifier")
        {
            const uint32_t subChildCount = ts_node_child_count(child);

            sig.baseType += childText;
        }
        else if (childType == "type_qualifier")
        {
            std::string txt = childText;
            if (txt == "const") sig.isConst = true;
            if (txt == "volatile") sig.isVolatile = true;
            if (txt == "mutable") sig.isMutable = true;
        }
        else if (childType == "pointer_declarator" ||
                 (childType == "init_declarator" && childText[0] == '*'))
        {
            sig.isPointer = true;
        }
        else if (childType == "reference_declarator" ||
                 (childType == "init_declarator" && childText[0] == '&'))
        {
            std::string txt = childText;

            if (txt.find("&&") != std::string::npos)
            {
                sig.isRValueRef = true;
            }
            else
            {
                sig.isLValueRef = true;
            }
        }
        else if (childType == "template_type")
        {
            sig.baseType =
                getNodeText(ts_node_child(child, 0), m_source->content); // the identifier

            if (ts_node_child_count(child) > 1)
            {
                TSNode argList = ts_node_child(child, 1);

                if (std::string(ts_node_type(argList)) == "template_argument_list")
                {
                    sig.templateArgs = parseTemplateArgumentsList(argList);
                }
            }
        }
    }

    return sig;
}

std::shared_ptr<EnumNode> Parser::parseEnum(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto enumNode = std::make_shared<EnumNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    enumNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "type_identifier")
        {
            enumNode->name = getNodeText(child, m_source->content);
        }
        else if (type == "class")
        {
            enumNode->isScoped = true;
        }
        else if (type == "primitive_type")
        {
            enumNode->underlyingType = getNodeText(child, m_source->content);
        }
        else if (type == "enumerator_list")
        {
            const uint32_t enumCount = ts_node_child_count(child);

            for (uint32_t j = 0; j < enumCount; ++j)
            {
                TSNode eChild = ts_node_child(child, j);
                std::string eType = ts_node_type(eChild);

                if (eType == "comment")
                {
                    m_leadingComment = parseComment(_node);
                }
                if (eType == "enumerator")
                {
                    std::string enumName;
                    std::string enumValue;

                    auto [s, e] = getPositionData(eChild);
                    const uint32_t parts = ts_node_child_count(eChild);

                    for (uint32_t k = 0; k < parts; ++k)
                    {
                        TSNode part = ts_node_child(eChild, k);
                        std::string pType = ts_node_type(part);

                        if (pType == "identifier")
                        {
                            // e.g. "VALUE1"
                            enumName = getNodeText(part, m_source->content);
                        }
                        else if (pType == "number_literal" || pType == "string_literal")
                        {
                            // e.g. "42" or "\"value\""
                            enumValue = getNodeText(part, m_source->content);
                        }
                    }

                    auto enumSpecNode =
                        std::make_shared<EnumSpecifierNode>(s.row, s.column, e.row, e.column);

                    enumSpecNode->name = enumName;
                    enumSpecNode->value = enumValue;

                    enumNode->enumerators.emplace_back(enumSpecNode);
                }
            }
        }
    }

    return enumNode;
}

std::shared_ptr<ConceptNode> Parser::parseConcept(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto conceptNode = std::make_shared<ConceptNode>(start.row, start.column, end.row, end.column);

    conceptNode->comment = getLeadingComment();
    conceptNode->templateDecl = getTemplateDeclaration();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "identifier")
        {
            // e.g. "Integral"
            conceptNode->name = getNodeText(child, m_source->content);
        }
        else if (childType == "qualified_identifier" || childType == "identifier" ||
                 childType == "template_function")
        {
            // Entire constraint expression just grab text
            conceptNode->constraint = getNodeText(child, m_source->content);
        }
    }

    return conceptNode;
}

std::shared_ptr<TypedefNode> Parser::parseTypedef(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    uint32_t childCount = ts_node_child_count(_node);

    auto typedefNode = std::make_shared<TypedefNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    typedefNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "primitive_type")
        {
            // e.g. "int"
            typedefNode->targetType = getNodeText(child, m_source->content);
        }
        else if (type == "struct_specifier" || type == "class_specifier" ||
                 type == "union_specifier")
        {
            // e.g. "struct MyStruct { ... }"
            typedefNode->targetType = getNodeText(child, m_source->content);
        }
        else if (type == "type_identifier")
        {
            // e.g. "MyType"
            typedefNode->aliasName = getNodeText(child, m_source->content);
        }
    }

    return typedefNode;
}

std::shared_ptr<TypeAliasNode> Parser::parseTypeAlias(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto typeAliasNode =
        std::make_shared<TypeAliasNode>(start.row, start.column, end.row, end.column);

    typeAliasNode->templateDecl = getTemplateDeclaration();
    typeAliasNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "type_identifier")
        {
            typeAliasNode->aliasName = getNodeText(child, m_source->content);
        }
        else if (childType == "type_descriptor" || childType == "primitive_type" ||
                 childType == "identifier")
        {
            typeAliasNode->targetType = getNodeText(child, m_source->content);
        }
    }

    return typeAliasNode;
}

std::shared_ptr<UnionNode> Parser::parseUnion(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto unionNode = std::make_shared<UnionNode>(start.row, start.column, end.row, end.column);

    unionNode->comment = getLeadingComment();
    unionNode->templateDecl = getTemplateDeclaration();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "attribute_declaration")
        {
            unionNode->attributes.emplace_back(getNodeText(child, m_source->content));
        }
        else if (childType == "type_identifier")
        {
            unionNode->name = getNodeText(child, m_source->content);
        }
        else if (childType == "template_type")
        {
            uint16_t subChildCount = ts_node_child_count(child);

            if (subChildCount == 0) continue;

            unionNode->name = getNodeText(ts_node_child(child, 0), m_source->content);

            for (uint16_t s = 1; s < subChildCount; ++s)
            {
                TSNode sub = ts_node_child(child, s);
                if (std::string(ts_node_type(sub)) == "template_argument_list")
                {
                    unionNode->templateArgs = parseTemplateArgumentsList(sub);
                }
            }
        }
    }

    unionNode->isAnonymous = unionNode->name.empty();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "field_declaration_list")
        {
            std::vector<std::shared_ptr<Node>> dummyFriends;

            parseMemberList(child, unionNode->name, unionNode->memberVariables,
                            unionNode->staticMemberVariables, unionNode->memberFunctions,
                            unionNode->staticMemberFunctions, unionNode->constructors,
                            unionNode->destructors, unionNode->operators, dummyFriends,
                            unionNode->nestedTypes);
        }
    }

    return unionNode;
}

std::shared_ptr<StructNode> Parser::parseStruct(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto structNode = std::make_shared<StructNode>(start.row, start.column, end.row, end.column);

    structNode->comment = getLeadingComment();
    structNode->templateDecl = getTemplateDeclaration();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "attribute_declaration")
        {
            structNode->attributes.emplace_back(getNodeText(child, m_source->content));
        }
        else if (childType == "type_identifier")
        {
            structNode->name = getNodeText(child, m_source->content);
        }
        else if (childType == "template_type")
        {
            uint16_t subChildCount = ts_node_child_count(child);

            if (subChildCount == 0) continue;

            structNode->name = getNodeText(ts_node_child(child, 0), m_source->content);

            for (uint16_t s = 1; s < subChildCount; ++s)
            {
                TSNode sub = ts_node_child(child, s);
                if (std::string(ts_node_type(sub)) == "template_argument_list")
                {
                    structNode->templateArgs = parseTemplateArgumentsList(sub);
                }
            }
        }
    }

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "base_class_clause")
        {
            structNode->baseClasses = parseBaseClassesList(child, AccessSpecifier::Public);
        }
        else if (childType == "virtual_specifier")
        {
            structNode->isFinal = getNodeText(child, m_source->content) == "final";
        }
        else if (childType == "field_declaration_list")
        {
            parseMemberList(child, structNode->name, structNode->memberVariables,
                            structNode->staticMemberVariables, structNode->memberFunctions,
                            structNode->staticMemberFunctions, structNode->constructors,
                            structNode->destructors, structNode->operators, structNode->friends,
                            structNode->nestedTypes);
        }
    }

    return structNode;
}

std::shared_ptr<ClassNode> Parser::parseClass(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    auto classNode = std::make_shared<ClassNode>(start.row, start.column, end.row, end.column);

    const uint32_t childCount = ts_node_child_count(_node);

    classNode->comment = getLeadingComment();
    classNode->templateDecl = getTemplateDeclaration();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "attribute_declaration")
        {
            classNode->attributes.emplace_back(getNodeText(child, m_source->content));
        }
        else if (childType == "type_identifier")
        {
            classNode->name = getNodeText(child, m_source->content);
        }
        else if (childType == "template_type")
        {
            uint16_t subChildCount = ts_node_child_count(child);

            if (subChildCount == 0) continue;

            classNode->name = getNodeText(ts_node_child(child, 0), m_source->content);

            for (uint16_t s = 1; s < subChildCount; ++s)
            {
                TSNode sub = ts_node_child(child, s);
                if (std::string(ts_node_type(sub)) == "template_argument_list")
                {
                    classNode->templateArgs = parseTemplateArgumentsList(sub);
                }
            }
        }
        else if (childType == "base_class_clause")
        {
            classNode->baseClasses = parseBaseClassesList(child, AccessSpecifier::Private);
        }
        else if (childType == "virtual_specifier")
        {
            classNode->isFinal = getNodeText(child, m_source->content) == "final";
        }
        else if (childType == "field_declaration_list")
        {
            parseClassMemberList(child, classNode->name, classNode->memberVariables,
                                 classNode->staticMemberVariables, classNode->memberFunctions,
                                 classNode->staticMemberFunctions, classNode->constructors,
                                 classNode->destructors, classNode->operators, classNode->friends,
                                 classNode->nestedTypes);
        }
    }

    return classNode;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Executables & Declarations
/////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<VariableNode> Parser::parseVariable(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto varNode = std::make_shared<VariableNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    varNode->comment = getLeadingComment();
    varNode->typeSignature = parseTypeSignature(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "attribute_declaration")
        {
            varNode->attributes.emplace_back(getNodeText(child, m_source->content));
        }
        else if (childType == "identifier" || childType == "field_identifier")
        {
            varNode->name = getNodeText(child, m_source->content);
        }
        else if (childType == "type_qualifier")
        {
            std::string q = getNodeText(child, m_source->content);

            if (q == "constexpr")
                varNode->isConstexpr = true;
            else if (q == "constinit")
                varNode->isConstinit = true;
        }
        else if (childType == "storage_class_specifier")
        {
            std::string s = getNodeText(child, m_source->content);

            if (s == "static")
                varNode->isStatic = true;
            else if (s == "extern")
                varNode->isExtern = true;
            else if (s == "thread_local")
                varNode->isThreadLocal = true;
            else if (s == "inline")
                varNode->isInline = true;
            else if (s == "mutable")
                varNode->typeSignature.isMutable = true;
        }
        else if (childType == "init_declarator")
        {
            parseInitDeclarator(child, varNode);
        }
    }

    return varNode;
}

std::shared_ptr<FunctionNode> Parser::parseFunction(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto fnNode = std::make_shared<FunctionNode>(start.row, start.column, end.row, end.column);

    fnNode->comment = getLeadingComment();
    fnNode->templateDecl = getTemplateDeclaration();
    fnNode->returnSignature = parseTypeSignature(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "attribute_declaration")
        {
            fnNode->attributes.emplace_back(getNodeText(child, m_source->content));
        }
        else if (type == "storage_class_specifier")
        {
            std::string txt = getNodeText(child, m_source->content);
            if (txt == "static") fnNode->isStatic = true;
            if (txt == "inline") fnNode->isInline = true;
        }
        else if (type == "type_qualifier")
        {
            std::string txt = getNodeText(child, m_source->content);
            if (txt == "constexpr") fnNode->isConstexpr = true;
            if (txt == "consteval") fnNode->isConsteval = true;
            if (txt == "const") fnNode->isConst = true;
            if (txt == "volatile") fnNode->isVolatile = true;
        }
        else if (type == "virtual")
        {
            fnNode->isVirtual = true;
        }
        else
        {
            if (type == "reference_declarator" || type == "pointer_declarator")
            {
                const uint32_t subChildCount = ts_node_child_count(child);

                for (uint32_t j = 0; j < subChildCount; ++j)
                {
                    TSNode subChild = ts_node_child(child, j);
                    std::string subType = ts_node_type(subChild);

                    if (subType != "function_declarator") continue;

                    parseFunctionDeclarator(subChild, fnNode);
                }
            }
            else if (type == "function_declarator")
            {
                parseFunctionDeclarator(child, fnNode);
            }
        }
    }

    return fnNode;
}

std::vector<FunctionParameter> Parser::parseFunctionParametersList(const TSNode& _node)
{
    std::vector<FunctionParameter> result;

    const uint32_t childCount = ts_node_child_count(_node);

    result.reserve(childCount);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType != std::string("parameter_declaration") &&
            childType != std::string("optional_parameter_declaration"))
        {
            continue;
        }

        FunctionParameter gp;
        gp.typeSignature = parseTypeSignature(child);

        const uint32_t subCount = ts_node_child_count(child);

        for (uint32_t j = 0; j < subCount; ++j)
        {
            TSNode subChild = ts_node_child(child, j);
            std::string subType = ts_node_type(subChild);

            if (subType == "reference_declarator" || subType == "pointer_declarator")
            {
                const uint32_t subChildCount = ts_node_child_count(subChild);

                for (uint32_t j = 0; j < subChildCount; ++j)
                {
                    TSNode subSubChild = ts_node_child(subChild, j);
                    std::string subSubType = ts_node_type(subSubChild);

                    if (subSubType == "identifier")
                    {
                        gp.name = getNodeText(subSubChild, m_source->content);
                    }
                    else if (subSubType == "number_literal" || subSubType == "string_literal")
                    {
                        gp.defaultValue = getNodeText(subSubChild, m_source->content);
                    }
                }
            }
            else if (subType == std::string("identifier"))
            {
                gp.name = getNodeText(subChild, m_source->content);
            }
            else if (subType == std::string("=") || subType == std::string("default_value"))
            {
                TSNode nextSub = ts_node_child(child, j + 1);
                gp.defaultValue = getNodeText(nextSub, m_source->content);
                ++j; // Skip consumed sibling
            }
        }

        result.emplace_back(std::move(gp));
    }

    return result;
}

std::shared_ptr<OperatorNode> Parser::parseOperator(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto opNode = std::make_shared<OperatorNode>(start.row, start.column, end.row, end.column);

    opNode->comment = getLeadingComment();
    opNode->templateDecl = getTemplateDeclaration();
    opNode->returnSignature = parseTypeSignature(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "attribute_declaration")
        {
            opNode->attributes.emplace_back(getNodeText(child, m_source->content));
        }
        else if (type == "function_declarator")
        {
            parseOperatorDeclarator(child, opNode);
        }
        else if (type == "storage_class_specifier")
        {
            std::string txt = getNodeText(child, m_source->content);
            if (txt == "static") opNode->isStatic = true;
            if (txt == "inline") opNode->isInline = true;
        }
        else if (type == "type_qualifier")
        {
            std::string text = getNodeText(child, m_source->content);
            if (text == "const") opNode->isConst = true;
            if (text == "constexpr") opNode->isConstexpr = true;
            if (text == "explicit") opNode->isExplicit = true;
        }
    }

    return opNode;
}

std::shared_ptr<Node> Parser::parseFriend(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto friendNode = std::make_shared<FriendNode>(start.row, start.column, end.row, end.column);

    clearTemplateDeclaration();
    friendNode->comment = getLeadingComment();

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "struct" || childType == "class")
        {
            friendNode->kind = getNodeText(child, m_source->content);
        }
        else if (childType == "type_identifier" || childType == "qualified_identifier")
        {
            friendNode->name = getNodeText(child, m_source->content);
        }
    }

    return friendNode;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Member List Handlers
/////////////////////////////////////////////////////////////////////////////////////////////

void Parser::parseMemberList(const TSNode& _listNode, const std::string& _parentName,
                             std::vector<std::shared_ptr<Node>>& _memberVars,
                             std::vector<std::shared_ptr<Node>>& _staticMemberVars,
                             std::vector<std::shared_ptr<Node>>& _memberFuncs,
                             std::vector<std::shared_ptr<Node>>& _staticMemberFuncs,
                             std::vector<std::shared_ptr<Node>>& _ctors,
                             std::vector<std::shared_ptr<Node>>& _dtors,
                             std::vector<std::shared_ptr<Node>>& _ops,
                             std::vector<std::shared_ptr<Node>>& _friends,
                             std::vector<std::shared_ptr<Node>>& _nestedTypes)
{
    const uint32_t fieldCount = ts_node_child_count(_listNode);

    for (uint32_t j = 0; j < fieldCount; ++j)
    {
        TSNode child = ts_node_child(_listNode, j);
        std::string childType = ts_node_type(child);

        if (childType == "comment")
        {
            m_leadingComment = parseComment(child);
        }
        else if (childType == "template_declaration")
        {
            m_templateDeclaration = parseTemplateDeclaration(child);
        }
        if (childType == "field_declaration" || childType == "declaration")
        {
            const uint32_t childCount = ts_node_child_count(child);

            TSNode firstSubChild = ts_node_child(child, 0);
            std::string firstSubChildType = ts_node_type(firstSubChild);

            if (firstSubChildType == "struct_specifier" || firstSubChildType == "class_specifier" ||
                firstSubChildType == "union_specifier" || firstSubChildType == "enum_specifier")
            {
                auto nested = dispatch(firstSubChild);
                _nestedTypes.emplace_back(nested);
            }
            else
            {
                auto declNode = parseAmbiguousDeclaration(child);

                if (declNode->kind == NodeKind::Variable)
                {
                    auto varNode = std::dynamic_pointer_cast<VariableNode>(declNode);
                    (varNode->isStatic ? _staticMemberVars : _memberVars).emplace_back(declNode);
                }
                else if (declNode->kind == NodeKind::Function)
                {
                    auto funcNode = std::dynamic_pointer_cast<FunctionNode>(declNode);

                    if (funcNode->name == _parentName) // constructor
                    {
                        _ctors.emplace_back(toConstructorNode(funcNode, _parentName));
                    }
                    else if (!funcNode->name.empty() && funcNode->name[0] == '~') // destructor
                    {
                        _dtors.emplace_back(toDestructorNode(funcNode));
                    }
                    else
                    {
                        (funcNode->isStatic ? _staticMemberFuncs : _memberFuncs)
                            .emplace_back(funcNode);
                    }
                }
                else
                {
                    _ops.emplace_back(declNode);
                }
            }
        }
        else if (childType == "function_definition")
        {
            auto defNode = parseAmbiguousDefinition(child);

            if (defNode->kind == NodeKind::Function)
            {
                auto funcNode = std::dynamic_pointer_cast<FunctionNode>(defNode);

                if (funcNode->name == _parentName) // constructor
                {
                    _ctors.emplace_back(toConstructorNode(funcNode, _parentName));
                }
                else if (!funcNode->name.empty() && funcNode->name[0] == '~') // destructor
                {
                    _dtors.emplace_back(toDestructorNode(funcNode));
                }
                else
                {
                    (funcNode->isStatic ? _staticMemberFuncs : _memberFuncs).emplace_back(funcNode);
                }
            }
            else
            {
                _ops.emplace_back(defNode);
            }
        }
        else if (childType == "friend_declaration")
        {
            _friends.emplace_back(parseFriend(child));
        }
    }
}

void Parser::parseClassMemberList(
    const TSNode& _listNode, const std::string& _parentName,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _memberVars,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _staticMemberVars,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _memberFuncs,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _staticMemberFuncs,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _ctors,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _dtors,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _ops,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _friends,
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& _nestedTypes)
{
    const uint32_t fieldCount = ts_node_child_count(_listNode);

    AccessSpecifier currentAccess = AccessSpecifier::Private;

    for (uint32_t j = 0; j < fieldCount; ++j)
    {
        TSNode child = ts_node_child(_listNode, j);
        std::string childType = ts_node_type(child);

        if (childType == "comment")
        {
            m_leadingComment = parseComment(child);
        }
        else if (childType == "template_declaration")
        {
            m_templateDeclaration = parseTemplateDeclaration(child);
        }
        else if (childType == "access_specifier")
        {
            std::string txt = getNodeText(child, m_source->content);

            if (txt == "public")
                currentAccess = AccessSpecifier::Public;
            else if (txt == "protected")
                currentAccess = AccessSpecifier::Protected;
            else if (txt == "private")
                currentAccess = AccessSpecifier::Private;
        }
        else if (childType == "field_declaration" || childType == "declaration")
        {
            const uint32_t childCount = ts_node_child_count(child);

            TSNode firstSubChild = ts_node_child(child, 0);
            std::string firstSubChildType = ts_node_type(firstSubChild);

            if (firstSubChildType == "struct_specifier" || firstSubChildType == "class_specifier" ||
                firstSubChildType == "union_specifier" || firstSubChildType == "enum_specifier")
            {
                auto nested = dispatch(firstSubChild);
                _nestedTypes.emplace_back(std::make_pair(currentAccess, nested));
            }
            else
            {
                auto declNode = parseAmbiguousDeclaration(child);

                if (declNode->kind == NodeKind::Variable)
                {
                    auto varNode = std::dynamic_pointer_cast<VariableNode>(declNode);
                    (varNode->isStatic ? _staticMemberVars : _memberVars)
                        .emplace_back(std::make_pair(currentAccess, declNode));
                }
                else if (declNode->kind == NodeKind::Function)
                {
                    auto funcNode = std::dynamic_pointer_cast<FunctionNode>(declNode);

                    if (funcNode->name == _parentName) // constructor
                    {
                        _ctors.emplace_back(std::make_pair(
                            currentAccess, toConstructorNode(funcNode, _parentName)));
                    }
                    else if (!funcNode->name.empty() && funcNode->name[0] == '~') // destructor
                    {
                        _dtors.emplace_back(
                            std::make_pair(currentAccess, toDestructorNode(funcNode)));
                    }
                    else
                    {
                        (funcNode->isStatic ? _staticMemberFuncs : _memberFuncs)
                            .emplace_back(std::make_pair(currentAccess, funcNode));
                    }
                }
                else
                {
                    _ops.emplace_back(std::make_pair(currentAccess, declNode));
                }
            }
        }
        else if (childType == "function_definition")
        {
            auto defNode = parseAmbiguousDefinition(child);

            if (defNode->kind == NodeKind::Function)
            {
                auto funcNode = std::dynamic_pointer_cast<FunctionNode>(defNode);

                if (funcNode->name == _parentName) // constructor
                {
                    _ctors.emplace_back(
                        std::make_pair(currentAccess, toConstructorNode(funcNode, _parentName)));
                }
                else if (!funcNode->name.empty() && funcNode->name[0] == '~') // destructor
                {
                    _dtors.emplace_back(std::make_pair(currentAccess, toDestructorNode(funcNode)));
                }
                else
                {
                    (funcNode->isStatic ? _staticMemberFuncs : _memberFuncs)
                        .emplace_back(std::make_pair(currentAccess, funcNode));
                }
            }
            else
            {
                _ops.emplace_back(std::make_pair(currentAccess, defNode));
            }
        }
        else if (childType == "friend_declaration")
        {
            _friends.emplace_back(std::make_pair(currentAccess, parseFriend(child)));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Specialized Declarators
/////////////////////////////////////////////////////////////////////////////////////////////

void Parser::parseInitDeclarator(const TSNode& _node, std::shared_ptr<VariableNode>& _varNode)
{
    const uint32_t childCount = ts_node_child_count(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "identifier" || type == "field_identifier")
        {
            _varNode->name = getNodeText(child, m_source->content);
        }
        else if (type == "number_literal" || type == "string_literal")
        {
            _varNode->defaultValue = getNodeText(child, m_source->content);
        }
        else if (type == "reference_declarator" || type == "pointer_declarator")
        {
            const uint32_t subChildCount = ts_node_child_count(child);

            for (uint32_t j = 0; j < subChildCount; ++j)
            {
                TSNode subChild = ts_node_child(child, j);
                std::string subType = ts_node_type(subChild);

                if (subType == "identifier")
                {
                    _varNode->name = getNodeText(subChild, m_source->content);
                }
                else if (subType == "number_literal" || subType == "string_literal")
                {
                    _varNode->defaultValue = getNodeText(subChild, m_source->content);
                }
            }
        }
    }
}

void Parser::parseFunctionDeclarator(const TSNode& _node, std::shared_ptr<FunctionNode>& _fn)
{
    const uint32_t childCount = ts_node_child_count(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "identifier" || type == "field_identifier" || type == "destructor_name")
        {
            _fn->name = getNodeText(child, m_source->content);
        }
        else if (type == "template_function" || type == "template_method")
        {
            if (ts_node_child_count(child) > 1)
            {
                TSNode argList = ts_node_child(child, 1);

                if (std::string(ts_node_type(argList)) == "template_argument_list")
                {
                    _fn->templateArgs = parseTemplateArgumentsList(argList);
                }
            }
        }
        else if (type == "parameter_list")
        {
            _fn->parameters = parseFunctionParametersList(child);
        }
        else if (type == "noexcept")
        {
            _fn->isNoexcept = true;
        }
        else if (type == "override")
        {
            _fn->isOverride = true;
        }
        else if (type == "final")
        {
            _fn->isFinal = true;
        }
        else if (type == "const")
        {
            _fn->isConst = true;
        }
    }
}

void Parser::parseOperatorDeclarator(const TSNode& _node, std::shared_ptr<OperatorNode>& _op)
{
    const uint32_t childCount = ts_node_child_count(_node);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string type = ts_node_type(child);

        if (type == "operator_name")
        {
            _op->operatorSymbol =
                getNodeText(child, m_source->content).substr(8); // remove "operator"
        }
        else if (type == "parameter_list")
        {
            _op->parameters = parseFunctionParametersList(child);
        }
        else if (type == "template_function" || type == "template_method")
        {
            if (ts_node_child_count(child) > 1)
            {
                TSNode argList = ts_node_child(child, 1);

                if (std::string(ts_node_type(argList)) == "template_argument_list")
                {
                    _op->templateArgs = parseTemplateArgumentsList(argList);
                }
            }
        }
        else if (type == "virtual")
        {
            _op->isVirtual = true;
        }
        else if (type == "override")
        {
            _op->isOverride = true;
        }
        else if (type == "final")
        {
            _op->isFinal = true;
        }
        else if (type == "pure_virtual_specifier")
        {
            _op->isPureVirtual = true;
        }
        else if (type == "noexcept")
        {
            _op->isNoexcept = true;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Template & Type Metadata Helpers
/////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<TemplateNode> Parser::parseTemplateDeclaration(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);
    const uint32_t childCount = ts_node_child_count(_node);

    auto templateDeclNode =
        std::make_shared<TemplateNode>(start.row, start.column, end.row, end.column);

    for (uint32_t i = 0; i < childCount; ++i)
    {
        TSNode child = ts_node_child(_node, i);
        std::string childType = ts_node_type(child);

        if (childType == "template_parameter_list")
        {
            templateDeclNode->parameters = parseTemplateParameterList(child);
        }
    }

    return templateDeclNode;
}

std::vector<TemplateParameter> Parser::parseTemplateParameterList(const TSNode& _node)
{
    std::vector<TemplateParameter> parameters;

    const uint32_t paramCount = ts_node_child_count(_node);

    for (uint32_t j = 0; j < paramCount; ++j)
    {
        TSNode param = ts_node_child(_node, j);
        std::string paramType = ts_node_type(param);

        TemplateParameter tp;

        if (paramType == "type_parameter_declaration")
        {
            // e.g. "typename T"
            tp.paramKind = TemplateParameterKind::Type;

            for (uint32_t k = 0; k < ts_node_child_count(param); ++k)
            {
                TSNode sub = ts_node_child(param, k);
                std::string subType = ts_node_type(sub);

                if (subType == "typename" || subType == "class")
                {
                    tp.keyword = getNodeText(sub, m_source->content);
                }
                else if (subType == "type_identifier")
                {
                    tp.name = getNodeText(sub, m_source->content);
                }
            }
        }
        else if (paramType == "variadic_type_parameter_declaration")
        {
            tp.paramKind = TemplateParameterKind::Type;
            tp.isVariadic = true;

            for (uint32_t k = 0; k < ts_node_child_count(param); ++k)
            {
                TSNode sub = ts_node_child(param, k);
                std::string subType = ts_node_type(sub);

                if (subType == "class" || subType == "typename")
                {
                    tp.keyword = getNodeText(sub, m_source->content);
                }
                else if (subType == "type_identifier")
                {
                    tp.name = getNodeText(sub, m_source->content);
                }
            }
        }
        else if (paramType == "optional_type_parameter_declaration")
        {
            // e.g. "typename T = int"
            tp.paramKind = TemplateParameterKind::Type;

            for (uint32_t k = 0; k < ts_node_child_count(param); ++k)
            {
                TSNode sub = ts_node_child(param, k);
                std::string subType = ts_node_type(sub);

                if (subType == "typename" || subType == "class")
                {
                    tp.keyword = getNodeText(sub, m_source->content);
                }
                else if (subType == "type_identifier")
                {
                    if (tp.name.empty() && tp.defaultValue.empty())
                        tp.name = getNodeText(sub, m_source->content);
                    else
                        tp.defaultValue = getNodeText(sub, m_source->content);
                }
                else if (subType == "type_descriptor" || subType == "template_type" ||
                         subType == "primitive_type" || subType == "qualified_identifier")
                {
                    tp.defaultValue = getNodeText(sub, m_source->content);
                }
            }
        }
        else if (paramType == "parameter_declaration")
        {
            // Non-type param: e.g. "int N" or "auto V"
            tp.paramKind = TemplateParameterKind::NonType;
            tp.typeSignature = parseTypeSignature(param);

            for (uint32_t k = 0; k < ts_node_child_count(param); ++k)
            {
                TSNode sub = ts_node_child(param, k);
                std::string subType = ts_node_type(sub);

                if (subType == "identifier")
                {
                    tp.name = getNodeText(sub, m_source->content);
                }
                else if (subType == "variadic_declarator")
                {
                    tp.isVariadic = true;
                    // The name is inside the variadic_declarator
                    for (uint32_t m = 0; m < ts_node_child_count(sub); ++m)
                    {
                        TSNode vsub = ts_node_child(sub, m);
                        if (std::string(ts_node_type(vsub)) == "identifier")
                        {
                            tp.name = getNodeText(vsub, m_source->content);
                        }
                    }
                }
            }
        }
        else if (paramType == "optional_parameter_declaration")
        {
            // Non-type with default: e.g. "int N = 42"
            tp.paramKind = TemplateParameterKind::NonType;
            tp.typeSignature = parseTypeSignature(param);

            for (uint32_t k = 0; k < ts_node_child_count(param); ++k)
            {
                TSNode sub = ts_node_child(param, k);
                std::string subType = ts_node_type(sub);

                if (subType == "identifier")
                {
                    tp.name = getNodeText(sub, m_source->content);
                }
                else if (subType == "number_literal" || subType == "string_literal" ||
                         subType == "char_literal" || subType == "boolean_literal" ||
                         subType == "floating_point_literal" || subType == "template_type" ||
                         subType == "type_descriptor" || subType == "qualified_identifier")
                {
                    tp.defaultValue = getNodeText(sub, m_source->content);
                }
            }
        }
        else if (paramType == "template_template_parameter_declaration")
        {
            // e.g. "template<typename> class C"
            tp.paramKind = TemplateParameterKind::TemplateTemplate;

            for (uint32_t k = 0; k < ts_node_child_count(param); ++k)
            {
                TSNode sub = ts_node_child(param, k);
                std::string subType = ts_node_type(sub);

                if (subType == "template_parameter_list")
                {
                    tp.innerParameters = parseTemplateParameterList(sub);
                }
                else if (subType == "type_parameter_declaration")
                {
                    // Contains "class C" or "typename C"
                    for (uint32_t m = 0; m < ts_node_child_count(sub); ++m)
                    {
                        TSNode inner = ts_node_child(sub, m);
                        std::string innerType = ts_node_type(inner);

                        if (innerType == "typename" || innerType == "class")
                        {
                            tp.keyword = getNodeText(inner, m_source->content);
                        }
                        else if (innerType == "type_identifier")
                        {
                            tp.name = getNodeText(inner, m_source->content);
                        }
                    }
                }
            }
        }
        else
        {
            continue;
        }

        if (!tp.keyword.empty() || !tp.name.empty() || !tp.typeSignature.baseType.empty() ||
            tp.paramKind == TemplateParameterKind::TemplateTemplate)
        {
            parameters.emplace_back(std::move(tp));
        }
    }

    return parameters;
}

std::vector<TemplateArgument> Parser::parseTemplateArgumentsList(const TSNode& node)
{
    std::vector<TemplateArgument> args;

    const uint32_t count = ts_node_child_count(node);

    args.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        TSNode child = ts_node_child(node, i);

        if (!ts_node_is_named(child)) continue;

        std::string type = ts_node_type(child);

        TemplateArgument arg;

        if (type == "type_descriptor" || type == "primitive_type" || type == "type_identifier")
        {
            arg.typeSignature = parseTypeSignature(child);
        }
        else if (type == "number_literal" || type == "string_literal" || type == "true" ||
                 type == "false" || type == "identifier" || type == "qualified_identifier")
        {
            arg.value = getNodeText(child, m_source->content);
        }
        else
        {
            // fallback: raw text
            arg.value = getNodeText(child, m_source->content);
        }

        args.emplace_back(std::move(arg));
    }

    return args;
}

std::vector<std::pair<AccessSpecifier, std::string>> Parser::parseBaseClassesList(
    const TSNode& _node, AccessSpecifier _defaultAccess)
{
    std::vector<std::pair<AccessSpecifier, std::string>> baseClasses = {};
    std::pair<AccessSpecifier, std::string> currentAccess = {AccessSpecifier::Private, ""};

    for (uint32_t j = 0; j < ts_node_child_count(_node); ++j)
    {
        TSNode baseChild = ts_node_child(_node, j);
        std::string type = ts_node_type(baseChild);
        std::string text = getNodeText(baseChild, m_source->content);

        if (type == "access_specifier")
        {
            if (text == "public")
                currentAccess.first = AccessSpecifier::Public;
            else if (text == "protected")
                currentAccess.first = AccessSpecifier::Protected;
            else if (text == "private")
                currentAccess.first = AccessSpecifier::Private;
        }
        else if (type == std::string("qualified_identifier") ||
                 type == std::string("type_identifier"))
        {
            currentAccess.second = text;
        }

        if (!currentAccess.second.empty())
        {
            baseClasses.emplace_back(currentAccess);
            currentAccess = {_defaultAccess, ""};
        }
    }

    return baseClasses;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// "Sticky" State Management (Floating Nodes)
/////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<CommentNode> Parser::parseComment(const TSNode& _node)
{
    auto [start, end] = getPositionData(_node);

    auto text = m_source->content.substr(ts_node_start_byte(_node),
                                         ts_node_end_byte(_node) - ts_node_start_byte(_node));

    return std::make_shared<CommentNode>(text, start.row, start.column, end.row, end.column);
}

std::shared_ptr<CommentNode> Parser::getLeadingComment()
{
    if (!m_leadingComment) return nullptr;

    auto copy = m_leadingComment;

    m_leadingComment = nullptr;

    return copy;
}

std::shared_ptr<TemplateNode> Parser::getTemplateDeclaration()
{
    if (!m_templateDeclaration) return nullptr;

    auto copy = m_templateDeclaration;

    m_templateDeclaration = nullptr;

    return copy;
}

} // namespace codex
