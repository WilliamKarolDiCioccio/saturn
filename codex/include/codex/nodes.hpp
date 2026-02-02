#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <filesystem>

#include "source.hpp"

namespace codex
{

// NOTE: the naming of these kinds doesn't respect our C++ naming conventions, a needed evil to
// avoid confusion with C++ keywords
enum struct NodeKind
{
    Source,
    Comment,
    Template,
    IncludeDirective,
    FunctionLikeMacro,
    ObjectLikeMacro,
    Namespace,
    NamespaceAlias,
    UsingNamespace,
    Typedef,
    TypeAlias,
    Enum,
    EnumSpecifier,
    Variable,
    Concept,
    Function,
    Constructor,
    Destructor,
    CopyConstructor,
    MoveConstructor,
    Operator,
    Union,
    Friend,
    Struct,
    Class,
};

inline std::string nodeKindToString(NodeKind kind)
{
    switch (kind)
    {
        case NodeKind::Source:
            return "Source";
        case NodeKind::IncludeDirective:
            return "IncludeDirective";
        case NodeKind::Namespace:
            return "Namespace";
        case NodeKind::Enum:
            return "Enumerator";
        case NodeKind::EnumSpecifier:
            return "EnumeratorSpecifier";
        case NodeKind::Struct:
            return "Struct";
        case NodeKind::Class:
            return "Class";
        case NodeKind::Constructor:
            return "Constructor";
        case NodeKind::Destructor:
            return "Destructor";
        case NodeKind::Function:
            return "Function";
        case NodeKind::Variable:
            return "Variable";
        case NodeKind::Comment:
            return "Comment";
        case NodeKind::TypeAlias:
            return "TypeAlias";
        case NodeKind::NamespaceAlias:
            return "NamespaceAlias";
        case NodeKind::Typedef:
            return "Typedef";
        case NodeKind::Union:
            return "Union";
        case NodeKind::Concept:
            return "Concept";
        case NodeKind::Template:
            return "Template";
        case NodeKind::Operator:
            return "Operator";
        case NodeKind::UsingNamespace:
            return "UsingNamespace";
        case NodeKind::FunctionLikeMacro:
            return "FunctionLikeMacro";
        case NodeKind::ObjectLikeMacro:
            return "ObjectLikeMacro";
        case NodeKind::CopyConstructor:
            return "CopyConstructor";
        case NodeKind::MoveConstructor:
            return "MoveConstructor";
        case NodeKind::Friend:
            return "Friend";
        default:
            return "Unknown";
    }
}

enum struct AccessSpecifier
{
    Public,
    Protected,
    Private,
    None
};

inline std::string accessSpecifierToString(AccessSpecifier access)
{
    switch (access)
    {
        case AccessSpecifier::Public:
            return "public";
        case AccessSpecifier::Protected:
            return "protected";
        case AccessSpecifier::Private:
            return "private";
        case AccessSpecifier::None:
            return "none";
        default:
            return "unknown";
    }
}

struct TemplateArgument;

struct TypeSignature
{
    std::string baseType;

    bool isConst = false;
    bool isVolatile = false;
    bool isMutable = false;
    bool isPointer = false;
    bool isLValueRef = false;
    bool isRValueRef = false;

    std::vector<TemplateArgument> templateArgs = {};

    inline std::string toString() const;
};

enum struct TemplateParameterKind
{
    Type,
    NonType,
    TemplateTemplate
};

struct TemplateParameter
{
    TemplateParameterKind paramKind = TemplateParameterKind::Type;

    // Common
    std::string name;
    bool isVariadic = false;
    std::string defaultValue;

    // Type params (keyword = "typename"/"class")
    std::string keyword;
    std::string constraint;

    // NonType params
    TypeSignature typeSignature;

    // TemplateTemplate params -- recursive
    std::vector<TemplateParameter> innerParameters;

    std::string toString() const
    {
        std::string result;

        switch (paramKind)
        {
            case TemplateParameterKind::TemplateTemplate:
            {
                result = "template<";
                for (size_t i = 0; i < innerParameters.size(); ++i)
                {
                    if (i > 0) result += ", ";
                    result += innerParameters[i].toString();
                }
                result += "> ";
                result += keyword.empty() ? "class" : keyword;
                if (!name.empty()) result += " " + name;
                break;
            }
            case TemplateParameterKind::NonType:
            {
                result = typeSignature.toString();
                if (isVariadic) result += "...";
                if (!name.empty()) result += " " + name;
                break;
            }
            case TemplateParameterKind::Type:
            default:
            {
                if (!constraint.empty())
                    result = constraint;
                else
                    result = keyword;
                if (isVariadic) result += "...";
                if (!name.empty()) result += " " + name;
                break;
            }
        }

        if (!defaultValue.empty()) result += " = " + defaultValue;

        return result;
    }
};

struct TemplateArgument
{
    std::string keyword;
    TypeSignature typeSignature;
    std::string value;

    std::string toString() const
    {
        if (!keyword.empty()) return keyword;
        if (!typeSignature.baseType.empty()) return typeSignature.toString();
        return value;
    }
};

inline std::string TypeSignature::toString() const
{
    std::string result = baseType;

    if (isConst) result = "const " + result;
    if (isVolatile) result = "volatile " + result;
    if (isMutable) result = "mutable " + result;
    if (isPointer) result += "*";
    if (isLValueRef) result += "&";
    if (isRValueRef) result += "&&";

    if (!templateArgs.empty())
    {
        result += "<";
        for (size_t i = 0; i < templateArgs.size(); ++i)
        {
            if (i > 0) result += ", ";
            result += templateArgs[i].toString();
        }
        result += ">";
    }

    return result;
}

struct GenericParameter
{
    TypeSignature typeSignature;
    std::string name;
    std::string defaultValue;

    std::string toString() const
    {
        std::string result = typeSignature.toString();
        if (!name.empty()) result += " " + name;
        if (!defaultValue.empty()) result += " = " + defaultValue;
        return result;
    }
};

struct Node
{
    NodeKind kind;
    uint32_t startLine, startColumn;
    uint32_t endLine, endColumn;
    std::shared_ptr<Node> comment = nullptr;

    explicit Node(NodeKind _kind, uint32_t _startLine, uint32_t _startColumn, uint32_t _endLine,
                  uint32_t _endColumn, std::shared_ptr<Node> _comment = nullptr)
        : kind(_kind),
          startLine(_startLine),
          startColumn(_startColumn),
          endLine(_endLine),
          endColumn(_endColumn),
          comment(std::move(_comment)) {};

    virtual ~Node() = default;

    virtual std::string toString(int _depth = 0) const
    {
        std::string indent(static_cast<size_t>(_depth * 2), ' ');
        std::string result = indent + nodeKindToString(kind);

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }

   protected:
    std::string getIndent(int depth) const
    {
        return std::string(static_cast<size_t>(depth * 2), ' ');
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Dependent nodes (requiring to be linked to other nodes and leaving outisde the main tree)
/////////////////////////////////////////////////////////////////////////////////////////////

struct CommentNode final : Node
{
    std::string text;

    explicit CommentNode(std::string _text, int _startLine, int _startColumn, int _endLine,
                         int _endColumn)
        : Node(NodeKind::Comment, _startLine, _startColumn, _endLine, _endColumn),
          text(std::move(_text)) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string displayText = text;

        if (displayText.length() > 50)
        {
            displayText = displayText.substr(0, 47) + "...";
        }

        size_t pos = 0;
        while ((pos = displayText.find('\n', pos)) != std::string::npos)
        {
            displayText.replace(pos, 1, "\\n");
            pos += 2;
        }

        return indent + "Comment(\"" + displayText + "\" " + std::to_string(startLine) + ":" +
               std::to_string(startColumn) + "-" + std::to_string(endLine) + ":" +
               std::to_string(endColumn) + ")";
    }
};

struct TemplateNode final : Node
{
    std::vector<TemplateParameter> parameters;

    explicit TemplateNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Template, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(";

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (i > 0) result += ", ";
            result += parameters[i].toString();
        }
        result += ")";

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Independent nodes
/////////////////////////////////////////////////////////////////////////////////////////////

struct SourceNode final : Node
{
    std::shared_ptr<Source> source;
    std::vector<std::shared_ptr<Node>> children;

    explicit SourceNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Source, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + source->name + "\")";
        result += "\n" + indent + "  Path: " + source->path.string();
        result += "\n" + indent + "  Encoding: " + source->encoding;
        result += "\n" + indent + "  Last Modified: " + std::to_string(source->lastModifiedTime);
        result += "\n" + indent + "  Source Length: " + std::to_string(source->content.length());

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        for (const auto& child : children)
        {
            result += "\n" + child->toString(_depth + 1);
        }

        return result;
    }
};

struct IncludeNode final : Node
{
    std::string path;
    bool isSystem;

    explicit IncludeNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::IncludeDirective, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + path + "\")";
        result += " [" + std::string(isSystem ? "system" : "local") + "]";

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct ObjectLikeMacroNode final : Node
{
    std::string name;
    std::string body;

    explicit ObjectLikeMacroNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::ObjectLikeMacro, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";
        if (!body.empty())
        {
            result += "\n" + indent + "  Body: \"" + body + "\"";
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct MacroParameter
{
    std::string name;
    bool isVariadic = false;

    std::string toString() const
    {
        std::string result = name;
        if (isVariadic) result += "...";
        return result;
    }
};

struct FunctionLikeMacroNode final : Node
{
    std::string name;
    std::string body;
    std::vector<MacroParameter> parameters;

    explicit FunctionLikeMacroNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::FunctionLikeMacro, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        result += "(";
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (i > 0) result += ", ";
            result += parameters[i].toString();
        }
        result += ")";

        if (!body.empty())
        {
            result += "\n" + indent + "  Body: \"" + body + "\"";
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct NamespaceNode final : Node
{
    std::string name;
    bool isAnonymous;
    bool isNested;
    std::vector<std::shared_ptr<Node>> children;

    explicit NamespaceNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Namespace, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        std::vector<std::string> flags;
        if (isAnonymous) flags.push_back("anonymous");
        if (isNested) flags.push_back("nested");

        if (!flags.empty())
        {
            result += " [";
            for (size_t i = 0; i < flags.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += flags[i];
            }
            result += "]";
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        for (const auto& child : children)
        {
            result += "\n" + child->toString(_depth + 1);
        }

        return result;
    }
};

struct UsingNamespaceNode final : Node
{
    std::string name;

    explicit UsingNamespaceNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::UsingNamespace, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct NamespaceAliasNode final : Node
{
    std::string aliasName;
    std::string targetNamespace;

    explicit NamespaceAliasNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::NamespaceAlias, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + aliasName + "\" -> \"" +
                             targetNamespace + "\")";

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct TypedefNode final : Node
{
    std::string aliasName;
    std::string targetType;

    explicit TypedefNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Typedef, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result =
            indent + nodeKindToString(kind) + "(\"" + aliasName + "\" -> \"" + targetType + "\")";

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct TypeAliasNode final : Node
{
    std::string aliasName;
    std::string targetType;
    std::shared_ptr<Node> templateDecl;

    explicit TypeAliasNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::TypeAlias, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result =
            indent + nodeKindToString(kind) + "(\"" + aliasName + "\" -> \"" + targetType + "\")";

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct EnumNode final : Node
{
    std::string name;
    std::string underlyingType;
    bool isScoped = false;
    std::vector<std::shared_ptr<Node>> enumerators;

    explicit EnumNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::EnumSpecifier, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        if (!underlyingType.empty())
        {
            result += " : " + underlyingType;
        }

        if (isScoped)
        {
            result += " [scoped]";
        }

        if (!enumerators.empty())
        {
            result += "\n" + indent + "  Enumerators:";
            for (const auto& enumerator : enumerators)
            {
                result += "\n" + enumerator->toString(_depth + 2);
            }
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct EnumSpecifierNode final : Node
{
    std::string name;
    std::string value;

    explicit EnumSpecifierNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Enum, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        if (!value.empty())
        {
            result += " = " + value;
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct VariableNode final : Node
{
    bool isStatic = false;
    bool isConstexpr = false;
    bool isThreadLocal = false;
    bool isInline = false;
    bool isExtern = false;
    bool isConstinit = false;

    TypeSignature typeSignature;
    std::string name;
    std::string initialValue;

    explicit VariableNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Variable, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        std::vector<std::string> modifiers;
        if (isStatic) modifiers.push_back("static");
        if (isConstexpr) modifiers.push_back("constexpr");
        if (isThreadLocal) modifiers.push_back("thread_local");
        if (isInline) modifiers.push_back("inline");
        if (isExtern) modifiers.push_back("extern");
        if (isConstinit) modifiers.push_back("constinit");

        if (!modifiers.empty())
        {
            result += " [";
            for (size_t i = 0; i < modifiers.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += modifiers[i];
            }
            result += "]";
        }

        result += "\n" + indent + "  Type: " + typeSignature.toString();

        if (!initialValue.empty())
        {
            result += "\n" + indent + "  Initial Value: " + initialValue;
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct ConceptNode final : Node
{
    std::string name;
    std::string constraint;
    std::shared_ptr<Node> templateDecl;

    explicit ConceptNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Concept, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        if (!constraint.empty())
        {
            result += "\n" + indent + "  Constraint: " + constraint;
        }

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct FunctionNode final : Node
{
    bool isStatic = false;
    bool isConst = false;
    bool isVolatile = false;
    bool isVirtual = false;
    bool isPureVirtual = false;
    bool isOverride = false;
    bool isNoexcept = false;
    bool isFinal = false;
    bool isInline = false;
    bool isConstexpr = false;
    bool isConsteval = false;
    bool isExplicit = false;

    std::vector<std::string> attributes;
    TypeSignature returnSignature;
    std::string name;
    std::vector<GenericParameter> parameters;
    std::shared_ptr<Node> templateDecl;
    std::vector<TemplateArgument> templateArgs;

    explicit FunctionNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Function, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        std::vector<std::string> modifiers;
        if (isStatic) modifiers.push_back("static");
        if (isConst) modifiers.push_back("const");
        if (isVolatile) modifiers.push_back("volatile");
        if (isVirtual) modifiers.push_back("virtual");
        if (isPureVirtual) modifiers.push_back("pure virtual");
        if (isOverride) modifiers.push_back("override");
        if (isNoexcept) modifiers.push_back("noexcept");
        if (isFinal) modifiers.push_back("final");
        if (isInline) modifiers.push_back("inline");
        if (isConstexpr) modifiers.push_back("constexpr");
        if (isConsteval) modifiers.push_back("consteval");
        if (isExplicit) modifiers.push_back("explicit");

        if (!modifiers.empty())
        {
            result += " [";
            for (size_t i = 0; i < modifiers.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += modifiers[i];
            }
            result += "]";
        }

        result += "\n" + indent + "  Return Type: " + returnSignature.toString();

        if (!parameters.empty())
        {
            result += "\n" + indent + "  Parameters:";
            for (const auto& param : parameters)
            {
                result += "\n" + indent + "    " + param.toString();
            }
        }

        if (!attributes.empty())
        {
            result += "\n" + indent + "  Attributes: ";
            for (size_t i = 0; i < attributes.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += attributes[i];
            }
        }

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (!templateArgs.empty())
        {
            result += "\n" + indent + "  Template Args: ";
            for (size_t i = 0; i < templateArgs.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += templateArgs[i].toString();
            }
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct ConstructorNode final : Node
{
    bool isExplicit = false;
    bool isNoexcept = false;
    bool isDefaulted = false;
    bool isDeleted = false;
    bool isConstexpr = false;
    bool isInline = false;
    bool isCopyConstructor = false;
    bool isMoveConstructor = false;

    std::string name;
    std::vector<GenericParameter> parameters;
    std::shared_ptr<Node> templateDecl;
    std::vector<TemplateArgument> templateArgs;

    explicit ConstructorNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Constructor, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        std::vector<std::string> modifiers;
        if (isExplicit) modifiers.push_back("explicit");
        if (isNoexcept) modifiers.push_back("noexcept");
        if (isDefaulted) modifiers.push_back("defaulted");
        if (isDeleted) modifiers.push_back("deleted");
        if (isConstexpr) modifiers.push_back("constexpr");
        if (isInline) modifiers.push_back("inline");
        if (isCopyConstructor) modifiers.push_back("copy");
        if (isMoveConstructor) modifiers.push_back("move");

        if (!modifiers.empty())
        {
            result += " [";
            for (size_t i = 0; i < modifiers.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += modifiers[i];
            }
            result += "]";
        }

        if (!parameters.empty())
        {
            result += "\n" + indent + "  Parameters:";
            for (const auto& param : parameters)
            {
                result += "\n" + indent + "    " + param.toString();
            }
        }

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (!templateArgs.empty())
        {
            result += "\n" + indent + "  Template Args: ";
            for (size_t i = 0; i < templateArgs.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += templateArgs[i].toString();
            }
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct DestructorNode final : Node
{
    bool isVirtual = false;
    bool isPureVirtual = false;
    bool isDefaulted = false;
    bool isDeleted = false;
    bool isNoexcept = false;
    bool isInline = false;
    bool isConstexpr = false;

    std::string name;

    explicit DestructorNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Destructor, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        std::vector<std::string> modifiers;
        if (isVirtual) modifiers.push_back("virtual");
        if (isPureVirtual) modifiers.push_back("pure virtual");
        if (isDefaulted) modifiers.push_back("defaulted");
        if (isDeleted) modifiers.push_back("deleted");
        if (isNoexcept) modifiers.push_back("noexcept");
        if (isInline) modifiers.push_back("inline");
        if (isConstexpr) modifiers.push_back("constexpr");

        if (!modifiers.empty())
        {
            result += " [";
            for (size_t i = 0; i < modifiers.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += modifiers[i];
            }
            result += "]";
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct OperatorNode final : Node
{
    bool isStatic = false;
    bool isConst = false;
    bool isVirtual = false;
    bool isPureVirtual = false;
    bool isOverride = false;
    bool isNoexcept = false;
    bool isFinal = false;
    bool isInline = false;
    bool isConstexpr = false;
    bool isExplicit = false;

    std::string operatorSymbol;
    TypeSignature returnSignature;
    std::vector<GenericParameter> parameters;
    std::shared_ptr<Node> templateDecl;
    std::vector<TemplateArgument> templateArgs;

    explicit OperatorNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Operator, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + operatorSymbol + "\")";

        std::vector<std::string> modifiers;
        if (isStatic) modifiers.push_back("static");
        if (isConst) modifiers.push_back("const");
        if (isVirtual) modifiers.push_back("virtual");
        if (isPureVirtual) modifiers.push_back("pure virtual");
        if (isOverride) modifiers.push_back("override");
        if (isNoexcept) modifiers.push_back("noexcept");
        if (isFinal) modifiers.push_back("final");
        if (isInline) modifiers.push_back("inline");
        if (isConstexpr) modifiers.push_back("constexpr");
        if (isExplicit) modifiers.push_back("explicit");

        if (!modifiers.empty())
        {
            result += " [";
            for (size_t i = 0; i < modifiers.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += modifiers[i];
            }
            result += "]";
        }

        result += "\n" + indent + "  Return Type: " + returnSignature.toString();

        if (!parameters.empty())
        {
            result += "\n" + indent + "  Parameters:";
            for (const auto& param : parameters)
            {
                result += "\n" + indent + "    " + param.toString();
            }
        }

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (!templateArgs.empty())
        {
            result += "\n" + indent + "  Template Args: ";
            for (size_t i = 0; i < templateArgs.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += templateArgs[i].toString();
            }
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct UnionNode final : Node
{
    bool isAnonymous = false;

    std::string name;
    std::shared_ptr<Node> templateDecl;
    std::vector<TemplateArgument> templateArgs;
    std::vector<std::shared_ptr<Node>> memberVariables;
    std::vector<std::shared_ptr<Node>> memberFunctions;
    std::vector<std::shared_ptr<Node>> staticMemberVariables;
    std::vector<std::shared_ptr<Node>> staticMemberFunctions;
    std::vector<std::shared_ptr<Node>> constructors;
    std::vector<std::shared_ptr<Node>> destructors;
    std::vector<std::shared_ptr<Node>> operators;
    std::vector<std::shared_ptr<Node>> nestedTypes;

    explicit UnionNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Union, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        if (isAnonymous)
        {
            result += " [anonymous]";
        }

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (!templateArgs.empty())
        {
            result += "\n" + indent + "  Template Args: ";
            for (size_t i = 0; i < templateArgs.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += templateArgs[i].toString();
            }
        }

        if (!memberVariables.empty())
        {
            result += "\n" + indent + "  Member Variables:";
            for (const auto& member : memberVariables)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!memberFunctions.empty())
        {
            result += "\n" + indent + "  Member Functions:";
            for (const auto& member : memberFunctions)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!staticMemberVariables.empty())
        {
            result += "\n" + indent + "  Static Member Variables:";
            for (const auto& member : staticMemberVariables)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!staticMemberFunctions.empty())
        {
            result += "\n" + indent + "  Static Member Functions:";
            for (const auto& member : staticMemberFunctions)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!constructors.empty())
        {
            result += "\n" + indent + "  Constructors:";
            for (const auto& ctor : constructors)
            {
                result += "\n" + ctor->toString(_depth + 2);
            }
        }

        if (!destructors.empty())
        {
            result += "\n" + indent + "  Destructors:";
            for (const auto& dtor : destructors)
            {
                result += "\n" + dtor->toString(_depth + 2);
            }
        }

        if (!operators.empty())
        {
            result += "\n" + indent + "  Operators:";
            for (const auto& op : operators)
            {
                result += "\n" + op->toString(_depth + 2);
            }
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct FriendNode final : Node
{
    std::string kind;
    std::string name;

    explicit FriendNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Friend, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + "Friend(" + kind + " \"" + name + "\")";

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct StructNode final : Node
{
    bool isFinal = false;

    std::string name;
    std::shared_ptr<Node> templateDecl;
    std::vector<TemplateArgument> templateArgs;
    std::vector<std::pair<AccessSpecifier, std::string>> baseClasses;
    std::vector<std::string> derivedClasses;
    std::vector<std::shared_ptr<Node>> memberVariables;
    std::vector<std::shared_ptr<Node>> memberFunctions;
    std::vector<std::shared_ptr<Node>> staticMemberVariables;
    std::vector<std::shared_ptr<Node>> staticMemberFunctions;
    std::vector<std::shared_ptr<Node>> constructors;
    std::vector<std::shared_ptr<Node>> destructors;
    std::vector<std::shared_ptr<Node>> operators;
    std::vector<std::shared_ptr<Node>> friends;
    std::vector<std::shared_ptr<Node>> nestedTypes;

    explicit StructNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Struct, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        if (isFinal)
        {
            result += " [final]";
        }

        if (!baseClasses.empty())
        {
            result += "\n" + indent + "  Base Classes: ";
            for (size_t i = 0; i < baseClasses.size(); ++i)
            {
                if (i > 0) result += ", ";
                result +=
                    accessSpecifierToString(baseClasses[i].first) + " " + baseClasses[i].second;
            }
        }

        if (!derivedClasses.empty())
        {
            result += "\n" + indent + "  Derived Classes: ";
            for (size_t i = 0; i < derivedClasses.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += derivedClasses[i];
            }
        }

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (!templateArgs.empty())
        {
            result += "\n" + indent + "  Template Args: ";
            for (size_t i = 0; i < templateArgs.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += templateArgs[i].toString();
            }
        }

        if (!memberVariables.empty())
        {
            result += "\n" + indent + "  Member Variables:";
            for (const auto& member : memberVariables)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!memberFunctions.empty())
        {
            result += "\n" + indent + "  Member Functions:";
            for (const auto& member : memberFunctions)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!staticMemberVariables.empty())
        {
            result += "\n" + indent + "  Static Member Variables:";
            for (const auto& member : staticMemberVariables)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!staticMemberFunctions.empty())
        {
            result += "\n" + indent + "  Static Member Functions:";
            for (const auto& member : staticMemberFunctions)
            {
                result += "\n" + member->toString(_depth + 2);
            }
        }

        if (!constructors.empty())
        {
            result += "\n" + indent + "  Constructors:";
            for (const auto& ctor : constructors)
            {
                result += "\n" + ctor->toString(_depth + 2);
            }
        }

        if (!destructors.empty())
        {
            result += "\n" + indent + "  Destructors:";
            for (const auto& dtor : destructors)
            {
                result += "\n" + dtor->toString(_depth + 2);
            }
        }

        if (!operators.empty())
        {
            result += "\n" + indent + "  Operators:";
            for (const auto& op : operators)
            {
                result += "\n" + op->toString(_depth + 2);
            }
        }

        if (!friends.empty())
        {
            result += "\n" + indent + "  Friends:";
            for (const auto& friend_node : friends)
            {
                result += "\n" + friend_node->toString(_depth + 2);
            }
        }

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

struct ClassNode final : Node
{
    bool isFinal = false;

    std::string name;
    std::shared_ptr<Node> templateDecl;
    std::vector<TemplateArgument> templateArgs;
    std::vector<std::pair<AccessSpecifier, std::string>> baseClasses;
    std::vector<std::string> derivedClasses;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> memberVariables;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> memberFunctions;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> staticMemberVariables;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> staticMemberFunctions;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> constructors;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> destructors;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> operators;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> friends;
    std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>> nestedTypes;

    explicit ClassNode(int _startLine, int _startColumn, int _endLine, int _endColumn)
        : Node(NodeKind::Class, _startLine, _startColumn, _endLine, _endColumn) {};

    std::string toString(int _depth = 0) const override
    {
        std::string indent = getIndent(_depth);
        std::string result = indent + nodeKindToString(kind) + "(\"" + name + "\")";

        if (isFinal)
        {
            result += " [final]";
        }

        if (!baseClasses.empty())
        {
            result += "\n" + indent + "  Base Classes: ";
            for (size_t i = 0; i < baseClasses.size(); ++i)
            {
                if (i > 0) result += ", ";
                result +=
                    accessSpecifierToString(baseClasses[i].first) + " " + baseClasses[i].second;
            }
        }

        if (!derivedClasses.empty())
        {
            result += "\n" + indent + "  Derived Classes: ";
            for (size_t i = 0; i < derivedClasses.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += derivedClasses[i];
            }
        }

        if (templateDecl)
        {
            result += "\n" + templateDecl->toString(_depth + 1);
        }

        if (!templateArgs.empty())
        {
            result += "\n" + indent + "  Template Args: ";
            for (size_t i = 0; i < templateArgs.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += templateArgs[i].toString();
            }
        }

        auto printAccessSpecifierGroup = [&](const std::string& groupName, const auto& members)
        {
            if (!members.empty())
            {
                result += "\n" + indent + "  " + groupName + ":";

                // Group by access specifier
                std::map<AccessSpecifier, std::vector<std::shared_ptr<Node>>> grouped;
                for (const auto& member : members)
                {
                    grouped[member.first].push_back(member.second);
                }

                for (const auto& [access, nodes] : grouped)
                {
                    std::string accessStr;
                    switch (access)
                    {
                        case AccessSpecifier::Public:
                            accessStr = "public";
                            break;
                        case AccessSpecifier::Protected:
                            accessStr = "protected";
                            break;
                        case AccessSpecifier::Private:
                            accessStr = "private";
                            break;
                    }

                    result += "\n" + indent + "    " + accessStr + ":";
                    for (const auto& node : nodes)
                    {
                        result += "\n" + node->toString(_depth + 3);
                    }
                }
            }
        };

        printAccessSpecifierGroup("Member Variables", memberVariables);
        printAccessSpecifierGroup("Member Functions", memberFunctions);
        printAccessSpecifierGroup("Static Member Variables", staticMemberVariables);
        printAccessSpecifierGroup("Static Member Functions", staticMemberFunctions);
        printAccessSpecifierGroup("Constructors", constructors);
        printAccessSpecifierGroup("Destructors", destructors);
        printAccessSpecifierGroup("Operators", operators);
        printAccessSpecifierGroup("Friends", friends);
        printAccessSpecifierGroup("Nested Types", nestedTypes);

        if (comment)
        {
            result += "\n" + comment->toString(_depth + 1);
        }

        return result;
    }
};

} // namespace codex
